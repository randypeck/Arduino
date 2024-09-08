// O_LEG.INO Rev: 09/05/24.
// LEG controls physical trains via the Train Progress and Delayed Action tables, and also controls accessories.
// LEG also monitors the control panel track-power toggle switches, to turn the four PowerMasters on and off at any time.
// 04/02/24: LEG Conductor/Engineer and Train Progress will always assume that Turnouts are being thrown elsewhere and won't worry
// about it.  However we might maintain a Turnout Reservation table just to keep the Train Progress code as similar as possible
// between MAS, OCC, and LEG.
// 08/04/24: Similarly, LEG doesn't care about Block Reservations, although it needs access to the table in order to retrieve
// block default speeds and block lengths.

// 07/30/24: Added debug parms to pDelayedAction->initDelayedActionTable() and pEngineer->initLegacyCommandBuf()
// 05/30/21: Added: const unsigned long SMOKE_TIME_LIMIT = 300000;  // Automatically turn off smoke on locos after this many ms.
//           300,000ms = 5 minutes.

// IMPORTANT consideration when BACKING INTO SINGLE-ENDED SIDINGS such as 23-26: When we're stopped when the back of the train has
// tripped the siding 23-26 exit (west) sensor, we will have route records that "accelerate" us from VL00 (stop) to VL01 (crawl)
// within a block that is *less* (by the length of the train) than the block length (because the *back* of our train will be
// sitting on the sensor, rather than the front - so we won't have as far to travel until the front trips the upcoming "stop"
// sensor.)  We must accelerate and then decelerate and stop within that same block -- upon reaching the siding's "new" exit (east)
// sensor.  Probably OK to accelerate at generic medium momentum until we reach VL01 (crawl) *or* trip the stop sensor (don't
// forget to "expire" any remaining Delayed Action records that might continue to accelerate the train if we have not yet executed
// them all by the time we trip the "stop" sensor!)
// When we pull in forward-facing, tripping the "end-of-siding" sensor, and then reversing until we trip the siding entry sensor,
// it's a similar situation.

// TRAIN PROGRESS TABLE SPEEDS:
// When we see a non-zero speed command i.e. VL02, it means we should change to that speed starting at the sensor before.  This
// could be from a stop, or from some other intermediate speed i.e. VL01 to VL02, or VL03 to VL2.
// We want to use low momentum when starting a train from a stop, and when changing from one non-zero speed to another.
// When we see a zero (STOP) speed command i.e. VL00, it means we should begin slowing two sensors before the Stop command.  The
// momentum of the train's deceleration will be calculated based on the length of the stopping siding.

// When a train completes a route, we won't want to start a new route immediately.  However, MAS could actually send a new route to
// LEG at any time, and LEG can just hold off on sending it to the Delayed Action table until all previous actions (such as
// departure announcements) have been completed.

// TO DO: REGISTRATION: As each loco is registered, run fast or slow startup and then toot the horn, turn smoke on if appropriate,
//        and set Legacy built-in momentum to zero (all done via Delayed Action.)
// TO DO: AUTO (and maybe PARK): Conductor should add loco dialogue commands to Delayed Action table for Engineer to request okay
//        to depart etc.  This will be in addition to "Station" announcements controlled by OCC (departure/arrival announcements.)
// TO DO: It would be very interesting, at least in the beginning, to chirp the piezo speaker each time LEG sends a speed command
//        to Legacy as a result of the train tripping a sensor -- just so we get a sense of the delay between visually seeing the
//        train hit the sensor, and LEG actually doing something in response.  BUT BEWARE: BUZZ TAKES TIME TO SOUND!
// TO DO: AUTO: Accessory commands will originate from triggers, such as "if a local passenger train stops at a passenger station,
//        then turn on the baggage handling accessory for 30 seconds."  Likewise with freight accessories etc.

// FUTURE: INCOMING SERIAL COMMANDS FROM LEGACY BASE: If we ever want to monitor *incoming* Legacy commands via the serial
//        interface, such as to watch for FF FF FF Halt commands initiated by the CAB-2 remote, we could connect the Legacy serial
//        output wire to a SEPARATE ARDUINO that would handle the massive input coming from Legacy, watching for emergency stops.
//        That Arduino could then simply pull the Emergency Halt line low so that MAS would stop assigning new routes etc.  Though
//        no harm done, because a CAB-2 emergency stop turns off all power and thus new routes would not be able to be executed;
//        everything would be stopped anyway.
// FUTURE: To control ACCESSORIES: We might want to have a table that corresponds to each accessory relay, to know which ones we
//        should normally turn on, and which ones are to be turned on/off depending on criteria such as if a train is arriving at
//        a station, etc.  Conductor could reference this file during Auto mode.  For now we will just hard-code it.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_LEG;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "LEG 07/30/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorage = nullptr;

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage = nullptr;

// *** CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Centipede) is already in <Centipede.h> so not needed here.
// #include <Centipede.h> is already in <Train_Functions.h> so not needed here.
Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (LEG only has ONE Centipede.)

// *** TURNOUT RESERVATION TABLE CLASS (IN FRAM) ***
// 08/04/24: I don't believe LEG needs to keep track of any Turnout Reservation data.
#include <Turnout_Reservation.h>
Turnout_Reservation* pTurnoutReservation = nullptr;

// *** SENSOR-BLOCK CROSS REFERENCE TABLE CLASS (IN FRAM) ***
#include <Sensor_Block.h>
Sensor_Block* pSensorBlock = nullptr;

// *** BLOCK RESERVATION AND LOOKUP TABLE CLASS (IN FRAM) ***
#include <Block_Reservation.h>
Block_Reservation* pBlockReservation = nullptr;

// *** LOCOMOTIVE REFERENCE LOOKUP TABLE CLASS (IN FRAM) ***
// Loco Ref needed by Train Progress so we can look up loco type i.e. E/T/N/R/A
#include <Loco_Reference.h>  // Must be set up before Deadlock as Deadlock requires Loco pointer.
Loco_Reference* pLoco = nullptr;

// *** ROUTE REFERENCE TABLE CLASS (IN FRAM) ***
#include <Route_Reference.h>
Route_Reference* pRoute = nullptr;

// *** TRAIN PROGRESS TABLE CLASS (ON HEAP) ***
#include <Train_Progress.h>
Train_Progress* pTrainProgress = nullptr;

// *** DELAYED-ACTION TABLE CLASS (ON HEAP) ***
// Delayed Action requires a Train Progress pointer, so always instantiate Train Progress *before* Delayed Action
#include <Delayed_Action.h>
Delayed_Action* pDelayedAction = nullptr;

// *** ENGINEER CLASS ***
#include <Engineer.h>
Engineer* pEngineer = nullptr;

// *** CONDUCTOR CLASS ***
#include <Conductor.h>
Conductor* pConductor = nullptr;

// *** MISC CONSTANTS AND GLOBALS NEEDED BY LEG ***

// Variables that come with new Route messages
byte         locoNum          = 0;
char         extOrCont        = ROUTE_TYPE_EXTENSION;  // or ROUTE_TYPE_CONTINUATION (E/C)
unsigned int routeRecNum      = 0;  // Record number in FRAM, 0..n
unsigned int countdown        = 0;  // Number of seconds to delay before starting train on an Extension route
byte         sensorNum        = 0;
char         trippedOrCleared = SENSOR_STATUS_CLEARED;  // or SENSOR_STATUS_TRIPPED (C/T)

byte         modeCurrent      = MODE_UNDEFINED;
byte         stateCurrent     = STATE_UNDEFINED;
char         msgType          = ' ';

char fastOrSlow = ' ';  // Loco startup can be F|S
char smokeOn    = ' ';  // Smoke can be S|N
char audioOn    = ' ';  // Audio can be A|N
bool debugOn    = false;
const unsigned long SMOKE_TIME_LIMIT = 180000;  // Automatically turn off smoke on locos after this many ms.  180,000 = 3 minutes.

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();

  // *** QUADRAM HEAP MEMORY MODULE ***
  initializeQuadRAM();  // Add-on memory board provides 56,832 bytes for heap

  // *** INITIALIZE SERIAL PORTS ***
  // For LEG, we are going to connect BOTH the PC's Serial port *and* the Mini Thermal Printer to Serial 0.
  // The test bench mini printer requires 19200 baud, but the white one on the layout is 9600 baud.
  // If using the Serial monitor, be sure to set the baud rate to match that of the mini thermal printer being used.
  // 11/10/22: I can't seem to get the thermal printer to *not* print a few inches of garbage as new programs are uploaded.  Once a
  // program is uploaded, pressing the Reset button on the Mega to re-start the program does *not* produce garbage.  It happens
  // during the upload process, so there does not seem to be a way to stop it.
  // THIS ONLY HAPPENS WITH SERIAL 0; i.e. not with Serial 1/2/3.  It must be that Visual Studio is sending "Opening Port, Port
  // Open" data to the serial monitor at some default baud rate?  Even though it's set correctly and I can read it in the window.
  Serial.begin(9600);  // SERIAL0_SPEED.  9600 for White thermal printer; 19200 for Black thermal printer
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200.
  Serial3.begin(SERIAL3_SPEED);  // Lionel Legacy serial interface 9600 baud

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
  //pStorage->testFRAM();         while (true) {}  // Writes then reads random data to entire FRAM.
  //pStorage->testSRAM();         while (true) {}  // Test 4K of SRAM just for comparison.
  //pStorage->testQuadRAM(56647); while (true) {}  // (unsigned long t_numBytesToTest); max about 56,647 testable bytes.

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // *** INITIALIZE CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Centipede class hangs the system if hardware is not connected.
  Wire.begin();                               // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Centipede;             // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initializePinsForOutput();  // Set all Centipede shift register pins to OUTPUT for Accessories.

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
  pRoute->begin(pStorage);  // Assuming we'll need a pointer to the Block Reservation class

  // *** INITIALIZE TRAIN PROGRESS CLASS AND OBJECT ***  (Heap uses 14,552 bytes)
  // WARNING: TRAIN PROGRESS MUST BE INSTANTIATED *AFTER* BLOCK RESERVATION AND ROUTE REFERENCE.
  pTrainProgress = new Train_Progress;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pTrainProgress->begin(pBlockReservation, pRoute);

  // *** INITIALIZE DELAYED ACTION CLASS AND OBJECT ***  (Heap uses ??? bytes)
  // WARNING: DELAYED ACTION MUST BE INSTANTIATED *AFTER* LOCO REF AND TRAIN PROGRESS.
  pDelayedAction = new Delayed_Action;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pDelayedAction->begin(pLoco, pTrainProgress);

  // *** INITIALIZE ENGINEER CLASS AND OBJECT ***
  // WARNING: ENGINEER MUST BE INSTANTIATED *AFTER* LOCO REF, TRAIN PROGRESS, AND DELAYED ACTION.
  pEngineer = new Engineer;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pEngineer->begin(pLoco, pTrainProgress, pDelayedAction);

  // *** INITIALIZE CONDUCTOR CLASS AND OBJECT ***  We may not need this class; may handle it in the main LEG loop... **************************************
  // CONDUCTOR MUST BE INSTANTIATED *AFTER* BLOCK RES'N, LOCO REF, ROUTE REF, TRAIN PROGRESS, DELAYED ACTION, AND ENGINEER.
  pConductor = new Conductor;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pConductor->begin(pStorage, pBlockReservation, pDelayedAction, pEngineer);

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and send e-stop to Legacy

  // IMPORTANT: If a control panel track power switch is held, this function delays until 40ms after it is released,
  // so if the operator holds a power switch on while a train is moving and/or there are commands in Delayed Action,
  // no events will be seen while switch is held and sensor trips/releases may be missed.  For this reason, it's best
  // to only check for PowerMaster switches when NOT in any mode (i.e. STOPPED state.)
  checkIfPowerMasterOnOffPressed();      // Turn track power on or off if operator is holding a track-power switch.
  pEngineer->executeConductorCommand();  // Run oldest ripe command in Legacy command buffer, if possible.
  // Legacy command buffer could include PowerMaster on/off commands, or commands still remaining after completion of Registration
  // mode, i.e. slow start-up and blowing horns etc.  So just keep checking until operator selects a new mode to start.

  // **************************************************************
  // ***** SUMMARY OF MESSAGES RECEIVED & SENT BY THIS MODULE *****
  // ***** REV: 03/10/23                                      *****
  // **************************************************************
  //
  // ********** OUTGOING MESSAGES THAT LEG WILL TRANSMIT: *********
  //
  // None.
  //
  // ********** INCOMING MESSAGES THAT LEG WILL RECEIVE: **********
  //
  // MAS-to-ALL: 'M' Mode/State change message:
  // pMessage->getMAStoALLModeState(byte &mode, byte &state);
  //
  // MAS-to-ALL: 'R' Route sent by FRAM Route Record Number:  ONLY DURING AUTO/PARK MODES RUNNING/STOPPING STATE.
  // pMessage->getMAStoALLRoute(byte &locoNum, char &extOrCont, unsigned int &routeRecNum, unsigned int &countdown);
  //
  // SNS-to-ALL: 'S' Sensor status (num [1..52] and Tripped|Cleared)
  // pMessage->getSNStoALLSensorStatus(byte &sensorNum, char &trippedOrCleared);
  //
  // OCC-to-LEG: 'F' Fast startup Fast/Slow:  REGISTRATION MODE ONLY.
  // pMessage->getOCCtoLEGFastOrSlow(char &fastOrSlow);
  //
  // OCC-to-LEG: 'K' Smoke/No smoke:  REGISTRATION MODE ONLY.
  // pMessage->getOCCtoLEGSmokeOn(char &smokeOrNoSmoke);
  //
  // OCC-to-LEG: 'A' Audio/No audio:  REGISTRATION MODE ONLY.
  // pMessage->getOCCtoLEGAudioOn(char &audioOrNoAudio);
  //
  // OCC-to-LEG: 'D' Debug/No debug:  REGISTRATION MODE ONLY.
  // pMessage->getOCCtoLEGDebugOn(char &debugOrNoDebug); // [D|N]
  //
  // OCC-to-ALL: 'L' Location of just-registered train:  REGISTRATION MODE ONLY.  One rec/occupied sensor, real and static.
  // pMessage->getOCCtoALLTrainLocation(byte &locoNum, routeElement &locoLocation);  // 1..50, BE03
  // NOTE: LEG will use this to establish an initial Train Progress "Route" and startup the loco.
  //
  // **************************************************************
  // **************************************************************
  // **************************************************************

  // See if there is an incoming message that we care about.
  // If we receive a new Mode/State message, we will always assume it's a legal transition based on MAS mode/state rules.
  // Blank means no message; otherwise need to call appropriate "pMessage->get" function to retrieve incoming data.
  // We will wait until a mode starts (MANUAL, REGISTER, or AUTO/PARK) and then process that mode until the mode ends.
  msgType = pMessage->available();
  switch (msgType) {

    case ' ':  // No message; move along, nothing to see here.
    {
      break;
    }

    case 'M':  // New mode/state message in incoming RS485 buffer *** VALID IN ANY MODE ***
    {
      pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);

      if ((modeCurrent == MODE_MANUAL) && (stateCurrent == STATE_RUNNING)) {

        // Run in Manual mode until stopped.
        sprintf(lcdString, "BEGIN MAN MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        LEGManualMode();
        sprintf(lcdString, "END MAN MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Now, modeCurrent == MODE_MANUAL and stateCurrent == STATE_STOPPED

      }  // *** MANUAL MODE COMPLETE! ***

      if ((modeCurrent == MODE_REGISTER) && (stateCurrent == STATE_RUNNING)) {

        // Run in Register mode until complete.
        sprintf(lcdString, "BEGIN REG MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        LEGRegistrationMode();
        sprintf(lcdString, "END REG MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Now, modeCurrent == MODE_REGISTER and stateCurrent == STATE_STOPPED

      }  // *** REGISTER MODE COMPLETE! ***

      if (((modeCurrent == MODE_AUTO) || (modeCurrent == MODE_PARK)) && (stateCurrent == STATE_RUNNING)) {

        // Run in Auto/Park mode until complete.
        sprintf(lcdString, "BEGIN AUTO MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        LEGAutoParkMode();
        sprintf(lcdString, "END AUTO MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Now, modeCurrent == MODE_AUTO or MODE_PARK, and stateCurrent == STATE_STOPPED

      }  // *** AUTO/PARK MODE COMPLETE! ***
      break;
    }

    default:  // *** ANY OTHER MESSAGE WE SEE HERE IS A BUG ***
    {
      sprintf(lcdString, "ERR MSG TYPE %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

  }
}  // End of loop()

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

// ********************************************************************************************************************************
// **************************************************** OPERATE IN MANUAL MODE ****************************************************
// ********************************************************************************************************************************

void LEGManualMode() {  // CLEAN THIS UP SO THAT MAS/OCC/LEG ARE MORE CONSISTENT WHEN DOING THE SAME THING I.E. RETRIEVING SENSORS AND UPDATING SENSOR-BLOCK *******************************

  // When starting MANUAL mode, MAS/SNS will always immediately send status of every sensor.
  // Recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  // We don't care about sensors in LEG Manual mode, so we disregard them (but other modules will see and want.)
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {
      sprintf(lcdString, "SNS %i %i NUM ERR", i, sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", i, trippedOrCleared); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
      sprintf(lcdString, "Sensor %i Tripped", i); pLCD2004->println(lcdString); Serial.println(lcdString);
    } else {
      sprintf(lcdString, "Sensor %i Cleared", i); Serial.println(lcdString);  // Not to LCD
    }
  }
  // Okay, we've received all TOTAL_SENSORS sensor status updates (and disregarded them.)

  // Now operate in Manual until mode stopped.
  // Just watch the Emergency Stop (halt) line, and monitor for messages:
  // * Sensor updates, though nothing to do with them in Manual mode.
  // * Mode update, in which case we're done and return to the main loop.
  do {
    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and send e-stop to Legacy

    msgType = pMessage->available();  // Could be ' ', 'S', or 'M'
    if (msgType == 'S') {  // Got a sensor message in Manual mode; nothing to do though.
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
      if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
        sprintf(lcdString, "Sensor %i Tripped", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      } else {
        sprintf(lcdString, "Sensor %i Cleared", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      }
    } else if (msgType == 'M') {  // If we get a Mode message it can only be Manual Stopped.
      pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
      if ((modeCurrent != MODE_MANUAL) || (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "MAN MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to stop Manual mode.
    } else if (msgType != ' ') {  // Remember, pMessage->available() returns ' ' if there is no message.
      sprintf(lcdString, "MAN MSG ERR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
  } while (stateCurrent != STATE_STOPPED);  // We'll just assume mode is still Manual
  return;
}

// ********************************************************************************************************************************
// ************************************************ OPERATE IN REGISTRATION MODE **************************************************
// ********************************************************************************************************************************

void LEGRegistrationMode() {

  // Rev: 08-04-24.
  // Note that although LEG uses Block Res'n to retrieve block speeds and lengths, it does NOT care about block or turnout res'ns.

  // Release all block reservations.
  //pBlockReservation->releaseAllBlocks();

  // When starting REGISTRATION mode, SNS will always immediately send status of every sensor.
  // Standby and recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {
      sprintf(lcdString, "SNS %i %i NUM ERR", i, sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", i, trippedOrCleared); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

    // Store the sensor status T/C in the Sensor Block table.
    pSensorBlock->setSensorStatus(sensorNum, trippedOrCleared);

    // If the sensor is TRIPPED, then reserve the corresponding block as Static in the Block Reservation table. Later, as we prompt
    // the user for the loco ID of each occupied block, we'll update the block reservation for each non-STATIC train.
    //if (trippedOrCleared == 'T') {
    //  if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_EAST) {  // 'E'
    //    pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BE, LOCO_ID_STATIC);
    //  }
    //  else if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_WEST) {  // 'W'
    //    pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BW, LOCO_ID_STATIC);
    //  }
    //  else {  // Not E or W, we have a problem!
    //    sprintf(lcdString, "BLK END %i %c", sensorNum, pSensorBlock->whichEnd(sensorNum)); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    //  }
    //}
  }
  sprintf(lcdString, "Sensors Rec'd"); pLCD2004->println(lcdString); Serial.println(lcdString);

  // *** NOW OCC WILL PROMPT OPERATOR FOR STARTUP FAST/SLOW, SMOKE ON/OFF, AUDIO ON/OFF, DEBUG ON/OFF (and send to us) ***
  while (pMessage->available() != 'F') {}  // Wait for Fast/Slow startup message
  pMessage->getOCCtoLEGFastOrSlow(&fastOrSlow);
  if ((fastOrSlow != 'F') && (fastOrSlow != 'S')) {
    sprintf(lcdString, "FAST SLOW MSG ERR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  sprintf(lcdString, "FAST/SLOW %c", fastOrSlow); pLCD2004->println(lcdString); Serial.println(lcdString);

  while (pMessage->available() != 'K') {}  // Wait for Smoke/No Smoke message
  pMessage->getOCCtoLEGSmokeOn(&smokeOn);
  if ((smokeOn != 'S') && (smokeOn != 'N')) {
    sprintf(lcdString, "SMOKE MSG ERR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  sprintf(lcdString, "SMOKE %c", smokeOn); pLCD2004->println(lcdString); Serial.println(lcdString);

  while (pMessage->available() != 'A') {}  // Wait for Audio/No Audio message
  pMessage->getOCCtoLEGAudioOn(&audioOn);
  if ((audioOn != 'A') && (audioOn != 'N')) {
    sprintf(lcdString, "AUDIO MSG ERR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  sprintf(lcdString, "AUDIO %c", audioOn); pLCD2004->println(lcdString); Serial.println(lcdString);

  while (pMessage->available() != 'D') {}  // Wait for Debug/No Debug message
  char debugMode;  // Can be D or N
  pMessage->getOCCtoLEGDebugOn(&debugMode);
  if (debugMode == 'D') {
    debugOn = true;
  } else if (debugMode == 'N') {
    debugOn = false;
  } else {
    sprintf(lcdString, "DEBUG MSG ERR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  sprintf(lcdString, "DEBUG %c", debugMode); pLCD2004->println(lcdString); Serial.println(lcdString);

  // Initialize the Delayed Action table as we'll use it when establishing initial Train Progress loco/startup.
  pDelayedAction->initDelayedActionTable(debugOn);

  // Initialize the Legacy Command Buffer, used when transferring loco commands from Delayed Action to Legacy
  pEngineer->initLegacyCommandBuf(debugOn);

  // This is as good a place as any to release all accessory relays
  pEngineer->initAccessoryRelays();

  // Initialize the Train Progress table for every possible train (1..TOTAL_TRAINS)
  for (locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {  // i.e. 1..50 trains
    pTrainProgress->resetTrainProgress(locoNum);  // Initialize the header and no route for a single train.
  }

  // *** NOW OCC WILL PROMPT OPERATOR FOR LOCO ID OF EVERY OCCUPIED BLOCK, ONE AT A TIME (and send to us) ***
  // As each (non-STATIC) loco ID and location is received, we will update Block Reservation and establish a Train Progress record
  // and startup the loco.
  // OCC will indicate there are no more locos to be registered by sending a message with locoNum == 0 == LOCO_ID_STATIC.
  locoNum = 99;  // Anything other than zero to start with
  routeElement locoBlock;
  while (locoNum != 0) {
    while (pMessage->available() != 'L') {  // Wait for newly-registered train Location message (locoNum and blockNum)
      pEngineer->executeConductorCommand();  // Such as previously-created smoke on, etc.
    }
    pMessage->getOCCtoALLTrainLocation(&locoNum, &locoBlock);
    if (locoNum != LOCO_ID_NULL) {  // We've got a real loco to set up in Train Progress!
      // When a new train is registered, establish new Train Progress rec for this loco.
      pTrainProgress->setInitialRoute(locoNum, locoBlock);
      // And update our local Block Reservation table to change the block status reservation from STATIC to this loco.
      //pBlockReservation->reserveBlock(locoBlock.routeRecVal, locoBlock.routeRecType, locoNum);
      // Now startup the train according to our default parms fast/slow, etc.
      if (fastOrSlow == 'F') {
        pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_STARTUP_FAST, 0, 0);
      } else {  // Slow startup
        pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_STARTUP_SLOW, 0, 0);
      }
      if (smokeOn == 'S') {  // Turn on smoke.  Possible levels are 0..3
        pDelayedAction->populateLocoCommand(millis() + 500, locoNum, LEGACY_ACTION_SET_SMOKE, 3, 0);
        // If we start with smoke on, add a command to turn it off in five minutes.
        pDelayedAction->populateLocoCommand(millis() + SMOKE_TIME_LIMIT, locoNum, LEGACY_ACTION_SET_SMOKE, 0, 0);
      } else {  // No smoke
        pDelayedAction->populateLocoCommand(millis() + 500, locoNum, LEGACY_ACTION_SET_SMOKE, 0, 0);
      }

      // Adding whistle command just for fun...
      pDelayedAction->populateLocoWhistleHorn(millis() + 3000, locoNum, LEGACY_PATTERN_BACKING);

      sprintf(lcdString, "REG'D LOCO %i", locoNum); pLCD2004->println(lcdString); Serial.println(lcdString);
    } else {  // Loconum == 0 == LOCO_ID_NULL means we're done
      sprintf(lcdString, "REG'N COMPLETE!"); pLCD2004->println(lcdString); Serial.println(lcdString);
    }
  }
  // When we fall through to here, OCC Registrar has finished sending us train locations.
  // We could still have un-executed ripe Delayed Action commands created for last loco, so we will continue to call
  // pEngineer->executeConductorCommand(); upon our return to loop().

  // *** REGISTRATION COMPLETE! ***
  // We're done, so just wait for the mode-change message from MAS to exit back to main loop...
  do {
    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and send e-stop to Legacy
    msgType = pMessage->available();  // Could only be ' ' or 'M' i.e. nothing or MODE message
    if (msgType == 'M') {  // If we get a Mode message it can only be Registration Stopped.
      pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
      if ((modeCurrent != MODE_REGISTER) || (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "REG MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to stop Registration mode.
    } else if (msgType != ' ') {  // Remember, pMessage->available() returns ' ' if there is no message.
      // WE HAVE A BIG PROBLEM with an unexpected message type at this point
      sprintf(lcdString, "REG MODE MSG ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
  } while (stateCurrent != STATE_STOPPED);  // We'll just assume mode is still Register
  return;
}

// ********************************************************************************************************************************
// ************************************************** OPERATE IN AUTO/PARK MODE ***************************************************
// ********************************************************************************************************************************

void LEGAutoParkMode() {

  // Rev: 09-08-24.
  // NOTE 09-02-24: MAS/OCC/LEG need to pay attention to how/when isParked, isStopped, timeStopped, timeToStart, currentSpeed,
  //   currentSpeedTime, and expectedStopTime are updated -- as I'm not yet confident they're all being updated appropriately.
  // Note that although LEG uses Block Res'n to retrieve block speeds and lengths, it does NOT care about block or turnout res'ns.
  // Upon entry here, modeCurrent == AUTO or PARK, and stateCurrent == RUNNING.
  // Mode may change from AUTO to PARK, and state may change from RUNNING to STOPPING, but OCC and LEG don't care and won't even
  // see those RS-485 messages.
  // So just run until we receive a Mode message (will still be AUTO or PARK) with a new state of STOPPED.

  // Note that when we're stopped, either in our initial Registration block or at the end of a route, before a train can start
  // moving, MAS must first obviously send a new Extension Route message - but that alone won't cause the train to move, and in
  // fact it will not be appropriate for a train to move until its Train Progress timeToStart field has expired.  So in addition
  // to sending a new Route message (which includes timeToStart,) MAS must regularly check the timeToStart field in its own Train
  // Progress header.  When it's time to get a train moving, MAS (not LEG) triggers the movement by sending a "fake" Sensor Trip
  // message to OCC and LEG for the sensor the loco is stopped on (which will always be Next-To-Trip.)
  // OCC and LEG will get the message that the train is "moving" and LEG will respond to the "trip" by sending FD/RD and VL##
  // commands to loco, based on subsequent Train Progress route elements.
  // So outside of the "check for messages" code, MAS, OCC, and LEG should all monitor Train Progress timeToStart:
  //   MAS uses this to know when to send a "fake" sensor trip message to get a train moving.
  //   OCC and LEG can use it to make pre-departure station announcements and tower comms seconds before scheduled departure.



  // THERE ARE ONLY THREE INCOMING MESSAGES LEG WILL BE SEEING:

  // 'R' Route: Extension (stopped) or Continuation (moving.)  Rev: 09-06-24.

  //   Get new Route (locoNum, extOrCont, routeRecNum, countdown.)
  //   Create a temp variable to note value of stopPtr.  We'll need it later.
  //   Call Train Progress addExtensionRoute() or addContinuationRoute() to add elements of the new Route to Train Progress (incl.
  //     update pointers, isParked, isStopped, timeToStart.)
  //   For every element from our old stopPtr (noted above) through our new stopPtr:
  //     MAS & OCC: Reserve each new Block in the Route in Block Reservation as Reserved for this loco.
  //       Reminder: OCC only tracks block res'ns for Reg'n to use as default loco for occupied blocks.
  //     MAS: Reserve each new Turnout in the Route in Turnout Reservation as Reserved for this loco.
  //   OCC: Re-paint Control Panel RED/BLUE BLOCK OCCUPANCY LEDs to show the new Blocks as RED/Reserved or BLUE/Occupied.
  //     No need to re-paint the white occupancy sensor LEDs when a new Route is received as they won't have changed.

  // 'S' Sensor Trip/Clear.

  //   Get the Sensor Trip/Clear message: sensorNum and Tripped/Cleared.

  //   *** SENSOR TRIP ***  Rev: 09-08-24.

  //     Get the locoNum that tripped the sensor.

  //     IF WE TRIPPED THE STOP SENSOR (END OF ROUTE):

  //       None of the 8 pointers need to be updated (CONT, STATION, CRAWL, STOP, TAIL, CLEAR, TRIP, or HEAD.
  //       Next element will always be VL00, followed by HEAD.
  //       LEG: Populate Delayed Action to stop the loco (in 3 seconds, from Crawl.)
  //       LEG: Delay 3 seconds, turn off bell, delay .5 seconds, blow horn LEGACY_PATTERN_STOPPED
  //       OCC/LEG: Delay as needed, make "has arrived" announcement.
  //       MAS/OCC/LEG: Update Train Progress isParked = true IFF block is Block Res'n "parking siding."
  //       MAS/OCC/LEG: Update Train Progress isStopped = true @ millis() + 3000 (due to 3-second stop time from tripping sensor.)
  //       LEG: Train Progress currentSpeed and currentSpeedTime will be written automatically by Engineer.

  //     ELSE (IF WE TRIPPED ANY SENSOR OTHER THAN STOP SENSOR (END-OF-ROUTE)):

  //       Since we know we didn't just trip the STOP sensor, we are guaranteed to have another sensor ahead in the route, and
  //         this will become the new Next-To-Trip sensor.
  //       IF WE TRIPPED THE CONTINUATION sensor: (we may or may not stop ahead)
  //         MAS: Decide if we want to assign a Continuation (not stopping) route for this train; do so if desired.
  //         OCC & LEG: Ignore.
  //       IF WE TRIPPED THE STATION STOP sensor: (means we are for sure stopping)
  //         OCC and LEG: Make announcement (via PA, via loco.)  Or wait until we trip the CRAWL sensor.
  //       IF WE TRIPPED THE CRAWL sensor:
  //         LEG: Turn on bell, make arrival announcement on loco.
  //         LEG: Blow horn LEGACY_PATTERN_APPROACHING
  //         OCC: Make arrival announcement at station.

  //       Now, FOR ALL SENSOR TRIPS EXCEPT STOP SENSOR, including the above CONT, STATION, CRAWL:
  //       Process each T.P. route element from first element following just-tripped sensor to next SN sensor record:
  //         FD00/RD00: LEG: Confirm we are stopped or will be stopped within 3 seconds (fatal error if not,) add delay to allow
  //                         stop time, then set loco direction via Delayed Action.
  //         VL00:    : LEG: Populate D.A. slowToStop (3-second stop.)
  //         VL01:    : LEG: If SLOWING to VL01, populate D.A. to change loco speed, but consider block length.
  //                         If STOPPED and FD00, blow horn LEGACY_PATTERN_DEPARTING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //                         If STOPPED and RD00, blow horn LEGACY_PATTERN_BACKING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //         VL02..04 : LEG: If STOPPED and FD00, blow horn LEGACY_PATTERN_DEPARTING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //                         ELSE If STOPPED and RD00, blow horn LEGACY_PATTERN_BACKING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //                         ELSE If NOT STOPPED, populateLocoSpeedChange at Med momentum.
  //         BE##/BW##: OCC: Change status of block from Reserved to Occupied (automatic in paintAllBlockOccupancyLEDs.)
  //                    LEG: May reference for slowToCrawl() when appropriate.
  //         TN##/TR##: MAS: Throw the turnout (per new rule as of 09-08-24.)
  //         SN##     : ALL: Set Next-To-Trip sensor to this sensor element number.
  //       Note regarding VL00 + FD00/RD00 + VL##: It's possible we'll encounter a VL00 to stop the loco, then RD or FD to reverse,
  //         then VL## to accelerate.  LEG won't be able to rely on the Train Progress currentSpeed when assigning the second set
  //         of speed commands!  However, we should always be at Crawl when encountering the VL00 command that preceeds the FD/RD
  //         command, so we'll assume it will always take 3 seconds for the loco to slow from Crawl to Stop, and maybe add a little
  //         more time and throw in a whistle/horn "reverse" or "forward" pattern.
  //       ALL: Advance Next-To-Trip pointer to next SN sensor record.
  //       ALL: Update Train Progress isParked = false.
  //       ALL: Update Train Progress isStopped = false.

  //   *** SENSOR CLEAR ***  Rev: 09-06-24.

  //     Get the locoNum that cleared the sensor.
  //       Note there will always be a subsequent sensor ahead of any sensor just cleared, even at the end of a route.

  //     Process each T.P. route element from "old" Tail through "old" (just cleared) Next-To-Clear sensor:
  //       FD00/RD00: ALL: Ignore.
  //       VL##     : ALL: Ignore.
  //       BE##/BW##: MAS, OCC: IFF block does not recur ahead in route, release block reservation.
  //       TN##/TR##: MAS: IFF turnout does not recur ahead in route, release turnout reservation.
  //       SN##     : ALL: Update T.P. Tail = old Next-To-Clear.
  //                       Update T.P. Next-To-Clear = sensorNum just encountered.

  //   *** EITHER TRIP OR CLEAR ***  Rev: 09-06-24.

  //     OCC: Update internal Occupancy LED array that keeps track of current sensor status, to reflect the change.
  //     OCC: Re-paint the Control Panel WHITE OCCUPANCY SENSOR LEDs
  //     OCC: Re-paint the Control Panel RED/BLUE BLOCK OCCUPANCY LEDs.

  // 'M' Mode/State change.  Rev: 09-06-24.
  //     Which for us will only be changing from Auto or Park Running or Stopping to Auto or Park Stopped.

  //   If new mode != MODE_AUTO and new mode != MODE_PARK, or new state != STATE_STOPPED, then fatal error.
  //   Else exit back to main loop.

// ********************************************************************************************************************************



// Left off here 9/6/24 *********************************************************************************************************************************************


  do {  // Operate in Auto/Park until mode == STOPPED

    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and send e-stop to Legacy
    pEngineer->executeConductorCommand();  // Run oldest ripe command in Legacy command buffer, if possible.

    // SPECIAL CONSIDERATION: Check to see if a stopped loco should start moving.  If a loco is stopped, by definition we can only
    // received an Extension route (for a stopped loco.)  The loco could be freshly Registered (sitting in the initial block), or
    // stopped following completion of a previous Route.  In either case, the new Route will have a countDown ms to delay before
    // starting the loco moving.  Thus, after the Route has been added to Train Progress, we'll need to periodically call
    // pTrainProgress->timeToStartLoco(locoNum) which returns true or false.
    // NOTE: When a train is Registered, timeToStartLoco is set to > 1 day, so will only set a "sooner" time to start after we have
    // assigned a new Route (no need to confirm that there is a Route ahead of a train that has reached it's time to start.)
// HOWEVER, when a train finishes a route (stops) then we need to always reset timetostart to be 99999999 again.
    // TO START A STOPPED LOCO:
    //   1. isActive() and isStopped() and timeToStartLoco(locoNum) must all be true, AND SIGNIFICANTLY
    //   2. There must be a Route ahead of the train: pTrainProgress->atEndOfRoute(locoNum) should be false
    // This is the ONLY way to get a stopped train moving!
    for (locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {
      if ((pTrainProgress->isActive(locoNum)) &&               // We are active
          (pTrainProgress->isStopped(locoNum)) &&              // We are stopped
          (pTrainProgress->atEndOfRoute(locoNum) == false) &&  // We are NOT at the end of the route (there is route ahead)
          (pTrainProgress->timeToStart(locoNum))) {            // We can start any time now
        // locoNum is stopped but ready to start moving on it's route!
        // We will NO LONGER be sitting on the nextToTripPtr sensor as it will have been advanced when new route was added.
        // Don't forget to also blow the horn, make an announcement, etc.


// ADD CODE HERE...MAYBE SAME LOGIC AS WHEN WE TRIP A SENSOR???  I.e. FD00/RD00, VLxx through the next sensor...


      }  // End of "we have started a stopped train"
    }  // End of "for each locoNum"
    // Okay, if there were any stopped trains that needed to be started, we've got them moving (via Delayed Action records.)

    // See if there is an incoming message for us...could be ' ', Sensor, Route, or Mode message (others ignored)
    msgType = pMessage->available();  // msgType ' ' (blank) indicates no message

    if (msgType == 'R') {  // We've got a new Route to add to Train Progress.  Could be Continuation or Extension.

// Test with a new route that begins in Reverse as well as with a Continuation route that should change the penultimate VL (VL01)
// command to the block's default speed for that direction, and adds records and updates the pointers as expected. *****************************************************

      // First, get the incoming Route message which we know is waiting for us...
      pMessage->getMAStoALLRoute(&locoNum, &extOrCont, &routeRecNum, &countdown);

      // Debug code to dump Train Progress for locoNum, before adding the Extension or Continuation route
      sprintf(lcdString, "T.P. loco %i BEFORE", locoNum); Serial.println(lcdString);
      pTrainProgress->display(locoNum);

      //   Add elements of the new Route to Train Progress
      if (extOrCont == ROUTE_TYPE_EXTENSION) {
        // Add this extension route to Train Progress (our train is currently stopped.)
        pTrainProgress->addExtensionRoute(locoNum, routeRecNum, countdown);
      } else if (extOrCont == ROUTE_TYPE_CONTINUATION) {
        // Add this continuation route to Train Progress (our train is currently moving.)
        pTrainProgress->addContinuationRoute(locoNum, routeRecNum);
      }

      // Debug code to dump Train Progress for locoNum, after adding the Extension or Continuation route
      sprintf(lcdString, "T.P. loco %i AFTER", locoNum); Serial.println(lcdString);
      pTrainProgress->display(locoNum);

    }  // End of "we received a new Route" message

    else if (msgType == 'S') {  // Got a Sensor-change message in Auto/Park mode

      // Get the Sensor Trip/Clear sensorNum and Tripped/Cleared.
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);

      if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {  // This is where the excitement happens!

        // Which loco tripped the sensor?
        locoNum = pTrainProgress->locoThatTrippedSensor(sensorNum);  // Will also update lastTrippedPtr to this sensor

// NOTE: If we've just tripped the CRAWL sensor, call pTrainProgress->currentSpeed(locoNum) and confirm that the current speed is equal to what we
// think our current speed should be -- which will be equal to the most recent VL## command which should be equal to exactly our
// Low, Med, or High speed for this loco (unless we are already at Crawl.)
// But do we send the slow-down comnmands to Delayed Action the moment we trip the CRAWL pointer, or after the delay?
// We can't know the loco's actual speed at the moment the slow-down commands begin unless we wait to populate D.A. after delay.

// NOTE: We have a function trainProgress->isStopped(locoNum) that Engineer sets True whenever speed is set to zero.
// But this could be even when stopping to reverse direction; not necessarily when stopped at the end of a route.
// Thus we need a different, more sophisticated function that will tell us if the loco is stopped at the end of a route or not.

        // If we just tripped the STATION sensor, we're guaranteed that we're going to stop ahead.  It's possible that we may stop
        // and reverse direction and crawl to the Stop sensor, but won't exceed Crawl speed once we slow down and it seems like the
        // appropriate place to make an arrival (i.e. station) announcement.
        // Note that we should also turn on the bell at this point.
        pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_DIALOGUE, LEGACY_DIALOGUE_E2T_ARRIVING , 0);
        pDelayedAction->populateLocoCommand(millis() + 3000, locoNum, LEGACY_SOUND_BELL_ON, 0 , 0);





        // If we're not already pointing to the STOP sensor, advance Next-To-Trip pointer until we find the next sensor, which will
        // become the new Next-To-Trip sensor.  As we move our pointer forward, handle each route element as necessary.

// *** WAIT A MINUTE, even if we just tripped the STOP sensor, we still want to follow the next element = VL00 to stop *********************************
// Maybe our test should be, advance pointer until we come to the next sensor OR headPtr ****************************************************


// ALSO remember that after we trip a sensor, we could have VL00 to stop, then RD00 to reverse, then VL01/02/03/04 to accelerate,
// BEFORE tripping the next sensor.  We'll need to deal with the timing so we stop and reverse before we begin accelerating.
// This will be an important thing to test as part of a route, when I'm running just a single loco to test initial trials.

// 8/5/24 QUESTION: Should we delete the currentSpeedTime field?  We have timeStopped which we can use to know how long we've been
// stopped -- but is there any use to know at what time we set the current loco's speed????
// ANSWER: Maybe if we find that we haven't reached our target incoming speed when we must begin slowing down to Crawl, we can
// compare the time that the incorrect incoming speed was set -- but that would typically be at the same moment we're checking it
// as that's when we'll want to start slowing down...
// OTOH we populate Delayed Action with the slow-down commands starting at some point in the future -- as we don't typically
// start slowing as soon as we trip the block's entry sensor -- so we only know if we've reached our target speed at the time we
// trip the entry sensor, but it's possible we might yet reach our target speed by the time we start slowing down.
// And our populate speed change command is based on our current speed -- so actually I'm confused -- do we populate the slow-down
// commands when we trip the entry sensor, or after waiting our delay time??? *********************************************************************************

        // First check if we're at the end of the Route (i.e. just tripped the STOP sensor) in which case we can't advance.
        if ((pTrainProgress->atEndOfRoute(locoNum) == false)) {  // If we didn't just trip the route's Stop sensor
          byte tempTPPointer = pTrainProgress->nextToTripPtr(locoNum);  // Element number, not a sensor number, just tripped
          routeElement tempTPElement;  // Working/scratch Train Progress element
          do {
            // Advance pointer to the next element in this loco's Train Progress table until we reach the next sensor...
            tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);
            tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);

            // If there is anything to do with this Train Progress element, do it here.  I.e. transfer commands to the Delayed
            // Action table for almost-immediate execution.
            // FD00/RD00 and VL## are the only things I can think of that LEG cares about here.
            // These commands can populate without delay, as they will be inserted into Delayed Action in the time-order rec'd.
            if (tempTPElement.routeRecType == FD) {
              pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_FORWARD, 0, 0);
            } else if (tempTPElement.routeRecType == RD) {
              pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_REVERSE, 0, 0);
            } else if (tempTPElement.routeRecType == VL) {  // VL00 = Stop, VL01 = Crawl, VL02-VL04 = Low/Med/High speed
              if (tempTPElement.routeRecVal == 0) {  // Stop, hopefully from Crawl or close to it
                pDelayedAction->populateLocoSlowToStop(locoNum);  // This will stop us from current speed to stopped in 3 seconds.
                // If we're at the end of a route i.e. at a station, toot the horn and make an announcement.
                // However if we're just stopping to reverse direction, DON'T do that.

// **** NEED A TEST HERE TO CONFIRM IF WE'RE AT A STATION STOP VERSUS JUST STOPPING TO REVERSE DIRECTION ***********************
// THIS IS HARDER THAN IT SOUNDS -- we will slow to Crawl before stopping to Reverse, and we'll also slow to a Crawl before station stop.
// SOLUTION: Turn on bell and make arrival announcement each time we trip STATION pointer sensor.






                if (WE ARE AT A STATION STOP == TRUE) {
                  // Turn off the bell
                  pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_SOUND_BELL_OFF, 0 , 0);
                  // After 4 seconds (1 second after stopping), toot the horn
                  pDelayedAction->populateLocoWhistleHorn((millis() + 4000), locoNum, LEGACY_PATTERN_STOPPED);  // Single short toot
                  // Now some dialogue that we have arrived: LEGACY_DIALOGUE_E2T_HAVE_ARRIVED
                  pDelayedAction->populateLocoCommand((millis() + 6000), locoNum, LEGACY_DIALOGUE, LEGACY_DIALOGUE_E2T_HAVE_ARRIVED, 0);
                }
              } else if (tempTPElement.routeRecVal == 1) {  // If target speed is Crawl, special case if we're moving > Crawl
                if ((pTrainProgress->currentSpeed(locoNum)) > 1) {  // If we're moving > Crawl, calculate slow to Crawl parms.
                  // This requires us to look up slow-down parms in Loco Ref and block length in Block Res'n.
                  // But ONLY if our current speed is > Crawl; i.e. Low/Med/High to Crawl.
                  // We won't want to handle a speed change from Stop to Crawl here; treat that like any other speed change.

// Here is our fancy logic where we calculate the distance needed to travel from current speed to Crawl speed, then look up the
// length of the block we're in, and figure out how long to continue at our current rate of speed before beginning to slow down.
                  // First figure out steps, step delay, and distance required to slow from current speed to Crawl.
                  // Then figure out the block length
                  // Finally figure out how long to continue at current speed before beginning to slow down.

                  // Also let's turn on our bell, and make an announcement LEGACY_DIALOGUE_E2T_ARRIVING.
                  // Let's do this at the time we begin slowing, rather than as soon as we trip the sensor.


                }
              } else {  // Any other speed change (i.e. not stopping, and not slowing to Crawl from some higher speed.)
                // This could be ANY acceleration (even Stop to Crawl), or declerating from High or Medium to Medium or Low.
                // Since this isn't slow to Crawl or stop, we'll use the loco's medium momentum parms to speed up or slow down.
                byte tempSpeedSteps = pLoco->medSpeedSteps(locoNum);
                unsigned int tempStepDelay = pLoco->medMsStepDelay(locoNum);
                // populateLocoSpeedChange() will automatically retrieve the current/incoming speed.
                pDelayedAction->populateLocoSpeedChange(millis(), locoNum, tempSpeedSteps, tempStepDelay, tempTPElement.routeRecVal);
              }


          } while (tempTPElement.routeRecType != SN);  // Watching for the new nextToTrip sensor.
          // We've reached the next sensor, so set it to be the new Next-To-Trip sensor...
          pTrainProgress->setNextToTripPtr(locoNum, tempTPPointer);
        }
        // Now the next-to-trip pointer is up to date; whether it was already at the end of the route, or if we advanced it.





  //   Sensor CLEAR:
  //     Advance Next-To-Clear pointer if possible.

















    }  // End of "we received a Senor tripped or cleared" message

    else if (msgType == 'M') {  // If we get a Mode message it can only be Auto/Park Stopped and we're done here.

      // Message class will have filtered out Mode Auto/Park, State STOPPING for OCC because it's irrelevant.
      // We'll just check to be sure...
      if (((modeCurrent != MODE_AUTO) && (modeCurrent != MODE_PARK)) ||
           (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "AUTO MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to STOP Auto/Park mode.  It must mean all locos are stopped.
      // We will just fall out of the loop below since stateCurrent is now STATE_STOPPED
    }

    else if (msgType != ' ') {  // AT this point, the only other valid response from pMessage->available() is BLANK (no message.)
      // Any message type other than S, R, M, or Blank means a serious bug.
      sprintf(lcdString, "AUTO MODE MSG ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

  } while (stateCurrent != STATE_STOPPED);

  return;
}

// ********************************************************************************************************************************
// ********************************************************************************************************************************
// ********************************************************************************************************************************

void checkIfPowerMasterOnOffPressed() {
  // Rev: 05/13/24.
  // When operator holds one of the four PowerMaster toggle switches in either the on or off position, create a Delayed Action
  // command to be executed as soon as possible.  This function handles debounce for both press and release, and pauses until the
  // operator has released the switch.
  // Although we insert a new Delayed Action record ready for immediate execution, WE DO NOT EXECUTE THE COMMAND.  Instead, the
  // caller must do the usual Engineer::executeConductorCommand() to retrieve the Delayed Action record and send it on to the
  // Legacy Command Buffer / Legacy base.
  // May 2024 we had some problems with the PowerMaster toggles reading as on or off the first time they were read, but then worked
  // fine after that and I can't recall what changed.  But this MAY have been related to the fact that I changed the PowerMaster
  // toggle switch input pins from digital inputs to analog inputs, and perhaps I was not configuring them as digital pins
  // correctly or in the proper order.
  // If this recurs, a possible solution per ChatGPT: "Performing an initial pin read in the setup() function and discarding the
  // result can help stabilize an input pin. This action essentially "settles" the pin's state, allowing any initial noise to
  // dissipate before your program starts its regular operation. After this initialization step, subsequent reads should be more
  // reliable, as the pin's state is less likely to be influenced by transient noise. This approach is commonly used to ensure
  // consistent behavior in digital input applications."
  // Another possible fix would be to insert a 1ms delay between reading pins.

  const byte POWERMASTER_OFF = 0;  // Turn PowerMaster #n OFF with Legacy Engine # Absolute Speed 0.
  const byte POWERMASTER_ON  = 1;  // Turn PowerMaster #n ON  with Legacy Engine # Absolute Speed 1.

  // Check the four control panel "PowerMaster" on/off switches and turn power on or off as needed via Legacy commands.
  // Note that the four power LEDs are driven directly by track power, so no need to turn them on or off in code.
  // Create Delayed Action cmd to set Legacy Engine LOCO_ID_POWERMASTER_n to Absolute Speed POWERMASTER_OFF or POWERMASTER_ON.
  // This amounts to Legacy Engine 91..94 to Abs Speed 0 or 1.
  // There is custom code in Delayed_Action::populateDelayedAction() to handle PowerMasters since their locoNum is out of range.

  // Is the operator holding the control panel BROWN TRACK POWER switch in the ON position at this moment?
  if (digitalRead(PIN_IN_PANEL_BROWN_ON) == LOW) {
    // Insert a Delayed Action command to execute PowerMaster #1 "on" as soon as possible.
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_1, LEGACY_ACTION_ABS_SPEED, POWERMASTER_ON, 0);
    // Note that this command won't be executed until operator RELEASES the switch AND we call Engineer::executeConductorCommand().
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_BROWN_ON) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  // Continue checking all other Track Power switches/positions...
  if (digitalRead(PIN_IN_PANEL_BROWN_OFF) == LOW) {
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_1, LEGACY_ACTION_ABS_SPEED, POWERMASTER_OFF, 0);
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_BROWN_OFF) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  if (digitalRead(PIN_IN_PANEL_BLUE_ON) == LOW) {
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_2, LEGACY_ACTION_ABS_SPEED, POWERMASTER_ON, 0);
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_BLUE_ON) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  if (digitalRead(PIN_IN_PANEL_BLUE_OFF) == LOW) {
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_2, LEGACY_ACTION_ABS_SPEED, POWERMASTER_OFF, 0);
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_BLUE_OFF) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  if (digitalRead(PIN_IN_PANEL_RED_ON) == LOW) {
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_3, LEGACY_ACTION_ABS_SPEED, POWERMASTER_ON, 0);
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_RED_ON) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  if (digitalRead(PIN_IN_PANEL_RED_OFF) == LOW) {
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_3, LEGACY_ACTION_ABS_SPEED, POWERMASTER_OFF, 0);
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_RED_OFF) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  if (digitalRead(PIN_IN_PANEL_4_ON) == LOW) {
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_4, LEGACY_ACTION_ABS_SPEED, POWERMASTER_ON, 0);
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_4_ON) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  if (digitalRead(PIN_IN_PANEL_4_OFF) == LOW) {
    pDelayedAction->populateLocoCommand(millis(), LOCO_ID_POWERMASTER_4, LEGACY_ACTION_ABS_SPEED, POWERMASTER_OFF, 0);
    delay(20);  // wait for press debounce
    while (digitalRead(PIN_IN_PANEL_4_OFF) == LOW) {}   // Pause everything while held
    delay(20);  // wait for release debounce
  }
  return;
}

