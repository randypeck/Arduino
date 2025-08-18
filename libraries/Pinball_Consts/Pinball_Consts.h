// TRAIN_CONSTS_GLOBAL.H Rev: 02/17/24.
// 02/17/23: Updated pin number for OCC WAV Trigger status input
// 03/21/23: Added RS485_MAS_ALL_ROUTE_EXT_CONT_OFFSET for send/getMAStoALLRoute()
// 03/02/23: Added LOCO_ID_POWERMASTER_n consts for use by LEG.
// 02/15/23: Changed CONTROL_TYPE_xx and LOCO_TYPE_xx to DEV_TYPE_xx
// 02/03/23: Added const chars for Loco Ref fields i.e. CONTROL_TYPE_LEGACY vs CONTROL_TYPE_TMCC
// 01/15/23: Changed FRAM_FIELDS_DEADLOCK from 9 to 11 (needed by Deadlock BW02.)
// 12/27/22: Added BX as Block Direction (in addition to BE and BW) for Deadlocks to indicate either direction is a risk.
//           Note that BX will *only* be used in the Deadlocks table, and never in Routes.
// 12/21/22: Changed FRAM Route EB and WB records to be contiguous, so eliminated some global consts used by Route Reference.
// Declares and defines all constants that are global to all (or nearly all) Arduino modules.
// Any non-const global variables should be declared here as extern, and defined in the .cpp file.
// 06/17/21 Removed Train Progress and Delayed Action constants from FRAM addresses, since they're going to be stored on the heap!
// 07/09/22 Added BR = Beginnnig-of-Route as a Route Element (for messages only; not part of Route Reference or Train Progress.)
//          12/21/22: Probably don't need BR since we're sending routes via Rec Num rather than piecemeal via RS-485.

// Header (.h) files should contain:
// * Header guards #ifndef/#define/#endif
// * Source code documentation i.e. purpose of parms, and return values of functions.
// * Type and constant DEFINITIONS.  Note this does not consume memory if not used.
// * External variable, structure, function, and object DECLARATIONS
// Large numbers of structure declarations should be split into separate headers.
// Struct declarations do not take up any memory, so no harm done if they get included with code that never defines them.
// Structure definitions can be in a corresponding .cpp file, *or* in the main source file (probably better for me.)
// enum and constants should be DEFINED not just declared in the .h file.

#ifndef PINBALL_CONSTS_H
#define PINBALL_CONSTS_H

#include <Arduino.h>  // Allows use of "byte" and other Arduino-specifics (eliminates 'byte' does not name a type compiler error.)

// *** ARDUINO DEVICE CONSTANTS: Here are all the different Arduinos and their "addresses" (ID numbers) for communication.
const byte ARDUINO_NUL =  0;              // Use this to initialize etc.
const byte ARDUINO_MAS =  1;              // Master Arduino (Main controller)
const byte ARDUINO_SLV =  2;              // Slave Arduino
const byte ARDUINO_ALL = 99;              // Master broadcasting to all i.e. mode change

// *** OPERATING MODES AND STATES:
const byte MODE_TOTAL      = 5;           // Five modes: MANUAL, REGISTER, AUTO, PARK, and POV
const byte MODE_UNDEFINED  = 0;
const byte MODE_MANUAL     = 1;
const byte MODE_REGISTER   = 2;
const byte MODE_AUTO       = 3;
const byte MODE_PARK       = 4;
const byte MODE_POV        = 5;           // Not yet supported
const byte STATE_UNDEFINED = 0;
const byte STATE_RUNNING   = 1;
const byte STATE_STOPPING  = 2;
const byte STATE_STOPPED   = 3;

// *** VARIOUS MAXIMUM VALUES ***
const byte TOTAL_BLOCKS           =  26;
const byte TOTAL_TURNOUTS         =  32;  // 30 connected, but 32 relays and we're using 32 bits in last-known.
const byte TOTAL_SENSORS          =  52;
const byte TOTAL_TRAINS           =  50;
const byte TOTAL_LEG_ACCY_RELAYS  =  16;  // 0..15 LEG accessory relays so far.

const byte LCD_WIDTH      = 20;  // 2004 (20 char by 4 lines) LCD display
const byte ALPHA_WIDTH    =  8;  // Width of 8-char alphanumeric display and of fields that use it such as train names, questions, etc.
const byte RESTRICT_WIDTH =  5;  // Locomotive 'Restrictions' field length used by Loco_Reference.h/.cpp

// routeReferenceStruct is only used in Route_Reference.h/.cpp, but routeElement is used in several modules so global here.
struct routeElement {
  byte routeRecType;  // i.e. 2 = BW = "Block West"
  byte routeRecVal;   // i.e. 4 (if BW04, for example)
};


// Serial port speed
// 6/5/22: These should really be i.e. SERIAL_SPEED_MONITOR, SERIAL_SPEED_2004LCD, SERIAL_SPEED_RS485, SERIAL_SPEED_LEGACY_BASE
const long unsigned int SERIAL0_SPEED = 115200;  // Serial port 0 is used for serial monitor
const long unsigned int SERIAL1_SPEED = 115200;  // Serial port 1 is Digole 2004 LCD display
const long unsigned int SERIAL2_SPEED = 115200;  // Serial port 2 is the RS485 communications bus
const long unsigned int SERIAL3_SPEED =   9600;  // Serial port 3 is LEG Legacy xface, and OCC WAV Trigger xface; both 9600.

// Note that the serial input buffer is only 64 bytes, which means that we need to keep emptying it since there
// will be many commands between Arduinos, even though most may not be for THIS Arduino.  If the buffer overflows,
// then we will be totally screwed up (but it will be apparent in the checksum.)
const byte RS485_TRANSMIT    = HIGH;      // HIGH = 0x1.  How to set TX_CONTROL pin when we want to transmit RS485
const byte RS485_RECEIVE     = LOW;       // LOW = 0x0.  How to set TX_CONTROL pin when we want to receive (or NOT transmit) RS485
const byte RS485_MAX_LEN     = 20;        // buf len to hold the longest possible RS485 msg incl to, from, CRC.  16 as of 3/3/23.
const byte RS485_LEN_OFFSET  =  0;        // first byte of message is always total message length in bytes
const byte RS485_FROM_OFFSET =  1;        // second byte of message is the ID of the Arduino the message is coming from
const byte RS485_TO_OFFSET   =  2;        // third byte of message is the ID of the Arduino the message is addressed to
const byte RS485_TYPE_OFFSET =  3;        // fourth byte of message is the type of message such as M for Mode, S for Smoke, etc.
// Note also that the LAST byte of every message is a CRC8 checksum of all bytes except the last.

// These RS485 message offsets are specific to individual modules (though all are used by O-MAS).
// The 3-char name refers to FROM, TO, and MESSAGE TYPE.
// Confirmed okay as of 03/03/23.
const byte RS485_MAS_ALL_MODE_OFFSET               =  4;  // const byte i.e. MODE_MANUAL
const byte RS485_MAS_ALL_STATE_OFFSET              =  5;  // const byte i.e. STATE_RUNNING
const byte RS485_OCC_LEG_FAST_SLOW_OFFSET          =  4;  // char F|S
const byte RS485_OCC_LEG_SMOKE_ON_OFF_OFFSET       =  4;  // char S|N
const byte RS485_OCC_LEG_AUDIO_ON_OFF_OFFSET       =  4;  // char A|N
const byte RS485_OCC_LEG_DEBUG_ON_OFF_OFFSET       =  4;  // char D|N
const byte RS485_OCC_ALL_REGISTER_LOCO_NUM_OFFSET  =  4;  // byte 1..50
const byte RS485_OCC_ALL_REGISTER_BLOCK_NUM_OFFSET =  5;  // byte 1..26
const byte RS485_OCC_ALL_REGISTER_BLOCK_DIR_OFFSET =  6;  // const byte BE or BW
const byte RS485_MAS_ALL_ROUTE_LOCO_NUM_OFFSET     =  4;  // byte 1..50
const byte RS485_MAS_ALL_ROUTE_EXT_CONT_OFFSET     =  5;  // char E|C
const byte RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET      =  6;  // byte 0..(FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST - 1)
const byte RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET   =  8;  // 2-byte unsigned int SECONDS (not ms) to delay before starting Extension (stopped) route.
const byte RS485_BTN_MAS_BUTTON_NUM_OFFSET         =  4;  // byte 1..32 (30 connected turnouts, but 32 relays)
const byte RS485_MAS_ALL_SET_TURNOUT_NUM_OFFSET    =  4;  // byte 1..30 (30 connected turnouts, but 32 relays)
const byte RS485_MAS_ALL_SET_TURNOUT_DIR_OFFSET    =  5;  // char N|R
const byte RS485_MAS_SNS_SENSOR_NUM_OFFSET         =  4;  // byte 1..52
const byte RS485_SNS_ALL_SENSOR_NUM_OFFSET         =  4;  // byte 1..52
const byte RS485_SNS_ALL_SENSOR_TRIP_CLEAR_OFFSET  =  5;  // char T|C

// *** ARDUINO PIN NUMBERS:
// *** STANDARD I/O PORT PIN NUMBERS ***
// const byte PIN_IN_MEGA_RX0             =  0;  // Serial 0 receive PC Serial monitor
// const byte PIN_OUT_MEGA_TX0            =  1;  // Serial 0 transmit PC Serial monitor
// const byte PIN_IN_MEGA_RX1             = 19;  // Serial 1 receive NOT NEEDED FOR 2004 LCD (previosly used for FRAM3)
// const byte PIN_OUT_MEGA_TX1            = 18;  // Serial 1 transmit 2004 LCD
// const byte PIN_IN_MEGA_RX2             = 17;  // Serial 2 receive RS485 network
// const byte PIN_OUT_MEGA_TX2            = 16;  // Serial 2 transmit RS485 network
// const byte PIN_IN_MEGA_RX3             = 15;  // Serial 3 receive A_LEG Legacy, A_OCC WAV Trigger
// const byte PIN_OUT_MEGA_TX3            = 14;  // Serial 3 transmit A_LEG Legacy, A_OCC WAV Trigger
// const byte PIN_IO_MEGA_SDA             = 20;  // I2C (also pin 44)
// const byte PIN_IO_MEGA_SCL             = 21;  // I2C (also pin 43)
// const byte PIN_IO_MEGA_MISO            = 50;  // SPI Master In Slave Out (MISO) a.k.a. Controller In Peripheral Out (CIPO)
// const byte PIN_IO_MEGA_MOSI            = 51;  // SPI Master Out Slave In (MOSI) a.k.a. Controller Out Peripheral In (COPI)
// const byte PIN_IO_MEGA_SCK             = 52;  // SPI Clock
// const byte PIN_IO_MEGA_SS              = 53;  // SPI Slave Select (SS) a.k.a. Chip Select (CS).  Used for the 256KB/512KB FRAM chip.

// *** COMMUNICATION PIN NUMBERS ***
const byte PIN_OUT_RS485_RX_LED        =  6;  // Output: set HIGH to turn on YELLOW when RS485 is RECEIVING data
const byte PIN_OUT_RS485_TX_ENABLE     =  4;  // Output: set HIGH when in RS485 transmit mode, LOW when not transmitting
const byte PIN_OUT_RS485_TX_LED        =  5;  // Output: set HIGH to turn on BLUE LED when RS485 is TRANSMITTING data
const byte PIN_OUT_REQ_TX_BTN          =  8;  // O_BTN output pin pulled LOW when it wants to tell A-MAS a turnout button has been pressed.
const byte PIN_IN_REQ_TX_BTN           =  8;  // O_MAS input pin pulled LOW by A-BTN when it wants to tell A-MAS a turnout button has been pressed.
const byte PIN_OUT_REQ_TX_LEG          =  8;  // O_LEG output pin pulled LOW when it wants to send A_MAS a message (unused as of 2/24.)
const byte PIN_IN_REQ_TX_LEG           =  2;  // O_MAS input pin pulled LOW by A_LEG when it wants to send A_MAS a message (unused as of 2/24.)
const byte PIN_OUT_REQ_TX_SNS          =  8;  // O_SNS output pin pulled LOW when it wants to send A_MAS an occupancy sensor change message.
const byte PIN_IN_REQ_TX_SNS           =  3;  // O_MAS input pin pulled LOW by A_SNS when it wants to send A_MAS an occupancy sensor change message.

// *** QuadMEM HEAP MEMORY MODULE PIN NUMBERS ***
const byte PIN_OUT_XMEM_ENABLE         = 38;
const byte PIN_OUT_XMEM_BANK_BIT_0     = 42;
const byte PIN_OUT_XMEM_BANK_BIT_1     = 43;
const byte PIN_OUT_XMEM_BANK_BIT_2     = 44;

// *** MAS CONTROL PANEL MAS MODE CONTROL PINS ***
// Need to move pins along back row to make room for QuadRAM card.
// 6/29/22 CHANGED THESE 14 PINS FROM D23-D49 to ANALOG PINS A02-A15 == DIGITAL PINS D56-D69
const byte PIN_IN_ROTARY_AUTO          = 67;  // Changed D27 to A13 = D67. Input : MAS Rotary mode "Auto."  Pulled LOW
const byte PIN_IN_ROTARY_MANUAL        = 69;  // Changed D23 to A15 = D69. Input : MAS Rotary mode "Manual."  Pulled LOW
const byte PIN_IN_ROTARY_PARK          = 66;  // Changed D29 to A12 = D66. Input : MAS Rotary mode "Park."  Pulled LOW
const byte PIN_IN_ROTARY_POV           = 65;  // Changed D31 to A11 = D65. Input : MAS Rotary mode "P.O.V."  Pulled LOW
const byte PIN_IN_ROTARY_REGISTER      = 68;  // Changed D25 to A14 = D68. Input : MAS Rotary mode "Register."  Pulled LOW
const byte PIN_IN_ROTARY_START         = 64;  // Changed D33 to A10 = D64. Input : MAS Rotary "Start" button.  Pulled LOW
const byte PIN_IN_ROTARY_STOP          = 63;  // Changed D35 to A09 = D63. Input : MAS Rotary "Stop" button.  Pulled LOW
const byte PIN_OUT_ROTARY_LED_AUTO     = 58;  // Changed D45 to A04 = D58. Output: MAS Rotary Auto position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_MANUAL   = 60;  // Changed D41 to A06 = D60. Output: MAS Rotary Manual position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_PARK     = 57;  // Changed D47 to A03 = D57. Output: MAS Rotary Park position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_POV      = 56;  // Changed D49 to A02 = D56. Output: MAS Rotary P.O.V. position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_REGISTER = 59;  // Changed D43 to A05 = D59. Output: MAS Rotary Register position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_START    = 62;  // Changed D37 to A08 = D62. Output: MAS Rotary Start button internal LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_STOP     = 61;  // Changed D39 to A07 = D61. Output: MAS Rotary Stop button internal LED.  Pull LOW to turn on.

// *** OCC CONTROL PANEL OCC ROTARY ENCODER PIN NUMBERS ***
const byte PIN_IN_ROTARY_1             =  2;  // Input: OCC Rotary Encoder pin 1 of 2 (plus Select)
const byte PIN_IN_ROTARY_2             =  3;  // Input: OCC Rotary Encoder pin 2 of 2 (plus Select)
const byte PIN_IN_ROTARY_PUSH          = 19;  // Input: OCC Rotary Encoder "Select" (pushbutton) pin

// *** LEG CONTROL PANEL TOGGLE SWITCH PIN NUMBERS ***
// Need to move pins along back row to make room for QuadRAM card.
// 6/29/22 CHANGED THESE 8 PINS FROM D23-D37 TO ANALOG PINS A8-A15 == DIGITAL PINS D62-D69
const byte PIN_IN_PANEL_4_OFF          = 62;  // Change D37 to A08 = D62. Input: LEG Control panel #4 PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_4_ON           = 63;  // Change D35 to A09 = D63. Input: LEG Control panel #4 PowerMaster toggled up.  Pulled LOW.
const byte PIN_IN_PANEL_BLUE_OFF       = 66;  // Change D29 to A12 = D66. Input: LEG Control panel "Blue" PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_BLUE_ON        = 67;  // Change D27 to A13 = D67. Input: LEG Control panel "Blue" PowerMaster toggled up.  Pulled LOW.
const byte PIN_IN_PANEL_BROWN_OFF      = 68;  // Change D25 to A14 = D68. Input: LEG Control panel "Brown" PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_BROWN_ON       = 69;  // Change D23 to A15 = D69. Input: LEG Control panel "Brown" PowerMaster toggled up.  Pulled LOW.
const byte PIN_IN_PANEL_RED_OFF        = 64;  // Change D33 to A10 = D64. Input: LEG Control panel "Red" PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_RED_ON         = 65;  // Change D31 to A11 = D65. Input: LEG Control panel "Red" PowerMaster toggled up.  Pulled LOW.

// *** MISC HARDWARE CONNECTION PINS ***
const byte PIN_IO_HALT                 =  9;  // Output: Pull low to tell A-LEG to issue Legacy Emergency Stop FE FF FF
                                              // Input: Check if pulled low
const byte PIN_OUT_SPEAKER             =  7;  // Output: Piezo buzzer connects positive here
const byte PIN_IN_WAV_STATUS           = 10;  // Input: LOW when OCC WAV Trigger track is playing, else HIGH.
const byte PIN_OUT_LED                 = 13;  // Built-in LED always pin 13
const byte PIN_IO_FRAM_CS              = 53;  // Output: FRAM  chip select.

#endif
