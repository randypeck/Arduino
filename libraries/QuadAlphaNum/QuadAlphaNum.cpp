// ALPHANUMERIC DISPLAY LIBRARY.  Rev: 03/12/23.
// Class library to send alphanumeric characters to Adafruit I2C 4-character LED "backpack."
// By combining two backpacks, we can have a string of 8 characters displayed.
// 03/11/23 by RDP: Added high-level "write 8 characters to display" function.
// 03/11/23 by RDP: Combined two Adafruit classes (stripped to bare bones) with my custom class members.
// 11/02/20 by RDP: Cleaned up and shrunk based on 12/2017 Adafruit version.

// These displays use I2C to communicate.  Two pins are required to interface. There are multiple selectable I2C addresses.
// Use I2C addresses: 0x70, 0x71, 0x72 and/or 0x73.

#include <Wire.h>
#include "QuadAlphaNum.h"

// alphafonttable[] stores the bit patterns for all characters for the 14-segment alphanumeric characters
static const uint16_t alphafonttable[] PROGMEM =  {

0b0000000000000001,
0b0000000000000010,
0b0000000000000100,
0b0000000000001000,
0b0000000000010000,
0b0000000000100000,
0b0000000001000000,
0b0000000010000000,
0b0000000100000000,
0b0000001000000000,
0b0000010000000000,
0b0000100000000000,
0b0001000000000000,
0b0010000000000000,
0b0100000000000000,
0b1000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0000000000000000,
0b0001001011001001,
0b0001010111000000,
0b0001001011111001,
0b0000000011100011,
0b0000010100110000,
0b0001001011001000,
0b0011101000000000,
0b0001011100000000,
0b0000000000000000, //  
0b0000000000000110, // !
0b0000001000100000, // "
0b0001001011001110, // #
0b0001001011101101, // $
0b0000110000100100, // %
0b0010001101011101, // &
0b0000010000000000, // '
0b0010010000000000, // (
0b0000100100000000, // )
0b0011111111000000, // *
0b0001001011000000, // +
0b0000100000000000, // ,
0b0000000011000000, // -
0b0100000000000000, // .  (Don't know why the Adafruit pattern was blank = 0b0000000000000000 = no dot displayed!)
0b0000110000000000, // /
0b0000110000111111, // 0
0b0000000000000110, // 1
0b0000000011011011, // 2
0b0000000010001111, // 3
0b0000000011100110, // 4
0b0010000001101001, // 5
0b0000000011111101, // 6
0b0000000000000111, // 7
0b0000000011111111, // 8
0b0000000011101111, // 9
0b0001001000000000, // :
0b0000101000000000, // ;
0b0010010000000000, // <
0b0000000011001000, // =
0b0000100100000000, // >
0b0001000010000011, // ?
0b0000001010111011, // @
0b0000000011110111, // A
0b0001001010001111, // B
0b0000000000111001, // C
0b0001001000001111, // D
0b0000000011111001, // E
0b0000000001110001, // F
0b0000000010111101, // G
0b0000000011110110, // H
0b0001001000000000, // I
0b0000000000011110, // J
0b0010010001110000, // K
0b0000000000111000, // L
0b0000010100110110, // M
0b0010000100110110, // N
0b0000000000111111, // O
0b0000000011110011, // P
0b0010000000111111, // Q
0b0010000011110011, // R
0b0000000011101101, // S
0b0001001000000001, // T
0b0000000000111110, // U
0b0000110000110000, // V
0b0010100000110110, // W
0b0010110100000000, // X
0b0001010100000000, // Y
0b0000110000001001, // Z
0b0000000000111001, // [
0b0010000100000000, // 
0b0000000000001111, // ]
0b0000110000000011, // ^
0b0000000000001000, // _
0b0000000100000000, // `
0b0001000001011000, // a
0b0010000001111000, // b
0b0000000011011000, // c
0b0000100010001110, // d
0b0000100001011000, // e
0b0000000001110001, // f
0b0000010010001110, // g
0b0001000001110000, // h
0b0001000000000000, // i
0b0000000000001110, // j
0b0011011000000000, // k
0b0000000000110000, // l
0b0001000011010100, // m
0b0001000001010000, // n
0b0000000011011100, // o
0b0000000101110000, // p
0b0000010010000110, // q
0b0000000001010000, // r
0b0010000010001000, // s
0b0000000001111000, // t
0b0000000000011100, // u
0b0010000000000100, // v
0b0010100000010100, // w
0b0010100011000000, // x
0b0010000000001100, // y
0b0000100001001000, // z
0b0000100101001001, // {
0b0001001000000000, // |
0b0010010010001001, // }
0b0000010100100000, // ~
0b0011111111111111,

};

// This is a library for Adafruit I2C LED 4-character "backpacks."
// Modified from old Dec 2017 version to streamline only what's needed for 4-char A/N display (two used.)
// Eliminated includsion of Adafruit_GFX.h/.cpp as they are not needed.

// ***************************************
// ***** LEDBackpack Class Functions *****
// ***************************************

LEDBackpack::LEDBackpack()  {  // Constructor
  // Rev: 03/12/23.
  return;
}

void LEDBackpack::begin(uint8_t t_i2cAddr) {
  // Rev: 03/12/23.
  // Backpack I2C address can be 0x70, 0x71, 0x72, or 0x73.
  m_i2cAddr = t_i2cAddr;
  Wire.begin();
  Wire.beginTransmission(m_i2cAddr);
  Wire.write(0x21);              // Turn on oscillator (takes us out of standby mode)
  Wire.endTransmission();
  blinkRate(HT16K33_BLINK_OFF);  // Full on; don't blink
  setBrightness(15);             // Max brightness
  return;
}

void LEDBackpack::setBrightness(uint8_t t_brightness) {
  // Rev: 03/12/23.
  // Brightness can be set 0..15.
  if (t_brightness > 15) t_brightness = 15;
  Wire.beginTransmission(m_i2cAddr);
  Wire.write(HT16K33_CMD_BRIGHTNESS | t_brightness);
  Wire.endTransmission();
  return;
}

void LEDBackpack::blinkRate(uint8_t t_blinkRate) {
  // Rev: 03/12/23.
  // Blink rate can be set to 0=off, 1=2hz, 2=1hz, or 3=1/2hz
  Wire.beginTransmission(m_i2cAddr);
  if (t_blinkRate > 3) t_blinkRate = 0; // turn off if not sure
  Wire.write(HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (t_blinkRate << 1));
  Wire.endTransmission();
  return;
}

void LEDBackpack::clear() {
  // Rev: 03/12/23.
  // Clear the display's buffer; does NOT write to the display.
  for (uint8_t i=0; i<8; i++) {
    m_displayBuffer[i] = 0;
  }
  return;
}

void LEDBackpack::writeCharAscii(uint8_t t_index, uint8_t t_asciiChar,  boolean t_dot) {
  // Rev: 03/11/23.
  // Populate the display buffer with a single-character's 2-byte raw bit pattern.
  // This function does NOT actually write to the LED Backpack (for that, call writeDisplay().)
  //   t_index     = character index 0..3
  //   t_asciiChar = ASCII character
  //   t_dot       = if true, also light corresponding dot segment
  uint16_t font = pgm_read_word(alphafonttable + t_asciiChar);
  m_displayBuffer[t_index] = font;
  if (t_dot) m_displayBuffer[t_index] |= (1<<14);
  return;
}

void LEDBackpack::writeDisplay() {
  // Rev: 03/12/23.
  // Send 4 chars of buffered data from local private char matrix array m_displayBuffer[] to LED Backpack display.
  // m_displayBuffer[] is NOT a character array; it stores each character's bit pattern in 2 bytes (16 bits.)
  // Thus 8 bytes are needed for a 4-character display.
  // You must have previously populated each of the 4 characters into m_displayBuffer[] using writeCharAscii().
  Wire.beginTransmission(m_i2cAddr);
  Wire.write((uint8_t)0x00); // start at address $00
  for (uint8_t i=0; i<8; i++) {
    Wire.write(m_displayBuffer[i] & 0xFF);
    Wire.write(m_displayBuffer[i] >> 8);
  }
  Wire.endTransmission();
  return;
}

// ****************************************
// ***** QuadAlphaNum Class Functions *****
// ****************************************

QuadAlphaNum::QuadAlphaNum() {  // Constructor
  // Rev: 03/12/23.
  // Instantiate two copies of the 4-char LED Backpack object (8 chars total display.)
  m_pLeftSide = new LEDBackpack;
  m_pRightSide = new LEDBackpack;
  return;
}

void QuadAlphaNum::begin(uint8_t t_i2cAddr1, uint8_t t_i2cAddr2) {
  // Rev: 03/12/23.
  // LED Backpacks can use I2C addresses: 0x70, 0x71, 0x72 and/or 0x73.
  // We will use addresses 0x70 and 0x71 for our side-by-side LED backpacks.
  m_pLeftSide->begin(t_i2cAddr1);   // Pass in the address of the left-hand backpack.
  m_pLeftSide->clear();          // clear() just clears the class internal 8-char buffer; it doesn't write to the display.
  m_pRightSide->begin(t_i2cAddr2);  // Pass in the address of the right-hand backpack.
  m_pRightSide->clear();
  return;
}

void QuadAlphaNum::writeToBackpack(const char t_nextLine[]) {
  // Rev: 03/12/23.
  // This is the public function that we call from our main program to send up to 8 characters to the pair of LED Backpacks.
  sendToAlpha(m_pLeftSide, m_pRightSide, t_nextLine);
  return;
}

void QuadAlphaNum::sendToAlpha(LEDBackpack* t_alpha4Left, LEDBackpack* t_alpha4Right, const char t_nextLine[])  {
  // Rev: 03/12/23.
  // PRIVATE FUNCTION: Send up to 8 characters to the pair of 4-character LED Backpack displays.
  // Based on sendToAlpha Rev 10/14/16 - TimMe and RDP.
  // The char array that holds the text is 9 bytes long, to allow for an 8-byte text msg (max.) plus null char.
  // Sample calls:
  //   char alphaString[9];  // Global array to hold strings sent to Adafruit 8-char a/n display.
  //   sendToAlpha("WP  83");   Note: more than 8 characters will crash!
  //   sprintf(alphaString, "WP  83");
  //   sendToAlpha(alphaString);   i.e. "WP  83"
  // Current draw can be up to 20ma per segment x 14 segments/char = 280ma/char * 8 chars = 2.24 AMPS if all illuminated.
  // The control panel powers them with a separate 5v wall wart so should not be an issue.
  if ((t_nextLine == (char *)NULL) || (strlen(t_nextLine) > ALPHA_WIDTH)) endWithFlashingLED(2);
  char alphaTextLine[] = "        ";  // Only 8 spaces because compiler will add null.
  for (byte i = 0; i < strlen(t_nextLine); i++) {
    alphaTextLine[i] = t_nextLine[i];  // Make sure the line is filled to 8 chars.
  }
  // Populate the display buffer with each single-character's 2-byte raw bit pattern.
  t_alpha4Left->writeCharAscii( 0, alphaTextLine[0]);
  t_alpha4Left->writeCharAscii( 1, alphaTextLine[1]);
  t_alpha4Left->writeCharAscii( 2, alphaTextLine[2]);
  t_alpha4Left->writeCharAscii( 3, alphaTextLine[3]);
  t_alpha4Right->writeCharAscii(0, alphaTextLine[4]);
  t_alpha4Right->writeCharAscii(1, alphaTextLine[5]);
  t_alpha4Right->writeCharAscii(2, alphaTextLine[6]);
  t_alpha4Right->writeCharAscii(3, alphaTextLine[7]);
  // Now send all 8 characters to the LED Backpacks.
  t_alpha4Left->writeDisplay();
  t_alpha4Right->writeDisplay();
  return;
}
