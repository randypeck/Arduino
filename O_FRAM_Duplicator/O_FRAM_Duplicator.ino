// FRAM_DUPLICATOR.INO Rev: 07/29/24.  FINISHED AND TESTED.
// 07/29/24: Commented out FRAM rev date check.
// Copies from one FRAM to another, in either direction depending on if digital pin is grounded or floating.
// Always performs a test to confirm both FRAMs are identical after copying.
// There's currently no way to just compare two FRAMs without first doing a copy one way or the other.
// DO NOT PLUG IN OR UNPLUG A FRAM WITH POWER CONNECTED TO THE ARDUINO!
// The reason we have a "direction of copy" is because it's such a hassle to populate the first updated FRAM, since we need to
// re-compile + re-run the O_FRAM_Populator.ino several times in order to get all of the FRAM tables populated, due to memory.
// So rather than have to go through so many steps for each of the three FRAMs (MAS, LEG, and OCC) it will be easier to just do
// all that work once to create the MAS FRAM, and then use this utility to copy the MAS FRAM contents to the built-in "master",
// and then copy from the Master to each of the remaining two FRAMs (LEG and OCC.)
 
//   1. Use an Arduino loaded with the O_FRAM_Populator program to populate O_MAS's FRAM.  This takes a long time since we need to
//      re-compile + re-run several times to populate all the tables (esp. Route Reference, which can populate only 25 at a time.)
//   2. DISCONNECT POWER from *both* the Populator Arduino *and* from the FRAM Duplicator Arduino.
//   3. Remove the newly-populated O_MAS FRAM from the Populator Arduino and plug it into the "COPY" socket of the FRAM Duplicator.
//   4. Set FRAM Duplicator swith to the "To Master" position to copy from the newly-created FRAM to the permanent "Master" FRAM.
//   5. Turn on power and let the program transfer the contents of the freshly-populated O_MAS FRAM to the Master FRAM.
//   6. TURN OFF POWER, then unplug the O_MAS FRAM and plug in the O_LEG FRAM into the "COPY" socket.
//   7. Set FRAM Duplicator switch to the "To Copy" position to copy from the "Master" FRAM to the O_LEG FRAM.
//   8. Turn on power and let the program run, to transfer the contents of the Master FRAM to the O_LEG FRAM.
//   9. TURN OFF POWER, then unplug the O_LEG FRAM and plug in the O_OCC FRAM into the "COPY" socket.
//  10. Leave FRAM Duplicator switch in the "To Copy" position.
//  11. Turn on power and let the program run, to transfer the contents of the Master FRAM to the O_OCC FRAM.
//  12. TURN OFF POWER, then unplug the O_OCC FRAM.

// PIN 10: If GROUNDED, will transfer contents of COPY FRAM to MASTER FRAM (to create a new Master from a freshly-populated FRAM.)
//         If FLOATING, will transfer contents of MASTER FRAM to COPY FRAM (normal duplication mode to clone MASTER to COPY.)

// Comment the following out if you don't want to wait for an initial lengthy byte-by-byte test of the FRAM being copied to.
// This can be skipped unless there is a question about the FRAM, or if it's never been used before.
//#define PERFORM_FRAM_TEST_BEFORE_COPY

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_NUL;  // Global just needs to be defined for use by Train_Functions.cpp and Message.cpp.
char lcdString[LCD_WIDTH + 1] = "DUP 02/27/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorageMaster = nullptr;
FRAM* pStorageCopy     = nullptr;
const byte PIN_IO_FRAM_2_CS    =  2;  // Output: FRAM #2 chip select pin.
                                      // Note: PIN_IO_FRAM_CS is the primary FRAM's CS pin defined in Train_Consts_Global.h
const byte PIN_IO_COPY_MODE    = 10;  // Leave digital pin 0 floating for normal operation; ground reads FROM "duplicate" FRAM.
const byte PIN_IO_START_BUTTON = 11;  // When START button is pressed, will ground pin 11.
const byte PIN_IO_START_LED    = 12;  // When pin 12 pulled LOW, START button's LED will turn on

const byte FRAM_MODE_READ   =  1;  // Value doesn't have any special meaning; just different than _WRITE
const byte FRAM_MODE_WRITE  =  2;  //                                "                            _READ
byte copyMode;                     // Will become either FRAM_MODE_READ or FRAM_MODE_WRITE

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();  // Does not initialize either FRAM chip select pin; that's done in Hackscribble_Ferro.cpp.
  pinMode(PIN_IO_COPY_MODE, INPUT_PULLUP);     // Leave floating for normal operation; ground to copy from "duplicate" to master FRAM.
  pinMode(PIN_IO_START_BUTTON, INPUT_PULLUP);  // This pin normally floating; will be pulled low when START button pressed
  digitalWrite(PIN_IO_START_LED, HIGH);
  pinMode(PIN_IO_START_LED, OUTPUT);           // Pull low to illuminate START button's built-in LED

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 instantiated via Display_2004/LCD2004.

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);

  // *** INITIALIZE FRAM CLASS AND OBJECT *** (Heap uses 76 bytes)
  // Must pass FRAM part num and Arduino pin num as parms to constructor (vs begin) because needed by parent Hackscribble_Ferro.
  // MB85RS4MT is 512KB (4Mb) SPI FRAM.  Our original FRAM was only 8KB SPI.
  pStorageMaster = new FRAM(MB85RS4MT, PIN_IO_FRAM_CS);     // Instantiate the ORIGINAL FRAM and assign the global pointer
  pStorageMaster->begin();                               // Will crash on its own if there is any problem with the FRAM
  pStorageCopy = new FRAM(MB85RS4MT, PIN_IO_FRAM_2_CS);  // Instantiate the DUPLICATE FRAM and assign the global pointer
  pStorageCopy->begin();                                 // Will crash on its own if there is any problem with the FRAM

  // *** COPY MODE MUST BE DETERMINED UPON POWER UP ***
  // So that our LCD Message prior to starting copy will be displayed correctly
  if (digitalRead(PIN_IO_COPY_MODE) == LOW) {  // If the "mode" pin is grounded...
    // User has plugged a freshly-populated FRAM and wants to transfer it to the Master Copy FRAM...  Do this FIRST.
    // Do this ONE TIME ONLY, after populating MAS's FRAM via O_FRAM_Populator.ino, to transfer to the Master Copy FRAM.
    copyMode = FRAM_MODE_READ;
    sprintf(lcdString, "XFER COPY TO MASTER."); pLCD2004->println(lcdString); Serial.println(lcdString);
  } else {                                     // If the "mode" pin is floating...
    // User has plugged in a "blank" FRAM and wants to populate it with contents from the Master FRAM...  Do this NEXT.
    // Do this TWO TIMES -- once for LEG and once for OCC (MAS has already been freshly populated by O_FRAM_Populator.ino.)
    copyMode = FRAM_MODE_WRITE;  // i.e. write from Master FRAM to duplicate (normal "duplicate a FRAM" operation)
    sprintf(lcdString, "XFER MASTER TO COPY."); pLCD2004->println(lcdString); Serial.println(lcdString);
  }

  // *** ILLUMINATE START BUTTON AND WAIT FOR USER TO PRESS START ***
  digitalWrite(PIN_IO_START_LED, LOW);
  sprintf(lcdString, "PRESS START..."); pLCD2004->println(lcdString); Serial.println(lcdString);
  while (digitalRead(PIN_IO_START_BUTTON) != LOW) {}
  digitalWrite(PIN_IO_START_LED, HIGH);  // Turn off START button LED while we are copying.

  duplicateFRAM();

  if (confirmDuplicateFRAM()) {
    sprintf(lcdString, "TRANSFER COMPLETE!"); pLCD2004->println(lcdString); Serial.println(lcdString);
  } else {
    sprintf(lcdString, "DUPLICATION ERROR!"); pLCD2004->println(lcdString); Serial.println(lcdString);
  }

  // Turn START button LED back on to indicate we're done here
  digitalWrite(PIN_IO_START_LED, LOW);  // Turn off START button LED while we are copying.

}

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {  // Nothing to do here

}

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

void duplicateFRAM() {
  // Rev: 03/06/23.
  // I could embed this as a new function in FRAM.h/FRAM.cpp, but with two separate FRAMs and this being the only code that will
  // ever duplicate a FRAM, we'll just keep it local to FRAM_Duplicator.ino.
  // Each 512KB FRAM (being used as of 3/1/23) has 524,288 bytes: 0..524,287.  This is 2,048 groups of 256 bytes.
  // Since I must specify the number of bytes to read/write as a single byte value, the maximum number of bytes to read/write at
  // once is 255.  Thus, I'll just do this in groups of 128 bytes for a nice round number.
  // So each 512KB FRAM has 4,096 groups of 128 bytes.

  unsigned long bottomAddress        = pStorageMaster->bottomAddress();  // 0
  unsigned long topAddress           = pStorageMaster->topAddress();     // 524,287 (524,288 - 1)
  byte          bytesToTransfer      = 128;                                // Number of bytes to read/write at a single go

  byte dataBuffer[bytesToTransfer];
  byte* pDataBuffer = dataBuffer;  // Same as saying &dataBuffer[0]

  if (copyMode == FRAM_MODE_READ) {  // Oh, we want to transfer from our "copy" into the master FRAM

    //pStorageCopy->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
    #ifdef PERFORM_FRAM_TEST_BEFORE_COPY
      // Test the Master FRAM (that's about to be overwritten) just for fun.  Don't test the COPY FRAM or you'll wipe the data!
      sprintf(lcdString, "Test MASTER FRAM..."); pLCD2004->println(lcdString); Serial.println(lcdString);
      pStorageMaster->testFRAM();  // Writes then reads random data to entire FRAM.
      // If we return from the test, it tested okay.
    #endif
    sprintf(lcdString, "Copying..."); pLCD2004->println(lcdString); Serial.println(lcdString);

    for (unsigned long FRAMAddress = bottomAddress; FRAMAddress < topAddress; FRAMAddress = FRAMAddress + bytesToTransfer) {
      if ((FRAMAddress % 16384) == 0) Serial.print(".");  // Occasionally print a dot to show progress is being made
      // Serial.println(FRAMAddress);  // This shows addresses written are 0, 128, 256, 384, ... 524,160 -- as expected.
      pStorageCopy->read(FRAMAddress, bytesToTransfer, pDataBuffer);
      pStorageMaster->write(FRAMAddress, bytesToTransfer, pDataBuffer);
    }

  } else if (copyMode == FRAM_MODE_WRITE) {  // Okay, this is our "normal" copy-from-master-to-duplicate FRAM mode

    pStorageMaster->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
    #ifdef PERFORM_FRAM_TEST_BEFORE_COPY
      // Test the COPY FRAM (that's about to be overwritten) just for fun.  Don't test the MASTER FRAM or you'll wipe the data!
      sprintf(lcdString, "Test COPY FRAM..."); pLCD2004->println(lcdString); Serial.println(lcdString);
      pStorageCopy->testFRAM();  // Writes then reads random data to entire FRAM.
      // If we return from the test, it tested okay.
    #endif
    sprintf(lcdString, "Copying..."); pLCD2004->println(lcdString); Serial.println(lcdString);
    for (unsigned long FRAMAddress = bottomAddress; FRAMAddress < topAddress; FRAMAddress = FRAMAddress + bytesToTransfer) {
      if ((FRAMAddress % 16384) == 0) Serial.print(".");  // Occasionally print a dot to show progress is being made
      // Serial.println(FRAMAddress);  // This shows addresses written are 0, 128, 256, 384, ... 524,160 -- as expected.
      pStorageMaster->read(FRAMAddress, bytesToTransfer, pDataBuffer);
      pStorageCopy->write(FRAMAddress, bytesToTransfer, pDataBuffer);
    }

  } else {  // Yikes we have a big problem!

    sprintf(lcdString, "BAD MODE! %c", copyMode); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);

  }

  return;

}

bool confirmDuplicateFRAM() {

  sprintf(lcdString, "Comparing..."); pLCD2004->println(lcdString); Serial.println(lcdString);

  unsigned long bottomAddress        = pStorageMaster->bottomAddress();  // 0
  unsigned long topAddress           = pStorageMaster->topAddress();     // 524,287 (524,288 - 1)
  byte          bytesToTransfer      = 128;                                // Number of bytes to read/write at a single go

  byte dataBuffer1[bytesToTransfer];
  byte dataBuffer2[bytesToTransfer];
  byte* pDataBuffer1 = dataBuffer1;  // Same as saying &dataBuffer1[0]
  byte* pDataBuffer2 = dataBuffer2;  // Same as saying &dataBuffer2[0]

  for (unsigned long FRAMAddress = bottomAddress; FRAMAddress < topAddress; FRAMAddress = FRAMAddress + bytesToTransfer) {
    if ((FRAMAddress % 16384) == 0) Serial.print(".");  // Occasionally print a dot to show progress is being made
    // Serial.println(FRAMAddress);  // This shows addresses written are 0, 128, 256, 384, ... 524,160 -- as expected.
    pStorageCopy->read(FRAMAddress, bytesToTransfer, pDataBuffer1);
    pStorageMaster->read(FRAMAddress, bytesToTransfer, pDataBuffer2);
    for (byte i = 0; i < bytesToTransfer; i++) {  // For bytes 0..127 of the buffers we just read
      if (pDataBuffer1[i] != pDataBuffer2[i]) {
        return false;
      }
    }
  }
  return true;  // Returns true if duplicate checks out okay
}
