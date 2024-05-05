// DELAYED_ACTION.CPP Rev: 05/03/24.  TESTED AND WORKING with a few exceptions such as PowerMasters and Accessory activation.
// Part of O_LEG (Conductor and Engineer.)
// One set of functions POPULATES the Delayed Action table (for Conductor)
// Another set of functions DE-POPULATES the Delayed Action table (for Engineer)
// ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.

#include "Delayed_Action.h"

Delayed_Action::Delayed_Action() {   // Constructor
  // Rev: 02/21/23.
  // HEAP STORAGE: We want the PRIVATE 1000-element delayedAction[] structure array to reside on the HEAP.  We will allocate
  // the memory (about 10K) using "new" in the constructor, and point our private pointer at it.
  m_pDelayedAction = new delayedActionStruct[HEAP_RECS_DELAYED_ACTION];
  // SINCE THE DELAYED ACTION OBJECT CREATED IN THE CALLING .INO PROGRAM IS CREATED ON THE HEAP (VIA "NEW"), WE COULD SET UP OUR
  // m_pDelayedAction[] ARRAY IN THIS CLASS USING REGULAR SRAM, AND WOULD NOT CONSUME ANY MORE SRAM SINCE IT WOULD BE ON THE HEAP.
  // I proved this using tests on 2/21/23:
  // SRAM Version, Delayed_Action.h :
  //   delayedActionStruct m_pDelayedAction[HEAP_RECS_DELAYED_ACTION]{};
  // SRAM Version, Delayed_Action.cpp constructor:
  //   NOTHING in constructor() or begin().Just use normally, same via SRAM or heap, for example :
  //   m_pDelayedAction[i].status = 'E';
  // HEAP Version, Delayed_Action.h:
  //   delayedActionStruct * m_pDelayedAction;
  // HEAP Version, Delayed_Action.cpp constructor:
  //   m_pDelayedAction = new delayedActionStruct[HEAP_RECS_DELAYED_ACTION];
  //   Then use normally, same via SRAM or heap, for example:
  //   m_pDelayedAction[i].status = 'E';
  // Both versions used idential amount of SRAM, and the HEAP(pointer) version used 4 more bytes of Heap space.
  // So there is no advantage to either version -- as long as the class is instantiated in the calling program using "new"; i.e.:
  //   pDelayedAction = new Delayed_Action;
  return;
};

void Delayed_Action::begin(Loco_Reference* t_pLoco, Train_Progress* t_pTrainProgress) {
  // Rev: 02/15/23.
  m_pLoco           = t_pLoco;           // Loco Reference needed to look up loco device type (Legacy/TMCC Engine/Train)
  m_pTrainProgress  = t_pTrainProgress;  // Needed to look up current/starting start speed for speed change and slow to stop.
  if ((m_pLoco == nullptr) || (m_pTrainProgress == nullptr)) {
    sprintf(lcdString, "UN-INIT'd DA PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  initialize();                          // Clear the entire Delayed Access table for a fresh start.
  return;
}

// *****************************************************************************************************************
// ***** HERE ARE FUNCTIONS THAT LEG CONDUCTOR IS GOING TO WANT ACCESS TO (Typically, WRITING COMMAND RECORDS) *****
// *****************************************************************************************************************

void Delayed_Action::initialize() {  // Size will always be HEAP_RECS_DELAYED_ACTION records
  // Rev: 06/09/22.
  // Whenever Registration (re)starts, we must re-initialize the whole Delayed Access table.
  m_TopActiveRec = -1;  // Highest record used so far in Delayed Action table/array, so we don't search all 1000 recs every scan.
  // Note that m_TopActiveRec == 0 means there is ONE active record; i.e. start at record zero.
  for (int i = 0; i < HEAP_RECS_DELAYED_ACTION; i++) {  // i.e. 0..999 for 1000 records
    m_pDelayedAction[i].status = 'E';  // Expired
    m_pDelayedAction[i].timeToExecute = 0;
    m_pDelayedAction[i].deviceType = ' ';
    m_pDelayedAction[i].deviceNum = 0;
    m_pDelayedAction[i].deviceCommand = LEGACY_ACTION_NULL;
    m_pDelayedAction[i].deviceParm1 = 0;
    m_pDelayedAction[i].deviceParm2 = 0;
  }
  sprintf(lcdString, "D.A. Initialized"); pLCD2004->println(lcdString); Serial.println(lcdString);
}

void Delayed_Action::populateLocoCommand(const unsigned long t_startTime, const byte t_devNum, const byte t_devCommand,
                                         const byte t_devParm1, const byte t_devParm2) {
  // Rev: 05/03/24.
  // populateLocoCommand() adds just about any of the DISCRETE, LOCO commands, such as LEGACY_ACTION_STARTUP_SLOW,
  // LEGACY_ACTION_REVERSE, LEGACY_ACTION_ABS_SPEED, LEGACY_SOUND_BELL_ON and LEGACY_DIALOGUE.
  // Some commands, such as LEGACY_DIALOGUE, require parm1 such as LEGACY_DIALOGUE_E2T_ARRIVING.
  // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
  // Speed changes and whistle/horn sequences are handled by other functions.
  // Absolute Speed commands requested from populateLocoCommand() are done at the caller's peril, because we don't know what speed
  // we might already be moving at.  So really it should only be used to stop a loco in an emergency.
  // Here are the commands that I've tested as of 2/22/23:
  //   LEGACY_ACTION_STARTUP_FAST -- Equivalent to quick tap on controller, sens Startup Sequence 2 only.
  //   LEGACY_ACTION_STARTUP_SLOW -- Startup Sequence 1.  MUST ALSO SEND FAST (Seq 2.)  Allow 10 seconds total.
  //     Upon press and hold Start button:
  //       LCS SENDS: F8 09 FB: Eng 4 start-up Sequence 1 (delayed) Audio only including startup sound after talk.
  //     Upon release of Start button (after holding long enough to trigger slow startup):
  //       LCS SENDS: F8 09 FC: Eng 4 start-up Sequence 2 (immed.)  Turns on the cab light and headlight.
  //   LEGACY_ACTION_SHUTDOWN_FAST
  //   LEGACY_ACTION_SHUTDOWN_SLOW -- PLAYS AUDIO ONLY, MUST ALSO SHUTDOWN FAST
  //   LEGACY_ACTION_ABS_SPEED, STOP_IMMED, and EMERGENCY_STOP (and also wipes existing speed commands)
  //   LEGACY_ACTION_SET_SMOKE
  //   LEGACY_ACTION_FORWARD, LEGACY_ACTION_REVERSE
  //   LEGACY_ACTION_FRONT_COUPLER, LEGACY_ACTION_REAR_COUPLER
  //   LEGACY_SOUND_HORN_NORMAL, LEGACY_SOUND_HORN_QUILLING
  //   LEGACY_SOUND_BELL_ON, BELL_OFF
  //   LEGACY_SOUND_MSTR_VOL_UP, VOL_DOWN does not work **************************************************
  //   LEGACY_SOUND_BLEND_VOL_UP, VOL_DOWN also does not work
  //   LEGACY_NUMERIC_PRESS = Parm1 = 1 works to turn VOLUME UP a notch, but must be spaced at least 300ms apart
  //   LEGACY_NUMERIC_PRESS = Parm1 = 4 works to turn VOLUME DOWN a notch, but must be spaced at least 300ms apart
  //   LEGACY_SOUND_LONG_LETOFF
  //   LEGACY_DIALOGUE, LEGACY_DIALOGUE_E2T_ARRIVING and presumably the rest, as long as supported by the loco

  // AS OF 2/22/23 HAVE NOT YET TESTED:
  //   LEGACY_ACTION_NULL
  //   LEGACY_ACTION_MOMENTUM_OFF
  //   LEGACY_ACTION_ACCESSORY_ON / _OFF
  //   PA_SYSTEM_ANNOUNCE

  // If we're moving and we're a loco, and asking to do any ABS_SPEED, STOP_IMMED, or EMERG_STOP, we'll first call
  // Delayed_Action::wipeLocoSpeedCommands() to ensure there are no residual speed commands in Delayed Action (just as we do with
  // populateLocoSpeedChange() and populateLocoSlowToStop().)
  // If it's a command that will affect the loco's speed, then erase any pre-existing not-yet-ripe Delayed Action speed commands.
  // Note that we can exclude PowerMasters and any other device with a deviceNum > TOTAL_TRAINs.
  if ((t_devCommand == LEGACY_ACTION_ABS_SPEED) ||
      (t_devCommand == LEGACY_ACTION_STOP_IMMED) ||
      (t_devCommand == LEGACY_ACTION_EMERG_STOP)) {
    if (t_devNum <= TOTAL_TRAINS) {  // I.e. not a PowerMaster (devNum 91..94)
      // It's a speed-affecting loco command; wipe any existing speed commands for this loco from Delayed Action.
      if (wipeLocoSpeedCommands(t_devNum)) {  // Returns true if there were commands that were wiped, so display a bit of info.
        sprintf(lcdString, "PLC D%2i C%3i P%3i", t_devNum, t_devCommand, t_devParm1); pLCD2004->println(lcdString); Serial.println(lcdString);
      }
    }
  }

  // If this was a speed-related command and there were any un-ripe Delayed Action speed commands, they've now been erased.
  // We may still be moving or not; the caller needs to be responsible for dealing with the consequences in this case.
  // In any event, whatever the command is, let's add it to Delayed Action now.
  populateDelayedAction(t_startTime, t_devNum, t_devCommand, t_devParm1, t_devParm2);
  return;
}

void Delayed_Action::populateAccessoryCommand(const unsigned long t_startTime, const byte t_devNum, const byte t_devCommand,
                                              const byte t_devParm1, const byte t_devParm2) {
  // Rev: 02/15/23.  NOT TESTED *******************************************************************************************************************
  // populateAccessoryCommand() adds just about any of the accessory or PA etc. commands i.e. PA_SYSTEM_ANNOUNCE (plus parm.)
  // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
  m_DelayedActionRecord.status = 'A';  // Active
  m_DelayedActionRecord.timeToExecute = t_startTime;
  m_DelayedActionRecord.deviceType = DEV_TYPE_ACCESSORY;  // 'A'; note we can have an accessory vs loco with the same devNum.
  m_DelayedActionRecord.deviceNum = t_devNum;
  m_DelayedActionRecord.deviceCommand = t_devCommand;
  m_DelayedActionRecord.deviceParm1 = t_devParm1;
  m_DelayedActionRecord.deviceParm2 = t_devParm2;
  insertDelayedAction(m_DelayedActionRecord);
  return;
}

void Delayed_Action::populateLocoSpeedChange(const unsigned long t_startTime, const byte t_devNum, const byte t_speedStep,
                                             const unsigned long t_stepDelay, const byte t_targetSpeed) {
  // Rev: 02/20/23.  TESTED AND WORKING.
  // Add a new set of records to Delayed Action to accelerate or decelerate loco from its current speed to a new target speed.
  // Generally this function is called to accelerate from 0/C/L/M to C/L/M/H, or decelerate from L/M/H to C/L/M.
  // Do NOT use this function to decelerate from Crawl to Stop -- use populateLocoSlowToStop() for that.
  // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
  // In order to ensure a smooth speed change even in the event that there are still un-executed "ripe" speed commands in the
  // Delayed Action table, both populateLocoSpeedChange() and populateLocoSlowToStop() will automatically do two things:
  // 1. Get the loco's current speed via m_pTrainProgress->currentSpeed() to use as the starting speed; and
  // 2. Call Delayed_Action::wipeLocoSpeedCommands() to ensure there are no residual speed commands in Delayed Action.
  // When slowing Crawl, caller should add a delay so loco will ideally reach Crawl just at the moment it trips the Stop sensor,
  // and we could also update "expectedStopTime" so we can compare it to the time we actually trip the Crawl sensor.


  // First, let's get our current speed, which we'll use as our starting speed...
  byte t_startSpeed = m_pTrainProgress->currentSpeed(t_devNum);
  if (t_startSpeed == t_targetSpeed) return;  // Nothing to do; we're already at our target speed!
  if (outOfRangeLocoSpeed(t_devNum, t_startSpeed) || outOfRangeLocoSpeed(t_devNum, t_targetSpeed)) {
    sprintf(lcdString, "DA 1 SPEED ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  if (wipeLocoSpeedCommands(t_devNum)) {  // Erase any remaining speed-related commands from Delayed Action for this loco.
    // Returns true if there were commands that were wiped, so display a bit of info.
    sprintf(lcdString, "PLSC Loco Spd %3i", t_startSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "PLSC Trgt Spd %3i", t_targetSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
  }

  // Start sending speeds one t_speedStep above/below t_startSpeed, because we're presumably already moving at t_startSpeed.
  // Stop sending speeds before we hit t_targetSpeed as we don't want to overshoot target; we'll end with a t_targetSpeed command.
  byte t_delayMultiplier = 0;  // This will increase with each speed step until we reach target; theoretically never more than 199
                               // because we can't have more than 199 steps since Legacy has max 199 speed settings.

  if (t_startSpeed < t_targetSpeed) {  // We want to speed up
    for (byte interrimSpeed = t_startSpeed + t_speedStep; interrimSpeed < t_targetSpeed; interrimSpeed = interrimSpeed + t_speedStep) {
      // The for..loop will stop short of sending t_targetSpeed.
      populateDelayedAction(t_startTime + (t_stepDelay * t_delayMultiplier), t_devNum, LEGACY_ACTION_ABS_SPEED, interrimSpeed, 0);
      t_delayMultiplier++;
    }
    // Now send a final command to move at t_targetSpeed.  We know we won't duplicate this because the above loop stops short of
    // sending t_targetSpeed.
    populateDelayedAction(t_startTime + (t_stepDelay * t_delayMultiplier), t_devNum, LEGACY_ACTION_ABS_SPEED, t_targetSpeed, 0);
  }

  else if (t_startSpeed > t_targetSpeed) {  // We want to slow down
    // 11/10/22: We are using int math with 'slow down' because if we subtract a byte from a byte and the second value is larger,
    // i.e. 2 - 5, the byte result could be a large number i.e. 253 (instead of -3) because byte values are always positive.  And
    // we can't use char because the max value is 127 and we need  up to 199 speed values for Legacy.  So we use "int" which can go
    // negative for the values we're potentially using.
    byte interrimSpeed = t_startSpeed;
    while (((int)interrimSpeed - (int)t_speedStep) > (int)t_targetSpeed) {  // Stop short of sending t_targetSpeed in while() loop.
      interrimSpeed = interrimSpeed - t_speedStep;  // Note that the first speed sent will be one step below t_startSpeed (good!)
      populateDelayedAction(t_startTime + (t_stepDelay * t_delayMultiplier), t_devNum, LEGACY_ACTION_ABS_SPEED, interrimSpeed, 0);
      t_delayMultiplier++;
    }
    // The above loop stops short of sending t_targetSpeed, so now send a final command to move at t_targetSpeed.
    populateDelayedAction(t_startTime + (t_stepDelay * t_delayMultiplier), t_devNum, LEGACY_ACTION_ABS_SPEED, t_targetSpeed, 0);
  }

  // else if t_startSpeed == t_targetSpeed, nothing to do; should never legitimately happen anyway.
  return;
}

void Delayed_Action::populateLocoSlowToStop(const byte t_devNum) {
  // Rev: 02/20/23.  TESTED AND WORKING.
  // Add new set of recs to Delayed Action to slow the loco from current speed (hopefully Crawl or nearly so) to Stop in 3 secs.
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

  // First, let's get our current speed, which we'll use as our starting speed...
  byte t_startSpeed = m_pTrainProgress->currentSpeed(t_devNum);
  if (t_startSpeed == 0) return;  // Nothing to do; we're already stopped!
  if (outOfRangeLocoSpeed(t_devNum, t_startSpeed)) {
    sprintf(lcdString, "DA 2 SPEED ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  if (wipeLocoSpeedCommands(t_devNum)) {  // Erase any remaining speed-related commands from Delayed Action for this loco.
    // Returns true if there were commands that were wiped, so display a bit of info.
    sprintf(lcdString, "PLSS Loco Spd %3i", t_startSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "PLSS Trgt Spd -0-"); pLCD2004->println(lcdString); Serial.println(lcdString);
  }
  unsigned long t_startTime = millis();  // We want to begin stopping immediately.
  // Now we can insert a new set of speed records for the loco...how many speed commands will we need?
  // First speed command will be t_startSpeed - 2 (because we won't send a Speed t_startSpeed command).
  // How many times can we subtract 2 from this value and still be > 1?
  // One second divided by number of steps will give us the delay (in ms) between successive -2 speed step commands.
  // EXAMPLE: t_startSpeed = 1.  Need to send only one command: @1000ms Speed 0.  (Stops in 1 sec rather than 2 secs.)
  //   Now: Nothing since we're already moving at speed 1, @1000ms Speed 0.
  // EXAMPLE: t_startSpeed = 2.  Need to send speed 1.  So 1 command / 1 second = 1000ms between.
  //   Now: Speed 1, @1000ms Speed 0.
  // EXAMPLE: t_startSpeed = 3.  Need to send speed 1.  So 1 command / 1 second = 1000ms between.
  //   Now: Speed 1, @1000ms Speed 0.
  // EXAMPLE: t_startSpeed = 4.  Need to send speed 2, then 1.  So 1 command / 1 second = 1000ms between.
  //   Now: Speed 2, @1000ms Speed 1, @2000ms Speed 0.
  // EXAMPLE: t_startSpeed = 5.  Need to send speeds 3, then 1.  So 1 command / 1 second = 1000ms between.
  //   Now: Speed 3, @1000ms Speed 1, @2000ms Speed 0.
  // EXAMPLE: t_startSpeed = 6.  Need to send speeds 4, 2, then 1.  So 2 commands / 1 second = 500ms between.
  //   Now: Speed 4, @500ms Speed 2, @1000ms Speed 1, @2000ms Speed 0.
  // EXAMPLE: t_startSpeed = 7.  Need to send speeds 5, 3, then 1.  So 2 commands / 1 second = 500ms between.
  //   Now: Speed 5, @500ms Speed 3, @1000ms Speed 1, @2000ms Speed 0.
  // EXAMPLE: t_startSpeed = 8.  Need to send speeds 6, 4, 2, then 1.  So 3 commands / 1 second = 333ms between.
  //   Now: Speed 6, @333ms Speed 4, @666ms Speed 2, @1000ms Speed 1, @2000ms Speed 0.

  // We can't just send successive new speeds - 2 until we're at 1, because we don't know the delay between them.
  // So we need to figure out the delay between speed steps (after the first) before we can start sending them.
  // Here are the values we need to be returned given a starting speed, and exclusing the Speed 1:
  //  0..3 = n/a
  //  4 = 1 1000ms delay  = Send speed 2 immediately, then add 1000ms.
  //  5 = 1 1000ms delay  = Send speed 3 immediately, then add 1000ms.
  //  6 = 2  500ms delays = Send speed 4, +500ms, speed 2, +500ms.
  //  7 = 2  500ms delays = Send speed 5, +500ms, speed 3, +500ms.
  //  8 = 3  333ms delays = Send speed 6, +333ms, speed 4, +333ms, speed 2, +333ms.
  //  9 = 3  333ms delays = Send speed 7, +333ms, speed 5, +333ms, speed 3, +333ms.
  // 10 = 4  250ms delays = Send speed 8, +250ms, speed 6, +250ms, speed 4, +250ms, speed 2, +250ms.
  // 11 = 4  250ms delays = Send speed 9, +250ms, speed 7, +250ms, speed 5, +250ms, speed 3, +250ms.

  // Need a function for values 4 and larger:
  // INPUT   OUTPUT
  //   4        1
  //   5        1
  //   6        2
  //   7        2
  //   8        3
  //   9        3
  // Thus: (Original number - 2) / 2  (because integer division rounds DOWN.)

  if (t_startSpeed > 3) {  // else startSpeed will be 1 or 2 or 3 so we won't need this preliminary loop
    byte stepsNeeded = (t_startSpeed - 2) / 2;
    // Now divide 1 second (1000ms) by the number of steps needed...
    unsigned long stepDelay = 1000 / stepsNeeded;   // How many ms to delay per step
    for (byte i = 0; i < stepsNeeded; i++) {  // For each speed command from current up to but not including speed 1...
      // new start time = original_start_time + (delay_in_ms * step_number)
      unsigned long newStartTime = t_startTime + (stepDelay * i);
      // new speed = original speed - (2 * (i + 1))
      byte newSpeed = t_startSpeed - (2 * (i + 1));
      populateDelayedAction(newStartTime, t_devNum, LEGACY_ACTION_ABS_SPEED, newSpeed, 0);
    }
  }
  populateDelayedAction((t_startTime + 1000), t_devNum, LEGACY_ACTION_ABS_SPEED, 1, 0);  // Drop to Speed 1 after 1 sec.
  populateDelayedAction((t_startTime + 3000), t_devNum, LEGACY_ACTION_ABS_SPEED, 0, 0);  // Full stop after 2 secs @ 1!
  return;
}

void Delayed_Action::populateLocoWhistleHorn(const unsigned long t_startTime, const byte t_devNum, const byte t_pattern) {
  // Rev: 02/22/23.  TESTED AND WORKING.
  // Unless it's a simple toot, whistle/horn commands require multiple records being written to Delayed Action.
  // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
  // Note that we do not specify a Regular horn/whistle versus a Quilling horn/whistle; this function will decide.
  // When the quilling horn is used, we will also hard-code its intensity.  Later we can make this a parm 0..15 if we want.
  // Quilling horn is LONGER than regular horn toot and has 16 possible Legacy commands that narrow to about 3 distinct tones:
  //   0..3 same as regular horn but longer
  //   4..12 deeper/louder
  //   13..15 very deep.  Sounds good.
  // A single regular-horn toot on steam E5 sounds unreliable and terrible -- often just a whisp.  So we will only use the quilling
  // horn for any of our sequences, at least for now.
  // LEGACY_PATTERN_SHORT_TOOT   =  1;  // S    (Used informally i.e. to tell operator "I'm here" when registering)
  // LEGACY_PATTERN_LONG_TOOT    =  2;  // L    (Used informally)
  // LEGACY_PATTERN_STOPPED      =  3;  // S    (Applying brakes)
  // LEGACY_PATTERN_APPROACHING  =  4;  // L    (Approaching PASSENGER station -- else not needed.  Some use S.)
  // LEGACY_PATTERN_DEPARTING    =  5;  // L-L  (Releasing air brakes. Some railroads that use L-S, but I prefer L-L)
  // LEGACY_PATTERN_BACKING      =  6;  // S-S-S
  // LEGACY_PATTERN_CROSSING     =  7;  // L-L-S-L
  // FOR DISCRETE REGULAR HORN TOOTS, 450ms between commands is the minimum delay; 400ms and they occasionally run together.
  //   Though 700ms sounds better and 800ms may be necessary for E6 loco.
  // FOR CONTINUOUS REGULAR HORN TOOTS, 175ms between commands is the maximum delay; 200ms and there are occasional gaps.
  // FOR DISCRETE QUILLING TOOTS, 1000ms between commands is the minimum delay; 950ms unreliable on SP 1440.
  // FOR CONTINUOUS QUILLING TOOTS, 650ms between commands is the maximum delay; 700ms introduces occasional gaps

  unsigned long tootTime = t_startTime;
  //unsigned long hornRegularDiscreteGapMinimum = 700;  // Gap between discrete toots must be AT LEAST 450ms, though 700ms sounds better
  unsigned long hornContinuousQuillingMaxDelay = 650;  // 650ms is reliable maximum on Big Boy, 700 has gaps
  unsigned long hornCrossingSequenceDelay1Quilling = 2000;  // 1st gap between crossing quilling toots
  unsigned long hornCrossingSequenceDelay2Quilling = 2000;  // 2nd gap between crossing quilling toots
  unsigned long hornCrossingSequenceDelay3Quilling = 1500;  // 3rd gap between crossing quilling toots
  byte quillIntensity = 14;  // Try 2, 6, and 14
  // quillIntensity 2 is same as regular horn; 6 is deeper, sounds great; 14 is really deep, also sounds great.

  switch (t_pattern) {

    case LEGACY_PATTERN_SHORT_TOOT:   // S Regular
    case LEGACY_PATTERN_STOPPED:      // S Regular
    {
      // Rev: 06/17/22.
      populateDelayedAction(t_startTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      break;
    }
    case LEGACY_PATTERN_LONG_TOOT:    // L Quilling horn
    case LEGACY_PATTERN_APPROACHING:  // L Quilling horn
    {
      // Rev: 06/17/22.
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      break;
    }
    case LEGACY_PATTERN_DEPARTING:    // L-L Quilling horn
    {
      // Rev: 06/17/22.
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornCrossingSequenceDelay1Quilling;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      break;
    }
    case LEGACY_PATTERN_BACKING:      // S-S-S Regular horn
    {
      // Rev: 06/17/22.
      // GOOD TIMING, BUT PITCH CAN CHANGE FOR EACH TOOT.
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornCrossingSequenceDelay1Quilling;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornCrossingSequenceDelay1Quilling;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      break;
    }
    case LEGACY_PATTERN_CROSSING:     // L-L-S-L Quilling
    {
      // Rev: 06/17/22.
      // Quilling sequence is longer than with the regular toot, but it sounds really great!
      // LONG
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      // LONG
      tootTime = tootTime + hornCrossingSequenceDelay1Quilling;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      // SHORT
      tootTime = tootTime + hornCrossingSequenceDelay2Quilling;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      // LOOOOONG
      tootTime = tootTime + hornCrossingSequenceDelay3Quilling;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      tootTime = tootTime + hornContinuousQuillingMaxDelay;
      populateDelayedAction(tootTime, t_devNum, LEGACY_SOUND_HORN_QUILLING, quillIntensity, 0);
      break;
    }
  }
}

unsigned long Delayed_Action::speedChangeTime(const byte t_startSpeed, const byte t_speedStep,
                                              const unsigned long t_stepDelay, const byte t_targetSpeed) {
  // Rev: 10/24/22.
  // How long will it take to speed/slow a loco using the passed parms?
  // The returned value is independent of *when* the sequence starts; it's an accurate predicted total elapsed time.
  // It's simply a matter of multiplying the number of speed steps by the length of the delay between steps.
  // Using endTime calculation, we need to consider if (t_startSpeed - t_targetSpeed) is evenly divisible by t_speedStep.
  // If there is a remainder, then we'll execute one more speed command than if there is no remainder.
  // The order of this calculation is important; remember that integer division TRUNCATES any remainder.
  // Because a change from one speed to another is two speeds, but only requires one t_stepDelay, subtract t_stepDelay from total.
  // Also adding 5ms to the total time required for some safety cushion.
  // Need to use integers because byte subtraction can result in large positive number; not negatives as may be required.
  // I.e. byte target speed 3 - byte start speed 5 = byte 253; but int 3 - int 5 = -2 which is what we'd want.
  int speedDiff = (int)t_targetSpeed - (int)t_startSpeed;  // May be positive or negative Legacy/TMCC speed difference.
  speedDiff = abs(speedDiff);  // Can't use a formula inside abs() function.  Strange but true.
  unsigned long speedChangeTime = ((speedDiff / t_speedStep) * t_stepDelay) - t_stepDelay + 5;  // Adding 5ms for safety's sake.
  if (speedDiff % t_speedStep == 0) {  // Change in speed is evenly divisible by number of steps
    //sprintf(lcdString, "Divisible."); pLCD2004->println(lcdString);
  }
  else {  // There will need to be one extra speed command since not evenly divisible
    //sprintf(lcdString, "Remainder."); pLCD2004->println(lcdString);
    // Since there is going to be one extra speed step, add t_stepDelay to the total time.
    speedChangeTime = speedChangeTime + t_stepDelay;
  }
  return speedChangeTime;
}

// *****************************************************************************************
// ***** HERE WHERE LEG ENGINEER IS GOING TO RETRIEVE RIPE RECORDS FROM DELAYED ACTION *****
// *****************************************************************************************

bool Delayed_Action::getAction(char* t_devType, byte* t_devNum, byte* t_devCommand, byte* t_devParm1, byte* t_devParm2) {
  // Rev: 06/09/22.
  // getAction returns false if no ripe records in Delayed Action; else returns true and populates all five parameter
  // fields that are passed as pointers.
  // A returned ripe record will automatically be Expired when it is returned by this function.
  // Just uses current millis(), no need to pass time as a parm.
  // We always re-start scanning Delayed Action with the first element, for reasons explained elsewhere.
  // These are not const parms as we are passing by pointer, because we are returning the "action" via the parms.
  // The call will look something like this: pDelayedAction->getAction(&devType, &devNum, &devCommand, &devParm1, &devParm2);
  // We could have returned a struct instead of five parms, but status and timeToExecute are moot to Engineer, and we like
  // returning "success" as a bool.
  if (m_TopActiveRec < 0) {  // There are no records at all to even look at
    return false;  // No ripe record found
  }
  for (int i = 0; i <= m_TopActiveRec; i++) {
    if ((m_pDelayedAction[i].status == 'A') && (m_pDelayedAction[i].timeToExecute <= millis())) {  // Got a ripe one!
      *t_devType    = m_pDelayedAction[i].deviceType;  // [E|T|N|R|A]
      *t_devNum     = m_pDelayedAction[i].deviceNum;
      *t_devCommand = m_pDelayedAction[i].deviceCommand;
      *t_devParm1   = m_pDelayedAction[i].deviceParm1;
      *t_devParm2   = m_pDelayedAction[i].deviceParm2;
Serial.print("getAction found record for device "); Serial.print(*t_devType); Serial.print(*t_devNum);
Serial.print(", Command "); Serial.print(*t_devCommand); Serial.print(", Parm "); Serial.println(*t_devParm1);
      m_pDelayedAction[i].status = 'E';  // Expire the record now that we're going to return it; no need to write as it's heap
      return true;  // Found a ripe record, and fields being returned via the pointer
    }
  }
  return false;  // No ripe record found
}

// *********************************************
// ***** UTILITY FUNCTION USED FOR TESTING *****
// *********************************************

void Delayed_Action::display() {
  // Rev: 12/20/22.
  // For debug purposes, display every Active (non-Expired) record to console.
  if (m_TopActiveRec < 0) {  // There are no records at all to even look at
    return;
  }
  Serial.print("Delayed Action Top Record: "); Serial.println(m_TopActiveRec);
  for (int i = 0; i <= m_TopActiveRec; i++) {
    if (m_pDelayedAction[i].status == 'A') {  // Got an active record (may or may not be ripe.)
      Serial.print("Rec "); Serial.print(i); Serial.print(", Time: "); Serial.print(m_pDelayedAction[i].timeToExecute);
      Serial.print(", Dev Type: "); Serial.print(m_pDelayedAction[i].deviceType); Serial.print(", Dev Num: "); Serial.print(m_pDelayedAction[i].deviceNum);
      Serial.print(", Cmd: "); Serial.print(m_pDelayedAction[i].deviceCommand); Serial.print(", P1: "); Serial.print(m_pDelayedAction[i].deviceParm1);
      Serial.print(", P2:"); Serial.println(m_pDelayedAction[i].deviceParm2);
    }
  }
}

// *******************************************
// ***** PRIVATE FUNCTIONS USED BY CLASS *****
// *******************************************

bool Delayed_Action::wipeLocoSpeedCommands(const byte t_devNum) {
  // Rev: 02/20/23.
  // Expire any/all unexpired ABS_SPEED, STOP_IMMED, and EMERG_STOP commands in the Delayed Action table for a given loco/device.
  // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
  // Returns TRUE if there WERE un-expired/un-executed speed commands in Delayed Action for this loco; else returns FALSE.
  // Should always be called before inserting a new speed command, or set of speed commands, into Delayed Action.
  // Currently called by populateCommand(), populateLocoSpeedChange(), and populateLocoSlowToStop() to automatically be sure we
  // don't step on residual Delayed Action speed commands.
  // If Delayed Action speed records need to be wiped, this function will output some data to the Serial monitor so we can debug.
  // Any error here will presumably be the result of a Route that does not allow enough time to reach the target speed before
  // tripping another sensor to change to some other speed.  Or when stopping, we will see how far off our calculations were in
  // reaching Crawl speed at just the moment that we trip the Stop sensor.
  // In the event there are un-ripe/un-executed speed commands in Delayed Action for this loco, of most interest will be the two
  // speeds t_nextSpeed (the first un-executed speed command) and t_targetSpeed (the "farthest out" un-executed speed command,) and
  // t_msDelayError, the additional time Delayed Action would have needed to bring the loco to the previous target speed.
  // All three values are displayed on the LCD and sent to the Serial monitor/mini thermal printer for later analysis.

  // Although this can tell us how much we were overshooting a Stop sensor, it can't help us determine if we *undershoot* a Stop
  //   sensor; i.e. reach Crawl speed much before tripping the Stop sensor.  We'll need to address that elsewhere.
  // Overshooting or undershooting a Stop sensor is going to happen virtually every time, because we'll never time a slow-to-Crawl
  //   Crawl so perfectly as to reach Crawl at the exact instant we trip the Stop sensor -- but we're only concerned with
  //   significant errors.
  // *****************
  // *** IMPORTANT ***
  // *****************
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
  // Currently, wipeLocoSpeedCommands() does NOT remove other commands that may have been coordinated with wiped commands, such as
  //   whistle/horn, dialogue, sound effects, etc. so we may want to look at this as a future enhancement.  However bear in mind
  //   that as long as the Route Reference routes have been designed so that a loco has sufficient time to reach a desired VLxx
  //   speed before another VLxx speed command is encountered (following a sensor trip) we *should* ALWAYS already be moving at the
  //   anticipated speed.  The exception would usually only be when we slow to Crawl, we may overshoot the Stop sensor.
  // So there should only be any unexpired speed commands in the Delayed Action table in the unfortunate cases where we have an
  //   issue where Route Reference has a VLxx speed command that's reached (by tripping a sensor) before the loco has reached the
  //   previous VLxx speed, or if we overshoot a Stop sensor slightly.

  // NOTE: We could use t_nextSpeed (see below) as the starting speed for the next set of speed commands (if we were to return that
  // value to the  caller,) but instead the caller will use Train Progress's m_pTrainProgress->currentSpeed(t_devNum), as it's
  // updated by Engineer each time a speed command is sent to the loco, and thus always exactly correct.  t_nextSpeed will be close
  // to the loco's actual speed (though not exact,) but let's standardize on the Train Progress value.

  // m_TopActiveRec is our global highest record used so far in Delayed Action table/array 0..n.
  if (m_TopActiveRec == -1) {  // If the value is still -1, we have not added any records, so nothing to check!
    return false;  // No un-expired Delayed Action records, obviously.
  }

  // Set up a few variable to be used just for displaying/printing debug information within this function...
  byte t_nextSpeed = 255;  // Set to 255 so we'll be able to detect if we have a relevant value below.
  long t_nextTime = 0;
  // The next speed that was due to be executed, but is being wiped.  This will be the speed that's the the most "off" from
  // whatever our previous target speed was, and will be one speed command beyond the loco's current speed.
  //   I.e. Accelerating from 20 to 80 step 2, and we only got to 68, then it will be 70.
  //   I.e. Decelerating from 80 to 20 step 2, and we only got to 34, then it will be 32.
  byte t_targetSpeed = 255;  // Set to 255 so we'll be able to detect if we have a relevant value below.
  // The final target speed of this sequence, that is being wiped.
  //   I.e. Accelerating from 20 to 80 step 2, and we only got to 68, then it will be 80.
  long t_msDelayError = 0;  // Set to zero so we'll be able to detect if we have a relevant value below.
  // How many more ms would have been needed to reach target speed? The extra time we would have needed to finish executing the
  // speed commands that are being purged.  The bigger the value, the worse our error was.
  //   I.e. Accel from 20 to 80 step 2 each 250ms, and we only got to 68, then it will be 1500ms (six more steps at 250ms each.)
  for (int i = 0; i <= m_TopActiveRec; i++) {  // Check every record of Delayed Action
    if ((m_pDelayedAction[i].status == 'A') && (m_pDelayedAction[i].deviceNum == t_devNum)) {
      // We have an Active (yet yet ripe) record for this device number, but it *could* be an accessory not a loco...
      if ((m_pDelayedAction[i].deviceType == DEV_TYPE_LEGACY_ENGINE) ||
          (m_pDelayedAction[i].deviceType == DEV_TYPE_LEGACY_TRAIN) ||
          (m_pDelayedAction[i].deviceType == DEV_TYPE_TMCC_ENGINE) ||
          (m_pDelayedAction[i].deviceType == DEV_TYPE_TMCC_TRAIN)) {
        // Okay, we have an active (not yet ripe) record for the device in question, *and* we know it's a loco.
        // Now see if it's a command that will affect the loco's speed...
        if ((m_pDelayedAction[i].deviceCommand == LEGACY_ACTION_ABS_SPEED) ||
            (m_pDelayedAction[i].deviceCommand == LEGACY_ACTION_STOP_IMMED) ||
            (m_pDelayedAction[i].deviceCommand == LEGACY_ACTION_EMERG_STOP)) {
          // Yikes, we found an active speed-changing command - not what we want, but that's what we're here for!
          m_pDelayedAction[i].status = 'E';  // Expire this baby!  No need to "write" since this is an array.
          // Set debug value "t_nextSpeed" to the FIRST unexpired speed.
Serial.print("Erasing speed  "); Serial.println(m_pDelayedAction[i].deviceParm1);
Serial.print("Ripening time  "); Serial.println(m_pDelayedAction[i].timeToExecute);
Serial.print("Current time   "); Serial.println(millis());
Serial.println("---------------------------");

          if (t_nextSpeed == 255) {  // If we have not yet set this value, it must be the first unexpired record.
            // Once it's been set to a non-255 value, it'll be left alone for any subsequent speed records.
            // This will give us the "farthest away from complete" speed so we'll know how far off we were.
            // If we happen to have only one ripe command, and it happens to be STOP_IMMED (should never be EMERG_STOP,) I'm
            // counting on the value of parm1 to be zero, just as it would be if the one ripe command was ABS_SPEED zero.
            t_nextSpeed = m_pDelayedAction[i].deviceParm1;  // Return speed "farthest" from the previous target.
            t_nextTime = m_pDelayedAction[i].timeToExecute;  // Gives us the next time we'd want to execute
            // This will be just one speed step away from current actual speed; close to our current speed.
          }
          // On the other hand, we'll update "target speed" and "delay error" each time through the loop, so by the last iteration
          // (i.e. the last unexpired speed command for this loco) t_targetSpeed will hold the speed that we had hoped to be moving
          // at (but weren't), and t_msDelayError will hold the largest difference between "now" and when the last un-expired
          // speed command was set to be executed.  This is correct because we're guaranteed that records progress in time, so we
          // keep searching and the last record we find will become the farthest-in-the-future time difference.
          t_targetSpeed = m_pDelayedAction[i].deviceParm1;
          t_msDelayError = m_pDelayedAction[i].timeToExecute - t_nextTime;  // Difference in time
        }
      }
    }
  }  // Keep going even if we found something -- need to scan every Active record

  // If t_nextSpeed is still 255, we didn't find any records to expire; otherwise let's report some data to the operator...
  if (t_nextSpeed != 255) {  // There were records to wipe
    sprintf(lcdString, "WARN: DA not empty!"); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "Loco     Num    %2i", t_devNum); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "Next     Speed %3i", t_nextSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "Farthest Speed %3i", t_targetSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "Time Err %8ld", t_msDelayError); pLCD2004->println(lcdString); Serial.println(lcdString);
    return true;  // We DID have at least one un-expired Delayed Action speed command.
  }
  return false;  // No un-expired Delayed Action speed records, which is what we'd hoped for!
}

void Delayed_Action::populateDelayedAction(const unsigned long t_startTime, const byte t_devNum, const byte t_devCommand,
                                           const byte t_devParm1, const byte t_devParm2) {
  // Rev: 02/20/23.
  // populateDelayedAction() PRIVATELY adds just about any of the *discrete* LOCO commands, such as LEGACY_ACTION_STARTUP_SLOW,
  // LEGACY_ACTION_REVERSE, LEGACY_ACTION_ABS_SPEED, LEGACY_SOUND_BELL_ON and LEGACY_DIALOGUE.
  // Some commands, such as LEGACY_DIALOGUE, require parm1 such as LEGACY_DIALOGUE_E2T_ARRIVING.
  // There is no error checking and no calling wipeLocoSpeedCommands() -- that must be done by the calling function.
  // ALL DEVICES INCLUDING ACCESSORIES START AT 1, not 0.
  m_DelayedActionRecord.status = 'A';  // Active
  m_DelayedActionRecord.timeToExecute = t_startTime;
  // There is special code to handle PowerMasters: They are numbered LOC_ID_POWERMASTER_1..4 = 91..94.  A they are out of range of
  // standard loco numbers, rather then call pLoco->devType, we'll just hard-code them here as type Legacy Engine ('N').
  // I could also have done this in Loco_Reference but I'd rather keep it isolated as much as possible.  Another option would have
  // been to number them within our 1..50 legitimate locoNums.
  if ((t_devNum >= LOCO_ID_POWERMASTER_1) && (t_devNum <= LOCO_ID_POWERMASTER_4)) {
    m_DelayedActionRecord.deviceType = DEV_TYPE_LEGACY_ENGINE;  // Hard code PowerMasters as 'E' Legacy Engines
  } else {
    m_DelayedActionRecord.deviceType = m_pLoco->devType(t_devNum);  // Look up the loco's type in Loco Reference i.e. E|T|N|R
  }
  m_DelayedActionRecord.deviceNum = t_devNum;
  m_DelayedActionRecord.deviceCommand = t_devCommand;
  m_DelayedActionRecord.deviceParm1 = t_devParm1;
  m_DelayedActionRecord.deviceParm2 = t_devParm2;
  insertDelayedAction(m_DelayedActionRecord);
  return;
}

void Delayed_Action::insertDelayedAction(const delayedActionStruct t_delayedActionRecord) {
  // Rev: 06/09/22.
  // Insert at the lowest Expired record, and update top active element num if appropriate.
  // Return type is moot because if we overflow the array, we'll terminate with a fatal error.
  // Class global m_TopActiveRec points to highest Active (or previously-Active, now-Expired) record; else -1 if no recs yet.
  // Assumes all records beyond m_TopActiveRec will have .status set to 'E'; never junk or 'A'
  // No need to use a pointer to pass the struct, as we won't be modifying the incoming data.

  // A little example if we were allowing HEAP_RECS_DELAYED_ACTION = 5 array elements:
  // 
  //                       <- m_TopActiveRec = -1, recNumForAdd = -1
  //   m_pDelayedAction[0]
  //   m_pDelayedAction[1]
  //   m_pDelayedAction[2]
  //   m_pDelayedAction[3]
  //   m_pDelayedAction[4] <- m_TopActiveRec = 4
  //                       <- HEAP_RECS_DELAYED_ACTION = 5

  int recNumForAdd = -1;  // This will hold the element number where we insert the new data
  // First, find the lowest Expired record...
  if (m_TopActiveRec < 0) {
    // There are no records yet so we'll add the first one
    recNumForAdd = 0;  // Congratulations, you're the first Delayed Action record!
  } else {
    // There's at least one rec, though it may already be expired, so search until we find an Expired rec *or* we reach the end.
    recNumForAdd = 0;  // We know there is a record 0 but don't yet know if it's Expired or Active
    while ((recNumForAdd < HEAP_RECS_DELAYED_ACTION) && (m_pDelayedAction[recNumForAdd].status == 'A')) {
      // Testing == 'A' is the same as testing != 'E'; the only two possible values.
      // Remember, HEAP_RECS_DELAYED_ACTION is one higher than the highest array element i.e. 1000 vs 999
      // Note that it's important to do the above two comparisons in the order shown, because if
      // recNumForAdd == HEAP_RECS_DELAYED_ACTION, then a check of m_pDelayedAction[recNumForAdd] is beyond top element.
      recNumForAdd++;
    }
  }
  // When we get here, recNumForAdd will be where we want to put the new data, unless it's == HEAP_RECS_DELAYED_ACTION, in which
  // case we are out of room.
  if (recNumForAdd == HEAP_RECS_DELAYED_ACTION) { // we've overflowed the D.A. array (tested/working 6/9/22)
    sprintf(lcdString, "DA Ovfl: %i", recNumForAdd); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  // Great, we know we have a place for our new Active record: recNumForAdd, and it's within the bounds of our D.A. array.
  // Still, we could be 1 beyond m_TopActiveRec, which just means we also need to expand the top rec.
  if (recNumForAdd > m_TopActiveRec) {
    m_TopActiveRec = recNumForAdd;
    // sprintf(lcdString, "New top: %i", m_TopActiveRec); pLCD2004->println(lcdString); Serial.println(lcdString);
  }
  // If we get here, we know that recNumForAdd is where we want to put the new data, and m_TopActiveRec is up to date.  Yay.
  // Now assign the data from the struct we've been passed into the Delayed Action array element.
  m_pDelayedAction[recNumForAdd].status = 'A';
  m_pDelayedAction[recNumForAdd].timeToExecute = t_delayedActionRecord.timeToExecute;
  m_pDelayedAction[recNumForAdd].deviceType = t_delayedActionRecord.deviceType; // [E | T | N | R | A]
  m_pDelayedAction[recNumForAdd].deviceNum = t_delayedActionRecord.deviceNum;
  m_pDelayedAction[recNumForAdd].deviceCommand = t_delayedActionRecord.deviceCommand;
  m_pDelayedAction[recNumForAdd].deviceParm1 = t_delayedActionRecord.deviceParm1;
  m_pDelayedAction[recNumForAdd].deviceParm2 = t_delayedActionRecord.deviceParm2;
  // Nothing needs to be "written" as this is a heap array (as opposed to a FRAM record.)
  // Yipee!  We have a new Delayed Action element, in the first (lowest) available element.
  return;
}

bool Delayed_Action::outOfRangeLocoSpeed(const byte t_devNum, const byte t_devSpeed) {
  // Legacy Engine/Train speed can be 0..199; TMCC Engine/Train speed can be 0..31
  if ((m_pLoco->devType(t_devNum) == DEV_TYPE_LEGACY_ENGINE) || (m_pLoco->devType(t_devNum) == DEV_TYPE_LEGACY_TRAIN)) {
    if ((t_devSpeed < 0) || (t_devSpeed > 199)) {  // Only 0..199 are valid speeds for Legacy
      return true;  // true means it's out of range
    } else {
      return false;  // Legacy speed AOK
    }
  } else if ((m_pLoco->devType(t_devNum) == DEV_TYPE_TMCC_ENGINE) || (m_pLoco->devType(t_devNum) == DEV_TYPE_TMCC_TRAIN)) {
    if ((t_devSpeed < 0) || (t_devSpeed > 31)) {  // Only 0..31 are valid speeds for TMCC
      return true;  // true means it's out of range
    } else {
      return false;  // TMCC speed AOK
    }
  } else {  // This must be an error if not Legacy or TMCC Engine or Train
    sprintf(lcdString, "DA SPD ERR L%i", t_devNum); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "DA SPD ERR S%i", t_devSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
    endWithFlashingLED(5);
  }
  return true;  // Will never get exectuted
}
