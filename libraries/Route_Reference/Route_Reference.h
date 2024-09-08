// ROUTE_REFERENCE.H Rev: 09/08/24.  TESTED AND WORKING.
// Route Reference table is stored in FRAM, and is used by MAS, OCC, and LEG to maintain their Train Progress tables.
// All three modules must have identical matching Route tables in FRAM, as MAS sends FRAM record number to identify routes.

// 09/08/24: Deprecated Route Rule 9; we will now allow turnouts to occur  multiple times in a route without any special
// considerations; let's hope MAS can throw them the instant the sensor ahead of them is tripped.

// 03/01/23: Eliminated levels field.
// 01/24/23: Did some tests and determined that as long as an object is instantiated in .ino via a global pointer and using "new"
// in setup(), all of the class data will be placed on the heap -- regardless of if the local class data, such as the struct that
// holds one record, is set up using a pointer (to heap) or just set up as a regular variable.  Thus we're going to define all
// local object variables inside of classes as stack variables, not heap variables.  So we'll be using the "dot" operator rather
// than the "arrow" operator in our class functions.
// 01/16/23: We no longer care about levels in our logic, although we will still track it as a field for each route.
// 12/22/22: WB routes are stored in FRAM immediately following EB routes; i.e. all routes are contiguous.  So now the starting
// address for WB routes = FRAM_ADDR_ROUTE_REF + (FRAM_RECS_ROUTE_EAST * sizeof(routeReferenceStruct))
// 12/20/22: Using actual FRAM Rec Num starting at zero.  Route ID is only used for reporting for speadsheet cross reference.
// FRAM Rec Num is the physical record of this route in FRAM, ranging from 0..(FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST - 1).
// FRAM Rec Num is used to quickly and easily pass routes to other modules for quick lookup, versus transmitting them as a sequence
// of route records.
// Spreadsheet records will be sorted by Origin + Priority + Destination fields prior to being imported into FRAM.
// Route ID is only used when reporting, so we can cross reference back to the spreadsheet row -- since we won't know the FRAM Rec
// Num in the spreadsheet (it's assigned after records have been sorted by Origin + Priority + Dest.)
// 11/30/22: Changed char Auto/Park to simply bool Park = true|false.  The only sidings I can think of that would not be Park would
// be BX04 (since it's on a slope) and BX13 (since it's in a tunnel.)
// 12/05/20: Changed populate to use the heap.

// ********** IMPORTANT TESTING *************
// 01/17/23: Will want to write special program to prompt operator for LocoNum and a Route ID, that will run the train through the
// route via Train Progress and Delayed Action.  Need a special sequential search function here to retrieve a route via Route ID.
// 12/22/22: TEST "getFirstMatchingOrigin" eventual speed by simply reading every record 0..51, doing a check for Origin and maybe
// one or two other parms, then reporting the total time to do all those reads and compares.  We figure that with maximum of just
// over 150 routes per EB/WB direction, we will need to scan AT MOST 150 routes to find an appropriate route for i.e. BE26 to BE20,
// or BW26 to BW20 (the last records in each direction.)
// So this will give us a sense of how long MAS will take to find a new/continuation/extension route each time it needs to.

// 12/23/22: FUTURE: We may want to add routes that include elements to execute upon CLEARING a sensor.  For example, when
// shuffling from BW03 to BW02 via BW09 it's better to stop and reverse upon Clearing SN35, rather than climbing the hill and
// waiting until we trip SN36. If we add this feature, we'll also need to figure out if there will be a problem with lack of "slow
// to crawl" upon tripping penultimate sensor - but in this case we're shuffling so our speed would be slow anyway, and we can just
// call "Stop From Slow" (takes 3 secs) or add a new "Stop from Slow" that takes an arbitrary speed and slows down over 4 secs
// rather than 2.
// So perhaps we can add an "SC" Route record type i.e. Sensor Clear.  We would still always only clear "next to clear", but we
// could follow SC records with i.e. VL00 + RD00 + VL02.  Have to think about how this would affect Next-To-Trip.
// The above would not add new capabilities; it would just speed up some otherwise-very-slow shuffling maneuvers.
// This could create a problem in short blocks such as Block 7 where, if we act upon Clearing SN31 rather than Tripping SN32, we
// wouldn't know for sure if we'll trip SN32 or not, due to variable train length.  If it's a long train, we may trip SN32 before
// clearing SN31, but a short train would clear SN31 without ever tripping SN32.  So our "next to trip" logic would be kaput.  Thus
// we'll need to incorporate a rule that any block used for SC Sensor Clear actions *must* be longer than the longest train, so we
// can be guaranteed that we will *not* trip a sensor ahead of us (i.e. SN32) before clearing a sensor behind us (i.e. SN31.)

// CLARIFICATION OF "recNum" vs "routeID" FIELDS rev: 01/16/23:
// recNum is the actual FRAM record number in the Route Reference table, BEGINNING AT RECORD 0 (not 1 as previously.)
//        For single-level operation there are 74 routes: 38 EB and 36 WB.
//        For full two-level operation there are over 300 routes.
//        Records are sorted in FRAM by ORIGIN + PRIORITY + DESTINATION.
//        Rather than define a fixed FRAM address where the WB route records start (previously used FRAM_FIRST_WEST_ROUTE,) we'll
//        just calculate it in the Route_Reference class, using:
//          FRAM_ADDR_ROUTE_REF + (FRAM_RECS_ROUTE_EAST + sizeof(routeReferenceStruct)).
//        Thus, we will still be using FRAM_ADDR_ROUTE_REF, FRAM_RECS_ROUTE_EAST, and FRAM_RECS_ROUTE_WEST.
// routeID is a unique ID for each route 1..n, sorted by how we like to view them in the "Routes" spreadsheet tab.
//        Route ID is simply used as a cross-reference back to the original spreadsheet, for reporting and debugging.
//        Route ID CANNOT be used as a lookup into the FRAM table since it's not sorted that way (unless by sequential search.)

// ROUTE RULES rev: 09/08/24:
// 1. All routes in Route Reference must begin with Block + Sensor + Direction + Velocity.
//    Train Progress new routes get the SN00+VL01 inserted at the beginning so that Tail has a sensor to point to, and VL01 just
//    to keep it looking like the end of a normal route.
// 2. All routes must end with the train moving Forward.
//    We require Routes to end in Forward, because we always want the FRONT of the train to be sitting on a sensor.  If we
//    allowed a route to end with a train reversing into Block 23-26, how could we assign a new route when it's not on a sensor?
// 3. All routes must have a final FOUR records of VL01 + Block + Sensor + VL00.
//    Exception is initial Registration "route" set up in Train Progress uses VL00 (not VL01) + Block + Sensor + VL00 since it
//    will be stopped and waiting.  Doesn't affect anything.
//    Train Progress EXTENSION and CONTINUATION routes overlay starting at the Block record found three elements before the end of
//    the route.
//    The VL01, always four elements before the end of the route, is only overwritten in cases where we have a CONTINUATION route
//    *and* where the new route does not begin in Reverse.
//    We must not have a turnout element following the final VL01 record; these need to be moved to *before* the VL01 such as with
//    Destination BX04 and BX14.
//    Route 165, which reverses into a single-ended siding but must somehow set the mainline turnout to Normal after it's fully
//    entered.  This demonstrates that we CAN have a rule that all routes must end with VL01 + Block + Sensor + VL00.
//    Here is the end of Route 165 (which reverses into single-ended siding): SN24 VL01 BW23 SN23 VL00 FD00 TN23 VL01 BE23 SN24 VL00
//    This makes it easy to find the VL01 when we want to overwrite it with a potentially faster speed, when appending a
//    CONTINUATION (not-stopping) route.
// 4. Ahead of any SNxx + VL00 stop (mid-route or end-of-route) there must be a SNxx + VL01 and a Block record somewhere between
//    those two Sensor records.
//    The VL01 indicates we want to target Crawl speed before tripping the Stop Sensor.
//    The Block record is needed so we can look up the distance available to slow from incoming speed to Crawl speed, ideally at
//    the moment we trip the Stop Sensor.
//    It's okay to have other records (such as Turnout commands) intespersed, so long as they don't violate any other rule
//    (i.e. can't have a Turnout record at the end of a route.)
//    The reason we allow other elements before mid-route stop, and require exactly four elements at a Destination stop, is because
//    the rule for Destinations makes it much easier to append routes; no need to do this with mid-route stops.
// 5. All speed and direction commands should immediately follow an SN sensor record; there should never be any after the first
//    Block or Turnout command (non-VL and non-direction) is encountered, until after the next sensor record.
//    Exception: There will be cases where a VL command follows a Direction command which follows a VL command, such as:
//    SN03 VL00 FD00 VL01 BE02
// 6. The distance of any block preceeding a Stop sensor (VL00) must be long enough that the train can reach Crawl speed at a
//    reasonable momentum based on it's incoming speed.
// 7. If a train reverses direction (from either direction,) it must be sitting on only one sensor.
//    I.e. all previous sensors must clear so that only the "stop here" sensor remains tripped before a train can proceed with a
//    route.
//    It's possible that a short siding and/or long train might take a moment for the previous sensor to clear, so logic must allow
//    for this rather than starting back up instantly after a stop of any kind.
// 8. Speeds in Route Reference must be such that we are guaranteed to be moving at the previous target speed *before* tripping the
//    penultimate sensor of any "stopping" block, when changing speed using Medium momentum.
//    This applies to a mid-route stop-to-reverse-direction as well as the end-of-route final stop.
//    If a loco is not traveling at a known speed when it trips a penultimate-before-stop sensor, our calculations to slow the loco
//    to Crawl speed will be incorrect.
//    So, for example, never have a route with a speed command upon entering block 16W, since that block is too short to make any
//    speed change using Medium momentum.
//    This will require some fine tuning on how we decide acceleration and deceleration rates other than when slowing to Crawl
//    (and including when backing into sidings and then pulling forward.)
// 9. This rule isn't going to work, as documented in MAS Auto/Park mode comments.  Now being written so allow Turnout elements to
//    recur anywhere, as long as MAS can throw quickly.
//    DEPRECATED RULE: When stopping then reversing then stopping again within a single block (i.e. back into single-ended siding,
//    then forward until we trip the exit sensor) speed must be VL01 (Crawl.)
//    This could get boring when this happens in a long siding and/or a short train, because the train will be crawling the length
//    of the siding, less the length of the train itself.
//    FUTURE: We can add logic to make the train speed up then slow back down to crawl within the limited space, but we'll need to
//    look up length of the train *and* length of the block to know how much track is available.
//    Actually, it would be cool to calculate the distance required to accelerate from VL00 to VL02 (Low) and then decelerate back
//    to VL01 (Crawl) using the Low-to-Crawl deceleration parameters.
//    I think the time and distance from Crawl-to-Low would be equal to the time and distance from Low-to-Crawl, but we'd need to
//    test.  And figure out additional time and distance req'd from Stop to Crawl.  We could always add a parm to Loco Ref to
//    store distance to accelerate from Stop to Crawl using its own set of parameters (step, delay.)  We'd need to calculate the
//    time (trivial) to know when to start the acceleration from Crawl to Low, and deceleration from Low to Crawl.
// 10. Due to the way turnouts are thrown on routes, a given turnout MUST NOT occur twice in a single route UNLESS there is a FD/RD
//     (direction) between the two occurrences.  So we couldn't have a route that used Turnout 4 to go from BE10 to BW10 via Block
//     7 unless it included a mid-route stop (which it could) (or technically just an inserted FD or RD even without stopping.)
//     This is because in Auto/Park mode, each time an FD/RD occurs ahead of a tripped sensor, we will throw all turnouts of a
//     route between that FD/RD and the next FD/RD along the route (or end of route) (even if beyond the next sensor.)
//     FUTURE: We could allow turnouts to appear twice without requiring intervening FD/RD if we have very low propogation delay.
//     I.e. each time we tripped a sensor, we could throw all turnouts through the next sensor.  But often a turnout will
//     immediately follow a sensor, so it better be quick.
// 11. FUTURE: If a Route includes a SC "Sensor Clear" record type, we must be guaranteed that the longest train will never trip
//     the next sensor in that direction.  Still working out how Next-To-Trip logic will be impacted.
//     If backing EB over SN31, I suppose we'd have NTT SN31, VL01, NTC SN31, VL00, FD00, VL02, NTT SN31...
// 12. Every Route must have at least five SN## Sensor records, else  Train Progress will crash because we need new sensor records
//     for CONT, STATION, CRAWL, and STOP.

#ifndef ROUTE_REFERENCE_H
#define ROUTE_REFERENCE_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <FRAM.h>

class Route_Reference {

  public:

    Route_Reference();  // Constructor
    void begin(FRAM* t_pStorage);  // Must be called right after Constructor creates instance.

    // Calling module keeps track of recNum since records are ordered that way, thus we can keep getting "next" physical record
    // until we no longer have a match for origin.  Route ID is just a cross reference back to the original spreadsheet.

    unsigned int getRecNum(const unsigned int t_recNum);   // Returns the FRAM Rec Num 0..n of the rec currently in memory.
    unsigned int getRouteID(const unsigned int t_recNum);  // Returns spreadsheet Route ID (cross ref) for a given record.

    unsigned int getFirstMatchingOrigin(const routeElement t_origin);
    // Returns FRAM Rec Num 0..n of the first rec that matches t_origin i.e. BW15.  Does NOT return the (private) struct.

    unsigned int getNextMatchingOrigin(const routeElement t_origin, const unsigned int t_recNum);
    // Returns FRAM Rec Num 1..n of the next match to t_origin, given FRAM t_recNum 0..n.
    // Returns 0 if no more matches.
    // This works even for t_recNum == 0 because rec 0 can never be the "next" matching rec, so a correct match will always be > 0.

    unsigned int searchRouteID(const unsigned int t_routeID);  // Returns FRAM Rec Num given a Route ID.  Brute force.

    routeElement getOrigin(const unsigned int t_recNum);    // Returns origin i.e. BW03 for a given Rec Num.
    routeElement getDest(const unsigned int t_recNum);      // Returns destination i.e. BE20 for a given Rec Num.
    bool         getPark(const unsigned int t_recNum);      // True if this rec's Destination can be used for parking
    byte         getPriority(const unsigned int t_recNum);  // 1..5; 1 = Highest priority
    routeElement getElement(const unsigned int t_recNum, const byte t_elementNum);  // Get 1 route element at a time, 0..79.

    void display(const unsigned int t_recNum);  // Display a single record to Serial COM.
    void populate();  // Special utility reads hard-coded data, writes records to FRAM.

  private:

    void          getRouteReference(const unsigned int t_recNum);      // Populate pRouteReference data for FRAM t_recNum.
    unsigned long routeReferenceAddress(const unsigned int t_recNum);  // Return the FRAM recNum address in Route Reference.

    // ROUTE REFERENCE STRUCT.  This struct is known only within this class.
    // This struct is 170 bytes long as of 03/01/23 (removed "level" field.)  Less than the max 255-byte FRAM buffer.
    // We use the struct itself as the buffer and it works fine with FRAM that way.
    // RouteElement R01..R80 [0..79] 2 bytes each = 160 bytes total, plus 10 bytes of header = 170 bytes/route.
    // For 1-level operation as of 12/21/22: There are 30 routes with Eastbound origins, and 22 routes with Westbound origins.
    // For 2-level operation as of 12/21/22: There will be 158 routes Eastbound, and 152 routes Westbound.
    // Routes in FRAM are SORTED by ORIGIN (incl. EB then WB,) then PRIORITY, then DESTINATION.
    // Thus, the original order of routes stored in the spreadsheet is lost in the FRAM table, and so we must track recNum (where
    // it can be accessed in FRAM) and routeID (a unique identifier used as cross reference back to Excel spreadsheet.)
    // In the Excel spreadsheet, Rout ID is known but FRAM Record Number isn't yet known, because spreadsheet records will be
    //   sorted by Oigin + Priority + Destination before they are exported to FRAM.
    // The Excel spreadsheet is sorted by my convenient order for organizing and editing routes, such as various types of routes
    //   grouped together (such as level 1 normal routes, level 1 "shuffling" routes, level 2 "escape from single-ended sidings",
    //   and so forth.  I happen to assign Route ID sequentially according to my own sort order, although even in the spreadsheet
    //   it doesn't matter how Route ID is sorted as long as every route has a unique Route ID.
    // The FRAM Route table is sorted by FRAM Record Number, which is assigned *after* re-sorting the spreadsheet by Origin +
    //   Priority + Destination, and *before* exporting to FRAM.  So the FRAM Record Number isn't known yet in the original
    //   iteration of the Routes spreadsheet -- which is why we track both a FRAM Record Number and a Route ID.
    // recNum is the record number 0..n in FRAM for this record.  So we have a unique key that can be used for instant lookup.
    // routeID is the unique Route identifier 1..310.  These are out of sequence in FRAM, after sorting the spreadsheet.
    struct routeReferenceStruct {
      unsigned int recNum;       // FRAM record number 0..n in the Eastbound and Westbound Train Progress tables.  2 bytes.
                                 // Ordered by Origin + Priority + Destination.
      unsigned int routeID;      // Unique xref back to original Excel spreadsheet.  1..310 as of 12/21/22.  2 bytes.
      routeElement origin;       // i.e. BE01.  2 bytes.
      routeElement destination;  // i.e. BW13.  2 bytes.
      bool         park;         // True/False routes that are good parking sidings (BX04 and BX13 are not Park sidings.)
      byte         priority;     // 1 = highest priority (prefer), 5 = lowest priority (goofy route, best avoided when possible)
      routeElement route[FRAM_SEGMENTS_ROUTE_REF];  // Each Route Reference record has 80 2-byte route elements reserved for it.
                                                    // 2 bytes/route element = 160 bytes for the route elements.
    };
    routeReferenceStruct m_routeReference;  // Local working struct variable holds one record.

    FRAM* m_pStorage;           // Pointer to the FRAM memory module

};

#endif
