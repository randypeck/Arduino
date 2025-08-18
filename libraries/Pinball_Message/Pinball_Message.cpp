// PINBALL_MESSAGE.CPP  Rev: 08/17/25

#include "Pinball_Message.h"

// NOTE REGARDING TIMING BETWEEN TRANSMIT AND RECEIVE: When BTN wanted to send a button press to MAS, MAS would send an "okay to
// transmit" message to BTN and then wait to receive the button-press message from BTN.  I had a slight delay caused by an LCD
// message right after MAS transmitted, and BTN replied so quickly that MAS didn't have a chance to transition from "send" to
// "receive" mode, so MAS did not receive the message.  So if I have problems with RS485 messages, check to see if there might be a
// timing problem.  Always insert a short delay between receipt of an RS-485 message and when I send one.

// NOTE REGARDING CHANGING FROM TRANSMIT TO/FROM RECEIVE MODE TOO QUICKLY (12/15/20):
// When transmitting an RS-485 message, we need to ensure that the output buffer is empty *before* we transition from transmit mode
// to receive mode, or else the transmission will be cut off.  This can be done by using Serial.flush() after sending a message,
// and before switching from send to receive mode.  Some versions of the Arduino core had a bug so that the last two bytes could be
// cut off even after waiting for flush() to return.  That seems to have been fixed, but be aware that an additional delay() may be
// needed.  Details here: https://forum.arduino.cc/index.php?topic=151014.0

// SERIAL has both a TX BUFFER *and* an RX BUFFER, both of which are 64 bytes long (63 bytes usable, apparently.)
// Need to constantly check that we are not overflowing, especially the input buffer.  If it ever gets close to being full,
//   need to display "SERIAL IN BUF FULL" on the LCD and trigger Emergency Stop, and update the logic.

// New info discovered 12/14/20: As of Arduino IDE 1.0, serial transmission is asynchronous. If there is enough empty space in the
// transmit buffer, Serial.write() will return before any characters are transmitted over serial. If the transmit buffer is full
// then Serial.write() will block until there is enough space in the buffer. To avoid blocking calls to Serial.write(), you can
// first check the amount of free space in the transmit buffer using availableForWrite().
// This does NOT mean that if I keep checking availableForWrite() that I won't overflow the recipient's incoming serial buffer!
// Even if Serial.Write() is blocking, it only cares about overflowing it's own, outgoing, serial buffer by feeding it too quickly.
// It doesn't know or care about how quickly the recipient is reading it's incoming buffer.

Pinball_Message::Pinball_Message() {  // Constructor
  return;
}

void Pinball_Message::begin(HardwareSerial * t_mySerial, long unsigned int t_myBaud) {
  // Rev: 12/06/20.
  m_mySerial = t_mySerial;      // Pointer to the serial port we want to use for RS485.
  m_myBaud = t_myBaud;          // RS485 serial port baud rate.
  m_mySerial->begin(m_myBaud);  // Initialize the RS485 serial port that this object will be using.
  memset(m_RS485Buf, 0, RS485_MAX_LEN);  // Fill the buffer (array) with zeroes.
  m_RS485Buf[RS485_LEN_OFFSET] = 0;      // Setting message len to zero just for fun.
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Put RS485 in receive mode (LOW)
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);
  sprintf(lcdString, "RS485 init ok!"); pLCD2004->println(lcdString);
}

char Pinball_Message::available() {
  // Rev: 03/07/23.
  // This function is called by a main module just to check what type, if any, message is waiting for it.
  // Returns char TYPE of "relevant" message waiting in RS485 incoming buffer (i.e. 'A'), else char = ' ' if no message.
  // IF A NON-BLANK CHAR IS RETURNED, CALLER MUST STILL CALL THE APPROPRIATE "GET" MESSAGE FUNCTION TO RETRIEVE THE MESSAGE!
  // i.e. pMessage->getUpdateScore()
  // Otherwise the contents of this message will be lost.
  // EXCEPTION: If the message contains no parms, such as "Set Tilt", then the recipient doesn't need to "get" anything else.

  while (getMessageRS485(m_RS485Buf) == true) {  // As long as we find a new incoming RS485 message, get it and check it.
    // OK, there *is* a message.
    return m_RS485Buf[RS485_TYPE_OFFSET];
  }
  // No message is waiting
  return(' ');
}

// *****************************************************************************************
// ************ P U B L I C   M O D U L E - S P E C I F I C   F U N C T I O N S ************
// *****************************************************************************************

// ALL incoming messages are retrieved by the calling modules by first calling pMessage->available().
// If a message is found in the incoming RS485 buffer, available() populates this class's m_RS485Buf[] buffer, and passes the
// message type back to the calling module i.e. MAS as the function's return value.
// If no message is found, the return char value is ' '.  Otherwise it is char message-type, such as 'G', 'C', etc.
// All of the following public message-get functions assume that pMessage->available() was called, and returned a non-blank message
// type, and that m_RS485Buf[] a valid message of that type.

void Pinball_Message::sendTurnOnGI() {
  int recLen = 3;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = 'G';    // Offset 1 = Type
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendRequestCredit() {
  int recLen = 3;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = 'C';    // Offset 1 = Type
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getCreditSuccess(bool* t_credits) {
  *t_credits = m_RS485Buf[2];
  return;
}

void Pinball_Message::sendStartNewGame() {
  int recLen = 3;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = 'N';    // Offset 1 = Type
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendTilt() {
  int recLen = 3;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = 'T';    // Offset 1 = Type
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendUpdateScore(const byte t_10K, const byte t_100K, const byte t_million) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = 'S';    // Offset 1 = Type
  m_RS485Buf[2] = t_10K;
  m_RS485Buf[3] = t_100K;
  m_RS485Buf[4] = t_million;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendRing10KBell() {
  int recLen = 3;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = '1';    // Offset 1 = Type
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendRing100KBell() {
  int recLen = 3;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = '2';    // Offset 1 = Type
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendRingSelectBell() {
  int recLen = 3;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = '3';    // Offset 1 = Type
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendCreditSuccess(const bool t_success) {
  int recLen = 4;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_TYPE_OFFSET] = 'C';    // Offset 1 = Type
  m_RS485Buf[2] = t_success;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getUpdateScore(byte* t_10K, byte* t_100K, byte* t_million) {
  *t_10K = m_RS485Buf[2];
  *t_100K = m_RS485Buf[3];
  *t_million = m_RS485Buf[4];
  return;
}

// HERE'S AN EXAMPLE OF PASSING A 2-BYTE INTEGER
/*
void Pinball_Message::sendMAStoALLRoute(const byte t_locoNum, const char t_extOrCont, const unsigned int t_routeRecNum,
                                const unsigned int t_countdown) {
  // Rev: 03/04/24.  NO IDEA IF THIS WILL WORK, NEEDS TESTING. **************************************************************************************************************
  // These messages will only be sent when running (or stopping) in Auto and Park modes; not during Registration or Manual modes.
  // Countdown in SECONDS (not ms) only applies to Extension (stopping) routes.
  int recLen = 11;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_ALL;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'R';  // Route number (1..n)
  m_RS485Buf[RS485_MAS_ALL_ROUTE_LOCO_NUM_OFFSET] = t_locoNum;
  m_RS485Buf[RS485_MAS_ALL_ROUTE_EXT_CONT_OFFSET] = t_extOrCont;  // E|C
  // I'm assigning a two-byte int to one byte of the buffer
// 3/3/23 MAYBE A PROBLEM - FIX THIS AS I'M NOW SENDING ROUTE RECORD NUMBER, NOT ROUTE NUMBER, MAY BE AN OFFSET-BY-1 ERROR ??? *************************************************************************************************************************
  // t_routeRecNum == Route number == FRAM record + 1
  m_RS485Buf[RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET]     = (t_routeRecNum >> 8) & 0xFF;
  m_RS485Buf[RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET + 1] = t_routeRecNum & 0xFF;
//  m_RS485Buf[RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET]        = t_routeRecNum / 256;
//  m_RS485Buf[RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET + 1]    = t_routeRecNum % 256;
  // Would be unusual to delay more than 256 seconds (4+ minutes) but we'll allow for it.
  m_RS485Buf[RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET]     = (t_countdown >> 8) & 0xFF;
  m_RS485Buf[RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET + 1] = t_countdown & 0xFF;
//  m_RS485Buf[RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET]     = t_countdown / 256;
//  m_RS485Buf[RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET + 1] = t_countdown % 256;

  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoALLRoute(byte* t_locoNum, char* t_extOrCont, unsigned int* t_routeRecNum, unsigned int* t_countdown) {
  // Rev: 03/04/24.  NO IDEA IF THIS WILL WORK, NEEDS TESTING. **************************************************************************************************************
  // Expects "real" locoNum 1..TOTAL_TRAINS.  Assume we wouldn't use this with locoNum == 0.
  *t_locoNum = m_RS485Buf[RS485_MAS_ALL_ROUTE_LOCO_NUM_OFFSET];
  if ((*t_locoNum < 1) || (*t_locoNum > TOTAL_TRAINS)) {
    sprintf(lcdString, "RS485 bad train no!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  *t_extOrCont = m_RS485Buf[RS485_MAS_ALL_ROUTE_EXT_CONT_OFFSET];  // E|C
  // **********************************************************************************************************************************************************************************************
  // I'm assigning one byte of the buffer to a two-byte field - I want two bytes from the buffer; how do I do this? *******************************************************************************
  // **********************************************************************************************************************************************************************************************
  *t_routeRecNum = (m_RS485Buf[RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET] << 8) | m_RS485Buf[RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET + 1];
  // Same as m_RS485Buf[] + m_RS485Buf[] * 256 ?
  *t_countdown = (m_RS485Buf[RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET] << 8) | m_RS485Buf[RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET + 1];
  return;
}

*/

// *****************************************************************************************
// *************************** P R I V A T E   F U N C T I O N S ***************************
// *****************************************************************************************

void Pinball_Message::sendMessageRS485(byte t_msg[]) {
  // Rev: 10-18-20.  Added delay between successive transmits to avoid overflowing modules' incoming serial buffer.
  // This routine must *only* be called when an entire message is ready to write, not a byte at a time.
  // This version, as part of the RS485 message class, automatically calculates and adds the CRC checksum.
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_TRANSMIT);  // Turn on transmit mode (set HIGH)
  byte tMsgLen = getLen(t_msg);
  if (tMsgLen > RS485_MAX_LEN) {
    sprintf(lcdString, "MSG OUT TOO LONG!"); pLCD2004->println(lcdString); endWithFlashingLED(5);  // Halts everything
  }
  t_msg[tMsgLen - 1] = calcChecksumCRC8(t_msg, tMsgLen - 1);  // Insert the checksum into the message
  // flush() doesn't mean that the recipient will be able to keep up with the amount of data we are sending i.e. to empty their
  // incoming buffer quickly enough.  About all we can do is use a timer to help limit sending subsequent outgoing messages
  // too quickly.  Here we will introduce an arbitrary delay to pause between RS485 transmissions.  If we find that a recipient
  // such as O_LEG is having their incoming buffer overflow, we can increase this delay -- potentially even based on recipient
  // and the type of message being transmitted.
  // This seems most likely to happen when LEG is receiving a long route, and also trying to populate and execute commands in the
  // Delayed Action table.  We should know if this happens because the receive function *should* detect a full input buffer and
  // do an emergency stop.  May not be a good permanent fix.  See header.
  while ((millis() - m_messageLastSentTime) < RS485_MESSAGE_DELAY_MS) {}  // Pause to help prevent receiver's in buffer overflow.
  while (m_mySerial->availableForWrite() < tMsgLen) {}  // Wait until there is enough room in the outgoing buffer for this message.
  m_mySerial->write(t_msg, tMsgLen);  
  // 12/15/20: In addition to waiting *before* we transmit, to avoid overflowing our own output buffer, and the recipient's input
  // buffer, we will now pause until the entire message has been transmitted.  If we eliminate the flush() command, then we will
  // likely transition the RS-485 chip from transmit mode to receive mode *before* our message has been fully transmitted!
  // See comments in header, and also https://forum.arduino.cc/index.php?topic=151014.0 for a discussion of this.
  m_mySerial->flush();  // REQUIRED: Wait for transmission of outgoing serial data to complete, about 0.1ms/byte @ 115Kb.
  // Note: I don't understand *how* I can overflow the outgoing serial buffer, even though I was able to make it happen in test
  // code, since (according to arduino.cc): "serial transmission is asynchronous. If there is enough empty space in the transmit
  // buffer, Serial.write() will return before any characters are transmitted over serial. If the transmit buffer is full then
  // Serial.write() will block until there is enough space in the buffer. To avoid blocking calls to Serial.write(), you can first
  // check the amount of free space in the transmit buffer using availableForWrite()."  Which I am now doing.
  // 12/15/20: CRITICALLY IMPORTANT: We must not have any delays here, such as displaying a message on the LCD.  This is because
  // when we transmit a request or permission to send (such as "okay, send me the button that was pressed"), the recipient is going
  // to broadcast their response *immediately* and we have to be sure that we are in receive mode, or it will be lost.
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Switch back to receive mode (set LOW)
  m_messageLastSentTime = millis();         // Keeps track of *when* this message was last sent.
  return;
}

bool Pinball_Message::getMessageRS485(byte* t_msg) {
  // Rev: 03/07/23.
  // getMessageRS485 returns true or false, depending if a complete message was read.
  // tmsg[] is also "returned" by the function (populated, iff there was a complete message) since arrays are passed by reference.
  // If this function returns true, then we are guaranteed to have a real/accurate message in the buffer, including good CRC.
  // However, this function does not check if it is to "us" (this Arduino) or not; that is handled elsewhere.
  // If the whole message is not available, t_msg[] will not be affected, so ok to call any time.
  // Does not require any data in the incoming RS485 serial buffer when it is called.
  // Detects incoming serial buffer overflow and fatal errors.
  // If there is a fatal error, calls endWithFlashingLED() (in calling module, so it can also invoke emergency stop if applicable.)
  // This only reads and returns one complete message at a time, regardless of how much more data may be in the incoming buffer.
  // Input byte t_msg[] is the initialized incoming byte array whose contents may be filled with a message by this function.
  byte bytesAvailableInBuffer = m_mySerial->available();  // How many bytes are waiting?  Size is 64.
  if (bytesAvailableInBuffer > 60) {  // RS485 serial input buffer should never get this close to 64-byte overflow.  Fatal!
    sprintf(lcdString, "RS485 in buf ovrflw!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  byte incomingMsgLen = m_mySerial->peek();  // First byte will be message length, or garbage is available = 0
  // bytesAvailableInBuffer must be greater than zero or there are no bytes in the incoming serial buffer.
  // bytesAvailableInBuffer must also be >= incomingMsgLen, or we don't have a complete message yet (we'll need to wait a moment.)
  if ((bytesAvailableInBuffer > 0) && (bytesAvailableInBuffer >= incomingMsgLen)) {
    // We have at least enough bytes for a complete incoming message!
    if (incomingMsgLen < 3) {  // Message too short to be a legit message.  Fatal!
      sprintf(lcdString, "RS485 msg too short!"); pLCD2004->println(lcdString); Serial.println(lcdString);
//      sprintf(lcdString, "Msg len = %3i", incomingMsgLen); pLCD2004->println(lcdString); Serial.println(lcdString);
//      sprintf(lcdString, "Avail = %3i", bytesAvailableInBuffer); pLCD2004->println(lcdString); Serial.println(lcdString);
      endWithFlashingLED(1);
    }
    else if (incomingMsgLen > RS485_MAX_LEN) {  // Message too long to be any real message.  Fatal!
      sprintf(lcdString, "RS485 msg too long!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    // So far, so good!  Now read the bytes of the message from the incoming serial buffer into the message buffer...
    for (byte i = 0; i < incomingMsgLen; i++) {  // Get the RS485 incoming bytes and put them in the t_msg[] byte array
      t_msg[i] = m_mySerial->read();
    }
    if (getChecksum(t_msg) != calcChecksumCRC8(t_msg, incomingMsgLen - 1)) {  // Bad checksum.  Fatal!
      sprintf(lcdString, "RS485 bad checksum!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    // At this point, we have a complete and legit message with good CRC, which may or may not be for us.
    return true;
  }
  else {  // We don't yet have an entire message in the incoming RS485 bufffer
    return false;
  }
}

void Pinball_Message::setLen(byte t_msg[], const byte t_len) {    // Inserts message length i.e. 7 into the appropriate byte
  t_msg[RS485_LEN_OFFSET] = t_len;
  return;
}

void Pinball_Message::setType(byte t_msg[], const char t_type) {  // Inserts the message type i.e. 'M'ode into the appropriate byte
  t_msg[RS485_TYPE_OFFSET] = t_type;
  return;
}

byte Pinball_Message::getLen(const byte t_msg[]) {   // Returns the 1-byte length of the RS485 message in tMsg[]
  return t_msg[RS485_LEN_OFFSET];
}

char Pinball_Message::getType(const byte t_msg[]) {  // Returns the 1-char message type i.e. 'M' for Mode
  return t_msg[RS485_TYPE_OFFSET];
}

byte Pinball_Message::getChecksum(const byte t_msg[]) {  // Just retrieves a byte from an existing message; does not calculate checksum!
  return t_msg[(getLen(t_msg) - 1)];
}

byte Pinball_Message::calcChecksumCRC8(const byte t_msg[], byte t_len) {  // Calculate checksum of an incoming or outgoing message.
  // Used for RS485 messages to return the CRC-8 checksum of all data fields except the checksum.
  // Sample call: msg[msgLen - 1] = calcChecksumCRC8(msg, msgLen - 1);
  // We will send (sizeof(msg) - 1) and make the LAST byte the CRC byte - so not calculated as part of itself ;-)
  byte crc = 0x00;
  while (t_len--) {
    byte extract = *t_msg++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}
