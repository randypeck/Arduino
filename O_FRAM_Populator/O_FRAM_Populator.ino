// POPULATOR.INO Rev: 02/17/24.  FINISHED AND TESTED.
// 02/27/23: Define all global pointers to nullptr at top so we can test for null in class begin().
// 02/09/23: Added and tested Turout Res'n, Sensor Block, Block Res'n, Loco Ref, Route Ref, and Deadlocks.

// Use this module to call all of the various populate() functions to populate FRAM i.e. Route Reference, Sensor_Block, etc.
// In most cases, only a single FRAM table can be populated at a time and the other calls must be commented out due to SRAM
// limits.  For Route Reference, due to its large size, populate() must be called several times, with different sets of records
// commented in/out each time it is compiled/run.
// Use the FRAM Duplicator utility to make copies each time a single FRAM has been updated with all of the latest tables.

// **************************************** HOW TO USE QUADRAM, HEAP, POINTERS, and NEW *****************************************
// NOTES REGARDING CLASS/OBJECT DEFINITION, WHEN PLACING OBJECTS ON THE HEAP: We must DECLARE a global pointer to an object of that
// type *above* setup(), so the object will be global (so it's available in loop().)  However, we can't INSTANTIATE some objects
// until we are inside of setup() because some objects, such as Display_2004, have constructors that require hardware that can only
// be accessed reliably once setup() begins.  Most constructors won't have this restriction, but let's make life easy by handling
// all global class declarations and instantiations similarly.
// So above setup() we use an #include statement for the class (unless already #included elsewhere such as Train_Functions.h) and
// define a pointer to it as "nullptr."  Then, within setup(),  we use the "new" command to instantiate a class object in heap
// memory, and assign it's address to the pointer.

// Use set of #define, #ifndef, #endif to selectively populate and test each FRAM table.
// We can populate and/or test some tables in a single compile run, but we must comment out some because there isn't enough SRAM to
// run them all (especially Route Reference, which must be split into separate compiled runs of 25 route records at a time!)
// Note that it is NOT necessary to bracket the "test" function with #ifdef/#endif because if the call is bracketed out, then the
// corresponding test function code will not be compiled anyway there is no memory difference.

// It appears possible to populate everything except Route Reference in one go.
// Then comment out everything *except* Route Reference, and populate it in groups of 25 routes.

//#define TURNOUT_RESERVATION_POPULATE
//#define SENSOR_BLOCK_POPULATE
//#define BLOCK_RESERVATION_POPULATE
#define LOCO_REFERENCE_POPULATE
//#define ROUTE_REFERENCE_POPULATE  // Be sure to indicate GROUP_n in Route_Reference::populate().
//#define DEADLOCK_POPULATE

//#define TURNOUT_RESERVATION_TEST
//#define SENSOR_BLOCK_TEST
//#define BLOCK_RESERVATION_TEST
//#define LOCO_REFERENCE_TEST
//#define ROUTE_REFERENCE_TEST      // Will fail until all GROUPs (i.e. all Routes) have been populated.
//#define DEADLOCK_TEST

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_NUL;  // Global just needs to be defined for use by Train_Functions.cpp and Message.cpp.
char lcdString[LCD_WIDTH + 1] = "POP 03/06/23";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorage = nullptr;

// *** TURNOUT RESERVATION TABLE CLASS (IN FRAM) ***
// This will be populated with default "not-reserved, LOCO_ID_NULL" values; no spreadsheet data needed.
#include <Turnout_Reservation.h>
Turnout_Reservation* pTurnoutReservation = nullptr;

// *** SENSOR-BLOCK CROSS REFERENCE TABLE CLASS (IN FRAM) ***
// This will be populated with hard-coded values that are built-in to the populate() function; no spreadsheet data needed.
#include <Sensor_Block.h>
Sensor_Block* pSensorBlock = nullptr;

// *** BLOCK RESERVATION AND LOOKUP TABLE CLASS (IN FRAM) ***
// This will be populated with hard-coded values that are built-in to the populate() function; no spreadsheet data needed.
#include <Block_Reservation.h>
Block_Reservation* pBlockReservation = nullptr;

// *** LOCOMOTIVE REFERENCE LOOKUP TABLE CLASS (IN FRAM) ***
// This will be populated with hard-coded values manually typed into the populate() function based on spreadsheet values.
// Whenever we add/remove a loco, or a loco's characteristics (speed, momentum, etc.) change, we must manually update BOTH
// the Excel "Loco Ref" spreadsheet tab *and* the hard-coded values in pLoco.populate().
#include <Loco_Reference.h>  // Must be set up before Deadlock as Deadlock requires Loco pointer.
Loco_Reference* pLoco = nullptr;

// *** ROUTE REFERENCE TABLE CLASS (IN FRAM) ***
// Routes must be exported to a .CSV file and then manually pasted into the Route_Reference.populate() function.
// Due to memory constraints, we can only populate FRAM with 25 routes at a time, and then change the "define" line in the
// populate() function and re-compile for each group.  I.e. "#define GROUP_3".
#include <Route_Reference.h>
Route_Reference* pRoute = nullptr;

// *** DEADLOCK LOOKUP TABLE CLASS (IN FRAM) ***
// This will be populated with hard-coded values manually typed into the populate() function based on spreadsheet values.
// It will only be necessary to re-populate FRAMs if we find a problem with our hard-coded values, possibly from time to time.
#include <Deadlock.h>
Deadlock_Reference* pDeadlock = nullptr;


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
  pStorage->setFRAMRevDate(6, 18, 60);  // Available function for testing only.
  pStorage->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
  //pStorage->testFRAM();         while (true) { }  // Writes then reads random data to entire FRAM.
  //pStorage->testSRAM();         while (true) { }  // Test 4K of SRAM just for comparison.
  //pStorage->testQuadRAM(56647); while (true) { }  // (unsigned long t_numBytesToTest); max about 56,647 testable bytes.

  // *** INITIALIZE TURNOUT RESERVATION CLASS AND OBJECT ***  (Heap uses 9 bytes)
  pTurnoutReservation = new Turnout_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pTurnoutReservation->begin(pStorage);
  #ifdef TURNOUT_RESERVATION_POPULATE
    sprintf(lcdString, "POP Turnouts"); pLCD2004->println(lcdString); Serial.println(lcdString);
    pTurnoutReservation->populate();
  #endif

  // *** INITIALIZE SENSOR-BLOCK CROSS REFERENCE CLASS AND OBJECT *** (Heap uses 9 bytes)
  pSensorBlock = new Sensor_Block;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pSensorBlock->begin(pStorage);
  #ifdef SENSOR_BLOCK_POPULATE
    sprintf(lcdString, "POP Sensor Block"); pLCD2004->println(lcdString); Serial.println(lcdString);
    pSensorBlock->populate();
  #endif

  // *** INITIALIZE BLOCK RESERVATION CLASS AND OBJECT *** (Heap uses 26 bytes)
  pBlockReservation = new Block_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pBlockReservation->begin(pStorage);
  #ifdef BLOCK_RESERVATION_POPULATE
    sprintf(lcdString, "POP Block Res'n"); pLCD2004->println(lcdString); Serial.println(lcdString);
    pBlockReservation->populate();  // Populate FRAM Block Reservation table.
  #endif

  // *** INITIALIZE LOCOMOTIVE REFERENCE TABLE CLASS AND OBJECT *** (Heap uses 81 bytes)
  pLoco = new Loco_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pLoco->begin(pStorage);
  #ifdef LOCO_REFERENCE_POPULATE
    sprintf(lcdString, "POP Loco Ref"); pLCD2004->println(lcdString); Serial.println(lcdString);
    pLoco->populate();
  #endif

  // *** INITIALIZE ROUTE REFERENCE CLASS AND OBJECT ***  (Heap uses 177 bytes)
  pRoute = new Route_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pRoute->begin(pStorage);  // Assuming we'll need a pointer to the Block Reservation class
  #ifdef ROUTE_REFERENCE_POPULATE
    sprintf(lcdString, "POP Route Ref"); pLCD2004->println(lcdString); Serial.println(lcdString);
    // IMPORTANT: Can only populate 25 records at a time; change the #define GROUP_X line in the populate() function and recompile.
    pRoute->populate();  // Populate FRAM Route Reference table; must be run multiple times due to SRAM limitation.
  #endif

  // *** INITIALIZE DEADLOCK LOOKUP CLASS AND OBJECT ***  (Heap uses 32 bytes)
  // WARNING: DEADLOCK MUST BE INSTANTIATED *AFTER* BLOCK RESERVATION AND LOCO REFERENCE.
  pDeadlock = new Deadlock_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pDeadlock->begin(pStorage, pBlockReservation, pLoco);
  #ifdef DEADLOCK_POPULATE
    sprintf(lcdString, "POP Deadlocks"); pLCD2004->println(lcdString); Serial.println(lcdString);
    pDeadlock->populate();
  #endif

  // There is no need to "populate" Train Progress or Delayed Action as those are heap files that contain no static data.
  sprintf(lcdString, "POP Complete!"); pLCD2004->println(lcdString); Serial.println(lcdString);


}

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  #ifdef TURNOUT_RESERVATION_TEST
    unitTestTurnoutReservation();
  #endif

  #ifdef SENSOR_BLOCK_TEST
    unitTestSensorBlock();
  #endif

  #ifdef BLOCK_RESERVATION_TEST
    unitTestBlockReservation();
  #endif

  #ifdef LOCO_REFERENCE_TEST
    unitTestLocoReference();
  #endif

  #ifdef ROUTE_REFERENCE_TEST
    unitTestRouteReference();
  #endif

  #ifdef DEADLOCK_TEST
    unitTestDeadlock();
  #endif

  sprintf(lcdString, "Test(s) complete!"); pLCD2004->println(lcdString); Serial.println(lcdString);

  while (true) {}

}

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

void unitTestTurnoutReservation() {
  // Rev: 01/25/23.  Tested and working.
   Serial.println("Turnout Reservation Unit Test...");

  // First we'll display the entire Turnout Reservation table...
  for (byte i = 1; i <= TOTAL_TURNOUTS; i++) {
    pTurnoutReservation->display(i);
  }
  // *** QUICK CHECK OF TURNOUT 17 RESERVATION DUE TO UNEXPLAINED PROBLEMS ON 12/12/20 ***
  // There was a problem reserving turnout 17, both individually and even if I reserved all (as above.)
  // Turnout 17 was reporting whatever the results were for Turnout 16 and I have NO IDEA WHY.
  // I ran the FRAM test twice with no errors, confirmed it was working properly, and suddenly I can now reserve Turnout 17.
  Serial.println("Reserve turnout 17 for train 12...");
  pTurnoutReservation->reserveTurnout(17, 12);
  for (byte t = 15; t <= 18; t++) {
    pTurnoutReservation->display(t);
  }
  if ((pTurnoutReservation->reservedForTrain(17)) != 12) {
    sprintf(lcdString, "PROBa W/ TRNOUT 17!"); pLCD2004->println(lcdString); Serial.println(lcdString); while (true) {}
  }
  Serial.println("Release turnout 17 for train 12...");
  pTurnoutReservation->release(17);
  for (byte t = 15; t <= 18; t++) {
    pTurnoutReservation->display(t);
  }
  if ((pTurnoutReservation->reservedForTrain(17)) != LOCO_ID_NULL) {
    sprintf(lcdString, "PROB W/ TRNOUT 17!"); pLCD2004->println(lcdString); Serial.println(lcdString); while (true) {}
  }
  Serial.println("Reserving turnout 1 for train 30, 2 for STATIC (99), 17 for train 6, 18 for train 1, and 32 for train 23...");
  pTurnoutReservation->reserveTurnout( 1, 30);
  pTurnoutReservation->reserveTurnout( 2, LOCO_ID_STATIC);
  pTurnoutReservation->reserveTurnout(17,  6);
  pTurnoutReservation->reserveTurnout(18,  1);
  pTurnoutReservation->reserveTurnout(32, 23);
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    pTurnoutReservation->display(t);
  }
  Serial.println("Releasing turnout 18...");
  pTurnoutReservation->release(18);
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    pTurnoutReservation->display(t);
  }
  // Try reserving an already-reserved-for-us turnout for us again, which should not be a problem (though redundant)...
  Serial.println("Reserving 17 for train 6, again...");
  pTurnoutReservation->reserveTurnout(17, 6);
  Serial.println("If we got here, then it was no problem, as expected.");

  return;  // STOP HERE IF WE WANT TO KEEP DOING OTHER THINGS WITHOUT CRASHING THE PROGRAM.

  // Now try reserving an already-reserved-for-someone-else turnout for us, which should crash the program
  Serial.println("Reserving 17 for train 20, which should crash the program now...");
  pTurnoutReservation->reserveTurnout(17, 1);  // This should trigger a fatal error, so comment it out for further tests.

  // THIS CODE SHOULD CRASH THE PROGRAM, RESERVING A TURNOUT FOR A LOCO OTHER THAN ITSELF THAT'S ALREADY RESERVED

  Serial.println("Setting turnout 1 Reverse, 2 Normal, 4 Reverse, 32 Reverse");
  pTurnoutReservation->setLastOrientation( 1, TURNOUT_DIR_REVERSE);
  pTurnoutReservation->setLastOrientation( 2, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 4, TURNOUT_DIR_REVERSE);
  pTurnoutReservation->setLastOrientation(32, TURNOUT_DIR_REVERSE);
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    pTurnoutReservation->display(t);
  }
  Serial.println("Setting turnout 1 Normal, 2 Reverse, 4 Normal, 32 Normal");
  pTurnoutReservation->setLastOrientation( 1, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation( 2, TURNOUT_DIR_REVERSE);
  pTurnoutReservation->setLastOrientation( 4, TURNOUT_DIR_NORMAL);
  pTurnoutReservation->setLastOrientation(32, TURNOUT_DIR_NORMAL);
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    pTurnoutReservation->display(t);
  }
  Serial.println("Releasing all turnouts...");
  pTurnoutReservation->releaseAll();
  for (byte t = 1; t <= TOTAL_TURNOUTS; t++) {
    pTurnoutReservation->display(t);
  }
  Serial.println("Tests complete.");
  return;
}

void unitTestSensorBlock() {
  // Rev: 01/25/23.  Tested and working.
  Serial.println("Sensor Block Unit Test...");

  // First we'll display the entire Sensor Block cross-reference table...
  for (byte i = 1; i <= TOTAL_SENSORS; i++) {
    pSensorBlock->display(i);
  }
  Serial.println("Tripping sensors 1, 26, and 52...");
  pSensorBlock->setSensorStatus( 1, SENSOR_STATUS_TRIPPED);
  pSensorBlock->setSensorStatus(26, SENSOR_STATUS_TRIPPED);
  pSensorBlock->setSensorStatus(52, SENSOR_STATUS_TRIPPED);
  for (byte s = 1; s <= TOTAL_SENSORS; s++) {
    pSensorBlock->display(s);
  }
  Serial.println("Clearing sensors 1 and 52...");
  pSensorBlock->setSensorStatus(1, SENSOR_STATUS_CLEARED);
  pSensorBlock->setSensorStatus(52, SENSOR_STATUS_CLEARED);
  for (byte s = 1; s <= TOTAL_SENSORS; s++) {
    pSensorBlock->display(s);
  }
  Serial.println("Tests complete.");
  return;
}

void unitTestBlockReservation() {
  // Rev: 01/25/23.  Tested and working.
  Serial.println("Block Reservation Unit Test...");

  pBlockReservation->releaseAllBlocks();
  Serial.println();
  Serial.println("BEGINNING BLOCK RESERVATION TABLE:");
  Serial.println("Block, Reserved-For-Train, Direction:");
  for (byte b = 1; b <= TOTAL_BLOCKS; b++) {
    unitTestBlockReservationDisplay(b);
  }
  Serial.println("Reserving BW01 for loco 12, BE03 for loco 37, and BW07 for loco 2...");
  pBlockReservation->reserveBlock(1, BW, 12);  // block 1, west, loco 12
  pBlockReservation->reserveBlock(3, BE, 37);  // block 3, east, loco 37
  pBlockReservation->reserveBlock(7, BW,  2);  // block 7, west, loco  2
  for (byte b = 1; b <= TOTAL_BLOCKS; b++) {
    unitTestBlockReservationDisplay(b);
  }
  Serial.println("Releasing BE03 and Reserving BE26 for STATIC...");
  pBlockReservation->releaseBlock(3);
  pBlockReservation->reserveBlock(26, BE, LOCO_ID_STATIC);  // block 26, east, loco 99
  for (byte b = 1; b <= TOTAL_BLOCKS; b++) {
    unitTestBlockReservationDisplay(b);
  }

  // THIS SHOULD CRASH SO COMMENT IT OUT AFTER CONFIRMING...
  //Serial.println("Reserving BW07 for loco 9...which should crash since it's already reserved...");
  //pBlockReservation->reserveBlock(7, BW, 9);

  // We really want to release all blocks now so we don't leave any garbage for whatever we do next.
  Serial.println("Releasing ALL blocks...");
  pBlockReservation->releaseAllBlocks();
  for (byte b = 1; b <= TOTAL_BLOCKS; b++) {
    unitTestBlockReservationDisplay(b);
  }
  return;
}

void unitTestBlockReservationDisplay(byte t_blockNum) {
  // Rev: 01/25/23.  Tested and working.
  Serial.println("Block Reservation DISPLAY...");

  if (pBlockReservation->reservedDirection(t_blockNum) == BE) {
    sprintf(lcdString, "BE%2i", t_blockNum); Serial.print(lcdString);
  }
  else if (pBlockReservation->reservedDirection(t_blockNum) == BW) {
    sprintf(lcdString, "BW%2i", t_blockNum); Serial.print(lcdString);
  }
  else if (pBlockReservation->reservedDirection(t_blockNum) == ER) {
    sprintf(lcdString, "ER%2i", t_blockNum); Serial.print(lcdString);
  }
  else {
    sprintf(lcdString, "BAD DIR %i", t_blockNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  sprintf(lcdString, "  Loco: %2i ", pBlockReservation->reservedForTrain(t_blockNum)); Serial.println(lcdString);
  return;
}

void unitTestLocoReference() {
  // Rev: 02/09/23.  Tested and working.
  Serial.println("Loco Reference Unit Test...");

  for (unsigned int i = 1; i <= TOTAL_TRAINS ; i++) {
    pLoco->display(i);
  }

  // Test getDistanceAndMomentum...tests AOK as of 2/8/23.
  // Try cases where siding is too short for given loco/speed, and speed doesn't match loco's low, med, or high.
  // void getDistanceAndMomentum(const byte t_locoNum, const byte t_currentSpeed, const unsigned int t_sidingLength,
  // byte * t_crawlSpeed, unsigned int* t_msDelayAfterEntry, byte * t_speedSteps, unsigned int* t_msStepDelay);
  // Need pointers to return crawlSpeed, msDelayAfterEntry, speedSteps, and msStepDelay
  byte crawlSpeed = 0;
  unsigned int msDelayAfterEntry = 0;
  byte speedSteps = 0;
  unsigned int msStepDelay = 0;
  byte locoNum = 4;  // SF NW-2 with real data
  pLoco->getDistanceAndMomentum(locoNum, 100, 1900, &crawlSpeed, &msDelayAfterEntry, &speedSteps, &msStepDelay);
  Serial.println(crawlSpeed);
  Serial.println(msDelayAfterEntry);
  Serial.println(speedSteps);
  Serial.println(msStepDelay);

  return;
}

void unitTestRouteReference() {
  // Rev: 01/25/23.  Tested and working.
  Serial.println("Route Reference Unit Test...");

  // First we'll display the entire Route Reference table...
  for (unsigned int i = 0; i < (FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST); i++) {
    pRoute->display(i);
  }
  Serial.println();
  // There is no data to write to Route Reference (once it's been populated) so we'll just do some queries.
  // unsigned int getFirstMatchingOrigin(const routeElement t_origin);  // Returns FRAM Rec Num 0..n
  // unsigned int getNextMatchingOrigin(const routeElement t_origin, const unsigned int t_recNum);
  // unsigned int searchRouteID(const unsigned int t_routeID);  // Returns FRAM Rec Num given a Route ID.  Brute force.
  unsigned int routeRecNum;
  routeElement re;
  // First we'll find all routes with an origin of BE01, which starts at FRAM record zero...
  re.routeRecType = BE;
  re.routeRecVal = 1;
  Serial.print("Query on 1st matching BE01 should return record 0: ");
  routeRecNum = pRoute->getFirstMatchingOrigin(re);  // Could return record 0..
  Serial.println(routeRecNum);
  pRoute->display(routeRecNum);
  unsigned int nextRouteNum = pRoute->getNextMatchingOrigin(re, routeRecNum);  // Returns 0 if none found; else > 0
  while (nextRouteNum != 0) {
    Serial.print("Next match is record "); Serial.println(nextRouteNum);
    pRoute->display(nextRouteNum);
    nextRouteNum = pRoute->getNextMatchingOrigin(re, nextRouteNum);
  }
  Serial.println("No more matches."); Serial.println();
  // Now we'll find all routes with an origin of BW15, which are the last three records in the table...
  re.routeRecType = BW;
  re.routeRecVal = 15;
  Serial.print("Query on 1st matching BW15 should return record 71: ");
  routeRecNum = pRoute->getFirstMatchingOrigin(re);
  Serial.println(routeRecNum);
  pRoute->display(routeRecNum);
  nextRouteNum = pRoute->getNextMatchingOrigin(re, routeRecNum);
  while (nextRouteNum != 0) {
    Serial.print("Next match is record "); Serial.println(nextRouteNum);
    pRoute->display(nextRouteNum);
    nextRouteNum = pRoute->getNextMatchingOrigin(re, nextRouteNum);
  }
  Serial.println("No more matches.");
  // Finally, we'll do a couple of brute-force searches for a given Route Num...
  Serial.print("Search for Route ID 1 should return record 0: ");
  Serial.println(pRoute->searchRouteID(1));
  Serial.print("Search for Route ID 74 should return record 73: ");
  Serial.println(pRoute->searchRouteID(74));
  Serial.print("Search for Route ID 57 should return record 43: ");
  Serial.println(pRoute->searchRouteID(57));
  Serial.println("Search for Route ID 99 should crash with BAD ROUTE error.");
  Serial.println(pRoute->searchRouteID(99));
  return;
}

void unitTestDeadlock() {
  // Rev: 02/09/23.  Tested and working.
  Serial.println("Deadlock Unit Test...");

  // At this time, all Deadlock threats are for either direction (BX), versus specifying BE or BW.  Although my code
  //   does include logic for unfavorable direction, I don't think I'll ever use it.  Thus, these tests only test cases
  //   where the train in a threat siding is irrelevant.  I can always add more test cases later if I ever determine
  //   that I want to start including BE and BW in thread siding lists.
  // In order to test Deadlocks, we'll need to:
  //   * Populate Block Reservations with various combinations to trigger (and not trigger) deadlocks
  //   * Compare each Threat's block length with Loco Ref's train length to disqualify threat due to block too short.
  //   * Compare each Loco's "type" (pass/frt) with each threat block's "forbidden" (P/F/L/T) to see if disqualifies.
  // Test cases including:
  //   * Deadlock record contains no threats (BE/BW 23-26.)
  //   * Check for deadlock on non-siding such as BE12.
  //   * Deadlock BW02 since it has all 11 threats populated; test for deadlock and no deadlock scenarios.
  //   * All threats vacant (should not deadlock.)
  //   * At least one threat is reserved for STATIC, BE or BW, and our threat entry for that siding is specific to the
  //     opposite direction.  Ensure that this threat doesn't get us out of a deadlock (though other threats may.)
  //     I.e. even if a STATIC train's block is reserved in a *favorable* direction, it still counts as unfavorable.
  //   * All threats occupied unfavorable direction except one is vacant (should not deadlock.)
  //   * All threats occupied unfavorable direction except one is facing a favorable direction (should not deadlock.)
  //   * All threats occupied unfavorable direction but one of them is reserved for this loco (should not deadlock.)
  //   * All threats are occupied unfavorable direction except one is vacant but too short for this train (should deadlock)
  //     -> Make a Loco Ref train length 3000mm, as that is longer than blocks 1-3 and 23-26.
  //   * All threats are occupied unfavorable direction except one is vacant but TRAIN TYPE forbidden (should deadlock)
  //     -> Block Reservation BX05 = Passenger only (no freight); BX06 = Freight only (no passenger.)
  //   NOTE: Block Reservation "Forbidden" field [b|Pass|Frt|Local|Thru] is too confusing to be useful.  We'll need to
  //         clarify this before really using it, because of awkward combinations.  May need to update Deadlock logic as
  //         well as Block Res'n and maybe also Loco Ref tables.
  //         For now, we'll just test a simple "Freight forbidden in this siding" scenario, against both a local and a
  //         through Freight train. 
  // We will NOT test for invalid t_candidateDestination or t_locoNum -- they are assume to be valid.
  // For a given loco : m_pLoco->passOrFreight(t_locoNum) = P/p/F/f/M (M.O.W.)
  // For a given block: m_pBlockReservation->forbidden(t_blockNum) = P/F/L/T = Pass, Freight, Local, Through

  pDeadlock->display();
  routeElement candidateDestination;
  byte locoNum;

  // First test condition where the Deadlock record contains no threats...
  locoNum = 1;
  candidateDestination.routeRecType = BE;
  candidateDestination.routeRecVal = 03;
  if (pDeadlock->deadlockExists(candidateDestination, locoNum)) {
    Serial.println(": DEADLOCK!");
  } else {
    Serial.println(": No deadlock!");
  }
  Serial.println();

  // Deadlock record BW14 lists BX01, BX02, BX05, and BX06.
  //   Reserve blocks 1, 2, and 6 -- and play with various options for block 5.
  //     Train too long to fit in block.
  //     Train type (P/p/F/f) not allowed in block (Pass/Freight/Through/Local) (deadlock).
  //     Block Reserved for some other train in an offending direction.
  //     Block Reserved for STATIC in EITHER direction, regardless of deadlock threat direction.
  //     Block Reserved for this train (not a deadlock)

  // NOTE: We'll need to monkey with and re-populate parms in Loco Ref, Block Res, and Deadlocks to test all possibilities.
  // 02/09/23 Tested every combination of Loco Type P/p/F/f vs Forbidden Pass/Freight/Through/Local, all work fine.
  // 02/09/23 Tested one remaining threat reserved for another train (dealock) vs reserved for us (ok), works as expected.
  // 02/09/23 Tested loco is too long to fit in block, works as expected.
  // 02/09/23 Tested block is not a siding i.e. BE12 and it failed as expected.
  // 02/09/23 Tested block is reserved for other train, but in a direction that doesn't offend the deadlock threat (okay.)
  // 02/09/23 Tested block is reserved for other train, in BX or in the same direction as the deadlock threat (deadlock.)
  // 02/09/23 Tested block is reserved for STATIC, no matter the direction BE/BW or threat direction BE/BW/BX (deadlock.)
  //          NOTE: Deadlock threats can be BE/BW/BX, but block res'ns (even not reserved) must be BE or BW only.
  // IMPORTANT: If train is reserved for STATIC, even if in opposite direction than threat direction, it's a deadlock!
  locoNum = 4;
  pBlockReservation->reserveBlock(1, BE, 2);  // Reserve BE01 for loco 2
  pBlockReservation->reserveBlock(2, BW, LOCO_ID_STATIC);  // Reserve BW02 for loco 99 (STATIC)
  pBlockReservation->releaseBlock(5);  // This will be okay to occupy as long as loco is ok type and short enough
  pBlockReservation->reserveBlock(6, BE, 3);  // Reserve BE06 for loco 3

  //pBlockReservation->reserveBlock(5, BW, 4);  // Reserve BW05 for loco 4 (ourself!) should NOT trigger a deadlock
  //pBlockReservation->reserveBlock(5, BW, 8);  // Reserve BW05 for loco 8 should trigger a deadlock IFF threat is BX05 or BW05
  //pBlockReservation->reserveBlock(5, BW, 8);  // Reserve BW05 for loco 8 should NOT trigger a deadlock IFF threat is BE05
  pBlockReservation->reserveBlock(5, BW, LOCO_ID_STATIC);  // Reserve BW05 for loco STATIC should trigger a deadlock for ANY threat BE/BW/BX

  candidateDestination.routeRecType = BW;
  candidateDestination.routeRecVal = 14;
  // We should NOT have a deadlock for loco 4 in BW14 since BX05 is available...unless prevented by train type.
  // Define Loco 4 as type f and F and confirm f = ok and F = deadlock (assuming other threats not available.)
  if (pDeadlock->deadlockExists(candidateDestination, locoNum)) {
    Serial.println(": DEADLOCK!");
  }
  else {
    Serial.println(": No deadlock!");
  }
  Serial.println();

  return;
}
