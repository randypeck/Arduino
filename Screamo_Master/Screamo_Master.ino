// Screamo_Master.INO Rev: 11/17/25
// 11/12/25: Moved Right Kickout from pin 13 to pin 44 to avoid MOSFET firing twice on power-up.  Don't use MOSFET on pin 13.
// Centipede #1 (0..63) is LAMP OUTPUTS
// Centipede #2 (64..127) is SWITCH INPUTS

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
#include <Tsunami.h>

// #include <EEPROM.h>               // For saving and recalling score, to persist between power cycles.
// const int EEPROM_ADDR_SCORE = 0;  // Address to store 16-bit score (uses addr 0 and 1)

const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 11/15/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which effectively makes it a global.
// No need to pass lcdString[] to any functions that use it!

// ************************************************
// ***** COIL AND MOTOR STRUCTS AND CONSTANTS *****
// ************************************************

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

// Define a struct to store coil/motor pin number, power, and time parms.  Coils that may be held on include a hold strength.
// In this case, pinNum refers to an actual Arduino Mega R3 I/O pin number.
// For Flippers, original Ball Gate, new Ball Release Post, and other coils that may be held after initial activation, include a
// "hold strength" parm as well as initial strength and time on.
// If Hold Strength == 0, then simply release after hold time; else change to Hold Strength until released by software.
// Example usage:
//   analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerInitial);
//   delay(deviceParm[DEV_IDX_FLIPPER_LEFT].timeOn);
//   analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerHold);
// loopsRemaining will be set to "timeOn" and decremented every 10ms in main loop until it reaches zero, at which time power will be set to powerHold.
struct deviceParmStruct {
  byte pinNum;          // Arduino pin number for this coil/motor
  byte powerInitial;    // 0..255 PWM power level when first energized
  byte timeOn;          // Number of 10ms loops to hold initial power level before changing to hold level or turning off
  byte powerHold;       // 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
  byte loopsRemaining;  // Internal use only: number of 10ms loops remaining at initial power level
};

deviceParmStruct deviceParm[NUM_DEVS] = {
  // pinNum, powerInitial, timeOn(ms), powerHold, loopsRemaining
  {  5, 255,  5,   0, 0 },  // DEV_IDX_POP_BUMPER          =  0, PWM MOSFET
  {  4, 200, 10,   0, 0 },  // DEV_IDX_KICKOUT_LEFT        =  1, PWM MOSFET (cannot modify PWM freq.)
  { 44, 190, 10,   0, 0 },  // DEV_IDX_KICKOUT_RIGHT       =  2, PWM MOSFET
  {  6, 255, 10,   0, 0 },  // DEV_IDX_SLINGSHOT_LEFT      =  3, PWM MOSFET
  {  7, 255, 10,   0, 0 },  // DEV_IDX_SLINGSHOT_RIGHT     =  4, PWM MOSFET
  {  8, 150, 10,  40, 0 },  // DEV_IDX_FLIPPER_LEFT        =  5, PWM MOSFET.  200 hits gobble hole too hard; 150 can get to top of p/f.
  {  9, 150, 10,  40, 0 },  // DEV_IDX_FLIPPER_RIGHT       =  6, PWM MOSFET
  { 10, 200, 20,  40, 0,},  // DEV_IDX_BALL_TRAY_RELEASE   =  7, PWM MOSFET (original tray release)
  { 23, 255,  5,   0, 0 },  // DEV_IDX_SELECTION_UNIT      =  8, Non-PWM MOSFET; on/off only.
  { 24, 255,  5,   0, 0 },  // DEV_IDX_RELAY_RESET         =  9, Non-PWM MOSFET; on/off only.
  { 11, 150, 23,   0, 0,},  // DEV_IDX_BALL_TROUGH_RELEASE = 10, PWM MOSFET (new up/down post)
  // For the ball trough release coil, 150 is the right power; 100 could not guarantee it could retract if there was pressure holding it from balls above.
  // 230ms is just enough time for ball to get 1/2 way past post and momentum carries it so it won't get pinned by the post.
  // The post springs back fully extended before the next ball hits it.
    { 12, 200, 25,   0, 0 },  // DEV_IDX_MOTOR_SHAKER        = 11, PWM MOSFET
    { 26, 200,  4,   0, 0 },  // DEV_IDX_KNOCKER             = 12, Non-PWM MOSFET; on/off only.
    { 25, 255, 25,   0, 0 }   // DEV_IDX_MOTOR_SCORE         = 13, A/C SSR; on/off only; NOT PWM.
};

// ****************************************
// ***** SWITCH STRUCTS AND CONSTANTS *****
// ****************************************

// Define array index constants - this list is rather arbitrary and doesn't relate to device number/pin numbers:
// Flipper buttons are direct-wired to Arduino pins for faster response, so not included here.
// CABINET SWITCHES:
const byte SWITCH_IDX_START_BUTTON        =  0;
const byte SWITCH_IDX_DIAG_1              =  1;  // Back
const byte SWITCH_IDX_DIAG_2              =  2;  // Minus/Left
const byte SWITCH_IDX_DIAG_3              =  3;  // Plus/Right
const byte SWITCH_IDX_DIAG_4              =  4;  // Select
const byte SWITCH_IDX_KNOCK_OFF           =  5;
const byte SWITCH_IDX_COIN_MECH           =  6;
const byte SWITCH_IDX_BALL_PRESENT        =  7;  // (New) Ball present at bottom of ball lift
const byte SWITCH_IDX_TILT_BOB            =  8;
// PLAYFIELD SWITCHES:
const byte SWITCH_IDX_BUMPER_S            =  9;  // 'S' bumper switch
const byte SWITCH_IDX_BUMPER_C            = 10;  // 'C' bumper switch
const byte SWITCH_IDX_BUMPER_R            = 11;  // 'R' bumper switch
const byte SWITCH_IDX_BUMPER_E            = 12;  // 'E' bumper switch
const byte SWITCH_IDX_BUMPER_A            = 13;  // 'A' bumper switch
const byte SWITCH_IDX_BUMPER_M            = 14;  // 'M' bumper switch
const byte SWITCH_IDX_BUMPER_O            = 15;  // 'O' bumper switch
const byte SWITCH_IDX_KICKOUT_LEFT        = 16;
const byte SWITCH_IDX_KICKOUT_RIGHT       = 17;
const byte SWITCH_IDX_SLINGSHOT_LEFT      = 18;  // Two switches wired in parallel
const byte SWITCH_IDX_SLINGSHOT_RIGHT     = 19;  // Two switches wired in parallel
const byte SWITCH_IDX_HAT_LEFT_TOP        = 20;
const byte SWITCH_IDX_HAT_LEFT_BOTTOM     = 21;
const byte SWITCH_IDX_HAT_RIGHT_TOP       = 22;
const byte SWITCH_IDX_HAT_RIGHT_BOTTOM    = 23;
// Note that there is no "LEFT_SIDE_TARGET_1" on the playfield; starting left side targets at 2 so they match right-side target numbers.
const byte SWITCH_IDX_LEFT_SIDE_TARGET_2  = 24;  // Long narrow side target near top left
const byte SWITCH_IDX_LEFT_SIDE_TARGET_3  = 25;  // Upper switch above left kickout
const byte SWITCH_IDX_LEFT_SIDE_TARGET_4  = 26;  // Lower switch above left kickout
const byte SWITCH_IDX_LEFT_SIDE_TARGET_5  = 27;  // Below left kickout
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_1 = 28;  // Top right just below ball gate
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_2 = 29;  // Long narrow side target near top right
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_3 = 30;  // Upper switch above right kickout
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_4 = 31;  // Lower switch above right kickout
const byte SWITCH_IDX_RIGHT_SIDE_TARGET_5 = 32;  // Below right kickout
const byte SWITCH_IDX_GOBBLE              = 33;
const byte SWITCH_IDX_DRAIN_LEFT          = 34;  // Left drain switch index in Centipede input shift register
const byte SWITCH_IDX_DRAIN_CENTER        = 35;  // Center drain switch index in Centipede input shift register
const byte SWITCH_IDX_DRAIN_RIGHT         = 36;  // Right drain switch index in Centipede input shift register

const byte NUM_SWITCHES                   = 37;

// Define a struct to store switch parameters.
// In this case, pinNum refers to Centipede #2 input shift register pin number (64..127).
// The loopConst and loopCount are going to change so that we always trigger on a switch closure, but use debounce delay before checking if it's open or closed again.
// That is, unless we find we're getting phantom closures, in which case we'll need to debounce both closing and opening.
struct switchParmStruct {
  byte pinNum;        // Centipede #2 pin number for this switch (64..127)
  byte loopConst;     // Number of 10ms loops to confirm stable state change
  byte loopCount;     // Current count of 10ms loops with stable state (initialized to zero)
};

// Store Centipede #2 pin numbers (64..127) for switches (original pins + 64)
switchParmStruct switchParm[NUM_SWITCHES] = {
  { 118,  0,  0 },  // SWITCH_IDX_START_BUTTON        =  0  (54 + 64)
  { 115,  0,  0 },  // SWITCH_IDX_DIAG_1              =  1  (51 + 64)
  { 112,  0,  0 },  // SWITCH_IDX_DIAG_2              =  2  (48 + 64)
  { 113,  0,  0 },  // SWITCH_IDX_DIAG_3              =  3  (49 + 64)
  { 116,  0,  0 },  // SWITCH_IDX_DIAG_4              =  4  (52 + 64)
  { 127,  0,  0 },  // SWITCH_IDX_KNOCK_OFF           =  5  (63 + 64)
  { 117,  0,  0 },  // SWITCH_IDX_COIN_MECH           =  6  (53 + 64)
  { 114,  0,  0 },  // SWITCH_IDX_BALL_PRESENT        =  7  (50 + 64)
  { 119,  0,  0 },  // SWITCH_IDX_TILT_BOB            =  8  (55 + 64)
  { 104,  0,  0 },  // SWITCH_IDX_BUMPER_S            =  9  (40 + 64)
  {  90,  0,  0 },  // SWITCH_IDX_BUMPER_C            = 10  (26 + 64)
  {  87,  0,  0 },  // SWITCH_IDX_BUMPER_R            = 11  (23 + 64)
  {  80,  0,  0 },  // SWITCH_IDX_BUMPER_E            = 12  (16 + 64)
  { 110,  0,  0 },  // SWITCH_IDX_BUMPER_A            = 13  (46 + 64)
  { 107,  0,  0 },  // SWITCH_IDX_BUMPER_M            = 14  (43 + 64)
  {  88,  0,  0 },  // SWITCH_IDX_BUMPER_O            = 15  (24 + 64)
  { 105,  0,  0 },  // SWITCH_IDX_KICKOUT_LEFT        = 16  (41 + 64)
  {  83,  0,  0 },  // SWITCH_IDX_KICKOUT_RIGHT       = 17  (19 + 64)
  {  84,  0,  0 },  // SWITCH_IDX_SLINGSHOT_LEFT      = 18  (20 + 64)
  {  98,  0,  0 },  // SWITCH_IDX_SLINGSHOT_RIGHT     = 19  (34 + 64)
  {  99,  0,  0 },  // SWITCH_IDX_HAT_LEFT_TOP        = 20  (35 + 64)
  {  81,  0,  0 },  // SWITCH_IDX_HAT_LEFT_BOTTOM     = 21  (17 + 64)
  {  89,  0,  0 },  // SWITCH_IDX_HAT_RIGHT_TOP       = 22  (25 + 64)
  {  92,  0,  0 },  // SWITCH_IDX_HAT_RIGHT_BOTTOM    = 23  (28 + 64)
  {  86,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_2  = 24  (22 + 64)
  { 108,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_3  = 25  (44 + 64)
  {  85,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_4  = 26  (21 + 64)
  {  96,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_5  = 27  (32 + 64)
  {  82,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_1 = 28  (18 + 64)
  { 111,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_2 = 29  (47 + 64)
  {  94,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_3 = 30  (30 + 64)
  {  95,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_4 = 31  (31 + 64)
  {  91,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_5 = 32  (27 + 64)
  { 109,  0,  0 },  // SWITCH_IDX_GOBBLE              = 33  (45 + 64)
  { 106,  0,  0 },  // SWITCH_IDX_DRAIN_LEFT          = 34  (42 + 64)
  {  97,  0,  0 },  // SWITCH_IDX_DRAIN_CENTER        = 35  (33 + 64)
  {  93,  0,  0 }   // SWITCH_IDX_DRAIN_RIGHT         = 36  (29 + 64)
};

// **************************************
// ***** LAMP STRUCTS AND CONSTANTS *****
// **************************************

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

// Define a struct to store lamp parameters.
// In this case, pinNum refers to Centipede #1 output shift register pin number (0..63).
struct lampParmStruct {
  byte pinNum;       // Centipede pin number for this lamp (0..63)
  byte groupNum;     // Group number for this lamp
  bool stateOn;      // true = lamp ON; false = lamp OFF
};

// Populate lampParm with pin numbers, groups, and initial state OFF
// Pin numbers range from 0 to 63 because these lamps are on Centipede #1 (outputs)
lampParmStruct lampParm[NUM_LAMPS] = {
  // pinNum, groupNum, stateOn
  { 52, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_LEFT_TOP       =  0
  { 34, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_LEFT_CENTER_1  =  1
  { 25, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_LEFT_CENTER_2  =  2
  { 20, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_LEFT_BOTTOM    =  3
  { 17, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_RIGHT_TOP      =  4
  { 39, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_RIGHT_CENTER_1 =  5
  { 49, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_RIGHT_CENTER_2 =  6
  { 61, LAMP_GROUP_GI,      LAMP_OFF }, // LAMP_IDX_GI_RIGHT_BOTTOM   = 7
  { 51, LAMP_GROUP_BUMPER,  LAMP_OFF }, // LAMP_IDX_S                 =  8
  { 16, LAMP_GROUP_BUMPER,  LAMP_OFF }, // LAMP_IDX_C                 =  9
  { 60, LAMP_GROUP_BUMPER,  LAMP_OFF }, // LAMP_IDX_R                 = 10
  { 28, LAMP_GROUP_BUMPER,  LAMP_OFF }, // LAMP_IDX_E                 = 11
  { 29, LAMP_GROUP_BUMPER,  LAMP_OFF }, // LAMP_IDX_A                 = 12
  { 18, LAMP_GROUP_BUMPER,  LAMP_OFF }, // LAMP_IDX_M                 = 13
  { 50, LAMP_GROUP_BUMPER,  LAMP_OFF }, // LAMP_IDX_O                 = 14
  { 21, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_1           = 15
  { 56, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_2           = 16
  { 53, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_3           = 17
  { 33, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_4           = 18
  { 36, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_5           = 19
  { 43, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_6           = 20
  { 24, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_7           = 21
  { 38, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_8           = 22
  { 26, LAMP_GROUP_WHITE,   LAMP_OFF }, // LAMP_IDX_WHITE_9           = 23
  { 23, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_1             = 24
  { 22, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_2             = 25
  { 48, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_3             = 26
  { 45, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_4             = 27
  { 35, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_5             = 28
  { 40, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_6             = 29
  { 27, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_7             = 30
  { 32, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_8             = 31
  { 57, LAMP_GROUP_RED,     LAMP_OFF }, // LAMP_IDX_RED_9             = 32
  { 30, LAMP_GROUP_HAT,     LAMP_OFF }, // LAMP_IDX_HAT_LEFT_TOP      = 33
  { 44, LAMP_GROUP_HAT,     LAMP_OFF }, // LAMP_IDX_HAT_LEFT_BOTTOM   = 34
  { 54, LAMP_GROUP_HAT,     LAMP_OFF }, // LAMP_IDX_HAT_RIGHT_TOP     = 35
  { 63, LAMP_GROUP_HAT,     LAMP_OFF }, // LAMP_IDX_HAT_RIGHT_BOTTOM  = 36
  { 47, LAMP_GROUP_KICKOUT, LAMP_OFF }, // LAMP_IDX_KICKOUT_LEFT      = 37
  { 41, LAMP_GROUP_KICKOUT, LAMP_OFF }, // LAMP_IDX_KICKOUT_RIGHT     = 38
  { 37, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // LAMP_IDX_SPECIAL           = 39
  { 19, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // LAMP_IDX_GOBBLE_1          = 40
  { 31, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // LAMP_IDX_GOBBLE_2          = 41
  { 58, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // LAMP_IDX_GOBBLE_3          = 42
  { 62, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // LAMP_IDX_GOBBLE_4          = 43
  { 42, LAMP_GROUP_GOBBLE,  LAMP_OFF }, // LAMP_IDX_GOBBLE_5          = 44
  { 46, LAMP_GROUP_NONE,    LAMP_OFF }, // LAMP_IDX_SPOT_NUMBER_LEFT  = 45
  { 59, LAMP_GROUP_NONE,    LAMP_OFF }  // LAMP_IDX_SPOT_NUMBER_RIGHT = 46
};

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
byte         modeCurrent      = MODE_UNDEFINED;
char         msgType          = ' ';

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

  // Set initial state for all MOSFET PWM output pins (pins 5-12)
  for (int i = 0; i < NUM_DEVS; i++) {
    digitalWrite(deviceParm[i].pinNum, LOW);         // Ensure all MOSFET outputs are off at startup.
    pinMode(deviceParm[i].pinNum, OUTPUT);
  }

  // Initialize I2C and Centipede expander as early as possible to minimize relay-on (turns on all lamps) at power-up
  Wire.begin();                                      // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Pinball_Centipede;            // Instantiate Centipede object as early as possible
  pShiftRegister->begin();                           // Set all registers to default.
  pShiftRegister->initScreamoMasterCentipedePins();

  // Increase PWM frequency for pins 11 and 12 (Timer 1) to reduce solenoid buzzing.
  // Pins 11 and 12 are Ball Trough Release coil and Shaker Motor control
  // Set Timer 1 prescaler to 1 for ~31kHz PWM frequency
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;

  // Increase PWM frequency for pins 9 and 10 (Timer 2) to reduce solenoid buzzing.
  // Pins 9 and 10 are Right Flipper and Ball Tray Release (original) coil control
  // Set Timer 2 prescaler to 1 for ~31kHz PWM frequency
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;

  // Increase PWM frequency for pin 6..8 (Timer 4) to reduce solenoid buzzing.
  // Pins 6, 7, 8 are Left and Right Slingshot, and Left Flipper coil control
  // Set Timer 4 prescaler to 1 for ~31kHz PWM frequency
  TCCR4B = (TCCR4B & 0b11111000) | 0x01;

  // Increase PWM frequency for pins44..46 (Timer 5) to reduce solenoid buzzing.
  // Pin 44 is Right Kickout coil control.
  // Pins44..46 are additional PWM outputs on Timer5 (Mega2560)
  // Set Timer 5 prescaler to 1 for ~31kHz PWM frequency
  TCCR5B = (TCCR5B & 0b11111000) | 0x01;

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
  Serial.println(lcdString);
  pLCD2004->println("Setup starting.");

  // *** INITIALIZE TSUNAMI WAV PLAYER OBJECT ***
  // Files must be WAV format, 16-bit PCM, 44.1kHz, Stereo.  Details in Tsunami.h comments.
  pTsunami = new Tsunami();  // Create Tsunami object on Serial3
  pTsunami->start();         // Start Tsunami WAV player


/*
  // Let's send some test messages to the Slave Arduino in the head.
  delay(2000);

  while (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == HIGH) { }  // Wait for right flipper button press to continue
  pLCD2004->println("G.I. On");
  pMessage->sendMAStoSLVGILamp(true);
  delay(1000);
  pLCD2004->println("10K Bell");
  pMessage->sendMAStoSLVBell10K();
  delay(1000);
  pLCD2004->println("Wait 1...");
  while (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == HIGH) { }
  pLCD2004->println("100K Bell");
  pMessage->sendMAStoSLVBell100K();
  delay(1000);
  pLCD2004->println("Wait 2...");
  while (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == HIGH) { }
  pLCD2004->println("Select Bell");
  pMessage->sendMAStoSLVBellSelect();
  delay(1000);
  pLCD2004->println("Wait 3...");
  while (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == HIGH) { }
  pLCD2004->println("Add 3 credits");
  pMessage->sendMAStoSLVCreditInc(3);
  delay(1000);
  pLCD2004->println("Wait 4...");
  while (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == HIGH) { }
  pMessage->sendMAStoSLVCreditStatusQuery();
//  delay(1000);

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

        default:
          sprintf(lcdString, "MSG TYPE ERROR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString);
          // It's printing a, b, c, etc. i.e. successive characters **************************************************************************
        }
      msgType = pMessage->available();
    }
  }
  pLCD2004->println("Setup complete.");

*/

  // Here is some code to test the Tsunami WAV player by playing a sound...
  // Call example: playTsunamiSound(1);
  // 001..016 are spoken numbers
  // 020 is coaster clicks
  // 021 is coaster clicks with 10-char filename
  // 030 is "cock the gun" being spoken
  // 031 is American Flyer announcement
  // Track number, output number is always 0, lock T/F
/*
  pTsunami->trackPlayPoly(21, 0, false);
  delay(2000);
  pTsunami->trackPlayPoly(30, 0, false);
  delay(2000);
  pTsunami->trackPlayPoly(1, 0, false);
  delay(2000);
  pTsunami->trackStop(21);
*/

/*
// Display the status of the flipper buttons on the LCD...
while (true) {
  int left = digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT);
  int right = digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT);
  sprintf(lcdString, "L:%s R:%s", (left == HIGH) ? "0  " : "1  ", (right == HIGH) ? "0  " : "1  ");
  pLCD2004->println(lcdString);
  Serial.println(lcdString);
  delay(100);
}
*/

/*
// Here is some code that reads Centipede #2 inputs manually, one at a time, and displays any closed switches.
// TESTING RESULTS SHOW IT TAKES 30ms TO READ ALL 64 SWITCHES MANUALLY
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
*/

/*
  // Here is some code that reads Centipede #2 inputs 16 bits at a time using portRead(), and displays any closed switches.
  // TESTING RESULTS SHOW IT ONLY TAKES 2-3ms TO READ ALL 64 SWITCHES USING portRead()!
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
    // Just some timing code to see how long it takes to read all 64 switches...
    //    long unsigned int currentMillis = millis();
    //    long unsigned int endMillis = millis();
    //    sprintf(lcdString, "Time: %lu ms", endMillis - currentMillis);
    //    pLCD2004->println(lcdString);
    //    Serial.println(lcdString);
    //    delay(1000);
  }
*/

// 11/14/25 DECISIONS REGARDING FLIPPER AND OTHER SWITCH HANDLING:
// At top of 10ms loop, read the status of the two flipper buttons continuously until 10ms has transpired i.e. ready for another loop iteration.
//   Handle flipper button presses immediately, outside of switch state table processing.
// Next read all 64 Centipede #2 inputs (16 bits at a time) into switchNewState[] before checking any switches.
//   Handle slingshot and bumper switches immediately, outside of switch state table processing.
//   Then process the rest of the switches via the switch state table as before.
// This way the flippers, slingshots, and bumpers are very responsive, while the other switches can be debounced via the state table.
// 10ms loop structure: Process game logic, then poll flippers
// in remaining time. Allows integer-based timing (timeOn units = 10ms)

/*
  void loop() {
    unsigned long loopStart = millis();

    // Fast flipper polling (uses remaining time after other tasks)
    while (millis() - loopStart < 10) {
      if (digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT) == LOW) {
        // Fire left flipper instantly
      }
      if (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == LOW) {
        // Fire right flipper instantly
      }
    }

    // Once per 10ms: read shift register switches
    for (int i = 0; i < 4; i++) {
      switchNewState[i] = pShiftRegister->portRead(i + 4);
    }

    // Process bumpers, lamps, score updates, etc.
    // ...

    // Update coil timers (decrement loopsRemaining, etc.)
    // ...
  }
*/

// Here is some code that fires the slingshot coils when their switches are pressed...

  byte shakerMotorPower = 0;

  while (true) {
    // Fast read all 64 Centipede #2 inputs (16 bits at a time) into switchNewState[] before checking any switches.
    // This is much faster (2-3ms) than calling digitalRead() for each switch individually (~30ms total).
    for (int i = 0; i < 4; i++) {
      switchNewState[i] = pShiftRegister->portRead(4 + i);  // read ports 4..7 which map to Centipede #2 inputs
    }

    // Check flipper buttons
    if (digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT) == LOW) {  // Fire the left flipper
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerInitial);
      delay(deviceParm[DEV_IDX_FLIPPER_LEFT].timeOn * 10);
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerHold);
      delay(20);
      while (digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT) == LOW) {  // Keep holding the flipper while button is pressed
        delay(10);
      }
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, LOW);  // Turn OFF
    }
    if (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == LOW) {  // Fire the right flipper
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, deviceParm[DEV_IDX_FLIPPER_RIGHT].powerInitial);
      delay(deviceParm[DEV_IDX_FLIPPER_RIGHT].timeOn * 10);
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, deviceParm[DEV_IDX_FLIPPER_RIGHT].powerHold);
      delay(20);
      while (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == LOW) {  // Keep holding the flipper while button is pressed
        delay(10);
      }
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, LOW);  // Turn OFF
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
      // Fire left kickout solenoid at initial power, hold/turn-off per deviceParm
      delay(200);
      analogWrite(deviceParm[devIdx].pinNum, deviceParm[devIdx].powerInitial);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_LEFT].pinNum, LOW);
      delay((int)deviceParm[devIdx].timeOn * 10);                 // timeOn is in 10ms ticks
      analogWrite(deviceParm[devIdx].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_LEFT].pinNum, HIGH);
    }

    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT)) {
      const byte devIdx = DEV_IDX_KICKOUT_RIGHT;
      // Fire right kickout solenoid at initial power, hold/turn-off per deviceParm
      delay(200);
      analogWrite(deviceParm[devIdx].pinNum, deviceParm[devIdx].powerInitial);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_RIGHT].pinNum, LOW);
      delay((int)deviceParm[devIdx].timeOn * 10);                 // timeOn is in 10ms ticks
      analogWrite(deviceParm[devIdx].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_RIGHT].pinNum, HIGH);
    }

    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_RIGHT].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_RIGHT].pinNum, HIGH);
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
      delay(500);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_3].pinNum, HIGH);
    }
    if (switchClosed(SWITCH_IDX_DIAG_4)) {
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_4].pinNum, LOW);
      delay(200);
      pShiftRegister->digitalWrite(lampParm[LAMP_IDX_WHITE_4].pinNum, HIGH);
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
    if (switchClosed(SWITCH_IDX_BALL_PRESENT)) {
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


  // Here is some code that waits for the Diagnostic "Select" button to be pressed, then fires the ball trough release coil...
  while (true) {

    sprintf(lcdString, "Press Select"); pLCD2004->println(lcdString);

    // Wait until Select closed (active LOW)
    while (pShiftRegister->digitalRead(switchParm[SWITCH_IDX_DIAG_4].pinNum) != LOW) {
      delay(10);
    }

    // Debounce confirmation
    delay(50);
    if (pShiftRegister->digitalRead(switchParm[SWITCH_IDX_DIAG_4].pinNum) == LOW) {
      sprintf(lcdString, "Low!"); pLCD2004->println(lcdString);
      const byte idx = DEV_IDX_BALL_TROUGH_RELEASE;
      const uint8_t pin = deviceParm[idx].pinNum;
      const uint8_t powerInit = deviceParm[idx].powerInitial;
      const uint8_t loops = deviceParm[idx].timeOn; // number of 10ms loops

      analogWrite(pin, powerInit);           // energize at initial strength
      delay((int)loops * 10);                // hold for timeOn * 10ms
      analogWrite(pin, LOW);               // turn off if no hold strength
      delay(1000);
    }
  }

  // Here is some code that tests Centipede #1 LAMP output pins by cycling them on and off...
  // 11/12/25: Works fine.
  while (true) {
    // Start with pinNum = 16 since the first 16 pins are unused on Centipede #1 in Screamo Master.
    for (int pinNum = 16; pinNum < 64; pinNum++) {  // Cycle through Centipede #1 output pins 16..63
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON
      sprintf(lcdString, "Pin %02d LOW ", pinNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);

      // Wait for a flipper button to be pressed...
      int left = digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT);
      int right = digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT);
      while ((left == HIGH) && (right == HIGH)) { // Both buttons unpressed
        left = digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT);
        right = digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT);
      }

      pShiftRegister->digitalWrite(pinNum, HIGH);   // Turn OFF
      sprintf(lcdString, "Pin %02d HIGH", pinNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
      delay(200);
    }
  }


  // Here is some code that tests the Mega's PWM outputs by cycling them on and off...



  // Wait for a flipper button to be pressed...
  int left = digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT);
  int right = digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT);
  while ((left == HIGH) && (right == HIGH)) { // Both buttons unpressed
    left = digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT);
    right = digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT);
  }

  for (int i = 0; i < NUM_DEVS; i++) {
    analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);  // Turn ON at initial power level
    sprintf(lcdString, "Dev %02d ON ", i);
    pLCD2004->println(lcdString);
    Serial.println(lcdString);
    delay(deviceParm[i].timeOn);
    analogWrite(deviceParm[i].pinNum, LOW);  // Turn OFF
    sprintf(lcdString, "Dev %02d OFF", i);
    pLCD2004->println(lcdString);
    Serial.println(lcdString);
    delay(500);
  }

  while (true) {
    if (digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT) == LOW) {  // Fire the left flipper
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerInitial);
      delay((int)deviceParm[DEV_IDX_FLIPPER_LEFT].timeOn * 10);
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerHold);
      delay(20);
      while (digitalRead(PIN_IN_BUTTON_FLIPPER_LEFT) == LOW) {  // Keep holding the flipper while button is pressed
        delay(10);
      }
      analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, LOW);  // Turn OFF
    }
    if (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == LOW) {  // Fire the right flipper
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, deviceParm[DEV_IDX_FLIPPER_RIGHT].powerInitial);
      delay((int)deviceParm[DEV_IDX_FLIPPER_RIGHT].timeOn * 10);
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, deviceParm[DEV_IDX_FLIPPER_RIGHT].powerHold);
      delay(20);
      while (digitalRead(PIN_IN_BUTTON_FLIPPER_RIGHT) == LOW) {  // Keep holding the flipper while button is pressed
        delay(10);
      }
      analogWrite(deviceParm[DEV_IDX_FLIPPER_RIGHT].pinNum, LOW);  // Turn OFF
    }
  }


  while (true) { }

  // *** STARTUP HOUSEKEEPING: ***

  // Turn on GI lamps so player thinks they're getting instant startup (NOTE: Can't do this until after pShiftRegister is initialized)
  // setGILamp(true);            // Turn on playfield G.I. lamps
  // pMessage->setGILamp(true);  // Tell Slave to turn on its G.I. lamps as well

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  sprintf(lcdString, "%lu", millis());
  pLCD2004->println(lcdString);
  delay(1000);

  /*
  // Check Centipede input pin and display state on LCD...
  // Tested and working
  while (true) {
    int x = pShiftRegister->digitalRead(74);
    if (x == HIGH) {
      sprintf(lcdString, "HIGH");
    } else if (x == LOW) {
      sprintf(lcdString, "LOW");
    } else {
      sprintf(lcdString, "ERROR");
    }
    pLCD2004->println(lcdString);
  }

  // Set Centipede output pin to cycle on and off each second...
  // Tested and working
  while (true) {
    pShiftRegister->digitalWrite(20, LOW);
    delay(1000);
    pShiftRegister->digitalWrite(20, HIGH);
    delay(1000);
  }
  */

  /*
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

}  // End of loop()

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

bool switchClosed(byte t_switchIdx) {
  // Accept only a switch index (0..NUM_SWITCHES-1).  The Centipede #2 pin for that switch
  // is looked up in switchParm[].pinNum (64..127).  Caller must ensure switchNewState[] has
  // been recently updated by calling portRead() for Centipede #2 inputs.
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


// ********************************************************************************************************************************
// **************************************************** OPERATE IN MANUAL MODE ****************************************************
// ********************************************************************************************************************************

void MASManualMode() {  // CLEAN THIS UP SO THAT MAS/OCC/LEG ARE MORE CONSISTENT WHEN DOING THE SAME THING I.E. RETRIEVING SENSORS AND UPDATING SENSOR-BLOCK *******************************
  /*
  // When starting MANUAL mode, MAS/SNS will always immediately send status of every sensor.
  // Recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  // We don't care about sensors in MAS Manual mode, so we disregard them (but other modules will see and want.)
  for (int i = 1; i <= TOTAL_SENSORS; i++) {  // For every sensor on the layout
    // First, MAS must transmit the request...
    pMessage->sendMAStoALLModeState(1, 1);
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    byte sensorNum;
    byte trippedOrCleared;
    pMessage->getMAStoALLModeState(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {
      sprintf(lcdString, "SNS %i %i NUM ERR", i, sensorNum); pLCD2004->println(lcdString);  Serial.println(lcdString); endWithFlashingLED(1);
    }
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", i, trippedOrCleared); pLCD2004->println(lcdString);  Serial.println(lcdString); endWithFlashingLED(1);
    }
    if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
      sprintf(lcdString, "Sensor %i Tripped", i); pLCD2004->println(lcdString); Serial.println(lcdString);
    } else {
      sprintf(lcdString, "Sensor %i Cleared", i); Serial.println(lcdString);  // Not to LCD
    }
    delay(20);  // Not too fast or we'll overflow LEG's input buffer
  }
  // Okay, we've received all TOTAL_SENSORS sensor status updates (and disregarded them.)
  */
  return;
}