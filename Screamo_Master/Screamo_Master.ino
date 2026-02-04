// Screamo_Master.INO Rev: 02/04/26
// 11/12/25: Moved Right Kickout from pin 13 to pin 44 to avoid MOSFET firing twice on power-up.  Don't use MOSFET on pin 13.
// 12/28/25: Changed flipper inputs from direct Arduino inputs to Centipede inputs.
// 01/07/26: Added "5 Balls in Trough" switch to Centipede inputs. All switches tested and working.

#include <Arduino.h>
#include <EEPROM.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
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
struct LampParmStruct {
  byte pinNum;       // Centipede pin number for this lamp (0..63)
  byte groupNum;     // Group number for this lamp
  bool stateOn;      // true = lamp ON; false = lamp OFF
};

const bool LAMP_ON  = true;
const bool LAMP_OFF = false;

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

// ***** SWITCH DEFINITIONS *****
// Centipede #1 (0..63) is LAMP OUTPUTS
// Centipede #2 (64..127) is SWITCH INPUTS
struct SwitchParmStruct {
  byte pinNum;        // Centipede #2 pin number for this switch (64..127)
  byte loopConst;     // Number of 10ms loops to confirm stable state change
  byte loopCount;     // Current count of 10ms loops with stable state (initialized to zero)
};

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

// ***** COIL AND MOTOR DEFINITIONS *****
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

const int8_t COIL_REST_TICKS           =  -8;  // Coil rest period: 8 ticks * 10ms = 80ms
const uint8_t IMPULSE_FLIPPER_UP_TICKS =  10;  // Hold impulse flippers up for 100ms (10 ticks) before releasing
const unsigned int KICKOUT_RETRY_TICKS = 100;  // 1 second (1000ms, 100 ticks) between retry attempts for stuck kickout ball
byte kickoutLeftRetryTicks             =   0;  // Countdown to next Left retry attempt
byte kickoutRightRetryTicks            =   0;  // Countdown to next Right retry attempt

const byte COIL_WATCHDOG_MARGIN_TICKS = 5;           // 50ms grace period for countdown corruption detection
const byte COIL_WATCHDOG_SCAN_INTERVAL_TICKS = 100;  // 1 second full PWM scan
byte coilWatchdogScanCounter = 0;                    // Counter for periodic full scan

// ********************************************************************************************************************************
// ***************************************************** GAME STATE ***************************************************************
// ********************************************************************************************************************************

const byte MAX_PLAYERS                       =    1;  // Currently only support single player, even for Enhanced
const byte MAX_BALLS                         =    5;
const byte MAX_MODES_PER_GAME                =    3;  // Maximum modes that can be played per game

byte gamePhase = PHASE_ATTRACT;

// ***** STARTUP SUB-STATE CONSTANTS *****
const byte STARTUP_INIT               = 0;
const byte STARTUP_EJECT_KICKOUTS     = 1;
const byte STARTUP_OPEN_TRAY          = 2;
const byte STARTUP_WAIT_FOR_BALLS     = 3;
const byte STARTUP_ANNOUNCE_MISSING   = 4;  // Enhanced only
const byte STARTUP_SCORE_MOTOR_SLOT1  = 5;  // Original/Impulse only
const byte STARTUP_SCORE_MOTOR_SLOT2  = 6;
const byte STARTUP_SCORE_MOTOR_SLOT3  = 7;
const byte STARTUP_SCORE_MOTOR_SLOT4  = 8;
const byte STARTUP_SCORE_MOTOR_SLOT5  = 9;
const byte STARTUP_SCORE_MOTOR_SLOT6  = 10;
const byte STARTUP_SCORE_RESET_WAIT   = 11;
const byte STARTUP_DISPENSE_BALL      = 12;
const byte STARTUP_WAIT_FOR_LIFT      = 13;
const byte STARTUP_COMPLETE           = 14;

byte startupSubState = STARTUP_INIT;
unsigned int startupTickCounter = 0;
unsigned int startupTimeoutTicks = 0;

// ***** STARTUP PHASE TIMEOUTS *****
const unsigned long BALL_TROUGH_TO_LIFT_TIMEOUT_MS = 3000;  // 3 seconds max for ball to reach lift from trough release
unsigned long startupBallDispenseMs = 0;  // When ball was dispensed (for timeout tracking)
const unsigned long GAME_TIMEOUT_MS     = 120000;  // 2 minutes default inactivity timeout ends game

// ***** BALL LIFT TRACKING *****
bool prevBallInLift = false;  // Previous state of ball-in-lift switch (for edge detection)

// ***** GAME STATE STRUCTURE *****
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
  byte ballsToDispense;      // Original/Impulse: 4 or 5
  byte ballsInTray;          // Enhanced: balls in Ball Tray
  byte ballsInTrough;        // Enhanced: balls in Ball Trough
  byte ballsLocked;          // Enhanced: balls locked in kickouts
  bool ballTrayOpen;         // Is Ball Tray currently open?
};
GameStateStruct gameState;

// ***** GAME-WITHIN-A-GAME MODE STRUCTURE *****
struct ModeStateStruct {
  bool active;
  byte currentMode;  // None, Bumper Cars, Roll-A-Ball, Gobble Hole
  unsigned long timerStartMs;
  unsigned long timerDurationMs;
  bool timerPaused;
  unsigned long pausedTimeRemainingMs;
  bool reminderPlayed;
  bool hurryUpPlayed;
  byte modeProgress;  // For BUMPER_CARS: next expected letter (0-6)
                      // For GOBBLE_HOLE: gobbles hit (0-5)
};
ModeStateStruct modeState;

// ***** MULTIBALL STATE *****
struct MultiballStateStruct {
  bool active;
  unsigned long ballSaveEndMs;
};
MultiballStateStruct multiballState;

// ***** BALL SAVE STATE *****
struct BallSaveState {
  bool active;              // Ball save timer active
  unsigned long endMs;      // When ball save expires
};

BallSaveState ballSave;
const byte BALL_SAVE_DEFAULT_SECONDS = 15;

// ***** MODE STATE TRACKING *****
unsigned long modeEndedAtMs = 0;  // Timestamp when mode ends, for mode ball drain grace period; else set to 0.
unsigned long modeTimerEndMs = 0;     // When mode timer expires
bool modeTimerPaused = false;
unsigned long modeTimerPausedAt = 0;  // When we paused it

// ***** START BUTTON DETECTION *****
const unsigned long START_STYLE_DETECT_WINDOW_MS = 500;  // 500ms to detect double-tap for Enhanced
unsigned long startFirstPressMs = 0;
byte startTapCount = 0;
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
unsigned long ballDetectionStableStartMs = 0;  // When switch first closed (for stability tracking)

byte gameStyle = STYLE_NONE;  // Selected style (set by start button taps)
bool playerAdditionWindowOpen = false;  // True from game start until first point scored (Original: for Impulse switch detection ONLY)
bool hillClimbStarted = false;  // True once hill climb audio/shaker has started (to prevent re-triggering)
bool ballSettledInLift = false;  // True once ball is confirmed resting at lift base (prevents premature hill climb trigger)

// ***** SHAKER MOTOR POWER LEVELS *****
const byte SHAKER_POWER_HILL_CLIMB = 80;   // Low rumble during hill climb
const byte SHAKER_POWER_HILL_DROP = 110;   // Medium intensity during first drop
const byte SHAKER_POWER_MIN = 70;          // Minimum power to start motor (below this won't run)
const byte SHAKER_POWER_MAX = 140;         // Maximum safe power (above this triggers hat switches)

// ***** HILL CLIMB/DROP TIMING *****
const unsigned long HILL_CLIMB_TIMEOUT_MS = 30000;  // 30 seconds before "shoot the ball" reminder
const unsigned long HILL_DROP_DURATION_MS = 11000;  // 11 seconds (matches track 0404 length)

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

// ***** MISC CONSTS *****
const unsigned long MODE_END_GRACE_PERIOD_MS = 5000;  // 5 seconds drain after mode ends, you still get the ball back
const byte BALL_DRAIN_INSULT_FREQUENCY = 50;  // 0..100 indicates percent of the time we make a random snarky comment when the ball drains
const unsigned long GAME_TIMEOUT_SECONDS = 300;  // 5 minutes of no activity and game reverts to Attract mode
const unsigned int STARTUP_BALL_TROUGH_TO_LIFT_TIMEOUT_MS = 3000;  // > 3 seconds and we throw a critical error
const unsigned SCORE_MOTOR_CYCLE_MS = 882;                 // one 1/4 revolution of score motor (ms)
const int EEPROM_SAVE_INTERVAL_MS             = 10000;  // Save interval for EEPROM (milliseconds)
int lastDisplayedScore = 0;  // Score to display at power-up (loaded from EEPROM)
unsigned long lastEepromSaveMs = 0;  // When we last saved to EEPROM

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
byte diagnosticState = 0;
const byte NUM_DIAG_BUTTONS = 4;
byte diagButtonLastState[NUM_DIAG_BUTTONS] = { 0, 0, 0, 0 };
bool attractDisplayDirty = true;
bool diagDisplayDirty = true;
byte diagLastRenderedSuite = 0xFF;

// Forward declaration for function with default parameter
void markDiagnosticsDisplayDirty(bool forceSuiteReset = false);

// ********************************************************************************************************************************
// ***************************************************** DISPLAY STATE ************************************************************
// ********************************************************************************************************************************

bool initialBootDisplayShown = false;
unsigned long bootDisplayStartMs = 0;
const unsigned long BOOT_DISPLAY_DURATION_MS = 3000;  // Show rev date for 3 seconds

// ********************************************************************************************************************************
// *************************************************** HARDWARE OBJECTS ***********************************************************
// ********************************************************************************************************************************

Pinball_LCD* pLCD2004 = nullptr;
#include <Pinball_Message.h>
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

// ***** MISC GLOBALS *****
char msgType = ' ';

// ********************************************************************************************************************************
// ********************************************************* SETUP ****************************************************************
// ********************************************************************************************************************************

void setup() {

  initScreamoMasterArduinoPins();  // Arduino direct I/O pins only.
  setAllDevicesOff();              // Set initial state for all MOSFET PWM and Centipede outputs

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
      diagnosticState = 0;
      lcdClear();
      markDiagnosticsDisplayDirty(true);
      resetDiagButtonStates();
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
// ********************************************** SWITCH & INPUT PROCESSING ********************************************************
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

// ***** COIN ENTRY HANDLER *****
void processCoinEntry() {
  if (switchJustClosedFlag[SWITCH_IDX_COIN_MECH]) {
    pMessage->sendMAStoSLVCreditInc(1);
    activateDevice(DEV_IDX_KNOCKER);
  }
}

// ***** KNOCKOFF BUTTON HANDLER *****
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

// ***** START BUTTON HANDLER *****
void processStartButton() {
  // Only process start button in attract mode
  if (gamePhase != PHASE_ATTRACT) {
    return;
  }

  // First tap detection
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON] && !startWaitingForSecondTap && gameStyle == STYLE_NONE) {
    startFirstPressMs = millis();
    startTapCount = 1;
    startWaitingForSecondTap = true;
    return;
  }

  // Second tap detection (within 500ms window)
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON] && startWaitingForSecondTap) {
    unsigned long elapsed = millis() - startFirstPressMs;
    if (elapsed < START_STYLE_DETECT_WINDOW_MS) {
      // Double-tap detected -> Enhanced mode
      startTapCount = 2;
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

      // Credits available - fall through to ball check
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

      // Credits available - fall through to ball check
    }
  }

  // Ball check and game start logic
  if (gameStyle != STYLE_NONE) {
    // ***** ORIGINAL MODE: Open tray IMMEDIATELY *****
    if (gameStyle == STYLE_ORIGINAL) {
      activateDevice(DEV_IDX_BALL_TRAY_RELEASE);
      gameState.ballTrayOpen = true;
      ballTrayOpenStartMs = millis();
    }

    // Eject any balls in kickouts
    if (switchClosed(SWITCH_IDX_KICKOUT_LEFT)) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
    }
    if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT)) {
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
    }

    // ***** BOTH MODES: Check for 5 balls *****
    if (!allBallsInTrough()) {
      // ***** CHECK LIFT SWITCH IMMEDIATELY (before any waiting) *****
      bool ballInLiftAtStart = switchClosed(SWITCH_IDX_BALL_IN_LIFT);

      // Balls missing
      if (gameStyle == STYLE_ENHANCED) {
        activateDevice(DEV_IDX_BALL_TRAY_RELEASE);
        gameState.ballTrayOpen = true;
        ballTrayOpenStartMs = millis();
      }

      // Wait up to 5 seconds for switch to close AND stay closed for 1 second
      // CONTINUOUSLY monitor kickouts during entire wait period
      unsigned long trayWaitStart = millis();
      bool ballsSettled = false;

      while (millis() - trayWaitStart < 5000) {
        // ***** CONTINUOUS KICKOUT EJECTION *****
        if (switchClosedImmediate(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
          activateDevice(DEV_IDX_KICKOUT_LEFT);
        }
        if (switchClosedImmediate(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
          activateDevice(DEV_IDX_KICKOUT_RIGHT);
        }

        // Check for balls in trough
        if (switchClosedImmediate(SWITCH_IDX_5_BALLS_IN_TROUGH)) {
          // Switch just closed - verify it stays closed for 1 full second
          unsigned long verifyStart = millis();
          bool stayedClosed = true;

          while (millis() - verifyStart < BALL_DETECTION_STABILITY_MS) {
            // Keep ejecting kickouts during verification too!
            if (switchClosedImmediate(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
              activateDevice(DEV_IDX_KICKOUT_LEFT);
            }
            if (switchClosedImmediate(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
              activateDevice(DEV_IDX_KICKOUT_RIGHT);
            }

            if (!switchClosedImmediate(SWITCH_IDX_5_BALLS_IN_TROUGH)) {
              // Opened during verification - ball rolled over
              stayedClosed = false;
              break;
            }
            delay(10);
          }

          if (stayedClosed) {
            // Switch stayed closed for full second - balls settled!
            ballsSettled = true;
            break;
          }
          // Otherwise, discard this closure and keep waiting
        }
        delay(10);
      }

      // If still not settled after 5 seconds, give kickouts 2 more seconds
      if (!ballsSettled) {
        unsigned long kickoutWaitStart = millis();

        while (millis() - kickoutWaitStart < 2000) {
          // Continue ejecting kickouts
          if (switchClosedImmediate(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
            activateDevice(DEV_IDX_KICKOUT_LEFT);
          }
          if (switchClosedImmediate(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
            activateDevice(DEV_IDX_KICKOUT_RIGHT);
          }

          if (switchClosedImmediate(SWITCH_IDX_5_BALLS_IN_TROUGH)) {
            // Switch closed - verify stability
            unsigned long verifyStart = millis();
            bool stayedClosed = true;

            while (millis() - verifyStart < BALL_DETECTION_STABILITY_MS) {
              // Keep ejecting during verification
              if (switchClosedImmediate(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
                activateDevice(DEV_IDX_KICKOUT_LEFT);
              }
              if (switchClosedImmediate(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
                activateDevice(DEV_IDX_KICKOUT_RIGHT);
              }

              if (!switchClosedImmediate(SWITCH_IDX_5_BALLS_IN_TROUGH)) {
                stayedClosed = false;
                break;
              }
              delay(10);
            }

            if (stayedClosed) {
              ballsSettled = true;
              break;
            }
          }
          delay(10);
        }
      }

      // Final check before entering ball search
      if (!ballsSettled) {
        // Still missing - enter ball search
        gamePhase = PHASE_BALL_SEARCH;
        ballSearchSubState = BALL_SEARCH_INIT;

        if (gameStyle == STYLE_ENHANCED) {
          // ***** IMMEDIATE FEEDBACK: Use lift switch state from BEFORE we started waiting *****
          if (ballInLiftAtStart) {
            // Ball was stuck at lift base from the start - play instruction FIRST
            byte liftTrackLength = comTracksBallMissing[1].lengthTenths;  // Track 302
            pTsunami->trackPlayPoly(TRACK_BALL_IN_LIFT, 0, false);
            audioApplyTrackGain(TRACK_BALL_IN_LIFT, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
            audioCurrentComTrack = TRACK_BALL_IN_LIFT;
            audioComEndMillis = millis() + ((unsigned long)liftTrackLength * 100UL);

            // Wait for lift instruction to finish
            delay((unsigned long)liftTrackLength * 100UL + 100);

            // Now play general "ball missing" announcement
            unsigned int trackNum = comTracksBallMissing[0].trackNum;
            byte trackLength = comTracksBallMissing[0].lengthTenths;
            pTsunami->trackPlayPoly((int)trackNum, 0, false);
            audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
            audioCurrentComTrack = trackNum;
            audioComEndMillis = millis() + ((unsigned long)trackLength * 100UL);
          } else {
            // No ball at lift - just play general "ball missing" announcement
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
      // ***** BALLS FOUND DURING WAIT - Continue to game start *****
      // (Fall through to "All 5 balls present!" section below)
    }

    // ***** All 5 balls present! *****
    if (gameStyle == STYLE_ENHANCED) {
      releaseDevice(DEV_IDX_BALL_TRAY_RELEASE);
      gameState.ballTrayOpen = false;

      // Play SCREAM immediately (before credit deduction)
      pTsunami->stopAllTracks();
      pTsunami->trackPlayPoly(TRACK_START_SCREAM, 0, false);
      audioApplyTrackGain(TRACK_START_SCREAM, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
      delay(1500);  // Let scream play (brief blocking delay acceptable during startup)
    }

    // Deduct credit and start game
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
    }
    // Open player addition window (Original: for Impulse switch)
    playerAdditionWindowOpen = true;

    // Transition to PHASE_STARTUP immediately (no blocking delay)
    gamePhase = PHASE_STARTUP;
    startupSubState = STARTUP_INIT;
    startupTickCounter = 0;

    // Optional: Brief LCD feedback (will be replaced by status line in PHASE_BALL_READY)
    if (gameStyle == STYLE_ENHANCED) {
      lcdClear();
      lcdPrintRow(1, "Starting Enhanced");
    }
    // For Original/Impulse: no LCD feedback (stays silent per design)

    return;
  }
}

// ********************************************************************************************************************************
// ***************************************************** DIAGNOSTICS **************************************************************
// ********************************************************************************************************************************

// ***** DIAGNOSTIC MENU HANDLER *****
static bool handleDiagnosticsMenuButtons() {
  if (diagButtonPressed(0)) {
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
    diagEnterSelectedSuite();
    return false;
  }
  return false;
}

void runDiagnosticsLoop() {
  if (diagnosticState != 0) {
    diagnosticState = 0;
  }
  if (handleDiagnosticsMenuButtons()) {
    return;
  }
  renderDiagnosticsMenuIfNeeded();
}

// ***** DIAGNOSTIC SUITE DISPATCHER *****
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
}

void updateModeDiagnostic() {
  runDiagnosticsLoop();
}

// ********************************************************************************************************************************
// ************************************************** GAME PHASE HANDLERS *********************************************************
// ********************************************************************************************************************************

// (Future: updatePhaseAttract, updatePhaseStartup, updatePhaseBallInPlay, etc. will go here)

// ***** BALL SEARCH PHASE HANDLER *****
void updatePhaseBallSearch() {
  // Check for timeout (30 seconds with tray open, no balls found)
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

    // Save score if we had started a game
    if (gameState.numPlayers > 0) {
      saveLastScore(gameState.score[gameState.currentPlayer - 1]);
    }

    lcdClear();
    markAttractDisplayDirty();
    gamePhase = PHASE_ATTRACT;
    gameStyle = STYLE_NONE;
    return;
  }
  // Handle START button press (user feedback)
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON]) {
    if (gameStyle == STYLE_ENHANCED) {
      // Enhanced: replay announcement
      pTsunami->stopAllTracks();
      audioCurrentComTrack = 0;
      audioComEndMillis = 0;

      // Check if ball is at lift base (immediate feedback!)
      bool ballInLift = switchClosed(SWITCH_IDX_BALL_IN_LIFT);

      if (ballInLift) {
        // Play lift instruction FIRST
        byte liftTrackLength = comTracksBallMissing[1].lengthTenths;  // Track 302
        pTsunami->trackPlayPoly(TRACK_BALL_IN_LIFT, 0, false);
        audioApplyTrackGain(TRACK_BALL_IN_LIFT, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = TRACK_BALL_IN_LIFT;
        audioComEndMillis = millis() + ((unsigned long)liftTrackLength * 100UL);

        // Wait for lift instruction to finish
        delay((unsigned long)liftTrackLength * 100UL + 100);

        // Then play general "ball missing" announcement
        unsigned int trackNum = comTracksBallMissing[0].trackNum;
        byte trackLength = comTracksBallMissing[0].lengthTenths;
        pTsunami->trackPlayPoly((int)trackNum, 0, false);
        audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = trackNum;
        audioComEndMillis = millis() + ((unsigned long)trackLength * 100UL);
      } else {
        // No ball at lift - just play general announcement
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
  // Check if all balls found
  if (allBallsInTrough()) {
    // Success! Stop audio
    pTsunami->stopAllTracks();
    audioCurrentComTrack = 0;
    audioComEndMillis = 0;

    // Close tray (Enhanced only)
    if (gameStyle == STYLE_ENHANCED) {
      releaseDevice(DEV_IDX_BALL_TRAY_RELEASE);
      gameState.ballTrayOpen = false;
    }

    lcdClear();
    lcdPrintRow(1, "All balls found!");
    lcdPrintRow(2, "Deducting credit...");
    delay(1000);

    // ***** PLAY SCREAM (Enhanced only) *****
    if (gameStyle == STYLE_ENHANCED) {
      pTsunami->trackPlayPoly(TRACK_START_SCREAM, 0, false);
      audioApplyTrackGain(TRACK_START_SCREAM, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
      delay(1500);  // Let scream play
    }

    pMessage->sendMAStoSLVCreditDec();

    gameState.numPlayers = 1;
    gameState.currentPlayer = 1;
    gameState.currentBall = 1;

    lcdClear();
    lcdPrintRow(1, "Starting game...");
    lcdPrintRow(2, gameStyle == STYLE_ENHANCED ? "Enhanced" : "Original");
    lcdPrintRow(3, "1 Player, Ball 1");
    lcdPrintRow(4, "Please wait...");

    if (gameStyle == STYLE_ENHANCED) {
      pTsunami->trackPlayPoly(TRACK_START_HEY_GANG, 0, false);
      audioApplyTrackGain(TRACK_START_HEY_GANG, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_START_HEY_GANG;
      audioComEndMillis = millis() + 3500UL;
    }

    delay(3000);

    // Transition to PHASE_STARTUP
    gamePhase = PHASE_STARTUP;
    startupSubState = STARTUP_INIT;
    startupTickCounter = 0;
    return;
  }

  // Ball search state machine
  switch (ballSearchSubState) {
  case BALL_SEARCH_INIT:
    // Fire kickouts once at start
    activateDevice(DEV_IDX_KICKOUT_LEFT);
    activateDevice(DEV_IDX_KICKOUT_RIGHT);
    ballSearchLastKickoutMs = millis();
    ballSearchSubState = BALL_SEARCH_MONITOR;
    break;

  case BALL_SEARCH_MONITOR:
  {
    unsigned long now = millis();

    // ***** CONTINUOUS KICKOUT MONITORING: Eject ball IMMEDIATELY if detected *****
    // Use switchClosedImmediate() for fastest possible detection
    if (switchClosedImmediate(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
    }
    if (switchClosedImmediate(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
    }

    // ***** PERIODIC KICKOUT FIRE: Every 15 seconds (in case ball jammed and not tripping switch) *****
    if (now - ballSearchLastKickoutMs >= BALL_SEARCH_KICKOUT_INTERVAL_MS) {
      activateDevice(DEV_IDX_KICKOUT_LEFT);
      activateDevice(DEV_IDX_KICKOUT_RIGHT);
      ballSearchLastKickoutMs = now;
    }
  }
  break;
  }
}

// ***** BALL READY PHASE HANDLER *****
void updatePhaseBallReady() {
  // ***** IMPULSE MODE SWITCH: Delayed second START press (Original -> Impulse) *****
  if (playerAdditionWindowOpen && gameStyle == STYLE_ORIGINAL && !gameState.hasScored) {
    if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON]) {
      gameStyle = STYLE_IMPULSE;
      playerAdditionWindowOpen = false;

      lcdClear();
      lcdPrintRow(1, "Impulse Mode");
      lcdPrintRow(2, "Activated!");
      delay(1500);
      lcdClear();

      snprintf(lcdString, LCD_WIDTH + 1, "Imp P%d Ball %d",
        gameState.currentPlayer,
        gameState.currentBall);
      lcdPrintRow(4, lcdString);

      Serial.println("IMPULSE MODE ACTIVATED");
    }
  }

  // ***** ENHANCED MODE: Wait for ball to settle before monitoring for hill climb *****
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

  // ***** ALL MODES: Detect ANY scoring switch to mark first point *****
  bool scoringSwitchHit = false;
  bool wasDrain = false;

  // Drains (check these FIRST so we can mark them separately)
  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_LEFT] ||
    switchJustClosedFlag[SWITCH_IDX_DRAIN_CENTER] ||
    switchJustClosedFlag[SWITCH_IDX_DRAIN_RIGHT] ||
    switchJustClosedFlag[SWITCH_IDX_GOBBLE]) {
    scoringSwitchHit = true;
    wasDrain = true;
  }

  // Other scoring switches
  if (!scoringSwitchHit) {
    // Bumpers
    for (byte i = SWITCH_IDX_BUMPER_S; i <= SWITCH_IDX_BUMPER_O; i++) {
      if (switchJustClosedFlag[i]) {
        scoringSwitchHit = true;
        break;
      }
    }
  }

  if (!scoringSwitchHit) {
    // Slingshots
    if (switchJustClosedFlag[SWITCH_IDX_SLINGSHOT_LEFT] ||
      switchJustClosedFlag[SWITCH_IDX_SLINGSHOT_RIGHT]) {
      scoringSwitchHit = true;
    }
  }

  if (!scoringSwitchHit) {
    // Side targets
    for (byte i = SWITCH_IDX_LEFT_SIDE_TARGET_2; i <= SWITCH_IDX_LEFT_SIDE_TARGET_5; i++) {
      if (switchJustClosedFlag[i]) {
        scoringSwitchHit = true;
        break;
      }
    }
  }

  if (!scoringSwitchHit) {
    for (byte i = SWITCH_IDX_RIGHT_SIDE_TARGET_1; i <= SWITCH_IDX_RIGHT_SIDE_TARGET_5; i++) {
      if (switchJustClosedFlag[i]) {
        scoringSwitchHit = true;
        break;
      }
    }
  }

  if (!scoringSwitchHit) {
    // Hat rollovers
    if (switchJustClosedFlag[SWITCH_IDX_HAT_LEFT_TOP] ||
      switchJustClosedFlag[SWITCH_IDX_HAT_LEFT_BOTTOM] ||
      switchJustClosedFlag[SWITCH_IDX_HAT_RIGHT_TOP] ||
      switchJustClosedFlag[SWITCH_IDX_HAT_RIGHT_BOTTOM]) {
      scoringSwitchHit = true;
    }
  }

  if (!scoringSwitchHit) {
    // Kickouts
    if (switchJustClosedFlag[SWITCH_IDX_KICKOUT_LEFT] ||
      switchJustClosedFlag[SWITCH_IDX_KICKOUT_RIGHT]) {
      scoringSwitchHit = true;
    }
  }

  // ***** FIRST POINT DETECTED *****
  if (scoringSwitchHit) {
    Serial.print("DEBUG: First scoring switch hit, wasDrain=");
    Serial.println(wasDrain);

    // Mark that player has scored (for Impulse mode switch window)
    gameState.hasScored = true;

    // If NOT a drain, handle first point transition
    if (!wasDrain) {
      handleFirstPointScored();
    }

    // Transition to BALL_IN_PLAY
    gamePhase = PHASE_BALL_IN_PLAY;
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

  // ***** BALL DRAIN DETECTION *****
  bool drainDetected = false;
  byte drainType = 0;  // 0=none, 1=left, 2=center, 3=right, 4=gobble

  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_LEFT]) {
    drainDetected = true;
    drainType = 1;
  } else if (switchJustClosedFlag[SWITCH_IDX_DRAIN_CENTER]) {
    drainDetected = true;
    drainType = 2;
  } else if (switchJustClosedFlag[SWITCH_IDX_DRAIN_RIGHT]) {
    drainDetected = true;
    drainType = 3;
  } else if (switchJustClosedFlag[SWITCH_IDX_GOBBLE]) {
    drainDetected = true;
    drainType = 4;
  }

  if (drainDetected) {
    handleBallDrain(drainType);
  }

  // TODO: Implement full scoring logic here
}

// ***** BALL DRAIN HANDLER *****
void handleBallDrain(byte drainType) {
  Serial.print("DEBUG: Ball drained (type ");
  Serial.print(drainType);
  Serial.print("), hasScored=");
  Serial.println(gameState.hasScored);

  // ***** ENHANCED MODE: Ball Save Logic *****
  if (gameStyle == STYLE_ENHANCED) {

    // Case 2: Ball save timer active and not used yet
    if (ballSave.active && millis() < ballSave.endMs) {
      Serial.println("DEBUG: Ball save timer active, saving ball");
      saveBall("timed_save");
      ballSave.active = false;
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
  // TODO: Implement full ball lost logic
  // - Check if more balls remain
  // - Dispense next ball OR end game
  // - Play drain audio
  // - Activate shaker motor

  // If this was the last ball of the last player:
  if (gameState.currentBall >= MAX_BALLS) {
    // Save score before returning to attract
    saveLastScore(gameState.score[gameState.currentPlayer - 1]);

    // Play game over audio, etc.
    // ...

    gamePhase = PHASE_ATTRACT;
    gameStyle = STYLE_NONE;
  }
  Serial.println("DEBUG: processBallLost() - NOT YET IMPLEMENTED");
}

// ***** STARTUP PHASE HANDLER *****
void updatePhaseStartup() {
  // Tick counter increments every loop (10ms)
  startupTickCounter++;

  switch (startupSubState) {
  case STARTUP_INIT:
    // Initialize game state for new ball
    gameState.tilted = false;
    gameState.tiltWarnings = 0;
    gameState.ballInPlay = false;
    gameState.ballSaveActive = false;

    // Move to dispense state
    startupSubState = STARTUP_DISPENSE_BALL;
    startupTickCounter = 0;
    break;

  case STARTUP_DISPENSE_BALL:
    // Fire ball trough release coil (230ms pulse)
    activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);

    // Record when we dispensed the ball (for timeout tracking)
    startupBallDispenseMs = millis();

    // Move to wait state
    startupSubState = STARTUP_WAIT_FOR_LIFT;
    startupTickCounter = 0;
    break;

  case STARTUP_WAIT_FOR_LIFT:
    // Check if ball has reached lift
    if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      // Success! Ball is in lift
      startupSubState = STARTUP_COMPLETE;
      startupTickCounter = 0;
      break;
    }

    // Check for timeout (3 seconds)
    if (millis() - startupBallDispenseMs >= BALL_TROUGH_TO_LIFT_TIMEOUT_MS) {
      // Timeout - ball didn't reach lift
      criticalError("BALL STUCK", "Ball did not reach", "lift in 3 seconds");
      return;
    }
    break;

  case STARTUP_COMPLETE:
    // Prepare for ball ready phase
    startupSubState = STARTUP_INIT;  // Reset for next ball
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
    break;
  }
}

// ***** FIRST POINT SCORED HANDLER *****
void handleFirstPointScored() {
  if (!playerAdditionWindowOpen) {
    return;  // Already handled first point
  }

  // Close player addition window
  playerAdditionWindowOpen = false;
  gameState.hasScored = true;

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
    ballSave.active = true;
    ballSave.endMs = millis() + 10000UL;  // 10 seconds for testing

    Serial.println("DEBUG: First point scored, scream + music + shaker started, ball save active");
  }
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

// ***** FLIPPER HANDLING *****
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

void audioAdjustMasterGain(int8_t t_deltaDb) {
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

void audioStopAllMusic() {
  if (audioCurrentMusicTrack != 0) {
    audioStopTrack(audioCurrentMusicTrack, pTsunami);
    audioCurrentMusicTrack = 0;
  }
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

// ***** AUDIO DUCKING HELPER *****
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
// ************************************************** SYSTEM UTILITIES ************************************************************
// ********************************************************************************************************************************

bool isInMode() {
  // Roll-A-Ball/Impulse/Enhanced mode active
  return (modeState.active && modeState.currentMode != MODE_NONE);
}

void initGameState() {
  gameState.numPlayers = 0;
  gameState.currentPlayer = 0;
  gameState.currentBall = 0;
  for (byte i = 0; i < MAX_PLAYERS; i++) {
    gameState.score[i] = 0;
  }
  gameState.tiltWarnings = 0;
  gameState.tilted = false;
  gameState.ballInPlay = false;
  gameState.ballSaveActive = false;
  gameState.ballSaveEndMs = 0;
  gameState.ballLaunchMs = 0;
  gameState.hasScored = false;
  gameState.ballsToDispense = 0;
  gameState.ballsInTray = 0;
  gameState.ballsInTrough = 0;
  gameState.ballsLocked = 0;
  gameState.ballTrayOpen = false;

  // Initialize ball save state
  ballSave.active = false;
  ballSave.endMs = 0;
}

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

// Also add a flag or logic to suppress Score Motor, Selection Unit, and Relay Reset Bank
// when gameStyle == STYLE_ENHANCED. For example:

bool useMechanicalScoringDevices() {
  // Suppress Score Motor, Selection Unit, and Relay Reset Bank when gameStyle == STYLE_ENHANCED
  // Returns true if Score Motor, Selection Unit, and Relay Reset Bank should be used.
  // These are only used in Original and Impulse styles, never in Enhanced.
  return (gameStyle != STYLE_ENHANCED);
}

// ***** SCORE PERSISTENCE *****
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

// ***** BALL READY PHASE ENTRY *****
void onEnterBallReady() {
  Serial.println("DEBUG: Entering BALL_READY phase");

  // Stop any running shakers
  stopAllShakers();

  // Reset ball save state
  ballSave.active = false;
  ballSave.endMs = 0;

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
  // (DO NOT set gameState.hasScored here - that happens on first switch hit)
}

// ***** SCORE MOTOR HELPER *****
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

// ***** BALL TRACKING HELPER *****
bool allBallsInTrough() {
  // Check if "5 Balls in Trough" switch is closed
  return switchClosed(SWITCH_IDX_5_BALLS_IN_TROUGH);
}

bool inGracePeriod() {
  // Check if we are within Grace Period after a ball has drained
  return (modeEndedAtMs > 0) &&
    ((millis() - modeEndedAtMs) < MODE_END_GRACE_PERIOD_MS);
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