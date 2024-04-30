// O_SWT.INO Rev: 03/08/23.  Finished but not tested.
// SWT receives Set Turnout messages from MAS to throw turnout solenoids, regardless of Mode or State.
// NOTE regarding turnout numbers: Turnout numbers 1..32 correspond to Centipede pins 0..31.
// All modules (other than SWT and BTN internally) refer to Turnout numbers and Button numbers starting at 1, not 0.

// TO DO: Note that facing switches are always switched together; never one without the other: 01/03, 02/05, 06/07, 27/28, 17/18.
// So when the operator presses i.e. turnout 3, either way, then turnout 1 will also be thrown.  17/18 and 27/28 are a bit odd in
// the way they face each other, but the rule should still apply.  So in these cases, when in Manual mode, MAS will send *two*
// switch throw commands to SWT.  Or perhaps SWT should recognize this and handle automatically?  Check to see how I did this prior
// to the OOP re-write.

// IMPORTANT: SWT and LED ignore "Set Route" messages since it's a FRAM Route Rec Num, not individual Route Elements.
// Since neither SWT nor LED have Route Reference tables, whenever MAS sends a Route message, it will must also send individual
// "Set Turnout" messages to SWT and LED.
// ALSO IMPORTANT: Just a batch of "set turnout" messages isn't going to be enough, because some Routes may include the same
// turnout more than once (such as a reverse loop.)  Thus MAS Dispatcher must analyze the Train Progress table and periodically
// send messages to SWT/LED to throw turnouts while a loco is still traversing a route.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_SWT;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SWT 04/15/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage = nullptr;

// *** CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Centipede) is already in <Centipede.h> so not needed here.
// #include <Centipede.h> is already in <Train_Functions.h> so not needed here.
Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (SWT only has ONE Centipede.)

// *** MISC CONSTANTS AND GLOBALS NEEDED BY SWT ***

byte turnoutNum = 0;
char turnoutDir = ' ';  // 'N'ormal or 'R'everse.  Derived from cmdType.

// Set MAX_TURNOUTS_TO_BUF to be the maximum number of turnout commands that will potentially pile up coming from MAS RS485
// i.e. could be several routes being assigned in rapid succession when Auto mode is started.  Longest "regular" route has 8
// turnouts, so conceivable we could get routes for maybe 10 trains = 80 records to buffer, maximum ever conceivable.
// If we overflow the buffer, we can easily increase the size by increasing this variable.
// The reason we need to buffer is because we take a (relatively) long time to throw each turnout, whereas the RS485 messages
// can flood in very quickly.
const byte MAX_TURNOUTS_TO_BUF            =  80;  // How many turnout commands might pile up before they can be executed?
const unsigned long TURNOUT_ACTIVATION_MS = 110;  // How many milliseconds to hold turnout solenoids before releasing.
#include <avr/wdt.h>     // Required to call wdt_reset() for watchdog timer for turnout solenoids

// ******************************************************************************************************************
// ************* PROBABLY MOVE THIS INTO ITS OWN CLASS, BUT THIS IS FROM THE ORIGINAL CODE THAT WORKS ***************
// ******************************************************************************************************************

// DESCRIPTION OF CIRCULAR BUFFERS Rev: 9/5/17.  See also OneNote "Circular Buffers 8/20/17."
// Each element is two bytes: byte turnoutNum and char turnoutDir.
// We use CLOCKWISE circular buffers, and track HEAD, TAIL, and COUNT.
// COUNT is needed for checking if the buffer is empty or full, and must be updated by both ENQUEUE and DEQUEUE.  The advantage of
// this method is that we can use all available elements of the buffer, and the math (modulo) is simpler than having just two
// variables, but we must maintain the third COUNT variable.
// Here is an example of a buffer showing storage of Data and Junk.
// HEAD always points to the unused cell where the next new element will be enqueued, UNLESS THE BUFFER IS FULL, in which case HEAD
// points to TAIL and data cannot be added.
// TAIL always points to the last active element, UNLESS THE BUFFER IS FULL.
// HEAD == TAIL both when the buffer is FULL *and* when it is EMPTY.
// We must examine COUNT to determine whether the buffer is FULL or EMPTY.
// New elements are added at the HEAD, which then moves to the right.
// Old elements are read from the TAIL, which also then moves to the right.
// HEAD, TAIL, and COUNT are initialized to zero.

//    0     1     2     3     4     5
//  ----- ----- ----- ----- ----- -----
// |  D  |  D  |  J  |  J  |  D  |  D  |
//  ----- ----- ----- ----- ----- -----
//                ^           ^
//                HEAD        TAIL

// HEAD always points to the UNUSED cell where the next new element will be inserted (enqueued), UNLESS the queue is full,
// in which case head points to tail and new data cannot be added until the tail data is dequeued.
// TAIL always points to the last active element, UNLESS the queue is empty, in which case it points to garbage.
// COUNT is the total number of active elements, which can range from zero to the size of the buffer.
// Initialize head, tail, and count to zero.
// To enqueue an element, if array is not full, insert data at HEAD, then increment both HEAD and COUNT.
// To dequeue an element, if array is not empty, read data at TAIL, then increment TAIL and decrement COUNT.
// Always incrment HEAD and TAIL pointers using MODULO addition, based on the size of the array.
// Note that HEAD == TAIL *both* when the buffer is empty and when full, so we use COUNT as the test for full/empty status.
// IsEmpty = (COUNT == 0)
// IsFull  = (COUNT == SIZE)

// IMPORTANT: LARGE AMOUNTS OF THIS CODE ARE IDENTIAL IN SWT/LED, SO ALWAYS UPDATE ONE WHEN WE MAKE CHANGES TO THE OTHER, IF NEEDED.

// *** TURNOUT COMMAND BUFFER...
// Create a circular buffer to store incoming RS485 "set turnout" commands from A-MAS.
byte turnoutCmdBufHead = 0;    // Next array element to be written.
byte turnoutCmdBufTail = 0;    // Next array element to be removed.
byte turnoutCmdBufCount = 0;   // Num active elements in buffer.  Max is MAX_TURNOUTS_TO_BUF.
struct turnoutCmdBufStruct {
  byte turnoutNum;             // 1..TOTAL_TURNOUTS
  char turnoutDir;             // 'N' = TURNOUT_DIR_NORMAL for Normal, 'R' = TURNOUT_DIR_REVERSE for Reverse
};
turnoutCmdBufStruct turnoutCmdBuf[MAX_TURNOUTS_TO_BUF];

bool turnoutClosed  = false;  // Keeps track if relay coil is currently energized, so we know to check if it should be released
unsigned long turnoutActivationTime = millis();  // Keeps track of *when* a turnout coil was energized

// *** TURNOUT CROSS REFERENCE...
// 10/25/16: Unique to A-SWT is a cross-reference array that gives a Centipede shift register bit number 0..127 for a corresponding
// turnout identification i.e. 17N or 22R.  This is just how we chose to wire which turnout to which relay.
// Even though this could have been done with one Centipede (64 bits for 32 turnouts,) we are using two
// Centipedes to allow for future expansion, and thus outputs range from 0 through 127.
// Relays that control turnouts 1..16 (1N/1R...16N/16R) are on Centipede #1 on pins 0..63
// Relays that control turnouts 17..32 (17N/17R...32N/32R) are on Centipede #2 on pins 64..127
// Array order is: 1N, 2N, 3N...30N, 31N, 32N,1R, 2R, 3R...30R, 31R, 32R.
// So the first 32 elements are for turnouts 1..32 Normal, the second 32 elements are for turnouts 1..31 Reverse.
byte turnoutCrossRef[(TOTAL_TURNOUTS * 2)] =
  {15,14,13,12,11,10, 9, 8,19,18,17,16,23,22,21,20,79,78,77,76,75,74,73,72,83,82,81,80,87,86,85,84,    // Normals
    0, 1, 2, 3, 4, 5, 6, 7,28,29,30,31,24,25,26,27,64,65,66,67,68,69,70,71,92,93,94,95,88,89,90,91};   // Reverses

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
  pShiftRegister->initializePinsForOutput();  // Set all Centipede shift register pins to OUTPUT for Turnout Solenoids.

  // *** INITIALIZE WATCHDOG TIMER ***
  wdtSetup();                                 // Set up the watchdog timer to prevent solenoids from burning out

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
  // ********** OUTGOING MESSAGES THAT SWT WILL TRANSMIT: *********
  //
  // None.
  //
  // ********** INCOMING MESSAGES THAT SWT WILL RECEIVE: **********
  //
  // NOTE: SWT is not looking for MODE CHANGE because it will never receive a Route or Turnout message from MAS unless it's
  // actionable.  SWT is simply always awaiting a message to throw turnouts, and obeys blindly.
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
  // msgType 'T' means MAS wants us to throw a turnout.
  // For any message, we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.

  while (msgType != ' ') {
    switch(msgType) {
      case 'T' :  // New Turnout message in incoming RS485 buffer.
        pMessage->getMAStoALLTurnout(&turnoutNum, &turnoutDir);
        sprintf(lcdString, "Rec'd: %2i %c", turnoutNum, turnoutDir); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Add the turnout command to the circular buffer for later processing...
        turnoutCmdBufEnqueue(turnoutNum, turnoutDir);
        break;
      default:
        sprintf(lcdString, "MSG TYPE ERROR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    msgType = pMessage->available();
  }

  // We have handled any incoming message.

  // Continuously check the Turnout Command Buffer to see if a turnout needs to be thrown, but once one is thrown, don't throw
  // another until the first one has been released.  Having a command buffer allows us to accept Turnout commands from MAS faster
  // than we can throw them.
  // So we throw the first turnout immediately, but each successive turnout throws one at a time.
  // Depending on whether or not a turnout solenoid is currently being activated, we want to either:
  //   1. If a relay is currently energized, see if it's time to release it yet, or
  //   2. Check if there is a new turnout to throw (from the turnout command buffer) and, if so, close a new relay.

  if (turnoutClosed) {  // A solenoid is being energized -- see if it's been long enough that we can release it
    // Keep the above and below "if" statements separate (not joined by &&), so we only do "else" if an energized solenoid gets
    // released.
    if ((millis() - turnoutActivationTime) > TURNOUT_ACTIVATION_MS) {   // Long enough, we can release it!
      pShiftRegister->initializePinsForOutput();  // Just release all relays, no need to identify the one that's activated.
      turnoutClosed = false;  // No longer energized, happy days.
    }
  } else {   // Only if relays are all open can we check for a new turnout command and potentially close a new relay.
    if (turnoutCmdBufProcess()) {    // Return of true means that we found a new turnout command in the buffer, and just energized a relay/solenoid...
      turnoutClosed = true;
      turnoutActivationTime = millis();
    }
  }

  // We know that if we get here, we just checked that if a turnout was closed, it was released if sufficient time has elapsed.
  // So this is a safe place to reset the watchdog counter.
  wdt_reset();

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

bool turnoutCmdBufProcess() {   // Special version for SWT, not the same as used by LED.
  // See if there is a record in the turnout command buffer, and if so, retrieve it and activate the relay/solenoid.
  // Only closes relay; does not release.  Returns true if a relay was energized, false if no action taken.
  byte t_turnoutNum = 0;
  char t_turnoutDir = ' ';
  if (turnoutCmdBufDequeue(&t_turnoutNum, &t_turnoutDir)) {   // Nothing previously activated, is there one to throw now?
    // If function returned true, we have a new turnout to throw!
    int bitToWrite = 0;
    // Activate the turnout solenoid by turning on the relay coil connected to the Centipede shift register.
    // Use the cross-reference table I wrote for Relay Number vs Turnout N/R.
    // Write a ZERO to a bit to turn on the relay, otherwise all bits should be set to 1.
    // turnoutDir will be either 'N' or 'R'
    // turnoutNum will be turnout number 1..32
    if (t_turnoutDir == TURNOUT_DIR_NORMAL) {  // 'N'
      bitToWrite = turnoutCrossRef[t_turnoutNum - 1];
      sprintf(lcdString, "Throw %2i N", t_turnoutNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
    } else if (t_turnoutDir == TURNOUT_DIR_REVERSE) {  // 'R'
      bitToWrite = turnoutCrossRef[(t_turnoutNum + 32) - 1];
      sprintf(lcdString, "Throw %2i R", t_turnoutNum);
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
    } else {
      sprintf(lcdString, "Turnout buf error!");
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
      endWithFlashingLED(3);   // error!
    }
    pShiftRegister->digitalWrite(bitToWrite, LOW);  // turn on the relay
    return true;    // So we will know that we need to turn it off
  } else {   // We did not retrieve a new turnout throw command and thus did not activate a relay
    return false;
  }
}

void wdtSetup() {
  // A watchdog timer is a hardware timer that automatically generates a system reset if the main program neglects to periodically
  // service it.  We use the function wdt_reset() at the end of every loop() to reset the timer.  If somehow we don't make it all
  // the way through the loop (i.e. we hang when reading RS485, or whatever), the ISR(WDT_vect) function will AUTOMATIALLY be
  // called.  This function will release the turnout solenoids (pShiftRegister->initializePinsForOutput) and terminate the program.
  // IMPORTANT: Arduino MEGA does not work properly with "normal" watchdog timer functions.  The functions here seem to work though.
  // Set up a watchdog timer to run ISR(WDT_vect) every x seconds, unless reset with wdt_reset().
  // This is used to ensure that a relay (and thus a turnout solenoid) is not held "on" for more than x seconds.
  // This function uses a macro defined in wdt.h: #define _BV(BIT) (1<<BIT).  It just defines which bit to set in a byte
  // WDP3 WDP2 WDP1 WDP0 Time-out(ms)
  // 0    0    0    0    16
  // 0    0    0    1    32
  // 0    0    1    0    64
  // 0    0    1    1    125
  // 0    1    0    0    250
  // 0    1    0    1    500
  // 0    1    1    0    1000
  // 0    1    1    1    2000
  // 1    0    0    0    4000
  // 1    0    0    1    8000
  cli();  // Disables all interrupts by clearing the global interrupt mask.
  WDTCSR |= (_BV(WDCE) | _BV(WDE));   // Enable the WD Change Bit
  WDTCSR =   _BV(WDIE) |              // Enable WDT Interrupt
             _BV(WDP2) | _BV(WDP1);   // Set Timeout to ~1 seconds
  sei();  // Enables interrupts by setting the global interrupt mask.
  return;
}

ISR(WDT_vect) {
  // Rev 11/01/16: Watchdog timer.  Turn off all relays and HALT if we don't update the timer at least every 1/2 second.
  sei();  // This is required
  wdt_disable();  // Turn off watchdog timer or it will keep getting called.  Defined in wdt.h.
  pShiftRegister->initializePinsForOutput();  // Release relays first, in case following serial commands blow up the code, even though also called by endWithFlashingLED().
  sprintf(lcdString, "Watchdog timeout!");
  pLCD2004->println(lcdString);
  Serial.println(lcdString);
  endWithFlashingLED(1);
}
