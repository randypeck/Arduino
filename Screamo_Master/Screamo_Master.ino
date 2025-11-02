// Screamo_Master.INO Rev: 11/01/25

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>

// #include <EEPROM.h>               // For saving and recalling score, to persist between power cycles.
// const int EEPROM_ADDR_SCORE = 0;  // Address to store 16-bit score (uses addr 0 and 1)

#include <Tsunami.h>

// *** CREATE AN ARRAY OF deviceParm STRUCT FOR ALL COILS AND MOTORS ***
// Define array index constants - this list is rather arbitrary and doesn't relate to device number/pin numbers:
const byte DEV_IDX_POP_BUMPER           =  0;
const byte DEV_IDX_KICKOUT_LEFT         =  1;
const byte DEV_IDX_KICKOUT_RIGHT        =  2;
const byte DEV_IDX_SLINGSHOT_LEFT       =  3;
const byte DEV_IDX_SLINGSHOT_RIGHT      =  4;
const byte DEV_IDX_FLIPPER_LEFT         =  5;
const byte DEV_IDX_FLIPPER_RIGHT        =  6;
const byte DEV_IDX_BALL_TRAY_RELEASE    =  7;
const byte DEV_IDX_SELECTION_UNIT       =  8;
const byte DEV_IDX_RELAY_RESET          =  9;
const byte DEV_IDX_BALL_TROUGH_RELEASE  = 10;
const byte DEV_IDX_MOTOR_SHAKER         = 11;
const byte DEV_IDX_KNOCKER              = 12;
const byte DEV_IDX_MOTOR_SCORE          = 13;

const byte NUM_DEVS = 14;

// Define a struct to store coil/motor power and time parameters.  Coils that may be held on include a hold strength parameter.
// For Flippers, original Ball Gate, new Ball Release Post, and other coils that may be held after initial activation, include a
// "hold strength" parm as well as initial strength and time on.
// If Hold Strength == 0, then simply release after hold time; else change to Hold Strength until released by software.
// Example usage:
//   analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerInitial);
//   delay(deviceParm[DEV_IDX_FLIPPER_LEFT].timeOn);
//   analogWrite(deviceParm[DEV_IDX_FLIPPER_LEFT].pinNum, deviceParm[DEV_IDX_FLIPPER_LEFT].powerHold);
struct deviceParmStruct {
  byte pinNum;        // Arduino pin number for this coil/motor
  byte powerInitial;  // 0..255 PWM power level when first energized
  byte timeOn;        // ms to hold initial power level before changing to hold level or turning off
  byte powerHold;     // 0..255 PWM power level to hold after initial timeOn; 0 = turn off after timeOn
};

deviceParmStruct deviceParm[NUM_DEVS] = {
  {  5, 200,  50,   0 },  // POP_BUMPER; PWM MOSFET
  {  4, 100,  50,   0 },  // KICKOUT_LEFT; PWM MOSFET (cannot modify PWM freq.)
  { 13, 100,  50,   0 },  // KICKOUT_RIGHT; PWM MOSFET (cannot modify PWM freq.)
  {  6, 200,  50,   0 },  // SLINGSHOT_LEFT; PWM MOSFET
  {  7, 200,  50,   0 },  // SLINGSHOT_RIGHT; PWM MOSFET
  {  8, 200, 100,  50 },  // FLIPPER_LEFT; PWM MOSFET
  {  9, 200, 100,  50 },  // FLIPPER_RIGHT; PWM MOSFET
  { 10, 100, 200,  50 },  // BALL_TRAY_RELEASE (original); PWM MOSFET
  { 23, 255,  50,   0 },  // SELECTION_UNIT: Non-PWM MOSFET; on/off only.
  { 24, 255,  50,   0 },  // RELAY_RESET: Non-PWM MOSFET; on/off only.
  { 11, 200, 200,   0 },  // BALL_TROUGH_RELEASE (new up/down post); PWM MOSFET
  { 12, 200, 250,   0 },  // MOTOR_SHAKER; PWM MOSFET
  { 26, 200,  40,   0 },  // KNOCKER: Non-PWM MOSFET; on/off only.
  { 25, 255, 250,   0 }   // AC_SCORE_MOTOR: A/C SSR; on/off only; NOT PWM.
};

// *********** NOW WE NEED A FUNCTION WE CAN CALL i.e. fireDevice(deviceIndex); that will use the above info. ***********
// For final code, we may want to make this non-blocking by using millis() timing instead of delay(), but for now delay() is simpler.

// *** CREATE AN ARRAY OF switchParm STRUCT FOR ALL SWITCHES AND BUTTONS ***
// Flipper buttons are direct-wired to Arduino pins for faster response, so not included here.
// Define array index constants - this list is rather arbitrary and doesn't relate to device number/pin numbers:
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

const byte NUM_SWITCHES = 64; // until we identify the pin numbers for each switch 37;  // Total number of switches connected to Centipede input shift register (1 Centipede = 64 inputs)

// Define a struct to store switch parameters.
// No doubt there will be other uses for this struct in the future, but this is all I can think of now.
// Example usage:
//   pShiftRegister->digitalRead(switchParm[SWITCH_IDX_START_BUTTON].pinNum);
struct switchParmStruct {
  byte pinNum;        // Centipede pin number for this switch (0..63)
  byte loopConst;     // Number of 10ms loops to confirm stable state change
  byte loopCount;     // Current count of 10ms loops with stable state (initialized to zero)
};

switchParmStruct switchParm[NUM_SWITCHES] = {
  { 54,  0,  0 },  // SWITCH_IDX_START_BUTTON        =  0;
  { 51,  0,  0 },  // SWITCH_IDX_DIAG_1              =  1;  // Back
  { 48,  0,  0 },  // SWITCH_IDX_DIAG_2              =  2;  // Minus/Left
  { 49,  0,  0 },  // SWITCH_IDX_DIAG_3              =  3;  // Plus/Right
  { 52,  0,  0 },  // SWITCH_IDX_DIAG_4              =  4;  // Select
  { 63,  0,  0 },  // SWITCH_IDX_KNOCK_OFF           =  5;
  { 53,  0,  0 },  // SWITCH_IDX_COIN_MECH           =  6;
  { 50,  0,  0 },  // SWITCH_IDX_BALL_PRESENT        =  7;  // (New) Ball present at bottom of ball lift
  { 55,  0,  0 },  // SWITCH_IDX_TILT_BOB            =  8;
  {  0,  0,  0 },  // SWITCH_IDX_BUMPER_S            =  9;  // 'S' bumper switch
  {  0,  0,  0 },  // SWITCH_IDX_BUMPER_C            = 10;  // 'C' bumper switch
  {  0,  0,  0 },  // SWITCH_IDX_BUMPER_R            = 11;  // 'R' bumper switch
  {  0,  0,  0 },  // SWITCH_IDX_BUMPER_E            = 12;  // 'E' bumper switch
  {  0,  0,  0 },  // SWITCH_IDX_BUMPER_A            = 13;  // 'A' bumper switch
  {  0,  0,  0 },  // SWITCH_IDX_BUMPER_M            = 14;  // 'M' bumper switch
  {  0,  0,  0 },  // SWITCH_IDX_BUMPER_O            = 15;  // 'O' bumper switch
  {  0,  0,  0 },  // SWITCH_IDX_KICKOUT_LEFT        = 16;
  {  0,  0,  0 },  // SWITCH_IDX_KICKOUT_RIGHT       = 17;
  {  0,  0,  0 },  // SWITCH_IDX_SLINGSHOT_LEFT      = 18;  // Two switches wired in parallel
  {  0,  0,  0 },  // SWITCH_IDX_SLINGSHOT_RIGHT     = 19;  // Two switches wired in parallel
  {  0,  0,  0 },  // SWITCH_IDX_HAT_LEFT_TOP        = 20;
  {  0,  0,  0 },  // SWITCH_IDX_HAT_LEFT_BOTTOM     = 21;
  {  0,  0,  0 },  // SWITCH_IDX_HAT_RIGHT_TOP       = 22;
  {  0,  0,  0 },  // SWITCH_IDX_HAT_RIGHT_BOTTOM    = 23;
  {  0,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_2  = 24;  // Long narrow side target near top left
  {  0,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_3  = 25;  // Upper switch above left kickout
  {  0,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_4  = 26;  // Lower switch above left kickout
  {  0,  0,  0 },  // SWITCH_IDX_LEFT_SIDE_TARGET_5  = 27;  // Below left kickout
  {  0,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_1 = 28;  // Top right just below ball gate
  {  0,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_2 = 29;  // Long narrow side target near top right
  {  0,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_3 = 30;  // Upper switch above right kickout
  {  0,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_4 = 31;  // Lower switch above right kickout
  {  0,  0,  0 },  // SWITCH_IDX_RIGHT_SIDE_TARGET_5 = 32;  // Below right kickout
  {  0,  0,  0 },  // SWITCH_IDX_GOBBLE              = 33;
  {  0,  0,  0 },  // SWITCH_IDX_DRAIN_LEFT          = 34;  // Left drain switch index in Centipede input shift register
  {  0,  0,  0 },  // SWITCH_IDX_DRAIN_CENTER        = 35;  // Center drain switch index in Centipede input shift register
  {  0,  0,  0 }   // SWITCH_IDX_DRAIN_RIGHT         = 36;  // Right drain switch index in Centipede input shift register
};

const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 11/01/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which effectively makes it a global.
// No need to pass lcdString[] to any functions that use it!

// *** SERIAL LCD DISPLAY CLASS ***
Pinball_LCD* pLCD2004 = nullptr;  // pLCD2004 is #included in Pinball_Functions.h, so it's effectively a global variable.

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Pinball_Message.h>
Pinball_Message* pMessage = nullptr;

// *** PINBALL_CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Pinball_Centipede) is already in <Pinball_Centipede.h> so not needed here.
// #include <Pinball_Centipede.h> is already in <Pinball_Functions.h> so not needed here.
Pinball_Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards

// *** TSUMANI WAV PLAYER CLASS ***
Tsunami* pTsunami = nullptr;  // Tsunami WAV player object pointer

// *** MISC CONSTANTS AND GLOBALS ***
byte         modeCurrent      = MODE_UNDEFINED;
byte         stateCurrent     = STATE_UNDEFINED;
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

  initScreamoMasterArduinoPins();  // Arduino direct I/O pins only.

  // Set initial state for all MOSFET PWM output pins (pins 5-12)
  for (int i = 0; i < NUM_DEVS; i++) {
    digitalWrite(deviceParm[i].pinNum, LOW);  // Ensure all MOSFET outputs are off at startup.
    pinMode(deviceParm[i].pinNum, OUTPUT);
  }
  // Increase PWM frequency for pins 11 and 12 (Timer 1) to reduce coil buzz - maybe no needed here in Master?
  // Pins 11 and 12 are Ball Trough Release coil and Shaker Motor control
  // Set Timer 1 prescaler to 1 for ~31kHz PWM frequency
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 LCD2004 instantiated via pLCD2004->begin.
  // Serial2 RS485   instantiated via pMessage->begin.
  // Serial3 Tsunami WAV Trigger instantiated via tsunami->begin.

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
  pShiftRegister->initScreamoMasterCentipedePins();

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


  // Here is some code to test the Tsunami WAV player by playing a sound...
  // Call example: playTsunamiSound(1);
  // 001..016 are spoken numbers
  // 020 is coaster clicks
  // 021 is coaster clicks with 10-char filename
  // 030 is "cock the gun" being spoken
  // 031 is American Flyer announcement
  // Track number, output number is always 0, lock T/F
  pTsunami->trackPlayPoly(21, 0, false);
  delay(2000);
  pTsunami->trackPlayPoly(30, 0, false);
  delay(2000);
  pTsunami->trackPlayPoly(1, 0, false);
  delay(2000);
  pTsunami->trackStop(21);

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

  /*
  // Here is some code that tests Centipede #1 output pins by cycling them on and off...
  while (true) {
    // Start with pinNum = 16 since the first 16 pins are unused on Centipede #1 in Screamo Master.
    for (int pinNum = 16; pinNum < 64; pinNum++) {  // Cycle through all 64 output pins on Centipede #1
      pShiftRegister->digitalWrite(pinNum, LOW);  // Turn ON
      sprintf(lcdString, "Pin %02d LOW ", pinNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
      delay(200);
      pShiftRegister->digitalWrite(pinNum, HIGH);   // Turn OFF
      sprintf(lcdString, "Pin %02d HIGH", pinNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
      delay(200);
    }
  }
  */

  /*
  // Here is some code that tests the Mega's PWM outputs by cycling them on and off...
  while (true) {
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
  }
  */
  







  while (true) {}

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

bool switchClosed(byte t_switchNum) {
  // This function will return True or False if a switch is closed for switch numbers 1..52 (NOT 0..51!)
  // Since we don't call this as a result of a detected change, we don't need to update switchOldState/switchNewState arrays.
  // Centipede input 0 = switch #1.
  if ((t_switchNum < 0) || (t_switchNum >= NUM_SWITCHES)) {
    sprintf(lcdString, "SWITCH NUM BAD!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(6);
  }
  t_switchNum = t_switchNum + 64;  // This will convert a pin number to Centipede #2's pin number.
  if (pShiftRegister->digitalRead(t_switchNum - 1) == LOW) {
    return true;
  }
  // The only other possibility is HIGH...
  return false;
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

