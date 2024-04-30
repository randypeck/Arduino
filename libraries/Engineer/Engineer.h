// ENGINEER.H Rev: 03/02/23. COMPLETE AND SEEMS TO WORK BUT NEEDS RIGOROUS TESTING.
// Part of O_LEG.
// 03/02/23: Complete and generally works but needs more rigorous testing, including Accessories.

// The Conductor sends commands to the Engineer via the Delayed Action Table, to be exectuted at specific times.
// When the Engineer sees that it's time to execute one of those commands, it passes the command to the Legacy Command Buffer which
// ensures that the commands aren't sent to the Lionel Legacy Base (and thence to the locos) faster than they can be accepted (the
// Legacy Base can only accept new commands every 30ms or so.)  From outside of this class, the Engineer will execute commands at
// nearly the instant the Conductor has told it to do so.
// The Engineer class requests pending/ripe Legacy/TMCC commands from Conductor, via the Delayed Action table, and sends those
// commands to the loco or other appropriate equipment (usually the locomotive, but also potentially StationSounds Diner cars,
// other operating rolling stock, TMCC-controlled accessories, and the Accessory Relay bank.)
// Delayed Action instructions are populated by the Conductor class, based on Routes received from MAS and also by some autonomous
// decision making about things like whistles, bells, loco sound effects, loco dialogue, StationSounds dialogue, etc.
// The Delayed Action class does the legwork of finding ripe records and expiring them as they are returned to the Engineer.
// Delayed Action records are considered "ripe" (by the Delayed Action class) based on the "time to execute" for that record, which
// must have passed (ideally as close to the current time as possible, but not before.)
// The Engineer asks for ripe commands, and when available, translates them into Legacy/TMCC 3/6/9-byte commands, and inserts them
// into the Legacy Command circular buffer in order to pace output with 30ms delays between commands sent out.

// Also, as ripe speed-change commands (ABS_SPEED, STOP_IMMED, EMERG_STOP) are read from Delayed Action and transferred into the
// Legacy Command Buffer (for almost-immediate execution,) getDelayedActionCommand() also keeps the loco's Train Progress "current
// speed & time" up to date (LEG only.)
// NOTE 3/5/23: isStopped is used primarily by MAS and thus needs to be updated based on tripping a Stop sensor, not by Engineer.

// Accessory (non-Legacy-related) commands control the Accessory Relay bank, used i.e. for lowering/raising crossing gates and
// other equipment that is affected by the location of the trains.  Accessory commands are executed immediately and are not
// impacted by Legacy/TMCC commands, so they don't reset the 30ms-minimum-between-successive-Legacy-commands countdown timer.

// At this time, the only valid TMCC commands will be "TMCC_DIALOGUE" type, and Parm1 must specify which of the 6 dialogues to say.
// We can add TMCC loco commands such as speed etc. easily enough, if we ever want to control a TMCC loco.

#ifndef ENGINEER_H
#define ENGINEER_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <Loco_Reference.h>  // So we can look up loco type i.e. E/T/N/R/A
#include <Train_Progress.h>  // Needed to update loco's latest speed/time as commands are sent to Legacy buffer.
#include <Delayed_Action.h>  // This is where we retrieve "ripe" commands to be sent to the Legacy command buffer/Legacy base.

class Engineer {

  public:

    Engineer();  // Constructor must be called above setup() so the object will be global to the module.

//    void begin(Centipede* t_pShift_Register, Loco_Reference* t_pLoco, Delayed_Action* t_pDelayedAction, Train_Progress* t_pTrainProgress);
    void begin(Loco_Reference* t_pLoco, Train_Progress* t_pTrainProgress, Delayed_Action* t_pDelayedAction);
    // We'll call Engineer::begin() when we initialize the Engineer object right after calling the constructor.
    // WE MUST ALSO CALL THIS FUNCTION each time Reg'n begins, to properly reset the Legacy Cmd Buf and release Accessory relays.
    // Since Engineer is also responsible for keeping Train_Progress::currentSpeed up to date, each loco's speed should also be set
    // to zero - but this is accomplished by LEG when Registration mode (re)starts via Train_Progress::resetTrainProgress().
    // One of the few classes where we don't pass the "FRAM* t_pStorage" parm or maintain the "FRAM* m_pStorage" local variable, as
    // this class doesn't need any access to FRAM (at least not as of this writing.)
    
    void legacyCommandBufInit();  // Always LEGACY_CMD_HEAP_RECS elements
    // (Re)init the whole m_pLegacyCommandBuf[] array.
    // Must be called whenever Registration mode (re)starts.

    void accessoryRelayInit();
    // Turn off all accessory relays.
    // Must be called whenever Registration mode (re)starts.

    void executeConductorCommand();
    // The Engineer can only do what the Conductor has commanded: Control the train and other equipment and accessories.
    // Calls both getDelayedActionCommand() and sendCommandToTrain(); both ultimately commands from Conductor to Engineer.
    // Should be called as frequently as possible, when running in Auto or Park mode.

  private:

    // ENGINEER Legacy/TMCC command structure.  This struct is only known inside the Engineer class.
    // Variables and heap array elements of this type will be used to store 3-, 6-, or 9-byte Legacy/TMCC commands.
    struct legacyCommandStruct {
      byte legacyCommandByte[LEGACY_CMD_BYTES];  // There will always be 9 elements, 0..8, though often only 0..2 will be used.
    };

    void getDelayedActionCommand();
    // This function must be called as frequently as possible (every time through loop) by Engineer to keep "ripe" commands that
    // are in the Delayed Action table moving out and into the Legacy Command Buffer (for loco commands) or accessory etc.
    // Retrieves ripe record (if any) from the Delayed Action table (human-readable version), then:
    //   a. If the record is an Accessory-control command, then activate or deactivate a relay
    //   b. If the record is a Legacy (Engine/Train) command, then:
    //      1) Translate from human-readable to actual 3-, 6-, or 9-byte Legacy/TMCC formatted command, then
    //      2) Enqueue it in the Legacy Command Buffer, then
    //      3) If it is a speed command (Abs Speed or Stop Immed), update legacySpeedAndTime heap array fields.
    // Even though we must still process a speed command via sendCommandToTrain() before the train will actually be moving at that
    // speed, we'll assume this will happen almost immediately and thus current speed and time set at this point will be accurate.
    // Especially since once a command has been sent to the Legacy command buffer, it will always be executed (i.e. unlike Delayed
    // Action records which can be removed before being executed such as if we trip a Stop sensor before fully slowing the train.)

    void sendCommandToTrain();
    // This function must be called as frequently as possible (every time through loop) by Engineer to keep loco commands that
    // are waiting in the Legacy Command Buffer streaming out via the Legacy Base to the locos.
    // Dequeues a 9-byte record (if any) from the Legacy Command Buffer (not more often than each 30ms).
    // At this point, we're just feeding a 3/6/9-byte command to Legacy as quickly as possible, with no knowledge of what the
    // command is or does, except to know that it's a Legacy (or TMCC) command, and not i.e. an Accessory command.
    // If a record is found, forward to the Legacy Command Base via the RS-232 interface.
    // We'll use our class global 9-byte struct variable m_legacyCommandRecord to hold our working command.

    void controlAccessoryRelay(byte t_RelayNum, byte t_CloseOrOpen);
    // t_RelayNum will be 0...n
    // t_CloseOrOpen will be 0=Open, 1=Close

    legacyCommandStruct translateToTMCC(byte t_devNum, byte t_devCommand, byte t_devParm1, byte t_devParm2);
    legacyCommandStruct translateToLegacy(byte t_devNum, byte t_devCommand, byte t_devParm1, byte t_devParm2);
    // Translate a Delayed Action "generic" command into a Legacy or TMCC 3-, 6-, or 9-byte command and throw it in the queue.

    bool commandBufIsEmpty();

    bool commandBufIsFull();

    void commandBufEnqueue(legacyCommandStruct t_legacyCommand);
    // Insert a 3-, 6-, or 9-byte Legacy/TMCC command into the circular Legacy buffer.
    // If returns then it worked. If error such as buf overflow, that will be a fatal error; halt.
    // Called by Engineer::getDelayedActionCommand().

    bool commandBufDequeue(legacyCommandStruct* t_legacyCommand);
    // Attempt to retrieve a command from the Command Buffer if conditions are met (i.e. 30ms has passed since last retrieval.)
    // Returns true if we were able to get a new command.
    // Called by Engineer::sendCommandToTrain().

    void sendCommandToLegacyBase(legacyCommandStruct t_legacyCommand);
    // Send a command to the Legacy base.
    // Called by Engineer::sendCommandToTrain().

    byte legacyChecksum(byte leg1, byte leg2, byte leg4, byte leg5, byte leg7);
    bool outOfRangeTMCCDevType(char t_devType);  // N/R = TMCC eNgine or tRain (not turnout Normal/Reverse)
    bool outOfRangeLegacyDevType(char t_devType);  // E/T = Legacy Engine or Train
    bool outOfRangeDevNum(char t_devType, byte t_devNum);  // Can be 1..9 or 1..50 depending on TMCC Train vs other
    bool outOfRangeTMCCDialogueNum(byte t_devParm1);
    bool outOfRangeLocoSpeed(char t_devType, byte t_devParm1);
    bool outOfRangeQuill(byte t_devParm1);
    bool outOfRangeRPM(byte t_devParm1);
    bool outOfRangeLabor(byte t_devParm1);
    bool outOfRangeSmoke(byte t_devParm1);
    bool outOfRangeLegacyDialogueNum(byte t_devParm1);

    // *** LEGACY COMMAND BUFFER ***
    // We'll use a circular buf to store incoming Delayed Action records to be queued for the Legacy base.
    // Create a REGULAR (non-pointer) variable so I can pass a single 9-byte Legacy/TMCC command between functions.
    legacyCommandStruct  m_legacyCommandRecord;
    // Create a POINTER variable to the entire Command Buffer struct heap array.
    // Can't point at anything yet; constructor will do that.
    legacyCommandStruct* m_pLegacyCommandBuf;
    // Circular buffer head/tail/count must be ints, not bytes, so we can expand size to > 255 elements if needed.
    unsigned int         m_legacyCommandBufHead  = 0;  // Next array element to be written.
    unsigned int         m_legacyCommandBufTail  = 0;  // Next array element to be removed.
    unsigned int         m_legacyCommandBufCount = 0;  // Number active elements in buffer.  Max is LEGACY_CMD_HEAP_RECS.
    unsigned long        m_legacyLastTransmit;         // Class global variable that updates each time we actually send a command (ms) 

    Loco_Reference* m_pLoco;
    Delayed_Action* m_pDelayedAction;   // Pointer to the Delayed Action class so we can call its functions.
    Train_Progress* m_pTrainProgress;   // Pointer to the Train Progress class so we can update loco current speed/time.

};

#endif
