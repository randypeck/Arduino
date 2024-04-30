// O_LED.INO Rev: 03/08/23.  Finished but not tested.
// LED paints the GREEN LEDs on the control panel, which indicate turnout orientation.
// LED listens for RS485 incoming commands (addressed to SWT) to know how turnouts are set, and illuminates turnout LEDs
// accordingly (mode/state permitting.)
// If two facing turnouts that are represented by a single LED are not aligned together, the green LED blinks.
// If state is not STATE_RUNNING or STATE_STOPPING, paints all green LEDs "off" regardless of mode.
// All modules (other than SWT and BTN internally) refer to Turnout numbers and Button numbers starting at 1, not 0.
// Uses built-in hard-coded data to know which turnout LEDs to illuminate based on various combinations (i.e. facing
// turnouts.)

// IMPORTANT: SWT and LED ignore "Set Route" messages since it's a FRAM Route Rec Num, not individual Route Elements.
// Since neither SWT nor LED have Route Reference tables, whenever MAS sends a Route message, it will must also send individual
// "Set Turnout" messages to SWT and LED.
// ALSO IMPORTANT: Just a batch of "set turnout" messages isn't going to be enough, because some Routes may include the same
// turnout more than once (such as a reverse loop.)  Thus MAS Dispatcher must analyze the Train Progress table and periodically
// send messages to SWT/LED to throw turnouts while a loco is still traversing a route.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_LED;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "LED 04/15/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage = nullptr;

// *** CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Centipede) is already in <Centipede.h> so not needed here.
// #include <Centipede.h> is already in <Train_Functions.h> so not needed here.
Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (LED only has ONE Centipede.)

// *** CONSTANTS NEEDED TO TRACK MODE AND STATE ***
byte modeCurrent    = MODE_UNDEFINED;
byte stateCurrent   = STATE_UNDEFINED;

// *** MISC CONSTANTS AND GLOBALS NEEDED BY LED ***

byte turnoutNum = 0;    // 1..30
char turnoutDir = ' ';  // 'N'ormal or 'R'everse.

// Set MAX_TURNOUTS_TO_BUF to be the maximum number of turnout commands that will potentially pile up coming from MAS RS485
// i.e. could be several routes being assigned in rapid succession when Auto mode is started.  Longest "regular" route has 8
// turnouts, so conceivable we could get routes for maybe 10 trains = 80 records to buffer, maximum ever conceivable.
// If we overflow the buffer, we can easily increase the size by increasing this variable.
// The reason we need to buffer is because we take a (relatively) long time to throw each turnout, whereas the RS485 messages
// can flood in very quickly.
const byte MAX_TURNOUTS_TO_BUF            =  80;  // How many turnout commands might pile up before they can be executed?
const unsigned long TURNOUT_ACTIVATION_MS = 110;  // We'll check for LED changes exactly as often as turnouts are thrown in SWT.
unsigned long turnoutUpdateTime = millis();       // Keeps track of *when* a turnout coil was energized

// Paint all green LEDs "off" if state is not STATE_RUNNING or STATE_STOPPING, regardless of mode.
const byte LED_DARK                       =   0;  // LED off
const byte LED_GREEN_SOLID                =   1;  // Green turnout indicator LED lit solid
const byte LED_GREEN_BLINKING             =   2;  // Green turnout indicator LED lit blinking
const unsigned long LED_FLASH_MS          = 500;  // Toggle "conflicted" LEDs every 1/2 second

// IMPORTANT: LARGE AMOUNTS OF THIS CODE ARE IDENTIAL IN SWT/LED, SO ALWAYS UPDATE ONE WHEN WE MAKE CHANGES TO THE OTHER, IF NEEDED.

// *** TURNOUT COMMAND BUFFER...
// Create a circular buffer to store incoming RS485 "set turnout" commands from A-MAS.
byte turnoutCmdBufHead = 0;    // Next array element to be written.
byte turnoutCmdBufTail = 0;    // Next array element to be removed.
byte turnoutCmdBufCount = 0;   // Num active elements in buffer.  Max is MAX_TURNOUTS_TO_BUF.
struct turnoutCmdBufStruct {
  byte turnoutNum;             // 1..TOTAL_TURNOUTS
  char turnoutDir;             // 'N' for Normal, 'R' for Reverse
};
turnoutCmdBufStruct turnoutCmdBuf[MAX_TURNOUTS_TO_BUF];

// *** TURNOUT DIRECTION CURRENT STATUS...
// 10/25/16: LED only.  Populated with current turnout positions/orientations.
// Record corresponds to [turnout number - 1] = (0..31).  I.e. turnout #1 at array position zero.
// turnoutDirStatus[[0..29] = 'N', 'R', or blank if unknown (at init).  We will give it 32 elements for future expansion.
char turnoutDirStatus[TOTAL_TURNOUTS] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};

// *** TURNOUT LED DESIRED STATUS...
// 10/25/16: LED only.  Populated with what the status of each green LED *should* be, based on above turnout positions.
// Record number corresponds to Centipede shift register output pin (0..63).
// There are twice as many LEDs as turnouts because we have (up to) one LED for *each* position, Normal or Reverse, for each
// turnout.  Although there are somewhat less, because some LEDs do double-duty to represent a pair of facing turnouts.
// For example, Turnouts 1 thru 5 have 10 positions, but are represented by only 6 LEDs since many are shared.
// However we won't worry about shared LEDs until we paint the control panel; for now, we act as if we have two per turnout.
// turnoutLEDStatus[0..63] = 0:off, 1:on, 2:blinking
// const byte LED_DARK = 0;                  // LED off
// const byte LED_GREEN_SOLID = 1;           // Green turnout indicator LED lit solid
// const byte LED_GREEN_BLINKING = 2;        // Green turnout indicator LED lit blinking
// The following zeroes should really be {LED_DARK, LED_DARK, LED_DARK, ... } but just use zeroes here for convenience
byte turnoutLEDStatus[TOTAL_TURNOUTS * 2] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// *** TURNOUT LED SHIFT REGISTER PIN NUMBERS...
// 10/25/16: LED only.  Cross-reference array that gives us a Centipede shift register bit number 0..63 for a corresponding
// turnout indication i.e. 17N or 22R.  This is just how we chose to wire which button to each Centipede I/O port.
// Note that when we illuminate one green turnout indicator LED, we must darken its counterpart since a turnout
// can only be thrown one way or the other, not both.  Or blink when an LED is shared by two turnouts and their orientations conflict.
// Here is the "Map" of which turnout position LEDs are connected to which outputs on the Centipedes:
// 1N = Centipede output 0      1R = Centipede output 16
// ...                          ...
// 16N = Centipede output 15    16R = Centipede output 31
// 17N = Centipede output 32    17R = Centipede output 48
// ...                          ...
// 32N = Centipede output 47    32R = Centipede output 63
//
// And here is a cross-reference of the possibly conflicting turnouts/orientations and their Centipede pin numbers:
// 1R/3R are pins 16 & 18
// 2R/5R are pins 17 & 20
// 6R/7R are pins 21 & 22
// 2N/3N are pins 1 & 2
// 4N/5N are pins 3 & 4
// 12N/13R are pins 11 & 28
// 17N/18R are pins 32 & 49
// 27N/28R are pins 42 & 59
// If one of the above pair is "on" and the other is "off", then both pins (which are connected together) should be set to blink.

// Update turnoutLEDStatus[0..63] for each LED using this cross reference, which provides Centipede shift register pin number for each turnout+orientation:
const byte LEDNormalTurnoutPin[TOTAL_TURNOUTS] = 
   { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};
const byte LEDReverseTurnoutPin[TOTAL_TURNOUTS] =
   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

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
  pShiftRegister->initializePinsForOutput();  // Set all Centipede shift register pins to OUTPUT for Turnout LEDs

}

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
  // ********** OUTGOING MESSAGES THAT LED WILL TRANSMIT: *********
  //
  // None.
  //
  // ********** INCOMING MESSAGES THAT LED WILL RECEIVE: **********
  //
  // MAS-to-ALL: 'M' Mode/State change message:
  // pMessage->getMAStoALLModeState(byte &mode, byte &state);
  //
  // MAS-to-ALL: Set 'T'urnout.  Includes number and orientation Normal|Reverse.
  // pMessage->getMAStoALLTurnout(byte &turnoutNum, char &turnoutDir);
  //
  // **************************************************************
  // **************************************************************
  // **************************************************************

  // See if there is an incoming message for us...
  char msgType = pMessage->available();

  // msgType ' ' (blank) means there was no message waiting for us.
  // msgType 'M' means this is a Mode/State update message.
  // msgType 'T' means MAS sent a Turnout message and we should update the green LEDs (mode and state permitting.)
  // For any message, we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.

  while (msgType != ' ') {
    switch(msgType) {
      case 'M' :  // New mode/state message in incoming RS485 buffer.
        pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
        // Just calling the function updates modeCurrent and modeState ;-)
        sprintf(lcdString, "M %i S %i", modeCurrent, stateCurrent); pLCD2004->println(lcdString); Serial.println(lcdString);
        break;
      case 'T' :  // New Turnout message in incoming RS485 buffer.
        pMessage->getMAStoALLTurnout(&turnoutNum, &turnoutDir);
        sprintf(lcdString, "Rec'd: %2i %c", turnoutNum, turnoutDir); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Add the turnout command to the circular buffer for later processing...
        turnoutCmdBufEnqueue(turnoutNum, turnoutDir);
        break;
      default:
        sprintf(lcdString, "MSG TYPE ERROR!");
        pLCD2004->println(lcdString);
        Serial.println(lcdString);
        endWithFlashingLED(1);
    }
    msgType = pMessage->available();
  }

  // We have handled any incoming message.

  if ((millis() - turnoutUpdateTime) > TURNOUT_ACTIVATION_MS) {   // Don't retrieve turnout buffers too quickly, for effect only.
    // About every 110ms.
    // turnoutCmdBufProcess() only takes about 2ms!  It updates "desired LED state" array for MAXIMUM ONE LED ONLY.
    // Pull a turnout command from the buffer, if any, and update the turnoutDirStatus[] and turnoutLEDStatus[] arrays.
    turnoutCmdBufProcess();
    // paintControlPanel() takes about 50ms, so we don't want to call it continuously.  It blinks LEDs at 500ms intervals.
    // Since this loop is begin called every TURNOUT_ACTIVATION_MS (110ms), that's plenty to blink the lights every 1/2 second,
    // and also ensure that the control panel LEDs are "painted" at least every time a turnout is thrown (turnoutCmdBufProcess(), above.)
    paintControlPanel();  // Refresh the green LEDs on the control panel

    turnoutUpdateTime = millis();  // refresh timer for delay between checking for "throws"
  }

}

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

bool turnoutCmdBufIsEmpty() {
  return (turnoutCmdBufCount == 0);
}

bool turnoutCmdBufIsFull() {
  return (turnoutCmdBufCount == MAX_TURNOUTS_TO_BUF);
}

void turnoutCmdBufEnqueue(const byte t_turnoutNum, const char t_turnoutDir) {
  // Insert a record at the head of the turnout command buffer, then increment head and count.
  // If the buffer is already full, trigger a fatal error and terminate.
  if (!turnoutCmdBufIsFull()) {
    turnoutCmdBuf[turnoutCmdBufHead].turnoutNum = t_turnoutNum;  // Store the turnout number 1..TOTAL_TURNOUTS
    turnoutCmdBuf[turnoutCmdBufHead].turnoutDir = t_turnoutDir;  // Store the orientation Normal or Reverse
    turnoutCmdBufHead = (turnoutCmdBufHead + 1) % MAX_TURNOUTS_TO_BUF;
    turnoutCmdBufCount++;
  } else {
    sprintf(lcdString, "Turnout buf ovflow!");
    pLCD2004->println(lcdString);
    Serial.println(lcdString);
    endWithFlashingLED(3);
  }
  return;
}

bool turnoutCmdBufDequeue(byte * t_turnoutNum, char * t_turnoutDir) {
  // Retrieve a record, if any, from the turnout command buffer.
  // If the turnout command buffer is not empty, retrieves a record from the buffer, clears it, and puts the
  // retrieved data into the called parameters.
  // Returns 'false' if buffer is empty, and the passed parameters remain undefined.  Not fatal.
  // Data will be at tail, then tail will be incremented (modulo size), and count will be decremented.
  if (!turnoutCmdBufIsEmpty()) {
    * t_turnoutNum = turnoutCmdBuf[turnoutCmdBufTail].turnoutNum;
    * t_turnoutDir = turnoutCmdBuf[turnoutCmdBufTail].turnoutDir;
    turnoutCmdBufTail = (turnoutCmdBufTail + 1) % MAX_TURNOUTS_TO_BUF;
    turnoutCmdBufCount--;
    return true;
  } else {
    return false;  // Turnout command buffer is empty
  }
}

void turnoutCmdBufProcess() {   // Special version for LED, not the same as used by SWT
  // Rev: 10/26/16.  If a turnout has been thrown, update the turnoutDirStatus[[0..31] array with 'R' or 'N'.
  // AND IF turnoutDirStatus[[] IS UPDATED HERE, then we must also update the turnoutLEDStatus[0..63] array with 0, 1, or 2,
  // which indicates if each LED should be off, on, or blinking.
  // Note that this function does NOT actually illuminate or darken the green control panel LEDs.
  // We use a 32-char array to maintain the current state of each turnout:
  //   turnoutDirStatus[[0..31] = N, R, or blank.
  // NOTE: array element 0..31 is one less than turnout number 1..32.

  byte t_turnoutNum = 0;
  char t_turnoutDir = ' ';

  if  (turnoutCmdBufDequeue(&t_turnoutNum, &t_turnoutDir)) {
    // If we get here, we have a new turnout to "throw".
    // Each time we have a new turnout to throw, we need to update TWO arrays:
    // 1: Update turnoutDirStatus[[0..31] (N|R|' ') for the turnout that was just "thrown."  So we have a list of all current positions.
    // 2: Update turnoutLEDStatus[0..63] (Dark, Solid, or Blinking) to indicate how every green LED *should* now look, with conflicts resolved.

    // First tell the operator what we are going to do...
    if (t_turnoutDir == TURNOUT_DIR_NORMAL) {  // 'N'
      sprintf(lcdString, "Throw %2i N", t_turnoutNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
    } else if (t_turnoutDir == TURNOUT_DIR_REVERSE) {  // 'R'
      sprintf(lcdString, "Throw %2i R", t_turnoutNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
    } else {
      sprintf(lcdString, "Turnout buf error!");
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
      endWithFlashingLED(3);   // error!
    }

    // For example, we might update turnoutDirStatus[[7] = 'R' to indicate turnout #7 is now in Reverse orientation
    turnoutDirStatus[t_turnoutNum - 1] = t_turnoutDir;  // 'R' and 'N' are the only valid possibilities

    // Now that we have revised our "turnoutDirStatus[[0..31] = N or R" array, we should fully re-populate our turnoutLEDStatus[] array
    // that indicates which green turnout LEDs should be on, off, or blinking -- taking into account "conflicts."
    // There are 8 special cases, where we have an LED that is shared by two turnouts.  If the LED should be lit based
    // on one of the pair, but off according to the other turnout, then the shared LED should be blinking.
    // This means that if two facing turnouts, that share an LED, are aligned with each other, then the LED will be solid.
    // If those two turnouts are BOTH aligned away from each other, then the LED will be dark.
    // Finally if one, but not both, turnout is aligned with the other, then the LED will blink.

    // First update the "lit" status of the newly-acquired turnout LED(s) to illuminate Normal or Revers, and turn off it's opposite.
    // This is done via the turnoutLEDStatus[0..63] array, where each element represents an LED, NOT A TURNOUT NUMBER.
    // The offset to the LED number into the turnoutLEDStatus[0..63] array is found in the LEDNormalTurnoutPin[0..31] (for turnouts 1..32 'Normal')
    // and LEDReverseTurnoutPin[0..31] (for turnouts 1..32 'Reverse') arrays, which are defined as constant byte arrays at the top of the code.
    // I.e. suppose we just read Turnout #7 is now set to Reverse:
    // Look up LEDNormalTurnoutPin[6] (turnout #7) =  Centipede pin 6, and LEDReverseTurnoutPin[6] (also turnout #7) = Centipede pin 22.
    // So we will want to set LED #6 (7N) off (turnoutLEDStatus[6] = 0), and set LED #22 (7R) on (turnoutLEDStatus[22] = 1).
    // For simplicity, and since this is all that this Arduino does, we'll re-populate the entire turnoutLEDStatus[] array every time
    // we pass through this block -- i.e. every time a turnout changes its orientation.
    // Then we will address any conflicts that should be resolved (facing turnouts that are misaligned.)

    // Here is the "Map" of which turnout position LEDs are connected to which outputs on the Centipede shift registers:
    // 1N = Centipede output 0      1R = Centipede output 16
    // ...                          ...
    // 16N = Centipede output 15    16R = Centipede output 31
    // 17N = Centipede output 32    17R = Centipede output 48
    // ...                          ...
    // 32N = Centipede output 47    32R = Centipede output 63
    // The above "map" is defined in the LEDNormalTurnoutPin[0..31] and LEDReverseTurnoutPin[0..31] arrays:
    // const byte LEDNormalTurnoutPin[TOTAL_TURNOUTS] = 
    //   { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};
    // const byte LEDReverseTurnoutPin[TOTAL_TURNOUTS] =
    //   {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

    for (byte i = 0; i < TOTAL_TURNOUTS; i++) {
      if (turnoutDirStatus[i] == TURNOUT_DIR_NORMAL) {  // 'N'
        turnoutLEDStatus[LEDNormalTurnoutPin[i]] = LED_GREEN_SOLID;
        turnoutLEDStatus[LEDReverseTurnoutPin[i]] = LED_DARK;
      } else if (turnoutDirStatus[i] == TURNOUT_DIR_REVERSE) {  // 'R'
        turnoutLEDStatus[LEDNormalTurnoutPin[i]] = LED_DARK;
        turnoutLEDStatus[LEDReverseTurnoutPin[i]] = LED_GREEN_SOLID;
      }
    }

    // Now we need to identify any conflicting turnout orientations, and set the appropriate turnoutLEDStatus[]'s to 2 = blinking.
    // Here is a cross-reference of the possibly conflicting turnouts/orientations and their Centipede shift register pin numbers.
    // Conflicting means when facing turnouts are not either *both* aligned with each other, or *both* aligned against the other.
    // We are just going to hard-code this rather than be table-driven, because this isn't a commercial product.
    // NOTE: This code must be modified if used on a new layout, obviously.
    // 1R/3R are pins 16 & 18
    // 2R/5R are pins 17 & 20
    // 6R/7R are pins 21 & 22
    // 2N/3N are pins 1 & 2
    // 4N/5N are pins 3 & 4
    // 12N/13R are pins 11 & 28
    // 17N/18R are pins 32 & 49
    // 27N/28R are pins 42 & 59
    // If one of the above pair is "on" and the other is "off", then both pins (which are connected together) should be set to blink.
    // DON'T FORGET TO SUBTRACT 1 FROM TURNOUT NUMBER WHEN SUBSTITUTING IN turnoutDirStatus[[]

    if ((turnoutDirStatus[0] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[2] == TURNOUT_DIR_NORMAL)) {   // Remember turnoutDirStatus[[0] refers to turnout #1
      turnoutLEDStatus[16] = LED_GREEN_BLINKING;
      turnoutLEDStatus[18] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[0] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[2] == TURNOUT_DIR_REVERSE)) {   // Remember turnoutDirStatus[[0] refers to turnout #1
      turnoutLEDStatus[16] = LED_GREEN_BLINKING;
      turnoutLEDStatus[18] = LED_GREEN_BLINKING;
    }

    if ((turnoutDirStatus[1] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[4] == TURNOUT_DIR_NORMAL)) {
      turnoutLEDStatus[17] = LED_GREEN_BLINKING;
      turnoutLEDStatus[20] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[1] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[4] == TURNOUT_DIR_REVERSE)) {
      turnoutLEDStatus[17] = LED_GREEN_BLINKING;
      turnoutLEDStatus[20] = LED_GREEN_BLINKING;
    }
 
    if ((turnoutDirStatus[5] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[6] == TURNOUT_DIR_NORMAL)) {
      turnoutLEDStatus[21] = LED_GREEN_BLINKING;
      turnoutLEDStatus[22] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[5] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[6] == TURNOUT_DIR_REVERSE)) {
      turnoutLEDStatus[21] = LED_GREEN_BLINKING;
      turnoutLEDStatus[22] = LED_GREEN_BLINKING;
    }

    if ((turnoutDirStatus[1] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[2] == TURNOUT_DIR_REVERSE)) {
      turnoutLEDStatus[1] = LED_GREEN_BLINKING;
      turnoutLEDStatus[1] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[1] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[2] == TURNOUT_DIR_NORMAL)) {
      turnoutLEDStatus[1] = LED_GREEN_BLINKING;
      turnoutLEDStatus[1] = LED_GREEN_BLINKING;
    }

    if ((turnoutDirStatus[3] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[4] == TURNOUT_DIR_REVERSE)) {
      turnoutLEDStatus[3] = LED_GREEN_BLINKING;
      turnoutLEDStatus[4] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[3] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[4] == TURNOUT_DIR_NORMAL)) {
      turnoutLEDStatus[3] = LED_GREEN_BLINKING;
      turnoutLEDStatus[4] = LED_GREEN_BLINKING;
    }

    if ((turnoutDirStatus[11] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[12] == TURNOUT_DIR_NORMAL)) {
      turnoutLEDStatus[11] = LED_GREEN_BLINKING;
      turnoutLEDStatus[28] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[11] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[12] == TURNOUT_DIR_REVERSE)) {
      turnoutLEDStatus[11] = LED_GREEN_BLINKING;
      turnoutLEDStatus[28] = LED_GREEN_BLINKING;
    }

    if ((turnoutDirStatus[16] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[17] == TURNOUT_DIR_NORMAL)) {
      turnoutLEDStatus[32] = LED_GREEN_BLINKING;
      turnoutLEDStatus[49] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[16] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[17] == TURNOUT_DIR_REVERSE)) {
      turnoutLEDStatus[32] = LED_GREEN_BLINKING;
      turnoutLEDStatus[49] = LED_GREEN_BLINKING;
    }

    if ((turnoutDirStatus[26] == TURNOUT_DIR_NORMAL) && (turnoutDirStatus[27] == TURNOUT_DIR_NORMAL)) {
      turnoutLEDStatus[42] = LED_GREEN_BLINKING;
      turnoutLEDStatus[59] = LED_GREEN_BLINKING;
    }
    if ((turnoutDirStatus[26] == TURNOUT_DIR_REVERSE) && (turnoutDirStatus[27] == TURNOUT_DIR_REVERSE)) {
      turnoutLEDStatus[42] = LED_GREEN_BLINKING;
      turnoutLEDStatus[59] = LED_GREEN_BLINKING;
    }
  }
  return;
}

void paintControlPanel() {
  // Rev 10/19/20: Based on the array of current turnout status, update every green turnout-indicator LED on the control panel.
  // Note that although there are not actually two LEDs for every turnout - since facing turnouts share an LED - we have 60
  // outputs wired, so we can code as if there are 60 LEDs (for 30 turnouts, as of Oct 2020.)
  // Any facing turnouts THAT ARE NOT IN SYNC will have their shared LED blinking.  Otherwise, just turn each green LED on or
  // off, as appropriate.
  // This routine should be called at least every 1/2 second to keep up with LEDs blinking, but also any time a turnout is thrown.
  // So we call it about every 110ms, each time we call the above turnoutCmdBufProcess() function, although the two functions are
  // not co-dependent.

  // At this point, we should already have: turnoutLEDStatus[0..63] = 0 (off), 1 (on), or 2 (blinking)
  // So now just update the physical LEDs on the control panel.
  
  // We have a timer to track the flashing LEDs to toggle on/off only every 1/2 second or so.
  // From above: LED_FLASH_MS = 500ms: Toggle "conflicted" LEDs every 1/2 second
  static unsigned long LEDFlashProcessed = millis();   // Delay between toggling flashing LEDs on control panel i.e. 1/2 second.
  static bool LEDsOn = false;    // Toggle this variable to know when conflict LEDs should be on versus off.

  // Each time through this function, decide if flashing LEDs will be on or off at this time...
  if ((millis() - LEDFlashProcessed) > LED_FLASH_MS) {
    LEDFlashProcessed = millis();   // Reset flash timer (about every 1/2 second)
    LEDsOn = !LEDsOn;    // Invert flash toggle
  }

  // Write a ZERO (LOW) to a bit to turn ON the LED, write a ONE (HIGH) to a bit to turn OFF the LED.
  // THIS IS OPPOSITE of our turnoutLEDStatus[] array.
  // Create a 16-bit integer for each of the four 16-bit "chips" on each Centipede.  We'll use these for portWrite().
  // Initialize to 0b1111111111111111, which means "all LEDs off", although they should all get set in this loop.
  unsigned centipedeChip[4] = {65535, 65535, 65535, 65535};
  for (int chipNum = 0; chipNum <= 3; chipNum++) {  // For each chip on the Arduino, right to left: 0..3
    for (int bitNum = 0; bitNum <= 15; bitNum++) {  // For each bit on this chip, right to left: 0..15
      int arrayBit = (chipNum * 16) + bitNum;  // arrayBit will be 0 through 63
      if (arrayBit < (TOTAL_TURNOUTS * 2)) {  // Don't overrun the array bounds; only 60 elements, not 64.
        // If we are *either* in REGISTER Mode in any State, or at STOPPED State in any Mode: Darken all green LEDs.
        if ((modeCurrent == MODE_REGISTER) || (stateCurrent == STATE_STOPPED)) {
          centipedeChip[chipNum] = setBit(centipedeChip[chipNum], bitNum);  // Set bit HIGH, which turns OFF the LED
        } else {   // Mode is not REGISTER, and State is either RUNNING or STOPPING: OK to illuminate LEDs.
          if (turnoutLEDStatus[arrayBit] == LED_DARK) {   // LED should be off
            centipedeChip[chipNum] = setBit(centipedeChip[chipNum], bitNum);  // Set bit HIGH, which turns OFF the LED
          } else if (turnoutLEDStatus[arrayBit] == LED_GREEN_SOLID) {   // LED should be on
            centipedeChip[chipNum] = clearBit(centipedeChip[chipNum], bitNum);  // Set bit LOW, which turns ON the LED
          } else {                          // Must be 2 = LED_GREEN_BLINKING
            if (LEDsOn) {
              centipedeChip[chipNum] = clearBit(centipedeChip[chipNum], bitNum);  // Set bit LOW, which turns ON the LED
            } else {
              centipedeChip[chipNum] = setBit(centipedeChip[chipNum], bitNum);  // Set bit HIGH, which turns OFF the LED
            }
          }
        }
      }
    }
  }

  // Okay, now we have populated the centipedeChip[] array with bits set appropriately for all four 16-bit chips/elements.
  // All we have to do now is write those 16-bit ints to each Centipede using portWrite.
  for (int chipNum = 0; chipNum <= 3; chipNum++) {  // For each chip on the Arduino, right to left: 0..3
    pShiftRegister->portWrite(chipNum, centipedeChip[chipNum]);
  }
  return;
}
