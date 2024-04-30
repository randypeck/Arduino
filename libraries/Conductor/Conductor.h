// CONDUCTOR.H Rev: 03/03/23.b
// Part of O_LEG.
// LEG Conductor receives routes from MAS Dispatcher and uses that data to populate LEG's Train Progress and Delayed Action tables.
// Conductor does NOT know the details of how to populate Delayed Action; it sends high-level requests to Delayed Action to do so.
// Conductor does NOT know how to track Train Progress; it sends high-level requests to Train Progress to provide that information.
// Conductor will need to add HORN/WHISTLE to Delayed Action table when Train Progress records show start, stop, reverse etc.

// CONDUCTOR'S PRIMARY DUTIES ARE:
// * MANUAL MODE:
//   * Watch for and execute PowerMaster startup requests, using Delayed Action/Engineer.
// * REGISTRATION MODE:
//   * Call Train_Progress::initialize()
//   * Call Delayed_Action::initialize()
//   * Call Engineer::legacyCommandBufInit()
//   * Call Engineer::accessoryRelayInit()
//   * Receive registration start-up info including Smoke on/off, Startup fast/slow.
//   * Receive data for each new "real" train, as it is registered: locoNum, blockNum, sensors.
//     * Create new initial "route" in the Train Progress table.
//     * Populate the Delayed Action table with this info plus commands to start the train.
//     * Pass control to Engineer to scan/execute/de-populate Delayed Action commands i.e. start the train.
// * AUTO AND PARK MODE:
//   * Receives Warrants from Dispatcher: locoNum, time, and Route.
//       Sensors, Speeds, Direction changes.  Don't care about turnouts but we need blocks to lookup length, default speed for
//       destination blocks that become Continuation routes.
//   * Populates Train Progress with Warrant (like those of MAS and OCC, except ignores turnouts)

// QUESTION: And at the beginning of an Auto-mode session, and also after we've stopped but are later sent an Extension route -- 
// the new route elements get added to Train Progress, but then how does the Conductor know to (re)start executing elements???
// ANSWER: For New and Extension routes (and obviously not Continuation routes,) MAS Dispatcher always sends a "ms delay until
// start" parm, so we can just fill that (plus current time) into the train's timeToStart field and Conductor can start populating
// at that time (and make pre-departure announcements before that.)

// Class functions such as Train Progress Initialize, Delayed Action Initialize, Legacy Command Buf Initialize and Accessory Relay
// Initialize are public because neither the Delayed Action class nor the Engineer class know anything about when Registration mode
// begins, and yet their tables need to be (re)initialized each time that happens.  It's a bit awkward that LEG Conductor would
// need to care about the private-to-Engineer Legacy Command Buf especially, but where else can we ensure that this gets
// initialized each time Registration begins?  There is some consolation in that at least many such functions need to be called
// each time Registration (re)starts, and they can all be called at the same time.

// WHEN SLOWING TO CRAWL here's how to calc if a loco reaches Crawl TOO SOON rather than too late as this function helps with:
//   The calling program (Conductor) can easily calculate the time the loco should reach Crawl speed using the time it tells
//   Delayed Action to begin slowing via populateSpeedChange() + speedChangeTime(), it can compare that time to the time that
//   the Stop sensor is tripped.  If the sensor is tripped BEFORE we calculate reaching Crawl speed, we've overshot; and if the
//   Stop sensor is tripped AFTER we calculate reaching Crawl speed, we've undershot.  Either way, we have valuable info.
//   Given the above, the t_msDelayError value returned is probably not even useful when slowing to Crawl speed.
// Unfortunately, Conductor may no longer have Route number (such as if a Continuation route has been assigned) so it may be
//   hard to debug these problems.  Anyway, Conductor, wipeLocoSpeedCommands, etc. should dump whatever data they can to the mini
//   thermal printer.  Maybe as much of the Route as still exists in Train Progress so we can manually try to figure out where it
//   came from in Route Reference, but only if the time diff is more than 2 or 3 seconds I'd think; else we can probably ignore.

// Train_Progress->currentSpeed NOTES AS USED BY LEG CONDUCTOR:
//   1. When creating a series of speed-change records, we will always use this value as our starting speed.  If we are populating
//      a Slow-To-Crawl series and currentSpeed is not exactly Low, Medium, or High for this loco, we won't be able to accurately
//      predict the time & distance we'll need to reach Crawl (since we won't have a deceleration entry for this starting speed,)
//      and we should report this via the mini thermal printer.  Really only a problem for Slow-To-Crawl, but it should still
//      never differ from our target speed per the Train Progress table.  And this should only happen if there is a problem with
//      our Route record, because it should always allow sufficient time for a loco to reach a target speed at the loco's
//      acceleration rate stored for the target speed, before another speed command is encountered.
//   2. If we trip the Stop sensor AFTER we've already reached Crawl, we'll compare the time that Crawl speed was reached vs the
//      time the Stop sensor was tripped, and output locoNum and other data, plus the number of ms spent at Crawl speed.  We can
//      divide Crawl time with Crawl rate to get the approx DISTANCE that we stopped too soon, and use that to update the stopping
//      distance from the incoming speed to Crawl (if the time/distance is great enough to worry about.)  But we'll also need to
//      know which of the three incoming speed locoRef data we used (Low/Med/High) in order to know which numbers to update.
//      Thus, we also track the "startingSpeedForCrawl" and "expectedTimeAtCrawl" fields ;-)
//   3. If we trip the Stop sensor BEFORE we've reached Crawl (i.e. sooner than planned) we'll use the current speed and time
//      compared to the remaining speed commands and times in Delayed Action, and output locoNum and relevant speeds/times for
//      analysis and adjustment of locoRef data.  The startingSpeedForCrawl and exectedTimeAtCrawl fields will also be useful.

// Functions Conductor can call in Delayed Action class include:
// * powerMasterOn()  // This is simply "Absoute Speed 1" for the appropriate "engine" number.
// * emergencyStop()  // All devices
// * void startUpSlow(byte locoNum); startUpFast("); shutDownSlow("); shutDownFast(");
// * void setVolume(n)  // This is accomplished by repeating Master Vol Up/Down i.e. for tunnels.  Fx 0x4.
// * void setSmoke(n)  // off, low, medium, or high.  Probably with an enum.
// * void setMomentum(n)  // Always set to zero.
// * setDirection(byte locoNum, char Forward or Reverse)
// * setSpeed(byte locoNum, char or enum speedCSMH, byte momentum)
// * stopImmediate(byte locoNum)  // Prefer over Abs Speed 0 as this overrides momentum (though momentum should already be zero.)
// * bellOn(), bellOff()  // Perhaps some bells should be controlled by Delayed Action rather than Conductor?
// * hornPattern getPattern(use hard-coded logic to retrieve horn pattern required when/where loco stops, starts, reverses,
// * void soundHorn( HornPattern some_pattern ) 
//   * Can monkey with "quilling horn intensity" for some of these patterns.
//   * enum HornPattern { S, SS, LLL, LLSL, LS } hornPattern;  // This also creates a variable of type HornPattern, called hornPattern (optional)
//   * hornPattern = LLSL;
//   * Here are the possible patterns:
//     S = When train has just come to a stop "air brakes applied and pressure is equalized"
//     SS = When train is going to proceed (forward) "train releases brakes and proceeds" (before moving)
//     LLL = When train is stopped, going to back up (before moving)
//     LLSL = Train is approaching public crossing at grade with engine in front (hold last Long until loco clear of crossing i.e. trips next sensor)
//     LS = Train is approaching men or equipment on or near the track.
// HORN/WHISTLE commands should be repeated every 100ms, in order to maintain a sustained blow, per Rudy.  However, my own tests
// show that every 250ms might be enough.  Every 330ms causes a stutter.  It's probably worth looking at the LCS Wi-Fi module to
// confirm this, although whatever works as a reliable minimum is what I should use.  Define it in a global CONST.
// TMCC commands are shorter, and may be enough to use instead of the longer Legacy horn/whistle commands.

// *** NOTE REGARDING SENSOR ACTION AND CROSSING GATES ETC. ***
// Crossing gate accessory-activation commands need to be populated in Delayed Action "on the fly",
// only after we're certain if we will continue through that block versus stopping or going to some other block next.
// I don't think we need to worry about Continuation routes messing up the crossing-gate logic, because we'll just say i.e. "if we
// trip sensor SN08 *and* we are in BE04 *and* currentSpeed > 0: lower the gate (and then raise after i.e. 20 seconds.)
// Or we could look at the various route pointers versus the loco's current position.
// Of course, we must read Train Progress, update Delayed Action with the VL00 command, and update the Train Progress header file
// with isStopped = true; and *then* do our analysis of Sensor Action.  Will this work for other scenarios as well?
// Maybe LEG Conductor needs a Sensor-Action function to look at the Sensor Action table and returns a value such as "LOWER
//    CROSSING GATE" if certain conditions are met.  I.e. SPECIAL LOGIC TO LOWER THE CROSSING GATE:
//    If we trip Sensor 7 and the next sensor is Sensor 8 and not-stopped and the following sensor is Sensor 32, lower the gate.
//    Or do a simplified version: If we trip Sensor 8 and the next sensor is Sensor 32, lower the gate.
//    If we clear Sensor 32 and the Tail sensor is Sensor 8, raise the gate.
//    Or do a simplfied version: If we clear Sensor 32 and the next sensor is Sensor 31, raise the gate.
//    And so on for the other direction.
//    We must also consider this when starting from a stop, when sitting on Sensor 8 (i.e. Block 4 east.)

// 1/10/21: Conductor in Auto/Park modes needs some tricky logic to deal with special circumstances, including:
//   * See Route 51: At the end of the route, the train shifts into forward and proceeds at speed 1 (CRAWL) until it trips the exit
//     sensor.  This is boring but necessary when we start and then stop in the same siding.  But if Dispatcher adds a Continuation
//     route, it would be much better to start accelerating right away.  How do we fix this?

// As the train progresses, LEG will receive messages Sensor Tripped and Sensor Cleared.  Conductor will need to quickly determine
// what actions must be taken in those cases -- i.e. if we trip Sensor 32 and the next sensor in Train Progress is Sensor 8, or
// vice versa, we should lower the crossing gate.  And then figure out how to raise it.  And we'll need to account for the fact
// that we *may* be stopped, and the Route gets added that will move us through the crossing.  Similarly, if we trip Sensor 7 *and*
// it is an Exit Sensor, then lower the crossing gate (even if it may not have been up, but what the heck.)
// Similar logic must be hard coded (including generic sensor based on train behavior) to train horn patters etc.
// Here are some things that Delayed Action can add to the Delayed Action table:
//   * If starting new route (not stopped to reverse), starting 30 seconds before moving, make an All Aboard announcement.
//   * If starting new route (not stopped to reverse), starting 10 seconds before moving, turn on bell.
//   * If starting new route (not stopped to reverse), starting 2 seconds before moving, toot the horn in the appropriate pattern (depending on forward or reverse.)
//   * Turn off the bell at some point.
//   * When coming to a stop and not reversing (end of route), turn on the bell.
//   * When we come to a final stop, turn off the bell.
//   * When we come to a final stop, toot the horn/whistle.
// All of the above -- speeding up, slowing down, bell, whistle, announcements -- all need to be hard-coded, at least until we add
// a Sensor Action table.

// 02/12/23: As Engineer executes ripe speed-change cmds ABS_SPEED, STOP_IMMED, EMERG_STOP via Engineer::executeConductorCommand(),
// (more deeply, from Engineer::getDelayedActionCommand() and Engineer::sendCommandToTrain(),) getDelayedActionCommand() also keeps
// the loco's Train Progress "current speed & time" up to date (LEG only.) This is useful when we make future speed adjustments so
// we begin the speed change at (nearly) the loco's current speed, even if it is not moving at the expected speed, and it's also
// usful to help determine when we under-shoot or over-shoot a destination siding's Stop sensor, or have not yet reached a target
// speed before triggering a new speed-change Route element, so that we can make adjustments to the loco's deceleration parms if
// necessary.
// WHEN WE WANT TO CHANGE LOCO SPEED, OTHER THAN WHEN SLOWING TO A CRAWL:
// - We can look up the current loco speed (and time that speed was set) and compare it to the speed that we should be moving,
//   which would be the previous VLxx Velocity command in Train Progress.
//   * If the current speed matches expected speed, all is well and we can send a new speed-change command.
//   * If the current speed does NOT match expected speed, we should compare the two (and also time difference) and determine if
//     there is enough of an error to report via the 2004 LCD and also via the mini thermal printer.
//   * Normally we will not have a problem if the speed is off, because we'll likely be able to reach the *next* target speed
//     before tripping the next speed-change sensor.  BUT this still shouldn't happen, because it implies that we have not allowed
//     enough distance in this part of the route to adjust our speed.  THE ROUTE REFERENCE TABLE SHOULD BE ADJUSTED TO FIX THIS.
// WHEN WE WANT TO CHANGE LOCO SPEED VIA SLOW TO CRAWL:
// - We can look up the current loco speed (and time) and compare it to the speed that we should be moving.  If different, this is
//   a more serious problem than a simple speed adjustment, because we are hoping to calculate an accurate set of slow-down parms
//   based on Loco Ref deceleration data -- WHICH ONLY WORKS IF OUR INCOMING SPEED IS EXACTLY THE LOCO'S LOW, MED, or HIGH SPEED.
//   So we'll do our best to slow the loco using our max decleration parms and maybe have a long Crawl, or maybe have to slam on
//   the brakes (so to speak) if we're still moving faster than Crawl when we trip the Stop sensor.  EITHER WAY, THIS IS AN ERROR
//   IN THE ROUTE TABLE THAT MUST BE FIXED.  We'll print as much debug data as we can on the mini thermal printer.
//   It won't be necessary to halt the program as we can still stop near the Stop sensor even if we trip it moving fairly fast.
// WHEN WE WANT TO CHANGE LOCO SPEED from (hopefully) Crawl VIA STOP-IN-3-SECONDS.  I.e. whenever we trip a Stop sensor:
// - We can look up the current loco speed (and time) and compare it to the speed that we should be moving, i.e. Crawl.
//   * If we are moving faster than Crawl, then our speed-change sequence has overshot the Stop sensor and we'll want to provide
//     debug info as to how far we've overshot in terms of the Legacy speed difference and the time difference (because we of
//     course know when we tripped the Stop sensor, and we can calulate what time we should have tripped it.)  We should expect
//     to overshoot the Stop sensor by a small amount fairly often, if our calcs are set to arrive at Crawl speed at the same
//     moment that we trip the Stop sensor.  So if the difference between actual vs expected speed at that point is small, we can
//     make note via the mini thermal printer, but really not a problem -- maybe make a slight adjustment to Loco Ref deceleration
//     parameters.  But if the speed and/or time difference is more substantial, then our Loco Ref deceleration data definitely
//     needs to be re-figured.  Perhaps due to the inexplicable fluke of a loco's mm/Sec at various speeds sometimes being
//     unpredictable -- we may have to go with the most conservative value (i.e. fastest expected mm/Sec at incoming speed.)
//   * If we are moving at Crawl already, that's great, but we should look at the time difference between the moment we reached
//     Crawl (which will be the loco's current speed/time value) and the current time (i.e. when we tripped the Stop sensor.)
//     If the time spent traveling at Crawl is short, say less than a few seconds, then that's what we would expect as normal.
//     But if we spent more than a few seconds at Crawl, we should report that data on the mini thermal printer and consider making
//     an adjustment to that loco's deceleration parms for whatever that incoming speed was.
//  THE ABOVE ERROR CALCULATIONS ASSUME THAT OUR INCOMING SPEED (i.e. speed at which we tripped the Crawl sensor) WAS EXACTLY OUR
//  EXPECTED SPEED AT THAT MOMENT.  If we were not moving at exactly the expected incoming speed, there is no point in considering
//  our speed and time when we trip the Stop sensor.

#ifndef CONDUCTOR_H
#define CONDUCTOR_H

//#define SENSOR_BLOCK         // MAS, OCC, and LEG
//#define BLOCK_RESERVATION    // MAS, OCC, and LEG
//#define LOCO_REFERENCE       // MAS, OCC, and LEG
//#define ROUTE_REFERENCE      // MAS, OCC, and LEG
//#define TRAIN_PROGRESS       // MAS, OCC, and LEG
//#define DELAYED_ACTION       // LEG only!
//#define ENGINEER             // LEG only!
//#define CONDUCTOR            // LEG only!

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <FRAM.h>
#include <Sensor_Block.h>
#include <Block_Reservation.h>  // Conductor needs access to block default speeds, block length for stopping calcs, maybe more?
#include <Loco_Reference.h>     // Must be set up before Deadlock as Deadlock requires Loco pointer.
#include <Route_Reference.h>
#include <Train_Progress.h>
#include <Delayed_Action.h>     // So at Registration we can call Delayed_Action::initialize()
#include <Engineer.h>           // So at Registration we can call Engineer::legacyCommandBufInit() and accessoryRelayInit()  IF CONDUCTOR DOES THIS, VERSUS DOING IT IN MAIN LEG LOOP *****************************************

// Conductor or whoever calls Conductor will need to call Delayed_Action::initialize() to clear the whole Delayed Action table
// whenever Registration mode (re)starts.

class Conductor {

  public:

    Conductor();  // Constructor must be called above setup() so the object will be global to the module.

    void begin(FRAM* t_pStorage, Block_Reservation* t_pBlockReservation, Delayed_Action* t_pDelayedAction, Engineer* t_pEngineer);

    void conduct(const byte t_mode, byte* t_state);  // Manage Train Progress and Delayed Action tables

  private:

//    routeElement m_routeElement;  not needed yet as of 2/17/23 since I haven't written any real code yet

    FRAM*              m_pStorage;           // Pointer to the FRAM memory module
    Block_Reservation* m_pBlockReservation;
    Delayed_Action*    m_pDelayedAction;     // So at Reg'n we can call Delayed_Action::initialize()
    Engineer*          m_pEngineer;          // So at Reg'n we can call Engineer::legacyCommandBufInit() and accessoryRelayInit()

};

#endif
