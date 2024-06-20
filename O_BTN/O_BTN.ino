// O_BTN.INO Rev: 03/08/23.  Finished but not tested.
// BTN reads button presses and forwards them along to MAS (mode/state permitting.)

// Re-wrote on 3/2/21 to check Centipede via two port reads rather than 32 bit reads -- much faster.
// Button presses are ignored at all times *except* when in MANUAL MODE, RUNNING STATE.
// RS485 will OVERFLOW if a button is being pressed when MAS MANUAL mode is started (probably also other modes.)
// NOTE regarding button numbers: Button numbers 1..32 correspond to Centipede pins 0..31.
// All modules outside of BTN (and SWT) refer to Turnout numbers and Button numbers starting at 1, not 0.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_BTN;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "BTN 04/15/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage = nullptr;

// *** CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Centipede) is already in <Centipede.h> so not needed here.
// #include <Centipede.h> is already in <Train_Functions.h> so not needed here.
Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (BTN only has ONE Centipede.)

// *** CONSTANTS NEEDED TO TRACK MODE AND STATE ***
byte modeCurrent    = MODE_UNDEFINED;
byte stateCurrent   = STATE_UNDEFINED;

// *** MISC CONSTANTS AND GLOBALS NEEDED BY BTN ***

unsigned long lastReleaseTimeMS      =   0;  // To prevent operator from pressing turnout buttons too quickly
const unsigned long RELEASE_DELAY_MS = 200;  // Force operator to wait this many milliseconds between presses

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // *** INITIALIZE CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Centipede class hangs the system if hardware is not connected.
  Wire.begin();                               // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Centipede;             // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initializePinsForInput();   // Set all Centipede shift register pins to INPUT for Turnout Buttons.

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop

  // **************************************************************
  // ***** SUMMARY OF MESSAGES RECEIVED & SENT BY THIS MODULE *****
  // ***** REV: 03/10/23                                      *****
  // **************************************************************
  //
  // ********** OUTGOING MESSAGES THAT BTN WILL TRANSMIT: *********
  //
  // BTN-to-MAS: 'B' button press: INCLUDES ASKING FOR PERMISSION TO TRANSMIT AND RECEIVING OKAY BEFORE SENDING:
  // pMessage->sendBTNtoMASButton(byte buttonNum);  // (MAS must decide which way to throw turnout)
  //
  // ********** INCOMING MESSAGES THAT BTN WILL RECEIVE: **********
  //
  // MAS-to-ALL: 'M' Mode/State change message:
  // pMessage->getMAStoALLModeState(byte &mode, byte &state);
  //
  // **************************************************************
  // **************************************************************
  // **************************************************************

  // See if there is an incoming message for us...
  char msgType = pMessage->available();

  // msgType ' ' (blank) means there was no message waiting for us.
  // msgType 'M' means this is a Mode/State update message.
  // For any message, we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.

  while (msgType != ' ') {
    switch(msgType) {
      case 'M' :  // New mode/state message in incoming RS485 buffer.  Tested and working 10/15/20.
        pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
        // Just calling the function updates modeCurrent and modeState ;-)
        sprintf(lcdString, "M %i S %i", modeCurrent, stateCurrent); pLCD2004->println(lcdString); Serial.println(lcdString);
        break;
      default:
        sprintf(lcdString, "MSG TYPE ERROR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    msgType = pMessage->available();
  }

  // We have handled any incoming message.

  // Now, if we are in MANUAL mode, and RUNNING state, see if operator pressed a button, and if so, send a message to MAS.
  if ((modeCurrent == MODE_MANUAL) && (stateCurrent == STATE_RUNNING)) {

    // Don't bother checking for button press unless we have waited at least RELEASE_DELAY_MS since the last button release.
    if ((millis() - lastReleaseTimeMS) > RELEASE_DELAY_MS) {

      // Okay, it's been at least RELEASE_DELAY_MS since operator released a turnout control button, so check for another press...
      // Find the FIRST control panel pushbutton that is being pressed.  Only handles one at a time.
      byte buttonNum = turnoutButtonPressed();   // Returns 0 if no button pressed; otherwise button number that was pressed.
      if (buttonNum > 0) {   // Operator pressed a pushbutton!
        //sprintf(lcdString, "Button %i press!", buttonNum); pLCD2004->println(lcdString); Serial.println(lcdString);

        pMessage->sendBTNtoMASButton(buttonNum);
        // The Message class function will automatically handle pulling the digital line low, waiting for permission from MAS to
        // send, and sending the Button-Pressed message via RS485.  If we receive some other relevant message from MAS before we
        // receive permission to transmit the button number (such as a Mode Change message), the system will halt and display a
        // messag on the LCD indicating "unexpected message."  Same general behavior for BTN, OCC, and LEG when they need to send a
        // message to MAS.

        // The operator just pressed the button, so wait for release before moving on...
        turnoutButtonDebounce(buttonNum);  // Won't return until operator has released the pushbutton.

        lastReleaseTimeMS = millis();   // Keep track of the last time a button was released

      }  // end of "if button pressed" code block
    }    // end of "if we've waited long enough since button was released...
  }      // end of "if we are in manual mode, running state...

}  // End of loop()

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

byte turnoutButtonPressed() {
  // Scans the Centipede input shift register connected to the control panel turnout buttons, and returns the button number being
  // pressed, if any, or zero if no buttons currently being pressed.
  // Since reading each Centipede input one pin at a time is so slow that we can overflow the RS485 input buffer, we'll find any
  // grounded pins by using "portRead()" which reads 16 bits at a time - much faster than 30 separate bit reads.  Note that a
  // chip = 65535 if no pins are grounded, and any ground pins become a zero for that bit.  So if all chips read 65535, we're done.
  // Otherwise just calculate which pin is being pulled low on whatever one we come to first (in the event more than one button is
  // being pressed.)  Each Centipede has four 16-bit chips, and we are only using two chips on one board = 32 buttons (we use 30.)
  byte buttonPressed = 0;  // This will be set to button number pressed, if any.  All real buttons are > 0 (1..30)
  unsigned int bankContents = 65535;  // Will be < 65535 if a button is being pressed in a given bank
  for (byte pinBank = 0; pinBank <= 1; pinBank++) {  // This is for ONE Centipede shift register board, first two 16-bit chips.
    bankContents = pShiftRegister->portRead(pinBank);  // Returns a 16-bit value; if < 65535 then a button is pressed on that bank
    if (bankContents < 65535) {  // Calculate which bit, then figure button num based on which bank
      for (byte bitToRead = 0; bitToRead <= 15; bitToRead++) {  // 16 bits per chip
        if ((readBit(bankContents, bitToRead)) == 0) {   // Returns true if bit set; false if bit clear
          buttonPressed = (((pinBank * 16) + bitToRead) + 1);  // Add 1 to return button number 1..n (not 0..n-1)
          sprintf(lcdString, "BTN %i", buttonPressed); pLCD2004->println(lcdString); Serial.println(lcdString);
          chirp();  // operator feedback!
          return buttonPressed;
        }
      }
    }
  }
  // If we fall through the loop to here, nothing was pressed so return zero
  return 0;
}

void turnoutButtonDebounce(const byte tButtonPressed) {
  // Rev 09/14/17.
  // After we detect a control panel turnout button press, and presumably after we have acted on it, we need a delay
  // before we can resume checking for subsequent button presses (or even another press of this same button.)
  // We must allow for some bounce when the pushbutton is pressed, and then more bounce when it is released.

  // Wait long enough to for pushbutton to stop possibly bouncing so our look for "release button" will be valid
  delay(20);  // don't let operator press buttons too quickly
  // Now watch and wait for the operator to release the pushbutton...
  while (pShiftRegister->digitalRead(tButtonPressed - 1) == LOW) {}    // LOW == 0x0; HIGH == 0x1
  // We also need some debounce after the operator releases the button
  delay(20);
  return;
}
