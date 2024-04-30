// DISPATCHER.H Rev: 02/09/21.
// Part of O_MAS.
// Run the layout in Auto or Park mode.

// 03/27/23: DISPATCHER calls a function in Train Progress to know if it can assign a new route or not, for a given train.
//           Will only call the function if the train is not Parked in Park mode, maybe also other factors.
// Function returns const char: ROUTE_TYPE_NULL ('N'), ROUTE_TYPE_EXTENSION ('E'), or ROUTE_TYPE_CONTINUATION ('C')
// ROUTE_TYPE_CONTINUATION after a train tripped the 4th-from-final sensor, but before it trips the 3rd-from-final sensor; *or*
// ROUTE_TYPE_EXTENSION if it is stopped i.e. sitting on the destination (final) exit sensor *and* maybe consider time to wait
//                      before starting again? *else*
// ROUTE_TYPE_NULL.
// ALWAYS RETURNS FALSE FOR ROUTES ENDING AT SINGLE-ENDED SIDINGS, WHERE WE SHOULD *ALWAYS* STOP because it would be weird not to.
// Once a train trips the penultimate sensor (and will be stopping), routes must ensure that any train has had enough time to reach
// the most recent speed command using medium momentum.  That is, we can't still be speeding up or slowing down; we must have a
// known speed when we trip the penultimate sensor (LOW/MEDIUM/HIGH) in order for the deceleration calculations to work properly.

// Dispatcher needs function: Find a route that is:
// NOTE: Checking train length and type are already part of Deadlock class so don't duplicate that code.  Rather, decide how best
//   to organize the filters in class(es)/function(s).
//   Deadlocks: Will this route result in a Deadlock condition?
//   Turnout Reservation: Are all turnouts in the Route unreserved (except possibly for this train?)
//     It's possible to have a Turnout that's still reserved for this train if we're adding a Continuation (not Extension) route.
//   Block Reservation: Are all blocks in the Route unreserved?
//   Block Reservation: If Mode == PARK, is the destination siding Parking == True?
//   Block Reservation: For all blocks in the Route, is this type of train forbidden in any of them?
//     i.e. FORBIDDEN_FREIGHT, FORBIDDEN_THROUGH, FORBIDDEN_NONE?
//   Block Reservation: Does the Destination block station-type restrict this train?
//     I.e. STATION_ANY, STATION_PASSENGER, STATION_ANY_TYPE, STATION_FREIGHT
//   Block Reservation: Is the Destination block length > this train length?
// FUTURE: Introduce some randomness so we don't always choose the highest-priority Route that will work, if there is also a
//   lower-priority route that could work.

// Must support transition from AUTO+RUNNING to PARK+RUNNING without stopping.

// TO DO: Dispatcher needs functions: "Reserve all turnouts in this route" and "Reserve all blocks in this route".
//   Should be part of function "Assign this route to this train."

// TO DO: Dispatcher needs functions: Release turnout and block reservations as appropriate, each time a sensor is tripped or
// cleared in Auto/Park modes.

// TO DO: Dispatcher should have a check when train trips destination sensor, confirm Train Progress is empty except for the final
// block.  Otherwise this would be a bug.

// TO DO: Dispatcher should have a check when last train stopped or parked, confirm all turnouts not reserved, and all blocks not
// reserved except for the blocks currently occupied by stopped trains.  Anything else would be a bug.

// TO DO: See Train Progress for functions that enable Dispatcher and other modules, as needed, to know where loco is in its route.

// LENGTH RESTRICTIONS: ALL TRAINS MUST BE SHORT ENOUGH TO FIT IN THE SHORTEST DOUBLE-ENDED SIDING (single-ended sidings excluded.)
// My shortest double-ended sidings, Blocks 2 and 3, can hold a train at most about 117" long.
// Two F-units (15" each = 30") plus 5 18" passenger cars (90") are actually 122", so too long.
// It's not practical to implement length restrictions on routes, because we might route to a siding that is long enough for a
// train, but none of the "next" sidings are long enough. For instance, Block 14 is 145", and B5/B6 are 154"/144", but then we
// *must* enter B2 or B3, which are only 125"/126"!
// We can't "fake" a siding length by saying it's shorter than it actually is, because that would screw up our deceleration
// time/distance calculations.
// We could add "new" routes that extend beyond the shorter sidings, but that's a huge can of worms.
// FOR NOW, WE MUST NOT HAVE ANY TRAIN THAT IS LONGER THAN OUR SHORTEST THROUGH SIDING (excludes single-ended sidings.)
// Note that I can probably lengthen some of the sidings by simply shortening the amount of copper foil at each end; most likely
// the 12" or so that we are currently allowing is much more than we will need.  Hopefully 4" of copper foil should be plenty.

// TO DO: Each time a Route is assigned, write to a log file in FRAM: locoNum, RouteID.  So we can review if something weird
// happens, such as a bug in a Route record.

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>         // Display_2004*        pLCD2004
#include <FRAM.h>                 // FRAM*                pStorage
#include <Message.h>              // Message*             pMessage;
#include <Loco_Reference.h>       // Loco_Reference*      pLoco;
#include <Block_Reservation.h>    // Block_Reservation*   pBlockReservation;
#include <Turnout_Reservation.h>  // Turnout_Reservation* pTurnoutReservation;
#include <Sensor_Block.h>         // Sensor_Block*        pSensorBlock;
#include <Route_Reference.h>      // Route_Reference*     pRoute;
#include <Deadlock.h>             // Deadlock_Reference*  pDeadlock;
#include <Train_Progress.h>       // Train_Progress*      pTrainProgress;
#include <Mode_Dial.h>            // Mode_Dial*           pModeSelector;

class Dispatcher {

  public:

    Dispatcher();  // Constructor must be called above setup() so the object will be global to the module.
    void begin(Message* t_pMessage, Loco_Reference* t_pLoco, Block_Reservation* t_pBlockReservation,
               Turnout_Reservation* t_pTurnoutReservation, Sensor_Block* t_pSensorBlock, Route_Reference* t_pRoute,
               Deadlock_Reference* t_pDeadlock, Train_Progress* t_pTrainProgress, Mode_Dial* t_pModeSelector);
    void dispatch(const byte t_mode, byte* t_state);  // Run the layout in Auto or Park mode

    // See if candidate destination is appropriate for this loco based on type (pass/frt) and length.
    bool trainPermittedInSiding(const byte t_locoNum, const routeElement t_blockNum);
    // NOTE: Dispatcher will call trainPermittedInSiding to see if a candidate destination is even an option, but then must call
    // Deadlock_Reference::deadlockExists(destination, locoNum);

  private:

    routeElement m_routeElement;

    Message* m_pMessage;
    Loco_Reference* m_pLoco;
    Block_Reservation* m_pBlockReservation;
    Turnout_Reservation* m_pTurnoutReservation;
    Sensor_Block* m_pSensorBlock;
    Route_Reference* m_pRoute;
    Deadlock_Reference* m_pDeadlock;
    Train_Progress* m_pTrainProgress;
    Mode_Dial* m_pModeSelector;

};

#endif
