// ENGINEER.CPP Rev: 05/04/24.
// Part of O_LEG.
// 05/04/24: Adding support for TMCC Abs Speed.  Already have support for TMCC Dialogue though not tested at all.
// 03/02/23: No support for TMCC else complete and generally works but needs more rigorous testing, including Accessories.

// The Engineer class requests pending/ripe Legacy/TMCC commands from Conductor, via the Delayed Action table, and sends those
// commands to the loco or other appropriate equipment (usually the locomotive, but also potentially StationSounds Diner cars,
// other operating rolling stock, TMCC-controlled accessories, and the Accessory Relay bank.)
// If the command affects a loco's speed, also update the loco's Train Progress Speed-and-Time and Stopped/Time Stopped fields.

// Although most Legacy, and all TMCC, commands are three bytes each, because *some* TMCC commands are 6 bytes long, and *some*
// Legacy commands can be 9 bytes long, we will maintain all struct elements and struct arrays to support up to nine bytes.
// * If byte 4 is zero then we assume it's a three-byte command
// * If byte 4 is non-zero but byte 7 is zero, we'll assume it's a 6-byte command
// * Otherwise (both bytes 4 and 7 are non-zero) it's assumed to be a 9-byte command.

// 06/01/22 CONFIRMED TRIPLICATE COMMANDS are NOT NEEDED.  Per Rudy (Lionel sound engineer), regarding sending TMCC/Legacy commands
// in triplicate (edited): "There has always been redundancy in the transmission of TMCC/Legacy commands. This is for power
// interruptions but also the electrical environment in the locomotive is a harsh one; there's a ton of noise. So commands in
// triplicate has always been prudent. IT IS ACTUALLY THE COMMAND BASE THAT DOES THE ACTUAL TRIPLING OF COMMANDS, so we don't
// even see them via the WiFi monitor of CAB 2.
// UPDATE 6/22: Confirmed that monitoring WiFi serial commands originated from the CAB-2 controller, I don't see any commands ever
// being duplicated.  So this must all be happening behind the scenes by the Legacy base and I don't need to worry about it.

// Minimum time of LEGACY_CMD_DELAY (30ms) must have passed since the last 3-/6-/9-byte instruction was sent to the Legacy Base.
// This is due to a limitation of the Legacy Base, which will ignore commands sent less than about 25ms apart.  The Delayed Action
// class will still remove from the Delayed Action table as soon as they are ripe; the Engineer is responsible for the 30ms
// separation.  Engineer can assume that if it's given a command from Delayed Action, it should be executed as quickly as possible.
// PER PROF. CHAOS, if  you are sending multiple 3-byte commands, you have to wait ~ 30 milliseconds between sets of 3/6/9 byte
// commands or the Base will ignore them. The approach I take is to deposit TMCC commands into a circular buffer.  Then every pass
// through loop(), I check to see if at least 30 milliseconds have elapsed since the last command transmitted to the Base.  If so,
// the next command is read from the buffer and sent to the base.  I think I found 25 milliseconds between command triplets was the
// minimum time via trial-and-error, and used 30ms to be safe.

#include "Engineer.h"

Engineer::Engineer() {  // Constructor
  // HEAP STORAGE: We want the PRIVATE 200 (or whatever) element nine-byte buffer (struct) array to reside on the HEAP.  We will
  // allocate the memory using "new" in the constructor, and point our private pointer at it.
  m_pLegacyCommandBuf = new legacyCommandStruct[LEGACY_CMD_HEAP_RECS];
  return;
}

void Engineer::begin(Loco_Reference* t_pLoco, Train_Progress* t_pTrainProgress, Delayed_Action* t_pDelayedAction) {
  // Rev: 02/17/23.  We'll call this when we initialize the Engineer object right after calling the constructor, but we should ALSO
  // call this function each time Registration begins, to properly reset the Legacy Cmd Buf release Accessory Relays.
  // SHIFT REGISTER LOGIC WILL CRASH THE SYSTEM IF CENTIPEDE NOT HOOKED UP.
  m_legacyLastTransmit = millis();  // Update each time we actually send a command to prevent Lionel Legacy base hardware overflow.
  // 2/17/23 m_pShiftRegister is NOT NEEDED as pShiftRegister is "extern" in Train_Functions.h
  // m_pShift_Register = t_pShift_Register;  // For accessory relay control.
  m_pLoco          = t_pLoco;           // Where we'll look up device type E/T/N/R/A
  m_pTrainProgress = t_pTrainProgress;  // We'll update Train Progress current loco speed as commands are retrieved.
  m_pDelayedAction = t_pDelayedAction;  // Where we'll retrieve "ripe" commands, to send to Legacy Cmd Buffer, thence loco.
  if ((m_pLoco == nullptr) || (m_pTrainProgress == nullptr) || (m_pDelayedAction == nullptr)) {
    sprintf(lcdString, "UN-INIT'd ER PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  initLegacyCommandBuf();  // Initialize all LEGACY_CMD_HEAP_RECS records of the Legacy command buffer
  initAccessoryRelays();    // Release all accessory relays here in begin().
  return;
}

void Engineer::initLegacyCommandBuf() {  // Size will be LEGACY_CMD_HEAP_RECS
  // Rev: 06/16/22.  Done but not tested.
  // (Re)init the whole m_pLegacyCommandBuf[] array.
  // Public because LEG will need to call this whenever Registration mode (re)starts.
  // Init circular buffer to store incoming Delayed Action Legacy/TMCC commands (translated to 3/6/9-byte commands.)
  // Number of elements is LEGACY_CMD_HEAP_RECS i.e. maybe 200?  They will be executed almost as quickly as they're read in.
  // Number of bytes/element is LEGACY_CMD_BYTES, which is 9 because that's the most bytes a Legacy/TMCC command can have.
  m_legacyCommandBufHead = 0;  // Next array element to be written.
  m_legacyCommandBufTail = 0;  // Next array element to be removed.
  m_legacyCommandBufCount = 0;  // Num active elements in buffer; i.e. LEGACY_CMD_HEAP_RECS
  // Initializing every element to zero is redundant since it's done every time we populate a command, but what the heck.
  for (int i = 0; i < LEGACY_CMD_HEAP_RECS; i++) {  // LEGACY_CMD_HEAP_RECS i.e. 0..199 for 200 records
    for (byte j = 0; j < LEGACY_CMD_BYTES; j++) {   // Always 9 bytes, 0..8
      m_pLegacyCommandBuf[i].legacyCommandByte[j] = 0;
    }
  }
}

void Engineer::initAccessoryRelays() {
  // Rev: 02/17/23.  DONE BUT NOT TESTED.
  // Turn off all accessory relays.
  // Public because LEG will need to call this whenever Registration mode (re)starts.
  for (byte i = 0; i < TOTAL_LEG_ACCY_RELAYS; i++) {
    Engineer::controlAccessoryRelay(i, LEGACY_ACTION_ACCESSORY_OFF);
  }
  return;
}

void Engineer::executeConductorCommand() {
  // Rev: 02/12/23.
  // Get a command from the Delayed Action table, if any, and execute a command from the Legacy Command buffer, if any.
  // We call it Execute *Conductor* Cmd because all Delayed Action (and thus Legacy Cmd Buf) commands originate with Conductor.
  // This function should be called as frequently as possible when running in Auto or Park modes.
  // The getDelayedActionCommand() function will ensure that only ripe commands are retrieved.
  // The sendCommandToTrain() function will ensure that Legacy/TMCC cmds are spaced in time as req'd by the Legacy base.
  Engineer::getDelayedActionCommand();
  Engineer::sendCommandToTrain();
  return;
}

// *******************************************
// ***** PRIVATE FUNCTIONS USED BY CLASS *****
// *******************************************

void Engineer::getDelayedActionCommand() {
  // Rev: 05/03/24.
  // Retrieve a ripe record (if any) from the Delayed Action table (human-readable version), then:
  //   a. If the record is an Accessory-control command, then activate or deactivate a relay
  //   b. If the record is a Legacy or TMCC, Engine orTrain command, then:
  //      1) Translate from human-readable to actual 3-, 6-, or 9-byte Legacy/TMCC formatted command, then
  //      2) Enqueue it in the Legacy Command Buffer (for almost-immediate transmission to Legacy Base/loco), then
  //      3) If it is a speed command, update loco's various speed and time fields in Train Progress.
  // Even though we must still transmit the speed command from the Legacy Cmd Buf to the Legacy Base (and then loco) before the
  // train will actually be moving at that speed, this will happen within milliseconds of updating Train Progress's current speed.
  // devType can be [E|T|N|R|A] DEV_TYPE LEGACY/TMCC ENGINE/TRAIN or DEV_TYPE_ACCESSORY.
  // This function must be called as frequently as possible (every time through loop) by Engineer to keep "ripe" commands that are
  // in the Delayed Action table moving out and into the Legacy Command Buffer (for loco commands) or accessory etc.

  // Create a set of local variables that Delayed Action can populate with ripe data, if any.
  char m_devType = ' ';
  byte m_devNum = 0;      // NOTE: All devices must start with 1, even accessories!
  byte m_devCommand = 0;  // From a list of CONST values in Train_Consts_Global.h, such as LEGACY_ACTION_SET_SMOKE
  byte m_devParm1 = 0;
  byte m_devParm2 = 0;

  // Try to get a Delayed Action record...(will return the oldest ripe record, if any exist.)
  if (!m_pDelayedAction->getAction(&m_devType, &m_devNum, &m_devCommand, &m_devParm1, &m_devParm2)) {
    return;  // If NOT found, returns false to indicate there wasn't a ripe record and thus nothing to do.
  }

  // Got a Delayed Action record; it could be an Accessory, or a 3/6/9-byte Legacy or TMCC command...
  // t_devType must be [E|T|N|R|A]: Engine (Legacy), or Train (Legacy), eNgine (TMCC), tRain (TMCC), Accessory
  // Note that the PowerMasters are stored as Legacy Engine absolute speed commands (0 = off, 1 = on.)

  // Is this a LEGACY Engine or Train command?
  if ((m_devType == DEV_TYPE_LEGACY_ENGINE) || (m_devType == DEV_TYPE_LEGACY_TRAIN)) {
    Serial.println("Found Legacy command in Delayed Action...");
    // We have a new Legacy Engine or Train command from Delayed Action.
    // First we must translate the Delayed Action data into a 3-, 6-, or 9-byte Legacy command.
    m_legacyCommandRecord = translateToLegacy(m_devNum, m_devCommand, m_devParm1, m_devParm2);
    // Then we'll enqueue it into the Legacy/TMCC command buffer (to be executed within milliseconds, we presume.)
    commandBufEnqueue(m_legacyCommandRecord);  // Insert Legacy command into the circular buffer.
    // If this is a loco speed adjustment, and not a PowerMaster, update Train Progress' loco speed and time header fields.
    if ((m_devCommand == LEGACY_ACTION_STOP_IMMED) ||
        (m_devCommand == LEGACY_ACTION_EMERG_STOP)) {
      Serial.print("Engineer setting Train Progress speed to zero (stop immed or emerg stop): m_devNum: "); Serial.print(m_devNum);
      m_pTrainProgress->setSpeedAndTime(m_devNum, 0, millis());
      m_pTrainProgress->setStopped(m_devNum, true);
      m_pTrainProgress->setTimeStopped(m_devNum, millis());
    } else if ((m_devCommand == LEGACY_ACTION_ABS_SPEED) &&
               (m_devNum <= TOTAL_TRAINS)) {  // TOTAL_TRAINS = 50; PowerMasters are 91..94 so they're excluded here.
      Serial.print("Engineer setting Train Progress speed for m_devNum: "); Serial.print(m_devNum); Serial.print(", Speed: "); Serial.println(m_devParm1);
      m_pTrainProgress->setSpeedAndTime(m_devNum, m_devParm1, millis());
      // NOTE 3/5/23: Not sure where I want to set Train Progress "stopped", "time stopped", and "time starting" fields, or what
      // I'll use them for.  For sure, I need MAS Dispatcher to send time to start when it sends an EXTENSION route.
      // Time to Start (not updated here, by the way) is useful for:
      //   * LEG Conductor to trigger pre-departure loco/tower comms i.e. get permission to depart
      //   * LEG Conductor to get the train moving
      //   * OCC Station Announcer to make pre-departure announcements
      // For speed adjustments, we can assume that these will be executed almost immediately because they're ripe now.
      // Thus, we'll update various Train Progress fields such as isStopped, timeStopped, currentSpeed, and currentSpeedTime.
      if (m_devParm1 == 0) {  // If the train is being stopped (or IMMED or EMERG), set isStopped true and timeStopped.
        Serial.println("Setting train STOPPED.");
        // Maybe we should have a function to combine setStopped and setTimeStopped, like we do with setSpeedAndTime, but when we
        // set Stopped to false (i.e. when we start moving) I don't think we'll want to update time stopped.
        m_pTrainProgress->setStopped(m_devNum, true);
        m_pTrainProgress->setTimeStopped(m_devNum, millis());
      } else {  // If the speed is changing but not stopping, update isStopped false (in case we were previously stopped.)
        Serial.println("Setting train NOT stopped.");
        m_pTrainProgress->setStopped(m_devNum, false);
      }
    }
    return;  // If LEGACY_ENGINE or LEGACY_TRAIN command, we're done.
  }

  // Is it a TMCC Engine or Train?
  if ((m_devType == DEV_TYPE_TMCC_ENGINE) || (m_devType == DEV_TYPE_TMCC_TRAIN)) {
    Serial.println("Found TMCC command in Delayed Action...");
    // We have a new TMCC Engine or Train command from Delayed Action.
    // First we must translate the Delayed Action data into a 3- or 6-byte TMCC command.
    m_legacyCommandRecord = translateToTMCC(m_devNum, m_devCommand, m_devParm1, m_devParm2);
    // Then we'll enqueue it into the Legacy/TMCC command buffer (to be executed within milliseconds, we presume.)
    commandBufEnqueue(m_legacyCommandRecord);  // Insert TMCC command into the circular buffer.
    // If this is a loco speed adjustment, update Train Progress' loco speed and time header fields.
    // Note that TMCC doesn't support Stop Immediate, and Emerg Stop is handled as Legacy, above.
    if (m_devCommand == LEGACY_ACTION_ABS_SPEED) {
      Serial.print("Engineer setting Train Progress speed for m_devNum: "); Serial.print(m_devNum); Serial.print(", Speed: "); Serial.println(m_devParm1);
      m_pTrainProgress->setSpeedAndTime(m_devNum, m_devParm1, millis());
      // For speed adjustments, we can assume that these will be executed almost immediately because they're ripe now.
      // Thus, we'll update various Train Progress fields such as isStopped, timeStopped, currentSpeed, and currentSpeedTime.
      if (m_devParm1 == 0) {  // If the train is being stopped, set isStopped true and timeStopped.
        Serial.println("Setting train STOPPED.");
        m_pTrainProgress->setStopped(m_devNum, true);
        m_pTrainProgress->setTimeStopped(m_devNum, millis());
      } else {  // If the speed is changing but not stopping, update isStopped false (in case we were previously stopped.)
        Serial.println("Setting train NOT stopped.");
        m_pTrainProgress->setStopped(m_devNum, false);
      }
    }
    return;  // If TMCC_ENGINE or TMCC_TRAIN command, we're done.
  }

  // Last resort: if it's an Accessory Relay command, execute immediately (just turn relay on or off)...
  // FUTURE: Perhaps at some point we'll want parm1 to be time to leave on or ???
  if (m_devType == DEV_TYPE_ACCESSORY) {
    Serial.println("Found Accessory command in Delayed Action...");
    // Open or close a relay.
    // m_devNum must be relay number 1..TOTAL_LEG_ACCY_RELAYS.
    // m_devCommand must be LEGACY_ACTION_ACCESSORY_ON = Close, or LEGACY_ACTION_ACCESSORY_OFF = Open.
    // FUTURE: Could include m_devParm1 to be *time* to leave on/off.
    controlAccessoryRelay(m_devNum, m_devCommand);
    return;
  }

  // If we get here, it's a fatal error since it's neither an Accessory, nor a Legacy or TMCC Engine or Train
  sprintf(lcdString, "BAD CMD TYPE: %c", m_devType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  return;  // Will never execute
}

void Engineer::sendCommandToTrain() {
  // Rev: 06/16/22.
  // This function must be called as frequently as possible (every time through loop) by Engineer to keep things moving.
  // Dequeues up to one "plain English" record, if any, from the Legacy/TMCC Command Buffer (not more often than each 30ms).
  // If a record is found, translate to Legacy/TMCC hex and forward to the Legacy Command Base via the RS-232 interface.
  // We'll use our class global 9-byte struct variable m_legacyCommandRecord to hold our working command.

  // First see if we have a 3/6/9-byte commands in the Legacy circular buffer that can be transmitted...  We'll only get
  // something if there is a record available *and* it has been at least 30ms since the last command was sent to the Legacy base.
  // If there is data, it's returned via pointer (m_legacyCommandRecord.)
  bool success = commandBufDequeue(&m_legacyCommandRecord);  // Will return nine zeroes if nothing; else populate command record.
  // If we got a Legacy command from the circular buffer (queue), then send it along to the Legacy base.
  if (success) {
    sendCommandToLegacyBase(m_legacyCommandRecord);
  }
  return;
}

void Engineer::controlAccessoryRelay(byte t_RelayNum, byte t_CloseOrOpen) {
  // Rev: 02/17/23.  COMPLETE BUT NEEDS TO BE TESTED ******************************************************************************************************************************
  // t_RelayNum will be 1...TOTAL_LEG_ACCY_RELAYS.
  // t_CloseOrOpen will be LEGACY_ACTION_ACCESSORY_ON = Close contact, or LEGACY_ACTION_ACCESSORY_OFF = Open contact.
  // pShiftRegister is a global pointer defined in Train_Functions.h, similar to pLCD2000.
  if (outOfRangeDevNum(DEV_TYPE_ACCESSORY, t_RelayNum)) {
    sprintf(lcdString, "ENG'R RELAY NUM ERR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  if (t_CloseOrOpen == LEGACY_ACTION_ACCESSORY_ON) {
    pShiftRegister->digitalWrite(t_RelayNum, LOW);   // turn ON the relay  DOES IT WORK TO USE THE GLOBAL/EXTERN pShiftRegister? **************************************************************************
  }
  else if (t_CloseOrOpen == LEGACY_ACTION_ACCESSORY_OFF) {
    pShiftRegister->digitalWrite(t_RelayNum, HIGH);  // turn OFF the relay
  }
  else {  // Fatal error!
    sprintf(lcdString, "ENG RELAY BAD CMD!"); pLCD2004->println(lcdString); Serial.println(lcdString);
  }
  return;
}

Engineer::legacyCommandStruct Engineer::translateToTMCC(byte t_devNum, byte t_devCommand, byte t_devParm1, byte t_devParm2) {
  // Rev: 05/04/24.
  // Populate a Legacy (actually TMCC) command buffer, 3- or 6-bytes long.
  // It's necessary to prefix this return type with "Engineer::" because it's a private struct.
  // Only gets called when t_devType = TMCC Engine or Train; other device types handled by other functions.
  // Translate a Delayed Action "plain English" command into a TMCC 3- or 6-byte command.
  // We'll use our class global 9-byte struct variable m_legacyCommandRecord to hold our calculated TMCC command bytes.
  // Remember, bytes 1..9 are element 0..8!
  // The following TMCC Engine/Train commands (t_devCommand) are supported:
  //   * LEGACY_ACTION_EMERG_STOP
  //   * LEGACY_ACTION_ABS_SPEED
  //   * LEGACY_ACTION_STOP_IMMED (treated as Abs Speed = 0)
  //   * LEGACY_ACTION_FORWARD
  //   * LEGACY_ACTION_REVERSE
  //   * LEGACY_ACTION_FRONT_COUPLER
  //   * LEGACY_ACTION_REAR_COUPLER
  //   * LEGACY_SOUND_HORN_NORMAL
  //   * LEGACY_SOUND_BELL_ON, LEGACY_SOUND_BELL_OFF: Future.  Not sure how they work as there is only one Bell command in TMCC.
  //   * TMCC_DIALOGUE, StationSounds Diner dialogue commands specified as t_devParm1:
  //     * TMCC_DIALOGUE_STATION_ARRIVAL         When STOPPED: PA announcement i.e. "The Daylight is now arriving."
  //     * TMCC_DIALOGUE_STATION_DEPARTURE       When STOPPED: PA announcement i.e. "The Daylight is now departing."
  //     * TMCC_DIALOGUE_CONDUCTOR_ARRIVAL       When STOPPED: Conductor says i.e. "Watch your step."
  //     * TMCC_DIALOGUE_CONDUCTOR_DEPARTURE     When STOPPED: Conductor says i.e. "Watch your step." then "All aboard!"
  //     * TMCC_DIALOGUE_CONDUCTOR_TICKETS_DINER When MOVING: Conductor says "Welcome aboard/Tickets please/1st seating."
  //     * TMCC_DIALOGUE_CONDUCTOR_STOPPING      When MOVING: Conductor says "Next stop coming up."
  // NOTE: The above commands are LEGACY_* constants because they're just the "plain English" version of the commands, such as
  //       LEGACY_ACTION_ABS_SPEED.  Based on the device type (Legacy vs TMCC) we'll call the appropriate "translateToXXX"
  //       function (here), which by virtue of being called knows what type of device (Legacy vs TMCC, though we'll have to look it
  //       up in Loco Ref in order to differentiate between Engine vs Train.)
  //       I'm not clear, as of 5/4/24, how TMCC_DIALOGUE will be specified in Delayed Action, because unlike the rest of the above
  //       commands, the Conductor (whoever populates Delayed Action) would need to know that it's talking to a TMCC device vs a
  //       Legacy device.  Of course at this point, ALL StationSounds diner cars, and other devices with dialogue, will be TMCC.
  // NOTE: I don't think we need to support the TMCC Momentum commands (Low/Med/High -- there is no "off") because momentum is
  //       handled by the Legacy remote.  I.e. when you set momentum, you see successive speed commands coming from the remote, not
  //       a single speed command sent to loco and then the loco slows down on its own.  Ditto with Legacy Momentum.

  // First confirm that we have a valid device type and number...
  byte t_devType;  // Will be TMCC eNgine or tRain (N/R)
  t_devType = m_pLoco->devType(t_devNum);  // N|R else bug in database
  // We should only be sent commands for TMCC Engines or Trains so confirm this:
  if (outOfRangeTMCCDevType(t_devType)) {
    sprintf(lcdString, "ENG'R TMCC DT ERR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  // TMCC train num can be 1..9, engine num can be 1..50.
  if (t_devCommand != LEGACY_ACTION_EMERG_STOP) {
    if (outOfRangeDevNum(t_devType, t_devNum)) {
      sprintf(lcdString, "ENG'R TMCC DN ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
    }
  }

  // Clear out all nine bytes...even though TMCC can use at most 6.
  for (byte j = 0; j < LEGACY_CMD_BYTES; j++) {  // Always 9 bytes, 0..8
    m_legacyCommandRecord.legacyCommandByte[j] = 0;
  }

  // We ALWAYS know what byte 0 will be for TMCC:
  m_legacyCommandRecord.legacyCommandByte[0] = 0xFE;  // 254

  // We ALWAYS know what byte 1 (second byte) will be for TMCC (except emergency stop, which we'll override when needed)
  // 05/05/24: HERE IS AN ALTERNATIVE WAY TO BUILD THE SECOND TMCC COMMAND BYTE:
  // Byte is combination of device type field and first 7 bit of TMCC address:
  // If TMCC Engine, binaryDeviceType = 0x00.  If TMCC Train, binaryDeviceType = 0xC8.
  //   m_legacyCommandRecord.legacyCommandByte[1] = (binaryDeviceType | (t_devNum >> 1));
  if (t_devType == DEV_TYPE_TMCC_ENGINE) {
    // Bit pattern is 00AA AAAA:
    //   AA AAAA is everything except the right-most bit of the device number.
    m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum >> 1);
  } else {  // Must be TMCC Train
    // Bit pattern is 1100 1AAA:
    //   AAA is everything except the right-most bit of the device number.
    // Hence, it's the same as for a TMCC Engine, except we need to add 1100 1000 which is 0xC8.
    m_legacyCommandRecord.legacyCommandByte[1] = (0xC8 + (t_devNum >> 1));  // 0xC8 = binary 1100 1000
  }

  // Byte 2 (third byte) of any TMCC command is the same regardless if this is for an Engine or a Train.
  // Bit pattern is ACCD DDDD:
  //   A is the right-most bit of the device number (t_locoNum)
  //   CC is the two-bit command category -- usually 00, but 01 for momentum commands and 11 for speed commands.
  //   D DDDD command number, such as 1 1100 for "Blow Horn."
  // If CC = 00, then add 0x00 = 0000 (all commands except momentum and absolute speed.)
  // If CC = 01, then add 0x02 = 0010 (momentum commands.)
  // If CC = 11, then add 0x06 = 0110 (speed commands.)
  // There are no Command Categories of 10.
  // 05/05/24: HERE IS AN ALTERNATIVE WAY TO BUILD THE THIRD TMCC COMMAND BYTE (using bitwise OR, rather than addition.)
  // Byte 3 is last bit of TMCC address, command field (2 bits), plus data field:
  //   m_legacyCommandRecord.legacyCommandByte[2] = ((t_devNum << 7) | (commandField << 5) | dataField);

  switch (t_devCommand) {

    case LEGACY_ACTION_EMERG_STOP:     //  We'll do System Halt which turns off PowerMasters too!
    {
      m_legacyCommandRecord.legacyCommandByte[1] = 0xFF;
      m_legacyCommandRecord.legacyCommandByte[2] = 0xFF;
      break;
    }

    case LEGACY_ACTION_ABS_SPEED:      // Requires Parm1 0..31
    {
      // Speed can be 0..31 for TMCC
      if (outOfRangeLocoSpeed(t_devType, t_devParm1)) {
        sprintf(lcdString, "ENG'R TMCC SPD ERR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
      }
      // First put the right-most bit of t_devNum as the left-most  bit of byte #3: X000 0000
      m_legacyCommandRecord.legacyCommandByte[2] = (t_devNum << 7);
      // For speed commands, the command bits are 11: 0110 0000
      m_legacyCommandRecord.legacyCommandByte[2] += 0x60;  // Adding binary 0110 0000 = 0x60
      // Plus the 5-bit speed 0..31.
      m_legacyCommandRecord.legacyCommandByte[2] += t_devParm1;
      break;
    }

    case LEGACY_ACTION_STOP_IMMED:      // We'll treat this as Abs Speed 0 for TMCC
    {
      // First put the right-most bit of t_devNum as the left-most  bit of byte #3: X000 0000
      m_legacyCommandRecord.legacyCommandByte[2] = (t_devNum << 7);
      // For speed commands, the command bits are 11: 0110 0000
      m_legacyCommandRecord.legacyCommandByte[2] += 0x60;  // Adding binary 0110 0000 = 0x60
      // Plus the 5-bit speed, which is zero, so we'll skip this step.
      break;
    }

    case LEGACY_ACTION_FORWARD:
    {
      break;
    }

    case LEGACY_ACTION_REVERSE:
    {
      break;
    }

    case LEGACY_ACTION_FRONT_COUPLER:
    {
      break;
    }

    case LEGACY_ACTION_REAR_COUPLER:
    {
      break;
    }

    case LEGACY_SOUND_HORN_NORMAL:
    {
      break;
    }

    case LEGACY_SOUND_BELL_ON:
    case LEGACY_SOUND_BELL_OFF:
      {
        // Unsupported but not an error, so just return a blank command record.
        m_legacyCommandRecord.legacyCommandByte[0] = 0;
        m_legacyCommandRecord.legacyCommandByte[1] = 0;
        break;
      }

    case TMCC_DIALOGUE:
      {
        // For "TMCC_DIALOGUE", which is for StationSounds Diner cars, t_devParm1 must specify which of the six supported dialogues are
        // wanted.  Some of these TMCC StationSounds Diner commands are 3 bytes (single keypad numeric key press), and some are 6 bytes
        // (combo numberic key press of Alt-1 + a numberic.)
        // NOTE: All of my StationSounds Diners are TMCC; no need for Legacy StationSounds support at this time.
        // These are either 3-byte or 6 bytes commands.
        // TMCC STATIONSOUNDS DINER DIALOGUE COMMANDS specified as Parm1:
        //   TMCC_DIALOGUE_STATION_ARRIVAL         = 1;  // 6 bytes: Aux1+7. When STOPPED: PA announcement i.e. "The Daylight is now arriving."
        //   TMCC_DIALOGUE_STATION_DEPARTURE       = 2;  // 3 bytes: 7.      When STOPPED: PA announcement i.e. "The Daylight is now departing."
        //   TMCC_DIALOGUE_CONDUCTOR_ARRIVAL       = 3;  // 6 bytes: Aux1+2. When STOPPED: Conductor says i.e. "Watch your step."
        //   TMCC_DIALOGUE_CONDUCTOR_DEPARTURE     = 4;  // 3 bytes: 2.      When STOPPED: Conductor says i.e. "Watch your step." then "All aboard!"
        //   TMCC_DIALOGUE_CONDUCTOR_TICKETS_DINER = 5;  // 3 bytes: 2.      When MOVING: Conductor says "Welcome aboard/Tickets please/1st seating."
        //   TMCC_DIALOGUE_CONDUCTOR_STOPPING      = 6;  // 6 bytes: Aux1+2. When MOVING: Conductor says "Next stop coming up."
        // So there are FOUR unique possible commands.
        if (outOfRangeTMCCDialogueNum(t_devParm1)) {  // Must be 1..6
          sprintf(lcdString, "ENG TMCC BAD DIALOG!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
        }

        // Regardless if it's a 3-byte or a 6-byte TMCC command, bytes 1 and 2 (and 4 and 5 if used) will always be the same.
        // So we could compress this code, but for readability we're going to assign every byte in every switch-case block.
        switch (t_devParm1) {  // Which of the six dialogues are desired?

          case TMCC_DIALOGUE_STATION_ARRIVAL:  // 2 bytes: Aux1 + 7
            {
              m_legacyCommandRecord.legacyCommandByte[2] = (t_devNum << 7) + 0x09;  // Aux button; same for Train vs Engine.
              // Since this is a 6-byte command, keep going...
              m_legacyCommandRecord.legacyCommandByte[3] = m_legacyCommandRecord.legacyCommandByte[0];
              m_legacyCommandRecord.legacyCommandByte[4] = m_legacyCommandRecord.legacyCommandByte[1];
              m_legacyCommandRecord.legacyCommandByte[5] = (t_devNum << 7) + 0x17;  // 0x17 = digit 7 on keypad
              break;
            }

          case TMCC_DIALOGUE_STATION_DEPARTURE:  // 1 byte: 7
            {
              m_legacyCommandRecord.legacyCommandByte[2] = (t_devNum << 7) + 0x17;  // 0x17 = digit 7 on keypad
              break;
            }

          case TMCC_DIALOGUE_CONDUCTOR_ARRIVAL:   // 2 bytes: Aux1 + 2
          case TMCC_DIALOGUE_CONDUCTOR_STOPPING:  // 2 bytes: Aux1 + 2
            {
              m_legacyCommandRecord.legacyCommandByte[2] = (t_devNum << 7) + 0x09;  // Aux button; same for Train vs Engine.
              // Since this is a 6-byte command, keep going...
              m_legacyCommandRecord.legacyCommandByte[3] = m_legacyCommandRecord.legacyCommandByte[0];
              m_legacyCommandRecord.legacyCommandByte[4] = m_legacyCommandRecord.legacyCommandByte[1];
              m_legacyCommandRecord.legacyCommandByte[5] = (t_devNum << 7) + 0x12;  // 0x12 = digit 2 on keypad
              break;
            }

          case TMCC_DIALOGUE_CONDUCTOR_DEPARTURE:      // 1 byte: 2
          case TMCC_DIALOGUE_CONDUCTOR_TICKETS_DINER:  // 1 byte: 2
            {
              m_legacyCommandRecord.legacyCommandByte[2] = (t_devNum << 7) + 0x12;  // 0x12 = digit 2 on keypad
              break;
            }

          default:  // Bad news if we get here; it means none of the TMCC dialogues was what we passed
            {
              sprintf(lcdString, "ENG'R TMCC DLG ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
            }
        }  // End of switch "which of the six TMCC Dialogue commands are we decoding?"
      }  // End of "it's a TMCC Dialogue command"

    default:  // Uh oh, we didn't recognize the command
      {
        sprintf(lcdString, "ENG'R TMCC BAD CMD!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
      }

  }  // End of "which device command were we sent to decode?"

  // Okay, we have populated m_legacyCommandRecord[] with the appropriate TMCC command; either 3 or 6 bytes.
  sprintf(lcdString, "TCMD %c %i %i", t_devType, t_devNum, t_devParm1); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "%X %X %X", m_legacyCommandRecord.legacyCommandByte[0], m_legacyCommandRecord.legacyCommandByte[1],
          m_legacyCommandRecord.legacyCommandByte[2]); pLCD2004->println(lcdString); Serial.println(lcdString);
  return m_legacyCommandRecord;  // We're done with TMCC so exit this function

}

Engineer::legacyCommandStruct Engineer::translateToLegacy(byte t_devNum, byte t_devCommand, byte t_devParm1, byte t_devParm2) {
  // Rev: 05/24/24.
  // Populate a Legacy command buffer, 3- to 9-bytes long.
  // It's necessary to prefix this return type with "Engineer::" because it's a private struct.
  // Only gets called when t_devType = Legacy Engine or Train; other device types handled by other functions.
  // Translate a Delayed Action "plain English" command into a Legacy 3-, 6-, or 9-byte command.
  // We'll use our class global 9-byte struct variable m_legacyCommandRecord to hold our calculated Legacy command bytes.
  // Remember, bytes 1..9 are element 0..8!
  // The following Legacy Engine/Train commands (t_devCommand) are supported:
  //   * LEGACY_ACTION_STARTUP_SLOW
  //   * LEGACY_ACTION_STARTUP_FAST
  //   * LEGACY_ACTION_SHUTDOWN_SLOW
  //   * LEGACY_ACTION_SHUTDOWN_FAST
  //   * LEGACY_ACTION_SET_SMOKE      (9-byte command)
  //   * LEGACY_ACTION_EMERG_STOP
  //   * LEGACY_ACTION_ABS_SPEED (includes support for PowerMaster 91..94 on/off as speed 1/0.)
  //   * LEGACY_ACTION_MOMENTUM_OFF
  //   * LEGACY_ACTION_STOP_IMMED
  //   * LEGACY_ACTION_FORWARD
  //   * LEGACY_ACTION_REVERSE
  //   * LEGACY_ACTION_FRONT_COUPLER
  //   * LEGACY_ACTION_REAR_COUPLER
  //   * LEGACY_SOUND_HORN_NORMAL
  //   * LEGACY_SOUND_HORN_QUILLING  Requires intensity 0..15
  //   * LEGACY_SOUND_REFUEL   With minor dialogue.  Nothing on SP 1440.  Steam only??? *******************
  //   * LEGACY_SOUND_DIESEL_RPM Diesel only. Requires Parm 0..7
  //   * LEGACY_SOUND_WATER_INJECT  Steam only.
  //   * LEGACY_SOUND_ENGINE_LABOR  Diesel only?  Barely discernable on SP 1440. *******************
  //   * LEGACY_SOUND_BELL_OFF
  //   * LEGACY_SOUND_BELL_ON
  //   * LEGACY_SOUND_BRAKE_SQUEAL    Only works when moving.  Short chirp.
  //   * LEGACY_SOUND_AUGER           Steam only.
  //   * LEGACY_SOUND_AIR_RELEASE     Can barely hear.  Not sure if it works on steam as well as diesel? **************
  //   * LEGACY_SOUND_LONG_LETOFF     Diesel and (presumably) steam. Long hiss. *****************
  //   * LEGACY_SOUND_MSTR_VOL_UP     There are 10 possible volumes (9 excluding "off") i.e. 0..9.
  //   * LEGACY_SOUND_MSTR_VOL_DOWN   No way to go directly to a master vol level, so we must step up/down as desired.
  //   * LEGACY_SOUND_BLEND_VOL_UP    Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
  //   * LEGACY_SOUND_BLEND_VOL_DOWN  Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
  //   * LEGACY_SOUND_COCK_CLEAR_ON   Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
  //   * LEGACY_SOUND_COCK_CLEAR_OFF  Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
  //   * LEGACY_DIALOGUE              Separate function.  For any of 38 Legacy Dialogues, which will be passed as a parm.

  // First confirm that we have a valid device type and number...
  byte t_devType;  // Will be Legacy Engine or Train (E/T)
  if ((t_devNum >= LOCO_ID_POWERMASTER_1) && (t_devNum <= LOCO_ID_POWERMASTER_4)) {
    t_devType = DEV_TYPE_LEGACY_ENGINE;  // E
  } else {  // Anything other than a PowerMaster look up -- better be DEV_TYPE_LEGACY_ENGINE or DEV_TYPE_LEGACY_TRAIN.
    t_devType = m_pLoco->devType(t_devNum);  // E|T else bug in database
  }
  // We should only be sent commands for Legacy Engines or Trains so confirm this:
  if (outOfRangeLegacyDevType(t_devType)) {
    sprintf(lcdString, "ENG'R LEG DT ERR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
  }
  // Loco engine and train nums can only be 1..50 (and PowerMasters 91.94), but ignore t_devNum for Emergency Stop.
  if (t_devCommand != LEGACY_ACTION_EMERG_STOP) {
    if (outOfRangeDevNum(t_devType, t_devNum)) {
      sprintf(lcdString, "ENG'R LEG DN ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
    }
  }

  // Clear out all nine bytes...so we can always check byte 4 (element 3) for 0 to see if it's a 3-byte, and also
  // check byte 7 (element 6) for 0 to see if it's a 6-byte command, else it's a 9-byte command.
  for (byte j = 0; j < LEGACY_CMD_BYTES; j++) {  // Always 9 bytes, 0..8
    m_legacyCommandRecord.legacyCommandByte[j] = 0;
  }

  // We ALWAYS know what byte 0 will be, depending if it's a Legacy Engine vs Train, so we'll do that at the top:
  if (t_devType == DEV_TYPE_LEGACY_ENGINE) {  // Legacy Engine
    m_legacyCommandRecord.legacyCommandByte[0] = 0xF8;
  } else {  // Can only be Legacy Train
    m_legacyCommandRecord.legacyCommandByte[0] = 0xF9;
  }

  switch (t_devCommand) {

    // Let's do the 3-byte Legacy commands first.
    case LEGACY_ACTION_STARTUP_SLOW:   //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xFB;
        break;
      }

    case LEGACY_ACTION_STARTUP_FAST:   //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xFC;
        break;
      }

    case LEGACY_ACTION_SHUTDOWN_SLOW:  //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xFD;
        break;
      }

    case LEGACY_ACTION_SHUTDOWN_FAST:  //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xFE;
        break;
      }

    case LEGACY_ACTION_EMERG_STOP:     //  E-stop is actually F8/F9 + FF FF; we'll do System Halt instead turns off PowerMasters too!
      {
        m_legacyCommandRecord.legacyCommandByte[0] = 0xFE;
        m_legacyCommandRecord.legacyCommandByte[1] = 0xFF;
        m_legacyCommandRecord.legacyCommandByte[2] = 0xFF;
        break;
      }

    case LEGACY_ACTION_ABS_SPEED:      // Requires Parm1 0..199
      {
        // Speed can be 0..199 for Legacy
        if (outOfRangeLocoSpeed(t_devType, t_devParm1)) {
          sprintf(lcdString, "ENG'R LEG SPD ERR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
        }
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2);      // Shift loco num left one bit, add 0
        m_legacyCommandRecord.legacyCommandByte[2] = t_devParm1;          // Absolute speed value 0..199
        break;
      }

    case LEGACY_ACTION_MOMENTUM_OFF:   // We will never set Momentum to any value but zero.
      {
        byte t_momentum = 0;  // We can set momentum from 0..7 but in this case 0 = momentum OFF
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2);      // Shift left one bit, add 0
        m_legacyCommandRecord.legacyCommandByte[2] = 0xC8 + t_momentum;   // Parm1 better be 0..7
        break;
      }

    case LEGACY_ACTION_STOP_IMMED:     // Prefer over Abs Spd 0 b/c overrides momentum if instant stop desired.
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2);      // Shift left one bit, add 0
        m_legacyCommandRecord.legacyCommandByte[2] = 0xFB;
        break;
      }

    case LEGACY_ACTION_FORWARD:        // Delayed Action table command types
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x00;
        break;
      }

    case LEGACY_ACTION_REVERSE:        //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x03;
        break;
      }

    case LEGACY_ACTION_FRONT_COUPLER:  //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x05;
        break;
      }

    case LEGACY_ACTION_REAR_COUPLER:   //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x06;
        break;
      }

    case LEGACY_NUMERIC_PRESS:
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x10 + t_devParm1;   // Keypad digit 0..9
        break;
      }

    case LEGACY_SOUND_HORN_NORMAL:     //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x1C;
        break;
      }

    case LEGACY_SOUND_HORN_QUILLING:   // Requires intensity 0..15
      {
        if (outOfRangeQuill(t_devParm1)) {
          sprintf(lcdString, "ENG'R QUILL ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
        }
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xE0 + t_devParm1;   // Parm1 better be 0..15
        break;
      }

    case LEGACY_SOUND_REFUEL:          // With minor dialogue.  Nothing on SP 1440.  Steam only??? *******************
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x2D;
        break;
      }

    case LEGACY_SOUND_DIESEL_RPM:      // Diesel only. Requires Parm 0..7
      {
        if (outOfRangeRPM(t_devParm1)) {
          sprintf(lcdString, "ENG'R RPM ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
        }
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xA0 + t_devParm1;   // Parm1 better be 0..7
        break;
      }

    case LEGACY_SOUND_WATER_INJECT:    // Steam only.
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xA8;
        break;
      }

    case LEGACY_SOUND_ENGINE_LABOR:    // Diesel only?  Barely discernable on SP 1440. *******************
      {
        if (outOfRangeLabor(t_devParm1)) {
          sprintf(lcdString, "ENG LABOR ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
        }
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xC0 + t_devParm1;   // Parm1 better be 0..31
        break;
      }

    case LEGACY_SOUND_BELL_OFF:        //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xF4;
        break;
      }

    case LEGACY_SOUND_BELL_ON:         //
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xF5;
        break;
      }

    case LEGACY_SOUND_BRAKE_SQUEAL:    // Only works when moving.  Short chirp.
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xF6;
        break;
      }

    case LEGACY_SOUND_AUGER:           // Steam only.
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xF7;
        break;
      }

    case LEGACY_SOUND_AIR_RELEASE:     // Can barely hear.  Not sure if it works on steam as well as diesel? **************
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xF8;
        break;
      }

    case LEGACY_SOUND_LONG_LETOFF:     // Diesel and (presumably) steam. Long hiss. *****************
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0xFA;
        break;
      }

    // Now handle all of the 9-byte Legacy commands now...

    case LEGACY_ACTION_SET_SMOKE:      // 0x7C FX Control Trigger requires parm 0x00..0x03 (Off/Low/Med/Hi)
      {
        if (outOfRangeSmoke(t_devParm1)) {
          sprintf(lcdString, "ENG SMOKE ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
        }
        // Rev: 06/12/22
        // Set Legacy loco smoke level to OFF, LOW, MED, or HIGH
        // level: 0=Smoke OFF, 1=Smoke LOW, 2=Smoke MED, 3=Smoke HIGH
        // This is a Legacy 9-byte Effects Control 0x7C
        // *** WORD 1 of 3 ***  Already know m_legacyCommandRecord.legacyCommandByte[0] = 0xF8 or 0xF9
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift loco num left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x7C;  // Word 1 of 3, byte 3 = 0x7C (Effects Controls)
        // *** WORD 2 of 3 ***
        m_legacyCommandRecord.legacyCommandByte[3] = 0xFB;  // Legacy 2nd and 3rd set of 3-byte "words" ALWAYS start with FB
        if (t_devType == 'E') {
          m_legacyCommandRecord.legacyCommandByte[4] = (t_devNum * 2) + 0;  // Add 0 if we're addressing a Legacy Engine
        } else {
          m_legacyCommandRecord.legacyCommandByte[4] = (t_devNum * 2) + 1;  // Add 1 if we're addressing a Legacy Train
        }
        m_legacyCommandRecord.legacyCommandByte[5] = t_devParm1;  // Smoke off = 00, smoke Low/Med/High = 01/02/03
        // *** WORD 3 of 3 ***
        // For the third 3-byte word, the first two bytes of word 3 are always the same as the first two bytes of word 2
        m_legacyCommandRecord.legacyCommandByte[6] = m_legacyCommandRecord.legacyCommandByte[3];
        m_legacyCommandRecord.legacyCommandByte[7] = m_legacyCommandRecord.legacyCommandByte[4];
        // Last byte is a one's complement checksum of 5 previous bytes: starting at zero: 1, 2, 4, 5, and 7.
        m_legacyCommandRecord.legacyCommandByte[8] = legacyChecksum(m_legacyCommandRecord.legacyCommandByte[1], 
                                                                    m_legacyCommandRecord.legacyCommandByte[2],
                                                                    m_legacyCommandRecord.legacyCommandByte[4],
                                                                    m_legacyCommandRecord.legacyCommandByte[5],
                                                                    m_legacyCommandRecord.legacyCommandByte[7]);
        break;
      }

    // We'll handle all six of the 0x74 Railsounds FX Triggers in one block, as only 1 byte differs among them...
    case LEGACY_SOUND_MSTR_VOL_UP:     // 0x74 Railsounds FX Trigger There are 10 poss vols: 0..9.  Can't set specifically and couldn't tell what it affected (not main vol.)
    case LEGACY_SOUND_MSTR_VOL_DOWN:   // 0x74 Railsounds FX Trigger No way to go directly to a master vol level.
    case LEGACY_SOUND_BLEND_VOL_UP:    // 0x74 Railsounds FX Trigger.  Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
    case LEGACY_SOUND_BLEND_VOL_DOWN:  // 0x74 Railsounds FX Trigger.  Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
    case LEGACY_SOUND_COCK_CLEAR_ON:   // 0x74 Railsounds FX Trigger.  Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
    case LEGACY_SOUND_COCK_CLEAR_OFF:  // 0x74 Railsounds FX Trigger.  Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
      {
        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift loco num left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x74;  // Word 1 of 3, byte 3 = 0x74 (Railsounds FX Triggers)
        // *** WORD 2 of 3 ***
        m_legacyCommandRecord.legacyCommandByte[3] = 0xFB;  // Legacy 2nd and 3rd set of 3-byte "words" ALWAYS start with FB
        if (t_devType == 'E') {
          m_legacyCommandRecord.legacyCommandByte[4] = (t_devNum * 2) + 0;  // Add 0 if we're addressing a Legacy Engine
        } else {
          m_legacyCommandRecord.legacyCommandByte[4] = (t_devNum * 2) + 1;  // Add 1 if we're addressing a Legacy Train
        }
        m_legacyCommandRecord.legacyCommandByte[5] = t_devCommand;  // Command = Legacy hex value of command needed
        // *** WORD 3 of 3 ***
        // For the third 3-byte word, the first two bytes of word 3 are always the same as the first two bytes of word 2
        m_legacyCommandRecord.legacyCommandByte[6] = m_legacyCommandRecord.legacyCommandByte[3];
        m_legacyCommandRecord.legacyCommandByte[7] = m_legacyCommandRecord.legacyCommandByte[4];
        // Last byte is a one's complement checksum of 5 previous bytes: starting at zero: 1, 2, 4, 5, and 7.
        m_legacyCommandRecord.legacyCommandByte[8] = legacyChecksum(m_legacyCommandRecord.legacyCommandByte[1], 
                                                                    m_legacyCommandRecord.legacyCommandByte[2],
                                                                    m_legacyCommandRecord.legacyCommandByte[4],
                                                                    m_legacyCommandRecord.legacyCommandByte[5],
                                                                    m_legacyCommandRecord.legacyCommandByte[7]);
        break;
      }

    // We'll handle all 38 of the 0x72 Railsounds Dialogue Triggers in one block, as only 1 byte differs among them...
    case LEGACY_DIALOGUE:              // Separate function.  For any of the 38 Legacy Dialogues (9-byte 0x72), which will be passed as a parm.
      {
        // Rev: 06/12/22
        // Dialogue is specified in t_devParm1 as a const, but equals the hex value of the Legacy 0x72 dialogue code
        if (outOfRangeLegacyDialogueNum(t_devParm1)) {
          sprintf(lcdString, "ENG LEG DIALOG ERR!"); pLCD2004->println(lcdString); endWithFlashingLED(5);
        }

        m_legacyCommandRecord.legacyCommandByte[1] = (t_devNum * 2) + 1;  // Shift loco num left one bit, add 1
        m_legacyCommandRecord.legacyCommandByte[2] = 0x72;  // Word 1 of 3, byte 3 = 0x72 (Railsounds Dialogue Triggers)
        // *** WORD 2 of 3 ***
        m_legacyCommandRecord.legacyCommandByte[3] = 0xFB;  // Legacy 2nd and 3rd set of 3-byte "words" ALWAYS start with FB
        if (t_devType == 'E') {
          m_legacyCommandRecord.legacyCommandByte[4] = (t_devNum * 2) + 0;  // Add 0 if we're addressing a Legacy Engine
        } else {
          m_legacyCommandRecord.legacyCommandByte[4] = (t_devNum * 2) + 1;  // Add 1 if we're addressing a Legacy Train
        }
        m_legacyCommandRecord.legacyCommandByte[5] = t_devParm1;  // Dialogue number i.e. 0x0E is specified in parm1
        // *** WORD 3 of 3 ***
        // For the third 3-byte word, the first two bytes of word 3 are always the same as the first two bytes of word 2
        m_legacyCommandRecord.legacyCommandByte[6] = m_legacyCommandRecord.legacyCommandByte[3];
        m_legacyCommandRecord.legacyCommandByte[7] = m_legacyCommandRecord.legacyCommandByte[4];
        // Last byte is a one's complement checksum of 5 previous bytes: starting at zero: 1, 2, 4, 5, and 7.
        m_legacyCommandRecord.legacyCommandByte[8] = legacyChecksum(m_legacyCommandRecord.legacyCommandByte[1], 
                                                                    m_legacyCommandRecord.legacyCommandByte[2],
                                                                    m_legacyCommandRecord.legacyCommandByte[4],
                                                                    m_legacyCommandRecord.legacyCommandByte[5],
                                                                    m_legacyCommandRecord.legacyCommandByte[7]);
        break;
      }
    default:
      {
        // If we fall thorough to here, we have an unsupported command type
        sprintf(lcdString, "ENG'R LEG BAD CMD!"); pLCD2004->println(lcdString); Serial.println(lcdString);
      }
  }  // End of "switch" which Legacy command are we decoding?

  // Okay, we have populated m_legacyCommandRecord[] with the appropriate Legacy command; either 3, 6, or 9 bytes.
  sprintf(lcdString, "LCMD %c %i %i", t_devType, t_devNum, t_devParm1); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "%X %X %X", m_legacyCommandRecord.legacyCommandByte[0], m_legacyCommandRecord.legacyCommandByte[1],
    m_legacyCommandRecord.legacyCommandByte[2]); pLCD2004->println(lcdString); Serial.println(lcdString);
  return m_legacyCommandRecord;  // We're done with Legacy so exit this function
}

void Engineer::sendCommandToLegacyBase(legacyCommandStruct t_legacyCommand) {
  // Rev: 06/11/22.  Done but not tested.
  // The only tricky thing here is we need to check the 4th and 7th bytes to know if this is a 3, 6, or 9-byte command.
  // I.e. even though, with a 3-byte command, bytes 4-9 will be zero, we don't want to send zeroes to the Legacy base.
  // We'll also update our global time tracker m_legacyLastTransmit so we know when we last sent a command to the Legacy base.
  Serial3.write(t_legacyCommand.legacyCommandByte[0]);    // Byte 1
  Serial3.write(t_legacyCommand.legacyCommandByte[1]);    // Byte 2
  Serial3.write(t_legacyCommand.legacyCommandByte[2]);    // Byte 3
  sprintf(lcdString, "OUT: %X %X %X", t_legacyCommand.legacyCommandByte[0], t_legacyCommand.legacyCommandByte[1],
    t_legacyCommand.legacyCommandByte[2]); pLCD2004->println(lcdString); Serial.println(lcdString);
  if (t_legacyCommand.legacyCommandByte[3] != 0) {        // We have at least six bytes to write
    Serial3.write(t_legacyCommand.legacyCommandByte[3]);  // Byte 4
    Serial3.write(t_legacyCommand.legacyCommandByte[4]);  // Byte 5
    Serial3.write(t_legacyCommand.legacyCommandByte[5]);  // Byte 6
  }
  if (t_legacyCommand.legacyCommandByte[6] != 0) {        // We have all nine bytes to write
    Serial3.write(t_legacyCommand.legacyCommandByte[6]);  // Byte 7
    Serial3.write(t_legacyCommand.legacyCommandByte[7]);  // Byte 8
    Serial3.write(t_legacyCommand.legacyCommandByte[8]);  // Byte 9
  }
  // Since we just wrote to the Legacy base, update our "last transmit time" tracker...
  m_legacyLastTransmit = millis();
  return;
}

// ***************************************************************
// ***** LEGACY/TMCC COMMAND QUEUE CIRCULAR BUFFER FUNCTIONS *****
// ***************************************************************

// DESCRIPTION OF CIRCULAR BUFFERS Rev: 9/5/17.  See also OneNote "Circular Buffers 8/20/17."  Also used in SWT and LED.
// Each element is a nine-byte array that can store 3, 6, or 9 bytes of a Legacy or TMCC command.
// We use CLOCKWISE circular buffers, and track HEAD, TAIL, and COUNT.
// COUNT is needed for checking if the buffer is empty or full, and must be updated by both ENQUEUE and DEQUEUE.  The advantage of
// this method is that we can use all available elements of the buffer, and the math (modulo) is simpler than having just two
// variables, but we must maintain the third COUNT variable.
// Here is an example of a buffer showing storage of Data and Junk.
// HEAD always points to the UNUSED cell where the next new element will be inserted (enqueued), UNLESS the queue is full,
// in which case HEAD points to TAIL and new data cannot be added until the tail data is dequeued.

// TAIL always points to the last active element, UNLESS the queue is empty, in which case it points to garbage.
// Note that HEAD == TAIL *both* when the buffer is empty and when full, so we use COUNT as the test for full/empty status.
// COUNT is the total number of active elements, which can range from zero to the size of the buffer - 1.

// We must examine COUNT to determine whether the buffer is FULL or EMPTY.
// New elements are added at the HEAD, which then moves to the right.
// Old elements are read from the TAIL, which also then moves to the right.
// HEAD, TAIL, and COUNT are initialized to zero.

//    0     1     2     3     4     5
//  ----- ----- ----- ----- ----- -----
// |  D  |  D  |  J  |  J  |  D  |  D  |
//  ----- ----- ----- ----- ----- -----
//                ^           ^
//                HEAD        TAIL

// To enqueue an element, if array is not full, insert data at HEAD, then increment both HEAD and COUNT.
// To dequeue an element, if array is not empty, read data at TAIL, then increment TAIL and decrement COUNT.
// Always incrment HEAD and TAIL pointers using MODULO addition, based on the size of the array.

// IsEmpty = (COUNT == 0)
// IsFull  = (COUNT == SIZE)

void Engineer::commandBufEnqueue(const legacyCommandStruct t_legacyCommand) {
  // Rev: 06/16/22.  Tested overflow 11/12/22 and it works.
  // Insert a Legacy/TMCC hex record at the head of the Legacy command buffer, then increment head and count.
  // t_legacyCommand is a 9-byte struct to enqueue.  We have no knowledge of the meaning; it's just Legacy-language bytes.
  // If returns then it worked. If error such as buf overflow, that will be a fatal error; halt.
  if (!commandBufIsFull()) {
    for (byte j = 0; j < LEGACY_CMD_BYTES; j++) {  // Always 9 bytes, 0..8
      m_pLegacyCommandBuf[m_legacyCommandBufHead].legacyCommandByte[j] = t_legacyCommand.legacyCommandByte[j];
    }
    m_legacyCommandBufHead = (m_legacyCommandBufHead + 1) % LEGACY_CMD_HEAP_RECS;
    m_legacyCommandBufCount++;
  } else {
    sprintf(lcdString, "Command buf ovflow!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(3);
  }
  return;
}

bool Engineer::commandBufDequeue(legacyCommandStruct* t_legacyCommand) {
  // Rev: 06/16/22.
  // If it's been at least 30ms since the last command was sent to the Legacy base, then see if there is a new command waiting in
  // the Legacy command circular buffer.  If so, grab it and put it in our pointed-to legacyCommandStruct variable t_legacyCommand.
  // We aren't sending any data to the Legacy base; we just return a 9-byte command via pointer if a command is ready to send.
  // The data is just a 9-byte hex struct to dequeue.  We have no knowledge of the meaning as it's Legacy/TMCC-language bytes.
  // Returns true if we were able to get a new command, else false (not fatal.)
  if ((millis() - m_legacyLastTransmit) < LEGACY_CMD_DELAY) {
    return false;  // If we transmitted less than 30ms ago, nothing to do even if something might be in the command buffer
  }
  // Since it's been at least 30ms, see if we have a command in the Legacy Command buffer
  // Data will be at TAIL, then TAIL will be incremented (modulo size), and COUNT will be decremented.
  if (!commandBufIsEmpty()) {
    for (byte j = 0; j < LEGACY_CMD_BYTES; j++) {  // Always 9 bytes, 0..8
      // Passing the Legacy Command struct by pointer requires use of arrow operator here...
      t_legacyCommand->legacyCommandByte[j] = m_pLegacyCommandBuf[m_legacyCommandBufTail].legacyCommandByte[j];
    }
    m_legacyCommandBufTail = (m_legacyCommandBufTail + 1) % LEGACY_CMD_HEAP_RECS;
    m_legacyCommandBufCount--;
    return true;  // And t_legacyCommand will be returned to the caller via pointer
  } else {
    return false;  // Legacy command buffer is empty
  }
}

bool Engineer::commandBufIsEmpty() {
  return (m_legacyCommandBufCount == 0);
}

bool Engineer::commandBufIsFull() {
  return (m_legacyCommandBufCount == LEGACY_CMD_HEAP_RECS);
}

// ***************************************************************************
// ***** LEGACY COMMAND CHECKSUM and LEGACY/TMCC FUNCTION RANGE CHECKING *****
// ***************************************************************************

byte Engineer::legacyChecksum(byte leg1, byte leg2, byte leg4, byte leg5, byte leg7) {  // Using leg0..leg8 (not leg1..leg9)
  // 6/29/15: Calculate the "one's compliment" checksum required when sending "multi-word" commands to Legacy.
  // This function receives the five bytes from a 3-word Legacy command that are used in calculation of the checksum, and returns
  // the checksum.  Add up the five byte values, take the MOD256 remainder (which is automatically done by virtue of storing the
  // result of the addition in a byte-size variable,) and then take the One's Compliment which is simply inverting all of the bits.
  // We use the "~" operator to do a bitwise invert.
  return ~(leg1 + leg2 + leg4 + leg5 + leg7);
}

bool Engineer::outOfRangeTMCCDevType(char t_devType) {
  if ((t_devType != DEV_TYPE_TMCC_ENGINE) && (t_devType != DEV_TYPE_TMCC_TRAIN)) {
    return true;  // true means it's out of range
  } else {
    return false;
  }
}

bool Engineer::outOfRangeLegacyDevType(char t_devType) {
  if ((t_devType != DEV_TYPE_LEGACY_ENGINE) && (t_devType != DEV_TYPE_LEGACY_TRAIN)) {
    return true;  // true means it's out of range
  } else {
    return false;
  }
}

bool Engineer::outOfRangeDevNum(char t_devType, byte t_devNum) {
  if (t_devType == DEV_TYPE_ACCESSORY) {
    if ((t_devNum < 0) || (t_devNum > (TOTAL_LEG_ACCY_RELAYS - 1))) {
      return true;  // true means it's out of range
    }
  } else if (t_devType == DEV_TYPE_TMCC_TRAIN) {  // TMCC Train can be 1..9
    if ((t_devNum < 1) || (t_devNum > 9)) {
      return true;  // true means it's out of range
    }
  } else {  // For all other device types except Accessories and TMCC Trains i.e. TMCC Engines or Legacy Trains/Engines
    if (((t_devNum < 1) || (t_devNum > 50)) &&
       ((t_devNum < LOCO_ID_POWERMASTER_1) || (t_devNum > LOCO_ID_POWERMASTER_4))) {  // Anything else can be 1..50 or 91..94
      return true;  // true means it's out of range
    }
  }
  // If we fall through to here, we've found no objections.
  return false;  // false means it's not out of range
}

bool Engineer::outOfRangeTMCCDialogueNum(byte t_devParm1) {  // Must be 1..6
  // Rev: 06/12/22
  // TMCC Stationsounds Diner
  // Valid values are six consts defined arbitrarily as 1..6 (as consts)
  if ((t_devParm1 < 0) || (t_devParm1 > 6)) {
    return true;  // true means it's out of range
  } else {
    return false;  // false means it's NOT out of range
  }
}

bool Engineer::outOfRangeLocoSpeed(char t_devType, byte t_devParm1) {
  if ((t_devType == DEV_TYPE_TMCC_ENGINE) || (t_devType == DEV_TYPE_TMCC_TRAIN)) {  // TMCC 'N' Engine or 'R' Train can be 0..31
    if ((t_devParm1 < 0) || (t_devParm1 > 31)) {
      return true;  // true means it's out of range
    }
  }
  // Anything else can have speed 0..1999
  if ((t_devParm1 < 0) || (t_devParm1 > 199)) {  // Only 0..199 are valid
    return true;  // true means it's out of range
  } else {
    return false;
  }
}

bool Engineer::outOfRangeQuill(byte t_devParm1) {
  if ((t_devParm1 < 0) || (t_devParm1 > 15)) {  // Only 0..15 are valid
    return true;  // true means it's out of range
  } else {
    return false;
  }
}

bool Engineer::outOfRangeRPM(byte t_devParm1) {
  if ((t_devParm1 < 0) || (t_devParm1 > 7)) {  // Only 0..7 are valid
    return true;  // true means it's out of range
  } else {
    return false;
  }
}

bool Engineer::outOfRangeLabor(byte t_devParm1) {
  if ((t_devParm1 < 0) || (t_devParm1 > 31)) {  // Only 0..31 are valid
    return true;  // true means it's out of range
  } else {
    return false;
  }
}

bool Engineer::outOfRangeSmoke(byte t_devParm1) {
  if ((t_devParm1 < 0) || (t_devParm1 > 3)) {  // Only 0..3 are valid
    return true;  // true means it's out of range
  } else {
    return false;
  }
}

bool Engineer::outOfRangeLegacyDialogueNum(byte t_devParm1) {
  // Rev: 06/12/22
  // Legacy Railsounds Dialogue Triggers (0x72)
  // Only 6-20, 34-35, 48, 61-65, and 104-118 are valid
  if (((t_devParm1 >=   6) && (t_devParm1 <=  20)) ||
      ((t_devParm1 >=  34) && (t_devParm1 <=  35)) ||
      ((t_devParm1 >=  48) && (t_devParm1 <=  48)) ||
      ((t_devParm1 >=  61) && (t_devParm1 <=  65)) ||
      ((t_devParm1 >= 104) && (t_devParm1 <= 118))) {
    return false;  // false means it's NOT out of range
  } else {
    return true;  // true means it's out of range
  }
}

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

// LEGACY RAILSOUND DIALOGUE COMMAND *PARAMETERS* (specified as Parm1 in Delayed Action, by Conductor)
// ONLY VALID WHEN Device Command = LEGACY_DIALOGUE.
// Part of 9-byte 0x72 Legacy commands i.e. three 3-byte "words"
// Rev: 06/11/22
// So the main command will be LEGACY_DIALOGUE and Parm 1 will be one of the following.
// Note: The following could just be random bytes same as above, but eventually the Conductor or Engineer will need to know the
// hex value (0x##) to trigger each one -- so they might just as well be the same as the hex codes, right?
// const byte LEGACY_DIALOGUE_T2E_STARTUP                   = 0x06;  // Tower tells engineer to startup
// const byte LEGACY_DIALOGUE_E2T_DEPARTURE_NO              = 0x07;  // Engineer asks okay to depart, tower says no
// const byte LEGACY_DIALOGUE_E2T_DEPARTURE_YES             = 0x08;  // Engineer asks okay to depart, tower says yes
// const byte LEGACY_DIALOGUE_E2T_HAVE_DEPARTED             = 0x09;  // Engineer tells tower "here we come" ?
// const byte LEGACY_DIALOGUE_E2T_CLEAR_AHEAD_YES           = 0x0A;  // Engineer asks tower "Am I clear?", Yardmaster says yes
// const byte LEGACY_DIALOGUE_T2E_NON_EMERG_STOP            = 0x0B;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_RESTRICTED_SPEED          = 0x0C;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_SLOW_SPEED                = 0x0D;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_MEDIUM_SPEED              = 0x0E;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_LIMITED_SPEED             = 0x0F;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_NORMAL_SPEED              = 0x10;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_T2E_HIGH_BALL_SPEED           = 0x11;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_E2T_ARRIVING                  = 0x12;  // Arriving, E2T asks if lead is clear, tower says yes
// const byte LEGACY_DIALOGUE_E2T_HAVE_ARRIVED              = 0x13;  // E2T in the clear and ready; T2E set brakes
// const byte LEGACY_DIALOGUE_E2T_SHUTDOWN                  = 0x14;  // Dialogue used in long shutdown "goin' to beans"
// const byte LEGACY_DIALOGUE_T2E_STANDBY                   = 0x22;  // Tower says "please hold" i.e. after req by eng to proceed
// const byte LEGACY_DIALOGUE_T2E_TAKE_THE_LEAD             = 0x23;  // T2E okay to proceed; "take the lead", clear to pull
// const byte LEGACY_DIALOGUE_CONDUCTOR_ALL_ABOARD          = 0x30;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_FUEL_LEVEL     = 0x3D;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_FUEL_REFILLED  = 0x3E;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_SPEED          = 0x3F;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_WATER_LEVEL    = 0x40;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_WATER_REFILLED = 0x41;  // Not implemented on SP 1440
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_NEXT_STOP        = 0x68;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_WATCH_YOUR_STEP  = 0x69;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_ALL_ABOARD       = 0x6A;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_TICKETS_PLEASE   = 0x6B;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_CONDUCTOR_PREMATURE_STOP   = 0x6C;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_WELCOME_ABOARD     = 0x6D;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_FIRST_SEATING      = 0x6E;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_SECOND_SEATING     = 0x6F;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STEWARD_LOUNGE_CAR_OPEN    = 0x70;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_ARRIVING  = 0x71;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_ARRIVED   = 0x72;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_BOARDING  = 0x73;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_DEPARTED  = 0x74;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PASS_CAR_STARTUP   = 0x75;  // Don't have any Legacy StationSounds diners, so n/a for now.
// const byte LEGACY_DIALOGUE_SS_STATION_PASS_CAR_SHUTDOWN  = 0x76;  // Don't have any Legacy StationSounds diners, so n/a for now.

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
