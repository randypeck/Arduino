// TRAIN_PROGRESS.CPP Rev: 08/07/24.  SOME UTILITY FUNCTIONS WORKING SO FAR BUT NOT TESTED ****************************************************************************************
// Part of O_MAS, O_OCC, and O_LEG.
// IMPORTANT: This class expects t_locoNum to always be passed and returned 1..50, but corresponding Train Progress class array
// elements are internally stored in array elements 0..49.
// So beware of bugs due to pTrainProgress[0..TOTAL_TRAINS - 1] rather than [1..TOTAL_TRAINS].
// All function calls use t_locoNum 1..50, and within these functions we can use our class global variable
//   m_trainProgressLocoTableNum as needed:
//   m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
// Each loco's Train Progress table is a CIRCULAR BUFFER.  So rather than maintaining COUNT, we simply disallow adding an element
// if (headPtr + 1) % HEAP_RECS_TRAIN_PROGRESS == tailPtr (i.e. can never fill last element.)

// ***** See Route_Reference.h for a list of Route Rules *****

#include "Train_Progress.h"

Train_Progress::Train_Progress() {   // Constructor
  // Rev: 04/11/24.
  // HEAP STORAGE: We want the PRIVATE 50-element trainProgress[] structure array to reside on the HEAP.  We will allocate
  // the memory using "new" in the constructor, and point our private pointer at it.
  // The calling .INO program defines a pointer pTrainProgress (via new) to a Train_Progress class instance.
  // So we have a single pointer to this class from the calling module.
  // Then Train_Progress.h defines the m_pTrainProgress pointer to a trainProgressStruct data type that is a member of this class,
  // but a null pointer until now.
  // Here we will create an array of this data type as part of our single instance of this class.
  // I.e. the following defines an array that is part of a single class instance; it's not creating multiple class instances.
  // And it doesn't matter if we create this array with "new" or just as a "static" array because all of the class data is already
  // defined to be on the heap.
  m_pTrainProgress = new trainProgressStruct[TOTAL_TRAINS];  // i.e. One Train Progress table for each of 50 locos = 0..49
  // NOTE FOR TESTING: SINCE WE'RE INSTANTIATING TRAIN PROGRESS ON THE HEAP (WITH "NEW") IN THE CALLING .INO PROGRAM, THE
  // ENTIRE CLASS OBJECT WILL RESIDE ON THE HEAP.  SO WE SHOULD BE ABLE TO ACCESS IT AS A REGULAR ARRAY AND SRAM USAGE SHOULD
  // BE EXACTLY THE SAME AS USING EITHER OF THESE STATEMENTS:
  //   m_pTrainProgress = new trainProgressStruct[TOTAL_TRAINS];  // Used "new"
  //   trainProgressStruct m_TrainProgress[TOTAL_TRAINS];  // Did not use "new"
  // See OneNote page "Heap versus SRAM lessons learned" for specifics about this.
  // There doesn't seem to be any advantage in doing it one way or the other...
  return;
};

void Train_Progress::begin(Block_Reservation* t_pBlockReservation, Route_Reference* t_pRoute) {
  // Rev: 04/15/24.
  // Initialize the header and route for all 50 trains to zero.
  // Okay to call this every time we begin Registration, not allocating anything new.
  m_pBlockReservation = t_pBlockReservation;
  if (m_pBlockReservation == nullptr) {
    sprintf(lcdString, "UN-INIT'd BR PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  m_pRoute = t_pRoute;
  if (m_pRoute == nullptr) {
    sprintf(lcdString, "UN-INIT'd RT PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  for (int locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {  // i.e. 1..50 trains
    resetTrainProgress(locoNum);  // Initialize the header and no route for a single train.
  }
  return;
}

void Train_Progress::resetTrainProgress(const byte t_locoNum) {
  // Rev: 03/04/24.
  // Initialize the header and route for a single train, locoNum 1..50 (not 0..49)
  // This just clears everything; you must later call setInitialRoute to set up an initial position.
  if (outOfRangeLocoNum(t_locoNum)) {  // Requires t_locoNum 1..50, not 0..49
    sprintf(lcdString, "T.P. LOCONUM ERR 1"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].isActive = false;
  m_pTrainProgress[m_trainProgressLocoTableNum].isParked = true;  // We'll set this later; can't know until we know blockNum.
  m_pTrainProgress[m_trainProgressLocoTableNum].isStopped = true;
  m_pTrainProgress[m_trainProgressLocoTableNum].timeStopped = millis();
  m_pTrainProgress[m_trainProgressLocoTableNum].timeToStart = 99999999;  // Don't start until someone tells us to.
  m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeed = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeedTime = millis();
  // The following eight "Pointer" fields are ELEMENT NUMBERS that point to the route element of interest.
  m_pTrainProgress[m_trainProgressLocoTableNum].headPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].nextToClearPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].contPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].stationPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].crawlPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].stopPtr = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].lastTrippedPtr = 0;
  for (byte routeElement = 0; routeElement < HEAP_RECS_TRAIN_PROGRESS; routeElement++) {  // 0..139 if max 140 route elements.xxxxx
      m_pTrainProgress[m_trainProgressLocoTableNum].route[routeElement].routeRecType = ER;
      m_pTrainProgress[m_trainProgressLocoTableNum].route[routeElement].routeRecVal = 0;
  }
  return;
}

void Train_Progress::setInitialRoute(const byte t_locoNum, const routeElement t_block) {
  // Rev: 08/22/24.  COMPLETE BUT NOT TESTED.
  // REGISTRATION MODE ONLY.
  // 08/22/24: Added code to look up if block is a Parking block, and set isParked appropriately.
  // 08/04/24: Added support for lastTrippedPtr.
  // 03/23/23: Eliminated sensorNum as parm since we can infer it by block route element i.e. BE03 -> Sensor 6.
  // (Only) during Registration, setInitialRoute() sets up a new Train Progress record as each real train (not STATIC) is learned.
  // Sets up an initial set of Train Progress route elements reflecting the location and direction of the newly-registered train,
  // and updates all the various header fields (isActive, speed, pointers, etc.)
  // All it needs is a locoNum and a route element i.e. BW03 and will infer a sensor i.e. SN05.
  // For example, "Set up a new route for locoNum 10 at BE02."  We'll figure out that's sittin on Senor 4.
  // LEG Conductor will then want to populate the Delayed Action table with startup parms and to start up the train, velocity zero,
  // forward direction, smoke on or off, etc.
  // STATIC trains aren't part of Train Progress -- MAS/OCC/LEG will reserve the blocks that STATIC trains are occupying during
  //   registration, and thus those blocks won't be available as part of routes being assigned to Train Progress.
  // 1. Sets up an initial set of Route Elements thus:
  //      00   01   02   03   04   05   06
  //    VL00 FD00 SN00 VL00 Bxnn SNnn VL00
  // 2. Updates all header fields of the loco's Train Progress record.
  // See Route_Reference.h for a list of route rules -- too many to list here.

  // To save memory, we can move the errors to the "outOfRange" functions, or just eliminate them entirely.
  if (outOfRangeLocoNum(t_locoNum)) {
    sprintf(lcdString, "T.P. LOCONUM ERR 2"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  if (((t_block.routeRecType != BE) && (t_block.routeRecType != BW)) || (outOfRangeBlockNum(t_block.routeRecVal))) {
    sprintf(lcdString, "T.P. BLOCK ERR 1"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  // Since this is a new train, let's make life simple and re-start everything at the beginning.
  resetTrainProgress(t_locoNum);  // t_locoNum, not m_trainProgressLocoTableNum
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  // Let's figure out what sensor the loco is sitting on, based on initial block element...
  byte initialSensor;
  if (t_block.routeRecType == BE) {
    initialSensor = m_pBlockReservation->eastSensor(t_block.routeRecVal);
  } else {  // Must be BW
    initialSensor = m_pBlockReservation->westSensor(t_block.routeRecVal);
  }

  m_pTrainProgress[m_trainProgressLocoTableNum].isActive = true;  // This is the only way this can become Active
  // Technically, our initial Registration block may not necessarily be a Parking siding (per Block Res'n) so look it up...
  if (m_pBlockReservation->isParkingSiding(t_block.routeRecVal)) {
    m_pTrainProgress[m_trainProgressLocoTableNum].isParked = true;
  } else {
    m_pTrainProgress[m_trainProgressLocoTableNum].isParked = false;
  }
  m_pTrainProgress[m_trainProgressLocoTableNum].isStopped = true;
  m_pTrainProgress[m_trainProgressLocoTableNum].timeStopped = millis();
  m_pTrainProgress[m_trainProgressLocoTableNum].timeToStart = 99999999;  // Don't start until someone tells us to.
  m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeed = 0;
  m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeedTime = millis();
  // The following eight "Pointer" fields are ELEMENT NUMBERS that point to the route element of interest.  Because of our rigid
  // rules about the initial Train Progress state, we're safe to just hard-code the initial pointer values here.
  m_pTrainProgress[m_trainProgressLocoTableNum].headPtr = 7;
  m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr = 5;
  m_pTrainProgress[m_trainProgressLocoTableNum].nextToClearPtr = 5;
  m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr = 2;
  m_pTrainProgress[m_trainProgressLocoTableNum].contPtr = 2;
  m_pTrainProgress[m_trainProgressLocoTableNum].stationPtr = 2;
  m_pTrainProgress[m_trainProgressLocoTableNum].crawlPtr = 2;
  m_pTrainProgress[m_trainProgressLocoTableNum].stopPtr = 5;
  m_pTrainProgress[m_trainProgressLocoTableNum].lastTrippedPtr = 5;

  // Element 0 VL00 must always be a Velocity zero command.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[0].routeRecType = VL;  // Velocity zero
  m_pTrainProgress[m_trainProgressLocoTableNum].route[0].routeRecVal = 0;
  // Element 1 FD00 must always be a Forward direction command since all locos must be facing forward when registered.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[1].routeRecType = FD;  // Forward direction
  m_pTrainProgress[m_trainProgressLocoTableNum].route[1].routeRecVal = 0;
  // Element 2 SN00 is basically a place-holder sensor-number-zero so we have a place to point the tail pointer.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[2].routeRecType = SN;  // Entry sensor of initial route will be behind loco
  m_pTrainProgress[m_trainProgressLocoTableNum].route[2].routeRecVal  =  0;  // i.e. SN00
  // Element 3 VL00 must always be a Velocity zero command.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[3].routeRecType = VL;  // Velocity zero
  m_pTrainProgress[m_trainProgressLocoTableNum].route[3].routeRecVal = 0;
  // Element 4 will be the block number and direction that the loco is in, per parms sent by MAS Registration/Dispatcher.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[4].routeRecType = t_block.routeRecType;  // i.e. BW
  m_pTrainProgress[m_trainProgressLocoTableNum].route[4].routeRecVal  = t_block.routeRecVal;   // i.e. 03
  // Element 5 will be the sensor number that the front of the loco is sitting on, per parms sent by MAS Reg'n/Dispatcher.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[5].routeRecType = SN;
  m_pTrainProgress[m_trainProgressLocoTableNum].route[5].routeRecVal  = initialSensor;   // i.e. 05
  // Element 6 VL00 must always be a Velocity zero command.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[6].routeRecType = VL;  // Velocity zero
  m_pTrainProgress[m_trainProgressLocoTableNum].route[6].routeRecVal = 0;
  // Element 7 is headPtr and will be ER00 just for good measure -- not actually needed.
  m_pTrainProgress[m_trainProgressLocoTableNum].route[7].routeRecType = ER;  // End of route (will already be set to this, but what the heck)
  m_pTrainProgress[m_trainProgressLocoTableNum].route[7].routeRecVal = 0;

  return;
}

void Train_Progress::addExtensionRoute(const byte t_locoNum, const unsigned int t_routeRecNum, const unsigned long t_countdown) {
  // Rev: 08/03/24.
  // Append an Extension route (TRAIN WILL BE STOPPED.)
  // Extension route begins with train stopped (in Auto or Park mode.)
  // t_countdown is ms to wait before starting train.

  addRoute(t_locoNum, t_routeRecNum, ROUTE_TYPE_EXTENSION, t_countdown);
  return;
}

void Train_Progress::addContinuationRoute(const byte t_locoNum, const unsigned int t_routeRecNum) {
  // Rev: 08/03/24.
  // Append a Continuation route (TRAIN WILL BE MOVING AND KEEPS MOVING.)
  // NOTE: If we add a Continuation route where the prior route ended with the loco reversing into a siding such as BW02 and then
  // pulls forward to stop i.e. BE02 stopping on SN04 -- and then the new Continuation route begins in Reverse such as BW02, that
  // will be a weird and awkward sequence of moves.  But seems unlikely I'd ever have a route that ended in Reverse (followed by
  // a short Forward to the stop sensor) and then assign a Continuation route that begins in Reverse.

  addRoute(t_locoNum, t_routeRecNum, ROUTE_TYPE_CONTINUATION, 0);
  return;
}

byte Train_Progress::locoThatTrippedSensor(const byte t_sensorNum) {
  // Rev: 08/04/24.  SEEMS GOOD BUT WOW DOES IT NEED TO BE TESTED!
  // 08/04/24: When called, updates lastTrippedPtr for the loco that tripped sensorNum.
  // Especially with a route where a sensor occurs more than once, though I think this will work fine.
  // Returns locoNum whose next-to-trip sensor was tripped, else fatal error.
  // A sensor may occur more than once in a train's route (such as if we reverse), but in this case we only need to know locoNum.
  // When any sensor is Tripped, we need only look the sensor pointed to by each active loco's nextToTrip pointer, until found.
  // Easy lookup, since a trip can *only* come from the sensor pointed to by some loco's Next-To-Trip pointer.
  // If this sensor doesn't match a sensor pointed to by any loco's next-to-trip pointer, this is a major bug and fatal error.
  // It's important to compare the ELEMENT NUMBER that was tripped, and that's stored in the pointer, and NOT the sensor number,
  // because sensors can appear more than once in a route so we want to be sure we're looking at the one we think we are.
  // Since this function returns locoNum, we need only check this loco's Train Progress nextToTripPtr *element number* and
  // compare it to each of the four cont/station/crawl/stop pointer's element numbers to see if there is a match.

  // Scan the sensor number pointed at by every active loco's next-to-trip pointer, until it matches the sensor number passed.
  // IMPORTANT: We are searching by m_trainProgressLocoTableNum i.e. 0..49, but returning a locoNum 1..50.
  for (m_trainProgressLocoTableNum = 0; m_trainProgressLocoTableNum < TOTAL_TRAINS; m_trainProgressLocoTableNum++) {
    // Train Progress Array Element number i.e. Loco Num - 1
    byte locoNum = m_trainProgressLocoTableNum + 1;
    // Skip this T.P. record if it's not Active
    if (isActive(locoNum)) {
      // nextToTripPtr only gives us an element number, not the sensor number so we have to retrieve that for comparison.
      byte elementNum = m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr;  // Loco's T.P. element num we want to examine
      // Although elementNum should ALWAYS contain a routeRecType of SN (sensor), we'll confirm and also check if sensorNum match.
      if ((m_pTrainProgress[m_trainProgressLocoTableNum].route[elementNum].routeRecType == SN) &&
          (m_pTrainProgress[m_trainProgressLocoTableNum].route[elementNum].routeRecVal == t_sensorNum)) {
        Train_Progress::setLastTrippedPtr(locoNum, elementNum);  // IMPORTANT: Set this to element num, NOT sensor num.
        return (locoNum);
      }
    }
  }
  // If we fall through loop and didn't find the sensor, it's a fatal error
  sprintf(lcdString, "T.P. TTS ERR 1"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  return 0;  // Will never get executed
}

byte Train_Progress::locoThatClearedSensor(const byte t_sensorNum) {
  // Rev: 08/03/24.  SEEMS GOOD BUT WOW DOES IT NEED TO BE TESTED!
  // Especially with a route where a sensor occurs more than once, though I think this will work fine.
  // Returns locoNum whose next-to-clear sensor was cleared, else fatal error.
  // A sensor may occur more than once in a train's route (such as if we reverse), but in this case we only need to know locoNum.
  // When any sensor is Cleared, we need only look at the sensor pointed to by each active loco's nextToClear pointer, until found.
  // Easy lookup, since a clear can *only* come from the sensor pointed to by some loco's Next-To-Clear pointer.
  // If this sensor doesn't match a sensor pointed to by any loco's next-to-clear pointer, this is a massive bug and fatal error.

  // Scan the sensor number pointed at by every active loco's next-to-clear pointer, until it matches the sensor number passed.
  // IMPORTANT: We are searching by m_trainProgressLocoTableNum i.e. 0..49, but returning a locoNum 1..50.
  for (m_trainProgressLocoTableNum = 0; m_trainProgressLocoTableNum < TOTAL_TRAINS; m_trainProgressLocoTableNum++) {
    // Train Progress Array Element number i.e. Loco Num - 1
    byte locoNum = m_trainProgressLocoTableNum + 1;
    // Skip this T.P. record if it's not Active
    if (isActive(locoNum)) {
      byte elementNum = m_pTrainProgress[m_trainProgressLocoTableNum].nextToClearPtr;  // Train i's T.P. element number we want to examine
      if ((m_pTrainProgress[m_trainProgressLocoTableNum].route[elementNum].routeRecType == SN) &&
          (m_pTrainProgress[m_trainProgressLocoTableNum].route[elementNum].routeRecVal == t_sensorNum)) {
        return (locoNum);
      }
    }
  }
  // If we fall through loop and didn't find the sensor, it's a fatal error
  sprintf(lcdString, "T.P. TCS ERR 1"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  return 0;  // Will never get executed
}

bool Train_Progress::timeToStartLoco(const byte t_locoNum) {
  // Rev: 08/03/24.  NOT YET TESTED
  // Returns true if "timeToStart" > millis()
  // Counterintuitively, this function is called by MAS (not LEG!) to decide when to get a loco moving -- MAS will send a "sensor
  // tripped" message to OCC and LEG when it wants it to start moving.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  if (m_pTrainProgress[m_trainProgressLocoTableNum].timeToStart > millis()) {
    return true;  // Yep, it's okay to get this loco moving!
  } else {
    return false;  // No, we don't want to start this loco yet
  }
}

void Train_Progress::display(const byte t_locoNum) {
  // Rev: 02/21/23.  SEEMS GOOD BUT NEEDS TO BE TESTED, CAN'T TEST UNTIL I GET VARIOUS POPULATE FUNCTIONS WRITTEN **************************************************************************
  // TRY TESTING BEFORE AND AFTER ADDING EXTENSION AND CONTINUATION ROUTES *****************************************************************************************************************
  // TRY TESTING WHEN ROUTE EXTENDS BEYOND HIGHEST ELEMENT OF TRAIN PROGRESS TO TEST MODULO MATH *******************************************************************************************
  // Sends one loco's Train Progress table to the Serial monitor.
  if (outOfRangeLocoNum(t_locoNum)) {
    sprintf(lcdString, "T.P. LOCONUM ERR 3"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  Serial.print(F("Current Time      : ")); Serial.println(millis());
  Serial.print(F("isActive          : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].isActive);
  Serial.print(F("isParked          : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].isParked);
  Serial.print(F("isStopped         : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].isStopped);
  Serial.print(F("Time Stopped      : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].timeStopped);
  Serial.print(F("Time Starting     : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].timeToStart);
  Serial.print(F("Current Speed     : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeed);
  Serial.print(F("Current Speed Time: ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeedTime);
  // The following eight "Pointer" fields are ELEMENT NUMBERS that point to the route element of interest.
  Serial.print(F("Head              : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].headPtr);
  Serial.print(F("Next To Trip      : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr);
  Serial.print(F("Next To Clear     : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].nextToClearPtr);
  Serial.print(F("Tail              : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr);
  Serial.print(F("Continuation      : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].contPtr);
  Serial.print(F("Station           : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].stationPtr);
  Serial.print(F("Crawl             : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].crawlPtr);
  Serial.print(F("Stop              : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].stopPtr);
  Serial.print(F("Last Tripped      : ")); Serial.println(m_pTrainProgress[m_trainProgressLocoTableNum].lastTrippedPtr);
  for (byte routeElement = m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr;
    routeElement != m_pTrainProgress[m_trainProgressLocoTableNum].headPtr;
    routeElement = (routeElement + 1) % HEAP_RECS_TRAIN_PROGRESS) {
    switch (m_pTrainProgress[m_trainProgressLocoTableNum].route[routeElement].routeRecType) {
    case ER: { Serial.print(F(" ER")); break; }
    case SN: { Serial.print(F(" SN")); break; }
    case BE: { Serial.print(F(" BE")); break; }
    case BW: { Serial.print(F(" BW")); break; }
    case TN: { Serial.print(F(" TN")); break; }
    case TR: { Serial.print(F(" TR")); break; }
    case FD: { Serial.print(F(" FD")); break; }
    case RD: { Serial.print(F(" RD")); break; }
    case VL: { Serial.print(F(" VL")); break; }
    case TD: { Serial.print(F(" TD")); break; }
    default:  // If we didn't find a legit routeRecType, that's fatal
      Serial.print(F("T.P. DUMP ERR!")); Serial.println(lcdString); while (true) {}
    }
    // Now print the numeric value of this routeElement
    sprintf(lcdString, "%2i ", m_pTrainProgress[m_trainProgressLocoTableNum].route[routeElement].routeRecVal);
    Serial.print(lcdString);
  }
  Serial.println();
  return;
}

// *** PUBLIC GETTERS ***

bool Train_Progress::isActive(const byte t_locoNum) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  // Not active means this loco is static or not on layout.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].isActive;
}

bool Train_Progress::isParked(const byte t_locoNum) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  // Stopped at end of current route in a 'P'arking siding.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].isParked;
}

bool Train_Progress::isStopped(const byte t_locoNum) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  // Stopped at end of current route; not necessarily in a 'P'arking siding.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].isStopped;
}

unsigned long Train_Progress::timeStopped(const byte t_locoNum) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].timeStopped;
}

unsigned long Train_Progress::timeToStart(const byte t_locoNum) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].timeToStart;
}

byte Train_Progress::currentSpeed(const byte t_locoNum) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeed;
}

unsigned long Train_Progress::currentSpeedTime(const byte t_locoNum) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeedTime;
}

byte Train_Progress::headPtr(const byte t_locoNum) {
  // Rev: 03/19/23.  DONE BUT NOT TESTED.
  // Returns element number stored in headPtr.  Not a sensor element.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].headPtr;
}

byte Train_Progress::nextToTripPtr(const byte t_locoNum) {
  // Rev: 03/19/23.  DONE BUT NOT TESTED.
  // Returns element number that holds NextToTrip sensor number.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr;
}

byte Train_Progress::nextToClearPtr(const byte t_locoNum) {
  // Rev: 04/11/24.  DONE BUT NOT TESTED.
  // Returns element number that holds NextToClear sensor number.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].nextToClearPtr;
}

byte Train_Progress::tailPtr(const byte t_locoNum) {
  // Rev: 03/19/23.  DONE BUT NOT TESTED.
  // Returns element number that holds Tail sensor number.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr;
}

byte Train_Progress::contPtr(const byte t_locoNum) {
  // Rev: 09/08/24.  DONE BUT NOT TESTED.
  // Returns element number that holds Continuation sensor number.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].contPtr;
}

byte Train_Progress::stationPtr(const byte t_locoNum) {
  // Rev: 09/08/24.  DONE BUT NOT TESTED.
  // Returns element number that holds Station sensor number.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].stationPtr;
}

byte Train_Progress::crawlPtr(const byte t_locoNum) {
  // Rev: 09/08/24.  DONE BUT NOT TESTED.
  // Returns element number that holds Crawl sensor number.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].crawlPtr;
}

byte Train_Progress::stopPtr(const byte t_locoNum) {
  // Rev: 04/11/24.  DONE BUT NOT TESTED.
  // Returns element number that holds Stop sensor number.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].stopPtr;
}

byte Train_Progress::lastTrippedPtr(const byte t_locoNum) {
  // Rev: 08/04/24.
  // Returns element number that holds the sensor number most recently tripped.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].lastTrippedPtr;
}

bool Train_Progress::atEndOfRoute(const byte t_locoNum) {  // True if the train has reached the end of the route.
  // Rev: 08/05/24.
  // Easiest way to determine if we have reached the end of our route is to check if lastTrippedPtr == stopPtr.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return (m_pTrainProgress[m_trainProgressLocoTableNum].lastTrippedPtr == m_pTrainProgress[m_trainProgressLocoTableNum].stopPtr);
}


// *** PUBLIC SETTERS ***

void Train_Progress::setActive(const byte t_locoNum, const bool t_active) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  // For inactive, set t_active = false
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].isActive = t_active;
  return;
}

void Train_Progress::setParked(const byte t_locoNum, const bool t_parked) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].isParked = t_parked;
  return;
}

void Train_Progress::setStopped(const byte t_locoNum, const bool t_stopped) {
  // Rev: 09/31/24.  DONE BUT NOT TESTED.
  // Updates isStopped as well as a couple of other obvious related fields in Train Progress header.
  // We could potentially also update isParked, since Train Progress has access to Block Res'n (which indicates if a block is a
  // "parking" block or not,) but we'd also have to pass the block number which we may not always want to do.
  // So the calling program (MAS/OCC/LEG Auto/Park) will need to maintain isParked manually.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].isStopped = t_stopped;  // True or False per the passed parm.
  if (t_stopped) {  // If we are setting STOPPED = TRUE
    m_pTrainProgress[m_trainProgressLocoTableNum].timeStopped = millis() + 3000;  // Allow for 3-second time to stop from Crawl.
    // currentSpeed and currentSpeedTime are automatically set in LEG, but MAS and OCC might want to know this too...
    // In fact, in LEG, Engineer will clobber this as the loco slows down until it's stopped.
    m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeed = 0;
    m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeedTime = millis();
  } // else we are setting STOPPED = FALSE, nothing to do (I don't think we want to set timeToStart = 99999999.)
  return;
}

void Train_Progress::setTimeStopped(const byte t_locoNum, unsigned long t_timeStopped) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].timeStopped = t_timeStopped;
  return;
}

void Train_Progress::setTimeToStart(const byte t_locoNum, unsigned long t_timeToStart) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].timeToStart = t_timeToStart;
  return;
}

void Train_Progress::setSpeedAndTime(const byte t_locoNum, const byte t_locoSpeed, const unsigned long t_locoTime) {
  // Rev: 03/04/23.  DONE BUT NOT TESTED.
  // LEG Engineer updates the Train Progress loco's current speed and the time each time it sends an ABS_SPEED, STOP_IMMED,
  // or EMERG_STOP command to the Legacy Command Buffer.
  // t_locoNum must be 1..50, not 0..49; t_locoSpeed must be 0..199 (no support for TMCC though it will accept 0..31.)
  // As Engineer retrieves each ripe SPEED-type record from Delayed Action, it will presumably be sent to the loco within
  // milliseconds, so we'll take this to be our new current speed.  Use this data to update the loco's Train Progress current
  // speed and time fields.  This will be useful for Conductor to look up when determining starting values for speed changes.
  if (outOfRangeLocoNum(t_locoNum)) {
    sprintf(lcdString, "T.P. LOCONUM ERR 4"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  if (outOfRangeLocoSpeed(t_locoSpeed)) {
    sprintf(lcdString, "T.P. LOCOSPD ERR 1"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeed = t_locoSpeed;
  m_pTrainProgress[m_trainProgressLocoTableNum].currentSpeedTime = t_locoTime;
  return;
}

bool Train_Progress::blockOccursAgainInRoute(const byte t_locoNum, const byte t_blockNum, const byte t_elementNum) {
  // Rev: 08/03/24.  NEEDS TO BE TESTED!
  // Does a given block occur (again) further ahead in this route (regardless of direction)?
  // So we can release reservation if possible.
  // Requires an element number from Train Progress to know where to start searching.
  // Note that OCC needs this function (in addition to MAS) to know if the block should continue to show as reserved/occupied.
  // Beginning with the first element BEYOND t_elementNum, scan each element in Train Progress forward until we reach End-of-Route.
  // If we find t_blockNum then return true; else return false.  USUALLY we will return false.
  byte tempTPPointer = t_elementNum;  // Element number, not a sensor number
  routeElement tempTPElement;  // Working/scratch Train Progress element
  // We are assuming that the beginning element t_elementNum will point to a Sensor so can't be a Block record.
  do {
    // Advance pointer to the next element in this loco's Train Progress table...
    tempTPPointer = incrementTrainProgressPtr(tempTPPointer);
    tempTPElement = peek(t_locoNum, tempTPPointer);  // locoNum, NOT loco table num
    // See what it is and if it's a Block, see if it's t_blockNum...
    if ((tempTPElement.routeRecType == BE) || (tempTPElement.routeRecType == BW)) {  // Block record type found!
      // Is the block we're pointing at t_blockNum?
      if (tempTPElement.routeRecVal == t_blockNum) {  // Oooh! We found an occurance of the block farther ahead in the route!
        return true;
      }
    }
  } while (tempTPPointer != stopPtr(t_locoNum));  // Calling our local function Train_Progress::stopPtr()
  // When scanning ahead for Block records, we know when we've reached stopPtr that there can be no more Block records.
  return false;
}

bool Train_Progress::turnoutOccursAgainInRoute(const byte t_locoNum, const byte t_turnoutNum, const byte t_elementNum) {
  // Rev: 08/03/24.  NEEDS TO BE TESTED!
  // Does a given turnout occur (again) further ahead in this route (regardless of orientation)?
  // So we can release reservation if possible.
  // Requires an element number from Train Progress to know where to start searching.
  // Beginning with the first element BEYOND t_elementNum, scan each element in Train Progress forward until we reach End-of-Route.
  // If we find t_turnoutNum then return true; else return false.  USUALLY we will return false.
  byte tempTPPointer = t_elementNum;  // Element number, not a sensor number
  routeElement tempTPElement;  // Working/scratch Train Progress element
  // We are assuming that the beginning element t_elementNum will point to a Sensor so can't be a Turnout record.
  do {
    // Advance pointer to the next element in this loco's Train Progress table...
    tempTPPointer = incrementTrainProgressPtr(tempTPPointer);
    tempTPElement = peek(t_locoNum, tempTPPointer);  // locoNum, NOT loco table num
    // See what it is and if it's a Turnout, see if it's t_turnoutNum...
    if ((tempTPElement.routeRecType == TN) || (tempTPElement.routeRecType == TR)) {  // Turnout record type found!
      // Is the turnout we're pointing at t_turnoutNum?
      if (tempTPElement.routeRecVal == t_turnoutNum) {  // Oooh! We found an occurance of the turnout farther ahead in the route!
        return true;
      }
    }
  } while (tempTPPointer != stopPtr(t_locoNum));  // Calling our local function Train_Progress::stopPtr()
  // When scanning ahead for Turnout records, we know when we've reached stopPtr that there can be no more Turnout records.
  return false;
}

routeElement Train_Progress::peek(const byte t_locoNum, const byte t_elementNum) {
  // Rev: 03/05/23.  FINISHED BUT NOT TESTED ***********************************************************************************************
  // Return contents of T.P. element for this loco.  t_locoNum should be 1..TOTAL_TRAINS (not zero offset.)
  // A peek() into Train Progress function seems useful when we need to search the route elements for various functions.
  // For instance, looking ahead for matching Turnout or Block records to know if we can release a reservation.
  // Also when traversing elements due to sensor trips and clears, to execute those commands and find new sensors.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  return m_pTrainProgress[m_trainProgressLocoTableNum].route[t_elementNum];
}

byte Train_Progress::incrementTrainProgressPtr(const byte t_oldPtrVal) {
  // Rev: 03/04/24.  COMPLETE BUT NEEDS TO BE TESTED.
  // Gives us the rec num 0..139 (num route elements - 1) of the NEXT element in Train Progress (regardless of loco.)
  // DOES NOT actually change the value of the pointer -- the caller should do that if desired.
  // Can be used for any Train Progress pointer such as headPtr, stationPtr, etc.
  // Simple modulo arithmetic to "wrap around" when we hit the end of the array.
  return (t_oldPtrVal + 1) % HEAP_RECS_TRAIN_PROGRESS;
}

void Train_Progress::setNextToTripPtr(const byte t_locoNum, const byte t_nextToTripPtr) {
  // Rev: 04/11/24.
  // t_locoNum must be 1..50.  nextToTripPtr is an element number in this loco's Train Progress record.
  if (outOfRangeLocoNum(t_locoNum)) {
    sprintf(lcdString, "T.P. LOCONUM ERR 6"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr = t_nextToTripPtr;
  return;
}

void Train_Progress::setNextToClearPtr(const byte t_locoNum, const byte t_nextToClearPtr) {
  // Rev: 04/11/24.
  // t_locoNum must be 1..50.  nextToClearPtr is an element number in this loco's Train Progress record.
  if (outOfRangeLocoNum(t_locoNum)) {
    sprintf(lcdString, "T.P. LOCONUM ERR 7"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].nextToClearPtr = t_nextToClearPtr;
  return;
}

void Train_Progress::setTailPtr(const byte t_locoNum, const byte t_tailPtr) {
  // Rev: 04/11/24.
  // t_locoNum must be 1..50.  tailPtr is an element number in this loco's Train Progress record.
  if (outOfRangeLocoNum(t_locoNum)) {
    sprintf(lcdString, "T.P. LOCONUM ERR 8"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr = t_tailPtr;
  return;
}

void Train_Progress::setLastTrippedPtr(const byte t_locoNum, const byte t_lastTrippedPtr) {
  // Rev: 08/04/24.  Used by LEG, though also updated each time locoThatTrippedSensor() is called.
  // t_locoNum must be 1..50.  lastTrippedPtr is an element number in this loco's Train Progress record.
  if (outOfRangeLocoNum(t_locoNum)) {
    sprintf(lcdString, "T.P. LOCONUM ERR 9"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].lastTrippedPtr = t_lastTrippedPtr;
  return;

}


// ***** PRIVATE FUNCTIONS ***

void Train_Progress::addRoute(const byte t_locoNum, const unsigned int t_routeRecNum, const char t_continuationOrExtension,
  const unsigned long t_countdown) {
  // Rev: 08/07/24.  READY FOR TESTING -- I feel very good about this.
  // *** Be sure to call pTrainProgress->display(locoNum) before and after adding a route, with every combination of Extension and
  // *** Continuation, with the new route starting in Forward and Reverse. ******************************************************************************
  // 08/07/24: New logic to handle Continuation route that starts in Reverse -- loco must stop before beginning new Route.
  // PRIVATE FUNCTION called by public functions addExtensionRoute() and addContinuationRoute().
  // Append either an Extension or Continuation route.  Logic is combined since they are so similar.
  // Append an EXTENSION route WHEN TRAIN HAS BEEN STOPPED.
  // Append a CONTINUATION route WHEN TRAIN IS MOVING AND KEEPS MOVING (except if it needs to stop before reversing, of course.)
  // t_continuationOrExtension from globals can be either ROUTE_TYPE_EXTENSION = 'E' or ROUTE_TYPE_CONTINUATION = 'C'.
  // t_countdown is ms to wait before starting train (only possibly > 0 for an Extension route.)  pTrainProgress->timeToStart().
  //   This isn't writing to Delayed Action, but will be used when Delayed Action records are populated for this new route.
  // WE'RE GUARANTEED THE FOLLOWING:
  //   The first four Route Reference records are always         BX## + SN## + FD/RD## + VL##.
  //   The last  four Train Progress  records are always  VL## + BX## + SN## + VL00.
  //   (And if this is an Extension (stopped), then we're sitting on the nextToClear/nextToTrip/stop sensor.)
  //   The new BX## and SN## records will match up with the end of the previous route, so no need to overwrite them.

  // Differences when adding an EXTENSION (train is stopped) versus a CONTINUATION route (train is moving):
  //   1. POINTERS:
  //      EXTENSION (stopped) route needs to update nextToTripPtr because it's been sitting stopped and there was no route element
  //      ahead that it could have been advanced to; we can only advance nextToTrip after we add the new route elements.
  //      CONTINUATION (moving) route won't need to update nextToTripPtr because our moving train hasn't tripped it yet.
  //      In BOTH CASES:
  //        tailPtr and nextToClearPtr should remain unchanged in both cases.
  //        headPtr will be updated to the front of the new route in both cases.
  //        contPtr, stationPtr, crawlPtr, and stopPtr will be updated identically in both cases.
  //   2. SPEED COMMANDS AT END OF OLD ROUTE / BEGINNING OF NEW ROUTE:
  //      EXTENSION (stopped) routes, where the NEW route starts in FORWARD:
  //        Overlay by placing the new route's 3rd element (FD or RD) at (headPtr - 1), which will be the last element of the old
  //        route, which will always be VL00.  The train is already stopped (that final VL00 has already been executed,) and the
  //        first two elements of the new route (BX## + SN##) are always identical to the old route's 3rd & 4th from last elements.
  //      EXTENSION (stopped) routes, where the NEW route starts in REVERSE:
  //        Exact same logic as above (EXTENSION route that ended in Forward) because the loco is stopped.
  //      CONTINUATION (moving) routes, where the NEW route starts in FORWARD:
  //        Exact same logic as EXTENSION routes (FORWARD or REVERSE), except we'll ALSO need to overwrite the old
  //        route's 4th-from-last VL01 element with the Block Res'n default block speed, in order to keep moving at reasonable
  //        speed rather than slowing to Crawl at the end of the old route (the new Route should start at VL02=Low or faster.)
  //      CONTINUATION (moving) routes, where the NEW route starts in REVERSE:
  //        Exact same logic as EXTENSION routes (including leaving the penultimate VL01 element intact,) except we must RETAIN the
  //        VL00 element at the end of the old route, just before the new Route's inital RD00 element.
  //        The loco will be at a Crawl as it nears the end of the previous route, but the above logic overwrites the final VL00
  //        "Stop" element.  And since we're reversing direction (routes always end moving forward,) we need to stop first.
  //        The easiest way to do this is to overwrite starting with the new route's 3rd element, placed at headPtr.  This will
  //        leave the old route's final VL00 in place -- and the elements before that (BX## + SN##) are duplicated.

  // Since we know this loco will be continuing on either immediately or after some delay, we can set some of it's Train Progress
  // parameters right off the bat:
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].isParked = false;   // If we'll be moving again, we aren't parked!
  // But we don't know how long until we start moving, so we'll leave let MAS/OCC/LEG update "isStopped" manually.
  // We'll also leave "timeToStart" unchanged if this is a Continuation (not stopping) route; else set per passed t_countdown parm.
  if (t_continuationOrExtension == ROUTE_TYPE_EXTENSION) {
    m_pTrainProgress[m_trainProgressLocoTableNum].timeToStart = millis() + t_countdown;  // Could be zero or some delay in ms
  }
  routeElement tempElement;  // Used in various operations below as a scratch element routeRecType, routeRecVal i.e. BE01
  byte tempElementPtr = 0;   // Used in various operations below as a scratch pointer

  // **********************************************************************************************************************
  // *** IF THIS IS A CONTINUATION THAT CONTINUES FORWARD, OVERWRITE OLD ROUTE'S FINAL CRAWL WITH BLOCK'S DEFAULT SPEED ***
  // **********************************************************************************************************************
  // If this is a CONTINUATION (not stopping) route that will continue FORWARD (not starting in reverse), then we need to overwrite
  // the VL01 Crawl command at headPtr - 4 with the default speed of that block.
  if (t_continuationOrExtension == ROUTE_TYPE_CONTINUATION) {
    // Take the THIRD element of the new Route, which must always be either FD00 or RD00, and see which it is.
    tempElement = m_pRoute->getElement(t_routeRecNum, 2);  // Third element (zero offset) of new route better be FD or RD!
    if ((tempElement.routeRecType != FD) && (tempElement.routeRecType != RD)) {  // Yikes, fatal error!  Whahappen?
      sprintf(lcdString, "T.P. ER ERR A"); pLCD2004->println(lcdString); endWithFlashingLED(5);
    }
    // If it's an FD00, then we'll want to overwrite the old route's penultimate velocity element, VL01, with a VL element that's
    // the default speed for that block (typically VL02..VL04.)
    if (tempElement.routeRecType == FD) {  // Look up the default block speed and use it to overwrite the old VL01 speed
      // The VL01 command we want to overwrite will ALWAYS be 2 elements before the (old) Stop pointer (4 elements before the Head.)
      tempElementPtr = m_pTrainProgress[m_trainProgressLocoTableNum].stopPtr;  // Set our temp pointer to the Stop pointer
      tempElementPtr = decrementTrainProgressPtr(tempElementPtr);  // Stop pointer minus 1 (will be a BX## element)
      tempElementPtr = decrementTrainProgressPtr(tempElementPtr);  // Stop pointer minus 2 (will be a VL## element)
      // Note that the tempElementPtr will always be a VL01 record EXCEPT if we're looking at the initial Registration route, in
      // which case it would be VL00 (and irrelevant to anything.)  But we will never assign a Continuation route to an initial
      // Registration route, so we can always assume that tempElementPtr will be VL01 (else it's a bug.)
      // So let's just confirm tempElementPtr is pointing at a VL01 record, otherwise we have a major bug.
      if ((m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecType != VL) ||
        (m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecVal != 1)) {
        sprintf(lcdString, "T.P. ER ERR B"); pLCD2004->println(lcdString); endWithFlashingLED(5);
      }
      // Okay we found the "old" Train Progress element with the VL01 that needs a higher speed (at tempElementPtr.)
      // We'll want to retrieve the default block speed for the block number that follows the old VL01 element.
      // Route rules guarantee that the old Train Progress route ends with VL01 + BXNN + SNXX + VL00.
      // So just add 1 to the tempElementPtr and we'll be pointing at the block number we need the speed of
      byte blockElementPtr = incrementTrainProgressPtr(tempElementPtr);
      // It had better be a BE or BW record -- let's confirm.
      if ((m_pTrainProgress[m_trainProgressLocoTableNum].route[blockElementPtr].routeRecType != BE) ||
          (m_pTrainProgress[m_trainProgressLocoTableNum].route[blockElementPtr].routeRecType != BW)) {
        sprintf(lcdString, "T.P. ER ERR C"); pLCD2004->println(lcdString); endWithFlashingLED(5);
      }
      // Yay.  We know the block number to look up, and the Train Progress VL element that needs an updated speed.
      byte blockNum = m_pTrainProgress[m_trainProgressLocoTableNum].route[blockElementPtr].routeRecVal;
      byte blockDefaultSpeed = 0;
      if (m_pTrainProgress[m_trainProgressLocoTableNum].route[blockElementPtr].routeRecType == BE) {  // Eastbound speed
        blockDefaultSpeed = m_pBlockReservation->eastboundSpeed(blockNum);  // VL02..VL04
      } else if (m_pTrainProgress[m_trainProgressLocoTableNum].route[blockElementPtr].routeRecType == BW) {  // Westbound speed
        blockDefaultSpeed = m_pBlockReservation->westboundSpeed(blockNum);
      } else {  // Serious bug
        sprintf(lcdString, "T.P. ER ERR D"); pLCD2004->println(lcdString); endWithFlashingLED(5);
      }
      // blockDefaultSpeed retrieved from Block Reservation will be LOCO_SPEED_STOP, _CRAWL, _LOW, _MEDIUM, or _HIGH = 0..4.
      // This corresponds nicely with the value assigned to the route element .routeRecVal = 0..4 for "VL" speed elements.
      // Now we have a default speed, overwrite the 4th-from-last element of the "old" route.
      m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecVal = blockDefaultSpeed;  // Will be 2..4
    }  // if it's FORWARD
  }    // if it's CONTINUATION

  // ************************************************************************************************************************
  // *** FOR ANY COMBINATION OF EXTENSION/CONTINUATION STARTING FORWARD/REVERSE, START OVERLAYING THE NEW ROUTE BEGINNING ***
  // *** WITH THE THIRD ELEMENT OF THE NEW ROUTE (always either FD00 or RD00.)  THE ONLY DIFFERENCE IS WHERE TO START.    ***
  // *** FOR CONTINUATION ROUTES THAT START IN REVERSE:                                                                   ***
  // ***   Overlay starting with the 3rd element of the new route AT OLD ROUTE headPtr.                                   ***
  // *** FOR EXTENSION ROUTES, and for CONTINUATION ROUTES THAT START IN FORWARD:                                         ***
  // ***   Overlay starting with the 3rd element of the new route AT OLD ROUTE (headPtr - 1) == VL00.                     ***
  // ************************************************************************************************************************

  // Start by taking the THIRD element of the new Route, which must always be either FD00 or RD00, and see which it is.
  tempElement = m_pRoute->getElement(t_routeRecNum, 2);  // Third element (zero offset) of new route better be FD or RD!
  if ((tempElement.routeRecType != FD) && (tempElement.routeRecType != RD)) {  // Yikes, fatal error!  Whahappen?
    sprintf(lcdString, "T.P. ER ERR A"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }

  // Decide at which element in the old route we want to start adding records from the new route...
  if ((t_continuationOrExtension == ROUTE_TYPE_CONTINUATION) && (tempElement.routeRecType == RD)) {
    // For Continuation routes that start in Reverse, overlay starting with the 3rd element of the new route AT OLD ROUTE headPtr.
    tempElementPtr = m_pTrainProgress[m_trainProgressLocoTableNum].headPtr;
  } else {
    // In all other cases, overlay starting with the 3rd element of the new route AT OLD ROUTE (headPtr - 1) == VL00.
    tempElementPtr = decrementTrainProgressPtr(m_pTrainProgress[m_trainProgressLocoTableNum].headPtr);
  }
  // Okay, now tempElementPtr is pointing to the element where we want to start overlaying the new route (starting w/ 3rd element.)
  tempElement = m_pRoute->getElement(t_routeRecNum, 2);  // 2 = 3rd element because zero offset; will be FD or RD.
  if ((tempElement.routeRecType != FD) && (tempElement.routeRecType != RD)) {  // Yikes, fatal error!  Whahappen?
    sprintf(lcdString, "T.P. ER ERR 2"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  // Okay, we've just retrieved the third element of the incoming Route, which is either FD00 or RD00.
  // Let's plop it at the end of Train Progress old route...
  m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr] = tempElement;

  // ********************************************************************************************************************
  // *** NOW BEGIN WRITING ELEMENTS AND INCREMENTING headPtr UNTIL WE'VE WRITTEN THE ENTIRE NEW ROUTE (OR OVERFLOWED) ***
  // ********************************************************************************************************************
  // Start retrieving Route elements from Route Reference at the 4th element and adding them Train Process for this train.
  // Note that since the route elements are zero offset, the 4th element of the new route is routeElementNum 3.
  for (byte routeElementNum = 3; routeElementNum < FRAM_SEGMENTS_ROUTE_REF; routeElementNum++) {  // We'll stop at "ER00"
    tempElement = m_pRoute->getElement(t_routeRecNum, routeElementNum);
    // If we retrieve an ER end-of-route record, we're done retrieving route elements...
    if (tempElement.routeRecType == ER) break;
    // We just grabbed a legit route element (it's not the end of the route,) so add it to Train Progress headPtr...
    m_pTrainProgress[m_trainProgressLocoTableNum].route[m_pTrainProgress[m_trainProgressLocoTableNum].headPtr] = tempElement;
    // Now increment headPtr...
    m_pTrainProgress[m_trainProgressLocoTableNum].headPtr =
      incrementTrainProgressPtr(m_pTrainProgress[m_trainProgressLocoTableNum].headPtr);
    checkIfTrainProgressFull(t_locoNum);  // And be sure Train Progress for this loco isn't full...
  }
  // All of the elements of the new route have been concatenated onto the "old" route.

  // *******************************************************
  // *** NOW UPDATE ANY POINTERS THAT NEED TO BE UPDATED ***
  // *******************************************************
  // The Train Progress pointers should be updated as follows:
  //   headPtr        Should now have been updated, above, so nothing more to do with it.
  //   nextToTripPtr  If (and only if) we're adding an Extension (stopped) route, this pointer will need to be advanced to the next
  //                  sensor of the route. (A Continuation (non-stop) route's nextToTripPtr should still be valid.)
  //   nextToClearPtr Should still be valid from before calling this function.
  //   tailPtr        Should still be valid from before calling this function.
  //   contPtr        Must be updated to point to the 4th-from-last sensor of the new Train Progress route.
  //   stationPtr     Must be updated to point to the 3rd-from-last sensor of the new Train Progress route.
  //   crawlPtr       Must be updated to point to the 2nd-from-last sensor of the new Train Progress route.
  //   stopPtr        Must be updated to point to the last sensor of the new Train Progress route.
  //   lastTrippedPtr Should still be valid from before calling this function.

  // *** FOR NEXT-TO-TRIP, IF WE'RE ADDING AN EXTENSION (STOPPED) ROUTE, ADVANCE IT POINTS TO THE NEXT SENSOR OF THE ROUTE ***
  // If adding an extension route (where we are stopped), it means we are currently sitting stopped on top of the next-to-trip
  // sensor and we need to advance the pointer to the next sensor along our newly-added route.
  if (t_continuationOrExtension == ROUTE_TYPE_EXTENSION) {
    tempElementPtr = incrementTrainProgressPtr(m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr);
    while (m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecType != SN) {
      tempElementPtr = incrementTrainProgressPtr(tempElementPtr);
    }
    // tempElementPtr now holds the element number of our new next-to-trip Sensor.
    m_pTrainProgress[m_trainProgressLocoTableNum].nextToTripPtr = tempElementPtr;
  }

  // *** FOR CONT, STATION, CRAWL, and STOP POINTERS, START AT HEAD AND MOVE BACKWARDS UNTIL ALL FOUR ARE ASSIGNED ***
  // Note that we have a rule that the new route must have at least five sensors, so we won't under-run as we search backwards.

  // Find the STOP sensor location...
  // Don't start by checking if headPtr points to a sensor, because it always points to garbage -- one element beyond the front.
  tempElementPtr = decrementTrainProgressPtr(m_pTrainProgress[m_trainProgressLocoTableNum].headPtr);
  while (m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecType != SN) {
    tempElementPtr = decrementTrainProgressPtr(tempElementPtr);
  }
  // Okay, we're pointing to the LAST sensor of the new route.  This will be our STOP pointer.
  m_pTrainProgress[m_trainProgressLocoTableNum].stopPtr = tempElementPtr;     // Set new value for STOP pointer.

  // Find the CRAWL sensor location...
  tempElementPtr = decrementTrainProgressPtr(tempElementPtr);
  while (m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecType != SN) {
    tempElementPtr = decrementTrainProgressPtr(tempElementPtr);
  }
  // Okay, we've found the CRAWL (2nd from last) sensor.
  m_pTrainProgress[m_trainProgressLocoTableNum].crawlPtr = tempElementPtr;    // Set new value for CRAWL pointer.

  // Find the STATION sensor location...
  tempElementPtr = decrementTrainProgressPtr(tempElementPtr);
  while (m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecType != SN) {
    tempElementPtr = decrementTrainProgressPtr(tempElementPtr);
  }
  // Okay, we've stumbled across the STATION (3rd from last) sensor.
  m_pTrainProgress[m_trainProgressLocoTableNum].stationPtr = tempElementPtr;  // Set new value for STATION pointer.

  // Find the CONTINUATION sensor location...
  tempElementPtr = decrementTrainProgressPtr(tempElementPtr);
  while (m_pTrainProgress[m_trainProgressLocoTableNum].route[tempElementPtr].routeRecType != SN) {
    tempElementPtr = decrementTrainProgressPtr(tempElementPtr);
  }
  // Okay, we've come across the CONTINUATION (4th from last) sensor.
  m_pTrainProgress[m_trainProgressLocoTableNum].contPtr = tempElementPtr;     // Set new value for CONTINUATION pointer.

  // FINALLY, the entire new Continuation or Extension route has been added, and all 8 pointers are up to date.  Done!
  return;
}

byte Train_Progress::decrementTrainProgressPtr(const byte t_oldPtrVal) {
  // Rev: 03/04/24.  COMPLETE BUT NEEDS TO BE TESTED.
  // Gives us the rec num 0..139 (num route elements - 1) of the PREVIOUS element in Train Progress (regardless of loco.)
  // DOES NOT actually change the value of the pointer -- the caller should do that if desired.
  // Can be used for any Train Progress pointer such as headPtr, stationPtr, etc.
  // Strange but true -- this magical hocus pokus will convert -1 into HEAP_RECS_TRAIN_PROGRESS - 1 (i.e. 139.)
  // I.e. if HEAP_RECS_TRAIN_PROGRESS = 100, and we send t_oldPtrVal = 0, this function will return 99.
  // Same as take mod of number and then check if the result is less than 0, if so, just add the modular number.
  return (((t_oldPtrVal - 1) % HEAP_RECS_TRAIN_PROGRESS) + HEAP_RECS_TRAIN_PROGRESS) % HEAP_RECS_TRAIN_PROGRESS;
}

void Train_Progress::checkIfTrainProgressFull(const byte t_locoNum) {  // Check this loco's Train Progress buffer full
  // Rev: 03/04/23.  WRITTEN BUT NEEDS TO BE TESTED FOR BOTH CONDITIONS.
  // Since a full Train Progress array is always fatal, we'll just kill ourself here rather than returning a bool.
  // Since we may want to call this either before or after incrementing headPtr, we'll call it full in either case.
  // Rather than maintaining Train Progress COUNT, we simply disallow adding an element if (HEAD + 1) % SIZE = TAIL (i.e. can never
  // fill last element.)
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  if (((m_pTrainProgress[m_trainProgressLocoTableNum].headPtr) ==
    m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr) ||
    ((m_pTrainProgress[m_trainProgressLocoTableNum].headPtr + 1) % HEAP_RECS_TRAIN_PROGRESS ==
      m_pTrainProgress[m_trainProgressLocoTableNum].tailPtr)) {
    // Yikes, this loco's Train Progress is full!  This will be a fatal error.
    sprintf(lcdString, "TRAIN PROGRESS FULL!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  return;  // No full; room for at least one more Train Progress element for this loco.
}

// ***** OUT OF RANGE FUNCTIONS *****

// TOTAL_TRAINS = 50, thus Engine numbers limited to 1..50 (Train numbers limited to 1..9.)
// TMCC and Legacy Engine numbers = 1..99 (but 1..50 since we have TOTAL_TRAINS = 50)
// TMCC and Legacy Train numbers = 1..9 (built-in limitation of TMCC and Legacy)
// Legacy speeds = 0..199
// TMCC speeds = 0..31

bool Train_Progress::outOfRangeLocoNum(byte t_locoNum) {  // 1..50, disallow LOCO_ID_NULL (0) or LOCO_ID_STATIC (99)
  if (((t_locoNum < 1) || (t_locoNum > TOTAL_TRAINS)) &&
      ((t_locoNum < LOCO_ID_POWERMASTER_1) || (t_locoNum > LOCO_ID_POWERMASTER_4))) {    // 1..50 or 91..94
    return true;  // true means it's out of range
  }
  else {
    return false;
  }
}

bool Train_Progress::outOfRangeLocoSpeed(byte t_locoSpeed) {
  // We're going to assume this is a Legacy loco with speed range 0..199.
  // If we ever add support for TMCC locos, we'll need to add support here similar to Delayed_Action::outOfRangeLocoSpeed().
  if ((t_locoSpeed < 0) || (t_locoSpeed > 199)) {  // Only 0..199 are valid
    // Legacy Engine or Train speed can be 0..199
    return true;  // true means it's out of range
  }
  else {
    return false;
  }
}

bool Train_Progress::outOfRangeBlockNum(const byte t_blockNum) {
  if ((t_blockNum < 1) || (t_blockNum > TOTAL_BLOCKS)) {  // 1..26
    return true;  // true means it's out of range
  }
  else {
    return false;
  }
}

bool Train_Progress::outOfRangeSensorNum(const byte t_sensorNum) {
  if ((t_sensorNum < 1) || (t_sensorNum > TOTAL_SENSORS)) {  // 1..52
    return true;  // true means it's out of range
  }
  else {
    return false;
  }
}

// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************


/*

void Train_Progress::poke(const byte t_locoNum, const byte t_elementNum, const routeElement t_routeElement) {
  // Rev: 03/05/23.  FINISHED BUT NOT TESTED ***********************************************************************************************
  // poke() blindly replaces an existing Train Progress route element with whatever we send it.  For instance, when we add a
  // Continuation route, we may need to replace the last two speed commands (VL01, VL00) with speed commands for the new route.
  m_trainProgressLocoTableNum = t_locoNum - 1;  // m_trainProgressLocoTableNum 0..49 == t_locoNum 1..50
  m_pTrainProgress[m_trainProgressLocoTableNum].route[t_elementNum] = t_routeElement;
  return;
}


void Train_Progress::updatePointers(const byte t_locoNum) {  // Just a wild guess that this will be possible or appropriate as a separate function. *************************************
  // Can be used to update and/all of the 8 pointers in each Train Progress table.  Or maybe we'll just want to update them
  // manually as we process elements...???

  return;
}

*/
