// PINBALL_CONSTS.H Rev: 02/04/26b.

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

// *** GAME STYLES (player selection at game start) ***
const byte STYLE_NONE      = 0;
const byte STYLE_ORIGINAL  = 1;
const byte STYLE_ENHANCED  = 2;
const byte STYLE_IMPULSE   = 3;

// *** GAME PHASES (system state machine) ***
const byte PHASE_ATTRACT      = 0;
const byte PHASE_STARTUP      = 1;
const byte PHASE_BALL_READY   = 2;
const byte PHASE_BALL_IN_PLAY = 3;
const byte PHASE_GAME_OVER    = 4;
const byte PHASE_TILT         = 5;
const byte PHASE_DIAGNOSTIC   = 6;
const byte PHASE_BALL_SEARCH  = 7;

// *** ENHANCED GAME MODES (game-within-a-game) ***
const byte MODE_NONE        = 0;
const byte MODE_BUMPER_CARS = 1;
const byte MODE_ROLL_A_BALL = 2;
const byte MODE_GOBBLE_HOLE = 3;

// Mode default times (seconds):
const byte MODE_1_DEFAULT_TIME = 45;  // Bumper cars
const byte MODE_2_DEFAULT_TIME = 45;  // Roll-A-Ball
const byte MODE_3_DEFAULT_TIME = 45;  // Gobble Hole Shooting Gallery

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
const byte RS485_TYPE_NO_MESSAGE                    =  0;  // No message.
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

// Start button tap detection states
const byte START_TAP_IDLE      = 0;  // No detection in progress
const byte START_TAP_WAITING   = 1;  // Waiting for possible 2nd tap (within 500ms)
const byte START_TAP_ORIGINAL  = 2;  // Original style selected (500ms expired, no 2nd tap)
const byte START_TAP_ENHANCED  = 3;  // Enhanced style selected (2nd tap within 500ms)
const byte START_TAP_IMPULSE   = 4;  // Impulse style (2nd tap after 500ms, before first point)

// Tsunami WAV Trigger settings incl. default values for out-of-range EEPROM reads
const int8_t TSUNAMI_GAIN_DB_DEFAULT    = -10;  // Master volume default
const int8_t TSUNAMI_GAIN_DB_MIN        = -40;
const int8_t TSUNAMI_GAIN_DB_MAX        =   0;
const int8_t TSUNAMI_CAT_GAIN_DEFAULT   =   0;  // Category offset default (voice, SFX, music vs. master)
const int8_t TSUNAMI_CAT_GAIN_MIN       = -20;
const int8_t TSUNAMI_CAT_GAIN_MAX       =  20;
const int8_t TSUNAMI_DUCKING_DB_DEFAULT = -20;  // Ducking level default

// ************************************************************************
// ************************** EEPROM ADDRESSES ****************************
// ************************************************************************
// All persistent storage addresses for game settings and state

// Last game score - 2 bytes
const int EEPROM_ADDR_LAST_SCORE              =  0;  // Last game score (0..999)

// Audio settings - 1 byte each
const int EEPROM_ADDR_TSUNAMI_GAIN            = 10;  // Master volume (-40 to 0 dB)
const int EEPROM_ADDR_TSUNAMI_GAIN_VOICE      = 11;  // Voice offset (-20 to +20 dB)
const int EEPROM_ADDR_TSUNAMI_GAIN_SFX        = 12;  // SFX offset (-20 to +20 dB)
const int EEPROM_ADDR_TSUNAMI_GAIN_MUSIC      = 13;  // Music offset (-20 to +20 dB)
const int EEPROM_ADDR_TSUNAMI_DUCK_DB         = 14;  // Ducking level (-40 to 0 dB)

// Music theme settings - 1 byte each
const int EEPROM_ADDR_THEME                   = 20;  // Primary theme (0=Circus, 1=Surf)
const int EEPROM_ADDR_LAST_CIRCUS_SONG_PLAYED = 21;  // Last Circus track index
const int EEPROM_ADDR_LAST_SURF_SONG_PLAYED   = 22;  // Last Surf track index
const int EEPROM_ADDR_LAST_MODE_PLAYED        = 25;  // Last game mode played

// Game settings - 1 byte each
const int EEPROM_ADDR_BALL_SAVE_TIME          = 30;  // Ball save duration (seconds)
const int EEPROM_ADDR_MODE_1_TIME             = 31;  // Mode 1 time limit (seconds)
const int EEPROM_ADDR_MODE_2_TIME             = 32;  // Mode 2 time limit (seconds)
const int EEPROM_ADDR_MODE_3_TIME             = 33;  // Mode 3 time limit (seconds)
const int EEPROM_ADDR_MODE_4_TIME             = 34;  // Mode 4 time limit (seconds)
const int EEPROM_ADDR_MODE_5_TIME             = 35;  // Mode 5 time limit (seconds)
const int EEPROM_ADDR_MODE_6_TIME             = 36;  // Mode 6 time limit (seconds)
const int EEPROM_ADDR_GAME_TIMEOUT            = 37;  // Inactivity before game ends automatically (minutes)

// Replay scores (Original/Impulse style) - 2 bytes each
const int EEPROM_ADDR_ORIGINAL_REPLAY_1       = 40;  // Replay threshold 1
const int EEPROM_ADDR_ORIGINAL_REPLAY_2       = 42;  // Replay threshold 2
const int EEPROM_ADDR_ORIGINAL_REPLAY_3       = 44;  // Replay threshold 3
const int EEPROM_ADDR_ORIGINAL_REPLAY_4       = 46;  // Replay threshold 4
const int EEPROM_ADDR_ORIGINAL_REPLAY_5       = 48;  // Replay threshold 5

// Replay scores (Enhanced style) - 2 bytes each
const int EEPROM_ADDR_ENHANCED_REPLAY_1       = 50;  // Replay threshold 1
const int EEPROM_ADDR_ENHANCED_REPLAY_2       = 52;  // Replay threshold 2
const int EEPROM_ADDR_ENHANCED_REPLAY_3       = 54;  // Replay threshold 3
const int EEPROM_ADDR_ENHANCED_REPLAY_4       = 56;  // Replay threshold 4
const int EEPROM_ADDR_ENHANCED_REPLAY_5       = 58;  // Replay threshold 5

// ********************************
// ***** AUDIO TRACK CONSTANTS ****
// ********************************

// *** AUDIO FLAGS ***
const byte AUDIO_FLAG_NONE     = 0x00;
const byte AUDIO_FLAG_LOOP     = 0x01;

// *** AUDIO PRIORITY (for COM tracks) ***
const byte AUDIO_PRIORITY_LOW  = 1;  // Routine callouts (compliments, drain comments)
const byte AUDIO_PRIORITY_MED  = 2;  // Important but interruptible (player up, ball saved)
const byte AUDIO_PRIORITY_HIGH = 3;  // Critical (mode end, tilt, awards)

// *** FIXED TRACK NUMBERS (used directly in code) ***
// Diagnostics
const unsigned int TRACK_DIAG_ENTER             =  101;
const unsigned int TRACK_DIAG_EXIT              =  102;
const unsigned int TRACK_DIAG_CRITICAL_ERROR    =  103;
const unsigned int TRACK_DIAG_SWITCH_CLOSE      =  104;
const unsigned int TRACK_DIAG_SWITCH_OPEN       =  105;
const unsigned int TRACK_DIAG_LAMPS             =  111;
const unsigned int TRACK_DIAG_SWITCHES          =  112;
const unsigned int TRACK_DIAG_COILS             =  113;
const unsigned int TRACK_DIAG_MOTORS            =  114;
const unsigned int TRACK_DIAG_MUSIC             =  115;
const unsigned int TRACK_DIAG_SFX               =  116;
const unsigned int TRACK_DIAG_COMMENTS          =  117;

// Tilt
const unsigned int TRACK_TILT_BUZZER            =  211;

// Start sequence
const unsigned int TRACK_BALL_MISSING           =  301;  // There's a ball missing
const unsigned int TRACK_BALL_IN_LIFT           =  302;  // Press the ball lift rod
const unsigned int TRACK_SHOOT_BALLS_IN_LANE    =  303;  // Shoot any balls in shooter lane; not used at this time
const unsigned int TRACK_BALL_STILL_MISSING     =  304;  // There's still a ball missing
const unsigned int TRACK_START_REJECT_HORN      =  311;
const unsigned int TRACK_START_P1_OK            =  351;  // Okay kid, you're in
const unsigned int TRACK_START_WILDER           =  352;  // Press start again for a wilder ride (unused; was for original->enhanced)
const unsigned int TRACK_START_MORE_FRIENDS     =  353;  // Keep pressin' Start if ya wanna admit more of your little friends
const unsigned int TRACK_START_P2               =  354;  // 2nd guest, c'mon in
const unsigned int TRACK_START_P3               =  355;  // 3rd guest, you're in
const unsigned int TRACK_START_P4               =  356;  // 4th guest, through the turnstile
const unsigned int TRACK_START_PARK_FULL        =  357;  // The park's full, no more admissions
const unsigned int TRACK_START_SCREAM           =  401;  // Scream SFX played at beginning of game
const unsigned int TRACK_START_HEY_GANG         =  402;  // Hey gang, let's ride the Screamo!
const unsigned int TRACK_START_LIFT_LOOP        =  403;  // Loopable 2-min coaster being hauled to top of first drop
const unsigned int TRACK_START_DROP             =  404;  // Roller coaster first big drop screaming/roaring SFX

// Player/Ball announcements (add player/ball number - 1 to get track)
const unsigned int TRACK_PLAYER_BASE            =  451;  // 451 + (playerNum - 1) (i.e. P2=452, P3=453, P4=454)
const unsigned int TRACK_BALL_BASE              =  461;  // 461 + (ballNum - 1) (i.e. Ball 2=462, Ball 3=463, Ball 4=464, Ball 5=465)

// Shoot prompts
const unsigned int TRACK_SHOOT_LIFT_ROD         =  611;  // Press the ball lift rod so we can start a game

// Ball saved special case
const unsigned int TRACK_BALL_SAVED_MODE_END    =  641;  // Here's your ball back if ball returned at end of timed mode

const unsigned int TRACK_SHOOT_AGAIN            =  660;  // Used with Extra Ball "Shoot again"; also part of 651-662 Ball Saved urgent prompts
const unsigned int TRACK_LOCK_1                 =  671;  // First ball locked
const unsigned int TRACK_LOCK_2                 =  672;  // Second ball locked
const unsigned int TRACK_MULTIBALL_START        =  674;  // Multiball! All rides are running!
const unsigned int TRACK_BUMPERS_DOUBLE         =  675;  // Every bumper worth double during multiball (not used)
const unsigned int TRACK_SPECIAL_LIT            =  811;  // Special is lit! (above Gobble Hole)
const unsigned int TRACK_REPLAY                 =  812;  // Replay!
const unsigned int TRACK_3_IN_A_ROW             =  821;  // You lit up 3 in a row! Replay!
const unsigned int TRACK_4_CORNERS              =  822;  // You hit all 4 corners! Five replays!
const unsigned int TRACK_1_2_3                  =  823;  // You hit 1, 2, and 3! Twenty replays!
const unsigned int TRACK_5_BALLS_IN_GOBBLE      =  824;  // All 5 balls in the Gobble Hole! Replay!
const unsigned int TRACK_SPELLED_SCREAMO        =  831;  // You spelled S-C-R-E-A-M-O!
const unsigned int TRACK_EXTRA_BALL_AWARDED     =  841;  // "EXTRA BALL!" Used when Extra Ball is earned, not collected
const unsigned int TRACK_EXTRA_BALL_COLLECTED   =  842;  // "Here's another ball! Send it!" Used when Extra Ball is collected

// Mode common
const unsigned int TRACK_MODE_STINGER_START     = 1001;  // School bell ringing
const unsigned int TRACK_MODE_JACKPOT           = 1002;  // Jackpot!
const unsigned int TRACK_MODE_HURRY             = 1003;  // Ten seconds left!
const unsigned int TRACK_MODE_COUNTDOWN         = 1004;  // SFX 10-second ticking timer
const unsigned int TRACK_MODE_TIME_UP           = 1005;  // Time's up!
const unsigned int TRACK_MODE_STINGER_END       = 1006;  // Factory whistle

// Mode intros
const unsigned int TRACK_MODE_BUMPER_INTRO      = 1101;  // Let's ride the Bumper Cars
const unsigned int TRACK_MODE_BUMPER_REMIND     = 1111;  // Keep smashing the bumpers
const unsigned int TRACK_MODE_BUMPER_ACHIEVED   = 1197;  // Bell ding-ding-ding!
const unsigned int TRACK_MODE_RAB_INTRO         = 1201;  // Let's play Roll-A-Ball
const unsigned int TRACK_MODE_RAB_REMIND        = 1211;  // Keep rolling over the hats
const unsigned int TRACK_MODE_RAB_ACHIEVED      = 1297;  // Ta da
const unsigned int TRACK_MODE_GOBBLE_INTRO      = 1301;  // Let's visit the Shooting Gallery
const unsigned int TRACK_MODE_GOBBLE_REMIND     = 1311;  // Keep shooting at the Gobble Hole
const unsigned int TRACK_MODE_GOBBLE_HIT        = 1321;  // Slide whistle down (only one SFX track for Gobble Hole hit since it's very rare)
const unsigned int TRACK_MODE_GOBBLE_ACHIEVED   = 1397;  // Applause

// *** TRACK RANGES (for random selection) ***
const unsigned int TRACK_TILT_WARNING_FIRST     =  201;  // First of several tilt warning announcements
const unsigned int TRACK_TILT_WARNING_LAST      =  205;
const unsigned int TRACK_TILT_COM_FIRST         =  212;  // First of many tilt announcements
const unsigned int TRACK_TILT_COM_LAST          =  216;
const unsigned int TRACK_START_REJECT_FIRST     =  312;  // First of several trying to start with no credits announcements
const unsigned int TRACK_START_REJECT_LAST      =  330;
const unsigned int TRACK_BALL1_COM_FIRST        =  511;  // First of many P2-4, B1 announcements i.e. "Climb aboard", "Fire away", etc.
const unsigned int TRACK_BALL1_COM_LAST         =  519;
const unsigned int TRACK_BALL5_COM_FIRST        =  531;  // First of many B5 announcements i.e. "Last chance", "Final ride", etc.
const unsigned int TRACK_BALL5_COM_LAST         =  540;
const unsigned int TRACK_GAME_OVER_FIRST        =  551;  // First of many Game Over announcements
const unsigned int TRACK_GAME_OVER_LAST         =  577;
const unsigned int TRACK_SHOOT_FIRST            =  612;  // First of several "Shoot the ball" prompts
const unsigned int TRACK_SHOOT_LAST             =  620;
const unsigned int TRACK_BALL_SAVED_FIRST       =  631;  // First of many Ball Saved announcements
const unsigned int TRACK_BALL_SAVED_LAST        =  636;
const unsigned int TRACK_BALL_SAVED_URG_FIRST   =  651;  // First of many urgent Ball Saved announcements "Hurry, shoot again!"
const unsigned int TRACK_BALL_SAVED_URG_LAST    =  662;
const unsigned int TRACK_COMPLIMENT_FIRST       =  701;  // First of many Compliment announcements "Good shot" etc.
const unsigned int TRACK_COMPLIMENT_LAST        =  714;
const unsigned int TRACK_DRAIN_FIRST            =  721;  // First of many Drain comments "Kid not that way!" etc.
const unsigned int TRACK_DRAIN_LAST             =  748;

const unsigned int TRACK_MODE_BUMPER_HIT_FIRST  = 1121;  // First of many Car Honks
const unsigned int TRACK_MODE_BUMPER_HIT_LAST   = 1133;
const unsigned int TRACK_MODE_BUMPER_MISS_FIRST = 1141;  // First of many Car Crashes
const unsigned int TRACK_MODE_BUMPER_MISS_LAST  = 1148;
const unsigned int TRACK_MODE_RAB_HIT_FIRST     = 1221;  // First of many Bowling Strikes
const unsigned int TRACK_MODE_RAB_HIT_LAST      = 1225;
const unsigned int TRACK_MODE_RAB_MISS_FIRST    = 1241;  // First of many Glass Breaking
const unsigned int TRACK_MODE_RAB_MISS_LAST     = 1254;
const unsigned int TRACK_MODE_GOBBLE_MISS_FIRST = 1341;  // First of many Ricochets
const unsigned int TRACK_MODE_GOBBLE_MISS_LAST  = 1348;

const unsigned int TRACK_MUSIC_CIRCUS_FIRST     = 2001;
const unsigned int TRACK_MUSIC_CIRCUS_LAST      = 2019;
const unsigned int TRACK_MUSIC_SURF_FIRST       = 2051;
const unsigned int TRACK_MUSIC_SURF_LAST        = 2068;

// *** SPECIAL CONSTANTS ***
const unsigned int TRACK_LIFT_LOOP_SECONDS      =  120;  // Loop 0403 before this expires
const byte NUM_CIRCUS_TRACKS                    =   19;  // 2001-2019
const byte NUM_SURF_TRACKS                      =   18;  // 2051-2068

// *** SETTINGS AND EEPROM CATEGORIES ***
// These constants define the three categories of settings available in the Diagnostics "Settings" menu:
const byte SETTINGS_CAT_GAME        = 0;  // General game settings (theme, ball save time, mode timers, etc.)
const byte SETTINGS_CAT_ORIG_REPLAY = 1;  // Replay score thresholds for Original/Impulse style (inferred, not actually referenced in code.)
const byte SETTINGS_CAT_ENH_REPLAY  = 2;  // Replay score thresholds for Enhanced style.
const byte NUM_SETTINGS_CATEGORIES  = 3;  // The total number of categories (3).

// Game settings parameter indices
const byte GAME_SETTING_THEME      = 0;
const byte GAME_SETTING_BALL_SAVE  = 1;
const byte GAME_SETTING_MODE_1     = 2;
const byte GAME_SETTING_MODE_2     = 3;
const byte GAME_SETTING_MODE_3     = 4;
const byte GAME_SETTING_MODE_4     = 5;
const byte GAME_SETTING_MODE_5     = 6;
const byte GAME_SETTING_MODE_6     = 7;
const byte GAME_SETTING_TIMEOUT    = 8;
const byte NUM_GAME_SETTINGS       = 9;

// Replay score count
const byte NUM_REPLAY_SCORES = 5;

// Theme selection values
const byte THEME_CIRCUS = 0;
const byte THEME_SURF   = 1;

// *** DEVICE (COIL/MOTOR) COUNTS AND INDICES ***
// Master has 14 devices (coils/motors), Slave has 8
const byte NUM_DEVS_MASTER              = 14;
const byte NUM_DEVS_SLAVE               =  8;

// Master device indices (0-13)
const byte DEV_IDX_POP_BUMPER           =  0;
const byte DEV_IDX_KICKOUT_LEFT         =  1;
const byte DEV_IDX_KICKOUT_RIGHT        =  2;
const byte DEV_IDX_SLINGSHOT_LEFT       =  3;
const byte DEV_IDX_SLINGSHOT_RIGHT      =  4;
const byte DEV_IDX_FLIPPER_LEFT         =  5;
const byte DEV_IDX_FLIPPER_RIGHT        =  6;
const byte DEV_IDX_BALL_TRAY_RELEASE    =  7;  // Original
const byte DEV_IDX_SELECTION_UNIT       =  8;  // Sound FX only
const byte DEV_IDX_RELAY_RESET          =  9;  // Sound FX only
const byte DEV_IDX_BALL_TROUGH_RELEASE  = 10;  // New up/down post
const byte DEV_IDX_MOTOR_SHAKER         = 11;  // 12vdc
const byte DEV_IDX_KNOCKER              = 12;
const byte DEV_IDX_MOTOR_SCORE          = 13;  // 50vac; sound FX only

// Slave device indices (defined in Screamo_Slave.ino - not shared)

const byte MOTOR_SHAKER_POWER_MIN       =  70;  // Minimum power needed to start shaker motor
const byte MOTOR_SHAKER_POWER_MAX       = 140;  // Maximum power before hats trigger

// *** LAMP COUNTS ***
// Master has 47 playfield/cabinet lamps, Slave has 29 head lamps
const byte NUM_LAMPS_MASTER             = 47;
const byte NUM_LAMPS_SLAVE              = 29;

// *** LAMP INDICES ***
const byte LAMP_IDX_GI_LEFT_TOP       =  0;
const byte LAMP_IDX_GI_LEFT_CENTER_1  =  1;  // Above left kickout
const byte LAMP_IDX_GI_LEFT_CENTER_2  =  2;  // Below left kickout
const byte LAMP_IDX_GI_LEFT_BOTTOM    =  3;  // Left slingshot
const byte LAMP_IDX_GI_RIGHT_TOP      =  4;
const byte LAMP_IDX_GI_RIGHT_CENTER_1 =  5;  // Above right kickout
const byte LAMP_IDX_GI_RIGHT_CENTER_2 =  6;  // Below right kickout
const byte LAMP_IDX_GI_RIGHT_BOTTOM   =  7;  // Right slingshot
const byte LAMP_IDX_S                 =  8;  // "S" bumper lamp
const byte LAMP_IDX_C                 =  9;  // "C" bumper lamp
const byte LAMP_IDX_R                 = 10;  // "R" bumper lamp
const byte LAMP_IDX_E                 = 11;  // "E" bumper lamp
const byte LAMP_IDX_A                 = 12;  // "A" bumper lamp
const byte LAMP_IDX_M                 = 13;  // "M" bumper lamp
const byte LAMP_IDX_O                 = 14;  // "O" bumper lamp
const byte LAMP_IDX_WHITE_1           = 15;
const byte LAMP_IDX_WHITE_2           = 16;
const byte LAMP_IDX_WHITE_3           = 17;
const byte LAMP_IDX_WHITE_4           = 18;
const byte LAMP_IDX_WHITE_5           = 19;
const byte LAMP_IDX_WHITE_6           = 20;
const byte LAMP_IDX_WHITE_7           = 21;
const byte LAMP_IDX_WHITE_8           = 22;
const byte LAMP_IDX_WHITE_9           = 23;
const byte LAMP_IDX_RED_1             = 24;
const byte LAMP_IDX_RED_2             = 25;
const byte LAMP_IDX_RED_3             = 26;
const byte LAMP_IDX_RED_4             = 27;
const byte LAMP_IDX_RED_5             = 28;
const byte LAMP_IDX_RED_6             = 29;
const byte LAMP_IDX_RED_7             = 30;
const byte LAMP_IDX_RED_8             = 31;
const byte LAMP_IDX_RED_9             = 32;
const byte LAMP_IDX_HAT_LEFT_TOP      = 33;
const byte LAMP_IDX_HAT_LEFT_BOTTOM   = 34;
const byte LAMP_IDX_HAT_RIGHT_TOP     = 35;
const byte LAMP_IDX_HAT_RIGHT_BOTTOM  = 36;
const byte LAMP_IDX_KICKOUT_LEFT      = 37;
const byte LAMP_IDX_KICKOUT_RIGHT     = 38;
const byte LAMP_IDX_SPECIAL           = 39;  // Special When Lit above gobble hole
const byte LAMP_IDX_GOBBLE_1          = 40;  // "1" BALL IN HOLE
const byte LAMP_IDX_GOBBLE_2          = 41;  // "2" BALL IN HOLE
const byte LAMP_IDX_GOBBLE_3          = 42;  // "3" BALL IN HOLE
const byte LAMP_IDX_GOBBLE_4          = 43;  // "4" BALL IN HOLE
const byte LAMP_IDX_GOBBLE_5          = 44;  // "5" BALL IN HOLE
const byte LAMP_IDX_SPOT_NUMBER_LEFT  = 45;
const byte LAMP_IDX_SPOT_NUMBER_RIGHT = 46;

const byte LAMP_GROUP_NONE              =  0;
const byte LAMP_GROUP_GI                =  1;
const byte LAMP_GROUP_BUMPER            =  2;
const byte LAMP_GROUP_WHITE             =  3;
const byte LAMP_GROUP_RED               =  4;
const byte LAMP_GROUP_HAT               =  5;
const byte LAMP_GROUP_KICKOUT           =  6;
const byte LAMP_GROUP_GOBBLE            =  7;

// *** SWITCH COUNTS ***
// Master has 40 switches, Slave has 2 (credit empty/full)
const byte NUM_SWITCHES_MASTER          = 40;
const byte NUM_SWITCHES_SLAVE           =  2;

// Define array index constants - this list is rather arbitrary and doesn't relate to device number/pin numbers:
// Flipper buttons are direct-wired to Arduino pins for faster response, so not included here.
// CABINET SWITCHES:
const byte SWITCH_IDX_START_BUTTON         =  0;
const byte SWITCH_IDX_DIAG_1               =  1;  // Back
const byte SWITCH_IDX_DIAG_2               =  2;  // Minus/Left
const byte SWITCH_IDX_DIAG_3               =  3;  // Plus/Right
const byte SWITCH_IDX_DIAG_4               =  4;  // Select
const byte SWITCH_IDX_KNOCK_OFF            =  5;  // Quick press adds a credit; long press forces software reset (Slave and Master.)
const byte SWITCH_IDX_COIN_MECH            =  6;
const byte SWITCH_IDX_5_BALLS_IN_TROUGH    =  7;  // Detects if there are 5 balls present in the trough
const byte SWITCH_IDX_BALL_IN_LIFT         =  8;  // (New) Ball present at bottom of ball lift
const byte SWITCH_IDX_TILT_BOB             =  9;
// PLAYFIELD SWITCHES:
const byte SWITCH_IDX_BUMPER_S             = 10;  // 'S' bumper switch
const byte SWITCH_IDX_BUMPER_C             = 11;  // 'C' bumper switch
const byte SWITCH_IDX_BUMPER_R             = 12;  // 'R' bumper switch
const byte SWITCH_IDX_BUMPER_E             = 13;  // 'E' bumper switch
const byte SWITCH_IDX_BUMPER_A             = 14;  // 'A' bumper switch
const byte SWITCH_IDX_BUMPER_M             = 15;  // 'M' bumper switch
const byte SWITCH_IDX_BUMPER_O             = 16;  // 'O' bumper switch
const byte SWITCH_IDX_KICKOUT_LEFT         = 17;
const byte SWITCH_IDX_KICKOUT_RIGHT        = 18;
const byte SWITCH_IDX_SLINGSHOT_LEFT       = 19;  // Two switches wired in parallel
const byte SWITCH_IDX_SLINGSHOT_RIGHT      = 20;  // Two switches wired in parallel
const byte SWITCH_IDX_HAT_LEFT_TOP         = 21;
const byte SWITCH_IDX_HAT_LEFT_BOTTOM      = 22;
const byte SWITCH_IDX_HAT_RIGHT_TOP        = 23;
const byte SWITCH_IDX_HAT_RIGHT_BOTTOM     = 24;
// Note that there is no "LEFT_SIDE_TARGET_1" on the playfield; starting left side targets at 2 so they match right-side target numbers.
const byte SWITCH_IDX_LEFT_SIDE_TARGET_2   = 25;  // Long narrow side target near top left
const byte SWITCH_IDX_LEFT_SIDE_TARGET_3   = 26;  // Upper switch above left kickout
const byte SWITCH_IDX_LEFT_SIDE_TARGET_4   = 27;  // Lower switch above left kickout
const byte SWITCH_IDX_LEFT_SIDE_TARGET_5   = 28;  // Below left kickout
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_1  = 29;  // Top right just below ball gate
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_2  = 30;  // Long narrow side target near top right
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_3  = 31;  // Upper switch above right kickout
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_4  = 32;  // Lower switch above right kickout
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_5  = 33;  // Below right kickout
const byte SWITCH_IDX_GOBBLE               = 34;
const byte SWITCH_IDX_DRAIN_LEFT           = 35;  // Left drain switch index in Centipede input shift register
const byte SWITCH_IDX_DRAIN_CENTER         = 36;  // Center drain switch index in Centipede input shift register
const byte SWITCH_IDX_DRAIN_RIGHT          = 37;  // Right drain switch index in Centipede input shift register
// Flipper buttons now arrive via Centipede inputs (entries at the end of switchParm[]).
const byte SWITCH_IDX_FLIPPER_LEFT_BUTTON  = 38;
const byte SWITCH_IDX_FLIPPER_RIGHT_BUTTON = 39;
const byte NUM_SWITCHES = 40;

#endif