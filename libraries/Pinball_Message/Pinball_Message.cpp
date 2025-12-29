// PINBALL_MESSAGE.CPP  Rev: 12/11/25

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

void Pinball_Message::begin(HardwareSerial* t_mySerial, long unsigned int t_myBaud) {
  // Rev: 11/01/25.  Confirm pLCD2004 isn't null before using it.
  m_mySerial = t_mySerial;      // Pointer to the serial port we want to use for RS485.
  m_myBaud = t_myBaud;          // RS485 serial port baud rate.
  m_mySerial->begin(m_myBaud);  // Initialize the RS485 serial port that this object will be using.
  memset(m_RS485Buf, 0, RS485_MAX_LEN);  // Fill the buffer (array) with zeroes.
  m_RS485Buf[RS485_LEN_OFFSET] = 0;      // Setting message len to zero just for fun.
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Put RS485 in receive mode (LOW)
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);
  if (pLCD2004) {
    sprintf(lcdString, "RS485 init ok!"); pLCD2004->println(lcdString);
  }
  return;
}

byte Pinball_Message::available() {
  // Rev: 12/11/25.
  // This function is called by a main module just to check what type, if any, message is waiting for it.
  // Returns byte TYPE of "relevant" message waiting in RS485 incoming buffer, else RS485_TYPE_NO_MESSAGE if no message.
  // Can also return error code if message was malformed.
  // IF A MESSAGE TYPE IS RETURNED, CALLER MUST STILL CALL THE APPROPRIATE "GET" MESSAGE FUNCTION TO RETRIEVE THE MESSAGE!
  // i.e. pMessage->getUpdateScore()
  // Otherwise the contents of this message will be lost.
  // EXCEPTION: If the message contains no parms, such as "Set Tilt", then the recipient doesn't need to "get" anything else.
  // Because we only have two Arduinos (versus seven with the Trains system,) we can assume that any message found is for us.
  if (getMessageRS485(m_RS485Buf) == true) {  // OK, there *is* a message (and it's not an error)
    return m_RS485Buf[RS485_TYPE_OFFSET];
  }
  // But there still could be an error condition flagged by getMessageRS485()
  if (m_lastError != 0) {
    byte err = m_lastError;  // Save error code
    m_lastError = 0;        // Clear error code for next time
    return err;             // Return error code to caller
  }
  // Else no error and no message is waiting
  return RS485_TYPE_NO_MESSAGE;
}

// *****************************************************************************************
// ************ P U B L I C   M O D U L E - S P E C I F I C   F U N C T I O N S ************
// *****************************************************************************************
// ALL incoming messages are retrieved by the calling modules by first calling pMessage->available().
// If a message is found in the incoming RS485 buffer, available() populates this class's m_RS485Buf[] buffer, and passes the
// message type back to the calling module i.e. MAS as the function's return value.
// If no message is found, the return value is RS485_TYPE_NO_MESSAGE; otherwise it returns the byte-value message type.
// All of the following public message-get functions assume that pMessage->available() was called, and returned a non-blank message
// type, and that m_RS485Buf[] a valid message of that type.

void Pinball_Message::sendMAStoSLVMode(const byte t_mode) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'M', mode, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_MODE;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = t_mode;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoSLVMode(byte* t_mode) {
  *t_mode = m_RS485Buf[RS845_PAYLOAD_OFFSET];
  return;
}

void Pinball_Message::sendMAStoSLVCommandReset() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_COMMAND_RESET;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendMAStoSLVGILamp(const bool t_onOrOff) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_GI_LAMP;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = t_onOrOff ? 1 : 0;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoSLVGILamp(bool* t_onOrOff) {
  *t_onOrOff = (m_RS485Buf[RS845_PAYLOAD_OFFSET] != 0);
  return;
}

void Pinball_Message::sendMAStoSLVTiltLamp(const bool t_onOrOff) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_TILT_LAMP;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = t_onOrOff ? 1 : 0;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoSLVTiltLamp(bool* t_onOrOff) {
  *t_onOrOff = (m_RS485Buf[RS845_PAYLOAD_OFFSET] != 0);
  return;
}

void Pinball_Message::sendMAStoSLVBell10K() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_BELL_10K;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendMAStoSLVBell100K() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_BELL_100K;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendMAStoSLVBellSelect() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_BELL_SELECT;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendMAStoSLV10KUnitPulse() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_10K_UNIT;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}


void Pinball_Message::sendMAStoSLVCreditStatusQuery() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Offset 0 = Length
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_CREDIT_STATUS;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendSLVtoMASCreditStatus(const bool t_creditsAvailable) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_SLV_TO_MAS_CREDIT_STATUS;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = t_creditsAvailable ? 1 : 0;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getSLVtoMASCreditStatus(bool* t_creditsAvailable) {
  *t_creditsAvailable = (m_RS485Buf[RS845_PAYLOAD_OFFSET] != 0);
  return;
}

void Pinball_Message::sendMAStoSLVCreditFullQuery() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_CREDIT_FULL_QUERY;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendSLVtoMASCreditFullStatus(const bool t_creditsFull) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_SLV_TO_MAS_CREDIT_FULL_STATUS;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = t_creditsFull ? 1 : 0;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getSLVtoMASCreditFullStatus(bool* t_creditsFull) {
  *t_creditsFull = (m_RS485Buf[RS845_PAYLOAD_OFFSET] != 0);
  return;
}

void Pinball_Message::sendMAStoSLVCreditInc(const byte t_numCreditsToAdd) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_CREDIT_INC;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = t_numCreditsToAdd;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoSLVCreditInc(byte* t_numCreditsToAdd) {
  *t_numCreditsToAdd = m_RS485Buf[RS845_PAYLOAD_OFFSET];
  return;
}

void Pinball_Message::sendMAStoSLVCreditDec() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_CREDIT_DEC;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendMAStoSLVScoreReset() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_SCORE_RESET;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::sendMAStoSLVScoreAbs(const int t_score) {
  int recLen = 7; // Len, From, To, Type, 2 payload bytes, CRC
  int clamped = (t_score < 0) ? 0 : (t_score > 999 ? 999 : t_score);
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_SCORE_ABS;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = (byte)((clamped >> 8) & 0xFF);
  m_RS485Buf[RS845_PAYLOAD_OFFSET + 1] = (byte)(clamped & 0xFF);
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoSLVScoreAbs(int* t_score) {
  *t_score = ((int)m_RS485Buf[RS845_PAYLOAD_OFFSET] << 8) | (int)m_RS485Buf[RS845_PAYLOAD_OFFSET + 1];
  if (*t_score < 0) *t_score = 0;
  if (*t_score > 999) *t_score = 999;
  return;
}

void Pinball_Message::sendMAStoSLVScoreFlash(const int t_score) {
  int recLen = 7; // Len, From, To, Type, 2 payload bytes, CRC
  int clamped = (t_score < 0) ? 0 : (t_score > 999 ? 999 : t_score);
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_SCORE_FLASH;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = (byte)((clamped >> 8) & 0xFF);
  m_RS485Buf[RS845_PAYLOAD_OFFSET + 1] = (byte)(clamped & 0xFF);
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoSLVScoreFlash(int* t_score) {
  *t_score = ((int)m_RS485Buf[RS845_PAYLOAD_OFFSET] << 8) | (int)m_RS485Buf[RS845_PAYLOAD_OFFSET + 1];
  if (*t_score < 0) *t_score = 0;
  if (*t_score > 999) *t_score = 999;
  return;
}

void Pinball_Message::sendMAStoSLVScoreInc10K(const int t_incrementIn10Ks) {  // Increase score by 1..999 in 10,000s
  int recLen = 7;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = (byte)((t_incrementIn10Ks >> 8) & 0xFF);    // High byte
  m_RS485Buf[RS845_PAYLOAD_OFFSET + 1] = (byte)(t_incrementIn10Ks & 0xFF);       // Low byte
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getMAStoSLVScoreInc10K(int* t_incrementIn10Ks) {
  *t_incrementIn10Ks = ((int)m_RS485Buf[RS845_PAYLOAD_OFFSET] << 8) | (int)m_RS485Buf[RS845_PAYLOAD_OFFSET + 1];
  return;
}

void Pinball_Message::sendMAStoSLVScoreQuery() {
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_MAS_TO_SLV_SCORE_QUERY;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

// NEW: score report single int 0..999
void Pinball_Message::sendSLVtoMASScoreReport(const int t_score) {
  int recLen = 7;
  int clamped = (t_score < 0) ? 0 : (t_score > 999 ? 999 : t_score);
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_SLV;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TYPE_OFFSET] = RS485_TYPE_SLV_TO_MAS_SCORE_REPORT;
  m_RS485Buf[RS845_PAYLOAD_OFFSET] = (byte)((clamped >> 8) & 0xFF);
  m_RS485Buf[RS845_PAYLOAD_OFFSET + 1] = (byte)(clamped & 0xFF);
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Pinball_Message::getSLVtoMASScoreReport(int* t_score) {
  *t_score = ((int)m_RS485Buf[RS845_PAYLOAD_OFFSET] << 8) | (int)m_RS485Buf[RS845_PAYLOAD_OFFSET + 1];
  if (*t_score < 0) *t_score = 0;
  if (*t_score > 999) *t_score = 999;
  return;
}

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
  while ((millis() - m_messageLastSentTime) < RS485_MESSAGE_DELAY_MS) { }  // Pause to help prevent receiver's in buffer overflow.
  while (m_mySerial->availableForWrite() < tMsgLen) { }  // Wait until there is enough room in the outgoing buffer for this message.
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
  // 11/16/25: Small turn-around safety delay: give RS-485 transceivers a couple ms to settle before switching back to receive.
  //   Some transceivers or wiring need a short gap even after flush() to ensure remote reply isn't lost:
  //   delay(10); // might work with 2ms, but use 10ms to be safe. *******************************************************************************************************
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
  if (bytesAvailableInBuffer > 60) {  // RS485 serial input buffer should never get this close to 64-byte overflow.
    // Non-fatal: log and drain to recover gracefully instead of halting the system.
    sprintf(lcdString, "RS485 in buf ovrflw!");  // Discard and continue
    pLCD2004->println(lcdString);
    Serial.println(lcdString);
    // Drain buffer
    while (m_mySerial->available()) {
      m_mySerial->read();
    }
    m_lastError = RS485_ERROR_BUFFER_OVERFLOW;
    return false;  // BUT, we're signaling an error code
  }
  byte incomingMsgLen = m_mySerial->peek();  // First byte will be message length, or garbage is available = 0
  // bytesAvailableInBuffer must be greater than zero or there are no bytes in the incoming serial buffer.
  // bytesAvailableInBuffer must also be >= incomingMsgLen, or we don't have a complete message yet (we'll need to wait a moment.)
  if ((bytesAvailableInBuffer > 0) && (bytesAvailableInBuffer >= incomingMsgLen)) {
    // We have at least enough bytes for a complete incoming message!
    if (incomingMsgLen < 3) {  // Message too short to be a legit message.
      sprintf(lcdString, "RS485 msg too short"); pLCD2004->println(lcdString);
      while (m_mySerial->available()) {  // Discard available bytes to recover.
        m_mySerial->read();
      }
      m_lastError = RS485_ERROR_MSG_TOO_SHORT;
      return false;  // BUT, we're signaling an error code
    } else if (incomingMsgLen > RS485_MAX_LEN) {  // Message too long to be any real message.
      sprintf(lcdString, "RS485 msg too long"); pLCD2004->println(lcdString);
      while (m_mySerial->available()) {  // Discard available bytes to recover.
        m_mySerial->read();
      }
      m_lastError = RS485_ERROR_MSG_TOO_LONG;
      return false;  // BUT, we're signaling an error code
    }
    // So far, so good!  Now read the bytes of the message from the incoming serial buffer into the message buffer...
    for (byte i = 0; i < incomingMsgLen; i++) {  // Get the RS485 incoming bytes and put them in the t_msg[] byte array
      t_msg[i] = m_mySerial->read();
    }
    if (getChecksum(t_msg) != calcChecksumCRC8(t_msg, incomingMsgLen - 1)) {  // Bad checksum.
      sprintf(lcdString, "RS485 bad checksum"); pLCD2004->println(lcdString);
      m_lastError = RS485_ERROR_CHECKSUM;
      return false;  // BUT, we're signaling an error code
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

void Pinball_Message::setFrom(byte t_msg[], const byte t_from) {  // Inserts "from" i.e. ARDUINO_MAS into the appropriate byte
  t_msg[RS485_FROM_OFFSET] = t_from;
  return;
}

void Pinball_Message::setTo(byte t_msg[], const byte t_to) {      // Inserts "to" i.e. ARDUINO_SLV into the appropriate byte
  t_msg[RS485_TO_OFFSET] = t_to;
  return;
}

void Pinball_Message::setType(byte t_msg[], const byte t_type) {  // Inserts the message type into the appropriate byte
  t_msg[RS485_TYPE_OFFSET] = t_type;
  return;
}

byte Pinball_Message::getLen(const byte t_msg[]) {   // Returns the 1-byte length of the RS485 message in tMsg[]
  return t_msg[RS485_LEN_OFFSET];
}

byte Pinball_Message::getFrom(const byte t_msg[]) {  // Returns the 1-byte "from" Arduino ID
  return t_msg[RS485_FROM_OFFSET];
}

byte Pinball_Message::getTo(const byte t_msg[]) {    // Returns the 1-byte "to" Arduino ID
  return t_msg[RS485_TO_OFFSET];
}

byte Pinball_Message::getType(const byte t_msg[]) {  // Returns the 1-char message type i.e. 'M' for Mode
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