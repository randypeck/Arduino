// MESSAGE.CPP Rev: 03/04/24.  COMPLETE AND READY FOR TESTING *****************************************************************************************************************

// 03/04/24: Added code to filter out Mode message STATE_STOPPING for OCC as no OCC mode cares about STOPPING.

// LEFT OFF HERE 8/17/22 ERROR:
// There is a problem with sendMAStoALLRoute and getMAStoALLRoute - transferring two bytes to/from m_RS485Buf.
// WRITE some test code that displays results on the serial monitor to test converting to/from two bytes in a byte array.

#include "Message.h"

// NOTE REGARDING BTN, SNS, and LEG PULLING THE DIGITAL LINE LOW AND WAITING FOR PERMISSION TO TRANSMIT TO MAS (03/07/23):
// BTN, SNS, and (potentially someday) LEG have a digital RTS line connected to MAS that they pull low when they have a message
// the want to transmit to MAS, such as "a sensor has been tripped" or "a turnout button has been pressed."
// MAS's pMessage->available() will see that line go low, and will send the module an Ok-To-Send message via RS485.
// After the sender (BTN, SNS, or LEG) pulls the line low, and while it's waiting to receive the Ok-To-Send message from MAS, it's
// possible that some other message(s) could arrive via RS485 -- either from MAS before it checks the incoming digital request
// line, or from some other module responding to a MAS request (such as OCC transmitting Registration data, or SNS sending a
// sensor change, or BTN sending a turnout button press, or even MAS sending SNS a request for the status of a particular sensor.
// This split-second timing collision is VERY UNLIKELY to happen, but how should the system react in such cases?
// Fortunately, BTN and SNS only care about mode-change messages, and LEG doesn't use the digital line since it has nothing to
// transmit to MAS.  If LEG ever starts using the digital line, then perhaps MAS can always check that digital line before sending
// any other message, to pre-empt sending a message that LEG might miss.  Anyway, not a problem in the foreseeable future.
// NEVERTHELESS, after BTN/SNS/LEG pulls the line low, while watching the incoming RS485 buffer for the OK-To-Send ack from MAS,
// they need to check if there is a message that *is* for us but that is *not* the Ok-To-Send ack -- such as EITHER a mode change,
// which these modules DO care about, OR some other message that they can safely ignore.
// This scenario is difficult to code around, but it is also extremely unlikely to occur.  So our solution will be that, after
// BTN/SNS/LEG pulls the RTS digital line low, while waiting for the Ok-To-Send message from MAS, it allows for the possibility
// that it may receive a different incoming message first.  If it is a message that this module doesn't care about anyway, just
// discard it.  But if it receives a message that is not the Ok-To-Send, but *is* a message it would normally care about (such as
// Mode Change), then our solution will be to display "UNEXPECTED MESSAGE" on the LCD and trigger an Emergency Stop.
// This is fine because it is extremely unlikely to ever happen, but if it does we will at least stop the system rather than
// continuing on and missing a message and going haywire.

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

Message::Message() {  // Constructor
  return;
}

void Message::begin(HardwareSerial * t_mySerial, long unsigned int t_myBaud) {
  // Rev: 12/06/20.
  m_mySerial = t_mySerial;      // Pointer to the serial port we want to use for RS485.
  m_myBaud = t_myBaud;          // RS485 serial port baud rate.
  m_mySerial->begin(m_myBaud);  // Initialize the RS485 serial port that this object will be using.
  memset(m_RS485Buf, 0, RS485_MAX_LEN);  // Fill the buffer (array) with zeroes.
  m_RS485Buf[RS485_LEN_OFFSET] = 0;      // Setting message len to zero just for fun.
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Put RS485 in receive mode (LOW)
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);
  digitalWrite(PIN_OUT_RS485_TX_LED, LOW);       // Turn off the transmit LED
  pinMode(PIN_OUT_RS485_TX_LED, OUTPUT);
  digitalWrite(PIN_OUT_RS485_RX_LED, LOW);       // Turn off the receive LED
  pinMode(PIN_OUT_RS485_RX_LED, OUTPUT);
  sprintf(lcdString, "RS485 init ok!"); pLCD2004->println(lcdString);
}

char Message::available() {
  // Rev: 03/07/23.
  // This function is called by a main module just to check what type, if any, message is waiting for it.
  // Returns char TYPE of "relevant" message waiting in RS485 incoming buffer (i.e. 'A'), else char = ' ' if no message.
  // IF A NON-BLANK CHAR IS RETURNED, CALLER MUST STILL CALL THE APPROPRIATE "GET" MESSAGE FUNCTION TO RETRIEVE THE MESSAGE!
  // i.e. pMessage->getMAStoALLRoute()
  // Otherwise the contents of this message will be lost.

  // If a message of any type (to anyone) is found to be waiting, then this function calls the private "forThisModule" function to
  // see if it is a message that the caller cares about.  If the caller *does* care about this message, then the message type is
  // returned as a char as described above.
  // HOWEVER, if the message that was retrieved is *not* a type that the calling module cares about, it's still possible that
  // there is *another* message waiting in the RS485 incoming buffer, so let's not give up yet.  We need to discard the irrelevant
  // message and keep testing the "TO" and "TYPE" of incoming messages (via "forThisModule") until we either 1) find a message that
  // is relevant to the callee; or 2) discard any/all non-relevant messages in the buffer, and find that the incoming serial buffer
  // is now empty.  So don't give up just because the first waiting message may not be relevant to the caller.  We'll use a loop.

  // FOR MESSAGES WITH A DIGITAL RTS COMPONENT:  This only applies to MAS, since no other module has a digital input RTS.
  // When MAS calls available(), available() will FIRST check if there is a relevant incoming message already in the RS485 input
  // buffer.  If so, of course it will return that as the waiting message type.
  // HOWEVER, if there are no messages in the incoming buffer that are relevant to MAS (after discarding any/all irrelevant
  // messages) available() will check all three incoming digital RTS lines, and for any/all that are pulled low (it's possible
  // to have more than one pulled low at the same time), it will automatically send an "okay to transmit" message to the remote
  // calling module, then receive the new incoming message (sensor change, button press, or something from LEG), and return the
  // message type to the current module.  Thus, available() handles incoming digital RTS lines automatically, and totally
  // transparently to the main MAS program.

  // NOTE: The return "message type" will be unclear if there are any message types for ALL that use the same message type letter
  // as any message to the specific calling module.  I.e. all this module says is "the message is for you, and it's of type 'X'" so
  // there better only be one message type 'X' that the module cares about.  My RS485 message protocol ensures that this problem
  // won't occur as of 10/14/20.  The only messages that use the same type are 'S'ensor and 'B'utton messages to/from MAS.

  while (getMessageRS485(m_RS485Buf) == true) {  // As long as we find a new incoming RS485 message, get it and check it.
    // OK, there *is* a message.  If it's one that the caller cares about, return the type and we're done here.
    if (forThisModule(m_RS485Buf) == true) {
      return m_RS485Buf[RS485_TYPE_OFFSET];
    }
    // Well, there *is* a message but it's not something the calling module cares about, so ignore it and look for another.
  }
  // If we drop from the above 'while' loop, it means that we have cleared the incoming RS485 buffer with no relevant messages.
  // But *if we were called from MAS* then we're not done yet!
  // See if we can find if any of the 3 digital RTS lines have been pulled low by BTN, SNS, or LEG asking to send MAS a message:

  // Is BTN telling MAS that it wants to send a message, by pulling MAS's incoming RTS line low?
  if ((THIS_MODULE == ARDUINO_MAS) && (digitalRead(PIN_IN_REQ_TX_BTN) == LOW)) {
    // BTN wants to send MAS an RS485 message that a turnout button has been pressed!
    // Go ahead and transmit an "okay to send me a button press message" to BTN, and then get the new incoming message...
    sendMAStoBTNRequestButton();  // No parameters; just sending the message gives permission for BTN to transmit an RS485 msg.
    // Now wait for a reply...
    // There is NO legitimate reason why we would get ANY RS485 message from anyone except BTN at this point.
    m_RS485Buf[RS485_TO_OFFSET] = 0;  // Anything other than ARDUINO_BTN
    while (getMessageRS485(m_RS485Buf) == false) { }  // Wait until we have a complete new incoming message.
    // We should NEVER get a message that isn't to MAS from BTN, but we'll check anyway...
    if ((m_RS485Buf[RS485_FROM_OFFSET] != ARDUINO_BTN) ||
        (m_RS485Buf[RS485_TO_OFFSET] != ARDUINO_MAS)||
        (m_RS485Buf[RS485_TYPE_OFFSET] != 'B')) {
      sprintf(lcdString, "RS485 MAS BTN Bad!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    // Yay!  We now have a message in the buffer from BTN to MAS holding a button press, so tell MAS it's ready to be checked.
    return m_RS485Buf[RS485_TYPE_OFFSET];  // 'B'
  }

  // Is SNS telling MAS that it wants to send a message, by pulling MAS's incoming RTS line low?
  if ((THIS_MODULE == ARDUINO_MAS) && (digitalRead(PIN_IN_REQ_TX_SNS) == LOW)) {
    // SNS wants to send MAS an RS485 message that a sensor has changed status (tripped or cleared)!
    // Go ahead and transmit an "okay to send me a sensor update message" to SNS, and then get the new incoming message...
    sendMAStoSNSRequestSensor(0);  // Not requesting specific sensor; sending 0 just gives permission for SNS to xmit via RS485.
    // Now wait for a reply...
    // There is NO legitimate reason why we would get ANY RS485 message from anyone except SNS at this point.
    m_RS485Buf[RS485_TO_OFFSET] = 0;   // Anything other than ARDUINO_SNS
    while (getMessageRS485(m_RS485Buf) == false) { }  // Wait until we have a complete new incoming message.
    // We should NEVER get a message that isn't to MAS from SNS, but we'll check anyway...
    if ((m_RS485Buf[RS485_FROM_OFFSET] != ARDUINO_SNS) ||
        (m_RS485Buf[RS485_TO_OFFSET] != ARDUINO_ALL) ||
        (m_RS485Buf[RS485_TYPE_OFFSET] != 'S')) {
      sprintf(lcdString, "RS485 MAS SNS Bad!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    // Yay!  We now have a message in the buffer from SNS to MAS holding a sensor update, so tell MAS it's ready to be checked.
    return m_RS485Buf[RS485_TYPE_OFFSET];  // 'S'
  }

  // Is LEG telling MAS that it wants to send a message, by pulling MAS's incoming RTS line low?
  if ((THIS_MODULE == ARDUINO_MAS) && (digitalRead(PIN_IN_REQ_TX_LEG) == LOW)) {
    // LEG wants to send MAS an RS485 message, but we have no idea why because we haven't defined any yet.  That's an error.
    sprintf(lcdString, "RS485 MAS LEG Bad!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  // Alas, we have emptied the incoming RS485 buffer and found no relevant messages, *and* if we were called by MAS, nobody is
  // pulling a digital RTS line low.
  // Sorry, caller, no soup for you today.
  return(' ');
}

// *****************************************************************************************
// ************ P U B L I C   M O D U L E - S P E C I F I C   F U N C T I O N S ************
// *****************************************************************************************

// ALL incoming messages are retrieved by the calling modules by first calling pMessage->available().
// If a message is found in the incoming RS485 buffer, available() populates this class's m_RS485Buf[] buffer, and passes the
// message type back to the calling module i.e. MAS as the function's return value.
// If no message is found, the return char value is ' '.  Otherwise it is char message-type, such as 'M', 'R', etc.
// All of the following public message-get functions assume that pMessage->available() was called, and returned a non-blank message
// type, and that m_RS485Buf[] a valid message of that type.

// *** MODE/STATE CHANGE MESSAGES ***

void Message::sendMAStoALLModeState(const byte t_mode, const byte t_state) {
  int recLen = 7;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 7 bytes: Length, From, To, 'M', mode, state, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_ALL;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'M';  // Mode and state
  m_RS485Buf[RS485_MAS_ALL_MODE_OFFSET] = t_mode;
  m_RS485Buf[RS485_MAS_ALL_STATE_OFFSET] = t_state;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getMAStoALLModeState(byte* t_mode, byte* t_state) {
  // Returns mode and state to caller via pointed-to parameters.
  *t_mode = m_RS485Buf[RS485_MAS_ALL_MODE_OFFSET];
  *t_state = m_RS485Buf[RS485_MAS_ALL_STATE_OFFSET];
  // Even though MODE_POV isn't supported, it's the highest-value mode so this test will still work.
  if ((*t_mode < MODE_UNDEFINED) || (*t_mode > MODE_POV) || (*t_state < STATE_UNDEFINED) || (*t_state > STATE_STOPPED)) {
    //sprintf(lcdString, "START M%i S%i", *t_mode, *t_state); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "RS485 BAD MODE STMT!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  return;
}

// *** LOCO SETUP AND LOCATION MESSAGES (REGISTRATION MODE ONLY) ***

void Message::sendOCCtoLEGFastOrSlow(const char t_fastOrSlow) {  // t_fastOrSlow = F|S
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'F', F|S, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_OCC;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_LEG;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'F';  // Fast or slow
  m_RS485Buf[RS485_OCC_LEG_FAST_SLOW_OFFSET] = t_fastOrSlow;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getOCCtoLEGFastOrSlow(char* t_fastOrSlow) {
  *t_fastOrSlow = m_RS485Buf[RS485_OCC_LEG_FAST_SLOW_OFFSET];  // F|S
  return;
}

void Message::sendOCCtoLEGSmokeOn(const char t_smokeOrNoSmoke) {  // t_smokeOrNoSmoke = S|N
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'K', S|N, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_LEG;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'K';  // smoKe
  m_RS485Buf[RS485_OCC_LEG_SMOKE_ON_OFF_OFFSET] = t_smokeOrNoSmoke;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getOCCtoLEGSmokeOn(char* t_smokeOrNoSmoke) {
  *t_smokeOrNoSmoke = m_RS485Buf[RS485_OCC_LEG_SMOKE_ON_OFF_OFFSET];  // S|N
  return;
}

void Message::sendOCCtoLEGAudioOn(const char t_audioOrNoAudio) {  // t_audioOrNoAudio = A|N
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'A', A|N, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_LEG;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'A';  // Audio
  m_RS485Buf[RS485_OCC_LEG_AUDIO_ON_OFF_OFFSET] = t_audioOrNoAudio;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getOCCtoLEGAudioOn(char* t_audioOrNoAudio) {
  *t_audioOrNoAudio = m_RS485Buf[RS485_OCC_LEG_AUDIO_ON_OFF_OFFSET];  // S|N
  return;
}


void Message::sendOCCtoLEGDebugOn(const char t_debugOrNoDebug) {  // t_debugOrNoDebug = D|N
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'D', D|N, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_LEG;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'D';  // Debug
  m_RS485Buf[RS485_OCC_LEG_DEBUG_ON_OFF_OFFSET] = t_debugOrNoDebug;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getOCCtoLEGDebugOn(char* t_debugOrNoDebug) {
  *t_debugOrNoDebug = m_RS485Buf[RS485_OCC_LEG_DEBUG_ON_OFF_OFFSET];  // D|N
  return;
}

void Message::sendOCCtoALLTrainLocation(const byte t_locoNum, const routeElement t_locoBlock) {
  // Note that although we pass a routeElement as a function parm, i.e. BW03, we pass byte blockNum and char blockDir in message.
  int recLen = 8;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 8 bytes: Length, From, To, 'L', locoNum, BlockNum, BlockDir E|W, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_OCC;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_ALL;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'L';  // train Location
  m_RS485Buf[RS485_OCC_ALL_REGISTER_LOCO_NUM_OFFSET] = t_locoNum;  // 3/3/23 Probably should send block num/dir as a route element **********************************************************************
  m_RS485Buf[RS485_OCC_ALL_REGISTER_BLOCK_NUM_OFFSET] = t_locoBlock.routeRecVal;
  if (t_locoBlock.routeRecType == BE) {
    m_RS485Buf[RS485_OCC_ALL_REGISTER_BLOCK_DIR_OFFSET] = LOCO_DIRECTION_EAST;  // 'E'
  } else {
    m_RS485Buf[RS485_OCC_ALL_REGISTER_BLOCK_DIR_OFFSET] = LOCO_DIRECTION_WEST;  // 'W'
  }
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getOCCtoALLTrainLocation(byte* t_locoNum, routeElement* t_locoBlock) {
  // Note that although we pass a routeElement as a function parm, i.e. BW03, we pass byte blockNum and char blockDir in message.
  *t_locoNum = m_RS485Buf[RS485_OCC_ALL_REGISTER_LOCO_NUM_OFFSET];  // locoNum should be 1..n, or 99 for static train.
  if (*t_locoNum < 1) {
    sprintf(lcdString, "RS485 no train zero!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  // Here is some tricky handling of a struct element passed via pointer...
  // Explanation: https://stackoverflow.com/questions/16841018/modifying-a-struct-passed-as-a-pointer-c
  (*t_locoBlock).routeRecVal = m_RS485Buf[RS485_OCC_ALL_REGISTER_BLOCK_NUM_OFFSET];  // Same as t_locoBlock->routeRecVal
  byte dir = m_RS485Buf[RS485_OCC_ALL_REGISTER_BLOCK_DIR_OFFSET];
  if (dir == LOCO_DIRECTION_EAST) {
    (*t_locoBlock).routeRecType = BE;
  } else {  // Must be west
    (*t_locoBlock).routeRecType = BW;
  }
  return;
}

// *** ROUTE MESSAGES (AUTO/PARK MODE ONLY) ***

void Message::sendMAStoALLRoute(const byte t_locoNum, const char t_extOrCont, const unsigned int t_routeRecNum,
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

void Message::getMAStoALLRoute(byte* t_locoNum, char* t_extOrCont, unsigned int* t_routeRecNum, unsigned int* t_countdown) {
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

// *** BUTTON PRESS MESSAGES (MANUAL MODE ONLY) ***

void Message::sendMAStoBTNRequestButton() {  
  // THIS PRIVATE FUNCTION IS ONLY CALLED BY "AVAILABLE()".  It will never be called from a main module i.e. O_MAS.
  // Transmit "permission to send me an RS485 button press message" from MAS to BTN, and wait until a reply appears in the incoming
  // RS485 serial buffer.
  // Only call this function if the digital line to MAS is being pulled low by O_BTN.
  // Send RS485 message to O_BTN asking for turnout button press update.
  // O_BTN only knows of a button press; it doesn't know what state the turnout was in or which turnout LED is lit.
  int recLen = 5;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 5 bytes: Length, From, To, 'B', CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_BTN;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'B';  // "You have permission to transmit button press info"
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getMAStoBTNRequestButton() {
  // This is the function BTN watches for after pulling the RTS digital line low, to get permission to transmit an RS485 message.
  // Nothing to do here, because "available()" would simply return "true" that we have received permission to transmit.
  // So we will never even call this function from BTN.  Let's throw a fatal error if this happens:
  sprintf(lcdString, "RS485 MAS BTN REC'D!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
}

void Message::sendBTNtoMASButton(const byte t_buttonNum) {
  // Always check for incoming messages BEFORE calling this function to send a button press, for two reasons:
  // 1. We need to clear out the incoming RS485 buffer in order to prepare to receive our "okay to xmit" message, and
  // 2. We need to be sure MAS isn't sending us a mode-change command that would negate us needing to send this message.
  // First pull PIN_OUT_REQ_TX_BTN low, to tell A-MAS that we want to send it a "turnout button pressed" message...
  digitalWrite(PIN_OUT_REQ_TX_BTN, LOW);
  // Wait for an RS485 command from MAS requesting button status.  Remember that it's possible that we may
  // receive some RS485 messages that are irrelevant first, so ignore all RS485 messages until we get ours.
  m_RS485Buf[RS485_TO_OFFSET] = 0;  // Anything other than ARDUINO_BTN
  do {
    getMessageRS485(m_RS485Buf);
  } while (m_RS485Buf[RS485_TO_OFFSET] != ARDUINO_BTN);
  // Got an RS485 command addressed to BTN.  We could also check to confirm that it was from MAS, and that
  // the command was 'B' for "send pushbutton number that was pressed", but since BTN has only ONE RS485
  // message that it could ever receive, and it would be this command from MAS, no need to check here.
  // Return the "I have a message for you, MAS" digital line back to HIGH state...
  digitalWrite(PIN_OUT_REQ_TX_BTN, HIGH);  // Turn off the "I have a button press to report" digital line
  // Send an RS485 message to MAS indicating which turnout pushbutton was pressed.  1..32 (not 0..31)
  // 12/15/20: If MAS is not seeing this message, it could be because we are transmitting so quickly after receiving the
  // "permission to transmit button number" message from MAS, that MAS hasn't had time to transition from transmit mode into
  // receive mode.  So we could put a slight delay here.  Similar for sendSNStoALLSensorStatus().
  // delay(2);  // We could add this 2ms delay if we want to allow extra time for MAS to transition from send to receive mode.
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'B', buttonNum, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_BTN;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'B';  // Button
  m_RS485Buf[RS485_BTN_MAS_BUTTON_NUM_OFFSET] = t_buttonNum;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getBTNtoMASButton(byte* t_buttonNum) {
  *t_buttonNum = m_RS485Buf[RS485_BTN_MAS_BUTTON_NUM_OFFSET];
  if ((*t_buttonNum < 1) || (*t_buttonNum > TOTAL_TURNOUTS)) {
    sprintf(lcdString, "RS485 bad button no!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  return;
}

// *** TURNOUT MESSAGES (ANY MODE EXCEPT REGISTRATION) ***

void Message::sendMAStoALLTurnout(const byte t_turnoutNum, const char t_turnoutDir) {
  int recLen = 7;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 7 bytes: Length, From, To, 'T', TurnoutNum, TurnoutDir, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_ALL;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'T';  // Turnout throw request
  m_RS485Buf[RS485_MAS_ALL_SET_TURNOUT_NUM_OFFSET] = t_turnoutNum;
  m_RS485Buf[RS485_MAS_ALL_SET_TURNOUT_DIR_OFFSET] = t_turnoutDir;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getMAStoALLTurnout(byte* t_turnoutNum, char* t_turnoutDir) {
  // Expects turnoutNum 1..TOTAL_TURNOUTS
  *t_turnoutNum = m_RS485Buf[RS485_MAS_ALL_SET_TURNOUT_NUM_OFFSET];
  if ((*t_turnoutNum < 1) || (*t_turnoutNum > TOTAL_TURNOUTS)) {
    sprintf(lcdString, "RS485 bad trnout no!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  *t_turnoutDir = m_RS485Buf[RS485_MAS_ALL_SET_TURNOUT_DIR_OFFSET];
  if ((*t_turnoutDir != TURNOUT_DIR_NORMAL) && (*t_turnoutDir != TURNOUT_DIR_REVERSE)) {  // 'N' or 'R'
    //sprintf(lcdString, "%c", *t_turnoutDir); pLCD2004->println(lcdString); Serial.println(lcdString);
    sprintf(lcdString, "RS485 bad trnout dir"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  return;
}

// *** SENSOR MESSAGES COULD BE USED IN ANY MODE ***

void Message::sendMAStoSNSRequestSensor(const byte t_sensorNum) {  
  // This function is called in two separate ways:
  // 1. By "available()" (with t_sensorNum = 0) when SNS has pulled MAS's digital RTS line low (requesting permission to xmit.)
  //    In which case, this message means "SNS, you have permission to transmit whatever sensor change you detected."
  // 2. By MAS when MAS wants to know the status of a particular sensor (i.e. digital RTS was not pulled low.)
  //    In which case, this message means "SNS, please transmit the status of sensor number t_sensorNum."
  // Thus, if outgoing parm t_sensorNum = 0, it means the digital line to MAS is being pulled low by SNS and this outgoing message
  //   is simply granting permission for SNS to transmit it's changed sensor status on the RS485 network.
  // Or, if the outgoing parm t_sensorNum is non-zero (1..52), the digital line from SNS is *not* being pulled low, but rather, MAS
  //   is asking SNS to transmit whatever the current state is of the given sensor -- either 'T'ripped or 'C'lear.
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'S', sensorNum, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_SNS;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'S';  // "You have permission to, or please, transmit sensor info"
  m_RS485Buf[RS485_MAS_SNS_SENSOR_NUM_OFFSET] = t_sensorNum;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getMAStoSNSRequestSensor(byte* t_sensorNum) {
  // If t_sensorNum = 0, this is just an okay to transmit a changed sensor number, per our (SNS) request.
  // If t_snesorNum > 1, MAS wants to know the status of a sensor (without our having asked.)
  *t_sensorNum = m_RS485Buf[RS485_MAS_SNS_SENSOR_NUM_OFFSET];
  return;
}

void Message::sendSNStoALLSensorStatus(const byte t_sensorNum, char t_sensorStatus) {
  // 7/7/22: Eliminated locoNum parm since we don't need SNS to maintain a Train Progress table and track that.
  // Always check for incoming messages BEFORE calling this function to send a button press, for two reasons:
  // 1. We need to clear out the incoming RS485 buffer in order to prepare to receive our "okay to xmit" message, and
  // 2. We need to be sure MAS isn't sending us a mode-change command that would negate us needing to send this message.
  // First pull PIN_OUT_REQ_TX_SNS low, to tell A-MAS that we want to send it a "sensor status" message...
  digitalWrite(PIN_OUT_REQ_TX_SNS, LOW);
  // Wait for an RS485 command from MAS requesting sensor status.  Remember that it's possible that we may
  // receive some RS485 messages that are irrelevant first, so ignore all RS485 messages until we get ours.
  m_RS485Buf[RS485_TO_OFFSET] = 0;   // Discard any incoming messages other than to ARDUINO_SNS.  Should not happen!
  // We shouldn't see anything other than to SNS, but also what we are doing here is waiting for MAS to xmit it's okay message.
  do {
    getMessageRS485(m_RS485Buf);
  } while (m_RS485Buf[RS485_TO_OFFSET] != ARDUINO_SNS);
  // Got an RS485 command addressed to SNS.  We could also check to confirm that it was from MAS, and that
  // the command was 'S' for "send sensor status", but we won't bother since we shouldn't get anything else.
  // Return the "I have a message for you, MAS" digital line back to HIGH state...
  digitalWrite(PIN_OUT_REQ_TX_SNS, HIGH);  // Turn off the "I have a sensor change to report" digital line
  // Send an RS485 message to MAS indicating status of requested sensor
  // 12/15/20: If MAS is not seeing this message, it could be because we are transmitting so quickly after receiving the
  // "permission to transmit sensor change" message from MAS, that MAS hasn't had time to transition from transmit mode into
  // receive mode.  So we could put a slight delay here.  Similar for sendBTNtoMASButton().
  // delay(2);  // We could add this 2ms delay if we want to allow extra time for MAS to transition from send to receive mode.
  int recLen = 7;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 8 bytes: Length, From, To, 'S', sensorNum, Trip|Clear, locoNum, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_SNS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_ALL;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'S';  // Sensor
  m_RS485Buf[RS485_SNS_ALL_SENSOR_NUM_OFFSET] = t_sensorNum;
  m_RS485Buf[RS485_SNS_ALL_SENSOR_TRIP_CLEAR_OFFSET] = t_sensorStatus;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getSNStoALLSensorStatus(byte* t_sensorNum, char* t_sensorStatus) {
  // 7/7/22: Eliminated locoNum parm since we don't need SNS to maintain a Train Progress table and track that.
  *t_sensorNum = m_RS485Buf[RS485_SNS_ALL_SENSOR_NUM_OFFSET];
  *t_sensorStatus = m_RS485Buf[RS485_SNS_ALL_SENSOR_TRIP_CLEAR_OFFSET];
  if ((*t_sensorNum < 1) || (*t_sensorNum > TOTAL_SENSORS)) {
    sprintf(lcdString, "RS485 bad sensor no!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  return;
}

// *****************************************************************************************
// *************************** P R I V A T E   F U N C T I O N S ***************************
// *****************************************************************************************

bool Message::forThisModule(const byte t_msg[]) {  // Check if the message in the buffer is for the calling module or not.
  // This function only works if it is called with a legitimate incoming message in the t_msg[] buffer.
  // For any given module, it will only care about incoming messages that are address specifically to it, and also *some* messages
  // that are addressed to ALL.
  char mType = t_msg[RS485_TYPE_OFFSET];  // mType just saves us from constantly typing t_msg[RS485_TYPE_OFFSET] below
  // First, let's just check/eliminate messages addressed specifically to the calling module...
  if (t_msg[RS485_TO_OFFSET] == THIS_MODULE) {  // Everyone cares about messages specifically addressed to them!
    // EXCEPT, BTN doesn't care about receiving (here) a message from MAS "okay to transmit", because we read that
    // message as part of the sendBTNtoMASButton() function.
    if ((THIS_MODULE == ARDUINO_BTN) && (mType == 'B')) {
      return false;  // BTN should just ignore this "okay to transmit" message from MAS
    }
    // SNS is not quite as straightforward as BTN, because if sensorNum > 0, then it's an unsolicited request from MAS to transmit
    // a particular sensor's status, and not an "okay to transmit" message from MAS.  So check the value of the incoming sensorNum
    // before we decide if it's relevant, or if we'll handle it as part of the sendSNStoMASSensor() function instead.
    if ((THIS_MODULE == ARDUINO_SNS) && (mType == 'S')) {
      if (t_msg[RS485_MAS_SNS_SENSOR_NUM_OFFSET] > 0) {
        return true;
      }
      return false;  // If incoming sensor num = 0, SNS should just ignore this "okay to transmit" message from MAS
    }
    // If we ever have data that LEG wants to transmit to MAS, then we'll need to evaluate here whether or not it's "relevant."
    // I.e. if it's just an "okay to transmit" from MAS, then we will likely want to ignore it hear and handle it similarly to the
    // way we handle this in BTN.  But if we ever want to solicit anything from LEG, we'll need to allow that here.
    return true;
  }
  // Now if the message is to any module other than ALL, then THIS_MODULE won't care about it, so eliminate those...
  if ((t_msg[RS485_TO_OFFSET] != THIS_MODULE) && (t_msg[RS485_TO_OFFSET] != ARDUINO_ALL)) {
    return false;
  }
  // If we get here, the message must be to ARDUINO_ALL.
  // Now we only care about messages to ALL and whether or not this specific module cares about it based on message type...
  if (THIS_MODULE == ARDUINO_MAS) {  // Check types of messages that MAS cares about...
    // MAS cares about all messages to it and to all, so simply return true (available() will return msg type to caller.)
    return true;
  }
  if (THIS_MODULE == ARDUINO_OCC) {  // What to:ALL messages does OCC care about?  Route, Sensor, and Mode (conditionally)
    // Route, Sensor
    if ((mType == 'R') || (mType == 'S')) {
      return true;
    } else if (mType == 'M') {  // If it's a Mode message, filter out STATE_STOPPING for OCC regardless of mode
      if (t_msg[RS485_MAS_ALL_STATE_OFFSET] != STATE_STOPPING) {
        return true;
      }
    }
    return false;
  }
  if (THIS_MODULE == ARDUINO_LEG) {  // What to:ALL messages does LEG care about?
    // Mode, Location, Route, Sensor (same as OCC)
    if ((mType == 'M') || (mType == 'L') || (mType == 'R') || (mType == 'S')) {
      return true;
    }
    return false;
  }
  if (THIS_MODULE == ARDUINO_SNS) {  // What to:ALL messages does SNS care about?
    // Only cares about Mode (so it will only send sensor updates when appropriate)
    if (mType == 'M') {
      return true;
    }
    return false;
  }
  if (THIS_MODULE == ARDUINO_BTN) {  // What to:ALL messages does BTN care about?
    // Only cares about Mode (so it will only send button presses when appropriate)
    if (mType == 'M') {
      return true;
    }
    return false;
  }
  if (THIS_MODULE == ARDUINO_LED) {  // What to:ALL messages does LED care about?
    // Mode, Turnouts (Mode, so it will only illuminate Turnout LEDs when appropriate)
    if ((mType == 'M') || (mType == 'T')) {
      return true;
    }
    return false;
  }
  if (THIS_MODULE == ARDUINO_SWT) {  // What to:ALL messages does SWT care about?
    // Turnouts.  The only module that doesn't care about Mode/State.
    if (mType == 'T') {
      return true;
    }
    return false;
  }
  // We should never get this far.
  sprintf(lcdString, "FTM ERROR!"); pLCD2004->println(lcdString); endWithFlashingLED(5); // Halts everything
  return false;  // Wil never reach this statement
}

void Message::sendMessageRS485(byte t_msg[]) {
  // Rev: 10-18-20.  Added delay between successive transmits to avoid overflowing modules' incoming serial buffer.
  // This routine must *only* be called when an entire message is ready to write, not a byte at a time.
  // This version, as part of the RS485 message class, automatically calculates and adds the CRC checksum.
  digitalWrite(PIN_OUT_RS485_TX_LED, HIGH);  // Turn on the transmit LED
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
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Switch back to receive mode (set LOW)
  digitalWrite(PIN_OUT_RS485_TX_LED, LOW);  // Turn off the transmit LED
  m_messageLastSentTime = millis();         // Keeps track of *when* this message was last sent.
  return;
}

bool Message::getMessageRS485(byte* t_msg) {
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
    digitalWrite(PIN_OUT_RS485_RX_LED, HIGH);  // Turn on the receive LED
    if (incomingMsgLen < 5) {  // Message too short to be a legit message.  Fatal!
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
    digitalWrite(PIN_OUT_RS485_RX_LED, LOW);  // Turn off the receive LED
    return true;
  }
  else {  // We don't yet have an entire message in the incoming RS485 bufffer
    return false;
  }
}

void Message::setLen(byte t_msg[], const byte t_len) {    // Inserts message length i.e. 7 into the appropriate byte
  t_msg[RS485_LEN_OFFSET] = t_len;
  return;
}

void Message::setFrom(byte t_msg[], const byte t_from) {  // Inserts "from" i.e. ARDUINO_MAS into the appropriate byte
  t_msg[RS485_FROM_OFFSET] = t_from;
  return;
}

void Message::setTo(byte t_msg[], const byte t_to) {      // Inserts "to" i.e. ARDUINO_BTN into the appropriate byte
  t_msg[RS485_TO_OFFSET] = t_to;
  return;
}

void Message::setType(byte t_msg[], const char t_type) {  // Inserts the message type i.e. 'M'ode into the appropriate byte
  t_msg[RS485_TYPE_OFFSET] = t_type;
  return;
}

byte Message::getLen(const byte t_msg[]) {   // Returns the 1-byte length of the RS485 message in tMsg[]
  return t_msg[RS485_LEN_OFFSET];
}

byte Message::getFrom(const byte t_msg[]) {  // Returns the 1-byte "from" Arduino ID
  return t_msg[RS485_FROM_OFFSET];
}

byte Message::getTo(const byte t_msg[]) {    // Returns the 1-byte "to" Arduino ID
  return t_msg[RS485_TO_OFFSET];
}

char Message::getType(const byte t_msg[]) {  // Returns the 1-char message type i.e. 'M' for Mode
  return t_msg[RS485_TYPE_OFFSET];
}

byte Message::getChecksum(const byte t_msg[]) {  // Just retrieves a byte from an existing message; does not calculate checksum!
  return t_msg[(getLen(t_msg) - 1)];
}

byte Message::calcChecksumCRC8(const byte t_msg[], byte t_len) {  // Calculate checksum of an incoming or outgoing message.
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

// ********************************************************************************************************************************
// ***** Deprecated functions as of 3/7/23:
// ********************************************************************************************************************************

/*

const byte RS485_MAS_OCC_QUESTION_PROMPT_OFFSET      =  4;  // Message from MAS to OCC offset of 8-byte prompt for A/N display.  There will be at least 2 for operator to select from.
const byte RS485_MAS_OCC_QUESTION_LAST_OFFSET        = 12;  // Message from MAS to OCC is this the last 8-byte prompt Y or N?
const byte RS485_OCC_MAS_ANSWER_REPLY_NUM_OFFSET     =  4;  // Message from OCC to MAS providing which question the operator selected as their choice, 0..n
const byte RS485_MAS_OCC_REGISTER_LOCO_NUM_OFFSET    =  4;  // Message from MAS to OCC giving train number to be used as registration prompt, with name (below).
const byte RS485_MAS_OCC_REGISTER_LOCO_NAME_OFFSET   =  5;  // Message from MAS to OCC giving first byte of 8 for name of train numbered above.
const byte RS485_MAS_OCC_REGISTER_LOCO_BLOCK_OFFSET  = 13;  // Message from MAS to OCC giving default block number to use for above train.
const byte RS485_MAS_OCC_REGISTER_LOCO_LAST_OFFSET   = 14;  // Message from MAS to OCC indicates if this is the last train to be used as registration options Y or N.
const byte RS485_MAS_OCC_PLAY_STATION_OFFSET         =  4;  // Message from MAS to OCC requesting to play announcement at this station number.
const byte RS485_MAS_OCC_PLAY_DATA_OFFSET            =  5;  // Message from MAS to OCC requesting to play 8-byte sequence on PA starting at this offset.

void Message::sendMAStoOCCQuestion(const char t_question[], const char t_lastQuestion) {  // t_lastQuestion = Y|N
  int recLen = 14;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 14 bytes: Length, From, To, 'Q', 8-char text, last_record, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_OCC;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'Q';  // Question
  for (int i = 0; i <= 7; i++) {
    m_RS485Buf[RS485_MAS_OCC_QUESTION_PROMPT_OFFSET + i] = (byte) t_question[i];
  }
  m_RS485Buf[RS485_MAS_OCC_QUESTION_LAST_OFFSET] = t_lastQuestion;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getMAStoOCCQuestion(char* t_question, char* t_lastQuestion) {  // t_question is an 8-char array
  // Rev: 10-14-20.
  for (int i = 0; i <= 7; i++) {
    t_question[i] = m_RS485Buf[RS485_MAS_OCC_QUESTION_PROMPT_OFFSET + i];  // NOTE THAT WE DO NOT USE ASTERISK WITH ARRAYS PASSED BY POINTER
  }
  *t_lastQuestion = char(m_RS485Buf[RS485_MAS_OCC_QUESTION_LAST_OFFSET]);  // Y|N
  return;
}

void Message::sendOCCtoMASAnswer(const byte t_replyNum) {
  int recLen = 6;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 6 bytes: Length, From, To, 'A', reply_num, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_OCC;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'A';  // Answer
  m_RS485Buf[RS485_OCC_MAS_ANSWER_REPLY_NUM_OFFSET] = t_replyNum;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getOCCtoMASAnswer(byte* t_replyNum) {
  *t_replyNum = m_RS485Buf[RS485_OCC_MAS_ANSWER_REPLY_NUM_OFFSET];
  return;
}

void Message::sendMAStoOCCTrainNames(const byte t_locoNum, const char t_trainName[], const byte t_dfltBlk, const char t_last) {  // t_last = Y|N for last record
  int recLen = 16;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 16 bytes: Length, From, To, 'N', train_num, 8-char name, dflt_block, last_record, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_OCC;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'N';  // Names
  m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_NUM_OFFSET] = t_locoNum;
  for (int i = 0; i <= 7; i++) {
    m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_NAME_OFFSET + i] = (byte) t_trainName[i];
  }
  m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_BLOCK_OFFSET] = t_dfltBlk;
  m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_LAST_OFFSET] = t_last;
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getMAStoOCCTrainNames(byte* t_locoNum, char* t_trainName, byte* t_dfltBlk, char* t_last) {  // t_trainName is an 8-char array
  *t_locoNum = m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_NUM_OFFSET];
  for (int i = 0; i <= 7; i++) {
    t_trainName[i] = m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_NAME_OFFSET + i];    // NOTE THAT WE DO NOT USE ASTERISK WITH ARRAYS PASSED BY POINTER
  }
  *t_dfltBlk = m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_BLOCK_OFFSET];
  *t_last = char(m_RS485Buf[RS485_MAS_OCC_REGISTER_LOCO_LAST_OFFSET]);  // Y|N
  return;
}

// ************ W A V   T R I G G E R   A N N O U N C E M E N T   F U N C T I O N S ************

void Message::sendMAStoOCCPlay(const byte t_stationNum, const char t_phrase[]) {
  // Message from MAS to OCC requesting to play announcement at this station number.
  int recLen = 14;
  m_RS485Buf[RS485_LEN_OFFSET] = recLen;  // Length is 14 bytes: Length, From, To, 'P', staion_num, 8-byte_phrase_array, CRC
  m_RS485Buf[RS485_FROM_OFFSET] = ARDUINO_MAS;
  m_RS485Buf[RS485_TO_OFFSET] = ARDUINO_OCC;
  m_RS485Buf[RS485_TYPE_OFFSET] = 'P';  // Play audio
  m_RS485Buf[RS485_MAS_OCC_PLAY_STATION_OFFSET] = t_stationNum;
  for (int i = 0; i <= 7; i++) {
    m_RS485Buf[RS485_MAS_OCC_PLAY_DATA_OFFSET + i] = (byte) t_phrase[i];
  }
  m_RS485Buf[recLen - 1] = calcChecksumCRC8(m_RS485Buf, recLen - 1);
  sendMessageRS485(m_RS485Buf);
  return;
}

void Message::getMAStoOCCPlay(byte* t_stationNum, char* t_phrase) {  // t_phrase is an array
  *t_stationNum = m_RS485Buf[RS485_MAS_OCC_PLAY_STATION_OFFSET];
  for (int i = 0; i <= 7; i++) {
    t_phrase[i] = m_RS485Buf[RS485_MAS_OCC_PLAY_DATA_OFFSET + i];  // NOTE THAT WE DO NOT USE ASTERISK WITH ARRAYS PASSED BY POINTER
  }
  return;
}

*/
