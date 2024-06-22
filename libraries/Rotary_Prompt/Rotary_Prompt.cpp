// ROTARY_PROMPT.CPP Rev: 03/23/23.
// Part of O_OCC.
// Pass an array of prompts to be displayed on the 8-char LED Backpack.  Operator can then use the Rotary Encoder to scroll through
// the list bidirectionally, and select one by pressing the Rotary Encoder.

#include "Rotary_Prompt.h"

Rotary_Prompt::Rotary_Prompt() {
  // no constructor tasks.
  return;
}

void Rotary_Prompt::begin(QuadAlphaNum* t_pQuadAlphaNum) {
  m_pQuadAlphaNum = t_pQuadAlphaNum;  // Pointer to the 8-char LED Backpack object
  return;
}

void Rotary_Prompt::clearDisplay() {
  char blank[] = "        ";
  m_pQuadAlphaNum->writeToBackpack(blank);
  return;
}

byte Rotary_Prompt::getSelection(rotaryEncoderPromptStruct* t_encoderPromptArray, byte t_topElement, byte t_startElement) {
  // Rev: 06/21/24.  Re-wrote.
  // t_encoderPromptArray is an array of 8-character strings to use as prompts, including some other details.
  // t_topElement is the number of the highest element in the passed array (0..t_topElement)
  // t_startElement is the element number to use as the first prompt (0..t_topElement)
  // Send strings one at a time to the 8-char a/n LED backpack and wait for operator to either turn the dial left, right, or press.
  // Upon turning left or right, display the next element/string below or above the current one, as a circular buffer.
  // Upon registering a press, return array index value 0..t_topElement of the element/string that was selected.
  // Special feature is if caller sets t_encoderPromptArray[t_startElement].expired == true, that element will be skipped.
  // However there is no checking to confirm if *every* element is expired, in which case this function will hang.
  // Also there is no checking if the first element to display (t_startElement) is expired; we assume it won't be.

  while (true) {

    m_pQuadAlphaNum->writeToBackpack(t_encoderPromptArray[t_startElement].alphaPrompt);  // Array passes as a pointer automatically
    // m_pQuadAlphaNum->writeToBackpack(&t_encoderPromptArray[t_startElement].alphaPrompt[0]);  // Same as above
    byte rotaryResponse = getRotaryResponse();  // Returns DIR_CCW, DIR_CW, or DIR_PUSH

    if (rotaryResponse == DIR_CCW) {        // Operator turned left; point at the next previous un-expired element
      do {
        if (t_startElement == 0) {
          t_startElement = t_topElement;
        }
        else {
          t_startElement = t_startElement - 1;
        }
      } while (t_encoderPromptArray[t_startElement].expired == true);

    } else if (rotaryResponse == DIR_CW) {    // Operator turned right; point at the next un-expired element
      do {
        t_startElement = t_startElement + 1;
        if (t_startElement > t_topElement) {
          t_startElement = 0;
        }
      } while (t_encoderPromptArray[t_startElement].expired == true);

    } else if (rotaryResponse == DIR_PUSH) {  // Operator selected something!
      chirp();
      return t_startElement;

    } else {
      sprintf(lcdString, "FATAL ROTARY ERR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);

    }
  }
}

// ***** PRIVATE FUNCTION *****

byte Rotary_Prompt::getRotaryResponse() {
  // Rev: 03/14/23.
  // Wait until either a rotation or press is detected, then return that info.
  // Returns direction of rotation as DIR_CCW (16) or DIR_CW (32), or press as DIR_PUSH (1).
  // Occasionially I did notice that it returned a "left" right after uploading but couldn't repeat reliably enough to find
  // a solution.  Won't be a problem as long as it doesn't register an anomolous "press."
  while (true) {

    // See if the dial was turned a notch in either direction...
    unsigned char pinState = (digitalRead(PIN_IN_ROTARY_2) << 1) | digitalRead(PIN_IN_ROTARY_1);
    byte rotaryState = rotaryTableFullStep[rotaryState & 0xf][pinState];
    byte stateReturn = (rotaryState & 0x30);
    if ((stateReturn == DIR_CCW) || (stateReturn == DIR_CW)) {  // The dial was turned in either direction!
      delay(10);  // Was getting very occasional bounce without a delay here.
      return stateReturn;
    }  // Else we didn't detect a rotation of the dial.

    // See if the dial was pressed...
    if (digitalRead(PIN_IN_ROTARY_PUSH) == LOW) {  // A press has been detected!
      delay(10);  // Software debounce: wait for push to settle.  Unnecessary but doesn't hurt.
      while (digitalRead(PIN_IN_ROTARY_PUSH) == LOW) {}  // Wait for operator to release the dial.
      delay(10);  // Software debounce: wait for release to release.  5ms give occasional bounce.
      return DIR_PUSH;  // rotaryPushed = true;
    }  // Else we didn't detect a press of the dial either.
  } // Keep trying until the operator either turns or presses the dial.
}
