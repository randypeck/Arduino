// O_LEG.INO Rev: 06/21/24.
// LEG controls physical trains via the Train Progress and Delayed Action tables, and also controls accessories.
// LEG also monitors the control panel track-power toggle switches, to turn the four PowerMasters on and off at any time.
// 04/02/24: LEG Conductor/Engineer and Train Progress will always assume that Turnouts are being thrown elsewhere and won't worry
// about it.  However we might maintain a Turnout Reservation table just to keep the Train Progress code as similar as possible
// between MAS, OCC, and LEG.

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

// LOCO CURRENT SPEED: It would be very difficult to keep track of the loco's actual speed in Train Progress at all times, since
// Conductor maintains Train Progress, but Engineer has the information (as it reads Delayed Action speed commands) yet does not
// interact with Train Progress.  Thus we will just track the *target* speed of each loco, and the Routes must guarantee that there
// is sufficient time for a loco to reach the target speed, using low momentum, by the time it trips a sensor where it must begin
// slowing to a stop.
// On the other hand, Engineer *could* update Train Progress currentSpeed and currentSpeedTime for each loco each time a speed
// command was sent to Legacy, but that's lots of writes and also would decrease encapsulation and increase coupling with Engineer.

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
char lcdString[LCD_WIDTH + 1] = "LEG 06/21/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

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

// *** TURNOUT RESERVATION TABLE CLASS (IN FRAM) ***      ***** NOT SURE IF LEG NEEDS TURNOUT RES'NS?  MAYBE JUST TO KEEP TRAIN PROGRESS LOGIC THE SAME FOR MAS, OCC, and LEG
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
char debugOn    = ' ';  // Debug can be D|N
const unsigned long SMOKE_TIME_LIMIT = 300000;  // Automatically turn off smoke on locos after this many ms.  300,000 = 5 minutes.

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

  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop

  // IMPORTANT: If a control panel track power switch is held, this function delays until 40ms after it is released,
  // so if the operator holds a power switch on while a train is moving and/or there are commands in Delayed Action,
  // no events will be seen while switch is held and sensor trips/releases may be missed.  For this reason, it's best
  // to only check for PowerMaster switches when NOT in any mode (i.e. stopped state.)
  checkIfPowerMasterOnOffPressed();  // Turn track power on or off if operator is holding a track-power switch.
  pEngineer->executeConductorCommand();  // Run oldest ripe command in Legacy command buffer, if possible.

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
  // pMessage->getOCCtoLEGDebugOn(char &debugOrNoDebug);
  //
  // OCC-to-ALL: 'L' Location of just-registered train.  One rec/occupied sensor, real and static.  REGISTRATION MODE ONLY.
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
        LEGManualMode();
        // Now, modeCurrent == MODE_MANUAL and stateCurrent == STATE_STOPPED

      }  // *** MANUAL MODE COMPLETE! ***

      if ((modeCurrent == MODE_REGISTER) && (stateCurrent == STATE_RUNNING)) {

        // Run in Register mode until complete.
        LEGRegistrationMode();
        // Now, modeCurrent == MODE_REGISTER and stateCurrent == STATE_STOPPED

      }  // *** REGISTER MODE COMPLETE! ***

      if (((modeCurrent == MODE_AUTO) || (modeCurrent == MODE_PARK)) && (stateCurrent == STATE_RUNNING)) {

        // Run in Auto/Park mode until complete.
        LEGAutoParkMode();
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

  sprintf(lcdString, "BEGIN MAN MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
  // When starting MANUAL mode, MAS/SNS will always immediately send status of every sensor.
  // Recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  // We don't care about sensors in LEG Manual mode, so we disregard them (but other modules will see and want.)
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {
      sprintf(lcdString, "SNS %i %i NUM ERR", i, sensorNum); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", i, trippedOrCleared); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }
  }
  // Okay, we've received all TOTAL_SENSORS sensor status updates (and disregarded them.)

  // Now operate in Manual until mode stopped.
  // Just watch the Emergency Stop (halt) line, and monitor for messages:
  // * Sensor updates, though nothing to do with them in Manual mode.
  // * Mode update, in which case we're done and return to the main loop.
  do {
    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop
    msgType = pMessage->available();  // Could be ' ', 'S', or 'M'
    if (msgType == 'S') {  // Got a sensor message in Manual mode; nothing to do though.
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
      if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
        sprintf(lcdString, "Sensor %2d Tripped.", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      } else {
        sprintf(lcdString, "Sensor %2d Cleared.", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
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
  sprintf(lcdString, "END MAN MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
  return;
}

// ********************************************************************************************************************************
// ************************************************ OPERATE IN REGISTRATION MODE **************************************************
// ********************************************************************************************************************************

void LEGRegistrationMode() {
  // Initialize Delayed Action, Legacy Command Buffer, Release all Block Reservations, and release accessory relays
  // Initialize the Train Progress table
  // Receive the status T/C of each sensor. For each sensor retrieved:
  //   Store sensor status T/C in Sensor-Block table for later use.
  //   If sensor is Tripped, reserve sensor's block in Block Res'n table.
  // Receive startup parms incl. Fast/Slow, Smoke On/Off, Audio On/Off, Debug On/Off and store results.
  // Receive list of active locos and which blocks they occupy.
  //   For each occupied block identified by operator, IF THE LOCO IS REAL (static already accounted for):
  //     Update Block Reservation to reflect that loco (will overwrite previous status as reserved for STATIC.)
  //     Create new entry in Train Progress for this new loco.
  //     Startup the train

  sprintf(lcdString, "BEGIN REG MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);

  // Initialize the Delayed Action table as we'll use it when establishing initial Train Progress loco/startup.
  pDelayedAction->initDelayedActionTable();

  // Initialize the Legacy Command Buffer, used when transferring loco commands from Delayed Action to Legacy
  pEngineer->initLegacyCommandBuf();

  // This is as good a place as any to release all accessory relays
  pEngineer->initAccessoryRelays();
sprintf(lcdString, "1"); pLCD2004->println(lcdString);

  // Release all block reservations.
  pBlockReservation->releaseAllBlocks();
sprintf(lcdString, "2"); pLCD2004->println(lcdString);

  // Initialize the Train Progress table for every possible train (1..TOTAL_TRAINS)
  for (locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {  // i.e. 1..50 trains
    pTrainProgress->resetTrainProgress(locoNum);  // Initialize the header and no route for a single train.
  }
sprintf(lcdString, "3"); pLCD2004->println(lcdString);

  // When starting REGISTRATION mode, SNS will always immediately send status of every sensor.
  // Standby and recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {
      sprintf(lcdString, "SNS %i %i NUM ERR", i, sensorNum); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", i, trippedOrCleared); pLCD2004->println(lcdString); endWithFlashingLED(1);
    }
    sprintf(lcdString, "SNS %i %c", sensorNum, trippedOrCleared); pLCD2004->println(lcdString); Serial.println(lcdString);

    // Store the sensor status T/C in the Sensor Block table.
    pSensorBlock->setSensorStatus(sensorNum, trippedOrCleared);

    // If the sensor is TRIPPED, then reserve the corresponding block as Static in the Block Reservation table. Later, as we prompt
    // the user for the loco ID of each occupied block, we'll update the block reservation for each non-STATIC train.
    if (trippedOrCleared == 'T') {
      if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_EAST) {  // 'E'
        pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BE, LOCO_ID_STATIC);
      }
      else if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_WEST) {  // 'W'
        pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BW, LOCO_ID_STATIC);
      }
      else {  // Not E or W, we have a problem!
        sprintf(lcdString, "BLK END %i %c", sensorNum, pSensorBlock->whichEnd(sensorNum)); pLCD2004->println(lcdString); endWithFlashingLED(1);
      }
    }
  }

  // *** NOW OCC WILL PROMPT OPERATOR FOR STARTUP FAST/SLOW, SMOKE ON/OFF, AUDIO ON/OFF, DEBUG ON/OFF (and send to us) ***
  sprintf(lcdString, "Waiting FAST SLOW"); pLCD2004->println(lcdString); Serial.println(lcdString);
  while (pMessage->available() != 'F') {}  // Wait for Fast/Slow startup message
  sprintf(lcdString, "Got FAST SLOW"); pLCD2004->println(lcdString); Serial.println(lcdString);
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
  pMessage->getOCCtoLEGDebugOn(&debugOn);
  if ((debugOn != 'D') && (debugOn != 'N')) {
    sprintf(lcdString, "DEBUG MSG ERR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  sprintf(lcdString, "DEBUG %c", debugOn); pLCD2004->println(lcdString); Serial.println(lcdString);

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
      pBlockReservation->reserveBlock(locoBlock.routeRecVal, locoBlock.routeRecType, locoNum);
      // Now startup the train according to our default parms fast/slow, etc.
      if (fastOrSlow == 'F') {
        pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_STARTUP_FAST, 0, 0);
      } else {  // Slow startup
        pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_STARTUP_SLOW, 0, 0);
      }
      if (smokeOn == 'S') {  // Turn on smoke.  Possible levels are 0..3
        pDelayedAction->populateLocoCommand(millis() + 500, locoNum, LEGACY_ACTION_SET_SMOKE, 3, 0);
        // If we start with smoke on, add a command to turn it off in five minutes.
        //pDelayedAction->populateLocoCommand(millis() + SMOKE_TIME_LIMIT, locoNum, LEGACY_ACTION_SET_SMOKE, 0, 0);
        pDelayedAction->populateLocoCommand(millis() + 5000, locoNum, LEGACY_ACTION_SET_SMOKE, 0, 0);
      } else {  // No smoke
        pDelayedAction->populateLocoCommand(millis() + 500, locoNum, LEGACY_ACTION_SET_SMOKE, 0, 0);
      }

// Adding some extra commands here to test out some features...
      pDelayedAction->populateLocoWhistleHorn(millis() + 3000, locoNum, LEGACY_PATTERN_BACKING);
      pDelayedAction->populateLocoSpeedChange(millis() + 1500, locoNum, 2, 200, 50);


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
    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop
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
  sprintf(lcdString, "END REG MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
  return;
}

// ********************************************************************************************************************************
// ************************************************** OPERATE IN AUTO/PARK MODE ***************************************************
// ********************************************************************************************************************************

void LEGAutoParkMode() {

  sprintf(lcdString, "BEGIN A/P MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);


  // IF OPERATOR SELECTS SMOKE ON (or works regardless) MIGHT WANT TO AUTOMATICALLY TURN OFF AFTER i.e. 10 MINUTES ***************************************************************************

  // Mode is allowed to change from Auto/Running to Park/Running, without first Auto/Stopping or Auto/Stop.

  // Be sure to call haltIfHaltPinPulledLow() frequently, as the halt line pulled low by another module will only last 1 second.
  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop


  sprintf(lcdString, "END A/P MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
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

