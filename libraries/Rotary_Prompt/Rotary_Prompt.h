// ROTARY_PROMPT.H Rev: 03/23/23.
// Part of O_OCC.
// Pass an array of prompts to be displayed on the 8-char LED Backpack.  Operator can then use the Rotary Encoder to scroll through
// the list bidirectionally, and select one by pressing the Rotary Encoder.
// Caller must instantiate QuadAlphaNum 8-char alphanumeric display class and pass Rotary_Prompt a pointer to it.
// We're doing this without interrupts, and will block the caller (i.e. not return) until operator selects one of the options.
// 03/23/23: Added chirp() to acknowledge operator pushed (selected) Rotary Encoder.

#ifndef ROTARY_PROMPT_H
#define ROTARY_PROMPT_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <QuadAlphaNum.h>  // The is alphanumeric LED array of 8 characters to display prompts
#include <Display_2004.h>

// Rotary Encoder prompt structure includes prompt to display on 8-char alphanumeric LED display.
// We'll use it to store a list of prompts that the operator can scroll through using the rotary encoder.
// This struct must be available to both the caller (who must pass an array of this struct) and inside this class.
// By specifying "#include <Rotary_Prompt.h>" in the calling program, it will be available there as well as here.
struct rotaryEncoderPromptStruct {
  bool expired;
    // Caller will set true when this element has previously been selected, so it won't be used again as a prompt.
    // When sending a list of items multiple times where we eliminate one at a time, such as when identifying locos during
    // Registration, setting an assigned loco as "expired" saves the caller from having to rearrange the array after every call.
  byte referenceNum;
    // For caller's use to cross reference array element number with item number they're prompting for.
    // During Loco registration, this will store locoNum, which will not be the same as array element num.
    // For simple choice of two or three option prompts, we won't need this field.
  char alphaPrompt[ALPHA_WIDTH + 1];
    // 8-char a/n description for rotary encoder prompting (0 thru 7, element 8 is null)
};

// Define constants for the three possible return values from the rotary encoder: Turn Left, Turn Right, and Press.
#define DIR_CCW  0x10  // decimal 16
#define DIR_CW   0x20  // decimal 32
#define DIR_PUSH 0x01  // Just for something other than DIR_CCW and DIR_CW as a return value

class Rotary_Prompt {

  public:

    Rotary_Prompt();  // Constructor must be called above setup() so the object will be global to the module.

    void begin(QuadAlphaNum* t_pQuadAlphaNum);
    // Pass a pointer to the 8-char LED Backpack array object

    byte getSelection(rotaryEncoderPromptStruct* t_encoderPromptArray, byte t_numberOfElements, byte t_startingElement);
    // Pass an array of prompts, to be scrolled on the LED Backpack using the rotary encoder, and return the index number into the
    // array that is selected by the user (when they press the rotary encoder.)

  private:

    byte getRotaryResponse();
    // Wait until either a rotation or press is detected, then return that info.
    // Returns direction of rotation as DIR_CCW (16) or DIR_CW (32), or press as DIR_PUSH (1).

    // This global full-step state table emits a code at 00 only:
    const unsigned char rotaryTableFullStep[7][4] = {
      {0x0, 0x2, 0x4,  0x0},
      {0x3, 0x0, 0x1, 0x10},
      {0x3, 0x2, 0x0,  0x0},
      {0x3, 0x2, 0x1,  0x0},
      {0x6, 0x0, 0x4,  0x0},
      {0x6, 0x5, 0x0, 0x20},
      {0x6, 0x5, 0x4,  0x0},
    };
    unsigned char rotaryNewState = 0;      // Value will be 0, 16, or 32 depending on how rotary being turned
    unsigned char rotaryTurned   = false;
    unsigned char rotaryPushed   = false;

    // We dimension MAX_ROTARY_PROMPTS to TOTAL_TRAINS + 2 to accommodate all train names, plus 'STATIC' and 'DONE'.  Other prompts
    // will use only the first couple of elements.  This is used for both various Y/N pre-registration prompts, as well as for
    // registering trains.
          byte numRotaryPrompts   = 0;                 // Used when calling function to prompt on a/n display with rotary encoder.

    rotaryEncoderPromptStruct rotaryEncoderPrompt[TOTAL_TRAINS + 2];  // The most prompts I can envision would be every train plus STATIC and DONE
    byte totalTrainsFound = 0;                 // How many trains we find in Loco Ref to use as Registration location prompts.
    char last = ' ';
    byte locoNum = 0;
    byte dfltBlk = 0;

    QuadAlphaNum* m_pQuadAlphaNum;  // Pointer to the 8-char LED Backpack object
};

#endif
