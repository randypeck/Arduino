// ALPHANUMERIC DISPLAY LIBRARY.  Rev: 03/12/23.
// Class library to send alphanumeric characters to Adafruit I2C 4-character LED "backpack."
// By combining two backpacks, we can have a string of 8 characters displayed.
// 03/11/23 by RDP: Added high-level "write 8 characters to display" function.
// 03/11/23 by RDP: Combined two Adafruit classes (stripped to bare bones) with my custom class members.
// 11/02/20 by RDP: Cleaned up and shrunk based on 12/2017 Adafruit version.

// These displays use I2C to communicate.  Two pins are required to interface. There are multiple selectable I2C addresses.
// Use I2C addresses: 0x70, 0x71, 0x72 and/or 0x73.

#ifndef QuadAlphaNum_h
#define QuadAlphaNum_h

#include "Arduino.h"
#include "Wire.h"
#include <Train_Consts_Global.h>  // Required for use of ALPHA_WIDTH (which other modules use as well)
#include <Train_Functions.h>      // Required for use of endWithFlashingLED()

#define HT16K33_BLINK_CMD 0x80
#define HT16K33_BLINK_DISPLAYON 0x01
// Set Blink Rate to BLINK_OFF for always on, no blink (i.e. not display off)
#define HT16K33_BLINK_OFF 0
#define HT16K33_BLINK_2HZ  1
#define HT16K33_BLINK_1HZ  2
#define HT16K33_BLINK_HALFHZ  3

#define HT16K33_CMD_BRIGHTNESS 0xE0

// ************************************************************************************************

class LEDBackpack {
  // Rev: 03/12/23.
  // This is the raw HT16K33 controller class.

  public:

    LEDBackpack(void);
    // Constructor

    void begin(uint8_t t_i2cAddr);
    // Initialize the display using I2C address _addr

    void setBrightness(uint8_t t_brightness);
    // t_brightness = 0 (min) to 15 (max)

    void blinkRate(uint8_t t_blinkRate);
    // t_blinkRate = HT16K33_BLINK_DISPLAYON = steady on
    // t_blinkRate = HT16K33_BLINK_OFF       = steady off
    // t_blinkRate = HT16K33_BLINK_2HZ       = 2 Hz blink
    // t_blinkRate = HT16K33_BLINK_1HZ       = 1 Hz blink
    // t_blinkRate = HT16K33_BLINK_HALFHZ    = 0.5 Hz blink

    void clear(void);
    // Clear display

    void writeCharAscii(uint8_t t_index, uint8_t t_asciiChar, boolean t_dot = false);
    // Populate the display buffer with a single-char's 2-byte (16 bits for 15 LED segments/char) raw bit pattern.
    // This function does NOT actually write to the LED Backpack (for that, call writeDisplay().)
    //   t_index     = character index 0..3
    //   t_asciiChar = ASCII character
    //   t_dot       = if true, also light corresponding dot segment

    void writeDisplay(void);
    // Send buffered data in m_displayBuffer[] to display.

  private:

    uint8_t  m_i2cAddr;            // Backpack I2C address can be 0x70, 0x71, 0x72, or 0x73.
    uint16_t m_displayBuffer[8];  // Each character requires 2 bytes to hold it's LED raw bit pattern

};

// ************************************************************************************************

class QuadAlphaNum : public LEDBackpack {
  // Rev: 03/12/23.
  // 8-character alphanumeric LED array inherits from raw HT16K33 controller class LEDBackPack.

  public:

    QuadAlphaNum(void);  // Constructor

    void begin(uint8_t t_i2cAddr1, uint8_t t_i2cAddr2);
    // Use I2C addresses: 0x70, 0x71, 0x72 and/or 0x73.

    void writeToBackpack(const char t_lineToWrite[]);
    // Public function to send 1 to 8 characters to the 8-char LED Backpack display.

  private:

    LEDBackpack* m_pLeftSide;
    LEDBackpack* m_pRightSide;

    void sendToAlpha(LEDBackpack* leftSide, LEDBackpack* rightSide, const char t_nextLine[]);
    // Send an 8-char (plus null) alphanumeric string to the 8-character (combined) LED "backpacks."

};

#endif // QuadAlphaNum_h

