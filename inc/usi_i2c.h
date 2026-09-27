/**
 * @file      usi_i2c.h
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     USI I2C Driver
 * @version   1.0
 * @date      2026-09-26
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

#ifndef USI_I2C_H
#define	USI_I2C_H

#ifdef	__cplusplus
extern "C" {
#endif

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


#ifdef	__cplusplus
}
#endif

#endif	/* USI_I2C_H */
