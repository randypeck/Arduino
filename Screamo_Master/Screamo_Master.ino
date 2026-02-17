// Screamo_Master.INO Rev: 02/16/26b
// 11/12/25: Moved Right Kickout from pin 13 to pin 44 to avoid MOSFET firing twice on power-up.  Don't use MOSFET on pin 13.
// 12/28/25: Changed flipper inputs from direct Arduino inputs to Centipede inputs.
// 01/07/26: Added "5 Balls in Trough" switch to Centipede inputs. All switches tested and working.

#include <Arduino.h>
#include <EEPROM.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
#include <Pinball_Message.h>
#include <Tsunami.h>
#include <avr/wdt.h>
#include <Pinball_Descriptions.h>
#include <Pinball_Diagnostics.h>
#include <Pinball_Audio.h>
#include <Pinball_Audio_Tracks.h>  // Audio track definitions (COM, SFX, MUS)

const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 01/26/26";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which makes it a global.
// And no need to pass lcdString[] to any functions that use it!

// ********************************************************************************************************************************
// ************************************************** HARDWARE DEFINITIONS ********************************************************
// ********************************************************************************************************************************

// ***** LAMP DEFINITIONS *****
// LampParmStruct is defined in Pinball_Consts.h

const bool LAMP_ON  = true;
const bool LAMP_OFF = false;

// Populate lampParm with pin numbers, groups, and initial state OFF
// Pin numbers range from 0 to 63 because these lamps are on Centipede #1 (outputs)
LampParmStruct lampParm[NUM_LAMPS_MASTER] = {
  // pinNum, groupNum
  { 52, LAMP_GROUP_GI      }, // GI_LEFT_TOP       =  0
  { 34, LAMP_GROUP_GI      }, // GI_LEFT_CENTER_1  =  1
  { 25, LAMP_GROUP_GI      }, // GI_LEFT_CENTER_2  =  2
  { 20, LAMP_GROUP_GI      }, // GI_LEFT_BOTTOM    =  3
  { 17, LAMP_GROUP_GI      }, // GI_RIGHT_TOP      =  4
  { 39, LAMP_GROUP_GI      }, // GI_RIGHT_CENTER_1 =  5
  { 49, LAMP_GROUP_GI      }, // GI_RIGHT_CENTER_2 =  6
  { 61, LAMP_GROUP_GI      }, // GI_RIGHT_BOTTOM   =  7
  { 51, LAMP_GROUP_BUMPER  }, // S                 =  8
  { 16, LAMP_GROUP_BUMPER  }, // C                 =  9
  { 60, LAMP_GROUP_BUMPER  }, // R                 = 10
  { 28, LAMP_GROUP_BUMPER  }, // E                 = 11
  { 29, LAMP_GROUP_BUMPER  }, // A                 = 12
  { 18, LAMP_GROUP_BUMPER  }, // M                 = 13
  { 50, LAMP_GROUP_BUMPER  }, // O                 = 14
  { 21, LAMP_GROUP_WHITE   }, // WHITE_1           = 15
  { 56, LAMP_GROUP_WHITE   }, // WHITE_2           = 16
  { 53, LAMP_GROUP_WHITE   }, // WHITE_3           = 17
  { 33, LAMP_GROUP_WHITE   }, // WHITE_4           = 18
  { 36, LAMP_GROUP_WHITE   }, // WHITE_5           = 19
  { 43, LAMP_GROUP_WHITE   }, // WHITE_6           = 20
  { 24, LAMP_GROUP_WHITE   }, // WHITE_7           = 21
  { 38, LAMP_GROUP_WHITE   }, // WHITE_8           = 22
  { 26, LAMP_GROUP_WHITE   }, // WHITE_9           = 23
  { 23, LAMP_GROUP_RED     }, // RED_1             = 24
  { 22, LAMP_GROUP_RED     }, // RED_2             = 25
  { 48, LAMP_GROUP_RED     }, // RED_3             = 26
  { 45, LAMP_GROUP_RED     }, // RED_4             = 27
  { 35, LAMP_GROUP_RED     }, // RED_5             = 28
  { 40, LAMP_GROUP_RED     }, // RED_6             = 29
  { 27, LAMP_GROUP_RED     }, // RED_7             = 30
  { 32, LAMP_GROUP_RED     }, // RED_8             = 31
  { 57, LAMP_GROUP_RED     }, // RED_9             = 32
  { 30, LAMP_GROUP_HAT     }, // HAT_LEFT_TOP      = 33
  { 44, LAMP_GROUP_HAT     }, // HAT_LEFT_BOTTOM   = 34
  { 54, LAMP_GROUP_HAT     }, // HAT_RIGHT_TOP     = 35
  { 63, LAMP_GROUP_HAT     }, // HAT_RIGHT_BOTTOM  = 36
  { 47, LAMP_GROUP_KICKOUT }, // KICKOUT_LEFT      = 37
  { 41, LAMP_GROUP_KICKOUT }, // KICKOUT_RIGHT     = 38
  { 37, LAMP_GROUP_GOBBLE  }, // SPECIAL           = 39
  { 19, LAMP_GROUP_GOBBLE  }, // GOBBLE_1          = 40
  { 31, LAMP_GROUP_GOBBLE  }, // GOBBLE_2          = 41
  { 58, LAMP_GROUP_GOBBLE  }, // GOBBLE_3          = 42
  { 62, LAMP_GROUP_GOBBLE  }, // GOBBLE_4          = 43
  { 42, LAMP_GROUP_GOBBLE  }, // GOBBLE_5          = 44
  { 46, LAMP_GROUP_NONE    }, // SPOT_NUMBER_LEFT  = 45
  { 59, LAMP_GROUP_NONE    }  // SPOT_NUMBER_RIGHT = 46
};

// ***** SWITCH DEFINITIONS *****
// SwitchParmStruct is defined in Pinball_Consts.h
// Centipede #1 (0..63) is LAMP OUTPUTS
// Centipede #2 (64..127) is SWITCH INPUTS

SwitchParmStruct switchParm[NUM_SWITCHES_MASTER] = {
  { 118 },  // SWITCH_IDX_START_BUTTON         =  0  (54 + 64)
  { 115 },  // SWITCH_IDX_DIAG_1               =  1  (51 + 64)
  { 112 },  // SWITCH_IDX_DIAG_2               =  2  (48 + 64)
  { 113 },  // SWITCH_IDX_DIAG_3               =  3  (49 + 64)
  { 116 },  // SWITCH_IDX_DIAG_4               =  4  (52 + 64)
  { 127 },  // SWITCH_IDX_KNOCK_OFF            =  5  (63 + 64)
  { 117 },  // SWITCH_IDX_COIN_MECH            =  6  (53 + 64)
  { 126 },  // SWITCH_IDX_5_BALLS_IN_TROUGH    =  7  (62 + 64)
  { 114 },  // SWITCH_IDX_BALL_IN_LIFT         =  8  (50 + 64)
  { 119 },  // SWITCH_IDX_TILT_BOB             =  9  (55 + 64)
  { 104 },  // SWITCH_IDX_BUMPER_S             = 10  (40 + 64)
  {  90 },  // SWITCH_IDX_BUMPER_C             = 11  (26 + 64)
  {  87 },  // SWITCH_IDX_BUMPER_R             = 12  (23 + 64)
  {  80 },  // SWITCH_IDX_BUMPER_E             = 13  (16 + 64)
  { 110 },  // SWITCH_IDX_BUMPER_A             = 14  (46 + 64)
  { 107 },  // SWITCH_IDX_BUMPER_M             = 15  (43 + 64)
  {  88 },  // SWITCH_IDX_BUMPER_O             = 16  (24 + 64)
  { 105 },  // SWITCH_IDX_KICKOUT_LEFT         = 17  (41 + 64)
  {  83 },  // SWITCH_IDX_KICKOUT_RIGHT        = 18  (19 + 64)
  {  84 },  // SWITCH_IDX_SLINGSHOT_LEFT       = 19  (20 + 64)
  {  98 },  // SWITCH_IDX_SLINGSHOT_RIGHT      = 20  (34 + 64)
  {  99 },  // SWITCH_IDX_HAT_LEFT_TOP         = 21  (35 + 64)
  {  81 },  // SWITCH_IDX_HAT_LEFT_BOTTOM      = 22  (17 + 64)
  {  89 },  // SWITCH_IDX_HAT_RIGHT_TOP        = 23  (25 + 64)
  {  92 },  // SWITCH_IDX_HAT_RIGHT_BOTTOM     = 24  (28 + 64)
  {  86 },  // SWITCH_IDX_LEFT_SIDE_TARGET_2   = 25  (22 + 64)
  { 108 },  // SWITCH_IDX_LEFT_SIDE_TARGET_3   = 26  (44 + 64)
  {  85 },  // SWITCH_IDX_LEFT_SIDE_TARGET_4   = 27  (21 + 64)
  {  96 },  // SWITCH_IDX_LEFT_SIDE_TARGET_5   = 28  (32 + 64)
  {  82 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_1  = 29  (18 + 64)
  { 111 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_2  = 30  (47 + 64)
  {  94 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_3  = 31  (30 + 64)
  {  95 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_4  = 32  (31 + 64)
  {  91 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_5  = 33  (27 + 64)
  { 109 },  // SWITCH_IDX_GOBBLE               = 34  (45 + 64)
  { 106 },  // SWITCH_IDX_DRAIN_LEFT           = 35  (42 + 64)
  {  97 },  // SWITCH_IDX_DRAIN_CENTER         = 36  (33 + 64)
  {  93 },  // SWITCH_IDX_DRAIN_RIGHT          = 37  (29 + 64)
  { 125 },  // SWITCH_IDX_FLIPPER_LEFT_BUTTON  = 38  (60 + 64)
  { 124 }   // SWITCH_IDX_FLIPPER_RIGHT_BUTTON = 39  (61 + 64)
};

// ***** COIL AND MOTOR DEFINITIONS *****
// DeviceParmStruct is defined in Pinball_Consts.h

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

const int8_t COIL_REST_TICKS           =  -8;  // Coil rest period: 8 ticks * 10ms = 80ms
const uint8_t IMPULSE_FLIPPER_UP_TICKS =  10;  // Hold impulse flippers up for 100ms (10 ticks) before releasing

const byte COIL_WATCHDOG_MARGIN_TICKS = 5;           // 50ms grace period for countdown corruption detection
const byte COIL_WATCHDOG_SCAN_INTERVAL_TICKS = 100;  // 1 second full PWM scan
byte coilWatchdogScanCounter = 0;                    // Counter for periodic full scan

// ********************************************************************************************************************************
// *************************************************** HARDWARE OBJECTS ***********************************************************
// ********************************************************************************************************************************

Pinball_LCD* pLCD2004 = nullptr;
Pinball_Message* pMessage = nullptr;
Pinball_Centipede* pShiftRegister = nullptr;
Tsunami* pTsunami = nullptr;

// ********************************************************************************************************************************
// ***************************************************** SWITCH STATE *************************************************************
// ********************************************************************************************************************************

unsigned int switchOldState[4] = { 65535, 65535, 65535, 65535 };
unsigned int switchNewState[4] = { 65535, 65535, 65535, 65535 };
bool switchJustClosedFlag[NUM_SWITCHES_MASTER] = { false };
bool switchJustOpenedFlag[NUM_SWITCHES_MASTER] = { false };
byte switchDebounceCounter[NUM_SWITCHES_MASTER] = { 0 };
byte switchLastState[NUM_SWITCHES_MASTER] = { 0 };
const byte SWITCH_DEBOUNCE_TICKS = 5;  // 50ms debounce

// ********************************************************************************************************************************
// ****************************************************** SCHEDULER ***************************************************************
// ********************************************************************************************************************************

const unsigned int LOOP_TICK_MS = 10;
unsigned long loopNextMillis = 0;

// ********************************************************************************************************************************
// ***************************************************** DISPLAY STATE ************************************************************
// ********************************************************************************************************************************

bool initialBootDisplayShown = false;
unsigned long bootDisplayStartMs = 0;
const unsigned long BOOT_DISPLAY_DURATION_MS = 3000;  // Show rev date for 3 seconds
bool attractDisplayDirty = true;

// ********************************************************************************************************************************
// ******************************************************** AUDIO STATE ***********************************************************
// ********************************************************************************************************************************

// ***** PLAYBACK TRACKING *****
unsigned int audioCurrentComTrack      = 0;  // Currently playing COM track (0 = none)
unsigned int audioCurrentSfxTrack      = 0;  // Currently playing SFX track (0 = none)
unsigned int audioCurrentMusicTrack    = 0;  // Currently playing MUS track (0 = none)
unsigned long audioComEndMillis        = 0;  // When current COM track ends
unsigned long audioMusicEndMillis      = 0;  // When current music track ends
unsigned long audioLiftLoopStartMillis = 0;  // When lift loop started (for re-loop)

// ***** MUSIC THEME SELECTION *****
byte primaryMusicTheme = 0;  // 0=Circus, 1=Surf (loaded from EEPROM)
byte lastCircusTrackIdx = 0;  // Last played Circus track index (to avoid repeats)
byte lastSurfTrackIdx = 0;    // Last played Surf track index (to avoid repeats)

// ***** TSUNAMI GAIN SETTINGS *****
int8_t tsunamiGainDb      = TSUNAMI_GAIN_DB_DEFAULT;  // Persisted Tsunami master gain
int8_t tsunamiVoiceGainDb = 0;  // Per-category relative gain (COM/Voice)
int8_t tsunamiSfxGainDb   = 0;  // Per-category relative gain (SFX)
int8_t tsunamiMusicGainDb = 0;  // Per-category relative gain (Music)
int8_t tsunamiDuckingDb   = -20;  // Ducking gain offset (default -20dB)

// ********************************************************************************************************************************
// ***************************************************** DIAGNOSTICS **************************************************************
// ********************************************************************************************************************************

// ***** DIAGNOSTIC SUITE CONSTANTS *****
const byte NUM_DIAG_SUITES = 6;
const char* diagSuiteNames[NUM_DIAG_SUITES] = {
  "VOLUME",
  "LAMP TESTS",
  "SWITCH TESTS",
  "COIL/MOTOR TESTS",
  "AUDIO TESTS",
  "SETTINGS"
};

const byte DIAG_SUITE_VOLUME   = 0;
const byte DIAG_SUITE_LAMPS    = 1;
const byte DIAG_SUITE_SWITCHES = 2;
const byte DIAG_SUITE_COILS    = 3;
const byte DIAG_SUITE_AUDIO    = 4;
const byte DIAG_SUITE_SETTINGS = 5;

// ***** DIAGNOSTIC STATE *****
byte diagnosticSuiteIdx = 0;
const byte NUM_DIAG_BUTTONS = 4;
byte diagButtonLastState[NUM_DIAG_BUTTONS] = { 0, 0, 0, 0 };
byte diagButtonDebounce[NUM_DIAG_BUTTONS] = { 0, 0, 0, 0 };
const byte DIAG_DEBOUNCE_TICKS = 10;  // 100ms debounce (10 ticks * 10ms)
bool diagDisplayDirty = true;
byte diagLastRenderedSuite = 0xFF;

// Forward declaration for function with default parameter
void markDiagnosticsDisplayDirty(bool forceSuiteReset = false);

// ********************************************************************************************************************************
// ***************************************************** GAME STATE ***************************************************************
// ********************************************************************************************************************************

const byte MAX_PLAYERS                       =    1;  // Currently only support single player, even for Enhanced
const byte MAX_BALLS                         =    5;
const byte MAX_MODES_PER_GAME                =    3;  // Maximum modes that can be played per game

byte gamePhase = PHASE_ATTRACT;

// ***** STARTUP SUB-STATE CONSTANTS *****
// Phase 1: Ball recovery (ensure all 5 balls are in trough before dispensing)
const byte STARTUP_RECOVER_INIT       = 0;   // Open tray, eject kickouts, start waiting
const byte STARTUP_RECOVER_WAIT       = 1;   // Tick-based wait for 5-balls-in-trough switch
const byte STARTUP_RECOVER_CONFIRMED  = 2;   // 5 balls confirmed, close tray, play start audio
// Phase 2: Ball dispense (release one ball to player)
const byte STARTUP_DISPENSE_BALL      = 3;   // Fire ball trough release coil
const byte STARTUP_WAIT_FOR_LIFT      = 4;   // Wait for ball-in-lift switch
const byte STARTUP_COMPLETE           = 5;   // Ball in lift, transition to PHASE_BALL_READY

byte startupSubState = STARTUP_RECOVER_INIT;
unsigned int startupTickCounter = 0;

// ***** STARTUP PHASE TIMEOUTS *****
const unsigned long BALL_TROUGH_TO_LIFT_TIMEOUT_MS = 3000;  // 3 seconds max for ball to reach lift from trough release
unsigned long startupBallDispenseMs = 0;  // When ball was dispensed (for timeout tracking)

// ***** BALL RECOVERY TIMING *****
// During STARTUP_RECOVER_WAIT, we wait for the 5-balls-in-trough switch to close and remain
// closed for BALL_DETECTION_STABILITY_MS. If it does not settle within BALL_RECOVERY_TIMEOUT_MS,
// we enter PHASE_BALL_SEARCH.
const unsigned long BALL_RECOVERY_TIMEOUT_MS = 7000;  // 7 seconds total (was 5s wait + 2s kickout in old blocking code)
unsigned long ballRecoveryStartMs = 0;         // When recovery started (for timeout)
unsigned long ballRecoveryStableStartMs = 0;   // When 5-balls switch first closed (for stability check)
bool ballRecoverySwitchWasStable = false;       // True if switch has been continuously closed
bool ballRecoveryBallInLiftAtStart = false;     // Snapshot of ball-in-lift at recovery start (for ball search audio)

// ***** BALL LIFT TRACKING *****
bool prevBallInLift = false;  // Previous state of ball-in-lift switch (for edge detection)

// ***** GAME STYLE SELECTION *****
byte gameStyle = STYLE_NONE;  // Selected style (set by start button taps)

// ***** GAME STATE *****
// GameStateStruct is defined in Pinball_Consts.h
GameStateStruct gameState;

// ***** BALL TRACKING *****
// Counter-based tracking per Overview Section 3.5.4.
// These are standalone globals (not in a struct) because they are read/written frequently in tight game logic.
byte ballsInPlay = 0;         // Balls actively on playfield (0-3). Increment on first score or replacement launch.
bool ballInLift = false;      // True if a ball is waiting at the base of the Ball Lift.
byte ballsLocked = 0;         // Balls captured in kickouts (0-2). Enhanced style only; persists across ball numbers.

// ***** MULTIBALL STATE *****
bool inMultiball = false;     // True during multiball (Enhanced style only)

// ***** EXTRA BALL STATE *****
bool extraBallAwarded = false;  // Extra Ball earned, waiting to be used

// ***** KICKOUT RETRY *****
// When a kickout fires, set its retry counter. Each tick, decrement. If switch still closed and counter
// reaches zero and coil is idle, fire again. See Overview Section 10.3.10.
const byte KICKOUT_RETRY_TICKS = 100;  // 100 ticks = 1000ms between retry attempts
byte kickoutLeftRetryTicks = 0;
byte kickoutRightRetryTicks = 0;

// ***** GAME-WITHIN-A-GAME MODE STATE *****
// ModeStateStruct is defined in Pinball_Consts.h
ModeStateStruct modeState;

// ***** BALL SAVE STATE *****
// BallSaveStruct is defined in Pinball_Consts.h
// Ball save uses a simple timestamp: active when endMs > 0 and millis() < endMs.
// No 'ballSaveUsed' flag needed -- if the timer is still running after a save, the player gets another save.
// The timer naturally expires, so no special one-shot logic is required.
BallSaveStruct ballSave;
const byte BALL_SAVE_DEFAULT_SECONDS = 15;

// ***** START BUTTON DETECTION *****
const unsigned long START_STYLE_DETECT_WINDOW_MS = 500;  // 500ms to detect double-tap for Enhanced
unsigned long startFirstPressMs = 0;
bool startWaitingForSecondTap = false;

// ***** BALL SEARCH STATES *****
// Ball search sub-state constants
const byte BALL_SEARCH_INIT           = 0;  // Initialize search
const byte BALL_SEARCH_EJECT_KICKOUTS = 1;  // Fire both kickouts
const byte BALL_SEARCH_OPEN_TRAY      = 2;  // Open ball tray
const byte BALL_SEARCH_WAIT_TRAY      = 3;  // Wait 5 seconds with tray open
const byte BALL_SEARCH_CLOSE_TRAY     = 4;  // Close ball tray (if needed)
const byte BALL_SEARCH_MONITOR        = 5;  // Passive monitoring loop

byte ballSearchSubState = BALL_SEARCH_INIT;
unsigned long ballSearchLastKickoutMs = 0;
const unsigned long BALL_SEARCH_KICKOUT_INTERVAL_MS = 15000;  // Fire kickouts during ball search every 15 seconds

// ***** BALL TRAY TIMEOUT *****
// If we start a game and need to do a ball search, give up after this much time has passed.
unsigned long ballTrayOpenStartMs = 0;
const unsigned long BALL_SEARCH_TIMEOUT_MS = 30000;  // 30 seconds for testing (change to 300000 for 5 minutes)

// ***** BALL DETECTION STABILITY CHECK *****
// When waiting for balls to arrive in trough, require "5 balls" switch to remain closed for this long
// to confirm balls are settled (not just rolling over the switch momentarily).
const unsigned long BALL_DETECTION_STABILITY_MS = 1000;  // 1 second (adjustable)

// ***** HILL CLIMB TRACKING *****
bool hillClimbStarted = false;  // True once hill climb audio/shaker has started (to prevent re-triggering)
bool ballSettledInLift = false;  // True once ball is confirmed resting at lift base (prevents premature hill climb trigger)

// ***** SHAKER MOTOR POWER LEVELS *****
const byte SHAKER_POWER_HILL_CLIMB = 80;   // Low rumble during hill climb
const byte SHAKER_POWER_HILL_DROP = 110;   // Medium intensity during first drop

// ***** HILL CLIMB/DROP TIMING *****
const unsigned long HILL_CLIMB_TIMEOUT_MS = 30000;  // 30 seconds before "shoot the ball" reminder

// ***** SHAKER MOTOR TIMING CONSTANTS *****
const unsigned long SHAKER_HILL_DROP_MS = 11000;    // Hill drop duration (matches track 404)
const unsigned long SHAKER_GOBBLE_MS = 1000;        // Gobble Hole Shooting Gallery hit
const unsigned long SHAKER_BUMPER_MS = 500;         // Bumper Cars hit
const unsigned long SHAKER_HAT_MS = 800;            // Roll-A-Ball hat rollover
const unsigned long SHAKER_DRAIN_MS = 2000;         // Ball drain (non-mode)
unsigned long shakerDropStartMs = 0;                // When hill drop shaker started (0 = not running)

// ***** FLIPPER STATE *****
bool leftFlipperHeld = false;
bool rightFlipperHeld = false;

// ***** KNOCKOFF BUTTON STATE *****
unsigned long knockoffPressStartMs = 0;
bool knockoffBeingHeld = false;
const unsigned long KNOCKOFF_RESET_HOLD_MS = 1000;  // Hold knockoff for 1s to trigger reset

// ***** SELECTION UNIT *****
const byte SELECTION_UNIT_RED_INSERT[10] = { 8, 3, 4, 9, 2, 5, 7, 2, 1, 6 };
// Position 0-49 maps to: RED = SELECTION_UNIT_RED_INSERT[position % 10]
// HATs lit when: (position % 10 == 0) && (position / 10) is 0, 2, or 4

// ***** EM EMULATION STATE *****
// These track the simulated state of original EM devices. See Overview Section 13.3.
byte selectionUnitPosition = 0;   // 0-49, randomized at game start. Advances on side target hits only.
byte gobbleCount = 0;             // Balls gobbled this game (0-5). 5th gobble awards replay if SPECIAL is lit.
byte bumperLitMask = 0x7F;        // Which bumpers are lit (bits 0-6 = S,C,R,E,A,M,O). 0x7F = all lit.
unsigned int whiteLitMask = 0;    // Which WHITE inserts are lit (bits 0-8 = numbers 1-9). 0 = none lit.

// ***** MISCELLANEOUS *****
const unsigned long MODE_END_GRACE_PERIOD_MS = 5000;  // 5 seconds drain after mode ends, you still get the ball back
const byte BALL_DRAIN_INSULT_FREQUENCY = 50;  // 0..100 indicates percent of the time we make a random snarky comment when the ball drains
const unsigned SCORE_MOTOR_CYCLE_MS = 882;                 // one 1/4 revolution of score motor (ms)
const int EEPROM_SAVE_INTERVAL_MS             = 10000;  // Save interval for EEPROM (milliseconds)
int lastDisplayedScore = 0;  // Score to display at power-up (loaded from EEPROM)
unsigned long lastEepromSaveMs = 0;  // When we last saved to EEPROM
unsigned long gameTimeoutMs = (unsigned long)diagReadGameTimeoutFromEEPROM() * 60000UL;

// ********************************************************************************************************************************
// ********************************************************* SETUP ****************************************************************
// ********************************************************************************************************************************

void setup() {

  initScreamoMasterArduinoPins();  // Arduino direct I/O pins only.
  setAllDevicesOff();              // Set initial state for all MOSFET PWM and Centipede outputs
  // Seed random number generator with some noise from an unconnected analog pin.
  randomSeed(analogRead(A0));      // We'll use this to select random COM lines and snarky comments on drains

  // Initialize I2C and Centipede as early as possible to minimize relay-on (turns on all lamps) at power-up
  Wire.begin();
  pShiftRegister = new Pinball_Centipede;
  pShiftRegister->begin();
  pShiftRegister->initScreamoMasterCentipedePins();

  setPWMFrequencies();  // Set non-default PWM frequencies so coils don't buzz
  clearAllPWM();        // Clear any PWM outputs that may have been set

  // Initialize serial ports
  Serial.begin(SERIAL0_SPEED);  // PC serial monitor window 115200

  // Initialize RS485 Message
  pMessage = new Pinball_Message;
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // Initialize LCD
  delay(750);  // Give Digole 2004 LCD time to power up
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);
  pLCD2004->begin();
  pLCD2004->println(lcdString);  // Display app version

  // Set boot display timing
  bootDisplayStartMs = millis();
  initialBootDisplayShown = false;

  // Load and display last game score (emulates EM behavior)
  loadLastScore();
  pMessage->sendMAStoSLVScoreAbs(lastDisplayedScore);
  sprintf(lcdString, "Last: %d", lastDisplayedScore); pLCD2004->println(lcdString);

  // Initialize Tsunami WAV player
  pTsunami = new Tsunami();
  pTsunami->start();
  delay(10);
  pTsunami->stopAllTracks();
  delay(50);
  audioResetTsunamiState();

  // Load music theme preference
  audioLoadThemePreference(&primaryMusicTheme, &lastCircusTrackIdx, &lastSurfTrackIdx);

  // Initialize lamps and game state
  setGILamps(true);
  pMessage->sendMAStoSLVGILamp(true);
  setAttractLamps();
  initGameState();
}

// ********************************************************************************************************************************
// ********************************************************* LOOP *****************************************************************
// ********************************************************************************************************************************

void loop() {
  // Simple 10ms tick-based main loop. All game logic runs at this cadence.
  unsigned long now = millis();
  if ((long)(now - loopNextMillis) < 0) {
    return;  // Not time yet for next tick
  }

  unsigned long loopStartMicros = micros();
  loopNextMillis = now + LOOP_TICK_MS;

  // Update Centipede #2 switch snapshot once per tick
  for (int i = 0; i < 4; i++) {
    switchOldState[i] = switchNewState[i];
    switchNewState[i] = pShiftRegister->portRead(4 + i);  // ports 4..7 => inputs
  }

  // Compute all switch edge detection flags (MUST be called after portRead, before any switch processing)
  updateAllSwitchStates();

  // ALWAYS process (every tick, all phases)
  updateDeviceTimers();
  processFlippers();
  processKnockoffButton();

  // Process coin entry in non-diagnostic phases
  if (gamePhase != PHASE_DIAGNOSTIC) {
    processCoinEntry();
  }

  // Dispatch by game phase
  switch (gamePhase) {
  case PHASE_DIAGNOSTIC:
    updateModeDiagnostic();
    break;

  case PHASE_BALL_SEARCH:
    updatePhaseBallSearch();
    break;

  case PHASE_STARTUP:
    updatePhaseStartup();
    break;

  case PHASE_BALL_IN_PLAY:
    updatePhaseBallInPlay();
    break;

  case PHASE_BALL_READY:
    updatePhaseBallReady();
    break;

  default:
    // Temporary attract mode: show screen and check for diagnostic entry
    renderAttractDisplayIfNeeded();

    // Auto-eject any balls in side kickouts (cleanup after games)
    if (switchClosed(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
    }
    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
    }

    // Check for start button (single tap = Original/Impulse, double tap = Enhanced)
    processStartButton();

    // Volume adjustment with LEFT/RIGHT buttons
    if (diagButtonPressed(1)) {  // LEFT (minus)
      if (audioCurrentMusicTrack == 0) {
        audioStartMusicTrack(musTracksCircus[0].trackNum, musTracksCircus[0].lengthSeconds, false);
      }
      audioAdjustMasterGainQuiet(-1);
    }
    if (diagButtonPressed(2)) {  // RIGHT (plus)
      if (audioCurrentMusicTrack == 0) {
        audioStartMusicTrack(musTracksCircus[0].trackNum, musTracksCircus[0].lengthSeconds, false);
      }
      audioAdjustMasterGainQuiet(+1);
    }
    if (diagButtonPressed(0)) {  // BACK - stop test music
      audioStopMusic();
      markAttractDisplayDirty();
    }

    // Check for SELECT button to enter diagnostics
    if (diagButtonPressed(3)) {
      audioStopMusic();
      gamePhase = PHASE_DIAGNOSTIC;
      diagnosticSuiteIdx = 0;
      lcdClear();
      markDiagnosticsDisplayDirty(true);
      resetDiagButtonStates();
      // handleDiagnosticsMenuButtons() will wait for release via its static flag
    }
    break;
  }

  // ONLY show SLOW LOOP warning during active gameplay
  if (gamePhase == PHASE_BALL_IN_PLAY) {
    unsigned long loopDuration = micros() - loopStartMicros;
    if (loopDuration > 9000) {
      Serial.print("SLOW LOOP: ");
      Serial.println(loopDuration);
      snprintf(lcdString, LCD_WIDTH + 1, "SLOW: %lu us", loopDuration);
      lcdPrintRow(4, lcdString);
    }
  }
}

// ********************************************************************************************************************************
// ********************************************** SWITCH & INPUT PROCESSING *******************************************************
// ********************************************************************************************************************************

// Called once per tick to update all switch states and edge flags
void updateAllSwitchStates() {
  for (byte i = 0; i < NUM_SWITCHES_MASTER; i++) {
    if (switchDebounceCounter[i] > 0) {
      switchDebounceCounter[i]--;
    }

    bool closedNow = switchClosed(i);
    bool wasOpen = (switchLastState[i] == 0);

    switchJustClosedFlag[i] = false;
    switchJustOpenedFlag[i] = false;

    // EDGE DETECTION: Switch just closed (with debounce)
    if (closedNow && wasOpen && switchDebounceCounter[i] == 0) {
      switchJustClosedFlag[i] = true;
      switchDebounceCounter[i] = SWITCH_DEBOUNCE_TICKS;
    }

    // EDGE DETECTION: Switch just opened (no debounce needed for opens)
    if (!closedNow && !wasOpen) {
      switchJustOpenedFlag[i] = true;
    }

    switchLastState[i] = closedNow ? 1 : 0;
  }
}

void processFlippers() {
  // Flippers ONLY work in these phases (NOT ball search, NOT attract)
  if (gamePhase != PHASE_BALL_IN_PLAY && gamePhase != PHASE_BALL_READY) {
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

  // Additional check: disable if tilted
  if (gameState.tilted) {
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

  // Normal flipper operation
  bool leftPressed = switchClosed(SWITCH_IDX_FLIPPER_LEFT_BUTTON);
  if (leftPressed && !leftFlipperHeld) {
    activateDevice(DEV_IDX_FLIPPER_LEFT);
    leftFlipperHeld = true;
  } else if (!leftPressed && leftFlipperHeld) {
    releaseDevice(DEV_IDX_FLIPPER_LEFT);
    leftFlipperHeld = false;
  }

  bool rightPressed = switchClosed(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
  if (rightPressed && !rightFlipperHeld) {
    activateDevice(DEV_IDX_FLIPPER_RIGHT);
    rightFlipperHeld = true;
  } else if (!rightPressed && rightFlipperHeld) {
    releaseDevice(DEV_IDX_FLIPPER_RIGHT);
    rightFlipperHeld = false;
  }
}

void processCoinEntry() {
  if (switchJustClosedFlag[SWITCH_IDX_COIN_MECH]) {
    pMessage->sendMAStoSLVCreditInc(1);
    activateDevice(DEV_IDX_KNOCKER);
  }
}

void processKnockoffButton() {
  bool knockoffPressed = switchClosed(SWITCH_IDX_KNOCK_OFF);

  // RISING EDGE: Button just pressed
  if (switchJustClosedFlag[SWITCH_IDX_KNOCK_OFF]) {
    knockoffPressStartMs = millis();
    knockoffBeingHeld = true;
  }

  // WHILE HELD: Check for long-press reset
  if (knockoffPressed && knockoffBeingHeld) {
    unsigned long holdTime = millis() - knockoffPressStartMs;
    if (holdTime >= KNOCKOFF_RESET_HOLD_MS) {
      lcdClear();
      lcdPrintRow(1, "RESET TRIGGERED");
      lcdPrintRow(2, "Resetting...");
      pMessage->sendMAStoSLVCommandReset();
      delay(100);
      softwareReset();
    }
  }

  // FALLING EDGE: Button just released
  if (switchJustOpenedFlag[SWITCH_IDX_KNOCK_OFF] && knockoffBeingHeld) {
    knockoffBeingHeld = false;
    if (gamePhase != PHASE_DIAGNOSTIC) {
      pMessage->sendMAStoSLVCreditInc(1);
      activateDevice(DEV_IDX_KNOCKER);
    }
  }
}

void processStartButton() {
  // Only process start button in attract mode
  if (gamePhase != PHASE_ATTRACT) {
    return;
  }

  // First tap detection
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON] && !startWaitingForSecondTap && gameStyle == STYLE_NONE) {
    startFirstPressMs = millis();
    startWaitingForSecondTap = true;
    return;
  }

  // Second tap detection (within 500ms window)
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON] && startWaitingForSecondTap) {
    unsigned long elapsed = millis() - startFirstPressMs;
    if (elapsed < START_STYLE_DETECT_WINDOW_MS) {
      // Double-tap detected -> Enhanced mode
      gameStyle = STYLE_ENHANCED;
      startWaitingForSecondTap = false;

      // Check for credits
      bool creditsAvailable = false;
      if (!queryCreditStatus(&creditsAvailable)) {
        return;
      }

      if (!creditsAvailable) {
        // Play audio rejection (no LCD)
        pTsunami->trackPlayPoly(TRACK_START_REJECT_HORN, 0, false);
        audioApplyTrackGain(TRACK_START_REJECT_HORN, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
        delay(800);

        byte randomIdx = (byte)(millis() % NUM_COM_START_REJECT);
        unsigned int trackNum = comTracksStartReject[randomIdx].trackNum;
        byte trackLength = comTracksStartReject[randomIdx].lengthTenths;
        pTsunami->trackPlayPoly((int)trackNum, 0, false);
        audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = trackNum;
        audioComEndMillis = millis() + ((unsigned long)trackLength * 100UL);

        delay(2000);
        gameStyle = STYLE_NONE;
        return;
      }

      // Credits available - transition to PHASE_STARTUP for ball recovery
      gamePhase = PHASE_STARTUP;
      startupSubState = STARTUP_RECOVER_INIT;
      startupTickCounter = 0;
      return;
    }
  }

  // Timeout check: single tap confirmed
  if (startWaitingForSecondTap) {
    unsigned long elapsed = millis() - startFirstPressMs;
    if (elapsed >= START_STYLE_DETECT_WINDOW_MS) {
      // Single tap confirmed -> Original mode
      gameStyle = STYLE_ORIGINAL;
      startWaitingForSecondTap = false;

      // Check for credits
      bool creditsAvailable = false;
      if (!queryCreditStatus(&creditsAvailable)) {
        return;
      }

      if (!creditsAvailable) {
        // Original mode: silent, no feedback
        delay(2000);
        gameStyle = STYLE_NONE;
        return;
      }

      // Credits available - transition to PHASE_STARTUP for ball recovery
      gamePhase = PHASE_STARTUP;
      startupSubState = STARTUP_RECOVER_INIT;
      startupTickCounter = 0;
      return;
    }
  }
}

// ********************************************************************************************************************************
// ***************************************************** DIAGNOSTICS **************************************************************
// ********************************************************************************************************************************

static bool handleDiagnosticsMenuButtons() {
  // Wait for ALL diag buttons to be released before accepting any new presses.
  // This prevents carry-over from the SELECT press that entered diagnostics,
  // and also prevents carry-over when returning from a suite.
  static bool waitingForRelease = true;
  if (waitingForRelease) {
    bool anyHeld = false;
    for (byte i = 0; i < NUM_DIAG_BUTTONS; i++) {
      if (switchClosed((byte)(SWITCH_IDX_DIAG_1 + i))) {
        anyHeld = true;
        break;
      }
    }
    if (anyHeld) {
      return false;  // Don't process buttons until all are released
    }
    // All released - reset edge detection and resume
    waitingForRelease = false;
    resetDiagButtonStates();
  }

  if (diagButtonPressed(0)) {
    waitingForRelease = true;  // Reset for next entry
    gamePhase = PHASE_ATTRACT;
    resetDiagButtonStates();
    markAttractDisplayDirty();
    markDiagnosticsDisplayDirty(true);
    lcdClear();
    lcdShowDiagScreen("Exit Diagnostics", "", "", "");
    delay(1000);
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
    waitingForRelease = true;  // Reset for when we return from suite
    diagEnterSelectedSuite();
    return false;
  }
  return false;
}

void runDiagnosticsLoop() {
  if (handleDiagnosticsMenuButtons()) {
    return;
  }
  renderDiagnosticsMenuIfNeeded();
}

void diagEnterSelectedSuite() {
  lcdClear();

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
    break;
  case DIAG_SUITE_SETTINGS:
    diagRunSettings(pLCD2004, pShiftRegister, switchOldState, switchNewState);
    break;
  default:
    break;
  }

  delay(100);
  resetDiagButtonStates();
  lcdClear();
  markDiagnosticsDisplayDirty(true);
  // Note: waitingForRelease is set by handleDiagnosticsMenuButtons() before calling us
}

void updateModeDiagnostic() {
  runDiagnosticsLoop();
}

// ********************************************************************************************************************************
// ************************************************** GAME PHASE HANDLERS *********************************************************
// ********************************************************************************************************************************

// ***** STARTUP PHASE HANDLER *****
// Runs every 10ms tick. Handles ball recovery (ensuring all 5 balls are in trough),
// then dispenses one ball to the player. No blocking delays except brief audio playback
// during game-start fanfare which is acceptable before gameplay begins.
void updatePhaseStartup() {
  startupTickCounter++;

  switch (startupSubState) {

    // -----------------------------------------------------------------------
    // PHASE 1: BALL RECOVERY - Get all 5 balls into the trough
    // -----------------------------------------------------------------------

  case STARTUP_RECOVER_INIT:
  {
    // Snapshot lift switch state before we start (for ball search audio later if needed)
    ballRecoveryBallInLiftAtStart = switchClosed(SWITCH_IDX_BALL_IN_LIFT);

    // Open ball tray so any balls in tray drop into trough
    activateDevice(DEV_IDX_BALL_TRAY_RELEASE);
    gameState.ballTrayOpen = true;

    // Eject any balls stuck in kickouts
    if (switchClosed(SWITCH_IDX_KICKOUT_LEFT)) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
    }
    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT)) {
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
    }

    // Start recovery timer
    ballRecoveryStartMs = millis();
    ballRecoveryStableStartMs = 0;
    ballRecoverySwitchWasStable = false;
    ballTrayOpenStartMs = millis();  // For ball search timeout if we get there

    // Check if balls are already in trough (common case - game just ended)
    if (allBallsInTrough()) {
      // Start stability verification immediately
      ballRecoveryStableStartMs = millis();
      ballRecoverySwitchWasStable = true;
    }

    startupSubState = STARTUP_RECOVER_WAIT;
    startupTickCounter = 0;
    Serial.println("DEBUG: STARTUP_RECOVER_INIT -> RECOVER_WAIT");
    break;
  }

  case STARTUP_RECOVER_WAIT:
  {
    // Continuously eject any balls that land in kickouts
    if (switchClosed(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
    }
    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
    }

    // Monitor the 5-balls-in-trough switch for stability
    bool troughClosed = allBallsInTrough();

    if (troughClosed) {
      if (!ballRecoverySwitchWasStable) {
        // Switch just closed - start stability timer
        ballRecoveryStableStartMs = millis();
        ballRecoverySwitchWasStable = true;
      } else {
        // Switch has been closed - check if stable long enough
        if (millis() - ballRecoveryStableStartMs >= BALL_DETECTION_STABILITY_MS) {
          // All 5 balls confirmed in trough!
          startupSubState = STARTUP_RECOVER_CONFIRMED;
          startupTickCounter = 0;
          Serial.println("DEBUG: 5 balls confirmed in trough, RECOVER_WAIT -> RECOVER_CONFIRMED");
          break;
        }
      }
    } else {
      // Switch opened - reset stability tracking (ball was rolling over switch)
      ballRecoverySwitchWasStable = false;
    }

    // Timeout check - if balls haven't settled, enter ball search
    if (millis() - ballRecoveryStartMs >= BALL_RECOVERY_TIMEOUT_MS) {
      Serial.println("DEBUG: Ball recovery timeout, entering PHASE_BALL_SEARCH");

      // Leave tray open (ball search will manage it)
      gamePhase = PHASE_BALL_SEARCH;
      ballSearchSubState = BALL_SEARCH_INIT;

      // Enhanced style: play "ball missing" audio
      if (gameStyle == STYLE_ENHANCED) {
        if (ballRecoveryBallInLiftAtStart) {
          // Ball was at lift base - play lift instruction first, then "ball missing"
          pTsunami->trackPlayPoly(TRACK_BALL_IN_LIFT, 0, false);
          audioApplyTrackGain(TRACK_BALL_IN_LIFT, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
          audioCurrentComTrack = TRACK_BALL_IN_LIFT;
          audioComEndMillis = millis() + ((unsigned long)comTracksBallMissing[1].lengthTenths * 100UL);
        } else {
          // No ball at lift - just play "ball missing"
          unsigned int trackNum = comTracksBallMissing[0].trackNum;
          byte trackLength = comTracksBallMissing[0].lengthTenths;
          pTsunami->trackPlayPoly((int)trackNum, 0, false);
          audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
          audioCurrentComTrack = trackNum;
          audioComEndMillis = millis() + ((unsigned long)trackLength * 100UL);
        }
      }
      return;
    }
    break;
  }

  case STARTUP_RECOVER_CONFIRMED:
  {
    // Close ball tray (all styles - balls are in trough now)
    releaseDevice(DEV_IDX_BALL_TRAY_RELEASE);
    gameState.ballTrayOpen = false;

    // Enhanced style: play SCREAM and "Hey Gang" game start fanfare
    // These blocking delays are acceptable here because gameplay hasn't started yet,
    // coils are idle (all balls in trough), and updateDeviceTimers() will run on the
    // next tick after the delays complete.
    if (gameStyle == STYLE_ENHANCED) {
      pTsunami->stopAllTracks();
      pTsunami->trackPlayPoly(TRACK_START_SCREAM, 0, false);
      audioApplyTrackGain(TRACK_START_SCREAM, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
      delay(1500);  // Let scream play
    }

    // Deduct credit
    pMessage->sendMAStoSLVCreditDec();
    gameState.numPlayers = 1;
    gameState.currentPlayer = 1;
    gameState.currentBall = 1;

    pTsunami->stopAllTracks();
    audioCurrentComTrack = 0;
    audioComEndMillis = 0;

    if (gameStyle == STYLE_ENHANCED) {
      pTsunami->trackPlayPoly(TRACK_START_HEY_GANG, 0, false);
      audioApplyTrackGain(TRACK_START_HEY_GANG, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_START_HEY_GANG;
      audioComEndMillis = millis() + 3500UL;

      lcdClear();
      lcdPrintRow(1, "Starting Enhanced");
    }

    // Move to ball dispense
    startupSubState = STARTUP_DISPENSE_BALL;
    startupTickCounter = 0;
    Serial.println("DEBUG: RECOVER_CONFIRMED -> DISPENSE_BALL");
    break;
  }

  // -----------------------------------------------------------------------
  // PHASE 2: BALL DISPENSE - Release one ball to the player
  // -----------------------------------------------------------------------

  case STARTUP_DISPENSE_BALL:
  {
    // Initialize per-ball game state
    gameState.tilted = false;
    gameState.tiltWarnings = 0;

    // Fire ball trough release coil (230ms pulse)
    activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);

    // Record when we dispensed the ball (for timeout tracking)
    startupBallDispenseMs = millis();

    // Move to wait state
    startupSubState = STARTUP_WAIT_FOR_LIFT;
    startupTickCounter = 0;
    Serial.println("DEBUG: DISPENSE_BALL -> WAIT_FOR_LIFT");
    break;
  }

  case STARTUP_WAIT_FOR_LIFT:
  {
    // Check if ball has reached lift
    if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      // Success! Ball is in lift
      startupSubState = STARTUP_COMPLETE;
      startupTickCounter = 0;
      Serial.println("DEBUG: Ball in lift, WAIT_FOR_LIFT -> COMPLETE");
      break;
    }

    // Check for timeout (3 seconds)
    if (millis() - startupBallDispenseMs >= BALL_TROUGH_TO_LIFT_TIMEOUT_MS) {
      // Timeout - ball didn't reach lift
      criticalError("BALL STUCK", "Ball did not reach", "lift in 3 seconds");
      return;
    }
    break;
  }

  case STARTUP_COMPLETE:
  {
    // Reset for potential next ball (ball 2, 3, etc. will re-enter at DISPENSE_BALL)
    startupSubState = STARTUP_RECOVER_INIT;
    startupTickCounter = 0;

    // Initialize ball ready state BEFORE transitioning
    onEnterBallReady();

    // NOW transition to PHASE_BALL_READY
    gamePhase = PHASE_BALL_READY;

    // Update LCD status line
    snprintf(lcdString, LCD_WIDTH + 1, "%s P%d Ball %d",
      gameStyle == STYLE_ENHANCED ? "Enh" : "Org",
      gameState.currentPlayer,
      gameState.currentBall);
    lcdPrintRow(4, lcdString);
    Serial.println("DEBUG: STARTUP_COMPLETE -> PHASE_BALL_READY");
    break;
  }

  }  // end switch
}

// ***** BALL SEARCH PHASE HANDLER *****
// Entered when ball recovery times out. Keeps tray open, periodically fires kickouts,
// and waits for 5-balls-in-trough switch. On timeout, gives up and returns to attract.
void updatePhaseBallSearch() {
  // Check for timeout
  if (millis() - ballTrayOpenStartMs >= BALL_SEARCH_TIMEOUT_MS) {
    // Timeout - give up and return to attract
    releaseDevice(DEV_IDX_BALL_TRAY_RELEASE);
    gameState.ballTrayOpen = false;

    pTsunami->stopAllTracks();
    audioCurrentComTrack = 0;
    audioComEndMillis = 0;

    lcdClear();
    lcdPrintRow(1, "Ball search");
    lcdPrintRow(2, "timeout");
    lcdPrintRow(3, "Returning to");
    lcdPrintRow(4, "attract mode");
    delay(2000);

    if (gameState.numPlayers > 0) {
      saveLastScore(gameState.score[gameState.currentPlayer - 1]);
    }

    lcdClear();
    markAttractDisplayDirty();
    gamePhase = PHASE_ATTRACT;
    gameStyle = STYLE_NONE;
    initGameState();
    return;
  }

  // Handle START button press (Enhanced: replay "ball missing" announcement)
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON]) {
    if (gameStyle == STYLE_ENHANCED) {
      pTsunami->stopAllTracks();
      audioCurrentComTrack = 0;
      audioComEndMillis = 0;

      bool liftHasBall = switchClosed(SWITCH_IDX_BALL_IN_LIFT);

      if (liftHasBall) {
        pTsunami->trackPlayPoly(TRACK_BALL_IN_LIFT, 0, false);
        audioApplyTrackGain(TRACK_BALL_IN_LIFT, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = TRACK_BALL_IN_LIFT;
        audioComEndMillis = millis() + ((unsigned long)comTracksBallMissing[1].lengthTenths * 100UL);
      } else {
        unsigned int trackNum = comTracksBallMissing[0].trackNum;
        byte trackLength = comTracksBallMissing[0].lengthTenths;
        pTsunami->trackPlayPoly((int)trackNum, 0, false);
        audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = trackNum;
        audioComEndMillis = millis() + ((unsigned long)trackLength * 100UL);
      }
    }
    lcdPrintRow(3, "Still searching...");
  }

  // Check if all balls found (with stability check built into allBallsInTrough tick monitoring)
  if (allBallsInTrough()) {
    // Verify stability using the same mechanism as STARTUP_RECOVER_WAIT
    if (!ballRecoverySwitchWasStable) {
      ballRecoveryStableStartMs = millis();
      ballRecoverySwitchWasStable = true;
    } else if (millis() - ballRecoveryStableStartMs >= BALL_DETECTION_STABILITY_MS) {
      // Confirmed! Transition back to startup to finish game start sequence
      Serial.println("DEBUG: Ball search found all balls, -> STARTUP_RECOVER_CONFIRMED");
      pTsunami->stopAllTracks();
      audioCurrentComTrack = 0;
      audioComEndMillis = 0;

      gamePhase = PHASE_STARTUP;
      startupSubState = STARTUP_RECOVER_CONFIRMED;
      startupTickCounter = 0;
      return;
    }
  } else {
    // Switch opened - reset stability
    ballRecoverySwitchWasStable = false;
  }

  // Ball search state machine - simple: just keep ejecting kickouts
  switch (ballSearchSubState) {
  case BALL_SEARCH_INIT:
    activateDevice(DEV_IDX_KICKOUT_LEFT);
    activateDevice(DEV_IDX_KICKOUT_RIGHT);
    ballSearchLastKickoutMs = millis();
    ballSearchSubState = BALL_SEARCH_MONITOR;
    break;

  case BALL_SEARCH_MONITOR:
  {
    unsigned long now = millis();

    // Eject balls from kickouts immediately if detected
    if (switchClosed(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
    }
    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
    }

    // Periodic blind kickout fire (in case ball jammed without tripping switch)
    if (now - ballSearchLastKickoutMs >= BALL_SEARCH_KICKOUT_INTERVAL_MS) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
      ballSearchLastKickoutMs = now;
    }
    break;
  }
  }
}

// ***** BALL READY PHASE HANDLER *****
void updatePhaseBallReady() {
  // ***** ENHANCED STYLE: Wait for ball to settle before monitoring for hill climb *****
  if (gameStyle == STYLE_ENHANCED && !hillClimbStarted) {
    bool ballInLiftNow = switchClosed(SWITCH_IDX_BALL_IN_LIFT);

    // ***** STEP 1: Wait for ball to ARRIVE (rising edge) *****
    if (!ballSettledInLift) {
      // Detect RISING EDGE (switch just closed)
      if (ballInLiftNow && !prevBallInLift) {
        ballSettledInLift = true;
        Serial.println("DEBUG: Ball settled in lift (rising edge), ready for player to push");
      }

      prevBallInLift = ballInLiftNow;
      return;  // Wait for ball to settle
    }

    // ***** STEP 2: Wait for ball to LEAVE (falling edge) *****
    if (ballInLiftNow && !prevBallInLift) {
      // Ball just re-arrived after being removed - NOT the launch we're looking for
      Serial.println("DEBUG: Ball returned to lift after removal");
    }

    if (!ballInLiftNow && prevBallInLift) {
      // FALLING EDGE: Ball just left the lift - START HILL CLIMB
      hillClimbStarted = true;

      Serial.println("DEBUG: Ball lift rod pushed, starting hill climb");

      pTsunami->trackLoop(TRACK_START_LIFT_LOOP, true);
      audioApplyTrackGain(TRACK_START_LIFT_LOOP, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
      pTsunami->trackPlayPoly(TRACK_START_LIFT_LOOP, 0, false);
      audioCurrentSfxTrack = TRACK_START_LIFT_LOOP;

      audioLiftLoopStartMillis = millis();

      analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_HILL_CLIMB);
    }

    prevBallInLift = ballInLiftNow;
  }

  // ***** ALL STYLES: Detect ANY playfield switch to transition to BALL_IN_PLAY *****
  bool wasDrain = false;
  byte hitSwitch = checkPlayfieldSwitchHit(&wasDrain);

  if (hitSwitch != 0xFF) {
    Serial.print("DEBUG: First playfield switch hit (idx ");
    Serial.print(hitSwitch);
    Serial.print("), wasDrain=");
    Serial.println(wasDrain);

    gameState.hasHitSwitch = true;

    // If NOT a drain, handle first point transition (music, shaker, ball save timer, etc.)
    if (!wasDrain) {
      handleFirstPointScored();
    }

    // Transition to BALL_IN_PLAY
    gamePhase = PHASE_BALL_IN_PLAY;
  }
}

// ***** FIRST POINT SCORED HANDLER *****
void handleFirstPointScored() {
  gameState.hasHitSwitch = true;

  // ***** ORIGINAL/IMPULSE: Close ball tray after first point *****
  if (gameStyle == STYLE_ORIGINAL || gameStyle == STYLE_IMPULSE) {
    if (gameState.ballTrayOpen) {
      releaseDevice(DEV_IDX_BALL_TRAY_RELEASE);
      gameState.ballTrayOpen = false;
      Serial.println("DEBUG: Ball tray closed after first point (Original/Impulse)");
    }
  }

  if (gameStyle == STYLE_ENHANCED) {
    // Stop hill climb audio
    pTsunami->trackStop(TRACK_START_LIFT_LOOP);

    // Play first drop screaming audio (11 seconds)
    pTsunami->trackPlayPoly(TRACK_START_DROP, 0, false);
    audioApplyTrackGain(TRACK_START_DROP, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
    audioCurrentSfxTrack = TRACK_START_DROP;

    // Ramp up shaker motor to hill drop power and record start time
    analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_HILL_DROP);
    shakerDropStartMs = millis();

    // Start primary music track
    audioStartPrimaryMusic(primaryMusicTheme,
      (primaryMusicTheme == 0) ? &lastCircusTrackIdx : &lastSurfTrackIdx,
      pTsunami, tsunamiMusicGainDb, tsunamiGainDb);

    // Start ball save timer (TODO: load from EEPROM)
    ballSave.endMs = millis() + 10000UL;  // 10 seconds for testing

    Serial.println("DEBUG: First point scored, scream + music + shaker started, ball save active");
  }
}

// ***** BALL IN PLAY PHASE HANDLER *****
void updatePhaseBallInPlay() {
  // ***** SHAKER MOTOR MANAGEMENT (Enhanced mode only) *****
  if (gameStyle == STYLE_ENHANCED && shakerDropStartMs != 0) {
    unsigned long elapsedMs = millis() - shakerDropStartMs;

    if (elapsedMs >= SHAKER_HILL_DROP_MS) {
      // Stop shaker motor after 11 seconds
      analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
      shakerDropStartMs = 0;
      Serial.println("DEBUG: Shaker motor stopped after 11 seconds");
    }
  }

  // ***** AUDIO DUCKING MANAGEMENT *****
  // If COM track has ended, un-duck music
  if (audioCurrentComTrack != 0 && millis() >= audioComEndMillis) {
    audioCurrentComTrack = 0;
    audioDuckMusicForVoice(false);  // Restore music volume
    Serial.println("DEBUG: Voice line ended, un-ducking music");
  }

  // ***** PLAYFIELD SWITCH DETECTION *****
  bool wasDrain = false;
  byte hitSwitch = checkPlayfieldSwitchHit(&wasDrain);

  if (hitSwitch != 0xFF) {
    if (wasDrain) {
      // Determine drain type from switch index
      byte drainType = DRAIN_TYPE_NONE;
      if (hitSwitch == SWITCH_IDX_DRAIN_LEFT)        drainType = DRAIN_TYPE_LEFT;
      else if (hitSwitch == SWITCH_IDX_DRAIN_CENTER) drainType = DRAIN_TYPE_CENTER;
      else if (hitSwitch == SWITCH_IDX_DRAIN_RIGHT)  drainType = DRAIN_TYPE_RIGHT;
      else if (hitSwitch == SWITCH_IDX_GOBBLE)       drainType = DRAIN_TYPE_GOBBLE;
      handleBallDrain(drainType);
    } else {
      // TODO: Implement full scoring logic here based on hitSwitch
    }
  }
}

// ***** BALL DRAIN HANDLER *****
void handleBallDrain(byte drainType) {
  Serial.print("DEBUG: Ball drained (type ");
  Serial.print(drainType);
  Serial.print("), hasHitSwitch=");
  Serial.println(gameState.hasHitSwitch);

  // ***** ENHANCED MODE: Ball Save Logic *****
  if (gameStyle == STYLE_ENHANCED) {

    // Ball save timer active? (endMs > 0 and not expired)
    if (ballSave.endMs > 0 && millis() < ballSave.endMs) {
      Serial.println("DEBUG: Ball save timer active, saving ball");
      saveBall("timed_save");
      return;
    }
  }

  // ***** NO BALL SAVE: Normal drain processing *****
  Serial.println("DEBUG: Ball lost (no save)");
  processBallLost();
}

// ***** BALL SAVE HANDLER *****
void saveBall(const char* reason) {
  Serial.print("DEBUG: Ball saved - reason: ");
  Serial.println(reason);

  // Stop ALL shaker sources (hill climb AND hill drop)
  stopAllShakers();

  // Stop hill climb audio if still playing
  if (audioCurrentSfxTrack == TRACK_START_LIFT_LOOP) {
    pTsunami->trackStop(TRACK_START_LIFT_LOOP);
    audioCurrentSfxTrack = 0;
  }

  // Duck music before playing voice line
  audioDuckMusicForVoice(true);

  // Play ball save audio
  if (strcmp(reason, "instant_drain") == 0) {
    // Special case: first point was drain
    pTsunami->trackPlayPoly(TRACK_BALL_SAVED_FIRST, 0, false);
    audioApplyTrackGain(TRACK_BALL_SAVED_FIRST, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
    audioCurrentComTrack = TRACK_BALL_SAVED_FIRST;
    audioComEndMillis = millis() + 3000UL;  // Approximate duration
  } else {
    // Normal ball save
    pTsunami->trackPlayPoly(TRACK_BALL_SAVED_FIRST, 0, false);
    audioApplyTrackGain(TRACK_BALL_SAVED_FIRST, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
    audioCurrentComTrack = TRACK_BALL_SAVED_FIRST;
    audioComEndMillis = millis() + 3000UL;  // Approximate duration
  }

  // Start music ONLY if not already playing (prevents double music)
  if (audioCurrentMusicTrack == 0) {
    audioStartPrimaryMusic(primaryMusicTheme,
      (primaryMusicTheme == 0) ? &lastCircusTrackIdx : &lastSurfTrackIdx,
      pTsunami, tsunamiMusicGainDb, tsunamiGainDb);
  }

  // Schedule music un-duck after voice line ends
  // (This would normally be handled by a timer in updatePhaseBallInPlay, but for now just unduck immediately after a short delay)
  // TODO: Add proper audio state machine to handle ducking timeouts

  // Release new ball (if lift is clear)
  if (!switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
    activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);
  } else {
    Serial.println("DEBUG: Ball already in lift, not releasing another");
  }
}

// ***** BALL LOST HANDLER (STUB - TO BE IMPLEMENTED) *****
void processBallLost() {
  // TODO: Implement full ball lost logic (drain audio, shaker, etc.)

  if (extraBallAwarded) {
    // Extra ball: replay same ball number
    extraBallAwarded = false;
    gamePhase = PHASE_STARTUP;
    startupSubState = STARTUP_DISPENSE_BALL;
    startupTickCounter = 0;
    Serial.println("DEBUG: Extra ball - re-entering STARTUP_DISPENSE_BALL");
    return;
  }

  if (gameState.currentBall < MAX_BALLS) {
    // More balls remain: advance to next ball
    gameState.currentBall++;
    gamePhase = PHASE_STARTUP;
    startupSubState = STARTUP_DISPENSE_BALL;
    startupTickCounter = 0;
    Serial.print("DEBUG: Ball lost, advancing to ball ");
    Serial.println(gameState.currentBall);
    return;
  }

  // Last ball - game over
  Serial.println("DEBUG: Last ball lost, game over");
  saveLastScore(gameState.score[gameState.currentPlayer - 1]);

  // TODO: Play game over audio, reset lamps, etc.

  gamePhase = PHASE_ATTRACT;
  gameStyle = STYLE_NONE;
  setAttractLamps();
  markAttractDisplayDirty();
  initGameState();
}

// ********************************************************************************************************************************
// ************************************************* DEVICE & COIL CONTROL ********************************************************
// ********************************************************************************************************************************

// ***** DEVICE ACTIVATION SAFETY CHECK *****
bool canActivateDeviceNow(byte t_devIdx) {
  if (gameState.tilted) {
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

// ***** DEVICE ACTIVATION REQUEST *****
void activateDevice(byte t_devIdx) {
  if (t_devIdx >= NUM_DEVS_MASTER) {
    sprintf(lcdString, "DEV ERR %u", t_devIdx);
    criticalError("CRITICAL ERROR", "activateDevice", lcdString);

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

// ***** DEVICE TIMER UPDATE WITH WATCHDOG *****
void updateDeviceTimers() {
  // Increment watchdog scan counter
  coilWatchdogScanCounter++;

  for (byte i = 0; i < NUM_DEVS_MASTER; i++) {
    // ***** WATCHDOG CHECK 1: Countdown corruption detection *****
    // For non-holdable coils, countdown should never exceed timeOn + margin
    if (deviceParm[i].powerHold == 0 && deviceParm[i].timeOn > 0) {
      if (deviceParm[i].countdown > (int8_t)(deviceParm[i].timeOn + COIL_WATCHDOG_MARGIN_TICKS)) {
        // Countdown is impossibly high - force off
        analogWrite(deviceParm[i].pinNum, 0);
        deviceParm[i].countdown = 0;
        deviceParm[i].queueCount = 0;
        sprintf(lcdString, "Coil %u", i);
        criticalError("CRITICAL ERROR", "Countdown 1", lcdString);
        continue;
      }
    }

    // Normal countdown processing
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

  // ***** WATCHDOG CHECK 2: Periodic full PWM scan (every 1 second) *****
  if (coilWatchdogScanCounter >= COIL_WATCHDOG_SCAN_INTERVAL_TICKS) {
    coilWatchdogScanCounter = 0;
    coilWatchdogFullScan();
  }
}

// ***** COIL WATCHDOG FULL SCAN *****
// Verifies no coil is stuck on without an active timer
void coilWatchdogFullScan() {
  for (byte i = 0; i < NUM_DEVS_MASTER; i++) {
    // Skip holdable coils and motors
    if (i == DEV_IDX_FLIPPER_LEFT || i == DEV_IDX_FLIPPER_RIGHT ||
      i == DEV_IDX_BALL_TRAY_RELEASE || i == DEV_IDX_MOTOR_SHAKER ||
      i == DEV_IDX_MOTOR_SCORE) {
      continue;
    }

    // For non-holdable coils: if countdown is 0 or negative (idle/resting),
    // the coil should NOT be outputting power
    if (deviceParm[i].countdown <= 0) {
      // We can't easily read PWM state, but we can force it off as a safety measure
      // This is a belt-and-suspenders approach
      analogWrite(deviceParm[i].pinNum, 0);
    }
  }
}

// ***** DEVICE RELEASE *****
void releaseDevice(byte t_devIdx) {
  if (t_devIdx >= NUM_DEVS_MASTER) {
    return;
  }
  analogWrite(deviceParm[t_devIdx].pinNum, 0);
  deviceParm[t_devIdx].countdown = 0;
  deviceParm[t_devIdx].queueCount = 0;
}

// ***** STOP ALL SHAKER SOURCES *****
void stopAllShakers() {
  // Stop hill climb shaker (controlled by hillClimbStarted flag)
  if (hillClimbStarted) {
    analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
    hillClimbStarted = false;
  }

  // Stop hill drop shaker (controlled by shakerDropStartMs timer)
  if (shakerDropStartMs != 0) {
    analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
    shakerDropStartMs = 0;
  }

  // Stop any other shaker source (if motor is running but flags are inconsistent)
  analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
}

// ********************************************************************************************************************************
// ***************************************************** LAMP CONTROL *************************************************************
// ********************************************************************************************************************************

void setGILamps(bool t_on) {
  for (int i = 0; i < NUM_LAMPS_MASTER; i++) {
    if (lampParm[i].groupNum == LAMP_GROUP_GI) {
      if (t_on) {
        pShiftRegister->digitalWrite(lampParm[i].pinNum, LOW);
      } else {
        pShiftRegister->digitalWrite(lampParm[i].pinNum, HIGH);
      }
    }
  }
}

void setAttractLamps() {
  setGILamps(true);
  pMessage->sendMAStoSLVGILamp(true);

  // Turn on all 7 bumper lamps (S-C-R-E-A-M-O)
  for (byte i = LAMP_IDX_S; i <= LAMP_IDX_O; i++) {
    pShiftRegister->digitalWrite(lampParm[i].pinNum, LOW);
  }

  // Turn off everything else on Master
  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    if (lampParm[i].groupNum != LAMP_GROUP_GI &&
      lampParm[i].groupNum != LAMP_GROUP_BUMPER) {
      pShiftRegister->digitalWrite(lampParm[i].pinNum, HIGH);
    }
  }

  // Turn off Slave score and tilt lamps
  pMessage->sendMAStoSLVScoreAbs(0);
  pMessage->sendMAStoSLVTiltLamp(false);
}

// ********************************************************************************************************************************
// **************************************************** SWITCH HELPERS ************************************************************
// ********************************************************************************************************************************

bool switchClosed(byte t_switchIdx) {
  if (t_switchIdx >= NUM_SWITCHES_MASTER) {
    return false;
  }
  byte pin = switchParm[t_switchIdx].pinNum;
  pin = pin - 64;
  byte portIndex = pin / 16;
  byte bitNum = pin % 16;
  return ((switchNewState[portIndex] & (1 << bitNum)) == 0);
}

bool switchClosedImmediate(byte t_switchIdx) {
  if (t_switchIdx >= NUM_SWITCHES_MASTER || pShiftRegister == nullptr) {
    return false;
  }
  return pShiftRegister->digitalRead(switchParm[t_switchIdx].pinNum) == LOW;
}

void resetDiagButtonStates() {
  for (byte i = 0; i < NUM_DIAG_BUTTONS; i++) {
    bool closedNow = switchClosed((byte)(SWITCH_IDX_DIAG_1 + i));
    diagButtonLastState[i] = closedNow ? 1 : 0;
    diagButtonDebounce[i] = DIAG_DEBOUNCE_TICKS;  // Suppress false edges after reset
  }
}

bool diagButtonPressed(byte t_diagIdx) {
  if (t_diagIdx >= NUM_DIAG_BUTTONS) {
    return false;
  }
  // If debounce timer is active, count down and suppress any edge
  if (diagButtonDebounce[t_diagIdx] > 0) {
    diagButtonDebounce[t_diagIdx]--;
    // Still track the current state so we don't get a false edge when debounce expires
    bool closedNow = switchClosed((byte)(SWITCH_IDX_DIAG_1 + t_diagIdx));
    diagButtonLastState[t_diagIdx] = closedNow ? 1 : 0;
    return false;
  }
  byte switchIdx = (byte)(SWITCH_IDX_DIAG_1 + t_diagIdx);
  bool closedNow = switchClosed(switchIdx);
  bool pressed = (closedNow && (diagButtonLastState[t_diagIdx] == 0));
  diagButtonLastState[t_diagIdx] = closedNow ? 1 : 0;
  if (pressed) {
    diagButtonDebounce[t_diagIdx] = DIAG_DEBOUNCE_TICKS;  // Start debounce after detected press
  }
  return pressed;
}

// ********************************************************************************************************************************
// ********************************************** PLAYFIELD SWITCH DETECTION ******************************************************
// ********************************************************************************************************************************

// Checks all playfield switches (scoring targets, non-scoring side targets, drains, gobble hole)
// for a just-closed edge this tick. Returns the switch index that fired (SWITCH_IDX_*),
// or 0xFF if no playfield switch was hit this tick.
// Also sets *pIsDrain to true if the hit was a drain rollover or gobble hole.
// This covers EVERY switch on the playfield that indicates ball contact:
//   - 7 bumpers (S,C,R,E,A,M,O)
//   - 2 slingshots
//   - 4 left side targets (2-5)
//   - 5 right side targets (1-5)
//   - 4 hat rollovers
//   - 2 kickouts
//   - 1 gobble hole
//   - 3 drain rollovers (left, center, right)
// It does NOT include cabinet switches (start, coin, diag, knockoff, tilt bob)
// or ball management switches (5-balls-in-trough, ball-in-lift).
byte checkPlayfieldSwitchHit(bool* pIsDrain) {
  *pIsDrain = false;

  // Drain rollovers and gobble hole (check first so we can flag drains)
  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_LEFT]) { *pIsDrain = true; return SWITCH_IDX_DRAIN_LEFT; }
  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_CENTER]) { *pIsDrain = true; return SWITCH_IDX_DRAIN_CENTER; }
  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_RIGHT]) { *pIsDrain = true; return SWITCH_IDX_DRAIN_RIGHT; }
  if (switchJustClosedFlag[SWITCH_IDX_GOBBLE]) { *pIsDrain = true; return SWITCH_IDX_GOBBLE; }

  // Bumpers
  for (byte i = SWITCH_IDX_BUMPER_S; i <= SWITCH_IDX_BUMPER_O; i++) {
    if (switchJustClosedFlag[i]) return i;
  }

  // Slingshots
  if (switchJustClosedFlag[SWITCH_IDX_SLINGSHOT_LEFT])  return SWITCH_IDX_SLINGSHOT_LEFT;
  if (switchJustClosedFlag[SWITCH_IDX_SLINGSHOT_RIGHT]) return SWITCH_IDX_SLINGSHOT_RIGHT;

  // Left side targets (2-5)
  for (byte i = SWITCH_IDX_LEFT_SIDE_TARGET_2; i <= SWITCH_IDX_LEFT_SIDE_TARGET_5; i++) {
    if (switchJustClosedFlag[i]) return i;
  }

  // Right side targets (1-5)
  for (byte i = SWITCH_IDX_RIGHT_SIDE_TARGET_1; i <= SWITCH_IDX_RIGHT_SIDE_TARGET_5; i++) {
    if (switchJustClosedFlag[i]) return i;
  }

  // Hat rollovers
  if (switchJustClosedFlag[SWITCH_IDX_HAT_LEFT_TOP])     return SWITCH_IDX_HAT_LEFT_TOP;
  if (switchJustClosedFlag[SWITCH_IDX_HAT_LEFT_BOTTOM])  return SWITCH_IDX_HAT_LEFT_BOTTOM;
  if (switchJustClosedFlag[SWITCH_IDX_HAT_RIGHT_TOP])    return SWITCH_IDX_HAT_RIGHT_TOP;
  if (switchJustClosedFlag[SWITCH_IDX_HAT_RIGHT_BOTTOM]) return SWITCH_IDX_HAT_RIGHT_BOTTOM;

  // Kickouts
  if (switchJustClosedFlag[SWITCH_IDX_KICKOUT_LEFT])  return SWITCH_IDX_KICKOUT_LEFT;
  if (switchJustClosedFlag[SWITCH_IDX_KICKOUT_RIGHT]) return SWITCH_IDX_KICKOUT_RIGHT;

  return 0xFF;  // No playfield switch hit this tick
}

// ********************************************************************************************************************************
// **************************************************** DISPLAY HELPERS ***********************************************************
// ********************************************************************************************************************************

void lcdClear() {
  if (pLCD2004 != nullptr) {
    pLCD2004->clearScreen();
  }
}

void lcdClearRow(byte row) {
  if (pLCD2004 != nullptr) {
    pLCD2004->clearRow(row);
  }
}

void lcdPrintRow(byte t_row, const char* t_text) {
  if (pLCD2004 == nullptr || t_row < 1 || t_row > 4) {
    return;
  }
  char buf[LCD_WIDTH + 1];
  memset(buf, ' ', LCD_WIDTH);
  buf[LCD_WIDTH] = '\0';
  byte len = strlen(t_text);
  if (len > LCD_WIDTH) len = LCD_WIDTH;
  memcpy(buf, t_text, len);
  pLCD2004->printRowCol(t_row, 1, buf);
}

void lcdPrintAt(byte t_row, byte t_col, const char* t_text) {
  if (pLCD2004 == nullptr) {
    return;
  }
  pLCD2004->printRowCol(t_row, t_col, t_text);
}

void lcdShowDiagScreen(const char* line1, const char* line2, const char* line3, const char* line4) {
  lcdPrintRow(1, line1);
  lcdPrintRow(2, line2);
  lcdPrintRow(3, line3);
  lcdPrintRow(4, line4);
}

void markAttractDisplayDirty() {
  attractDisplayDirty = true;
}

void renderAttractDisplayIfNeeded() {
  if (pLCD2004 == nullptr) {
    return;
  }

  if (!initialBootDisplayShown) {
    if (millis() - bootDisplayStartMs < BOOT_DISPLAY_DURATION_MS) {
      return;
    }
    initialBootDisplayShown = true;
    attractDisplayDirty = true;
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
  lcdShowDiagScreen(
    "DIAGNOSTICS",
    diagSuiteNames[diagnosticSuiteIdx],
    "-/+ to scroll",
    "SEL=run  BACK=exit"
  );
}

// ********************************************************************************************************************************
// **************************************************** AUDIO HELPERS *************************************************************
// ********************************************************************************************************************************

void audioResetTsunamiState() {
  if (pTsunami == nullptr) {
    return;
  }
  for (int trk = 1; trk <= 256; trk++) {
    pTsunami->trackStop(trk);
  }
  audioLoadMasterGain(&tsunamiGainDb);
  audioLoadCategoryGains(&tsunamiVoiceGainDb, &tsunamiSfxGainDb, &tsunamiMusicGainDb);
  audioLoadDucking(&tsunamiDuckingDb);
  audioApplyMasterGain(tsunamiGainDb, pTsunami);
  audioCurrentComTrack   = 0;
  audioCurrentSfxTrack   = 0;
  audioCurrentMusicTrack = 0;
}

void audioAdjustMasterGainQuiet(int8_t t_deltaDb) {
  int newVal = (int)tsunamiGainDb + (int)t_deltaDb;
  if (newVal < TSUNAMI_GAIN_DB_MIN) newVal = TSUNAMI_GAIN_DB_MIN;
  if (newVal > TSUNAMI_GAIN_DB_MAX) newVal = TSUNAMI_GAIN_DB_MAX;
  if ((int8_t)newVal == tsunamiGainDb) return;
  tsunamiGainDb = (int8_t)newVal;
  audioApplyMasterGain(tsunamiGainDb, pTsunami);
  audioSaveMasterGain(tsunamiGainDb);
  sprintf(lcdString, "Vol %3d dB", (int)tsunamiGainDb);
  lcdPrintRow(4, lcdString);
}

bool audioMusicHasEnded() {
  if (audioCurrentMusicTrack == 0) {
    return true;
  }
  unsigned long now = millis();
  return (now >= audioMusicEndMillis);
}

bool audioStartMusicTrack(unsigned int t_trackNum, byte t_lengthSeconds, bool t_loop) {
  if (pTsunami == nullptr || t_trackNum == 0) {
    return false;
  }
  if (audioCurrentMusicTrack == t_trackNum && !audioMusicHasEnded()) {
    return false;
  }
  if (audioCurrentMusicTrack != 0) {
    pTsunami->trackStop((int)audioCurrentMusicTrack);
  }
  audioCurrentMusicTrack = t_trackNum;
  audioMusicEndMillis = millis() + ((unsigned long)t_lengthSeconds * 1000UL);
  if (t_loop) {
    pTsunami->trackLoop((int)t_trackNum, true);
    audioMusicEndMillis = millis() + 3600000UL;
  }
  pTsunami->trackPlayPoly((int)t_trackNum, 0, false);
  audioApplyTrackGain(t_trackNum, tsunamiMusicGainDb, tsunamiGainDb, pTsunami);
  return true;
}

void audioStopMusic() {
  if (pTsunami != nullptr && audioCurrentMusicTrack != 0) {
    pTsunami->trackStop((int)audioCurrentMusicTrack);
  }
  audioCurrentMusicTrack = 0;
  audioMusicEndMillis = 0;
}

void audioDuckMusicForVoice(bool duck) {
  if (pTsunami == nullptr || audioCurrentMusicTrack == 0) {
    return;
  }

  if (duck) {
    // Apply ducking offset (typically -20dB)
    int duckGain = tsunamiMusicGainDb + tsunamiDuckingDb;
    if (duckGain < TSUNAMI_GAIN_DB_MIN) duckGain = TSUNAMI_GAIN_DB_MIN;
    pTsunami->trackGain(audioCurrentMusicTrack, duckGain);
  } else {
    // Restore normal music gain
    pTsunami->trackGain(audioCurrentMusicTrack, tsunamiMusicGainDb + tsunamiGainDb);
  }
}

// ********************************************************************************************************************************
// ************************************************ GAME STATE MANAGEMENT *********************************************************
// ********************************************************************************************************************************

void initGameState() {
  gameState.numPlayers = 0;
  gameState.currentPlayer = 0;
  gameState.currentBall = 0;
  for (byte i = 0; i < MAX_PLAYERS; i++) {
    gameState.score[i] = 0;
  }
  gameState.tiltWarnings = 0;
  gameState.tilted = false;
  gameState.hasHitSwitch = false;
  gameState.ballTrayOpen = false;

  // Ball tracking
  ballsInPlay = 0;
  ballInLift = false;
  ballsLocked = 0;

  // Multiball / Extra Ball
  inMultiball = false;
  extraBallAwarded = false;

  // Mode state
  modeState.active = false;
  modeState.currentMode = MODE_NONE;
  modeState.modeCount = 0;
  modeState.timerStartMs = 0;
  modeState.timerDurationMs = 0;
  modeState.timerPaused = false;
  modeState.pausedTimeRemainingMs = 0;
  modeState.reminderPlayed = false;
  modeState.hurryUpPlayed = false;
  modeState.modeProgress = 0;

  // Ball save
  ballSave.endMs = 0;

  // Kickout retry
  kickoutLeftRetryTicks = 0;
  kickoutRightRetryTicks = 0;

  // EM emulation state
  selectionUnitPosition = (byte)random(0, 50);  // Random starting position per Overview Section 13.3.3
  gobbleCount = 0;
  bumperLitMask = 0x7F;  // All 7 bumpers lit
  whiteLitMask = 0;      // No WHITE inserts lit
}

void onEnterBallReady() {
  Serial.println("DEBUG: Entering BALL_READY phase");

  // Stop any running shakers
  stopAllShakers();

  // Ball tracking: reset per-ball state. See Overview Section 10.3.1.
  ballsInPlay = 0;
  ballInLift = true;  // Ball was just dispensed to lift by STARTUP_COMPLETE
  // ballsLocked is NOT reset -- locked balls persist across ball numbers

  // Ball save: reset for new ball number. Timer is the only gate (no ballSaveUsed flag).
  ballSave.endMs = 0;

  // Multiball ends if we got here (ballsInPlay == 0 means multiball already ended)
  inMultiball = false;

  // Reset hill climb tracking
  hillClimbStarted = false;

  // Reset ball settled tracking WITH EDGE DETECTION
  // If ball is already in lift, we need to wait for it to be removed and returned
  prevBallInLift = switchClosed(SWITCH_IDX_BALL_IN_LIFT);

  if (prevBallInLift) {
    // Ball already in lift - wait for player to remove it first
    ballSettledInLift = false;
    Serial.println("DEBUG: Ball already in lift, waiting for player to remove");
  } else {
    // No ball in lift - wait for it to arrive
    ballSettledInLift = false;
    Serial.println("DEBUG: Waiting for ball to arrive in lift");
  }

  // Reset first point tracking
  gameState.hasHitSwitch = false;
  gameState.tiltWarnings = 0;
  gameState.tilted = false;
}

bool isInMode() {
  return modeState.active;
}

// ********************************************************************************************************************************
// *************************************************** SCORING & RS485 ************************************************************
// ********************************************************************************************************************************

void sendScoreIncrement(int amount) {
  // Determine silence flags based on game style and mode state
  bool silentBells = false;
  bool silent10KUnit = false;
  if (gameStyle == STYLE_ENHANCED) {
    // Enhanced style: 10K Unit always silent, bells silent only during modes
    silent10KUnit = true;
    silentBells = isInMode();  // Silent bells during Bumper Cars, Roll-A-Ball, Gobble Hole
  }
  // Original and Impulse styles: both flags remain false (full EM sounds)
  pMessage->sendMAStoSLVScoreInc10K(amount, silentBells, silent10KUnit);
}

void sendScoreReset() {
  // Determine silence flags for score reset based on game style
  // Score reset happens AFTER game style is known, at game start
  bool silentBells = false;
  bool silent10KUnit = false;

  if (gameStyle == STYLE_ENHANCED) {
    // Enhanced: 10K Unit always silent, bells ENABLED during reset (not in a mode yet)
    silent10KUnit = true;
    silentBells = false;  // Bells are allowed during reset (authentic EM feel at game start)
  }
  // Original and Impulse: both flags false (full EM sounds)

  pMessage->sendMAStoSLVScoreReset(silentBells, silent10KUnit);
}

bool useMechanicalScoringDevices() {
  // Returns true if Score Motor, Selection Unit, and Relay Reset Bank should be used.
  // These are only used in Original and Impulse styles, never in Enhanced.
  return (gameStyle != STYLE_ENHANCED);
}

unsigned int calculateScoreMotorQuarterRevs(int deltaScore) {
  // Usage:
  // byte quarterRevs = calculateScoreMotorQuarterRevs(delta);
  // if (quarterRevs > 0) {
  //   activateDevice(DEV_IDX_MOTOR_SCORE);
  //   scoreMotorOffTicks = quarterRevs * 88;  // 88 ticks = 880ms approx. 882ms
  // }
  // Single 10K or 100K: no motor
  if (deltaScore == 1 || deltaScore == 10) return 0;

  // 2-5 units (20K-50K or 200K-500K): 1 quarter-rev
  int units = (deltaScore >= 10) ? (deltaScore / 10) : deltaScore;
  if (units <= 5) return 1;

  // 6-10 units: 2 quarter-revs
  return 2;
}

// ********************************************************************************************************************************
// ************************************************** SCORE PERSISTENCE ***********************************************************
// ********************************************************************************************************************************

void loadLastScore() {
  // Load 2-byte score from EEPROM (little-endian)
  byte low = EEPROM.read(EEPROM_ADDR_LAST_SCORE);
  byte high = EEPROM.read(EEPROM_ADDR_LAST_SCORE + 1);
  int score = (high << 8) | low;

  // Validate range (0..999)
  if (score < 0 || score > 999) {
    score = 0;
  }
  lastDisplayedScore = score;
}

void saveLastScore(int score) {
  // Clamp to valid range
  if (score < 0) score = 0;
  if (score > 999) score = 999;

  // Only write if changed (reduce EEPROM wear)
  byte low = score & 0xFF;
  byte high = (score >> 8) & 0xFF;

  if (EEPROM.read(EEPROM_ADDR_LAST_SCORE) != low) {
    EEPROM.write(EEPROM_ADDR_LAST_SCORE, low);
  }
  if (EEPROM.read(EEPROM_ADDR_LAST_SCORE + 1) != high) {
    EEPROM.write(EEPROM_ADDR_LAST_SCORE + 1, high);
  }

  lastDisplayedScore = score;
}

void saveCurrentScoreAndEnterAttract() {
  // Save the current player's score for display at next power-up
  if (gameState.numPlayers > 0 && gameState.currentPlayer > 0) {
    saveLastScore(gameState.score[gameState.currentPlayer - 1]);
  }

  // Reset to attract mode
  gamePhase = PHASE_ATTRACT;
  gameStyle = STYLE_NONE;
  setAttractLamps();
  markAttractDisplayDirty();
}

// ********************************************************************************************************************************
// ************************************************** SYSTEM UTILITIES ************************************************************
// ********************************************************************************************************************************

bool allBallsInTrough() {
  // Check if "5 Balls in Trough" switch is closed
  return switchClosed(SWITCH_IDX_5_BALLS_IN_TROUGH);
}

bool queryCreditStatus(bool* creditsAvailable) {
  pMessage->sendMAStoSLVCreditStatusQuery();
  unsigned long queryStart = millis();
  while (millis() - queryStart < 100) {
    byte msgType = pMessage->available();
    if (msgType == RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS) {
      pMessage->getSLVtoMASCreditStatus(creditsAvailable);
      return true;
    }
    delay(1);
  }
  criticalError("RS485 ERROR", "No C.Q. Response", "Check Connection");
  return false;
}

void setAllDevicesOff() {
  for (int i = 0; i < NUM_DEVS_MASTER; i++) {
    digitalWrite(deviceParm[i].pinNum, LOW);
    pinMode(deviceParm[i].pinNum, OUTPUT);
  }
  centipedeForceAllOff();
}

void setPWMFrequencies() {
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;
  TCCR4B = (TCCR4B & 0b11111000) | 0x01;
  TCCR5B = (TCCR5B & 0b11111000) | 0x01;
}

void clearAllPWM() {
  for (int i = 0; i < NUM_DEVS_MASTER; i++) {
    analogWrite(deviceParm[i].pinNum, 0);
  }
}

void softwareReset() {
  setAllDevicesOff();
  clearAllPWM();
  cli();
  wdt_enable(WDTO_15MS);
  while (1) { }
}

void criticalError(const char* line1, const char* line2, const char* line3) {
  lcdClear();
  lcdPrintRow(1, line1);
  lcdPrintRow(2, line2);
  lcdPrintRow(3, line3);
  lcdPrintRow(4, "HOLD KNOCKOFF=RST");
  Serial.println("CRITICAL ERROR:");
  Serial.println(line1);
  Serial.println(line2);
  Serial.println(line3);

  if (pTsunami != nullptr) {
    pTsunami->trackPlayPoly(TRACK_TILT_BUZZER, 0, false);
    delay(1500);
    pTsunami->trackPlayPoly(TRACK_DIAG_CRITICAL_ERROR, 0, false);
  }

  unsigned long knockoffStart = 0;
  bool knockoffWasPressed = false;

  while (true) {
    bool knockoffPressed = switchClosedImmediate(SWITCH_IDX_KNOCK_OFF);

    if (knockoffPressed) {
      if (!knockoffWasPressed) {
        knockoffStart = millis();
        knockoffWasPressed = true;
      } else if (millis() - knockoffStart >= KNOCKOFF_RESET_HOLD_MS) {
        lcdPrintRow(4, "RESETTING NOW...");
        delay(500);
        softwareReset();
      }
    } else {
      knockoffWasPressed = false;
    }

    delay(10);
  }
}