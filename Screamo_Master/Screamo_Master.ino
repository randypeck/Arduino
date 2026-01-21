// Screamo_Master.INO Rev: 01/20/26c
// 11/12/25: Moved Right Kickout from pin 13 to pin 44 to avoid MOSFET firing twice on power-up.  Don't use MOSFET on pin 13.
// 12/28/25: Changed flipper inputs from direct Arduino inputs to Centipede inputs.
// 01/07/26: Added "5 Balls in Trough" switch to Centipede inputs. All switches tested and working.

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
#include <Tsunami.h>
#include <avr/wdt.h>
#include <Pinball_Descriptions.h>
#include <Pinball_Diagnostics.h>

const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 01/20/26";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which makes it a global.
// And no need to pass lcdString[] to any functions that use it!

bool initialBootDisplayShown = false;
unsigned long bootDisplayStartMs = 0;
const unsigned long BOOT_DISPLAY_DURATION_MS = 3000;  // Show rev date

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

const bool LAMP_ON                    = true;  // Or maybe these should be LOW / HIGH?
const bool LAMP_OFF                   = false;

// Populate lampParm with pin numbers, groups, and initial state OFF
// Pin numbers range from 0 to 63 because these lamps are on Centipede #1 (outputs)
LampParmStruct lampParm[NUM_LAMPS_MASTER] = {
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

// Store Centipede #2 pin numbers (64..127) for switches (original pins + 64)
SwitchParmStruct switchParm[NUM_SWITCHES_MASTER] = {
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

DeviceParmStruct deviceParm[NUM_DEVS_MASTER] = {
  // pinNum, powerInitial, timeOn(ms), powerHold, countdown, queueCount
  {  5, 255,  5,   0, 0, 0 },  // POP_BUMPER          =  0, PWM MOSFET
  {  4, 190, 10,   0, 0, 0 },  // KICKOUT_LEFT        =  1, PWM MOSFET (cannot modify PWM freq.)
  { 44, 200, 10,   0, 0, 0 },  // KICKOUT_RIGHT       =  2, PWM MOSFET
  {  6, 180, 10,   0, 0, 0 },  // SLINGSHOT_LEFT      =  3, PWM MOSFET
  {  7, 180, 10,   0, 0, 0 },  // SLINGSHOT_RIGHT     =  4, PWM MOSFET
  {  8, 150, 10,  40, 0, 0 },  // FLIPPER_LEFT        =  5, PWM MOSFET.  200 hits gobble hole too hard; 150 can get to top of p/f.
  {  9, 150, 10,  40, 0, 0 },  // FLIPPER_RIGHT       =  6, PWM MOSFET
  { 10, 200, 20,  40, 0, 0 },  // BALL_TRAY_RELEASE   =  7, PWM MOSFET (original tray release)
  { 23, 255,  5,   0, 0, 0 },  // SELECTION_UNIT      =  8, Non-PWM MOSFET; on/off only.
  { 24, 255,  5,   0, 0, 0 },  // RELAY_RESET         =  9, Non-PWM MOSFET; on/off only.
  { 11, 150, 23,   0, 0, 0 },  // BALL_TROUGH_RELEASE = 10, PWM MOSFET (new up/down post)
  // For the ball trough release coil, 150 is the right power; 100 could not guarantee it could retract if there was pressure holding it from balls above.
  // 230ms is just enough time for ball to get 1/2 way past post and momentum carries it so it won't get pinned by the post.
  // The post springs back fully extended before the next ball hits it, regardless of how many balls are stacked above it.
  { 12,   0,  0,   0, 0, 0 },  // MOTOR_SHAKER        = 11, PWM MOSFET. Range is 70 to 140. Below won't start; above triggers hats.
  { 26, 255,  5,   0, 0, 0 },  // KNOCKER             = 12, Non-PWM MOSFET; on/off only.
  { 25, 255,  0,   0, 0, 0 }   // MOTOR_SCORE         = 13, A/C SSR; on/off only; NOT PWM.
};

// ******************************************
// ***** AUDIO TRACK STRUCTS AND CONSTS *****
// ******************************************

// *** STRUCTURE DEFINITIONS ***

// COM (Voice/Speech) tracks - need length for ducking timing, priority for preemption
struct AudioComTrackDef {
  unsigned int trackNum;
  byte         lengthTenths;  // 0.1s units (16 = 1.6s, max 255 = 25.5s)
  byte         priority;      // AUDIO_PRIORITY_LOW/MED/HIGH
};

// SFX tracks - no length needed (except special cases handled separately)
struct AudioSfxTrackDef {
  unsigned int trackNum;
  byte         flags;  // AUDIO_FLAG_LOOP, etc.
};

// MUS (Music) tracks - need length for sequencing to next song
struct AudioMusTrackDef {
  unsigned int trackNum;
  byte         lengthSeconds;  // seconds (max 255 = 4m15s)
};

// *** COM TRACK ARRAYS ***
// Organized by category for easy maintenance

// DIAG COM tracks (101-102, 111-117)
const AudioComTrackDef comTracksDiag[] = {
  { 101, 15, AUDIO_PRIORITY_MED },   // Entering diagnostics
  { 102, 17, AUDIO_PRIORITY_MED },   // Exiting diagnostics
  { 111,  6, AUDIO_PRIORITY_MED },   // Lamps
  { 112,  7, AUDIO_PRIORITY_MED },   // Switches
  { 113,  7, AUDIO_PRIORITY_MED },   // Coils
  { 114,  7, AUDIO_PRIORITY_MED },   // Motors
  { 115,  7, AUDIO_PRIORITY_MED },   // Music
  { 116, 12, AUDIO_PRIORITY_MED },   // Sound Effects
  { 117,  7, AUDIO_PRIORITY_MED }    // Comments
};
const byte NUM_COM_DIAG = sizeof(comTracksDiag) / sizeof(comTracksDiag[0]);

// TILT WARNING COM tracks (201-205)
const AudioComTrackDef comTracksTiltWarning[] = {
  { 201, 26, AUDIO_PRIORITY_HIGH },  // Shake it again...
  { 202, 15, AUDIO_PRIORITY_HIGH },  // Easy there, Hercules
  { 203, 21, AUDIO_PRIORITY_HIGH },  // Whoa there, King Kong
  { 204,  6, AUDIO_PRIORITY_HIGH },  // Careful
  { 205,  6, AUDIO_PRIORITY_HIGH }   // Watch it
};
const byte NUM_COM_TILT_WARNING = sizeof(comTracksTiltWarning) / sizeof(comTracksTiltWarning[0]);

// TILT COM tracks (212-216)
const AudioComTrackDef comTracksTilt[] = {
  { 212, 28, AUDIO_PRIORITY_HIGH },  // Nice goin Hercules...
  { 213, 32, AUDIO_PRIORITY_HIGH },  // Congratulations King Kong...
  { 214, 19, AUDIO_PRIORITY_HIGH },  // Ya broke it ya big palooka
  { 215, 27, AUDIO_PRIORITY_HIGH },  // This aint the bumper cars...
  { 216, 29, AUDIO_PRIORITY_HIGH }   // Tilt! Thats what ya get...
};
const byte NUM_COM_TILT = sizeof(comTracksTilt) / sizeof(comTracksTilt[0]);

// BALL MISSING COM tracks (301-304)
const AudioComTrackDef comTracksBallMissing[] = {
  { 301, 55, AUDIO_PRIORITY_MED },   // Theres a ball missing...
  { 302, 53, AUDIO_PRIORITY_MED },   // Press the ball lift rod...
  { 303, 46, AUDIO_PRIORITY_MED },   // Shoot any balls...
  { 304, 41, AUDIO_PRIORITY_MED }    // Theres still a ball missing...
};
const byte NUM_COM_BALL_MISSING = sizeof(comTracksBallMissing) / sizeof(comTracksBallMissing[0]);

// START REJECT COM tracks (312-330)
const AudioComTrackDef comTracksStartReject[] = {
  { 312, 36, AUDIO_PRIORITY_MED },   // No free admission today...
  { 313, 19, AUDIO_PRIORITY_MED },   // Go ask yer mommy...
  { 314, 19, AUDIO_PRIORITY_MED },   // I told yas ta SCRAM
  { 315, 18, AUDIO_PRIORITY_MED },   // Ya aint gettin in...
  { 316, 19, AUDIO_PRIORITY_MED },   // Any o yous kids got a cigarette
  { 317, 19, AUDIO_PRIORITY_MED },   // Go shake down da couch...
  { 318, 28, AUDIO_PRIORITY_MED },   // When Is a kid...
  { 319, 26, AUDIO_PRIORITY_MED },   // This aint a charity...
  { 320, 25, AUDIO_PRIORITY_MED },   // Whadda I look like...
  { 321, 27, AUDIO_PRIORITY_MED },   // No ticket, no tumblin...
  { 322, 30, AUDIO_PRIORITY_MED },   // Quit pressin buttons...
  { 323, 33, AUDIO_PRIORITY_MED },   // Im losin money...
  { 324, 21, AUDIO_PRIORITY_MED },   // Ya broke, go sell a balloon
  { 325, 24, AUDIO_PRIORITY_MED },   // Step right up, after ya pay
  { 326, 30, AUDIO_PRIORITY_MED },   // Dis aint da soup kitchen...
  { 327, 24, AUDIO_PRIORITY_MED },   // Beat it kid...
  { 328, 21, AUDIO_PRIORITY_MED },   // Yas tryin ta sneak in...
  { 329, 26, AUDIO_PRIORITY_MED },   // Come back when yas got...
  { 330, 21, AUDIO_PRIORITY_MED }    // No coin, no joyride
};
const byte NUM_COM_START_REJECT = sizeof(comTracksStartReject) / sizeof(comTracksStartReject[0]);

// START COM tracks (351-357, 402)
const AudioComTrackDef comTracksStart[] = {
  { 351, 17, AUDIO_PRIORITY_MED },   // Okay kid, youre in
  { 352, 46, AUDIO_PRIORITY_MED },   // Press start again for a wilder ride...
  { 353, 36, AUDIO_PRIORITY_MED },   // Keep pressin Start...
  { 354, 19, AUDIO_PRIORITY_MED },   // Second guest, cmon in
  { 355, 18, AUDIO_PRIORITY_MED },   // Third guest, youre in
  { 356, 22, AUDIO_PRIORITY_MED },   // Fourth guest, through the turnstile
  { 357, 25, AUDIO_PRIORITY_MED },   // The parks full...
  { 402, 35, AUDIO_PRIORITY_MED }    // Hey Gang, Lets Ride the Screamo
};
const byte NUM_COM_START = sizeof(comTracksStart) / sizeof(comTracksStart[0]);

// PLAYER COM tracks (451-454)
const AudioComTrackDef comTracksPlayer[] = {
  { 451, 11, AUDIO_PRIORITY_MED },   // Guest 1
  { 452, 13, AUDIO_PRIORITY_MED },   // Guest 2
  { 453, 13, AUDIO_PRIORITY_MED },   // Guest 3
  { 454, 13, AUDIO_PRIORITY_MED }    // Guest 4
};
const byte NUM_COM_PLAYER = sizeof(comTracksPlayer) / sizeof(comTracksPlayer[0]);

// BALL COM tracks (461-465)
const AudioComTrackDef comTracksBall[] = {
  { 461, 11, AUDIO_PRIORITY_MED },   // Ball 1
  { 462, 10, AUDIO_PRIORITY_MED },   // Ball 2
  { 463, 11, AUDIO_PRIORITY_MED },   // Ball 3
  { 464, 12, AUDIO_PRIORITY_MED },   // Ball 4
  { 465, 12, AUDIO_PRIORITY_MED }    // Ball 5
};
const byte NUM_COM_BALL = sizeof(comTracksBall) / sizeof(comTracksBall[0]);

// BALL 1 COMMENT COM tracks (511-519) - for P2-P4 first ball
const AudioComTrackDef comTracksBall1Comment[] = {
  { 511, 11, AUDIO_PRIORITY_LOW },   // Climb aboard
  { 512, 14, AUDIO_PRIORITY_LOW },   // Explore the park
  { 513, 14, AUDIO_PRIORITY_LOW },   // Fire away
  { 514,  9, AUDIO_PRIORITY_LOW },   // Launch it
  { 515, 17, AUDIO_PRIORITY_LOW },   // Lets find a ride
  { 516, 18, AUDIO_PRIORITY_LOW },   // Show em how its done
  { 517,  8, AUDIO_PRIORITY_LOW },   // Youre up
  { 518, 16, AUDIO_PRIORITY_LOW },   // Your turn to ride
  { 519, 13, AUDIO_PRIORITY_LOW }    // Lets ride
};
const byte NUM_COM_BALL1_COMMENT = sizeof(comTracksBall1Comment) / sizeof(comTracksBall1Comment[0]);

// BALL 5 COMMENT COM tracks (531-540)
const AudioComTrackDef comTracksBall5Comment[] = {
  { 531, 18, AUDIO_PRIORITY_LOW },   // Dont embarrass yourself
  { 532, 16, AUDIO_PRIORITY_LOW },   // Its now or never
  { 533, 17, AUDIO_PRIORITY_LOW },   // Last ride of the day
  { 534, 12, AUDIO_PRIORITY_LOW },   // Make it flashy
  { 535, 10, AUDIO_PRIORITY_LOW },   // No pressure
  { 536, 17, AUDIO_PRIORITY_LOW },   // This ball decides it
  { 537, 17, AUDIO_PRIORITY_LOW },   // This is it
  { 538, 15, AUDIO_PRIORITY_LOW },   // This is your last ticket
  { 539, 11, AUDIO_PRIORITY_LOW },   // Last ball
  { 540, 11, AUDIO_PRIORITY_LOW }    // Make it count
};
const byte NUM_COM_BALL5_COMMENT = sizeof(comTracksBall5Comment) / sizeof(comTracksBall5Comment[0]);

// GAME OVER COM tracks (551-577)
const AudioComTrackDef comTracksGameOver[] = {
  { 551, 17, AUDIO_PRIORITY_MED },   // End of the ride, thrillseeker
  { 552, 25, AUDIO_PRIORITY_MED },   // Hope you enjoyed the ride...
  { 553, 15, AUDIO_PRIORITY_MED },   // Its curtains for you, pal
  { 554, 19, AUDIO_PRIORITY_MED },   // Its game over for you, dude
  { 555, 30, AUDIO_PRIORITY_MED },   // No more rides for you...
  { 556, 17, AUDIO_PRIORITY_MED },   // No more tickets for this ride
  { 557, 20, AUDIO_PRIORITY_MED },   // Pack it up...
  { 558, 47, AUDIO_PRIORITY_MED },   // Randy warned me...
  { 559, 17, AUDIO_PRIORITY_MED },   // Rides over, move along
  { 560, 35, AUDIO_PRIORITY_MED },   // Screamo is now closed, but please...
  { 561, 31, AUDIO_PRIORITY_MED },   // Step right down...
  { 562, 18, AUDIO_PRIORITY_MED },   // Thats all she wrote...
  { 563, 33, AUDIO_PRIORITY_MED },   // The fat lady has sung...
  { 564, 22, AUDIO_PRIORITY_MED },   // Screamo is now closed, pal
  { 565, 15, AUDIO_PRIORITY_MED },   // Youre out of the running...
  { 566, 20, AUDIO_PRIORITY_MED },   // The shows over for you...
  { 567, 16, AUDIO_PRIORITY_MED },   // Youre out of tickets...
  { 568, 40, AUDIO_PRIORITY_MED },   // Thats the end of the line...
  { 569, 36, AUDIO_PRIORITY_MED },   // The Park is Now Closed...
  { 570, 34, AUDIO_PRIORITY_MED },   // Screamo is parked...
  { 571, 27, AUDIO_PRIORITY_MED },   // Youve hit the brakes...
  { 572, 39, AUDIO_PRIORITY_MED },   // Youve reached the end...
  { 573, 28, AUDIO_PRIORITY_MED },   // You gave it a whirl...
  { 574, 32, AUDIO_PRIORITY_MED },   // Your Ride is Over Park Now Closed
  { 575, 26, AUDIO_PRIORITY_MED },   // Your tickets punched...
  { 576, 42, AUDIO_PRIORITY_MED },   // Your Ride is Over Safety Bar
  { 577, 27, AUDIO_PRIORITY_MED }    // Your ride is over; better luck...
};
const byte NUM_COM_GAME_OVER = sizeof(comTracksGameOver) / sizeof(comTracksGameOver[0]);

// SHOOT COM tracks (611-620)
const AudioComTrackDef comTracksShoot[] = {
  { 611, 37, AUDIO_PRIORITY_LOW },   // Press the Ball Lift rod...
  { 612, 28, AUDIO_PRIORITY_LOW },   // This ride will be a lot more fun...
  { 613, 26, AUDIO_PRIORITY_LOW },   // I recommend you consider...
  { 614, 28, AUDIO_PRIORITY_LOW },   // Dont be afraid of the ride...
  { 615, 23, AUDIO_PRIORITY_LOW },   // For Gods sake shoot the ball
  { 616, 25, AUDIO_PRIORITY_LOW },   // No dilly dallying...
  { 617, 25, AUDIO_PRIORITY_LOW },   // Now would be a good time...
  { 618, 29, AUDIO_PRIORITY_LOW },   // Now would be an excellent time...
  { 619, 13, AUDIO_PRIORITY_LOW },   // Shoot the Ball
  { 620, 35, AUDIO_PRIORITY_LOW }    // This would be an excellent time... (annoyed)
};
const byte NUM_COM_SHOOT = sizeof(comTracksShoot) / sizeof(comTracksShoot[0]);

// BALL SAVED COM tracks (631-636, 641)
const AudioComTrackDef comTracksBallSaved[] = {
  { 631, 25, AUDIO_PRIORITY_MED },   // Ball saved, launch again
  { 632, 24, AUDIO_PRIORITY_MED },   // Heres another ball keep shooting
  { 633, 20, AUDIO_PRIORITY_MED },   // Keep going shoot again
  { 634, 21, AUDIO_PRIORITY_MED },   // Ball saved, ride again
  { 635, 18, AUDIO_PRIORITY_MED },   // Ball saved, shoot again
  { 636, 25, AUDIO_PRIORITY_MED },   // Ball saved; get back on the ride
  { 641, 25, AUDIO_PRIORITY_MED }    // Heres your ball back... (mode end)
};
const byte NUM_COM_BALL_SAVED = sizeof(comTracksBallSaved) / sizeof(comTracksBallSaved[0]);

// BALL SAVED URGENT COM tracks (651-662)
const AudioComTrackDef comTracksBallSavedUrgent[] = {
  { 651, 20, AUDIO_PRIORITY_MED },   // Heres another ball; send it
  { 652, 25, AUDIO_PRIORITY_MED },   // Heres another ball; fire at will
  { 653, 22, AUDIO_PRIORITY_MED },   // Heres a new ball; fire away
  { 654, 21, AUDIO_PRIORITY_MED },   // Heres a new ball; quick, shoot it
  { 655, 21, AUDIO_PRIORITY_MED },   // Heres another ball; shoot it now
  { 656, 18, AUDIO_PRIORITY_MED },   // Hurry, shoot another ball
  { 657, 15, AUDIO_PRIORITY_MED },   // Quick, shoot again
  { 658, 21, AUDIO_PRIORITY_MED },   // Keep going; shoot another ball
  { 659, 26, AUDIO_PRIORITY_MED },   // Its still your turn; keep shooting
  { 660, 10, AUDIO_PRIORITY_MED },   // Shoot again
  { 661, 24, AUDIO_PRIORITY_MED },   // For Gods sake shoot another ball
  { 662, 18, AUDIO_PRIORITY_MED }    // Quick shoot another ball
};
const byte NUM_COM_BALL_SAVED_URGENT = sizeof(comTracksBallSavedUrgent) / sizeof(comTracksBallSavedUrgent[0]);

// MULTIBALL COM tracks (671-675)
const AudioComTrackDef comTracksMultiball[] = {
  { 671, 34, AUDIO_PRIORITY_HIGH },  // First ball locked...
  { 672, 36, AUDIO_PRIORITY_HIGH },  // Second ball locked...
  { 673, 14, AUDIO_PRIORITY_HIGH },  // Multiball
  { 674, 33, AUDIO_PRIORITY_HIGH },  // Multiball; all rides are running
  { 675, 34, AUDIO_PRIORITY_HIGH }   // Every bumper is worth double...
};
const byte NUM_COM_MULTIBALL = sizeof(comTracksMultiball) / sizeof(comTracksMultiball[0]);

// COMPLIMENT COM tracks (701-714)
const AudioComTrackDef comTracksCompliment[] = {
  { 701,  9, AUDIO_PRIORITY_LOW },   // Good shot
  { 702, 20, AUDIO_PRIORITY_LOW },   // Awesome great work
  { 703, 10, AUDIO_PRIORITY_LOW },   // Great job
  { 704, 13, AUDIO_PRIORITY_LOW },   // Great shot
  { 705, 10, AUDIO_PRIORITY_LOW },   // Youve done it
  { 706, 15, AUDIO_PRIORITY_LOW },   // Mission accomplished
  { 707,  9, AUDIO_PRIORITY_LOW },   // Youve done it
  { 708, 13, AUDIO_PRIORITY_LOW },   // You did it
  { 709, 11, AUDIO_PRIORITY_LOW },   // Amazing
  { 710, 12, AUDIO_PRIORITY_LOW },   // Great job
  { 711, 27, AUDIO_PRIORITY_LOW },   // Great shot nicely done
  { 712, 11, AUDIO_PRIORITY_LOW },   // Well done
  { 713, 11, AUDIO_PRIORITY_LOW },   // Great work
  { 714, 20, AUDIO_PRIORITY_LOW }    // You did it great shot
};
const byte NUM_COM_COMPLIMENT = sizeof(comTracksCompliment) / sizeof(comTracksCompliment[0]);

// DRAIN COM tracks (721-748)
const AudioComTrackDef comTracksDrain[] = {
  { 721, 23, AUDIO_PRIORITY_LOW },   // Did you forget where the flipper buttons are
  { 722, 19, AUDIO_PRIORITY_LOW },   // Gravity called and you answered
  { 723, 19, AUDIO_PRIORITY_LOW },   // Have you been to an eye doctor lately
  { 724, 20, AUDIO_PRIORITY_LOW },   // Ill Pretend I Didnt See That
  { 725, 11, AUDIO_PRIORITY_LOW },   // Gravity wins
  { 726, 19, AUDIO_PRIORITY_LOW },   // Im So Sorry For Your Loss
  { 727,  9, AUDIO_PRIORITY_LOW },   // That was quick
  { 728, 36, AUDIO_PRIORITY_LOW },   // Maybe try playing with your eyes open...
  { 729, 29, AUDIO_PRIORITY_LOW },   // Nice drain, was that part of your strategy
  { 730, 20, AUDIO_PRIORITY_LOW },   // Oh I Didnt See That Coming
  { 731, 14, AUDIO_PRIORITY_LOW },   // Oh Thats Humiliating
  { 732, 11, AUDIO_PRIORITY_LOW },   // So long, ball
  { 733, 12, AUDIO_PRIORITY_LOW },   // Thats gotta hurt
  { 734, 23, AUDIO_PRIORITY_LOW },   // Thats The Saddest Thing Ive Ever Seen
  { 735,  7, AUDIO_PRIORITY_LOW },   // Yikes
  { 736, 17, AUDIO_PRIORITY_LOW },   // Oh that hurt to watch
  { 737, 17, AUDIO_PRIORITY_LOW },   // That Was Just Terrible
  { 738, 39, AUDIO_PRIORITY_LOW },   // Your ball saw the drain...
  { 739, 11, AUDIO_PRIORITY_LOW },   // Whoopsie daisy
  { 740, 11, AUDIO_PRIORITY_LOW },   // Get outta there, kid
  { 741, 13, AUDIO_PRIORITY_LOW },   // Hey, wrong way
  { 742, 14, AUDIO_PRIORITY_LOW },   // Kid not that way
  { 743, 12, AUDIO_PRIORITY_LOW },   // Leaving so soon
  { 744, 12, AUDIO_PRIORITY_LOW },   // No no no
  { 745, 15, AUDIO_PRIORITY_LOW },   // Not that way
  { 746, 11, AUDIO_PRIORITY_LOW },   // Not the exit
  { 747,  8, AUDIO_PRIORITY_LOW },   // Oh boy
  { 748, 11, AUDIO_PRIORITY_LOW }    // Where ya goin, kid
};
const byte NUM_COM_DRAIN = sizeof(comTracksDrain) / sizeof(comTracksDrain[0]);

// AWARD COM tracks (811-842)
const AudioComTrackDef comTracksAward[] = {
  { 811, 15, AUDIO_PRIORITY_HIGH },  // Special is lit
  { 812, 19, AUDIO_PRIORITY_HIGH },  // Reeee-plaaay
  { 821, 32, AUDIO_PRIORITY_HIGH },  // You lit three in a row, replay
  { 822, 44, AUDIO_PRIORITY_HIGH },  // You lit all four corners, five replays
  { 823, 46, AUDIO_PRIORITY_HIGH },  // You lit one, two and three, twenty replays
  { 824, 40, AUDIO_PRIORITY_HIGH },  // Five balls in the Gobble Hole - replay
  { 831, 18, AUDIO_PRIORITY_HIGH },  // You spelled SCREAMO
  { 841, 19, AUDIO_PRIORITY_HIGH },  // EXTRA BALL
  { 842, 20, AUDIO_PRIORITY_HIGH }   // Heres another ball; send it
};
const byte NUM_COM_AWARD = sizeof(comTracksAward) / sizeof(comTracksAward[0]);

// MODE COM tracks (1002-1003, 1005, 1101, 1111, 1201, 1211, 1301, 1311)
const AudioComTrackDef comTracksMode[] = {
  { 1002,  8, AUDIO_PRIORITY_HIGH },  // Jackpot
  { 1003, 13, AUDIO_PRIORITY_HIGH },  // Ten seconds left
  { 1005,  8, AUDIO_PRIORITY_HIGH },  // Time
  { 1101, 93, AUDIO_PRIORITY_HIGH },  // Lets ride the Bumper Cars
  { 1111, 18, AUDIO_PRIORITY_MED },   // Keep smashing the bumpers
  { 1201, 82, AUDIO_PRIORITY_HIGH },  // Lets play Roll-A-Ball
  { 1211, 18, AUDIO_PRIORITY_MED },   // Keep rolling over the hats
  { 1301, 71, AUDIO_PRIORITY_HIGH },  // Lets visit the Shooting Gallery
  { 1311, 19, AUDIO_PRIORITY_MED }    // Keep shooting at the Gobble Hole
};
const byte NUM_COM_MODE = sizeof(comTracksMode) / sizeof(comTracksMode[0]);

// *** SFX TRACK ARRAY ***
const AudioSfxTrackDef sfxTracks[] = {
  // DIAG SFX
  { 103, AUDIO_FLAG_NONE },   // Diagnostics continuous tone
  { 104, AUDIO_FLAG_NONE },   // Switch Close 1000hz
  { 105, AUDIO_FLAG_NONE },   // Switch Open 500hz
  // TILT SFX
  { 211, AUDIO_FLAG_NONE },   // Buzzer
  // START SFX
  { 311, AUDIO_FLAG_NONE },   // Car honk aoooga
  { 401, AUDIO_FLAG_NONE },   // Scream player 1
  { 403, AUDIO_FLAG_LOOP },   // Lift, loopable (120s)
  { 404, AUDIO_FLAG_NONE },   // First drop multi-screams
  // MODE COMMON SFX
  { 1001, AUDIO_FLAG_NONE },  // School Bell stinger start
  { 1004, AUDIO_FLAG_NONE },  // Timer countdown
  { 1006, AUDIO_FLAG_NONE },  // Factory whistle stinger end
  // MODE BUMPER CAR HIT SFX (1121-1133)
  { 1121, AUDIO_FLAG_NONE },  // Car honk
  { 1122, AUDIO_FLAG_NONE },  // Car honk la cucaracha
  { 1123, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1124, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1125, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1126, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1127, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1128, AUDIO_FLAG_NONE },  // Car honk 2x rubber
  { 1129, AUDIO_FLAG_NONE },  // Car honk 2x rubber
  { 1130, AUDIO_FLAG_NONE },  // Car honk aoooga
  { 1131, AUDIO_FLAG_NONE },  // Car honk diesel train
  { 1132, AUDIO_FLAG_NONE },  // Car honk
  { 1133, AUDIO_FLAG_NONE },  // Car honk truck air horn
  // MODE BUMPER CAR MISS SFX (1141-1148)
  { 1141, AUDIO_FLAG_NONE },  // Car crash
  { 1142, AUDIO_FLAG_NONE },  // Car crash
  { 1143, AUDIO_FLAG_NONE },  // Car crash
  { 1144, AUDIO_FLAG_NONE },  // Car crash
  { 1145, AUDIO_FLAG_NONE },  // Car crash x
  { 1146, AUDIO_FLAG_NONE },  // Cat screech
  { 1147, AUDIO_FLAG_NONE },  // Car crash x
  { 1148, AUDIO_FLAG_NONE },  // Car crash x
  // MODE BUMPER CAR ACHIEVED SFX
  { 1197, AUDIO_FLAG_NONE },  // Bell ding ding ding
  // MODE ROLL-A-BALL HIT SFX (1221-1225)
  { 1221, AUDIO_FLAG_NONE },  // Bowling strike
  { 1222, AUDIO_FLAG_NONE },  // Bowling strike
  { 1223, AUDIO_FLAG_NONE },  // Bowling strike
  { 1224, AUDIO_FLAG_NONE },  // Bowling strike
  { 1225, AUDIO_FLAG_NONE },  // Bowling strike
  // MODE ROLL-A-BALL MISS SFX (1241-1254)
  { 1241, AUDIO_FLAG_NONE },  // Glass
  { 1242, AUDIO_FLAG_NONE },  // Glass
  { 1243, AUDIO_FLAG_NONE },  // Glass
  { 1244, AUDIO_FLAG_NONE },  // Glass
  { 1245, AUDIO_FLAG_NONE },  // Glass
  { 1246, AUDIO_FLAG_NONE },  // Glass
  { 1247, AUDIO_FLAG_NONE },  // Glass
  { 1248, AUDIO_FLAG_NONE },  // Glass
  { 1249, AUDIO_FLAG_NONE },  // Glass
  { 1250, AUDIO_FLAG_NONE },  // Glass
  { 1251, AUDIO_FLAG_NONE },  // Glass
  { 1252, AUDIO_FLAG_NONE },  // Glass
  { 1253, AUDIO_FLAG_NONE },  // Glass
  { 1254, AUDIO_FLAG_NONE },  // Goat sound
  // MODE ROLL-A-BALL ACHIEVED SFX
  { 1297, AUDIO_FLAG_NONE },  // Ta da
  // MODE GOBBLE HOLE HIT SFX
  { 1321, AUDIO_FLAG_NONE },  // Slide whistle down
  // MODE GOBBLE HOLE MISS SFX (1341-1348)
  { 1341, AUDIO_FLAG_NONE },  // Ricochet
  { 1342, AUDIO_FLAG_NONE },  // Ricochet
  { 1343, AUDIO_FLAG_NONE },  // Ricochet
  { 1344, AUDIO_FLAG_NONE },  // Ricochet
  { 1345, AUDIO_FLAG_NONE },  // Ricochet
  { 1346, AUDIO_FLAG_NONE },  // Ricochet
  { 1347, AUDIO_FLAG_NONE },  // Ricochet
  { 1348, AUDIO_FLAG_NONE },  // Ricochet
  // MODE GOBBLE HOLE ACHIEVED SFX
  { 1397, AUDIO_FLAG_NONE }   // Applause mixed
};
const byte NUM_SFX_TRACKS = sizeof(sfxTracks) / sizeof(sfxTracks[0]);

// *** MUSIC TRACK ARRAYS ***
// CIRCUS music (2001-2019)
const AudioMusTrackDef musTracksCircus[] = {
  { 2001, 147 },  // Barnum and Baileys Favorite (2m27s)
  { 2002, 156 },  // Rensaz Race March (2m36s)
  { 2003,  77 },  // Twelfth Street Rag (1m17s)
  { 2004, 195 },  // Chariot Race, Ben Hur March (3m15s)
  { 2005,  58 },  // Organ Grinders Serenade (0m58s)
  { 2006, 155 },  // Hands Across the Sea (2m35s)
  { 2007, 132 },  // Field Artillery March (2m12s)
  { 2008, 102 },  // You Flew Over (1m42s)
  { 2009,  65 },  // Unter dem Doppeladler (1m5s)
  { 2010, 100 },  // Ragtime Cowboy Joe (1m40s)
  { 2011, 183 },  // Billboard March (3m3s)
  { 2012, 110 },  // El capitan (1m50s)
  { 2013, 119 },  // Smiles (1m59s)
  { 2014, 151 },  // Spirit of St. Louis March (2m31s)
  { 2015, 115 },  // The Free Lance (1m55s)
  { 2016, 139 },  // The Roxy March (2m19s)
  { 2017, 119 },  // The Stars and Stripes Forever (1m59s)
  { 2018, 141 },  // The Washington Post (2m21s)
  { 2019, 154 }   // Bombasto (2m34s)
};
const byte NUM_MUS_CIRCUS = sizeof(musTracksCircus) / sizeof(musTracksCircus[0]);

// SURF music (2051-2068)
const AudioMusTrackDef musTracksSurf[] = {
  { 2051, 131 },  // Miserlou (2m11s)
  { 2052, 185 },  // Bumble Bee Stomp (3m5s)
  { 2053, 177 },  // Wipe Out (2m57s)
  { 2054, 103 },  // Banzai Washout (1m43s)
  { 2055, 120 },  // Hava Nagila (2m0s)
  { 2056, 130 },  // Sabre Dance (2m10s)
  { 2057, 139 },  // Malaguena (2m19s)
  { 2058,  98 },  // Wildfire (1m38s)
  { 2059, 113 },  // The Wedge (1m53s)
  { 2060, 108 },  // Exotic (1m48s)
  { 2061, 182 },  // The Victor (3m2s)
  { 2062, 113 },  // Mr. Eliminator (1m53s)
  { 2063,  90 },  // Night Rider (1m30s)
  { 2064,  97 },  // The Jester (1m37s)
  { 2065, 129 },  // Pressure (2m9s)
  { 2066, 110 },  // Shootin Beavers (1m50s)
  { 2067, 127 },  // Riders in the Sky (2m7s)
  { 2068, 114 }   // Bumble Bee Boogie (1m54s)
};
const byte NUM_MUS_SURF = sizeof(musTracksSurf) / sizeof(musTracksSurf[0]);

// *** AUDIO PLAYBACK STATE ***
unsigned int audioCurrentComTrack   = 0;  // Currently playing COM track (0 = none)
unsigned int audioCurrentSfxTrack   = 0;  // Currently playing SFX track (0 = none)
unsigned int audioCurrentMusicTrack = 0;  // Currently playing MUS track (0 = none)
unsigned long audioComEndMillis     = 0;  // When current COM track ends
unsigned long audioMusicEndMillis   = 0;  // When current music track ends
unsigned long audioLiftLoopStartMillis = 0;  // When lift loop started (for re-loop)

// *** TSUNAMI GAIN SETTINGS ***
// Master gain can range from -40dB to 0dB (full level); defaults to -10dB to allow some headroom.
const int8_t TSUNAMI_GAIN_DB_DEFAULT = -10;
const int8_t TSUNAMI_GAIN_DB_MIN     = -40;
const int8_t TSUNAMI_GAIN_DB_MAX     =   0;

int8_t tsunamiGainDb      = TSUNAMI_GAIN_DB_DEFAULT;  // Persisted Tsunami master gain
int8_t tsunamiVoiceGainDb = 0;  // Per-category relative gain (COM/Voice)
int8_t tsunamiSfxGainDb   = 0;  // Per-category relative gain (SFX)
int8_t tsunamiMusicGainDb = 0;  // Per-category relative gain (Music)
int8_t tsunamiDuckingDb = -20;  // Ducking gain offset (default -20dB)

// ********************************
// ****** DIAGNOSTICS SETUP *******
// ********************************

void markDiagnosticsDisplayDirty(bool forceSuiteReset = false);  // Must forward declare since it has a default parm.

byte diagnosticSuiteIdx  = 0;  // 0..NUM_DIAG_SUITES-1: which top-level suite selected
byte diagnosticState     = 0;  // 0 = at suite menu, >0 = inside a suite (future use)

const byte NUM_DIAG_SUITES = 6;
const char* diagSuiteNames[NUM_DIAG_SUITES] = {
  "VOLUME",
  "LAMP TESTS",
  "SWITCH TESTS",
  "COIL/MOTOR TESTS",
  "AUDIO TESTS",
  "SETTINGS"
};

const byte NUM_DIAG_BUTTONS = 4;
byte diagButtonLastState[NUM_DIAG_BUTTONS] = { 0, 0, 0, 0 };
bool attractDisplayDirty = true;
bool diagDisplayDirty = true;
byte diagLastRenderedSuite = 0xFF;

// ******************************************
// ***** GAME STATE STRUCTURE AND VARS ******
// ******************************************

// Game state tracks multi-player state, ball tracking, and timing.
// Memory efficient: ~24 bytes total.
struct GameStateStruct {
  byte numPlayers;           // 1-4 (Enhanced) or 1 (Original/Impulse)
  byte currentPlayer;        // 1-4 (1-indexed for display)
  byte currentBall;          // 1-5 (1-indexed for display)
  uint16_t score[4];         // 0..999 per player (in 10K units, so 999 = 9,990,000)
  byte tiltWarnings;         // 0-2 warnings before tilt (per ball)
  bool tilted;               // True if current ball has tilted
  bool ballInPlay;           // True if ball is live on playfield
  bool ballSaveActive;       // True if ball save timer is running
  unsigned long ballSaveEndMs;      // When ball save expires (millis)
  unsigned long ballLaunchMs;       // When ball was launched (for shoot prompts)
  bool hasScored;            // True if player scored this ball (for ball save activation)
};

GameStateStruct gameState;

// Coil rest period: after timeOn expires and powerHold==0, coil enters rest for this many ticks.
// Negative countdown values indicate rest period; 0 = idle; positive = active.
// 8 ticks * 10ms = 80ms rest, ensuring coils can't rapid-fire dangerously.
const int8_t COIL_REST_TICKS = -8;

// *** FLIPPER STATE ***
// Flippers need immediate response, tracked separately from other coils.
bool leftFlipperHeld = false;
bool rightFlipperHeld = false;

// Knockoff button state
unsigned long knockoffPressStartMs = 0;
bool knockoffBeingHeld = false;
byte knockoffLastState = 0;
const unsigned long KNOCKOFF_RESET_HOLD_MS = 1000;     // Hold knockoff for 1s to trigger reset

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

  // Set boot display timing
  bootDisplayStartMs = millis();
  initialBootDisplayShown = false;  // Will transition after delay

  // *** INITIALIZE TSUNAMI WAV PLAYER OBJECT ***
  // Files must be WAV format, 16-bit PCM, 44.1kHz, Stereo.  Details in Tsunami.h comments.
  pTsunami = new Tsunami();  // Create Tsunami object on Serial3
  pTsunami->start();         // Start Tsunami WAV player
  delay(10);                 // Give Tsunami time to initialize  // *************************** Change to 1000 if 10ms is insufficient **********************
  pTsunami->stopAllTracks(); // Clear any leftover playback
  delay(50);
  // Ensure Tsunami is in a clean state on any reset:
  //  - stop any tracks that might have been left playing
  //  - reload and apply master / category gains
  //  - clear our "currently playing" state
  audioResetTsunamiState();

  setGILamps(true);          // Turn on playfield G.I. lamps
  pMessage->sendMAStoSLVGILamp(true);  // Tell Slave to turn on its G.I. lamps as well
  setAttractLamps();

  initGameState();  // Initialize game state to attract mode defaults

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

  // Okay we're going to start our main loop. Record start time for performance monitoring.
  unsigned long loopStartMicros = micros();

  loopNextMillis = now + LOOP_TICK_MS;

  // Update Centipede #2 switch snapshot once per tick
  for (int i = 0; i < 4; i++) {
    switchOldState[i] = switchNewState[i];
    switchNewState[i] = pShiftRegister->portRead(4 + i);  // ports 4..7 => inputs
  }

  // Process device timers FIRST - ensures coil state is current before any activation attempts
  updateDeviceTimers();

  // Process flippers immediately for responsive feel (every tick, regardless of mode)
  processFlippers();

  // Dispatch by mode
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

  // Let's see if our loop took more than 9ms (90% of our 10ms budget):
  if (modeCurrent != MODE_DIAGNOSTIC) {
    unsigned long loopDuration = micros() - loopStartMicros;
    if (loopDuration > 9000) {  // Warn if >9ms
      Serial.print("SLOW LOOP: ");
      Serial.println(loopDuration);
      // Show warning on row 4 without clearing other rows
      snprintf(lcdString, LCD_WIDTH + 1, "SLOW: %lu us", loopDuration);
      lcdPrintRow(4, lcdString);
    }
  }
}  // End of loop()

// ********************************************************************************************************************************
// ******************************************************** DIAGNOSTICS ***********************************************************
// ********************************************************************************************************************************

// Handles top-level diagnostic button navigation. Returns true when we should exit diagnostics.
static bool handleDiagnosticsMenuButtons() {
  if (diagButtonPressed(0)) {
    modeCurrent = MODE_ATTRACT;
    resetDiagButtonStates();
    markAttractDisplayDirty();
    markDiagnosticsDisplayDirty(true);
    lcdClear();  // Clear display before showing exit message
    lcdShowDiagScreen("Exit Diagnostics", "", "", "");
    delay(1000);  // Brief pause to show message
    return true;
  }
  if (diagButtonPressed(1)) {
    if (diagnosticSuiteIdx == 0) {
      diagnosticSuiteIdx = NUM_DIAG_SUITES - 1;
    } else {
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
    diagEnterSelectedSuite();
    return false;  // Stay in diagnostics mode after suite exits
  }  return false;
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
// *********************************************** DIAGNOSTIC SUITE HANDLERS ******************************************************
// ********************************************************************************************************************************

// Diagnostic suite indices (must match diagSuiteNames[] order)
const byte DIAG_SUITE_VOLUME      = 0;
const byte DIAG_SUITE_LAMPS       = 1;
const byte DIAG_SUITE_SWITCHES    = 2;
const byte DIAG_SUITE_COILS       = 3;
const byte DIAG_SUITE_AUDIO       = 4;
const byte DIAG_SUITE_SETTINGS    = 5;

// Add the main settings suite handler function:

// Find diagEnterSelectedSuite() and REPLACE the entire function:

void diagEnterSelectedSuite() {
  lcdClear();

  // Execute the selected suite
  switch (diagnosticSuiteIdx) {
  case DIAG_SUITE_VOLUME:
    diagRunVolume(pLCD2004, pShiftRegister, pTsunami,
      &tsunamiGainDb, &tsunamiVoiceGainDb, &tsunamiSfxGainDb,
      &tsunamiMusicGainDb, &tsunamiDuckingDb,
      switchOldState, switchNewState);
    break;
  case DIAG_SUITE_LAMPS:
    diagRunLamps(pLCD2004, pShiftRegister, pMessage, switchOldState, switchNewState);
    break;
  case DIAG_SUITE_SWITCHES:
    diagRunSwitches(pLCD2004, pShiftRegister, pTsunami, switchOldState, switchNewState);
    break;
  case DIAG_SUITE_COILS:
    diagRunCoils(pLCD2004, pShiftRegister, pMessage, switchOldState, switchNewState);
    break;
  case DIAG_SUITE_AUDIO:
    diagRunAudio(pLCD2004, pShiftRegister, pTsunami,
      tsunamiGainDb, tsunamiVoiceGainDb, tsunamiSfxGainDb, tsunamiMusicGainDb,
      switchOldState, switchNewState);
  break;  case DIAG_SUITE_SETTINGS:
    diagRunSettings(pLCD2004, pShiftRegister, switchOldState, switchNewState);
    break;
  default:
    break;
  }

  // SIMPLE FIX: Just wait a bit and reset button states
  // The suite functions already handle their own button logic
  delay(100);  // Brief delay to let user release buttons naturally

  // Reset button states so we don't immediately trigger another action
  resetDiagButtonStates();

  // After suite exits, refresh the menu display
  lcdClear();
  markDiagnosticsDisplayDirty(true);
}

// New helper function to set attract lamp state
void setAttractLamps() {
  // Turn on GI lamps (both Master playfield and Slave head)
  setGILamps(true);
  pMessage->sendMAStoSLVGILamp(true);

  // Turn on all 7 bumper lamps (S-C-R-E-A-M-O)
  for (byte i = LAMP_IDX_S; i <= LAMP_IDX_O; i++) {
    pShiftRegister->digitalWrite(lampParm[i].pinNum, LOW);  // ON
  }

  // Turn off everything else on Master
  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    if (lampParm[i].groupNum != LAMP_GROUP_GI &&
      lampParm[i].groupNum != LAMP_GROUP_BUMPER) {
      pShiftRegister->digitalWrite(lampParm[i].pinNum, HIGH);  // OFF
    }
  }

  // Turn off Slave score and tilt lamps
  pMessage->sendMAStoSLVScoreAbs(0);
  pMessage->sendMAStoSLVTiltLamp(false);
}

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

void setGILamps(bool t_on) {
  // Turn on or off all G.I. lamps
  for (int i = 0; i < NUM_LAMPS_MASTER; i++) {
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
  if (t_switchIdx >= NUM_SWITCHES_MASTER) {
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
  if (t_switchIdx >= NUM_SWITCHES_MASTER || pShiftRegister == nullptr) {
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
  if (pLCD2004 == nullptr) {
    return;
  }

  // On first boot, keep the rev date displayed for a few seconds
  if (!initialBootDisplayShown) {
    if (millis() - bootDisplayStartMs < BOOT_DISPLAY_DURATION_MS) {
      return;  // Keep showing boot screen
    }
    // Time expired, transition to attract screen
    initialBootDisplayShown = true;
    attractDisplayDirty = true;  // Force refresh to attract screen
  }

  if (!attractDisplayDirty) {
    return;
  }

  lcdShowDiagScreen("Screamo Ready", "", "-/+ Vol  SEL=Diag", "");
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
  // Show 4-line diagnostic menu
  lcdShowDiagScreen(
    "DIAGNOSTICS",
    diagSuiteNames[diagnosticSuiteIdx],
    "-/+ to scroll",
    "SEL=run  BACK=exit"
  );
}

// Initialize game state to attract mode defaults
void initGameState() {
  gameState.numPlayers = 0;
  gameState.currentPlayer = 0;
  gameState.currentBall = 0;
  for (byte i = 0; i < 4; i++) {
    gameState.score[i] = 0;
  }
  gameState.tiltWarnings = 0;
  gameState.tilted = false;
  gameState.ballInPlay = false;
  gameState.ballSaveActive = false;
  gameState.ballSaveEndMs = 0;
  gameState.ballLaunchMs = 0;
  gameState.hasScored = false;
}

// Handle knockoff button - long press resets system
void handleKnockoffButton() {
  bool knockoffPressed = switchClosed(SWITCH_IDX_KNOCK_OFF);
  bool wasPressed = (knockoffLastState != 0);

  if (knockoffPressed && !wasPressed) {
    // Just pressed - record start time
    knockoffPressStartMs = millis();
    knockoffBeingHeld = true;
  } else if (knockoffPressed && knockoffBeingHeld) {
    // Being held - check if held long enough for reset
    unsigned long holdTime = millis() - knockoffPressStartMs;
    if (holdTime >= KNOCKOFF_RESET_HOLD_MS) {
      // Long hold - trigger software reset of both Master and Slave
      snprintf(lcdString, LCD_WIDTH + 1, "RESET TRIGGERED");
      pLCD2004->println(lcdString);
      pMessage->sendMAStoSLVCommandReset();
      delay(100);
      softwareReset();
    }
  } else if (!knockoffPressed && wasPressed) {
    // Just released - no action on short press (credit logic removed)
    knockoffBeingHeld = false;
  }

  knockoffLastState = knockoffPressed ? 1 : 0;
}

// ******************************************
// ***** COIL / DEVICE TICK HANDLER *********
// ******************************************

// Check if a device can be activated now (mechanical/safety guards).
// Returns true if activation is allowed.
bool canActivateDeviceNow(byte t_devIdx) {
  // Add any device-specific safety checks here.
  // For example, prevent flipper activation if tilted.
  if (gameState.tilted) {
    // Block flippers and scoring devices during tilt
    if (t_devIdx == DEV_IDX_FLIPPER_LEFT ||
      t_devIdx == DEV_IDX_FLIPPER_RIGHT ||
      t_devIdx == DEV_IDX_SLINGSHOT_LEFT ||
      t_devIdx == DEV_IDX_SLINGSHOT_RIGHT ||
      t_devIdx == DEV_IDX_POP_BUMPER) {
      return false;
    }
  }
  return true;
}

// Request activation of a device (coil/motor).
// If device is busy (active or resting), request is queued.
// If idle and allowed, starts immediately.
void activateDevice(byte t_devIdx) {
  if (t_devIdx >= NUM_DEVS_MASTER) {
    sprintf(lcdString, "DEV ERR %u", t_devIdx);
    pLCD2004->println(lcdString);
    return;
  }
  if (deviceParm[t_devIdx].countdown != 0) {
    deviceParm[t_devIdx].queueCount++;
    return;
  }
  if (canActivateDeviceNow(t_devIdx)) {
    analogWrite(deviceParm[t_devIdx].pinNum, deviceParm[t_devIdx].powerInitial);
    deviceParm[t_devIdx].countdown = deviceParm[t_devIdx].timeOn;
  }
}

void updateDeviceTimers() {
  for (byte i = 0; i < NUM_DEVS_MASTER; i++) {
    if (deviceParm[i].countdown > 0) {
      deviceParm[i].countdown--;
      if (deviceParm[i].countdown == 0) {
        if (deviceParm[i].powerHold > 0) {
          analogWrite(deviceParm[i].pinNum, deviceParm[i].powerHold);
        } else {
          analogWrite(deviceParm[i].pinNum, 0);
          if (deviceParm[i].timeOn > 0) {
            deviceParm[i].countdown = COIL_REST_TICKS;
          }
        }
      }
      continue;
    }

    if (deviceParm[i].countdown < 0) {
      deviceParm[i].countdown++;
      if (deviceParm[i].countdown == 0 && deviceParm[i].queueCount > 0) {
        if (canActivateDeviceNow(i)) {
          deviceParm[i].queueCount--;
          analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
          deviceParm[i].countdown = deviceParm[i].timeOn;
        }
      }
      continue;
    }

    if (deviceParm[i].queueCount > 0) {
      if (canActivateDeviceNow(i)) {
        deviceParm[i].queueCount--;
        analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
        deviceParm[i].countdown = deviceParm[i].timeOn;
      }
    }
  }
}

void releaseDevice(byte t_devIdx) {
  if (t_devIdx >= NUM_DEVS_MASTER) {
    return;
  }
  analogWrite(deviceParm[t_devIdx].pinNum, 0);
  deviceParm[t_devIdx].countdown = 0;
  deviceParm[t_devIdx].queueCount = 0;
}

// ******************************************
// ***** FLIPPER HANDLING *******************
// ******************************************

// Process flipper buttons - called every tick for immediate response.
// Flippers use hold power after initial activation.
void processFlippers() {
  // Skip flipper processing if game is tilted or not in play
  if (gameState.tilted || modeCurrent == MODE_ATTRACT || modeCurrent == MODE_DIAGNOSTIC) {
    // Ensure flippers are released
    if (leftFlipperHeld) {
      releaseDevice(DEV_IDX_FLIPPER_LEFT);
      leftFlipperHeld = false;
    }
    if (rightFlipperHeld) {
      releaseDevice(DEV_IDX_FLIPPER_RIGHT);
      rightFlipperHeld = false;
    }
    return;
  }

  // Left flipper
  bool leftPressed = switchClosed(SWITCH_IDX_FLIPPER_LEFT_BUTTON);
  if (leftPressed && !leftFlipperHeld) {
    // Button just pressed - activate flipper
    activateDevice(DEV_IDX_FLIPPER_LEFT);
    leftFlipperHeld = true;
  } else if (!leftPressed && leftFlipperHeld) {
    // Button released - release flipper
    releaseDevice(DEV_IDX_FLIPPER_LEFT);
    leftFlipperHeld = false;
  }

  // Right flipper
  bool rightPressed = switchClosed(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  if (rightPressed && !rightFlipperHeld) {
    activateDevice(DEV_IDX_FLIPPER_RIGHT);
    rightFlipperHeld = true;
  } else if (!rightPressed && rightFlipperHeld) {
    releaseDevice(DEV_IDX_FLIPPER_RIGHT);
    rightFlipperHeld = false;
  }
}

// ********************************************************************************************************************************
// ************************************************* LCD DISPLAY HELPERS **********************************************************
// ********************************************************************************************************************************

// Clear LCD and mark display states dirty so they refresh
void lcdClear() {
  if (pLCD2004 != nullptr) {
    pLCD2004->clearScreen();
  }
}

// Clear a specific row (1-4)
void lcdClearRow(byte row) {
  // Wrapper for Pinball_LCD clearRow() function.
  // Row parameter: 1..4
  if (pLCD2004 != nullptr) {
    pLCD2004->clearRow(row);
  }
}

// Display text on a specific row (1-4), left-justified, padded/truncated to 20 chars
void lcdPrintRow(byte t_row, const char* t_text) {
  if (pLCD2004 == nullptr || t_row < 1 || t_row > 4) {
    return;
  }
  // Build a 20-char padded string
  char buf[LCD_WIDTH + 1];
  memset(buf, ' ', LCD_WIDTH);
  buf[LCD_WIDTH] = '\0';
  byte len = strlen(t_text);
  if (len > LCD_WIDTH) len = LCD_WIDTH;
  memcpy(buf, t_text, len);
  pLCD2004->printRowCol(t_row, 1, buf);
}

// Display text at specific row/col (1-based), no padding
void lcdPrintAt(byte t_row, byte t_col, const char* t_text) {
  if (pLCD2004 == nullptr) {
    return;
  }
  pLCD2004->printRowCol(t_row, t_col, t_text);
}

// Format and display a 4-line diagnostic screen
void lcdShowDiagScreen(const char* line1, const char* line2, const char* line3, const char* line4) {
  lcdPrintRow(1, line1);
  lcdPrintRow(2, line2);
  lcdPrintRow(3, line3);
  lcdPrintRow(4, line4);
}

void setAllDevicesOff() {
  // Set the output latch low first, then configure as OUTPUT.
  // This avoids a brief HIGH when changing direction.
  for (int i = 0; i < NUM_DEVS_MASTER; i++) {
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
  for (int i = 0; i < NUM_DEVS_MASTER; i++) {
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
  // Handle knockoff button for reset functionality only
  handleKnockoffButton();

  // Volume adjustment with +/- buttons
  if (diagButtonPressed(1)) {  // Minus
    if (audioCurrentMusicTrack == 0) {
      audioStartMusicTrack(musTracksCircus[0].trackNum, musTracksCircus[0].lengthSeconds, false);
    }
    audioAdjustMasterGainQuiet(-1);
  }
  if (diagButtonPressed(2)) {  // Plus
    if (audioCurrentMusicTrack == 0) {
      audioStartMusicTrack(musTracksCircus[0].trackNum, musTracksCircus[0].lengthSeconds, false);
    }
    audioAdjustMasterGainQuiet(+1);
  }
  if (diagButtonPressed(0)) {  // Back - stop any test music
    audioStopMusic();
    markAttractDisplayDirty();
  }
  if (diagButtonPressed(3)) {  // Enter diagnostics
    audioStopMusic();
    resetDiagButtonStates();
    modeCurrent = MODE_DIAGNOSTIC;
    diagnosticSuiteIdx = 0;
    diagnosticState = 0;
    lcdClear();
    markDiagnosticsDisplayDirty(true);
    return;
  }

  renderAttractDisplayIfNeeded();
}

void updateModeOriginal() {
  // TODO: Implement Original style gameplay
}

void updateModeEnhanced() {
  // TODO: Implement Enhanced style gameplay
}

void updateModeImpulse() {
  // TODO: Implement Impulse style gameplay
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

// ************************************
// ***** TSUNAMI HELPER FUNCTIONS *****
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

// Apply category-specific gain offset to a track after starting it.
// Call this immediately after trackPlay() to apply voice/sfx/music offsets.
void audioApplyTrackGain(unsigned int t_trackNum, int8_t t_categoryOffset) {
  if (pTsunami == nullptr) {
    return;
  }
  // Calculate total gain: master + category offset
  int totalGain = (int)tsunamiGainDb + (int)t_categoryOffset;
  // Clamp to valid range
  if (totalGain < -70) totalGain = -70;
  if (totalGain > 10) totalGain = 10;
  // Apply to this specific track
  pTsunami->trackGain((int)t_trackNum, totalGain);
}

void audioSaveMasterGain() {
  diagSaveMasterGain(tsunamiGainDb);
}

void audioLoadMasterGain() {
  diagLoadMasterGain(&tsunamiGainDb);
}

void audioSaveCategoryGains() {
  diagSaveCategoryGains(tsunamiVoiceGainDb, tsunamiSfxGainDb, tsunamiMusicGainDb);
}

void audioLoadCategoryGains() {
  diagLoadCategoryGains(&tsunamiVoiceGainDb, &tsunamiSfxGainDb, &tsunamiMusicGainDb);
}

void audioSaveDucking() {
  diagSaveDucking(tsunamiDuckingDb);
}

void audioLoadDucking() {
  diagLoadDucking(&tsunamiDuckingDb);
}

// Stop all playback on Tsunami and restore gains from EEPROM.
// Call once during setup() after pTsunami->start().
void audioResetTsunamiState() {
  if (pTsunami == nullptr) {
    return;
  }
  // Stop all tracks
  for (int trk = 1; trk <= 256; trk++) {
    pTsunami->trackStop(trk);
  }
  // Reload and apply gains from EEPROM
  audioLoadMasterGain();
  audioLoadCategoryGains();
  audioLoadDucking();  // Add this line
  audioApplyMasterGain();
  // Clear "currently playing" bookkeeping
  audioCurrentComTrack   = 0;
  audioCurrentSfxTrack   = 0;
  audioCurrentMusicTrack = 0;
}

void audioAdjustMasterGain(int8_t t_deltaDb) {
  int newVal = (int)tsunamiGainDb + (int)t_deltaDb;
  if (newVal < TSUNAMI_GAIN_DB_MIN) newVal = TSUNAMI_GAIN_DB_MIN;
  if (newVal > TSUNAMI_GAIN_DB_MAX) newVal = TSUNAMI_GAIN_DB_MAX;
  if ((int8_t)newVal == tsunamiGainDb) return;
  tsunamiGainDb = (int8_t)newVal;
  audioApplyMasterGain();
  audioSaveMasterGain();
  sprintf(lcdString, "Vol %3d dB", (int)tsunamiGainDb);
  lcdPrintRow(4, lcdString);  // Changed from pLCD2004->println()
}

// Play a track with category-specific gain applied.
// category: 0=voice, 1=sfx, 2=music
void audioPlayTrackWithCategory(unsigned int t_trackNum, byte t_category) {
  if (pTsunami == nullptr || t_trackNum == 0) {
    return;
  }
  pTsunami->trackPlayPoly((int)t_trackNum, 0, false);

  // Apply category-specific gain offset
  int8_t offset = 0;
  switch (t_category) {
  case 0: offset = tsunamiVoiceGainDb; break;
  case 1: offset = tsunamiSfxGainDb; break;
  case 2: offset = tsunamiMusicGainDb; break;
  }
  audioApplyTrackGain(t_trackNum, offset);
}

// Legacy version for backward compatibility (defaults to SFX)
void audioPlayTrack(unsigned int t_trackNum) {
  audioPlayTrackWithCategory(t_trackNum, 1); // Default to SFX category
}

// Simple primitive to stop a track by number.
void audioStopTrack(unsigned int t_trackNum) {
  if (pTsunami == nullptr || t_trackNum == 0) {
    return;
  }
  pTsunami->trackStop((int)t_trackNum);
}

// Stop all currently playing music.
void audioStopAllMusic() {
  if (audioCurrentMusicTrack != 0) {
    audioStopTrack(audioCurrentMusicTrack);
    audioCurrentMusicTrack = 0;
  }
}

// Very simple pseudo-random index 0..(n-1).
int audioRandomInt(int t_maxExclusive) {
  if (t_maxExclusive <= 0) {
    return 0;
  }
  return (int)(millis() % (unsigned long)t_maxExclusive);
}

// Adjust master gain without playing any audio feedback.
// Use this during gameplay or attract mode.
void audioAdjustMasterGainQuiet(int8_t t_deltaDb) {
  int newVal = (int)tsunamiGainDb + (int)t_deltaDb;
  if (newVal < TSUNAMI_GAIN_DB_MIN) newVal = TSUNAMI_GAIN_DB_MIN;
  if (newVal > TSUNAMI_GAIN_DB_MAX) newVal = TSUNAMI_GAIN_DB_MAX;
  if ((int8_t)newVal == tsunamiGainDb) return;
  tsunamiGainDb = (int8_t)newVal;
  audioApplyMasterGain();
  audioSaveMasterGain();
  sprintf(lcdString, "Vol %3d dB", (int)tsunamiGainDb);
  lcdPrintRow(4, lcdString);  // Changed from pLCD2004->println()
}

// Check if current music track has finished playing (based on start time and length).
// Returns true if no music is playing or if the track has ended.
bool audioMusicHasEnded() {
  if (audioCurrentMusicTrack == 0) {
    return true;  // Nothing playing
  }
  unsigned long now = millis();
  return (now >= audioMusicEndMillis);
}

// Start a music track if not already playing. Sets end time based on track length.
// Returns true if track was started, false if same track already playing.
bool audioStartMusicTrack(unsigned int t_trackNum, byte t_lengthSeconds, bool t_loop) {
  if (pTsunami == nullptr || t_trackNum == 0) {
    return false;
  }
  // Don't restart the same track if it's already playing
  if (audioCurrentMusicTrack == t_trackNum && !audioMusicHasEnded()) {
    return false;
  }
  // Stop any currently playing music first
  if (audioCurrentMusicTrack != 0) {
    pTsunami->trackStop((int)audioCurrentMusicTrack);
  }
  // Start the new track
  audioCurrentMusicTrack = t_trackNum;
  audioMusicEndMillis = millis() + ((unsigned long)t_lengthSeconds * 1000UL);
  if (t_loop) {
    pTsunami->trackLoop((int)t_trackNum, true);
    audioMusicEndMillis = millis() + 3600000UL;  // 1 hour
  }
  pTsunami->trackPlayPoly((int)t_trackNum, 0, false);

  // Apply music category gain offset
  audioApplyTrackGain(t_trackNum, tsunamiMusicGainDb);

  return true;
}

// Stop current music track.
void audioStopMusic() {
  if (pTsunami != nullptr && audioCurrentMusicTrack != 0) {
    pTsunami->trackStop((int)audioCurrentMusicTrack);
  }
  audioCurrentMusicTrack = 0;
  audioMusicEndMillis = 0;
}

// Start a random circus music track (not the same as last played).
void audioStartRandomCircusMusic() {
  // Pick a random track different from the last one
  byte idx = audioRandomInt(NUM_MUS_CIRCUS);
  unsigned int trackNum = musTracksCircus[idx].trackNum;
  // Simple check to avoid immediate repeat (optional enhancement: use EEPROM)
  if (trackNum == audioCurrentMusicTrack && NUM_MUS_CIRCUS > 1) {
    idx = (idx + 1) % NUM_MUS_CIRCUS;
    trackNum = musTracksCircus[idx].trackNum;
  }
  audioStartMusicTrack(trackNum, musTracksCircus[idx].lengthSeconds, false);
}

// Start a random surf music track (not the same as last played).
void audioStartRandomSurfMusic() {
  byte idx = audioRandomInt(NUM_MUS_SURF);
  unsigned int trackNum = musTracksSurf[idx].trackNum;
  if (trackNum == audioCurrentMusicTrack && NUM_MUS_SURF > 1) {
    idx = (idx + 1) % NUM_MUS_SURF;
    trackNum = musTracksSurf[idx].trackNum;
  }
  audioStartMusicTrack(trackNum, musTracksSurf[idx].lengthSeconds, false);
}

// *** STUB FUNCTIONS - TO BE IMPLEMENTED ***
// These are placeholders that will be built out with the new audio system.

void audioStartMainMusic() {
  // Start a random circus track (or surf based on theme setting - TODO)
  audioStartRandomCircusMusic();
}


