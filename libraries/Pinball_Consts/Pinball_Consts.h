// PINBALL_CONSTS.H Rev: 12/28/25.

#ifndef PINBALL_CONSTS_H
#define PINBALL_CONSTS_H

#include <Arduino.h>  // Allows use of "byte" and other Arduino-specifics (eliminates 'byte' does not name a type compiler error.)

// *** NOTE ABOUT MOSFETs AND PWM PINS ***
// NOTE: For MOSFETs, can do PWM on pins 4-13 and 44-46 = 13 pins.  However, pins 4 and 13 can't have frequency modified.
// Use pin numbers 23+ for regular digital pins where PWM is not needed (pin 22 reserved for RS-485 Tx Enable.)
// If a coil is held on, especially at less than 100%, such as ball tray release, it may hum if using PWM at default frequency.
// Increase frequency to eliminate hum (pins 5-12 and 44-46, but *not* pins 4 or 13.)
// Only use pin 4 and 13 for coils that are momentary *and* where PWM may be desired: pop bumper, kickouts, slingshots.
// Other PWM pins can be used for any coil or motor, but especially those that may be held on *and* where PWM is desired: flippers,
// ball tray release, ball trough release.
// Coils that are both momentary and when PWM not needed: for full-strength momentary MOSFET-on, use any digital output: knocker,
// selection unit, relay reset unit.
 
// *** ARDUINO DEVICE CONSTANTS: Here are all the different Arduinos and their "addresses" (ID numbers) for communication.
const byte ARDUINO_NUL =  0;              // Use this to initialize etc.
const byte ARDUINO_MAS =  1;              // Master Arduino (Main controller)
const byte ARDUINO_SLV =  2;              // Slave Arduino
const byte ARDUINO_ALL = 99;              // Master broadcasting to all i.e. mode change

// *** OPERATING MODES:
const byte MODE_UNDEFINED   = 0;
const byte MODE_ATTRACT     = 1;
const byte MODE_ORIGINAL    = 2;
const byte MODE_ENHANCED    = 3;
const byte MODE_IMPULSE     = 4;
const byte MODE_TILT        = 5;
const byte MODE_DIAGNOSTIC  = 6;

const byte LCD_WIDTH      = 20;  // 2004 (20 char by 4 lines) LCD display

// Serial port speed
// 6/5/22: These should really be i.e. SERIAL_SPEED_MONITOR, SERIAL_SPEED_2004LCD, SERIAL_SPEED_RS485, SERIAL_SPEED_LEGACY_BASE
const long unsigned int SERIAL0_SPEED = 115200;  // Serial port 0 is used for serial monitor
const long unsigned int SERIAL1_SPEED = 115200;  // Serial port 1 is Digole 2004 LCD display
const long unsigned int SERIAL2_SPEED = 115200;  // Serial port 2 is the RS485 communications bus
const long unsigned int SERIAL3_SPEED =  57600;  // Serial port 3 is Tsunami WAV Trigger defaults to 57.6k baud

// Note that the serial input buffer is only 64 bytes, which means that we need to keep emptying it since there will be
// many commands between Arduinos.  If the buffer overflows, then we will be totally screwed up (but it will be
// apparent in the checksum.)
const byte RS485_TRANSMIT       = HIGH;      // HIGH = 0x1.  How to set TX_CONTROL pin when we want to transmit RS485
const byte RS485_RECEIVE        = LOW;       // LOW = 0x0.  How to set TX_CONTROL pin when we want to receive (or NOT transmit) RS485
const byte RS485_MAX_LEN        = 20;        // buf len to hold the longest possible RS485 msg incl to, from, CRC.
const byte RS485_LEN_OFFSET     =  0;        // first byte of message is always total message length in bytes
const byte RS485_FROM_OFFSET    =  1;        // second byte of message is the ID of the Arduino the message is coming from
const byte RS485_TO_OFFSET      =  2;        // third byte of message is the ID of the Arduino the message is addressed to
const byte RS485_TYPE_OFFSET    =  3;        // fourth byte of message is the type of message such as 'C' for Credit
const byte RS845_PAYLOAD_OFFSET =  4;        // fifth byte of message is the first byte of the payload, i.e. the data
// Note also that the LAST byte of every message is a CRC8 checksum of all bytes except the last.

// Here is a list of all RS485 message types (the 1-byte TYPE field):
const byte RS485_TYPE_NO_MESSAGE                    =  0;  // No message
const byte RS485_TYPE_MAS_TO_SLV_MODE               =  1;  // Mode change.
const byte RS485_TYPE_MAS_TO_SLV_COMMAND_RESET      =  2;  // Software reset.
// MODE_ORIGINAL/ENHANCED/IMPULSE starts new game; tilt off, GI on, revert score, but does not deduct a credit.
const byte RS485_TYPE_MAS_TO_SLV_CREDIT_STATUS      =  3;  // Request if credits > zero
const byte RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS      =  4;  // Slave response to credit status request: credits zero or > zero
const byte RS485_TYPE_MAS_TO_SLV_CREDIT_FULL_QUERY  =  5;  // Master requesting if credit unit full
const byte RS485_TYPE_SLV_TO_MAS_CREDIT_FULL_STATUS =  6;  // Slave reporting if credit unit full
const byte RS485_TYPE_MAS_TO_SLV_CREDIT_INC         =  7;  // Credit increment
const byte RS485_TYPE_MAS_TO_SLV_CREDIT_DEC         =  8;  // Credit decrement (will not return error even if credits already zero)
const byte RS485_TYPE_MAS_TO_SLV_SCORE_RESET        =  9;  // Reset score to zero
const byte RS485_TYPE_MAS_TO_SLV_SCORE_ABS          = 10;  // Absolute score update (0.999 in 10,000s)
const byte RS485_TYPE_MAS_TO_SLV_SCORE_FLASH        = 11;  // Flash score (used w/ 1M..4M to indicate player number 1..4)
const byte RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K      = 12;  // Increment score update (1..999in 10,000s)
const byte RS485_TYPE_MAS_TO_SLV_SCORE_INC_100K     = 13;  // Increment score update (1..999 in 100,000s)
const byte RS485_TYPE_MAS_TO_SLV_SCORE_DEC_10K      = 14;  // Decrement score update (-999..-1 in 10,000s) (won't go below zero)
const byte RS485_TYPE_MAS_TO_SLV_BELL_10K           = 15;  // Ring the 10K bell
const byte RS485_TYPE_MAS_TO_SLV_BELL_100K          = 16;  // Ring the 100K bell
const byte RS485_TYPE_MAS_TO_SLV_BELL_SELECT        = 17;  // Ring the Select bell
const byte RS485_TYPE_MAS_TO_SLV_10K_UNIT           = 18;  // Pulse the 10K Unit coil (for testing)
const byte RS485_TYPE_MAS_TO_SLV_SCORE_QUERY        = 19;  // Master requesting current score displayed by Slave (in 10,000s)
const byte RS485_TYPE_SLV_TO_MAS_SCORE_REPORT       = 20;  // Slave reporting current score (in 10,000s)
const byte RS485_TYPE_MAS_TO_SLV_GI_LAMP            = 21;  // Master command to turn G.I. lamps on or off
const byte RS485_TYPE_MAS_TO_SLV_TILT_LAMP          = 22;  // Master command to turn Tilt lamp on or off
// RS-485 ERROR MESSAGE TYPES:
// Error codes that available() can return instead of a message type
const byte RS485_ERROR_BEGIN                    = 50;  // Error codes start here
const byte RS485_ERROR_UNEXPECTED_TYPE          = 51;  // Message type not recognized
const byte RS485_ERROR_BUFFER_OVERFLOW          = 52;  // Input buffer overflow
const byte RS485_ERROR_MSG_TOO_SHORT            = 53;  // Message length < 3
const byte RS485_ERROR_MSG_TOO_LONG             = 54;  // Message length > RS485_MAX_LEN
const byte RS485_ERROR_CHECKSUM                 = 55;  // CRC checksum mismatch
const byte SLAVE_SCORE_QUEUE_FULL               = 56;  // Slave score command queue full

// *** ARDUINO PIN NUMBERS:
// *** STANDARD I/O PORT PIN NUMBERS ***
const byte PIN_IN_MEGA_RX0             =  0;  // Serial 0 receive PC Serial monitor
const byte PIN_OUT_MEGA_TX0            =  1;  // Serial 0 transmit PC Serial monitor
const byte PIN_IN_MEGA_RX1             = 19;  // Serial 1 receive NOT NEEDED FOR 2004 LCD (previosly used for FRAM3)
const byte PIN_OUT_MEGA_TX1            = 18;  // Serial 1 transmit 2004 LCD
const byte PIN_IN_MEGA_RX2             = 17;  // Serial 2 receive RS485 network
const byte PIN_OUT_MEGA_TX2            = 16;  // Serial 2 transmit RS485 network
const byte PIN_IN_MEGA_RX3             = 15;  // Serial 3 receive Tsunami WAV Trigger
const byte PIN_OUT_MEGA_TX3            = 14;  // Serial 3 transmit Tsunami WAV Trigger
const byte PIN_IO_MEGA_SDA             = 20;  // I2C (also pin 44) Centipede SDA
const byte PIN_IO_MEGA_SCL             = 21;  // I2C (also pin 43) Centipede SCL
const byte PIN_IO_MEGA_MISO            = 50;  // SPI Master In Slave Out (MISO) a.k.a. Controller In Peripheral Out (CIPO)
const byte PIN_IO_MEGA_MOSI            = 51;  // SPI Master Out Slave In (MOSI) a.k.a. Controller Out Peripheral In (COPI)
const byte PIN_IO_MEGA_SCK             = 52;  // SPI Clock
const byte PIN_IO_MEGA_SS              = 53;  // SPI Slave Select (SS) a.k.a. Chip Select (CS).  Used for the 256KB/512KB FRAM chip.
// Tx Enable was previously pin 4 but that's a PWM which we need for coils, so moved to pin 22
const byte PIN_OUT_RS485_TX_ENABLE     = 22;  // Output: set HIGH when in RS485 transmit mode, LOW when not transmitting
// My custom-built train-layout RS-485 boards included Tx and Rx LEDs, but I'm using generic boards on Screamo.
// It would be fun to add the LED boards to Screamo just for visual effect.
//const byte PIN_OUT_RS485_TX_LED        =  5;  // Output: set HIGH to turn on BLUE LED when RS485 is TRANSMITTING data
//const byte PIN_OUT_RS485_RX_LED        =  6;  // Output: set HIGH to turn on YELLOW when RS485 is RECEIVING data

// *** MISC HARDWARE CONNECTION PINS ***
//const byte PIN_OUT_SPEAKER             =  7;  // Output: Piezo buzzer connects positive here
//const byte PIN_IN_WAV_STATUS           = 10;  // Input: LOW when OCC WAV Trigger track is playing, else HIGH.
//const byte PIN_IO_FRAM_CS              = 53;  // Output: FRAM  chip select.

const byte PIN_OUT_LED                 = 13;  // Built-in LED always pin 13

// SLAVE ARDUINO HEAD DIRECT INPUT PIN NUMBERS (Switches):
const byte PIN_IN_SWITCH_CREDIT_EMPTY       =  2;  // Input : Credit wheel "Empty" switch.  Pulled LOW when empty.
const byte PIN_IN_SWITCH_CREDIT_FULL        =  3;  // Input : Credit wheel "Full" switch.  Pulled LOW when full.

#endif
