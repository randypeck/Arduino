// DISPATCHER.CPP Rev: 02/09/21.
// Part of O_MAS.
// Run the layout in Auto or Park mode.

#include <Dispatcher.h>

Dispatcher::Dispatcher() {  // Constructor
  return;
}

void Dispatcher::begin(Message* t_pMessage, Loco_Reference* t_pLoco, Block_Reservation* t_pBlockReservation,
                       Turnout_Reservation* t_pTurnoutReservation, Sensor_Block* t_pSensorBlock, Route_Reference* t_pRoute,
                       Deadlock_Reference* t_pDeadlock, Train_Progress* t_pTrainProgress, Mode_Dial* t_pModeSelector) {
  m_pMessage = t_pMessage;
  m_pLoco = t_pLoco;
  m_pBlockReservation = t_pBlockReservation;
  m_pTurnoutReservation = t_pTurnoutReservation;
  m_pSensorBlock = t_pSensorBlock;
  m_pRoute = t_pRoute;
  m_pDeadlock = t_pDeadlock;
  m_pTrainProgress = t_pTrainProgress;
  m_pModeSelector = t_pModeSelector;
  return;
}

// Some notes about Auto/Park RUNNING, STOPPING, STOPPED:
// When in AUTO mode, RUNNING state, and press the (solid red) STOP button and allow all trains to stop by themselves:
//   This means we want to stop assigning new routes, but allow moving trains to continue until they reach their current
//   destination, and *then* stop.  This will be STOPPING state, and will automatically transition to STOPPED state.  The red
//   Stop button on the control panel will blink until the last train has stopped, at which time it will be extinguished.
//   At this point, all train positions are known and we will maintain "registered" status so the operator can re-start either
//   AUTO or PARK mode without having to re-register the layout.
//   IMPORTANT: AUTO+STOPPING should allow operator to change modes to PARK+RUNNING, so complete stop isn't required. *************************************
// When in PARK mode, RUNNING state, and do NOT press STOP, allowing all trains to stop by themselves:
//   We will immediately transition into STOPPING state, so the red Stop button will be flashing and cannot be pressed.
//   This is exactly the same as STOPPING in AUTO mode, except we won't be done until every train is in a 'P'ark-type siding.
//   We will run trains, putting priority on 'P'ark routes, until all trains have been stopped in 'P'ark sidings.
//   If we can't find a destination siding that is part of a 'P'ark route, we will assign a regular route, and then try again later
//   to assign a 'P'ark route for that train.  We will probably always find a 'P'ark route on our first try, but if not, we will
//   repeat this assignment until we do find a 'P'ark route that puts the train in a "parked" destination siding.
//   Or we could reach a point at which all-but-one trains are Parked, and the train that is in a non-Park siding (i.e.BW13) does
//   not have any route into *any* siding (i.e. B01 and B02 were already Parked.)  We'll need to track a flag so we know if we've
//   cycled all the way through with no possible moves, and thus we will just quit at that point (once all trains have stopped.)
//   If all goes well, the trains will eventually find sidings and stop by themselves, and we will set state to STOPPED.
//   The "registered" flag will be set to (or remain?) true, so that the operator could re-start either AUTO or PARK immediately.
// When in AUTO or PARK mode, STOPPING state, the flashing red STOP button will disregard presses.  Operator must Halt to stop.


void Dispatcher::dispatch(const byte t_mode, byte* t_state) {  // Operate the layout in Auto or Park mode.

  // Since Train_Progress will have 50 possible trains, need an easy way to only examine active Locos and not look at all 50. *********************************
  // Need an small array we can scan of each train that includes Registered T/F, and a flag to indicate if the train is stopped
  // meaning done for the session -- when operator presses Stop button in Auto and this train has reached its final block,
  // or when running in Park and this train has reached its final block.
  // Probably the array will only use as many elements as their are running trains, rather than one element per train, so
  // we only scan the number of records as there are trains.  Each record will have the train number = index into the Train
  // Progress table.

//byte numRegisteredTrains = 0;  // We can calculate this based on data found in Train_Progress.

// HOLD ON: Rather than setting up this 150-element array just so we don't need to scan all 50 Train_Progress records, let's just
// add a couple of flags to each Train_Progress element: bool activeTrain and bool trainParked.
// It's not much slower to scan our FRAM-based Train_Progress table and just look at that first element than it is to scan a
// trainTracker array, is it?  I guess it depends if we load the entire Train Progress record for each train.

 // 12/18/20: Train Progress: need to make struct w/ 'active', 'parked', head, tail, and count fields followed by 140 2-byte elements. ***********************************


  // If we scan this small array and all active trains have their "trainParked" flag set to true, we're done with this function.
  struct trainTracker {
    byte locoNum;  // Train number 1..TOTAL_TRAINS (i.e. 1..50)
    byte progressIndex;  // Which element of the Train Progress struct array is this train in?  0..locoNum - 1.
    bool trainParked;    // Starts false; sets true when train is done "stopping" in either Auto or Park mode.
  };
  // We're going to give this array a full TOTAL_TRAINS elements, because we won't know how many we'll actually need until we run
  // the function that populates it (by scanning Train_Progress.)  We could scan Train_Progress in two passes, but if we're going
  // to potentially allow 50 trains, then we might as well find out now if an array that big blows up.
  // We can always put the trainTracker array on the heap, by creating it in setup() and sending this class a pointer.
// 
//trainTracker progressPointer[TOTAL_TRAINS];  // Allow up to an element for every possible Registered train.  150 bytes total.

// Write the code to scan Train_Progress (already built fresh and populated by Registar class), add an element to trainTracker
// for every "active" (non-Static) train, and keep adding 1 to numRegisteredTrains until done.




// Now write the Train_Progress class, which we might have an instance for every possible train, rather than loading them one at a
// time as we do with other classes.  Since it will be on the heap, nothing to store in FRAM, and plenty of heap space.
// We should instantiate the whole thing in MAS so it's on the "global" heap, not here.




  // Mode will initially be either AUTO or PARK, State will be RUNNING.

  // Train_Progress table will have one record for every registered loco
  // Loco table will have last-known block/direction set for each registered loco.  Un-registered locos can potentially be
  //   occupying STATIC blocks, or may not be on the layout at all.
  // Block Reservation table will have each occupied block reserved for either a real train or Static.
  // Turnout Reservation table will have all turnouts un-reserved.
  // Sensor-Block table will reflect last-known status of each sensor, ready for constant updates each time sensor status changes.
  // Route and Deadlock tables are ready for lookups.

  bool allTrainsAreStopped = true;  // Set to false as soon as we start moving; set true when stopped in STOPPING mode


  // Get last-known turnout positions, and throw them all, one at a time.
  // Sending 'T'urnout messages to SWT will also be seen by LED to update green control panel LEDs.

  // Call a function that compares the following to be sure they all match up and there are now widows or orphans:
  // * Train_Progress: Each active train should occupy one specific block and direction; thus we will know the sensor number;
  // * Block_Reservation: Be sure there is precisely one reserved block to match each loco in Train_Progress, no more/no less.
  // * Sensor_Block: Be sure there is precisely one occupied sensor to match each loco in Train_Progress, no more/no less.
  // If Block_Reservation and Sensor_Block both match perfectly to Train_Progress, I don't think there is a need to compare
  // Block_Reservation directly against Sensor_Block.


  // OCC will independently start maintaining its own tables so keep the Red/Blue and White block-occupancy LEDs appropriately lit.




  // If we are in PARK mode, we automatically want to transition right into STOPPING state.
  if (t_mode == MODE_PARK) {
    // Note: t_state is *already* a pointer, so don't need to use an asterisk when calling from here.
//    m_pModeSelector->setStateToSTOPPING(t_mode, t_state);
  }

  // So long as we are RUNNING or STOPPING (not STOPPED), we'll zoom around this big loop.  Then we're done.
  while (*t_state != STATE_STOPPED) {
    // If RUNNING in either AUTO or PARK, see if operator wants to transition into STOPPING.
    if (*t_state == STATE_RUNNING) {
      // Note: t_state is *already* a pointer, so don't need to use an asterisk when calling from here.
      m_pModeSelector->stopButtonIsPressed(t_mode, t_state);  // Changes t_state to STOPPING if operator pressed Stop button
    }
    // If STOPPING, we must paint Stop button LED at least every 500ms so it blinks
    if (*t_state == STATE_STOPPING) {  // No harm done to paint it in any mode, but no point unless STOPPING
      // Note: paintModeLEDs expects const state, not a pointer, so we must use asterisk here.
      m_pModeSelector->paintModeLEDs(t_mode, *t_state);
    }
    // Now we are in either AUTO or PARK, RUNNING or STOPPING.

    // Check for incoming SENSOR_CHANGE message.  This is the *only* incoming message we will receive, and drives our actions.



    // If a train is MOVING and has just tripped it's Train Progress penultimate exit sensor, *and* we are not stopping, try to
    // find a "next" route for this train.  Note that we could also be in Park mode, which is always "stopping", but we might not
    // yet have a Destination for our train that is part of a Park-type route -- in which case we may still need to schedule
    // another route.





    // If a train is STOPPED, and we are either 1) in AUTO RUNNING mode, or 2) in PARK STOPPING mode *but* not yet in a "parking"
    // block, then let's see if we can find a new route (taking into account if we're looking for a Parking block, etc.)
    // Even if there is more than one train stopped, we will only schedule a route for one train at a time, in order to keep the
    // loop running as quickly as possible (to re-paint the red Stop LED, check for another sensor change, etc.)



    if ((*t_state == STATE_STOPPING) && (allTrainsAreStopped)) {  // In either AUTO or PARK mode, but park mode trains must also be parked, so add this code ***************************
       m_pModeSelector->setStateToSTOPPED(t_mode, t_state);  // This will cause us to exit this "dispatch" function
    }

  } // End-of-While-Loop

  return;
}

bool Dispatcher::trainPermittedInSiding(const byte t_locoNum, const routeElement t_blockNum) {

  return false;
}

/*
// See if candidate destination is appropriate for this loco based on type (pass/frt) and length.
// 01/26/23: THIS IS JUST A BUNCH OF JUNK COPIED FROM DEADLOCK.CPP that shows how I did it there...
bool Dispatcher::trainPermittedInSiding(const byte t_locoNum, const routeElement t_blockNum) {
  // NOTE: Dispatcher will call trainPermittedInSiding to see if a candidate destination is even an option, but then must call
  // Deadlock_Reference::deadlockExists(destination, locoNum);

  const byte candidateBlockDir = t_candidateDestination.routeRecType;
  const byte candidateBlockNum = t_candidateDestination.routeRecVal;
  const unsigned int trainLength = m_pLoco->length(t_locoNum);
  const char trainType = m_pLoco->passOrFreight(t_locoNum);  // Returns P|p|F|f (or blank or other should be allowed.)

  Serial.println(candidateBlockNum);
  Serial.print("Train length: "); Serial.print(trainLength); Serial.print(", Train Type: "); Serial.println(trainType);

  // 1. Is t_locoNum too long to fit into this candidate siding, OR
  // 2. Is t_locoNum of a type (i.e. freight) that is not allowed to enter this candidate siding?
  // or put another way...
  // 1. Is t_locoNum short enough to fit into this candidate siding; AND
  // 2. is t_locoNum of a type (i.e. freight) that IS ALLOWED to use this candidate siding?

    // Let's first see if this threat siding will even allow our length and type of train, before we see if it's occupied...
  const unsigned int threatBlockLength = m_pBlockReservation->length(threatBlockNum);
  if (trainLength > threatBlockLength) {  // Train too long; forget this threat block as a possibility
    Serial.println("Loco is too long.");
    continue;  // Go back up to the top of this for-loop and try the next threat
  }
  // Okay, we know that our train is long enough to fit in the threat siding.  How about restrictions?
  // We have trainType = P/p/F/f.  Block Reservation has "Forbidden" P/F/L/etc.
  // Block Reservation only allows P or F to be forbidden, which includes both through and local for simplicity.
  // If we add more possibilities to either Train Type or Block Forbidden, we'll need to update this code.
  if ((((trainType == 'P') || (trainType == 'p')) && (m_pBlockReservation->forbidden(threatBlockNum) == 'P')) ||
    (((trainType == 'F') || (trainType == 'f')) && (m_pBlockReservation->forbidden(threatBlockNum) == 'F'))) {  // Forbidden
    Serial.print("Loco type "); Serial.print(trainType); Serial.print(" forbidden in Block "); Serial.println(threatBlockNum);
    continue;  // Go back up to the top of this for-loop and try the next threat
  }
  // Well, our train is long enough and not forbidden to enter the threat siding.  Is it reserved for some other train though?


}
*/