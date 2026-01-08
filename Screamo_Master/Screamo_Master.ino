// Screamo_Master.INO Rev: 01/02/26
// 11/12/25: Moved Right Kickout from pin 13 to pin 44 to avoid MOSFET firing twice on power-up.  Don't use MOSFET on pin 13.
// 12/28/25: Changed flipper inputs from direct Arduino inputs to Centipede inputs.
// 01/07/26: Added "5 Balls in Trough" switch to Centipede inputs. All switches tested and working.

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
#include <Tsunami.h>
#include <avr/wdt.h>
#include <EEPROM.h>               // For saving and recalling score, volume, etc., to persist between power cycles.
#include <Pinball_Descriptions.h> 

const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 01/02/26";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which makes it a global.
// And no need to pass lcdString[] to any functions that use it!

// ******************************
// ***** EEPROM ADDRESS MAP *****
// ******************************

const int EEPROM_ADDR_SCORE                =  0;  // 2-byte unsigned addr to store 0..999 score (uses addr 0 and 1)

const int EEPROM_ADDR_TSUNAMI_GAIN         = 10;  // 1-byte signed: Overall gain (-40 to 0; default -10dB.)
const int EEPROM_ADDR_TSUNAMI_GAIN_VOICE   = 11;  // 1-byte signed: Voice gain OFFSET +/- from overall. Default 0dB.
const int EEPROM_ADDR_TSUNAMI_GAIN_SFX     = 12;  // 1-byte signed: SFX gain OFFSET +/- from overall. Default 0dB.
const int EEPROM_ADDR_TSUNAMI_GAIN_MUSIC   = 13;  // 1-byte signed: Music gain OFFSET +/- from overall. Default 0dB.
const int EEPROM_ADDR_TSUNAMI_DUCK_DB      = 14;  // 1-byte signed: Ducking gain OFFSET +/- from SFX and Music when Voice playing. Default -20dB.

const int EEPROM_ADDR_THEME                = 20;  // 1-byte unsigned: Callipe or Surf Rock theme (music) selection
const int EEPROM_ADDR_LAST_SONG_PLAYED     = 21;  // 1-byte unsigned: Last song number played (to avoid repeats)

const int EEPROM_ADDR_BALL_SAVE_TIME       = 30;  // 1-byte unsigned: Ball save time (0=off, 1-30 seconds) from first point scored that ball.
const int EEPROM_ADDR_HURRY_UP_1_TIME      = 31;  // 1-byte unsigned: Mode 1 time limit (in seconds)
const int EEPROM_ADDR_HURRY_UP_2_TIME      = 32;  // 1-byte unsigned: i.e. "Roll-A-Ball" mode etc.
const int EEPROM_ADDR_HURRY_UP_3_TIME      = 33;  // 1-byte unsigned
const int EEPROM_ADDR_HURRY_UP_4_TIME      = 34;  // 1-byte unsigned
const int EEPROM_ADDR_HURRY_UP_5_TIME      = 35;  // 1-byte unsigned
const int EEPROM_ADDR_HURRY_UP_6_TIME      = 36;  // 1-byte unsigned

const int EEPROM_ADDR_ORIGINAL_REPLAY_1    = 40;  // 2-byte unsigned: 1st replay score for Original/Impulse mode
const int EEPROM_ADDR_ORIGINAL_REPLAY_2    = 42;  // 2-byte unsigned
const int EEPROM_ADDR_ORIGINAL_REPLAY_3    = 44;  // 2-byte unsigned
const int EEPROM_ADDR_ORIGINAL_REPLAY_4    = 46;  // 2-byte unsigned
const int EEPROM_ADDR_ORIGINAL_REPLAY_5    = 48;  // 2-byte unsigned
const int EEPROM_ADDR_ENHANCED_REPLAY_1    = 50;  // 2-byte unsigned: 1st replay score for Enhanced mode
const int EEPROM_ADDR_ENHANCED_REPLAY_2    = 52;  // 2-byte unsigned
const int EEPROM_ADDR_ENHANCED_REPLAY_3    = 54;  // 2-byte unsigned
const int EEPROM_ADDR_ENHANCED_REPLAY_4    = 56;  // 2-byte unsigned
const int EEPROM_ADDR_ENHANCED_REPLAY_5    = 58;  // 2-byte unsigned

// **************************************
// ***** LAMP STRUCTS AND CONSTANTS *****
// **************************************

// Define a struct to store lamp parameters.
// Centipede #1 (0..63) is LAMP OUTPUTS
// Centipede #2 (64..127) is SWITCH INPUTS
// In this case, pinNum refers to Centipede #1 output shift register pin number (0..63).
struct LampParmStruct {
  byte pinNum;       // Centipede pin number for this lamp (0..63)
  byte groupNum;     // Group number for this lamp
  bool stateOn;      // true = lamp ON; false = lamp OFF
};

// Define array index constants - this list is rather arbitrary and doesn't relate to device number/pin number/relay numbers.
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

const byte NUM_LAMPS                  = 47;

const byte LAMP_GROUP_NONE            =  0;
const byte LAMP_GROUP_GI              =  1;
const byte LAMP_GROUP_BUMPER          =  2;
const byte LAMP_GROUP_WHITE           =  3;
const byte LAMP_GROUP_RED             =  4;
const byte LAMP_GROUP_HAT             =  5;
const byte LAMP_GROUP_KICKOUT         =  6;
const byte LAMP_GROUP_GOBBLE          =  7;

const bool LAMP_ON                    = true;  // Or maybe these should be LOW / HIGH?
const bool LAMP_OFF                   = false;

// Populate lampParm with pin numbers, groups, and initial state OFF
// Pin numbers range from 0 to 63 because these lamps are on Centipede #1 (outputs)
LampParmStruct lampParm[NUM_LAMPS] = {
  // pinNum, groupNum, stateOn
  { 52, LAMP_GROUP_GI,      LAMP_OFF }, // GI_LEFT_TOP       =  0
  { 34, LAMP_GROUP_GI,      LAMP_OFF }, // GI_LEFT_CENTER_1  =  1
  { 25, LAMP_GROUP_GI,      LAMP_OFF }, // GI_LEFT_CENTER_2  =  2
  { 20, LAMP_GROUP_GI,      LAMP_OFF }, // GI_LEFT_BOTTOM    =  3
  { 17, LAMP_GROUP_GI,      LAMP_OFF }, // GI_RIGHT_TOP      =  4
  { 39, LAMP_GROUP_GI,      LAMP_OFF }, // GI_RIGHT_CENTER_1 =  5
  { 49, LAMP_GROUP_GI,      LAMP_OFF }, // GI_RIGHT_CENTER_2 =  6
  { 61, LAMP_GROUP_GI,      LAMP_OFF }, // GI_RIGHT_BOTTOM   =  7
  { 51, LAMP_GROUP_BUMPER,  LAMP_OFF }, // S                 =  8
  { 16, LAMP_GROUP_BUMPER,  LAMP_OFF }, // C                 =  9
  { 60, LAMP_GROUP_BUMPER,  LAMP_OFF }, // R                 = 10
  { 28, LAMP_GROUP_BUMPER,  LAMP_OFF }, // E                 = 11
  { 29, LAMP_GROUP_BUMPER,  LAMP_OFF }, // A                 = 12
  { 18, LAMP_GROUP_BUMPER,  LAMP_OFF }, // M                 = 13
  { 50, LAMP_GROUP_BUMPER,  LAMP_OFF }, // O                 = 14
  { 21, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_1           = 15
  { 56, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_2           = 16
  { 53, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_3           = 17
  { 33, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_4           = 18
  { 36, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_5           = 19
  { 43, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_6           = 20
  { 24, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_7           = 21
  { 38, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_8           = 22
  { 26, LAMP_GROUP_WHITE,   LAMP_OFF }, // WHITE_9           = 23
  { 23, LAMP_GROUP_RED,     LAMP_OFF }, // RED_1             = 24
  { 22, LAMP_GROUP_RED,     LAMP_OFF }, // RED_2             = 25
  { 48, LAMP_GROUP_RED,     LAMP_OFF }, // RED_3             = 26
  { 45, LAMP_GROUP_RED,     LAMP_OFF }, // RED_4             = 27
  { 35, LAMP_GROUP_RED,     LAMP_OFF }, // RED_5             = 28
  { 40, LAMP_GROUP_RED,     LAMP_OFF }, // RED_6             = 29
  { 27, LAMP_GROUP_RED,     LAMP_OFF }, // RED_7             = 30
  { 32, LAMP_GROUP_RED,     LAMP_OFF }, // RED_8             = 31
  { 57, LAMP_GROUP_RED,     LAMP_OFF }, // RED_9             = 32
  { 30, LAMP_GROUP_HAT,     LAMP_OFF }, // HAT_LEFT_TOP      = 33
  { 44, LAMP_GROUP_HAT,     LAMP_OFF }, // HAT_LEFT_BOTTOM   = 34
  { 54, LAMP_GROUP_HAT,     LAMP_OFF }, // HAT_RIGHT_TOP     = 35
  { 63, LAMP_GROUP_HAT,     LAMP_OFF }, // HAT_RIGHT_BOTTOM  = 36
  { 47, LAMP_GROUP_KICKOUT, LAMP_OFF }, // KICKOUT_LEFT      = 37
  { 41, LAMP_GROUP_KICKOUT, LAMP_OFF }, // KICKOUT_RIGHT     = 38
  { 37, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // SPECIAL           = 39
  { 19, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // GOBBLE_1          = 40
  { 31, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // GOBBLE_2          = 41
  { 58, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // GOBBLE_3          = 42
  { 62, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // GOBBLE_4          = 43
  { 42, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // GOBBLE_5          = 44
  { 46, LAMP_GROUP_NONE,    LAMP_OFF }, // SPOT_NUMBER_LEFT  = 45
  { 59, LAMP_GROUP_NONE,    LAMP_OFF }  // SPOT_NUMBER_RIGHT = 46
};

// ****************************************
// ***** SWITCH STRUCTS AND CONSTANTS *****
// ****************************************

// Define a struct to store switch parameters.
// Centipede #1 (0..63) is LAMP OUTPUTS
// Centipede #2 (64..127) is SWITCH INPUTS
// In this case, pinNum refers to Centipede #2 input shift register pin number (64..127).
// The loopConst and loopCount are going to change so that we always trigger on a switch closure, but use debounce delay before checking if it's open or closed again.
// That is, unless we find we're getting phantom closures, in which case we'll need to debounce both closing and opening.
struct SwitchParmStruct {
  byte pinNum;        // Centipede #2 pin number for this switch (64..127)
  byte loopConst;     // Number of 10ms loops to confirm stable state change
  byte loopCount;     // Current count of 10ms loops with stable state (initialized to zero)
};

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

// Store Centipede #2 pin numbers (64..127) for switches (original pins + 64)
SwitchParmStruct switchParm[NUM_SWITCHES] = {
  { 118,  0,  0 },  // SWITCH_IDX_START_BUTTON         =  0  (54 + 64)
  { 115,  0,  0 },  // SWITCH_IDX_DIAG_1               =  1  (51 + 64)
  { 112,  0,  0 },  // SWITCH_IDX_DIAG_2               =  2  (48 + 64)
  { 113,  0,  0 },  // SWITCH_IDX_DIAG_3               =  3  (49 + 64)
  { 116,  0,  0 },  // SWITCH_IDX_DIAG_4               =  4  (52 + 64)
  { 127,  0,  0 },  // SWITCH_IDX_KNOCK_OFF            =  5  (63 + 64)
  { 117,  0,  0 },  // SWITCH_IDX_COIN_MECH            =  6  (53 + 64)
  { 126,  0,  0 },  // SWITCH_IDX_5_BALLS_IN_TROUGH    =  7  (62 + 64)
  { 114,  0,  0 },  // SWITCH_IDX_BALL_IN_LIFT         =  8  (50 + 64)
  { 119,  0,  0 },  // SWITCH_IDX_TILT_BOB             =  9  (55 + 64)
  { 104,  0,  0 },  // SWITCH_IDX_BUMPER_S             = 10  (40 + 64)
  {  90,  0,  0 },  // SWITCH_IDX_BUMPER_C             = 11  (26 + 64)
  {  87,  0,  0 },  // SWITCH_IDX_BUMPER_R             = 12  (23 + 64)
  {  80,  0,  0 },  // SWITCH_IDX_BUMPER_E             = 13  (16 + 64)
  { 110,  0,  0 },  // SWITCH_IDX_BUMPER_A             = 14  (46 + 64)
  { 107,  0,  0 },  // SWITCH_IDX_BUMPER_M             = 15  (43 + 64)
  {  88,  0,  0 },  // SWITCH_IDX_BUMPER_O             = 16  (24 + 64)
  { 105,  0,  0 },  // SWITCH_IDX_KICKOUT_LEFT         = 17  (41 + 64)
  {  83,  0,  0 },  // SWITCH_IDX_KICKOUT_RIGHT        = 18  (19 + 64)
  {  84,  0,  0 },  // SWITCH_IDX_SLINGSHOT_LEFT       = 19  (20 + 64)
  {  98,  0,  0 },  // SWITCH_IDX_SLINGSHOT_RIGHT      = 20  (34 + 64)
  {  99,  0,  0 },  // SWITCH_IDX_HAT_LEFT_TOP         = 21  (35 + 64)
  {  81,  0,  0 },  // SWITCH_IDX_HAT_LEFT_BOTTOM      = 22  (17 + 64)
  {  89,  0,  0 },  // SWITCH_IDX_HAT_RIGHT_TOP        = 23  (25 + 64)
  {  92,  0,  0 },  // SWITCH_IDX_HAT_RIGHT_BOTTOM     = 24  (28 + 64)
  {  86,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_2   = 25  (22 + 64)
  { 108,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_3   = 26  (44 + 64)
  {  85,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_4   = 27  (21 + 64)
  {  96,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_5   = 28  (32 + 64)
  {  82,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_1  = 29  (18 + 64)
  { 111,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_2  = 30  (47 + 64)
  {  94,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_3  = 31  (30 + 64)
  {  95,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_4  = 32  (31 + 64)
  {  91,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_5  = 33  (27 + 64)
  { 109,  0,  0 },  // SWITCH_IDX_GOBBLE               = 34  (45 + 64)
  { 106,  0,  0 },  // SWITCH_IDX_DRAIN_LEFT           = 35  (42 + 64)
  {  97,  0,  0 },  // SWITCH_IDX_DRAIN_CENTER         = 36  (33 + 64)
  {  93,  0,  0 },  // SWITCH_IDX_DRAIN_RIGHT          = 37  (29 + 64)
  { 125,  0,  0 },  // SWITCH_IDX_FLIPPER_LEFT_BUTTON  = 38  (60 + 64)
  { 124,  0,  0 }   // SWITCH_IDX_FLIPPER_RIGHT_BUTTON = 39  (61 + 64)
};

// ************************************************
// ***** COIL AND MOTOR STRUCTS AND CONSTANTS *****
// ************************************************

// Define a struct to store coil/motor pin number, power, and time parms.  Coils that may be held on include a hold strength.
// In this case, pinNum refers to an actual Arduino Mega R3 I/O pin number.
// For Flippers, original Ball Gate, new Ball Release Post, and other coils that may be held after initial activation, include a
// "hold strength" parm as well as initial strength and time on.
// If Hold Strength == 0, then simply release after hold time; else change to Hold Strength until released by software.
// Example usage:
//   analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerInitial);
//   delay(deviceParm[DEV_IDX_FLIPPER_LEFT].timeOn);
//   analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerHold);
// countdown will be set to "timeOn" and decremented every 10ms in main loop until it reaches zero, at which time power will be set to powerHold.
struct DeviceParmStruct {
  byte   pinNum;        // Arduino pin number for this coil/motor
  byte   powerInitial;  // 0..255 PWM power level when first energized
  byte   timeOn;        // Number of 10ms loop ticks (NOT raw ms). 4 => ~40ms.
  byte   powerHold;     // 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
  int8_t countdown;     // Current countdown in 10ms ticks; -1..-8 = rest period, 0 = idle, >0 = active
  byte   queueCount;    // Number of pending activation requests while coil busy/resting
};

// Define array index constants - this list is rather arbitrary and doesn't relate to device number/pin numbers:
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

const byte NUM_DEVS                     = 14;

DeviceParmStruct deviceParm[NUM_DEVS] = {
  // pinNum, powerInitial, timeOn(ms), powerHold, countdown, queueCount
  {  5, 255,  5,   0, 0, 0 },  // POP_BUMPER          =  0, PWM MOSFET
  {  4, 190, 10,   0, 0, 0 },  // KICKOUT_LEFT        =  1, PWM MOSFET (cannot modify PWM freq.)
  { 44, 200, 10,   0, 0, 0 },  // KICKOUT_RIGHT       =  2, PWM MOSFET
  {  6, 180, 10,   0, 0, 0 },  // SLINGSHOT_LEFT      =  3, PWM MOSFET
  {  7, 180, 10,   0, 0, 0 },  // SLINGSHOT_RIGHT     =  4, PWM MOSFET
  {  8, 150, 10,  40, 0, 0 },  // FLIPPER_LEFT        =  5, PWM MOSFET.  200 hits gobble hole too hard; 150 can get to top of p/f.
  {  9, 150, 10,  40, 0, 0 },  // FLIPPER_RIGHT       =  6, PWM MOSFET
  { 10, 200, 20,  40, 0, 0,},  // BALL_TRAY_RELEASE   =  7, PWM MOSFET (original tray release)
  { 23, 255,  5,   0, 0, 0 },  // SELECTION_UNIT      =  8, Non-PWM MOSFET; on/off only.
  { 24, 255,  5,   0, 0, 0 },  // RELAY_RESET         =  9, Non-PWM MOSFET; on/off only.
  { 11, 150, 23,   0, 0, 0,},  // BALL_TROUGH_RELEASE = 10, PWM MOSFET (new up/down post)
  // For the ball trough release coil, 150 is the right power; 100 could not guarantee it could retract if there was pressure holding it from balls above.
  // 230ms is just enough time for ball to get 1/2 way past post and momentum carries it so it won't get pinned by the post.
  // The post springs back fully extended before the next ball hits it, regardless of how many balls are stacked above it.
  { 12,   0,  0,   0, 0, 0 },  // MOTOR_SHAKER        = 11, PWM MOSFET. Range is 70 to 140. Below won't start; above triggers hats.
  { 26, 200,  4,   0, 0, 0 },  // KNOCKER             = 12, Non-PWM MOSFET; on/off only.
  { 25, 255,  0,   0, 0, 0 }   // MOTOR_SCORE         = 13, A/C SSR; on/off only; NOT PWM.
};

const byte MOTOR_SHAKER_POWER_MIN =  70;  // Minimum power needed to start shaker motor; less will stall and overheat
const byte MOTOR_SHAKER_POWER_MAX = 140;  // Maximum power before hats trigger


// ********************************
// ***** AUDIO TRACK METADATA *****
// ********************************

// Separate structs for MUSIC, SFX, and VOICE tracks due to different parameters needed.

struct AudioVoiceTrackDef {
  unsigned int trackNum;     // Tsunami file number (0001..4095)
  byte  eventId;             // VOICE_EVENT_* (what is this used for?)
  byte  priority;            // VOICE_PRIORITY_HIGH/MED/LOW
  byte  lengthTenths;        // Track length in 0.1s (for ducking)
  byte  minPlayers;          // 0 = any, else 1..4
  byte  maxPlayers;          // 0 = any
  byte  requiredPlayerIndex; // 0 = any, 1..4 = specific player
  byte  phaseMask;           // Bitfield: game start, ball start, hurry-up, game over, etc.
  byte  flags;               // AUDIO_FLAG_* (loop, non-interrupt, etc.)
};

struct AudioSfxTrackDef {
  unsigned int trackNum;
  byte  eventId;       // SFX_EVENT_*
  byte  lengthTenths;  // For optional timing/overlap management
  byte  flags;         // AUDIO_FLAG_LOOP, etc.
};

struct AudioMusicTrackDef {  // This may not even be needed, except for diagnostic purposes.
  unsigned int trackNum;     // Tsunami track number (0001..4095 on SD card)
  unsigned int EEPROMStringNum;  // Identifies the string stored in EEPROM that describes this music track
  byte  themeId;  // MUSIC_THEME_CIRCUS / MUSIC_THEME_SURF
  byte  role;     // MUSIC_ROLE_MAIN / MUSIC_ROLE_MULTIBALL / MUSIC_ROLE_ATTRACT
  byte  flags;    // AUDIO_FLAG_LOOP (always set for most music)
};


// Audio categories (use consts instead of enums)
const byte AUDIO_CAT_VOICE = 0;
const byte AUDIO_CAT_SFX   = 1;
const byte AUDIO_CAT_MUSIC = 2;

// Voice priority levels
const byte VOICE_PRIORITY_HIGH = 3;  // Highest priority i.e. urgent voice such as "Here's another ball; shoot again!"
const byte VOICE_PRIORITY_MED  = 2;  // Medium priority i.e. important voice such as mode instructions
const byte VOICE_PRIORITY_LOW  = 1;  // Low priority i.e. non-urgent voice such as "Good luck!"

// Voice subcategories (for random selection within an event type)
const byte VOICE_SUB_NONE        = 0;
const byte VOICE_SUB_GAME_START  = 1;
const byte VOICE_SUB_PLAYER_UP   = 2;
const byte VOICE_SUB_BALL_SAVED  = 3;
const byte VOICE_SUB_BAD_PLAY    = 4;
const byte VOICE_SUB_GAME_OVER   = 5;
// Add more as needed.
// Bit flags controlling behavior
const byte AUDIO_FLAG_LOOP          = 0x01;  // Track should loop until explicitly stopped
const byte AUDIO_FLAG_NON_INTERRUPT = 0x02;  // Do not start if another same-category track playing
const byte AUDIO_FLAG_DUCK_OTHERS   = 0x04;  // Future: lower volume on other categories while this plays


// Master list of all Tsunami tracks we care about.
// NOTE: Track numbers here must match the Tsunami SD card filenames.
const AudioMusicTrackDef audioTracks[] = {
  // trackNum, category,        voiceSubcat,            flags
  {  1,        AUDIO_CAT_VOICE, VOICE_SUB_GAME_START,   0 },               // "Let's ride Screamo"
  {  2,        AUDIO_CAT_VOICE, VOICE_SUB_PLAYER_UP,    0 },               // "Player 2, you're up"
  {  3,        AUDIO_CAT_VOICE, VOICE_SUB_BALL_SAVED,   0 },               // "Ball saved"
  {  4,        AUDIO_CAT_VOICE, VOICE_SUB_BALL_SAVED,   0 },               // alternate "Ball saved"
  {  5,        AUDIO_CAT_VOICE, VOICE_SUB_BAD_PLAY,     0 },               // "That was terrible"
  {  6,        AUDIO_CAT_VOICE, VOICE_SUB_BAD_PLAY,     0 },               // alternate bad-play quip

  { 20,        AUDIO_CAT_SFX,   VOICE_SUB_NONE,         AUDIO_FLAG_LOOP }, // Coaster clicking up hill
  { 21,        AUDIO_CAT_SFX,   VOICE_SUB_NONE,         0 },               // Coaster roaring down hill
  { 22,        AUDIO_CAT_SFX,   VOICE_SUB_NONE,         0 },               // Girl scream, short
  { 23,        AUDIO_CAT_SFX,   VOICE_SUB_NONE,         0 },               // Gobble hole thud / roar

  { 30,        AUDIO_CAT_MUSIC, VOICE_SUB_NONE,         AUDIO_FLAG_LOOP }, // Main background music
  { 31,        AUDIO_CAT_MUSIC, VOICE_SUB_NONE,         AUDIO_FLAG_LOOP }, // Multiball music
  { 32,        AUDIO_CAT_MUSIC, VOICE_SUB_NONE,         AUDIO_FLAG_LOOP }  // Attract mode music
};

const byte NUM_AUDIO_TRACKS = (byte)(sizeof(audioTracks) / sizeof(audioTracks[0]));

// Index constants into audioTracks[] for commonly used sounds.
// Keep these in sync with the ordering of audioTracks[] above.
const byte AUDIO_IDX_VOICE_GAME_START   = 0;  // "Let's ride Screamo"
const byte AUDIO_IDX_VOICE_PLAYER_UP    = 1;  // "Player 2, you're up"
const byte AUDIO_IDX_VOICE_BALL_SAVED_1 = 2;  // "Ball saved"
const byte AUDIO_IDX_VOICE_BALL_SAVED_2 = 3;  // alt "Ball saved"
const byte AUDIO_IDX_VOICE_BAD_PLAY_1   = 4;  // "That was terrible"
const byte AUDIO_IDX_VOICE_BAD_PLAY_2   = 5;  // alt bad-play quip

const byte AUDIO_IDX_SFX_COASTER_CLIMB  = 6;  // track 20
const byte AUDIO_IDX_SFX_COASTER_DROP   = 7;  // track 21
const byte AUDIO_IDX_SFX_SCREAM         = 8;  // track 22
const byte AUDIO_IDX_SFX_GOBBLE         = 9;  // track 23

const byte AUDIO_IDX_MUSIC_MAIN         = 10; // track 30
const byte AUDIO_IDX_MUSIC_MULTIBALL    = 11; // track 31
const byte AUDIO_IDX_MUSIC_ATTRACT      = 12; // track 32




// Simple per-category what's-currently-playing state (trackNum 0 == none)
unsigned int audioCurrentVoiceTrack = 0;
unsigned int audioCurrentSfxTrack   = 0;
unsigned int audioCurrentMusicTrack = 0;


// Tsunami master gain settings (in dB)
// Master gain can range from -40db to 0dB (full level); defaults to -10dB to allow some headroom.
// Per-category RELATIVE gains are applied on top of master gain; can range from -40dB to +40dB but clamped within -40dB to 0dB overall.
const int8_t TSUNAMI_GAIN_DB_DEFAULT = -10;  // Default Tsunami master gain in dB
const int8_t TSUNAMI_GAIN_DB_MIN     = -40;  // Clamp range; adjust as needed
const int8_t TSUNAMI_GAIN_DB_MAX     =   0;  // 0 dB = full level (example)

int8_t tsunamiGainDb      = TSUNAMI_GAIN_DB_DEFAULT;  // Persisted Tsunami master gain
// Per-category relative gains in dB (applied on top of master).
// 0 == no change; negative to reduce that category, positive to boost (within clamp limits).
int8_t tsunamiVoiceGainDb = 0;
int8_t tsunamiSfxGainDb   = 0;
int8_t tsunamiMusicGainDb = 0;

// ********************************
// ****** DIAGNOSTICS SETUP *******
// ********************************

void markDiagnosticsDisplayDirty(bool forceSuiteReset = false);  // Must forward declare since it has a default parm.

byte diagnosticSuiteIdx  = 0;  // 0..NUM_DIAG_SUITES-1: which top-level suite selected
byte diagnosticState     = 0;  // 0 = at suite menu, >0 = inside a suite (future use)

const byte NUM_DIAG_SUITES = 5;
const char* diagSuiteNames[NUM_DIAG_SUITES] = {
  "VOLUME",
  "LAMP TESTS",
  "SWITCH TESTS",
  "COIL/MOTOR TESTS",
  "AUDIO TESTS"
};

const byte NUM_DIAG_BUTTONS = 4;
byte diagButtonLastState[NUM_DIAG_BUTTONS] = { 0, 0, 0, 0 };
bool attractDisplayDirty = true;
bool diagDisplayDirty = true;
byte diagLastRenderedSuite = 0xFF;

// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************

// *** SERIAL LCD DISPLAY CLASS ***
Pinball_LCD* pLCD2004 = nullptr;  // pLCD2004 is #included in Pinball_Functions.h, so it's effectively a global variable.

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Pinball_Message.h>
Pinball_Message* pMessage = nullptr;

// *** PINBALL_CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Pinball_Centipede) is already in <Pinball_Centipede.h> so not needed here.
// #include <Pinball_Centipede.h> is already in <Pinball_Functions.h> so not needed here.
Pinball_Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards

// *** TSUNAMI WAV PLAYER CLASS ***
Tsunami* pTsunami = nullptr;  // Tsunami WAV player object pointer

// *** MISC CONSTANTS AND GLOBALS ***
byte         modeCurrent      = MODE_ATTRACT;  // MODE_UNDEFINED;  Just to get going; maybe change to ATTRACT later.
char         msgType          = ' ';

// 10ms scheduler
const unsigned int LOOP_TICK_MS = 10;
unsigned long loopNextMillis = 0;

int currentScore = 0; // Current game score (0..999).

bool debugOn    = false;

// *** SWITCH STATE TABLE: Arrays contain 4 elements (unsigned ints) of 16 bits each = 64 bits = 1 Centipede
// Although we're using two Centipede boards (one for OUTPUTs, one for INPUTs), we only need one set of switch state arrays.
unsigned int switchOldState[] = { 65535,65535,65535,65535 };
unsigned int switchNewState[] = { 65535,65535,65535,65535 };

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  initScreamoMasterArduinoPins();                    // Arduino direct I/O pins only.

  setAllDevicesOff();  // Set initial state for all MOSFET PWM and Centipede outputs

  // *** INITIALIZE PINBALL_CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Pinball_Centipede class hangs the system if hardware is not connected.
  // Initialize I2C and Centipede as early as possible to minimize relay-on (turns on all lamps) at power-up
  Wire.begin();                            // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Pinball_Centipede;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                 // Set all registers to default.
  pShiftRegister->initScreamoMasterCentipedePins();

  setPWMFrequencies(); // Set non-default PWM frequencies so coils don't buzz.

  clearAllPWM();  // Clear any PWM outputs that may have been set during setPWMFrequencies()

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 LCD2004 instantiated via pLCD2004->begin.
  // Serial2 RS485   instantiated via pMessage->begin.
  // Serial3 Tsunami WAV Trigger instantiated via tsunami->begin.

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Pinball_Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // Insert a delay() in order to give the Digole 2004 LCD time to power up and be ready to receive commands (esp. the 115K speed command).
  delay(750);  // 500ms was occasionally not enough.
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.




  // Temporary: test switch wiring after hardware changes
  testSwitchWiring();





  // *** INITIALIZE TSUNAMI WAV PLAYER OBJECT ***
 // Files must be WAV format, 16-bit PCM, 44.1kHz, Stereo.  Details in Tsunami.h comments.
  pTsunami = new Tsunami();  // Create Tsunami object on Serial3
  pTsunami->start();         // Start Tsunami WAV player
  delay(10);                 // Give Tsunami time to initialize
  // Ensure Tsunami is in a clean state on any reset:
  //  - stop any tracks that might have been left playing
  //  - reload and apply master / category gains
  //  - clear our "currently playing" state
  audioResetTsunamiState();

  //tsunamiTest();             // Play test sound on startup to verify Tsunami is working.


  if (pTsunami != nullptr) {
    const int MUSIC_FADE_TIME_MS         = 500;
    const int MUSIC_GAIN_DEFAULT_DB      = -5;
    const int MUSIC_GAIN_FADE_DB         = -20;
    const unsigned int TRACK_MODE_EXPLANATION = 35;
    const unsigned int TRACK_MUSIC            = 36;
    const unsigned int TRACK_TARGET_SFX       = 37;

    audioResetTsunamiState();

    // 1) Start looped music.
    pTsunami->trackGain(TRACK_MUSIC, MUSIC_GAIN_DEFAULT_DB);
    pTsunami->trackLoop(TRACK_MUSIC, true);
    pTsunami->trackPlayPoly(TRACK_MUSIC, 0, false);
    delay(5000);

    // 2) Fade music down, then play the explanation.
    pTsunami->trackFade(TRACK_MUSIC, MUSIC_GAIN_FADE_DB, MUSIC_FADE_TIME_MS, false);
    delay(MUSIC_FADE_TIME_MS + 100);
    pTsunami->trackPlayPoly(TRACK_MODE_EXPLANATION, 0, false);
    delay(7000);

    // 3) Bring the music back up and let it run 5 seconds.
    pTsunami->trackFade(TRACK_MUSIC, MUSIC_GAIN_DEFAULT_DB, MUSIC_FADE_TIME_MS, false);
    delay(MUSIC_FADE_TIME_MS + 100);
    delay(5000);

    // 4) Fire the target-hit SFX twice.
    pTsunami->trackPlayPoly(TRACK_TARGET_SFX, 0, false);
    delay(1000);
    delay(3000);
    pTsunami->trackPlayPoly(TRACK_TARGET_SFX, 0, false);
    delay(5000);
    // 5) Stop the music.
    pTsunami->trackStop(TRACK_MUSIC);
    delay(1000);

    while (true) {
      delay(1000);
    }
  }











  setGILamps(true);          // Turn on playfield G.I. lamps
  pMessage->sendMAStoSLVGILamp(true);  // Tell Slave to turn on its G.I. lamps as well

  // runBasicDiagnostics();

  // runRudimentarySwitchAndLampTests();

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  // Simple 10ms tick-based main loop. All game logic runs at this cadence.
  unsigned long now = millis();
  if ((long)(now - loopNextMillis) < 0) {
    // Not time yet for next tick.
    return;
  }

  loopNextMillis = now + LOOP_TICK_MS;

  // Fast path: handle flipper buttons immediately (always active)
  // NOTE: This is placeholder; real flipper handling will be added later.
  // if (switchClosed(SWITCH_IDX_FLIPPER_LEFT_BUTTON)) { /* handle left flipper */ }
  // if (switchClosed(SWITCH_IDX_FLIPPER_RIGHT_BUTTON)) { /* handle right flipper */ }

  // Update Centipede #2 switch snapshot once per tick (for non-flipper switches)
  for (int i = 0; i < 4; i++) {
    switchOldState[i] = switchNewState[i];
    switchNewState[i] = pShiftRegister->portRead(4 + i);  // ports 4..7 => inputs
  }

  // Dispatch by mode.
  switch (modeCurrent) {
  case MODE_ATTRACT:
    updateModeAttract();
    break;
  case MODE_ORIGINAL:
    updateModeOriginal();
    break;
  case MODE_ENHANCED:
    updateModeEnhanced();
    break;
  case MODE_IMPULSE:
    updateModeImpulse();
    break;
  case MODE_TILT:
    updateModeTilt();
    break;
  case MODE_DIAGNOSTIC:
    updateModeDiagnostic();
    break;
  case MODE_UNDEFINED:
  default:
    // If we ever land here, fall back to a safe state (attract).
    modeCurrent = MODE_ATTRACT;
    break;
  }
}  // End of loop()

// ********************************************************************************************************************************
// ******************************************************** DIAGNOSTICS ***********************************************************
// ********************************************************************************************************************************

// Temporary function to test/verify switch wiring after hardware changes.
// Call this near the end of setup() to enter an interactive switch test mode.
// Close each switch on the playfield and verify the displayed index matches expectations.
void testSwitchWiring() {
  sprintf(lcdString, "Switch Test Mode");
  pLCD2004->println(lcdString);
  sprintf(lcdString, "Close switches...");
  pLCD2004->println(lcdString);
  delay(2000);

  byte lastClosedSwitch = 0xFF;  // Track last reported switch to avoid spam
  unsigned long lastCheckMillis = 0;

  while (true) {

    // Read all Centipede #2 inputs (16 bits at a time)
    for (int i = 0; i < 4; i++) {
      switchNewState[i] = pShiftRegister->portRead(4 + i);  // ports 4..7 => Centipede #2 inputs
    }

    // Scan all switches to find the most recent closure
    byte closedSwitch = 0xFF;
    for (byte sw = 0; sw < NUM_SWITCHES; sw++) {
      if (switchClosed(sw)) {
        closedSwitch = sw;
        break;  // Report first closed switch found
      }
    }

    // Display if different from last reported
    if (closedSwitch != lastClosedSwitch) {
      if (closedSwitch == 0xFF) {
        sprintf(lcdString, "No switches closed");
        pLCD2004->println(lcdString);
        Serial.println(lcdString);
      } else {
        // Display switch index and Centipede pin number
        byte centPin = switchParm[closedSwitch].pinNum;
        sprintf(lcdString, "SW%02d Pin%03d", closedSwitch, centPin);
        pLCD2004->println(lcdString);
        Serial.println(lcdString);

        // Also show name if available
        if (closedSwitch < NUM_DIAG_SWITCHES) {
          char swName[18];
          diagCopyProgmemString(diagSwitchNames, closedSwitch, swName, sizeof(swName));
          sprintf(lcdString, "%s", swName);
          pLCD2004->println(lcdString);
          Serial.println(lcdString);
        }
      }
      lastClosedSwitch = closedSwitch;
    }

    // Exit test mode with DIAG_4 (Select) button
    if (switchClosed(SWITCH_IDX_DIAG_4)) {
      sprintf(lcdString, "Exiting test mode");
      pLCD2004->println(lcdString);
      delay(1000);
      break;
    }

    delay(10);  // Small delay to prevent tight loop
  }
}

// Handles top-level diagnostic button navigation. Returns true when we should exit diagnostics.
static bool handleDiagnosticsMenuButtons() {
  if (diagButtonPressed(0)) {
    modeCurrent = MODE_ATTRACT;
    resetDiagButtonStates();
    markAttractDisplayDirty();
    markDiagnosticsDisplayDirty(true);
    sprintf(lcdString, "Exit Diagnostics");
    pLCD2004->println(lcdString);
    return true;
  }

  if (diagButtonPressed(1)) {
    if (diagnosticSuiteIdx == 0) {
      diagnosticSuiteIdx = NUM_DIAG_SUITES - 1;
    }
    else {
      diagnosticSuiteIdx--;
    }
    markDiagnosticsDisplayDirty();
  }

  if (diagButtonPressed(2)) {
    diagnosticSuiteIdx++;
    if (diagnosticSuiteIdx >= NUM_DIAG_SUITES) {
      diagnosticSuiteIdx = 0;
    }
    markDiagnosticsDisplayDirty();
  }

  if (diagButtonPressed(3)) {
    sprintf(lcdString, "Running Tests");
    pLCD2004->println(lcdString);
    markDiagnosticsDisplayDirty();
  }

  return false;
}

void runDiagnosticsLoop() {
  // Top-level diagnostics menu:
  // Line 1: "Diagnostics"
  // Line 2: suite name from diagSuiteNames[diagnosticSuiteIdx]
  // DIAG_1: Back (exit diagnostics -> Attract)
  // DIAG_2: Previous suite
  // DIAG_3: Next suite
  // DIAG_4: Select current suite (future expansion)

  if (diagnosticState != 0) {
    diagnosticState = 0;
  }

  if (handleDiagnosticsMenuButtons()) {
    return;
  }

  renderDiagnosticsMenuIfNeeded();
}

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

void setGILamps(bool t_on) {
  // Turn on or off all G.I. lamps
  for (int i = 0; i < NUM_LAMPS; i++) {
    if (lampParm[i].groupNum == LAMP_GROUP_GI) {
      if (t_on) {
        pShiftRegister->digitalWrite(lampParm[i].pinNum, LOW);  // Active LOW
      } else {
        pShiftRegister->digitalWrite(lampParm[i].pinNum, HIGH);
      }
    }
  }
}

bool switchClosed(byte t_switchIdx) {
  // CALLER MUST ENSURE switchNewState[] HAS BEEN UPDATED BY CALLING portRead() FOR Centipede #2 INPUTS!!!
  // Accept only a switch index (0..NUM_SWITCHES-1).  The Centipede #2 pin for that switch
  // is looked up in switchParm[].pinNum (64..127).
  if (t_switchIdx >= NUM_SWITCHES) {
    return false;
  }
  byte pin = switchParm[t_switchIdx].pinNum; // Centipede pin number 64..127
  pin = pin - 64;               // Convert to 0..63 for indexing into switchNewState[]
  byte portIndex = pin / 16;    // 0..3
  byte bitNum = pin % 16;       // 0..15
  // switch is active LOW on Centipede, so bit==0 means closed.
  return ((switchNewState[portIndex] & (1 << bitNum)) == 0);
}

bool switchClosedImmediate(byte t_switchIdx) {
  if (t_switchIdx >= NUM_SWITCHES || pShiftRegister == nullptr) {
    return false;
  }
  return pShiftRegister->digitalRead(switchParm[t_switchIdx].pinNum) == LOW;
}

void waitForSwitchClosedImmediate(byte t_switchIdx) {
  while (!switchClosedImmediate(t_switchIdx)) {
    delay(1);
  }
}

void waitForSwitchOpenImmediate(byte t_switchIdx) {
  while (switchClosedImmediate(t_switchIdx)) {
    delay(1);
  }
}

bool switchPressedEdge(byte t_switchIdx, byte* t_lastState) {
  bool closedNow = switchClosed(t_switchIdx);
  bool pressed = (closedNow && (*t_lastState == 0));
  *t_lastState = closedNow ? 1 : 0;
  return pressed;
}

void resetDiagButtonStates() {
  for (byte i = 0; i < NUM_DIAG_BUTTONS; i++) {
    bool closedNow = switchClosed((byte)(SWITCH_IDX_DIAG_1 + i));
    diagButtonLastState[i] = closedNow ? 1 : 0;
  }
}

bool diagButtonPressed(byte t_diagIdx) {
  if (t_diagIdx >= NUM_DIAG_BUTTONS) {
    return false;
  }
  byte switchIdx = (byte)(SWITCH_IDX_DIAG_1 + t_diagIdx);
  return switchPressedEdge(switchIdx, &diagButtonLastState[t_diagIdx]);
}

void markAttractDisplayDirty() {
  attractDisplayDirty = true;
}

void renderAttractDisplayIfNeeded() {
  if (!attractDisplayDirty || pLCD2004 == nullptr) {
    return;
  }
  sprintf(lcdString, "Screamo Ready");
  pLCD2004->println(lcdString);
  sprintf(lcdString, "Press Start");
  pLCD2004->println(lcdString);
  attractDisplayDirty = false;
}

void markDiagnosticsDisplayDirty(bool forceSuiteReset) {
  diagDisplayDirty = true;
  if (forceSuiteReset) {
    diagLastRenderedSuite = 0xFF;
  }
}

void renderDiagnosticsMenuIfNeeded() {
  if (pLCD2004 == nullptr) {
    return;
  }
  if (!diagDisplayDirty && diagLastRenderedSuite == diagnosticSuiteIdx) {
    return;
  }
  diagDisplayDirty = false;
  diagLastRenderedSuite = diagnosticSuiteIdx;
  sprintf(lcdString, "Diagnostics");
  pLCD2004->println(lcdString);
  sprintf(lcdString, "%s", diagSuiteNames[diagnosticSuiteIdx]);
  pLCD2004->println(lcdString);
}

void setAllDevicesOff() {
  // Set the output latch low first, then configure as OUTPUT.
  // This avoids a brief HIGH when changing direction.
  for (int i = 0; i < NUM_DEVS; i++) {
    digitalWrite(deviceParm[i].pinNum, LOW);  // set PORTx latch to 0 while pin still INPUT
    pinMode(deviceParm[i].pinNum, OUTPUT);    // now become driven LOW
  }
  // Use shared helper to force Centipede outputs OFF (safe if pShiftRegister == nullptr)
  centipedeForceAllOff();
}

void setPWMFrequencies() {
  // Configure PWM timers / prescalers for higher PWM frequency (~31kHz)
  // Keep this separate from clearAllPWM() which only clears PWM outputs.
  // Timer 1 -> pins 11, 12 (Ball Trough Release, Shaker)
  TCCR1B = (TCCR1B & 0b11111000) | 0x01; // prescaler = 1
  // Timer 2 -> pins 9, 10 (Right Flipper, Ball Tray Release)
  TCCR2B = (TCCR2B & 0b11111000) | 0x01; // prescaler = 1
  // Timer 4 -> pins 6, 7, 8 (Left/Right Slingshots, Left Flipper)
  TCCR4B = (TCCR4B & 0b11111000) | 0x01; // prescaler = 1
  // Timer 5 -> pins 44..46 (Right Kickout, etc.)
  TCCR5B = (TCCR5B & 0b11111000) | 0x01; // prescaler = 1
}

void clearAllPWM() {
  // Call this after PWM timers / prescalers have been configured.
  // On PWM-capable pins this clears compare outputs; harmless on non-PWM pins.
  for (int i = 0; i < NUM_DEVS; i++) {
    analogWrite(deviceParm[i].pinNum, 0);
  }
}

void softwareReset() {
  // Ensure everything is driven off (both latch and PWM) before reset.
  setAllDevicesOff();
  clearAllPWM();
  cli(); // disable interrupts
  wdt_enable(WDTO_15MS); // enable watchdog timer to trigger a system reset with shortest timeout
  while (1) { }  // wait for watchdog to reset system
}

// ********************************************************************************************************************************
// ************************************************* MODE UPDATE / DISPATCHERS ****************************************************
// ********************************************************************************************************************************

// Attract mode: idle lamps/audio, wait for credits/start.
void updateModeAttract() {
  renderAttractDisplayIfNeeded();

  if (audioCurrentMusicTrack == 0) {
    audioStartMainMusic();
  }

  if (diagButtonPressed(1)) {
    audioAdjustMasterGain(-1);
    audioPlayCoasterDrop();
  }

  if (diagButtonPressed(2)) {
    audioAdjustMasterGain(+1);
    audioPlayCoasterDrop();
  }

  if (diagButtonPressed(3)) {
    resetDiagButtonStates();
    modeCurrent = MODE_DIAGNOSTIC;
    diagnosticSuiteIdx = 0;
    diagnosticState = 0;
    markDiagnosticsDisplayDirty(true);
    return;
  }

  // Additional attract-mode logic (credits/start) goes here.
}

// Original mode: play game using EM rules, Slave drives score reels.
void updateModeOriginal() {
  // TODO: implement Original rules, scoring, ball handling.
  // For now, just a placeholder.
}

// Enhanced mode: modern rules on top of EM hardware.
void updateModeEnhanced() {
  // TODO: main Enhanced game logic here.

  if (diagButtonPressed(1)) {
    audioAdjustMasterGain(-1);
  }

  if (diagButtonPressed(2)) {
    audioAdjustMasterGain(+1);
  }
}

// Impulse mode: special limited-time / impulse-play mode as per overview.
void updateModeImpulse() {
  // TODO: implement Impulse rules.
}

// Tilt mode: frozen game, minimal response until cleared.
void updateModeTilt() {
  // TODO: turn off flippers, coils, most lamps, show TILT indications on LCD and playfield.
  // Example placeholder: stop all music.
  audioStopAllMusic();
}

// Diagnostic mode: run manual tests of lamps, coils, switches, Tsunami, RS485.
void updateModeDiagnostic() {
  // For now, route to a simple input-driven diagnostic handler.
  runDiagnosticsLoop();
}

void tsunamiTest() {
  // Here is some code to test the Tsunami WAV player by playing a sound...
  // Call example: playTsunamiSound(1);
  // 001..016 are spoken numbers
  // 020 is coaster clicks
  // 021 is coaster clicks with 10-char filename
  // 030 is "cock the gun" being spoken
  // 031 is American Flyer announcement
  // Track number, output number is always 0, lock T/F

  pTsunami->trackPlayPoly(21, 0, false);
  delay(2000);
  pTsunami->trackPlayPoly(30, 0, false);
  delay(2000);
  pTsunami->trackPlayPoly(1, 0, false);
  delay(2000);
  pTsunami->trackStop(21);
}

// ************************************
// ***** TSUNAMI HELPER FUNCTIONS *****
// ************************************

// This gives us simple primitives: audioPlayTrack(trackNum); and audioStopTrack(trackNum);
// We can call these directly for “fire and forget” events.

// ************************************
// ***** TSUNAMI GAIN / VOLUME    *****
// ************************************

// Master gain applies to all outputs; category gains are applied per-track on top of master.
void audioApplyMasterGain() {
  if (pTsunami == nullptr) {
    return;
  }

  int gain = (int)tsunamiGainDb;

  for (int out = 0; out < TSUNAMI_NUM_OUTPUTS; out++) {
    pTsunami->masterGain(out, gain);
  }
}

void audioSaveMasterGain() {
  // Store tsunamiGainDb as a single signed byte
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN, (byte)tsunamiGainDb);
}

void audioLoadMasterGain() {
  byte raw = EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN);
  int8_t val = (int8_t)raw;

  if (val < TSUNAMI_GAIN_DB_MIN || val > TSUNAMI_GAIN_DB_MAX) {
    tsunamiGainDb = TSUNAMI_GAIN_DB_DEFAULT;
    audioSaveMasterGain();
  }
  else {
    tsunamiGainDb = val;
  }
}

// Persist per-category relative gains in EEPROM.
// These are smaller-range trims layered on top of tsunamiGainDb.
void audioSaveCategoryGains() {
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN_VOICE, (byte)tsunamiVoiceGainDb);
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN_SFX, (byte)tsunamiSfxGainDb);
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN_MUSIC, (byte)tsunamiMusicGainDb);
}

void audioLoadCategoryGains() {
  int8_t v = (int8_t)EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN_VOICE);
  int8_t s = (int8_t)EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN_SFX);
  int8_t m = (int8_t)EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN_MUSIC);

  // Use a modest range for category trims; these are relative to master.
  const int8_t CAT_TRIM_MIN = -20;
  const int8_t CAT_TRIM_MAX =  20;

  if (v < CAT_TRIM_MIN || v > CAT_TRIM_MAX) {
    v = 0;
  }
  if (s < CAT_TRIM_MIN || s > CAT_TRIM_MAX) {
    s = 0;
  }
  if (m < CAT_TRIM_MIN || m > CAT_TRIM_MAX) {
    m = 0;
  }

  tsunamiVoiceGainDb = v;
  tsunamiSfxGainDb   = s;
  tsunamiMusicGainDb = m;

  // Optional: normalize stored values in case they were invalid.
  audioSaveCategoryGains();
}

// Stop all playback on Tsunami and restore default/master/category gains.
// Call once during setup after pTsunami->start().
void audioResetTsunamiState() {
  if (pTsunami == nullptr) {
    return;
  }

  // Stop ALL tracks (Tsunami supports up to 4096, but we can cover what we use).
  // trackStop(0) is typically "stop all", but we are explicit for safety.
  for (int trk = 1; trk <= 256; trk++) {
    pTsunami->trackStop(trk);
  }

  // Reload master and category gains from EEPROM (in case reset happened
  // after changes but before they were persisted, or after a crash).
  audioLoadMasterGain();
  audioLoadCategoryGains();

  // Reapply master gain to all outputs.
  audioApplyMasterGain();

  // Clear "currently playing" bookkeeping so mode code does not think
  // something is active when Tsunami is now stopped.
  audioCurrentVoiceTrack = 0;
  audioCurrentSfxTrack   = 0;
  audioCurrentMusicTrack = 0;
}

// Compute effective gain (in Tsunami units) for a given category,
// combining master gain and relative category trim, and clamping to Tsunami range.
int audioGetEffectiveGainDb(byte t_category) {
  int base = (int)tsunamiGainDb;

  if (t_category == AUDIO_CAT_VOICE) {
    base += (int)tsunamiVoiceGainDb;
  }
  else if (t_category == AUDIO_CAT_SFX) {
    base += (int)tsunamiSfxGainDb;
  }
  else if (t_category == AUDIO_CAT_MUSIC) {
    base += (int)tsunamiMusicGainDb;
  }

  if (base < TSUNAMI_GAIN_DB_MIN) {
    base = TSUNAMI_GAIN_DB_MIN;
  }
  else if (base > TSUNAMI_GAIN_DB_MAX) {
    base = TSUNAMI_GAIN_DB_MAX;
  }

  return base;
}

void audioAdjustMasterGain(int8_t t_deltaDb) {
  int newVal = (int)tsunamiGainDb + (int)t_deltaDb;
  if (newVal < TSUNAMI_GAIN_DB_MIN) {
    newVal = TSUNAMI_GAIN_DB_MIN;
  }
  else if (newVal > TSUNAMI_GAIN_DB_MAX) {
    newVal = TSUNAMI_GAIN_DB_MAX;
  }

  if ((int8_t)newVal == tsunamiGainDb) {
    // No change; nothing to do.
    return;
  }

  tsunamiGainDb = (int8_t)newVal;
  audioApplyMasterGain();
  audioSaveMasterGain();

  // Optional: show gain on LCD line (kept under 20 chars).
  sprintf(lcdString, "Vol %3d dB", (int)tsunamiGainDb);
  pLCD2004->println(lcdString);
}

// Adjust a specific category's relative gain (in dB) and persist.
void audioAdjustCategoryGain(byte t_category, int8_t t_deltaDb) {
  int8_t* pGain = nullptr;

  switch (t_category) {
  case AUDIO_CAT_VOICE: pGain = &tsunamiVoiceGainDb; break;
  case AUDIO_CAT_SFX:   pGain = &tsunamiSfxGainDb;   break;
  case AUDIO_CAT_MUSIC: pGain = &tsunamiMusicGainDb; break;
  default:
    return;
  }

  // Category trims use a smaller window than the absolute Tsunami range.
  const int8_t CAT_TRIM_MIN = -20;
  const int8_t CAT_TRIM_MAX =  20;

  int newVal = (int)(*pGain) + (int)t_deltaDb;
  if (newVal < CAT_TRIM_MIN) {
    newVal = CAT_TRIM_MIN;
  } else if (newVal > CAT_TRIM_MAX) {
    newVal = CAT_TRIM_MAX;
  }

  if ((int8_t)newVal == *pGain) {
    return;
  }

  *pGain = (int8_t)newVal;
  audioSaveCategoryGains();

  // Simple feedback; caller can decide when to show this.
  sprintf(lcdString, "Cat %u %+3d dB", (unsigned)t_category, (int)(*pGain));
  pLCD2004->println(lcdString);
}

// Internal: update per-category "now playing" state based on a track index.
void audioSetCurrentFromIndex(byte t_index, bool t_set) {
  if (t_index >= NUM_AUDIO_TRACKS) {
    return;
  }
/*
  if (!t_set) {
    unsigned int trackNum = audioTracks[t_index].trackNum;
    byte category = audioTracks[t_index].category;

    if (category == AUDIO_CAT_VOICE && audioCurrentVoiceTrack == trackNum) {
      audioCurrentVoiceTrack = 0;
    } else if (category == AUDIO_CAT_SFX && audioCurrentSfxTrack == trackNum) {
      audioCurrentSfxTrack = 0;
    } else if (category == AUDIO_CAT_MUSIC && audioCurrentMusicTrack == trackNum) {
      audioCurrentMusicTrack = 0;
    }
    return;
  }

  unsigned int trackNum = audioTracks[t_index].trackNum;
  byte category = audioTracks[t_index].category;

  if (category == AUDIO_CAT_VOICE) {
    audioCurrentVoiceTrack = trackNum;
  } else if (category == AUDIO_CAT_SFX) {
    audioCurrentSfxTrack = trackNum;
  } else if (category == AUDIO_CAT_MUSIC) {
    audioCurrentMusicTrack = trackNum;
  }
*/
}

// Core helper: start a specific track by index, using its metadata flags.
void audioPlayTrackByIndex(byte t_index) {
  if (pTsunami == nullptr) {
    return;
  }
/*  if (t_index >= NUM_AUDIO_TRACKS) {
    return;
  }

  // Read fields directly from the table (no local struct, no &)
  unsigned int trackNum = audioTracks[t_index].trackNum;
  byte  category = audioTracks[t_index].category;
  byte  flags    = audioTracks[t_index].flags;

  // Handle NON_INTERRUPT flag: do not start if same category already playing.
  if (flags & AUDIO_FLAG_NON_INTERRUPT) {
    if (category == AUDIO_CAT_VOICE && audioCurrentVoiceTrack != 0) {
      return;
    }
    if (category == AUDIO_CAT_SFX && audioCurrentSfxTrack != 0) {
      return;
    }
    if (category == AUDIO_CAT_MUSIC && audioCurrentMusicTrack != 0) {
      return;
    }
  }

  // Compute effective gain for this category (master + per-category trim).
  int gainDb = audioGetEffectiveGainDb(category);

  // Apply per-track gain before playing.
  pTsunami->trackGain((int)trackNum, gainDb);

  if (flags & AUDIO_FLAG_LOOP) {
    pTsunami->trackLoad((int)trackNum, 0, false);
    pTsunami->trackLoop((int)trackNum, true);
    pTsunami->trackPlayPoly((int)trackNum, 0, false);
  }
  else {
    pTsunami->trackLoop((int)trackNum, false);
    pTsunami->trackPlayPoly((int)trackNum, 0, false);
  }

  audioSetCurrentFromIndex(t_index, true);
*/
}

// Stop a track by Tsunami track number (001..nnn).
void audioStopTrack(unsigned int t_trackNum) {
  if (t_trackNum == 0 || pTsunami == nullptr) {
    return;
  }
  pTsunami->trackStop((int)t_trackNum);

  // Walk the table once to clear the appropriate "current" state if needed.
  for (byte i = 0; i < NUM_AUDIO_TRACKS; i++) {
    if (audioTracks[i].trackNum == t_trackNum) {
      audioSetCurrentFromIndex(i, false);
      break;
    }
  }
}

// Convenience: play by raw Tsunami track number (only uses flags/category if it finds a match).
void audioPlayTrack(unsigned int t_trackNum) {
  if (pTsunami == nullptr || t_trackNum == 0) {
    return;
  }
  for (byte i = 0; i < NUM_AUDIO_TRACKS; i++) {
    if (audioTracks[i].trackNum == t_trackNum) {
      audioPlayTrackByIndex(i);
      return;
    }
  }
}

// For voice tracks where we have several within a category, we can use the voiceSubcat field and pick a random matching entry.
// Call sites will now pass the VOICE_SUB_* consts.
// Examples of how to use this in mode code:
// On Enhanced game start:
//   audioPlayRandomVoice(VOICE_SUB_GAME_START);
// When switching to Player 2:
//   audioPlayRandomVoice(VOICE_SUB_PLAYER_UP);
// When a ball is saved:
//   audioPlayRandomVoice(VOICE_SUB_BALL_SAVED);
// After a drain with terrible play:
//   audioPlayRandomVoice(VOICE_SUB_BAD_PLAY);

// Very simple pseudo-random index 0..(n-1). Replace with better RNG if desired.
int audioRandomInt(int t_maxExclusive) {
  if (t_maxExclusive <= 0) {
    return 0;
  }
  return (int)(millis() % (unsigned long)t_maxExclusive);
}

void audioPlayRandomVoice(byte t_subcat) {
  if (pTsunami == nullptr) {
    return;
  }
/*
  byte indices[NUM_AUDIO_TRACKS];
  byte count = 0;

  for (byte i = 0; i < NUM_AUDIO_TRACKS; i++) {
    if (audioTracks[i].category == AUDIO_CAT_VOICE &&
      audioTracks[i].voiceSubcat == t_subcat) {
      indices[count++] = i;
    }
  }

  if (count == 0) {
    return;
  }

  int pick = audioRandomInt(count);
  byte chosenIndex = indices[pick];
  audioPlayTrackByIndex(chosenIndex);
*/
}

// Finally, some named wrappers for specific SFX/music use cases:
// Start/stop coaster "clicking up hill" loop
void audioStartCoasterClimb() {
  audioPlayTrackByIndex(AUDIO_IDX_SFX_COASTER_CLIMB);
}

void audioStopCoasterClimb() {
  audioStopTrack(audioTracks[AUDIO_IDX_SFX_COASTER_CLIMB].trackNum);
}

// Play a single roar down the hill
void audioPlayCoasterDrop() {
  audioPlayTrackByIndex(AUDIO_IDX_SFX_COASTER_DROP);
}

// Short scream SFX
void audioPlayScream() {
  audioPlayTrackByIndex(AUDIO_IDX_SFX_SCREAM);
}

// Background music helpers
void audioStartMainMusic() {
  audioPlayTrackByIndex(AUDIO_IDX_MUSIC_MAIN);
}

void audioStartMultiballMusic() {
  if (audioCurrentMusicTrack != 0 &&
    audioCurrentMusicTrack != audioTracks[AUDIO_IDX_MUSIC_MULTIBALL].trackNum) {
    audioStopTrack(audioCurrentMusicTrack);
  }
  audioPlayTrackByIndex(AUDIO_IDX_MUSIC_MULTIBALL);
}

void audioStopAllMusic() {
  if (audioCurrentMusicTrack != 0) {
    audioStopTrack(audioCurrentMusicTrack);
  }
}

// ********************************************************************************************************************************
// ***** OLD TEST CODE CAN BE REMOVED FROM PROJECT ********************************************************************************
// ********************************************************************************************************************************

void runBasicDiagnostics() {
  // Let's send some test messages to the Slave Arduino in the head.
  // void sendMAStoSLVMode(const byte t_mode);                     // RS485_TYPE_MAS_TO_SLV_MODE
  // void getMAStoSLVMode(byte* t_mode);                           // RS485_TYPE_MAS_TO_SLV_MODE

  // void sendMAStoSLVScoreQuery();                                                            // RS485_TYPE_MAS_TO_SLV_SCORE_QUERY
  // void sendSLVtoMASScoreReport(const byte t_10K, const byte t_100K, const byte t_million);  // RS485_TYPE_SLV_TO_MAS_SCORE_REPORT
  // void getSLVtoMASScoreReport(byte* t_10K, byte* t_100K, byte* t_million);                  // RS485_TYPE_SLV_TO_MAS_SCORE_REPORT

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);  // Wait for right flipper button press to continue
  pMessage->sendMAStoSLVTiltLamp(false);
  delay(500);
  pMessage->sendMAStoSLVTiltLamp(true);
  delay(500);
  pMessage->sendMAStoSLVTiltLamp(false);
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVGILamp(true);
  delay(500);
  pMessage->sendMAStoSLVGILamp(false);
  delay(500);
  pMessage->sendMAStoSLVGILamp(true);
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVCreditInc(3);
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVCreditInc(1);
  pMessage->sendMAStoSLVCreditInc(1);
  pMessage->sendMAStoSLVCreditInc(1);
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVCreditDec();
  pMessage->sendMAStoSLVCreditDec();
  pMessage->sendMAStoSLVCreditDec();
  pMessage->sendMAStoSLVCreditDec();
  pMessage->sendMAStoSLVCreditDec();
  pMessage->sendMAStoSLVCreditDec();
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell10K();
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBell100K();
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLVBellSelect();
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLV10KUnitPulse();
  pMessage->sendMAStoSLV10KUnitPulse();
  pMessage->sendMAStoSLV10KUnitPulse();
  pMessage->sendMAStoSLV10KUnitPulse();
  pMessage->sendMAStoSLV10KUnitPulse();
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLV10KUnitPulse();
  delay(10);
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLV10KUnitPulse();
  delay(10);
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLV10KUnitPulse();
  delay(10);
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLV10KUnitPulse();
  delay(10);
  pMessage->sendMAStoSLVBell10K();
  pMessage->sendMAStoSLVBell100K();
  pMessage->sendMAStoSLVBellSelect();
  pMessage->sendMAStoSLV10KUnitPulse();
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreAbs(999);
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreReset();
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreAbs(121);
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreReset();
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreAbs(998);  // 9,980,000
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(4);  // 9,990,000
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(-4);  // 9,950,000
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreAbs(989);   // 9,890,000
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(40);  // 9,990,000
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(-40);  // 9,590,000
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreAbs(3);  // 30,000
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(-8);  //  0
  delay(500);
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(4);  //  40,000
  delay(500);

  // Six 10K increments in a row with no pauses
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(1);
  pMessage->sendMAStoSLVScoreInc10K(1);
  pMessage->sendMAStoSLVScoreInc10K(1);
  pMessage->sendMAStoSLVScoreInc10K(1);
  pMessage->sendMAStoSLVScoreInc10K(1);
  pMessage->sendMAStoSLVScoreInc10K(1);
  delay(500);

  // Five, pause, one, pause, one 10K increments
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(6);
  pMessage->sendMAStoSLVScoreInc10K(1);
  delay(500);

  // Six negative 10K increments in a row with no pauses
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(-1);
  pMessage->sendMAStoSLVScoreInc10K(-1);
  pMessage->sendMAStoSLVScoreInc10K(-1);
  pMessage->sendMAStoSLVScoreInc10K(-1);
  pMessage->sendMAStoSLVScoreInc10K(-1);
  pMessage->sendMAStoSLVScoreInc10K(-1);
  delay(500);

  // Five, pause, one, pause, one 10K decrements
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(-6);
  pMessage->sendMAStoSLVScoreInc10K(-1);
  delay(500);

  // Six 100K increments in a row with no pauses
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(10);
  pMessage->sendMAStoSLVScoreInc10K(10);
  pMessage->sendMAStoSLVScoreInc10K(10);
  pMessage->sendMAStoSLVScoreInc10K(10);
  pMessage->sendMAStoSLVScoreInc10K(10);
  pMessage->sendMAStoSLVScoreInc10K(10);
  delay(500);

  // Five, pause, one, pause, one 100K increments
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(60);
  pMessage->sendMAStoSLVScoreInc10K(10);
  delay(500);

  // Six negative 100K increments in a row with no pauses
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(-10);
  pMessage->sendMAStoSLVScoreInc10K(-10);
  pMessage->sendMAStoSLVScoreInc10K(-10);
  pMessage->sendMAStoSLVScoreInc10K(-10);
  pMessage->sendMAStoSLVScoreInc10K(-10);
  pMessage->sendMAStoSLVScoreInc10K(-10);
  delay(500);

  // Five, pause, one, pause, one 100K decrements
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreInc10K(-60);
  pMessage->sendMAStoSLVScoreInc10K(-10);
  delay(500);

  // 2, pause, 3, pause, 4, pause,  5, pause, 5, pause, 1 10K increments
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);  // Gives 14 single advances
  pMessage->sendMAStoSLVScoreInc10K(2);
  pMessage->sendMAStoSLVScoreInc10K(3);
  pMessage->sendMAStoSLVScoreInc10K(4);
  pMessage->sendMAStoSLVScoreInc10K(5);
  pMessage->sendMAStoSLVScoreInc10K(6);
  delay(500);

  // 5, pause, 5, pause, 1, pause, 1 10K increments
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);  // 5-5-4 as expected
  pMessage->sendMAStoSLVScoreInc10K(11);
  pMessage->sendMAStoSLVScoreInc10K(1);
  delay(500);

  // 2, pause, 3, pause, 4, pause, 5, pause, 5, pause, 1 100K increments
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);  // Gives 14 single advances
  pMessage->sendMAStoSLVScoreInc10K(20);
  pMessage->sendMAStoSLVScoreInc10K(30);
  pMessage->sendMAStoSLVScoreInc10K(40);
  pMessage->sendMAStoSLVScoreInc10K(50);
  pMessage->sendMAStoSLVScoreInc10K(60);
  delay(500);

  // 2 @ 100K, pause, 3 @ 10K, pause, 5 @ 10K, pause, 5 @ 10K, pause, 2 @ 10K decrements
  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);  // Works perfectly!
  pMessage->sendMAStoSLVScoreInc10K(20);
  pMessage->sendMAStoSLVScoreInc10K(3);
  pMessage->sendMAStoSLVScoreInc10K(-12);
  delay(500);

  waitForSwitchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  pMessage->sendMAStoSLVScoreQuery();
  pMessage->sendMAStoSLVCreditStatusQuery();

  while (true) {
    // See if there is an incoming message for us...
    byte msgType = pMessage->available();
    while (msgType != RS485_TYPE_NO_MESSAGE) {
      switch (msgType) {
      case RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS:
      {
        bool creditsAvailable = false;
        pMessage->getSLVtoMASCreditStatus(&creditsAvailable);
        sprintf(lcdString, "Credits: %s", creditsAvailable ? "YES" : "NO");
        pLCD2004->println(lcdString);
      }
      break;
      case RS485_TYPE_SLV_TO_MAS_SCORE_REPORT:
      {
        int t_score = 0;
        pMessage->getSLVtoMASScoreReport(&t_score);
        sprintf(lcdString, "Score Rpt: %d", t_score);
        pLCD2004->println(lcdString); Serial.println(lcdString);
      }
      break;
      default:
        sprintf(lcdString, "MSG TYPE ERROR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString);
        // It's printing a, b, c, etc. i.e. successive characters **************************************************************************
      }
      msgType = pMessage->available();
    }
  }
}

void runRudimentarySwitchAndLampTests() {
  byte shakerMotorPower = 0;  // Tested range 12/12/25 is 70 to 140.  At 150 the hats sometimes false trigger.

  while (true) {
    // Fast read all 64 Centipede #2 inputs (16 bits at a time) into switchNewState[] before checking any switches.
    // This is much faster (2-3ms) than calling digitalRead() for each switch individually (~30ms total).
    for (int i = 0; i < 4; i++) {
      switchNewState[i] = pShiftRegister->portRead(4 + i);  // read ports 4..7 which map to Centipede #2 inputs
    }

    // Check flipper buttons
    if (switchClosed(SWITCH_IDX_FLIPPER_LEFT_BUTTON)) {  // Fire the left flipper
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerInitial);
      delay(deviceParm[DEV_IDX_FLIPPER_LEFT].timeOn * 10);
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerHold);
      delay(20);
      while (switchClosedImmediate(SWITCH_IDX_FLIPPER_LEFT_BUTTON)) {  // Keep holding the flipper while button is pressed
        delay(10);
      }
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, LOW);  // Turn OFF
      pMessage->sendMAStoSLVScoreFlash(400);  // Flash 4M
    }

    if (switchClosed(SWITCH_IDX_FLIPPER_RIGHT_BUTTON)) {  // Fire the right flipper
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, deviceParm[DEV_IDX_FLIPPER_RIGHT].powerInitial);
      delay(deviceParm[DEV_IDX_FLIPPER_RIGHT].timeOn * 10);
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, deviceParm[DEV_IDX_FLIPPER_RIGHT].powerHold);
      delay(20);
      while (switchClosedImmediate(SWITCH_IDX_FLIPPER_RIGHT_BUTTON)) {  // Keep holding the flipper while button is pressed
        delay(10);
      }
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, LOW);  // Turn OFF
      pMessage->sendMAStoSLVScoreFlash(0);  // Flash off (same as sending absScore(0)
    }
    // Check left slingshot
    if (switchClosed(SWITCH_IDX_SLINGSHOT_LEFT)) {
      const byte idx = DEV_IDX_SLINGSHOT_LEFT;
      analogWrite(deviceParm[DEV_IDX_SLINGSHOT_LEFT].pinNum, deviceParm[idx].powerInitial);           // energize at initial strength
      delay((int)deviceParm[DEV_IDX_SLINGSHOT_LEFT].timeOn * 10);                // hold for timeOn * 10ms
      analogWrite(deviceParm[DEV_IDX_SLINGSHOT_LEFT].pinNum, LOW);               // turn off if no hold strength
      delay(100);
    }
    // Check right slingshot
    if (switchClosed(SWITCH_IDX_SLINGSHOT_RIGHT)) {
      analogWrite(deviceParm[DEV_IDX_SLINGSHOT_RIGHT].pinNum, deviceParm[DEV_IDX_SLINGSHOT_RIGHT].powerInitial);           // energize at initial strength
      delay((int)deviceParm[DEV_IDX_SLINGSHOT_RIGHT].timeOn * 10);                // hold for timeOn * 10ms
      analogWrite(deviceParm[DEV_IDX_SLINGSHOT_RIGHT].pinNum, LOW);               // turn off if no hold strength
      delay(100);
    }
    // Check pop bumper
    if (switchClosed(SWITCH_IDX_BUMPER_E)) {
      analogWrite(deviceParm[DEV_IDX_POP_BUMPER].pinNum, deviceParm[DEV_IDX_POP_BUMPER].powerInitial);           // energize at initial strength
      const byte lampIdx = LAMP_IDX_E;
      const byte pinNum = lampParm[lampIdx].pinNum;
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON lamp
      delay((int)deviceParm[DEV_IDX_POP_BUMPER].timeOn * 10);                // hold for timeOn * 10ms
      analogWrite(deviceParm[DEV_IDX_POP_BUMPER].pinNum, LOW);               // turn off if no hold strength
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH); // Turn OFF lamp
    }
    // For each of the other bumpers, light the corresponding lamp when hit for 500ms then turn it off...
    if (switchClosed(SWITCH_IDX_BUMPER_S)) {
      const byte lampIdx = LAMP_IDX_S;
      const byte pinNum = lampParm[lampIdx].pinNum;
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON lamp
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH); // Turn OFF lamp
    }
    if (switchClosed(SWITCH_IDX_BUMPER_C)) {
      const byte lampIdx = LAMP_IDX_C;
      const byte pinNum = lampParm[lampIdx].pinNum;
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON lamp
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH); // Turn OFF lamp
    }
    if (switchClosed(SWITCH_IDX_BUMPER_R)) {
      const byte lampIdx = LAMP_IDX_R;
      const byte pinNum = lampParm[lampIdx].pinNum;
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON lamp
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH); // Turn OFF lamp
    }
    if (switchClosed(SWITCH_IDX_BUMPER_A)) {
      const byte lampIdx = LAMP_IDX_A;
      const byte pinNum = lampParm[lampIdx].pinNum;
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON lamp
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH); // Turn OFF lamp
    }
    if (switchClosed(SWITCH_IDX_BUMPER_M)) {
      const byte lampIdx = LAMP_IDX_M;
      const byte pinNum = lampParm[lampIdx].pinNum;
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON lamp
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH); // Turn OFF lamp
    }
    if (switchClosed(SWITCH_IDX_BUMPER_O)) {
      const byte lampIdx = LAMP_IDX_O;
      const byte pinNum = lampParm[lampIdx].pinNum;
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON lamp
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH); // Turn OFF lamp
    }
    // Check hat switches - light corresponding lamps
    if (switchClosed(SWITCH_IDX_HAT_LEFT_TOP)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_LEFT_TOP].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_LEFT_TOP].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_HAT_LEFT_BOTTOM)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_LEFT_BOTTOM].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_LEFT_BOTTOM].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_HAT_RIGHT_TOP)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_RIGHT_TOP].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_RIGHT_TOP].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_HAT_RIGHT_BOTTOM)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_RIGHT_BOTTOM].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_RIGHT_BOTTOM].pinNum, HIGH);
    }
    // Check kickout switches - fire solenoids and flash corresponding lamps
    if (switchClosed(SWITCH_IDX_KICKOUT_LEFT)) {
      const byte devIdx = DEV_IDX_KICKOUT_LEFT;
      pMessage->sendMAStoSLVScoreInc10K(50);  // Will resolve to 5x 100K advances on slave
      digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, HIGH); // Start the score motor (explicit, non?PWM)
      delay(588);  // Let score motor run for 4 cycles = 147ms * 4 = 588ms
      analogWrite(deviceParm[devIdx].pinNum, deviceParm[devIdx].powerInitial);  // Fire kickout solenoid
      delay((int)deviceParm[devIdx].timeOn * 10); // Leave coil energized in timeOn 10ms ticks
      analogWrite(deviceParm[devIdx].pinNum, LOW);  // De-energize kickout solenoid
      digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, LOW); // Stop the score motor (explicit)
    }
    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT)) {
      const byte devIdx = DEV_IDX_KICKOUT_RIGHT;
      pMessage->sendMAStoSLVScoreInc10K(50);  // Will resolve to 5x 100K advances on slave
      digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, HIGH); // Start the score motor (explicit, non?PWM)
      delay(588);  // Let score motor run for 4 cycles = 147ms * 4 = 588ms
      analogWrite(deviceParm[devIdx].pinNum, deviceParm[devIdx].powerInitial);  // Fire kickout solenoid
      delay((int)deviceParm[devIdx].timeOn * 10); // Leave coil energized in timeOn 10ms ticks
      analogWrite(deviceParm[devIdx].pinNum, LOW);  // De-energize kickout solenoid
      digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, LOW); // Stop the score motor (explicit)
    }
    // Check drain switches
    if (switchClosed(SWITCH_IDX_DRAIN_LEFT)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_LEFT].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_LEFT].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_DRAIN_CENTER)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_8].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_8].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_DRAIN_RIGHT)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_RIGHT].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_RIGHT].pinNum, HIGH);
    }
    // Check gobble hole - light special when lit
    if (switchClosed(SWITCH_IDX_GOBBLE)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPECIAL].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPECIAL].pinNum, HIGH);
    }
    // Check left side target switches - light nearby G.I. lamps
    if (switchClosed(SWITCH_IDX_LEFT_SIDE_TARGET_2)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_TOP].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_TOP].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_LEFT_SIDE_TARGET_3)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_CENTER_1].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_CENTER_1].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_LEFT_SIDE_TARGET_4)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_CENTER_1].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_CENTER_1].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_LEFT_SIDE_TARGET_5)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_CENTER_2].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_LEFT_CENTER_2].pinNum, HIGH);
    }
    // Check right side target switches - light nearby G.I. lamps
    if (switchClosed(SWITCH_IDX_RIGHT_SIDE_TARGET_1)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_TOP].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_TOP].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_RIGHT_SIDE_TARGET_2)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_TOP].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_TOP].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_RIGHT_SIDE_TARGET_3)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_CENTER_1].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_CENTER_1].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_RIGHT_SIDE_TARGET_4)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_CENTER_1].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_CENTER_1].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_RIGHT_SIDE_TARGET_5)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_CENTER_2].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_GI_RIGHT_CENTER_2].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_START_BUTTON)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_5].pinNum, LOW);
      const byte idx = DEV_IDX_BALL_TRAY_RELEASE;
      analogWrite(deviceParm[DEV_IDX_BALL_TRAY_RELEASE].pinNum, deviceParm[idx].powerInitial);           // energize at initial strength
      delay((int)deviceParm[DEV_IDX_BALL_TRAY_RELEASE].timeOn * 10);                // hold for timeOn * 10ms
      analogWrite(deviceParm[DEV_IDX_BALL_TRAY_RELEASE].pinNum, deviceParm[idx].powerHold);           // energize at hold strength
      // Hold at low power until Start button is released
      sprintf(lcdString, "dR = %02d", pShiftRegister->digitalRead(switchParm[SWITCH_IDX_START_BUTTON].pinNum));
      pLCD2004->println(lcdString);
      // Wait while Start is physically pressed (active LOW)
      while (pShiftRegister->digitalRead(switchParm[SWITCH_IDX_START_BUTTON].pinNum) == LOW) {
        sprintf(lcdString, "dR = %02d", pShiftRegister->digitalRead(switchParm[SWITCH_IDX_START_BUTTON].pinNum));
        pLCD2004->println(lcdString);
        delay(500);
      }
      analogWrite(deviceParm[DEV_IDX_BALL_TRAY_RELEASE].pinNum, LOW);               // turn off if no hold strength
      delay(2000);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_5].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_DIAG_1)) {
      digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, HIGH); // Start the score motor
      delay(5000);
      digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, LOW); // Stop the score motor (explicit)
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_1].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_1].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_DIAG_2)) {  // "-"
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_2].pinNum, LOW);
      if (shakerMotorPower >= 80) {  // 70 is the minimum power to run the motor
        shakerMotorPower = shakerMotorPower - 10;
        analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, shakerMotorPower); // energize shaker motor
      }
      else if (shakerMotorPower > 0) {
        shakerMotorPower = 0;
        analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, shakerMotorPower); // turn off shaker motor
      }
      sprintf(lcdString, "Shaker %i", shakerMotorPower); pLCD2004->println(lcdString);
      delay(500);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_2].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_DIAG_3)) {  // "+"
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_3].pinNum, LOW);
      if (shakerMotorPower < 70) {  // Minimum power to run the motor
        shakerMotorPower = 70;
        analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, shakerMotorPower); // energize shaker motor
      }
      else if (shakerMotorPower < 240) {  // Can't exceed 255
        shakerMotorPower = shakerMotorPower + 10;
        analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, shakerMotorPower); // energize shaker motor
      }
      sprintf(lcdString, "Shaker %i", shakerMotorPower); pLCD2004->println(lcdString);
      delay(500);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_3].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_DIAG_4)) {  // TEST SOFTWARE RESET
      // Send message to slave to also reset
      pMessage->sendMAStoSLVCommandReset();
      delay(10);  // Allow message to fully transmit and Slave to process before Master resets
      softwareReset();
    }
    if (switchClosed(SWITCH_IDX_KNOCK_OFF)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_6].pinNum, LOW);
      const byte idx = DEV_IDX_BALL_TROUGH_RELEASE;
      analogWrite(deviceParm[DEV_IDX_BALL_TROUGH_RELEASE].pinNum, deviceParm[idx].powerInitial);           // energize at initial strength
      delay((int)deviceParm[DEV_IDX_BALL_TROUGH_RELEASE].timeOn * 10);                // hold for timeOn * 10ms
      analogWrite(deviceParm[DEV_IDX_BALL_TROUGH_RELEASE].pinNum, LOW);               // turn off if no hold strength
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_6].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_COIN_MECH)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_7].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_7].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_RED_9].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_RED_9].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_TILT_BOB)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_RED_7].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_RED_7].pinNum, HIGH);
    }
    delay(10);  // Simulate 10ms loop delay
  }
}

// ********************************************************************************************************************************
// ********************************************************************************************************************************
// ********************************************************************************************************************************

/*
// Example code to query Slave for credits available
  // Send message asking Slave if we have any credits; will return a message with bool TRUE or FALSE
  sprintf(lcdString, "Sending to Slave"); Serial.println(lcdString);
  pMessage->sendRequestCredit();
  sprintf(lcdString, "Sent to Slave"); Serial.println(lcdString);

  // Wait for response from Slave...
  // msgType ' ' (blank) means there was no message waiting for us.
  // msgType 'C' means this is a response from Slave we're expecting
  // For any message with contents (such as this, which is a bool value), we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.

  // Wait for a respons from Slave re: are there any credits?
  // msgType will be 'C' if there is a response, or ' ' (
  while (msgType == ' ') {
    msgType = pMessage->available();
  }

  bool credits = false;
  switch(msgType) {
    case 'C' :  // New credit message in incoming RS485 buffer as expected
      pMessage->getCreditSuccess(&credits);
      // Just calling the function updates "credits" value ;-)
      if (credits) {
        sprintf(lcdString, "True!");
      } else {
        sprintf(lcdString, "False!");
      }
      pLCD2004->println(lcdString);
      break;
    default:
      sprintf(lcdString, "MSG TYPE ERROR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString);
      // It's printing a, b, c, etc. i.e. successive characters **************************************************************************
  }
  msgType = pMessage->available();
*/

// ***** CENTIPEDE INPUT SPEED TEST RESULTS FOR FUTURE REFERENCE *****
// Here is some code that reads Centipede #2 inputs manually, one at a time, and displays any closed switches.
// TESTING RESULTS SHOW IT TAKES 30ms TO READ ALL 64 SWITCHES MANUALLY
/*
while (true) {
  for (int i = 0; i < 64; i++) {  // We'll read Centipede #2, but the function will correct for this
    if (switchClosed(i)) {
      sprintf(lcdString, "SW %02d CLOSED", i);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
    }
  }
  long unsigned int endMillis = millis();
}

// Here is some code that reads Centipede #2 inputs 16 bits at a time using portRead(), and displays any that are closed.
// TESTING RESULTS SHOW IT ONLY TAKES 2-3ms TO READ ALL 64 SWITCHES USING portRead()!
/*
while (true) {
  // This block of code will read all 64 inputs from Centipede #2, 16 bits at a time, and display any that are closed.
  // We will use our switchOldState/switchNewState arrays to hold the 64 bits read from Centipede #2.
  // We will use the shiftRegister->portRead function to read 16 bits at a time.
  for (int i = 0; i < 4; i++) {  // There are 4 ports of 16 bits each = 64 bits total
    switchNewState[i] = pShiftRegister->portRead(i + 4);  // "+4" so we'll read from Centipede #2; input ports 4..7
    // Now check each of the 16 bits read for closed switches
    for (int bitNum = 0; bitNum < 16; bitNum++) {
      // Check if this bit is LOW (closed switch)
      if ((switchNewState[i] & (1 << bitNum)) == 0) {
        int switchNum = (i * 16) + bitNum;  // Calculate switch number 0..63
        sprintf(lcdString, "SW %02d CLOSED", switchNum);
        pLCD2004->println(lcdString);
        Serial.println(lcdString);
        delay(1000);
      }
    }
  }
}
*/

// ********************************************************************************************************************************
// SAMPLE LOOP CODE AND COMMENTS FOR FUTURE REFERENCE:

// At top of 10ms game-play loop, read the status of the two flipper buttons continuously until 10ms has transpired i.e. ready for another loop iteration.
//   Handle flipper button presses immediately, outside of switch state table processing for 1st priority handling.
// Next read all 37 non-flipper Centipede #2 inputs (16 bits at a time) into switchNewState[] before checking any switches.
//   Handle slingshot and pop bumper switches immediately, outside of switch state table processing for 2nd priority handline.
//   Then process the rest of the switches via the switch state table as before (3rd priority handling).
// This way the flippers, slingshots, and bumpers are very responsive, while the other switches can be debounced via the state table.

/*
  void loop() {
    unsigned long loopStart = millis();

    // Fast flipper polling (uses remaining time after other tasks)
    while (millis() - loopStart < 10) {
      if (switchClosed(SWITCH_IDX_FLIPPER_LEFT_BUTTON)) {
        // Fire left flipper instantly
      }
      if (switchClosed(SWITCH_IDX_FLIPPER_RIGHT_BUTTON)) {
        // Fire right flipper instantly
      }
    }

    // Once per 10ms: read shift register switches
    for (int i = 0; i < 4; i++) {
      switchNewState[i] = pShiftRegister->portRead(i + 4);
    }

    // Process bumpers, lamps, score updates, etc.
    // ...

    // Update coil timers (decrement countdown, etc.)
    // ...
  }
*/
