// DELAYED_ACTION.H Rev: 02/22/23.  HEAP STORAGE.  TESTED AND WORKING with a few exceptions such as Accessory activation.
// Part of O_LEG (Conductor and Engineer.)
// Delayed Action table is used ONLY by LEG.  Populated by LEG Conductor and de-populated by LEG Engineer.
// Uses about 10K on HEAP.
// 02/15/23: populateLocoCommand() no longer needs to send devType as a parm.  Uses ptr to Loco Ref to just lookup.

// AT SOME POINT, keep track of how big the Delayed Action table gets i.e. the number of the highest record used.
// Maybe provide a call that Conductor can retrieve size from time to time and display on 2004 LCD or thermal printer.

// ***** IMPORTANT REGARDING SPEED CHANGES IF WE HAVE NOT YET REACHED A STANDARD LOW/MED/HIGH SPEED IN A ROUTE *****
// Before populating Delayed Action with a series of speed-change commands, including "slow to a stop", we must first expire any
// pre-existing Absolute Speed and Stop Immediate records, and use the loco's current speed (even if not yet reached the target
// speed) as the starting speed -- just in case we trip a speed-change sensor before all "speed up" or "slow down" records have
// been executed.
// * When changing from one speed to any other except Crawl, via populateLocoSpeedChange(), this should never happen if we follow
//   our rule that each speed change must allow enough time to reach that speed before another speed-change command is sent.  But
//   it still could happen occasionally, and shouldn't be a fatal error as it's not a big deal, especially since our logic
//   automatically begins the next speed change starting at the speed the loco is actually going, not what we think/hope it should
//   have been going.  But it will be useful to get some data so we can make the appropriate adjustment(s) to the route.
// * When changing from any speed to Crawl, via populateLocoSlowToStop(), this can happen fairly frequently because we're trying to
//   achieve Crawl speed at the exact moment we trip the Stop sensor.  So 50% of the time we'll probably undershoot the sensor a
//   bit (in which case we will have reached our target speed in time) but 50% of the time we may overshoot it (i.e. we will NOT
//   have reached our target speed quickly enough) but only by a small margin of error.  Again, no big deal as our starting speed
//   will automatically be set to whatever speed the loco is actually going, and we can stop from pretty much any speed in the
//   three seconds this function allows the loco to stop.  But it will be useful to get some data so we can make appropriate
//   adjustments to the loco's deceleration parameters and/or the route (maybe trip Crawl sensor at a lower speed.)

// Conductor won't populate Delayed Action table with the next slew of records until a loco trips each sensor; not all at once for
// an entire Train Progress route, because Delayed Action records are tripped by TIME, which we won't know until we trip a SENSOR.
// However we may be updating the Tail pointer when rear sensors are cleared.

// For SPEED ADJUSTMENTS, CONDUCTOR will be responsible for determining momentum parameters (start time, number of speed steps,
// size of steps, delay between steps; calculated via Loco_Reference::getDistanceAndMomentum()), but the Delayed Action class will
// translate into multiple records as part of populate().
// Conductor will add horn toots, bell, loco annoumcements, etc. to Delayed Action table when Train Progress records show start,
// stop, reverse etc. As with speed adjustments, for HORN/WHISTLE commands, Conductor will send a pattern command with parmeters
// i.e. BACKUP, GRADE CROSSING, etc., and the Delayed Action class will translate into multiple records.  
// Although DELAYED ACTION *could* automatically insert some commands, when the populate command is to start from a stop, or when
// stopped, etc. (it could know which whistle/horn pattern to blow, turn the bell on/off, make tower announcements etc,) this would
// take away the Conductor's ability to precisely say what time the movements will happen. THUS, THE CONDUCTOR MUST MAKE ALL
// DECISIONS/REQUESTS TO BLOW HORN, BELL, MAKE ANNOUNCEMENTS, SOUND FX BRAKING, etc.

// FUTURE SOUND EFFECTS:
// The CONDUCTOR could insert some sound effects and other commands, at the same time it calls class functions that populate
// Delayed Action with a series of i.e. slow-down or speed-up commands.  Such as RPM (Diesel Run Level, 0..6 (not 7)) and/or Brake
// Squeal sounds when changing speed, steam Long Let-Off Sound and/or Brake Air Release sound when stopped, announcements when
// approaching the station, etc.
// Need to study what sounds steam and diesel locos make when starting, stopping, and standing.  Also confirm if some of these
// sounds are automatic, such as steam let-off when engine stops.  If engine is on a grade or whatever, Conductor will probably
// see commands to slow the loco in the Train Progress file (from Route Reference), and should also send commands to change Engine
// Labor etc.

// PA STATION ANNOUNCEMENTS:
// LEG CONDUCTOR can call functions for Legacy and TMCC loco and car dialogue, including tower communications, but OCC will need to
// independently send PA Station announcements via the WAV Trigger card to actual stations on the layout - since it controls the
// WAV Trigger board. Both LEG and OCC will have a copy of Train Progress and thus know the DESTINATION etc.

// A typical set of Delayed Action records for a train starting a new route might be:
// * Loco-to-tower: request permission to depart / tower gives permission (or not)
// * Station PA: Make departure announcement (this will probably need to be done by OCC, via monitoring the RS-485 bus and/or Train
//   Progress -- BUT HOW WILL OCC (or MAS) KNOW WHEN THE TRAIN IS ACTUALLY DEPARTING???  THAT'S DECIDED BY THE CONDUCTOR...
//   Maybe we need to have LEG use the digital line to MAS to request to send a message "Train 7 departing in x seconds." ******************************
// * Turn on bell, blow whistle sequence, set of increasing speed commands, turn off bell

// Hard-coded (or Action Reference table) commands could include:
// * If entering a tunnel, turn off sound and smoke; turn back on upon exiting.
// * If crossing a road, blow whistle sequence, lower then raise crossing gate, banjo signal, etc.
// * If stopping at a freight siding, activate freight accessories; similar for passenger station (announcements etc.)
// * Randomly activate other accessories around the layout.
// FUTURE: Could also include: open front or rear coupler, other sound effects.
// FUTURE: For a TMCC/Legacy operating accessory, such as the magnet crane and culvert loader, we should send commands
//         to make it look busy.
// The LEG module controls *both* the TMCC commands for operating accessories and cars (obviously) and *also* the Accessory Control
// relay module.

// ***** REGARDING OPERATING CARS SUCH AS TMCC STATIONSOUNDS DINERS *****
// TO DO: When we start up a loco, check the Loco Ref table for "Op Car LocoNum" to see if it has an associated i.e. Stationsounds
// Diner or other operating car.  BE SURE TO INCLUDE A COMMAND TO START UP THAT CAR WHENEVER WE START UP THE LOCO.  I'm not sure
// if the car will work if it isn't first started up.
// As the train progresses through Train Progress, we'll need to keep adding TMCC Stationsounds Diner commands to Delayed Action.

// SCANNING ORDER: If we don't scan quickly enough, it's possible that there will be more than one ripe command to speed up or slow
// down a loco.  If we insert records randomly, we could encounter these out of order, which could slow then speed up then slow
// etc.  Thus our commands that insert speed records into Delayed Action array must always be written to ever-increasing elements.
// Since we write the whole series at once, this will automatically happen, because we will always start at the first Expired
// record and work our way up.  Similarly, when we scan Delayed Action, we must always re-start at the beginning when looking for
// ripe records.

// LATENCY: When populating this table, especially when stopping, we might need to consider a propogation delay (i.e. 200ms) plus
// some additional latency based on the number of trains concurrently running.  Upon starting Auto mode, there will be relatively
// few records in this table, and thus minimal latency.  However as more and more trains start to run, the number of records will
// increase a lot, and latency will increase.  Possibly will remain trivial; time will tell.

#ifndef DELAYED_ACTION_H
#define DELAYED_ACTION_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <Loco_Reference.h>  // Needed to lookup loco's Legacy/TMCC and Engine/Train device type.
#include <Train_Progress.h>  // Needed to lookup current speed for speed change and slow to stop (written by Engineer.)

class Delayed_Action {

  public:

    Delayed_Action();  // Constructor must be called above setup() so the object will be global to the module.

    void begin(Loco_Reference* t_pLoco, Train_Progress* t_pTrainProgress);
    // Initializes entire Delayed Action table to 'E'xpired records.
    // One of the few classes where we don't pass the "FRAM* t_pStorage" parm or maintain the "FRAM* m_pStorage" local variable, as
    // this class doesn't need any access to FRAM (at least not as of this writing.)

    // ****************************************************************************
    // ***** HERE ARE FUNCTIONS THAT LEG CONDUCTOR IS GOING TO WANT ACCESS TO *****
    // ****************************************************************************

    void initialize();  // Will always have HEAP_RECS_DELAYED_ACTION records
    // Init Delayed Action to empty; set all records to 'E'xpired.
    // Whenever Registration (re)starts, we must re-initialize the whole Delayed Access table.

    void populateLocoCommand(const unsigned long t_startTime, const byte t_devNum, const byte t_devCommand, const byte t_devParm1,
                             const byte t_devParm2);
    // populateLocoLocoCommand() adds just about any of the *discrete* LOCO commands, such as LEGACY_ACTION_STARTUP_SLOW,
    // LEGACY_ACTION_REVERSE, LEGACY_SOUND_BELL_ON and LEGACY_DIALOGUE.
    // Some commands, such as LEGACY_DIALOGUE, require parm1 such as LEGACY_DIALOGUE_E2T_ARRIVING.
    // Speed changes and whistle/horn sequences are handled by other functions (which will call populateLocoCommand.)
    // LEGACY_ABS_SPEED, LEGACY_STOP_IMMED, or LEGACY_EMERG_STOP are at the caller's peril, since we don't know how fast we may
    // already be moving.  In any event, we'll erase any remaining Delayed Action speed commands before inserting this one.
    // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
    // See list of all const command values that can be used with populateCommand(), below.

    void populateAccessoryCommand(const unsigned long t_startTime, const byte t_devNum, const byte t_devCommand,
                                  const byte t_devParm1, const byte t_devParm2);
    // populateAccessoryCommand() adds just about any of the accessory or PA etc. commands i.e. PA_SYSTEM_ANNOUNCE (plus parm.)
    // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.

    void populateLocoSpeedChange(const unsigned long t_startTime, const byte t_devNum, const byte t_speedStep,
                                 const unsigned long t_stepDelay, const byte t_targetSpeed);
    // Add a new set of records to Delayed Action to accelerate or decelerate loco from its current speed to a new target speed.
    // Generally this function is called to accelerate from 0/C/L/M to C/L/M/H, or decelerate from L/M/H to C/L/M.
    // Do NOT use this function to decelerate from Crawl to Stop -- use populateLocoSlowToStop() for that.
    // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
    // In order to ensure a smooth speed change even in the event that there are still un-executed "ripe" speed commands in the
    // Delayed Action table, both populateLocoSpeedChange() and populateLocoSlowToStop() will automatically do two things:
    // 1. Get the loco's current speed via m_pTrainProgress->currentSpeed() to use as the starting speed; and
    // 2. Call Delayed_Action::wipeLocoSpeedCommands() to ensure there are no residual speed commands in Delayed Action.
    // t_startTime is usually going to be immediate, unless we want to slow to Crawl speed (in preparation to Stop.)
    // t_speedStep MUST be a positive value, even if we are slowing down.  I.e. 1, 2, 3...
    // t_targetSpeed MUST be a valid Legacy or TMCC value 0..199 or 0..15, as appropriate.
    // t_targetSpeed SHOULD (not must) be one of this loco's "standard" speeds Crawl/Low/Med/High, 0..199, per Loco Ref.
    // The first speed command populated to Delayed Action by this function will be one t_speedStep more/less than our current
    // speed, as there is no point in sending a command to go the speed we're already moving at!
    // When slowing Crawl, caller should add a delay so loco will ideally reach Crawl just at the moment it trips the Stop sensor,
    // and it could also update "expectedStopTime" so we can compare it to the time we actually trip the Crawl sensor.

    void populateLocoSlowToStop(const byte t_devNum);
    // Add new set of recs to Delayed Action to stop the loco from current speed (hopefully Crawl or nearly so) to Stop in 3 secs.
    // Slows from current speed to Legacy speed 1 in one second, then rolls at speed 1 for two seconds, then stops.
    // We could roll at speed 1 for only 1 sec instead of 2, but we travel so little distance and 2 secs at Crawl looks better.
    // This should be a reasonably smooth stop even if we aren't yet at Crawl speed; i.e. for any reasonably slow speed.
    // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
    // In order to ensure a smooth speed change even in the event that there are still un-executed "ripe" speed commands in the
    // Delayed Action table, both populateLocoSpeedChange() and populateLocoSlowToStop() will automatically do two things:
    // 1. Get the loco's current speed via m_pTrainProgress->currentSpeed() to use as the starting speed; and
    // 2. Call Delayed_Action::wipeLocoSpeedCommands() to ensure there are no residual speed commands in Delayed Action.
    // startTime is assumed to be "immediately"
    // startSpeed will be set automatically to the loco's current speed via Train_Progress::currentSpeed()

    void populateLocoWhistleHorn(const unsigned long t_startTime, const byte t_devNum, const byte t_pattern);
    // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
    // The t_pattern parm must be a const horn pattern such as LEGACY_PATTERN_CROSSING.
    // LEGACY HORN COMMAND *PATTERNS* (specified as Parm1 in Delayed Action, by Conductor)
    // For the quilling horn, we'll also need to decide what pitch to specify -- there are about three distinct pitches, and we'll
    // hard code whatever sounds best in the function itself (not as a passed parm.)
    // const byte LEGACY_PATTERN_SHORT_TOOT   =  1;  // S    (Used informally i.e. to tell operator "I'm here" when registering)
    // const byte LEGACY_PATTERN_LONG_TOOT    =  2;  // L    (Used informally)
    // const byte LEGACY_PATTERN_STOPPED      =  3;  // S    (Applying brakes)
    // const byte LEGACY_PATTERN_APPROACHING  =  4;  // L    (Approaching PASSENGER station -- else not needed.  Some use S.)
    // const byte LEGACY_PATTERN_DEPARTING    =  5;  // L-L  (Releasing air brakes. Some railroads that use L-S, but I prefer L-L)
    // const byte LEGACY_PATTERN_BACKING      =  6;  // S-S-S
    // const byte LEGACY_PATTERN_CROSSING     =  7;  // L-L-S-L

    unsigned long speedChangeTime(const byte t_startSpeed, const byte t_speedStep, const unsigned long t_stepDelay,
                                  const byte t_targetSpeed);
    // How long will it take to speed up or slow down a loco using the passed parms?
    // The returned value is independent of *when* the sequence starts; it's an accurate predicted total elapsed time.
    // Determining how *far* a loco will travel when slowing down or speeding up is an educated estimate, but this function returns
    // a very accurate prediction of how long it will take, in milliseconds.

    // *****************************************************************************************
    // ***** HERE WHERE LEG ENGINEER IS GOING TO RETRIEVE RIPE RECORDS FROM DELAYED ACTION *****
    // *****************************************************************************************

    bool getAction(char* t_devType, byte* t_devNum, byte* t_devCommand, byte* t_devParm1, byte* t_devParm2);
    // getAction returns false if there are no ripe records in Delayed Action; else returns true and populates all five parameter
    // fields that are passed as pointers.
    // A returned ripe record will automatically be Expired when it is returned by this function.
    // Just uses current millis(), no need to pass time as a parm.
    // These are not const parms as we are passing by pointer, because we are returning the "action" via the parms.
    // The call will look something like this: pDelayedAction->getAction(&devType, &devNum, &devCommand, &devParm1, &devParm2);
    // We could have returned a struct instead of five parms, but status (Active) and timeToExecute (now!) are moot to Engineer.
    // Engineer is typically expected to use returned data (when available) to immediately populate the Legacy Command Buffer, to
    // be exectuted as soon as possible (typically within milliseconds.)
    // REMINDER: If Engineer retrieves a ripe ABS_SPEED, STOP_IMMED, or EMERG_STOP command (and sends it to the Legacy Command
    // Buffer,) it should also immediately send that info to the loco's Train Progress via Train_Progress::setSpeedAndTime().

    // ****************************************
    // ***** UTILITY FUNCTION FOR TESTING *****
    // ****************************************

    void display();  // For debug purposes, send contents of every Active (non-Expired) record to the console.

  private:

    // DELAYED ACTION STRUCT.  This struct is known only within this class.
    // We will have a HEAP array of 1000 (to start) delayedActionStruct records
    struct delayedActionStruct {        // This struct is used ONLY within this Delayed_Action class, so can be private.
      char          status;             // 'A'ctive, 'E'xpired
      unsigned long timeToExecute;      // Time in ms versus the Arduino's millis()
      char          deviceType;         // [E|T|N|R|A] Legacy Engine or Train, TMCC eNgine or tRain, Accessory (future: Switch, Route)
      byte          deviceNum;          // 0 or 1..255 (Legacy Engines/Trains limited to 1..50) (TMCC Trains limited 1..9 but not used anyway)
                                        // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
      byte          deviceCommand;      // consts i.e. LEGACY_ACTION_ABS_SPEED
      byte          deviceParm1;        // Speed, smoke level, horn pattern, dialogue number, diesel RPM, etc.
      byte          deviceParm2;        // Unused so far
    };

    bool wipeLocoSpeedCommands(const byte t_devNum);
    // Expire any/all unexpired ABS_SPEED, STOP_IMMED, and EMERG_STOP commands in the Delayed Action table for a given loco/device.
    // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
    // Returns TRUE if there WERE un-expired/un-executed speed commands in Delayed Action for this loco; else returns FALSE.

    void populateDelayedAction(const unsigned long t_startTime, const byte t_devNum, const byte t_devCommand,
                               const byte t_devParm1, const byte t_devParm2);
    // Bare-bones version of populateLocoCommand(), called by Delayed_Action class functions only (i.e. private.)

    void insertDelayedAction(const delayedActionStruct t_pDelayedAction);  // Must follow struct definition
    // Insert D.A. element at the lowest Expired record, and update top active element num if appropriate.  Overflow = fatal error.

    bool outOfRangeLocoSpeed(const byte t_devNum, const byte t_devSpeed);

    // Create a pointer variable for the entire Delayed Action struct array; constructor will define it.  We're using a pointer
    // because we want the array to reside on the heap, not in regular RAM.
    delayedActionStruct* m_pDelayedAction;

    // Create a regular (non-pointer) variable so I can pass a regular Delayed Action record between functions.
    delayedActionStruct m_DelayedActionRecord;

    // Create a class private variable to point to the highest record used so far in the Delayed Action table/array.
    // Note that m_TopActiveRec == 0 means there is ONE active record; i.e. start at record zero.
    // So we don't have to scan all 1000 records if most are not even used.
    // This will also be helpful to determine how many records are required based on number of trains running etc.
    // At the end of an operating session, when Auto is stopped or Park is complete, we should display this value.
    int m_TopActiveRec = -1;

    Loco_Reference* m_pLoco;  // Pointer to the Loco Ref class so we can lookup Legacy/TMCC Engine/Train
    Train_Progress* m_pTrainProgress;   // Pointer to the Train Progress class so we can update loco current speed/time.

};

#endif

// ***** OLDER NOTES *****

// LEGACY MOMENTUM/DECELERATION notes: Legacy sends speed commands separated by 1 speed stop when slowing using momentum, and only
// modifies the time between steps to change momentum.  Using the Legacy CAB-2 remote:
//   Legacy momentum 6: Speed steps 1 every 320ms.  This is a reasonable rate of deceleration to use when feasible.
//   Legacy momentum 5: Speed steps 1 every 160ms.
//   Legacy momentum 4: Speed steps 1 every 80ms.
//   Legacy momentum 3: Speed steps 1 every 40ms.
//   Legacy momentum 2: Speed steps 1 every 20ms.
//   Legacy momentum 1: Speed steps 1 every 16ms.  So maybe this is the maximum sustainable Legacy command rate?

// TO MINIMIZE BANDWIDTH REQUIRED FOR DECELERATION MOMENTUM (and also when accelerating):
// Rather than sending individual speed steps every x ms, i.e. 80ms for momentum 4, we cand send speed steps decreasing by 2,
// only half as often i.e. every 160ms, or 4 steps 320ms.  This works fine.

// TRIPLING UP OF LEGACY COMMANDS 6/8/22:
// Per my testing, it is actually the command base that does the tripling up of commands, as monitoring the CAB-2 via the WiFi
// monitor software never shows more than single commands.  So don't worry about it.

// ARCHIVE NOTE: Here is the getAction call I originally designed that passed a Delayed Action struct back and forth via ptr:
//   bool getActionRec(delayedActionStruct* t_delayedActionRecord);
// And here is what the function header looked like:
//   bool Delayed_Action::getActionRec(delayedActionStruct* t_delayedActionRecord) {
// The record would have been returned via the struct pointer that got passed to it.
// A call from i.e. Engineer would have looked something like this (though ever tested):
//   delayedActionStruct delayedActionRecord;  // Or should this be a pointer, and pass the pointer by value???
//   bool gotRec = getActionRec(&delayedActionRecord);

// SUMMARY OF ALL LEGACY/TMCC COMMANDS THAT CAN BE POPULATED IN THE DELAYED-ACTION TABLE as Device Command in Delayed Action:
// Rev: 02/22/23
// Station PA Announcements aren't part of Delayed Action, because OCC controls the WAV Trigger.
//   Don't confuse this with Loco and StationSounds Diner announcements, which ARE handled by Delayed Action.
// const byte LEGACY_ACTION_NULL          =  0;
// const byte LEGACY_ACTION_STARTUP_SLOW  =  1;  // 3-byte
// const byte LEGACY_ACTION_STARTUP_FAST  =  2;  // 3-byte
// const byte LEGACY_ACTION_SHUTDOWN_SLOW =  3;  // 3-byte
// const byte LEGACY_ACTION_SHUTDOWN_FAST =  4;  // 3-byte
// const byte LEGACY_ACTION_SET_SMOKE     =  5;  // 9-byte.  0x7C FX Control Trigger requires parm 0x00..0x03 (Off/Low/Med/Hi)
// const byte LEGACY_ACTION_EMERG_STOP    =  6;  // 3-byte
// const byte LEGACY_ACTION_ABS_SPEED     =  7;  // 3-byte.  Requires Parm1 0..199
// const byte LEGACY_ACTION_MOMENTUM_OFF  =  8;  // 3-byte.  We will never set Momentum to any value but zero.
// const byte LEGACY_ACTION_STOP_IMMED    =  9;  // 3-byte.  Prefer over Abs Spd 0 b/c overrides momentum if instant stop desired.
// const byte LEGACY_ACTION_FORWARD       = 10;  // 3-byte.  Delayed Action table command types
// const byte LEGACY_ACTION_REVERSE       = 11;  // 3-byte.  
// const byte LEGACY_ACTION_FRONT_COUPLER = 12;  // 3-byte.  
// const byte LEGACY_ACTION_REAR_COUPLER  = 13;  // 3-byte.  
// const byte LEGACY_ACTION_ACCESSORY_ON  = 14;  // Not part of Legacy/TMCC; Relays managed by OCC and LEG
// const byte LEGACY_ACTION_ACCESSORY_OFF = 15;  // Not part of Legacy/TMCC; Relays managed by OCC and LEG
// const byte PA_SYSTEM_ANNOUNCE          = 16;  // Not part of Legacy/TMCC; WAV Trigger managed by OCC.
// const byte LEGACY_SOUND_HORN_NORMAL    = 20;  // 3-byte.
// const byte LEGACY_SOUND_HORN_QUILLING  = 21;  // 3-byte.  Requires intensity 0..15
// const byte LEGACY_SOUND_REFUEL         = 22;  // 3-byte.  With minor dialogue.  Nothing on SP 1440.  Steam only??? *******************
// const byte LEGACY_SOUND_DIESEL_RPM     = 23;  // 3-byte.  Diesel only. Requires Parm 0..7
// const byte LEGACY_SOUND_WATER_INJECT   = 24;  // 3-byte.  Steam only.
// const byte LEGACY_SOUND_ENGINE_LABOR   = 25;  // 3-byte.  Diesel only?  Barely discernable on SP 1440. *******************
// const byte LEGACY_SOUND_BELL_OFF       = 26;  // 3-byte.  
// const byte LEGACY_SOUND_BELL_ON        = 27;  // 3-byte.  
// const byte LEGACY_SOUND_BRAKE_SQUEAL   = 28;  // 3-byte.  Only works when moving.  Short chirp.
// const byte LEGACY_SOUND_AUGER          = 29;  // 3-byte.  Steam only.
// const byte LEGACY_SOUND_AIR_RELEASE    = 30;  // 3-byte.  Can barely hear.  Not sure if it works on steam as well as diesel? **************
// const byte LEGACY_SOUND_LONG_LETOFF    = 31;  // 3-byte.  Diesel and (presumably) steam. Long hiss. *****************
// const byte LEGACY_SOUND_MSTR_VOL_UP    = 32;  // 9-byte.  There are 10 possible volumes (9 excluding "off") i.e. 0..9.
// const byte LEGACY_SOUND_MSTR_VOL_DOWN  = 33;  // 9-byte.  No way to go directly to a master vol level, so we must step up/down as desired.
// const byte LEGACY_SOUND_BLEND_VOL_UP   = 34;  // 9-byte.  Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
// const byte LEGACY_SOUND_BLEND_VOL_DOWN = 35;  // 9-byte.  Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
// const byte LEGACY_SOUND_COCK_CLEAR_ON  = 36;  // 9-byte.  Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
// const byte LEGACY_SOUND_COCK_CLEAR_OFF = 37;  // 9-byte.  Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
// const byte LEGACY_DIALOGUE             = 41;  // Separate function.  For any of the 38 Legacy Dialogues (9-byte 0x72), which will be passed as a parm.
// const byte TMCC_DIALOGUE               = 42;  // Separate function.  For any of the 6 TMCC StationSounds Diner cmds, which will be passed as a parm.
// const byte LEGACY_NUMERIC_PRESS        = 43;  // 3-byte

// LEGACY HORN COMMAND *PATTERNS* (only used for Delayed_Action private "populate horn pattern" function.)
// const byte LEGACY_PATTERN_SHORT_TOOT   =  1;  // S    (Used informally i.e. to tell operator "I'm here" when registering)
// const byte LEGACY_PATTERN_LONG_TOOT    =  2;  // L    (Used informally)
// const byte LEGACY_PATTERN_STOPPED      =  3;  // S    (Applying brakes)
// const byte LEGACY_PATTERN_APPROACHING  =  4;  // L    (Approaching PASSENGER station -- else not needed.  Some use S.)
// const byte LEGACY_PATTERN_DEPARTING    =  5;  // L-L  (Releasing air brakes. Some railroads that use L-S, but I prefer L-L)
// const byte LEGACY_PATTERN_BACKING      =  6;  // S-S-S
// const byte LEGACY_PATTERN_CROSSING     =  7;  // L-L-S-L

// LEGACY RAILSOUND DIALOGUE COMMAND *PARAMETERS* (specified as Parm1 in Delayed Action, by Conductor)
// ONLY VALID WHEN Device Command = LEGACY_DIALOGUE.
// Part of 9-byte 0x72 Legacy commands i.e. three 3-byte "words"
// Rev: 06/11/22
// So the main command will be LEGACY_DIALOGUE and Parm 1 will be one of the following.
// Note: The following could just be random bytes same as above, but eventually the Conductor or Engineer will need to know the
// hex value (0x##) to trigger each one -- so they might just as well be the same as the hex codes, right?
// const byte LEGACY_DIALOGUE_T2E_STARTUP                   = 0x06;  // 06 Tower tells engineer to startup
// const byte LEGACY_DIALOGUE_E2T_DEPARTURE_NO              = 0x07;  // 07 Engineer asks okay to depart, tower says no
// const byte LEGACY_DIALOGUE_E2T_DEPARTURE_YES             = 0x08;  // 08 Engineer asks okay to depart, tower says yes
// const byte LEGACY_DIALOGUE_E2T_HAVE_DEPARTED             = 0x09;  // 09 Engineer tells tower "here we come" ?
// const byte LEGACY_DIALOGUE_E2T_CLEAR_AHEAD_YES           = 0x0A;  // 10 Engineer asks tower "Am I clear?", Yardmaster says yes
// const byte LEGACY_DIALOGUE_T2E_NON_EMERG_STOP            = 0x0B;  // 11 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_RESTRICTED_SPEED          = 0x0C;  // 12 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_SLOW_SPEED                = 0x0D;  // 13 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_MEDIUM_SPEED              = 0x0E;  // 14 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_LIMITED_SPEED             = 0x0F;  // 15 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_NORMAL_SPEED              = 0x10;  // 16 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_HIGH_BALL_SPEED           = 0x11;  // 17 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_E2T_ARRIVING                  = 0x12;  // 18 Arriving, E2T asks if lead is clear, tower says yes
// const byte LEGACY_DIALOGUE_E2T_HAVE_ARRIVED              = 0x13;  // 19 E2T in the clear and ready; T2E set brakes
// const byte LEGACY_DIALOGUE_E2T_SHUTDOWN                  = 0x14;  // 20 Dialogue used in long shutdown "goin' to beans"
// const byte LEGACY_DIALOGUE_T2E_STANDBY                   = 0x22;  // 34 Tower says "please hold" i.e. after req by eng to proceed
// const byte LEGACY_DIALOGUE_T2E_TAKE_THE_LEAD             = 0x23;  // 35 T2E okay to proceed; "take the lead", clear to pull
// const byte LEGACY_DIALOGUE_CONDUCTOR_ALL_ABOARD          = 0x30;  // 48 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_FUEL_LEVEL     = 0x3D;  // 61 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_FUEL_REFILLED  = 0x3E;  // 62 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_SPEED          = 0x3F;  // 63 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_WATER_LEVEL    = 0x40;  // 64 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_WATER_REFILLED = 0x41;  // 65 Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_NEXT_STOP        = 0x68;  // 104 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_WATCH_YOUR_STEP  = 0x69;  // 105 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_ALL_ABOARD       = 0x6A;  // 106 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_TICKETS_PLEASE   = 0x6B;  // 107 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_PREMATURE_STOP   = 0x6C;  // 108 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_WELCOME_ABOARD     = 0x6D;  // 109 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_FIRST_SEATING      = 0x6E;  // 110 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_SECOND_SEATING     = 0x6F;  // 111 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_LOUNGE_CAR_OPEN    = 0x70;  // 112 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_ARRIVING  = 0x71;  // 113 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_ARRIVED   = 0x72;  // 114 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_BOARDING  = 0x73;  // 115 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_DEPARTED  = 0x74;  // 116 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PASS_CAR_STARTUP   = 0x75;  // 117 Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PASS_CAR_SHUTDOWN  = 0x76;  // 118 Don't have any Legacy StationSounds diners, so n/a for now.

// TMCC STATIONSOUNDS DINER DIALOGUE COMMAND *PARAMETERS* (specified as Parm1 in Delayed Action, by Conductor)
// ONLY VALID WHEN Device Command = TMCC_DIALOGUE.
// Rev: 06/11/22.
// All of my StationSounds Diners are TMCC; no need for Legacy StationSounds support but we have it.
// These are either single-digit (single 3-byte) commands, or Aux-1 + digit commands (total of 6 bytes.)
// Dialogue commands depend on if the train is stopped or moving, and sometimes what announcements were already made.
//   Although it's possible to assign a Stationsounds Diner as part of a Legacy Train (vs Engine), I think it will be easier to
//   just give each SS Diner a unique Engine ID and associate it with a particular Engine/Train in the Loco Ref table.  Then I can
//   just send discrete TMCC Stationsounds commands to the diner when I know the train is doing whatever is applicable.
//   IMPORTANT: May need to STARTUP (and SHUTDOWN) the StationSounds Diner before using it, so somehow part of train startup.
// TMCC STATIONSOUNDS DINER DIALOGUE COMMANDS specified as Parm1:
//   TMCC_DIALOGUE_STATION_ARRIVAL         = 1;  // 6 bytes: Aux1+7. When STOPPED: PA announcement i.e. "The Daylight is now arriving."
//   TMCC_DIALOGUE_STATION_DEPARTURE       = 2;  // 3 bytes: 7.      When STOPPED: PA announcement i.e. "The Daylight is now departing."
//   TMCC_DIALOGUE_CONDUCTOR_ARRIVAL       = 3;  // 6 bytes: Aux1+2. When STOPPED: Conductor says i.e. "Watch your step."
//   TMCC_DIALOGUE_CONDUCTOR_DEPARTURE     = 4;  // 3 bytes: 2.      When STOPPED: Conductor says i.e. "Watch your step." then "All aboard!"
//   TMCC_DIALOGUE_CONDUCTOR_TICKETS_DINER = 5;  // 3 bytes: 2.      When MOVING: Conductor says "Welcome aboard/Tickets please/1st seating."
//   TMCC_DIALOGUE_CONDUCTOR_STOPPING      = 6;  // 6 bytes: Aux1+2. When MOVING: Conductor says "Next stop coming up."
