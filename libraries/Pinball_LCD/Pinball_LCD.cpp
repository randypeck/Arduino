// PINBALL_LCD.CPP Rev: 01/14/26.

#include "Pinball_LCD.h"

Pinball_LCD::Pinball_LCD(HardwareSerial * t_hdwrSerial, long unsigned int t_baud):DigoleSerialDisp(t_hdwrSerial, t_baud) {
  // Constructor
  // DigoleSerialDisp is the name of the class in DigoleSerial.h/.cpp.  This is what talks directly to the LCD.
  return;
}

void Pinball_LCD::begin() {
  // Remember LCD_WIDTH = 20.  Each "lineX" array is LCD_WIDTH + 1 = 21 bytes.  Set byte 21 (offset 20) to null.
  memset(lineA, ' ', LCD_WIDTH); lineA[LCD_WIDTH] = '\0';
  memset(lineB, ' ', LCD_WIDTH); lineB[LCD_WIDTH] = '\0';
  memset(lineC, ' ', LCD_WIDTH); lineC[LCD_WIDTH] = '\0';
  memset(lineD, ' ', LCD_WIDTH); lineD[LCD_WIDTH] = '\0';
  DigoleSerialDisp::digoleBegin();               // Required to initialize LCD (renamed Digole's "begin" to not duplicate our LCD wrapper class "begin")
  DigoleSerialDisp::setLCDColRow(LCD_WIDTH, 4);  // Maps starting RAM address on LCD (if other than 1602)
  DigoleSerialDisp::disableCursor();             // We don't need to see a cursor on the LCD
  DigoleSerialDisp::backLightOn();
  delay(30);                                     // About 15ms required to allow LCD to boot before clearing screen
  DigoleSerialDisp::clearScreen();               // FYI, won't execute as the *first* LCD command
  delay(100);                                    // At 115200 baud, needs > 90ms after CLS before sending text.  No delay needed at 9600 baud.
  return;
}

void Pinball_LCD::println(const char t_nextLine[]) {
  // println() scrolls the bottom 3 (of 4) lines up one line, and inserts the passed text into the bottom line of the LCD.
  // t_nextLine[] is a 20-byte max character string with a null terminator.
  // Sample calls:
  //   char lcdString[LCD_WIDTH + 1];  // Global array to hold strings sent to Digole 2004 LCD
  //   LCD.println("O-SWT Ready!");       // Note: more than 20 characters will crash!
  //   sprintf(lcdString, "%.20s", "Hello world."); // 20 is hard-coded since don't know how to format with const LCD_WIDTH
  //   LCD.println(lcdString);            // i.e. "Hello world."
  //   int a = 7; unsigned long t = millis(); char c = 'R';
  //   sprintf(lcdString, "I %3i T %6lu C %3c", a, t, c);  // Will also crash if longer than 20 chars!
  //   LCD.println(lcdString);            // i.e. "I...7.T...3149.C...R"

  // If the incoming char array (string) is longer than the 21-byte array (20 chars plus null), then we will
  // have stepped on memory and must declare a fatal programming error.
  if ((t_nextLine == (char *)NULL) || (strlen(t_nextLine) > LCD_WIDTH)) endWithFlashingLED(2);
  // Scroll all lines up to make room for the new bottom line.
  strcpy(lineA, lineB);
  strcpy(lineB, lineC);
  strcpy(lineC, lineD);
  strncpy(lineD, t_nextLine, LCD_WIDTH);         // Copy the new bottom line into lineD, padded to 20 chars with nulls.
  int newLineLen = strlen(lineD);    // Get the length of the new bottom line (to the first null char.)
  // Pad the new bottom line with trailing spaces as needed.
  while (newLineLen < LCD_WIDTH) lineD[newLineLen++] = ' ';  // Last byte not touched; always remains "null."
  // Update the display.  Updated 10/28/16 by TimMe to add delays to fix rare random chars on display.
  DigoleSerialDisp::setPrintPos(0, 0);
  DigoleSerialDisp::print(lineA);
  delay(1);
  DigoleSerialDisp::setPrintPos(0, 1);
  DigoleSerialDisp::print(lineB);
  delay(2);
  DigoleSerialDisp::setPrintPos(0, 2);
  DigoleSerialDisp::print(lineC);
  delay(3);
  DigoleSerialDisp::setPrintPos(0, 3);
  DigoleSerialDisp::print(lineD);
  delay(1);
  return;
}

void Pinball_LCD::clearScreen() {
  // Clear all four line buffers and the physical display
  memset(lineA, ' ', LCD_WIDTH); lineA[LCD_WIDTH] = '\0';
  memset(lineB, ' ', LCD_WIDTH); lineB[LCD_WIDTH] = '\0';
  memset(lineC, ' ', LCD_WIDTH); lineC[LCD_WIDTH] = '\0';
  memset(lineD, ' ', LCD_WIDTH); lineD[LCD_WIDTH] = '\0';
  DigoleSerialDisp::clearScreen();
  delay(100);  // Required after CLS at 115200 baud
}

void Pinball_LCD::printRowCol(const byte row, const byte col, const char t_nextLine[]) {
  // Rev: 06-30-24.
  // Row 1..4, Col 1..20.
  // Prints char(s) of text at a specific row and column, rather than scrolling up lines 2..4 and printing on row 4, col 1.
  // If the incoming char array (string) is longer than the 21-byte array (20 chars plus null), then we will
  // have stepped on memory and must declare a fatal programming error.
  if ((row < 1) || (row > 4) || (col < 1) || (col > 20) ||
      (t_nextLine == (char *)NULL) ||
      ((strlen(t_nextLine) + col - 1) > LCD_WIDTH)) {
      endWithFlashingLED(2);
  }
  // We are assured via above test that our new string at it's position won't exceed the width of the LCD.
  if (row == 1) {
    memcpy(lineA + col - 1, t_nextLine, strlen(t_nextLine));  // Drop the new text into the line.
    DigoleSerialDisp::setPrintPos(0, 0);
    DigoleSerialDisp::print(lineA);
  } else if (row == 2) {
    memcpy(lineB + col - 1, t_nextLine, strlen(t_nextLine));  // Drop the new text into the line.
    DigoleSerialDisp::setPrintPos(0, 1);
    DigoleSerialDisp::print(lineB);
  } else if (row == 3) {
    memcpy(lineC + col - 1, t_nextLine, strlen(t_nextLine));  // Drop the new text into the line.
    DigoleSerialDisp::setPrintPos(0, 2);
    DigoleSerialDisp::print(lineC);
  } else if (row == 4) {
    memcpy(lineD + col - 1, t_nextLine, strlen(t_nextLine));  // Drop the new text into the line.
    DigoleSerialDisp::setPrintPos(0, 3);
    DigoleSerialDisp::print(lineD);
  }
  return;
}

// *****************************************************************
// *************** OLD NOTES JUST KEPT FOR REFERENCE ***************
// *****************************************************************

// 09/01/18: WARNING WARNING WARNING: The constructor must not use delay() if it is called in the main program before setup().
// If we want to roll the initialization into the constructor, we can use a microsecond timer instead of delay, which does work
// on Arduino pre-setup().
// Don't use Serial.begin() or any timers/delays in constructors:  There is a very good reason why the Serial class has that begin
// method that gets called in setup.  There's no other way for it to work.  Any class that uses it will need a begin method to call
// from setup that can call that code at the proper time.  There is a known problem with all Arduino-functions that involve Timers
// (Interrupts?) and constructors. For instance the delay() function also doesn't work in a constructor. Using a custom begin()
// function in your code is the right way handle this.
// The begin() function is what gets the hardware ready.  It is why pinMode(), digitalRead(), digitalWrite(), and a host of other
// hardware - related functions can't be used in constructors, either. The timer that controls millis() is initialized in main()
// which definitely will not have been called yet.
// The only thing a constructor should ever do is set the initial value of member variables, which doesn't depend on any other
// machinery (such as timers) being in a working state at that point.  Other work should be done via a begin()
// function that is called in setup().
// If you really want to have a delay in the constructor you could use delayMicroSeconds with the appropriate argument.
// This does work in the constructor as (unlike Delay) it does not user a timer interrupt.
// A "for" loop calling delayMicroseconds 10 times with an argument of 10k us would do 100ms.
// Alternately, the constructor, called before setup(), should just create the object.  Then, have a separate function in setup()
// that does the initialization.  Thus the object will have global scope since it was created above setup(), and we can also use
// delay() in the begin() routine inside of setup().

// Why not make this a parent/child relationship?  Although it "uses" and LCD display, rather than being "a kind of" LCD display...???
// How would we instantiate the object in that case?

// Also, my C++ Primer book shows a containment example, where the main object's constructor references an
// already-created object of the type being contained.  But what I'm doing below doesn't call the
// digole constructor; it just references a pointer to an object of that type...  I'm confused.
// Refer to the student class example in that book.
// The way I've written it below, I'm using the digole class but my 2004 object doesn't *contain* it, I think...???  Why???

// ***************** NOTES ON A FEW DIFFERENT WAYS TO SET UP THE CONSTRUCTOR *********************

// Pinball_LCD::Pinball_LCD(HardwareSerial* t_hdwrSerial, long unsigned int t_baud) {  // Constructor for methods 1 or 2
// Pinball_LCD::Pinball_LCD(DigoleSerialDisp * t_digoleLCD) {  // Constructor using Method 3

// DigoleSerialDisp is the name of the class in DigoleSerial.h/.cpp.  Note: Neither a parent nor a child class.
// pLCD2004 is a private pointer of type DigoleSerialDisp that will point to the Digole object.

// There are three ways to instantiate the DigoleSerialDisp object needed by the Pinball_LCD constructor.

// Method 1 uses the "new" keyword to create the object on the heap.  
// The advantage is that it encapsulates the use of the DigoleSerial class, so the calling .ino doesn't even know about it.
// Also, it creates a new object each time the constructor is called, so you can have more than one LCD display.
// The disadvantage is that use of "new" with Arduino is discouraged, because it may fragment the heap and memory is so small.
// Constructor definition:
//   Pinball_LCD::Pinball_LCD(HardwareSerial * t_hdwrSerial, long unsigned int t_baud) {
// Constructor content:
//   m_pLCD2004 = new DigoleSerialDisp(t_hdwrSerial, t_baud);
// hdwrSerial needs to be the address of Serial thru Serial3, a long unsigned int.
// baud can be any legit baud rate such as 115200.

// Method 2 uses the more traditional (for Arduino) way to instantiate an object, creating the object on the stack.
// It also has the advantage of encapsulating the use of the DigoleSerial class, so the calling .ino doesn't even know about it.
// The disadvantage of Method 2 is that, by using static here, you can only have *one* LCD display object.  Which is fine in this case.
// Constructor definition:
//   Pinball_LCD::Pinball_LCD(HardwareSerial * t_hdwrSerial, long unsigned int t_baud) {
// Constructor content:
//   static DigoleSerialDisp DigoleLCD(t_hdwrSerial, t_baud);
//   m_pLCD2004 = &DigoleLCD;

// Method 3 requires you to instantiate the DigoleSerialDisp object in the calling .ino code, instead of this constructor.
// It overcomes the disadvantages of both above methods: It does not use "new", and it allows instantiation of multiple objects.
// The disadvantage is that you need #define and #include statements about DigoleSerial in the calling .ino program, so the
// Digole class is no longer encapsulated in the Pinball_LCD class.
// The calling .ino program would need to following lines:
// #define _Digole_Serial_UART_  // To tell compiler compile the serial communication only
// #include "DigoleSerial.h"     // Tell the compiler to use the DigoleSerial class library
// DigoleSerialDisp digoleLCD(&Serial1, (long unsigned int) 115200); // Instantiate and name the Digole object
// #include "Pinball_LCD.h"                // Class in quotes = in the A_SWT directory; angle brackets = in the library directory.
// Pinball_LCD LCD(&digoleLCD);  // Instantiate our LCD object called LCD, pass pointer to DigoleLCD
// And the constructor definition would simply require a pointer to the Digole object:
//   Pinball_LCD::Pinball_LCD(DigoleSerialDisp * digoleLCD) {  // Constructor for method 3
// And the constructor would need this line:

// m_pLCD2004 = t_digoleLCD;  // METHOD 3: Assigns the Digole LCD pointer we were passed to our local private pointer

// 07/14/18: Can't be part of constructor if it's called above begin(), since constructor is called before Setup(), and you can't use
// delay() *or* Serial.begin() before Setup() on Arduino.
// However if we call the constructor at the top of loop(), it's no problem.
// m_pLCD2004->begin();                     // Required to initialize LCD
// m_pLCD2004->setLCDColRow(LCD_WIDTH, 4);  // Maps starting RAM address on LCD (if other than 1602)
// m_pLCD2004->disableCursor();             // We don't need to see a cursor on the LCD
// m_pLCD2004->backLightOn();
// delay(30);                           // About 15ms required to allow LCD to boot before clearing screen
// m_pLCD2004->clearScreen();               // FYI, won't execute as the *first* LCD command
// delay(100);                          // At 115200 baud, needs > 90ms after CLS before sending text.  No delay needed at 9600 baud.
// return;
