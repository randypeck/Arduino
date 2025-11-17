// Screamo_Slave.INO Rev: 11/16/25
// Need code to save and recall previous score when machine was turned off or last game ended, so we can display it on power-up.
//   Maybe automatically save current score every x times through the 10ms loop; i.e. every 15 or 30 seconds.  Or just at game-over.
//   Maybe save to EEPROM every time the score changes, but that could wear out the EEPROM
// Need code to realistically reset score from previous score back to zero: advancing the 10K unit and "resetting" the 100K unit.
// 
// Messages handled:
//   'G' - Turn on or off G.I. lamps; pass TRUE to turn on, FALSE to turn off.
//   'T' - Turn on or off Tilt lamp; pass TRUE to turn on, FALSE to turn off.
//   'H' - Check if credits are available; return TRUE if so; else FALSE.
//   'C' - Add credit(s); passed number to add (usually 1) if credit wheel not already full.  Master should also fire Knocker coil.
//   'D' - Deduct one credit, if available. Return TRUE if there was one to deduct; else return FALSE.
//   'Z' - Reset score to zero (should display score at end of last game, even upon power-up)
//   'S' - Adjust score by n points (-999..999); i.e. pass 5 to add 5,000 points; pass -1 to deduct 1,000 points.  Returns new score.
//   'L' - Ring the 10K bell
//   'M' - Ring the 100K bell
//   'X' - Ring the Select bell
//   'P' - Pulse the 10K Up coil (for testing)
//   'N' - Start a new game; pass which version (1=Williams, 2=Gottlieb, 3=Advanced)
//   ' ' - No message waiting for us

// Define struct to store coil/motor power and time parameters.  Coils that may be held on include a hold strength parameter.
struct deviceParmStruct {
  byte pinNum;        // Arduino pin number for this coil/motor
  byte powerInitial;  // 0..255 PWM power level when first energized
  byte timeOn;        // ms to hold initial power level before changing to hold level or turning off
  byte powerHold;     // 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
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

// DeviceParm table: { pinNum, powerInitial, timeOn(ms), powerHold }
//   pinNum: Arduino pin number for this coil
//   Power 0..255 for MOSFET coils and lamps
//   Example usage:
//     analogWrite(deviceParm[DEV_IDX_CREDIT_UP].pinNum, deviceParm[DEV_IDX_CREDIT_UP].powerInitial);
//     delay(deviceParm[DEV_IDX_CREDIT_UP].timeOn);
//     analogWrite(deviceParm[DEV_IDX_CREDIT_UP].pinNum, deviceParm[DEV_IDX_CREDIT_UP].powerHold);
deviceParmStruct deviceParm[NUM_DEVS] = {
  {  5, 170,  40,   0 },  // CREDIT UP. 120 confirmed min power to work reliably; 30ms works but we'll match 40 needed by 10K bell coil.
  {  6, 200,  30,   0 },  // CREDIT DOWN.  150 confirmed min power to work reliably; 160ms confirmed is bare minimum time to work reliably.  140 no good.
  {  7, 160,  40,   0 },  // 10K UP.  Tim thinks 40ms sounds like it's happening too fast; try longer hold time to try to match score motor's behavior.
  {  8, 140,  30,   0 },  // 10K BELL.  140 plenty of power; 30ms confirmed via test okay.
  {  9, 120,  30,   0 },  // 100K BELL.  120 confirmed via test, loud enough;  30ms okay.
  { 10, 120,  30,   0 },  // SELECT BELL
  { 11,  40,   0,   0 },  // LAMP SCORE.  PWM MOSFET.  Initial brightness for LED Score lamps (0..255)  40 is actually as bright as they get @ 12vdc.
  { 12,  40,   0,   0 }   // LAMP HEAD G.I./TILT.  PWM MOSFET.  Initial brightness for LED G.I. and Tilt lamps (0..255)  40 is actually as bright as they get @ 12vdc.
};

// *********** NOW WE NEED A FUNCTION WE CAN CALL i.e. fireDevice(deviceIndex); that will use the above info. ***********
// For final code, we may want to make this non-blocking by using millis() timing instead of delay(), but for now delay() is simpler.

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>

#include <EEPROM.h>               // For saving and recalling score, to persist between power cycles.
const int EEPROM_ADDR_SCORE = 0;  // Address to store 16-bit score (uses addr 0 and 1)

const byte THIS_MODULE = ARDUINO_SLV;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SLAVE 11/16/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
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

// *** MISC CONSTANTS AND GLOBALS ***
byte         modeCurrent      = MODE_UNDEFINED;
byte         stateCurrent     = STATE_UNDEFINED;
char         msgType          = ' ';

int currentScore = 0; // Current game score (0..999).

bool debugOn    = false;

// *** SWITCH STATE TABLE: Arrays contain 4 elements (unsigned ints) of 16 bits each = 64 bits = 1 Centipede
unsigned int switchOldState[] = { 65535, 65535, 65535, 65535 };
unsigned int switchNewState[] = { 65535, 65535, 65535, 65535 };

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  initScreamoSlaveArduinoPins();  // Arduino direct I/O pins only.

  // Set initial state for all MOSFET PWM output pins (pins 5-12)
  for (int i = 0; i < NUM_DEVS; i++) {
    digitalWrite(deviceParm[i].pinNum, LOW);  // Ensure all MOSFET outputs are off at startup.
    pinMode(deviceParm[i].pinNum, OUTPUT);
  }
  // Increase PWM frequency for pins 11 and 12 (Timer 1) to reduce LED flicker
  // Pins 11 and 12 are Score lamp and G.I./Tilt lamp brightness control
  // Set Timer 1 prescaler to 1 for ~31k
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;

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
  //setGILamp(true);
  setGITiltLampBrightness(deviceParm[DEV_IDX_LAMP_HEAD_GI_TILT].powerInitial);  // Ready to turn on as soon as relay contacts close.
  //setTiltLamp(false);
  // Display previously saved score from EEPROM
  //displayScore(recallScoreFromEEPROM());

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // Insert a delay() in order to give the Digole 2004 LCD time to power up and be ready to receive commands (esp. the 115K speed command).
  delay(750);  // 500ms was occasionally not enough.
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);
  pLCD2004->println("Setup starting.");

  addCredit();  // Test pulse to credit up coil
  delay(1000);
  removeCredit();  // Test pulse to credit down coil
  delay(1000);

/*



  for (int i = 0; i < 3; i++) {
    analogWrite(deviceParm[DEV_IDX_10K_UP].pinNum, deviceParm[DEV_IDX_10K_UP].powerInitial); delay(deviceParm[DEV_IDX_10K_UP].timeOn); analogWrite(deviceParm[DEV_IDX_10K_UP].pinNum, 0);
    delay(250);
  }
  delay(1000);
  analogWrite(deviceParm[DEV_IDX_10K_BELL].pinNum, deviceParm[DEV_IDX_10K_BELL].powerInitial); delay(deviceParm[DEV_IDX_10K_BELL].timeOn); analogWrite(deviceParm[DEV_IDX_10K_BELL].pinNum, 0);
  delay(1000);
  analogWrite(deviceParm[DEV_IDX_100K_BELL].pinNum, deviceParm[DEV_IDX_100K_BELL].powerInitial); delay(deviceParm[DEV_IDX_100K_BELL].timeOn); analogWrite(deviceParm[DEV_IDX_100K_BELL].pinNum, 0);
  delay(1000);
  analogWrite(deviceParm[DEV_IDX_SELECT_BELL].pinNum, deviceParm[DEV_IDX_SELECT_BELL].powerInitial); delay(deviceParm[DEV_IDX_SELECT_BELL].timeOn); analogWrite(deviceParm[DEV_IDX_SELECT_BELL].pinNum, 0);
  delay(1000);

  for (int i = 0; i <= 100; i++) {  // Count score from 0 to 9,990,000
    displayScore(i);
    if (i <= 100) {
      analogWrite(deviceParm[DEV_IDX_10K_UP].pinNum, deviceParm[DEV_IDX_10K_UP].powerInitial);
      analogWrite(deviceParm[DEV_IDX_10K_BELL].pinNum, deviceParm[DEV_IDX_10K_BELL].powerInitial);
      // If the score is a multiple of 100,000, ring the 100K bell
      if (i % 10 == 0) { // Each increment is 10,000, so every 10 is 100,000
        analogWrite(deviceParm[DEV_IDX_100K_BELL].pinNum, deviceParm[DEV_IDX_100K_BELL].powerInitial);
      }
    }
    // If the score is a multiple of 1,000,000, ring the Select bell
    if (i % 100 == 0) { // Each increment is 10,000, so every 100 is 1,000,000
      analogWrite(deviceParm[DEV_IDX_SELECT_BELL].pinNum, deviceParm[DEV_IDX_SELECT_BELL].powerInitial);
    }
    delay(deviceParm[DEV_IDX_10K_BELL].timeOn);  // Just use same delay for all for this test
    analogWrite(deviceParm[DEV_IDX_10K_UP].pinNum,      0);  // Turn off 10K up coil
    analogWrite(deviceParm[DEV_IDX_10K_BELL].pinNum,    0);  // Turn off 10K bell coil
    analogWrite(deviceParm[DEV_IDX_100K_BELL].pinNum,   0);  // Turn off 100K bell coil
    analogWrite(deviceParm[DEV_IDX_SELECT_BELL].pinNum, 0);  // Turn off Select bell coil

    delay(100);
    // If the score is a multipple of 5,000, pause for 500ms
    if (i % 5 == 0) { // Each increment is 10,000, so every 5 is 50,000
      delay(500);
    }
  }
  setGILamp(false);
  setTiltLamp(true);

  pLCD2004->println("Setup done.");
  Serial.println("Setup done.");

  currentScore = recallScoreFromEEPROM();
  displayScore(currentScore);

  // Wait for player to start a game and determine which version they want to play...
  pLCD2004->println("Waiting for Msgs");
  Serial.println("Waiting for Msgs");
  // Message will come from Master when it's time to start a new game, and parm will be which version.
  // We might also get messages to add or remove credits while waiting.
  // 1) Normal game with Williams impulse flipper action (player presses Start along with left flipper button.)
  // 2) Normal game but with Gottlieb flipper action (player presses Start and no flipper button.)
  // 3) Advanced game (with audio, shaker motor, etc.) (player double-presses Start within 1 second.)
  // Pressing Start again, after at least 1 second, adds a player to the game.
  // We will set modeCurrent = MODE_WILLIAMS, MODE_GOTTLEIB, or MODE_ADVANCED accordingly.




  while (true) {}
*/
}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  // See if there is an incoming message for us...
  byte msgType = pMessage->available();

  // For any message with parms, we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.
  // But for messages that don't have parms, we can just act on the message type immediately.

  // const byte RS485_TYPE_MAS_TO_SLV_MODE_STATE    =  1;  // Mode and/or State change
  // const byte RS485_TYPE_MAS_TO_SLV_NEW_GAME      =  2;  // Start a new game (tilt off, GI on, revert score zero; does not deduct credit)
  // const byte RS485_TYPE_MAS_TO_SLV_CREDIT_STATUS =  3;  // Request if credits > zero
  // const byte RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS =  4;  // Slave response to credit status request: credits zero or > zero
  // const byte RS485_TYPE_MAS_TO_SLV_CREDIT_INC    =  5;  // Credit increment
  // const byte RS485_TYPE_MAS_TO_SLV_CREDIT_DEC    =  6;  // Credit decrement (will not return error even if credits already zero)
  // const byte RS485_TYPE_MAS_TO_SLV_SCORE_RESET   =  7;  // Reset score to zero
  // const byte RS485_TYPE_MAS_TO_SLV_SCORE_ABS     =  8;  // Absolute score update (0.999 in 10,000s)
  // const byte RS485_TYPE_MAS_TO_SLV_SCORE_INC     =  9;  // Increment score update (1..999in 10,000s)
  // const byte RS485_TYPE_MAS_TO_SLV_SCORE_DEC     = 10;  // Decrement score update (-999..-1 in 10,000s) (won't go below zero)
  // const byte RS485_TYPE_MAS_TO_SLV_BELL_10K      = 11;  // Ring the 10K bell
  // const byte RS485_TYPE_MAS_TO_SLV_BELL_100K     = 12;  // Ring the 100K bell
  // const byte RS485_TYPE_MAS_TO_SLV_BELL_SELECT   = 13;  // Ring the Select bell
  // const byte RS485_TYPE_MAS_TO_SLV_10K_UNIT      = 14;  // Pulse the 10K Unit coil (for testing)
  // const byte RS485_TYPE_MAS_TO_SLV_SCORE_QUERY   = 15;  // Master requesting current score displayed by Slave (in 10,000s)
  // const byte RS485_TYPE_SLV_TO_MAS_SCORE_REPORT  = 16;  // Slave reporting current score (in 10,000s)
  // const byte RS485_TYPE_MAS_TO_SLV_GI_LAMP       = 17;  // Master command to turn G.I. lamps on or off
  // const byte RS485_TYPE_MAS_TO_SLV_TILT_LAMP     = 18;  // Master command to turn Tilt lamp on or off

  // Process all available incoming messages (non-blocking)
  while (msgType != RS485_TYPE_NO_MESSAGE) {
    switch (msgType) {
      case RS485_TYPE_MAS_TO_SLV_CREDIT_STATUS:
        pLCD2004->println("Credit status req.");
        pMessage->sendSLVtoMASCreditStatus(hasCredits());
        break;
      case RS485_TYPE_MAS_TO_SLV_CREDIT_INC:
        pLCD2004->println("Credit inc.");
        {
          byte numCreditsToAdd = 0;
          pMessage->getMAStoSLVCreditInc(&numCreditsToAdd);
          for (byte i = 0; i < numCreditsToAdd; i++) {
            addCredit();
            delay(250);  // Small delay between credits 
          }
        }
        break;
      case RS485_TYPE_MAS_TO_SLV_BELL_10K:
        pLCD2004->println("10K Bell");
        analogWrite(deviceParm[DEV_IDX_10K_BELL].pinNum, deviceParm[DEV_IDX_10K_BELL].powerInitial);
        delay(deviceParm[DEV_IDX_10K_BELL].timeOn);
        analogWrite(deviceParm[DEV_IDX_10K_BELL].pinNum, 0);
        break;
      case RS485_TYPE_MAS_TO_SLV_BELL_100K:
        pLCD2004->println("100K Bell");
        analogWrite(deviceParm[DEV_IDX_100K_BELL].pinNum, deviceParm[DEV_IDX_100K_BELL].powerInitial);
        delay(deviceParm[DEV_IDX_100K_BELL].timeOn);
        analogWrite(deviceParm[DEV_IDX_100K_BELL].pinNum, 0);
        break;
      case RS485_TYPE_MAS_TO_SLV_BELL_SELECT:
        pLCD2004->println("Select Bell");
        analogWrite(deviceParm[DEV_IDX_SELECT_BELL].pinNum, deviceParm[DEV_IDX_SELECT_BELL].powerInitial);
        delay(deviceParm[DEV_IDX_SELECT_BELL].timeOn);
        analogWrite(deviceParm[DEV_IDX_SELECT_BELL].pinNum, 0);
        break;
      case RS485_TYPE_MAS_TO_SLV_GI_LAMP:
        pLCD2004->println("G.I. ON/OFF");
        {
          bool giOn = false;
          pMessage->getMAStoSLVGILamp(&giOn);
          setGILamp(giOn);
        }
        break;
      default:
        sprintf(lcdString, "MSG TYPE ERROR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString);
      }
    // Read next message (non-blocking). This lets the loop exit when no more complete messages are available.
    msgType = pMessage->available();
  }
  delay(100);

//  msgType = pMessage->available();

  // We have handled any incoming message.


}  // End of loop()

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

void setScoreLampBrightness(byte t_brightness) {  // 0..255
  analogWrite(deviceParm[DEV_IDX_LAMP_SCORE].pinNum, t_brightness);
}

void setGITiltLampBrightness(byte t_brightness) {  // 0..255
  analogWrite(deviceParm[DEV_IDX_LAMP_HEAD_GI_TILT].pinNum, t_brightness);
}

void setGILamp(bool t_on) {
  pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_HEAD_GI, t_on ? LOW : HIGH);
}

void setTiltLamp(bool t_on) {
  pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_TILT, t_on ? LOW : HIGH);
}

// Display score using Centipede shift register
void displayScore(int t_score) {
   // t_score is 0..999 (i.e. displayed t_score / 10,000)
   // Clamp t_score to 0..999
    if (t_score < 0) t_score = 0;
    if (t_score > 999) t_score = 999;
    static int lastScore = -1;
    int millions = t_score / 100;         // 1..9
    int hundredK = (t_score / 10) % 10;   // 1..9
    int tensK    = t_score % 10;          // 1..9

    int lastMillions = lastScore < 0 ? 0 : lastScore / 100;
    int lastHundredK = lastScore < 0 ? 0 : (lastScore / 10) % 10;
    int lastTensK    = lastScore < 0 ? 0 : lastScore % 10;

    // Only update lamps that change
    if (lastMillions != millions) {
        if (lastMillions > 0) pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_1M[lastMillions - 1], HIGH); // turn off old
        if (millions > 0)     pShiftRegister->digitalWrite(PIN_OUT_SR_LAMP_1M[millions - 1], LOW);     // turn on new
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

// Fire the Credit Up coil if Credit Full switch is closed (LOW)
// Master should also fire Knocker coil when adding a credit.
void addCredit() {
  if (digitalRead(PIN_IN_SWITCH_CREDIT_FULL) == LOW) {
    analogWrite(deviceParm[DEV_IDX_CREDIT_UP].pinNum, deviceParm[DEV_IDX_CREDIT_UP].powerInitial);
    delay(deviceParm[DEV_IDX_CREDIT_UP].timeOn);
    analogWrite(deviceParm[DEV_IDX_CREDIT_UP].pinNum, 0);
  }
}

// Fire the Credit Down coil if Credit Empty switch is closed (LOW)
bool removeCredit() {
  if (hasCredits()) {
    analogWrite(deviceParm[DEV_IDX_CREDIT_DOWN].pinNum, deviceParm[DEV_IDX_CREDIT_DOWN].powerInitial);
    delay(deviceParm[DEV_IDX_CREDIT_DOWN].timeOn);
    analogWrite(deviceParm[DEV_IDX_CREDIT_DOWN].pinNum, 0);
    return true;  // Successfully removed a credit
  } else {
    return false; // No credits to remove
  }
}

// Returns true if credits remain (Credit Empty switch is closed/LOW)
bool hasCredits() {
  return digitalRead(PIN_IN_SWITCH_CREDIT_EMPTY) == LOW;
}

// Persist a score (0..999) to EEPROM.  Writes only when value changes to reduce EEPROM wear.
void saveScoreToEEPROM(int t_score) {
  if (t_score < 0)  t_score = 0;
  if (t_score > 999) t_score = 999;
  uint16_t s = (uint16_t)t_score;

  // Read existing value and only write if different (minimizes EEPROM writes).
  uint16_t existing = 0;
  EEPROM.get(EEPROM_ADDR_SCORE, existing);
  if (existing != s) {
    EEPROM.put(EEPROM_ADDR_SCORE, s); // EEPROM.put/update writes only changed bytes on AVR
    // Small UI/debug feedback
    sprintf(lcdString, "Saved score %d", currentScore);
    if (pLCD2004) pLCD2004->println(lcdString);
    Serial.println(lcdString);
  }
}

// Read persisted score from EEPROM. Returns clamped value in 0..999.
// If EEPROM contains out-of-range data, returns 0 (safe fallback).
int recallScoreFromEEPROM() {
  uint16_t s = 0;
  EEPROM.get(EEPROM_ADDR_SCORE, s);
  if (s > 999) s = 0;
  sprintf(lcdString, "Recall score %d", s);
  return (int)s;
}