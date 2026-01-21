// Screamo_Slave.INO Rev: 01/19/26
// 12/28/25: Updated SCORE_INC_10K and SCORE_RESET to remain silent (no bells or 10K unit) in Enhanced style.
// 01/19/26: Updated SCORE_INC_10K message to include silent parm

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
#include <avr/wdt.h>

#include <EEPROM.h>               // For saving and recalling score, to persist between power cycles.
const int EEPROM_ADDR_SCORE = 0;  // Address to store 16-bit score (uses addr 0 and 1)

const byte THIS_MODULE = ARDUINO_SLV;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SLAVE 01/19/26";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which effectively makes it a global.
// No need to pass lcdString[] to any functions that use it!

// Score Motor in cabinet runs at 17 RPM, and measures steps in 1/4 revolutions (i.e. 4 steps per revolution).
// Each 1/4 revolution takes 882ms (0.882 seconds).
// Each 1/4 revolution is divided into 6 timed slots: up to 5 score advances, followed by 1 rest slot.
// Thus every 147ms (882ms / 6 slots) the score motor timing window allows either an advance or a rest.
// Single-score advances:
//   - A single 10K unit (exactly one 10K increment) is handled as a non-motor pulse: fire 10K unit coil + 10K bell once.
//   - A single 100K unit (exactly one 100K increment) is handled similarly: 10K unit coil + 10K bell + 100K bell once.
// Motor-paced cycles:
//   - Any batch requiring more than one 10K (2..5 tens) runs one full motor cycle: up to 5 advances spaced 147ms apart, then one 147ms rest.
//   - Any batch requiring more than one 100K (2..5 hundreds) likewise runs one full motor cycle with hundred advances (each advance = +100K) then the rest.
//   - Larger batches (>5 tens or >5 hundreds) repeat full cycles until remaining units <= 1.
//   - A remaining single 10K or single 100K after all full cycles is finished as a non-motor single pulse.
// Summary:
//   - Motor runs for any multi-unit batch (not only exactly 50K or 500K); thresholds are “>1 and up to 5” per cycle.
//   - Each cycle = 5 advance slots max + 1 rest slot = 882ms total.
//   - Sounds (bells, unit coil) are fired on each advance; rest slot is silent.
// Relay Reset and game start:
//   - Firing the relay reset bank (under p/f; controlled by Master) consumes its own full 1/4 revolution (6 slots, 882ms).
//   - Starting a new game requires 1/4 revolution to fire the Relay Reset bank, followed by up to 9 10K advances, thus up to two more 1/4 revolutions.
// These devices are all only used for sound effects; the actual score changes are handled in code and via RS-485 messages from Master to Slave.
// But Slave is responsible for timing the score changes to match realistic score motor activation.
// Thus, when the score is advanced by five 10K or 100K units, Slave must do so with five advances of 1/6 of 1/4 rev,
// spaced 147ms apart (882ms / 6 steps = 147ms per step) plus a "rest" period of 147ms.  It can then repeat this for an
// additional five advances (plus one rest) if needed.

// Need code to save and recall previous score when machine was turned off or last game ended, so we can display it on power-up.
//   Maybe automatically save current score every x times through the 10ms loop; i.e. every 15 or 30 seconds.  Or just at game-over.

// Need code to realistically reset score from previous score back to zero: advancing the 10K unit and "resetting" the 100K unit.
//   startNewGame() should do this automatically.

// Need to update 10K and 100K scoring for realistic timing and to match score motor.
//   SCORE_BATCH_GAP_MS not being used.
//   Write Master code to activate score motor when score update commands are sent from Master.
//   May need function(s) to estimate time needed to update score, so Master can time score motor activation appropriately.

// *** SERIAL LCD DISPLAY CLASS ***
Pinball_LCD* pLCD2004 = nullptr;  // pLCD2004 is #included in Pinball_Functions.h, so it's effectively a global variable;

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Pinball_Message.h>
Pinball_Message* pMessage = nullptr;

// *** PINBALL_CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Pinball_Centipede) is already in <Pinball_Centipede.h> so not needed here.
// #include <Pinball_Centipede.h> is already in <Pinball_Functions.h> so not needed here.
Pinball_Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (SNS only has ONE Centipede.)

// Define struct to store coil/motor power and time parameters.  Coils that may be held on include a hold strength parameter.
struct DeviceParmStruct {
  byte   pinNum;        // Arduino pin number for this coil/motor
  byte   powerInitial;  // 0..255 PWM power level when first energized
  byte   timeOn;        // Number of 10ms loop ticks (NOT raw ms). 4 => ~40ms.
  byte   powerHold;     // 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
  int8_t countdown;     // Current countdown in 10ms ticks; -1..-8 = rest period, 0 = idle, >0 = active
  byte   queueCount;    // Number of pending activation requests while coil busy/resting
};

// *** CREATE AN ARRAY OF DeviceParm STRUCT FOR ALL COILS AND MOTORS ***
const byte DEV_IDX_CREDIT_UP         =  0;
const byte DEV_IDX_CREDIT_DOWN       =  1;
const byte DEV_IDX_10K_UP            =  2;
const byte DEV_IDX_10K_BELL          =  3;
const byte DEV_IDX_100K_BELL         =  4;
const byte DEV_IDX_SELECT_BELL       =  5;
const byte DEV_IDX_LAMP_SCORE        =  6;
const byte DEV_IDX_LAMP_HEAD_GI_TILT =  7;

// DeviceParm table: { pinNum, powerInitial, timeOn, powerHold, countdown, queueCount }
//   pinNum: Arduino pin number for this coil
//   powerInitial: 0..255 for MOSFET coils and lamps
//   timeOn: Number of 10ms loops to hold initial power level before changing to hold level or turning off
//   powerHold: 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
//   countdown: current countdown in 10ms ticks; -1..-8 = rest period, 0 = idle, >0 = active
//   queueCount: number of pending activation requests while coil busy/resting
DeviceParmStruct deviceParm[NUM_DEVS_SLAVE] = {
  {  5, 100, 10, 0, 0, 0 },  // CREDIT UP.   timeOn 10 (=100ms); not score-motor paced.
  {  6, 140, 10, 0, 0, 0 },  // CREDIT DOWN. timeOn 10 (=100ms); not score-motor paced.
  {  7, 150,  5, 0, 0, 0 },  // 10K UP. 5 ticks ON (50ms) + 8 ticks rest (80ms) = 130ms cycle (<147ms).
  {  8, 130,  5, 0, 0, 0 },  // 10K BELL. 5 ticks ON (50ms) + 8 ticks rest (80ms) = 130ms cycle (<147ms).
  {  9, 130,  5, 0, 0, 0 },  // 100K BELL. 5 ticks ON (50ms) + 8 ticks rest (80ms) = 130ms cycle (<147ms).
  { 10, 130,  5, 0, 0, 0 },  // SELECT BELL. Not required to meet 147ms; uses default rest.
  { 11,  40,  0, 0, 0, 0 },  // LAMP SCORE (timeOn=0 -> no pulse timing).
  { 12,  40,  0, 0, 0, 0 }   // LAMP HEAD G.I./TILT (timeOn=0).
};

// SLAVE ARDUINO HEAD SHIFT REGISTER OUTPUT PIN NUMBERS (Lamps):
//   Note Master Centipede output pins are connected to RELAYS that switch 6.3vac to the actual lamps.
//   However, we just need to know which Centipede pin controls which lamp, regardless of which relay it uses.
//   Centipede shift register pin-to-lamp mapping (not the relay numbers, but the lamps that they illuminate):
//   Pin  0 =  20K   | Pin  8 = 600K  | Pin 16 = 900K  | Pin 24 = unused
//   Pin  1 =  40K   | Pin  9 = 400K  | Pin 17 = 2M    | Pin 25 = G.I.  
//   Pin  2 =  60K   | Pin 10 = 200K  | Pin 18 = 4M    | Pin 26 = 9M    
//   Pin  3 =  80K   | Pin 11 =  90K  | Pin 19 = 6M    | Pin 27 = 7M    
//   Pin  4 = 100K   | Pin 12 =  70K  | Pin 20 = 8M    | Pin 28 = 5M    
//   Pin  5 = 300K   | Pin 13 =  50K  | Pin 21 = TILT  | Pin 29 = 3M    
//   Pin  6 = 500K   | Pin 14 =  30K  | Pin 22 = unused| Pin 30 = 1M    
//   Pin  7 = 700K   | Pin 15 =  10K  | Pin 23 = unused| Pin 31 = 800K  
const byte PIN_OUT_SR_LAMP_10K[9]  = { 15,  0, 14,  1, 13,  2, 12,  3, 11 };  // 10K, 20K, ..., 90K
const byte PIN_OUT_SR_LAMP_100K[9] = { 4, 10,  5,  9,  6,  8,  7, 31, 16 };  // 100K, 200K, ..., 900K
const byte PIN_OUT_SR_LAMP_1M[9]   = { 30, 17, 29, 18, 28, 19, 27, 20, 26 };  // 1M, 2M, ..., 9M
const byte PIN_OUT_SR_LAMP_HEAD_GI = 25;
const byte PIN_OUT_SR_LAMP_TILT    = 21;

// *** MISC CONSTANTS AND GLOBALS ***
byte modeCurrent                 = MODE_UNDEFINED;
byte msgType                     = RS485_TYPE_NO_MESSAGE;  // Current RS485 message type being processed.

const byte SCREAMO_SLAVE_TICK_MS = 10;  // One scheduler tick == 10ms

int currentScore                 =  0;    // Current game score (0..999).
int targetScore                  =  0;    // Desired final score (0..999) updated by incoming RS485 score deltas.
bool scoreCommandSilent = false;  // True when current score command should suppress bells/10K unit

unsigned long lastLoopTime       =  0;    // Used to track 10ms loop timing.

// Rapid-fire devices (10K Unit, 10K Bell, 100K Bell) may have timeOn up to n 10ms ticks (i.e. 50ms).
// They must be able to re-fire every 147ms score motor sub-step.
// Using rest ricks = -8 (80ms) means i.e. 50ms ON + 80ms REST = 130ms < 147ms.
const int8_t COIL_REST_TICKS   = -8;

// Command queue (FIFO) for batching score deltas:
const byte SCORE_CMD_QUEUE_SIZE = 30;  // up to 30 pending commands
int  scoreCmdQueue[SCORE_CMD_QUEUE_SIZE];
byte scoreCmdHead = 0;         // dequeue index
byte scoreCmdTail = 0;         // enqueue index
// scoreCmdActiveDirection represents the sign of the currently executing score adjustment batch; the direction to move
//   currentScore toward targetScore during the active batch.
// 0 = no active command; +1 = active incrementing command; -1 = active decrementing command.
// +1 incrementing = add 10K or 100K units per step
// -1 decrementing = subtract 10K or 100K units per step
// Set to zero when work is completed and state is cleared; idle.
int  scoreCmdActiveDirection = 0;

// Score motor timing:
const unsigned SCORE_MOTOR_CYCLE_MS = 882;                 // one 1/4 revolution of score motor (ms)
const unsigned SCORE_MOTOR_STEPS    = 6;                   // sub-steps per 1/4 rev (up to 5 advances + 1 rest)
const unsigned SCORE_MOTOR_STEP_MS  = SCORE_MOTOR_CYCLE_MS / SCORE_MOTOR_STEPS; // spacing per sub-step (ms)

// Score saving:
const unsigned long EEPROM_SAVE_INTERVAL_MS = 60000;  // Save score every 60 seconds
unsigned long lastEEPROMSaveMs = 0;
int lastSavedScore = -1;  // Track last saved value to avoid redundant writes

// Score reset phases used for Reset State Machine:
// The score reset sequence runs through several phases to simulate mechanical score motor behavior.
// SCORE_RESET_PHASE_REV_HIGH:
//   Phase 0. Rapid visual reversal of the high-order digits (millions and hundredK combined as a 0..99 composite).
//   Runs as fast-decrement steps spaced by RESET_HIGH_STEP_MS (not motor-slot paced) until high units reach 0.
//   After completion, a full motor cycle (SCORE_MOTOR_CYCLE_MS) is waited to simulate 1/4 rev before tens work begins.
// SCORE_RESET_PHASE_10K_CYCLE1:
//   Phase 1. First motor-paced tensK advancement cycle (up to 5 increments) using score motor slot pacing (147ms per slot).
//   Only tensK digit changes; hundredK/millions do NOT roll over visually here (suppressed). Rollover at tensK=9->0 triggers 100K bell only.
// SCORE_RESET_PHASE_10K_CYCLE2:
//   Phase 2. Optional second tensK cycle if more than 5 tensK steps were required (i.e. 6..10 total). Same pacing rules as cycle 1.
// SCORE_RESET_PHASE_DONE:
//   Final phase. Reset sequence finished; score persisted to EEPROM and resetActive cleared.
const byte SCORE_RESET_PHASE_REV_HIGH   = 0;
const byte SCORE_RESET_PHASE_10K_CYCLE1 = 1;
const byte SCORE_RESET_PHASE_10K_CYCLE2 = 2;
const byte SCORE_RESET_PHASE_DONE       = 3;

// State used when executing batched (multi-step) 10K adjustments
bool scoreAdjustActive          = false;  // gates processScoreAdjust(); true while a batch is executing
bool scoreAdjustMotorMode       = false;  // true when current 10K command uses motor pacing
bool scoreCycleHundred          = false;  // true if current motor cycle is hundredK-type
byte scoreAdjustCycleIndex      = 0;      // 0..(SCORE_MOTOR_STEPS-1) index into sub-step
byte cycleActionLimit           = 0;      // caps advances per cycle (1-5) for pacing; planned advances in this cycle (1..5)
unsigned long scoreAdjustLastMs = 0;      // last timestamp (ms) a slot or single pulse was attempted
unsigned long motorCycleStartMs = 0;      // Timestamp when current motor cycle began; enforce post-cycle rest for non-motor singles and detect completion timing
bool lastCommandWasMotor        = false;  // True if current command uses motor cycle timing; used to choose required rest window size in scoreAdjustFinishIfDone().
bool motorPersistent            = false;  // True if command started as multi-unit (>1) and must remain motor-paced even for final single.
                                          //   Preserves motor pacing when a multi-unit batch shrinks to single; influences mode switch decisions.
bool  resetActive               = false;  // Flag gating processScoreReset() and preempting score adjust.
byte  resetPhase                = SCORE_RESET_PHASE_DONE;  // Core state variable for reset phases
byte  resetCycleIndex           = 0;      // 0..(SCORE_MOTOR_STEPS-1) within current motor cycle; tracks sub-step within tensK motor cycles.
unsigned long resetLastMs       = 0;      // pacing timestamp; Timing gate for 147ms slots during reset tens cycles.
int   reset10KStepsRemaining    = 0;      // total 10K increments still to perform (6..10 or 1..5);
                                          //    Remaining tensK advances needed in reset; drives phase transitions.

// High reversal pacing; when resetting score, millions and 100K are rapidly reversed from current value down to zero.
int  resetHighCurrent           = 0;      // current high (millions*10 + hundredK) value during reversal (0..99)
int  resetHighUnitsRemaining    = 0;      // number of 100K units (millions*10 + hundredK)
unsigned long resetHighLastMs   = 0;      // timestamp of last high decrement
unsigned resetHighIntervalMs    = 0;      // ms between high decrements during reversal
unsigned long resetHighCycleStartMs = 0;
int  resetTensKStart            = 0;      // tensK digit to hold constant during high reversal

const unsigned RESET_HIGH_STEP_MS = 8;  // ms between rapid high (100K) reversal decrements during score reset phase

// ---------------- Unified batch decomposition state ----------------
int  batchHundredsRemaining = 0;   // number of 100K steps left in current batch (each == 10 units)
int  batchTensRemaining     = 0;   // number of 10K steps left in current batch

// ---- New globals for flashing state ----
bool flashActive = false;
int flashScoreValue = 0;            // 0..999 value being flashed
bool flashShowOn = false;           // current visible state (true => show flashScoreValue; false => show 0)
unsigned long flashLastToggleMs = 0;
const unsigned FLASH_RATE_MS = 250; // 250ms half-period => 2 Hz blink
int lastDisplayedScore = -1;        // replaced displayScore's static cache (global so we can reset it)

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  initScreamoSlaveArduinoPins();                     // Arduino direct I/O pins only.

  setAllDevicesOff();  // Set initial state for all MOSFET PWM and Centipede outputs

  // *** INITIALIZE PINBALL_CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Pinball_Centipede class hangs the system if hardware is not connected.
  // Initialize I2C and Centipede as early as possible to minimize relay-on (turns on all lamps) at power-up
  Wire.begin();                            // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Pinball_Centipede;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                 // Set all registers to default.

  pShiftRegister->initScreamoSlaveCentipedePins();

  setPWMFrequencies(); // Set non-default PWM frequencies so coils don't buzz.

  clearAllPWM();  // Clear any PWM outputs that may have been set during setPWMFrequencies()

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 LCD2004 instantiated via pLCD2004->begin.
  // Serial2 RS485   instantiated via pMessage->begin.

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Pinball_Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // Turn on GI lamps so player thinks they're getting instant startup (NOTE: Can't do this until after pShiftRegister is initialized)
  setScoreLampBrightness(deviceParm[DEV_IDX_LAMP_SCORE].powerInitial);     // Ready to turn on as soon as relay contacts close.
  setGILamp(true);
  setGITiltLampBrightness(deviceParm[DEV_IDX_LAMP_HEAD_GI_TILT].powerInitial);  // Ready to turn on as soon as relay contacts close.
  setTiltLamp(false);

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // Insert a delay() in order to give the Digole 2004 LCD time to power up and be ready to receive commands (esp. the 115K speed command).
  delay(750);  // 500ms was occasionally not enough.
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.

  // Display previously saved score from EEPROM
  currentScore = recallScoreFromEEPROM();
  displayScore(currentScore);
  // targetScore is the intended final score after all queued adjustments are applied.
  // It accumulates incoming deltas immediately and is always clamped to 0..999.
  // currentScore moves toward targetScore over time via processScoreAdjust().
  targetScore  = currentScore;  // initialize target
  sprintf(lcdString, "EEPROM score %d", currentScore); pLCD2004->println(lcdString);

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {
  // Enforce 10ms cooperative scheduler tick.
  // - Keeps timing deterministic for device timers and score motor slots.
  // - Early-return style keeps loop short and predictable; avoid adding blocking work here.
  unsigned long now = millis();
  if (now - lastLoopTime < SCREAMO_SLAVE_TICK_MS) return;
  lastLoopTime = now;

  // Check for RS485 communication errors
  byte incomingMsg = pMessage->available();
  if (incomingMsg >= RS485_ERROR_BEGIN) {  // Error code range
    handleError(incomingMsg);
    return;  // Stop processing
  }

  // Drain RS-485 messages. If flashing, any incoming message should stop flashing FIRST.
  if (incomingMsg != RS485_TYPE_NO_MESSAGE) {
    // If we're flashing and message is NOT another flash-start, stop flashing and prepare display state.
    if (flashActive && incomingMsg != RS485_TYPE_MAS_TO_SLV_SCORE_FLASH) {
      // Per requirement: if incoming is NOT score-related, set score to zero before processing.
      if (incomingMsg != RS485_TYPE_MAS_TO_SLV_SCORE_ABS &&
        incomingMsg != RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K &&
        incomingMsg != RS485_TYPE_MAS_TO_SLV_SCORE_RESET &&
        incomingMsg != RS485_TYPE_MAS_TO_SLV_SCORE_QUERY) {
        currentScore = 0;
        targetScore = 0;
        scoreAdjustActive = false;
      }
      stopFlashScore();
    }
    processMessage(incomingMsg);
  }

  // Advance per-device timers first so device state (countdown/rest) reflects this tick
  // before any state machines attempt to activate coils/lamps again.
  updateDeviceTimers();

  // Run score state machines (these call displayScore internally). displayScore will no-op while flashActive==true.
  // 1) processScoreAdjust() moves currentScore toward targetScore (may activate coils/bells).
  // 2) processScoreReset() runs a reset sequence that preempts/blocks adjustments when active.
  // Order matters: updateDeviceTimers() happens before these so an activation decision sees
  // up-to-date device cooldown/rest state.
  processScoreAdjust();
  processScoreReset();

  // Periodic EEPROM save (i.e. every 60 seconds) (only if score has changed.)
  // Since we don't *really* care if the score displayed at power-up is the absolute latest score.
  // We could just as easily have the score come up at some always-the-same value; just so it's not zero.
  // The original game would persist score-at-power-off until the next power-on, so we're just simulating this.
  now = millis();
  if ((now - lastEEPROMSaveMs) >= EEPROM_SAVE_INTERVAL_MS) {
    if (currentScore != lastSavedScore) {
      saveScoreToEEPROM(currentScore);
      lastSavedScore = currentScore;
    }
    lastEEPROMSaveMs = now;
  }

  // If flash active, toggle lamps at the requested rate (non-blocking).
  if (flashActive) {
    unsigned long nowMs = millis();
    if ((nowMs - flashLastToggleMs) >= FLASH_RATE_MS) {
      flashLastToggleMs = nowMs;
      flashShowOn = !flashShowOn;
      if (flashShowOn) {
        setScoreLampsDirect(flashScoreValue);
      } else {
        setScoreLampsDirect(0);
      }
    }
  }
}  // End of loop()

// *****************************************************************************************
// ***************************  M E S S A G E   H A N D L I N G  ***************************
// *****************************************************************************************

void processMessage(byte t_msgType) {
  // Dispatches a single RS-485 message already detected by available().
  // Messages without parameters act directly.
  // Messages with parameters must call corresponding get*() before using payload.
  // Many score-related messages modify score state machines; score reset preempts adjust.
  switch (t_msgType) {
  case RS485_TYPE_MAS_TO_SLV_MODE: {
    // Mode change triggers new game start (resets score and lamp state).
    byte newMode = 0;
    pMessage->getMAStoSLVMode(&newMode);
    sprintf(lcdString, "Mode change %u", newMode); pLCD2004->println(lcdString);
    startNewGame(newMode);
  } break;

  case RS485_TYPE_MAS_TO_SLV_COMMAND_RESET: {
    // Software reset
    softwareReset();
  } break;

  case RS485_TYPE_MAS_TO_SLV_CREDIT_STATUS: {
    // Report current credit presence (TRUE if any remain).
    sprintf(lcdString, "Credits %s", hasCredits() ? "avail" : "zero"); pLCD2004->println(lcdString);
    pMessage->sendSLVtoMASCreditStatus(hasCredits());
  } break;

  case RS485_TYPE_MAS_TO_SLV_CREDIT_FULL_QUERY: {
    // Master is asking if the credit unit full switch is closed
    bool creditsFull = (digitalRead(PIN_IN_SWITCH_CREDIT_FULL) == LOW);  // Pin pulled LOW when full
    sprintf(lcdString, "Credit full %s", creditsFull ? "YES" : "NO "); pLCD2004->println(lcdString);
    pMessage->sendSLVtoMASCreditFullStatus(creditsFull);
    break;
  }

  case RS485_TYPE_MAS_TO_SLV_CREDIT_INC: {
    // Queue credit additions; coil driver enforces safe timing and switch limits.
    byte numCredits = 0;
    pMessage->getMAStoSLVCreditInc(&numCredits);
    sprintf(lcdString, "Credit inc %u", numCredits); pLCD2004->println(lcdString);
    for (unsigned i = 0; i < numCredits; ++i) {
      activateDevice(DEV_IDX_CREDIT_UP);
    }
  } break;

  case RS485_TYPE_MAS_TO_SLV_CREDIT_DEC: {
    // Attempt credit decrement only if credits remain.
    pLCD2004->println("Credit dec.");
    if (hasCredits()) {
      activateDevice(DEV_IDX_CREDIT_DOWN);
    }
  } break;

  case RS485_TYPE_MAS_TO_SLV_SCORE_RESET: {
    // Initiates a multi-phase visual score reset sequence if not already active.
    // Clears pending score adjust queue before beginning.
    pLCD2004->println("Score Reset");
    if (!resetActive) {
      scoreCmdHead = scoreCmdTail = 0;
      scoreAdjustActive = false;

      int startScore = currentScore;
      int tensK      = startScore % 10;
      int hundredK   = (startScore / 10) % 10;
      int millions   = (startScore / 100) % 10;

      resetHighCurrent        = millions * 10 + hundredK;
      resetHighUnitsRemaining = millions * 10 + hundredK;
      resetTensKStart         = tensK;

      resetHighIntervalMs     = (resetHighUnitsRemaining > 0) ? RESET_HIGH_STEP_MS : 0;
      resetHighLastMs         = millis();
      resetHighCycleStartMs   = resetHighLastMs;

      int rawSteps = (10 - tensK) % 10;
      if (tensK == 0) {
        rawSteps = 10;
      }
      reset10KStepsRemaining = rawSteps;

      resetPhase      = SCORE_RESET_PHASE_REV_HIGH;
      resetCycleIndex = 0;
      resetLastMs     = millis() - SCORE_MOTOR_STEP_MS;
      resetActive     = true;
    }
  } break;

  case RS485_TYPE_MAS_TO_SLV_SCORE_ABS: {
    // Sets score immediately to absolute value; clears pending adjustments.
    // Sets both currentScore and targetScore immediately to the absolute value (0..999).
    // This clears any queued adjustments; display and persistence are updated at once.
    int absScore = 0;
    pMessage->getMAStoSLVScoreAbs(&absScore);
    int newScore = constrain(absScore, 0, 999);
    currentScore = newScore;
    targetScore  = newScore;  // target = current because we jump directly to absolute score
    scoreCmdHead = scoreCmdTail = 0;
    scoreAdjustActive = false;
    scoreCmdActiveDirection = 0;
    sprintf(lcdString, "Score set %u", (unsigned)currentScore); pLCD2004->println(lcdString);
    displayScore(currentScore);
  } break;

  case RS485_TYPE_MAS_TO_SLV_SCORE_FLASH: {
    int flashScore = 0;
    pMessage->getMAStoSLVScoreFlash(&flashScore);
    if (flashScore == 0) {
      // Shortcut: behave like a no-op score change; stop flash and resume previous display.
      stopFlashScore();
      // No score change; displayScore() is already invoked in stopFlashScore().
    } else {
      startFlashScore(flashScore);
    }
  } break;

  case RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K: {
    // Forward signed delta (in 10K units) to the centralized enqueue/validation logic.
    int incAmount = 0;
    bool silent = false;
    pMessage->getMAStoSLVScoreInc10K(&incAmount, &silent);  // CHANGED: now gets silent flag
    scoreCommandSilent = silent;  // NEW: store for use by processScoreAdjust()
    sprintf(lcdString, "Scr adj %d%s", incAmount, silent ? " SIL" : ""); // FIXED: max 19 chars
    pLCD2004->println(lcdString);

    // requestScoreAdjust() performs clamping, updates targetScore, prunes saturation, and starts processing if idle.
    requestScoreAdjust(incAmount);

    // If we hit a boundary as a result of enqueue, remove same-direction queued work.
    if (targetScore == 999) {
      pruneQueuedAtBoundary(1);
    } else if (targetScore == 0) {
      pruneQueuedAtBoundary(-1);
    }
  } break;

  case RS485_TYPE_MAS_TO_SLV_BELL_10K: {
    // Rings 10K bell only (manual sound trigger).
    pLCD2004->println("10K Bell");
    activateDevice(DEV_IDX_10K_BELL);
  } break;

  case RS485_TYPE_MAS_TO_SLV_BELL_100K: {
    // Rings 100K bell only.
    pLCD2004->println("100K Bell");
    activateDevice(DEV_IDX_100K_BELL);
  } break;

  case RS485_TYPE_MAS_TO_SLV_BELL_SELECT: {
    // Rings select bell only.
    pLCD2004->println("Select Bell");
    activateDevice(DEV_IDX_SELECT_BELL);
  } break;

  case RS485_TYPE_MAS_TO_SLV_10K_UNIT: {
    // Direct pulse of 10K unit (diagnostic).
    activateDevice(DEV_IDX_10K_UP);
  } break;

  case RS485_TYPE_MAS_TO_SLV_SCORE_QUERY: {
    // Reports current score (0..999) back to Master.
    sprintf(lcdString, "Score rpt %u", currentScore); pLCD2004->println(lcdString);
    pMessage->sendSLVtoMASScoreReport(currentScore);
  } break;

  case RS485_TYPE_MAS_TO_SLV_GI_LAMP: {
    // Sets GI relay lamp state (active LOW).
    bool giOn = false;
    pMessage->getMAStoSLVGILamp(&giOn);
    setGILamp(giOn);
  } break;

  case RS485_TYPE_MAS_TO_SLV_TILT_LAMP: {
    // Sets Tilt lamp state (active LOW).
    bool tiltOn = false;
    pMessage->getMAStoSLVTiltLamp(&tiltOn);
    setTiltLamp(tiltOn);
  } break;

  case RS485_ERROR_UNEXPECTED_TYPE: {
    handleError(RS485_ERROR_UNEXPECTED_TYPE);
  } break;

  default: {
    // Unknown message type.
    handleError(RS485_ERROR_UNEXPECTED_TYPE);
  } break;
  }  // End of switch
}

// ************************************************************************************
// ********************** H A R D W A R E / D E V I C E   C O N T R O L ************************
// ************************************************************************************

void setAllDevicesOff() {
  // Set the output latch low first, then configure as OUTPUT.
  // This avoids a brief HIGH when changing direction.
  // Called at startup; safe to call again if a fatal error requires a known-safe hardware state.
  for (int i = 0; i < NUM_DEVS_SLAVE; i++) {
    deviceParm[i].countdown = 0;     // 0 = idle (not active, not resting)
    deviceParm[i].queueCount = 0;    // Clear any pending activations
    digitalWrite(deviceParm[i].pinNum, LOW);
    pinMode(deviceParm[i].pinNum, OUTPUT);
  }
  // Use shared helper to force Centipede outputs OFF (safe if pShiftRegister == nullptr)
  centipedeForceAllOff();
}

void setPWMFrequencies() {
  // Configure PWM timers / prescalers for higher PWM frequency (~31kHz)
  // Keep this separate from clearAllPWM() which only clears PWM outputs.
  // Prescaler set to 1 => ~31kHz. Only affects Timer1-controlled outputs.
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;
}

void clearAllPWM() {
  // Call this after PWM timers / prescalers have been configured.
  // On PWM-capable pins this clears compare outputs; harmless on non-PWM pins.
  for (int i = 0; i < NUM_DEVS_SLAVE; i++) {
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

bool canActivateDeviceNow(byte t_devIdx) {
  // Returns true if device may be activated immediately (mechanical protection checks).
  // CREDIT UP: inhibit if wheel is FULL (switch HIGH).
  // CREDIT DOWN: inhibit if wheel is EMPTY (no credits to remove).
  if (t_devIdx == DEV_IDX_CREDIT_UP) {
    if (digitalRead(PIN_IN_SWITCH_CREDIT_FULL) == HIGH) {
      return false;
    }
  } else if (t_devIdx == DEV_IDX_CREDIT_DOWN) {
    if (!hasCredits()) {
      return false;
    }
  }
  return true;
}

void activateDevice(byte t_devIdx) {
  // Requests immediate activation of a device:
  // - If active (countdown > 0) or resting (countdown < 0) the request is queued.
  // - If idle (countdown == 0) and allowed, starts initial power phase.
  // Invalid index: halts system after forcing all outputs off (fatal configuration error).
  if (t_devIdx >= NUM_DEVS_SLAVE) {
    setAllDevicesOff();
    sprintf(lcdString, "DEV NUM ERR %u", t_devIdx); pLCD2004->println(lcdString);
    while (true) { }
  }
  if (deviceParm[t_devIdx].countdown != 0) {
    // Busy or resting; accumulate request.
    deviceParm[t_devIdx].queueCount++;
    return;
  }
  if (canActivateDeviceNow(t_devIdx)) {
    analogWrite(deviceParm[t_devIdx].pinNum, deviceParm[t_devIdx].powerInitial);
    deviceParm[t_devIdx].countdown = deviceParm[t_devIdx].timeOn; // Begin active phase.
  }
}

void updateDeviceTimers() {
  // Advances per-device timing:
  // Active phase: countdown > 0; decrements toward 0. On reaching 0:
  //   - If powerHold > 0: transition to hold (steady output, no rest).
  //   - Else: turn OFF and enter rest if timeOn > 0.
  // Rest phase: countdown < 0; increments toward 0; when reaches 0:
  //   - If queued requests exist and guard conditions pass, starts next activation.
  // Idle phase: countdown == 0; may start queued activation immediately.
  for (byte i = 0; i < NUM_DEVS_SLAVE; ++i) {
    int8_t& ct = deviceParm[i].countdown;

    if (ct > 0) { // Active
      ct--;
      if (ct == 0) {
        if (deviceParm[i].powerHold > 0) {
          analogWrite(deviceParm[i].pinNum, deviceParm[i].powerHold); // Hold steady
        } else {
          analogWrite(deviceParm[i].pinNum, LOW);
          if (deviceParm[i].timeOn > 0) {
            ct = COIL_REST_TICKS; // Enter rest
          } else {
            ct = 0;
          }
        }
      }
      continue;
    }

    if (ct < 0) { // Rest
      ct++;
      if (ct == 0 && deviceParm[i].queueCount > 0) {
        if (canActivateDeviceNow(i)) {
          deviceParm[i].queueCount--;
          analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
          deviceParm[i].countdown = deviceParm[i].timeOn;
        } else if (i == DEV_IDX_CREDIT_UP) {
          // Credit add blocked (already full) -> discard queued adds and notify Master.
          sprintf(lcdString, "Credit FULL %u", deviceParm[i].queueCount); pLCD2004->println(lcdString);
          deviceParm[i].queueCount = 0;
          pMessage->sendSLVtoMASCreditStatus(hasCredits());
        }
      }
      continue;
    }

    // Idle
    if (deviceParm[i].queueCount > 0) {
      if (canActivateDeviceNow(i)) {
        deviceParm[i].queueCount--;
        analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
        deviceParm[i].countdown = deviceParm[i].timeOn;
      } else if (i == DEV_IDX_CREDIT_UP) {
        // Discard queued adds if wheel is full while idle.
        sprintf(lcdString, "Credit FULL %u", deviceParm[i].queueCount); pLCD2004->println(lcdString);
        deviceParm[i].queueCount = 0;
        pMessage->sendSLVtoMASCreditStatus(hasCredits());
      }
    }
  }
}

void setScoreLampBrightness(byte t_brightness) {
  // Sets PWM brightness for score lamps (0..255). Does not alter GI state.
  analogWrite(deviceParm[DEV_IDX_LAMP_SCORE].pinNum, t_brightness);
}

void setGITiltLampBrightness(byte t_brightness) {
  // Sets PWM brightness for GI/Tilt lamp group (0..255). GI relay logic separate.
  analogWrite(deviceParm[DEV_IDX_LAMP_HEAD_GI_TILT].pinNum, t_brightness);
}

void setGILamp(bool t_on) {
  // Controls GI relay lamp (active LOW on Centipede pin).
  pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_HEAD_GI, t_on ? LOW : HIGH);
}

void setTiltLamp(bool t_on) {
  // Controls Tilt lamp (active LOW on Centipede pin).
  pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_TILT, t_on ? LOW : HIGH);
}

// ************************************************************************************
// ****************** S C O R E   M A N A G E M E N T   F U N C T I O N S *************
// ************************************************************************************

bool dequeueScoreCmdIfAny(int* t_cmd) {
  // Dequeue one signed delta from scoreCmdQueue if available.
  // Returns true and sets *t_cmd on success; returns false if queue empty.
  if (scoreCmdHead == scoreCmdTail) {
    return false;
  }
  *t_cmd = scoreCmdQueue[scoreCmdHead];
  scoreCmdHead = (byte)((scoreCmdHead + 1) % SCORE_CMD_QUEUE_SIZE);
  return true;
}

void requestScoreAdjust(int t_delta10Ks) {
  // Purpose of targetScore here:
  // - targetScore is updated immediately by the requested delta (clamped to 0..999).
  // - Using targetScore instead of currentScore ensures correct boundary handling
  //   while commands are queued (i.e. targetScore reflects all pending work).
  // - currentScore will move toward targetScore over subsequent slots/cycles.

  // Enqueue a signed score offset in 10K units (t_delta10Ks).
  // Performs range clamping and saturation check before queuing.
  // Starts adjustment immediately if idle.
  if (t_delta10Ks == 0) {
    return;
  }
  if ((t_delta10Ks > 0 && targetScore >= 999) || (t_delta10Ks < 0 && targetScore <= 0)) {
    // Already saturated at boundary considering pending work reflected in targetScore.
    return;
  }

  // Hard clamp requested delta magnitude.
  if (t_delta10Ks > 999) {
    t_delta10Ks = 999;
  }
  if (t_delta10Ks < -999) {
    t_delta10Ks = -999;
  }

  // Clamp delta so that updated targetScore remains within 0..999.
  // Note: targetScore already includes previously queued but not-yet-executed deltas.
  if (t_delta10Ks > 0 && targetScore + t_delta10Ks > 999) {
    t_delta10Ks = 999 - targetScore;
  }
  if (t_delta10Ks < 0 && targetScore + t_delta10Ks < 0) {
    t_delta10Ks = -targetScore;
  }
  if (t_delta10Ks == 0) {
    return;
  }

  byte nextTail = (byte)((scoreCmdTail + 1) % SCORE_CMD_QUEUE_SIZE);
  if (nextTail == scoreCmdHead) {
    // Queue full: drop this new request and halt.
    sprintf(lcdString, "Queue FULL drop %d", t_delta10Ks); pLCD2004->println(lcdString);
    handleError(SLAVE_SCORE_QUEUE_FULL);
    return;
  }
  scoreCmdQueue[scoreCmdTail] = t_delta10Ks;
  scoreCmdTail = nextTail;

  // Update targetScore immediately; always clamp 0..999.
  // This makes targetScore the authoritative projected score used by all saturation logic.
  targetScore += t_delta10Ks;
  if (targetScore < 0) {
    targetScore = 0;
  }
  if (targetScore > 999) {
    targetScore = 999;
  }

  // If already processing a batch, defer start until current completes.
  if (scoreAdjustActive) {
    return;
  }

  // Begin first command immediately: dequeue via helper and hand to centralized starter.
  int cmd = 0;
  if (dequeueScoreCmdIfAny(&cmd)) {
    startBatchFromCmd(cmd);
  }
}

void startBatchFromCmd(int cmd) {
  // Centralized helper that initializes state for executing a queued score adjustment command.
  //
  // Input:
  //   cmd - signed delta expressed in 10K units (e.g. +6 means +60K; -10 means -100K)
  //
  // Responsibilities / side-effects:
  // - Decompose the absolute magnitude into either "hundreds" (100K units) or "tens" (10K units).
  //   Rule: if magnitude % 10 == 0 -> treat as hundreds (magnitude/10), else treat as pure tens.
  // - Set 'scoreCmdActiveDirection' to the sign of the command (+1 or -1).
  // - Initialize batch state variables:
  //     'batchHundredsRemaining' or 'batchTensRemaining'
  //     'scoreAdjustMotorMode' (true for motor-paced multi-unit batches)
  //     'scoreCycleHundred' (true when current cycle advances are 100K steps)
  //     'cycleActionLimit' (planned advances in the first cycle, capped to 5)
  //     'motorPersistent' (true if the command STARTED as multi-unit (>1) and thus must keep motor pacing
  //                       even if it later reduces to a single remainder)
  //     'lastCommandWasMotor' (used by finish logic to select required rest window)
  //     'motorCycleStartMs' (timestamp marking start of motor cycle or single-pulse rest window)
  //     'scoreAdjustActive' and 'scoreAdjustLastMs' (activate state machine and allow immediate first slot)
  //
  // Notes / examples:
  //  - cmd = 6  => magnitude 6 (tens). batchTensRemaining = 6 -> motor mode with cycleActionLimit = 5,
  //                motorPersistent = true. Execution pattern: cycle1: 5 advances + rest, cycle2: 1 advance + 5 rests.
  //  - cmd = 1  => single tens: processed as a non-motor single pulse (lastCommandWasMotor = false).
  //  - cmd = 10 => magnitude 10 divisible by 10 => batchHundredsRemaining = 1 -> single 100K pulse (non-motor).
  //  - This helper does NOT touch the command queue (head/tail) or 'targetScore'.
  //
  // Implementation details:
  // - Use min(remaining, 5) to set cycleActionLimit so each motor cycle has up to 5 advances.
  // - scoreAdjustLastMs is set to (now - SCORE_MOTOR_STEP_MS) so processScoreAdjust() may execute the first
  //   advance slot immediately on the next loop iteration if device timers permit.
  //
  scoreCmdActiveDirection = (cmd > 0) ? 1 : -1;
  int magnitude = abs(cmd);

  if (magnitude % 10 == 0) {
    // Treat as hundreds (100K units)
    batchHundredsRemaining = magnitude / 10;
    batchTensRemaining     = 0;
  } else {
    // Treat as tens (10K units)
    batchHundredsRemaining = 0;
    batchTensRemaining     = magnitude;
  }

  // Clear per-batch transient state
  scoreAdjustMotorMode  = false;
  scoreCycleHundred     = false;
  cycleActionLimit      = 0;
  scoreAdjustCycleIndex = 0;
  motorPersistent       = false;

  // Decide initial pacing and persistence
  if (batchHundredsRemaining > 1) {
    // Multi-hundred batch -> motor-paced hundred cycles
    scoreAdjustMotorMode = true;
    scoreCycleHundred    = true;
    cycleActionLimit     = (byte)min(batchHundredsRemaining, 5);
    motorPersistent      = true;   // preserve motor pacing even if remainder == 1
    lastCommandWasMotor  = true;
    motorCycleStartMs    = millis();
  } else if (batchHundredsRemaining == 1) {
    // Single 100K -> single non-motor pulse
    lastCommandWasMotor  = false;
    motorCycleStartMs    = millis();
  } else if (batchTensRemaining > 1) {
    // Multi-ten batch -> motor-paced tens cycles
    scoreAdjustMotorMode = true;
    scoreCycleHundred    = false;
    cycleActionLimit     = (byte)min(batchTensRemaining, 5);
    motorPersistent      = true;   // preserve motor pacing even if remainder == 1
    lastCommandWasMotor  = true;
    motorCycleStartMs    = millis();
  } else {
    // Single 10K -> single non-motor pulse
    lastCommandWasMotor  = false;
    motorCycleStartMs    = millis();
  }

  // Activate score adjust state machine and allow immediate first slot execution
  scoreAdjustActive = true;
  scoreAdjustLastMs = millis() - SCORE_MOTOR_STEP_MS;
}

void processScoreAdjust() {
  // Non-blocking batch execution of queued score adjustments:
  // - Groups multi-unit (2..5) 10K or 100K steps into a motor-paced cycle (5 advances + 1 rest).
  // - Repeats cycles for large batches.
  // - Single unit (10K or 100K) executes as a stand-alone pulse (non-motor) still followed by a rest window.
  // - Aborts if a reset is active.
  if (!scoreAdjustActive || resetActive) {
    return;
  }

  // Completed batch waiting for rest window: attempt finish.
  if (batchHundredsRemaining == 0 && batchTensRemaining == 0 && !scoreAdjustMotorMode) {
    scoreAdjustFinishIfDone();
    return;
  }

  unsigned long now = millis();
  // Timing gate per motor sub-step (147ms).
  if (now - scoreAdjustLastMs < SCORE_MOTOR_STEP_MS) {
    return;
  }
  // 10K unit coil must be idle before next advance slot.
  if (deviceParm[DEV_IDX_10K_UP].countdown != 0) {
    return;
  }

  scoreAdjustLastMs = now;

  // Singular (non-motor) execution path.
  if (!scoreAdjustMotorMode) {
    if (batchHundredsRemaining == 1) {
      int nextScore = constrain(currentScore + scoreCmdActiveDirection * 10, 0, 999);
      if (nextScore != currentScore) {
        if (noiseMakersEnabled()) {
          activateDevice(DEV_IDX_10K_UP);
          activateDevice(DEV_IDX_10K_BELL);
          activateDevice(DEV_IDX_100K_BELL);
        }
        currentScore = nextScore;
        displayScore(currentScore);
      }
      batchHundredsRemaining = 0;
      lastCommandWasMotor = false;
      motorCycleStartMs   = millis();
    }
    else if (batchHundredsRemaining == 0 && batchTensRemaining == 1) {
      int nextScore = constrain(currentScore + scoreCmdActiveDirection, 0, 999);
      if (nextScore != currentScore) {
        if (noiseMakersEnabled()) {
          activateDevice(DEV_IDX_10K_UP);
          activateDevice(DEV_IDX_10K_BELL);
          if ((nextScore % 10) == 0 || (scoreCmdActiveDirection < 0 && (currentScore % 10) == 0)) {
            activateDevice(DEV_IDX_100K_BELL);
          }
        }
        currentScore = nextScore;
        displayScore(currentScore);
      }
      batchTensRemaining = 0;
      lastCommandWasMotor = false;
      motorCycleStartMs   = millis();
    }

    // Decide if we need to enter motor pacing for remaining work.
    if (batchHundredsRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = true;
      cycleActionLimit     = (byte)min(batchHundredsRemaining, 5);
      scoreAdjustCycleIndex = 0;
    } else if (batchHundredsRemaining == 0 && batchTensRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = false;
      cycleActionLimit     = (byte)min(batchTensRemaining, 5);
      scoreAdjustCycleIndex = 0;
    } else if (batchHundredsRemaining == 0 && batchTensRemaining == 0) {
      scoreAdjustFinishIfDone();
    }
    return;
  }

  // Motor-paced cycle: cycleActionLimit advance slots (0..cycleActionLimit-1), remainder are rest slots.
  if (scoreAdjustCycleIndex < SCORE_MOTOR_STEPS) {
    if (scoreAdjustCycleIndex < cycleActionLimit) {
      int stepUnits = scoreCycleHundred ? 10 : 1;
      int nextScore = constrain(currentScore + scoreCmdActiveDirection * stepUnits, 0, 999);
      if (nextScore != currentScore) {
        if (noiseMakersEnabled()) {
          activateDevice(DEV_IDX_10K_UP);
          activateDevice(DEV_IDX_10K_BELL);
          if (scoreCycleHundred ||
            (nextScore % 10) == 0 ||
            (scoreCmdActiveDirection < 0 && (currentScore % 10) == 0)) {
            activateDevice(DEV_IDX_100K_BELL);
          }
        }
        currentScore = nextScore;
        displayScore(currentScore);
        if (scoreCycleHundred) {
          batchHundredsRemaining--;
        }
        else {
          batchTensRemaining--;
        }
      }
      else {
        // Saturation occurred mid-batch (boundary reached).
        if (scoreCycleHundred) {
          batchHundredsRemaining = 0;
        } else {
          batchTensRemaining = 0;
        }
      }
    }
  }

  // Advance sub-step index and handle cycle completion.
  scoreAdjustCycleIndex++;
  if (scoreAdjustCycleIndex >= SCORE_MOTOR_STEPS) {
    scoreAdjustCycleIndex = 0;
    if (scoreCycleHundred) {
      if (batchHundredsRemaining > 0) {
        cycleActionLimit = (byte)min(batchHundredsRemaining, (int)5);
        if (batchHundredsRemaining == 1 && !motorPersistent) {
          scoreAdjustMotorMode = false;
          lastCommandWasMotor  = false;
          motorCycleStartMs    = millis();
        } else {
          scoreAdjustMotorMode = true;
          lastCommandWasMotor  = true;
          motorCycleStartMs    = millis();
        }
      } else {
        if (batchTensRemaining > 0) {
          scoreCycleHundred = false;
          cycleActionLimit = (byte)min(batchTensRemaining, (int)5);
          if (batchTensRemaining == 1 && !motorPersistent) {
            scoreAdjustMotorMode = false;
            lastCommandWasMotor  = false;
            motorCycleStartMs    = millis();
          } else if (batchTensRemaining > 0) {
            scoreAdjustMotorMode = true;
            lastCommandWasMotor  = true;
            motorCycleStartMs    = millis();
          } else {
            scoreAdjustMotorMode = false;
            lastCommandWasMotor  = motorPersistent;
          }
        } else {
          scoreAdjustMotorMode = false;
        }
      }
    } else { // Tens cycles
      if (batchTensRemaining > 0) {
        cycleActionLimit = (byte)min(batchTensRemaining, (int)5);
        if (batchTensRemaining == 1 && !motorPersistent) {
          scoreAdjustMotorMode = false;
          lastCommandWasMotor  = false;
          motorCycleStartMs    = millis();
        } else {
          scoreAdjustMotorMode = true;
          lastCommandWasMotor  = true;
          motorCycleStartMs    = millis();
        }
      } else {
        scoreAdjustMotorMode = false;
      }
    }
  }

  // Attempt finish if batch consumed and not in motor pacing.
  if (batchHundredsRemaining == 0 && batchTensRemaining == 0 && !scoreAdjustMotorMode) {
    scoreAdjustFinishIfDone();
  }
}

void scoreAdjustFinishIfDone() {
  // Finalizes score adjustment when all batch units consumed and rest interval satisfied.
  // If more queued commands remain, starts next batch; else persists score and clears state.
  if (batchHundredsRemaining != 0 || batchTensRemaining != 0) {
    return;
  }
  if (scoreAdjustMotorMode) {
    return;
  }

  unsigned long now = millis();
  unsigned requiredMs = lastCommandWasMotor ? SCORE_MOTOR_CYCLE_MS : SCORE_MOTOR_STEP_MS;
  if (now - motorCycleStartMs < requiredMs) {
    return;
  }

  // Next queued command? Use centralized dequeue helper to avoid duplicated head/tail logic.
  int cmd = 0;
  if (dequeueScoreCmdIfAny(&cmd)) {
    // Delegate to helper that sets up batch state, timestamps and flags.
    startBatchFromCmd(cmd);
    // startBatchFromCmd() sets scoreAdjustActive and scoreAdjustLastMs.
    return;
  }

  // No pending commands; persist and clear.
  scoreAdjustActive       = false;
  scoreAdjustMotorMode    = false;
  scoreCycleHundred       = false;
  scoreCmdActiveDirection = 0;
  scoreAdjustCycleIndex   = 0;
  scoreCommandSilent      = false;  // NEW: clear silent flag when batch completes
}

void pruneQueuedAtBoundary(int t_saturatedDir) {
  // Removes queued commands that continue in a saturated direction (already at min/max score).
  // Also aborts current active batch if it matches saturation direction.
  if (scoreCmdHead == scoreCmdTail) {
    return;
  }
  int tmp[SCORE_CMD_QUEUE_SIZE]; byte wt = 0; byte rd = scoreCmdHead;
  while (rd != scoreCmdTail) {
    int cmd = scoreCmdQueue[rd];
    if (((cmd > 0) ? 1 : -1) != t_saturatedDir) {
      tmp[wt++] = cmd;
    }
    rd = (byte)((rd + 1) % SCORE_CMD_QUEUE_SIZE);
  }
  scoreCmdHead = 0;
  scoreCmdTail = wt;
  for (byte i = 0; i < wt; i++) {
    scoreCmdQueue[i] = tmp[i];
  }
  if (scoreAdjustActive && scoreCmdActiveDirection == t_saturatedDir) {
    batchHundredsRemaining = 0;
    batchTensRemaining     = 0;
    scoreAdjustMotorMode   = false;
    scoreAdjustActive      = false;
  }
}

void processScoreReset() {
  // Drives multi-phase visual reset sequence:
  // Phase REV_HIGH: rapidly decrement hundredK/millions without timing slots (simulated fast reverse).
  // Phase 10K_CYCLE1 / 10K_CYCLE2: motor-paced tensK advances in one or two cycles.
  // Phase DONE: persist final score and exit.
  // Non-blocking score reset state machine.
  // Overall goal: visually (and audibly) return displayed score to a zeroed state without
  // performing normal rollover side-effects (lamps for hundredK/millions stay suppressed during tens cycling).
  //
  // PHASE OVERVIEW:
  //   REV_HIGH:
  //     Rapid decrement of the combined high portion (millions*10 + hundredK) from its starting value down to 0.
  //     Each decrement updates display immediately. Uses a fixed short interval (RESET_HIGH_STEP_MS) to
  //     simulate a rapid reverse action distinct from standard motor pacing. After value reaches 0, waits one full
  //     motor cycle duration (882ms) before moving to tens cycles for timing realism.
  //   10K_CYCLE1:
  //     First motor-paced tensK cycle. Up to 5 advance slots (each 147ms) followed by one rest slot (also 147ms).
  //     Only tensK digit increments. If a tensK rollover (from 9 to 0) occurs, the 100K bell is rung but hundredK/millions digits
  //     remain visually frozen. If more tens steps remain after first cycle completion, transition to 10K_CYCLE2.
  //   10K_CYCLE2:
  //     Second (and final) tensK cycle if required (for total requested steps >5). Same pacing pattern. When remaining tens steps reach 0,
  //     transition to DONE.
  //   DONE:
  //     Persist resulting displayed score and clear resetActive.
  //
  // TIMING NOTES:
  //   - Motor slot pacing uses SCORE_MOTOR_STEP_MS (147ms).
  //   - Each cycle logically spans SCORE_MOTOR_STEPS (6): first up to 5 advance slots, last slot is rest/silence.
  //   - The high reversal phase uses RESET_HIGH_STEP_MS for each decrement and then enforces a whole 882ms
  //     delay (SCORE_MOTOR_CYCLE_MS) before beginning the tens cycles to emulate a consumed quarter revolution.
  //
  // SAFETY:
  //   - Function exits early if resetActive is false.
  //   - No score adjust operations occur while resetActive is true (handled externally).
  if (!resetActive) {
    return;
  }

  unsigned long now = millis();

  // Phase REV_HIGH (rapid high-order reverse)
  if (resetPhase == SCORE_RESET_PHASE_REV_HIGH) {
    if (resetHighUnitsRemaining > 0) {
      // Perform as many fixed-interval high decrements as time allows without skipping intermediate values.
      while (resetHighUnitsRemaining > 0 &&
        (now - resetHighLastMs) >= resetHighIntervalMs) {
        resetHighLastMs += resetHighIntervalMs;
        resetHighCurrent--;
        resetHighUnitsRemaining--;
        if (resetHighCurrent < 0) {
          resetHighCurrent = 0;
          resetHighUnitsRemaining = 0;
        }
        int newMillions = resetHighCurrent / 10;
        int newHundredK = resetHighCurrent % 10;
        int tensK       = resetTensKStart;
        currentScore = newMillions * 100 + newHundredK * 10 + tensK;
        displayScore(currentScore);
      }
    }
    // After all high reversals complete, wait a full motor cycle for realism before tens cycling.
    if (resetHighUnitsRemaining == 0 &&
      (now - resetHighCycleStartMs) >= SCORE_MOTOR_CYCLE_MS) {
      resetPhase       = SCORE_RESET_PHASE_10K_CYCLE1;
      resetCycleIndex  = 0;
      resetLastMs      = now - SCORE_MOTOR_STEP_MS; // Immediate slot allowed.
    }
    return;
  }

  // TensK phases (1 or 2 cycles).
  // TensK phases use motor sub-step pacing; enforce slot delay before advancing.
  if ((now - resetLastMs) < SCORE_MOTOR_STEP_MS) {
    return;
  }
  resetLastMs = now;
  // Track current slot index; index rolls modulo SCORE_MOTOR_STEPS (6).
  byte localIndex = resetCycleIndex;
  resetCycleIndex = (byte)((resetCycleIndex + 1) % SCORE_MOTOR_STEPS);
  // Phase 10K cycles (first and optional second)
  if (resetPhase == SCORE_RESET_PHASE_10K_CYCLE1 ||
    resetPhase == SCORE_RESET_PHASE_10K_CYCLE2) {
    bool secondCycle = (resetPhase == SCORE_RESET_PHASE_10K_CYCLE2);
    // Advance tensK for first 5 slots only; slot 5 (index == SCORE_MOTOR_STEPS - 1) is the rest slot.
    if (localIndex < (SCORE_MOTOR_STEPS - 1) && reset10KStepsRemaining > 0) {
      int tensK    = currentScore % 10;
      int hundredK = (currentScore / 10) % 10;
      int millions = (currentScore / 100) % 10;
      int newTensK = (tensK + 1) % 10;
      // At rollover (9->0) play the 100K bell but suppress lamp promotion of hundredK/millions during reset.
      if (noiseMakersEnabled()) {
        if (newTensK == 0) {
          activateDevice(DEV_IDX_100K_BELL);
        }
        // Fire 10K unit and bell each advance.
        activateDevice(DEV_IDX_10K_UP);
        activateDevice(DEV_IDX_10K_BELL);
      }

      currentScore = millions * 100 + hundredK * 10 + newTensK;
      displayScore(currentScore);
      reset10KStepsRemaining--;
    }

    // After rest slot (cycleIndex rolled to 0) decide next phase.
    if (resetCycleIndex == 0) {
      if (reset10KStepsRemaining == 0) {
        // All tensK increments done.
        resetPhase = SCORE_RESET_PHASE_DONE;
      } else if (!secondCycle) {
        // Need a second tens cycle (total steps >5)
        resetPhase = SCORE_RESET_PHASE_10K_CYCLE2;
      } else {
        // Should not need more than two cycles (sanity fallback)
        resetPhase = SCORE_RESET_PHASE_DONE;
      }
    }
    return;
  }
  // Finalization phase
  if (resetPhase == SCORE_RESET_PHASE_DONE) {
    // Finalize reset sequence.
    resetActive = false;
    pLCD2004->println("Reset done");
  }
}

void startNewGame(byte t_mode) {
  // Initialize new game state:
  // - Stores mode
  // - Clears tilt lamp, turns on G.I.
  // - Resets score and score processing queues (any queued adjustments are discarded.)
  //   This ensures score logic restarts from a clean baseline.
  // - Updates score display to zero.
  modeCurrent   = t_mode;
  setTiltLamp(false);
  setGILamp(true);
  currentScore  = 0;
  targetScore   = 0;
  scoreCmdHead  = scoreCmdTail = 0;
  scoreAdjustActive = false;
  scoreCmdActiveDirection = 0;
  displayScore(currentScore);
}

// ************************************************************************************
// ****************** D I S P L A Y   &   P E R S I S T E N C E   F U N C T I O N S *************
// ************************************************************************************

void setScoreLampsDirect(int t_score) {
  if (t_score < 0) t_score = 0;
  if (t_score > 999) t_score = 999;

  // Fast path: for zero, force all 27 lamps OFF to avoid any stale-on conditions.
  if (t_score == 0) {
    // Millions (1..9)
    for (int i = 0; i < 9; ++i) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_1M[i], HIGH);
    }
    // HundredK (1..9)
    for (int i = 0; i < 9; ++i) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_100K[i], HIGH);
    }
    // TensK (1..9)
    for (int i = 0; i < 9; ++i) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_10K[i], HIGH);
    }
    lastDisplayedScore = 0;
    return;
  }

  int millions = t_score / 100;
  int hundredK = (t_score / 10) % 10;
  int tensK    = t_score % 10;

  int lastMillions = (lastDisplayedScore < 0) ? 0 : lastDisplayedScore / 100;
  int lastHundredK = (lastDisplayedScore < 0) ? 0 : (lastDisplayedScore / 10) % 10;
  int lastTensK    = (lastDisplayedScore < 0) ? 0 : lastDisplayedScore % 10;

  // Millions lamps (active LOW)
  if (lastMillions != millions) {
    if (lastMillions > 0) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_1M[lastMillions - 1], HIGH); // OFF
    }
    if (millions > 0) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_1M[millions - 1], LOW);      // ON
    }
  }
  // 100K lamps (active LOW)
  if (lastHundredK != hundredK) {
    if (lastHundredK > 0) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_100K[lastHundredK - 1], HIGH);
    }
    if (hundredK > 0) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_100K[hundredK - 1], LOW);
    }
  }
  // 10K lamps (active LOW)
  if (lastTensK != tensK) {
    if (lastTensK > 0) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_10K[lastTensK - 1], HIGH);
    }
    if (tensK > 0) {
      pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_10K[tensK - 1], LOW);
    }
  }
  lastDisplayedScore = t_score;
}

// ---- Start/Stop flash helpers ----
void startFlashScore(int t_score) {
  // If t_score == 0, stop flashing and resume normal display (no score changes).
  if (t_score <= 0) {
    if (flashActive) {
      stopFlashScore();        // restores current display
    } else {
      displayScore(currentScore);
    }
    return;
  }
  flashActive = true;
  flashScoreValue = constrain(t_score, 0, 999);
  flashShowOn = true;
  flashLastToggleMs = millis();
  setScoreLampsDirect(flashScoreValue);
}

void stopFlashScore() {
  if (!flashActive) return;
  flashActive = false;

  // Force a clean base state: turn all score lamps OFF and reset cache.
  setScoreLampsDirect(0);   // explicit all-off path
  lastDisplayedScore = -1;  // force next displayScore() to recalc transitions fully

  // Restore normal displayed score immediately
  displayScore(currentScore);
}

void displayScore(int t_score) {
  // Do not override flashing display
  if (flashActive) {
    return;
  }

  if (t_score < 0) {
    t_score = 0;
  }
  if (t_score > 999) {
    t_score = 999;
  }

  // Centralize lamp updates in setScoreLampsDirect() to avoid duplicated logic.
  setScoreLampsDirect(t_score);
}

void saveScoreToEEPROM(int t_score) {
  // Persists score only if changed to reduce EEPROM wear.
  // Input clamped into 0..999 before comparison/write.
  if (t_score < 0) {
    t_score = 0;
  }
  if (t_score > 999) {
    t_score = 999;
  }
  unsigned int newVal = (unsigned int)t_score;
  unsigned int oldVal = 0;
  EEPROM.get(EEPROM_ADDR_SCORE, oldVal);
  if (oldVal != newVal) {
    EEPROM.put(EEPROM_ADDR_SCORE, newVal);
    sprintf(lcdString, "Saved score %d", t_score); pLCD2004->println(lcdString);
  }
}

int recallScoreFromEEPROM() {
  // Reads last persisted score (unsigned int) from EEPROM and clamps to 0..999.
  // Returns 0 if stored value > 999 (treat as invalid).
  unsigned int s = 0;
  EEPROM.get(EEPROM_ADDR_SCORE, s);
  if (s > 999) {
    s = 0;
  }
  return (int)s;
}

// ************************************************************************************
// ****************** H E L P E R   &   U T I L I T Y   F U N C T I O N S *************
// ************************************************************************************

bool hasCredits() {
  // Credit Empty switch LOW indicates at least one credit remains.
  return digitalRead(PIN_IN_SWITCH_CREDIT_EMPTY) == LOW;
}

bool noiseMakersEnabled() {
  // Return true when mechanical score sounds (10K unit, bells in head; Selection, reset units in cabinet)
  // should be used for *automatic* scoring behavior.
  // Original and Impulse styles => authentic EM behavior: noise makers ON.
  // Enhanced style              => silent scoring: bells/10K unit suppressed for auto scoring.
  // Silent flag                 => Master explicitly requests silence regardless of mode.
  // NOTE: This does NOT affect explicit "fire bell/10K unit" commands coming from Master.
  //       Those commands still directly activate the devices regardless of style.

  // If Master explicitly requested silence, honor that first
  if (scoreCommandSilent) {
    return false;
  }

  // Otherwise use mode-based logic
  if (modeCurrent == MODE_ENHANCED) {
    return false;
  }
  return true;
}

void handleError(byte t_errorCode) {
  // Fatal error; display on LCD and flash TILT lamp
  // Force all hardware OFF
  setAllDevicesOff();
  clearAllPWM();
  // Display error on LCD
  pLCD2004->println("RS485 COMM ERROR:");
  if (t_errorCode == RS485_ERROR_UNEXPECTED_TYPE)       sprintf(lcdString, "Unknown type");
  else if (t_errorCode == RS485_ERROR_BUFFER_OVERFLOW)  sprintf(lcdString, "Buffer overflow");
  else if (t_errorCode == RS485_ERROR_MSG_TOO_SHORT)    sprintf(lcdString, "Msg too short");
  else if (t_errorCode == RS485_ERROR_MSG_TOO_LONG)     sprintf(lcdString, "Msg too long");
  else if (t_errorCode == RS485_ERROR_CHECKSUM)         sprintf(lcdString, "Bad checksum");
  else                                                  sprintf(lcdString, "Unknown %i", t_errorCode);
  pLCD2004->println(lcdString);
  // Flash TILT lamp at FLASH_RATE_MS
  setGITiltLampBrightness(deviceParm[DEV_IDX_LAMP_HEAD_GI_TILT].powerInitial);  // We just turned MOSFETs off; turn TILT back on!
  while (true) {
    setTiltLamp(true);
    delay(FLASH_RATE_MS);
    setTiltLamp(false);
    delay(FLASH_RATE_MS);
  }
}