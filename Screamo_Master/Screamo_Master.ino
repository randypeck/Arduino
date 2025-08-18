// Screamo_Master.INO Rev: 08/17/25

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MASTER 08/17/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
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
  // Serial1 instantiated via Pinball_LCD/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200.

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Pinball_Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);
//  delay(1000);  // Master-only delay gives the Slave a chance to get ready to receive data.

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

// wait for a message
while (msgType == ' ') {}

  while (msgType != ' ') {
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
      case 'S' :  // Request from MAS to send the status of a specific sensor.
        // So, which sensor number does MAS want the status of?  It better be 1..52, and *not* 0..51.
//        pMessage->getMAStoSNSRequestSensor(&sensorNum);
        // Look at the Centipede shift register to find the status of the specified sensor in stateOfSensor().
        // We may immediately transmit that information back to MAS -- permission to xmit was implicit with the message
//        pMessage->sendSNStoALLSensorStatus(sensorNum, stateOfSensor(sensorNum));
//        if (stateOfSensor(sensorNum) == SENSOR_STATUS_TRIPPED) {
//          sprintf(lcdString, "Sensor %i Tripped", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
//        } else {
//          sprintf(lcdString, "Sensor %i Cleared", sensorNum); Serial.println(lcdString);  // Not to LCD
//        }
          sprintf(lcdString, "Hello"); pLCD2004->println(lcdString);
        break;
      default:
        sprintf(lcdString, "MSG TYPE ERROR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString);
        // It's printing a, b, c, etc. i.e. successive characters **************************************************************************
    }
    msgType = pMessage->available();
  }



delay(3000);


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

