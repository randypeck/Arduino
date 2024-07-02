// DISPLAY_2004.H Rev: 06-30-24.
// Handles display of messages from the modules to the 20-char, 4-line (2004) Digole LCD display.
// It simplifies use of the LCD display by encapsulating all of the initialization and scrolling logic within the class.
// Display_2004 is a child class of parent DigoleSerialDisp, in order to be able to make both of them static, via a
// single instantiation from the main program.
// 06-30-24: Added printRowCol() function to print char(s) at specific row 1..4 and col 1..20.
// 12/06/20: SAVED 84 BYTES SRAM!  Re-wrote to put the four 21-char arrays on the heap.

#ifndef DISPLAY_2004_H
#define DISPLAY_2004_H

#include "Train_Consts_Global.h"

// The following lines are required by the Digole serial LCD display, connected to an Arduino hardware serial port.
#define _Digole_Serial_UART_    // To tell compiler compile the Digole library serial communication only
#include <DigoleSerial.h>       // The class that talks directly to the Digole LCD hardware via the serial port.

class Display_2004 : private DigoleSerialDisp {

  public:

    Display_2004(HardwareSerial* t_hdwrSerial, long unsigned int t_baud);  // Constructor

    void begin();  // Initialize the Digole serial LCD display, must be called in setup() before using the display.

    void println(const char t_nextLine[]);  // Scroll rows 2..4 up to rows 1..3, and display new line at row 4.
    void printRowCol(const byte row, const byte col, const char t_nextLine[]);  // Row 1..4, Col 1..20

  protected:

  private:

    char lineA[LCD_WIDTH + 1];  // 20 characters plus null terminator (required!)
    char lineB[LCD_WIDTH + 1];  // Confirmed these buffers are stored on the HEAP not the stack 12/6/20.
    char lineC[LCD_WIDTH + 1];
    char lineD[LCD_WIDTH + 1];

};

extern void endWithFlashingLED(int t_numFlashes); 

#endif

