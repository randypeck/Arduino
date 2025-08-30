// Screamo_Master.INO Rev: 08/30/25

// Pin assignments:
// 		RS-485 Serial 2 Tx: Pin 16 = PIN_OUT_MEGA_TX2
// 		RS-485 Serial 2 Rx: Pin 17 = PIN_IN_MEGA_RX2
// 		LCD Serial 1 Tx: Pin 18 = PIN_OUT_MEGA_TX1
// 		LCD Serial 1 Rx: Pin 19 = PIN_IN_MEGA_RX1 (not used)
// 		Centipede I2C SDA: Pin 20 (also pin 44) = PIN_IO_MEGA_SDA
// 		Centipede I2C SCL: Pin 21 (also pin 43) = PIN_IO_MEGA_SCL
//    RS-485 Tx Enable: Pin 22 = PIN_OUT_RS485_TX_ENABLE

// PROBABLY WANT TO SET CONSTS FOR TIME TO PULSE COILS, ETC.  I.e. 50ms is probably long enough for most coils.
// However, it may vary by coil.
// Ideally, have an array with both a hold time (in multiples of 10ms) and a strength (0..255) for coils.
// For Flippers, original Ball Gate, new Ball Release Post, and other coils that may be held after initial activation, include a "hold strength" parm as well.
// If Hold Strength == 0, then simply release after hold time; else change to Hold Strength until released by software.

// Define Arduino pin numbers unique to this Screamo Master Arduino Mega:
// For MOSFETs, can do PWM on pins 4-13 and 44-46 = 13 pins.  However, pins 4 and 13 can't have frequency modified; all others can.
// Use pin numbers 23+ for regular digital pins where PWM is not needed (pin 22 reserved for RS-485 Tx Enable.)
// If a coil is held on, especially at less than 100%, such as ball tray release, it may hum if using PWM at default frequency.
// Increase frequency to eliminate hum (pins 5-12 and 44-46, but *not* pins 4 or 13.)
// Only use pin 4 and 13 for coils that are momentary *and* where PWM may be desired: pop bumper, kickouts, slingshots.
// Other PWM pins can be used for any coil or motor, but especially those that may be held on *and* where PWM is desired: flippers, ball tray release, ball trough release.
// Coils that are both momentary and where PWM is not needed, for full-strength momentary MOSFET-on, use any digital output: knocker, selection unit, relay reset unit.

const byte PIN_OUT_MOSFET_COIL_POP_BUMPER          =  5;  // Output: PWM MOSFET to pop bumper coil.
const byte PIN_OUT_MOSFET_COIL_LEFT_KICKOUT        =  4;  // Output: PWM MOSFET to Left Eject coil (cannot modify PWM freq.)
const byte PIN_OUT_MOSFET_COIL_RIGHT_KICKOUT       = 13;  // Output: PWM MOSFET to Right Eject coil (cannot modify PWM freq.)
const byte PIN_OUT_MOSFET_COIL_LEFT_SLINGSHOT      =  6;  // Output: PWM MOSFET to Left Eject coil.
const byte PIN_OUT_MOSFET_COIL_RIGHT_SLINGSHOT     =  7;  // Output: PWM MOSFET to Right Eject coil.
const byte PIN_OUT_MOSFET_COIL_LEFT_FLIPPER        =  8;  // Output: PWM MOSFET to coil to add one 10K to the score.  Reduce to low power after initial activation.
const byte PIN_OUT_MOSFET_COIL_RIGHT_FLIPPER       =  9;  // Output: PWM MOSFET to coil to ring the 10K bell.  Reduce to low power after initial activation.
const byte PIN_OUT_MOSFET_COIL_BALL_TRAY_RELEASE   = 10;  // Output: PWM MOSFET to (original) Ball Tray Release coil.  Reduce to low power after initial activation.

const byte PIN_OUT_MOSFET_COIL_SELECTION_UNIT      = 23;  // Output: Non-PWM MOSFET to coil that activates Selection Unit (for sound FX only)
const byte PIN_OUT_MOSFET_COIL_RELAY_RESET         = 24;  // Output: Non-PWM MOSFET to coil that activates Relay Reset Bank (for sound FX only)

const byte PIN_OUT_AC_SSR_MOTOR_SCORE_MOTOR        = 25;  // Output (front of cabinet): AC SSR controls 50vac Score Motor (for sound FX only)
const byte PIN_OUT_MOSFET_COIL_KNOCKER             = 26;  // Output (front of cabinet): Non-PWM MOSFET to Knocker coil.

const byte PIN_OUT_MOSFET_COIL_BALL_TROUGH_RELEASE = 11;  // Output (front of cabinet): PWM MOSFET to new up/down post to release individual balls from trough.  Reduce to low power after initial activation.

const byte PIN_OUT_MOSFET_MOTOR_SHAKER             = 12;  // Output (front of cabinet): PWM MOSFET to Shaker Motor

// The following switches will be direct inputs to Arduino pins, rather than via Centipede shift register input:
const byte PIN_IN_BUTTON_FLIPPER_LEFT              = 30;
const byte PIN_IN_BUTTON_FLIPPER_RIGHT             = 31;
const byte PIN_IN_SWITCH_POP_BUMPER_SKIRT          = 32;
const byte PIN_IN_SWITCH_LEFT_SLINGSHOT            = 33;
const byte PIN_IN_SWITCH_RIGHT_SLINGSHOT           = 34;

// The following are array index numbers that cross reference Centipede Input Pin numbers with front-of-cabinet switches that they are connected to:
const byte PIN_IN_BUTTON_START                     =  0;
const byte PIN_IN_BUTTON_DIAG_1                    =  0;
const byte PIN_IN_BUTTON_DIAG_2                    =  0;
const byte PIN_IN_BUTTON_DIAG_3                    =  0;
const byte PIN_IN_BUTTON_DIAG_4                    =  0;
const byte PIN_IN_BUTTON_KNOCK_OFF                 =  0;

const byte PIN_IN_SWITCH_COIN_MECH                 =  0;  // Input: Add a credit, even if during game play
const byte PIN_IN_SWITCH_BALL_PRESENT              =  0;  // Input: Is there a ball at the base of the ball lift?
const byte PIN_IN_SWITCH_TILT                      =  0;  // Input: Tilt

// The following are array index numbers that cross reference Centipede Input Pin numbers with playfield switches that they are connected to:

const byte PIN_IN_SWITCH_DEAD_BUMPER_1             =  0;
const byte PIN_IN_SWITCH_DEAD_BUMPER_2             =  0;
const byte PIN_IN_SWITCH_DEAD_BUMPER_3             =  0;
const byte PIN_IN_SWITCH_DEAD_BUMPER_4             =  0;
const byte PIN_IN_SWITCH_DEAD_BUMPER_5             =  0;
const byte PIN_IN_SWITCH_DEAD_BUMPER_6             =  0;
const byte PIN_IN_SWITCH_LEFT_KICKOUT              =  0;
const byte PIN_IN_SWITCH_RIGHT_KICKOUT             =  0;
const byte PIN_IN_SWITCH_HAT_ROLLOVER_1            =  0;
const byte PIN_IN_SWITCH_HAT_ROLLOVER_2            =  0;
const byte PIN_IN_SWITCH_HAT_ROLLOVER_3            =  0;
const byte PIN_IN_SWITCH_HAT_ROLLOVER_4            =  0;
const byte PIN_IN_SWITCH_TARGET_1                  =  0;
const byte PIN_IN_SWITCH_TARGET_2                  =  0;
const byte PIN_IN_SWITCH_TARGET_3                  =  0;
const byte PIN_IN_SWITCH_TARGET_4                  =  0;
const byte PIN_IN_SWITCH_TARGET_5                  =  0;
const byte PIN_IN_SWITCH_TARGET_6                  =  0;
const byte PIN_IN_SWITCH_TARGET_7                  =  0;
const byte PIN_IN_SWITCH_TARGET_8                  =  0;
const byte PIN_IN_SWITCH_TARGET_9                  =  0;
const byte PIN_IN_SWITCH_TRAP_DOOR                 =  0;
const byte PIN_IN_SWITCH_ROLLOVER_LEFT             =  0;
const byte PIN_IN_SWITCH_ROLLOVER_CENTER           =  0;
const byte PIN_IN_SWITCH_ROLLOVER_RIGHT            =  0;



#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 08/30/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
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

// *** MISC CONSTANTS AND GLOBALS ***

byte         modeCurrent      = MODE_UNDEFINED;
byte         stateCurrent     = STATE_UNDEFINED;
char         msgType          = ' ';

bool debugOn    = false;

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initScreamoMasterPins();

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 LCD2004 instantiated via pLCD2004->begin.
  // Serial2 RS485   instantiated via pMessage->begin.

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  // Serial.println(lcdString);

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Pinball_Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);
delay(1000);  // Master-only delay gives the Slave a chance to get ready to receive data.  ????????????????????????????????? NEEDED?

/*
  // *** INITIALIZE PINBALL_CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Centipede class hangs the system if hardware is not connected.
  Wire.begin();                                 // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Pinball_Centipede;     // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initializePinsForMaster();  // Set Centipede #1 pins for OUTPUT, Centipede #2 pins for INPUT
*/

 // *** STARTUP HOUSEKEEPING: ***
  pMessage->sendTurnOnGI();

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

}  // End of loop()

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

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

