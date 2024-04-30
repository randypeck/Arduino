// LOCO_REFERENCE.H Rev: 02/17/24.
// A set of functions to retrieve data from the Locomotive Reference table, which is stored in FRAM.
// Needed only by MAS, OCC and LEG.

// IMPORTANT: As of 1/27/23, our populate() data is not correct, but the Excel spreadsheet reflects known values, so when we're
// done testing and ready to use, manually enter the legit data in the Loco_Reference::populate() function of Loco_Reference.cpp.

// 02/17/24: Removed Last-Known Block and Total Run Time fields as unnecessary (6 bytes total.)
// 03/16/23: Wrote getLocoInBlock() that GIVEN A BLOCK NUM returns the LOCO NUM, if any, with a matching Last-Known Block Num;
//           else returns 0.  Called by OCC during Registration to use as the default locoNum when prompting what loco is
//           occupying a given occupied block.  Regarding non-locos such as StationSounds Diners and Crane Cars -- they will be
//           defined as Active records and will have a Device Type of E/T/N/R -- but will *not* be Steam/Diesel type S or D, but
//           rather 1 or 2 (or maybe others in the future.)  We will never Register diners or crane cars, and thus we don't need
//           to worry about Dispatcher sending them on routes during Auto/Park modes.  They will eventually be included by virtue
//           of them being associated with an Op Car Num.
// 03/16/23: Added "Active Record" bool field; true for any type of legitimate data, even if Diner or Crane car.
// 02/15/23: Combined Legacy/TMCC and Engine/Train fields into single devType E|T|N|R
// 02/04/23: Memory errors I though were from populate() but were from display() trying to display alphaDesc and restrictions
//           improperly, not clobbering memory but clobbering the Serial display so it looked like it was a memory problem.
// 01/27/23: Reduced the set of deceleration parms for each loco from 9 to 3.
// 12/05/20: Changed populate to use the heap.

#ifndef LOCO_REFERENCE_H
#define LOCO_REFERENCE_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <FRAM.h>

class Loco_Reference {

  public:

    Loco_Reference();  // Constructor must be called above setup() so the object will be global to the module.
    void begin(FRAM* t_pStorage);  // Called at beginning of registration.

    // These functions expect locoNum to start at 1!

    // Here are all the getters...
    byte          locoNum(const byte t_locoNum);            // Always 1..50, never NULL or STATIC.  FRAM RecNum + 1.
    bool          active(const byte t_locoNum);             // True if this record contains ANY sort of legit data, loco or not.
    void          alphaDesc(const byte t_locoNum, char* t_alphaDesc, const byte t_numBytes);
                  // Caller better pass pointer to char array that's 8 bytes!
    char          devType(const byte t_locoNum);            // E|T|N|R = Legacy Engine|Train, TMCC eNgine|tRain
    char          steamOrDiesel(const byte t_locoNum);      // S|D|1|2    1=StationSounds Diner, 2=Crane/Boom car.
    char          passOrFreight(const byte t_locoNum);      // P|p|F|f|M  M=M.O.W.
    void          restrictions(const byte t_locoNum, char* t_alphaRestrictions, const byte t_numBytes);
                  // Caller better pass pointer to char array that's 5 bytes!
    unsigned int  length(const byte t_locoNum);             // in mm
    byte          opCarLocoNum(const byte t_locoNum);       // Points to this loco's StationSounds diner, Crane car, etc., IF ANY.
//    routeElement  lastKnownLocation(const byte t_locoNum);  // Not guaranteed to be a siding.
//    unsigned long totalRunTime(const byte t_locoNum);       // Unused.
    byte          crawlSpeed(const byte t_locoNum);         // Legacy or TMCC speed setting for "Crawl." 0..199 or 0..31.  i.e. 7.
    unsigned int  crawlMmPerSec(const byte t_locoNum);      // Rate of travel in mm/sec at this speed.  i.e. 23.
    byte          lowSpeed(const byte t_locoNum);           // Legacy or TMCC speed setting for "Low".
    unsigned int  lowMmPerSec(const byte t_locoNum);        // Rate of travel in mm/sec at this speed.
    byte          lowSpeedSteps(const byte t_locoNum);      // Legacy or TMCC speed steps to skip when slowing.  i.e. 3 at a time.
    unsigned int  lowMsStepDelay(const byte t_locoNum);     // Delay in ms to use between each decreasing speed step.  i.e. 500 ms.
    unsigned int  lowMmToCrawl(const byte t_locoNum);       // Distance in mm to slow from Low to Crawl @ above rate.  i.e. 824 mm.
    byte          medSpeed(const byte t_locoNum);
    unsigned int  medMmPerSec(const byte t_locoNum);
    byte          medSpeedSteps(const byte t_locoNum);
    unsigned int  medMsStepDelay(const byte t_locoNum);
    unsigned int  medMmToCrawl(const byte t_locoNum);
    byte          highSpeed(const byte t_locoNum);
    unsigned int  highMmPerSec(const byte t_locoNum);
    byte          highSpeedSteps(const byte t_locoNum);
    unsigned int  highMsStepDelay(const byte t_locoNum);
    unsigned int  highMmToCrawl(const byte t_locoNum);

    void getDistanceAndMomentum(const byte t_locoNum, const byte t_currentSpeed, const unsigned int t_sidingLength,
         byte* t_crawlSpeed, unsigned int* t_msDelayAfterEntry, byte* t_speedSteps, unsigned int* t_msStepDelay);
    // Used by LEG Conductor to slow a train from an incoming speed to the loco's Crawl speed in a given destination siding.
    // Should be called the moment our train trips a destination entry sensor (i.e. the "slow" sensor.)
    // Returned data is calculated to reach this loco's Crawl speed at just the moment it trips Destination sensor; consequently,
    //   we are likely to undershoot 50% of the time, and overshoot 50% of the time -- hopefully by only a small amount.
    // Requires: LocoNum (1..50), current Legacy speed (1..199, not L/M/H), and the length (in mm) of the siding it needs to stop in.
    // Current speed MUST be equal to the loco's Low, Medium, or High speed per this table entry, else fatal error.
    // Siding MUST be long enough to reach Crawl speed from incoming speed using Loco Ref's decel parms, else fatal error.
    // Returns POINTERS to a parms that will allow slowing from current speed to Crawl, plus other info maybe useful or not:
    //   Crawl speed (Legacy/TMCC value) for this loco, which will be our target speed (1..199, or 1..31 if TMCC.)
    //   Time in ms to keep moving at current speed before beginning to slow.
    //     Time_to_wait_before_slowing (in ms) = dist_to_travel_before_slowing / speed_mm_per_second * 1000.
    //   Legacy speed steps to skip (i.e. 5) as we decelerate (always a positive integer.)
    //   Time in ms to delay between speed commands (i.e. 320.)
    // If useful, we can calculate the distance we will travel from tripping the entry sensor until we start slowing:
    //   (t_msDelayAfterEntry / 1000) * m_locoReference.xxxMmPerSec (at the entry speed i.e. Low, Med, or High.)
    // We can alternately calculate the distance we will travel from tripping the entry sensor until we start slowing:
    //   t_sidingLength - m_locoReference.xxxMmToCrawl (same as t_sidingLength - t_mmToReachCrawl.)
    // If useful, we can calculate the time needed to reach Crawl speed once we start slowing; just use our function:
    //   Delayed_Action::speedChangeTime(t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed)
    // If useful, we can retrieve the distance to Crawl once we start slowing:
    //   m_locoReference.xxxMmToCrawl
    // If we are still moving faster than Crawl when we trip the Stop sensor, no problem; we can still stop the train quickly, using
    //   Delayed_Action::populateSlowToStop() (stops from any speed in 3 seconds.)
    // NOTE: Depending on how much we under- or over-shoot our target Crawl location (destination Stop sensor,) either before or long
    //   after all of the "slow down" speed commands in the Delayed Action table have been executed, we will want to report that data
    //   to the operator so they can adjust the Loco Reference deceleration parms if necessary.
    // NOTE: LEG Conductor can't call this function until the moment we trip the destination entry sensor (and note the time so we
    //   can potentially deduct the delay getting data from the "keep moving" time returned.)  This is because the stopping must be
    //   timed precisely starting at the destination entry sensor, since we only know the distance from there to the destination
    //   stop sensor.  I anticipate that we will be able to call this function and get our results virtually instantaneously upon
    //   tripping the destination entry sensor, and we won't need to bother with deducting the small propogation delay.
    // NOTE: Routes must be set up such that the loco speed upon tripping the Crawl (penultimate) sensor must always be slow enough
    //   to allow every loco to reach Crawl speed within the length of the destination siding.  The exception would be if a loco is
    //   restricted from entering that siding then n/a.

//    void setLastKnownLocation(const byte t_locoNum, const routeElement t_lastKnownBlock);  // i.e. BW03.
//
//    byte getLocoInBlock(byte t_blockNum);
//    // Given a blockNum, returns locoNum of the loco with a matching lastKnownLocation.routeRecVal, if any; else return 0.
//    // Called by OCC during Registration to use as the default locoNum when prompting what loco is occupying a given block.

    void display(const byte t_locoNum);  // Display the entire table to Serial COM.
    void populate();  // Special utility reads hard-coded data, writes records to FRAM.

  private:

    void          getLocoReference(const byte t_locoNum);  // Retrieves record from FRAM for given loco.  Loco 1 will be record 0 in the file.
    void          setLocoReference(const byte t_locoNum);
    unsigned long locoReferenceAddress(const byte t_locoNum);  // Return the FRAM address of the *record* in the Loco Reference table for this train.

    // Rev: 01/27/23.  This struct is used ONLY within the Loco Reference class so is private to this class.
    // We use unsigned int for msStepDelay, rather than unsigned long as we do for most time values, just because a step delay of
    // more than a few thousand ms, let alone 64000ms, is inconceivable.
    struct locoReferenceStruct {
      byte          locoNum;                 // Actual train number 1..50 as defined in Legacy/TMCC Engine or Train.
      bool          active;                  // True if this record contains any sort of real data, even if for a diner or crane.
      char          alphaDesc[ALPHA_WIDTH];  // 8 chars.  Not allowing for null char at end since don't want to use Strings.
      char          devType;                 // E|T|N|R = How controlled: Legacy Engine/Train or TMCC eNgine/tRain.
      char          steamOrDiesel;           // S|D|1|2 = Steam, Diesel, StationSounds Diner, operating Crane Boom, etc.
      char          passOrFreight;           // P/p for thru or local Passenger, F/f for thru or local Freight, M for M.O.W.
      char          restrictions[RESTRICT_WIDTH];  // Up to 5 characters.  Not used as of Nov 2020.
      unsigned int  length;                  // i.e. 1500.  Total train length in mm. (i.e. to decide if we can fit in a siding.)
      byte          opCarLocoNum;            // Only if this loco is pulling a StationSounds Diner, Crane/Boom car, etc.; else 0.
//      routeElement  lastKnownLocation;       // Updated by Train Progress? each time the loco trips a sensor; i.e. BW03.
//      unsigned long totalRunTime;            // Future.  In seconds.  Total time this loco has been in Auto/Park mode.
      byte          crawlSpeed;              // Legacy or TMCC speed setting for "Crawl." 0..199 or 0..31.  i.e. 7.
      unsigned int  crawlMmPerSec;           // Rate of travel in mm/sec at this speed.  i.e. 23.
      byte          lowSpeed;                // Legacy or TMCC speed setting for "Low".
      unsigned int  lowMmPerSec;             // Rate of travel in mm/sec at this speed.
      byte          lowSpeedSteps;           // Legacy or TMCC speed steps to skip when slowing.  i.e. 3 at a time.
      unsigned int  lowMsStepDelay;              // Delay in ms to use between each decreasing speed step.  i.e. 500 ms.
      unsigned int  lowMmToCrawl;            // Distance in mm to slow from Low to Crawl @ above rate.  i.e. 824 mm.
      byte          medSpeed;
      unsigned int  medMmPerSec;
      byte          medSpeedSteps;
      unsigned int  medMsStepDelay;
      unsigned int  medMmToCrawl;
      byte          highSpeed;
      unsigned int  highMmPerSec;
      byte          highSpeedSteps;
      unsigned int  highMsStepDelay;
      unsigned int  highMmToCrawl;
    };
    locoReferenceStruct m_locoReference;

    FRAM* m_pStorage;           // Pointer to the FRAM memory module

};

#endif
