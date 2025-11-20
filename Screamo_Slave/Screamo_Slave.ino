// Screamo_Slave.INO Rev: 11/19/25
// Need code to save and recall previous score when machine was turned off or last game ended, so we can display it on power-up.
//   Maybe automatically save current score every x times through the 10ms loop; i.e. every 15 or 30 seconds.  Or just at game-over.
//   Maybe save to EEPROM every time the score changes, but that could wear out the EEPROM
// Need code to realistically reset score from previous score back to zero: advancing the 10K unit and "resetting" the 100K unit.

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>

#include <EEPROM.h>               // For saving and recalling score, to persist between power cycles.
const int EEPROM_ADDR_SCORE = 0;  // Address to store 16-bit score (uses addr 0 and 1)

const byte THIS_MODULE = ARDUINO_SLV;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SLAVE 11/19/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
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
  byte   timeOn;        // Number of 10ms loop ticks to hold initial power before changing to hold level or turning off
  byte   powerHold;     // 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
  int8_t countdown;     // Current countdown in 10ms ticks; -1..-8 = rest period, 0 = idle, >0 = active
  byte   queueCount;    // Number of pending activation requests while coil busy/resting
};

// *** CREATE AN ARRAY OF DeviceParm STRUCT FOR ALL COILS AND MOTORS ***
// Start by defining array index constants:
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
deviceParmStruct deviceParm[NUM_DEVS] = {
  {  5, 170, 4, 0, 0, 0 },  // CREDIT UP. 120 confirmed min power to work reliably; 30ms works but we'll match 40 needed by 10K bell coil.
  {  6, 200, 3, 0, 0, 0 },  // CREDIT DOWN.  150 confirmed min power to work reliably; 160ms confirmed is bare minimum time to work reliably.  140 no good.
  {  7, 160, 4, 0, 0, 0 },  // 10K UP  (~40ms)
  {  8, 140, 3, 0, 0, 0 },  // 10K BELL(~30ms)
  {  9, 120, 3, 0, 0, 0 },  // 100K BELL(~30ms)
  { 10, 120, 3, 0, 0, 0 },  // SELECT BELL
  { 11,  40, 0, 0, 0, 0 },  // LAMP SCORE.  PWM MOSFET.  Initial brightness for LED Score lamps (0..255)  40 is full bright @ 12vdc.
  { 12,  40, 0, 0, 0, 0 }   // LAMP HEAD G.I./TILT.  PWM MOSFET.  Initial brightness for LED G.I. and Tilt lamps (0..255)
};

// *** MISC CONSTANTS AND GLOBALS ***
byte modeCurrent               = MODE_UNDEFINED;
byte msgType                   = RS485_TYPE_NO_MESSAGE;  // Current RS485 message type being processed.

int currentScore               =  0;  // Current game score (0..999).
int targetScore                =  0;  // NEW: merged desired final score (0..999) updated by incoming RS485 score deltas.

unsigned long lastLoopTime     =  0;  // Used to track 10ms loop timing.
const int8_t REST_PERIOD_TICKS = -8;  // Number of 10ms ticks for coil rest period (i.e. 80ms)

bool debugOn    = false;

// ---------------- Score adjust timing constants (ms) ----------------
// Tune these to match the real mechanism timings (mechanical measurements can refine these).
const unsigned SCORE_10K_STEP_MS   = 100;  // spacing between single 10K steps within a batch
const unsigned SCORE_BATCH_GAP_MS  = 500;  // spacing between batches of up to 5x10K
const unsigned SCORE_100K_STEP_MS  = 120;  // spacing between successive 100K advances

// ---------------- Non-blocking score-adjust state (global) ----------------
// Phases: 0=idle, 1=single (alignment) 10K steps, 2=bulk 100K jumps, 3=batched 10K (groups of up to 5), 4=batch gap.
static int scoreAdjustDir              = 0;  // +1 or -1 toward targetScore
static int scoreAdjustPhase            = 0;  // current phase (see above)
static int scoreAdjustBatchRemaining   = 0;  // remaining 10K steps in current batch (phase 3)
static unsigned long scoreAdjustLastMs = 0;  // timestamp of last mechanical action

// Forward declarations
void requestScoreAdjust(int delta10Ks);
void processScoreAdjust();
static void scoreAdjustFinishIfDone();

// ---- Forward declarations for lamp / EEPROM helpers (restore) ----
void setScoreLampBrightness(byte t_brightness);      // 0..255
void setGITiltLampBrightness(byte t_brightness);     // 0..255
void setGILamp(bool t_on);
void setTiltLamp(bool t_on);
int  recallScoreFromEEPROM();
void saveScoreToEEPROM(int t_score);
bool hasCredits();
void startNewGame(byte t_mode);
void displayScore(int t_score);

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
  // Display previously saved score from EEPROM
  currentScore = recallScoreFromEEPROM();
  targetScore  = currentScore;  // initialize target

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // Insert a delay() in order to give the Digole 2004 LCD time to power up and be ready to receive commands (esp. the 115K speed command).
  delay(750);  // 500ms was occasionally not enough.
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  // Enforce 10ms tick (optional but recommended for deterministic timing)
  unsigned long now = millis();
  if (now - lastLoopTime < 10) return;  // Still within this 10ms window
  lastLoopTime = now;

  // Handle all pending RS-485 messages (non-blocking drain)
  while ((msgType = pMessage->available()) != RS485_TYPE_NO_MESSAGE) {
    processMessage(msgType);
  }

  // Update all active device timers (solenoids, bells, etc.)
  updateDeviceTimers();

  // Run non-blocking score adjustment state machine
  processScoreAdjust();

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
    // Prevent firing if credit wheel is full (protect mechanism)
    if (digitalRead(PIN_IN_SWITCH_CREDIT_FULL) == LOW) {
      return false; // full -> cannot activate
    }
  } else if (t_devIdx == DEV_IDX_CREDIT_DOWN) {
    // Prevent firing if no credits available (protect mechanism)
    if (!hasCredits()) {
      return false; // no credits -> cannot activate
    }
  }
  // Add other device-specific guards if needed
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

    case RS485_TYPE_MAS_TO_SLV_SCORE_RESET:
      pLCD2004->println("Score reset");
      currentScore = 0;
      targetScore  = 0;              // NEW: keep target in sync
      scoreAdjustPhase = 0;
      scoreAdjustBatchRemaining = 0;
      displayScore(currentScore);
      saveScoreToEEPROM(currentScore);
      break;

    case RS485_TYPE_MAS_TO_SLV_SCORE_ABS:
      {
        byte tensK = 0, hundredK = 0, millions = 0;
        pMessage->getMAStoSLVScoreAbs(&tensK, &hundredK, &millions);
        int newScore = (int)millions * 100 + (int)hundredK * 10 + (int)tensK;
        newScore = constrain(newScore, 0, 999);
        currentScore = newScore;
        targetScore  = newScore;     // NEW: synchronize target with absolute set
        scoreAdjustPhase = 0;
        scoreAdjustBatchRemaining = 0;
        snprintf(lcdString, sizeof(lcdString), "Score set %u", (unsigned)currentScore);
        pLCD2004->println(lcdString);
        displayScore(currentScore);
        saveScoreToEEPROM(currentScore);
      }
      break;

    case RS485_TYPE_MAS_TO_SLV_SCORE_INC:  // Increment score update (1..999 in 10,000s)
      {
        int incAmount = 0;
        pMessage->getMAStoSLVScoreInc(&incAmount);
        snprintf(lcdString, sizeof(lcdString), "Score inc %u", (unsigned)incAmount);
        pLCD2004->println(lcdString);
        requestScoreAdjust(incAmount);   // NEW: non-blocking mechanical update
      }
      break;

    case RS485_TYPE_MAS_TO_SLV_SCORE_DEC:  // Decrement score update (-999..-1 in 10,000s) (won't go below zero)
      {
        int decAmount = 0;
        pMessage->getMAStoSLVScoreDec(&decAmount);
        snprintf(lcdString, sizeof(lcdString), "Score dec %u", (unsigned)decAmount);
        pLCD2004->println(lcdString);
        requestScoreAdjust(-decAmount);  // NEW: non-blocking mechanical update
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
        // currentScore is 0..999 where each unit == 10,000
        byte tensK    = (byte)(currentScore % 10);          // 10K units (0..9)
        byte hundredK = (byte)((currentScore / 10) % 10);   // 100K units (0..9)
        byte millions = (byte)(currentScore / 100);         // 1M units (0..9)
        // Send three separate bytes: 10K, 100K, 1M (same ordering as getMAStoSLVScoreAbs)
        pMessage->sendSLVtoMASScoreReport(tensK, hundredK, millions);
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

void updateDeviceTimers() {
  // Call this once per 10ms tick to update timers and start queued activations safely.
  for (byte i = 0; i < NUM_DEVS; ++i) {
    int8_t& ct = deviceParm[i].countdown;
    if (ct > 0) {
      // Active phase
      ct--;
      if (ct == 0) {
        // Turn off or set hold level
        if (deviceParm[i].powerHold > 0) {
          analogWrite(deviceParm[i].pinNum, deviceParm[i].powerHold);
          // If you want a held state, decide how to represent it (here we treat hold as staying on)
        } else {
          analogWrite(deviceParm[i].pinNum, LOW);
        }
        // Enter rest period
        ct = REST_PERIOD_TICKS;  // i.e. -8 = 80ms rest
      }
      continue;
    }
    if (ct < 0) {
      // Rest phase: increment toward 0
      ct++;
      if (ct == 0) {
        // Rest finished, see if there is a queued request to start
        if (deviceParm[i].queueCount > 0) {
          // Safety check before starting
          if (canActivateDeviceNow(i)) {
            deviceParm[i].queueCount--;
            analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
            ct = deviceParm[i].timeOn; // start active countdown
          }
          else {
            // Unsafe to start (e.g. credit wheel full). Discard queued credit requests.
            if (i == DEV_IDX_CREDIT_UP) {
              // Notify / log and discard queue
              snprintf(lcdString, sizeof(lcdString), "Credit FULL, discarding %u", deviceParm[i].queueCount);
              pLCD2004->println(lcdString);
              deviceParm[i].queueCount = 0;
              // Optional: inform Master explicitly
              pMessage->sendSLVtoMASCreditStatus(hasCredits());
            }
            // For other devices you may choose to keep queueCount or drop it.
          }
        }
      }
      continue;
    }
    // ct == 0 (idle) - if there is queued activation and we are idle, try to start immediately
    if (deviceParm[i].queueCount > 0) {
      if (canActivateDeviceNow(i)) {
        deviceParm[i].queueCount--;
        analogWrite(deviceParm[i].pinNum, deviceParm[i].powerInitial);
        deviceParm[i].countdown = deviceParm[i].timeOn;
      }
      else {
        if (i == DEV_IDX_CREDIT_UP) {
          // Same handling as above
          snprintf(lcdString, sizeof(lcdString), "Credit FULL, discarding %u", deviceParm[i].queueCount);
          pLCD2004->println(lcdString);
          deviceParm[i].queueCount = 0;
          pMessage->sendSLVtoMASCreditStatus(hasCredits());
        }
      }
    }
  }
}

// ====== ADD (OR REPLACE OLD VERSION OF) requestScoreAdjust() AND HELPERS ======
// Queue a requested delta (in 10K units). Merged into targetScore so bursts accumulate.
// Existing comments about mechanical behavior elsewhere remain valid.
void requestScoreAdjust(int delta10Ks) {
  if (delta10Ks == 0) return;
  targetScore += delta10Ks;              // NEW: accumulate relative to prior target
  if (targetScore < 0)   targetScore = 0;
  if (targetScore > 999) targetScore = 999;

  // Start state machine if idle
  if (scoreAdjustPhase == 0 && targetScore != currentScore) {
    scoreAdjustDir            = (targetScore > currentScore) ? 1 : -1;
    scoreAdjustPhase          = 1;   // begin with alignment single steps
    scoreAdjustBatchRemaining = 0;
    scoreAdjustLastMs         = millis() - SCORE_10K_STEP_MS; // allow immediate first step
  }
  // If already running and direction changed mid-flight, restart alignment phase
  else if (scoreAdjustPhase != 0) {
    int dir = (targetScore > currentScore) ? 1 : -1;
    if (dir != scoreAdjustDir) {
      scoreAdjustDir            = dir;
      scoreAdjustPhase          = 1;
      scoreAdjustBatchRemaining = 0;
      scoreAdjustLastMs         = millis() - SCORE_10K_STEP_MS;
    }
  }
}

// Persist score when adjust sequence completes; leave bell logic elsewhere untouched.
static void scoreAdjustFinishIfDone() {
  if (currentScore == targetScore) {
    saveScoreToEEPROM(currentScore);
    snprintf(lcdString, sizeof(lcdString), "Score now %u", (unsigned)currentScore);
    pLCD2004->println(lcdString);
    Serial.println(lcdString);
    scoreAdjustPhase = 0;
  }
}

// Call every 10ms tick after updateDeviceTimers(). Non-blocking mechanical score stepping.
void processScoreAdjust() {
  if (scoreAdjustPhase == 0) return;

  unsigned long now = millis();
  int diff = targetScore - currentScore;
  if (diff == 0) { scoreAdjustFinishIfDone(); return; }

  int remaining = abs(diff);
  int dir       = (diff > 0) ? 1 : -1;
  bool aligned100K = (currentScore % 10) == 0;

  // Direction change mid-flight (incoming decrements after increments or vice versa)
  if (dir != scoreAdjustDir) {
    scoreAdjustDir            = dir;
    scoreAdjustPhase          = 1;
    scoreAdjustBatchRemaining = 0;
    scoreAdjustLastMs         = now - SCORE_10K_STEP_MS;
  }

  // Auto-promote to 100K jump phase if conditions allow
  if (aligned100K && remaining >= 10 && scoreAdjustPhase != 2) {
    scoreAdjustPhase  = 2;
    scoreAdjustLastMs = now - SCORE_100K_STEP_MS;
  }

  switch (scoreAdjustPhase) {

    case 1: { // alignment single 10K steps until we can do 100K jumps or need batches
      if (aligned100K && remaining >= 10) {
        scoreAdjustPhase  = 2;
        scoreAdjustLastMs = now - SCORE_100K_STEP_MS;
        break;
      }
      if (now - scoreAdjustLastMs >= SCORE_10K_STEP_MS) {
        int nextScore = currentScore + scoreAdjustDir;
        if (nextScore < 0) nextScore = 0;
        if (nextScore > 999) nextScore = 999;

        // Always pulse 10K unit and ring 10K bell; ring 100K bell if crossing boundary.
        activateDevice(DEV_IDX_10K_UP);
        bool boundary = (nextScore % 10) == 0;
        activateDevice(DEV_IDX_10K_BELL);
        if (boundary) activateDevice(DEV_IDX_100K_BELL);

        currentScore = nextScore;
        displayScore(currentScore);
        scoreAdjustLastMs = now;

        int newRem = abs(targetScore - currentScore);
        if (newRem == 0) {
          scoreAdjustFinishIfDone();
        } else if ((currentScore % 10) == 0 && newRem >= 10) {
          scoreAdjustPhase  = 2;
          scoreAdjustLastMs = now - SCORE_100K_STEP_MS;
        } else if (newRem < 10) {
          scoreAdjustPhase          = 3;
          scoreAdjustBatchRemaining = 0;
          scoreAdjustLastMs         = now - SCORE_10K_STEP_MS;
        }
      }
    } break;

    case 2: { // bulk 100K jumps (advance by 10 units of 10K)
      if (!(aligned100K && remaining >= 10)) {
        // Conditions changed (new target shrank or lost alignment)
        scoreAdjustPhase          = (remaining < 10) ? 3 : 1;
        scoreAdjustBatchRemaining = 0;
        scoreAdjustLastMs         = now - SCORE_10K_STEP_MS;
        break;
      }
      if (now - scoreAdjustLastMs >= SCORE_100K_STEP_MS) {
        activateDevice(DEV_IDX_100K_BELL);
        activateDevice(DEV_IDX_10K_UP); // sound substitute for missing physical 100K unit coil
        currentScore += (scoreAdjustDir > 0) ? 10 : -10;
        if (currentScore < 0) currentScore = 0;
        if (currentScore > 999) currentScore = 999;
        displayScore(currentScore);
        scoreAdjustLastMs = now;

        int newRem = abs(targetScore - currentScore);
        bool aligned2 = (currentScore % 10) == 0;
        if (newRem == 0) {
          scoreAdjustFinishIfDone();
        } else if (!(aligned2 && newRem >= 10)) {
          scoreAdjustPhase          = (newRem < 10) ? 3 : 1;
          scoreAdjustBatchRemaining = 0;
          scoreAdjustLastMs         = now - SCORE_10K_STEP_MS;
        }
      }
    } break;

    case 3: { // batched remaining 10K steps (up to 5 then gap)
      if (remaining == 0) { scoreAdjustFinishIfDone(); break; }
      if (aligned100K && remaining >= 10) {
        scoreAdjustPhase  = 2;
        scoreAdjustLastMs = now - SCORE_100K_STEP_MS;
        break;
      }
      if (scoreAdjustBatchRemaining == 0) {
        scoreAdjustBatchRemaining = (remaining > 5) ? 5 : remaining;
        scoreAdjustLastMs         = now - SCORE_10K_STEP_MS;
      }
      if (now - scoreAdjustLastMs >= SCORE_10K_STEP_MS) {
        int nextScore = currentScore + scoreAdjustDir;
        if (nextScore < 0) nextScore = 0;
        if (nextScore > 999) nextScore = 999;

        activateDevice(DEV_IDX_10K_UP);
        bool boundary = (nextScore % 10) == 0;
        activateDevice(DEV_IDX_10K_BELL);
        if (boundary) activateDevice(DEV_IDX_100K_BELL);

        currentScore = nextScore;
        displayScore(currentScore);
        scoreAdjustBatchRemaining--;
        scoreAdjustLastMs = now;

        int newRem = abs(targetScore - currentScore);
        if (newRem == 0) {
          scoreAdjustFinishIfDone();
        } else if (scoreAdjustBatchRemaining == 0) {
          scoreAdjustPhase  = 4;
          scoreAdjustLastMs = now;
        }
      }
    } break;

    case 4: { // inter-batch gap
      if (remaining == 0) { scoreAdjustFinishIfDone(); break; }
      if (aligned100K && remaining >= 10) {
        scoreAdjustPhase  = 2;
        scoreAdjustLastMs = now - SCORE_100K_STEP_MS;
        break;
      }
      if (now - scoreAdjustLastMs >= SCORE_BATCH_GAP_MS) {
        scoreAdjustPhase          = 3;
        scoreAdjustBatchRemaining = 0;
        scoreAdjustLastMs         = now - SCORE_10K_STEP_MS;
      }
    } break;

    default:
      scoreAdjustPhase = 0;
      break;
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
  sprintf(lcdString, "Recall score %d", s);  // (Optional: print after LCD init)
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
  modeCurrent = t_mode;
  // Turn off Tilt lamp
  setTiltLamp(false);
  // Turn on G.I. lamps
  setGILamp(true);
  // Reset score to zero and cancel any pending non-blocking adjustments
  currentScore = 0;
  targetScore  = 0;
  scoreAdjustPhase = 0;
  scoreAdjustBatchRemaining = 0;
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