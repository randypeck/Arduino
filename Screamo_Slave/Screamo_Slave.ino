// Screamo_Slave.INO Rev: 08/17/25

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
const byte THIS_MODULE = ARDUINO_SLV;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SLAVE 08/17/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which effectively makes it a global.
// No need to pass lcdString[] to any functions that use it!

// *** SERIAL LCD DISPLAY CLASS ***
Pinball_LCD* pLCD2004 = nullptr;  // pLCD2004 is #included in Pinball_Functions.h, so it's effectively a global variable.

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Pinball_Message.h>
Pinball_Message* pMessage = nullptr;

// *** PINBALL_CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Centipede) is already in <Pinball_Centipede.h> so not needed here.
// #include <Pinball_Centipede.h> is already in <Pinball_Functions.h> so not needed here.
Pinball_Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (SNS only has ONE Centipede.)

// *** MISC CONSTANTS AND GLOBALS ***

byte         modeCurrent      = MODE_UNDEFINED;
byte         stateCurrent     = STATE_UNDEFINED;
char         msgType          = ' ';

bool debugOn    = false;

// *** SENSOR STATE TABLE: Arrays contain 4 elements (unsigned ints) of 16 bits each = 64 bits = 1 Centipede
unsigned int sensorOldState[] = {65535,65535,65535,65535};
unsigned int sensorNewState[] = {65535,65535,65535,65535};


// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initScreamoSlavePins();

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 instantiated via Pinball_LCD/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200

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


  // *** INITIALIZE PINBALL_CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Pinball_Centipede class hangs the system if hardware is not connected.
// COMMENT OUT FOR NOW since no Centipede is connected to our slave yet, and we know it works
/*
  Wire.begin();                               // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Pinball_Centipede;     // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initializePinsForSlave();   // Set all Centipede shift register pins to INPUT for Sensors.
*/
}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  // See if there is an incoming message for us...
  char msgType = pMessage->available();

  // msgType ' ' (blank) means there was no message waiting for us.
  // msgType 'C' means this is a request "are there any credits, and if so please deduct one" message.
  // For any message, we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.

  while (msgType != ' ') {
    bool t_success = true;
    switch(msgType) {
      case 'G' :  // Turn on G.I.
        sprintf(lcdString, "Turning on G.I."); Serial.println(lcdString);
        break;

      case 'C' :  // New credit request.  There are no parms being passed to us; we just need to check the Slave's "Credits" switch, and:
                  //   If there are credits, then deduct one and return TRUE
                  //   Else if no credits, return FALSE
        // Here is where we'll add the code to check the switch and possibly deduct a credit on the credit wheel
        // Now tell Master what we did
        sprintf(lcdString, "Sending to Master"); Serial.println(lcdString);
        delay(250);
        pMessage->sendCreditSuccess(t_success);
        sprintf(lcdString, "Msg sent to Master"); Serial.println(lcdString);
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
        break;

      default:
        sprintf(lcdString, "MSG TYPE ERROR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString);
        // It's printing a, b, c, etc. i.e. successive characters **************************************************************************
    }
    msgType = pMessage->available();
  }

  // We have handled any incoming message.


}  // End of loop()

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

