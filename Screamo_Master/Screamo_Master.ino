// Screamo_Master.INO Rev: 03/18/26
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
#include <Pinball_Lamp_Effects.h>   // Position-based lamp effect engine

const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 03/18/26";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
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

  {  8, 150, 10,   40, 0, 0 },  // FLIPPER_LEFT        =  5, PWM MOSFET.  200 hits gobble hole too hard; 150 can get to top of p/f.
  {  9, 150, 10,   40, 0, 0 },  // FLIPPER_RIGHT       =  6, PWM MOSFET


//  {  8, 150, 10,  40, 0, 0 },  // FLIPPER_LEFT        =  5, PWM MOSFET.  200 hits gobble hole too hard; 150 can get to top of p/f.
//  {  9, 150, 10,  40, 0, 0 },  // FLIPPER_RIGHT       =  6, PWM MOSFET
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
const uint8_t IMPULSE_FLIPPER_UP_TICKS =   5;  // Hold impulse flippers up for 50ms (5 ticks) before releasing

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

// ***** DEFERRED COM ANNOUNCEMENT *****
// When a verbal award (Extra Ball, replay, SCREAMO, Special lit, etc.) coincides with
// bells ringing on Slave, the announcement is deferred until the bells and knocker finish.
// processAudioTick() checks pendingComFireMs each tick and plays the track when ready.
unsigned int pendingComTrack = 0;           // Track number to play (0 = nothing pending)
byte pendingComLenTenths = 0;              // Track length in 0.1s units
unsigned long pendingComFireMs = 0;        // millis() when the track should start playing
bool pendingComDuckMusic = false;          // Whether to duck music when the track plays

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

// ***** GAME CONSTANTS *****
const byte MAX_PLAYERS                       =    1;  // Currently only support single player, even for Enhanced
const byte MAX_BALLS                         =    5;
const byte MAX_MODES_PER_GAME                =    3;  // Maximum modes that can be played per game

// ***** CORE GAME STATE *****
byte gamePhase = PHASE_ATTRACT;
byte gameStyle = STYLE_NONE;  // Selected style (set by start button taps)
GameStateStruct gameState;    // GameStateStruct is defined in Pinball_Consts.h

// ***** STARTUP SUB-STATE *****
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

// ***** START BUTTON DETECTION *****
const unsigned long START_STYLE_DETECT_WINDOW_MS = 500;  // 500ms window for each tap stage
unsigned long startFirstPressMs = 0;
unsigned long startSecondPressMs = 0;
bool startWaitingForSecondTap = false;
bool startWaitingForThirdTap = false;
bool creditDeducted = false;  // True once credit has been deducted for this game attempt

// ***** BALL TRACKING *****
// Counter-based tracking per Overview Section 3.5.4.
// These are standalone globals (not in a struct) because they are read/written frequently in tight game logic.
byte ballsInPlay = 0;          // Balls actively on playfield (0-3). Increment on first score or replacement launch.
bool ballInLift = false;       // True if a ball is waiting at the base of the Ball Lift.
bool ballHasLeftLift = false;  // Latches true once the ball departs the lift; never reverts to false for this ball attempt.
byte ballsLocked = 0;          // Balls captured in kickouts (0-2). Enhanced style only; persists across ball numbers.
bool prevBallInLift = false;   // Previous state of ball-in-lift switch (for edge detection)

// ***** TROUGH SANITY CHECK *****
// During PHASE_BALL_IN_PLAY, if the 5-balls-in-trough switch is held closed for
// BALL_DETECTION_STABILITY_MS (1 second), all 5 balls are physically in the trough.
// If ballsInPlay > 0 at that point, a drain was missed (e.g. two balls rolled over
// a drain rollover back-to-back too quickly for us to catch - it can happen). Correct
// ballsInPlay to 0 and handle the drain. Requires stability to distinguish the 5th
// ball settling on the switch from a ball momentarily rolling over it.
const unsigned long BALL_DETECTION_STABILITY_MS = 1000;  // 1 second (adjustable)
unsigned long troughSanityStableStartMs = 0;
bool troughSanityWasStable = false;

// ***** UNDETECTED DRAIN RECOVERY *****
// Safety net for the rare case where a ball drains behind a raised flipper without
// triggering any drain switch. Only relevant when ballsLocked >= 1, because when
// ballsLocked == 0, all 5 balls end up in the trough and the trough sanity check
// (above) catches the missed drain within 1 second -- much faster than this timer.
// When ballsLocked >= 1, the trough switch will NOT close (only 3 or 4 balls in
// trough), so this timer is the only detection mechanism.
//
// The timer starts when PHASE_BALL_IN_PLAY is entered (first playfield switch hit).
// It is reset by any playfield switch hit. Flipper button HOLDS pause the countdown
// (player may be cradling a ball), but flipper TAPS do not reset the timer (player
// may be pressing buttons in frustration after an undetected drain).
//
// When the timer expires, calls handleBallDrain(DRAIN_TYPE_NONE) so all drain
// processing (ball save, mode replacement, extra ball, game over) is handled
// through the normal pipeline.
const unsigned long UNDETECTED_DRAIN_TIMEOUT_MS = 15000;  // 15 seconds of inactivity
unsigned long undetectedDrainDeadlineMs = 0;              // millis() deadline (0 = inactive)
unsigned long undetectedDrainPausedRemainingMs = 0;       // Remaining ms when paused by flipper hold (0 = not paused)

// ***** BALL SAVE *****
// Ball save uses a simple timestamp: active when endMs > 0 and millis() < endMs.
// ballSaveUsed tracks whether the save for the current ball number has been consumed.
// Once used, no new timer is started until the next ball number (or new game).
BallSaveStruct ballSave;                       // BallSaveStruct is defined in Pinball_Consts.h
bool ballSaveUsed = false;                     // True once a ball save has been consumed for this ball number
unsigned long ballSaveConsumedMs = 0;          // millis() when ball save was consumed (for snarky drain audio)
unsigned long savedBallFirstSwitchMs = 0;      // millis() when the saved replacement ball first hit a playfield switch
const byte BALL_SAVE_DEFAULT_SECONDS = 15;
byte ballSaveSeconds = BALL_SAVE_DEFAULT_SECONDS;  // Loaded from EEPROM at game start

// ***** EXTRA BALL *****
bool extraBallAwarded = false;  // Extra Ball earned, waiting to be used

// ***** MULTIBALL *****
bool inMultiball = false;       // True during multiball (Enhanced style only)
bool multiballPending = false;  // True when 2 balls are locked and waiting for a switch hit to start multiball

// Multiball ball save: when multiball starts, a ball save timer runs for ballSaveSeconds.
// Any ball that drains while the timer is active is replaced from the trough without
// changing game phase (gameplay continues on the surviving balls). The replacement is
// dispensed in the background by processMultiballSaveDispenseTick(), which monitors
// ball-in-lift and increments ballsInPlay when the new ball arrives.
// multiballSavesRemaining tracks how many replacement balls are in transit or pending.
// It is decremented when a saved ball is dispensed (not when it arrives), so rapid
// double-drains within the save window each get their own replacement.
unsigned long multiballSaveEndMs = 0;            // When multiball ball save expires (0 = not active)
byte multiballSavesRemaining = 0;                // Replacement balls pending dispense or in transit
bool multiballSaveDispensePending = false;       // True when a replacement needs to be dispensed
bool multiballSaveWaitingForLift = false;        // True when waiting for replacement to reach lift
unsigned long multiballSaveDispenseMs = 0;       // When the replacement was dispensed (for timeout)

// Locked kickout tracking: tracks WHICH kickouts have locked balls (not just a count).
// Bit 0 = left kickout locked, bit 1 = right kickout locked.
// This prevents: (a) phantom edges from a locked ball triggering a second lock,
// (b) the same kickout being locked twice, and (c) scoring dispatch confusion.
byte kickoutLockedMask = 0;  // Bitmask: bit 0 = left locked, bit 1 = right locked
const byte KICKOUT_LOCK_LEFT  = 0x01;
const byte KICKOUT_LOCK_RIGHT = 0x02;

// ***** KICKOUT EJECT TIMING *****
// After a kickout switch closes during gameplay, scoring logic will decide when to eject.
// Original/Impulse: eject after score motor quarter-rev (~882ms).
// Enhanced: eject immediately unless locking for multiball.
// Ball search / startup / attract: eject immediately via activateDevice().
const byte KICKOUT_RETRY_TICKS = 100;  // Reserved for future stuck-ball retry if needed
const unsigned long KICKOUT_EJECT_DELAY_MS = 735;  // 5 sub-slots * 147ms = 735ms
unsigned long kickoutLeftEjectMs = 0;   // millis() deadline for left kickout eject (0 = not pending)
unsigned long kickoutRightEjectMs = 0;  // millis() deadline for right kickout eject (0 = not pending)

// ***** MODE STATE *****
// ModeStateStruct is defined in Pinball_Consts.h
ModeStateStruct modeState;

// Pre-mode EM state snapshot: these variables can be mutated by normal scoring
// handlers during the mode. Save/restore ensures the mode cannot permanently
// corrupt the EM tracking state.
byte preModeBumperLitMask = 0x7F;
unsigned int preModeWhiteLitMask = 0;
byte preModeSelectionUnitPosition = 0;
byte preModeGobbleCount = 0;

// Mode intro tracking: the mode timer does not start until the intro COM track ends.
// modeIntroEndMs is set to millis() + intro duration when the mode starts.
// processModeTick() waits for this deadline before starting the gameplay timer.
unsigned long modeIntroEndMs = 0;
bool modeTimerStarted = false;
bool modeStartFlashPending = false;  // True while flash-all is running; processModeTick() starts fill-drain when it finishes

// Mode ball replacement: uses background dispense (same mechanism as multiball save).
// These are separate from the multiball save variables to avoid cross-contamination.
bool modeSaveDispensePending = false;
bool modeSaveWaitingForLift = false;
unsigned long modeSaveDispenseMs = 0;
unsigned long modeEndMusicRestartMs = 0;  // When to restart primary music after mode ends (0 = not pending)

// Cached state for updateSpotsNumberLamps() skip-if-unchanged optimization.
// Must be reset to 0xFF at game start and after any bulk lamp write that changes
// these lamps outside of updateSpotsNumberLamps() (mode start/end, tilt restore).
byte spotsNumberLastTenKDigit = 0xFF;
byte spotsNumberLastKickoutMask = 0xFF;

const unsigned long MODE_END_GRACE_PERIOD_MS = 5000;  // 5 seconds drain after mode ends, you still get the ball back

// ***** HILL CLIMB / BALL READY *****
bool hillClimbStarted = false;          // True once hill climb audio/shaker has started (to prevent re-triggering)
bool ballSettledInLift = false;         // True once ball is confirmed resting at lift base (prevents premature hill climb trigger)
bool ball1SequencePlayed = false;       // True once Ball 1's hill drop + ball save timer have been armed (prevents re-triggering after ball save)
const unsigned long HILL_CLIMB_TIMEOUT_MS = 15000;  // 15 seconds before "shoot the ball" reminder
unsigned long shootReminderDeadlineMs = 0;          // millis() deadline for next shoot reminder (0 = inactive)

// ***** SHAKER MOTOR *****
const byte SHAKER_POWER_HILL_CLIMB = 80;            // Low rumble during hill climb
const byte SHAKER_POWER_HILL_DROP = 110;            // Medium intensity during first drop
const byte SHAKER_POWER_ATTENTION = 100;            // Higher intensity for "pay attention" COMs during modes
const unsigned long SHAKER_HILL_DROP_MS = 11000;    // Hill drop duration (matches track 404)
const unsigned long SHAKER_GOBBLE_MS = 1000;        // Gobble Hole Shooting Gallery hit
const unsigned long SHAKER_BUMPER_MS = 500;         // Bumper Cars hit
const unsigned long SHAKER_HAT_MS = 800;            // Roll-A-Ball hat rollover
const unsigned long SHAKER_DRAIN_MS = 2000;         // Ball drain (non-mode)
const unsigned long SHAKER_MODE_START_MS = 1000;    // Mode start school bell "ding" (~matches track 1001)
const unsigned long SHAKER_JACKPOT_MS = 2000;       // Jackpot celebration burst (matches starburst lamp effect)
const unsigned long SHAKER_TIME_UP_MS = 800;         // Subtle "time's up" nudge
unsigned long shakerDropStartMs = 0;                // When hill drop shaker started (0 = not running)

// ***** TILT *****
// Tilt lamp flashing (visual feedback after full tilt)
unsigned long tiltLampFlashStartMs = 0;             // When tilt lamp flashing started (0 = not flashing)
const unsigned long TILT_LAMP_FLASH_MS = 4000;      // Flash TILT lamp for 4 seconds
const unsigned long TILT_LAMP_TOGGLE_MS = 250;      // Toggle every 250ms (2Hz flash)
bool tiltLampCurrentlyOn = false;
// Tilt bob settle time: after an Enhanced tilt warning, suppress further tilt bob
// detection for this duration to let the physical tilt bob stop swinging. Without this,
// a single nudge causes the bob to bounce and register both a warning AND a full tilt
// almost simultaneously.
const unsigned long TILT_BOB_SETTLE_MS = 1000;      // 1 second settle time after warning
unsigned long tiltBobSettleUntilMs = 0;             // millis() deadline; ignore tilt bob until this time

// ***** SCORE MOTOR *****
// Timing constants shared by startup and gameplay
const unsigned long SCORE_MOTOR_SUB_SLOT_MS = 147;       // One sub-slot = 882ms / 6
const unsigned long SCORE_MOTOR_QUARTER_REV_MS = 882;    // One quarter-revolution of the Score Motor
// Startup sequence (Original/Impulse Only): Score Motor runs for 4 quarter-turns (3528ms)
// during game start. Actions are fired at specific sub-slot offsets within the sequence.
// This runs concurrently with ball recovery in STARTUP_RECOVER_WAIT.
// The credit has already been deducted and the ball tray already opened before this
// sequence starts, so those sub-slots are no-ops.
unsigned long scoreMotorStartMs = 0;                     // When score motor was turned on (0 = not running)
bool scoreMotorSlotRelayReset2 = false;                  // True once 2nd relay reset has fired
bool scoreMotorSlotScoreReset = false;                   // True once score reset has been sent to Slave
// Gameplay timer (Original/Impulse Only): separate from scoreMotorStartMs which is
// used only for the startup sequence. This timer manages the Score Motor SSR during
// gameplay scoring events. The motor SSR has timeOn=0, so activateDevice()/
// updateDeviceTimers() cannot manage its timing -- we handle it manually here.
unsigned long scoreMotorGameplayEndMs = 0;               // millis() deadline; 0 = motor not running for gameplay

// ***** FLIPPER STATE *****
bool leftFlipperHeld = false;
bool rightFlipperHeld = false;
byte impulseFlipperTicksRemaining = 0;  // Countdown for impulse flipper release

// ***** KNOCKOFF BUTTON *****
unsigned long knockoffPressStartMs = 0;
bool knockoffBeingHeld = false;
const unsigned long KNOCKOFF_RESET_HOLD_MS = 1000;  // Hold knockoff for 1s to trigger reset

// ***** EM EMULATION STATE *****
// These track the simulated state of original EM devices. See Overview Section 13.3.
const byte SELECTION_UNIT_RED_INSERT[10] = { 8, 3, 4, 9, 2, 5, 7, 2, 1, 6 };
// Position 0-49 maps to: RED = SELECTION_UNIT_RED_INSERT[position % 10]
// HATs lit when: (position % 10 == 0) && (position / 10) is 0, 2, or 4
byte selectionUnitPosition = 0;   // 0-49, randomized at game start. Advances on side target hits only.
byte gobbleCount = 0;             // Balls gobbled this game (0-5). 5th gobble awards replay if SPECIAL is lit.
byte bumperLitMask = 0x7F;        // Which bumpers are lit (bits 0-6 = S,C,R,E,A,M,O). 0x7F = all lit.
unsigned int whiteLitMask = 0;    // Which WHITE inserts are lit (bits 0-8 = numbers 1-9). 0 = none lit.
byte whiteRowsAwarded = 0;        // Bitmask of which 3-in-a-row lines have been awarded this matrix cycle (bits 0-7).
bool whiteFourCornersAwarded = false;  // True if four-corners+center pattern has been awarded this matrix cycle.

// ***** REPLAY QUEUE *****
// When replays are awarded, the knocker fires at Score Motor cadence (one knock per 147ms
// sub-slot, 5 knocks per quarter-rev with a 147ms rest). The queue tracks how many
// knocker fires + credit additions remain. processReplayKnockerTick() drains the queue.
byte replayQueueCount = 0;              // Number of replay knocks remaining to fire
unsigned long replayNextKnockMs = 0;    // millis() deadline for next knocker fire (0 = idle)
byte replayKnocksThisCycle = 0;         // Knocks fired in current quarter-rev cycle (0-5; rest after 5)
byte replayPendingCount = 0;            // Replays waiting for Score Motor to finish before knocker fires
const byte NUM_REPLAY_THRESHOLDS = 5;
byte replayScoreAwarded = 0;            // Bitmask of which replay scores have been awarded this game

// ***** BALL SEARCH *****
const byte BALL_SEARCH_INIT           = 0;  // Initialize search
const byte BALL_SEARCH_EJECT_KICKOUTS = 1;  // Fire both kickouts
const byte BALL_SEARCH_OPEN_TRAY      = 2;  // Open ball tray
const byte BALL_SEARCH_WAIT_TRAY      = 3;  // Wait 5 seconds with tray open
const byte BALL_SEARCH_CLOSE_TRAY     = 4;  // Close ball tray (if needed)
const byte BALL_SEARCH_MONITOR        = 5;  // Passive monitoring loop
byte ballSearchSubState = BALL_SEARCH_INIT;
unsigned long ballSearchLastKickoutMs = 0;
const unsigned long BALL_SEARCH_KICKOUT_INTERVAL_MS = 15000;  // Fire kickouts during ball search every 15 seconds
unsigned long ballTrayOpenStartMs = 0;
const unsigned long BALL_SEARCH_TIMEOUT_MS = 30000;  // 30 seconds for testing (change to 300000 for 5 minutes)

// ***** DRAIN AUDIO *****
const byte BALL_DRAIN_INSULT_FREQUENCY = 50;  // 0..100 indicates percent of the time we make a random snarky comment when the ball drains

// ***** EEPROM / SCORE PERSISTENCE *****
const int EEPROM_SAVE_INTERVAL_MS             = 10000;  // Save interval for EEPROM (milliseconds)
int lastDisplayedScore = 0;  // Score to display at power-up (loaded from EEPROM)
unsigned long lastEepromSaveMs = 0;  // When we last saved to EEPROM

// ***** GAME TIMEOUT *****
unsigned long gameTimeoutMs = 0;
unsigned long gameTimeoutDeadlineMs = 0;  // millis() deadline; if exceeded with no activity, game ends

// ********************************************************************************************************************************
// ********************************************************* SETUP ****************************************************************
// ********************************************************************************************************************************

void setup() {

  initScreamoMasterArduinoPins();  // Arduino direct I/O pins only.
  setAllDevicesOff();              // Set initial state for all MOSFET PWM and Centipede outputs
  // Seed random number generator with some noise from an unconnected analog pin.
  randomSeed(analogRead(A0));      // We'll use this to select random COM lines and snarky comments on drains
  gameTimeoutMs = (unsigned long)diagReadGameTimeoutFromEEPROM() * 60000UL;

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

  // Initialize game state (loads persisted EM state from EEPROM)
  initGameState();

  // Initialize lamps to reflect persisted EM state (must come after initGameState
  // loads selectionUnitPosition, whiteLitMask, gobbleCount from EEPROM)
  setGILamps(true);
  pMessage->sendMAStoSLVGILamp(true);
  setAttractLamps();

  // Prime switch state arrays from live hardware BEFORE loop() starts.
  // Without this, switchLastState[] is all zeros (open) from initialization,
  // and the first portRead in loop() can produce a phantom "just closed" edge
  // on any switch that happens to read as closed due to power-up transients.
  // This was the root cause of the Start button firing a phantom tap on power-up,
  // which corrupted the tap-detection state machine and caused wrong game styles.
  for (int i = 0; i < 4; i++) {
    switchNewState[i] = pShiftRegister->portRead(4 + i);
    switchOldState[i] = switchNewState[i];
  }
  for (byte i = 0; i < NUM_SWITCHES_MASTER; i++) {
    switchLastState[i] = switchClosed(i) ? 1 : 0;
  }
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

  // Update Centipede #2 switch snapshot once per tick.
  // READ EACH PORT TWICE and only accept the result if both reads agree.
  // Flipper hold-power PWM (~4-8 kHz) can corrupt individual I2C portRead()
  // transactions, causing phantom switch closures on random pins. If the two
  // reads disagree, do a third read as a tiebreaker (majority vote). This
  // avoids discarding the entire tick.
  for (int i = 0; i < 4; i++) {
    switchOldState[i] = switchNewState[i];
    unsigned int read1 = pShiftRegister->portRead(4 + i);
    unsigned int read2 = pShiftRegister->portRead(4 + i);
    if (read1 == read2) {
      switchNewState[i] = read1;
    } else {
      // Tiebreaker: third read, majority vote
      unsigned int read3 = pShiftRegister->portRead(4 + i);
      switchNewState[i] = (read1 == read3) ? read1 : read2;
    }
  }

  // Compute all switch edge detection flags (MUST be called after portRead, before any switch processing)
  updateAllSwitchStates();

  // ALWAYS process (every tick, all phases)
  updateDeviceTimers();
  processLampEffectTick(pShiftRegister, lampParm);
  processAudioTick();
  processScoreMotorStartupTick();
  processScoreMotorGameplayTick();
  processKickoutEjectTick();
  processMultiballSaveDispenseTick();
  processModeSaveDispenseTick();
  processModeTick();
  processFlippers();
  processTiltBob();
  processKnockoffButton();
  processVolumeButtons();
  processMidGameStart();
  processGameTimeout();
  processUndetectedDrainCheck();
  processPeriodicScoreSave();
  processReplayKnockerTick();

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

  case PHASE_TILT:
    updatePhaseTilt();
    break;

  default:
    // Temporary attract mode: show screen and check for diagnostic entry
    renderAttractDisplayIfNeeded();

    // Auto-eject any balls in side kickouts (cleanup after games)
    ejectKickoutsIfOccupied();

    // Check for start button (single tap = Enhanced, double tap = Original)
    processStartButton();

    // ***** DEBUG: Right flipper cycles next mode during Attract *******************************************************************************************
    // Tap right flipper to rotate which mode will start next.
    // Writes to EEPROM so startMode() picks it up. Remove when done testing.
    if (switchJustClosedFlag[SWITCH_IDX_FLIPPER_RIGHT_BUTTON]) {
      byte lastMode = EEPROM.read(EEPROM_ADDR_LAST_MODE_PLAYED);
      if (lastMode < MODE_BUMPER_CARS || lastMode > MODE_GOBBLE_HOLE) {
        lastMode = MODE_GOBBLE_HOLE;
      }
      // Advance: the "last played" determines next, so to get BUMPER_CARS next
      // we store GOBBLE_HOLE, etc.
      if (lastMode == MODE_BUMPER_CARS) {
        lastMode = MODE_ROLL_A_BALL;
      } else if (lastMode == MODE_ROLL_A_BALL) {
        lastMode = MODE_GOBBLE_HOLE;
      } else {
        lastMode = MODE_BUMPER_CARS;
      }
      EEPROM.update(EEPROM_ADDR_LAST_MODE_PLAYED, lastMode);

      // Show which mode is NEXT (one after what we just stored)
      const char* nextNames[] = { "", "ROLL-A-BALL", "GOBBLE HOLE", "BUMPER CARS" };
      snprintf(lcdString, LCD_WIDTH + 1, "Next: %s", nextNames[lastMode]);
      lcdPrintRow(4, lcdString);
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

  // ONLY show SLOW LOOP warning during active gameplay.
  // Threshold is 15ms (1.5x the 10ms tick). Brief overruns up to ~12ms are
  // imperceptible to the player and common on scoring events that involve
  // RS-485 sends (~2.5ms each) plus I2C lamp writes (~0.65ms each).
  // Rate-limit to once per second to avoid the feedback loop where the LCD
  // write itself (~3ms) causes the NEXT tick to also trigger a SLOW warning.
  if (gamePhase == PHASE_BALL_IN_PLAY) {
    unsigned long loopDuration = micros() - loopStartMicros;
    static unsigned long lastSlowDisplayMs = 0;
    if (loopDuration > 15000 && (long)(millis() - lastSlowDisplayMs) >= 1000) {
      lastSlowDisplayMs = millis();
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
    // Always clear edge flags at start of tick
    switchJustClosedFlag[i] = false;
    switchJustOpenedFlag[i] = false;

    // If debounce is active, just count down and skip all edge detection.
    // CRITICAL: Do NOT update switchLastState during debounce. If we tracked
    // the physical state during debounce, a switch that bounced open and back
    // closed within the debounce window would fire a SECOND edge when debounce
    // expires (because switchLastState would show "open" from the bounce).
    // This was the root cause of double "ball saved" audio on the gobble hole.
    if (switchDebounceCounter[i] > 0) {
      switchDebounceCounter[i]--;
      if (switchDebounceCounter[i] == 0) {
        // Debounce just expired: snap switchLastState to current physical state
        // so edge detection resumes from a clean baseline.
        switchLastState[i] = switchClosed(i) ? 1 : 0;
      }
      continue;  // Skip edge detection AND state update
    }

    bool closedNow = switchClosed(i);
    bool wasOpen = (switchLastState[i] == 0);

    // EDGE DETECTION: Switch just closed
    if (closedNow && wasOpen) {
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
    releaseAllFlippers();    return;
  }

  // Additional check: disable if tilted
  if (gameState.tilted) {
    releaseAllFlippers();    return;
  }

  // ***** IMPULSE STYLE: Both flippers fire briefly on EITHER button press *****
  // Press either button: both flippers fire once for IMPULSE_FLIPPER_UP_TICKS (100ms)
  // then drop. No hold. Must release and press again to flip again.
  // NOTE: We use our own tick counter rather than the coil countdown because timeOn
  // controls the full-power-to-hold-power transition (needed for Original/Enhanced),
  // whereas impulse needs a shorter, independently tunable up-time.
  if (gameStyle == STYLE_IMPULSE) {
    bool leftPressed = switchClosed(SWITCH_IDX_FLIPPER_LEFT_BUTTON);
    bool rightPressed = switchClosed(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);
    bool eitherJustClosed = switchJustClosedFlag[SWITCH_IDX_FLIPPER_LEFT_BUTTON]
      || switchJustClosedFlag[SWITCH_IDX_FLIPPER_RIGHT_BUTTON];

    if (eitherJustClosed && !leftFlipperHeld && !rightFlipperHeld) {
      // Fire both flippers
      activateDevice(DEV_IDX_FLIPPER_LEFT);
      activateDevice(DEV_IDX_FLIPPER_RIGHT);
      leftFlipperHeld = true;
      rightFlipperHeld = true;
      impulseFlipperTicksRemaining = IMPULSE_FLIPPER_UP_TICKS;
    }

    // Count down and release when timer expires
    if (leftFlipperHeld && impulseFlipperTicksRemaining > 0) {
      impulseFlipperTicksRemaining--;
      if (impulseFlipperTicksRemaining == 0) {
        releaseDevice(DEV_IDX_FLIPPER_LEFT);
        releaseDevice(DEV_IDX_FLIPPER_RIGHT);
        leftFlipperHeld = false;
        rightFlipperHeld = false;
      }
    }

    // Also release if both buttons are released (so player can't hold them up)
    if (!leftPressed && !rightPressed) {
      if (leftFlipperHeld) {
        releaseDevice(DEV_IDX_FLIPPER_LEFT);
        releaseDevice(DEV_IDX_FLIPPER_RIGHT);
        leftFlipperHeld = false;
        rightFlipperHeld = false;
        impulseFlipperTicksRemaining = 0;
      }
    }
    return;
  }

  // ***** ORIGINAL / ENHANCED STYLE: Normal independent holdable flippers *****
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

// ***** TILT BOB PROCESSING *****
// Called every tick from loop(). Checks for tilt bob closures and handles
// warnings (Enhanced) or full tilt (Original/Impulse/Enhanced 2nd warning).
// Per Overview Section 4.6: tilt bob is only recognized when ballsInPlay > 0.
void processTiltBob() {
  // Only process during active gameplay phases
  if (gamePhase != PHASE_BALL_IN_PLAY && gamePhase != PHASE_BALL_READY) {
    return;
  }

  // Already tilted - nothing more to do
  if (gameState.tilted) {
    return;
  }

  // Only recognize tilt bob when a ball is actually on the playfield
  if (ballsInPlay == 0) {
    return;
  }

  // Check for tilt bob closure
  if (!switchJustClosedFlag[SWITCH_IDX_TILT_BOB]) {
    return;
  }

  // Enforce settle time after Enhanced warning to let tilt bob stop swinging.
  // Without this, a single nudge causes the bob to bounce and register both
  // a warning AND a full tilt almost simultaneously.
  if (tiltBobSettleUntilMs != 0 && (long)(millis() - tiltBobSettleUntilMs) < 0) {
    return;
  }
  tiltBobSettleUntilMs = 0;  // Clear expired deadline

  // Tilt bob closed with ball(s) in play
  gameState.tiltWarnings++;

  if (gameStyle == STYLE_ENHANCED && gameState.tiltWarnings == 1) {
    // Enhanced first warning: audio warning + visual flash, resume play
    audioDuckMusicForVoice(true);
    byte randomIdx = (byte)random(0, NUM_COM_TILT_WARNING);
    unsigned int trackNum = pgmReadComTrackNum(&comTracksTiltWarning[randomIdx]);
    byte trackLenTenths = pgmReadComLengthTenths(&comTracksTiltWarning[randomIdx]);
    pTsunami->trackPlayPoly((int)trackNum, 0, false);
    audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
    audioCurrentComTrack = trackNum;
    audioComEndMillis = millis() + ((unsigned long)trackLenTenths * 100UL);

    // Flash all playfield lamps as a visual tilt warning.
    // Per Section 14.7.1: cancel any running effect before starting a new one.
    // The flash is non-blocking (~600ms total) so gameplay continues normally.
    // If the player triggers a full tilt during the flash, handleFullTilt()
    // calls cancelLampEffect() which safely restores lamps before the blackout.
    cancelLampEffect(pShiftRegister);
    startLampEffect(LAMP_EFFECT_FLASH_ALL, 120, 0);

    // Start settle timer so the bob can stop swinging before we accept another closure
    tiltBobSettleUntilMs = millis() + TILT_BOB_SETTLE_MS;

    return;
  }

  // Full tilt: 1st tilt in Original/Impulse, or 2nd tilt in Enhanced
  handleFullTilt();
}

// ***** FULL TILT HANDLER *****
// Called when a full tilt is triggered. Sets tilted flag, disables scoring devices,
// ejects locked balls, starts TILT lamp flash, and transitions to PHASE_TILT.
void handleFullTilt() {
  gameState.tilted = true;

  // Cancel ball save (no saves after tilt)
  ballSave.endMs = 0;

  // Cancel any active mode
  if (modeState.active) {
    modeState.active = false;
    modeState.currentMode = MODE_NONE;
    modeTimerStarted = false;
    modeIntroEndMs = 0;
    modeSaveDispensePending = false;
    modeSaveWaitingForLift = false;
    modeSaveDispenseMs = 0;
  }

  // Cancel multiball
  inMultiball = false;
  multiballPending = false;
  multiballSaveEndMs = 0;
  multiballSavesRemaining = 0;
  multiballSaveDispensePending = false;
  multiballSaveWaitingForLift = false;

  // Stop all shakers
  stopAllShakers();
  cancelPendingComTrack();

  // Flippers are disabled automatically by processFlippers() checking gameState.tilted.
  // Release them immediately here for responsiveness.
  releaseAllFlippers();

  // Cancel any running lamp effect before writing lamp state directly.
  // This restores the pre-effect lamp state so the tilt blackout starts from
  // a clean baseline, and prevents the effect engine from overwriting the
  // tilt blackout on its next tick.
  cancelLampEffect(pShiftRegister);

  // Turn off ALL playfield lamps INCLUDING GI for a dramatic total blackout.
  // GI stays off throughout PHASE_TILT while the ball(s) drain in the dark.
  // GI is restored when the tilt phase exits (next ball or game over).
  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    pShiftRegister->digitalWrite(lampParm[i].pinNum, HIGH);  // All lamps off
  }

  // Eject any balls locked in side kickouts (all styles, per Overview 4.6).
  // IMPORTANT: Add the locked balls back to ballsInPlay BEFORE zeroing ballsLocked.
  // When a ball was locked by handleSwitchKickout(), ballsInPlay was decremented.
  // Those balls are now being ejected back onto the playfield and must drain before
  // updatePhaseTilt() can exit. Without this, the exit condition (ballsInPlay == 0)
  // fires too early, advancing to the next ball while ejected balls are still draining.
  ballsInPlay += ballsLocked;
  ballsLocked = 0;
  kickoutLockedMask = 0;  // All kickouts are now empty
  if (switchClosed(SWITCH_IDX_KICKOUT_LEFT)) {
    activateDevice(DEV_IDX_KICKOUT_LEFT);
  }
  if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT)) {
    activateDevice(DEV_IDX_KICKOUT_RIGHT);
  }

  // Turn on TILT lamp (flashing) and start flash timer
  pMessage->sendMAStoSLVTiltLamp(true);
  tiltLampFlashStartMs = millis();
  tiltLampCurrentlyOn = true;

  // Enhanced style: play buzzer + random tilt announcement
  if (gameStyle == STYLE_ENHANCED) {
    // Stop current audio
    if (audioCurrentComTrack != 0) {
      pTsunami->trackStop(audioCurrentComTrack);
      audioCurrentComTrack = 0;
      audioComEndMillis = 0;
    }
    if (audioCurrentSfxTrack != 0) {
      pTsunami->trackStop(audioCurrentSfxTrack);
      audioCurrentSfxTrack = 0;
    }
    audioDuckMusicForVoice(true);

    // Play buzzer SFX
    pTsunami->trackPlayPoly(TRACK_TILT_BUZZER, 0, false);
    audioApplyTrackGain(TRACK_TILT_BUZZER, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);

    // Play random tilt voice announcement after brief pause
    delay(500);
    byte randomIdx = (byte)random(0, NUM_COM_TILT);
    unsigned int trackNum = pgmReadComTrackNum(&comTracksTilt[randomIdx]);
    byte trackLenTenths = pgmReadComLengthTenths(&comTracksTilt[randomIdx]);
    pTsunami->trackPlayPoly((int)trackNum, 0, false);
    audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
    audioCurrentComTrack = trackNum;
    audioComEndMillis = millis() + ((unsigned long)trackLenTenths * 100UL);
  }

  // Transition to PHASE_TILT to wait for all balls to drain
  gamePhase = PHASE_TILT;
}

// ***** TILT PHASE HANDLER *****
// Waits for all balls to drain after a full tilt. No scoring, no flippers.
// Monitors drain switches to decrement ballsInPlay. Also handles TILT lamp flashing
// and kickout ejection (in case a ball lands in a kickout while draining).
void updatePhaseTilt() {
  // Track ball-in-lift switch so we can detect when all balls have drained.
  // Without this, ballInLift could be stuck true from PHASE_BALL_READY entry,
  // preventing the exit condition (ballsInPlay == 0 && !ballInLift) from ever firing.
  bool ballInLiftNow = switchClosed(SWITCH_IDX_BALL_IN_LIFT);
  if (!ballInLiftNow && ballInLift) {
    ballInLift = false;
  } else if (ballInLiftNow && !ballInLift) {
    ballInLift = true;
  }

  // Flash TILT lamp for about 4 seconds (toggle every 250ms)
  if (tiltLampFlashStartMs != 0) {
    unsigned long elapsed = millis() - tiltLampFlashStartMs;
    if (elapsed >= TILT_LAMP_FLASH_MS) {
      // Done flashing - leave TILT lamp on solid
      pMessage->sendMAStoSLVTiltLamp(true);
      tiltLampFlashStartMs = 0;
      tiltLampCurrentlyOn = true;
    } else {
      // Toggle based on elapsed time
      bool shouldBeOn = ((elapsed / TILT_LAMP_TOGGLE_MS) % 2) == 0;
      if (shouldBeOn != tiltLampCurrentlyOn) {
        pMessage->sendMAStoSLVTiltLamp(shouldBeOn);
        tiltLampCurrentlyOn = shouldBeOn;
      }
    }
  }

  // Eject any balls that land in kickouts while draining
  ejectKickoutsIfOccupied();

  // Monitor drain switches for ball drains (decrement ballsInPlay).
  // Count EACH drain switch independently so that two balls draining on
  // the same tick through different switches are both counted. The original
  // code used a single if/else-if chain which only decremented once per tick,
  // causing ballsInPlay to never reach 0 if two balls drained simultaneously
  // during a multiball tilt.
  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_LEFT]) {
    if (ballsInPlay > 0) ballsInPlay--;
  }
  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_CENTER]) {
    if (ballsInPlay > 0) ballsInPlay--;
  }
  if (switchJustClosedFlag[SWITCH_IDX_DRAIN_RIGHT]) {
    if (ballsInPlay > 0) ballsInPlay--;
  }
  if (switchJustClosedFlag[SWITCH_IDX_GOBBLE]) {
    if (ballsInPlay > 0) ballsInPlay--;
  }

  // Exit condition: all balls drained and no ball in lift
  if (ballsInPlay == 0 && !ballInLift) {

    // Stop TILT lamp flash if still going, and turn TILT lamp OFF
    tiltLampFlashStartMs = 0;
    pMessage->sendMAStoSLVTiltLamp(false);

    // Enhanced: stop any remaining tilt audio, un-duck music
    if (gameStyle == STYLE_ENHANCED) {
      if (audioCurrentComTrack != 0) {
        pTsunami->trackStop(audioCurrentComTrack);
        audioCurrentComTrack = 0;
        audioComEndMillis = 0;
      }
      audioDuckMusicForVoice(false);
    }

    // Advance to next ball or game over
    if (gameState.currentBall < MAX_BALLS) {
      gameState.currentBall++;
      ballSaveUsed = false;

      // Restore ALL playfield lamps that were blanked by handleFullTilt(),
      // including GI which was left off for the dramatic tilt blackout.
      // setInitialGameplayLamps() restores bumpers, RED, HATs, WHITEs,
      // GOBBLEs, SPECIAL, and Spots Number lamps but does not touch GI.
      setGILamps(true);
      setInitialGameplayLamps();

      gamePhase = PHASE_STARTUP;
      startupSubState = STARTUP_DISPENSE_BALL;
      startupTickCounter = 0;
    } else {
      // Last ball - game over
      // processBallLost() -> teardownGame() -> setAttractLamps() restores GI.
      processBallLost(DRAIN_TYPE_NONE);  // Tilt: no special drain audio
    }
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

  // *** STAGE 1: First tap detection ***
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON] && !startWaitingForSecondTap
    && !startWaitingForThirdTap && gameStyle == STYLE_NONE) {
    startFirstPressMs = millis();
    startWaitingForSecondTap = true;
    return;
  }

  // *** STAGE 2: Second tap detection (within 500ms of first) ***
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON] && startWaitingForSecondTap) {
    unsigned long elapsed = millis() - startFirstPressMs;
    if (elapsed < START_STYLE_DETECT_WINDOW_MS) {
      // Double-tap detected. Now wait for potential third tap.
      startWaitingForSecondTap = false;
      startWaitingForThirdTap = true;
      startSecondPressMs = millis();
      return;
    }
  }

  // *** STAGE 3: Third tap detection (within 500ms of second) ***
  if (switchJustClosedFlag[SWITCH_IDX_START_BUTTON] && startWaitingForThirdTap) {
    unsigned long elapsed = millis() - startSecondPressMs;
    if (elapsed < START_STYLE_DETECT_WINDOW_MS) {
      // Triple-tap confirmed -> Impulse mode
      startWaitingForThirdTap = false;
      gameStyle = STYLE_IMPULSE;
      startGameIfCredits();
      return;
    }
  }

  // *** STAGE 2 TIMEOUT: No second tap within 500ms -> Enhanced ***
  if (startWaitingForSecondTap) {
    unsigned long elapsed = millis() - startFirstPressMs;
    if (elapsed >= START_STYLE_DETECT_WINDOW_MS) {
      startWaitingForSecondTap = false;
      gameStyle = STYLE_ENHANCED;
      startGameIfCredits();
      return;
    }
  }

  // *** STAGE 3 TIMEOUT: No third tap within 500ms -> Original ***
  if (startWaitingForThirdTap) {
    unsigned long elapsed = millis() - startSecondPressMs;
    if (elapsed >= START_STYLE_DETECT_WINDOW_MS) {
      startWaitingForThirdTap = false;
      gameStyle = STYLE_ORIGINAL;
      startGameIfCredits();
      return;
    }
  }
}

// Common helper: check credits, deduct, play style-specific audio, enter PHASE_STARTUP.
// Called once from processStartButton() after gameStyle has been determined.
void startGameIfCredits() {
  bool creditsAvailable = false;
  if (!queryCreditStatus(&creditsAvailable)) {
    return;  // RS485 error -- criticalError already called
  }

  if (!creditsAvailable) {
    // No credits: Enhanced plays rejection audio; Original/Impulse stay silent
    if (gameStyle == STYLE_ENHANCED) {
      pTsunami->trackPlayPoly(TRACK_START_REJECT_HORN, 0, false);
      audioApplyTrackGain(TRACK_START_REJECT_HORN, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
      delay(800);

      byte randomIdx = (byte)random(0, NUM_COM_START_REJECT);
      unsigned int trackNum = pgmReadComTrackNum(&comTracksStartReject[randomIdx]);
      byte trackLength = pgmReadComLengthTenths(&comTracksStartReject[randomIdx]);
      pTsunami->trackPlayPoly((int)trackNum, 0, false);
      audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = trackNum;
      audioComEndMillis = millis() + ((unsigned long)trackLength * 100UL);
      delay(2000);
    } else {
      delay(2000);
    }
    gameStyle = STYLE_NONE;
    return;
  }

  // Credits available -- deduct immediately for ALL styles
  pMessage->sendMAStoSLVCreditDec();
  creditDeducted = true;
  gameState.numPlayers = 1;
  gameState.currentPlayer = 1;
  gameState.currentBall = 1;

  // Load ball save duration from EEPROM (1-30 seconds, default 15 if invalid)
  ballSaveSeconds = diagReadBallSaveTimeFromEEPROM();
  if (ballSaveSeconds == 0) {
    ballSaveSeconds = BALL_SAVE_DEFAULT_SECONDS;
  }

  // Enhanced: play SCREAM immediately for shock value
  if (gameStyle == STYLE_ENHANCED) {
    pTsunami->stopAllTracks();
    pTsunami->trackPlayPoly(TRACK_START_SCREAM, 0, false);
    audioApplyTrackGain(TRACK_START_SCREAM, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
  }

  // Transition to PHASE_STARTUP for ball recovery
  gamePhase = PHASE_STARTUP;
  startupSubState = STARTUP_RECOVER_INIT;
  startupTickCounter = 0;
  resetGameTimeoutDeadline();
  lastEepromSaveMs = millis();  // Start periodic save timer from game start
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
// NOTE: The score motor tick processing was moved to processScoreMotorStartupTick()
// in loop() so it continues running after PHASE_STARTUP transitions to PHASE_BALL_READY.
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

    // Original/Impulse: Start Score Motor startup sequence (runs concurrently with ball recovery).
    // Per Overview Section 5.3, the 1st quarter-turn fires relay resets, deducts credit, and
    // opens ball tray. Credit and tray are already handled, so we just need the motor sound
    // and relay reset firings here. The score reset is sent at the 2nd quarter-turn (T+882ms)
    // by the tick processor at the top of updatePhaseStartup().
    if (gameStyle == STYLE_ORIGINAL || gameStyle == STYLE_IMPULSE) {
      activateDevice(DEV_IDX_MOTOR_SCORE);
      activateDevice(DEV_IDX_RELAY_RESET);
      scoreMotorStartMs = millis();
      scoreMotorSlotRelayReset2 = false;
      scoreMotorSlotScoreReset = false;
    }

    // Enhanced style: play "Hey Gang" during ball recovery (SCREAM already played in processStartButton)
    if (gameStyle == STYLE_ENHANCED) {
      // Brief delay to let SCREAM sound start before overlapping with "Hey Gang"
      delay(1500);
      pTsunami->trackPlayPoly(TRACK_START_HEY_GANG, 0, false);
      audioApplyTrackGain(TRACK_START_HEY_GANG, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_START_HEY_GANG;
      audioComEndMillis = millis() + 3500UL;

      // If a ball is already at the lift base, tell the player immediately rather than
      // waiting for the 7-second ball recovery timeout to enter PHASE_BALL_SEARCH.
      // Play it right after "Hey Gang" so the two announcements are back-to-back.
      if (ballRecoveryBallInLiftAtStart) {
        delay(3500);  // Wait for "Hey Gang" to finish
        pTsunami->trackPlayPoly(TRACK_BALL_IN_LIFT, 0, false);
        audioApplyTrackGain(TRACK_BALL_IN_LIFT, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = TRACK_BALL_IN_LIFT;
        audioComEndMillis = millis() + ((unsigned long)pgmReadComLengthTenths(&comTracksBallMissing[1]) * 100UL);
      }

      lcdClear();
      lcdPrintRow(1, "Starting Enhanced");
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
    break;
  }

  case STARTUP_RECOVER_WAIT:
  {
    // Continuously eject any balls that land in kickouts
    ejectKickoutsIfOccupied();

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
          break;
        }
      }
    } else {
      // Switch opened - reset stability tracking (ball was rolling over switch)
      ballRecoverySwitchWasStable = false;
    }

    // Timeout check - if balls haven't settled after recovery timeout.
    // Original/Impulse: Use a shorter timeout (4 seconds) since the Score Motor
    // startup sequence only takes 3528ms. The full 7-second timeout caused a
    // noticeable delay before flippers were enabled (flippers only work in
    // PHASE_BALL_IN_PLAY or PHASE_BALL_READY, and we stay in PHASE_STARTUP
    // until this timeout fires).
    // Enhanced: Keep the full 7 seconds since STARTUP_RECOVER_INIT has up to
    // 5 seconds of blocking audio delays before ballRecoveryStartMs is set.
    unsigned long recoveryTimeout = BALL_RECOVERY_TIMEOUT_MS;
    if (gameStyle == STYLE_ORIGINAL || gameStyle == STYLE_IMPULSE) {
      recoveryTimeout = 4000;  // 4 seconds: enough for Score Motor (3528ms) + margin
    }

    if (millis() - ballRecoveryStartMs >= recoveryTimeout) {

      // ***** ORIGINAL/IMPULSE: Skip ball search -- start game with ball already out *****
      // The original EM game had no ball search. If a ball was already on the playfield
      // (at the lift base or in the shooter lane) when the player pressed Start, the game
      // simply started with that ball as Ball 1. The remaining 4 balls in the trough would
      // be dispensed as Balls 2-5. We replicate that behavior here: proceed directly to
      // STARTUP_RECOVER_CONFIRMED (which handles score reset, lamps, etc.), then skip
      // STARTUP_DISPENSE_BALL and jump straight to PHASE_BALL_IN_PLAY with ballsInPlay=1.
      // The stray ball will score normally when it hits targets, and drain normally when
      // it reaches a drain switch.
      if (gameStyle == STYLE_ORIGINAL || gameStyle == STYLE_IMPULSE) {
        // Let the Score Motor startup sequence continue running in the background
        // (processScoreMotorStartupTick handles it independently in loop).

        // Do the same work as STARTUP_RECOVER_CONFIRMED: score reset, lamps, etc.
        if (!scoreMotorSlotScoreReset) {
          sendScoreReset();
          scoreMotorSlotScoreReset = true;
        }
        setInitialGameplayLamps();

        // Initialize per-ball state (same as STARTUP_DISPENSE_BALL does)
        gameState.tilted = false;
        gameState.tiltWarnings = 0;

        // The stray ball IS Ball 1 -- it is already on the playfield.
        // Set up state as if the ball has been dispensed and left the lift.
        ballsInPlay = 1;
        ballInLift = false;
        ballHasLeftLift = true;  // Stray ball is already on the playfield
        gameState.hasHitSwitch = false;
        shootReminderDeadlineMs = 0;
        hillClimbStarted = false;
        ballSettledInLift = false;
        ball1SequencePlayed = false;
        prevBallInLift = false;
        tiltBobSettleUntilMs = 0;
        ballSave.endMs = 0;
        inMultiball = false;

        // Go directly to PHASE_BALL_IN_PLAY (not BALL_READY, since the ball is not
        // sitting at the lift in a known state -- it could be anywhere on the playfield).
        gamePhase = PHASE_BALL_IN_PLAY;
        // Start undetected drain inactivity timer
        undetectedDrainDeadlineMs = millis() + UNDETECTED_DRAIN_TIMEOUT_MS;

        // Update LCD status line
        const char* styleStr = (gameStyle == STYLE_IMPULSE) ? "Imp" : "Org";
        snprintf(lcdString, LCD_WIDTH + 1, "%s P%d Ball %d",
          styleStr,
          gameState.currentPlayer,
          gameState.currentBall);
        lcdPrintRow(4, lcdString);
        return;
      }

      // ***** ENHANCED: Enter ball search (unchanged) *****

      // Turn off Score Motor if still running (ball search will handle its own timing)
      if (scoreMotorStartMs != 0) {
        releaseDevice(DEV_IDX_MOTOR_SCORE);
        scoreMotorStartMs = 0;
      }

      // Leave tray open (ball search will manage it)
      gamePhase = PHASE_BALL_SEARCH;
      ballSearchSubState = BALL_SEARCH_INIT;

      // Enhanced style: play contextual "ball missing" audio based on LIVE switch state
      if (gameStyle == STYLE_ENHANCED) {
        playBallMissingAudio();
      }
      return;
    }
    break;
  }

  case STARTUP_RECOVER_CONFIRMED:
  {
    // Ball tray stays OPEN for the entire game in all styles.
    // It is already open from STARTUP_RECOVER_INIT, so nothing to do here.
    // The tray is closed by teardownGame() at game end (game over, timeout,
    // or mid-game Start).
// 
    // Enhanced style: credit already deducted in processStartButton().
    // Audio (SCREAM + Hey Gang) already played. Just send score reset.
    if (gameStyle == STYLE_ENHANCED) {
      // Don't stop "Hey Gang" -- let it finish naturally.
      // Just clear COM tracking so ducking logic doesn't interfere.
      audioCurrentComTrack = 0;
      audioComEndMillis = 0;
      sendScoreReset();
    }

    // Original/Impulse: The score motor tick processor (at the top of updatePhaseStartup)
    // sends the score reset at T+882ms. If ball recovery finished before that (unlikely
    // but possible since recovery takes 1000ms stability + whatever settling time), send
    // the score reset now so Slave starts resetting immediately.
    if (gameStyle == STYLE_ORIGINAL || gameStyle == STYLE_IMPULSE) {
      if (!scoreMotorSlotScoreReset) {
        sendScoreReset();
        scoreMotorSlotScoreReset = true;
      }
      // Score Motor continues running in the background; the tick processor at the top
      // of updatePhaseStartup() will turn it off at T+3528ms.
    }

    // Set all gameplay lamps to reflect initial EM emulation state.
    // initGameState() already randomized selectionUnitPosition and set bumperLitMask,
    // whiteLitMask, etc., but setAttractLamps() turned off all non-GI/non-bumper lamps.
    // Now that the game is officially starting, light the correct lamps.
    setInitialGameplayLamps();

    // Move to ball dispense
    startupSubState = STARTUP_DISPENSE_BALL;
    startupTickCounter = 0;
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

    // Wait for any pending deferred COM track (e.g. snarky drain comment from previous ball)
    // to fire and finish before playing the "Ball N" announcement. Without this, "Ball N"
    // would overwrite the snarky comment before the player hears it.
    if (gameStyle == STYLE_ENHANCED) {
      // If a deferred track has not fired yet, fire it now (its delay has likely elapsed
      // by the time we reach DISPENSE_BALL, but if not, fire it immediately).
      if (pendingComTrack != 0) {
        firePendingComTrack();
      }
      // If a COM track is playing (either the just-fired deferred track or one that
      // was already playing), wait for it to finish.
      if (audioCurrentComTrack != 0 && audioComEndMillis > 0) {
        long remaining = (long)(audioComEndMillis - millis());
        if (remaining > 0) {
          delay((unsigned long)remaining + 200UL);
        }
        audioCurrentComTrack = 0;
        audioComEndMillis = 0;
        audioDuckMusicForVoice(false);
      }
    }

    // Enhanced style: announce ball number for balls 2-5, with Ball 5 follow-up comment
    if (gameStyle == STYLE_ENHANCED && gameState.currentBall >= 2) {
      // Duck music so the ball number announcement is audible
      audioDuckMusicForVoice(true);

      // Play "Ball 2", "Ball 3", "Ball 4", or "Ball 5"
      unsigned int ballTrack = TRACK_BALL_BASE + (gameState.currentBall - 1);
      pTsunami->trackPlayPoly((int)ballTrack, 0, false);
      audioApplyTrackGain(ballTrack, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);

      // Look up length from comTracksBall[] array (index 0 = Ball 1, index 4 = Ball 5)
      byte ballTrackLenTenths = pgmReadComLengthTenths(&comTracksBall[gameState.currentBall - 1]);
      unsigned long ballTrackMs = (unsigned long)ballTrackLenTenths * 100UL;

      if (gameState.currentBall == 5) {
        // Ball 5: follow with a random "last ball" comment (tracks 531-540)
        // Wait for "Ball 5" to finish, then play the follow-up
        delay(ballTrackMs + 200UL);  // Brief gap between announcements

        byte randomIdx = (byte)random(0, NUM_COM_BALL5_COMMENT);
        unsigned int commentTrack = pgmReadComTrackNum(&comTracksBall5Comment[randomIdx]);
        byte commentLenTenths = pgmReadComLengthTenths(&comTracksBall5Comment[randomIdx]);
        pTsunami->trackPlayPoly((int)commentTrack, 0, false);
        audioApplyTrackGain(commentTrack, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = commentTrack;
        audioComEndMillis = millis() + ((unsigned long)commentLenTenths * 100UL);
      } else {
        // Balls 2-4: just track the ball number announcement as the active COM track
        audioCurrentComTrack = ballTrack;
        audioComEndMillis = millis() + ballTrackMs;
      }
    }

    // Fire ball trough release coil (230ms pulse) -- but only if lift is empty.
    // If a ball is already at the lift base (e.g. a previous ball rolled back),
    // skip the dispense and treat the existing ball as the newly fed ball.
    if (!switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);
    }

    // Record when we dispensed the ball (for timeout tracking)
    startupBallDispenseMs = millis();

    // Move to wait state
    startupSubState = STARTUP_WAIT_FOR_LIFT;
    startupTickCounter = 0;
    break;
  }

  case STARTUP_WAIT_FOR_LIFT:
  {
    // Check if ball has reached lift
    if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      // Success! Ball is in lift
      startupSubState = STARTUP_COMPLETE;
      startupTickCounter = 0;
      break;
    }

    // Periodic status update on LCD so we can see what the switch is doing
    // if the timeout ever fires again. Show elapsed time and raw switch state.
    if (startupTickCounter % 50 == 0) {  // Every 500ms
      unsigned long elapsedMs = millis() - startupBallDispenseMs;
      snprintf(lcdString, LCD_WIDTH + 1, "Lift wait %lums", elapsedMs);
      lcdPrintRow(3, lcdString);
    }

    // Check for timeout (3 seconds)
    if (millis() - startupBallDispenseMs >= BALL_TROUGH_TO_LIFT_TIMEOUT_MS) {
      // Log diagnostic info so we can see what happened
      unsigned long elapsedMs = millis() - startupBallDispenseMs;
      bool liftNow = switchClosed(SWITCH_IDX_BALL_IN_LIFT);
      bool troughNow = allBallsInTrough();
      snprintf(lcdString, LCD_WIDTH + 1, "L=%d T=%d %lums",
        (int)liftNow, (int)troughNow, elapsedMs);
      lcdPrintRow(3, lcdString);
      Serial.print(F("LIFT TIMEOUT L="));
      Serial.print(liftNow);
      Serial.print(F(" T="));
      Serial.print(troughNow);
      Serial.print(F(" B"));
      Serial.println(gameState.currentBall);

      // Instead of halting with criticalError, fall back to full ball recovery.
      // A ball may be stuck between the trough and lift (e.g. after a multiball
      // save double-dispense race condition), or a kickout may still hold a ball.
      // STARTUP_RECOVER_INIT opens the tray, ejects kickouts, and waits for
      // the 5-balls-in-trough switch, giving the system a chance to self-heal.
      if (scoreMotorStartMs != 0) {
        releaseDevice(DEV_IDX_MOTOR_SCORE);
        scoreMotorStartMs = 0;
      }
      startupSubState = STARTUP_RECOVER_INIT;
      startupTickCounter = 0;
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
    const char* styleStr = "Org";
    if (gameStyle == STYLE_ENHANCED) styleStr = "Enh";
    else if (gameStyle == STYLE_IMPULSE) styleStr = "Imp";
    snprintf(lcdString, LCD_WIDTH + 1, "%s P%d Ball %d",
      styleStr,
      gameState.currentPlayer,
      gameState.currentBall);
    lcdPrintRow(4, lcdString);
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

    // Refund the credit that was deducted when the player pressed Start.
    // The game never actually started, so the player should not lose a credit.
    if (creditDeducted) {
      pMessage->sendMAStoSLVCreditInc(1);
      creditDeducted = false;
    }

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

      playBallMissingAudio();
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
    ejectKickoutsIfOccupied();

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
  // ***** ENHANCED STYLE, BALL 1 ONLY: Wait for ball to settle before monitoring for hill climb *****
  // Only run hill climb on Ball 1 when it has never been played (first Ball 1 of the game).
  // After a ball save, ball1SequencePlayed is true so this block is skipped for the replacement ball.
  if (gameStyle == STYLE_ENHANCED && gameState.currentBall == 1
    && !hillClimbStarted && !ball1SequencePlayed) {
    bool ballInLiftNow = switchClosed(SWITCH_IDX_BALL_IN_LIFT);

    // ***** STEP 1: Wait for ball to ARRIVE (rising edge) *****
    if (!ballSettledInLift) {
      // Detect RISING EDGE (switch just closed)
      if (ballInLiftNow && !prevBallInLift) {
        ballSettledInLift = true;
      }

      prevBallInLift = ballInLiftNow;
      return;  // Wait for ball to settle
    }

    // ***** STEP 2: Wait for ball to LEAVE (falling edge) *****
    if (ballInLiftNow && !prevBallInLift) {
      // Ball just re-arrived after being removed - NOT the launch we're looking for
    }

    if (!ballInLiftNow && prevBallInLift) {
      // FALLING EDGE: Ball just left the lift - START HILL CLIMB
      hillClimbStarted = true;

      // Start lift loop audio (looping)
      pTsunami->trackPlayPoly(TRACK_START_LIFT_LOOP, 0, false);
      pTsunami->trackLoop(TRACK_START_LIFT_LOOP, true);
      audioApplyTrackGain(TRACK_START_LIFT_LOOP, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
      audioCurrentSfxTrack = TRACK_START_LIFT_LOOP;

      audioLiftLoopStartMillis = millis();

      analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_HILL_CLIMB);
    }

    prevBallInLift = ballInLiftNow;
  }

  // ***** ENHANCED STYLE: "Shoot the ball" reminder after HILL_CLIMB_TIMEOUT_MS of inactivity *****
  // If the player hasn't hit any target within the timeout, play a contextual reminder.
  // Repeats every HILL_CLIMB_TIMEOUT_MS until the player scores (exits PHASE_BALL_READY).
  if (gameStyle == STYLE_ENHANCED && shootReminderDeadlineMs != 0
    && (long)(millis() - shootReminderDeadlineMs) >= 0) {
    // Duck music so the reminder is audible
    audioDuckMusicForVoice(true);

    if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      // Ball is sitting in the lift - tell player to push the lift rod
      pTsunami->trackPlayPoly(TRACK_SHOOT_LIFT_ROD, 0, false);
      audioApplyTrackGain(TRACK_SHOOT_LIFT_ROD, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_SHOOT_LIFT_ROD;
      audioComEndMillis = millis() + ((unsigned long)pgmReadComLengthTenths(&comTracksShoot[0]) * 100UL);
    } else {
      // Ball has left the lift but hasn't hit anything - tell player to shoot
      byte randomIdx = 1 + (byte)random(0, NUM_COM_SHOOT - 1);  // indices 1..9
      unsigned int trackNum = pgmReadComTrackNum(&comTracksShoot[randomIdx]);
      byte trackLenTenths = pgmReadComLengthTenths(&comTracksShoot[randomIdx]);
      pTsunami->trackPlayPoly((int)trackNum, 0, false);
      audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = trackNum;
      audioComEndMillis = millis() + ((unsigned long)trackLenTenths * 100UL);
    }

    // Reset deadline for next reminder
    shootReminderDeadlineMs = millis() + HILL_CLIMB_TIMEOUT_MS;
  }

  // ***** ALL STYLES: Monitor ball-in-lift switch for rollback handling *****
  bool ballInLiftNow = switchClosed(SWITCH_IDX_BALL_IN_LIFT);
  if (!ballInLiftNow && ballInLift) {
    // Ball just left the lift (player pushed lift rod or ball launched)
    ballInLift = false;
    ballHasLeftLift = true;  // Latch: ball has departed the lift at least once
  } else if (ballInLiftNow && !ballInLift) {
    // Ball just arrived or rolled back to lift
    ballInLift = true;
  }

  // ***** ALL STYLES: Detect ANY playfield switch to transition to BALL_IN_PLAY *****
  // Per Overview Section 12.7: drains are scoring events too. ALL first switch hits
  // (drain or non-drain) set hasHitSwitch and start the ball save timer.
  // Then, if the hit was a drain, the ball save check runs and saves the ball
  // because the timer just started (effectively 0ms elapsed).
  // HOWEVER: Only process drain switches if the ball has actually left the lift.
  // If ballInLift is true, no ball is on the playfield, so any drain closure
  // is trough noise (balls rolling over drain switches on their way to the trough).
  bool wasDrain = false;
  byte hitSwitch = checkPlayfieldSwitchHit(&wasDrain);
  if (hitSwitch != 0xFF) {
    // Ignore drain switches while ball has never left the lift (trough noise).
    // Once the ball has departed the lift at least once (ballHasLeftLift == true),
    // drain closures are always real events -- even if the ball rolled back to the
    // lift and ballInLift is true again. This prevents a drain from being silently
    // suppressed when a trough ball is resting on a drain switch at the moment
    // onEnterBallReady() snaps switchLastState, and the active ball later drains
    // through the same switch after hitting a target.
    if (wasDrain && !ballHasLeftLift) {
      // DEBUG: Show when a drain is suppressed due to ball never having left lift
      snprintf(lcdString, LCD_WIDTH + 1, "DRN SUPPR HL=%d d=%u",
        (int)ballHasLeftLift, switchDebounceCounter[hitSwitch]);
      lcdPrintRow(3, lcdString);
      return;
    }

    // Mark ball as in play and count it (whether drain or non-drain)
    gameState.hasHitSwitch = true;
    ballsInPlay++;
    resetGameTimeoutDeadline();
    resetUndetectedDrainDeadline();

    // Only call handleFirstPointScored() for NON-drain hits.
    // Per Overview Section 12.7: a drain is not the right trigger for the hill drop.
    // The ball save timer is started by handleFirstPointScored() for non-drains,
    // or directly here for drains (same timer, same duration).
    if (!wasDrain) {
      handleFirstPointScored();
      // Transition to BALL_IN_PLAY
      gamePhase = PHASE_BALL_IN_PLAY;
      // Start undetected drain inactivity timer
      undetectedDrainDeadlineMs = millis() + UNDETECTED_DRAIN_TIMEOUT_MS;

      // Check if this first non-drain hit should trigger multiball.
      // The multiball check in updatePhaseBallInPlay() only runs on SUBSEQUENT
      // hits (next tick). Without this check here, the first hit after the
      // replacement ball arrives would be scored but NOT trigger multiball,
      // requiring a second hit -- which the player experiences as a missed start.
      if (multiballPending && gameStyle == STYLE_ENHANCED) {
        startMultiball();
      }

      // Dispatch the first hit to its scoring handler. handleFirstPointScored()
      // handles ball tray, hill drop, ball save, and music -- but does NOT
      // score the hit or fire the associated device. Without this dispatch,
      // the edge flag is consumed and the hit is silently lost.
      if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT || hitSwitch == SWITCH_IDX_SLINGSHOT_RIGHT) {
        handleSwitchSlingshot(hitSwitch);
      } else if (hitSwitch >= SWITCH_IDX_BUMPER_S && hitSwitch <= SWITCH_IDX_BUMPER_O) {
        handleSwitchBumper(hitSwitch);
      } else if ((hitSwitch >= SWITCH_IDX_LEFT_SIDE_TARGET_2 && hitSwitch <= SWITCH_IDX_LEFT_SIDE_TARGET_5)
        || (hitSwitch >= SWITCH_IDX_RIGHT_SIDE_TARGET_1 && hitSwitch <= SWITCH_IDX_RIGHT_SIDE_TARGET_5)) {
        handleSwitchSideTarget(hitSwitch);
      } else if (hitSwitch == SWITCH_IDX_HAT_LEFT_TOP || hitSwitch == SWITCH_IDX_HAT_LEFT_BOTTOM
        || hitSwitch == SWITCH_IDX_HAT_RIGHT_TOP || hitSwitch == SWITCH_IDX_HAT_RIGHT_BOTTOM) {
        handleSwitchHat(hitSwitch);
      } else if (hitSwitch == SWITCH_IDX_KICKOUT_LEFT || hitSwitch == SWITCH_IDX_KICKOUT_RIGHT) {
        handleSwitchKickout(hitSwitch);
      }
    } else {
      // Drain as first hit: score the drain BEFORE handling ball-lost logic,
      // exactly as updatePhaseBallInPlay() does for mid-game drains.
      if (hitSwitch == SWITCH_IDX_GOBBLE) {
        handleGobbleHoleScoring();
      } else {
        handleDrainRolloverScoring(hitSwitch);
      }

      // Drain as first hit: start ball save timer directly (same as handleFirstPointScored
      // would do, but without the hill drop sequence).
      // Only start if a save has not already been consumed for this ball number.
      if (gameStyle == STYLE_ENHANCED && !ballSaveUsed) {
        ballSave.endMs = millis() + ((unsigned long)ballSaveSeconds * 1000UL);
      }
      // Record when the saved replacement ball first hit a switch (drain counts too)
      if (ballSaveUsed && savedBallFirstSwitchMs == 0) {
        savedBallFirstSwitchMs = millis();
      }

      // CLEAR the triggering drain's edge flag BEFORE calling handleBallDrain().
      // See the same fix in updatePhaseBallInPlay() for the full explanation.
      switchJustClosedFlag[hitSwitch] = false;

      // Now process the drain. handleBallDrain() will check the timer.
      byte drainType = DRAIN_TYPE_NONE;
      if (hitSwitch == SWITCH_IDX_DRAIN_LEFT)        drainType = DRAIN_TYPE_LEFT;
      else if (hitSwitch == SWITCH_IDX_DRAIN_CENTER) drainType = DRAIN_TYPE_CENTER;
      else if (hitSwitch == SWITCH_IDX_DRAIN_RIGHT)  drainType = DRAIN_TYPE_RIGHT;
      else if (hitSwitch == SWITCH_IDX_GOBBLE)       drainType = DRAIN_TYPE_GOBBLE;
      handleBallDrain(drainType);
      return;  // handleBallDrain handles phase change
    }
  }
}

// ***** FIRST POINT SCORED HANDLER *****
// Called when the first NON-DRAIN playfield switch is hit. Drains as first hit
// are handled directly in updatePhaseBallReady() without calling this function.
void handleFirstPointScored() {
  // gameState.hasHitSwitch already set by caller (updatePhaseBallReady)

  // Record when the saved replacement ball first hit a playfield switch.
  // This is used by processBallLost() to detect quick drains after a ball save.
  // The snarky comment window is measured from this moment (not from when saveBall()
  // was called), so the ball transit time and shooter lane time are excluded.
  if (ballSaveUsed && savedBallFirstSwitchMs == 0) {
    savedBallFirstSwitchMs = millis();
  }

  // Cancel shoot reminder (player scored, no longer needed)
  shootReminderDeadlineMs = 0;

  if (gameStyle == STYLE_ENHANCED) {
    // Ball 1 hill drop sequence: fires ONCE per game. After a ball save on Ball 1,
    // ball1SequencePlayed prevents the hill drop from re-triggering.
    if (gameState.currentBall == 1 && !ball1SequencePlayed) {
      ball1SequencePlayed = true;

      // Stop hill climb audio
      pTsunami->trackStop(TRACK_START_LIFT_LOOP);

      // Play first drop screaming audio (11 seconds)
      audioDuckMusicForVoice(true);
      pTsunami->trackPlayPoly(TRACK_START_DROP, 0, false);
      audioApplyTrackGain(TRACK_START_DROP, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_START_DROP;
      audioComEndMillis = millis() + 11000UL;

      // Ramp up shaker motor to hill drop power and record start time
      analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_HILL_DROP);
      shakerDropStartMs = millis();

    }

    // Start ball save timer ONLY if a save has not already been consumed for this ball number.
    // Once a save is used, the replacement ball does NOT get a new timer.
    if (!ballSaveUsed) {
      ballSave.endMs = millis() + ((unsigned long)ballSaveSeconds * 1000UL);
    }

    // Start primary music track ONLY if not already playing (may be playing after ball save)
    ensureMusicPlaying();
  }
}

// ***** MULTIBALL START *****
// Called when multiballPending is true and the replacement ball hits a non-drain
// playfield switch. Ejects both locked balls, plays multiball announcement,
// and sets ballsInPlay to 3.
void startMultiball() {
  multiballPending = false;
  inMultiball = true;

  // Cancel any pending delayed kickout ejects from previous scoring events.
  // startMultiball() fires both kickout coils immediately below, so any scheduled
  // delayed eject would be a redundant (and potentially confusing) second fire.
  kickoutLeftEjectMs = 0;
  kickoutRightEjectMs = 0;

  // Eject both locked balls
  activateDevice(DEV_IDX_KICKOUT_LEFT);
  activateDevice(DEV_IDX_KICKOUT_RIGHT);
  ballsLocked = 0;
  kickoutLockedMask = 0;  // Both kickouts are now empty

  // The replacement ball is already on the playfield (it just hit a switch).
  // The two ejected balls are now also on the playfield.
  ballsInPlay = 3;

  // Start multiball ball save timer. Any ball that drains within this window
  // is replaced from the trough without interrupting gameplay. This gives the
  // player a grace period since balls often drain very quickly after multiball
  // starts (before the player can react to three balls in play).
  multiballSaveEndMs = millis() + ((unsigned long)ballSaveSeconds * 1000UL);
  multiballSavesRemaining = 0;
  multiballSaveDispensePending = false;
  multiballSaveWaitingForLift = false;

  // Cancel single-ball save (multiball save replaces it)
  ballSave.endMs = 0;

  // Switch to alternate music theme for multiball excitement.
  // Stop the current (primary) music track and start the alternate theme.
  // getActiveMusicTheme() returns the alternate theme because inMultiball is now true.
  audioStopMusic();
  ensureMusicPlaying();

  // Duck music for the multiball announcement
  audioDuckMusicForVoice(true);

  // Play multiball announcement
  byte mbLenTenths = pgmReadComLengthTenths(&comTracksMultiball[3]);  // Index 3 = track 674
  pTsunami->trackPlayPoly((int)TRACK_MULTIBALL_START, 0, false);
  audioApplyTrackGain(TRACK_MULTIBALL_START, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
  audioCurrentComTrack = TRACK_MULTIBALL_START;
  audioComEndMillis = millis() + ((unsigned long)mbLenTenths * 100UL);

  // Turn off kickout lock lamps (balls are no longer locked)
  // updateSpotsNumberLamps() will set them back to the correct Spots Number state
  updateSpotsNumberLamps();

  // Multiball celebration light show: a wave fills up from the drain end (where the
  // kickout balls are ejecting from) to the top, holds with all lamps blazing, then
  // drains back down -- restoring the real gameplay lamp state when complete.
  // Total duration ~2.1s (800ms fill + 240ms pause + 800ms drain + 300ms hold).
  // Per Section 14.7.1: cancel any running effect before starting a new one.
  cancelLampEffect(pShiftRegister);
  startLampEffect(LAMP_EFFECT_FILL_UP_DRAIN_DOWN, 800, 300);

  snprintf(lcdString, LCD_WIDTH + 1, "MULTIBALL!");
  lcdPrintRow(3, lcdString);
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
    }
  }

  // ***** PLAYFIELD SWITCH DETECTION *****
  bool wasDrain = false;
  byte hitSwitch = checkPlayfieldSwitchHit(&wasDrain);

  if (hitSwitch != 0xFF) {
    resetGameTimeoutDeadline();
    resetUndetectedDrainDeadline();

    // Mark that the ball has hit a playfield switch. This is normally set by
    // updatePhaseBallReady() on the first hit, but when the stray-ball shortcut
    // (Original/Impulse recovery timeout) jumps directly to PHASE_BALL_IN_PLAY,
    // PHASE_BALL_READY is skipped entirely. Without this, processMidGameStart()
    // never sees gameCommitted==true and the Start button is permanently ignored.
    if (!wasDrain) {
      if (!gameState.hasHitSwitch && ballSaveUsed && savedBallFirstSwitchMs == 0) {
        savedBallFirstSwitchMs = millis();
      }
      gameState.hasHitSwitch = true;
    }

    // ***** ENHANCED: Check if this non-drain hit should trigger multiball *****
    // When multiballPending is true (2 balls locked), the next non-drain playfield
    // switch hit starts multiball: eject both locked balls, set ballsInPlay to 3.
    if (!wasDrain && multiballPending && gameStyle == STYLE_ENHANCED) {
      startMultiball();
      // Do NOT return -- fall through to the normal scoring dispatch below
      // so this hit is scored normally.
    }

    if (wasDrain) {
      // Score the drain event BEFORE handling ball-lost logic.
      // During modes, drain scoring is suppressed: gobble hole drains do NOT
      // call handleGobbleHoleScoring() (which would mutate gobbleCount and
      // light GOBBLE lamps that get restored incorrectly at mode end), and
      // drain rollovers do NOT call handleDrainRolloverScoring() (which would
      // award points outside the mode's scoring rules).
      // EXCEPTION: Gobble Hole Shooting Gallery mode has its own gobble scoring.
      if (modeState.active) {
        if (hitSwitch == SWITCH_IDX_GOBBLE && modeState.currentMode == MODE_GOBBLE_HOLE) {
          handleGobbleHoleModeScoring();
        }
        // All other drains during any mode: no scoring, no lamp changes.
        // handleBallDrain() below handles mode ball replacement.
      } else {
        // Normal (non-mode) drain scoring
        if (hitSwitch == SWITCH_IDX_GOBBLE) {
          handleGobbleHoleScoring();
        } else {
          handleDrainRolloverScoring(hitSwitch);
        }
      }

      // CLEAR the triggering drain's edge flag BEFORE calling handleBallDrain().
      // checkPlayfieldSwitchHit() does NOT consume the flag -- it only reads it.
      // handleBallDrain()'s extraDrains scan checks ALL drain switchJustClosedFlags
      // to catch same-tick multi-drains during multiball. If we do not clear the
      // triggering switch's flag here, the extraDrains scan will find it still set
      // and count it as a SECOND drain, double-decrementing ballsInPlay and
      // double-scoring the drain. This was the root cause of multiball ending
      // prematurely: ballsInPlay dropped from 3 to 1 instead of 3 to 2 on a
      // single drain, because the same drain was counted twice.
      switchJustClosedFlag[hitSwitch] = false;

      // Now handle ball drain (ball save check, ball lost, etc.)
      byte drainType = DRAIN_TYPE_NONE;
      if (hitSwitch == SWITCH_IDX_DRAIN_LEFT)        drainType = DRAIN_TYPE_LEFT;
      else if (hitSwitch == SWITCH_IDX_DRAIN_CENTER) drainType = DRAIN_TYPE_CENTER;
      else if (hitSwitch == SWITCH_IDX_DRAIN_RIGHT)  drainType = DRAIN_TYPE_RIGHT;
      else if (hitSwitch == SWITCH_IDX_GOBBLE)       drainType = DRAIN_TYPE_GOBBLE;
      handleBallDrain(drainType);
    } else {
      // ***** NON-DRAIN SCORING DISPATCH *****
      // During an active mode, ALL non-drain switches are routed to the mode
      // scoring handler. Only mode-specific targets score; everything else is
      // silently ignored (per Overview Section 14.7.1 "Scoring During Modes").
      if (modeState.active) {
        handleModeScoringSwitch(hitSwitch);
      }
      // Normal (non-mode) scoring dispatch
      else {
        // Slingshots
        if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT || hitSwitch == SWITCH_IDX_SLINGSHOT_RIGHT) {
          handleSwitchSlingshot(hitSwitch);
        }
        // Bumpers (S=10, C=11, R=12, E=13, A=14, M=15, O=16)
        else if (hitSwitch >= SWITCH_IDX_BUMPER_S && hitSwitch <= SWITCH_IDX_BUMPER_O) {
          handleSwitchBumper(hitSwitch);
        }
        // Side targets (left 2-5, right 1-5)
        else if ((hitSwitch >= SWITCH_IDX_LEFT_SIDE_TARGET_2 && hitSwitch <= SWITCH_IDX_LEFT_SIDE_TARGET_5)
          || (hitSwitch >= SWITCH_IDX_RIGHT_SIDE_TARGET_1 && hitSwitch <= SWITCH_IDX_RIGHT_SIDE_TARGET_5)) {
          handleSwitchSideTarget(hitSwitch);
        }
        // Hat rollovers
        else if (hitSwitch == SWITCH_IDX_HAT_LEFT_TOP || hitSwitch == SWITCH_IDX_HAT_LEFT_BOTTOM
          || hitSwitch == SWITCH_IDX_HAT_RIGHT_TOP || hitSwitch == SWITCH_IDX_HAT_RIGHT_BOTTOM) {
          handleSwitchHat(hitSwitch);
        }
        // Kickouts
        else if (hitSwitch == SWITCH_IDX_KICKOUT_LEFT || hitSwitch == SWITCH_IDX_KICKOUT_RIGHT) {
          handleSwitchKickout(hitSwitch);
        }
      }
    }
  }
  // ***** TROUGH SANITY CHECK *****
  // If the 5-balls-in-trough switch has been held closed for 1 second, all 5 balls
  // are physically in the trough. If ballsInPlay > 0, we missed a drain -- most likely
  // two balls rolled over a drain rollover back-to-back without the switch opening
  // between them. Correct ballsInPlay and handle the missed drain(s).
  // The 1-second stability requirement prevents false triggers from a ball momentarily
  // rolling over the trough switch on its way to a resting position.
  if (allBallsInTrough()) {
    if (!troughSanityWasStable) {
      troughSanityStableStartMs = millis();
      troughSanityWasStable = true;
    } else if (millis() - troughSanityStableStartMs >= BALL_DETECTION_STABILITY_MS) {
      if (ballsInPlay > 0) {
        snprintf(lcdString, LCD_WIDTH + 1, "TROUGH FIX %u->0", ballsInPlay);
        lcdPrintRow(3, lcdString);
        Serial.print(F("TROUGH FIX "));
        Serial.print(ballsInPlay);
        Serial.print(F("->0 B"));
        Serial.println(gameState.currentBall);

        // Route through handleBallDrain() so that multiball save, mode ball
        // replacement, and single-ball drain logic all follow the same path.
        // Set ballsInPlay to 1 so handleBallDrain()'s initial decrement
        // brings it back to 0, matching the "all balls in trough" reality.
        ballsInPlay = 1;
        handleBallDrain(DRAIN_TYPE_NONE);
      }
      // Reset tracking whether or not we corrected (switch is still closed,
      // but we only need to fire the correction once)
      troughSanityWasStable = false;
    }
  } else {
    // Switch opened -- reset stability tracking
    troughSanityWasStable = false;
  }
}

// ***** BALL DRAIN HANDLER *****
void handleBallDrain(byte drainType) {
  // Normal switch debounce (SWITCH_DEBOUNCE_TICKS = 5) is already applied by
  // updateAllSwitchStates() when it detected the drain switch closing edge.
  // No additional drain switch suppression is needed here:
  //   - A ball rolling over one drain rollover cannot physically cause an adjacent
  //     drain switch to close (the motion is smooth, not vibratory).
  //   - A ball-save replacement ball travels from the trough into the ball lift
  //     base and cannot physically close any drain switch in transit.
  // The same-tick multi-drain check below handles the case where multiple balls
  // drain on the same tick during multiball.

  // Decrement ball count (guard against underflow per Overview 10.3.1)
  if (ballsInPlay > 0) {
    ballsInPlay--;
  }

  // ***** SAME-TICK MULTI-DRAIN CHECK (MULTIBALL ONLY) *****
  // checkPlayfieldSwitchHit() returns only one switch per call. If two (or three)
  // balls drained on the same tick through different drain switches, only the first
  // was returned to updatePhaseBallInPlay(). The remaining drains' edge flags are
  // still set in switchJustClosedFlag[]. The suppression we just applied above will
  // cause updateAllSwitchStates() on the NEXT tick to clear those unconsumed edge
  // flags without ever processing them -- silently losing drain events.
  //
  // Fix: check for any remaining drain edge flags NOW, score them, and count them
  // as additional drains. This must happen BEFORE the multiball/single-ball logic
  // below so that ballsInPlay accurately reflects reality when we decide whether
  // multiball has ended, whether to fire ball saves, etc.
  //
  // Only do this during multiball. In single-ball play there can only be one ball
  // on the playfield, so any additional drain edges are electrical noise/bounce.
  byte extraDrains = 0;
  if (inMultiball) {
    if (switchJustClosedFlag[SWITCH_IDX_DRAIN_LEFT]) {
      handleDrainRolloverScoring(SWITCH_IDX_DRAIN_LEFT);
      switchJustClosedFlag[SWITCH_IDX_DRAIN_LEFT] = false;
      extraDrains++;
    }
    if (switchJustClosedFlag[SWITCH_IDX_DRAIN_CENTER]) {
      handleDrainRolloverScoring(SWITCH_IDX_DRAIN_CENTER);
      switchJustClosedFlag[SWITCH_IDX_DRAIN_CENTER] = false;
      extraDrains++;
    }
    if (switchJustClosedFlag[SWITCH_IDX_DRAIN_RIGHT]) {
      handleDrainRolloverScoring(SWITCH_IDX_DRAIN_RIGHT);
      switchJustClosedFlag[SWITCH_IDX_DRAIN_RIGHT] = false;
      extraDrains++;
    }
    if (switchJustClosedFlag[SWITCH_IDX_GOBBLE]) {
      handleGobbleHoleScoring();
      switchJustClosedFlag[SWITCH_IDX_GOBBLE] = false;
      extraDrains++;
    }
    // Decrement ballsInPlay for each extra drain
    for (byte i = 0; i < extraDrains; i++) {
      if (ballsInPlay > 0) {
        ballsInPlay--;
      }
    }
  }

  // ***** MULTIBALL DRAIN *****
  if (inMultiball && ballsInPlay >= 1) {

    // Check multiball ball save: if the timer is still active, queue a replacement
    // ball for EACH ball that drained this tick (1 + extraDrains).
    if (multiballSaveEndMs > 0 && millis() < multiballSaveEndMs) {
      // Ball save is active -- queue replacements for all balls that drained
      byte totalDrains = 1 + extraDrains;
      ballsInPlay += totalDrains;  // Undo the decrements: all balls are being replaced
      multiballSavesRemaining += totalDrains;
      multiballSaveDispensePending = true;

      // Schedule an urgent "ball saved" announcement AFTER drain scoring bells finish.
      // The drain was just scored (500K or 100K), so bells ring for up to 1764ms.
      cancelPendingComTrack();
      byte randomIdx = (byte)random(0, NUM_COM_BALL_SAVED_URGENT);
      unsigned int trackNum = pgmReadComTrackNum(&comTracksBallSavedUrgent[randomIdx]);
      byte trackLenTenths = pgmReadComLengthTenths(&comTracksBallSavedUrgent[randomIdx]);
      scheduleComTrack(trackNum, trackLenTenths, 1800, true);

      snprintf(lcdString, LCD_WIDTH + 1, "MB SAVE balls=%u", ballsInPlay);
      lcdPrintRow(3, lcdString);
      return;
    }

    // No ball save -- normal multiball drain (just lost ball(s))
    snprintf(lcdString, LCD_WIDTH + 1, "MB balls=%u", ballsInPlay);
    lcdPrintRow(3, lcdString);
    if (ballsInPlay <= 1) {
      // Multiball is over; at most one ball survives on the playfield.
      // NOTE: We previously also checked (ballInLift ? 1 : 0) here to account for
      // a ball sitting at the lift base. However, ballInLift is only actively tracked
      // during PHASE_BALL_READY (not PHASE_BALL_IN_PLAY), so it can be stale and
      // incorrectly prevent this transition. This was the most likely cause of Surf
      // Rock music persisting after the second multiball ended. ballsInPlay already
      // accurately reflects reality, so the ballInLift term was redundant.
      inMultiball = false;
      multiballSaveEndMs = 0;  // Clear multiball save timer
      multiballSavesRemaining = 0;
      multiballSaveDispensePending = false;
      multiballSaveWaitingForLift = false;

      // No drain suppression needed here. If one ball survives, it must be able
      // to drain normally. If all balls are gone (ballsInPlay == 0), no ball is
      // on the playfield to trigger drains. Normal 5-tick debounce is sufficient.

      // Revert to primary music theme now that multiball has ended.
      audioStopMusic();
      ensureMusicPlaying();

      // If a COM track is still playing (e.g. multiball announcement), duck the new music
      if (audioCurrentComTrack != 0) {
        audioDuckMusicForVoice(true);
      }

      // If one ball survives, continue single-ball play
      if (ballsInPlay >= 1) {
        return;
      }

      // ballsInPlay == 0: all balls drained simultaneously. Fall through to
      // single-ball drain handling below (ball save check or processBallLost).
      snprintf(lcdString, LCD_WIDTH + 1, "MB all drain!");
      lcdPrintRow(3, lcdString);
    }

    // ballsInPlay >= 2 (or multiball was not ended above): still in multiball
    if (inMultiball) {
      return;
    }
  }

  // If multiball just ended AND ballsInPlay hit 0 on the FIRST drain (no extra drains
  // detected because there was only 1 ball when the function was entered -- e.g. the
  // multiball was already down to 2 balls and both drained on different ticks, with
  // the extra drain caught above), fall through to normal drain below.
  if (inMultiball && ballsInPlay == 0) {
    inMultiball = false;
    multiballSaveEndMs = 0;
    multiballSavesRemaining = 0;
    multiballSaveDispensePending = false;
    multiballSaveWaitingForLift = false;

    // No drain suppression needed. All balls are gone; no ball can trigger drains.

    // Stop multiball music and switch to primary theme.
    audioStopMusic();
    ensureMusicPlaying();
  }

  if (gameStyle == STYLE_ENHANCED) {
    if (ballSave.endMs > 0 && millis() < ballSave.endMs) {
      saveBall();
      return;
    }
  }

  // ***** MODE BALL REPLACEMENT *****
  // Per Overview Section 14.7.1: drained balls are ALWAYS replaced during modes.
  // This applies whether the mode timer is running OR the intro is still playing.
  // The timer does NOT pause during replacement -- it continues running (or has
  // not yet started, if still in intro).
  // If we are already waiting for a replacement (timerPaused), fall through
  // -- the ball is truly lost (e.g. trough sanity correction after a stuck ball).
  if (gameStyle == STYLE_ENHANCED && modeState.active && !modeState.timerPaused) {
    // Check if the mode timer has time remaining, OR if the intro is still playing
    bool introStillPlaying = !modeTimerStarted;
    bool timerHasTime = false;
    if (modeTimerStarted) {
      unsigned long elapsed = millis() - modeState.timerStartMs;
      timerHasTime = (elapsed < modeState.timerDurationMs);
    }

    if (introStillPlaying || timerHasTime) {
      // Dispense a replacement ball. If the timer is running, pause it
      // so the remaining time is preserved while the replacement is in transit.
      // If the intro is still playing, timerPaused just marks that a replacement
      // is in flight (the timer hasn't started yet, so nothing to pause).
      modeState.timerPaused = true;
      if (timerHasTime) {
        unsigned long elapsed = millis() - modeState.timerStartMs;
        modeState.pausedTimeRemainingMs = modeState.timerDurationMs - elapsed;
      } else {
        // Intro still playing: no timer to pause. Store 0; the timer will
        // start fresh when the intro ends and the replacement ball arrives.
        modeState.pausedTimeRemainingMs = 0;
      }

      modeSaveDispensePending = true;

      snprintf(lcdString, LCD_WIDTH + 1, "MODE SAVE %lums",
        modeState.pausedTimeRemainingMs);
      lcdPrintRow(3, lcdString);

      return;
    }
  }

  // End any active mode before advancing to the next ball. This handles the
  // cases where mode replacement was NOT granted: timer expired, intro still
  // playing, or timerPaused already true (replacement already in flight).
  // Without this, the mode would leak into the next ball.
  if (modeState.active) {
    endMode(false);
  }

  // No ball save: normal drain processing
  processBallLost(drainType);
}

// ***** BALL SAVE HANDLER *****
void saveBall() {

  // CLEAR the ball save timer and mark save as consumed for this ball number.
  // The replacement ball will NOT get a new timer.
  ballSave.endMs = 0;
  ballSaveUsed = true;
  ballSaveConsumedMs = millis();  // Record when save was used (for snarky drain audio)

  // No extended drain switch suppression is needed. The replacement ball travels
  // from the trough into the ball lift base and cannot close any drain switch.
  // Normal 5-tick debounce from updateAllSwitchStates() handles leaf switch bounce.

  // Stop ALL shaker sources (hill climb AND hill drop)
  stopAllShakers();

  // Brief shaker pulse for ball-save drama (the ball just drained -- give tactile feedback)
  analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_HILL_DROP);
  shakerDropStartMs = millis() - (SHAKER_HILL_DROP_MS - SHAKER_DRAIN_MS);

  // Stop hill climb audio if still playing
  if (audioCurrentSfxTrack == TRACK_START_LIFT_LOOP) {
    pTsunami->trackStop(TRACK_START_LIFT_LOOP);
    audioCurrentSfxTrack = 0;
  }

  // Eject any balls stuck in kickouts
  ejectKickoutsIfOccupied();

  // Preempt any currently playing COM track (e.g. hill drop scream) so the
  // ball save announcement is not buried under it.
  audioPreemptComTrack();

  // If a high-priority award announcement is pending (e.g. "Special is lit!",
  // "5 balls in the Gobble Hole! Replay!"), preserve it instead of replacing
  // with a generic "Ball saved!" line. The award is more important to the player.
  // This happens when the gobble hole is both a scoring event (4th/5th gobble)
  // AND a drain -- the award was scheduled by handleGobbleHoleScoring() but would
  // be wiped by cancelPendingComTrack() before it ever fires.
  if (pendingComTrack == 0) {
    // No award pending -- schedule the normal ball save announcement.
    byte savedRandomIdx = (byte)random(0, NUM_COM_BALL_SAVED);
    unsigned int savedTrack = pgmReadComTrackNum(&comTracksBallSaved[savedRandomIdx]);
    byte savedLenTenths = pgmReadComLengthTenths(&comTracksBallSaved[savedRandomIdx]);
    scheduleComTrack(savedTrack, savedLenTenths, 1800, true);
  }
  // else: keep the existing pending award track; it will fire on its own schedule.

  // Start music ONLY if not already playing (prevents double music)
  ensureMusicPlaying();

  // Clear all drain switch edge flags to prevent bounce from triggering again
  switchJustClosedFlag[SWITCH_IDX_DRAIN_LEFT] = false;
  switchJustClosedFlag[SWITCH_IDX_DRAIN_CENTER] = false;
  switchJustClosedFlag[SWITCH_IDX_DRAIN_RIGHT] = false;
  switchJustClosedFlag[SWITCH_IDX_GOBBLE] = false;

  // Release new ball (if lift is clear)
  if (!switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
    activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);
  }

  // Return to PHASE_STARTUP to wait for ball in lift, then PHASE_BALL_READY
  gamePhase = PHASE_STARTUP;
  startupSubState = STARTUP_WAIT_FOR_LIFT;
  startupBallDispenseMs = millis();
  startupTickCounter = 0;
}

// ***** BALL LOST HANDLER *****
// Called when a ball drains and is NOT saved (ball save expired or already used).
// Handles drain audio, shaker feedback, extra ball redemption, ball advancement,
// and game over. Enhanced style plays snarky comments when the replacement ball
// drains quickly after a ball save, and random insults on normal drains per
// BALL_DRAIN_INSULT_FREQUENCY. The snarky comment plays to completion (blocking)
// before the next ball is dispensed, so "Ball N" does not overwrite it.
void processBallLost(byte drainType) {

  // Stop all shakers and hill climb state
  stopAllShakers();

  // Cancel any in-flight multiball save dispense. If processBallLost() was called
  // from the trough sanity check while multiball save replacements were still
  // queued or in transit, processMultiballSaveDispenseTick() may have already
  // fired a trough release earlier this tick. Without this cleanup,
  // STARTUP_DISPENSE_BALL would fire a SECOND release, ejecting two balls
  // simultaneously and jamming the lift mechanism (L=0 T=0 after 3s timeout).
  multiballSavesRemaining = 0;
  multiballSaveDispensePending = false;
  multiballSaveWaitingForLift = false;

  // If a high-priority award announcement is pending (e.g. "Special is lit!",
  // "5 balls in the Gobble Hole! Replay!", or a score-threshold "Replay!"),
  // preserve it. Otherwise cancel any stale pending track so the drain/game-over
  // audio can take its slot.
  bool preservePendingAward = (pendingComTrack != 0);
  if (!preservePendingAward) {
    cancelPendingComTrack();
  }

  // Enhanced: stop SFX and COM tracks, but keep music playing through ball changes.
  // Music only stops on game over (last ball).
  // EXCEPTION: If extraBallAwarded is true AND the "EXTRA BALL!" announcement
  // (track 841) is currently playing, do NOT stop it -- the player needs to hear
  // the announcement. It will finish on its own or be preempted by the "Here's
  // another ball!" announcement in the extra ball handler below.
  if (gameStyle == STYLE_ENHANCED) {
    if (audioCurrentSfxTrack != 0) {
      pTsunami->trackStop(audioCurrentSfxTrack);
      audioCurrentSfxTrack = 0;
    }
    bool keepComTrack = extraBallAwarded
      && (audioCurrentComTrack == TRACK_EXTRA_BALL_AWARDED
        || pendingComTrack == TRACK_EXTRA_BALL_AWARDED);
    if (audioCurrentComTrack != 0 && !keepComTrack) {
      pTsunami->trackStop(audioCurrentComTrack);
      audioCurrentComTrack = 0;
      audioComEndMillis = 0;
    }
    // Un-duck music in case it was ducked for a voice line (unless keeping Extra Ball announcement)
    if (!keepComTrack) {
      audioDuckMusicForVoice(false);
    }

    // Shaker motor feedback on drain (non-game-over only; game over has its own teardown)
    if (gameState.currentBall < MAX_BALLS || extraBallAwarded) {
      analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_HILL_DROP);
      shakerDropStartMs = millis() - (SHAKER_HILL_DROP_MS - SHAKER_DRAIN_MS);
    }

    // Enhanced drain audio: depends on drain type and ball save state.
    // - Gobble Hole drain: ALWAYS play the scream SFX (track 401) instead of a
    //   snarky comment. Gobble drains are rare and dramatic -- the scream adds
    //   to the roller coaster theme. Shaker also gets extra intensity.
    // - Post-save quick drain: if the replacement ball drained within the ball
    //   save time window (measured from the replacement ball's first playfield
    //   switch hit), ALWAYS play a random snarky comment.
    // - Normal drain: random snarky comment per BALL_DRAIN_INSULT_FREQUENCY.
    // Ball 5 (game over) is excluded -- it gets game over audio instead.
    // Skip when extra ball is awarded or a high-priority award is pending.
    if ((gameState.currentBall < MAX_BALLS || extraBallAwarded)
      && !extraBallAwarded && !preservePendingAward) {

      if (drainType == DRAIN_TYPE_GOBBLE) {
        // Gobble Hole drain: play the scream SFX (same as game start).
        // This plays as an SFX track so it does not interfere with COM scheduling.
        // Duck music so the scream is prominent.
        audioDuckMusicForVoice(true);
        pTsunami->trackPlayPoly((int)TRACK_START_SCREAM, 0, false);
        audioApplyTrackGain(TRACK_START_SCREAM, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
        audioCurrentSfxTrack = TRACK_START_SCREAM;
        // Un-duck after ~2 seconds (processAudioTick handles COM un-duck, but SFX
        // does not auto-un-duck. Set a fake COM end time so processAudioTick un-ducks.)
        audioCurrentComTrack = TRACK_START_SCREAM;
        audioComEndMillis = millis() + 2000UL;
      } else {
        // Bottom three drains: snarky comment logic (unchanged)
        bool playSnarky = false;

        // Case 1: Post-save quick drain. The player just got saved and wasted it.
        if (ballSaveUsed && savedBallFirstSwitchMs != 0) {
          unsigned long sinceFirstHit = millis() - savedBallFirstSwitchMs;
          if (sinceFirstHit < (unsigned long)ballSaveSeconds * 1000UL) {
            playSnarky = true;
          }
        }

        // Case 2: Random insult on normal (non-save) drains
        if (!playSnarky && !ballSaveUsed) {
          if ((byte)random(0, 100) < BALL_DRAIN_INSULT_FREQUENCY) {
            playSnarky = true;
          }
        }

        if (playSnarky) {
          byte randomIdx = (byte)random(0, NUM_COM_DRAIN);
          unsigned int trackNum = pgmReadComTrackNum(&comTracksDrain[randomIdx]);
          byte trackLenTenths = pgmReadComLengthTenths(&comTracksDrain[randomIdx]);
          scheduleComTrack(trackNum, trackLenTenths, 1800, true);
        }
      }
    }
  }

  if (extraBallAwarded) {
    // Extra ball: replay same ball number
    extraBallAwarded = false;
    ballSaveUsed = false;  // New ball attempt gets a fresh save
    ballSaveConsumedMs = 0;
    savedBallFirstSwitchMs = 0;

    // Enhanced: fire any pending "EXTRA BALL!" announcement, wait for it, then play
    // "Here's another ball! Send it!" before dispensing.
    if (gameStyle == STYLE_ENHANCED) {
      // If the deferred "EXTRA BALL!" announcement has not fired yet, fire it now.
      if (pendingComTrack == TRACK_EXTRA_BALL_AWARDED) {
        firePendingComTrack();
      }

      // Wait for the "EXTRA BALL!" announcement to finish playing.
      if (audioCurrentComTrack == TRACK_EXTRA_BALL_AWARDED && audioComEndMillis > 0) {
        long remaining = (long)(audioComEndMillis - millis());
        if (remaining > 0) {
          delay((unsigned long)remaining + 200UL);
        }
      }

      // Now play "Here's another ball! Send it!"
      audioDuckMusicForVoice(true);
      audioPreemptComTrack();
      pTsunami->trackPlayPoly(TRACK_EXTRA_BALL_COLLECTED, 0, false);
      audioApplyTrackGain(TRACK_EXTRA_BALL_COLLECTED, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_EXTRA_BALL_COLLECTED;
      byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[8]);  // Index 8 = track 842
      unsigned long trackMs = (unsigned long)trackLenTenths * 100UL;
      audioComEndMillis = millis() + trackMs;

      // Block until announcement finishes.
      delay(trackMs + 200UL);
      audioCurrentComTrack = 0;
      audioComEndMillis = 0;
      audioDuckMusicForVoice(false);
    }

    gamePhase = PHASE_STARTUP;
    startupSubState = STARTUP_DISPENSE_BALL;
    startupTickCounter = 0;
    return;
  }

  if (gameState.currentBall < MAX_BALLS) {
    // More balls remain: advance to next ball
    gameState.currentBall++;
    ballSaveUsed = false;  // New ball number gets a fresh save
    ballSaveConsumedMs = 0;
    savedBallFirstSwitchMs = 0;
    gamePhase = PHASE_STARTUP;
    startupSubState = STARTUP_DISPENSE_BALL;
    startupTickCounter = 0;
    return;
  }

  // Last ball - game over
  // DEBUG TELEMETRY: Log ball number at game over. If this shows a ball number
  // less than MAX_BALLS, something advanced currentBall unexpectedly.
  snprintf(lcdString, LCD_WIDTH + 1, "GAMEOVER B%u", gameState.currentBall);
  lcdPrintRow(3, lcdString);
  Serial.print(F("GAMEOVER B"));
  Serial.println(gameState.currentBall);

  if (gameStyle == STYLE_ENHANCED) {
    // If a pending award track is still waiting to fire, fire it now and wait for it
    // before the game over announcement overwrites it.
    if (pendingComTrack != 0) {
      firePendingComTrack();
      if (audioCurrentComTrack != 0 && audioComEndMillis > 0) {
        long remaining = (long)(audioComEndMillis - millis());
        if (remaining > 0) {
          delay((unsigned long)remaining + 200UL);
        }
        audioCurrentComTrack = 0;
        audioComEndMillis = 0;
      }
    }

    // Stop music immediately (bells from the final drain are still ringing on Slave)
    pTsunami->stopAllTracks();
    audioCurrentSfxTrack = 0;
    audioCurrentMusicTrack = 0;
    audioMusicEndMillis = 0;

    // Start a fill-and-drain lamp effect while we wait for the bells to finish.
    // The effect runs non-blocking via processLampEffectTick() in loop(), but
    // since we are in a delay() here, we must drive it manually.
    // Fill top-to-bottom (600ms) then drain bottom-to-top (600ms) + 200ms hold = ~1400ms.
    cancelLampEffect(pShiftRegister);
    startLampEffect(LAMP_EFFECT_FILL_DOWN_DRAIN_UP, 600, 200);

    // Delay 2 seconds to let the bells finish scoring and the lamp effect play out.
    // We must manually pump the lamp effect engine during this delay since loop()
    // is not running. The effect needs processLampEffectTick() called every 10ms.
    {
      unsigned long delayEndMs = millis() + 2000UL;
      while ((long)(millis() - delayEndMs) < 0) {
        processLampEffectTick(pShiftRegister, lampParm);
        delay(10);
      }
    }

    // Cancel the lamp effect and restore gameplay lamps (teardownGame will set
    // attract lamps momentarily, but cancelling here ensures a clean state).
    cancelLampEffect(pShiftRegister);

    // Play a random game over announcement (tracks 551-577)
    byte randomIdx = (byte)random(0, NUM_COM_GAME_OVER);
    unsigned int gameOverTrack = pgmReadComTrackNum(&comTracksGameOver[randomIdx]);
    byte gameOverLenTenths = pgmReadComLengthTenths(&comTracksGameOver[randomIdx]);
    pTsunami->trackPlayPoly((int)gameOverTrack, 0, false);
    audioApplyTrackGain(gameOverTrack, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
    audioCurrentComTrack = gameOverTrack;
    audioComEndMillis = millis() + ((unsigned long)gameOverLenTenths * 100UL);

    // Wait for the game over announcement to finish before resetting state.
    // This is acceptable because the game is over -- no game logic needs to run.
    delay((unsigned long)gameOverLenTenths * 100UL + 500UL);
  }

  teardownGame(true);
}

// ***** SCORE HELPERS *****

// Adds points to the current player's score, sends the increment to Slave,
// and runs the Score Motor if needed (Original/Impulse only).
// t_amount is in 10K units (1 = 10,000 points, 5 = 50,000 points, etc.)
// Clamps score to 999 (9,990,000 points max).
void addScore(int t_amount) {
  if (t_amount == 0) {
    return;
  }
  byte p = gameState.currentPlayer - 1;
  int newScore = (int)gameState.score[p] + t_amount;
  if (newScore > 999) newScore = 999;
  if (newScore < 0) newScore = 0;
  gameState.score[p] = (unsigned int)newScore;

  // Send score increment to Slave (handles bells, 10K Unit, silence flags)
  sendScoreIncrement(t_amount);

  // Original/Impulse: run Score Motor for multi-unit increments.
  // IMPORTANT: The Score Motor SSR (DEV_IDX_MOTOR_SCORE) has timeOn=0 and powerHold=0,
  // which means activateDevice() turns it ON but the device timer system will never turn
  // it OFF (countdown starts at 0, so updateDeviceTimers() treats it as idle).
  // We must manage the Score Motor's on/off timing ourselves using scoreMotorStartMs.
  // The processScoreMotorGameplayTick() function (called every tick from loop()) handles
  // turning it off after the calculated duration.
  if (useMechanicalScoringDevices()) {
    unsigned long motorMs = calculateScoreMotorRunMs(RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K, t_amount);
    if (motorMs > 0) {
      // Only start the motor if it is not already running. If it IS already running
      // (from a previous scoring event), extend the deadline if the new event needs
      // more time. This prevents overlapping activateDevice() calls from queuing
      // additional motor-on pulses.
      unsigned long now = millis();
      unsigned long newDeadline = now + motorMs;
      if (scoreMotorGameplayEndMs == 0) {
        // Motor is not running -- start it
        analogWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, deviceParm[DEV_IDX_MOTOR_SCORE].powerInitial);
        scoreMotorGameplayEndMs = newDeadline;
      } else {
        // Motor is already running -- extend deadline if needed
        if ((long)(newDeadline - scoreMotorGameplayEndMs) > 0) {
          scoreMotorGameplayEndMs = newDeadline;
        }
      }
    }
  }

  // Update "Spots Number When Lit" lamps based on new 10K digit
  updateSpotsNumberLamps();

  // Check if the new score crossed any replay score thresholds
  checkReplayScoreThresholds();
}

// Updates the four "Spots Number When Lit" lamps based on the current 10K digit.
// Per Overview Section 13.3.2:
//   10K digit 0 -> Left Kickout LIT, Right Drain LIT
//   10K digit 5 -> Right Kickout LIT, Left Drain LIT
//   All other digits -> All four off
// Skips the 4 I2C writes if the 10K digit has not changed since the last call.
void updateSpotsNumberLamps() {
  byte p = gameState.currentPlayer - 1;
  byte tenKDigit = gameState.score[p] % 10;

  // Skip I2C writes if nothing has changed since the last call
  if (tenKDigit == spotsNumberLastTenKDigit
    && kickoutLockedMask == spotsNumberLastKickoutMask) {
    return;  // Nothing changed, skip 4 expensive I2C calls
  }
  spotsNumberLastTenKDigit = tenKDigit;
  spotsNumberLastKickoutMask = kickoutLockedMask;

  // Left Kickout + Right Drain lit when 10K digit is 0
  bool leftKickoutLit = (tenKDigit == 0);
  // Right Kickout + Left Drain lit when 10K digit is 5
  bool rightKickoutLit = (tenKDigit == 5);

  // If a kickout has a locked ball, force its lamp ON regardless of Spots Number state.
  if (kickoutLockedMask & KICKOUT_LOCK_LEFT) {
    leftKickoutLit = true;
  }
  if (kickoutLockedMask & KICKOUT_LOCK_RIGHT) {
    rightKickoutLit = true;
  }

  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_LEFT].pinNum,
    leftKickoutLit ? LOW : HIGH);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_RIGHT].pinNum,
    (tenKDigit == 0) ? LOW : HIGH);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_RIGHT].pinNum,
    rightKickoutLit ? LOW : HIGH);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_LEFT].pinNum,
    (tenKDigit == 5) ? LOW : HIGH);
}

// ***** MID-GAME START BUTTON HANDLER *****
// Per Overview Section 4.4: after ball 1 has hit any playfield target, pressing Start
// ends the current game and returns to attract mode. No credit check is needed here --
// the player simply wants to stop the current game. If they want to start a new game,
// they will press Start again in attract mode, which performs the normal credit check
// via processStartButton() -> startGameIfCredits().
// Before ball 1's first hit, Start is ignored (reserved for future multi-player support).
void processMidGameStart() {
  // Only active during gameplay phases (not startup, not ball search)
  if (gamePhase != PHASE_BALL_IN_PLAY && gamePhase != PHASE_BALL_READY) {
    return;
  }

  // Determine if the game is past the "add players" window.
  // Ball 1 before first hit: Start is reserved for future multi-player add.
  // Ball 1 after first hit, or any ball 2+: Start ends the game.
  // We check three conditions because hasHitSwitch is reset per-ball in onEnterBallReady(),
  // but we still need to recognize mid-game Start on a ball-save replacement ball that
  // has not yet hit a target. ball1SequencePlayed is a game-level flag (set once ball 1
  // hits any non-drain target, never reset until initGameState) that covers this gap.
  bool gameCommitted = (gameState.currentBall >= 2)
    || gameState.hasHitSwitch
    || ball1SequencePlayed;
  if (!gameCommitted) {
    return;
  }

  // Detect Start button press
  if (!switchJustClosedFlag[SWITCH_IDX_START_BUTTON]) {
    return;
  }

  // DEBUG TELEMETRY: Log mid-game Start so we can distinguish it from game-over.
  // If this message appears when the player did NOT press Start, the Start button
  // switch or I2C bus produced a phantom edge.
  snprintf(lcdString, LCD_WIDTH + 1, "MID-START B%u P%u",
    gameState.currentBall, gameState.currentPlayer);
  lcdPrintRow(3, lcdString);
  Serial.print(F("MID-START B"));
  Serial.print(gameState.currentBall);
  Serial.print(F(" P"));
  Serial.println(gameState.currentPlayer);

  // CONSUME the edge so no other handler (especially processStartButton, which runs
  // later in the same tick after we set gamePhase = PHASE_ATTRACT) can see it.
  // Without this, the same press that ends the game also registers as the first tap
  // of a new game's style detection, causing processStartButton() to start a 500ms
  // timer. After the 150-tick debounce expires (1.5s), that timer has long since
  // timed out, and processStartButton() immediately starts an Original game.
  switchJustClosedFlag[SWITCH_IDX_START_BUTTON] = false;

  // Suppress Start button for 1.5 seconds so that any remaining taps from a
  // double-tap or triple-tap gesture are absorbed before attract mode begins
  // listening for style detection. Without this, the second tap of a double-tap
  // would immediately start a new Original game (single-tap timeout), which is
  // not what the player intended. 150 ticks * 10ms = 1500ms > 1000ms (two
  // 500ms style detection windows).
  switchDebounceCounter[SWITCH_IDX_START_BUTTON] = 150;

  // Tear down and return to attract
  teardownGame(true);

  // Clear any stale tap-detection state so processStartButton() starts clean.
  // Without this, if processStartButton() had already seen a tap before the
  // mid-game Start was pressed, it could have leftover timer state.
  startWaitingForSecondTap = false;
  startWaitingForThirdTap = false;
  startFirstPressMs = 0;
  startSecondPressMs = 0;
}

// ***** GAME TIMEOUT HELPERS *****
// Resets the inactivity deadline to now + gameTimeoutMs.
// Called on any playfield switch hit, flipper press, or game start.
void resetGameTimeoutDeadline() {
  if (gameTimeoutMs > 0) {
    gameTimeoutDeadlineMs = millis() + gameTimeoutMs;
  }
}

// Resets the undetected drain timer to a full UNDETECTED_DRAIN_TIMEOUT_MS from now.
// Called ONLY on playfield switch hits (not flipper presses). Flipper holds pause
// the timer (handled in processUndetectedDrainCheck), but flipper taps are ignored
// so a frustrated player pressing buttons after an undetected drain does not delay
// detection indefinitely.
void resetUndetectedDrainDeadline() {
  if (undetectedDrainDeadlineMs != 0 || undetectedDrainPausedRemainingMs != 0) {
    undetectedDrainDeadlineMs = millis() + UNDETECTED_DRAIN_TIMEOUT_MS;
    undetectedDrainPausedRemainingMs = 0;  // Clear any paused state; ball is clearly in play
  }
}

// ***** GAME TIMEOUT ENFORCEMENT *****
// If no playfield switch or flipper button has been hit within the configured
// timeout period, end the game automatically. See Overview Section 14.10.
void processGameTimeout() {
  // Only enforce during active gameplay phases
  if (gamePhase != PHASE_BALL_IN_PLAY && gamePhase != PHASE_BALL_READY) {
    return;
  }

  // If timeout is disabled (0 minutes in EEPROM), skip
  if (gameTimeoutMs == 0) {
    return;
  }

  // Check for flipper activity this tick (resets deadline without needing a scoring switch)
  if (switchJustClosedFlag[SWITCH_IDX_FLIPPER_LEFT_BUTTON]
    || switchJustClosedFlag[SWITCH_IDX_FLIPPER_RIGHT_BUTTON]) {
    resetGameTimeoutDeadline();
  }

  // Check deadline
  if ((long)(millis() - gameTimeoutDeadlineMs) < 0) {
    return;  // Not expired yet
  }

  // TIMEOUT: Show message briefly, then tear down
  lcdClear();
  lcdPrintRow(1, "GAME TIMEOUT");
  lcdPrintRow(2, "No activity");
  Serial.print(F("GAME TIMEOUT B"));
  Serial.println(gameState.currentBall);
  delay(2000);

  teardownGame(true);
}

// ***** UNDETECTED DRAIN RECOVERY *****
// If no playfield switch has been hit for UNDETECTED_DRAIN_TIMEOUT_MS during
// PHASE_BALL_IN_PLAY, the ball likely drained without triggering any drain switch
// (e.g. slipped behind a raised flipper). Only runs when ballsLocked >= 1, because
// when ballsLocked == 0, the trough sanity check catches missed drains faster via
// the 5-balls-in-trough switch.
//
// Flipper button HOLDS pause the countdown (player may be cradling a ball on a
// raised flipper). Flipper TAPS are ignored -- they do not reset or pause the
// timer. This prevents a frustrated player (pressing buttons after an undetected
// drain) from delaying detection indefinitely.
void processUndetectedDrainCheck() {
  // Only relevant during active gameplay, not multiball (trough sanity handles that),
  // and only when at least one ball is locked (otherwise trough switch catches it).
  if (gamePhase != PHASE_BALL_IN_PLAY || inMultiball || ballsLocked == 0) {
    return;
  }

  // Timer not yet started (set when entering PHASE_BALL_IN_PLAY)
  if (undetectedDrainDeadlineMs == 0 && undetectedDrainPausedRemainingMs == 0) {
    return;
  }

  bool flipperHeld = switchClosed(SWITCH_IDX_FLIPPER_LEFT_BUTTON)
    || switchClosed(SWITCH_IDX_FLIPPER_RIGHT_BUTTON);

  if (flipperHeld) {
    // Flipper is held: PAUSE the timer (save remaining time, stop counting down).
    // If already paused, do nothing (remaining time is already saved).
    if (undetectedDrainDeadlineMs != 0) {
      long remaining = (long)(undetectedDrainDeadlineMs - millis());
      if (remaining < 0) remaining = 0;
      undetectedDrainPausedRemainingMs = (unsigned long)remaining;
      undetectedDrainDeadlineMs = 0;  // Stop the countdown
    }
    return;
  }

  // Flipper is NOT held: if the timer was paused, RESUME it.
  if (undetectedDrainDeadlineMs == 0 && undetectedDrainPausedRemainingMs > 0) {
    undetectedDrainDeadlineMs = millis() + undetectedDrainPausedRemainingMs;
    undetectedDrainPausedRemainingMs = 0;
  }

  // Check deadline
  if ((long)(millis() - undetectedDrainDeadlineMs) < 0) {
    return;  // Not expired yet
  }

  // Inactivity timeout with no flippers held: the ball almost certainly drained
  // without triggering a drain switch.
  snprintf(lcdString, LCD_WIDTH + 1, "PHANTOM DRAIN B%u", gameState.currentBall);
  lcdPrintRow(3, lcdString);
  Serial.print(F("PHANTOM DRAIN B"));
  Serial.print(gameState.currentBall);
  Serial.print(F(" style="));
  Serial.println(gameStyle);

  // Disable the timer so it does not re-fire
  undetectedDrainDeadlineMs = 0;
  undetectedDrainPausedRemainingMs = 0;

  // Process as a normal ball drain (ball save check, mode replacement, etc.)
  handleBallDrain(DRAIN_TYPE_NONE);
}

// ***** PERIODIC EEPROM SCORE SAVE *****
// Saves the current score and EM emulation state to EEPROM approximately every
// EEPROM_SAVE_INTERVAL_MS during active gameplay. This approximates the original
// EM behavior of retaining state across power cycles. See Overview Section 6.1.
void processPeriodicScoreSave() {
  // Only save during active gameplay
  if (gamePhase != PHASE_BALL_IN_PLAY && gamePhase != PHASE_BALL_READY) {
    return;
  }

  // Only save if a game is in progress with a valid player
  if (gameState.numPlayers == 0 || gameState.currentPlayer == 0) {
    return;
  }

  unsigned long now = millis();
  if ((long)(now - lastEepromSaveMs) >= EEPROM_SAVE_INTERVAL_MS) {
    saveLastScore(gameState.score[gameState.currentPlayer - 1]);
    saveEmState();
    lastEepromSaveMs = now;
  }
}

// ********************************************************************************************************************************
// ************************************************** SCORING SWITCH HANDLERS *****************************************************
// ********************************************************************************************************************************

// ***** MODE SCORING SWITCH HANDLER *****
// Called from updatePhaseBallInPlay() when modeState.active is true.
// Routes ALL non-drain switch hits through mode-specific scoring rules.
// Per Overview Section 14.7.1: Only mode-specific targets score. Near-miss targets
// play a miss SFX. Everything else (slingshots, non-near-miss targets, kickouts) is
// silent and scores nothing. Side targets do NOT advance the Selection Unit during modes.
void handleModeScoringSwitch(byte hitSwitch) {

  // During the mode intro announcement, scoring and lamp changes work normally
  // so the player can start working on the objective immediately. Only SFX is
  // suppressed so the player can hear the intro instructions.
  bool suppressSfx = !modeTimerStarted;

  switch (modeState.currentMode) {

  // ***** BUMPER CARS MODE SCORING *****
  // Per Overview Section 14.7.2:
  //   Objective: hit bumpers in S-C-R-E-A-M-O order.
  //   Correct bumper (next in sequence, lamp lit): 100K, extinguish lamp, advance modeProgress.
  //   Wrong bumper (out of sequence or already extinguished): car honk SFX, no points.
  //   Near-miss side targets: car crash SFX, no points.
  //   All others (slingshots, hats, other side targets, kickouts): silent, no points.
  case MODE_BUMPER_CARS: {
    // Bumper hit
    if (hitSwitch >= SWITCH_IDX_BUMPER_S && hitSwitch <= SWITCH_IDX_BUMPER_O) {
      // Fire the pop bumper coil for "E" (the only bumper with a solenoid)
      if (hitSwitch == SWITCH_IDX_BUMPER_E) {
        activateDevice(DEV_IDX_POP_BUMPER);
      }

      // Which bumper index? 0=S, 1=C, 2=R, 3=E, 4=A, 5=M, 6=O
      byte bumperIdx = hitSwitch - SWITCH_IDX_BUMPER_S;

      // Is this the next expected bumper in the sequence?
      if (bumperIdx == modeState.modeProgress) {
        // Correct hit! Award 100K, extinguish lamp, advance progress.
        addScore(10);  // 10 x 10K = 100K

        // Extinguish this bumper's lamp
        byte lampIdx = LAMP_IDX_S + bumperIdx;
        setLampState(lampIdx, false);  // Lamp off (effect-safe)

        modeState.modeProgress++;

        // Play achievement SFX: ding-ding-ding (suppressed during intro)
        if (!suppressSfx && pTsunami != nullptr) {
          pTsunami->trackPlayPoly(TRACK_MODE_BUMPER_ACHIEVED, 0, false);
          audioApplyTrackGain(TRACK_MODE_BUMPER_ACHIEVED, tsunamiSfxGainDb,
            tsunamiGainDb, pTsunami);
        }

        // If the sequence is complete (7th bumper), award 1,000,000 point jackpot.
        // The 100K for this hit was already scored above; the jackpot is additional.
        // processModeTick() will call endMode(true) on the next tick.
        if (modeState.modeProgress >= 7) {
          addScore(100);  // 100 x 10K = 1,000,000
        }
      } else {
        // Wrong bumper -- play random car honk SFX, no points (suppressed during intro)
        if (!suppressSfx) {
          unsigned int track = TRACK_MODE_BUMPER_HIT_FIRST
            + (unsigned int)random(0, TRACK_MODE_BUMPER_HIT_LAST
              - TRACK_MODE_BUMPER_HIT_FIRST + 1);
          if (pTsunami != nullptr) {
            pTsunami->trackPlayPoly((int)track, 0, false);
            audioApplyTrackGain(track, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
          }
        }
      }
    }
    // Near-miss side targets: car crash SFX (suppressed during intro)
    else if (hitSwitch == SWITCH_IDX_LEFT_SIDE_TARGET_2
      || hitSwitch == SWITCH_IDX_LEFT_SIDE_TARGET_3
      || hitSwitch == SWITCH_IDX_RIGHT_SIDE_TARGET_1
      || hitSwitch == SWITCH_IDX_RIGHT_SIDE_TARGET_2
      || hitSwitch == SWITCH_IDX_RIGHT_SIDE_TARGET_3) {
      if (!suppressSfx) {
        unsigned int track = TRACK_MODE_BUMPER_MISS_FIRST
          + (unsigned int)random(0, TRACK_MODE_BUMPER_MISS_LAST
            - TRACK_MODE_BUMPER_MISS_FIRST + 1);
        if (pTsunami != nullptr) {
          pTsunami->trackPlayPoly((int)track, 0, false);
          audioApplyTrackGain(track, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
        }
      }
    }
    // Kickouts: eject immediately, no points, no sound
    else if (hitSwitch == SWITCH_IDX_KICKOUT_LEFT
      || hitSwitch == SWITCH_IDX_KICKOUT_RIGHT) {
      handleModeKickoutEject(hitSwitch);
    }
    // Slingshots: fire the coil (physical feedback) but no points, no sound
    else if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT
      || hitSwitch == SWITCH_IDX_SLINGSHOT_RIGHT) {
      if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT) {
        activateDevice(DEV_IDX_SLINGSHOT_LEFT);
      } else {
        activateDevice(DEV_IDX_SLINGSHOT_RIGHT);
      }
    }
    // All others (hats, other side targets): silent, no points
    break;
  }

  // ***** ROLL-A-BALL MODE SCORING *****
  // Per Overview Section 14.7.3:
  //   Objective: hit hat rollovers as many times as possible before time expires.
  //   Any hat rollover: 50K + bowling strike SFX. No lamp changes (hats stay lit).
  //   Near-miss targets: glass breaking SFX, no points.
  //     Bumpers R, E, A, M, O (the 5 bumpers nearest the hats)
  //     Left side targets 4 and 5 (flanking the left approach)
  //     Right side targets 4 and 5 (flanking the right approach)
  //   Pop bumper coil fires for "E" regardless.
  //   All others (slingshots, other bumpers/targets, kickouts): silent/coil-only, no points.
  //   Jackpot: 10th hat awards 1,000,000 bonus; processModeTick() ends the mode.
  case MODE_ROLL_A_BALL: {
    // Hat rollover hit
    if (hitSwitch == SWITCH_IDX_HAT_LEFT_TOP || hitSwitch == SWITCH_IDX_HAT_LEFT_BOTTOM
      || hitSwitch == SWITCH_IDX_HAT_RIGHT_TOP || hitSwitch == SWITCH_IDX_HAT_RIGHT_BOTTOM) {
      addScore(5);  // 5 x 10K = 50K
      modeState.modeProgress++;  // Track total hits for LCD/diagnostics

      // Play bowling strike SFX (suppressed during intro)
      if (!suppressSfx && pTsunami != nullptr) {
        unsigned int track = TRACK_MODE_RAB_HIT_FIRST
          + (unsigned int)random(0, TRACK_MODE_RAB_HIT_LAST
            - TRACK_MODE_RAB_HIT_FIRST + 1);
        pTsunami->trackPlayPoly((int)track, 0, false);
        audioApplyTrackGain(track, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
      }

      // If 10th hat is hit, award 1,000,000 point jackpot.
      // The 50K for this hit was already scored above; the jackpot is additional.
      // processModeTick() will call endMode(true) on the next tick.
      if (modeState.modeProgress >= 10) {
        addScore(100);  // 100 x 10K = 1,000,000
      }
    }
    // Near-miss bumpers R, E, A, M, O: glass breaking SFX, no points
    else if (hitSwitch >= SWITCH_IDX_BUMPER_R && hitSwitch <= SWITCH_IDX_BUMPER_O) {
      if (hitSwitch == SWITCH_IDX_BUMPER_E) {
        activateDevice(DEV_IDX_POP_BUMPER);
      }
      if (!suppressSfx) {
        unsigned int track = TRACK_MODE_RAB_MISS_FIRST
          + (unsigned int)random(0, TRACK_MODE_RAB_MISS_LAST
            - TRACK_MODE_RAB_MISS_FIRST + 1);
        if (pTsunami != nullptr) {
          pTsunami->trackPlayPoly((int)track, 0, false);
          audioApplyTrackGain(track, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
        }
      }
    }
    // Near-miss side targets: left 4-5 and right 4-5
    else if (hitSwitch == SWITCH_IDX_LEFT_SIDE_TARGET_4
      || hitSwitch == SWITCH_IDX_LEFT_SIDE_TARGET_5
      || hitSwitch == SWITCH_IDX_RIGHT_SIDE_TARGET_4
      || hitSwitch == SWITCH_IDX_RIGHT_SIDE_TARGET_5) {
      if (!suppressSfx) {
        unsigned int track = TRACK_MODE_RAB_MISS_FIRST
          + (unsigned int)random(0, TRACK_MODE_RAB_MISS_LAST
            - TRACK_MODE_RAB_MISS_FIRST + 1);
        if (pTsunami != nullptr) {
          pTsunami->trackPlayPoly((int)track, 0, false);
          audioApplyTrackGain(track, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
        }
      }
    }
    // Bumpers S and C: not near the hats, no SFX, no points
    else if (hitSwitch == SWITCH_IDX_BUMPER_S || hitSwitch == SWITCH_IDX_BUMPER_C) {
      // Silent, no points
    }
    // Kickouts: eject immediately, no points, no sound
    else if (hitSwitch == SWITCH_IDX_KICKOUT_LEFT
      || hitSwitch == SWITCH_IDX_KICKOUT_RIGHT) {
      handleModeKickoutEject(hitSwitch);
    }
    // Slingshots: fire the coil but no points, no sound
    else if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT
      || hitSwitch == SWITCH_IDX_SLINGSHOT_RIGHT) {
      if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT) {
        activateDevice(DEV_IDX_SLINGSHOT_LEFT);
      } else {
        activateDevice(DEV_IDX_SLINGSHOT_RIGHT);
      }
    }
    // All others (other side targets): silent, no points
    break;
  }

  // ***** GOBBLE HOLE SHOOTING GALLERY MODE SCORING *****
  // Per Overview Section 14.7.4:
  //   Objective: sink 5 balls into the Gobble Hole before time expires.
  //   Gobble Hole hits are scored via handleGobbleHoleModeScoring() in the
  //   drain path (updatePhaseBallInPlay), NOT here -- because the gobble hole
  //   is a drain event and handleBallDrain() must run for mode ball replacement.
  //   Near-miss targets: ricochet SFX, no points.
  //     Bumpers R, E, A, M, O (the 5 bumpers nearest the gobble hole)
  //     Left side targets 4 and 5 (flanking the left approach)
  //     Right side targets 4 and 5 (flanking the right approach)
  //   Pop bumper coil fires for "E" regardless.
  //   All others (slingshots, hats, other bumpers/targets, kickouts): silent/coil-only, no points.
  case MODE_GOBBLE_HOLE: {
    // Near-miss bumpers R, E, A, M, O: ricochet SFX, no points
    if (hitSwitch >= SWITCH_IDX_BUMPER_R && hitSwitch <= SWITCH_IDX_BUMPER_O) {
      if (hitSwitch == SWITCH_IDX_BUMPER_E) {
        activateDevice(DEV_IDX_POP_BUMPER);
      }
      if (!suppressSfx) {
        unsigned int track = TRACK_MODE_GOBBLE_MISS_FIRST
          + (unsigned int)random(0, TRACK_MODE_GOBBLE_MISS_LAST
            - TRACK_MODE_GOBBLE_MISS_FIRST + 1);
        if (pTsunami != nullptr) {
          pTsunami->trackPlayPoly((int)track, 0, false);
          audioApplyTrackGain(track, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
        }
      }
    }
    // Near-miss side targets: left 4-5 and right 4-5
    else if (hitSwitch == SWITCH_IDX_LEFT_SIDE_TARGET_4
      || hitSwitch == SWITCH_IDX_LEFT_SIDE_TARGET_5
      || hitSwitch == SWITCH_IDX_RIGHT_SIDE_TARGET_4
      || hitSwitch == SWITCH_IDX_RIGHT_SIDE_TARGET_5) {
      if (!suppressSfx) {
        unsigned int track = TRACK_MODE_GOBBLE_MISS_FIRST
          + (unsigned int)random(0, TRACK_MODE_GOBBLE_MISS_LAST
            - TRACK_MODE_GOBBLE_MISS_FIRST + 1);
        if (pTsunami != nullptr) {
          pTsunami->trackPlayPoly((int)track, 0, false);
          audioApplyTrackGain(track, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
        }
      }
    }
    // Bumpers S and C: not near the gobble hole, fire coil only for E (already handled above)
    else if (hitSwitch == SWITCH_IDX_BUMPER_S || hitSwitch == SWITCH_IDX_BUMPER_C) {
      // No SFX, no points. Pop bumper coil doesn't apply (S and C have no coil).
    }
    // Kickouts: eject immediately, no points, no sound
    else if (hitSwitch == SWITCH_IDX_KICKOUT_LEFT
      || hitSwitch == SWITCH_IDX_KICKOUT_RIGHT) {
      handleModeKickoutEject(hitSwitch);
    }
    // Slingshots: fire the coil but no points, no sound
    else if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT
      || hitSwitch == SWITCH_IDX_SLINGSHOT_RIGHT) {
      if (hitSwitch == SWITCH_IDX_SLINGSHOT_LEFT) {
        activateDevice(DEV_IDX_SLINGSHOT_LEFT);
      } else {
        activateDevice(DEV_IDX_SLINGSHOT_RIGHT);
      }
    }
    // All others (hats, other side targets): silent, no points
    break;
  }

  default:
    break;
  }
}

// ***** MODE KICKOUT EJECT *****
// Per Overview Section 14.7.1: Kickouts during modes eject immediately.
// No points, no sound, no spotted number. Balls locked before the mode
// started remain locked (handled by kickoutLockedMask).
void handleModeKickoutEject(byte hitSwitch) {
  // If this kickout has a locked ball, ignore the edge (it's the locked ball
  // sitting on the switch, not a new ball entering).
  if (hitSwitch == SWITCH_IDX_KICKOUT_LEFT
    && (kickoutLockedMask & KICKOUT_LOCK_LEFT)) {
    return;
  }
  if (hitSwitch == SWITCH_IDX_KICKOUT_RIGHT
    && (kickoutLockedMask & KICKOUT_LOCK_RIGHT)) {
    return;
  }

  // Eject immediately
  if (hitSwitch == SWITCH_IDX_KICKOUT_LEFT) {
    activateDevice(DEV_IDX_KICKOUT_LEFT);
  } else {
    activateDevice(DEV_IDX_KICKOUT_RIGHT);
  }
}

// ***** SLINGSHOT HANDLER *****
// Per Overview Section 13.4.1: 10K points, 10K Bell, fires 10K Unit.
// Slingshots do NOT fire the Selection Unit (only side targets do).
// The 10K Unit is on the Slave side and is controlled via sendScoreIncrement().
void handleSwitchSlingshot(byte t_switchIdx) {
  // Suppress re-triggering for 150ms. Each slingshot has two leaf switches wired
  // in parallel behind a shared rubber band. A single ball impact sometimes closes both
  // switches sequentially, producing a second edge after the standard 50ms debounce
  // expires. 15 ticks (150ms) is long enough to absorb the second switch's bounce
  // but short enough that two separate multiball hits (~200ms+ apart) are both counted.
  const byte SLINGSHOT_SUPPRESS_TICKS = 15;
  switchDebounceCounter[t_switchIdx] = SLINGSHOT_SUPPRESS_TICKS;

  addScore(1);  // 10K

  // Fire the slingshot coil
  if (t_switchIdx == SWITCH_IDX_SLINGSHOT_LEFT) {
    activateDevice(DEV_IDX_SLINGSHOT_LEFT);
  } else {
    activateDevice(DEV_IDX_SLINGSHOT_RIGHT);
  }
}

// ***** BUMPER HANDLER *****
// Per Overview Section 13.4.2:
//   Normal hit: 10K, extinguish bumper lamp.
//   Last-lit hit: 10K + 500K, spotted number, relight all bumpers.
// NOTE: Only the "E" bumper has a physical pop bumper solenoid.
// The other six bumpers (S, C, R, A, M, O) are dead bumpers --
// they score and have lamps, but no coil to fire.
void handleSwitchBumper(byte t_switchIdx) {
  // Map switch index to bumper bit (0=S, 1=C, ... 6=O)
  byte bumperBit = t_switchIdx - SWITCH_IDX_BUMPER_S;

  // Fire the pop bumper coil ONLY for the "E" bumper (the only one with a solenoid)
  if (t_switchIdx == SWITCH_IDX_BUMPER_E) {
    activateDevice(DEV_IDX_POP_BUMPER);
  }

  // Score 10K (all bumper hits, whether lit or not, whether last or not)
  addScore(1);

  // If this bumper is lit, extinguish it
  if (bumperLitMask & (1 << bumperBit)) {
    bumperLitMask &= ~(1 << bumperBit);
    byte lampIdx = LAMP_IDX_S + bumperBit;
    setLampState(lampIdx, false);  // Lamp off (effect-safe)

    // Check if this was the LAST lit bumper
    if (bumperLitMask == 0) {
      // Last-lit bumper: award spotted number + 500K + relight all bumpers
      handleLastLitBumper();
    }
  }
}

// ***** LAST-LIT BUMPER HANDLER *****
// Per Overview Section 13.4.2:
//   +500K points (5 x 100K via Score Motor in Original/Impulse)
//   Award spotted number (light WHITE insert, ring Selection Bell)
//   Relight all 7 bumper lamps
//   Fire Relay Reset Bank (for sound, Original/Impulse only)
// Per Overview Section 14.7.1:
//   If Enhanced style and mode preconditions are met, start a mode.
void handleLastLitBumper() {

  // Determine NOW whether a mode will start, before any scoring.
  // This lets us silence bells so they don't compete with mode start audio.
  bool modeWillStart = (gameStyle == STYLE_ENHANCED
    && !modeState.active
    && !inMultiball
    && modeState.modeCount < MAX_MODES_PER_GAME);

  // Award spotted number FIRST (rings Selection Bell, unless mode is starting)
  if (modeWillStart) {
    awardSpottedNumberSilent();
  } else {
    awardSpottedNumber();
  }

  // Score 500K (5 x 100K increment).
  // If a mode is about to start, send with silent bells so the 5x 100K bells
  // don't compete with the mode-start stinger and intro announcement.
  if (modeWillStart) {
    // Silent score: update in-memory score and send silent message to Slave
    byte p = gameState.currentPlayer - 1;
    int newScore = (int)gameState.score[p] + 50;
    if (newScore > 999) newScore = 999;
    if (newScore < 0) newScore = 0;
    gameState.score[p] = (unsigned int)newScore;
    pMessage->sendMAStoSLVScoreInc10K(50, true, true);  // Silent bells, silent 10K unit
    updateSpotsNumberLamps();
    checkReplayScoreThresholds();
  } else {
    addScore(50);  // 50 x 10K = 500K
  }

  // Original/Impulse: override Score Motor deadline to one quarter-rev.
  overrideScoreMotorDeadline(1);

  // Fire Relay Reset Bank for sound (Original/Impulse only)
  if (useMechanicalScoringDevices()) {
    activateDevice(DEV_IDX_RELAY_RESET);
  }

  // Enhanced: announce "You spelled SCREAMO!" after the scoring bells finish.
  // SKIP if a mode is about to start (mode has its own audio).
  if (gameStyle == STYLE_ENHANCED && pendingComTrack == 0 && !modeWillStart) {
    byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[6]);  // Index 6 = track 831
    scheduleComTrack(TRACK_SPELLED_SCREAMO, trackLenTenths, 1000, true);
  }

  // Relight all 7 bumper lamps using fast port-level read-modify-write.
  // setBumperLampsFromMask() touches only the 2 ports that contain bumper pins
  // (4 I2C transactions: 2 reads + 2 writes, ~2.6ms) vs 7 individual
  // digitalWrite() calls (14 I2C transactions, ~4.5ms).
  bumperLitMask = 0x7F;
  setBumperLampsFromMask(bumperLitMask);

  // Enhanced: check if we should start a mode.
  // Per Overview Section 14.8.7: mode starts only if:
  //   - Enhanced style
  //   - Not currently in a mode (modes don't nest)
  //   - Not currently in multiball (multiball takes priority)
  //   - modeCount < 3 (max 3 modes per game)
  // Note: multiballPending is already checked implicitly. If ballsLocked == 2,
  // startMultiball() was called at the top of updatePhaseBallInPlay() on this
  // same tick BEFORE scoring dispatch, so inMultiball is already true. The
  // !inMultiball check correctly blocks the mode.
  if (modeWillStart) {
    startMode();
  }
}

// ***** AWARD SPOTTED NUMBER *****
// Lights the WHITE insert corresponding to the current RED insert number.
// The RED insert number is determined by the Selection Unit position.
// Rings the Selection Bell (via RS-485 to Slave).
// After lighting the WHITE insert, checks for matrix completion patterns
// that award replays (Original/Impulse) or an Extra Ball (Enhanced).
void awardSpottedNumber() {
  // Get current RED number from Selection Unit position
  byte redNumber = SELECTION_UNIT_RED_INSERT[selectionUnitPosition % 10];
  // redNumber is 1-9

  // Light the corresponding WHITE insert (if not already lit)
  unsigned int whiteBit = (1 << (redNumber - 1));  // bit 0 = number 1, bit 8 = number 9
  bool wasAlreadyLit = (whiteLitMask & whiteBit) != 0;
  if (!wasAlreadyLit) {
    whiteLitMask |= whiteBit;
    byte whiteLampIdx = LAMP_IDX_WHITE_1 + (redNumber - 1);
    setLampState(whiteLampIdx, true);  // Lamp on (effect-safe)
  }

  // Ring Selection Bell (Slave handles actual bell firing)
  pMessage->sendMAStoSLVBellSelect();

  // Check for WHITE matrix patterns (only if a NEW number was just lit)
  if (!wasAlreadyLit) {
    checkWhiteMatrixPatterns();
  }
}

// Silent variant of awardSpottedNumber() -- lights the WHITE insert and checks
// patterns but does NOT ring the Selection Bell. Used when a mode is about to
// start, so the bell doesn't compete with the mode-start stinger.
void awardSpottedNumberSilent() {
  byte redNumber = SELECTION_UNIT_RED_INSERT[selectionUnitPosition % 10];
  unsigned int whiteBit = (1 << (redNumber - 1));
  bool wasAlreadyLit = (whiteLitMask & whiteBit) != 0;
  if (!wasAlreadyLit) {
    whiteLitMask |= whiteBit;
    byte whiteLampIdx = LAMP_IDX_WHITE_1 + (redNumber - 1);
    setLampState(whiteLampIdx, true);  // Lamp on (effect-safe)
  }
  // No Selection Bell
  if (!wasAlreadyLit) {
    checkWhiteMatrixPatterns();
  }
}

// ***** WHITE MATRIX PATTERN CHECK *****
// Called from awardSpottedNumber() when a new WHITE insert is lit.
// Checks the 3x3 number matrix (1-9) for patterns that award replays.
// Per Overview Section 13.2:
//   - All 9 numbers (1-9): 20 replays (Original/Impulse) or Extra Ball (Enhanced)
//   - Four corners (1,3,7,9) + center (5): 5 replays
//   - Any 3-in-a-row (horizontal, vertical, diagonal): 1 replay
//
// The 3x3 matrix layout is:
//   1 2 3
//   4 5 6
//   7 8 9
//
// Pattern replays are queued in replayPendingCount for Original/Impulse so that
// the knocker fires AFTER the current Score Motor cycle finishes (the original
// EM game's replay relay did not trip until the Score Motor reached INDEX).
// Enhanced replays fire immediately since there is no Score Motor.
//
// Multiple patterns can be completed by the same spotted number. For example,
// lighting number 9 could complete both a 3-in-a-row (3-6-9) AND the four
// corners pattern simultaneously. All matching patterns are awarded.
//
// We track which patterns have already been awarded using bitmasks so that
// each pattern fires exactly once per matrix cycle. The tracking bitmasks
// are reset whenever the WHITE matrix is cleared (1-9 completion or game start).
void checkWhiteMatrixPatterns() {
  // Define the 8 possible 3-in-a-row lines (horizontal, vertical, diagonal)
  // Each line is three bit positions (0-indexed: bit 0 = number 1, bit 8 = number 9)
  static const unsigned int threeInARow[8] = {
    0x007,  // 1-2-3 (bits 0,1,2)
    0x038,  // 4-5-6 (bits 3,4,5)
    0x1C0,  // 7-8-9 (bits 6,7,8)
    0x049,  // 1-4-7 (bits 0,3,6)
    0x092,  // 2-5-8 (bits 1,4,7)
    0x124,  // 3-6-9 (bits 2,5,8)
    0x111,  // 1-5-9 (bits 0,4,8)
    0x054   // 3-5-7 (bits 2,4,6)
  };

  // Check for all 9 numbers complete (1-9)
  if (whiteLitMask == 0x01FF) {
    if (gameStyle == STYLE_ENHANCED) {
      // Enhanced: award Extra Ball instead of 20 replays.
      extraBallAwarded = true;

      // Reset all WHITE insert lamps (matrix cleared for potential second Extra Ball)
      whiteLitMask = 0;
      for (byte i = LAMP_IDX_WHITE_1; i <= LAMP_IDX_WHITE_9; i++) {
        setLampState(i, false);  // WHITE off (effect-safe)
      }

      // Play Fanfare WAV, then schedule "EXTRA BALL!" after it finishes.
      byte bellLenTenths = pgmReadComLengthTenths(&comTracksAward[9]);  // Index 9 = track 843
      audioDuckMusicForVoice(true);
      audioPreemptComTrack();
      pTsunami->trackPlayPoly(TRACK_EXTRA_BALL_HORNS, 0, false);
      audioApplyTrackGain(TRACK_EXTRA_BALL_HORNS, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_EXTRA_BALL_HORNS;
      unsigned long bellMs = (unsigned long)bellLenTenths * 100UL;
      audioComEndMillis = millis() + bellMs;

      // Schedule "EXTRA BALL!" announcement AFTER bell chime finishes.
      // The caller's scoring event also rings bells for up to 882ms; the bell chime
      // WAV replaces those visual bells, so we schedule based on whichever is longer.
      unsigned long delayMs = bellMs + 200UL;  // Bell chime length + small gap
      if (delayMs < 1000) delayMs = 1000;      // At least 1000ms to cover scoring bells
      byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[7]);  // Index 7 = track 841
      scheduleComTrack(TRACK_EXTRA_BALL_AWARDED, trackLenTenths, delayMs, true);

    } else {
      // Original/Impulse: award 20 replays.
      replayPendingCount += 20;

      // Reset all WHITE insert lamps.
      whiteLitMask = 0;
      for (byte i = LAMP_IDX_WHITE_1; i <= LAMP_IDX_WHITE_9; i++) {
        setLampState(i, false);  // WHITE off (effect-safe)
      }
    }
    // Reset pattern tracking since the matrix was cleared.
    whiteRowsAwarded = 0;
    whiteFourCornersAwarded = false;

    // After awarding 1-9, skip the other pattern checks. The matrix has been reset.
    return;
  }

  // Check for four corners (1,3,7,9) + center (5)
  static const unsigned int fourCornersCenter = 0x0155;
  if (!whiteFourCornersAwarded
    && (whiteLitMask & fourCornersCenter) == fourCornersCenter) {
    whiteFourCornersAwarded = true;
    if (gameStyle == STYLE_ENHANCED) {
      awardReplays(5);
      // Schedule "You hit 1,3,5,7,9! Five replays!" after bells + knocker finish.
      // 5 knocker fires at 147ms intervals = 735ms, plus scoring bells.
      byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[3]);  // Index 3 = track 822
      scheduleComTrack(TRACK_1_3_5_7_9, trackLenTenths, 1500, true);
    } else {
      replayPendingCount += 5;
    }
  }

  // Check for any 3-in-a-row
  bool anyNewRow = false;
  for (byte i = 0; i < 8; i++) {
    if (whiteRowsAwarded & (1 << i)) {
      continue;
    }
    if ((whiteLitMask & threeInARow[i]) == threeInARow[i]) {
      whiteRowsAwarded |= (1 << i);
      if (gameStyle == STYLE_ENHANCED) {
        awardReplays(1);
        anyNewRow = true;
      } else {
        replayPendingCount += 1;
      }
    }
  }
  // Schedule "Three in a row! Replay!" once (even if multiple lines completed).
  // Only schedule if no higher-priority announcement (four-corners, Extra Ball) is pending.
  if (anyNewRow && gameStyle == STYLE_ENHANCED && pendingComTrack == 0) {
    byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[2]);  // Index 2 = track 821
    scheduleComTrack(TRACK_3_IN_A_ROW, trackLenTenths, 1200, true);
  }
}

// ***** SIDE TARGET HANDLER *****
// Per Overview Section 13.4.3: 10K points, fires Selection Unit (which in turn fires 10K Unit).
// The Selection Unit is the ONLY device that controls the RED insert and HAT lamps.
// Side targets are the ONLY switches that advance the Selection Unit.
void handleSwitchSideTarget(byte t_switchIdx) {
  // Score 10K (Selection Unit is wired to fire the 10K Unit on every advance)
  addScore(1);

  // Compute the OLD RED number and HAT state before advancing.
  byte oldRedNumber = SELECTION_UNIT_RED_INSERT[selectionUnitPosition % 10];
  byte oldStepInCycle = selectionUnitPosition % 10;
  byte oldCycleNum = selectionUnitPosition / 10;
  bool oldHatsLit = (oldStepInCycle == 0) && (oldCycleNum == 0 || oldCycleNum == 2 || oldCycleNum == 4);

  // Advance Selection Unit position (0-49, wrapping)
  selectionUnitPosition = (selectionUnitPosition + 1) % 50;

  // Fire Selection Unit coil for sound (Original/Impulse only)
  if (useMechanicalScoringDevices()) {
    activateDevice(DEV_IDX_SELECTION_UNIT);
  }

  // Update RED insert lamp: turn off the OLD one, turn on the NEW one.
  // If old and new happen to be the same RED number (possible since the lookup
  // table has duplicates, e.g. positions 3 and 7 both map to RED 2), skip the
  // off/on since the lamp is already in the correct state.
  byte newRedNumber = SELECTION_UNIT_RED_INSERT[selectionUnitPosition % 10];
  if (oldRedNumber != newRedNumber) {
    byte oldRedLampIdx = LAMP_IDX_RED_1 + (oldRedNumber - 1);
    setLampState(oldRedLampIdx, false);  // Old RED off (effect-safe)
    byte newRedLampIdx = LAMP_IDX_RED_1 + (newRedNumber - 1);
    setLampState(newRedLampIdx, true);   // New RED on (effect-safe)
  }

  // Update HAT lamps ONLY if the HAT state actually changed.
  // Hats are lit on step 0 of cycles 0, 2, 4 (positions 0, 20, 40).
  // Out of 50 positions, hats change state on only a few transitions (e.g. 0->1 turns
  // off, 19->20 turns on, etc.). Skipping unchanged states saves 4 I2C calls (~2.6ms)
  // on the vast majority of side target hits.
  byte newStepInCycle = selectionUnitPosition % 10;
  byte newCycleNum = selectionUnitPosition / 10;
  bool newHatsLit = (newStepInCycle == 0) && (newCycleNum == 0 || newCycleNum == 2 || newCycleNum == 4);
  if (newHatsLit != oldHatsLit) {
    setLampState(LAMP_IDX_HAT_LEFT_TOP, newHatsLit);
    setLampState(LAMP_IDX_HAT_LEFT_BOTTOM, newHatsLit);
    setLampState(LAMP_IDX_HAT_RIGHT_TOP, newHatsLit);
    setLampState(LAMP_IDX_HAT_RIGHT_BOTTOM, newHatsLit);
  }
}

// ***** HAT ROLLOVER HANDLER (STUB) *****
// Per Overview Section 13.4.4: 10K (unlit) or 100K (lit).
void handleSwitchHat(byte t_switchIdx) {
  // Check if hats are currently lit
  byte stepInCycle = selectionUnitPosition % 10;
  byte cycleNum = selectionUnitPosition / 10;
  bool hatsLit = (stepInCycle == 0) && (cycleNum == 0 || cycleNum == 2 || cycleNum == 4);

  if (hatsLit) {
    addScore(10);  // 100K (10 x 10K)
    // Override Score Motor deadline: 10 x 10K = 2 quarter-revs
    overrideScoreMotorDeadline(2);
  } else {
    addScore(1);   // 10K
  }
}

// ***** KICKOUT HANDLER *****
// Per Overview Section 13.4.5: 500K + optional spotted number.
// Original/Impulse: Score Motor runs one quarter-rev (882ms). The 500K is scored
// as 5 x 100K at 147ms sub-slot intervals. The kickout solenoid fires on the 5th
// sub-slot (735ms), coinciding with the final 100K increment. The ball ejects at
// the same moment the last 100K is added.
// Enhanced: Score immediately. If not in multiball and fewer than 2 balls are locked,
// hold the ball in the kickout (lock it) and dispense a replacement. If 2 are already
// locked or multiball is active, eject immediately.
void handleSwitchKickout(byte t_switchIdx) {
  // Suppress re-triggering of this kickout switch while the ball is sitting in the
  // pocket. Original/Impulse: the ball sits for 735ms before the delayed eject fires.
  // Even Enhanced ejects take a few ticks for the coil to fire and the ball to leave.
  // The standard 50ms debounce is far too short -- any slight ball movement in the
  // pocket re-closes the switch and fires a second 500K scoring event. 100 ticks
  // (1000ms) covers the eject delay (735ms) plus coil fire time plus margin for the
  // ball to physically leave the pocket. Must not be longer than the time it takes
  // for a ball to leave, travel up the playfield, and return -- otherwise a legitimate
  // second entry is swallowed and the ball sits in the pocket with no eject scheduled.
  const byte KICKOUT_SUPPRESS_TICKS = 100;
  switchDebounceCounter[t_switchIdx] = KICKOUT_SUPPRESS_TICKS;

  // Check if "Spots Number When Lit" is active for this kickout.
  // IMPORTANT: Capture this BEFORE addScore(), because the 500K increment will
  // change the 10K digit and potentially toggle the Spots Number lamps.
  byte p = gameState.currentPlayer - 1;
  byte tenKDigit = gameState.score[p] % 10;
  bool leftKickoutLit = (tenKDigit == 0);
  bool rightKickoutLit = (tenKDigit == 5);

  bool spotsNumber = false;
  if (t_switchIdx == SWITCH_IDX_KICKOUT_LEFT && leftKickoutLit) {
    spotsNumber = true;
  } else if (t_switchIdx == SWITCH_IDX_KICKOUT_RIGHT && rightKickoutLit) {
    spotsNumber = true;
  }

  // Award spotted number FIRST if lit (Selection Bell rings immediately per Overview 13.4.5)
  if (spotsNumber) {
    awardSpottedNumber();
  }

  // Score 500K. addScore() will send the increment to Slave and start the Score Motor
  // via calculateScoreMotorRunMs(). However, the generic motor timing includes a 2-
  // quarter-rev safety margin that makes the motor run ~2646ms instead of 882ms.
  // For kickouts we know exactly how long the motor needs: one quarter-rev. So we
  // let addScore() handle the score and RS485 message, then override the motor timing.
  addScore(50);

  // Original/Impulse: override the Score Motor deadline to exactly one quarter-rev
  // from now. addScore() already started the motor and set scoreMotorGameplayEndMs
  // with the generic (too-long) deadline; we just shorten it.
  overrideScoreMotorDeadline(1);

  // Determine the lock bit for this kickout
  byte lockBit = (t_switchIdx == SWITCH_IDX_KICKOUT_LEFT) ? KICKOUT_LOCK_LEFT : KICKOUT_LOCK_RIGHT;

  // Eject timing depends on game style and multiball lock state.
  // Lock conditions: Enhanced, not in multiball, fewer than 2 locked, and THIS kickout
  // is not already locked (prevents same-kickout double-lock from a ball that enters
  // the same pocket twice, e.g. if the first locked ball was ejected by tilt and the
  // player shoots into the same kickout again -- though that scenario resets ballsLocked).
  if (gameStyle == STYLE_ENHANCED && !inMultiball && ballsLocked < 2
    && !(kickoutLockedMask & lockBit)) {
    // ***** ENHANCED: Lock the ball for multiball *****
    ballsLocked++;
    kickoutLockedMask |= lockBit;

    // Schedule lock announcement AFTER bells finish (500K = 882ms of 100K bells).
    // If awardSpottedNumber() already scheduled an Extra Ball announcement, it has
    // higher importance and overwrites are fine -- but we skip scheduling here to
    // preserve it. The kickout lamp provides visual lock feedback either way.
    if (pendingComTrack == 0) {
      unsigned int lockTrack = (ballsLocked == 1) ? TRACK_LOCK_1 : TRACK_LOCK_2;
      byte lockIdx = ballsLocked - 1;  // 0 = first lock, 1 = second lock
      byte lockLenTenths = pgmReadComLengthTenths(&comTracksMultiball[lockIdx]);
      scheduleComTrack(lockTrack, lockLenTenths, 1000, true);
    }

    // Flash the kickout lamp to indicate a locked ball (turn it on solid)
    if (t_switchIdx == SWITCH_IDX_KICKOUT_LEFT) {
      setLampState(LAMP_IDX_KICKOUT_LEFT, true);
    } else {
      setLampState(LAMP_IDX_KICKOUT_RIGHT, true);
    }

    if (ballsLocked == 2) {
      // Two balls locked: set multiballPending. The next non-drain playfield switch
      // hit will trigger startMultiball() from updatePhaseBallInPlay().
      multiballPending = true;
    }

    // Dispense a replacement ball (same ball number).
    // The ball will travel through the trough to the lift. We go through the
    // normal STARTUP path to wait for it to arrive, then back to BALL_READY/BALL_IN_PLAY.
    // NOTE: We do NOT change gameState.currentBall -- this is the same ball number.
    // We also do NOT reset ballSaveUsed -- the ball save state carries over.
    // Only dispense if the lift is empty; if a ball is already there, use it.
    if (!switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);
    }
    gamePhase = PHASE_STARTUP;
    startupSubState = STARTUP_WAIT_FOR_LIFT;
    startupBallDispenseMs = millis();
    startupTickCounter = 0;

  } else if (gameStyle == STYLE_ENHANCED) {
    // Enhanced but already in multiball or 2 balls locked: delay eject so the
    // player sees the 500K score increment before the ball is kicked out.
    // This uses the same 735ms delay as Original/Impulse (5 sub-slots of 147ms),
    // which is enough time for the score to visibly update on the display.
    // processKickoutEjectTick() in loop() fires these regardless of style.
    unsigned long ejectTime = millis() + KICKOUT_EJECT_DELAY_MS;
    if (t_switchIdx == SWITCH_IDX_KICKOUT_LEFT) {
      kickoutLeftEjectMs = ejectTime;
    } else {
      kickoutRightEjectMs = ejectTime;
    }
  } else {
    // Original/Impulse: delay eject until 5th sub-slot of the Score Motor quarter-rev.
    // The Score Motor is already running (started by addScore -> calculateScoreMotorRunMs).
    // We just need to schedule the solenoid firing.
    unsigned long ejectTime = millis() + KICKOUT_EJECT_DELAY_MS;
    if (t_switchIdx == SWITCH_IDX_KICKOUT_LEFT) {
      kickoutLeftEjectMs = ejectTime;
    } else {
      kickoutRightEjectMs = ejectTime;
    }
  }
}

// ***** DRAIN ROLLOVER SCORING HANDLER *****
// Per Overview Sections 13.4.6-13.4.7:
//   Left/Right drain: 100K + optional spotted number
//   Center drain: 500K, no spotted number
// NOTE: This handles the SCORING portion only. The drain/ball-lost logic
// is handled separately by handleBallDrain() which is called from the
// drain detection code in updatePhaseBallInPlay().
void handleDrainRolloverScoring(byte t_switchIdx) {
  byte p = gameState.currentPlayer - 1;
  byte tenKDigit = gameState.score[p] % 10;
  bool leftKickoutLit = (tenKDigit == 0);
  bool rightKickoutLit = (tenKDigit == 5);

  if (t_switchIdx == SWITCH_IDX_DRAIN_CENTER) {
    // Center drain: 500K, no spotted number
    addScore(50);
    // Override Score Motor deadline: 5 x 100K = 1 quarter-rev
    overrideScoreMotorDeadline(1);
  } else if (t_switchIdx == SWITCH_IDX_DRAIN_LEFT) {
    // Left drain: 100K
    addScore(10);
    // Override Score Motor deadline: 10 x 10K = 2 quarter-revs
    overrideScoreMotorDeadline(2);
    // "Spots Number When Lit" for left drain is active when 10K digit is 5
    if (rightKickoutLit) {
      awardSpottedNumber();
    }
  } else if (t_switchIdx == SWITCH_IDX_DRAIN_RIGHT) {
    // Right drain: 100K
    addScore(10);
    // Override Score Motor deadline: 10 x 10K = 2 quarter-revs
    overrideScoreMotorDeadline(2);
    // "Spots Number When Lit" for right drain is active when 10K digit is 0
    if (leftKickoutLit) {
      awardSpottedNumber();
    }
  }
}

// ***** GOBBLE HOLE SCORING HANDLER *****
// Per Overview Section 13.4.8:
//   500K + ALWAYS awards spotted number + gobble count tracking
// NOTE: This handles the SCORING portion only. The drain/ball-lost logic
// is handled separately by handleBallDrain().
void handleGobbleHoleScoring() {
  // Score 500K
  addScore(50);

  // Original/Impulse: override Score Motor deadline to one quarter-rev.
  overrideScoreMotorDeadline(1);

  // Gobble Hole ALWAYS awards spotted number
  awardSpottedNumber();

  // Track gobble count and update gobble lamps
  if (gobbleCount < 5) {
    gobbleCount++;
    byte gobbleLampIdx = LAMP_IDX_GOBBLE_1 + (gobbleCount - 1);
    setLampState(gobbleLampIdx, true);  // Lamp on (effect-safe)

    // 4th gobble: light SPECIAL WHEN LIT lamp and announce it
    if (gobbleCount == 4) {
      setLampState(LAMP_IDX_SPECIAL, true);  // Lamp on (effect-safe)

      // Enhanced: announce "Special is lit!" after bells finish.
      if (gameStyle == STYLE_ENHANCED && pendingComTrack == 0) {
        byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[0]);  // Index 0 = track 811
        scheduleComTrack(TRACK_SPECIAL_LIT, trackLenTenths, 1000, true);
      }
    }

    // 5th gobble with SPECIAL lit: award replay and announce it
    if (gobbleCount == 5) {
      awardReplays(1);

      // Enhanced: announce "Five balls in the Gobble Hole! Replay!" after bells + knocker.
      if (gameStyle == STYLE_ENHANCED && pendingComTrack == 0) {
        byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[5]);  // Index 5 = track 824
        scheduleComTrack(TRACK_5_BALLS_IN_GOBBLE, trackLenTenths, 1200, true);
      }
    }
  }
}

// ***** GOBBLE HOLE MODE-SPECIFIC SCORING *****
// Called from updatePhaseBallInPlay() when the gobble hole drains during
// Gobble Hole Shooting Gallery mode. Replaces handleGobbleHoleScoring()
// for mode-specific rules: 100K per hit, light GOBBLE lamp, track progress.
// 5th hit awards 1,000,000 jackpot; processModeTick() calls endMode(true).
void handleGobbleHoleModeScoring() {
  // Award 100K per gobble hit
  addScore(10);  // 10 x 10K = 100K

  modeState.modeProgress++;

  // Light the next GOBBLE lamp (1-based: hit 1 lights GOBBLE_1, etc.)
  // Cap at 5 lamps even if somehow hit more times.
  // IMPORTANT: If a lamp effect is running (e.g. hurry-up FLASH_ALL), the effect
  // engine will overwrite our digitalWrite() when it restores savedPortState[].
  // We must also patch savedPortState[] so the lamp survives the restore.
  if (modeState.modeProgress <= 5) {
    byte lampIdx = LAMP_IDX_GOBBLE_1 + (modeState.modeProgress - 1);
    setLampState(lampIdx, true);  // Lamp on (effect-safe)
  }

  // Play slide whistle SFX (only one track for gobble hit)
  bool suppressSfx = !modeTimerStarted;
  if (!suppressSfx && pTsunami != nullptr) {
    pTsunami->trackPlayPoly(TRACK_MODE_GOBBLE_HIT, 0, false);
    audioApplyTrackGain(TRACK_MODE_GOBBLE_HIT, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
  }

  // 5th gobble: award 1,000,000 jackpot (processModeTick detects modeProgress >= 5
  // and calls endMode(true) with applause SFX on the next tick)
  if (modeState.modeProgress >= 5) {
    addScore(100);  // 100 x 10K = 1,000,000
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

void releaseAllFlippers() {
  if (leftFlipperHeld) {
    releaseDevice(DEV_IDX_FLIPPER_LEFT);
    leftFlipperHeld = false;
  }
  if (rightFlipperHeld) {
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

// Eject any balls sitting in kickout pockets (if coil is not already firing).
// Called from attract, startup, ball search, tilt, and ball save contexts.
// During active Enhanced gameplay, locked balls are NOT ejected (they are
// intentionally held for multiball). Only non-gameplay phases force-eject.
void ejectKickoutsIfOccupied() {
  // During Enhanced gameplay with locked balls, do NOT force-eject.
  // The lock/multiball logic in handleSwitchKickout() and startMultiball()
  // manages when locked balls are released.
  if (gameStyle == STYLE_ENHANCED && ballsLocked > 0
    && (gamePhase == PHASE_BALL_IN_PLAY || gamePhase == PHASE_BALL_READY
      || gamePhase == PHASE_STARTUP)) {
    return;
  }

  if (switchClosed(SWITCH_IDX_KICKOUT_LEFT) && deviceParm[DEV_IDX_KICKOUT_LEFT].countdown == 0) {
    activateDevice(DEV_IDX_KICKOUT_LEFT);
  }
  if (switchClosed(SWITCH_IDX_KICKOUT_RIGHT) && deviceParm[DEV_IDX_KICKOUT_RIGHT].countdown == 0) {
    activateDevice(DEV_IDX_KICKOUT_RIGHT);
  }
}

// Override the Score Motor gameplay deadline to the specified number of quarter-revs
// from now. Only applies to Original/Impulse styles when the motor is already running.
// Call after addScore() when you know the exact motor duration needed.
void overrideScoreMotorDeadline(byte quarterRevs) {
  if (useMechanicalScoringDevices() && scoreMotorGameplayEndMs != 0) {
    scoreMotorGameplayEndMs = millis() + ((unsigned long)quarterRevs * SCORE_MOTOR_QUARTER_REV_MS);
  }
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
  // Bumpers always relight at game start (Relay Reset Bank), so they are always lit in attract.
  for (byte i = LAMP_IDX_S; i <= LAMP_IDX_O; i++) {
    pShiftRegister->digitalWrite(lampParm[i].pinNum, LOW);
  }

  // Set all persisted EM state lamps (RED, HATs, WHITEs, GOBBLEs, SPECIAL, Spots Number)
  // This emulates the original EM behavior where these lamps retained their state between
  // games and across power cycles because the physical hardware stayed where it was.
  setEmStateLamps();

  // Turn off Tilt lamp only -- do NOT zero the score display
  pMessage->sendMAStoSLVTiltLamp(false);
}

// Sets all gameplay lamps to reflect the EM emulation state at game start.
// Called once from STARTUP_RECOVER_CONFIRMED after initGameState() has loaded
// persisted EM state from EEPROM and setAttractLamps() has set the attract display.
// Also called from updatePhaseTilt() when restoring lamps after a tilt.
// Uses writeLampsBulk() for a single fast bulk write (~2ms) instead of many
// individual digitalWrite() calls.
void setInitialGameplayLamps() {
  byte desired[NUM_LAMPS_MASTER];

  // Start with all lamps off
  memset(desired, 0, sizeof(desired));

  // GI lamps stay on (managed separately by setGILamps)
  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    if (lampParm[i].groupNum == LAMP_GROUP_GI) {
      desired[i] = 1;
    }
  }

  // Bumper lamps based on current bumperLitMask
  for (byte i = 0; i < 7; i++) {
    if (bumperLitMask & (1 << i)) {
      desired[LAMP_IDX_S + i] = 1;
    }
  }

  // RED insert for current Selection Unit position
  byte redNumber = SELECTION_UNIT_RED_INSERT[selectionUnitPosition % 10];
  desired[LAMP_IDX_RED_1 + (redNumber - 1)] = 1;

  // HAT lamps if on an eligible step
  byte stepInCycle = selectionUnitPosition % 10;
  byte cycleNum = selectionUnitPosition / 10;
  bool hatsLit = (stepInCycle == 0) && (cycleNum == 0 || cycleNum == 2 || cycleNum == 4);
  if (hatsLit) {
    desired[LAMP_IDX_HAT_LEFT_TOP] = 1;
    desired[LAMP_IDX_HAT_LEFT_BOTTOM] = 1;
    desired[LAMP_IDX_HAT_RIGHT_TOP] = 1;
    desired[LAMP_IDX_HAT_RIGHT_BOTTOM] = 1;
  }

  // WHITE inserts from persisted bitmask
  for (byte i = 0; i < 9; i++) {
    if (whiteLitMask & (1 << i)) {
      desired[LAMP_IDX_WHITE_1 + i] = 1;
    }
  }

  // GOBBLE lamps (1 through gobbleCount)
  for (byte i = 0; i < gobbleCount; i++) {
    desired[LAMP_IDX_GOBBLE_1 + i] = 1;
  }

  // SPECIAL lamp if 4+ gobbles
  if (gobbleCount >= 4) {
    desired[LAMP_IDX_SPECIAL] = 1;
  }

  // Spots Number When Lit (based on live score, not persisted score)
  byte p = gameState.currentPlayer - 1;
  byte tenKDigit = gameState.score[p] % 10;
  if (tenKDigit == 0) {
    desired[LAMP_IDX_KICKOUT_LEFT] = 1;
    desired[LAMP_IDX_SPOT_NUMBER_RIGHT] = 1;
  } else if (tenKDigit == 5) {
    desired[LAMP_IDX_KICKOUT_RIGHT] = 1;
    desired[LAMP_IDX_SPOT_NUMBER_LEFT] = 1;
  }

  // Kickout lock overrides
  if (kickoutLockedMask & KICKOUT_LOCK_LEFT) {
    desired[LAMP_IDX_KICKOUT_LEFT] = 1;
  }
  if (kickoutLockedMask & KICKOUT_LOCK_RIGHT) {
    desired[LAMP_IDX_KICKOUT_RIGHT] = 1;
  }

  writeLampsBulk(desired);

  // Reset Spots Number lamp cache since bulk write changed those lamps directly
  spotsNumberLastTenKDigit = 0xFF;
  spotsNumberLastKickoutMask = 0xFF;
}

// ***** FAST BULK LAMP WRITE *****
// Writes all 47 lamp states in 4 fast portWrite() calls (~2ms total) instead
// of up to 47 individual digitalWrite() calls (~30ms). Each lamp's desired
// state (on/off) is packed into the 4 x 16-bit Centipede port words, then all
// four ports are written at once.
//
// t_lampStates: array of NUM_LAMPS_MASTER bytes, one per lamp.
//   0 = lamp OFF (pin HIGH), nonzero = lamp ON (pin LOW).
// Non-lamp pins on Centipede #1 ports 0-3 are set HIGH (off/safe) since they
// are unused outputs. This is the same convention used by the lamp effect engine.
void writeLampsBulk(const byte* t_lampStates) {
  // Start with all bits HIGH (all lamps off). We will pull LOW for lamps that are ON.
  uint16_t portVal[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    if (t_lampStates[i]) {
      // Lamp ON = pin LOW: clear the bit
      byte pin = lampParm[i].pinNum;
      byte port = pin >> 4;          // pins 0-15 = port 0, 16-31 = port 1, etc.
      byte bit = pin & 0x0F;         // bit position within the 16-bit port word
      portVal[port] &= ~((uint16_t)1 << bit);
    }
  }

  for (byte p = 0; p < 4; p++) {
    pShiftRegister->portWrite(p, portVal[p]);
  }
}

// ***** SET BUMPER LAMPS FROM MASK *****
// Turns bumper lamps on/off to match the given bitmask using port-level
// read-modify-write. This is much faster than 7 individual digitalWrite()
// calls (4 I2C transactions vs 14) because bumper pins span only 2 ports.
// t_mask: bits 0-6 = S,C,R,E,A,M,O. Bit set = lamp ON, bit clear = lamp OFF.
void setBumperLampsFromMask(byte t_mask) {
  // Build a set of bits to force LOW (lamp ON) and bits to force HIGH (lamp OFF)
  // for each port that has bumper pins. We read the port, modify only the bumper
  // bits, then write back. This preserves the state of all non-bumper lamps on
  // those ports.

  // Bumper pin mapping: LAMP_IDX_S(pin 51) through LAMP_IDX_O(pin 50)
  // Port = pin >> 4, bit = pin & 0x0F

  // Determine which ports are affected (precomputed: ports 1 and 3)
  // We do a generic loop so this works even if pin assignments change.
  uint16_t onBits[4]  = { 0, 0, 0, 0 };  // Bits to force LOW (lamp on)
  uint16_t offBits[4] = { 0, 0, 0, 0 };  // Bits to force HIGH (lamp off)
  bool portTouched[4] = { false, false, false, false };

  for (byte i = 0; i < 7; i++) {
    byte pin = lampParm[LAMP_IDX_S + i].pinNum;
    byte port = pin >> 4;
    byte bit = pin & 0x0F;
    portTouched[port] = true;
    if (t_mask & (1 << i)) {
      onBits[port] |= ((uint16_t)1 << bit);   // Lamp ON = bit LOW
    } else {
      offBits[port] |= ((uint16_t)1 << bit);  // Lamp OFF = bit HIGH
    }
  }

  // Read-modify-write only the affected ports.
  // Use read-twice-compare to protect against I2C corruption from flipper PWM,
  // matching the defense in WriteRegisterPin() and the switch input reads.
  for (byte p = 0; p < 4; p++) {
    if (!portTouched[p]) continue;
    uint16_t read1 = (uint16_t)pShiftRegister->portRead(p);
    uint16_t read2 = (uint16_t)pShiftRegister->portRead(p);
    uint16_t val;
    if (read1 == read2) {
      val = read1;
    } else {
      // Tiebreaker read
      uint16_t read3 = (uint16_t)pShiftRegister->portRead(p);
      val = (read1 == read3) ? read1 : read2;
    }
    val &= ~onBits[p];   // Clear bits for lamps that should be ON (LOW = on)
    val |= offBits[p];   // Set bits for lamps that should be OFF (HIGH = off)
    pShiftRegister->portWrite(p, val);
  }
}

// ***** EFFECT-SAFE LAMP WRITE *****
// Writes a lamp state to the Centipede AND patches the lamp effect engine's
// saved state if an effect is currently running. Without the patch, the effect
// engine's restore would overwrite the lamp change on its next tick.
// t_lampIdx: lamp index (0..NUM_LAMPS_MASTER-1)
// t_on: true = lamp ON (pin LOW), false = lamp OFF (pin HIGH)
void setLampState(byte t_lampIdx, bool t_on) {
  byte pin = lampParm[t_lampIdx].pinNum;
  pShiftRegister->digitalWrite(pin, t_on ? LOW : HIGH);

  if (isLampEffectActive()) {
    byte port = pin >> 4;
    byte bit = pin & 0x0F;
    patchLampEffectSavedState(port, bit, t_on);
  }
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

// Returns true if the given kickout switch index belongs to a kickout that currently
// has a locked ball. Used to suppress phantom edges from locked balls vibrating on
// their switches. Only relevant for Enhanced style with ballsLocked > 0.
bool isLockedKickout(byte t_switchIdx) {
  if (gameStyle != STYLE_ENHANCED || kickoutLockedMask == 0) {
    return false;
  }
  if (t_switchIdx == SWITCH_IDX_KICKOUT_LEFT && (kickoutLockedMask & KICKOUT_LOCK_LEFT)) {
    return true;
  }
  if (t_switchIdx == SWITCH_IDX_KICKOUT_RIGHT && (kickoutLockedMask & KICKOUT_LOCK_RIGHT)) {
    return true;
  }
  return false;
}

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
// Kickout switches with locked balls are EXCLUDED -- those edges are phantom
// vibration artifacts, not real ball entries. See isLockedKickout().
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

  // Kickouts (skip locked kickouts -- those edges are phantom vibration, not real ball entries)
  if (switchJustClosedFlag[SWITCH_IDX_KICKOUT_LEFT] && !isLockedKickout(SWITCH_IDX_KICKOUT_LEFT)) {
    return SWITCH_IDX_KICKOUT_LEFT;
  }
  if (switchJustClosedFlag[SWITCH_IDX_KICKOUT_RIGHT] && !isLockedKickout(SWITCH_IDX_KICKOUT_RIGHT)) {
    return SWITCH_IDX_KICKOUT_RIGHT;
  }

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
// ************************************************** AUDIO TICK PROCESSOR ********************************************************
// ********************************************************************************************************************************

// Schedules a COM track to play after a delay (typically after bells and knocker finish).
// If a track is already pending, the new request overwrites it -- award announcements are
// rare enough that two never realistically overlap in the same bell window.
void scheduleComTrack(unsigned int trackNum, byte lenTenths, unsigned long delayMs, bool duckMusic) {
  pendingComTrack = trackNum;
  pendingComLenTenths = lenTenths;
  pendingComFireMs = millis() + delayMs;
  pendingComDuckMusic = duckMusic;
}

// Fires the pending COM track. Called from processAudioTick() when the delay expires.
void firePendingComTrack() {
  if (pendingComTrack == 0) {
    return;
  }
  audioPreemptComTrack();
  if (pendingComDuckMusic) {
    audioDuckMusicForVoice(true);
  }
  pTsunami->trackPlayPoly((int)pendingComTrack, 0, false);
  audioApplyTrackGain(pendingComTrack, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
  audioCurrentComTrack = pendingComTrack;
  audioComEndMillis = millis() + ((unsigned long)pendingComLenTenths * 100UL);

  pendingComTrack = 0;
  pendingComLenTenths = 0;
  pendingComFireMs = 0;
  pendingComDuckMusic = false;
}

// Cancels any pending deferred COM track. Called on tilt, game over, mid-game start.
void cancelPendingComTrack() {
  pendingComTrack = 0;
  pendingComLenTenths = 0;
  pendingComFireMs = 0;
  pendingComDuckMusic = false;
}

// Called every 10ms tick from loop(), regardless of game phase.
// Handles all time-based audio state transitions:
//   1. COM track end detection -> un-duck music
//   2. Music track end detection -> start next song
//   3. Future: SFX timeout, queued audio, priority preemption
void processAudioTick() {
  // Skip if no Tsunami hardware
  if (pTsunami == nullptr) {
    return;
  }

  // Only process audio events during active game phases (not attract, not diagnostic)
  if (gamePhase != PHASE_BALL_READY && gamePhase != PHASE_BALL_IN_PLAY
    && gamePhase != PHASE_STARTUP && gamePhase != PHASE_BALL_SEARCH
    && gamePhase != PHASE_TILT) {
    return;
  }

  // Only Enhanced style uses managed audio (Original/Impulse use mechanical sounds)
  if (gameStyle != STYLE_ENHANCED) {
    return;
  }

  unsigned long now = millis();

  // --- COM TRACK END DETECTION ---
  // When a voice/commentary track finishes, clear the tracking state and un-duck music.
  // This is the ONLY place un-ducking happens, guaranteeing it works regardless of which
  // game phase the COM track ended in.
  if (audioCurrentComTrack != 0 && now >= audioComEndMillis) {
    audioCurrentComTrack = 0;
    audioComEndMillis = 0;
    audioDuckMusicForVoice(false);
  }

  // --- DEFERRED COM TRACK FIRE ---
  // If a verbal announcement was scheduled to play after bells/knocker finish, fire it now.
  if (pendingComTrack != 0 && (long)(now - pendingComFireMs) >= 0) {
    if (audioCurrentComTrack == 0) {
      firePendingComTrack();
    } else {
      // A COM track started playing in the meantime (e.g. tilt warning).
      // Award announcements are high priority -- preempt and play.
      firePendingComTrack();
    }
  }

  // --- DEFERRED MUSIC RESTART (after mode end) ---
  // When a mode ends, music restart is deferred so the end-of-mode COM and stinger
  // are audible. Once the deadline passes, start primary theme music.
  if (modeEndMusicRestartMs != 0 && (long)(now - modeEndMusicRestartMs) >= 0) {
    modeEndMusicRestartMs = 0;
    ensureMusicPlaying();
    // If a COM track is still playing, duck the freshly started music
    if (audioCurrentComTrack != 0) {
      audioDuckMusicForVoice(true);
    }
  }

  // --- MUSIC TRACK END DETECTION ---
  // When the current music track finishes, automatically start the next track in the
  // selected theme playlist. This keeps music playing continuously during the entire game.
  // Music is NOT restarted if it was explicitly stopped (audioCurrentMusicTrack == 0).
  if (audioCurrentMusicTrack != 0 && now >= audioMusicEndMillis) {
    // Current track has ended -- advance to next track in playlist
    byte theme = getActiveMusicTheme();
    byte* lastIdxPtr = (theme == 0) ? &lastCircusTrackIdx : &lastSurfTrackIdx;
    audioStartPrimaryMusic(theme, lastIdxPtr,
      pTsunami, tsunamiMusicGainDb, tsunamiGainDb);

    const AudioMusTrackDef* trackArray = (theme == 0) ? musTracksCircus : musTracksSurf;
    audioCurrentMusicTrack = pgmReadMusTrackNum(&trackArray[*lastIdxPtr]);
    audioMusicEndMillis = now + ((unsigned long)pgmReadMusLengthSeconds(&trackArray[*lastIdxPtr]) * 1000UL);

    // If a COM track is currently playing, the new music track should start ducked
    if (audioCurrentComTrack != 0) {
      audioDuckMusicForVoice(true);
    }

    // Save theme preference so we resume from the right track after power cycle
    audioSaveThemePreference(primaryMusicTheme, lastCircusTrackIdx, lastSurfTrackIdx);
  }

  // --- SFX TRACK END DETECTION ---
  // Hill drop shaker timing is already handled in updatePhaseBallInPlay().
  // Future: add SFX timeout tracking here if needed.
}

// ********************************************************************************************************************************
// **************************************************** AUDIO HELPERS *************************************************************
// ********************************************************************************************************************************

void processVolumeButtons() {
  // Allow master volume adjustment via LEFT/RIGHT diag buttons in any non-diagnostic phase.
  // In attract mode this also starts a preview music track so the player can hear the change.
  if (gamePhase == PHASE_DIAGNOSTIC) {
    return;
  }

  if (diagButtonPressed(1)) {  // LEFT (minus)
    if (gamePhase == PHASE_ATTRACT && audioCurrentMusicTrack == 0) {
      audioStartMusicTrack(pgmReadMusTrackNum(&musTracksCircus[0]), pgmReadMusLengthSeconds(&musTracksCircus[0]), false);
    }
    audioAdjustMasterGainQuiet(-1);
  }
  if (diagButtonPressed(2)) {  // RIGHT (plus)
    if (gamePhase == PHASE_ATTRACT && audioCurrentMusicTrack == 0) {
      audioStartMusicTrack(pgmReadMusTrackNum(&musTracksCircus[0]), pgmReadMusLengthSeconds(&musTracksCircus[0]), false);
    }
    audioAdjustMasterGainQuiet(+1);
  }
}

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

// ***** PREEMPT CURRENT COM TRACK *****
// Stops any currently playing COM track on the Tsunami hardware so a new
// higher-priority COM track can play cleanly. Without this, trackPlayPoly()
// layers the new track on top of the old one and both are audible.
// Called before playing any COM track that should have exclusive voice priority.
void audioPreemptComTrack() {
  if (audioCurrentComTrack != 0 && pTsunami != nullptr) {
    pTsunami->trackStop((int)audioCurrentComTrack);
    audioCurrentComTrack = 0;
    audioComEndMillis = 0;
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

void audioDuckMusicForVoice(bool duck) {
  if (pTsunami == nullptr || audioCurrentMusicTrack == 0) {
    return;
  }

  // Only apply category offset as per-track gain.
  // Master gain is already applied at the Tsunami output level by audioApplyMasterGain().
  int normalGain = (int)tsunamiMusicGainDb;

  if (duck) {
    int duckGain = normalGain + (int)tsunamiDuckingDb;
    if (duckGain < -70) duckGain = -70;
    pTsunami->trackGain(audioCurrentMusicTrack, duckGain);
  } else {
    pTsunami->trackGain(audioCurrentMusicTrack, normalGain);
  }
}

// Return the music theme to use right now: alternate theme during multiball
// or mode, primary theme otherwise. Primary theme 0 = Circus, 1 = Surf.
// Alternate is simply the other one (1 - primary).
byte getActiveMusicTheme() {
  if (inMultiball || modeState.active) {
    return 1 - primaryMusicTheme;  // Swap: Circus <-> Surf
  }
  return primaryMusicTheme;
}

// Start the next primary music track if none is currently playing.
// Returns true if a new track was started, false if music was already playing.
bool ensureMusicPlaying() {
  if (audioCurrentMusicTrack != 0) {
    return false;
  }
  byte theme = getActiveMusicTheme();
  byte* lastIdxPtr = (theme == 0) ? &lastCircusTrackIdx : &lastSurfTrackIdx;
  audioStartPrimaryMusic(theme, lastIdxPtr,
    pTsunami, tsunamiMusicGainDb, tsunamiGainDb);
  const AudioMusTrackDef* trackArray = (theme == 0) ? musTracksCircus : musTracksSurf;
  audioCurrentMusicTrack = pgmReadMusTrackNum(&trackArray[*lastIdxPtr]);
  audioMusicEndMillis = millis() + ((unsigned long)pgmReadMusLengthSeconds(&trackArray[*lastIdxPtr]) * 1000UL);
  return true;
}

// Play the appropriate "ball missing" or "ball in lift" voice announcement.
// Checks the ball-in-lift switch live and plays the contextual track.
void playBallMissingAudio() {
  if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
    pTsunami->trackPlayPoly(TRACK_BALL_IN_LIFT, 0, false);
    audioApplyTrackGain(TRACK_BALL_IN_LIFT, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
    audioCurrentComTrack = TRACK_BALL_IN_LIFT;
    audioComEndMillis = millis() + ((unsigned long)pgmReadComLengthTenths(&comTracksBallMissing[1]) * 100UL);
  } else {
    unsigned int trackNum = pgmReadComTrackNum(&comTracksBallMissing[0]);
    byte trackLenTenths = pgmReadComLengthTenths(&comTracksBallMissing[0]);
    pTsunami->trackPlayPoly((int)trackNum, 0, false);
    audioApplyTrackGain(trackNum, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
    audioCurrentComTrack = trackNum;
    audioComEndMillis = millis() + ((unsigned long)trackLenTenths * 100UL);
  }
}

// ********************************************************************************************************************************
// ************************************************ GAME STATE MANAGEMENT *********************************************************
// ********************************************************************************************************************************

void initGameState() {
  // Close ball tray if still open (safety - prevents solenoid overheating)
  if (gameState.ballTrayOpen) {
    releaseDevice(DEV_IDX_BALL_TRAY_RELEASE);
    gameState.ballTrayOpen = false;
  }
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
  multiballPending = false;
  extraBallAwarded = false;
  kickoutLockedMask = 0;
  multiballSaveEndMs = 0;
  multiballSavesRemaining = 0;
  multiballSaveDispensePending = false;
  multiballSaveWaitingForLift = false;

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
  modeIntroEndMs = 0;
  modeTimerStarted = false;
  modeSaveDispensePending = false;
  modeSaveWaitingForLift = false;
  modeSaveDispenseMs = 0;
  modeEndMusicRestartMs = 0;
  preModeBumperLitMask = 0x7F;
  preModeWhiteLitMask = 0;
  preModeSelectionUnitPosition = 0;
  preModeGobbleCount = 0;
  troughSanityStableStartMs = 0;
  troughSanityWasStable = false;

  // Undetected drain recovery
  undetectedDrainDeadlineMs = 0;
  undetectedDrainPausedRemainingMs = 0;

  // Ball save
  ballSave.endMs = 0;
  ballSaveUsed = false;
  ballSaveConsumedMs = 0;
  savedBallFirstSwitchMs = 0;

  // EM emulation state: load persisted values from EEPROM.
  // The original game retained Selection Unit position across games and power cycles
  // because the physical stepper simply stayed where it was.
  // However, the Relay Reset Bank fires at game start, which clears the WHITE insert
  // relay latches, the gobble count relays, and relights all bumpers.
  // So we load selectionUnitPosition from EEPROM but reset the others.
  loadEmState();
  bumperLitMask = 0x7F;  // All 7 bumpers lit (Relay Reset Bank fires at game start)
  whiteLitMask = 0;       // All WHITE inserts off (Relay Reset Bank clears latches)
  gobbleCount = 0;        // Gobble count reset (Relay Reset Bank clears latches)
  whiteRowsAwarded = 0;   // Reset 3-in-a-row tracking for new matrix cycle
  whiteFourCornersAwarded = false;  // Reset four-corners tracking for new matrix cycle

  // Replay tracking
  replayScoreAwarded = 0;  // Reset replay score threshold tracking for new game
  replayQueueCount = 0;    // Clear any pending replay knocker queue
  replayNextKnockMs = 0;
  replayKnocksThisCycle = 0;
  replayPendingCount = 0;  // Clear any replays waiting for Score Motor

  // Credit tracking
  creditDeducted = false;

  // Ball 1 one-time sequence tracking
  ball1SequencePlayed = false;

  // Shoot reminder
  shootReminderDeadlineMs = 0;

  // Score motor startup sequence
  if (scoreMotorStartMs != 0) {
    releaseDevice(DEV_IDX_MOTOR_SCORE);
    scoreMotorStartMs = 0;
  }
  scoreMotorSlotRelayReset2 = false;
  scoreMotorSlotScoreReset = false;

  // Score motor gameplay timer
  if (scoreMotorGameplayEndMs != 0) {
    analogWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, 0);
    scoreMotorGameplayEndMs = 0;
  }

  // Reset Spots Number lamp cache so first addScore() unconditionally writes
  spotsNumberLastTenKDigit = 0xFF;
  spotsNumberLastKickoutMask = 0xFF;

}

void onEnterBallReady() {

  // Stop any running shakers
  stopAllShakers();

  // Ball tracking: reset per-ball state. See Overview Section 10.3.1.
  ballsInPlay = 0;
  ballInLift = true;  // Ball was just dispensed to lift by STARTUP_COMPLETE
  ballHasLeftLift = false;  // Ball has not left the lift yet; latches true on first departure
  // ballsLocked is NOT reset -- locked balls persist across ball numbers
  // kickoutLockedMask is NOT reset -- tracks which kickouts have locked balls

  // Ball save timer: always clear the timer on entry to BALL_READY.
  // A new timer will be started when the first playfield switch is hit,
  // BUT only if ballSaveUsed is false (save not yet consumed this ball number).
  // ballSaveUsed is NOT cleared here -- it persists across save replacements
  // and is only cleared when advancing to a new ball number (in processBallLost)
  // or at game start (in initGameState).
  ballSave.endMs = 0;

  // Multiball: only clear these flags if NOT entering with 2 locked balls.
  // When the 2nd ball is locked, handleSwitchKickout() sets multiballPending = true
  // and dispatches a replacement ball through STARTUP_WAIT_FOR_LIFT. That replacement
  // arrives here. If we cleared multiballPending, the replacement ball would never
  // trigger startMultiball() when it hits a target. So we preserve multiballPending
  // when ballsLocked == 2. inMultiball is always cleared because if we got here,
  // multiball has ended (ballsInPlay was 0).
  inMultiball = false;
  multiballSaveEndMs = 0;
  multiballSavesRemaining = 0;
  multiballSaveDispensePending = false;
  multiballSaveWaitingForLift = false;
  if (ballsLocked < 2) {
    multiballPending = false;
  }

  // Hill climb tracking: reset unconditionally. The ball1SequencePlayed guard
  // in updatePhaseBallReady() prevents the hill climb from running on replacement balls.
  hillClimbStarted = false;

  // Reset ball settled tracking WITH EDGE DETECTION
  // STARTUP_COMPLETE confirmed the ball is in the lift, so the ball is already settled.
  prevBallInLift = true;
  ballSettledInLift = true;

  // ALWAYS reset hasHitSwitch for each new ball attempt (including ball save replacements).
  // The ball save timer is the gate for saves, not hasHitSwitch.
  gameState.hasHitSwitch = false;
  gameState.tiltWarnings = 0;
  gameState.tilted = false;

  // Clear tilt bob settle timer for new ball
  tiltBobSettleUntilMs = 0;

  // Start shoot reminder timer (Enhanced only; checked in updatePhaseBallReady)
  shootReminderDeadlineMs = millis() + HILL_CLIMB_TIMEOUT_MS;

  // Clear any residual debounce and snap baselines to current physical state
  // for ALL playfield switches, not just drains. Without this, a playfield
  // switch whose switchLastState was stuck at "closed" (from a stale debounce
  // snap during PHASE_STARTUP) would never fire a closing edge when the player
  // actually hits it -- making the switch appear intermittently dead.
  // This was the root cause of the "E" pop bumper intermittently not responding.
  for (byte sw = SWITCH_IDX_BUMPER_S; sw <= SWITCH_IDX_DRAIN_RIGHT; sw++) {
    switchDebounceCounter[sw] = 0;
    switchLastState[sw] = switchClosed(sw) ? 1 : 0;
  }

  // Reset trough sanity check tracking for new ball
  troughSanityWasStable = false;
  troughSanityStableStartMs = 0;

  // Reset undetected drain timer (will be started when PHASE_BALL_IN_PLAY begins)
  undetectedDrainDeadlineMs = 0;
  undetectedDrainPausedRemainingMs = 0;

  // Re-suppress locked kickout switches. When a ball is locked in a kickout for
  // multiball, the ball sits on the switch indefinitely. Any prior suppression from
  // handleSwitchKickout() may have expired during the replacement ball's transit
  // through the trough. Without fresh suppression here, updateAllSwitchStates()
  // would have already snapped switchLastState to "closed" when the old suppression
  // expired, and any tiny vibration causing the switch to open and re-close would
  // fire a phantom switchJustClosedFlag edge -- triggering a second lock/score event.
  // We apply 200 ticks (2 seconds) of suppression so the replacement ball has time
  // to be launched and hit a real target before the locked kickout switch is eligible
  // to fire again. This is a belt-and-suspenders safety net -- the primary protection
  // is isLockedKickout() in checkPlayfieldSwitchHit(), which permanently blocks edges
  // from locked kickouts regardless of debounce state.
  if (gameStyle == STYLE_ENHANCED && kickoutLockedMask != 0) {
    if (kickoutLockedMask & KICKOUT_LOCK_LEFT) {
      switchDebounceCounter[SWITCH_IDX_KICKOUT_LEFT] = 200;
      switchLastState[SWITCH_IDX_KICKOUT_LEFT] = 1;  // Baseline = closed (ball is there)
    }
    if (kickoutLockedMask & KICKOUT_LOCK_RIGHT) {
      switchDebounceCounter[SWITCH_IDX_KICKOUT_RIGHT] = 200;
      switchLastState[SWITCH_IDX_KICKOUT_RIGHT] = 1;  // Baseline = closed (ball is there)
    }
  }
}

bool isInMode() {
  return modeState.active;
}

// ********************************************************************************************************************************
// ************************************************* MODE LIFECYCLE ***************************************************************
// ********************************************************************************************************************************

// ***** START MODE *****
// Called from handleLastLitBumper() when SCREAMO is completed and all preconditions
// are met (Enhanced style, not in multiball, not in mode, modeCount < 3, multiball
// not just started on this tick).
// Per Overview Section 14.7.1: Mode Start Sequence.
void startMode() {
  // Determine which mode is next (rotate: BUMPER_CARS -> ROLL_A_BALL -> GOBBLE_HOLE)
  byte lastModePlayed = EEPROM.read(EEPROM_ADDR_LAST_MODE_PLAYED);
  if (lastModePlayed < MODE_BUMPER_CARS || lastModePlayed > MODE_GOBBLE_HOLE) {
    lastModePlayed = MODE_GOBBLE_HOLE;  // So next will be BUMPER_CARS
  }
  byte nextMode;
  if (lastModePlayed == MODE_BUMPER_CARS) {
    nextMode = MODE_ROLL_A_BALL;
  } else if (lastModePlayed == MODE_ROLL_A_BALL) {
    nextMode = MODE_GOBBLE_HOLE;
  } else {
    nextMode = MODE_BUMPER_CARS;
  }

  // Populate mode state
  modeState.active = true;
  modeState.currentMode = nextMode;
  modeState.modeCount++;
  modeState.modeProgress = 0;
  modeState.reminderPlayed = false;
  modeState.hurryUpPlayed = false;

  // Read mode time limit from EEPROM (modeNum 1-3 maps to BUMPER_CARS..GOBBLE_HOLE)
  byte modeSeconds = diagReadModeTimeFromEEPROM(nextMode);
  modeState.timerDurationMs = (unsigned long)modeSeconds * 1000UL;
  modeState.timerStartMs = 0;  // Timer starts after intro ends
  modeState.timerPaused = false;
  modeState.pausedTimeRemainingMs = 0;

  // Mode ball replacement state
  modeSaveDispensePending = false;
  modeSaveWaitingForLift = false;
  modeSaveDispenseMs = 0;

  // Cancel single-ball save timer (mode handles its own ball replacement)
  ballSave.endMs = 0;

  // Persist which mode we are starting to EEPROM
  EEPROM.update(EEPROM_ADDR_LAST_MODE_PLAYED, nextMode);

  // Cancel any running lamp effect before setting mode lamps.
  cancelLampEffect(pShiftRegister);

  // Save EM state that mode gameplay might mutate. The mode has its own scoring
  // rules, but until mode scoring dispatch is fully implemented (Step 4), normal
  // handlers run and can change bumperLitMask. Even after Step 4, this provides
  // a safety net so the mode cannot permanently corrupt the EM tracking state.
  preModeBumperLitMask = bumperLitMask;
  preModeWhiteLitMask = whiteLitMask;
  preModeSelectionUnitPosition = selectionUnitPosition;
  preModeGobbleCount = gobbleCount;

  // Set mode-specific lamp layout (turn off non-GI, light targets)
  setModeLamps(nextMode);

  // Dramatic mode-start lamp effect: first flash all lamps a few times for dramatic
  // impact, then fill-up/drain-down reveals the mode target lamps underneath.
  // Phase 1 (flash-all) starts here; processModeTick() chains phase 2 (fill-drain)
  // when the flash completes and isLampEffectActive() returns false.
  // Per Section 14.7.1: cancel any running effect before starting a new one.
  cancelLampEffect(pShiftRegister);
  startLampEffect(LAMP_EFFECT_FLASH_ALL, 120, 0);
  modeStartFlashPending = true;

  // ***** MODE START AUDIO SEQUENCE *****
  // Per Overview Section 14.7.1:
  //   1. Stop regular play music
  //   2. Play mode-start stinger (school bell SFX)
  //   3. Play mode introduction announcement (mode-specific COM)
  //   4. Alternate theme music starts when intro ends (handled in processModeTick)

  // Stop current music
  audioStopMusic();

  // Cancel any pending COM track (e.g. "You spelled SCREAMO!" scheduled by handleLastLitBumper)
  pendingComTrack = 0;

  // Play mode-start stinger SFX (school bell ringing)
  if (pTsunami != nullptr) {
    pTsunami->trackPlayPoly(TRACK_MODE_STINGER_START, 0, false);
    audioApplyTrackGain(TRACK_MODE_STINGER_START, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
  }

  // Tactile "ding" shaker pulse to match the school bell stinger.
  // Uses the back-dated shakerDropStartMs trick so the existing shaker management
  // in updatePhaseBallInPlay() auto-stops after SHAKER_MODE_START_MS.
  if (gameStyle == STYLE_ENHANCED) {
    analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_ATTENTION);
    shakerDropStartMs = millis() - (SHAKER_HILL_DROP_MS - SHAKER_MODE_START_MS);
  }

  // Schedule mode intro COM track after a 1-second delay so the school bell
  // stinger is audible before the voice starts.
  audioPreemptComTrack();
  unsigned int introTrack = 0;
  byte introLenTenths = 0;
  switch (nextMode) {
  case MODE_BUMPER_CARS:
    introTrack = TRACK_MODE_BUMPER_INTRO;
    introLenTenths = pgmReadComLengthTenths(&comTracksMode[3]);  // Index 3 = track 1101
    break;
  case MODE_ROLL_A_BALL:
    introTrack = TRACK_MODE_RAB_INTRO;
    introLenTenths = pgmReadComLengthTenths(&comTracksMode[5]);  // Index 5 = track 1201
    break;
  case MODE_GOBBLE_HOLE:
    introTrack = TRACK_MODE_GOBBLE_INTRO;
    introLenTenths = pgmReadComLengthTenths(&comTracksMode[7]);  // Index 7 = track 1301
    break;
  }
  if (introTrack != 0) {
    scheduleComTrack(introTrack, introLenTenths, 1000, false);  // 1s delay for stinger
  }

  // Mode timer starts when the intro COM track finishes.
  // Intro starts after 1000ms delay, then plays for introLenTenths * 100ms.
  modeIntroEndMs = millis() + 1000UL + ((unsigned long)introLenTenths * 100UL);
  modeTimerStarted = false;

  // Debug display
  const char* modeNames[] = { "", "BUMPER CARS", "ROLL-A-BALL", "GOBBLE HOLE" };
  snprintf(lcdString, LCD_WIDTH + 1, "MODE: %s", modeNames[nextMode]);
  lcdPrintRow(3, lcdString);
  snprintf(lcdString, LCD_WIDTH + 1, "Time: %us  #%u/3",
    modeSeconds, modeState.modeCount);
  lcdPrintRow(4, lcdString);
}

// ***** PROCESS MODE TICK *****
// Called every tick from loop(). Manages mode timer, checks for expiration,
// triggers reminder and hurry-up audio at the right times.
// Scoring dispatch is handled separately in updatePhaseBallInPlay().
void processModeTick() {
  if (!modeState.active) {
    return;
  }

  // Chain the fill-drain lamp effect after the flash-all completes.
  // The flash-all was started by startMode(); once it finishes, we kick off
  // the fill-up/drain-down that reveals the mode target lamps underneath.
  if (modeStartFlashPending && !isLampEffectActive()) {
    modeStartFlashPending = false;
    startLampEffect(LAMP_EFFECT_FILL_UP_DRAIN_DOWN, 400, 200);
  }

  // Wait for intro to finish before starting the gameplay timer
  if (!modeTimerStarted) {
    if ((long)(millis() - modeIntroEndMs) >= 0) {
      modeTimerStarted = true;
      modeState.timerStartMs = millis();

      // Start alternate theme music now that the intro has finished.
      // getActiveMusicTheme() returns the alternate theme because modeState.active is true.
      ensureMusicPlaying();
    } else {
      return;  // Still in intro
    }
  }

  // Calculate remaining time
  unsigned long elapsed = millis() - modeState.timerStartMs;
  unsigned long remainingMs = 0;
  if (elapsed < modeState.timerDurationMs) {
    remainingMs = modeState.timerDurationMs - elapsed;
  }

  // Check for mode completion (jackpot achieved)
  // BUMPER_CARS: modeProgress == 7 means all 7 bumpers hit in sequence
  // GOBBLE_HOLE: modeProgress == 5 means 5 balls sunk
  // ROLL_A_BALL: modeProgress == 10 means 10 hat rollovers
  if (modeState.currentMode == MODE_BUMPER_CARS && modeState.modeProgress >= 7) {
    endMode(true);
    return;
  }
  if (modeState.currentMode == MODE_GOBBLE_HOLE && modeState.modeProgress >= 5) {
    endMode(true);
    return;
  }
  if (modeState.currentMode == MODE_ROLL_A_BALL && modeState.modeProgress >= 10) {
    endMode(true);
    return;
  }

  // Check timer expiration
  if (remainingMs == 0) {
    endMode(false);
    return;
  }

  // Reminder announcement: 10 seconds after the mode timer starts
  if (!modeState.reminderPlayed) {
    unsigned long reminderThresholdMs = 10000UL;
    if (elapsed >= reminderThresholdMs) {
      modeState.reminderPlayed = true;
      unsigned int reminderTrack = 0;
      byte reminderLenTenths = 0;
      switch (modeState.currentMode) {
      case MODE_BUMPER_CARS:
        reminderTrack = TRACK_MODE_BUMPER_REMIND;
        reminderLenTenths = pgmReadComLengthTenths(&comTracksMode[4]);  // Index 4 = track 1111
        break;
      case MODE_ROLL_A_BALL:
        reminderTrack = TRACK_MODE_RAB_REMIND;
        reminderLenTenths = pgmReadComLengthTenths(&comTracksMode[6]);  // Index 6 = track 1211
        break;
      case MODE_GOBBLE_HOLE:
        reminderTrack = TRACK_MODE_GOBBLE_REMIND;
        reminderLenTenths = pgmReadComLengthTenths(&comTracksMode[8]);  // Index 8 = track 1311
        break;
      }
      if (reminderTrack != 0 && pTsunami != nullptr) {
        audioPreemptComTrack();
        audioDuckMusicForVoice(true);
        pTsunami->trackPlayPoly((int)reminderTrack, 0, false);
        audioApplyTrackGain(reminderTrack, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
        audioCurrentComTrack = reminderTrack;
        audioComEndMillis = millis() + ((unsigned long)reminderLenTenths * 100UL);
      }
    }
  }

  // Hurry-up: 10 seconds remaining -- "Ten seconds!" + countdown SFX + lamp effect
  if (!modeState.hurryUpPlayed && remainingMs <= 10000) {
    modeState.hurryUpPlayed = true;

    // Play "Ten seconds left!" COM
    audioPreemptComTrack();
    audioDuckMusicForVoice(true);
    byte hurryLenTenths = pgmReadComLengthTenths(&comTracksMode[1]);  // Index 1 = track 1003
    if (pTsunami != nullptr) {
      pTsunami->trackPlayPoly(TRACK_MODE_HURRY, 0, false);
      audioApplyTrackGain(TRACK_MODE_HURRY, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_MODE_HURRY;
      audioComEndMillis = millis() + ((unsigned long)hurryLenTenths * 100UL);
    }

    // Play countdown timer SFX (plays alongside COM)
    if (pTsunami != nullptr) {
      pTsunami->trackPlayPoly(TRACK_MODE_COUNTDOWN, 0, false);
      audioApplyTrackGain(TRACK_MODE_COUNTDOWN, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
    }

    // Start urgency lamp effect (fast flash)
    startLampEffect(LAMP_EFFECT_FLASH_ALL, 120, 0);
  }

  // Debug: show countdown on LCD row 4, but ONLY when the seconds value changes.
  // lcdPrintRow() sends ~25 bytes over serial at 115200 baud (~3-4ms per call).
  // Calling it every 10ms tick would consume 30-40% of the tick budget and trigger
  // the SLOW LOOP warning, which itself calls lcdPrintRow(), creating a feedback loop.
  static byte lastDisplayedSeconds = 0xFF;
  byte secondsLeft = (byte)(remainingMs / 1000UL);
  if (secondsLeft != lastDisplayedSeconds) {
    lastDisplayedSeconds = secondsLeft;
    snprintf(lcdString, LCD_WIDTH + 1, "Mode %us remain", secondsLeft);
    lcdPrintRow(4, lcdString);
  }
}

// ***** END MODE *****
// Called when mode timer expires or mode objective is completed.
// t_completed: true if the player achieved the jackpot, false if time ran out.
// Per Overview Section 14.7.1: Mode End Sequence.
void endMode(bool t_completed) {
  byte endingMode = modeState.currentMode;  // Save before clearing

  modeState.active = false;
  modeState.currentMode = MODE_NONE;
  modeTimerStarted = false;
  modeIntroEndMs = 0;

  // Clear mode ball replacement state
  modeSaveDispensePending = false;
  modeSaveWaitingForLift = false;
  modeSaveDispenseMs = 0;

  // Cancel any lamp effect that was started during the mode (e.g. hurry-up
  // flash) before restoring pre-mode lamps. Without this, the effect engine
  // would overwrite the restored lamps on its next tick.
  cancelLampEffect(pShiftRegister);

  // Restore EM tracking state that was saved when the mode started.
  // This must happen BEFORE setInitialGameplayLamps() so the lamp rebuild
  // uses the correct pre-mode values for bumperLitMask, whiteLitMask, etc.
  bumperLitMask = preModeBumperLitMask;
  whiteLitMask = preModeWhiteLitMask;
  selectionUnitPosition = preModeSelectionUnitPosition;
  gobbleCount = preModeGobbleCount;

  // Rebuild all gameplay lamps from the authoritative in-memory EM variables.
  // This is more reliable than the previous approach of reading back the
  // Centipede output latches (portRead) at mode start and restoring them
  // at mode end, which was vulnerable to I2C read errors and stale state.
  setInitialGameplayLamps();

  // Dramatic mode-end lamp effect. The effect engine saves the just-restored
  // lamps, plays the animation, then restores them -- so the player sees the
  // light show followed by their normal gameplay lamps reappearing.
  if (t_completed) {
    // Jackpot celebration: starburst radiating out from the gobble hole center
    // then collapsing back in. Extended sweep time for dramatic impact. ~2.2s total.
    startLampEffect(LAMP_EFFECT_STARBURST_GOBBLE, 800, 300);
  } else {
    // Time expired: sweep top-to-bottom then bottom-to-top (like the lights
    // dimming and coming back on). Brisk and clear. ~1.2s total.
    startLampEffect(LAMP_EFFECT_SWEEP_DOWN_UP, 500, 100);
  }

  // ***** MODE END AUDIO SEQUENCE *****
  // Per Overview Section 14.7.1: Mode End Sequence.
  //   1. Stop mode music
  //   2. Play completion or time-up announcement
  //   3. Play mode-end stinger SFX (factory whistle)
  //   4. Restart primary theme music

  // Stop mode music, all SFX, and any pending/current COM so the completion
  // or time-up announcement is clearly audible without competing audio.
  audioStopMusic();
  if (pTsunami != nullptr) {
    pTsunami->trackStop(TRACK_MODE_COUNTDOWN);  // Stop countdown if still playing
    // Stop any mode SFX still playing (e.g. slide whistle from final gobble hit,
    // ding-ding-ding from final bumper hit). Without this, "Jackpot!" is buried.
    if (audioCurrentSfxTrack != 0) {
      pTsunami->trackStop(audioCurrentSfxTrack);
      audioCurrentSfxTrack = 0;
    }
  }

  // Cancel any pending COM and preempt current COM
  pendingComTrack = 0;
  audioPreemptComTrack();

  // Play completion or time-up announcement + stinger + schedule music restart
  if (t_completed) {
    // ***** UNIFIED JACKPOT CELEBRATION (all three modes) *****
    // 1. "Jackpot! One million points!" COM (track 1002, ~9.0s with applause)
    // 2. Shaker burst for tactile impact
    // 3. Extended starburst lamp effect (started above)
    // 4. No factory whistle -- the applause baked into track 1002 IS the stinger
    // 5. Music restarts after the full jackpot track plays out
    if (pTsunami != nullptr) {
      audioDuckMusicForVoice(true);
      byte jackpotLenTenths = pgmReadComLengthTenths(&comTracksMode[0]);  // Index 0 = track 1002
      pTsunami->trackPlayPoly(TRACK_MODE_JACKPOT, 0, false);
      audioApplyTrackGain(TRACK_MODE_JACKPOT, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = TRACK_MODE_JACKPOT;
      audioComEndMillis = millis() + ((unsigned long)jackpotLenTenths * 100UL);
    }

    // Shaker burst for jackpot celebration
    if (gameStyle == STYLE_ENHANCED) {
      analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_HILL_DROP);
      shakerDropStartMs = millis() - (SHAKER_HILL_DROP_MS - SHAKER_JACKPOT_MS);
    }

    // Defer music restart until after the full jackpot track (applause included).
    // Track 1002 is 9.0s; add a small gap so the last note fades cleanly.
    byte jackpotLenTenths = pgmReadComLengthTenths(&comTracksMode[0]);
    modeEndMusicRestartMs = millis() + ((unsigned long)jackpotLenTenths * 100UL) + 500UL;

  } else {
    // ***** TIME EXPIRED *****
    // Play mode-specific time-up COM (includes factory whistle baked in) + subtle shaker.
    // Each mode has its own encouraging sign-off: "Time! Great job!" (Bumper Cars),
    // "Time! Great work!" (Roll-A-Ball), "Time! Well done!" (Gobble Hole).
    // These replace the generic TRACK_MODE_TIME_UP (1005) and the separate
    // TRACK_MODE_STINGER_END (1006) -- the whistle is already in each track.
    if (pTsunami != nullptr) {
      audioDuckMusicForVoice(true);
      unsigned int timeUpTrack = TRACK_MODE_TIME_UP;  // Fallback (should never be needed)
      byte timeUpLenTenths = pgmReadComLengthTenths(&comTracksMode[2]);  // Index 2 = generic 1005
      switch (endingMode) {
      case MODE_BUMPER_CARS:
        timeUpTrack = TRACK_MODE_BUMPER_TIME_UP;
        timeUpLenTenths = pgmReadComLengthTenths(&comTracksMode[5]);   // Index 5 = track 1198
        break;
      case MODE_ROLL_A_BALL:
        timeUpTrack = TRACK_MODE_RAB_TIME_UP;
        timeUpLenTenths = pgmReadComLengthTenths(&comTracksMode[8]);   // Index 8 = track 1298
        break;
      case MODE_GOBBLE_HOLE:
        timeUpTrack = TRACK_MODE_GOBBLE_TIME_UP;
        timeUpLenTenths = pgmReadComLengthTenths(&comTracksMode[11]);  // Index 11 = track 1398
        break;
      }
      pTsunami->trackPlayPoly((int)timeUpTrack, 0, false);
      audioApplyTrackGain(timeUpTrack, tsunamiVoiceGainDb, tsunamiGainDb, pTsunami);
      audioCurrentComTrack = timeUpTrack;
      audioComEndMillis = millis() + ((unsigned long)timeUpLenTenths * 100UL);
    }

    // Subtle shaker pulse — a gentle "time's up" nudge, softer than the jackpot burst.
    if (gameStyle == STYLE_ENHANCED) {
      analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, SHAKER_POWER_ATTENTION);
      shakerDropStartMs = millis() - (SHAKER_HILL_DROP_MS - SHAKER_TIME_UP_MS);
    }

    // Defer primary theme music restart until the mode-specific time-up track finishes.
    // The tracks are ~3.3s each; add a small gap so the last word fades cleanly.
    byte deferLenTenths = pgmReadComLengthTenths(&comTracksMode[2]);  // Fallback length
    switch (endingMode) {
    case MODE_BUMPER_CARS:
      deferLenTenths = pgmReadComLengthTenths(&comTracksMode[5]);
      break;
    case MODE_ROLL_A_BALL:
      deferLenTenths = pgmReadComLengthTenths(&comTracksMode[8]);
      break;
    case MODE_GOBBLE_HOLE:
      deferLenTenths = pgmReadComLengthTenths(&comTracksMode[11]);
      break;
    }
    modeEndMusicRestartMs = millis() + ((unsigned long)deferLenTenths * 100UL) + 500UL;
  }

  // If 2 balls are locked, set multiball pending so the next scoring switch
  // triggers multiball (per Overview Section 14.7.1 Mode End Sequence step 8).
  if (ballsLocked == 2) {
    multiballPending = true;
  }

  // Debug display
  if (t_completed) {
    snprintf(lcdString, LCD_WIDTH + 1, "MODE COMPLETE!");
    lcdPrintRow(3, lcdString);
  } else {
    snprintf(lcdString, LCD_WIDTH + 1, "MODE TIME UP!");
    lcdPrintRow(3, lcdString);
  }
  snprintf(lcdString, LCD_WIDTH + 1, "Modes used: %u/3", modeState.modeCount);
  lcdPrintRow(4, lcdString);
}

// ***** MODE LAMP SETUP *****
// Sets the playfield lamps for the active mode. Called from startMode() after
// saving the pre-mode lamp state. GI lamps are untouched (they stay on).
// All non-GI lamps are turned off first, then only the mode-relevant target
// lamps are lit so the player can see what to aim for.
// Uses writeLampsBulk() for a single fast bulk write (~2ms) instead of
// individual digitalWrite() calls (~30ms for 40+ lamps).
void setModeLamps(byte t_mode) {
  // Build the desired lamp state array: GI stays as-is, everything else off,
  // then turn on the mode-specific target lamps.
  byte desired[NUM_LAMPS_MASTER];

  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    if (lampParm[i].groupNum == LAMP_GROUP_GI) {
      desired[i] = 1;  // GI stays on
    } else {
      desired[i] = 0;  // Everything else off by default
    }
  }

  switch (t_mode) {
  case MODE_BUMPER_CARS:
    // Light all 7 bumper lamps. They turn off as the player hits each one.
    for (byte i = LAMP_IDX_S; i <= LAMP_IDX_O; i++) {
      desired[i] = 1;
    }
    break;

  case MODE_ROLL_A_BALL:
    // Light all 4 HAT lamps. They are the scoring targets for this mode.
    desired[LAMP_IDX_HAT_LEFT_TOP] = 1;
    desired[LAMP_IDX_HAT_LEFT_BOTTOM] = 1;
    desired[LAMP_IDX_HAT_RIGHT_TOP] = 1;
    desired[LAMP_IDX_HAT_RIGHT_BOTTOM] = 1;
    break;

  case MODE_GOBBLE_HOLE:
    // GOBBLE lamps start OFF. Each successful gobble hit lights one (1-5).
    // SPECIAL lamp: if it was already lit (gobbleCount >= 4), keep it lit.
    if (gobbleCount >= 4) {
      desired[LAMP_IDX_SPECIAL] = 1;
    }
    break;
  }

  writeLampsBulk(desired);
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
  bool silentBells = false;
  bool silent10KUnit = false;

  if (gameStyle == STYLE_ENHANCED) {
    silent10KUnit = true;
    // Bells ring during reset (same as normal Enhanced play).
    // Only modes silence bells, and we are never in a mode during reset.
  }

  pMessage->sendMAStoSLVScoreReset(silentBells, silent10KUnit);
}

bool useMechanicalScoringDevices() {
  // Returns true if Score Motor, Selection Unit, and Relay Reset Bank should be used.
  // These are only used in Original and Impulse styles, never in Enhanced.
  return (gameStyle != STYLE_ENHANCED);
}

unsigned long calculateScoreMotorRunMs(int t_currentScore) {
  // OVERLOAD #1: For score reset sequence, we estimate total motor run time based on the current score being reset.
  // Returns the total time in milliseconds that the Score Motor SSR should remain ON
  // for a given game-start score reset sequence in Original/Impulse style.
  // Based on the real Screamo score motor observed in slow motion:
  //   1st quarter-turn (882ms): relay reset x2 + credit deduction (all within first 1/4 rev)
  //   2nd quarter-turn (882ms): 100K fast reset begins + 10K step-up begins simultaneously
  //   Additional quarter-turns: as needed by Slave to finish the 10K step-up (up to 10 steps = 2 cycles)
  // Master does NOT need to calculate exactly how many steps Slave needs; Master just needs
  // to keep the motor running long enough. Running it a bit too long is harmless.
  // t_currentScore: the score being reset (0..999), used to estimate how long Slave needs.
  // 1st quarter-rev: relay resets + credit deduction
  unsigned long totalMs = SCORE_MOTOR_QUARTER_REV_MS;
  // 2nd quarter-rev: 100K fast reset + 10K step-up begins
  totalMs += SCORE_MOTOR_QUARTER_REV_MS;
  // 10K step-up: Slave advances the 10K unit from its current position back to 0.
  // Worst case is 10 steps (if tensK digit is 0, Slave advances all the way around).
  // 10 steps = 2 motor cycles (5 advances + 1 rest each) = 2 quarter-revs.
  // We always allow 2 extra quarter-revs to be safe; the motor running a bit extra is harmless.
  totalMs += SCORE_MOTOR_QUARTER_REV_MS * 2;
  return totalMs;  // 4 quarter-revs = 3528ms total
}

unsigned long calculateScoreMotorRunMs(byte msgType, int magnitude) {
  // OVERLOAD #2: For individual score increments during play, Master can calculate a more precise motor run time based
  // on the message type and magnitude.
  if (msgType == RS485_TYPE_MAS_TO_SLV_SCORE_RESET) {
    // Worst case: 3 quarter-revs of actual work + 1 quarter-rev safety margin.
    return 4 * SCORE_MOTOR_QUARTER_REV_MS;
  }
  if (msgType == RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K) {
    int absMag = (magnitude < 0) ? -magnitude : magnitude;
    if (absMag == 0) {
      return 0;  // No-op, no motor needed.
    }

    // Determine how many "units" Slave will process per motor cycle.
    // If absMag is evenly divisible by 10, Slave treats it as 100K steps (each step = 10 units).
    // Otherwise, Slave treats it as individual 10K steps.
    int units;
    if ((absMag % 10) == 0) {
      units = absMag / 10;  // Number of 100K steps
    } else {
      units = absMag;       // Number of 10K steps
    }

    // Single-unit commands (exactly 1 step) do not use the score motor.
    if (units == 1) {
      return 0;
    }
    // Multi-unit: ceil(units / 5) motor cycles, each taking one quarter-rev.
    // Plus one quarter-rev post-cycle rest, plus one quarter-rev safety margin.
    int cycles = (units + 4) / 5;  // Integer ceiling division
    return (unsigned long)(cycles + 2) * SCORE_MOTOR_QUARTER_REV_MS;
  }

  // Any other message type does not use the Score Motor.
  return 0;
}

// ***** SCORE MOTOR STARTUP SEQUENCE TICK PROCESSOR *****
// Called every tick from loop() (not just during PHASE_STARTUP) so the score motor
// completes its 3528ms run even after the game transitions to PHASE_BALL_READY.
// Without this, when all 5 balls are already in the trough the startup phase completes
// in ~1 second, but the score motor needs ~3.5 seconds to finish its 4 quarter-turns.
void processScoreMotorStartupTick() {
  if (scoreMotorStartMs == 0) {
    return;
  }

  unsigned long elapsed = millis() - scoreMotorStartMs;

  // Sub-slot 4 (T+441ms): Fire Relay Reset Bank (2nd firing)
  if (!scoreMotorSlotRelayReset2 && elapsed >= SCORE_MOTOR_SUB_SLOT_MS * 3) {
    activateDevice(DEV_IDX_RELAY_RESET);
    scoreMotorSlotRelayReset2 = true;
  }

  // Start of 2nd quarter-turn (T+882ms): Send score reset to Slave
  if (!scoreMotorSlotScoreReset && elapsed >= SCORE_MOTOR_QUARTER_REV_MS) {
    sendScoreReset();
    scoreMotorSlotScoreReset = true;
  }

  // End of 4th quarter-turn (T+3528ms): Turn off Score Motor
  if (elapsed >= SCORE_MOTOR_QUARTER_REV_MS * 4) {
    releaseDevice(DEV_IDX_MOTOR_SCORE);
    scoreMotorStartMs = 0;
  }
}

// ***** SCORE MOTOR GAMEPLAY TICK PROCESSOR *****
// Called every tick from loop(). Turns off the Score Motor SSR when the
// calculated run duration for a gameplay scoring event has elapsed.
// This is separate from processScoreMotorStartupTick() which handles
// the 4-quarter-turn startup sequence.
void processScoreMotorGameplayTick() {
  if (scoreMotorGameplayEndMs == 0) {
    return;
  }
  if ((long)(millis() - scoreMotorGameplayEndMs) >= 0) {
    // Time is up -- turn off the Score Motor
    analogWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, 0);
    scoreMotorGameplayEndMs = 0;
  }
}

// ***** KICKOUT EJECT TICK PROCESSOR *****
// Called every tick from loop(). Fires delayed kickout ejects for Original/Impulse style.
// The eject is timed to coincide with the 5th scoring sub-slot of the Score Motor
// quarter-rev (735ms after the kickout switch closed).
void processKickoutEjectTick() {
  if (kickoutLeftEjectMs != 0 && (long)(millis() - kickoutLeftEjectMs) >= 0) {
    activateDevice(DEV_IDX_KICKOUT_LEFT);
    kickoutLeftEjectMs = 0;
  }
  if (kickoutRightEjectMs != 0 && (long)(millis() - kickoutRightEjectMs) >= 0) {
    activateDevice(DEV_IDX_KICKOUT_RIGHT);
    kickoutRightEjectMs = 0;
  }
}

// ***** MULTIBALL SAVE DISPENSE TICK PROCESSOR *****
// Called every tick from loop(). Dispenses replacement balls during multiball ball
// save without changing the game phase. The game stays in PHASE_BALL_IN_PLAY so the
// surviving balls continue scoring normally while the replacement transits the trough.
// State machine:
//   1. multiballSaveDispensePending: fire the trough release coil
//   2. multiballSaveWaitingForLift: wait for ball-in-lift switch, then increment count
// Multiple replacements can be queued (multiballSavesRemaining > 1) if two balls drain
// within the save window before the first replacement arrives. Each is dispensed in
// sequence after the previous one clears the lift.
void processMultiballSaveDispenseTick() {
  // Only active during multiball (or briefly after if replacements are still in transit)
  if (multiballSavesRemaining == 0 && !multiballSaveWaitingForLift) {
    return;
  }

  // STEP 1: Dispense a replacement ball if one is pending
  if (multiballSaveDispensePending && !multiballSaveWaitingForLift) {
    // Only fire the trough release if the lift is empty. If a ball is already
    // at the lift base, skip the dispense -- STEP 2 will detect it immediately
    // and treat it as the replacement.
    if (!switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);
    }
    multiballSaveDispenseMs = millis();
    multiballSaveDispensePending = false;
    multiballSaveWaitingForLift = true;
    multiballSavesRemaining--;
    return;
  }

  // STEP 2: Wait for the replacement ball to reach the lift
  if (multiballSaveWaitingForLift) {
    if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      // Ball arrived at the lift -- it will be launched by the player (or roll out
      // on its own if the lift rod is already up). The ball is already counted in
      // ballsInPlay (we undid the decrement in handleBallDrain), so no increment needed.
      multiballSaveWaitingForLift = false;

      // If more replacements are queued, start the next one
      if (multiballSavesRemaining > 0) {
        multiballSaveDispensePending = true;
      }
      return;
    }

    // Timeout: if the ball does not reach the lift within 3 seconds, give up on this
    // replacement. Decrement ballsInPlay since the replacement failed.
    // This prevents the game from hanging if the trough is empty or jammed.
    if (millis() - multiballSaveDispenseMs >= BALL_TROUGH_TO_LIFT_TIMEOUT_MS) {
      multiballSaveWaitingForLift = false;
      if (ballsInPlay > 0) {
        ballsInPlay--;
      }

      snprintf(lcdString, LCD_WIDTH + 1, "MB SAVE TIMEOUT");
      lcdPrintRow(3, lcdString);

      // If more replacements are queued, try the next one anyway
      if (multiballSavesRemaining > 0) {
        multiballSaveDispensePending = true;
      }

      // Check if multiball should end due to the failed replacement
      if (ballsInPlay <= 1 && inMultiball) {
        inMultiball = false;
        multiballSaveEndMs = 0;
        multiballSavesRemaining = 0;
        multiballSaveDispensePending = false;

        const byte DRAIN_SUPPRESS_TICKS = 200;
        switchDebounceCounter[SWITCH_IDX_DRAIN_LEFT] = DRAIN_SUPPRESS_TICKS;
        switchDebounceCounter[SWITCH_IDX_DRAIN_CENTER] = DRAIN_SUPPRESS_TICKS;
        switchDebounceCounter[SWITCH_IDX_DRAIN_RIGHT] = DRAIN_SUPPRESS_TICKS;
        switchDebounceCounter[SWITCH_IDX_GOBBLE] = DRAIN_SUPPRESS_TICKS;

        audioStopMusic();
        ensureMusicPlaying();
        if (audioCurrentComTrack != 0) {
          audioDuckMusicForVoice(true);
        }
      }
    }
  }
}

// ***** MODE SAVE DISPENSE TICK PROCESSOR *****
// Called every tick from loop(). Dispenses a replacement ball when the ball drains
// during an active mode (set by handleBallDrain when modeState.active is true and
// the mode timer has not expired). The game stays in PHASE_BALL_IN_PLAY so the mode
// timer (paused) and mode state are preserved. Unlike multiball save, only one
// replacement is ever in flight at a time (modes are single-ball).
// State machine:
//   1. modeSaveDispensePending: fire the trough release coil
//   2. modeSaveWaitingForLift: wait for ball-in-lift switch, then set ballsInPlay = 1
// Timer resume happens in Step 4 when the replacement ball hits its first playfield switch.
void processModeSaveDispenseTick() {
  if (!modeSaveDispensePending && !modeSaveWaitingForLift) {
    return;
  }

  // STEP 1: Dispense the replacement ball
  if (modeSaveDispensePending && !modeSaveWaitingForLift) {
    // Only fire the trough release if the lift is empty. If a ball is already
    // at the lift base, skip the dispense -- STEP 2 will detect it immediately.
    if (!switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      activateDevice(DEV_IDX_BALL_TROUGH_RELEASE);
    }
    modeSaveDispenseMs = millis();
    modeSaveDispensePending = false;
    modeSaveWaitingForLift = true;
    return;
  }

  // STEP 2: Wait for the replacement ball to reach the lift
  if (modeSaveWaitingForLift) {
    if (switchClosed(SWITCH_IDX_BALL_IN_LIFT)) {
      // Ball arrived at the lift. The player will launch it by pushing the lift rod.
      // Set ballsInPlay back to 1 so scoring and drain detection work normally.
      // Clear timerPaused so the next drain during the mode gets a replacement too.
      modeSaveWaitingForLift = false;
      ballsInPlay = 1;
      modeState.timerPaused = false;

      snprintf(lcdString, LCD_WIDTH + 1, "MODE BALL READY");
      lcdPrintRow(3, lcdString);
      return;
    }

    // Timeout: if the ball does not reach the lift within 3 seconds, give up.
    // End the mode and process the ball as lost.
    if (millis() - modeSaveDispenseMs >= BALL_TROUGH_TO_LIFT_TIMEOUT_MS) {
      modeSaveWaitingForLift = false;
      modeSaveDispensePending = false;
      modeState.timerPaused = false;
      modeState.pausedTimeRemainingMs = 0;

      snprintf(lcdString, LCD_WIDTH + 1, "MODE SAVE TIMEOUT");
      lcdPrintRow(3, lcdString);

      // End the mode, then process as a normal ball lost
      endMode(false);
      processBallLost(DRAIN_TYPE_NONE);
    }
  }
}


// ***** REPLAY AWARD FUNCTIONS *****

// Queues the specified number of replays. Each replay adds one credit and fires the
// knocker once. When count > 1, the knocker fires are spaced at Score Motor sub-slot
// intervals (147ms each, 5 per quarter-rev with a 147ms rest) to match the original
// EM cadence. The Score Motor SSR also runs for the duration (Original/Impulse only).
// Per Overview Section 13.2.1.
void awardReplays(byte count) {
  if (count == 0) {
    return;
  }

  if (count == 1) {
    // Single replay: fire immediately, no queue needed
    pMessage->sendMAStoSLVCreditInc(1);
    activateDevice(DEV_IDX_KNOCKER);
    return;
  }

  // Multiple replays: add to the queue. If the queue is already running,
  // just increase the count and let the tick processor handle it.
  replayQueueCount += count;

  // If the queue was idle, kick it off now
  if (replayNextKnockMs == 0) {
    // Fire the first knock immediately
    pMessage->sendMAStoSLVCreditInc(1);
    activateDevice(DEV_IDX_KNOCKER);
    replayQueueCount--;
    replayKnocksThisCycle = 1;
    replayNextKnockMs = millis() + SCORE_MOTOR_SUB_SLOT_MS;

    // Original/Impulse: start the Score Motor for the knocker sequence.
    // Calculate how many quarter-revs we need: ceil(totalKnocks / 5).
    // We add 1 quarter-rev safety margin.
    byte totalKnocks = replayQueueCount + 1;  // +1 for the one we just fired
    byte quarterRevs = ((totalKnocks + 4) / 5) + 1;
    unsigned long motorMs = (unsigned long)quarterRevs * SCORE_MOTOR_QUARTER_REV_MS;
    if (useMechanicalScoringDevices()) {
      unsigned long now = millis();
      unsigned long newDeadline = now + motorMs;
      if (scoreMotorGameplayEndMs == 0) {
        analogWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, deviceParm[DEV_IDX_MOTOR_SCORE].powerInitial);
        scoreMotorGameplayEndMs = newDeadline;
      } else if ((long)(newDeadline - scoreMotorGameplayEndMs) > 0) {
        scoreMotorGameplayEndMs = newDeadline;
      }
    }
  }
}

// Called every tick from loop(). Drains the replay knocker queue at Score Motor cadence:
// one knock per 147ms sub-slot, with a 147ms rest after every 5 knocks.
// Also transfers pending replays (from checkReplayScoreThresholds) to the active queue
// once the Score Motor has finished displaying the winning score.
void processReplayKnockerTick() {
  // Transfer pending replays to active queue once Score Motor is idle.
  // This ensures the knocker fires AFTER the display shows the threshold score.
  if (replayPendingCount > 0 && scoreMotorGameplayEndMs == 0) {
    awardReplays(replayPendingCount);
    replayPendingCount = 0;
  }

  if (replayQueueCount == 0 || replayNextKnockMs == 0) {
    return;
  }

  if ((long)(millis() - replayNextKnockMs) < 0) {
    return;  // Not time yet
  }

  // Check if we need a rest slot (after 5 knocks in a cycle)
  if (replayKnocksThisCycle >= 5) {
    // This tick is the rest slot; reset cycle counter for the next quarter-rev
    replayKnocksThisCycle = 0;
    replayNextKnockMs = millis() + SCORE_MOTOR_SUB_SLOT_MS;
    return;
  }

  // Fire the next knock
  pMessage->sendMAStoSLVCreditInc(1);
  activateDevice(DEV_IDX_KNOCKER);
  replayQueueCount--;
  replayKnocksThisCycle++;
  replayNextKnockMs = millis() + SCORE_MOTOR_SUB_SLOT_MS;

  // If queue is now empty, stop
  if (replayQueueCount == 0) {
    replayNextKnockMs = 0;
    replayKnocksThisCycle = 0;
    // Score Motor will turn off on its own via processScoreMotorGameplayTick()
  }
}

// ***** REPLAY SCORE THRESHOLD CHECK *****
// Called from addScore() after the score is updated. Compares the current score
// against the 5 configurable replay thresholds for the active game style.
// Each threshold awards exactly 1 replay, and each can only be awarded once per game.
// Per Overview Section 13.2: score-threshold replays fire inline (immediately) for ALL
// styles. On the original EM game, the 100K switch on the score stepper drive arm
// directly pulsed the replay unit when the score was at a threshold -- no Score Motor
// involvement. We replicate this by firing the knocker the moment Master's in-memory
// score crosses the threshold.
void checkReplayScoreThresholds() {
  if (gameState.currentPlayer == 0) {
    return;
  }

  bool enhanced = (gameStyle == STYLE_ENHANCED);
  unsigned int currentScore = gameState.score[gameState.currentPlayer - 1];

  bool anyThresholdCrossed = false;

  for (byte i = 0; i < NUM_REPLAY_THRESHOLDS; i++) {
    // Skip if already awarded
    if (replayScoreAwarded & (1 << i)) {
      continue;
    }

    unsigned int threshold = diagReadReplayScoreFromEEPROM(enhanced, i + 1);
    if (currentScore >= threshold) {
      replayScoreAwarded |= (1 << i);
      // All styles: fire immediately (inline). On the original EM game, score-threshold
      // replays pulsed the replay unit directly from the score stepper, not via the
      // Score Motor. The knocker fires at the moment the threshold is crossed.
      awardReplays(1);
      anyThresholdCrossed = true;
    }
  }

  // Enhanced: announce "Replay!" for score-threshold replays, but only if no
  // higher-priority award announcement is already pending. Pattern completions
  // (3-in-a-row, four-corners, all 9, gobble awards) schedule their own more
  // descriptive announcements and take priority over this generic "Replay!" call.
  // checkReplayScoreThresholds() is called from addScore(), which runs BEFORE
  // pattern checks in the scoring handler, so pendingComTrack == 0 here means
  // nothing higher-priority has been scheduled yet by the current scoring event.
  // If the caller later schedules a higher-priority track (e.g. "Three in a row!
  // Replay!"), scheduleComTrack() will overwrite this one -- which is the desired
  // behavior since the descriptive announcement already includes "Replay!".
  if (anyThresholdCrossed && enhanced && pendingComTrack == 0) {
    byte trackLenTenths = pgmReadComLengthTenths(&comTracksAward[1]);  // Index 1 = track 812
    scheduleComTrack(TRACK_REPLAY, trackLenTenths, 1200, true);
  }
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

// ********************************************************************************************************************************
// ******************************************* PERSISTENT EM STATE (EEPROM) ********************************************************
// ********************************************************************************************************************************

// The original EM game retained Selection Unit position, WHITE inserts (relay latches),
// and gobble count across games and power cycles because the physical hardware simply
// stayed where it was. We emulate this by persisting these values to EEPROM.
// Bumper lit mask is NOT persisted -- the Relay Reset Bank relights all bumpers at game start.
// Score is persisted separately via saveLastScore().

// Saves the persistent EM emulation state to EEPROM.
// Called at game over and periodically during gameplay (same cadence as score save).
// Only writes bytes that have actually changed to reduce EEPROM wear.
void saveEmState() {
  byte pos = selectionUnitPosition;
  if (EEPROM.read(EEPROM_ADDR_SELECTION_UNIT_POS) != pos) {
    EEPROM.write(EEPROM_ADDR_SELECTION_UNIT_POS, pos);
  }

  byte whiteLow = whiteLitMask & 0xFF;
  byte whiteHigh = (whiteLitMask >> 8) & 0xFF;
  if (EEPROM.read(EEPROM_ADDR_WHITE_LIT_MASK) != whiteLow) {
    EEPROM.write(EEPROM_ADDR_WHITE_LIT_MASK, whiteLow);
  }
  if (EEPROM.read(EEPROM_ADDR_WHITE_LIT_MASK + 1) != whiteHigh) {
    EEPROM.write(EEPROM_ADDR_WHITE_LIT_MASK + 1, whiteHigh);
  }

  byte gc = gobbleCount;
  if (EEPROM.read(EEPROM_ADDR_GOBBLE_COUNT) != gc) {
    EEPROM.write(EEPROM_ADDR_GOBBLE_COUNT, gc);
  }
}

// Loads the persistent EM emulation state from EEPROM into global variables.
// Called once at power-up in setup(), before the first game starts.
// Values are validated; if out of range (e.g. first boot with uninitialized EEPROM),
// sensible defaults are used.
void loadEmState() {
  byte pos = EEPROM.read(EEPROM_ADDR_SELECTION_UNIT_POS);
  if (pos >= 50) {
    pos = 0;  // Invalid -- default to position 0
  }
  selectionUnitPosition = pos;

  byte whiteLow = EEPROM.read(EEPROM_ADDR_WHITE_LIT_MASK);
  byte whiteHigh = EEPROM.read(EEPROM_ADDR_WHITE_LIT_MASK + 1);
  unsigned int mask = (whiteHigh << 8) | whiteLow;
  // Only bits 0-8 are valid (numbers 1-9)
  mask &= 0x01FF;
  whiteLitMask = mask;

  byte gc = EEPROM.read(EEPROM_ADDR_GOBBLE_COUNT);
  if (gc > 5) {
    gc = 0;  // Invalid -- default to 0
  }
  gobbleCount = gc;
}

// Sets all playfield lamps to reflect the current persisted EM emulation state.
// Called from setup() at power-up and from setAttractLamps() to show the lamp state
// that the original EM game would have displayed between games.
// This lights: RED insert, HAT lamps (if eligible), WHITE inserts, GOBBLE lamps,
// SPECIAL lamp, and "Spots Number When Lit" kickout/drain lamps based on the
// persisted score's 10K digit.
void setEmStateLamps() {
  // Light the RED insert for the current Selection Unit position
  for (byte i = LAMP_IDX_RED_1; i <= LAMP_IDX_RED_9; i++) {
    pShiftRegister->digitalWrite(lampParm[i].pinNum, HIGH);  // All RED off
  }
  byte redNumber = SELECTION_UNIT_RED_INSERT[selectionUnitPosition % 10];
  byte redLampIdx = LAMP_IDX_RED_1 + (redNumber - 1);
  pShiftRegister->digitalWrite(lampParm[redLampIdx].pinNum, LOW);  // Current RED on

  // Light HAT lamps if the Selection Unit is on an eligible step
  byte stepInCycle = selectionUnitPosition % 10;
  byte cycleNum = selectionUnitPosition / 10;  // 0-4
  bool hatsLit = (stepInCycle == 0) && (cycleNum == 0 || cycleNum == 2 || cycleNum == 4);
  byte hatState = hatsLit ? LOW : HIGH;
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_LEFT_TOP].pinNum, hatState);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_LEFT_BOTTOM].pinNum, hatState);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_RIGHT_TOP].pinNum, hatState);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_HAT_RIGHT_BOTTOM].pinNum, hatState);

  // Light WHITE inserts from persisted bitmask
  for (byte i = 0; i < 9; i++) {
    byte lampIdx = LAMP_IDX_WHITE_1 + i;
    if (whiteLitMask & (1 << i)) {
      pShiftRegister->digitalWrite(lampParm[lampIdx].pinNum, LOW);   // Lamp on
    } else {
      pShiftRegister->digitalWrite(lampParm[lampIdx].pinNum, HIGH);  // Lamp off
    }
  }

  // Light GOBBLE lamps (1 through gobbleCount)
  for (byte i = 0; i < 5; i++) {
    byte lampIdx = LAMP_IDX_GOBBLE_1 + i;
    if (i < gobbleCount) {
      pShiftRegister->digitalWrite(lampParm[lampIdx].pinNum, LOW);   // Lamp on
    } else {
      pShiftRegister->digitalWrite(lampParm[lampIdx].pinNum, HIGH);  // Lamp off
    }
  }

  // SPECIAL WHEN LIT: on if 4 or more gobbles
  if (gobbleCount >= 4) {
    pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPECIAL].pinNum, LOW);
  } else {
    pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPECIAL].pinNum, HIGH);
  }

  // "Spots Number When Lit" lamps based on persisted score's 10K digit.
  // In attract mode this uses lastDisplayedScore; during gameplay it uses the live score.
  // At power-up and game-over, the score is in lastDisplayedScore.
  byte tenKDigit = (unsigned int)lastDisplayedScore % 10;
  bool leftKickoutLit = (tenKDigit == 0);
  bool rightKickoutLit = (tenKDigit == 5);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_LEFT].pinNum,
    leftKickoutLit ? LOW : HIGH);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_RIGHT].pinNum,
    leftKickoutLit ? LOW : HIGH);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_KICKOUT_RIGHT].pinNum,
    rightKickoutLit ? LOW : HIGH);
  pShiftRegister->digitalWrite(lampParm[LAMP_IDX_SPOT_NUMBER_LEFT].pinNum,
    rightKickoutLit ? LOW : HIGH);
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

// ***** COMMON GAME TEARDOWN *****
// Stops all active hardware (audio, shakers, flippers, ball tray, score motor)
// and resets to attract mode. Called from processMidGameStart(), processGameTimeout(),
// and processBallLost() (game over). Does NOT handle scoring, save, or style-specific
// audio (game over announcement, snarky drain comments) -- callers handle those before
// calling this function.
// If t_saveScore is true, saves the current player's score to EEPROM for power-up display.
void teardownGame(bool t_saveScore) {
  // ***** SAFETY: Cancel any running lamp effect before resetting lamps *****
  // Without this, the effect engine would overwrite attract-mode lamps on the
  // next tick, then restore the pre-game lamp state when the effect finishes.
  cancelLampEffect(pShiftRegister);

  // Stop all audio
  if (pTsunami != nullptr) {
    pTsunami->stopAllTracks();
  }
  audioCurrentComTrack = 0;
  audioComEndMillis = 0;
  audioCurrentSfxTrack = 0;
  audioCurrentMusicTrack = 0;
  audioMusicEndMillis = 0;
  cancelPendingComTrack();

  // Stop shakers
  stopAllShakers();

  // Release flippers
  releaseAllFlippers();

  // Close ball tray if open
  if (gameState.ballTrayOpen) {
    releaseDevice(DEV_IDX_BALL_TRAY_RELEASE);
    gameState.ballTrayOpen = false;
  }

  // Turn off score motor if running (startup sequence)
  if (scoreMotorStartMs != 0) {
    releaseDevice(DEV_IDX_MOTOR_SCORE);
    scoreMotorStartMs = 0;
  }

  // Turn off score motor if running (gameplay timer)
  if (scoreMotorGameplayEndMs != 0) {
    analogWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, 0);
    scoreMotorGameplayEndMs = 0;
  }

  // Cancel any pending kickout ejects
  kickoutLeftEjectMs = 0;
  kickoutRightEjectMs = 0;

  // Cancel any pending replay knocker queue
  replayQueueCount = 0;
  replayNextKnockMs = 0;
  replayKnocksThisCycle = 0;
  replayPendingCount = 0;

  // Save score if requested
  if (t_saveScore && gameState.numPlayers > 0 && gameState.currentPlayer > 0) {
    saveLastScore(gameState.score[gameState.currentPlayer - 1]);
    saveEmState();
  }

  // Reset to attract mode
  gamePhase = PHASE_ATTRACT;
  gameStyle = STYLE_NONE;
  setAttractLamps();
  markAttractDisplayDirty();
  initGameState();
  lcdClear();
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
  // ***** SAFETY: Shut down all active hardware before halting *****
  // Force every coil, motor, and solenoid off. This covers flippers, ball tray,
  // shaker motor, score motor, kickouts, slingshots, pop bumper -- everything.
  // We zero countdown and queueCount so no queued pulse can restart a coil.
  // Tracking variables (leftFlipperHeld, gameState.ballTrayOpen, etc.) are NOT
  // updated because criticalError() never returns -- they will never be read.
  for (byte i = 0; i < NUM_DEVS_MASTER; i++) {
    analogWrite(deviceParm[i].pinNum, 0);
    deviceParm[i].countdown = 0;
    deviceParm[i].queueCount = 0;
  }

  // Stop all audio tracks, then play error alert
  if (pTsunami != nullptr) {
    pTsunami->stopAllTracks();
  }

  // ***** Display error and play alert *****
  lcdClear();
  lcdPrintRow(1, line1);
  lcdPrintRow(2, line2);
  lcdPrintRow(3, line3);
  lcdPrintRow(4, "HOLD KNOCKOFF=RST");

  if (pTsunami != nullptr) {
    pTsunami->trackPlayPoly(TRACK_TILT_BUZZER, 0, false);
    audioApplyTrackGain(TRACK_TILT_BUZZER, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
    delay(1500);
    pTsunami->trackPlayPoly(TRACK_DIAG_CRITICAL_ERROR, 0, false);
    audioApplyTrackGain(TRACK_DIAG_CRITICAL_ERROR, tsunamiSfxGainDb, tsunamiGainDb, pTsunami);
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