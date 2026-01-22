// Screamo_Master.INO Rev: 01/21/26
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
#include <Pinball_Audio.h>
#include <Pinball_Audio_Tracks.h>  // Audio track definitions (COM, SFX, MUS)

const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 01/21/26";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
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

// *** AUDIO PLAYBACK STATE ***
unsigned int audioCurrentComTrack   = 0;  // Currently playing COM track (0 = none)
unsigned int audioCurrentSfxTrack   = 0;  // Currently playing SFX track (0 = none)
unsigned int audioCurrentMusicTrack = 0;  // Currently playing MUS track (0 = none)
unsigned long audioComEndMillis     = 0;  // When current COM track ends
unsigned long audioMusicEndMillis   = 0;  // When current music track ends
unsigned long audioLiftLoopStartMillis = 0;  // When lift loop started (for re-loop)

// *** TSUNAMI GAIN SETTINGS ***
// Master gain can range from -40dB to 0dB (full level); defaults to -10dB to allow some headroom.
// NOTE: Constants now defined in Pinball_Audio.h to avoid duplication

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

// Start button state
struct StartButtonState {
  byte pressCount;           // Presses in current window
  unsigned long firstPressMs; // When window opened
  unsigned long lastPressMs;  // Last press time
  bool windowOpen;           // True if counting presses
};

StartButtonState startButtonState = { 0, 0, 0, false };

// Start button timing constants
const unsigned long START_DEBOUNCE_MS = 100;     // Minimum time between presses

byte startButtonLastState = 0;  // For edge detection

// ******************************************
// ***** START BUTTON TAP DETECTION *********
// ******************************************

// Start button tap detection window
const unsigned long START_STYLE_DETECT_WINDOW_MS = 500;  // 500ms to detect double-tap for Enhanced

// Start button tap detection state (constants defined in Pinball_Consts.h)
byte currentStartTapState = START_TAP_IDLE;

unsigned long startTapFirstPressMs = 0;
bool startGameRequested = false;
byte startTapLastButtonState = 0;

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

// ******************************************
// ***** CREDIT SYSTEM ********************
// ******************************************

// Credits are managed by Slave - we query/command via RS485
// No local credit tracking needed

byte coinLastState = 0;  // For edge detection

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
byte         modeCurrent      = MODE_ATTRACT;
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

  // ALWAYS process (every tick, all modes except diagnostic)
  updateDeviceTimers();
  processFlippers();

  // Process these in all modes (including diagnostic for safety reset)
  processKnockoffButton();

  // Process coin entry in Attract and Game modes only
  if (modeCurrent == MODE_ATTRACT ||
    modeCurrent == MODE_ORIGINAL ||
    modeCurrent == MODE_ENHANCED ||
    modeCurrent == MODE_IMPULSE) {
    processCoinEntry();
  }

  // Dispatch by mode
  switch (modeCurrent) {
  case MODE_ATTRACT:
    updateModeAttract();
    break;
  case MODE_ORIGINAL:
  case MODE_ENHANCED:
  case MODE_IMPULSE:
    updateModeGame();
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

// Process knockoff button - long press resets system (runs every tick, all modes)
void processKnockoffButton() {
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
    // Just released - short press gives a credit
    pMessage->sendMAStoSLVCreditInc(1);
    knockoffBeingHeld = false;
  }

  knockoffLastState = knockoffPressed ? 1 : 0;
}

// ******************************************
// ***** COIN ENTRY PROCESSING **************
// ******************************************

// Process coin switch - tell Slave to add credit on closure edge
void processCoinEntry() {
  bool coinClosed = switchClosed(SWITCH_IDX_COIN_MECH);
  bool wasClosed = (coinLastState != 0);

  if (coinClosed && !wasClosed) {
    // Coin switch just closed - tell Slave to add a credit
    pMessage->sendMAStoSLVCreditInc(1);

    // Play coin sound
    // TODO: audioPlayCoinSound();

    // Update display if in attract mode (query Slave for actual count)
    if (modeCurrent == MODE_ATTRACT) {
      // We don't display credit count - Slave handles that on its own display
      // Could show a brief "CREDIT ADDED" message
      lcdPrintRow(2, "CREDIT ADDED");
      delay(500);
      markAttractDisplayDirty();
    }
  }

  coinLastState = coinClosed ? 1 : 0;
}

// ******************************************
// ***** START BUTTON PROCESSING ************
// ******************************************

// Process start button tap detection and player addition
// Called every tick from updateModeAttract()
void processStartButtonTapDetection() {
  bool startPressed = switchClosed(SWITCH_IDX_START_BUTTON);
  bool wasPressed = (startTapLastButtonState != 0);
  unsigned long now = millis();

  // Detect press edge (button just pressed)
  if (startPressed && !wasPressed) {

    // Debounce: ignore if too soon after last press
    if (now - startButtonState.lastPressMs < START_DEBOUNCE_MS) {
      startTapLastButtonState = 1;
      return;
    }
    startButtonState.lastPressMs = now;

    // Handle based on current state
    switch (currentStartTapState) {

    case START_TAP_IDLE:
      // First press - start detection window
      currentStartTapState = START_TAP_WAITING;
      startTapFirstPressMs = now;
      startGameRequested = false;
      break;

    case START_TAP_WAITING:
      // Second press within 500ms window - Enhanced style!
      if (now - startTapFirstPressMs < START_STYLE_DETECT_WINDOW_MS) {
        // Query credits
        pMessage->sendMAStoSLVCreditStatusQuery();
        delay(10);  // Brief wait for response

        bool creditsAvailable = false;
        bool gotResponse = false;
        unsigned long queryStart = millis();

        while (millis() - queryStart < 50) {
          byte msgType = pMessage->available();
          if (msgType == RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS) {
            pMessage->getSLVtoMASCreditStatus(&creditsAvailable);
            gotResponse = true;
            break;
          }
          delay(1);
        }

        if (gotResponse && creditsAvailable) {
          // Deduct credit and start Enhanced style
          pMessage->sendMAStoSLVCreditDec();
          currentStartTapState = START_TAP_ENHANCED;
          startButtonState.pressCount = 1;  // Player 1
          startGameRequested = true;

          // Play SCREAM sound effect
          // TODO: Play scream

          lcdClear();
          lcdPrintRow(1, "ENHANCED STYLE");
          lcdPrintRow(2, "PLAYER 1");
        } else {
          // No credits - play horn and announcement
          // TODO: Play Aoooga horn
          // TODO: Play "no credit" announcement
          lcdPrintRow(2, "INSERT COIN");
          currentStartTapState = START_TAP_IDLE;
        }
      }
      break;

    case START_TAP_ORIGINAL:
      // Second press AFTER 500ms - switch to Impulse style
      currentStartTapState = START_TAP_IMPULSE;
      lcdPrintRow(1, "IMPULSE STYLE");
      // No additional credit deducted
      break;

    case START_TAP_ENHANCED:
      // Additional Start press in Enhanced - try to add player
      if (startButtonState.pressCount < 4) {
        // Query credits
        pMessage->sendMAStoSLVCreditStatusQuery();
        delay(10);

        bool creditsAvailable = false;
        bool gotResponse = false;
        unsigned long queryStart = millis();

        while (millis() - queryStart < 50) {
          byte msgType = pMessage->available();
          if (msgType == RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS) {
            pMessage->getSLVtoMASCreditStatus(&creditsAvailable);
            gotResponse = true;
            break;
          }
          delay(1);
        }

        if (gotResponse && creditsAvailable) {
          // Deduct credit and add player
          pMessage->sendMAStoSLVCreditDec();
          startButtonState.pressCount++;

          // Duck music and play appropriate announcement
          // TODO: Duck music if playing
          switch (startButtonState.pressCount) {
          case 2:
            // TODO: Play "Second guest, c'mon in!"
            break;
          case 3:
            // TODO: Play "Third guest, you're in!"
            break;
          case 4:
            // TODO: Play "Fourth guest, through the turnstile!"
            break;
          }

          snprintf(lcdString, LCD_WIDTH + 1, "PLAYER %d ADDED", startButtonState.pressCount);
          lcdPrintRow(2, lcdString);
        } else {
          // No credits - play horn and announcement
          // TODO: Play Aoooga horn
          // TODO: Play "no credit" announcement
          lcdPrintRow(2, "NO CREDITS");
        }
      } else {
        // Already 4 players
        // TODO: Play "park's full" announcement
        lcdPrintRow(2, "4 PLAYERS MAX");
      }
      break;

    case START_TAP_IMPULSE:
      // Ignore additional presses in Impulse mode
      break;
    }
  }

  // Check if 500ms window has expired (transition from WAITING to ORIGINAL)
  if (currentStartTapState == START_TAP_WAITING) {
    if (now - startTapFirstPressMs >= START_STYLE_DETECT_WINDOW_MS) {
      // 500ms expired, no 2nd tap - Original style

      // Query credits
      pMessage->sendMAStoSLVCreditStatusQuery();
      delay(10);

      bool creditsAvailable = false;
      bool gotResponse = false;
      unsigned long queryStart = millis();

      while (millis() - queryStart < 50) {
        byte msgType = pMessage->available();
        if (msgType == RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS) {
          pMessage->getSLVtoMASCreditStatus(&creditsAvailable);
          gotResponse = true;
          break;
        }
        delay(1);
      }

      if (gotResponse && creditsAvailable) {
        // Deduct credit and start Original style
        pMessage->sendMAStoSLVCreditDec();
        currentStartTapState = START_TAP_ORIGINAL;
        startButtonState.pressCount = 1;  // Single player
        startGameRequested = true;

        lcdClear();
        lcdPrintRow(1, "ORIGINAL STYLE");
        lcdPrintRow(2, "PLAYER 1");
      } else {
        // No credits - reset
        currentStartTapState = START_TAP_IDLE;
      }
    }
  }

  startTapLastButtonState = startPressed ? 1 : 0;
}

// Check if we should start a game (called from updateModeAttract)
bool shouldStartGame() {
  if (startGameRequested) {
    startGameRequested = false;
    return true;
  }
  return false;
}

// Check if first point has been scored (called when score changes)
// This closes the Enhanced player addition window
void checkFirstPointScored() {
  if (currentStartTapState == START_TAP_ENHANCED && !gameState.hasScored) {
    // First point scored in Enhanced mode - close player addition window
    gameState.hasScored = true;

    // TODO: Close Ball Tray
    // TODO: Stop Ball 1 Lift sounds if playing
    // TODO: Play First Hill Screaming sounds
    // TODO: Increase Shaker Motor speed
    // TODO: Start first music track

    lcdPrintRow(3, "Game started!");

    // Reset tap detection state
    currentStartTapState = START_TAP_IDLE;
  }
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
  // Process start button tap detection
  processStartButtonTapDetection();

  // Check if we should start a game
  if (shouldStartGame()) {
    // Determine mode based on tap state
    byte numPlayers = startButtonState.pressCount;

    switch (currentStartTapState) {
    case START_TAP_ORIGINAL:
      modeCurrent = MODE_ORIGINAL;
      break;
    case START_TAP_ENHANCED:
      modeCurrent = MODE_ENHANCED;
      break;
    case START_TAP_IMPULSE:
      modeCurrent = MODE_IMPULSE;
      break;
    default:
      return;  // Shouldn't happen
    }

    // Initialize game state
    gameState.numPlayers = numPlayers;
    gameState.currentPlayer = 1;
    gameState.currentBall = 1;
    for (byte i = 0; i < 4; i++) {
      gameState.score[i] = 0;
    }
    gameState.tiltWarnings = 0;
    gameState.tilted = false;
    gameState.ballInPlay = false;
    gameState.ballSaveActive = false;
    gameState.hasScored = false;

    // Set up game display
    lcdClear();
    snprintf(lcdString, LCD_WIDTH + 1, "PLAYER 1  BALL 1");
    lcdPrintRow(1, lcdString);
    snprintf(lcdString, LCD_WIDTH + 1, "SCORE: 0");
    lcdPrintRow(2, lcdString);

    // TODO: Start game sequence (mode-specific)
    return;
  }

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

// ********************************************************************************************************************************
// ************************************************* GAME MODE (Original/Enhanced/Impulse) ****************************************
// ********************************************************************************************************************************

void updateModeGame() {
  // Handle tilt check first
  if (gameState.tilted) {
    // Already tilted - wait for ball to drain, then move to next ball/player
    // TODO: Check drain switches
    return;
  }

  // Check for tilt
  if (switchClosed(SWITCH_IDX_TILT_BOB)) {
    gameState.tiltWarnings++;
    if (gameState.tiltWarnings >= 3) {
      // TILT!
      gameState.tilted = true;
      gameState.ballInPlay = false;
      audioStopAllMusic();
      // TODO: Play tilt sound, show TILT on display and lamps
      lcdPrintRow(1, "*** TILT ***");
      return;
    } else {
      // Warning
      // TODO: Play warning sound
      snprintf(lcdString, LCD_WIDTH + 1, "WARNING %d", gameState.tiltWarnings);
      lcdPrintRow(4, lcdString);
    }
  }

  // If ball not in play, we need to dispense one
  if (!gameState.ballInPlay) {
    // Check if 5 balls are in trough (ready to dispense)
    if (switchClosed(SWITCH_IDX_5_BALLS_IN_TROUGH)) {
      // Dispense a ball
      activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);
      gameState.ballInPlay = true;
      gameState.ballLaunchMs = millis();
      gameState.hasScored = false;
      gameState.tiltWarnings = 0;
      gameState.tilted = false;

      // Start ball save timer (mode-dependent)
      // TODO: Read ball save time from EEPROM
      gameState.ballSaveActive = true;
      gameState.ballSaveEndMs = millis() + 10000UL;  // 10 second ball save

      // Update display
      snprintf(lcdString, LCD_WIDTH + 1, "PLAYER %d  BALL %d",
        gameState.currentPlayer, gameState.currentBall);
      lcdPrintRow(1, lcdString);
    }
    return;
  }

  // Ball is in play - process playfield switches
  processPlayfieldSwitches();

  // Check for drain
  if (switchClosed(SWITCH_IDX_DRAIN_LEFT) ||
    switchClosed(SWITCH_IDX_DRAIN_CENTER) ||
    switchClosed(SWITCH_IDX_DRAIN_RIGHT)) {

    // Ball save check
    if (gameState.ballSaveActive && millis() < gameState.ballSaveEndMs) {
      // Ball saved! Dispense another
      gameState.ballInPlay = false;  // Will dispense on next tick
      // TODO: Play ball saved sound
      lcdPrintRow(4, "BALL SAVED!");
      return;
    }

    // Ball is lost
    gameState.ballInPlay = false;
    gameState.ballSaveActive = false;

    // Check for end of ball
    handleEndOfBall();
  }
}

// Process all playfield scoring switches
void processPlayfieldSwitches() {
  // Pop bumpers
  for (byte i = SWITCH_IDX_BUMPER_S; i <= SWITCH_IDX_BUMPER_O; i++) {
    if (switchClosed(i)) {
      activateDevice(DEV_IDX_POP_BUMPER);
      addScore(1);  // 10,000 points (1 unit)
      gameState.hasScored = true;
      // Light corresponding bumper lamp
      // TODO: Bumper lamp logic
    }
  }

  // Slingshots
  if (switchClosed(SWITCH_IDX_SLINGSHOT_LEFT)) {
    activateDevice(DEV_IDX_SLINGSHOT_LEFT);
    addScore(1);
    gameState.hasScored = true;
  }
  if (switchClosed(SWITCH_IDX_SLINGSHOT_RIGHT)) {
    activateDevice(DEV_IDX_SLINGSHOT_RIGHT);
    addScore(1);
    gameState.hasScored = true;
  }

  // Kickout holes
  if (switchClosed(SWITCH_IDX_KICKOUT_LEFT)) {
    // Ball is in left kickout - score and kick it out
    addScore(5);  // 50,000 points
    gameState.hasScored = true;
    activateDevice(DEV_IDX_KICKOUT_LEFT);
  }
  if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT)) {
    addScore(5);
    gameState.hasScored = true;
    activateDevice(DEV_IDX_KICKOUT_RIGHT);
  }

  // Hat switches (rollovers)
  if (switchClosed(SWITCH_IDX_HAT_LEFT_TOP) ||
    switchClosed(SWITCH_IDX_HAT_LEFT_BOTTOM) ||
    switchClosed(SWITCH_IDX_HAT_RIGHT_TOP) ||
    switchClosed(SWITCH_IDX_HAT_RIGHT_BOTTOM)) {
    addScore(1);
    gameState.hasScored = true;
  }

  // Side targets
  // TODO: Add side target processing

  // Gobble hole
  if (switchClosed(SWITCH_IDX_GOBBLE)) {
    addScore(10);  // 100,000 points
    gameState.hasScored = true;
    // TODO: Gobble hole special logic
  }
}

// Add score to current player (in 10K units)
void addScore(uint16_t units) {
  byte playerIdx = gameState.currentPlayer - 1;  // Convert to 0-indexed
  uint16_t newScore = gameState.score[playerIdx] + units;
  if (newScore > 999) newScore = 999;  // Cap at 9,990,000
  gameState.score[playerIdx] = newScore;

  // Check if this is the first point (closes Enhanced player addition window)
  checkFirstPointScored();

  // Update display
  unsigned long displayScore = (unsigned long)newScore * 10000UL;
  snprintf(lcdString, LCD_WIDTH + 1, "SCORE: %lu", displayScore);
  lcdPrintRow(2, lcdString);

  // Send score to Slave for reel display
  pMessage->sendMAStoSLVScoreAbs(newScore);
}

// Handle end of ball - advance player/ball or end game
void handleEndOfBall() {
  // Check if this was the last ball for this player
  if (gameState.currentBall >= 5) {
    // Last ball - check if more players
    if (gameState.currentPlayer >= gameState.numPlayers) {
      // Game over!
      handleGameOver();
      return;
    } else {
      // Next player, ball 1
      gameState.currentPlayer++;
      gameState.currentBall = 1;
    }
  } else {
    // Same player, next ball
    gameState.currentBall++;
  }

  // Reset per-ball state
  gameState.tiltWarnings = 0;
  gameState.tilted = false;

  // Update display
  snprintf(lcdString, LCD_WIDTH + 1, "PLAYER %d  BALL %d",
    gameState.currentPlayer, gameState.currentBall);
  lcdPrintRow(1, lcdString);

  // Show this player's score
  byte playerIdx = gameState.currentPlayer - 1;
  unsigned long displayScore = (unsigned long)gameState.score[playerIdx] * 10000UL;
  snprintf(lcdString, LCD_WIDTH + 1, "SCORE: %lu", displayScore);
  lcdPrintRow(2, lcdString);

  // Send score to Slave
  pMessage->sendMAStoSLVScoreAbs(gameState.score[playerIdx]);
}

// Handle game over - return to attract mode
void handleGameOver() {
  // TODO: Play game over sound, show final scores
  lcdClear();
  lcdPrintRow(1, "GAME OVER");

  // Show high score or final score
  byte highPlayer = 0;
  uint16_t highScore = 0;
  for (byte i = 0; i < gameState.numPlayers; i++) {
    if (gameState.score[i] > highScore) {
      highScore = gameState.score[i];
      highPlayer = i + 1;
    }
  }
  unsigned long displayScore = (unsigned long)highScore * 10000UL;
  snprintf(lcdString, LCD_WIDTH + 1, "P%d: %lu", highPlayer, displayScore);
  lcdPrintRow(2, lcdString);

  delay(3000);  // Show for 3 seconds

  // Return to attract mode
  modeCurrent = MODE_ATTRACT;
  initGameState();
  setAttractLamps();
  lcdClear();
  markAttractDisplayDirty();
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
// NOTE: All audio helper functions have been moved to Pinball_Audio.h/.cpp
// The following functions are now in the Pinball_Audio library:
// - audioApplyMasterGain() - Apply master gain to all Tsunami outputs
// - audioApplyTrackGain() - Apply category-specific gain to a track
// - audioSaveMasterGain() - Save master gain to EEPROM
// - audioLoadMasterGain() - Load master gain from EEPROM
// - audioSaveCategoryGains() - Save category gains to EEPROM
// - audioLoadCategoryGains() - Load category gains from EEPROM
// - audioSaveDucking() - Save ducking level to EEPROM
// - audioLoadDucking() - Load ducking level from EEPROM
// - audioPlayTrackWithCategory() - Play track with category-specific gain
// - audioPlayTrack() - Play track (defaults to SFX)
// - audioStopTrack() - Stop a specific track
// - audioRandomInt() - Pseudo-random number generator
//
// These functions remain here as wrappers that call Pinball_Audio functions:

void audioResetTsunamiState() {
  if (pTsunami == nullptr) {
    return;
  }
  // Stop all tracks
  for (int trk = 1; trk <= 256; trk++) {
    pTsunami->trackStop(trk);
  }
  // Reload and apply gains from EEPROM using Pinball_Audio functions
  audioLoadMasterGain(&tsunamiGainDb);
  audioLoadCategoryGains(&tsunamiVoiceGainDb, &tsunamiSfxGainDb, &tsunamiMusicGainDb);
  audioLoadDucking(&tsunamiDuckingDb);
  audioApplyMasterGain(tsunamiGainDb, pTsunami);
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
  audioApplyMasterGain(tsunamiGainDb, pTsunami);  // CHANGED: Pass parameters
  audioSaveMasterGain(tsunamiGainDb);  // CHANGED: Pass parameter
  sprintf(lcdString, "Vol %3d dB", (int)tsunamiGainDb);
  lcdPrintRow(4, lcdString);
}

void audioAdjustMasterGainQuiet(int8_t t_deltaDb) {
  int newVal = (int)tsunamiGainDb + (int)t_deltaDb;
  if (newVal < TSUNAMI_GAIN_DB_MIN) newVal = TSUNAMI_GAIN_DB_MIN;
  if (newVal > TSUNAMI_GAIN_DB_MAX) newVal = TSUNAMI_GAIN_DB_MAX;
  if ((int8_t)newVal == tsunamiGainDb) return;
  tsunamiGainDb = (int8_t)newVal;
  audioApplyMasterGain(tsunamiGainDb, pTsunami);  // CHANGED: Pass parameters
  audioSaveMasterGain(tsunamiGainDb);  // CHANGED: Pass parameter
  sprintf(lcdString, "Vol %3d dB", (int)tsunamiGainDb);
  lcdPrintRow(4, lcdString);
}

void audioStopAllMusic() {
  if (audioCurrentMusicTrack != 0) {
    audioStopTrack(audioCurrentMusicTrack, pTsunami);  // CHANGED: Pass pTsunami
    audioCurrentMusicTrack = 0;
  }
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

  // Apply music category gain offset using Pinball_Audio function
  audioApplyTrackGain(t_trackNum, tsunamiMusicGainDb, tsunamiGainDb, pTsunami);  // CHANGED: Pass all parameters

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
