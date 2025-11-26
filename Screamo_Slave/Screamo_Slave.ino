// Screamo_Slave.INO Rev: 11/26/25

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

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>

#include <EEPROM.h>               // For saving and recalling score, to persist between power cycles.
const int EEPROM_ADDR_SCORE = 0;  // Address to store 16-bit score (uses addr 0 and 1)

const byte THIS_MODULE = ARDUINO_SLV;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SLAVE 11/26/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which effectively makes it a global.
// No need to pass lcdString[] to any functions that use it!

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
struct deviceParmStruct {
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

const byte NUM_DEVS = 8;

// DeviceParm table: { pinNum, powerInitial, timeOn, powerHold, countdown, queueCount }
//   pinNum: Arduino pin number for this coil
//   powerInitial: 0..255 for MOSFET coils and lamps
//   timeOn: Number of 10ms loops to hold initial power level before changing to hold level or turning off
//   powerHold: 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
//   countdown: current countdown in 10ms ticks; -1..-8 = rest period, 0 = idle, >0 = active
//   queueCount: number of pending activation requests while coil busy/resting
//   I.e.: analogWrite(deviceParm[DEV_IDX_CREDIT_UP].pinNum, deviceParm[DEV_IDX_CREDIT_UP].powerInitial);
// --- UPDATE deviceParm[] (adjust comments to reflect rapid-fire constraints) ---
deviceParmStruct deviceParm[NUM_DEVS] = {
  {  5, 120, 10, 0, 0, 0 },  // CREDIT UP.   timeOn 10 (=100ms); not score-motor paced.
  {  6, 150, 10, 0, 0, 0 },  // CREDIT DOWN. timeOn 10 (=100ms); not score-motor paced.
  {  7, 200,  6, 0, 0, 0 },  // 10K UP. 6 ticks ON (60ms) + 8 ticks rest (80ms) = 140ms cycle (<147ms).
  {  8, 150,  5, 0, 0, 0 },  // 10K BELL. 5 ticks ON (50ms) + 8 ticks rest (80ms) = 130ms cycle (<147ms).
  {  9, 150,  5, 0, 0, 0 },  // 100K BELL. 5 ticks ON (50ms) + 8 ticks rest (80ms) = 130ms cycle (<147ms).
  { 10, 150,  5, 0, 0, 0 },  // SELECT BELL. Not required to meet 147ms; uses default rest.
  { 11,  40,  0, 0, 0, 0 },  // LAMP SCORE (timeOn=0 -> no pulse timing).
  { 12,  40,  0, 0, 0, 0 }   // LAMP HEAD G.I./TILT (timeOn=0).
};

// *** MISC CONSTANTS AND GLOBALS ***
byte modeCurrent               = MODE_UNDEFINED;
byte msgType                   = RS485_TYPE_NO_MESSAGE;  // Current RS485 message type being processed.

const byte SCREAMO_SLAVE_TICK_MS = 10;  // One scheduler tick == 10ms

int currentScore               =  0;    // Current game score (0..999).
int targetScore                =  0;    // NEW: merged desired final score (0..999) updated by incoming RS485 score deltas.

unsigned long lastLoopTime     =  0;    // Used to track 10ms loop timing.

bool debugOn    = false;

// --- ADD RAPID-FIRE REST CONSTANTS (place near other globals) ---
// Rapid-fire devices (10K Unit, 10K Bell, 100K Bell) may have timeOn up to 6 ticks (60ms).
// They must be able to re-fire every 147ms score motor sub-step.
// Fixed rest for those = 8 ticks (80ms) so: 60ms ON + 80ms REST = 140ms < 147ms.
const int8_t RAPID_REST_TICKS   = -8;  // 80ms rest for rapid-fire scoring coils
const int8_t DEFAULT_REST_TICKS = -8;  // Existing rest for other pulse coils (credits, select bell, etc.)

// Helper to identify rapid-fire scoring coils
static inline bool isRapidFireDevice(byte idx) {
  return (idx == DEV_IDX_10K_UP) || (idx == DEV_IDX_10K_BELL) || (idx == DEV_IDX_100K_BELL);
}

// Add command queue (FIFO) for sequential execution:
static int  scoreCmdQueue[8];          // up to 8 pending commands
static byte scoreCmdHead = 0;          // dequeue index
static byte scoreCmdTail = 0;          // enqueue index
static int  activeDirection      = 0;  // +1 or -1 for current command

// Score motor timing (modifiable)
const unsigned SCORE_MOTOR_CYCLE_MS = 882;                 // one 1/4 revolution of score motor (ms)
const unsigned SCORE_MOTOR_STEPS    = 6;                   // sub-steps per 1/4 rev (up to 5 advances + 1 rest)
const unsigned SCORE_MOTOR_STEP_MS  = SCORE_MOTOR_CYCLE_MS / SCORE_MOTOR_STEPS; // spacing per sub-step (ms)

const byte SCORE_RESET_PHASE_REV_HIGH   = 0;
const byte SCORE_RESET_PHASE_10K_CYCLE1 = 1;
const byte SCORE_RESET_PHASE_10K_CYCLE2 = 2;
const byte SCORE_RESET_PHASE_DONE       = 3;

// State used when executing batched (multi-step) 10K adjustments
static bool       scoreAdjustActive  = false;   // true while a batch is executing
static bool   scoreAdjustMotorMode   = false;  // true when current 10K command uses motor pacing
static bool scoreCycleHundred      = false; // true if current motor cycle is hundredK-type
static byte   scoreAdjustCycleIndex  = 0;      // 0..(SCORE_MOTOR_STEPS-1) index into sub-step
static byte cycleActionLimit       = 0;   // planned advances in this cycle (1..5)
static unsigned long scoreAdjustLastMs = 0;       // last timestamp (ms) a slot or single pulse was attempted
static unsigned long motorCycleStartMs = 0;  // Timestamp when current motor cycle began
static bool lastCommandWasMotor = false;      // True if current command uses motor cycle timing
static bool motorPersistent     = false;      // True if command started as multi-unit (>1) and must remain motor-paced even for final single

static bool  resetActive            = false;
static byte  resetPhase             = SCORE_RESET_PHASE_DONE;
static byte  resetCycleIndex        = 0;        // 0..(SCORE_MOTOR_STEPS-1) within current motor cycle
static unsigned long resetLastMs    = 0;        // pacing timestamp
static int   reset10KStepsRemaining = 0;        // total 10K increments still to perform (6..10 or 1..5)

// High reversal pacing
static int  resetHighCurrent      = 0;   // current high (millions*10 + hundredK) value during reversal (0..99)
static int  resetTensKStart       = 0;   // tensK digit to hold constant during high reversal
static int  resetHighUnitsRemaining   = 0;     // number of 100K units (millions*10 + hundredK)
static unsigned long resetHighLastMs  = 0;     // timestamp of last high decrement
static unsigned resetHighIntervalMs   = 0;     // ms between high decrements during reversal
static unsigned long resetHighCycleStartMs = 0;
const unsigned RESET_HIGH_FIXED_INTERVAL_MS = 8;  // ms between rapid high (100K) reversal decrements

// ---------------- Unified batch decomposition state ----------------
static int  batchHundredsRemaining = 0;   // number of 100K steps left in current batch (each == 10 units)
static int  batchTensRemaining     = 0;   // number of 10K steps left in current batch

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  initScreamoSlaveArduinoPins();  // Arduino direct I/O pins only.

  setAllDevicesOff();  // Set initial state for all MOSFET PWM output pins (pins 5-12)

  setPWMFrequencies(); // Set non-default PWM frequencies for certain pins.

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 LCD2004 instantiated via pLCD2004->begin.
  // Serial2 RS485   instantiated via pMessage->begin.

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Pinball_Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // *** INITIALIZE PINBALL_CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Pinball_Centipede class hangs the system if hardware is not connected.
  // We're doing this near the top of the code so we can turn on G.I. as quickly as possible.
  Wire.begin();                               // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Pinball_Centipede;     // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initScreamoSlaveCentipedePins();

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
  sprintf(lcdString, "Recall score %d", currentScore); pLCD2004->println(lcdString);
  displayScore(currentScore);
  targetScore  = currentScore;  // initialize target

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  // Enforce 10ms tick (optional but recommended for deterministic timing)
  unsigned long now = millis();
  if (now - lastLoopTime < SCREAMO_SLAVE_TICK_MS) return;  // Still within this 10ms window
  lastLoopTime = now;

  // Handle all pending RS-485 messages (non-blocking drain)
  while ((msgType = pMessage->available()) != RS485_TYPE_NO_MESSAGE) {
    processMessage(msgType);
  }

  // Update all active device timers (solenoids, bells, etc.)
  updateDeviceTimers();

  // Run non-blocking score adjustment state machine
  processScoreAdjust();
  processScoreReset();  // handle active score reset sequence

  // Other housekeeping (score lamp updates, switch reads, etc.)

}  // End of loop()

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

void setAllDevicesOff() {
  for (int i = 0; i < NUM_DEVS; i++) {
    deviceParm[i].countdown = 0;
    deviceParm[i].queueCount = 0;
    digitalWrite(deviceParm[i].pinNum, LOW);  // Ensure all MOSFET outputs are off at startup.
    pinMode(deviceParm[i].pinNum, OUTPUT);
  }
}

void setPWMFrequencies() {
  // Increase PWM frequency for pins 11 and 12 (Timer 1) to reduce LED flicker
  // Pins 11 and 12 are Score lamp and G.I./Tilt lamp brightness control
  // Set Timer 1 prescaler to 1 for ~31k
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;
}

bool canActivateDeviceNow(byte t_devIdx) {
  // Return true if it's safe to start energizing device t_devIdx right now.
  if (t_devIdx == DEV_IDX_CREDIT_UP) {
    // Prevent firing if credit full (protect mechanism)
    if (digitalRead(PIN_IN_SWITCH_CREDIT_FULL) == HIGH) {
      // Full -> block to protect mechanism
      return false;
    }
  } else if (t_devIdx == DEV_IDX_CREDIT_DOWN) {
    // Prevent firing if no credits available (protect mechanism)
    if (!hasCredits()) {
      // Empty -> block to protect mechanism
      return false;
    }
  }
  return true;
}

void activateDevice(byte t_devIdx) {
  if (t_devIdx >= NUM_DEVS) {
    // Invalid device index
    setAllDevicesOff();
    // In activateDevice() invalid index branch:
    snprintf(lcdString, sizeof(lcdString), "DEV NUM ERROR %u", (unsigned)t_devIdx);
    pLCD2004->println(lcdString);
    while (true) { }  // Halt on error
  }
  if (deviceParm[t_devIdx].countdown > 0) {
    // coil already active; queue this request
    deviceParm[t_devIdx].queueCount++;
    return;
  }
  if (deviceParm[t_devIdx].countdown < 0) {
    // coil in rest period; queue this request
    deviceParm[t_devIdx].queueCount++;
    return;
  }
  // countdown == 0 -> coil idle; safe to start
  if (canActivateDeviceNow(t_devIdx)) {  // Check any device-specific conditions i.e. credit full/empty before adding/deducting credit
    analogWrite(deviceParm[t_devIdx].pinNum, deviceParm[t_devIdx].powerInitial);
    deviceParm[t_devIdx].countdown = deviceParm[t_devIdx].timeOn;
    return;
  }
}

void processMessage(byte t_msgType) {
  // Only call when pMessage->available() has indicated a message is waiting (i.e. not RS485_TYPE_NO_MESSAGE).
  // For any message with parms, we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.
  // But for messages that don't have parms, we can just act on the message type immediately.

  switch (t_msgType) {
    case RS485_TYPE_MAS_TO_SLV_MODE:
      {
        byte newMode = 0;
        pMessage->getMAStoSLVMode(&newMode);
        // 1: Tilt, 2: Game Over, 3: Diagnostic, 4: Original, 5: Enhanced, 6: Impulse
        sprintf(lcdString, "Mode change %u", newMode); pLCD2004->println(lcdString);
        startNewGame(newMode);
      }
      break;

    case RS485_TYPE_MAS_TO_SLV_CREDIT_STATUS:
      sprintf(lcdString, "Credits %s", hasCredits() ? "avail" : "zero"); pLCD2004->println(lcdString);
      pMessage->sendSLVtoMASCreditStatus(hasCredits());
      break;

    case RS485_TYPE_MAS_TO_SLV_CREDIT_INC:
      // Non-blocking: queue one activation per requested credit so updateDeviceTimers()
      // will pulse the CREDIT_UP coil safely and enforce rest periods.
      {
        byte numCredits = 0;
        pMessage->getMAStoSLVCreditInc(&numCredits);
        // Log to LCD (use snprintf to avoid buffer overflow)
        snprintf(lcdString, sizeof(lcdString), "Credit inc %u", (unsigned)numCredits);
        pLCD2004->println(lcdString);
        // Queue activations (activateDevice() will handle busy/rest/full logic)
        int swFull = digitalRead(PIN_IN_SWITCH_CREDIT_FULL);
        int swEmpty = digitalRead(PIN_IN_SWITCH_CREDIT_EMPTY);
        // Show switch states once per message for troubleshooting.
        snprintf(lcdString, sizeof(lcdString), "Sw FULL=%d EMPTY=%d", swFull, swEmpty);
        pLCD2004->println(lcdString);
        for (unsigned i = 0; i < numCredits; ++i) {
          activateDevice(DEV_IDX_CREDIT_UP);
        }
      }
      break;

    case RS485_TYPE_MAS_TO_SLV_CREDIT_DEC:
      pLCD2004->println("Credit dec.");
      // Fire the Credit Down coil if Credit Empty switch is closed (LOW)
      if (hasCredits()) {
        activateDevice(DEV_IDX_CREDIT_DOWN);
      }
      break;

      // PATCH 2: Remove extra 10K Unit coil pulse at start of reset (RS485_TYPE_MAS_TO_SLV_SCORE_RESET)
      //          so only the planned reset sequence drives 1–10 pulses.
      // In processMessage() switch, replace the reset case body with:
    case RS485_TYPE_MAS_TO_SLV_SCORE_RESET:
      if (!resetActive) {
        scoreCmdHead = scoreCmdTail = 0;
        scoreAdjustActive = false;

        int startScore = currentScore;
        int tensK    = startScore % 10;
        int hundredK = (startScore / 10) % 10;
        int millions = (startScore / 100) % 10;

        resetHighCurrent        = millions * 10 + hundredK;
        resetHighUnitsRemaining = millions * 10 + hundredK;
        resetTensKStart         = tensK;

        resetHighIntervalMs     = (resetHighUnitsRemaining > 0) ? RESET_HIGH_FIXED_INTERVAL_MS : 0;
        resetHighLastMs         = millis();
        resetHighCycleStartMs   = resetHighLastMs;

        int rawSteps = (10 - tensK) % 10;
        if (tensK == 0) rawSteps = 10;
        reset10KStepsRemaining = rawSteps;

        resetPhase      = SCORE_RESET_PHASE_REV_HIGH;
        resetCycleIndex = 0;
        resetLastMs     = millis() - SCORE_MOTOR_STEP_MS;
        resetActive     = true;

        pLCD2004->println("Reset begin");
      }
      break;

    case RS485_TYPE_MAS_TO_SLV_SCORE_ABS:
     {
      int absScore = 0;
      pMessage->getMAStoSLVScoreAbs(&absScore);
      int newScore = constrain(absScore, 0, 999);
      currentScore = newScore;
      targetScore  = newScore;
      scoreCmdHead = scoreCmdTail = 0;
      scoreAdjustActive = false;
      activeDirection = 0;
      snprintf(lcdString, sizeof(lcdString), "Score set %u", (unsigned)currentScore);
      pLCD2004->println(lcdString);
      displayScore(currentScore);
      saveScoreToEEPROM(currentScore);
    }
    break;

      // MODIFY: processMessage() case RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K to accept signed (-999..999) and clamp/queue prune
    case RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K:  // Increment (or decrement if negative) score update (-999..999 in 10Ks)
    {
      int incAmount = 0;
      pMessage->getMAStoSLVScoreInc10K(&incAmount);  // now treated as signed
      // Hard clamp incoming parameter range
      if (incAmount > 999) incAmount = 999;
      else if (incAmount < -999) incAmount = -999;

      // If already at boundary and trying to go further, ignore
      if ((incAmount > 0 && targetScore >= 999) || (incAmount < 0 && targetScore <= 0)) {
        // Prune any already queued same-direction commands (saturation)
        pruneQueuedAtBoundary(incAmount > 0 ? 1 : -1);
        snprintf(lcdString, sizeof(lcdString), "Sat ignore %d", incAmount);
        pLCD2004->println(lcdString);
        break;
      }

      // Adjust incAmount so targetScore stays within 0..999 considering existing queued work
      long prospective = (long)targetScore + incAmount;
      if (prospective > 999) {
        incAmount = 999 - targetScore;
      }
      else if (prospective < 0) {
        incAmount = -targetScore;
      }

      if (incAmount == 0) {
        // Nothing effective to do (already saturated after adjustment)
        pruneQueuedAtBoundary((targetScore == 999) ? 1 : (targetScore == 0 ? -1 : 0));
        break;
      }

      snprintf(lcdString, sizeof(lcdString), "Score adj %d", incAmount);
      pLCD2004->println(lcdString);

      requestScoreAdjust(incAmount);

      // If we just saturated the target after enqueue, prune future same-direction commands
      if (targetScore == 999) pruneQueuedAtBoundary(1);
      else if (targetScore == 0) pruneQueuedAtBoundary(-1);
    }
    break;

    case RS485_TYPE_MAS_TO_SLV_BELL_10K:  // Ring the 10K bell
      pLCD2004->println("10K Bell");
      activateDevice(DEV_IDX_10K_BELL);
      break;

    case RS485_TYPE_MAS_TO_SLV_BELL_100K:  // Ring the 100K bell
      pLCD2004->println("100K Bell");
      activateDevice(DEV_IDX_100K_BELL);
      break;

    case RS485_TYPE_MAS_TO_SLV_BELL_SELECT:  // Ring the Select bell
      pLCD2004->println("Select Bell");
      activateDevice(DEV_IDX_SELECT_BELL);
      break;

    case  RS485_TYPE_MAS_TO_SLV_10K_UNIT:  // Pulse the 10K Unit coil (for testing)
      activateDevice(DEV_IDX_10K_UP);
      break;

    case RS485_TYPE_MAS_TO_SLV_SCORE_QUERY:  // Master requesting current score displayed by Slave(in 10,000s)
      {
        snprintf(lcdString, sizeof(lcdString), "Score rpt %u", (unsigned)currentScore);
        pLCD2004->println(lcdString);
        pMessage->sendSLVtoMASScoreReport(currentScore);
      }
      break;

    case RS485_TYPE_MAS_TO_SLV_GI_LAMP:  // Master command to turn G.I. lamps on or off
      {
        bool giOn = false;
        pMessage->getMAStoSLVGILamp(&giOn);
        setGILamp(giOn);
      }
      break;

    case RS485_TYPE_MAS_TO_SLV_TILT_LAMP:  // Master command to turn Tilt lamp on or off
      {
        bool tiltOn = false;
        pMessage->getMAStoSLVTiltLamp(&tiltOn);
        setTiltLamp(tiltOn);
    }
    break;

    default:
      setAllDevicesOff();
      // In processMessage() default branch:
      snprintf(lcdString, sizeof(lcdString), "MSG TYPE ERROR %u", (unsigned)t_msgType);
      pLCD2004->println(lcdString);
      while (true) { }  // Halt on error
  }
}

// --- MODIFY updateDeviceTimers(): assign rest based on rapid-fire classification ---
void updateDeviceTimers() {
  for (byte i = 0; i < NUM_DEVS; ++i) {
    int8_t& ct = deviceParm[i].countdown;

    if (ct > 0) {              // Active phase
      ct--;
      if (ct == 0) {
        // End active: either go to hold (unused here) or turn off then rest.
        if (deviceParm[i].powerHold > 0) {
          analogWrite(deviceParm[i].pinNum, deviceParm[i].powerHold);
          // Hold coils (none on Slave) would stay with countdown==0.
        }
        else {
          analogWrite(deviceParm[i].pinNum, LOW);
          if (deviceParm[i].timeOn > 0) {
            // Enter rest: rapid-fire coils use RAPID_REST_TICKS, others DEFAULT_REST_TICKS.
            ct = isRapidFireDevice(i) ? RAPID_REST_TICKS : DEFAULT_REST_TICKS;
          }
          else {
            ct = 0; // Non-pulse devices (lamps)
          }
        }
      }
      continue;
    }

    if (ct < 0) {              // Rest phase
      ct++;
      if (ct == 0 && deviceParm[i].queueCount > 0) {
        if (canActivateDeviceNow(i)) {
          deviceParm[i].queueCount--;
          analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
          deviceParm[i].countdown = deviceParm[i].timeOn;
        }
        else if (i == DEV_IDX_CREDIT_UP) {
          snprintf(lcdString, sizeof(lcdString), "Credit FULL discard %u", deviceParm[i].queueCount);
          pLCD2004->println(lcdString);
          deviceParm[i].queueCount = 0;
          pMessage->sendSLVtoMASCreditStatus(hasCredits());
        }
      }
      continue;
    }

    // Idle (ct == 0)
    if (deviceParm[i].queueCount > 0) {
      if (canActivateDeviceNow(i)) {
        deviceParm[i].queueCount--;
        analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
        deviceParm[i].countdown = deviceParm[i].timeOn;
      }
      else if (i == DEV_IDX_CREDIT_UP) {
        snprintf(lcdString, sizeof(lcdString), "Credit FULL discard %u", deviceParm[i].queueCount);
        pLCD2004->println(lcdString);
        deviceParm[i].queueCount = 0;
        pMessage->sendSLVtoMASCreditStatus(hasCredits());
      }
    }
  }
}

// REPLACE entire requestScoreAdjust() with:
void requestScoreAdjust(int delta10Ks) {
  if (delta10Ks == 0) return;
  if ((delta10Ks > 0 && targetScore >= 999) || (delta10Ks < 0 && targetScore <= 0)) return;

  if (delta10Ks > 999)  delta10Ks = 999;
  if (delta10Ks < -999) delta10Ks = -999;

  if (delta10Ks > 0 && targetScore + delta10Ks > 999) delta10Ks = 999 - targetScore;
  if (delta10Ks < 0 && targetScore + delta10Ks < 0)   delta10Ks = -targetScore;
  if (delta10Ks == 0) return;

  byte nextTail = (byte)((scoreCmdTail + 1) % (sizeof(scoreCmdQueue) / sizeof(scoreCmdQueue[0])));
  if (nextTail == scoreCmdHead) {
    snprintf(lcdString, sizeof(lcdString), "Score Q OVERFLOW %d", delta10Ks);
    pLCD2004->println(lcdString);
    while (true) { }
  }
  scoreCmdQueue[scoreCmdTail] = delta10Ks;
  scoreCmdTail = nextTail;

  targetScore += delta10Ks;
  if (targetScore < 0) targetScore = 0;
  if (targetScore > 999) targetScore = 999;

  if (!scoreAdjustActive) {
    int cmd = scoreCmdQueue[scoreCmdHead];
    scoreCmdHead = (byte)((scoreCmdHead + 1) % (sizeof(scoreCmdQueue) / sizeof(scoreCmdQueue[0])));
    activeDirection = (cmd > 0) ? 1 : -1;
    int magnitude = abs(cmd);

    if (magnitude % 10 == 0) {
      batchHundredsRemaining = magnitude / 10;
      batchTensRemaining     = 0;
    }
    else {
      batchHundredsRemaining = 0;
      batchTensRemaining     = magnitude;
    }

    scoreAdjustMotorMode  = false;
    scoreCycleHundred     = false;
    cycleActionLimit      = 0;
    scoreAdjustCycleIndex = 0;

    // Decide initial mode and persistence
    motorPersistent = false;
    if (batchHundredsRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = true;
      cycleActionLimit     = (byte)min(batchHundredsRemaining, 5);
      motorPersistent      = true;
      lastCommandWasMotor  = true;
      motorCycleStartMs    = millis();
    }
    else if (batchHundredsRemaining == 1) {
      // single 100K (non-motor)
      lastCommandWasMotor  = false;
      motorCycleStartMs    = millis();
    }
    else if (batchTensRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = false;
      cycleActionLimit     = (byte)min(batchTensRemaining, 5);
      motorPersistent      = true;
      lastCommandWasMotor  = true;
      motorCycleStartMs    = millis();
    }
    else { // single 10K
      lastCommandWasMotor  = false;
      motorCycleStartMs    = millis();
    }

    scoreAdjustActive = true;
    scoreAdjustLastMs = millis() - SCORE_MOTOR_STEP_MS;
  }
}

// -------- FINISH / MERGE NEXT BATCH (FIXED) --------
// REPLACE scoreAdjustFinishIfDone() with this version:
static void scoreAdjustFinishIfDone() {
  // Batch not done yet
  if (batchHundredsRemaining != 0 || batchTensRemaining != 0) return;
  // Should never finish while still flagged motor mode
  if (scoreAdjustMotorMode) return;

  unsigned long now = millis();
  // Motor-based command: full cycle rest; single pulse: one step rest
  unsigned requiredMs = lastCommandWasMotor ? SCORE_MOTOR_CYCLE_MS : SCORE_MOTOR_STEP_MS;
  if (now - motorCycleStartMs < requiredMs) {
    return;  // Still resting
  }

  // Start next queued command if any
  if (scoreCmdHead != scoreCmdTail) {
    int cmd = scoreCmdQueue[scoreCmdHead];
    scoreCmdHead = (byte)((scoreCmdHead + 1) % (sizeof(scoreCmdQueue) / sizeof(scoreCmdQueue[0])));
    activeDirection = (cmd > 0) ? 1 : -1;
    int magnitude = abs(cmd);

    if (magnitude % 10 == 0) {
      batchHundredsRemaining = magnitude / 10;
      batchTensRemaining     = 0;
    }
    else {
      batchHundredsRemaining = 0;
      batchTensRemaining     = magnitude;
    }

    scoreAdjustMotorMode  = false;
    scoreCycleHundred     = false;
    cycleActionLimit      = 0;
    scoreAdjustCycleIndex = 0;

    motorPersistent       = false;
    if (batchHundredsRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = true;
      cycleActionLimit     = (byte)min(batchHundredsRemaining, 5);
      motorPersistent      = true;
      lastCommandWasMotor  = true;
      motorCycleStartMs    = millis();
    }
    else if (batchHundredsRemaining == 1) {
      lastCommandWasMotor  = false;
      motorCycleStartMs    = millis();
    }
    else if (batchTensRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = false;
      cycleActionLimit     = (byte)min(batchTensRemaining, 5);
      motorPersistent      = true;
      lastCommandWasMotor  = true;
      motorCycleStartMs    = millis();
    }
    else { // single 10K
      lastCommandWasMotor  = false;
      motorCycleStartMs    = millis();
    }

    scoreAdjustActive = true;
    scoreAdjustLastMs = millis() - SCORE_MOTOR_STEP_MS; // allow immediate first advance
    return;
  }

  // Queue empty: finalize
  scoreAdjustActive     = false;
  scoreAdjustMotorMode  = false;
  scoreCycleHundred     = false;
  activeDirection       = 0;
  scoreAdjustCycleIndex = 0;
  saveScoreToEEPROM(currentScore);
  snprintf(lcdString, sizeof(lcdString), "Score now %u", (unsigned)currentScore);
  pLCD2004->println(lcdString);
  Serial.println(lcdString);
}

// -------- PROCESS SCORE ADJUST (UPDATED) --------
void processScoreAdjust() {
  if (!scoreAdjustActive || resetActive) return;

  if (batchHundredsRemaining == 0 && batchTensRemaining == 0 && !scoreAdjustMotorMode) {
    scoreAdjustFinishIfDone();
    return;
  }

  unsigned long now = millis();
  if (now - scoreAdjustLastMs < SCORE_MOTOR_STEP_MS) return;
  if (deviceParm[DEV_IDX_10K_UP].countdown != 0) return;

  scoreAdjustLastMs = now;

  // Non-motor singles
  if (!scoreAdjustMotorMode) {
    if (batchHundredsRemaining == 1) {
      int nextScore = constrain(currentScore + activeDirection * 10, 0, 999);
      if (nextScore != currentScore) {
        activateDevice(DEV_IDX_10K_UP);
        activateDevice(DEV_IDX_10K_BELL);
        activateDevice(DEV_IDX_100K_BELL);
        currentScore = nextScore;
        displayScore(currentScore);
      }
      batchHundredsRemaining = 0;
      lastCommandWasMotor = false;
      motorCycleStartMs   = millis();  // START rest window for single 100K
    }
    else if (batchHundredsRemaining == 0 && batchTensRemaining == 1) {
      int nextScore = constrain(currentScore + activeDirection, 0, 999);
      if (nextScore != currentScore) {
        activateDevice(DEV_IDX_10K_UP);
        activateDevice(DEV_IDX_10K_BELL);
        if ((nextScore % 10) == 0 || (activeDirection < 0 && (currentScore % 10) == 0)) {
          activateDevice(DEV_IDX_100K_BELL);
        }
        currentScore = nextScore;
        displayScore(currentScore);
      }
      batchTensRemaining = 0;
      lastCommandWasMotor = false;
      motorCycleStartMs   = millis();  // START rest window for single 10K
    }
    // Transition to motor cycles if more work
    if (batchHundredsRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = true;
      cycleActionLimit     = (byte)min(batchHundredsRemaining, 5);
      scoreAdjustCycleIndex = 0;
    }
    else if (batchHundredsRemaining == 0 && batchTensRemaining > 1) {
      scoreAdjustMotorMode = true;
      scoreCycleHundred    = false;
      cycleActionLimit     = (byte)min(batchTensRemaining, 5);
      scoreAdjustCycleIndex = 0;
    }
    else if (batchHundredsRemaining == 0 && batchTensRemaining == 0) {
      scoreAdjustFinishIfDone();
    }
    return;
  }

  // Motor cycle: 6 sub-steps (0..5): first 'cycleActionLimit' are advances; remainder are rests.
  if (scoreAdjustCycleIndex < 6) {
    if (scoreAdjustCycleIndex < cycleActionLimit) {
      int stepUnits = scoreCycleHundred ? 10 : 1;
      int nextScore = constrain(currentScore + activeDirection * stepUnits, 0, 999);

      if (nextScore != currentScore) {
        activateDevice(DEV_IDX_10K_UP);
        activateDevice(DEV_IDX_10K_BELL);
        if (scoreCycleHundred ||
          (nextScore % 10) == 0 ||
          (activeDirection < 0 && (currentScore % 10) == 0)) {
          activateDevice(DEV_IDX_100K_BELL);
        }
        currentScore = nextScore;
        displayScore(currentScore);

        if (scoreCycleHundred) batchHundredsRemaining--;
        else batchTensRemaining--;
      }
      else {
        // Saturated mid-category
        if (scoreCycleHundred) batchHundredsRemaining = 0;
        else batchTensRemaining = 0;
      }
    }
  }

  // In processScoreAdjust(), REPLACE the cycle completion block (inside motor mode) with:
    // At end of a sub-step
  scoreAdjustCycleIndex++;
  if (scoreAdjustCycleIndex >= 6) {
    // Full cycle elapsed (advances + rests)
    scoreAdjustCycleIndex = 0;

    if (scoreCycleHundred) {
      if (batchHundredsRemaining > 0) { // still work
        if (batchHundredsRemaining > 5) {
          cycleActionLimit = 5;
        }
        else {
          cycleActionLimit = (byte)batchHundredsRemaining; // may be 1
        }
        // Stay motor mode even if remainder == 1 if motorPersistent set
        if (batchHundredsRemaining == 1 && !motorPersistent) {
          // drop to single non-motor
          scoreAdjustMotorMode = false;
          lastCommandWasMotor  = false;
          motorCycleStartMs    = millis();
        }
        else {
          // continue motor cycle
          scoreAdjustMotorMode = true;
          lastCommandWasMotor  = true;
          motorCycleStartMs    = millis();
        }
      }
      else { // hundreds done
        if (batchTensRemaining > 0) {
          scoreCycleHundred = false;
          if (batchTensRemaining > 5) {
            cycleActionLimit = 5;
          }
          else {
            cycleActionLimit = (byte)batchTensRemaining;
          }
          if (batchTensRemaining == 1 && !motorPersistent) {
            // single tens, non-motor
            scoreAdjustMotorMode = false;
            lastCommandWasMotor  = false;
            motorCycleStartMs    = millis();
          }
          else {
            // If command originated as tens multi-unit (motorPersistent) keep motor mode even for final single
            if (batchTensRemaining > 0) {
              scoreAdjustMotorMode = true;
              lastCommandWasMotor  = true;
              motorCycleStartMs    = millis();
            }
            else {
              scoreAdjustMotorMode = false;
              lastCommandWasMotor  = motorPersistent; // finishing last motor cycle
            }
          }
        }
        else {
          // No tens; command complete after this cycle
          scoreAdjustMotorMode = false;
        }
      }
    }
    else { // tens cycles
      if (batchTensRemaining > 0) {
        if (batchTensRemaining > 5) {
          cycleActionLimit = 5;
        }
        else {
          cycleActionLimit = (byte)batchTensRemaining;
        }
        if (batchTensRemaining == 1 && !motorPersistent) {
          // single non-motor
          scoreAdjustMotorMode = false;
          lastCommandWasMotor  = false;
          motorCycleStartMs    = millis();
        }
        else {
          // continue motor cycle (even if remainder 1) if persistent
          scoreAdjustMotorMode = true;
          lastCommandWasMotor  = true;
          motorCycleStartMs    = millis();
        }
      }
      else {
        scoreAdjustMotorMode = false;
      }
    }
  }

  // If all work consumed and not in motor mode, attempt finish
  if (batchHundredsRemaining == 0 && batchTensRemaining == 0 && !scoreAdjustMotorMode) {
    scoreAdjustFinishIfDone();
  }
}

// ====== END SCORE ADJUST BLOCK ======

// ---- Restored helper function definitions ----

void setScoreLampBrightness(byte t_brightness) {  // 0..255
  analogWrite(deviceParm[DEV_IDX_LAMP_SCORE].pinNum, t_brightness);
}

void setGITiltLampBrightness(byte t_brightness) {  // 0..255
  analogWrite(deviceParm[DEV_IDX_LAMP_HEAD_GI_TILT].pinNum, t_brightness);
}

void setGILamp(bool t_on) {
  // Centipede output is active LOW: lamp on => LOW
  pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_HEAD_GI, t_on ? LOW : HIGH);
}

void setTiltLamp(bool t_on) {
  // Centipede output is active LOW: lamp on => LOW
  pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_TILT, t_on ? LOW : HIGH);
}

int recallScoreFromEEPROM() {
  // Read persisted score from EEPROM. Returns clamped 0..999. Out-of-range -> 0.
  uint16_t s = 0;
  EEPROM.get(EEPROM_ADDR_SCORE, s);
  if (s > 999) s = 0;
  return (int)s;
}

void saveScoreToEEPROM(int t_score) {
  // Persist score (0..999) only if changed to reduce EEPROM wear.
  if (t_score < 0) t_score = 0;
  if (t_score > 999) t_score = 999;
  uint16_t newVal = (uint16_t)t_score;
  uint16_t oldVal = 0;
  EEPROM.get(EEPROM_ADDR_SCORE, oldVal);
  if (oldVal != newVal) {
    EEPROM.put(EEPROM_ADDR_SCORE, newVal);
    sprintf(lcdString, "Saved score %d", t_score);
    pLCD2004->println(lcdString);
    Serial.println(lcdString);
  }
}

bool hasCredits() {
  // Returns true if credits remain (Credit Empty switch is closed/LOW)
  return digitalRead(PIN_IN_SWITCH_CREDIT_EMPTY) == LOW;
}

void startNewGame(byte t_mode) {  // Start a new game in the specified mode.
  // 1: Tilt, 2: Game Over, 3: Diagnostic, 4: Original, 5: Enhanced, 6: Impulse
  modeCurrent = t_mode;
  setTiltLamp(false);
  setGILamp(true);
  currentScore = 0;
  targetScore  = 0;
  // Flush queue/state
  scoreCmdHead = scoreCmdTail = 0;
  scoreAdjustActive = false;
  activeDirection = 0;
  displayScore(currentScore);
  return;
}

void displayScore(int t_score) {
  // Display score using Centipede shift register
  // t_score is 0..999 (i.e. displayed t_score / 10,000)
  // We only digitalWrite() lamps that changed state since last call.

  // Clamp t_score to 0..999
  if (t_score < 0) t_score = 0;
  if (t_score > 999) t_score = 999;

  static int lastScore = -1;

  int millions = t_score / 100;         // 1..9
  int hundredK = (t_score / 10) % 10;   // 1..9
  int tensK    = t_score % 10;          // 1..9

  int lastMillions = (lastScore < 0) ? 0 : lastScore / 100;
  int lastHundredK = (lastScore < 0) ? 0 : (lastScore / 10) % 10;
  int lastTensK    = (lastScore < 0) ? 0 : lastScore % 10;

  // Only update lamps that change
  if (lastMillions != millions) {
    if (lastMillions > 0) pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_1M[lastMillions - 1], HIGH); // turn off old
    if (millions > 0)     pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_1M[millions - 1], LOW);      // turn on new
  }
  if (lastHundredK != hundredK) {
    if (lastHundredK > 0) pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_100K[lastHundredK - 1], HIGH);
    if (hundredK > 0)     pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_100K[hundredK - 1], LOW);
  }
  if (lastTensK != tensK) {
    if (lastTensK > 0) pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_10K[lastTensK - 1], HIGH);
    if (tensK > 0)     pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_10K[tensK - 1], LOW);
  }

  lastScore = t_score;
}

void processScoreReset() {
  if (!resetActive) return;

  unsigned long now = millis();

  // Phase 0: Rapid high reversal (millions+hundredK only)
  if (resetPhase == SCORE_RESET_PHASE_REV_HIGH) {
    // Perform as many single 100K decrements as time allows WITHOUT skipping values.
    if (resetHighUnitsRemaining > 0) {
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

    // When all high units are cleared, wait until full motor cycle time has elapsed to mimic 1/4 rev.
    if (resetHighUnitsRemaining == 0 &&
        (now - resetHighCycleStartMs) >= SCORE_MOTOR_CYCLE_MS) {
      // Transition to tensK cycle(s)
      resetPhase = SCORE_RESET_PHASE_10K_CYCLE1;
      resetCycleIndex   = 0;
      resetLastMs       = now - SCORE_MOTOR_STEP_MS; // allow immediate first tensK sub-step
    }
    return;
  }

  // Shared pacing for tensK cycles (use motor sub-step timing 147ms)
  if ((now - resetLastMs) < SCORE_MOTOR_STEP_MS) return;
  resetLastMs = now;
  byte localIndex = resetCycleIndex;
  resetCycleIndex = (byte)((resetCycleIndex + 1) % SCORE_MOTOR_STEPS);

  // Phase 1 & 2: tensK advancement in batches of up to 5 (+rest)
  if (resetPhase == SCORE_RESET_PHASE_10K_CYCLE1 ||
      resetPhase == SCORE_RESET_PHASE_10K_CYCLE2) {

    bool secondCycle = (resetPhase == SCORE_RESET_PHASE_10K_CYCLE2);

    if (localIndex < (SCORE_MOTOR_STEPS - 1) && reset10KStepsRemaining > 0) {
      // Advance tensK ONLY; do NOT increment hundredK/millions during reset.
      int tensK    = currentScore % 10;
      int hundredK = (currentScore / 10) % 10;
      int millions = (currentScore / 100) % 10;

      int newTensK = (tensK + 1) % 10;

      // Suppress hundredK/million roll during reset: when newTensK == 0 we just show 0 tensK (lamps off for that digit)
      if (newTensK == 0) {
        // Ring 100K bell to emulate boundary sound, but do not change hundredK lamp group in reset context.
        activateDevice(DEV_IDX_100K_BELL);
      }

      activateDevice(DEV_IDX_10K_UP);
      activateDevice(DEV_IDX_10K_BELL);

      currentScore = millions * 100 + hundredK * 10 + newTensK;
      displayScore(currentScore);
      reset10KStepsRemaining--;
    }

    // After rest sub-step (cycleIndex == 0) decide next phase.
    if (resetCycleIndex == 0) {
      if (reset10KStepsRemaining == 0) {
        resetPhase = SCORE_RESET_PHASE_DONE;
      } else if (!secondCycle) {
        // Need a second tensK cycle if more than 5 steps remain.
        resetPhase = SCORE_RESET_PHASE_10K_CYCLE2;
      } else {
        // Second cycle completed but still remaining (shouldn't happen unless >10 requested)
        resetPhase = SCORE_RESET_PHASE_DONE;
      }
    }
    return;
  }

  // Phase done
  if (resetPhase == SCORE_RESET_PHASE_DONE) {
    resetActive = false;
    saveScoreToEEPROM(currentScore);
    pLCD2004->println("Reset done");
  }
}

// -------- PRUNE QUEUE AT BOUNDARY (UPDATED) --------
static void pruneQueuedAtBoundary(int saturatedDir) {
  if (scoreCmdHead == scoreCmdTail) return;
  int tmp[8]; byte wt = 0; byte rd = scoreCmdHead;
  while (rd != scoreCmdTail) {
    int cmd = scoreCmdQueue[rd];
    if (((cmd > 0) ? 1 : -1) != saturatedDir) tmp[wt++] = cmd;
    rd = (byte)((rd + 1) % (sizeof(scoreCmdQueue) / sizeof(scoreCmdQueue[0])));
  }
  scoreCmdHead = 0; scoreCmdTail = wt;
  for (byte i = 0; i < wt; i++) scoreCmdQueue[i] = tmp[i];
  if (scoreAdjustActive && activeDirection == saturatedDir) {
    // Terminate current batch
    batchHundredsRemaining = 0;
    batchTensRemaining     = 0;
    scoreAdjustMotorMode   = false;
    scoreAdjustActive      = false;
  }
}
