// TRAINS_2020.INO Rev: 06/08/22.
#include <Dispatcher.h>
const char APPVERSION[15] = "O_MAS 02/28/21";
#include <Conductor.h>
#include "Engineer.h"

typedef uint8_t byte;

// 6/16/21: We will have a HEAP ARRAY of 50 Train Progress OBJECTS (structs), but EACH OBJECT/STRUCT CONTAINS ITS OWN CIRCULAR
//          BUFFER ARRAY, including head/tail pointers, plus the array of 140 route elements!  *******************************
// So Train Progress is an array of buffers; i.e. an array of arrays.
// For Delayed Action (in O_LEG only), each object only requires 10 bytes each, and we will have a heap array of approx 1000 objects.

// This module, MAS (Trains_2020), is the master controller; everyone else is a slave.
// Loco, Route, Block, Turnout, Button, Sensor, etc. numbers always start at 1, unless accessing data file directly.
// Dec 2020: Converted all global objects from stack to heap to save SRAM.

// In AUTO and PARK modes (and future POV mode), MAS is responsible for throwing all turnouts on Routes assigned by Dispatcher.
// Each time the train stops (either originally stopped, or when reversing,) throw all turnouts until the next VL00 (stop) in the
// route, as found in the Train Progress table.  We can't throw the entire route, because if we reverse, we might go over a
// turnout (and block) that previously occurred, and it needs to be thrown with a different orientation.
// LEG Conductor/Engineer will always assume that Turnouts are being thrown elsewhere and won't worry about it.

// Dispatcher will not release Turnout or Block reservations if they occur farther ahead in a route, but is responsible to release
// all Turnout and Block reservations as soon as a loco no longer needs them (per the Train Progress table.)

// TO DO: Any mode where we know a loco's position (end of Reg, Auto, Park -- but lost during Manual and POV): Update the "last-
//        known position" of each train from time to time, to be used as next Reg'n default.
// TO DO: AUTO MODE: When a loco has completed a Train Progress route, wait a while before assigning a new route for that train, to
//        give the Conductor time to make announcements etc.
// TO DO: Dispatcher: THE LONGEST TRAIN MUST FIT IN THE SHORTEST SIDING i.e. all trains must fit into all sidings.  So we don't
//        need to worry about siding length vs train length.  With the possible exception of the single-ended sidings.  In order to
//        eliminate the "next siding is fine, but the one after that is too short" problem.
// TO DO: Route.cpp needs a function to display/print contents of Route table to compare to spreadsheet to veryify accuracy.

// FUTURE: AUTO VS POV mode: I think POV mode should be similar to Auto, in that the system automatically schedules routes, throws
//         turnout, reserves blocks and turnouts, and controls signals.  But the OPERATOR is required to manually control the
//         throttle -- including obeying signals, blowing the horn/whistle, and slowing and stopping at the appropriate destination
//         sidings.  Since we currently treat POV like Manual mode, we'll need to do some heavy modifications to implement.
// FUTURE: In MANUAL/POV, keep track of occupied blocks and prevent collisions.  Harder than it sounds though, but we could do it
//         if we knew where the trains were when starting Manual mode, and kept track of them as if we were in Auto mode but
//         without assigning routes.

// Include the following #define if we want to run the system with just the lower-level track.
#define SINGLE_LEVEL     // Comment this out for full double-level routes.  Use it for single-level route testing.

#include <Train_Consts_Global.h>
const byte THIS_MODULE = ARDUINO_MAS;  // Declare as extern in classes that need to know which module is calling them, such as RS485.
char lcdString[LCD_WIDTH + 1];   // Global array to hold strings sent to Digole 2004 LCD; last char is for null terminator.
// lcdString is a global char array to hold 20-char (plus null) strings to be sent to the LCD.
// To use lcdString in a class that we call, just include "extern lcdString[]" in the class .h file.
// We must define it at the top of each module's code, *before* Train_Functions.h, so that it may be used there.
#include <Train_Functions.h>  // Include this *after* defining global lcdString[].

// *** SERIAL LCD DISPLAY CLASS ***
#include <Display_2004.h>
Display_2004* pLCD2004;

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorage;

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage;

// *** CONTROL PANEL MODE DIAL AND LEDs CLASS ***
#include <Mode_Dial.h>  // A class with a set of functions for reading and updating the mode-control hardware on the control panel
Mode_Dial* pModeSelector;

// *** LOCOMOTIVE REFERENCE LOOKUP TABLE CLASS (IN FRAM) ***
#include <Loco_Reference.h>
Loco_Reference* pLoco;

// *** BLOCK RESERVATION AND LOOKUP TABLE CLASS (IN FRAM) ***
#include <Block_Reservation.h>
Block_Reservation* pBlockReservation;

// *** TURNOUT RESERVATION TABLE CLASS (IN FRAM) ***
#include <Turnout_Reservation.h>
Turnout_Reservation* pTurnoutReservation;  // Create a pointer to a Turnout_Reservation type

// *** SENSOR-BLOCK CROSS REFERENCE TABLE CLASS (IN FRAM) ***
#include <Sensor_Block.h>
Sensor_Block* pSensorBlock;

// *** ROUTE REFERENCE TABLE CLASS (IN FRAM) ***
#include <Route_Reference.h>
Route_Reference* pRoute;

// *** DEADLOCK LOOKUP TABLE CLASS (IN FRAM) ***
#include <Deadlock.h>
Deadlock_Reference* pDeadlock;

// *** DELAYED ACTION CLASS ***
#include<Delayed_Action.h>
Delayed_Action* pDelayedAction;

// *** TRAIN PROGRESS TABLE CLASS (IN FRAM) ***
// 6/16/21: Train Progress table (MAS, OCC, LEG, LED, and SNS) must be on the heap, because its objects will be greater than 255 bytes each. *****************************************************
#include <Train_Progress.h>
Train_Progress* pTrainProgress;  // Create a pointer to a Train_Progress type
// 6/16/21: Since rec len > 255, Train Progress MUST be stored on the HEAP, not FRAM.  A struct/array element/object can be any size.
//   And since it's on the heap, we can set it up as an ARRAY of objects, not records that need to be looked up as with FRAM.
// Since we plan to use a heap array to represent the entire Train Progress table, rather than swapping records in/out for
// different locos (similar to the way we swap records for Routes, Block Reservations, etc.), we'll want to have STATIC variables
// for those that are not unique to each loco.  In this case, our pointers to the LCD and FRAM hardware devices.  There may be
// other global STATICs needed as we develop this class; in any event, we must declare global pointers here:
Display_2004* Train_Progress::m_pLCD2004; // Pointer to the LCD display for error messages.  STATIC in the class.
FRAM* Train_Progress::m_pStorage; // = pStorage;  // Pointer to FRAM.  STATIC in the class.  *********** THIS ASSUMING TRAIN PROGRESS EVEN NEEDS ACCESS TO FRAM?????? **********

// *** OPERATOR CLASS ***
#include <Operator.h>  // Used when in Manual and POV modes
Operator* pOperator;

// *** REGISTRAR CLASS ***
#include <Registrar.h>  // Used when in Registration mode
Registrar* pRegistrar;

// *** DISPATCHER CLASS ***
//#include <Dispatcher.h>  // Used when in Auto and Park modes
//Dispatcher* pDispatcher;

// *** DISPATCH BOARD CLASS ***
//#include <Dispatch_Board>  // Used in all modes.  Communicates between Operator/Registrar/Dispatch and O_LEG Conductor classes
//Dispatch_Board* pDispatchBoard;

// *** MISC CONSTANTS AND GLOBALS NEEDED BY MAS ***
byte modeCurrent = MODE_MANUAL; //UNDEFINED;
byte stateCurrent = STATE_STOPPED;
bool newModeOrState = true;  // A flag so we know when we've just started a new mode and/or state.

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();

  // *** QUADRAM EXTERNAL SRAM MODULE ***
  initializeQuadRAM();  // Add-on memory board provides 56,832 bytes for heap

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200

  // ******************************************************************************************************************************
  // 6/25/21: Reminding myself about explicitly casting pointer when using "new":
  // The 'new' operator in C++ returns a pointer of the allocated type. So there is no need to cast.  I.e. these are equivalent:
  //   pLCD2004 = (Display_2004 *)new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate object and assign pointer
  //   pLCD2004 =                 new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate object and assign pointer
  // There is a lot of confusion regarding malloc() in C and C++ etc., but ultimately C++ with 'new' does not require a cast.
  // ******************************************************************************************************************************

  // **************************************** HOW TO USE QUADRAM, HEAP, POINTERS, and NEW *****************************************
  // Rev: 06/08/22.  Removed recommendation to cast the pointer returned by 'new'.  Per Tim 5/1/22: The new operator in C++ creates
  //   a properly typed pointer for you, and it is not meaningful to do any casting on it.
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
  // Don't forget that the source code (.cpp) files that use i.e. pLCD2004 will need a declaration of that variable to compile
  // without complaint:
  //   extern Display_2004* ptrLCD2004;
  // Declarations are typically put in a header (.h) file which is then included (via #include) at the top of any source file(s)
  // that reference those variables.  This is done to let the compiler know the name and type of the variable, and also to let it
  // know that the variable has been defined somewhere else, so the compiler can just go ahead and reference it as though it
  // existed.  The linker will take care of attaching such references to the variable definition at link time.
  // ******************************************************************************************************************************

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because this object has a parent (DigoleSerialDisp) that needs them.
  // FYI it turns out that the (Display_2004 *) cast is unnecessary but also not harmful.  Ditto for all other heap pointers.
  pLCD2004 = (Display_2004 *)new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1

  // *** DISPLAY APP VERSION ON LCD AND SERIAL MONITOR *** (No heap space used)
  sprintf(lcdString, APPVERSION);
  pLCD2004->println(lcdString);
  Serial.println(lcdString);

  // *** INITIALIZE FRAM CLASS AND OBJECT *** (Heap uses 76 bytes)
  // We must pass a parm to the constructor (vs begin) because this object has a parent (Hackscribble_Ferro) that needs it.
  pStorage = (FRAM *)new FRAM(MB85RS4MT, PIN_IO_FRAM);  // Instantiate the object and assign the global pointer
  pStorage->begin(pLCD2004);  // Will crash on its own if there is any problem with the FRAM
  //pStorage->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
  //pStorage->testFRAM(); Serial.println("Test okay"); while (true) { }  // Writes then reads random data to entire FRAM, confirming 100% working.
  //pStorage->testSRAM(); Serial.println("Test okay"); while (true) { }  // Test 4K of SRAM just for comparison
  //pStorage->setFRAMRevDate(6, 18, 60);  // Available function for testing only.
  
  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // Quirk about C++: NO PARENS in constructor call if there are no params; else thinks it's a function declaration!
  pMessage = (Message *)new Message;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pMessage->begin(pLCD2004, &Serial2, SERIAL2_SPEED);
  delay(1000);  // O_MAS-only delay gives the slaves a chance to get ready to receive data.

  // *** INITIALIZE CONTROL PANEL MODE DIAL CLASS AND OBJECT *** (Heap uses 31 bytes)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pModeSelector = (Mode_Dial *)new Mode_Dial;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pModeSelector->begin(&modeCurrent, &stateCurrent, pLCD2004);  // Sets Mode = MANUAL, State = STOPPED

  // *** INITIALIZE LOCOMOTIVE REFERENCE TABLE CLASS AND OBJECT *** (Heap uses 81 bytes)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pLoco = (Loco_Reference *)new Loco_Reference;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pLoco->begin(pLCD2004, pStorage);

  // *** INITIALIZE BLOCK RESERVATION CLASS AND OBJECT *** (Heap uses 26 bytes)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pBlockReservation = (Block_Reservation *)new Block_Reservation;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pBlockReservation->begin(pLCD2004, pStorage);

  // *** INITIALIZE TURNOUT RESERVATION CLASS AND OBJECT ***  (Heap uses 9 bytes)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pTurnoutReservation = (Turnout_Reservation *)new Turnout_Reservation;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pTurnoutReservation->begin(pLCD2004, pStorage);

  // *** INITIALIZE SENSOR-BLOCK CROSS REFERENCE CLASS AND OBJECT *** (Heap uses 9 bytes)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pSensorBlock = (Sensor_Block *)new Sensor_Block;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pSensorBlock->begin(pLCD2004, pStorage);

  // *** INITIALIZE ROUTE REFERENCE CLASS AND OBJECT ***  (Heap uses 177 bytes)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pRoute = (Route_Reference *)new Route_Reference;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pRoute->begin(pLCD2004, pStorage);  // Assuming we'll need a pointer to the Block Reservation class

  // *** INITIALIZE DEADLOCK LOOKUP CLASS AND OBJECT ***  (Heap uses 32 bytes)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pDeadlock = (Deadlock_Reference *)new Deadlock_Reference;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pDeadlock->begin(pLCD2004, pStorage, pBlockReservation);

  // *** INITIALIZE TRAIN PROGRESS CLASS AND OBJECT ***  (Heap uses 14,552 bytes)
  // 6/16/21: Since rec len > 255, Train Progress MUST be stored on the HEAP, not FRAM.  A struct/array element/object can be any size. **********************************
  //   And since it's on the heap, we can set it up as an ARRAY of objects, not records that need to be looked up as with FRAM.

  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  // Unlike almost every other object, we will instantiate 50 copies of Train Progress on the heap (which still will require only
  // *one* 2-byte pointer in SRAM -- so no SRAM stack penalty.)
  // Since we are going to have an array of Train Progress tables (objects), we MUST define a constructor for our class that takes
  // no arguments (this is a rule of C++, per Tim Meighan.)  (We can still init default values for fields in the constructor, and
  // of course in the begin() function.)
  // The data itself will reside on the heap.  This is faster and easier than instantiating a single copy of Train Progress and
  // only being able to read/write a single loco's Train Progress table at a time; requiring reading and writing to/from FRAM.
  // The extra heap space required for an array of objects is justified here because we spend so much time accessing Train Progress
  // in Auto and Park modes.  Also, we are able to easily do this because TRAIN PROGRESS IS ALWAYS INITIALIZED TO BLANK; versus
  // other tables where we are using stored data from FRAM -- i.e. Loco parameters, Routes, etc.
  // 02/26/21: Confirmed that the following 50-element Train Progress table is using 14,552 bytes (of 56,831) on the heap. ***************************************************
  // Each Train Progress element needs 291 bytes * 50 elements = 14,550 bytes.  Not sure what the other two bytes are used for.
  // At this point, there are still 41,703 bytes remaining free on the heap.
  // Instantiate 50 copies of Train_Progress objects on the heap, and point to them (we only need one pointer, in SRAM):    6/16/21 THUS WE HAVE AN ARRAY OF 50 CLASS OBJECTS ************
  pTrainProgress = (Train_Progress *)new Train_Progress[TOTAL_TRAINS];  // NO PARENS for the constructor SINCE NO PARMS!
  for (int i = 0; i < TOTAL_TRAINS; i++) {  // Train Progress object array elements start at 0 (not 1.)
    pTrainProgress[i].begin();  // Use the dot operator, not the arrow ->, because the array makes this a reference not a pointer
    // Details here: https://stackoverflow.com/questions/17716293/arrow-operator-and-dot-operator-class-pointer
  }
  // In order to avoid using space for the LCD and FRAM pointers in every element of TrainProgress, we made them STATIC in the
  // class.  The pointers must be declared above setup() so they're global, and then assigned to pre-defined pointers here:
  Train_Progress::m_pLCD2004 = pLCD2004;  // Pointer to the LCD display for error messages (declared/defined above.)
  Train_Progress::m_pStorage = pStorage;  // Pointer to FRAM (declared/defined above.)   ************ 6/16/21 Confirm if Train Progress even needs to access FRAM??? **************
  // 02/24/21: WRITE SOME TEST CODE TO WORK WITH SEVERAL TRAIN PROGRESS ELEMENTS, TO BE SURE IT WORKS ***************************************************************
  //   This will confirm I just use the dot operator to access any Train Progress element.
// Remember, we have an ARRAY of 50 Train Progress OBJECTS, but EACH OBJECT CONTAINS ITS OWN CIRCULAR BUFFER ARRAY, including head/tail pointers, plus the array of 140 route elements!  *******************************************************************

  // *** INITIALIZE OPERATOR CLASS AND OBJECT ***
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pOperator = (Operator *)new Operator;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pOperator->begin(pLCD2004, pStorage, pMessage, pTurnoutReservation);

  // *** INITIALIZE REGISTRAR CLASS AND OBJECT ***
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pRegistrar = (Registrar *)new Registrar;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pRegistrar->begin(pLCD2004, pStorage, pMessage, pLoco, pBlockReservation);

  // *** INITIALIZE DISPATCHER CLASS AND OBJECT ***
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  //pDispatcher = (Dispatcher *)new Dispatcher;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  //pDispatcher->begin(pLCD2004, pMessage, pLoco, pBlockReservation, pTurnoutReservation, pSensorBlock, pRoute, pDeadlock,
  //                   pTrainProgress, pModeSelector);

  // *** STARTUP HOUSEKEEPING: TURNOUTS, SENSORS, ETC. ***
  pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent); // MODE_MANUAL, STATE_STOPPED.  Keep everyone waiting while we set up.

  // *** OPTIONAL LINES TO POPULATE THE VARIOUS FRAM DATA FILES ***
  //pLoco->populate();                // Uses the heap when populating.
  //pBlockReservation->populate();    // Uses the stack when populating; the table is only 520 bytes long.
  //pTurnoutReservation->populate();  // Fill file with empty data.
  //pSensorBlock->populate();         // Uses the stack when populating (very small.)
  //pRoute->populateRoute();          // Uses the heap when populating.  Huge, so only do this one populate
  //pDeadlock->populate();            // Uses the stack when populating.  File is only 518 bytes total.
  //pTrainProgress->populate();       // Not written yet as of 12/14/20
  //sprintf(lcdString, "POPULATED!"); pLCD2004->println(lcdString); Serial.println(lcdString); while (true) { }
  //runFileAccessTests();  // Just runs through some basic operations to confirm the classes are generally working as expected

  // *** QUICK CHECK OF TURNOUT 17 RESERVATION DUE TO UNEXPLAINED PROBLEMS ON 12/12/20 ***
  // There was a problem reserving turnout 17, both individually and even if I reserved all (as above.)
  // Turnout 17 was reporting whatever the results were for Turnout 16 and I have NO IDEA WHY.
  // I ran the FRAM test twice with no errors, confirmed it was working properly, and suddenly I can now reserve Turnout 17.
  pTurnoutReservation->reserve(17, 12);  
  if ((pTurnoutReservation->reservedForTrain(17)) != 12) {
    sprintf(lcdString, "PROBa W/ TRNOUT 17!"); pLCD2004->println(lcdString); Serial.println(lcdString); while (true) { }
  }
  pTurnoutReservation->release(17);
  if ((pTurnoutReservation->reservedForTrain(17)) != TRAIN_ID_NULL) {
    sprintf(lcdString, "PROBb W/ TRNOUT 17!"); pLCD2004->println(lcdString); Serial.println(lcdString); while (true) { }
  }

}

/*


// Left off here 03/06/21: ********************* NOTE WE START LOOPS AT 0 NOT 1 SO DON'T USE THESE WITHOUT FIXING THAT!
// How does the following figure into Operator and/or Registrar?

  // Request the status of every sensor from SNS to get that tracking started, and use it to reserve occupied blocks for STATIC.
  for (int i = 0; i < TOTAL_SENSORS; i++) {
    pMessage->sendMAStoSNSRequestSensor(i + 1);  // SNS expects actual sensor numbers, which begin at 1.
    while (pMessage->available() != 'S') { }  // Don't do anything until we have a Sensor message incoming from SNS
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);  // Will receive sensor 1..52, T or C
    if (sensorNum != (i + 1)) { sprintf(lcdString, "SNS %i %i MATCH ERR", i + 1, sensorNum); pLCD2004->println(lcdString); endWithFlashingLED(1); }  // That's a bug!
    // All of the other Arduinos (specifically OCC and LEG) saw these sensor statuses.  We will all use them to initally set
    // occupancy of whatever block the sensor is part of to RESERVED for a STATIC train.  Can be updated during Registration.
    // Now we also want to set the "reservedForTrain" field of the Block Reservation table to "Reserved for STATIC."
    // Given an occupied sensor, figure out which block, and which end of that block, the sensor is on.
    // The train is always assumed to be facing the direction of the end that the sensor occupies.  So if a sensor is at the
    // East end of Block 3, then a train is assumed to be in block 3 facing East.
    // We just intitialized block reservations so that ALL blocks are unreserved (Train = 0.)
    // Initially we'll just reserve all occupied blocks for static train = TRAIN_STATIC.  Then we'll ask OCC to have the operator
    // tell us which of those are occupied by "real" trains, and which specific trains...then we will update the Block
    // Reservation table again -- at which time our trains will be "registered."
    if (trippedOrCleared == 'T') {  // If this sensor is occupied, determine the block and direction train must be facing
      blockReservations[(sensorBlock[sensorNum - 1].whichBlock) - 1].reservedForTrain = TRAIN_ID_STATIC;  // Default occupancy to "static train"
      blockReservations[(sensorBlock[sensorNum - 1].whichBlock) - 1].whichDirection = sensorBlock[sensorNum].whichEnd;   //  'E' or 'W'
    }
    // TEST CODE: PRINT THE BLOCK RESERVATION TABLE SHOWING ALL OCCUPIED BLOCKS AS STATIC RESERVED...*************************************************
    Serial.println("BEGINNING BLOCK RESERVATION TABLE, WITH ALL OCCUPIED BLOCKS RESERVED AS #99 STATIC:");
    Serial.println("Block, Reserved-For-Train, Direction:");
    for (byte i=0; i < TOTAL_BLOCKS; i++) {  // Block number is a zero offset, so record 0 = block 1
      Serial.print(i + 1); Serial.print("      ");
      Serial.print(blockReservations[i].reservedForTrain);  // Should be 0 or TRAIN_STATIC at this point
      Serial.print("      ");
      Serial.println(blockReservations[i].whichDirection);         // Should be E or W
    }
  //for (byte sensor=0; sensor < TOTAL_SENSORS; sensor++) {   // 0..63 for sensors 1..64
  //  if (sensorStatus[sensor] == 1) {  // If this sensor is occupied, determine the block and direction train must be facing
  //    blockReservation[sensorBlock[sensor].whichBlock - 1].reservedForTrain = TRAIN_ID_STATIC;  // Default occupancy to "static train"
  //    blockReservation[sensorBlock[sensor].whichBlock - 1].whichDirection = sensorBlock[sensor].whichEnd;   //  'E' or 'W'
  //  }
  //}

// *** AS WE REGISTER! TRANSMIT LAST-KNOWN TRAIN POSITIONS SO EVERYONE CAN INITIALIZE THEIR TRAIN PROGRESS TABLES ***
// NOTE: In MANUAL and POV mode, OCC does NOT illuminate RED/BLUE block-occupancy LEDs.  See that module for justification.
// At beginning of REGISTRATION, send OCC a list of occupied blocks (tripped sensors) so it can update red/blue block occupany
// and white sensor occupancy LEDs.


*/


// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

// 12/12/20: LEFT OFF HERE.  Now that things are basically working, see if we can recognize button presses, and throw turnout
// and illuminate the appropriate turnout LEDs as a result.  Basic stuff that should be working but somehow isn't.

void loop() {

  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop

  // ***** Here is a list of the INCOMING messages that MAS is interested in knowing about: 10/15/20 *****
  // Note that all incoming functions are of return type "void", and all values are returned via pointers.

  // OCC-to-MAS REGISTRATION: 'A'nswer to a prompted question, such as Smoke/No Smoke, Fast/Slow Startup.
  //   Call this *after* sending two or more questions to OCC, and OCC will prompt operator which one they want.
  //   For example, question 0 could be "SMOKE ON" and question 1 could be "NO SMOKE".  Reply would be 0 or 1.
  // Function header       : void pMessage->getOCCtoMASAnswer(byte* t_replyNum)
  // Function call from MAS: void pMessage->getOCCtoMASAnswer(&replyNum)

  // OCC-to-ALL (MAS,LEG,OCC) REGISTRATION 'L'ocation (num, block, dir) of newly-registered train.  One rec/occupied sensor, real
  //   or static.  Any module that maintains a Train Progress table must use "real" records to set up new Train Progress records.
  //   This could use same logic as "add route to Train Progress" with condition to check if any records existed yet or not, and
  //   in Registration mode there better not be any yet or fatal error.  If ok, just create single-record "route" in Train Progress.
  //   OCC needs this, and Route assignments and sensor changes, to maintain correct block occupancy illumination on Control Panel.
  //   IMPORTANT: MAS will need to reserve these blocks, as if it were in AUTO or PARK mode.
  // Function header       : void pMessage->getOCCtoALLTrainLocations(byte* t_locoNum, byte* t_blkNum, char* t_dir)
  // Function call from MAS: void pMessage->getOCCtoALLTrainLocations(&locoNum, &blkNum, &dir)

  // SNS-to-ALL (MAS, LEG, OCC) MANUAL/POV/AUTO/PARK 'S' sensor status (num [1..52] and tripped|cleared.)
  // SNS-to-ALL (MAS, LEG, OCC) 'S' sensor status (num [1..52] and tripped|cleared, and locoNum if applicable else zero.)
  // Function header       : void pMessage->getSNStoALLSensorStatus(byte* t_sensorNum, char* t_trippedOrCleared)
  // Function call from OCC: void pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared)

  // BTN-to-MAS MANUAL: 'B' button press (button num, after permission-to-send.)  MAS must decide which way to throw turnout.
  // Function header       : void pMessage->getBTNtoMASButton(byte* t_buttonNum)
  // Function call from MAS: void pMessage->getBTNtoMASButton(&buttonNum)

  // LEG-to-MAS FUTURE USE: Actual data that LEG wants to send (after permission-to-send.)

  // ***** Here is a list of the OUTGOING messages that MAS will transmit: 10/15/20 *****
  // Note that all outgoing functions are of return type "void", and all values are passed by value (incl. arrays) as "const".

  // MAS-to-ALL (LEG,SNS,BTN,LED,OCC) 'M' Mode/State change message
  // Function header       : void pMessage->sendMAStoALLModeState(const byte t_mode, const byte t_state)
  // Function call from MAS: void pMessage->sendMAStoALLModeState(mode, state)

  // MAS-to-ALL (LEG,OCC,SWT,LED) AUTO/PARK Set 'R'oute.  Requires 'ER' end-of-route record.  Blocks, Turnouts, Direction changes, etc.
  //   Longest route is about 40 fields of Cmd_Type + Cmd_Val.  Each record would be Len, From, To, 'R'oute, Train Num, Cmd_Type,
  //     Cmd_Val, Cksum = 8 bytes.  We don't need a separate byte for end-of-transmission because that can be the msg type.
  //   So 8 bytes * 40 fields = 320 bytes to transmit for the *longest* route.  115K baud = 14,375 bytes/second.
  //   So a 320 byte longest route would take 0.022 seconds = about 1/50th of a second.
  //   One strategy to eliminate RS485 incoming buffer overflow would be to always wait for an RS485 ACK from the recipient
  //     before sending the next message (to anyone.)  An ACK could be just five bytes, following *every* record receipt.
  //     Add 5 byte ACK * 40 transmissions = additional 200 bytes.  320 + 200 = 520 bytes = .036 seconds for longest route.
  //     This means that in the worst-possible-case scenario we could transmit a new long route for 10 trains in .36 seconds.
  //     BEST STRATEGY is to NOT send ACK replies, but always monitor number of bytes in the incoming serial buffer.  If that
  //       number ever approaches overflow, display a message to that effect and trigger an Emergency Stop.
  // Function header       : void pMessage->sendMAStoALLRoute(const byte t_locoNum, const byte t_cmdType, const byte t_cmdVal)
  // Function call from MAS: void pMessage->sendMAStoALLRoute(locoNum, cmdType, cmdVal)

  // MAS-to-ALL (SWT,LED) MANUAL/POV Set 'T'urnout.  Includes number and orientation Normal|Reverse.
  // Function header       : void pMessage->sendMAStoALLTurnout(const byte t_turnoutNum, const char t_setting)
  // Function call from MAS: void pMessage->sendMAStoALLTurnout(turnoutNum, setting)  // setting = N|R

  // MAS-to-OCC AUTO/PARK 'P'lay audio sequence.  Includes station number, and up to 8 bytes of sound clip numbers.
  //   I.e. "Train number 14, The Southwest Chief, Now Departing From Track Two, (pause), All Aboard!"
  //   In order for these to make sense, they will all need to be recorded using the same "voice."  But this also means that
  //   all announcements for every train from every station will use the same voice, which is not good.  Hmmm.
  // Function header       : void pMessage->sendMAStoOCCPlay(const byte t_stationNum, const char t_phrase[])
  // Function call from MAS: void pMessage->sendMAStoOCCPlay(stationNum, phrase)  // phrase is 8-char array

  // MAS-to-SNS MANUAL/POV/AUTO/PARK Permission or Request to transmit 'S'ensor status.  Permission if in response to pin-pulled-low RTS, Request if
  //   the Sensor Number outgoing field is non-zero (and not in response to digital line being pulled low.)
  // Note that we don't call this function to grant permission to send -- that's done inside of the Message() class.
  // However we *will* use this function in this main program to request the status of a specific (non-zero) sensor.
  // Function header       : void pMessage->sendMAStoSNSRequestSensor(const byte t_sensorNum)
  // Function call from MAS: void pMessage->sendMAStoSNSRequestSensor(sensorNum)  // sensorNum 0 = ok to send; 1..52 = request

  // MAS-to-BTN MANUAL/POV Permission to transmit 'B'utton press.  In response to pin-pulled-low RTS.
  // Note that we don't actually call this function from the main program; it's done inside of the Message() class.
  // Function header       : void pMessage->sendMAStoBTNRequestButton()  // No parms; just sending message gives permission
  // Function call from MAS: void pMessage->sendMAStoBTNRequestButton()  // No parms; just sending message gives permission

  // MAS-to-LEG FUTURE USE: Permission to transmit data.  In response to pin-pulled-low RTS.

  // ******************************************************************************************************************************

  // NOTE: 3/6/21: Unlike other modules, MAS does *not* check for incoming Messages in its main loop.  Each mode/state is
  //   responsible for checking for messages (and sending messages) that relate to it.

  // Regardless of what MODE and STATE we are in, see if operator is LEGALLY pressing the START button.  If so, then immediately
  // change the MODE and STATE as appropriate, and broadcast that to everyone.
  if (pModeSelector->startButtonIsPressed(&modeCurrent, &stateCurrent)) {
    // This can only come back true if it was legal to press the START button at this time.
    // Operator has started a new mode!
    pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent);  // Broadcast the new MODE and STATE.
    // MAYBE WE WANT TO SET A FLAG INDICATING STARTING A NEW MODE??? ********************************************************************************
    sprintf(lcdString, "START M%i S%i", modeCurrent, stateCurrent); pLCD2004->println(lcdString); Serial.println(lcdString);
    newModeOrState = true;  // Currently used when starting MANUAL or POV mode
  }

  // Regardless of what MODE and STATE we are in, see if operator is LEGALLY pressing the STOP button.  If so, then immediately
  // change the MODE and STATE as appropriate, and broadcast that to everyone.
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
  pModeSelector->paintModeControls(modeCurrent, stateCurrent);

  // We have whatever MODE and STATE the operator wants to be in, so let's do some work...
  // Note that there's nothing to do in any mode if we are STOPPED, except keep checking if op wants to start a new mode (and keep
  //   the Mode Dial LEDs up to date, in case the dial is turned by the operator.)

  if ((stateCurrent == STATE_RUNNING) || (stateCurrent == STATE_STOPPING)) {  // We are RUNNING or STOPPING in any mode

    // *************************************
    // ***** MANUAL and POV MODE LOGIC *****
    // *************************************

    if ((modeCurrent == MODE_MANUAL) || (modeCurrent == MODE_POV)) {  // We are MANUAL+RUNNING or POV+RUNNING
      // There is no such thing as MANUAL+STOPPING or POV+STOPPING.
      // NOTE: In MANUAL and POV mode, OCC does NOT illuminate RED/BLUE block-occupancy LEDs.

      // Upon starting MANUAL or POV, initialize turnouts, green turnout LEDs, and white sensor LEDs.
      if (newModeOrState == true) {
        pOperator->prepare();
        newModeOrState = false;
      }

      // Monitor turnout button presses, throw turnouts, monitor sensors, and allow OCC to keep white sensor LEDs up to date.
      pOperator->operate();

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
        //pModeSelector->paintModeControls(modeCurrent, stateCurrent);  // Service the LEDs since Stop button LED will be blinking every 500ms.  PROBABLY DON'T NEED TO DO THIS HERE ********************

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
        pRegistrar->promptFastOrSlowStartup();
        pRegistrar->promptSmokeOrNoSmoke();

        // NOW PROMPT OPERATOR FOR THE LOCO ID OF EVERY OCCUPIED BLOCK (SENSOR).  DONE VIA OCC A/N DISPLAY and ROTARY ENCODER.

        // Send all TOTAL_TRAINS (50) loco info (description, last-known block) to OCC in preparation for prompting.
        // We will also include each train's last-known block number, if found in the Block Reservation table.
        pRegistrar->sendLocoNamesAndBlocksToOCC();


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
        byte blkNum = 0;
        char dir = 'X';
        while (pMessage->available() != 'L') { }  // Dump everything until we get a reply from OCC
        pMessage->getOCCtoALLTrainLocations(&locoNum, &blkNum, &dir);
        sprintf(lcdString, "T%i B%i D%c", locoNum, blkNum, dir); pLCD2004->println(lcdString); Serial.println(lcdString);

        while (pMessage->available() != 'L') { }  // Dump everything until we get a reply from OCC
        pMessage->getOCCtoALLTrainLocations(&locoNum, &blkNum, &dir);
        sprintf(lcdString, "T%i B%i D%c", locoNum, blkNum, dir); pLCD2004->println(lcdString); Serial.println(lcdString);


        // Tell Mode_Dial class we're registered so it can set internal flag
        pModeSelector->setStateToSTOPPED(modeCurrent, &stateCurrent);  // Will change stateCurrent to STOPPED
        sprintf(lcdString, "REGISTRATION DONE!"); pLCD2004->println(lcdString); Serial.println(lcdString);
        pMessage->sendMAStoALLModeState(modeCurrent, stateCurrent);  // Broadcast that we are in REGISTRATION STOPPED
    }

  }  // Else we are STOPPED and nothing to do

}

/*

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

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

void runFileAccessTests() {
  // Test basic fetch of loco, block, turnout, sensor-block, route, and deadlock functions.
  // This just gives us some confidence that things are generally working as expected.
  // always check if Turnout 17 reservations don't work as expected - that was an unexplained error that resolved itself 12/12/20.
  Serial.println(pLoco->legacyID(1));
  Serial.println(pBlockReservation->length(3));
  pTurnoutReservation->reserve(4, 10);
  Serial.print(F("Reserved for train: ")); Serial.println(pTurnoutReservation->reservedForTrain(4));
  Serial.println(pSensorBlock->whichBlock(4));
  pSensorBlock->setStatus(17, 'T');
  pSensorBlock->setStatus(18, 'C');
  Serial.println(pSensorBlock->getStatus(17));
  Serial.println(pSensorBlock->getStatus(18));
  pSensorBlock->setStatus(17, 'C');
  pSensorBlock->setStatus(18, 'T');
  Serial.println(pSensorBlock->getStatus(17));
  Serial.println(pSensorBlock->getStatus(18));

  Serial.print(F("Route 1 levels: ")); Serial.println(pRoute->getLevels(1));
  // Test basic classes: loco, block, turnout, sensor-block, route, deadlock
  // *** HERE IS A WAY TO TEST/CONFIRM THAT THE "getDistanceAndMomentum" function works properly.  But it's been fully tested so no need to re-test. ***
    byte         tLocoNum           =    1;
    byte         tCurrentSpeed      =   83;  // Low = 38, Med = 64, High = 83
    unsigned int tSidingLength      = 1275;  // 1275 implies 1000ms delay beflor slowing at 80ms-delay momentum
    unsigned int tMSDelayAfterEntry =    0;
    unsigned int tMomentum          =    0;
    unsigned int tCrawlSpeed        =    0;
  pLoco->getDistanceAndMomentum(tLocoNum, tCurrentSpeed, tSidingLength, &tMSDelayAfterEntry, &tMomentum, &tCrawlSpeed);
    Serial.print("Loco                 : "); Serial.println(tLocoNum);
    Serial.print("Incoming Legacy speed: "); Serial.println(tCurrentSpeed);
    Serial.print("Siding length mm     : "); Serial.println(tSidingLength);
    Serial.print("Delay after trip ms  : "); Serial.println(tMSDelayAfterEntry);
    Serial.print("Momentum ms          : "); Serial.println(tMomentum);
    Serial.print("Crawl Legacy speed   : "); Serial.println(tCrawlSpeed);

  // Test block reservations / last-known block/direction
  for (int b = 1; b <= TOTAL_BLOCKS; b++) {
    Serial.print(b); Serial.print(": ");
    Serial.print(pBlockReservation->reservedForTrain(b));  // Returns train number this block is currently reserved for, including TRAIN_ID_NULL and TRAIN_ID_STATIC.
    Serial.print(", ");
    Serial.print(pBlockReservation->reservedDirection(b));  // Returns BE or BW (const defined elsewhere) if reserved, else undefined.
    Serial.print(", ");
    Serial.print(pBlockReservation->length(b));  // Returns block length in mm
    Serial.println();
  }
  pBlockReservation->reserveBlock(2, 12, BE);  // block 2, loco 12, east
  pBlockReservation->reserveBlock(3, 37, BE);  // block 3, loco 37, east
  pBlockReservation->reserveBlock(7, 2, BW);   // block 7, loco 2, west
  for (int b = 1; b <= TOTAL_BLOCKS; b++) {
    Serial.print(b); Serial.print(": ");
    Serial.print(pBlockReservation->reservedForTrain(b));  // Returns train number this block is currently reserved for, including TRAIN_ID_NULL and TRAIN_ID_STATIC.
    Serial.print(", ");
    Serial.print(pBlockReservation->reservedDirection(b));  // Returns BE or BW (const defined elsewhere) if reserved, else undefined.
    Serial.print(", ");
    Serial.print(pBlockReservation->length(b));  // Returns block length in mm
    Serial.println(); Serial.println();
  }
  pBlockReservation->releaseBlock(3);  // Could return bool if we want to confirm it was previously reserved.
  for (int b = 1; b <= TOTAL_BLOCKS; b++) {
    Serial.print(b); Serial.print(": ");
    Serial.print(pBlockReservation->reservedForTrain(b));  // Returns train number this block is currently reserved for, including TRAIN_ID_NULL and TRAIN_ID_STATIC.
    Serial.print(", ");
    Serial.print(pBlockReservation->reservedDirection(b));  // Returns BE or BW (const defined elsewhere) if reserved, else undefined.
    Serial.print(", ");
    Serial.print(pBlockReservation->length(b));  // Returns block length in mm
    Serial.println(); Serial.println();
  }
  pBlockReservation->releaseAllBlocks();  // Just a time-saver.  Does what .begin() does except doesn't initialize LCD and FRAM pointers.
  for (int b = 1; b <= TOTAL_BLOCKS; b++) {
    Serial.print(b); Serial.print(": ");
    Serial.print(pBlockReservation->reservedForTrain(b));  // Returns train number this block is currently reserved for, including TRAIN_ID_NULL and TRAIN_ID_STATIC.
    Serial.print(", ");
    Serial.print(pBlockReservation->reservedDirection(b));  // Returns BE or BW (const defined elsewhere) if reserved, else undefined.
    Serial.print(", ");
    Serial.print(pBlockReservation->length(b));  // Returns block length in mm
    Serial.println(); Serial.println();
  }
  // Test the turnout reservation system
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    Serial.print("Turnout "); Serial.print(t); Serial.print(" :");
    if (pTurnoutReservation->reservedForTrain(t)) {
      Serial.print(" RESERVED for Train "); Serial.println(pTurnoutReservation->reservedForTrain(t));
    } else {
      Serial.println("Not reserved.");
    }
  }
  pTurnoutReservation->reserve(4, 9);  
  pTurnoutReservation->reserve(2, 3);
  pTurnoutReservation->reserve(19, 7);
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    Serial.print("Turnout "); Serial.print(t); Serial.print(" :");
    if (pTurnoutReservation->reservedForTrain(t)) {
      Serial.print(" RESERVED for Train "); Serial.println(pTurnoutReservation->reservedForTrain(t));
    } else {
      Serial.println("Not reserved.");
    }
  }
  pTurnoutReservation->release(4);
  //for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
  //  pTurnoutReservation->reserve(t, t+10);
  //}
  pTurnoutReservation->reserve(21, 8);
  pTurnoutReservation->reserve(13, 2);
  pTurnoutReservation->reserve(17, 6);  
  // There was a problem reserving turnout 17, both individually and even if I reserved all (as above.)
  // Turnout 17 was reporting whatever the results were for Turnout 16 and I have NO IDEA WHY.
  // I ran the FRAM test twice with no errors, confirmed it was working properly, and suddenly I can now reserve Turnout 17.
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    Serial.print("Turnout "); Serial.print(t); Serial.print(" :");
    if (pTurnoutReservation->reservedForTrain(t)) {
      Serial.print(" RESERVED for Train "); Serial.println(pTurnoutReservation->reservedForTrain(t));
    } else {
      Serial.println("Not reserved.");
    }
  }
  pTurnoutReservation->releaseAll();
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    Serial.print("Turnout "); Serial.print(t); Serial.print(" :");
    if (pTurnoutReservation->reservedForTrain(t)) {
      Serial.print(" RESERVED for Train "); Serial.println(pTurnoutReservation->reservedForTrain(t));
    } else {
      Serial.println("Not reserved.");
    }
  }
  // Test the Route class functions:
  routeElement routeOrigin;
  routeOrigin.routeRecType = BW;
  routeOrigin.routeRecVal = 1;
  unsigned int routeIndex = 0;
  routeIndex = pRoute->getFirstMatchingOrigin(routeOrigin);
  while (routeIndex > 0) {
    Serial.print("Looking at route index "); Serial.println(pRoute->getRouteIndex());
    pRoute->display(routeIndex);
    routeIndex = pRoute->getNextMatchingOrigin(routeOrigin, routeIndex);  // Returns 0 if there are no more matching origin records
  }
  // Simple check for the accuracy of the Deadlock table constant data...
  //for (byte d = 1; d <= FRAM_RECS_DEADLOCK; d++) {
  //  Deadlock.print(d);
  //}
  pBlockReservation->reserveBlock(19, 12, BW);  // block 2 east
  pBlockReservation->reserveBlock(5, 37, BW);  // block 3 east
  pBlockReservation->reserveBlock(20, 2, BW);   // block 7 west
  pBlockReservation->releaseBlock(2);
  byte lvls = LEVEL_2_ONLY;  // LEVEL_1_ONLY, LEVEL_2_ONLY, or LEVEL_EITHER
  routeElement re;
  re.routeRecType = BW;
  re.routeRecVal = 3;
  if (pDeadlock->deadlockExists(re, lvls)) {  // Return true if deadlock found, else false, given destination i.e. BE01.
    Serial.println("Deadlock exists!");
  } else {
    Serial.println("Deadlock not found!");
  }

  // Here is an example of a call to SNS to request the status of a particular sensor:
  //pMessage->sendMAStoSNSRequestSensor(8);
  // The result will show up above when we call pMessage->available() and we'll get an 'S' record.
  //byte turnoutNum = 4;
  //char turnoutDir = 'R';
  //pMessage->sendMAStoALLTurnout(turnoutNum, turnoutDir);

  Serial.println("Done");
  while (true) { }
}