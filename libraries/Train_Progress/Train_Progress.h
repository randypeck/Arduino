// TRAIN_PROGRESS.H Rev: 08/04/24.  HEAP STORAGE.  Used by MAS, LEG, and OCC.  Not needed by SNS or LED.
// Part of O_MAS, O_OCC, and O_LEG.
// Keeps track of the route and location of each train during Registration, Auto and Park modes.
// 08/04/24: Added lastTrippedPtr to Train Progress.  LEG needs to keep track of where the loco is located and I can't think of a
// way to check that using nextToClearPtr, nextToTripPtr, stopPtr, headPtr, etc.
// 08/05/24: Removed expectedStopTime field and functions as not needed for anything.

// The Train Progress table is used by MAS, LEG, and OCC during Registration, Auto and Park modes.
//   Train Progress is cleared then populated (enqueued) with its inital parked position during Registration.
//   Train Progress is enqueued, monitored, and dequeued during Auto/Park modes.
// LED and SNS also operate in Auto/Park modes, but don't incorporate any special logic and don't need access to Train Progress.
// This class can use THIS_MODULE to determine which module it is a class of if different behavior is needed per module.

// See Route_Reference.h for a list of Route Rules.

// FOR TESTING: In addition to display(), it would be great to have a function that ran Train Progress and updated Delayed Action
// but didn't send Legacy commands to locos; it just accepted routes from MAS and monitored sensor trips/clears and updated the
// serial display (and maybe thermal printer) each time any change was made.  Then we manually drive the loco (or push a car) along
// the route to confirm correct operation.

// FOR TESTING: Be sure to include a route that trips at least one sensor twice, such as BE03 to BE02 and BW03 to BE02.  Just to be
// sure that our pointer logic is working even when multiple pointers point at (different elements containing) the same sensorNums.

// IMPORTANT: It's possible that a sensor can appear more than once in a Route, such with a route that ends by backing into a
// single-ended siding (and then pulling forward), or (in the future) possibly with a reverse loop, or a circular loop route.
// Even with single-level operation, we have a Route that moves from BE03 to BE02, which trips SN04 twice.
// Thus, CONT, STATION, CRAWL and STOP pointers (which contain ELEMENT NUMBERS not sensor numbers) can sometimes contain the SAME
// Route element number and thus point to the same sensor in the Route!
// It is also possible to have a sensor that we CLEAR twice in the same route, such as when traversing a reverse loop.
// Whether we Trip Next-To-Trip or Clear Next-To-Clear, that sensor MIGHT still occur ahead again in our Route.
// These MIGHT NOT even be at the end of the route, so we can't count on those being CONT/STATION/CRAWL/STOP, but they might be.
// FOR EXAMPLE, consider a route that backs into BE02, tripping SN31 then SN04 then SN03, and then moves FORWARD to SN04 (again.)
// The first time we trip SN04 it will be pointed to by Next-To-Trip *and* STATION *and* STOP!
// So how do we know if we've tripped SN04 as the STATION vs STOP sensor?  Easy!  Rather than looking at the Sensor Numbers, we
// look at the Train Progress Element Numbers.  Only one of those "last four sensors" pointers will contain the same Element
// Number as Next-To-Trip, so that's the one that we tripped.  We don't even need to check that it points to the same sensor
// number as Next-To-Trip, because it's implied: a Train Progress element number contains the same data, regardless of which
// pointer(s) is/are pointing at it!

// NOTE REGARDING TRAIN PROGRESS POINTERS:
//   The HEAD, TRIP, CLEAR, and TAIL pointers bracket the entire Train Progress route, and help us keep track of the train's
//     progress around the layout, and also which blocks are occupied versus reserved versus cleared.
//   The CONT, STN, CRAWL, and STOP pointers are always at the end of each route, even if there are stops to revers direction
//     earlier in the route.  These pointers help us know when we can assign new Continuation and Extension routes, and also when
//     to turn on the bell and make announcements etc. when coming to a station stop.
//   LAST_TRIPPED_PTR points to the element the loco is currently sitting on (if stopped) or most recently tripped.  Initially set
//     to the STOP/CLEAR/TRIP pointer values when a new train is Registered, then updated during Auto/Park each time
//     locoThatTrippedSensor() is called.  Can also be updated via setLastTrippedPtr() and retrieved via lastTrippedPtr().

// TERMINOLOGY: To differentiate when a train stops at it's destination re-starting, versus non-stop route extension:
// * EXTENSION ROUTE when a train is stopped (incl. just following Registration), or is about to stop, and starts on a newly-
//   appended route.
// * CONTINUATION ROUTE when a train is moving, and a new route is appended immediately following tripping of contPtr.
// For a Continuation route, if the new route begins in FORWARD, we will overwrite the previous route's VL01 with the default speed
// for that block found in the Block Reservation & Reference table.
// For a Continuation route that begins in REVERSE, we will have to stop the loco at the end of the previous route, before we can
// start moving in Reverse for the new route.  So we will retain the previous route's VL01 rather than overwriting it.

// *** WHO NEEDS TRAIN PROGRESS INFO ***
// MAS Dispatcher will find and broadcast initial location and direction to Train Progress at Registration.
// MAS Dispatcher will broadcast Extension/Continuation routes as each train progresses along it's route during Auto/Park modes.
// Train Progress will reserve Blocks and Turnouts at Registration and in Auto/Park as new route segements are assigned.
//     We're doing this in Train Progress, rather than in MAS, because OCC also needs to know Block and Turnout Res'n status.
// Train Progress will release Block and Turnout reservations as Tail sensors are cleared, if the Block(s) and/or Turnout(s) do not
//     occur again farther ahead in the route (i.e. they will be released the last time they are encountered.)
// MAS Dispatcher follows Train Progress to know when it can assign new Extension or Continuation routes for a given train.
// MAS Dispatcher looks for T.P. VL00 (stop) cmds to determine when to throw Turnouts (up until next VL00 train stop.)
//     NOTE: Because Routes contain multiple Turnout commands, and farther along a Route the same Turnout might need to be switched
//     the other way, Dispatcher can't simply throw the entire route at the time it is assigned.  So it only throws Turnouts
//     in Train Progress that occur from the current position through the next VL00.  Then, after each successive stop, repeat.
//     Thus a turnout cannot appear twice between a pair of VL00 records.
// LEG Conductor needs to follow T.P. to know when to populate the Delayed Action table with speed, whistle, etc. commands;
//     typically each time a sensor is tripped.  LEG can disregard Turnout commands.
// LEG Conductor will need to access the Block Ref/Res'n table for default block speed when appending Continuation routes (that
//     begin in Forward) where we don't want to stop the train via the VL01/VL00 commands from the end of the previous route.
// OCC needs to know which Sensor and Block LEDs to illuminate, etc.  The red/blue Block LED status will mirror logic used by MAS
//     to reserve and un-reserve Blocks, and the white occupancy LEDs will only be illuminated when physically occupied.
// OCC Updates it's copy of Block Res'n so it will have a default last-known-loco in a given block during Registration.
// OCC will follow the route and sensor pointers to determine when to send PA Announcements to stations.
// LED will autonomously display green turnout LEDs by monitoring messages to SWT; no need to maintain Train Progress.

// ********************************************************************************************************************************
// ********************************************************************************************************************************
// ********************************************************************************************************************************
// ***                                                  VERY IMPORTANT POINT!                                                   ***
// ***  SENSOR TRIPS CAN ONLY OCCUR AT THE "NextToTrip" SENSOR!  (Could *also* be "NextToClear" if train is between sensors.)   ***
// ***  SENSOR CLEARS CAN ONLY OCCUR AT THE "NextToClear" SENSOR!                                                               ***
// ***  So we only need to monitor one sensor per train for trips and one for clears, and easily accessed via pointers.         ***
// ***  If any other sensor trips or clears within a Train Progress record, that's a bug that triggers an emergency halt.       ***
// ***  If the sensor is not found in any train's Train Progress record, that's also a bug that triggers an emergency halt.     ***
// ********************************************************************************************************************************
// ********************************************************************************************************************************
// ********************************************************************************************************************************

// *** SPEED UP/SLOW DOWN RULES ***
// All VLxx Velocity commands must follow sensor elements in Train Progress (and Routes.)
// VL00 = Stop immediately.  Locos must be moving at (nearly) VL01 Crawl speed whenever a VL00 Stop command is encountered.
// VL01 = Use block length to slow to crawl at precisely next sensor trip.
// Thus, the penultimate sensor before a VL00 command must be followed by a VL01 Crawl speed command *and* a BExx/BWxx block
//   element.  The VL01 tells the system that we want to be going at Crawl speed before tripping the next sensor, and the
//   BE/BW block command allows the system to lookup the block length, so it can calculate delay/momentum/speed parameters.
// VL02..VL04 = adjust speed using Medium momentum and don't worry about block length.  If we ever find that we don't reach the
//   target VL02..VL04 speed before encountering a subsequent speed-change record, we can always implement slow-down algorithms
//   that consider the speed change needed and the distance until we must reach that target.  Perhaps using our old quadratic
//   equations that we originally considered for the "slow to a crawl" logic will give us at least a ballpark.  Or, we could look
//   at block length in the Block Res'n/Ref table every time we encounter a VL02..VL04 command, if necessary.

// Here is an interesting sequence that we may want to add special logic for, to prevent it from being too boring:
// [moving westbound in Reverse]: SN04, VL01, BW02, SN03, VL00, FD00, VL01, BE02, SN04, VL00
// Upon tripping SN04, use the length of BW02 to slow to VL01 (Crawl.)
// Upon tripping SN03, remove any remaining speed commands from Delayed Action and insert a "stop immediate" element.
// Accelerate from stop to VL01 Crawl using medium momentum (ignore Block length since speed <= VL01)
// Upon tripping SN04, remove any remaining speed commands from Delayed Action and insert a "stop immediate" element.
// Note that we still have a boring situation where we have stopped then Crawled then stopped within the destination block.
// At some point, we may want to add special logic so we can speed up a little bit before slowing back to a Crawl.

// *************************************************************************************
// ***   SPECIFIC CONDITIONS THAT EACH MODULE NEEDS TO MONITOR TRAIN PROGRESS FOR.   ***
// *************************************************************************************

// MAS needs a function: each time a Route encounters FD00 or RD00 (change direction), throw all of the turnouts that fall from
//   that element in Train Progress through the next occurrence of FD00 or RD00, or the HEAD if end of route.  Since both Extension
//   and Continuation routes will include FD00 or RD00, we're assured of acting on new routes as well.

// MAS and OCC need a function: Each time a loco clears a NextToClear sensor, before advancing pointers, release all Block and
//   Turnout reservations between Tail and this sensor IF AND ONLY IF the Block/Turnout does not occur farther ahead in this Train
//   Progress route.  Can still advance Tail because the turnout will get un-reserved when it is next encountered in the route.

// MAS and OCC need a function: "Is there another occurrence of Block X (BE or BW) farther ahead in Train Progress" so can decide
//   if it is okay to release a block reservation, for blocks that are ahead of Tail but behind the just-cleared Next-To-Clear.
//   MAS needs it to release reservations, but also OCC will want to appropriately illuminate control panel block LEDs.
//   AND/OR MAYBE WE INCLUDE FUNCTIONS HERE IN loop() that MAS/OCC can call each time through its loop:
//     for (int i = 0; i < TOTAL_TRAINS; i++) {  // Train Progress object array elements start at 0 (not 1.)
//       pTrainProgress[i].releaseTurnouts;  // Release any reserved turnouts that are no longer in our route ahead
//       pTrainProgress[i].releaseBlocks;    // Release any reserved blocks that are no longer in our route ahead
//     }

// Need a function: "Is there another occurrence of Turnout X farther ahead in Train Progress" so we can decide if it is ok to
//   release a turnout reservation, for turnouts that are ahead of Tail but behind the just-cleared Next-To-Clear.
// Train Progress is responsible for reserving and releasing block and turnout reservations in a route, because both MAS and OCC
// need Block reservations to be kept up to date (not just MAS.)
// NOTE that we can still advance Tail to the just-cleared Next-To-Clear even if a block or turnout occurs again ahead in the
//   route, because its reservation will be released the *last* time it occurs in the route - regardless of the Tail pointer.
// MAS needs a function: Is it okay to send a new/continuation route, based on if contPtr tripped, stnPtr *not* tripped, Park
//   status, etc.  Maybe routinely call each time a sensor is tripped, and if function returns true, then assign new route.
// OCC: Needs to know when to send PA Announcements.  In addition to when starting a new route (i.e. "Train departing"):
//   stationPtr trip: PA Announcement "Train x now arriving at station...", stopPtr trip: PA Annoucement "Train has arrived." etc.
// OCC: isStopped (i.e. at a station, not just to reverse): Could make a "watch your step" announcement or similar.
//   These could be functions of Train Progress that is only called by OCC, returns true/false: "Did we just trip penultimatePtr?"
//   or "Did we just stop at a station?"
// LEG Conductor: When we trip NextToTrip (and when we first begin a new Train Progress route, when we are already sitting on that
//   sensor): Create new records in Delayed Action for all DIRECTION and VELOCITY elements between here and the next sensor (which
//   will become the new NextToTrip.)  This includes: FDnn/RDnn direction and VLxx speed.
// LEG: If we see a VL01 "Crawl" element, we are guaranteed to also find a BExx/BWxx "Block" element, which we must use to get
//   block length -- to be used in calculating stopping distance/momentum parameters.
// LEG: If we see a VL02..VL04 Velocity element, simply populate Delayed Action with speed-up or slow-down commands to adjust speed
//   from the current speed to the new requested speed, using medium momentum.
// MAS Dispatcher: When we trip a loco's "contPtr" sensor, optionally search for and send a Continuation Route to LEG Conductor.

// * TRIP NEXT-TO-TRIP SENSOR: Traverse Train Progress from NEXT-TO-TRIP through the next SENSOR record, execute records as
//   appropriate, and finally set NEXT-TO-TRIP to that not-yet-tripped Sensor.
//   NOTE: Don't advance NEXT-TO-TRIP if [NEXT-TO-TRIP] + 2 == HEAD, as that means we're at the end of our Route (for now.)
//   - VL Velocity command: LEG Conductor: Depopulate/populate Delayed Action as necessary.
//   - BE, BW Block command: Nothing to do.
//   - TN, TR Turnout command: Nothing to do.
//   - FD, RD Direction change command: Conductor: Write record to Delayed Action.
//   - FD, RD Direction change command: MAS scan ahead Train Progress and execute every turnout command until reaching the next
//       Direction command or End-of-Record (this may "pass" beyond the next sensor, no problem.)
//   - ER End-of-Route (really, looking at headPtr, which will be two elements ahead of NEXT-TO-TRIP): Exit function.
//   - SN Sensor: When we encounter the next sensor ahead, set NEXT-TO-TRIP to that record and exit function.

// * CLEAR NEXT-TO-CLEAR SENSOR: Traverse Train Progress from (old) TAIL through (just-cleared) NEXT-TO-CLEAR, execute records as
//   appropriate; finally set TAIL to the old NEXT-TO-CLEAR, and advance NEXT-TO-CLEAR to the next sensor ahead in Train Progress.
//   As we examine elements from TAIL to the just-cleared NEXT-TO-CLEAR:
//   - VL Velocity: Nothing for anyone to do.
//   - BE, BW Block command: MAS Dispatcher: Check the rest of Train Progress ahead, and for any block that does not appear again
//       *in either direction*, release that block's reservation.
//   - TN, TR Turnout command: MAS Dispatcher: Check the rest of Train Progress ahead, and for any turnout that does not appear
//       again, release that turnout's reservation.
//   - FD, RD Direction change command: Nothing for anyone to do.
//   - ER End-of-Route: I don't think it's possible to clear the last sensor in a route.
//   - SN Sensor: When we encounter the next sensor ahead, set TAIL = (old) NEXT-TO-CLEAR, set NEXT-TO-CLEAR to the
//     newly-encountered sensor record.

//   ******************************************************************************
//   ***** EXPLANATION OF TRAIN PROGRESS FLAGS AND POINTERS *** REV: 03/05/23 *****
//   ******************************************************************************
//
//   isActive  is set false by MAS/OCC/LEG Train Progress when Registration begins (resetTrainProgress().)
//   isActive  is set true  by MAS/OCC/LEG Train Progress when MAS Dispatcher sends setInitialRoute() and never changes.
//   isParked  MAS ONLY is set true  by MAS/OCC/LEG Train Progress when Registration begins.
//   isParked  is set true/false by MAS/OCC/LEG Train Progress each time it receives an Extension or Continuation Route, based on
//             the destination block's type (parking true/false.)  Thus, it will indicate if the loco *will be* parked upon
//             finishing the current Route.
//             Needed only by MAS, in Park mode, but could potentially also be used by LEG to shut down loco for nice effect.
//   isStopped is set true  by MAS/OCC/LEG Train Progress when Registration begins (resetTrainProgress().)
//   isStopped is set true  by MAS/OCC/LEG Train Progress when stopPtr "stop" sensor is tripped.
//   isStopped is set false by MAS/OCC/LEG Train Progress each time any other sensor is tripped, or any sensor clears.
//             Can't let Engineer keep it up to date because MAS needs it.  So we have to rely on sensor trips/clears.
//   isStopped could alternatively be set false by MAS/OCC/LEG Train Progress each time nextToTripPtr is advanced.  Train may not
//             actually be moving at that point, but it will be moving soon-ish (per timeToStart.)
//   timeStopped is updated by MAS/OCC/LEG Train Progress each time isStopped is set true.
//   timeToStart is set to 99999999 by MAS/OCC/LEG Train Progress  when Registration begins (resetTrainProgress().)
//   timeToStart is updated by MAS/OCC/LEG Train Progress each time an Extension Route is received.  This field is not relevant
//             when setInitialRoute() is transmitted because there is nowhere to go, and not relevant when addContinuationRoute()
//             is transmitted because we will already be moving.
//             The field can be used by OCC and LEG to make announcements, and of course by Conductor to start the train moving.
//   currentSpeed LEG ONLY is set to 0, and currentSpeedTime is set to the current time millis() by MAS/OCC/LEG Train Progress when
//             Registration begins (resetTrainProgress().)  Actually currentSpeedTime isn't even used by LEG.
//   currentSpeed and currentSpeedTime are kept up to date by (only) LEG Engineer, each time a speed-related command is sent to the
//             Legacy Command Buffer/Base.  They're used/needed ONLY by LEG Conductor when populating a D.A. speed change.
//             Conductor will always use this as our starting speed when calculating new accel/decel commands.  If the value is not
//             an exact match for one of the loco's three speeds (L/M/H) then we have some sort of a problem, likely a "bug" in the
//             Route Reference spreadsheet (not allowing enough time/distance to reach a subsequent speed.)
//
// NOTE: The following pointer fields are ELEMENT NUMBERS, NOT SENSOR NUMBERS.  Trivial array index lookup to get sensorNums.
//
// headPtr        Points to first empty Route element (often ER00 but garbage in any event), which is always the next element to
//                write.  The element headPtr will always be preceeded by a VL01 + Block + Sensor + VL00 records (end-of-route
//                rule.)
//                headPtr is updated:
//                  1. Initially set when train is registered, the first unused element in Train Progress.
//                  2. Each time a new route is appended to Train Progress.
// nextToTripPtr  Points to first not-yet-tripped sensor ahead of loco, if any, else front-most tripped sensor under train.
//                When this sensor trips:
//                  1. nextToTripPtr always advances to the next sensor (which will not yet be tripped), IF ANY.
// nextToClearPtr Points to the sensor number that is expected to clear next, as the loco moves.  IT IS NOT NECESSARILY ALREADY
//                TRIPPED and NOT NECESSARILY UNDER THE LOCO i.e. it could be ahead of the loco if the loco is between sensors.
//                Note that if a train is *between* sensors, nextToClearPtr will be *ahead* of the train (and will be same as
//                next-to-trip.)
//                When this sensor clears:
//                  1. Everything behind it (i.e. from tailPtr to here) can be released (even if turnout/block must remain
//                     reserved, because needing-to-remain-reserved blocks/turnouts will be release the *final* time they are
//                     cleared.
//                  2. This element becomes the new tailPtr.
//                  3. nextToClearPtr advances to the next sensor along the route.
// tailPtr        Points to most-recently-cleared sensor, if any, else rear-most sensor under train.  This will be the last
//                sensor of the route that anyone cares about.  Tail is always one sensor behind NextToClear, if any, and
//                advances to nextToClearPtr when nextToClearPtr advances, potentially releasing reserved turnouts and blocks
//                along the way (iff they do not appear again farther ahead in the route.)  I can't think of a scenario where
//                this would not be a cleared sensor, even at Registration.
// contPtr        Points to the 4th-from-last sensor in a route, if any, else (initially) points to the rear-most sensor.  Gets
//                updated by MAS (only) each time a new route is assigned to a train, whether a moving-continuation route or a
//                new route after stopping.
//                USED BY MAS DISPATCHER ONLY to know at what point it *may* assign a running-continuation route to an existing
//                route.  MAS gets just one chance to do this, when the contPtr sensor is tripped; else it must wait until the
//                loco stops at the next station before it may send a new route to Train Progress.
//                If we find that trains *always* stop at every siding, or *never* stop, then we can tweak this logic to make
//                things more interesting (sometimes stop but not always, perhaps never more than n route sections without a
//                stop.)  For now, we'll hope to sometimes but not always pass a new route before a train reaches the entry
//                sensor of the destination.  More likely to stop if more trains are running; less likely with fewer trains
//                running.
// stationPtr     Points to 3rd-from-last sensor in a route, if any, else (initially) points to rear-most sensor.  Since this
//                gets updated every time we add an extension route, we'll only trip it if we are for sure stopping at a station.
//                When tripped, LEG Conductor and OCC can send station arrival announcements.
// crawlPtr       Points to the 2nd-from-last sensor in a route.  Like stationPtr, only gets tripped when we know we're
//                stopping.  When tripped, Conductor will turn on the loco's bell, and maybe a horn toot too, but mostly it will
//                calculate exactly when we should start decelerating to reach Crawl speed at the moment we trip the Stop sensor.
//                Since this gets updated every time we add an extension route, we'll only trip it if we are for sure stopping at
//                a station.  Like contPtr and stationPtr, crawlPtr would not be hard to calculate by looking at headPtr and
//                traversing backwards two SNxx records, but LEG and OCC will query it every time a sensor is tripped, so we
//                might as well just record it each time a route is added, for easy lookup.
// stopPtr        Points to the last sensor in a route, where we are stopping.  When tripped, Conductor can turn off bell and
//                make an arrival horn toot.  LEG Conductor and OCC PA can send "train has arrived" announcements.
// lastTrippedPtr Points to the element number of the sensor that was most recently tripped by a loco.  It helps LEG Auto/Park mode
//                determine if it should start a stopped loco, such as when Auto/Park starts after Registration, or after a train
//                has stopped and is then assigned an Extension route.  Automatically updated when train is Registered, and whenever
//                locoThatTrippedSensor() is called.  Can also be updated by setLastTrippedPtr() and retrieved by lastTrippedPtr().

#ifndef TRAIN_PROGRESS_H
#define TRAIN_PROGRESS_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
//#include <FRAM.h>
//#include <Turnout_Reservation.h>  // I don't see why Train Progress (or LEG) needs Turnout Reservation
//#include <Sensor_Block.h>         // Don't think I need this cross reference
#include <Block_Reservation.h>    // Need to look up east/west sensorNum based on blockNum for initial route
//#include <Loco_Reference.h>       // Can't think why Train Progress would need Loco Ref.
#include <Route_Reference.h>  // Needed by Train Progress to look up Routes by Route Rec Num passed from MAS Dispatcher

class Train_Progress {

  // Functions expect locoNum, blockNum, etc. to start at 1 ("unreserved" LOCO_ID_NULL is 0 and LOCO_ID_STATIC is 99.)
  // FRAM Route Record Numbers (not Route IDs) start at zero.

  public:

    Train_Progress();  // Constructor must be called above setup() so the object will be global to the module.
    // Reserves heap memory for 50 Train Progress tables, all pointed to by a single pointer (a single class instance.)

    void begin(Block_Reservation* t_pBlockReservation, Route_Reference* t_pRoute);
    // Initializes the header and route for all 50 trains 1..50 to inactive; zeroes everything out.

    void resetTrainProgress(const byte t_locoNum);  // Initialize the header and no route for a single train 1..50.

    void setInitialRoute(const byte t_locoNum, const routeElement t_block);  // I.e. Loco 3 is in BE03 (sitting on Sensor 6.)
    // During Registration, setInitialRoute() sets up a new Train Progress record as each real train (not STATIC) is learned.
    // Sets up an initial set of Train Progress route elements reflecting the location and direction of the newly-registered train,
    // and updates all the various header fields (isActive, speed, pointers, etc.)
    // All it needs is a locoNum and a route element i.e. BW03 and will infer a sensor i.e. SN05.
    // LEG Conductor will then want to populate the Delayed Action table with startup parms and to start up the train, volocity
    // zero, forward direction, smoke on or off, etc.
    // STATIC trains aren't part of Train Progress -- MAS will simply reserve the blocks that STATIC trains are occupying during
    // registration, and thus those blocks won't be available as part of routes being assigned to Train Progress.

    void addExtensionRoute(const byte t_locoNum, const unsigned int t_routeRecNum, const unsigned long t_countdown);
    // Extension route begins with train stopped (in Auto or Park mode.)
    // t_countdown is ms to wait before starting train.

    void addContinuationRoute(const byte t_locoNum, const unsigned int t_routeRecNum);
    // Continuation route means the train is moving and won't stop as it continues into this route (unless the new route begins
    // with a direction reverse, in which case it obviously must stop long enought to reverse.)

    byte locoThatTrippedSensor(const byte t_sensorNum);  // Returns locoNum whose next-to-trip sensor was tripped.  Also updates
                                                         // lastTrippedPtr for the loco, using the sensor pointer number.
    byte locoThatClearedSensor(const byte t_sensorNum);  // Returns locoNum whose next-to-clear sensor was cleared.

    bool timeToStartLoco(const byte t_locoNum);  // Returns true if "timeToStart" > millis()

    void display(const byte t_locoNum);  // Sends one loco's Train Progress table to the Serial monitor.

    // *** PUBLIC GETTERS ***
    bool isActive(const byte t_locoNum);   // Not active means this loco is static or not on layout.
    bool isParked(const byte t_locoNum);   // MAS only
    bool isStopped(const byte t_locoNum);  // LEG only.  Kept up-to-date by Engineer.  Does not imply stopped at end of a route.
    unsigned long timeStopped(const byte t_locoNum);       // LEG only.  Kept up-to-date by Engineer.
    unsigned long timeToStart(const byte t_locoNum);
    byte currentSpeed(const byte t_locoNum);               // LEG only.  Kept up-to-date by Engineer.  0.199.
    unsigned long currentSpeedTime(const byte t_locoNum);  // LEG only.  Kept up-to-date by Engineer.

    // *** PUBLIC SETTERS ***
    void setActive(const byte t_locoNum, const bool t_active);    // True if Active
    void setParked(const byte t_locoNum, const bool t_parked);    // True if Parked
    void setStopped(const byte t_locoNum, const bool t_stopped);  // True if Stopped
    void setTimeStopped(const byte t_locoNum, unsigned long t_timeStopped);
    void setTimeToStart(const byte t_locoNum, unsigned long t_timeToStart);
    void setSpeedAndTime(const byte t_locoNum, const byte t_locoSpeed, const unsigned long t_locoTime);

    bool blockOccursAgainInRoute(const byte t_locoNum, const byte t_blockNum, const byte t_elementNum);
    // Does a given block occur (again) further ahead in this route (regardless of direction)?
    // So we can release reservation if possible.
    // Requires t_elementNum so it knows where in Train Progress to start looking.
    // Note that OCC needs this function (in addition to MAS) to know if the block should continue to show as reserved/occupied.

    bool turnoutOccursAgainInRoute(const byte t_locoNum, const byte t_turnoutNum, const byte t_elementNum);
    // Does a given turnout occur (again) further ahead in this route (regardless of orientation)?
    // So we can release reservation if possible.
    // Requires t_elementNum so it knows where in Train Progress to start looking.

// FUNCTIONS CALLED BY Occupancy_LEDs and maybe LEG Auto/Park.  Sloppy encapsulation, should be private, but okay for now...

    // Increment/Decrement Pointer functions return the next/prev rec num 0..139; they don't update the passed value.
    byte incrementTrainProgressPtr(const byte t_oldPtrVal);  // Just add 1, but use MODULO to wrap from end to 0.
    byte decrementTrainProgressPtr(const byte t_oldPtrVal);  // Just subtract 1, but use MODULO to wrap from 0 back to end.
    byte headPtr(const byte t_locoNum);                // Note: Head does not point to a sensor (just an empty element)
    byte nextToTripPtr(const byte t_locoNum);          // Returns element number that holds NextToTrip sensor.
    byte nextToClearPtr(const byte t_locoNum);         // Returns element number that holds NextToClear sensor.
    byte stopPtr(const byte t_locoNum);                // Returns element number that holds last sensor in route.
    byte lastTrippedPtr(const byte t_locoNum);         // Returns element number that holds the sensor most recently tripped.

    byte tailPtr(const byte t_locoNum);                // Returns element number that holds Tail sensor.
    routeElement peek(const byte t_locoNum, const byte t_elementNum);  // Return contents of T.P. element for this loco.
    // peek() allows us to search Train Progress elements without affecting the contents.
    // For instance, looking ahead for matching Turnout or Block records to know if we can release a reservation, and looking back
    // when we add route extensions if we need to change a VL01 to the block standard speed (which requires poke().)  Also when
    // traversing elements due to sensor trips and clears, to execute those commands and find new sensors.

    bool atEndOfRoute(const byte t_locoNum);  // True if lastTrippedPtr == stopPtr.
    void setNextToTripPtr(const byte t_locoNum, const byte t_nextToTripPtr);
    void setNextToClearPtr(const byte t_locoNum, const byte t_nextToClearPtr);
    void setTailPtr(const byte t_locoNum, const byte t_tailPtr);
    void setLastTrippedPtr(const byte t_locoNum, const byte t_lastTrippedPtr);

private:

    void addRoute(const byte t_locoNum, const unsigned int t_routeRecNum, const char t_continuationOrExtension,
      const unsigned long t_countdown);


    void checkIfTrainProgressFull(const byte t_locoNum);
    // We're calling it "check if" because it doesn't return a bool; it just fatal errors the system if T.P. is full.

    bool outOfRangeLocoNum(byte t_locoNum);            // 1..50, disallow LOCO_ID_NULL (0) or LOCO_ID_STATIC (99)
    bool outOfRangeLocoSpeed(byte t_locoSpeed);        // 0..199, assumes we only support Legacy and not TMCC locos.
    bool outOfRangeBlockNum(const byte t_blockNum);    // 1..26
    bool outOfRangeSensorNum(const byte t_sensorNum);  // 1..52

    // TRAIN PROGRESS STRUCT.  This struct is known only within this class.
    // We will have a HEAP array of 50 trainProgressStruct records (one for each of up to TOTAL_TRAINS trains.)

    struct trainProgressStruct {
      bool          isActive;
      bool          isParked;
      bool          isStopped;
      unsigned long timeStopped;
      unsigned long timeToStart;
      byte          currentSpeed;    // 0..199 (Legacy) or 0..31 (TMCC.)
      unsigned long currentSpeedTime;
      // Note: The following pointers are ELEMENT NUMBERS in Train Progress; i.e. 0..(HEAP_RECS_TRAIN_PROGRESS - 1).
      byte          headPtr;
      byte          nextToTripPtr;
      byte          nextToClearPtr;
      byte          tailPtr;
      byte          contPtr;         // 4th-from-last sensor.  Cont'n route may be assigned once this sensor is tripped.
      byte          stationPtr;      // 3rd-from-last sensor.  Cont'n route may not be assigned once this sensor is tripped.
      byte          crawlPtr;        // Penultimate sensor must always slow to Crawl, turn on loco's bell, etc.
      byte          stopPtr;         // Last sensor in route, must always stop immediately when tripped.
      byte          lastTrippedPtr;  // Element of sensor that loco is sitting on, if stopped, or most recently tripped, if moving.
      routeElement  route[HEAP_RECS_TRAIN_PROGRESS];  // 0..(HEAP_RECS_TRAIN_PROGRESS - 1) = 0..139.  I.e. BW03, TN23, VL00.
    };

    // Create a pointer variable for the entire Train Progress struct array, but can't point at anything yet.
    trainProgressStruct* m_pTrainProgress;

    byte m_trainProgressLocoTableNum;  // Scratch var set to locoNum - 1; i.e. 0..49 rather than 1..50, for internal private use.

    FRAM* m_pStorage;
    Block_Reservation* m_pBlockReservation;
    Route_Reference* m_pRoute;

// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************
// *************************************************************************************************************************************************************************

/*
    // *** The following GETTERS should really not be public as they break encapsulation.  But we're adding them in order to
    // *** expedite OCC's functions (and eventually MAS and LEG's) at least for now, as of 04/07/24.
//    byte nextToTripSensor(const byte t_locoNum);       // Returns the sensor number at element nextToTripPtr.
//    byte nextToClearSensor(const byte t_locoNum);      // Returns the sensor number at element nextToClearPtr.
//    byte tailSensor(const byte t_locoNum);             // Returns the sensor number at Tail, if any, else zero.
//    byte contPtr(const byte t_locoNum);                // MAS only.  Returns element number that holds 4th-from-last sensor in route.
//    byte contSensor(const byte t_locoNum);             // Returns the sensor number
//    byte stationPtr(const byte t_locoNum);             // Returns element number that holds 3rd-from-last sensor in route.
//    byte stationSensor(const byte t_locoNum);          // Returns the sensor number
//    byte crawlPtr(const byte t_locoNum);               // Returns element number that holds 2nd-from-last sensor in route.
//    byte crawlSensor(const byte t_locoNum);            // Returns the sensor number
//    byte stopSensor(const byte t_locoNum);             // Returns the sensor number

//    void updatePointers(const byte t_locoNum);  // Just a wild guess that this will be possible or appropriate as a separate function. *************************************
    // Can be used to update and/all of the 8 pointers in each Train Progress table.  Or maybe we'll just want to update them
    // manually as we process elements...???

    void poke(const byte t_locoNum, const byte t_elementNum, const routeElement t_routeElement);
    // poke() blindly replaces an existing Train Progress route element with whatever we send it.  For instance, when we add a
    // Continuation route, we may need to replace the last two speed commands (VL01, VL00) with speed commands for the new route.

    // Each time Next-To-Clear advances, Tail becomes the "old" Next-To-Clear. So always set Tail=Next-To-Clear, *then* advance
    // Next-To-Clear.

    // *** SETTERS FOR POINTERS ***
//    void setHeadPtr(const byte t_locoNum, const byte headPtr);
//    void setContPtr(const byte t_locoNum, const byte contPtr);  // MAS only.  Called and calculated each time a route extension is added.
//    void setStationPtr(const byte t_locoNum);          // Called and calculated each time a route extension is added.
//    void setCrawlPtr(const byte t_locoNum);            // Called and calculated each time a route extension is added.
//    void setStopPtr(const byte t_locoNum);             // Called and calculated each time a route extension is added.

*/

};

#endif

// ********************************************************************************************************************************
// ***** NOTES FOR FUTURE REFERENCE ***********************************************************************************************
// ********************************************************************************************************************************

// FUTURE TRAIN PROGRESS / ROUTES: Maybe each route should consist of a trigger such as tripping or clearing a sensor, followed by
// actions.  This would make it easier to stop and reverse after clearing a sensor.  However I’ll have to figure out how block and
// turnout reservations would be affected. But may simplify Train Progress…???

// NOTES REGARDING TIME-DELAY RELAYS, SPANNING MULTIPLE SENSORS, COPPER FOIL SENSOR LENGTH, ETC.  FOR ARCHIVE REFERENCE:
// Note that it's possible to have up to 4 sensors tripped at the same time (i.e. 14, 43, 44, and 52 with a long train moving
//   slowly), especially considering the time-delay sensor relays.
// IF THE RELAY TIME DELAY ON TWO CONSECUTIVE SENSORS IS MUCH DIFFERENT, WE COULD SEE TWO SENSORS RELEASE IN THE WRONG ORDER!
// For example, sensors that are very close together such as 14, 43, 44, and 52 -- but really any nearby block exit sensor to
//   the next block's entry sensor.
// THIS IS AN ARGUMENT TO KEEP TIME-DELAY RELAY TIMES VERY PRECISELY IDENTICAL, and delay length is less critical.
// ALSO we must ensure that no copper-foil sensor strip is shorter than the distance between the farthest-apart pair of trucks.
//   The distance between front and rear wheels on an 18" passenger car is 10-3/4", so this should be the absolute minimum
//   length of each copper-foil sensor strip, if the time-delay relays are set to near-zero delay.  However as of Jan 2021,
//   all sensor strips are 10" long, and thus short enough to be briefly spanned by passenger car wheelsets.
// MY BEST GUESS IS TO TRY TO SET EVERY TIME-DELAY RELAY TO AS CLOSE TO EXACTLY TWO SECONDS AS POSSIBLE.
//   Longer is better, due to some goofy sensor strips in blocks 14 and 16.  Two seconds should be more than enough, and
//   the time could be even longer as long as all relays (especially critical ones like 14/43/44/52) are timed the same.

// NOTES REGARDING FRAM ROUTE RECORD NUMBER VERSUS ROUTE ID:
// Since Dispatcher sends each new Route via a single RS-485 message (rather than one route element at a time,) we will pass the
// Route's FRAM Rec Num, which is different than Route ID (which is not indexed) so they can easily look it up.
// FRAM Rec Num = physical record number in FRAM, 0..n, and is sorted by ORIGIN + PRIORITY + DESTINATION.
// FRAM Route ID = Unique route identifier that will cross-ref any record in FRAM back to this spreadsheet.  No particular order.
// FRAM order <> spreadsheet order, because records in FRAM must be sorted by ORIGIN + PRIORITY + DESTINATION before they are
// exported into FRAM.  And spreadsheets are sorted by whatever groupings are convenient for me to work with.

// NOTES REGARDING WHY TRAIN PROGRESS IS STORED ON THE HEAP VERSUS FRAM:
// Train Progress (MAS, OCC, LEG) is on the heap, not in FRAM, because Train Progress does not store any static information between
// operating sessions -- it's always created from scratch.  And Heap storage allows us to access the entire Train Progress
// table-of-tables as array elements, which is faster/easier than FRAM reads/writes.
// Further, entire Train Progress table will be stored on the HEAP as a SINGLE OBJECT -- as a table of tables.  50 "master" Train
// Progress tables, for trains 1..50 (record 0..49), and each maintain it's own Train Progress header and route.
// I don't think it matters if we have 1 object that contains an array for 50 trains, versus 50 objects.  We chose the former.
// So we have a SINGLE TRAIN PROGRESS OBJECT that represents the entire Train Progress table for all 50 locos.  I.e. we do NOT
// have one object per train.  So think of this as analagous to how we set up Route Reference, except this is on the heap not FRAM.

// If we had set this up as an array of 50 Train Progress records, our calling module (i.e. O_LEG) would instantiate thus:
//   pTrainProgress = new Train_Progress[TOTAL_TRAINS];  // NO PARENS for the constructor SINCE NO PARMS!
//   for (int i = 0; i < TOTAL_TRAINS; i++) {  // Train Progress object array elements start at 0 (not 1.)
//   // Use the dot operator, not the arrow ->, because the array makes this a reference not a pointer!
//     pTrainProgress[i].begin(pStorage, pRoute);
//   }
// Details here: https://stackoverflow.com/questions/17716293/arrow-operator-and-dot-operator-class-pointer

// 02/12/23: HOW I DECIDED FOR SURE THAT I'LL HAVE JUST ONE TRAIN PROGRESS OBJECT VERSUS AN ARRAY OF TRAIN PROGRESS OBJECTS 0..49:
// From my research, there doesn't seem to be a compelling advantage of using one method over the other, in general.
// If I have 50 objects, then they'll want to be numbers 0..49, not 1..50.  So I'd have to waste element zero and have a total of
// 51 objects, with element 0 unused -- because I do NOT want to think about a zero-offset for a loco's Train Progress table.
// But if I have just ONE object, I'll address it very much like Routes which have a Route Number that includes header data
// followed by an array.  And I could pass it locoNum and let the class translate locoNum 1 to Train Progress element 0.
// Many calls don't even require a locoNum, but rather return one -- such as getTrainFromSensorTripped().  Which object woudl I
// even pass to the function in that case?  Call it for every locoNum until I got a hit?
// THUS, IT WILL BE BEST TO HAVE JUST ONE TRAIN PROGRESS OBJECT -- it will eliminate having to think about zero-offset parms.

// 02/12/23: DOES NOT APPLY, BUT MY THOUGHTS RE: STATIC VARIABLES, HEAP, and ONE VS MANY OBJECTS:
// If we had decided to set this up as an array of 50 separate Train Progress objects, we would want to think about defining
// STATIC class global pointers for i.e. pStorage (FRAM), so that we only reserved memory for a single pointer and not 50 pointers.
// If we did want to do that, I think it would look like this (in the main program i.e. O_LEG, not in the class):
//   FRAM* Train_Progress::m_pStorage;  // Pointer to FRAM.  STATIC in the class.
// Also, if we were going to have an array of Train Progress tables (objects), we MUST define a constructor for our class that
// took no arguments (this is a rule of C++, per Tim Meighan.)  (We could still init default values for class fields in the
// constructor, and of course pass parms in the begin() function.)  Of course, most of our constructors take no arguments anyway.
