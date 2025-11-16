// PINBALL_MESSAGE.H  Rev: 08/17/25

// This class includes all of the low-level logic to send and receive messages on the RS-485 network
// FOR NOTES RE: RS485 INCOMING SERIAL BUFFER OVERFLOW "RS485 in buf ovflow." DISPLAYED ON LCD, see comments at bottom of code.

#ifndef PINBALL_MESSAGE_H
#define PINBALL_MESSAGE_H

#include <Pinball_Consts.h>
#include <Pinball_Functions.h>
#include <Pinball_LCD.h>
class Pinball_Message {

  public:

    Pinball_Message();  // Constructor

    void begin(HardwareSerial* t_mySerial, long unsigned int t_myBaud);

    char available();
    // Returns message type waiting in RS485 incoming buffer, else char = ' ' if there are no messages.
    // Caller MUST call the appropriate Message "get" function to retrieve the data, or the message will be lost.
    // Regardless of Mode, messaged may not be sent/received if State == STATE_STOPPED.

    // *** MESSAGES THAT MASTER CARES ABOUT ***
    void setGILamp(bool t_onOrOff);          // Msg type "G"  Later we can add requests for FX such as random dimming etc.
    void sendRequestCredit();                // Msg type "C"  Asks to deduct a credit if one is available (i.e. to start a game)
    void getCreditSuccess(bool* t_credits);  // Msg type "C"  Response from Slave: TRUE if there was a credit and it was subtracted; FALSE if no credits available
    void sendStartNewGame();                 // Msg type "N"  Tilt off, GI on, Score zero (does not deduct a credit)
    void sendTilt();                         // Msg type "T"  Turns on the tilt light; other lights off
    void sendUpdateScore(const byte t_10K, const byte t_100K, const byte t_million);  // Msg type "S"  Updates score to indicated value (not +/- current value)
    void sendRing10KBell();                  // Msg type "1"  Later we can request multiple dings and specify delay between dings in increments of 100ms
    void sendRing100KBell();                 // Msg type "2"
    void sendRingSelectBell();               // Msg type "3"

    // *** MESSAGES THAT SLAVE CARES ABOUT ***
    // NOTE: Messages that don't have any parameters don't need a "get" function; the Message Type is the message
    void sendCreditSuccess(const bool t_success);  // Sends TRUE if it was able to deduct a credit; else FALSE
    void getUpdateScore(byte* t_10K, byte* t_100K, byte* t_million);

  private:

    // ***** HERE ARE THE LOW-LEVEL CALLS THAT TALK TO THE RS485 NETWORK, AND ASSEMBLE AND DISASSEMBLE MESSAGES *****

    void sendMessageRS485(byte t_msg[]);     // t_msg[] *not* const because we populate CRC before sending.
    bool getMessageRS485(byte* t_msg);       // Returns true if a complete message was read.
    // tmsg[] is also "returned" (populated, iff there was a complete message) since arrays are passed by reference.

    void setLen(byte t_msg[], const byte t_len);    // Inserts message length i.e. 7 into the appropriate byte
    void setType(byte t_msg[], const char t_type);  // Inserts the message type i.e. 'M'ode into the appropriate byte

    byte getLen(const byte t_msg[]);                // Returns the 1-byte length of the RS485 message in tMsg[]
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

// NOTES REGARDING RS485 INCOMING SERIAL BUFFER OVERFLOW "RS485 in buf ovflow." DISPLAYED ON LCD:
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
