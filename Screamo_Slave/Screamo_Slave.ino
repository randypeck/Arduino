// Screamo_Slave.INO Rev: 08/30/25

// Pin assignments:
// 		RS-485 Serial 2 Tx: Pin 16 = PIN_OUT_MEGA_TX2
// 		RS-485 Serial 2 Rx: Pin 17 = PIN_IN_MEGA_RX2
// 		LCD Serial 1 Tx: Pin 18 = PIN_OUT_MEGA_TX1
// 		LCD Serial 1 Rx: Pin 19 = PIN_IN_MEGA_RX1 (not used)
// 		Centipede I2C SDA: Pin 20 (also pin 44) = PIN_IO_MEGA_SDA
// 		Centipede I2C SCL: Pin 21 (also pin 43) = PIN_IO_MEGA_SCL
//    RS-485 Tx Enable: Pin 22 = PIN_OUT_RS485_TX_ENABLE

// Define Arduino pin numbers unique to this Screamo Slave Arduino Mega:
const byte PIN_IN_CREDIT_WHEEL_EMPTY       =  2;  // Input : Credit wheel "Empty" switch.  Pulled LOW when empty.
const byte PIN_IN_CREDIT_WHEEL_FULL        =  3;  // Input : Credit wheel "Full" switch.  Pulled LOW when full.
const byte PIN_OUT_MOSFET_COIL_CREDIT_UP   =  5;  // Output: MOSFET to coil to add one credit to the credit wheel.  Active HIGH, pulse 100-500ms.
const byte PIN_OUT_MOSFET_COIL_CREDIT_DOWN =  6;  // Output: MOSFET to coil to deduct one credit from the credit wheel.  Active HIGH, pulse 100-500ms.
const byte PIN_OUT_MOSFET_COIL_10K_UP      =  7;  // Output: MOSFET to coil to add one 10K to the score.  Active HIGH, pulse 100-500ms.
const byte PIN_OUT_MOSFET_COIL_10K_BELL    =  8;  // Output: MOSFET to coil to ring the 10K bell.  Active HIGH, pulse 100-500ms.
const byte PIN_OUT_MOSFET_COIL_100K_BELL   =  9;  // Output: MOSFET to coil to ring the 100K bell.  Active HIGH, pulse 100-500ms.
const byte PIN_OUT_MOSFET_COIL_SELECT_BELL = 10;  // Output: MOSFET to coil to ring the Select bell.  Active HIGH, pulse 100-500ms.
const byte PIN_OUT_MOSFET_LAMP_SCORE       = 11;  // Output: MOSFET to PWM score lamp brightness.  0-255.
const byte PIN_OUT_MOSFET_LAMP_GI_TILT     = 12;  // Output: MOSFET to PWM G.I. and Tilt lamp brightness.  0-255.

// Centipede shift register pin-to-lamp mapping (not the relay numbers, but the lamps that they illuminate):
// Pin  0 =  20K   | Pin 16 = 900K
// Pin  1 =  40K   | Pin 17 = 2M
// Pin  2 =  60K   | Pin 18 = 4M
// Pin  3 =  80K   | Pin 19 = 6M
// Pin  4 = 100K   | Pin 20 = 8M
// Pin  5 = 300K   | Pin 21 = TILT
// Pin  6 = 500K   | Pin 22 = unused
// Pin  7 = 700K   | Pin 23 = unused
// Pin  8 = 600K   | Pin 24 = unused
// Pin  9 = 400K   | Pin 25 = G.I.
// Pin 10 = 200K   | Pin 26 = 9M
// Pin 11 =  90K   | Pin 27 = 7M
// Pin 12 =  70K   | Pin 28 = 5M
// Pin 13 =  50K   | Pin 29 = 3M
// Pin 14 =  30K   | Pin 30 = 1M
// Pin 15 =  10K   | Pin 31 = 800K

// Score lamp lookup tables (index 0 is digit '1', index 8 is digit '9')
const byte tensK_pins[9]    = {15, 0, 14, 1, 13, 2, 12, 3, 11};      // 10K, 20K, ..., 90K
const byte hundredK_pins[9] = {4, 10, 5, 9, 6, 8, 7, 31, 16};        // 100K, 200K, ..., 900K
const byte million_pins[9]  = {30, 17, 29, 18, 28, 19, 27, 20, 26};  // 1M, 2M, ..., 9M
const byte GI_pin           = 25;
const byte TILT_pin         = 21;

// Helper function prototypes
void displayScore(int score); // score is 0..999 (i.e. displayed score / 10,000)
void setScoreLampBrightness(byte brightness); // 0..255
void setGITiltLampBrightness(byte brightness); // 0..255
void setGILamp(bool on);
void setTiltLamp(bool on);

// Fires the Credit Up coil if Credit Full switch is closed (LOW)
void addCredit() {
    if (digitalRead(PIN_IN_CREDIT_WHEEL_FULL) == LOW) {
        analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_UP, 255);
        delay(50);
        analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_UP, 0);
    }
}

// Fires the Credit Down coil if Credit Empty switch is closed (LOW)
void removeCredit() {
    if (digitalRead(PIN_IN_CREDIT_WHEEL_EMPTY) == LOW) {
        analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_DOWN, 255);
        delay(50);
        analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_DOWN, 0);
    }
}

// Returns true if credits remain (Credit Empty switch is closed/LOW)
bool hasCredits() {
    return digitalRead(PIN_IN_CREDIT_WHEEL_EMPTY) == LOW;
}

#include <Arduino.h>
#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
const byte THIS_MODULE = ARDUINO_SLV;  // Global needed by Pinball_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SLAVE 08/30/25";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Pinball_Functions.h>" includes the line "extern char lcdString[];" which effectively makes it a global.
// No need to pass lcdString[] to any functions that use it!

// *** SERIAL LCD DISPLAY CLASS ***
Pinball_LCD* pLCD2004 = nullptr;  // pLCD2004 is #included in Pinball_Functions.h, so it's effectively a global variable;

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
  digitalWrite(PIN_OUT_LED, LOW);       // Built-in LED LOW=off
  pinMode(PIN_OUT_LED, OUTPUT);
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Put RS485 in receive mode
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);              // HIGH = RS485 transmit, LOW = not transmitting (receive)

  // Set pin modes for all input pins
  pinMode(PIN_IN_CREDIT_WHEEL_EMPTY, INPUT_PULLUP);
  pinMode(PIN_IN_CREDIT_WHEEL_FULL, INPUT_PULLUP);

  // Set pin modes for all output pins
  pinMode(PIN_OUT_MOSFET_COIL_CREDIT_UP, OUTPUT);
  pinMode(PIN_OUT_MOSFET_COIL_CREDIT_DOWN, OUTPUT);
  pinMode(PIN_OUT_MOSFET_COIL_10K_UP, OUTPUT);
  pinMode(PIN_OUT_MOSFET_COIL_10K_BELL, OUTPUT);
  pinMode(PIN_OUT_MOSFET_COIL_100K_BELL, OUTPUT);
  pinMode(PIN_OUT_MOSFET_COIL_SELECT_BELL, OUTPUT);
  pinMode(PIN_OUT_MOSFET_LAMP_SCORE, OUTPUT);
  pinMode(PIN_OUT_MOSFET_LAMP_GI_TILT, OUTPUT);

  // Increase PWM frequency for pins 11 and 12 (Timer 1) to reduce LED flicker
  // Pins 11 and 12 are Score lamp and G.I./Tilt lamp brightness control
  // Set Timer 1 prescaler to 1 for ~31kHz PWM frequency
  TCCR1B = (TCCR1B & 0b11111000) | 0x01;

  // Set initial state for all MOSFET PWM output pins (pins 5-12)
  analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_UP, 0);
  analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_DOWN, 0);
  analogWrite(PIN_OUT_MOSFET_COIL_10K_UP, 0);
  analogWrite(PIN_OUT_MOSFET_COIL_10K_BELL, 0);
  analogWrite(PIN_OUT_MOSFET_COIL_100K_BELL, 0);
  analogWrite(PIN_OUT_MOSFET_COIL_SELECT_BELL, 0);
  setScoreLampBrightness(40);   // Ready to turn on as soon as relay contacts close
  setGITiltLampBrightness(40);  // Ready to turn on as soon as relay contacts close

  // *** INITIALIZE PINBALL_CENTIPDE SHIFT REGISTER ***
  // WARNING: Instantiating Pinball_Centipede class hangs the system if hardware is not connected.
  // We're doing this near the top of the code so we can turn on G.I. as quickly as possible.
  Wire.begin();                               // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Pinball_Centipede;     // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initializePinsForSlave();   // Set all Centipede shift register pins to OUTPUT for Sensors.

  // Turn on GI lamps so player thinks they're getting instant startup (NOTE: Can't do this until after pShiftRegister is initialized)
  setGILamp(true);

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 LCD2004 instantiated via pLCD2004->begin.
  // Serial2 RS485   instantiated via pMessage->begin.

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // Insert a delay() in order to give the Digole 2004 LCD time to power up and be ready to receive commands (esp. the 115K speed command).
  delay(750);  // 500ms was occasionally not enough.
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Pinball_LCD(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  // Serial.println(lcdString);

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Pinball_Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  pLCD2004->println("Setup starting.");

/*
  while (true) {

    if (digitalRead(PIN_IN_CREDIT_WHEEL_FULL) == LOW) {
      pLCD2004->println("Credits not maxed");
    } else {
      pLCD2004->println("Credit maxed!");
    }

    if (digitalRead(PIN_IN_CREDIT_WHEEL_EMPTY) == LOW) {
      pLCD2004->println("Has credits!");
    } else {
      pLCD2004->println("No credits!");
    }
    pLCD2004->println("...");
    delay(1000);
  }
*/


  for (int i = 0; i < 1000; i++) {  // Count score from 0 to 9,990,000
    displayScore(i);
    delay(1000);
  }
  pShiftRegister->digitalWrite(GI_pin, HIGH); 

  while (true) {}  // Stop here for now.

/*
  for (int i = 0; i < 32; i++) {  // Write each Centipede pin for 200ms then off, then on to the next pin

    pShiftRegister->digitalWrite(i, LOW); 
    delay(200);
    if (i==25) delay(5000);
    pShiftRegister->digitalWrite(i, HIGH);
    delay(500);
  }

  pLCD2004->println("Centipede done.");
  while (true) {}
*/
/*
  analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_UP, 255); delay(50); analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_UP, 0);  // Test pulse to credit up coil
  delay(2000);
  analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_DOWN, 255); delay(50); analogWrite(PIN_OUT_MOSFET_COIL_CREDIT_DOWN, 0);  // Test pulse to credit down coil
  delay(2000);
  analogWrite(PIN_OUT_MOSFET_COIL_10K_UP, 255); delay(50); analogWrite(PIN_OUT_MOSFET_COIL_10K_UP, 0);  // Test pulse to 10K up coil
  delay(2000);
  analogWrite(PIN_OUT_MOSFET_COIL_10K_BELL, 255); delay(50); analogWrite(PIN_OUT_MOSFET_COIL_10K_BELL, 0);  // Test pulse to 10K bell coil
  delay(2000);
  analogWrite(PIN_OUT_MOSFET_COIL_100K_BELL, 255); delay(50); analogWrite(PIN_OUT_MOSFET_COIL_100K_BELL, 0);  // Test pulse to 100K bell coil
  delay(2000);
  analogWrite(PIN_OUT_MOSFET_COIL_SELECT_BELL, 255); delay(50); analogWrite(PIN_OUT_MOSFET_COIL_SELECT_BELL, 0);  // Test pulse to Select bell coil
  delay(2000);
 */
  pLCD2004->println("Setup done.");
  Serial.println("Setup done.");

  while (true) {}

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
        sprintf(lcdString, "Turning on G.I."); pLCD2004->println(lcdString);
        break;

      case 'C' :  // New credit request.  There are no parms being passed to us; we just need to check the Slave's "Credits" switch, and:
                  //   If there are credits, then deduct one and return TRUE
                  //   Else if no credits, return FALSE
        // Here is where we'll add the code to check the switch and possibly deduct a credit on the credit wheel
        // Now tell Master what we did
        sprintf(lcdString, "Sending to Master"); pLCD2004->println(lcdString);
        delay(250);
        pMessage->sendCreditSuccess(t_success);
        sprintf(lcdString, "Msg sent to Master"); pLCD2004->println(lcdString);
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

// Display score using Centipede shift register
void displayScore(int score) {
    // Clamp score to 0..999
    if (score < 0) score = 0;
    if (score > 999) score = 999;
    static int lastScore = -1;
    int millions = score / 100;         // 1..9
    int hundredK = (score / 10) % 10;   // 1..9
    int tensK = score % 10;             // 1..9

    int lastMillions = lastScore < 0 ? 0 : lastScore / 100;
    int lastHundredK = lastScore < 0 ? 0 : (lastScore / 10) % 10;
    int lastTensK = lastScore < 0 ? 0 : lastScore % 10;

    // Only update lamps that change
    if (lastMillions != millions) {
        if (lastMillions > 0) pShiftRegister->digitalWrite(million_pins[lastMillions-1], HIGH); // turn off old
        if (millions > 0)     pShiftRegister->digitalWrite(million_pins[millions-1], LOW);     // turn on new
    }
    if (lastHundredK != hundredK) {
        if (lastHundredK > 0) pShiftRegister->digitalWrite(hundredK_pins[lastHundredK-1], HIGH);
        if (hundredK > 0)     pShiftRegister->digitalWrite(hundredK_pins[hundredK-1], LOW);
    }
    if (lastTensK != tensK) {
        if (lastTensK > 0) pShiftRegister->digitalWrite(tensK_pins[lastTensK-1], HIGH);
        if (tensK > 0)     pShiftRegister->digitalWrite(tensK_pins[tensK-1], LOW);
    }
    lastScore = score;
}

void setScoreLampBrightness(byte brightness) {
    analogWrite(PIN_OUT_MOSFET_LAMP_SCORE, brightness);
}

void setGITiltLampBrightness(byte brightness) {
    analogWrite(PIN_OUT_MOSFET_LAMP_GI_TILT, brightness);
}

void setGILamp(bool on) {
    pShiftRegister->digitalWrite(GI_pin, on ? LOW : HIGH);
}

void setTiltLamp(bool on) {
    pShiftRegister->digitalWrite(TILT_pin, on ? LOW : HIGH);
}

