// O_MAS.INO Rev: 04/16/24.
// MAS is the master controller; everyone else is a slave.

// 03/03/24: No more Dispatch Board object.
// 02/19/24: Eliminate support for POV mode; not worth the effort until I'm ready.  If selected by user, just ignore.  Thus, I
// don't need to include ANY other logic to handle if Mode == MODE_POV.
// 03/11/23: MAS must now always send all individual "throw turnout" commands to SWT and LED, for any mode.
// Previously, the RS-485 "Route" message, which was valid in any mode, sent Route Elements which SWT and LED could parse in order
// to identify and execute just the individual Turnout commands within a series of Route Element commands.  Now MAS will *only*
// send Routes during Auto/Park modes, via the updated "Route" message, which is just a FRAM Route Element Num.  SWT and LED could
// still be programmed to throw all of those turnouts by installing FRAMs and letting them look up the Turnout route elements, but
// this won't work because, in Auto and Park modes, we don't necessarily want to throw an entire route's worth of turnouts at the
// time the Train Progress Route is assigned.  This is because any given turnout *might* occur more than once in a single Route, in
// a different orientation (such as when part of a reverse loop.)  Thus...

// 3/11/23: In Auto/Park mode, MAS Dispatcher will only throw Train Progress Route turnouts through the next point the loco stops.
// In Auto/Park modes, each time a loco starts moving from a stop, even if it stopped just to reverse direction, MAS will throw all
// turnouts from that point in the Route through the next VL00 "stop" element in the Route.
// In Manual mode, MAS will tell SWT/LED to throw turnouts according to button press messages received from BTN.

// **************************************** HOW TO USE QUADRAM, HEAP, POINTERS, and NEW *****************************************
// NOTES REGARDING CLASS/OBJECT DEFINITION, WHEN PLACING OBJECTS ON THE HEAP: We must define a global pointer to an object of that
// type *above* setup(), so the object will be global.  However, we can't instantiate the object until we are inside of setup().
// So above setup(), we use an #include statement to establish the class and also create a pointer to an (as-yet-undefined) object
// of that class.  Then, in setup(),  we use the "new" command to instantiate an object of that class (reserving memory to store
// it,) and assign it's address to the pointer.
// 
// So first, above setup(), define a global pointer variable of the appropriate class type.  I.e.:
//   #include <Display_2004.h>
//   Display_2004* pLCD2004;  // This creates a pointer to the Display_2004 class but does not assign it any value.
// Then at the top of setup(), set up the QuadRAM, I.e.:
//   initializeQuadRAM();  // Add-on memory board provides 56,832 bytes for heap
// Then, also in setup(), create the objects you need by using "new", and assign the pointer returned from new into your global
//   pointer variables.  i.e.:
//     pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer
// Don't forget that the source code (.h) files that use i.e. pLCD2004 will need a declaration of that variable to compile
// without complaint.  We put it in Train_Functions.h which is #included in classes that need to use Display_2004.
//   extern Display_2004* ptrLCD2004;
// Declarations are typically put in a header (.h) file which is then included (via #include) at the top of any source file(s)
// that reference those variables.  This is done to let the compiler know the name and type of the variable, and also to let it
// know that the variable has been defined somewhere else, so the compiler can just go ahead and reference it as though it
// existed.  The linker will take care of attaching such references to the variable definition at link time.
// ******************************************************************************************************************************

// 03/01/23: Until we add the upper level tracks, we must automatically reserve blocks 19-20 and 23-26 as Reserved for STATIC.
// Thus it's as if those blocks are there but unavailable, which means we can populate the entire Route Reference table and run the
// software as usual.  This will allow us to test the system will a full 300+ Route Reference records in FRAM, which will allow us
// to identify performance bottlenecks, memory problems, etc. now rather than only after the second level is constructed.
// We'll reserve these blocks in MAS but not also in OCC so OCC doesn't illuminate those phantom block LEDs on the control panel.
#define SINGLE_LEVEL  // This will trigger blocks 17-20 and 22-26 to be automatically reserved for STATIC during Registration.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_MAS;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "MAS 04/15/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.
// The above "#include <Train_Functions.h>" includes the line "extern char lcdString[];" which effectively makes it a global.
// No need to pass lcdString[] to any functions that use it!

// *** SERIAL LCD DISPLAY CLASS ***
// The above "#include <Train_Functions.h> includes the lines "#include <Display_2004.h>" and "extern Display_2004* pLCD2004;",
// effectively making pLCD2004 a global.  No need to pass pLCD2004 to any functions that use it!
//#include <Display_2004.h> (redundant)
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorage = nullptr;

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage = nullptr;

// *** CONTROL PANEL MODE DIAL AND LEDs CLASS ***
#include <Mode_Dial.h>  // A class with a set of functions for reading and updating the mode-control hardware on the control panel.
Mode_Dial* pModeSelector;

// *** TURNOUT RESERVATION TABLE CLASS (IN FRAM) ***
#include <Turnout_Reservation.h>
Turnout_Reservation* pTurnoutReservation = nullptr;

// *** SENSOR-BLOCK CROSS REFERENCE TABLE CLASS (IN FRAM) ***
#include <Sensor_Block.h>
Sensor_Block* pSensorBlock = nullptr;

// *** BLOCK RESERVATION AND LOOKUP TABLE CLASS (IN FRAM) ***
#include <Block_Reservation.h>
Block_Reservation* pBlockReservation = nullptr;

// *** LOCOMOTIVE REFERENCE LOOKUP TABLE CLASS (IN FRAM) ***
#include <Loco_Reference.h>  // Must be set up before Deadlock as Deadlock requires Loco pointer.
Loco_Reference* pLoco = nullptr;

// *** ROUTE REFERENCE TABLE CLASS (IN FRAM) ***
#include <Route_Reference.h>
Route_Reference* pRoute = nullptr;

// *** DEADLOCK LOOKUP TABLE CLASS (IN FRAM) ***
#include <Deadlock.h>
Deadlock_Reference* pDeadlock = nullptr;

// *** TRAIN PROGRESS TABLE CLASS (ON HEAP) ***
#include <Train_Progress.h>
Train_Progress* pTrainProgress = nullptr;

// *** OPERATOR CLASS ***
//#include <Operator.h>    // Called to operate in Manual mode
//Operator* pOperator;

// *** REGISTRAR CLASS ***
//#include <Registrar.h>   // Called to operate in Register mode
//Registrar* pRegistrar;

// *** DISPATCHER CLASS ***
#include <Dispatcher.h>  // Called to operate in Auto/Park mode
Dispatcher* pDispatcher = nullptr;

// *** MISC CONSTANTS AND GLOBALS NEEDED BY MAS ***

// It won't hurt to initialize mode to MANUAL so long as state is STOPPED.  Nobody will do anything until a mode starts.
// Mode and State will also be set to MANUAL/STOPPED when we call modeDial->begin()
byte modeCurrent    = MODE_MANUAL;
//byte modeOld        = modeCurrent;
//bool modeChanged    = false;
byte stateCurrent   = STATE_STOPPED;
//byte stateOld       = stateCurrent;
//bool stateChanged   = false;
//bool newModeOrState = true;  // A flag so we know when we've just started a new mode and/or state. ***** DUNNO IF I'LL NEED THESE *****************************

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();

  // *** QUADRAM HEAP MEMORY MODULE ***
  initializeQuadRAM();  // Add-on memory board provides 56,832 bytes for heap

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200.

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);

  // *** INITIALIZE FRAM CLASS AND OBJECT *** (Heap uses 76 bytes)
  // Must pass FRAM part num and Arduino pin num as parms to constructor (vs begin) because needed by parent Hackscribble_Ferro.
  // MB85RS4MT is 512KB (4Mb) SPI FRAM.  Our original FRAM was only 8KB SPI.
  pStorage = new FRAM(MB85RS4MT, PIN_IO_FRAM_CS);  // Instantiate the object and assign the global pointer
  pStorage->begin();  // Will crash on its own if there is any problem with the FRAM
  //pStorage->setFRAMRevDate(6, 18, 60);  // Available function for testing only.
  //pStorage->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
  //pStorage->testFRAM();         while (true) { }  // Writes then reads random data to entire FRAM.
  //pStorage->testSRAM();         while (true) { }  // Test 4K of SRAM just for comparison.
  //pStorage->testQuadRAM(56647); while (true) { }  // (unsigned long t_numBytesToTest); max about 56,647 testable bytes.

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);
  delay(1000);  // O_MAS-only delay gives the slaves a chance to get ready to receive data.

  // *** INITIALIZE CONTROL PANEL MODE DIAL CLASS AND OBJECT *** (Heap uses 31 bytes)
  pModeSelector = new Mode_Dial;  //  C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pModeSelector->begin(&modeCurrent, &stateCurrent);  // Sets (RETRIEVES from function) Mode = MANUAL, State = STOPPED

  // *** INITIALIZE TURNOUT RESERVATION CLASS AND OBJECT ***  (Heap uses 9 bytes)
  pTurnoutReservation = new Turnout_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pTurnoutReservation->begin(pStorage);

  // *** INITIALIZE SENSOR-BLOCK CROSS REFERENCE CLASS AND OBJECT *** (Heap uses 9 bytes)
  pSensorBlock = new Sensor_Block;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pSensorBlock->begin(pStorage);

  // *** INITIALIZE BLOCK RESERVATION CLASS AND OBJECT *** (Heap uses 26 bytes)
  pBlockReservation = new Block_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pBlockReservation->begin(pStorage);

  // *** INITIALIZE LOCOMOTIVE REFERENCE TABLE CLASS AND OBJECT *** (Heap uses 81 bytes)
  pLoco = new Loco_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pLoco->begin(pStorage);

  // *** INITIALIZE ROUTE REFERENCE CLASS AND OBJECT ***  (Heap uses 177 bytes)
  pRoute = new Route_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pRoute->begin(pStorage);

  // *** INITIALIZE DEADLOCK LOOKUP CLASS AND OBJECT ***  (Heap uses 32 bytes)
  // WARNING: DEADLOCK MUST BE INSTANTIATED *AFTER* BLOCK RESERVATION AND LOCO REFERENCE.
  pDeadlock = new Deadlock_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pDeadlock->begin(pStorage, pBlockReservation, pLoco);

  // *** INITIALIZE TRAIN PROGRESS CLASS AND OBJECT ***  (Heap uses 14,552 bytes)
  // WARNING: TRAIN PROGRESS MUST BE INSTANTIATED *AFTER* BLOCK RESERVATION AND ROUTE REFERENCE.
  pTrainProgress = new Train_Progress;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pTrainProgress->begin(pBlockReservation, pRoute);

  // *** INITIALIZE OPERATOR CLASS AND OBJECT ***
  // WARNING: OPERATOR MUST BE INSTANTIATED *AFTER* TURNOUT RESERVATION.
  //pOperator = new Operator;  //  C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  //pOperator->begin(pStorage, pMessage, pTurnoutReservation);

  // *** INITIALIZE REGISTRAR CLASS AND OBJECT ***
  //pRegistrar = new Registrar;  //  C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  //pRegistrar->begin();  // Don't yet know what pointers I'll need to pass

  // *** INITIALIZE DISPATCHER CLASS AND OBJECT ***
  // WARNING: DISPATCHER MUST BE INSTANTIATED *AFTER* ALMOST ALL OTHER CLASSES.
  pDispatcher = new Dispatcher;  //  C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pDispatcher->begin(pMessage, pLoco, pBlockReservation, pTurnoutReservation, pSensorBlock, pRoute, pDeadlock, pTrainProgress, pModeSelector);

  // *** STARTUP HOUSEKEEPING: TURNOUTS, SENSORS, ETC. ***
  pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent); // MODE_MANUAL, STATE_STOPPED.
  // No harm in sending MODE_MANUAL so long as state is STATE_STOPPED.  Keep everyone waiting while we set up.

  // *** QUICK CHECK OF TURNOUT 17 RESERVATION DUE TO UNEXPLAINED PROBLEMS ON 12/12/20 ***
  checkForTurnout17Problem();

}  // End of Setup

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop

  // **************************************************************
  // ***** SUMMARY OF MESSAGES RECEIVED & SENT BY THIS MODULE *****
  // ***** REV: 04/14/24                                      *****
  // **************************************************************
  //
  // ********** OUTGOING MESSAGES THAT MAS WILL TRANSMIT: *********
  //
  // MAS-to-ALL: 'M' Mode/State change message:
  // pMessage->sendMAStoALLModeState(byte mode, byte state);
  //
  // MAS-to-ALL: 'T' throw a Turnout.  Applies to SWT and LED.  MANUAL and AUTO/PARK modes.
  // pMessage->sendMAStoALLTurnout(byte turnoutNum, char turnoutDir);
  //
  // MAS-to-SNS: 'S' Request status of sensor, or permission to SNS to transmit sensor trip (ack to digital line request from SNS.)
  //                 Permission if in response to pin-pulled-low RTS, or Request if sensorNum outgoing field is non-zero (and not
  //                 in response to digital line being pulled low.)  Note that we don't call this function to grant permission to
  //                 send; that's done inside of the Message() class.  However we *will* use this function in this main program to
  //                 request the status of a specific (non-zero) sensor.
  // pMessage->sendMAStoSNSRequestSensor(const byte t_sensorNum);  // sensorNum 0 = ok to send; 1..52 = request for status.
  //
  // MAS-to-ALL: 'R' Route sent by FRAM Route Record Number:  AUTO/PARK MODES ONLY.
  // pMessage->sendMAStoALLRoute(byte locoNum, char extOrCont, unsigned int routeRecNum, unsigned long countdown);
  //
  // ********** INCOMING MESSAGES THAT MAS WILL RECEIVE: **********
  //
  // BTN-to-MAS: 'B' Button.  Receive the number of the control panel turnout button that was pressed by the operator.
  //                 MANUAL MODE ONLY.
  // pMessage->getBTNtoMASButton(byte &buttonNum);
  //
  // SNS-to-ALL: 'S' Sensor status.  Either in response to MAS request for a sensor status, or initiated by SNS upon a trip/clear.
  // pMessage->getSNStoALLSensorStatus(byte &sensorNum, char &trippedOrCleared);
  //
  // OCC-to-LEG: 'F' Fast startup Fast/Slow:  REGISTRATION MODE ONLY.  MAS doesn't care about this but will receive it.
  // pMessage->getOCCtoLEGFastOrSlow(char &fastOrSlow);
  //
  // OCC-to-LEG: 'K' Smoke/No smoke:  REGISTRATION MODE ONLY.  MAS doesn't care about this but will receive it.
  // pMessage->getOCCtoLEGSmokeOn(char &smokeOrNoSmoke);
  //
  // OCC-to-LEG: 'A' Audio/No audio:  REGISTRATION MODE ONLY.  MAS probably won't care about this but will receive it.
  // pMessage->getOCCtoLEGAudioOn(char &audioOrNoAudio);
  // 
  // OCC-to-LEG: 'D' Debug/No debug:  REGISTRATION MODE ONLY.
  // pMessage->getOCCtoLEGDebugOn(char &debugOrNoDebug);
  //
  // OCC-to-ALL: 'L' Location of just-registered train.  One rec/occupied sensor, real and statuc.  REGISTRATION MODE ONLY.
  // pMessage->getOCCtoALLTrainLocation(byte &locoNum, routeElement &locoLocation);  // 1..50, BE03

  // **************************************************************
  // **************************************************************
  // **************************************************************

  // We will only be in this main loop while stateCurrent == STATE_STOPPED.  So we'll just hang out here until the operator
  // LEGALLY presses the START button to begin a new mode (Manual, Register, Auto, or Park.)  If so, then immediately change the
  // Mode and State as appropriate, broadcast that to everyone, and call a function to handle that mode until it is stopped.

  pModeSelector->paintModeLEDs(modeCurrent, stateCurrent);

  if (pModeSelector->startButtonIsPressed(&modeCurrent, &stateCurrent)) {  // Operator has legally STARTED a new mode RUNNING!
    // This can only come back true if it was legal to press the START button at this time.  So for example, we can't start Auto
    // mode if we did not previously complete Registration mode.
    // modeCurrent and stateCurrent will now be populated with the newly-started Mode and State selected by the operator.
    pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent);  // Broadcast the new MODE and STATE.
    sprintf(lcdString, "START M%i S%i", modeCurrent, stateCurrent); pLCD2004->println(lcdString); Serial.println(lcdString);
    // We are now in a new Mode (Manual, Register, Auto, or Park) and State is RUNNING.

    if ((modeCurrent == MODE_MANUAL) && (stateCurrent == STATE_RUNNING)) {

      // Run in Manual mode until stopped.
      MASManualMode();
      // Now, modeCurrent == MODE_MANUAL and stateCurrent == STATE_STOPPED

    }  // *** MANUAL MODE COMPLETE! ***

    else if ((modeCurrent == MODE_REGISTER) && (stateCurrent == STATE_RUNNING)) {

      // Run in Register mode until complete.
      MASRegistrationMode();
      // Now, modeCurrent == MODE_REGISTER and stateCurrent == STATE_STOPPED

    }  // *** REGISTER MODE COMPLETE! ***

    else if (((modeCurrent == MODE_AUTO) || (modeCurrent == MODE_PARK)) && (stateCurrent == STATE_RUNNING)) {

      // Run in Auto/Park mode until complete.
      MASAutoParkMode();
      // Now, modeCurrent == MODE_AUTO or MODE_PARK, and stateCurrent == STATE_STOPPED

    }  // *** AUTO/PARK MODE COMPLETE! ***

    else {  // Yikes, we started some unexpected mode/state.  This is a bug.

      sprintf(lcdString, "MODE ERROR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);

    }

  }
}  // End of Loop

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

// ********************************************************************************************************************************
// **************************************************** OPERATE IN MANUAL MODE ****************************************************
// ********************************************************************************************************************************

void MASManualMode() {

  // Paint the control panel Mode Dial LEDs to reflect Manual mode, and illuminate the Stop button so operator can press Stop.
  pModeSelector->paintModeLEDs(modeCurrent, stateCurrent);

  // Upon starting MANUAL mode, first retrieve all sensor statuses.
  getAllSensorStatuses();

  // Now throw all turnouts to a known default starting position, and update the current position in the Turnout Reservation table.
  // If we don't do this, we won't know how to initially illuminate the green turnout position LEDs and we won't know which way to
  // throw a turnout when the operator first presses a turnout button.
  throwAllTurnoutsToDefault();

  // Now operate in Manual mode (Running) until operator presses the Stop button.
  do {
    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop
    // Watch for button presses and sensor changes.
    // Check for an incoming message -- it will ONLY be either Button press request, or Sensor change request
    char msgType = pMessage->available();  // Blank = no message; else call appropriate "pMessage->get" function to retrieve data.

    if (msgType == 'B') {  // Button press from BTN
      byte buttonNum = 0;   // Will be set to 1..30
      char position = ' ';  // Will be set to N or R
      pMessage->getBTNtoMASButton(&buttonNum);  // Will return buttonNum 1..30 corresponding to turnout 1..30
      sprintf(lcdString, "Button press %i", (int)buttonNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      // Get position of turnout from Turnout_Reservation
      position = pTurnoutReservation->getLastOrientation(buttonNum);  // N or R
      // However it's set, reverse it: Normal<->Reverse
      if (position == TURNOUT_DIR_NORMAL) { // 'N'
        position = TURNOUT_DIR_REVERSE; // 'R'
      } else {  // Current setting must be 'R'
        position = TURNOUT_DIR_NORMAL; // 'N'
      }
      // Send 'T'urnout message to SWT to throw turnout, also seen by LED to update green LED on control panel
      pMessage->sendMAStoALLTurnout(buttonNum, position);  // setting = N|R
      // Save new (opposite) position of turnout in Turnout_Reservation.  This is *not* reserving the turnout.
      pTurnoutReservation->setLastOrientation(buttonNum, position);  // Set "last-known" orientation to 'N'ormal or 'R'everse
    }

    else if (msgType == 'S') {  // Sensor status from SNS
      // This will be the result of SNS independently wanting to send us a change that it detected; not a request from us.
      byte sensorNum = 0;  // Will be set to 1..52
      char trippedOrCleared = ' ';  // Will be set to T or C
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
      sprintf(lcdString, "S: %i %c", (int)sensorNum, trippedOrCleared); pLCD2004->println(lcdString); Serial.println(lcdString);
      // Just receive the message.  Nothing to do here, but OCC will see the message and update the white occupancy LEDs.
    }

    else if (msgType != ' ') {  // If it's not Button, Sensor, or blank, we have a bug!
      sprintf(lcdString, "MSG ERR! %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

  } while (pModeSelector->stopButtonIsPressed(modeCurrent, &stateCurrent) == false);

  // Since this mode has been stopped, update stateCurrent to reflect that.  modeCurrent remains unchanged.
  pModeSelector->setStateToSTOPPED(modeCurrent, &stateCurrent);

  // The operator just pressed the Stop button (legally) to stop Manual mode.  The new stateCurrent == STATE_STOPPED
  // Let's be sure we know what we're doing here...
  if ((modeCurrent != MODE_MANUAL) || (stateCurrent != STATE_STOPPED)) {  // Oops!  Fatal bug.
    sprintf(lcdString, "MAN MODE ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  // Let everyone know what we are in Manual mode, Stopped...
  pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent);  // Broadcast MODE (Manual) and STATE (Stopped.)

}

void getAllSensorStatuses() {
  // Rev: 04-15-24.
  // Get sensor status of every sensor so that OCC can illuminate every occupancy sensor LED appropriately:
  for (byte sensorNum = 1; sensorNum <= TOTAL_SENSORS; sensorNum++) {  // For every sensor on the layout
    // Get the sensor's current status: Tripped or Cleared.
    // We (MAS) don't care about the result, but OCC will see the response and illuminate the White occupancy LEDs.
    // First we transmit the request...
    pMessage->sendMAStoSNSRequestSensor(sensorNum);
    // Now wait for the reply message that will give us Tripped or Cleared for sensorNum...
    while (pMessage->available() != 'S') {}  // Don't do anything until we have a Sensor message incoming from SNS
    byte sensorSent = 0;
    char trippedOrCleared = ' ';
    // All modules that care, not just MAS, will see this sensor status message and use it as needed.
    pMessage->getSNStoALLSensorStatus(&sensorSent, &trippedOrCleared);
    if (sensorSent != (sensorNum)) {
      sprintf(lcdString, "SNS %i %i NUM ERR", sensorSent, sensorNum); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }  // That's a bug!
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", sensorNum, trippedOrCleared); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }  // That's a bug!
  }
  return;
}

void throwAllTurnoutsToDefault() {
  // Rev: 04-15-24.
  // Throw all turnouts to a known starting orientation, and update the Turnout Reservation file to reflect the current position.
  // ESPECIALLY throw all single-ended siding turnouts to align with the mainline: 9R, 11R, 13R, 19N, 22R, 23N, 24N, 25N, and 26R.
  // We could update Turnout Reservation Last-Known Orientation here, but I don't think we'll ever use this function as of 4/14/24.
  pMessage->sendMAStoALLTurnout( 1, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 1, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 2, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 2, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 3, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 3, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 4, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 4, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 5, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 5, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 6, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 6, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 7, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 7, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 8, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 8, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout( 9, TURNOUT_DIR_REVERSE);           // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation( 9, TURNOUT_DIR_REVERSE);
  delay(200);
  pMessage->sendMAStoALLTurnout(10, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(10, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(11, TURNOUT_DIR_REVERSE);           // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(11, TURNOUT_DIR_REVERSE);
  delay(200);
  pMessage->sendMAStoALLTurnout(12, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(12, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(13, TURNOUT_DIR_REVERSE);           // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(13, TURNOUT_DIR_REVERSE);
  delay(200);
  pMessage->sendMAStoALLTurnout(14, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(14, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(15, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(15, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(16, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(16, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(17, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(17, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(18, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(18, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(19, TURNOUT_DIR_NORMAL);            // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(19, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(20, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(20, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(21, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(21, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(22, TURNOUT_DIR_REVERSE);           // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(22, TURNOUT_DIR_REVERSE);
  delay(200);
  pMessage->sendMAStoALLTurnout(23, TURNOUT_DIR_NORMAL);            // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(23, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(24, TURNOUT_DIR_NORMAL);            // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(24, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(25, TURNOUT_DIR_NORMAL);            // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(25, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(26, TURNOUT_DIR_REVERSE);           // SINGLE-ENDED SIDING!
  pTurnoutReservation->setLastOrientation(26, TURNOUT_DIR_REVERSE);
  delay(200);
  pMessage->sendMAStoALLTurnout(27, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(27, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(28, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(28, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(29, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(29, TURNOUT_DIR_NORMAL);
  delay(200);
  pMessage->sendMAStoALLTurnout(30, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(30, TURNOUT_DIR_NORMAL);
  return;
}

// ********************************************************************************************************************************
// ************************************************ OPERATE IN REGISTRATION MODE **************************************************
// ********************************************************************************************************************************

void MASRegistrationMode() {

  // Paint the control panel Mode Dial LEDs to reflect Register mode.  Stop button LED will be dark.
  pModeSelector->paintModeLEDs(modeCurrent, stateCurrent);

  // Clear the Block Res'n table...
  pBlockReservation->releaseAllBlocks();  // No blocks are reserved now.

  // *** UNTIL 2ND LEVEL IS BUILT, PERMANENTLY RESERVE 2ND LEVEL BLOCKS 17-20, 22-26 ***
  #ifdef SINGLE_LEVEL  // This will trigger blocks 17-20 and 22-26 to be automatically reserved for STATIC.
    reserveSecondLevelBlocks();
  #endif  // SINGLE_LEVEL

  // When starting REGISTRATION mode, MAS/SNS must ALWAYS send status of every sensor (after a short pause to let us init.)
  // *** GET THE TRIP/CLEAR STATUS OF EVERY OCCUPANCY SENSOR ***
  // No need to clear first as we'll get them all.
  // Registration should reserve occupied blocks as STATIC and then OCC registration questions will change some to locoNums.
  for (byte sensorNum = 1; sensorNum <= TOTAL_SENSORS; sensorNum++) {
    pMessage->sendMAStoSNSRequestSensor(sensorNum);
    // Now wait for the reply message that will give us Tripped or Cleared for sensorNum...
    while (pMessage->available() != 'S') {}  // Don't do anything until we have a Sensor message incoming from SNS
    byte sensorSent = 0;
    char trippedOrCleared = ' ';  // Valid values are SENSOR_STATUS_TRIPPED or SENSOR_STATUS_CLEARED
    // All modules that care, not just MAS, will see this sensor status message and use it as needed.
    pMessage->getSNStoALLSensorStatus(&sensorSent, &trippedOrCleared);
    if (sensorSent != (sensorNum)) {
      sprintf(lcdString, "SNS %i %i NUM ERR", sensorSent, sensorNum); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }  // That's a bug!
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", sensorNum, trippedOrCleared); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }  // That's a bug!
    // If the sensor is TRIPPED, then reserve the corresponding block as Static in the Block Reservation table.
    if (trippedOrCleared == 'T') {  // If this sensor is occupied, reserve the corresponding block/direction for Static equipment.
      if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_EAST) {  // 'E'
        pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BE, LOCO_ID_STATIC);
      } else if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_WEST) {  // 'W'
        pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BW, LOCO_ID_STATIC);
      } else {  // Not E or W, we have a problem!
        sprintf(lcdString, "BLK END %i %c", sensorNum, pSensorBlock->whichEnd(sensorNum)); pLCD2004->println(lcdString); endWithFlashingLED(1);
      }
    }
    delay(50);  // To avoid overwhelming OCC's incoming RS485 buffer which will overflow without a delay here
  }
  // Now all occupied sensors have been identified and all occupied blocks are reserved for Static equipment.

  // Clear the Train Progress table and whatever other tables we might be using
  pTrainProgress->begin(pBlockReservation, pRoute);  // Reset the Train Progress table to no trains anywhere


delay(2000);  // just for testing

  // Since this mode is done, update stateCurrent to reflect that.  modeCurrent remains unchanged.
  pModeSelector->setStateToSTOPPED(modeCurrent, &stateCurrent);


// LEFT OFF HERE 4/14/24 *******************************************************************************************************************************************************************
// See Registration code below, such as it is...



}

// ********************************************************************************************************************************
// ************************************************** OPERATE IN AUTO/PARK MODE ***************************************************
// ********************************************************************************************************************************

void MASAutoParkMode() {


}


/*
  // See if operator is LEGALLY pressing the STOP button.  If so, then change the MODE and STATE as appropriate, and broadcast.
  if (pModeSelector->stopButtonIsPressed(modeCurrent, &stateCurrent)) {
    // This can only come back true if it was legal to press the STOP button at this time.
    // Our new state could be *either* STOPPING or STOPPED, depending on what mode we were in.
    pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent);  // Broadcast the new MODE and STATE.
    sprintf(lcdString, "STOP PRESSED!"); pLCD2004->println(lcdString); Serial.println(lcdString);
  }

  // Now, re-paint the control panel to reflect anything that might have been done.
  // NOTE that we need to update the Mode Dial LEDs under the following circumstances:
  //   1. When the operator turns the Mode Dial and we may need to illuminate the START or STOP LED;
  //   2. When the operator presses START or STOP (as checked above.)
  //   3. When in AUTO or PARK mode, and the state is STOPPING -- we must refresh frequently enough to keep the Mode LED blinking.
  pModeSelector->paintModeLEDs(modeCurrent, stateCurrent);

  // We have whatever MODE and STATE the operator wants to be in, so let's do some work...
  // Note that there's nothing to do in any mode if we are STOPPED, except keep checking if op wants to start a new mode (and keep
  //   the Mode Dial LEDs up to date, in case the dial is turned by the operator.)

  if ((stateCurrent == STATE_RUNNING) || (stateCurrent == STATE_STOPPING)) {  // We are RUNNING or STOPPING in any mode

    // ************************************
    // ***** AUTO and PARK MODE LOGIC *****
    // ************************************

    } else if ((modeCurrent == MODE_AUTO) || (modeCurrent == MODE_PARK)) {  // We are AUTO+RUNNING, AUTO+STOPPING, or PARK+STOPPING.
      // AUTO+RUNNING = We can transition to AUTO+STOPPING or PARK+STOPPING.
      //   (PARK+RUNNING instantly transitions to PARK+STOPPING.  Thus there is no such thing as PARK+RUNNING.)
      // The operator may not press STOP when AUTO or PARK status is STOPPING -- he (yes, *he*!) must just wait.
      // AUTO and PARK are almost identical, except that PARK only assigns routes to Park sidings, and stops assigning new routes
      //   as each train is parked.  This is similar to STOPPING when in AUTO mode.
      // We'll just use a flag for when there are these small differences in logic between AUTO and PARK, RUNNING and STOPPING.
      // When RUNNING in AUTO mode, if operator presses STOP, the mode becomes STOPPING and the system lets all trains finish their
      //   previously-assigned routes.  When all trains have stopped, the status is changed to STOPPED.

      if (stateCurrent == STATE_RUNNING) {  // We are AUTO+RUNNING
        // There is no such thing as PARK+RUNNING.
        // Just keep assigning new routes until operator changes to AUTO+STOPPING or PARK+STOPPING.

        sprintf(lcdString, "AUTO RUNNING"); pLCD2004->println(lcdString); Serial.println(lcdString);

      } else {  // We are either AUTO+STOPPING or PARK+STOPPING.

        sprintf(lcdString, "AUTO/PARK STOPPING"); pLCD2004->println(lcdString); Serial.println(lcdString);







        // All done stopping, so set new state to STOPPED (could be AUTO or PARK.)
        pModeSelector->setStateToSTOPPED(modeCurrent, &stateCurrent);
        sprintf(lcdString, "M%i S%i", modeCurrent, stateCurrent); pLCD2004->println(lcdString); Serial.println(lcdString);
        pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent);  // Broadcast that AUTO or PARK has STOPPED
        //pModeSelector->paintModeLEDs(modeCurrent, stateCurrent);  // Service the LEDs since Stop button LED will be blinking every 500ms.  PROBABLY DON'T NEED TO DO THIS HERE ********************

      }

    // ***********************************
    // ***** REGISTRATION MODE LOGIC *****
    // ***********************************

    } else if (modeCurrent == MODE_REGISTER) {  // Run REGISTRATION mode
        // There is no STOPPING state during REGISTRATION - operator must see it through until STOPPED.
        // So we'll stay in this block the whole time, and don't need to worry about refreshing (painting) the MODE LEDs.
        sprintf(lcdString, "REGISTERING"); pLCD2004->println(lcdString); Serial.println(lcdString);

        // INITIALIZE A FRESH TRAIN PROGRESS TABLE FOR ALL 50 TRAINS.
        // LEG and other modules will be doing likewise as their own Registration begins simultaneously.
        pTrainProgress->clearAll();  // Sets every loco's Train Progress table to active == false.

        // PROMPT OPERATOR FOR GENERAL STARTUP OPTIONS: FAST/SLOW STARTUP, and SMOKE ON/OFF.
        // Operator's responses will be sent to LEG to be used during its own registration process.
        // We (MAS) don't actually care what the responses are, thus no return values.
//        pRegistrar->promptFastOrSlowStartup();
//        pRegistrar->promptSmokeOrNoSmoke();

        // NOW PROMPT OPERATOR FOR THE LOCO ID OF EVERY OCCUPIED BLOCK (SENSOR).  DONE VIA OCC A/N DISPLAY and ROTARY ENCODER.

        // Send all TOTAL_TRAINS (50) loco info (description, last-known block) to OCC in preparation for prompting.
        // We will also include each train's last-known block number, if found in the Block Reservation table.
//        pRegistrar->sendLocoNamesAndBlocksToOCC();


        // Now OCC is going to prompt the operator for the locoNum of whatever is sitting on every occupied sensor.  It will use
        // the char names, and default last-known-block number if any.
        // As each loco is registered, OCC will send us a message with the locoNum, blockNum, and direction.  All modules that
        // maintain a Train Progress table, including us (MAS) will use that to create initial Train Progress "routes" (which will
        // only consist of a single block+direction, and sensor record.

        // OCC will scan each sensor, one at a time, to see if it's occupied.  If it is occupied, it will then prompt the operator
        // for the locoNum of the train that's sitting in that corresponding block.  The operator can select starting with the
        // default loco we passed it for that block, if any, or select any other locoNum, or select STATIC.
        // OCC will then transmit occupancy data for each occupied block, one at a time, for all modules to see.
        // For blocks occupied by STATIC trains, only we (MAS) need to reserve the block.
        // But all modules, including us, need to establish a Train Progress table record for non-static active locos.



        // We (MAS) will also use this data to RESERVE those occupied blocks.

        byte locoNum = 0;
        routeElement block;
        block.routeRecVal = 0;
        block.routeRecType = BE;
        char dir = 'X';
        while (pMessage->available() != 'L') { }  // Dump everything until we get a reply from OCC
        pMessage->getOCCtoALLTrainLocations(&locoNum, &block);
        sprintf(lcdString, "T%i B%i D%c", locoNum, block.routeRecType, block.routeRecVal); pLCD2004->println(lcdString); Serial.println(lcdString);

        while (pMessage->available() != 'L') { }  // Dump everything until we get a reply from OCC
        pMessage->getOCCtoALLTrainLocations(&locoNum, &block);
        sprintf(lcdString, "T%i B%i D%c", locoNum, block.routeRecType, block.routeRecVal); pLCD2004->println(lcdString); Serial.println(lcdString);


        // Tell Mode_Dial class we're registered so it can set internal flag
        pModeSelector->setStateToSTOPPED(modeCurrent, &stateCurrent);  // Will change stateCurrent to STOPPED
        sprintf(lcdString, "REGISTRATION DONE!"); pLCD2004->println(lcdString); Serial.println(lcdString);
        pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent);  // Broadcast that we are in REGISTRATION STOPPED
    }

  }  // Else we are STOPPED and nothing to do

}

*/

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

void checkForTurnout17Problem() {
  // Rev: 03/24/23.
  // There was a problem reserving turnout 17, both individually and even if I reserved all (as above.)
  // Turnout 17 was reporting whatever the results were for Turnout 16 and I have NO IDEA WHY.  No longer happening.
  pTurnoutReservation->reserveTurnout(17, 12);  
  if ((pTurnoutReservation->reservedForTrain(17)) != 12) {
    sprintf(lcdString, "PROBa W/ TRNOUT 17!"); pLCD2004->println(lcdString); Serial.println(lcdString); while (true) { }
  }
  pTurnoutReservation->release(17);
  if ((pTurnoutReservation->reservedForTrain(17)) != LOCO_ID_NULL) {
    sprintf(lcdString, "PROBb W/ TRNOUT 17!"); pLCD2004->println(lcdString); Serial.println(lcdString); while (true) { }
  }
}

void reserveSecondLevelBlocks() {
  // Rev: 03/24/23.
  // *** UNTIL 2ND LEVEL IS BUILT, PERMANENTLY RESERVE 2ND LEVEL BLOCKS: 17-20, 22-26 ***
  // This will trigger blocks 17-20 and 22-26 to be automatically reserved for STATIC so they can't be used in an Auto/Park route.
  // Note that we'll reserve all 2nd-level blocks, not just siding blocks, although sidings would be sufficient for our purposes.
  // In fact, simply reserving blocks 17 and 18 would be enough to prevent any routes from using the second level.
  // This may be moot depending on how we handle the status of the phantom sensors on the second level.
  for (byte blockNum = 17; blockNum <=20; blockNum++) {
    pBlockReservation->reserveBlock(blockNum, BE, LOCO_ID_STATIC);
  }
  for (byte blockNum = 22; blockNum <=26; blockNum++) {
    pBlockReservation->reserveBlock(blockNum, BE, LOCO_ID_STATIC);
  }
}


/*











// *****************************************************************************************
// *** FIRST WE DEFINE FUNCTIONS UNIQUE TO THIS ARDUINO (not shared by all in any event) ***
// *****************************************************************************************

// SELECTING ROUTES (AUTO and PARK): We'll need to somehow look at SEVERAL routes -- for example there are 18 routes that start at
// BE02!  We don't always just want to accept the first one that we find.
// Maybe we need a smaller array of up to 18 elements (define MAX_MATCHING_ORIGIN_RECORDS in consts) that just stores the route
// number (record number) and maybe priority, and scan every possible route and eliminate the ones that are not possible due to
// block or turnout reservations, or levels, etc., and if there are still more than one possibilities then try to eliminate more by
// looking at deadlocks.  After that if we have more than one that will work, we can assign a weight to each route based on
// priority, but sometimes a lower-priority/lower-weight route will still get chosen.  Use some randomness for interest.

void getLastKnownTrainPosns() {  // Last-known Block Num and Direction (E/W) of every train
  // Call at the beginning of Registration to use as defaults when prompting operator for occupied sensors.
  Storage.read(FRAM_ADDR_LAST_TRAIN, FRAM_SIZE_LAST_TRAIN, FRAMDataBuf);
  for (byte i = 0; i < TOTAL_TRAINS; i++) {  // Record (train) number starting with zero
    byte j = (i * 2);  // Record location in buffer
    lastTrainPosn[i].blockNum = FRAMDataBuf[j];    // 1..26
    lastTrainPosn[i].whichDirection = FRAMDataBuf[j + 1];  // E|W
  }
}

void setLastKnownTrainPosns() {  // Last-known Block Num and Direction (E/W) of every train
  // Try to call this function whenever a train location changes, to keep FRAM up to date.
  memcpy(FRAMDataBuf, lastTrainPosn, FRAM_SIZE_LAST_TRAIN);
  Storage.write(FRAM_ADDR_LAST_TRAIN, FRAM_SIZE_LAST_TRAIN, FRAMDataBuf);
}
*/

