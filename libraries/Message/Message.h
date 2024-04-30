// MESSAGE.H Rev: 02/20/24.  COMPLETE AND READY FOR TESTING *****************************************************************************************************************
// 2/20/24 Added Debug on/off prompt:
//   MAS-to-LEG Registration Debug On|Off.
// 3/9/23 Add Audio on/off prompt:
//   MAS-to-LEG Registration Audio On|Off.  Sent as soon as it's been received from OCC.

// This class includes all of the low-level logic to send and receive messages on the RS-485 network, including checking the
// digital request pins.  It also knows about every message used by every module, based on THIS_MODULE.
// We could have the RS-485 functions such as setLen, getMessageRS485, etc. in a parent class and the higher-level functions in a
// child class, but it just seems simpler to keep them all rolled into this one class, at least for the time being.
// FOR NOTES RE: RS485 INCOMING SERIAL BUFFER OVERFLOW "RS485 in buf ovflow." DISPLAYED ON LCD, see comments at bottom of code.

#ifndef MESSAGE_H
#define MESSAGE_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>


class Message {

  public:

    Message();  // Constructor

    void begin(HardwareSerial* t_mySerial, long unsigned int t_myBaud);

    char available();
    // Returns message type waiting in RS485 incoming buffer, else char = ' ' if there are no messages.
    // Caller MUST call the appropriate Message "get" function to retrieve the data, or the message will be lost.
    // Regardless of Mode, messaged may not be sent/received if State == STATE_STOPPED.

    // *** MODE/STATE CHANGE MESSAGES ***
    void sendMAStoALLModeState(const byte t_mode, const byte t_state);
    void getMAStoALLModeState(byte* t_mode, byte* t_state);

    // *** LOCO SETUP AND LOCATION MESSAGES (REGISTRATION MODE ONLY) ***
    // Registration complete when sendOCCtoALLTrainLocations t_locoNum == 0.
    void sendOCCtoLEGFastOrSlow(const char t_fastOrSlow);
    void  getOCCtoLEGFastOrSlow(char* t_fastOrSlow);
    void sendOCCtoLEGSmokeOn(const char t_smokeOrNoSmoke);
    void  getOCCtoLEGSmokeOn(char* t_smokeOrNoSmoke);
    void sendOCCtoLEGAudioOn(const char t_audioOrNoAudio);
    void  getOCCtoLEGAudioOn(char* t_audioOrNoAudio);
    void sendOCCtoLEGDebugOn(const char t_debugOrNoDebug);
    void  getOCCtoLEGDebugOn(char* t_debugOrNoDebug);
    void sendOCCtoALLTrainLocation(const byte t_locoNum, const routeElement t_locoBlock);

    void  getOCCtoALLTrainLocation(byte* t_locoNum, routeElement* t_locoBlock);

    // *** ROUTE MESSAGES (AUTO/PARK MODE ONLY) ***
    // There are no Route messages sent during Registration; the initial Train Progress record is based on each loco's location.
    // Countdown in seconds only applies to Extension (when train is moving) routes, not Continuation (train stopped.)
    void sendMAStoALLRoute(const byte t_locoNum, const char t_extOrCont, const unsigned int t_routeRecNum, 
                           const unsigned int t_countdown);
    void  getMAStoALLRoute(byte* t_locoNum, char* t_extOrCont, unsigned int* t_routeRecNum, unsigned int* t_countdown);

    // *** BUTTON PRESS MESSAGES (MANUAL MODE ONLY) ***
    void sendMAStoBTNRequestButton();  // MAS to BTN: Okay to send me a button press on the RS485 bus.
    void  getMAStoBTNRequestButton();  // Simply receiving the msg via "available()" is all BTN needs to know; function not needed.
    void sendBTNtoMASButton(const byte t_buttonNum);
    void  getBTNtoMASButton(byte* t_buttonNum);

    // *** TURNOUT MESSAGES (ANY MODE EXCEPT REGISTRATION) ***
    void sendMAStoALLTurnout(const byte t_turnoutNum, const char t_turnoutDir);
    void  getMAStoALLTurnout(byte* t_turnoutNum, char* t_turnoutDir);  // Set 'T'urnout.  Includes num & Normal|Reverse

    // *** SENSOR MESSAGES COULD BE USED IN ANY MODE ***
    // Sensor status may be requested by MAS in any mode, but SNS may not request to send (via digital line) in Registration mode.
    void sendMAStoSNSRequestSensor(const byte t_sensorNum);  // MAS to SNS: Send status of sensor n, or if n=0, ok to xmit update.
    void  getMAStoSNSRequestSensor(byte* t_sensorNum);
    void sendSNStoALLSensorStatus(const byte t_sensorNum, const char t_sensorStatus);
    void  getSNStoALLSensorStatus(byte* t_sensorNum, char* t_sensorStatus);

  private:

    // ***** HERE ARE THE LOW-LEVEL CALLS THAT TALK TO THE RS485 NETWORK, AND ASSEMBLE AND DISASSEMBLE MESSAGES *****

    bool forThisModule(const byte t_msg[]);  // Returns true if a message is for the calling module i.e. MAS, OCC, etc.
    void sendMessageRS485(byte t_msg[]);     // t_msg[] *not* const because we populate CRC before sending.
    bool getMessageRS485(byte* t_msg);       // Returns true if a complete message was read.
    // tmsg[] is also "returned" (populated, iff there was a complete message) since arrays are passed by reference.

    void setLen(byte t_msg[], const byte t_len);    // Inserts message length i.e. 7 into the appropriate byte
    void setFrom(byte t_msg[], const byte t_from);  // Inserts "from" i.e. ARDUINO_MAS into the appropriate byte
    void setTo(byte t_msg[], const byte t_to);      // Inserts "to" i.e. ARDUINO_BTN into the appropriate byte
    void setType(byte t_msg[], const char t_type);  // Inserts the message type i.e. 'M'ode into the appropriate byte

    byte getLen(const byte t_msg[]);                // Returns the 1-byte length of the RS485 message in tMsg[]
    byte getFrom(const byte t_msg[]);               // Returns the 1-byte "from" Arduino ID
    byte getTo(const byte t_msg[]);                 // Returns the 1-byte "to" Arduino ID
    char getType(const byte t_msg[]);               // Returns the 1-char message type i.e. 'M' for Mode

    byte getChecksum(const byte t_msg[]);  // Just retrieves a byte from an existing message; does not calculate checksum!
    byte calcChecksumCRC8(const byte t_msg[], const byte t_len);  // Calculate checksum of an incoming or outgoing message.

    HardwareSerial* m_mySerial;      // Pointer to the hardware serial port that we want to use.
    long unsigned int m_myBaud;      // Baud rate for serial port i.e. 9600 or 115200

    // m_RS485Buf[] is our 20-byte PRIVATE array used to hold incoming and outgoing RS485 messages, when assembling and
    // disassembling.  Note that this buffer is *not* sent to/from the calling module; it is for internal use by Message.
    // Assuming Message is instantiated on the heap (with "new") in the calling module, this buffer will also be on the heap.
    byte m_RS485Buf[RS485_MAX_LEN];  // May eventually need separate buffer for in vs out, but for now we'll use just one

    // Variables to delay between successive message sends, to help avoid recipients incoming serial buffer overflow.
    // Keep them together here since they're related (i.e. don't move RS485_MESSAGE_DELAY_MS to Train_Consts_Global.h)
          unsigned long m_messageLastSentTime  = 0;  // Keeps track of *when* a message was last sent
    const unsigned long RS485_MESSAGE_DELAY_MS = 2;  // How long should we wait between RS485 transmissions?

};

#endif

// 03/07/23: Many changes to message formats especially how MAS and OCC communicate during Registration.
// 08/19/22: Added "ms from now to start train moving" parm to sendMAStoALLRoute() and getMAStoAllRoute().
// 08/16/22: Changed sendMAStoALLRoute and getMAStoALLRoute to send Route Number versus one Route Element at a time.
// 07/07/22: Eliminated locoNum parm from SNStoALLSensorStatus, since we don't need SNS to maintain a Train Progress table and track that.
// 02/19/21: Added locoNum as a parm to SNStoALLSensorStatus functions.  1..TOTAL_TRAINS, or zero if not applicable.
// 12/15/20: Eliminated delay caused by LCD after MAS transmitted "okay to send" to BTN, before it switched to receive mode.  This
// caused MAS to not receive the button-press message that BTN sent.  See notes in Message.cpp header.
// 12/12/20: Renamed class from MessageClass to just plain Message.
// 12/07/20: Removed static attribute of RS485 message buffer; just not needed.

// Deprecated functions as of 3/7/23:
//   void sendMAStoOCCQuestion(const char t_question[], const char t_lastQuestion);
//   void getMAStoOCCQuestion(char* t_question, char* t_lastQuestion);  // t_question is an array
//   void sendOCCtoMASAnswer(const byte t_replyNum);
//   void getOCCtoMASAnswer(byte* t_replyNum);
//   void sendMAStoOCCTrainNames(const byte t_locoNum, const char t_trainName[], const byte t_dfltBlk, const char t_last);  // t_last = Y|N for last record
//   void getMAStoOCCTrainNames(byte* t_locoNum, char* t_trainName, byte* t_dfltBlk, char* t_last); // t_trainName is an array
//   *** PLAY AUDIO MESSAGES ONLY USED IN AUTO (AND MAYBE PARK) MODE ***
//   03/07/23: We're not including this code at this time, as OCC is better off making decisions on what/when to play audio.
//   void sendMAStoOCCPlay(const byte t_stationNum, const char t_phrase[]);
//   void getMAStoOCCPlay(byte* t_stationNum, char* t_phrase);  // t_phrase is an array

// NOTES REGARDING RS485 INCOMING SERIAL BUFFER OVERFLOW "RS485 in buf ovflow." DISPLAYED ON LCD:  See comments at bottom of code.
// 03/07/23: If RS-485 buffer overflow becomes a problem, could use an output buffer similar to Legacy Command Buffer to dole out
// data at no-more-than some maximum speed (though it's more a question of the number of bytes in a given amount of time.)
// Some of the following comments are moot because we no longer send Routes piecemeal; now we just send the Route record number.
// If we send too many RS485 messages too quickly, we will overflow the receiving module's serial input buffers.
// For example, before re-writing LED, the loop to paint the control panel took about 50ms, and sending a lot of turnout commands
// while the control panel was being painted was overflowing the incoming RS485 serial buffer.  Speeding up that function to from
// 50ms to 2ms fixed the problem, but other modules may need more time to complete their work in loop(), especially LEG.
// Sending too many messages too quickly will show up on the affected module's LCD display as "RS485 in buf ovflow."
// Note that this can occur even if the module receiving the messages discards them -- they still need to be read first.
// ONE FACTOR TO BEAR IN MIND IS THAT SENDING MESSAGES TO SERIAL AND LCD2004 IN LOOP() SLOWS THE LOOP DOWN A LOT!
// When the incoming serial buffer overflows (such as in LEG,) we'll need to either figure a way for receiving modules to process
// the incoming messages more quickly (getting them out of the serial buffer and perhaps into an internal circular buffer) *or*
// increase the RS485_MESSAGE_DELAY_MS to some larger value, *or* add new logic that considers how many bytes are being sent in a
// short time, and insert a delay only in that condition, *or* have MAS require an ACK from one of the recipients of each message.
// Since MAS is the only module that would potentially send a large number of bytes in rapid succession, such as a Route,
// we could even add logic to MAS when it broadcast a new Route *iff* it was more than 60 bytes, to impose a delay itself.
// The same could happen when MAS sends out Turnout commands at the beginning of a session, to set everything to last-known.  A
// slew of Turnout messages would be 7 bytes times 30 turnouts = 210 bytes.
// Another point is MAS that could send a large number of bytes quickly would be during Registration, when it sends a list of train
// 'N'ames to LEG.  Those records are 15 bytes each, and for 8 trains that would be 120 bytes.  So maybe that bit of code needs to
// have a custom delay() inserted.
// Similarly, MAS could send a large number of bytes quickly during Registration, when it sends the list of all occupied blocks to
// LEG and OCC.  Those messages are only 8 bytes each, but there could be a lot of them -- theoretically as many as 26 (1/block.)
// And time is not critical during Registration, so no problem, and no need to impose that delay on all other outgoing messages.
// Note that we would not likely see this when MAS was requesting the status of every sensor from SNS, at startup.  The total bytes
// transmitted would be 6 bytes per message times 62 sensors = 372 bytes!  However SNS only transmits each message upon request by
// MAS, so MAS would be transmitting requests between each receive and thus would not itself overflow.  HOWEVER, some other
// module could overflow, because it's having to process (or disregard) *all* of the messages coming from both MAS and SNS.
// A more general way to handle this would be to have Message() keep track of how many bytes have been sent out within the last n
// milliseconds.  So if we spew out some number of bytes very quickly, and the next message would put that number over 60 (the size
// of each module's incoming RS485 serial buffer), *then* we will impose a brief delay before we continue.  This is a great
// generalized solution to build into pMessage->sendMessageRS485(), but it will require some extra logic to keep track of bytes.
// FUTURE OPTION: Send not more than 60 bytes every n milliseconds, and insert some delay before continuing.

// It makes sense to have RS485_MESSAGE_DELAY_MS be a "per message" delay, rather than a "per byte" delay, because the
// incoming RS485 serial buffer errors that we're seeing in LED, for example, are caused by the receiving module (i.e. LED) being
// too slow getting through it's loop().  Once it gets back to the top to receive a message, it can keep up -- but while it's in
// loop() it's possible that too many bytes will come in while it is away.  So just adding a slight delay between a flood of
// incoming messages might make the difference.
// My guess is that LEG (followed by OCC) has the most work to do between checking for new messages, so a very long route seems
// most likely to cause an RS485 incoming serial buffer overflow in that module.  Time will tell.
// We can slow down outgoing messages even more by increasing the value of the const RS485_MESSAGE_DELAY_MS (in Message.h)
// But that basically makes us insert a delay before *every* message that allows the slowest module (probably LEG) get through
// it's loop().  Very inefficient.
// Anyway, we can't have delays in any module's main loop() unless we know that no RS485 messages will be coming in at that time.
