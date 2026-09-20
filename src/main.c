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
//   Phase 9: Receive NEC Frames via IR and store on SAO Client Interface
//
//      NEC-IN Input will decode a filtered infrared signal and will store
//   the values internally in RAM.  The SAO Client can be used to read the
//   last stored NEC Frame by reading 3 bytes.
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
//     - All other Values - null values of 0xFF per I2C standard behaivior
//
//      The Code for Switch Y and Button X have been left in the code
//   however, the initialization code is disabled so the flags for the
//   inputs will not change state.
//
//============================================================================

#define FIRMWARE_ID "MFOC 2026 Badge V"
#define FIRMWARE_VERSION "0.01a"

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

#define NEC_INPUT_BUFFER_LIMIT                                              70

#define NEC_PIN_SNAPSHOT_BUFFER_LIMIT                                       36

//============================================================================

// States for tracking behavior for each USI Interrupt
enum sao_i2c_states {
    SAO_CHECK_ADDRESS,
    SAO_ACK_WRITE,
    SAO_ACK_READ,
    SAO_WRITE_DATA,
    SAO_READ_DATA,
    SAO_NACK_READ,
    SAO_WAIT
};

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

volatile uint8_t sao_device_address = 0;
volatile uint8_t sao_buffer_index = 0;
volatile uint8_t sao_buffer[SAO_BUFFER_LIMIT];
volatile uint8_t sao_output_buffer[SAO_BUFFER_LIMIT];
volatile uint8_t pulse_buffer[NEC_PIN_SNAPSHOT_BUFFER_LIMIT];

// Loop counter incremented by Timer 0 interrupt
volatile uint8_t loop_counter = 0;

volatile enum sao_i2c_states sao_state;

queue_t nec_input_queue;

//============================================================================
//
// Section: Macros for USI Control Register
//
//      This section contains Macros for managing the USI Control Register.
//   Definitions are provided in a descriptive manner to ensure the proper
//   configuration of the USI Control Register.
//
//============================================================================

// USI Operates in Two-Wire Mode where SCL is held low during USI_OVF_vect
#define USI_TWI_MODE                             (1 << USIWM1) | (1 << USIWM0)

// USI is Disbled and USI pins are returned to GPIO functionality
#define USI_DISABLED_MODE                        (0 << USIWM1) | (0 << USIWM0)

// USI Overflow ISR uses SCL line with Edge Triggered 4-bit Counter
#define USI_OVF_SRC_SCL_4BIT     (1 << USICS1) | (0 << USICS0) | (0 << USICLK)

// USI Start Condition Interrupt is enabled
#define USI_START_ISR                                            (1 << USISIE)

// USI Counter Overflow Interrupt is enabled
#define USI_OVERFLOW_ISR                                         (1 << USIOIE)

// USI clear the Counter Overflow Interrupt Flag
#define USI_CLEAR_OVERFLOW_FLAG                        USISR |= (1 << USIOIF);

// Clear all USI Flags and Reset USI Counter to 0
//  - Clear Start Condition Interrupt Flag
//  - Clear Counter Overflow Interrupt Flag
//  - Clear Stop Condition Flag
//  - Clear Data Output Collision Flag
//  - Reset USI counter to 0 (ready for 8 bits)
#define USI_CLEAR_ALL_ISR_FLAGS                      USISR = (1 << USISIF) | \
                                                             (1 << USIOIF) | \
                                                              (1 << USIPF) | \
                                                              (1 << USIDC) | \
                                                               (0 << USICNT0);

//============================================================================
//
// Section: Macros for SAO Management
//
//      This section contains Macros for managing the SAO Client. Definitions
//   are provided in a descriptive manner to ensure the proper configuration
//   and operation of the SAO CLient.
//
//============================================================================

// Iniitalize SAO pins to check for Start Condition
//  - Set SDA pin as an input
//  - Set SCL pin as an input
//  - Set SDA pull-up is disabled
//  - Set SCL pull-up is disabled
#define SAO_INITIALIZE_PINS                   DDRB &= ~((1 << SAO_SDA_PIN) | \
                                                        (1 << SAO_SCL_PIN)); \
                                             PORTB &= ~((1 << SAO_SDA_PIN) | \
                                                          (1 << SAO_SCL_PIN));

// Set SDA pin low for one Period
//  - Set USI Data Register to 0x00
//  - Set SDA pin to high (required to send data)
//  - Set SDA pin direction to output
//  - Clear Start Condition Flag
//  - Clear Counter Overflow Flag
//  - Clear Stop Condition Flag
//  - Clear Data Output Collision Flag
//  - Reset USI counter to 14 (ready for 1 bit)
#define SAO_SEND_ACK                             DDRB |= (1 << SAO_SDA_PIN); \
                                                               USIDR = 0x00; \
                                                     USISR = (1 << USISIF) | \
                                                             (1 << USIOIF) | \
                                                              (1 << USIPF) | \
                                                              (1 << USIDC) | \
                                                              (14 << USICNT0);

// Set SDA pin high for one Period
//  - Set USI Data Register to 0xFF
//  - Set SDA pin to High (required to send data)
//  - Set SDA pin direction to output
//  - Clear Start Condition Flag
//  - Clear Counter Overflow Flag
//  - Clear Stop Condition Flag
//  - Clear Data Output Collision Flag
//  - Reset USI counter to 14 (ready for 1 bit)
#define SAO_SEND_NACK                                          USIDR = 0xFF; \
                                                PORTB |= (1 << SAO_SDA_PIN); \
                                                 DDRB |= (1 << SAO_SDA_PIN); \
                                                     USISR = (1 << USISIF) | \
                                                             (1 << USIOIF) | \
                                                              (1 << USIPF) | \
                                                              (1 << USIDC) | \
                                                              (14 << USICNT0);

// Set SDA pin to read for one period
//  - Set SDA pin direction to input
//  - Clear Counter Overflow Flag
//  - Reset USI counter to 14 (ready for 1 bit)
#define SAO_RECEIVE_ACK                         DDRB &= ~(1 << SAO_SDA_PIN); \
                                      USISR = (1 << USIOIF) | (14 << USICNT0);

// Set SDA pin to read for eight periods where data will be written to USIDR
//  - Set SDA pin direction to input
//  - Clear Start Condition Flag
//  - Clear Counter Overflow Flag
//  - Clear Stop Condition Flag
//  - Clear Data Output Collision Flag
//  - Reset USI counter to 14 (ready for 1 bit)
#define SAO_RECEIVE_DATA_IN_USIDR               DDRB &= ~(1 << SAO_SDA_PIN); \
                                                     USISR = (1 << USISIF) | \
                                                             (1 << USIOIF) | \
                                                              (1 << USIPF) | \
                                                              (1 << USIDC) | \
                                                               (0 << USICNT0);

// Set SDA pin to write for eight periods with data in USIDR
//  - Set SDA pin to High (required to send data)
//  - Set SDA pin direction to output
//  - Clear Start Condition Flag
//  - Clear Counter Overflow Flag
//  - Clear Stop Condition Flag
//  - Clear Data Output Collision Flag
//  - Reset USI counter to 14 (ready for 1 bit)
#define SAO_SEND_DATA_IN_USIDR                  PORTB |= (1 << SAO_SDA_PIN); \
                                                 DDRB |= (1 << SAO_SDA_PIN); \
                                                     USISR = (1 << USISIF) | \
                                                             (1 << USIOIF) | \
                                                              (1 << USIPF) | \
                                                              (1 << USIDC) | \
                                                               (0 << USICNT0);

// Monitor SDA pin for new I2C Frames
//  - Set SDA pin direction to input
#define SAO_MONITOR_FOR_FRAMES                    DDRB &= ~(1 << SAO_SDA_PIN);

// Setup USI to be disabled
//  - Clear the USI Overflow Interrupt Flag
// Enable GPIO Interrupt for SCL Pin
#define SAO_DISABLED                             USICR =  USI_DISABLED_MODE; \
                                                    USI_CLEAR_OVERFLOW_FLAG; \
                                    PCMSK |= (1 << SAO_CLOCK_DETECT_INTERRUPT);

// Disable GPIO Interrupt for SCL Pin
#define SAO_ENABLED                PCMSK &= ~(1 << SAO_CLOCK_DETECT_INTERRUPT);


// Setup USI for SAO with no Interrupts Enabled
//  - Set USI_OVF_vect with Edge Triggered 4-bit Counter on SCL
//  - Clear the USI Overflow Interrupt Flag
#define SAO_INTERRUPT_DISABLED                 USICR = USI_OVF_SRC_SCL_4BIT; \
                                                      USI_CLEAR_OVERFLOW_FLAG;

// Setup USI for SAO when I2C Frame is not Present
//  - Enable Start Condition Interrupt
//  - Disable all other USI Interrupts
//  - Set USI to Two-Wire Mode where SCL is held low on USI_OVF_vect
//  - Set USI_OVF_vect with Edge Triggered 4-bit Counter on SCL
//  - Clear the USI Overflow Interrupt Flag
//  - Setup the SDA pin as an Input
//  - Setup SAO State Machine to Monitor for Start of Frame
#define SAO_INTERRUPT_ON_START_CONDITION             USICR = USI_START_ISR | \
                                                              USI_TWI_MODE | \
                                                         USI_OVF_SRC_SCL_4BIT;

// Setup USI for SAO when ingesting an I2C Frame
//  - Enable Start Condition Interrupt
//  - Enable Counter Overflow Interrupt
//  - Disable all other USI Interrupts
//  - Set USI to Two-Wire Mode where SCL is held low on USI_OVF_vect
//  - Set USI_OVF_vect with Edge Triggered 4-bit Counter on SCL
#define SAO_INTERRUPT_ON_BYTE_CAPTURE                USICR = USI_START_ISR | \
                                                          USI_OVERFLOW_ISR | \
                                                              USI_TWI_MODE | \
                                                         USI_OVF_SRC_SCL_4BIT;

// Check SAO Pin states
#define SAO_PIN_SDA_HIGH                             PINB & (1 << SAO_SDA_PIN)
#define SAO_PIN_SDA_LOW                           !(PINB & (1 << SAO_SDA_PIN))
#define SAO_PIN_SCL_HIGH                             PINB & (1 << SAO_SCL_PIN)
#define SAO_PIN_SCL_LOW                           !(PINB & (1 << SAO_SCL_PIN))

// Start SAO Client
#define SAO_START                                     SAO_CLEAR_CLOCK_DETECT \
                                                                   SAO_ENABLED

// Stop SAO Client
#define SAO_STOP                                   SAO_CLEAR_INTERRUPT_FLAGS \
                                                                  SAO_DISABLED

// Check if SAO Client is stopped
#define SAO_NOT_ACTIVE                                !(USICR & USI_START_ISR)
#define SAO_ACTIVE                                     (USICR & USI_START_ISR)

// Updated Flag Management Macros for SAO Client
#define SAO_CLOCK_DETECTED                        READ_FLAG__SAO_STATE_TRIGGER
#define SAO_CLEAR_CLOCK_DETECT                   CLEAR_FLAG__SAO_STATE_TRIGGER
#define SAO_FRAME_DETECTED                        READ_FLAG__SAO_STATE_TRIGGER
#define SAO_CLEAR_FRAME_DETECT                   CLEAR_FLAG__SAO_STATE_TRIGGER
#define SAO_CLEAR_INTERRUPT_FLAGS                CLEAR_FLAG__SAO_STATE_TRIGGER

//============================================================================

void i2c_device_init(uint8_t address) {

    uint8_t counter;

    SAO_START

    for (counter = 0; counter < SAO_BUFFER_LIMIT; counter++)
    {
        sao_buffer[counter] = 0xFF;
        sao_output_buffer[counter] = 0xFF;
    }

    sao_device_address = address;

    SAO_INITIALIZE_PINS

    USI_CLEAR_ALL_ISR_FLAGS

    SAO_INTERRUPT_ON_START_CONDITION

    sao_state = SAO_CHECK_ADDRESS;
}

ISR(USI_START_vect) {
    uint8_t i2c_start_condition_active = 1U;
    uint8_t i2c_frame_started = 0U;
    uint32_t time_out_counter = 0U;

    // Set state machine to check to check for the SAO Device Address
    sao_state = SAO_CHECK_ADDRESS;

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
    DDRB &= ~(1 << SWITCH_Y_PIN);
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

    SAO_CLEAR_INTERRUPT_FLAGS

    // set processor to sleep mode idle to save power between interrupts
    set_sleep_mode(SLEEP_MODE_IDLE);

    SET_FLAG__NEC_IN_RESTART_COUNT

    // Enable global interrupts
    sei();
}

int main(void) {

    // Add State Machine State Variable
    uint8_t state = 0;

    // Add State Machine for WS2812 LED
    enum led_colors next_led_state = COLOR_WHITE;

    // Add RGB Color Values for WS2812 LED
    uint8_t led_value_red = 255U;
    uint8_t led_value_green = 255U;
    uint8_t led_value_blue = 255U;

    // Initialize the ADC Input
    uint8_t adc_value = 0U;

    uint8_t nec_input_buffer[NEC_INPUT_BUFFER_LIMIT];
    nec_code_parser_t nec_input_data;

    // Initialize the Global NEC receiver variables
    nec_input_address = 0x0000U;
    nec_input_command = 0x00U;
    nec_input_capture_age = NEC_INPUT_CAPTURE_MAX_AGE;

    initialize();

    Queue_Initialize(&nec_input_queue, nec_input_buffer, NEC_INPUT_BUFFER_LIMIT);

    nec_input_initialize(&nec_input_data);

    CLEAR_ALL_FLAGS

    // Initialize the loop counter
    loop_counter = 0;

    SET_FLAG__NEC_IN_RESTART_COUNT

    i2c_device_init(0x42);

    // Enable global interrupts
    sei();

    while(1) {

        // Read the ADC value
        adc_value = adc_read();

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

        // Update LED Color based on state value
        switch(state) {
            case 0:
            case 2:
            case 4:
            case 6:
            case 8:
                if (adc_value < 0x2BU) {
                    next_led_state = COLOR_GREEN;
                } else if (adc_value < 0x6AU) {
                    next_led_state = COLOR_AQUA;
                } else if (adc_value < 0x95U) {
                    next_led_state = COLOR_BLUE;
                } else if (adc_value < 0xD5U) {
                    next_led_state = COLOR_VIOLET;
                } else {
                    next_led_state = COLOR_RED;
                }
                break;
            case 1:
            case 3:
            case 5:
            case 7:
                if (nec_input_command == 0x00U) {
                    next_led_state = COLOR_BLACK;
                } else if (nec_input_command == 0x02U) {
                    next_led_state = COLOR_WHITE;
                } else {
                    next_led_state = COLOR_YELLOW;
                }
                break;
            case 9:
            default:
                next_led_state = COLOR_BLACK;
                break;
        }

        // State increments to 9, then back to 0
        if (state < 9) {
            state = state + 1;
        } else {
            state = 0;
        }

    if (READ_FLAG__BUTTON_X_PRESSED) {
        next_led_state = COLOR_BLACK;
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
