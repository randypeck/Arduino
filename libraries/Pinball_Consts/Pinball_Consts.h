// PINBALL_CONSTS.H Rev: 08/30/25.

#ifndef PINBALL_CONSTS_H
#define PINBALL_CONSTS_H

#include <Arduino.h>  // Allows use of "byte" and other Arduino-specifics (eliminates 'byte' does not name a type compiler error.)

// *** ARDUINO DEVICE CONSTANTS: Here are all the different Arduinos and their "addresses" (ID numbers) for communication.
const byte ARDUINO_NUL =  0;              // Use this to initialize etc.
const byte ARDUINO_MAS =  1;              // Master Arduino (Main controller)
const byte ARDUINO_SLV =  2;              // Slave Arduino
const byte ARDUINO_ALL = 99;              // Master broadcasting to all i.e. mode change

// *** OPERATING MODES AND STATES:
const byte MODE_UNDEFINED  = 0;
const byte MODE_DIAGNOSTIC = 1;           // Diagnostics (Default mode until a game is started)
const byte MODE_ORIGINAL   = 2;           // Plays as original but with normal flippers (not impulse.)
const byte MODE_ENHANCED   = 3;           // Randy's Screamo rules
const byte MODE_IMPULSE    = 4;           // Impulse flippers: yuk!
const byte STATE_UNDEFINED = 0;
const byte STATE_GAME_OVER = 1;
const byte STATE_PLAYING   = 2;
const byte STATE_TILT      = 3;

const byte LCD_WIDTH      = 20;  // 2004 (20 char by 4 lines) LCD display

// Serial port speed
// 6/5/22: These should really be i.e. SERIAL_SPEED_MONITOR, SERIAL_SPEED_2004LCD, SERIAL_SPEED_RS485, SERIAL_SPEED_LEGACY_BASE
const long unsigned int SERIAL0_SPEED = 115200;  // Serial port 0 is used for serial monitor
const long unsigned int SERIAL1_SPEED = 115200;  // Serial port 1 is Digole 2004 LCD display
const long unsigned int SERIAL2_SPEED = 115200;  // Serial port 2 is the RS485 communications bus
const long unsigned int SERIAL3_SPEED =   9600;  // Serial port 3 is Tsunami WAV Trigger

// Note that the serial input buffer is only 64 bytes, which means that we need to keep emptying it since there
// will be many commands between Arduinos, even though most may not be for THIS Arduino.  If the buffer overflows,
// then we will be totally screwed up (but it will be apparent in the checksum.)
const byte RS485_TRANSMIT       = HIGH;      // HIGH = 0x1.  How to set TX_CONTROL pin when we want to transmit RS485
const byte RS485_RECEIVE        = LOW;       // LOW = 0x0.  How to set TX_CONTROL pin when we want to receive (or NOT transmit) RS485
const byte RS485_MAX_LEN        = 20;        // buf len to hold the longest possible RS485 msg incl to, from, CRC.  16 as of 3/3/23.
const byte RS485_LEN_OFFSET     =  0;        // first byte of message is always total message length in bytes
const byte RS485_TYPE_OFFSET    =  1;        // second byte of message is the type of message such as 'C' for Credit
const byte RS845_PAYLOAD_OFFSET =  2;        // third byte of message is the first byte of the payload, i.e. the data
// Note also that the LAST byte of every message is a CRC8 checksum of all bytes except the last.

// *** ARDUINO PIN NUMBERS:
// *** STANDARD I/O PORT PIN NUMBERS ***
const byte PIN_IN_MEGA_RX0             =  0;  // Serial 0 receive PC Serial monitor
const byte PIN_OUT_MEGA_TX0            =  1;  // Serial 0 transmit PC Serial monitor
const byte PIN_IN_MEGA_RX1             = 19;  // Serial 1 receive NOT NEEDED FOR 2004 LCD (previosly used for FRAM3)
const byte PIN_OUT_MEGA_TX1            = 18;  // Serial 1 transmit 2004 LCD
const byte PIN_IN_MEGA_RX2             = 17;  // Serial 2 receive RS485 network
const byte PIN_OUT_MEGA_TX2            = 16;  // Serial 2 transmit RS485 network
const byte PIN_IN_MEGA_RX3             = 15;  // Serial 3 receive A_LEG Legacy, A_OCC WAV Trigger
const byte PIN_OUT_MEGA_TX3            = 14;  // Serial 3 transmit A_LEG Legacy, A_OCC WAV Trigger
const byte PIN_IO_MEGA_SDA             = 20;  // I2C (also pin 44)
const byte PIN_IO_MEGA_SCL             = 21;  // I2C (also pin 43)
const byte PIN_IO_MEGA_MISO            = 50;  // SPI Master In Slave Out (MISO) a.k.a. Controller In Peripheral Out (CIPO)
const byte PIN_IO_MEGA_MOSI            = 51;  // SPI Master Out Slave In (MOSI) a.k.a. Controller Out Peripheral In (COPI)
const byte PIN_IO_MEGA_SCK             = 52;  // SPI Clock
const byte PIN_IO_MEGA_SS              = 53;  // SPI Slave Select (SS) a.k.a. Chip Select (CS).  Used for the 256KB/512KB FRAM chip.

// *** COMMUNICATION PIN NUMBERS ***
// Tx Enable was previously pin 4 but that's a PWM which we need for coils, so moved to pin 22
const byte PIN_OUT_RS485_TX_ENABLE     =  22;  // Output: set HIGH when in RS485 transmit mode, LOW when not transmitting
// My custom-built train-layout RS-485 boards included Tx and Rx LEDs, but I'm using generic boards on Screamo.
// It would be fun to add the LED boards to Screamo just for visual effect.
//const byte PIN_OUT_RS485_TX_LED        =  5;  // Output: set HIGH to turn on BLUE LED when RS485 is TRANSMITTING data
//const byte PIN_OUT_RS485_RX_LED        =  6;  // Output: set HIGH to turn on YELLOW when RS485 is RECEIVING data

// *** MISC HARDWARE CONNECTION PINS ***
/*
const byte PIN_OUT_SPEAKER             =  7;  // Output: Piezo buzzer connects positive here
const byte PIN_IN_WAV_STATUS           = 10;  // Input: LOW when OCC WAV Trigger track is playing, else HIGH.
const byte PIN_IO_FRAM_CS              = 53;  // Output: FRAM  chip select.
*/

const byte PIN_OUT_LED                 = 13;  // Built-in LED always pin 13

#endif
