/**
 * @file      main.c
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     2026 Orange County Maker Faire Badge Source Code
 * @version   1.0
 * @date      2026-05-06
 *
 * @copyright Copyright (C) 2026 Ed Weaver
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://gnu.org>.
 */

#include "main.h"

//============================================================================
//
//   Phase 14: Post Maker Faire Updates
//
//      I2C Address, LED Color, and NEC Address have been updated to match
//   the PCB Silkscreen.  The LED Color sets the I2C Address and NEC Address
//   as follows:
//     - Red - I2C Address 0x21 and NEC Address 0xFB11
//     - Violet - I2C Address 0x22 and NEC Address 0xFB12
//     - Blue - I2C Address 0x23 and NEC Address 0xFB13
//     - Green - I2C Address 0x24 and NEC Address 0xFB14
//     - Yellow - I2C Address 0x25 and NEC Address 0xFB15
//
//      NEC-OUT Output will transmit infrared pulsed encoded as an NEC Frame.
//   The Transmitions will occur as soon the SAO Client processes the
//   instructions for NEC Frame generation.  The instructions for NEC Frame
//   generation are as follows:
//     - Value in 0x00 - 0x02 - NEC Tramitter Instructions
//     - Values in 0x01 and 0x02 - NEC Frame Address
//     - Values in 0x03 - NEC Frame Command
//     - Values in 0x04 - Number of Frames to Send
//     - Values in 0x05 - Time between each Frame Transmission
//
//      First Byte of an SAO Write will switch the values of the SAO Read.
//   The first byte of the SAO Read will match the first byte of the last
//   SAO Write.  Subsequent bytes on the SAO Read will be generated as
//   follows:
//     - 0x00 - Badge Identification and Firmware Version
//            - Byte 0x01 and greater - Firmware ID and Version
//            - String is terminated with 0xFF
//     - 0x01 - NEC Receiver information
//            - Values in 0x01 and 0x02 - NEC Frame Address
//            - Value in 0x03 - NEC Frame Command
//            - Value in 0x04 - Age of the last Received NEC Frame
//     - 0x02 - NEC Tramitter information
//            - Values in 0x01 and 0x02 - NEC Frame Address
//            - Value in 0x03 - NEC Frame Command
//            - Value in 0x04 - Number of Frames left to Send
//            - Value in 0x05 - Time left before the next transmission
//     - All other Values - null values of 0xFF per I2C standard behaivior
//
//      All unlocked messages will geneated and display in order.  Once all
//   messages have displayed, a 10 minute cooldown is then set prior to
//   regenerating all messages.  The Button X will clear this cooldown.
//
//      On initialization, the state of Switch Y is checked.  If Switch Y
//   is ON, start badge in Game mode.  If Switch Y is OFF, start badge in
//   SAO mode.
//
//      Game mode will display all unlocked messages every 10 minutes.
//   Pressing Button X will display messages immediately.
//
//      SAO mode will blink the LED red every time a NEC Frame is
//   transmitted.
//
//      Add a delay between each message.
//
//============================================================================

#define FIRMWARE_ID "MFOC 2026 Badge V"
#define FIRMWARE_VERSION "1.11a"

typedef struct {
    enum led_colors start_color;
    enum led_colors end_color;
} badge_message_data_t;

typedef struct {
    enum led_colors start_color;
    enum led_colors end_color;
    uint8_t count;
} message_color_t;

//============================================================================
//
// Section: EEPROM Definition
//
//============================================================================

// Define 4 initial variables directly inside the EEPROM section
// The compiler assigns sequential addresses starting from 0x000 automatically
__attribute__((used, section(".eeprom")))
uint32_t EEMEM eeprom_message_flags = 0x00000000;

//============================================================================
//
// SAO Setup
//
//============================================================================

// The SAO Client byte limit of a single transaction
#define SAO_BUFFER_LIMIT                                                    24

// The SAO Client will abandon a transaction stuck in the start condition
#define SAO_START_CONDITION_TIMEOUT_USEC                                   500

//============================================================================

// Count of Timer 0 interrupts to debounce Switch Y
#define SWITCH_DEBOUNCE_COUNT                                                5

// Count of Timer 0 interrupts to debounce Switch X
#define BUTTON_DEBOUNCE_COUNT                                                5

// Maximum Age of the NEC Input Capture before it is considered stale
#define NEC_INPUT_CAPTURE_MAX_AGE                                          254

// Count of Timer 0 interrupts to start a new loop cycle for the Main Loop
#define LOOP_COUNT                                                          25

// Main Loop Cycles before repeating message (600 Seconds * 5 Cycles/Second)
#define TIME_DELAY_MESSAGES                                               3000

// Quantity of Hidden Messages
#define HIDDEN_MESSAGES                                                     22

#define MESSAGE_CHARACTERS    16

const char badge_messages[HIDDEN_MESSAGES][MESSAGE_CHARACTERS] PROGMEM = {
    "* ",
    "HELLO MAKER",
    "WELCOME MFOC",
    "OC MAKES",
    "KEEP MAKING",
    "BUILD MORE",
    "CREATE TOGETHER",
    "SHARE SKILLS",
    "TRY TEST FIX",
    "HACK THE BADGE",
    "MFOC CONNECTS",
    "PLUG IN SAO",
    "I2C READY",
    "MASTER CONTROL",
    "CONTROL ME",
    "SEND IR",
    "TV REMOTE MODE",
    "MADE IN THE OC",
    "SURFING MAKEY",
    "OLYMPICS 2028?",
    "MASTER?",
    "SAO READY"
};

#define NEC_INPUT_BUFFER_LIMIT                                              70

#define NEC_PIN_SNAPSHOT_BUFFER_LIMIT                                       36

//============================================================================

// Define states for the NEC Transmitter behavior
enum nec_transmission_states {
    BADGE_SAO_ACTIVE_NEC_IDLE,
    BADGE_NEC_TRIGGERED_VIA_SAO,
    BADGE_NEC_TRIGGERED_VIA_BUTTON,
    BADGE_NEC_TRIGGERED_VIA_IR,
    BADGE_SAO_DISABLED
};

// Define states for the I2C SAO Port behavior
enum sao_port_states {
    SAO_PORT_I2C_ACTIVE,
    SAO_PORT_I2C_DISABLED
};

//============================================================================
//
// Section: Strong functions for blocking preventing race conditions
//
//      This section contains the strong functions that will prevent time
//   critical from executing when another time critial function is in
//   process.
//
//============================================================================

void ws2812_block_until_safe() {

    // Wait until Time Critical Functions are complete.
    while (READ_FLAG__TIME_CRITICAL_ACTIVE) {
        ;
    }
}

void ir_nec_block_until_safe() {

    // Wait until Time Critical Functions are complete.
    while (READ_FLAG__TIME_CRITICAL_ACTIVE) {
        ;
    }
}

//============================================================================
//
// Section: Global Variables
//
//      This section contains the variables used by the Main Loop and
//   the Interrupt Service Routines.  The variables are declared as volatile
//   to ensure that the compiler will refresh each variable from memory each
//   time it is used.
//
//      This is required ensuring that the Main Loop and the Interrupt
//   Service Routines are using the same values for each variable.
//
//============================================================================

// Variables to hold the NEC code from IR Receiver
volatile uint8_t nec_input_command;
volatile uint16_t nec_input_address;
volatile uint8_t nec_input_capture_age;

// Variables to hold the NEC code to IR LED Transmitter
volatile uint16_t nec_transmit_address;
volatile uint8_t nec_transmit_command;
volatile uint8_t nec_transmit_repeats;
volatile uint8_t nec_transmit_rate;
volatile uint8_t nec_transmit_trigger;

// Variables to hold the pulses detected from the NEC Receiver
volatile uint8_t pulse_buffer[NEC_PIN_SNAPSHOT_BUFFER_LIMIT];

volatile uint8_t sao_device_address = 0;

// Loop counter incremented by Timer 0 interrupt
volatile uint8_t loop_counter = 0;

queue_t nec_input_queue;

//============================================================================


ISR(USI_START_vect) {
    uint8_t i2c_start_condition_active = 1U;
    uint8_t i2c_frame_started = 0U;
    uint32_t time_out_counter = 0U;

    // Set state machine to check to check for the SAO Device Address
    SET_FLAG__I2C_START_OF_FRAME

    // Wait for the start condition to finish
    while (i2c_start_condition_active) {

        if (time_out_counter > SAO_START_CONDITION_TIMEOUT_USEC) {
            i2c_start_condition_active = 0;
            CLEAR_FLAG__I2C_ACTIVE
        }
        else if (SAO_PIN_SDA_HIGH) {
            i2c_start_condition_active = 0;
            CLEAR_FLAG__I2C_ACTIVE
        }
        else if (SAO_PIN_SCL_LOW) {
            i2c_frame_started = 1;
            i2c_start_condition_active = 0;
            CLEAR_FLAG__I2C_ACTIVE
        }
        else {
            i2c_start_condition_active = 1;
            SET_FLAG__I2C_ACTIVE
        }

        time_out_counter = time_out_counter + 1;

        _delay_us(1);
    }

    // Clear all USI Flags, Overflow Counter reset to 16 Edge changes
    USI_CLEAR_ALL_ISR_FLAGS

    // Setup USI when actively processing I2C Frame
    if (i2c_frame_started == 1) {
        SAO_INTERRUPT_ON_BYTE_CAPTURE
    }
}

ISR(USI_OVF_vect) {

    static enum sao_i2c_states sao_state = SAO_CHECK_ADDRESS;
    static uint8_t sao_buffer_index = 0;
    static uint8_t sao_buffer[SAO_BUFFER_LIMIT] = { [0 ... SAO_BUFFER_LIMIT-1] = 0xFF };
    static uint8_t sao_output_buffer[SAO_BUFFER_LIMIT] = { [0 ... SAO_BUFFER_LIMIT-1] = 0xFF };

    if (READ_FLAG__I2C_START_OF_FRAME) {
        sao_state = SAO_CHECK_ADDRESS;
        CLEAR_FLAG__I2C_START_OF_FRAME
    }

    switch (sao_state) {
        case SAO_CHECK_ADDRESS:
            // Check if frame matches the 7-bit SAO Device Address
            if ((USIDR >> 1) == sao_device_address) {
                // Check if frame is a Read Transaction or Write Transaction
                if (USIDR & 0x01) {
                    // Read Transaction
                    sao_state = SAO_ACK_READ;
                } else {
                    // Write Transaction
                    sao_state = SAO_ACK_WRITE;
                }
                sao_buffer_index = 1;
                SAO_SEND_ACK
            } else {
                SAO_INTERRUPT_ON_START_CONDITION
                SAO_INITIALIZE_PINS
                sao_state = SAO_CHECK_ADDRESS;
                CLEAR_FLAG__I2C_ACTIVE
            }
            break;
        case SAO_ACK_WRITE:
            SAO_RECEIVE_DATA_IN_USIDR
            sao_state = SAO_WRITE_DATA;
            break;
        case SAO_WRITE_DATA:
            if (sao_buffer_index < SAO_BUFFER_LIMIT) {
                sao_buffer[sao_buffer_index - 1] = USIDR;
                sao_state = SAO_ACK_WRITE;
                SAO_SEND_ACK
                if (sao_buffer_index == 1) {
                    sao_output_buffer[0] = sao_buffer[0];
                } else if (sao_buffer_index == 6) {
                    if (sao_output_buffer[0] == 0x02U) {
                        nec_transmit_address = (uint16_t) ((sao_buffer[2] << 8) | sao_buffer[1]);
                        nec_transmit_command = sao_buffer[3];
                        nec_transmit_repeats = sao_buffer[4];
                        nec_transmit_rate = sao_buffer[5];
                        nec_transmit_trigger = sao_buffer[5];
                    }
                }
                sao_buffer_index++;
            } else {
                SAO_SEND_NACK
                sao_state = SAO_NACK_READ;
                CLEAR_FLAG__I2C_ACTIVE
            }
            break;
        case SAO_READ_DATA:
            if (sao_buffer_index <= SAO_BUFFER_LIMIT) {
                sao_state = SAO_ACK_READ;
                SAO_RECEIVE_ACK
            } else {
                SAO_SEND_NACK
                sao_state = SAO_NACK_READ;
                CLEAR_FLAG__I2C_ACTIVE
            }
            break;
        case SAO_ACK_READ:
            if (sao_buffer_index <= SAO_BUFFER_LIMIT) {
                USIDR = sao_output_buffer[sao_buffer_index - 1];
                switch(sao_output_buffer[0]) {
                    default:
                        sao_output_buffer[sao_buffer_index] = 0xFF;
                        break;
                    case 0x00U:
                        switch (sao_buffer_index) {
                            case 1:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[0];
                                break;
                            case 2:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[1];
                                break;
                            case 3:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[2];
                                break;
                            case 4:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[3];
                                break;
                            case 5:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[4];
                                break;
                            case 6:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[5];
                                break;
                            case 7:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[6];
                                break;
                            case 8:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[7];
                                break;
                            case 9:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[8];
                                break;
                            case 10:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[9];
                                break;
                            case 11:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[10];
                                break;
                            case 12:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[11];
                                break;
                            case 13:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[12];
                                break;
                            case 14:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[13];
                                break;
                            case 15:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[14];
                                break;
                            case 16:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[15];
                                break;
                            case 17:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_ID[16];
                                break;
                            case 18:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_VERSION[0];
                                break;
                            case 19:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_VERSION[1];
                                break;
                            case 20:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_VERSION[2];
                                break;
                            case 21:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_VERSION[3];
                                break;
                            case 22:
                                sao_output_buffer[sao_buffer_index] = (uint8_t) FIRMWARE_VERSION[4];
                                break;
                            default:
                                sao_output_buffer[sao_buffer_index] = 0xFF;
                                break;
                        }
                        break;
                    case 0x01U:
                        switch (sao_buffer_index) {
                            case 1:
                                sao_output_buffer[1] = (uint8_t) ((nec_input_address & 0xFF00U) >> 8);
                                break;
                            case 2:
                                sao_output_buffer[2] = (uint8_t) (nec_input_address & 0x00FFU);
                                break;
                            case 3:
                                sao_output_buffer[3] = nec_input_command;
                                break;
                            case 4:
                                sao_output_buffer[4] = nec_input_capture_age;
                                break;
                            default:
                                sao_output_buffer[sao_buffer_index] = 0xFF;
                                break;
                        }
                        break;
                    case 0x02U:
                        switch (sao_buffer_index) {
                            case 1:
                                sao_output_buffer[1] = (uint8_t) ((nec_transmit_address & 0xFF00U) >> 8);
                                break;
                            case 2:
                                sao_output_buffer[2] = (uint8_t) (nec_transmit_address & 0x00FFU);
                                break;
                            case 3:
                                sao_output_buffer[3] = nec_transmit_command;
                                break;
                            case 4:
                                sao_output_buffer[4] = nec_transmit_repeats;
                                break;
                            case 5:
                                sao_output_buffer[5] = nec_transmit_trigger;
                                break;
                            default:
                                sao_output_buffer[sao_buffer_index] = 0xFF;
                                break;
                        }
                        break;
                }
                sao_buffer_index++;
                sao_state = SAO_READ_DATA;
                SAO_SEND_DATA_IN_USIDR
            } else {
                sao_state = SAO_NACK_READ;
                SAO_SEND_NACK
                CLEAR_FLAG__I2C_ACTIVE
            }
            break;
        default:
            SAO_INTERRUPT_ON_START_CONDITION
            SAO_INITIALIZE_PINS
            sao_state = SAO_CHECK_ADDRESS;
            CLEAR_FLAG__I2C_ACTIVE
            break;
    }
    USISR |= (1 << USIOIF);

    SET_FLAG__SAO_STATE_TRIGGER
}

//============================================================================

/* Timer/Counter0 Compare Match A */
ISR(TIMER0_COMPA_vect) {
    static uint8_t switch_y_high_counter = 0;
    static uint8_t switch_y_low_counter = 0;
    static uint8_t button_x_high_counter = 0;
    static uint8_t button_x_low_counter = 0;

    if (loop_counter < 255U) {
        loop_counter++;
    }

    // Clear Switch Y relevant counter based on Switch Y GPIO state 
    if (PINB & (1 << SWITCH_Y_PIN)) {
        // Increment high counter until debounced
        if (switch_y_high_counter <= SWITCH_DEBOUNCE_COUNT) {
            switch_y_high_counter++;
        }
        // Clear low counter
        switch_y_low_counter = 0;
    } else {
        // Clear high counter
        switch_y_high_counter = 0;
        // Increment low counter until debounced
        if (switch_y_low_counter <= SWITCH_DEBOUNCE_COUNT) {
            switch_y_low_counter++;
        }
    }

    // Clear Button X relevant counter based on Button X GPIO state
    if (PINB & (1 << BUTTON_X_PIN)) {
        // Clear low counter
        button_x_low_counter = 0;
        // Increment high counter until debounced
        if (button_x_high_counter <= BUTTON_DEBOUNCE_COUNT) {
            button_x_high_counter++;
        }
    } else {
        // Increment low counter until debounced
        if (button_x_low_counter <= BUTTON_DEBOUNCE_COUNT) {
            button_x_low_counter++;
        }
        // Clear high counter
        button_x_high_counter = 0;
    }

    // Edge trigger of debounced switch change
    if (switch_y_high_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__SWITCH_Y_OFF;
    } else {
        CLEAR_FLAG__SWITCH_Y_OFF;
    }

    // Edge trigger of debounced switch change
    if (switch_y_low_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__SWITCH_Y_ON;
    } else {
        CLEAR_FLAG__SWITCH_Y_ON;
    }

    // Edge trigger of debounced button change
    if (button_x_high_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__BUTTON_X_RELEASED;
    } else {
        CLEAR_FLAG__BUTTON_X_RELEASED;
    }

    // Edge trigger of debounced button change
    if (button_x_low_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__BUTTON_X_PRESSED;
    } else {
        CLEAR_FLAG__BUTTON_X_PRESSED;
    }
}

/* Timer/Counter1 Compare Match A */
ISR(TIMER1_COMPA_vect) {

    static uint8_t nec_time_counter = 0;

    // Check if the NEC time counter needs to be reset NEC input ISR
    if (READ_FLAG__NEC_IN_RESTART_COUNT) {
        // Clear the NEC time counter restart flag
        CLEAR_FLAG__NEC_IN_RESTART_COUNT

        // Reset the NEC time counter
        nec_time_counter = 0;
    }

    // Update flags after reseting the nec time counter for the initial
    //    12.5 msec to 13.0 msec
    if (nec_time_counter <= 30) {

        // Update flags based on time since nec time counter reset
        if (nec_time_counter == 3) {
            // Flag time after 1.5 msec and before 2.0 msec
            SET_FLAG__NEC_IN_DETECT_1
        }
        else if (nec_time_counter == 16) {
            // Flag time after 8.0 msec and before 8.5 msec
            SET_FLAG__NEC_IN_HEADER_0
        }
        else if (nec_time_counter == 24) {
            // Flag time after 12.0 msec and before 12.5 msec
            SET_FLAG__NEC_IN_HEADER_1
        }
        else if (nec_time_counter == 30) {
            // Flag time after 15.0 msec
            SET_FLAG__NEC_IN_LIMIT_COUNT
        }

        // Increment the NEC time counter every interrupt (0.5 second interval)
        nec_time_counter++;
    }
}

//============================================================================

ISR(PCINT0_vect) {
    uint8_t nec_timing_flags;

    if (!(PINB & (1 << IR_RECEIVER_PIN))) {
        nec_timing_flags = NEC_INPUT_LOW_TIMING_FLAGS;

        // Reset the NEC Time Counter
        SET_FLAG__NEC_IN_RESTART_COUNT

        // Clear all NEC Timing Flags
        CLEAR_FLAG__NEC_IN_ALL_TIMING_FLAGS
    } else {
        nec_timing_flags = NEC_INPUT_HIGH_TIMING_FLAGS;
    }

    Queue_Inject(&nec_input_queue, nec_timing_flags);
}

//============================================================================

/* Initialize Timer0 */
void timer0_init() {
    // Set CTC Mode (WGM01=1, WGM00=0)
    TCCR0A = (1 << WGM01);

    // Set the compare value for 8ms
    // Processor Clock: 8MHz
    // Prescaler: 256
    // (8MHz / (256 * 125Hz)) - 1 = 249
    OCR0A = 249;

    // Enable the Compare Match A interrupt
    TIMSK |= (1 << OCIE0A);

    // Set Prescaler to 256 and start the timer
    // CS02 = 1, CS01 = 0, CS00 = 0
    TCCR0B = (1 << CS02);
}

/* Initialize Timer1 */
void timer1_init() {

    // Timer1 Clock Prescaler to 16 (CS13=0, CS12=1, CS11=0, CS10=1)
    // Timer1 clear the counter on compare match with OCR1A (CTC1=1)
    TCCR1 = (1 << CS12) | (1 << CS10) | (1 << CTC1);

    // Clear Timer1 counter
    TCNT1 = 0;

    // Set Compare Match value for 0.5 msec
    OCR1A = 249;

    // Enable the Compare Match A interrupt
    TIMSK |= (1 << OCIE1A);
}

//============================================================================

/* Initialize the ADC Input */
void adc_init() {
    // set ADC input pin as input
    DDRB &= ~(1 << ADC_INPUT_PIN);

    // Set ADC input pin, Left adjust ADC, and set Vcc to Vref
    ADMUX = (1 << ADLAR) | ADC_INPUT_CHANNEL;

    // Enable the ADC and set the prescaler to 64 for an ADC clock of 125 kHz
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint8_t adc_read() {
    // Start conversion
    ADCSRA |= (1 << ADSC);

    // Wait for conversion complete
    while (ADCSRA & (1 << ADSC));

    return ADCH; // Return 8-bit value
}

//============================================================================

/* Initialize the Switch Y Input */
void switch_y_init() {
    PORTB |= (1 << SWITCH_Y_PIN);
    DDRB &= ~(1 << SWITCH_Y_PIN);
    CLEAR_FLAG__SAO_STATE_TRIGGER
    SAO_STOP
    GIMSK |= (1 << PCIE);
    PCMSK |= (1 << SAO_CLOCK_DETECT_INTERRUPT);
}

/* Initialize the Button X Input */
void button_x_init() {
    PORTB |= (1 << BUTTON_X_PIN);
    DDRB &= ~(1 << BUTTON_X_PIN);
}

//============================================================================

void initialize(void) {
    //initialize Timer0 for 8ms interrupts
    timer0_init();

    //initialize Timer0 for 0.5ms interrupts
    timer1_init();

    // Initialize the ADC Input
    adc_init();

    // Initialize the IR Receiver pin
    receiver_nec_set_pin();

    // Initialize the WS2812 DI pin
    ws2812_set_di_pin();

    // Initialize the Switch Y Input
    switch_y_init();

    // Initialize the Button X Input
    button_x_init();

    CLEAR_FLAG__SAO_STATE_TRIGGER

    // Initialize the NEC Transmitter pin
    ir_nec_set_pin();

    // set processor to sleep mode idle to save power between interrupts
    set_sleep_mode(SLEEP_MODE_IDLE);

    SET_FLAG__NEC_IN_RESTART_COUNT

    // Enable global interrupts
    sei();
}

uint8_t count_bits(uint32_t value) {
    uint8_t count = 0U;
    while (value != 0U) {
        if (value & 0x00000001U) {
            count = count + 1U;
        }
        value = value >> 1;
    }
    if (count > (HIDDEN_MESSAGES - 1U)) {
        count = (HIDDEN_MESSAGES - 1U);
    }
    return count;
}

int main(void) {

    // 22 hidden messages
    badge_message_data_t badge_data[HIDDEN_MESSAGES] = {
        {COLOR_TEAM, COLOR_TEAM},
        {COLOR_TEAM, COLOR_RAINBOW_REVERSE},
        {COLOR_RED, COLOR_GREEN},
        {COLOR_GREEN, COLOR_BLUE},
        {COLOR_BLUE, COLOR_RED},
        {COLOR_YELLOW, COLOR_RAINBOW_FORWARD},
        {COLOR_RED, COLOR_GREEN},
        {COLOR_GREEN, COLOR_BLUE},
        {COLOR_BLUE, COLOR_RED},
        {COLOR_AQUA, COLOR_RED},
        {COLOR_RED, COLOR_GREEN},
        {COLOR_GREEN, COLOR_BLUE},
        {COLOR_BLUE, COLOR_RED},
        {COLOR_VIOLET, COLOR_GREEN},
        {COLOR_RED, COLOR_GREEN},
        {COLOR_GREEN, COLOR_BLUE},
        {COLOR_BLUE, COLOR_RED},
        {COLOR_VIOLET, COLOR_RAINBOW_FORWARD},
        {COLOR_RED, COLOR_GREEN},
        {COLOR_GREEN, COLOR_BLUE},
        {COLOR_BLUE, COLOR_RED},
        {COLOR_TEAM, COLOR_WHITE}
    };

    PGM_P badge_message_address;
    char badge_message_buffer[MESSAGE_CHARACTERS] = {0};

    // counter for badge message
    uint8_t counter;
    uint8_t nec_input_size;
    uint8_t badge_data_index;
    uint8_t messages_unlocked;
    uint16_t delay_message_count;
    uint32_t message_flags;
    uint8_t save_message_flags;

    // trigger for NEC Frame transmission
    uint8_t trigger_transmit_nec_frame;
    uint16_t transmit_nec_address;
    uint8_t transmit_nec_command;

    // Morse Code Parser Data
    morse_code_parser_t morse_engine;

    // LED Color of the Morse Coded Message
    message_color_t morse_led_state;

    // Buffer to hold encoded Morse Code Phrase
    char morse_phrase_buffer[MORSE_MAXIMUM_CHARACTERS];

    // Add State Machine to manage the NEC Transmitter
    enum nec_transmission_states current_nec_transmit_state = BADGE_SAO_DISABLED;
    enum nec_transmission_states next_nec_transmit_state = BADGE_SAO_DISABLED;

    // Monitor the SAO Port State (Active or Disabled)
    enum sao_port_states sao_port_status;

    // Add State Machine for WS2812 LED
    enum led_colors next_led_state = COLOR_WHITE;

    // Add RGB Color Values for WS2812 LED
    uint8_t led_value_red = 255U;
    uint8_t led_value_green = 255U;
    uint8_t led_value_blue = 255U;

    // Initialize the ADC Input
    uint8_t adc_value = 0U;

    // Team ID and SAO Device Address for the Badge
    uint8_t badge_team_id;
    uint16_t nec_transmit_team_address;
    uint8_t skip_message_delay;

    uint8_t nec_input_buffer[NEC_INPUT_BUFFER_LIMIT];
    nec_code_parser_t nec_input_data;

    // Initialize the Global NEC transmit variables
    nec_transmit_address = 0x0000U;
    nec_transmit_command = 0x00U;
    nec_transmit_rate = 0U;
    nec_transmit_repeats = 0U;
    nec_transmit_trigger = 0U;

    // Initialize the Global NEC receiver variables
    nec_input_address = 0x0000U;
    nec_input_command = 0x00U;
    nec_input_capture_age = NEC_INPUT_CAPTURE_MAX_AGE;

    initialize();

    morse_init(&morse_engine, morse_phrase_buffer, MORSE_MAXIMUM_CHARACTERS);

    Queue_Initialize(&nec_input_queue, nec_input_buffer, NEC_INPUT_BUFFER_LIMIT);

    nec_input_initialize(&nec_input_data);

    CLEAR_ALL_FLAGS

    eeprom_busy_wait();

    message_flags = eeprom_read_dword(&eeprom_message_flags);
    messages_unlocked = count_bits(message_flags);

    trigger_transmit_nec_frame = 0U;
    save_message_flags = 0U;

    // Initialize to First Message without delay
    morse_engine.parser_state = MORSE_END;
    badge_data_index = HIDDEN_MESSAGES;
    skip_message_delay = 0U;
    delay_message_count = 0U;

    // Initialize the loop counter
    loop_counter = 0;

    SET_FLAG__NEC_IN_RESTART_COUNT

    // Detect and configure the Team Address
    adc_value = adc_read();

    // Update Team ID and SAO Device Address based on ADC value
    if (adc_value < 0x2BU) {
        badge_team_id = COLOR_YELLOW;
        sao_device_address = 0x25U;
        nec_transmit_team_address = 0xFB15;
    } else if (adc_value < 0x6AU) {
        badge_team_id = COLOR_VIOLET;
        sao_device_address = 0x22U;
        nec_transmit_team_address = 0xFB12;
    } else if (adc_value < 0x95U) {
        badge_team_id = COLOR_BLUE;
        sao_device_address = 0x23U;
        nec_transmit_team_address = 0xFB13;
    } else if (adc_value < 0xD5U) {
        badge_team_id = COLOR_GREEN;
        sao_device_address = 0x24U;
        nec_transmit_team_address = 0xFB14;
    } else {
        badge_team_id = COLOR_RED;
        sao_device_address = 0x21U;
        nec_transmit_team_address = 0xFB11;
    }

    // Initialize SAO if switch is high
    if (PINB & (1 << SWITCH_Y_PIN)) {
        cli();
        i2c_initialize();
        sei();
        CLEAR_FLAG__SAO_STATE_TRIGGER
        sao_port_status = SAO_PORT_I2C_ACTIVE;
    } else {
        sao_port_status = SAO_PORT_I2C_DISABLED;
    }

    // Enable global interrupts
    sei();

    while(1) {

        // Update the current nec transmission state based on prior loop
        current_nec_transmit_state = next_nec_transmit_state;

        // Set WS2812 LED Color
        ws2812_set_color(led_value_red, led_value_green, led_value_blue);

        // Hold for 2 Timer 0 interrupt (16 milliseconds) to resynch the loop
        //   Setting the WS2812 LED color takes up to 12 milliseconds
        while(loop_counter < 2) {
            // Sleep until Next Interrupt
            cli();
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        }

        // Transmit NEC Frame
        if (trigger_transmit_nec_frame == 1U) {
            trigger_transmit_nec_frame = 0U;
            ir_nec_send(transmit_nec_address, transmit_nec_command);
        }

        // Hold for 2 Timer 0 interrupt (120 milliseconds) to resynch the loop
        //   Resynchronizing for the WS2812 LED took 16 milliseconds
        //   Sending an NEC Frame takes up to 104 milliseconds
        while(loop_counter < 15) {
            // Sleep until Next Interrupt
            cli();
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        }

        // Update behaviors when SAO Port is Active/Inactive and Unknown
        if (sao_port_status == SAO_PORT_I2C_ACTIVE) {

            // Setup NEC Transmissions as requested by SAO
            //   Otherwise disable NEC Transmissions
            switch (current_nec_transmit_state) {
                case BADGE_SAO_ACTIVE_NEC_IDLE:
                    if (nec_transmit_repeats > 0U) {
                        if (nec_transmit_trigger == 1U) {
                            next_nec_transmit_state = BADGE_NEC_TRIGGERED_VIA_SAO;

                            nec_transmit_repeats--;

                            nec_transmit_trigger = nec_transmit_rate;
                        } else if (nec_transmit_trigger > 0U) {
                            next_nec_transmit_state = BADGE_SAO_ACTIVE_NEC_IDLE;
                            nec_transmit_trigger--;
                        } else {
                            next_nec_transmit_state = BADGE_SAO_ACTIVE_NEC_IDLE;
                            nec_transmit_repeats = 0U;
                            nec_transmit_trigger = 0U;
                        }
                    } else {
                        next_nec_transmit_state = BADGE_SAO_ACTIVE_NEC_IDLE;
                        nec_transmit_trigger = 0U;
                    }
                    break;
                case BADGE_NEC_TRIGGERED_VIA_SAO:
                    next_nec_transmit_state = BADGE_SAO_ACTIVE_NEC_IDLE;
                    nec_transmit_trigger = nec_transmit_rate;
                    break;
                default:
                    next_nec_transmit_state = BADGE_SAO_ACTIVE_NEC_IDLE;
                    nec_transmit_trigger = 0U;
                    break;
            }

            // If an NEC transmission is set to occur, the LED Color is Red
            //   Otherwise, the LED Color is black
            switch (current_nec_transmit_state) {
                case BADGE_NEC_TRIGGERED_VIA_SAO:
                    next_led_state = COLOR_RED;
                    break;
            default:
                next_led_state = COLOR_BLACK;
                break;
            }

        } else if (sao_port_status == SAO_PORT_I2C_DISABLED) {

            // Process the Button X Press
            //   - Trigger an NEC Frame Transmission
            //   - Start First message without delay
            if (READ_FLAG__BUTTON_X_PRESSED) {
                next_nec_transmit_state = BADGE_NEC_TRIGGERED_VIA_BUTTON;
                morse_engine.parser_state = MORSE_END;
                badge_data_index = HIDDEN_MESSAGES;
                skip_message_delay = 1U;
                delay_message_count = 0U;
            } else {
                next_nec_transmit_state = BADGE_SAO_DISABLED;
            }

            // Process message displaying
            if (delay_message_count > 0U) {

                // Decrement the Delay
                delay_message_count = delay_message_count - 1U;

                // Keep LED off when Delaying the next Message
                next_led_state = COLOR_BLACK;
            } else {

                // Update to the next state based on the Morse Code
                //   Element Buffer contents.  Once the state is ended,
                //   stop updating states.
                morse_parse_phrase(&morse_engine);

                // Set LED if Morse is active based on the LED Color Count
                // Deactivate LED if Morse is not active
                if ((morse_engine.parser_state == MORSE_ACTIVE_EDGE) ||
                    (morse_engine.parser_state == MORSE_PROCESS_ACTIVE)) {
                    switch (morse_led_state.end_color) {
                        default:
                            if (morse_led_state.count == 0x00U) {
                                next_led_state = morse_led_state.start_color;
                            } else {
                                next_led_state = morse_led_state.end_color;
                            }
                            break;
                        case COLOR_RAINBOW_FORWARD:
                        case COLOR_RAINBOW_REVERSE:
                            switch (morse_led_state.count) {
                                default:
                                    next_led_state = COLOR_GREEN;
                                    break;
                                case 1U:
                                    next_led_state = COLOR_AQUA;
                                    break;
                                case 2U:
                                    next_led_state = COLOR_BLUE;
                                    break;
                                case 3U:
                                    next_led_state = COLOR_VIOLET;
                                    break;
                                case 4U:
                                    next_led_state = COLOR_RED;
                                    break;
                                case 5U:
                                    next_led_state = COLOR_YELLOW;
                                    break;
                            }
                            break;
                    }
                } else {
                    next_led_state = COLOR_BLACK;
                }

                // Update the Message Index if the Morse Code Message is ended
                if (morse_engine.parser_state == MORSE_END) {

                    // Set Index to next message or zero if index overflows
                    if (badge_data_index < (HIDDEN_MESSAGES - 1U)) {
                        badge_data_index = badge_data_index + 1U;
                    } else {
                        badge_data_index = 0U;
                    }

                    // Set Index to Zero if Index if message is locked
                    if (badge_data_index > messages_unlocked) {
                        badge_data_index = 0U;
                    }

                    // Copy message from ROM to RAM
                    badge_message_address = badge_messages[badge_data_index];
                    strcpy_P(badge_message_buffer, badge_message_address);
                }

                // Set Morse Code Parser State to START if parser state is at
                //   the end of a character or phrase.  Populate the Morse
                //   Code Element buffer if the Morse Code Parser State is set
                //   set to START by this function.
                morse_convert_to_phrase(&morse_engine, badge_message_buffer);

                // When the Morse Code Parser State started parsing a new
                //   character, update the character elements
                if (morse_engine.parser_state == MORSE_START) {

                    // When new character is the started of a new message
                    //     - Increment the Morse Code Phrase Index
                    //     - Wrap the Morse Code Phrase Index to Zero when
                    //       - Index increments past maximum value
                    //       - Index increments past unlocked messages
                    //     - When the Morse Code Phrase Index is set to Zero
                    //       - Initialize the delay sending the first Phrase
                    //     - Initialize First Color of the Phrase
                    //     - Initialize Color Changes of the Phrase
                    if (morse_engine.character_index == 0) {

                        // Add a delay if this is the start of message 0 unless
                        //   delay is flagged to be skipped
                        if (badge_data_index == 0){
                            if (skip_message_delay == 1) {
                                delay_message_count = 1U;
                                skip_message_delay = 0;
                            } else {
                                delay_message_count = TIME_DELAY_MESSAGES;
                            }
                        } else {
                            // Add a delay between each message
                            delay_message_count = 6U;
                        }

                        // Set LED color for the first character
                        morse_led_state.start_color =
                            badge_data[badge_data_index].start_color;
                        if ((morse_led_state.start_color == COLOR_TEAM) ||
                            ((morse_led_state.start_color == COLOR_RAINBOW_FORWARD) ||
                            (morse_led_state.start_color == COLOR_RAINBOW_REVERSE))) {
                            morse_led_state.start_color = badge_team_id;
                        }

                        // Set LED color for the next character
                        //   Set count used to cycle colors based on next color
                        morse_led_state.end_color =
                            badge_data[badge_data_index].end_color;
                        if ((morse_led_state.end_color == COLOR_RAINBOW_FORWARD) ||
                            (morse_led_state.end_color == COLOR_RAINBOW_REVERSE)) {
                            switch (morse_led_state.start_color) {
                                case COLOR_GREEN:
                                    morse_led_state.count = 0U;
                                    break;
                                case COLOR_AQUA:
                                    morse_led_state.count = 1U;
                                    break;
                                case COLOR_BLUE:
                                    morse_led_state.count = 2U;
                                    break;
                                case COLOR_VIOLET:
                                    morse_led_state.count = 3U;
                                    break;
                                case COLOR_RED:
                                    morse_led_state.count = 4U;
                                    break;
                                default:
                                case COLOR_YELLOW:
                                    morse_led_state.count = 5U;
                                    break;
                            }
                        } else if (morse_led_state.end_color == COLOR_TEAM) {
                            morse_led_state.end_color = badge_team_id;
                            morse_led_state.count = 0U;
                        } else {
                            morse_led_state.count = 0U;
                        }
                    } else {

                        // Update the LED Color
                        switch (morse_led_state.end_color) {
                            case COLOR_RAINBOW_FORWARD:
                                // Update the LED Color Count
                                if (morse_led_state.count > 0U) {
                                    morse_led_state.count--;
                                } else {
                                    morse_led_state.count = 5U;
                                }
                                break;
                            case COLOR_RAINBOW_REVERSE:
                                // Update the LED Color Count
                                if (morse_led_state.count <= 4U) {
                                    morse_led_state.count++;
                                } else {
                                    morse_led_state.count = 0U;
                                }
                                break;
                            default:
                                // Toggle the LED Color Count
                                if (morse_led_state.count == 1U) {
                                    morse_led_state.count = 0U;
                                } else {
                                    morse_led_state.count = 1U;
                                }
                                break;
                        }
                    }
                }
            }
        } else {
            // Disable NEC Transmissions
            next_nec_transmit_state = BADGE_SAO_DISABLED;

            // Setup Transmission of NEC Frame
            trigger_transmit_nec_frame = 0U;

            // Disable LED
            next_led_state = COLOR_BLACK;
        }

        // Process GPIO Pin Change Interrupts for NEC Frame Processing
        Queue_Length(&nec_input_queue, &nec_input_size);

        for (counter = 0U; counter < nec_input_size; counter++)
        {
            Queue_Eject(&nec_input_queue, &nec_input_data.pin_state);
            nec_input_parser(&nec_input_data);
            if (nec_input_data.parser_state == NEC_TAIL_00) {

                // The NEC frame was processed
                nec_input_address = nec_input_data.address;
                nec_input_command = nec_input_data.command;
                nec_input_capture_age = 1U;
            }
        }

        // Process NEC Frams for transmission
        if (next_nec_transmit_state == BADGE_NEC_TRIGGERED_VIA_BUTTON) {
            // Trigger NEC Frame Transmission if a Button Press is detected
            trigger_transmit_nec_frame = 1U;
            transmit_nec_address = nec_transmit_team_address;
            transmit_nec_command = 0x01;
        } else if (next_nec_transmit_state == BADGE_NEC_TRIGGERED_VIA_IR) {
            // Trigger NEC Frame Transmission if a transmit NEC Frame is received
            trigger_transmit_nec_frame = 1U;
            transmit_nec_address = nec_transmit_team_address;
            transmit_nec_command = messages_unlocked;
        } else if (next_nec_transmit_state == BADGE_NEC_TRIGGERED_VIA_SAO) {
            // Trigger NEC Frame Transmission if a transmit NEC Frame is received
            trigger_transmit_nec_frame = 1U;
            transmit_nec_address = nec_transmit_address;
            transmit_nec_command = nec_transmit_command;
        }

        // Update unlocked hidden messages
        if (nec_input_capture_age == 1U) {
            if (nec_input_command == 7U) {
                switch (nec_input_address) {
                    case 0xFB20:
                        message_flags = 0x00000000;
                        break;
                    case 0xFB21:
                        message_flags = message_flags | 0x0000000F;
                        break;
                    case 0xFB22:
                        message_flags = message_flags | 0x0000001E;
                        break;
                    case 0xFB23:
                        message_flags = message_flags | 0x000001E0;
                        break;
                    case 0xFB24:
                        message_flags = message_flags | 0x00001E00;
                        break;
                    case 0xFB25:
                        message_flags = message_flags | 0x0001E000;
                        break;
                    case 0xFB26:
                        message_flags = message_flags | 0x001E0000;
                        break;
                    default:
                        break;
                }
                messages_unlocked = count_bits(message_flags);
                save_message_flags = 1U;
            }
        }

        // Cardputer Interaction
        if (nec_input_capture_age == 1U) {
            if (nec_input_address == 0x12EDU) {
                switch (nec_input_command) {
                    case 0x00:
                        message_flags = 0x00000000;
                        break;
                    case 0x01:
                        message_flags = message_flags | 0x0000000F;
                        break;
                    case 0x02:
                        message_flags = message_flags | 0x000000F0;
                        break;
                    case 0x03:
                        message_flags = message_flags | 0x00000F00;
                        break;
                    case 0x04:
                        message_flags = message_flags | 0x0000F000;
                        break;
                    case 0x05:
                        message_flags = message_flags | 0x000F0000;
                        break;
                    case 0x06:
                        message_flags = message_flags | 0x00F00000;
                        break;
                    default:
                        break;
                }
                messages_unlocked = count_bits(message_flags);
                save_message_flags = 1U;
            }
        }

        // check if eeprom is ready to save and save changed message flags if new save is required
        if (save_message_flags != 0U) {
            if (eeprom_is_ready()) {
                eeprom_update_dword((uint32_t*) &eeprom_message_flags,
                                    message_flags);
                save_message_flags = 0U;
            }
            morse_engine.parser_state = MORSE_END;
            badge_data_index = HIDDEN_MESSAGES;
            skip_message_delay = 1U;
            delay_message_count = 0U;
        }

        // Sets led color values based on current led state
        switch(next_led_state) {
            case COLOR_VIOLET:
                led_value_red = 255U;
                led_value_green = 0U;
                led_value_blue = 255U;
                break;
            case COLOR_YELLOW:
                led_value_red = 255U;
                led_value_green = 255U;
                led_value_blue = 0U;
                break;
            case COLOR_AQUA:
                led_value_red = 0U;
                led_value_green = 255U;
                led_value_blue = 255U;
                break;
            case COLOR_RED:
                led_value_red = 255U;
                led_value_green = 0U;
                led_value_blue = 0U;
                break;
            case COLOR_GREEN:
                led_value_red = 0U;
                led_value_green = 255U;
                led_value_blue = 0U;
                break;
            case COLOR_BLUE:
                led_value_red = 0U;
                led_value_green = 0U;
                led_value_blue = 255U;
                break;
            case COLOR_WHITE:
                led_value_red = 255U;
                led_value_green = 255U;
                led_value_blue = 255U;
                break;
            default:
            case COLOR_BLACK:
                led_value_red = 0U;
                led_value_green = 0U;
                led_value_blue = 0U;
                break;
        }

        if (nec_input_capture_age < NEC_INPUT_CAPTURE_MAX_AGE) {
            // Increment the NEC input capture age if it is less than the maximum
            nec_input_capture_age++;
        }

        // Hold for Loop Period
        while(loop_counter < LOOP_COUNT) {

            // Sleep until Next Interrupt
            cli();
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        }

        // Subtract Loop Period for the next loop
        loop_counter = loop_counter - LOOP_COUNT;
    }
}
