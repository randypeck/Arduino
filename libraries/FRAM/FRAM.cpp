// FRAM.CPP Rev: 11/12/22 
// 11/12/20: begin() no longer needs to pass Display_2004* t_pLCD2004 as a parm, so long as we #include <Train_Functions.h>
//           The original begin was: void FRAM::begin(Display_2004* t_pLCD2004) {
//                                   pLCD2004 = t_pLCD2004;
// 6/28/22: Updated to work with Adafruit 512KB FRAM breakout boards; add FRAM CS pin num parm back in.
// As of 11/10/20, constructor hard-coded MB85RS2MT, which was our home-built 256KB FRAM up until 6/2022.
// FRAM handles access to/from the FRAM memory chip used to store our tables -- behaves like a direct-access data file.
// Note that address length is 24 bits (for the 2Mb and 4Mb chips), so passing an unsigned long int is fine (it's 32 bits.)
// We want to call the constructor *above* setup() so that the object is global, and then call begin() in setup().

#include "FRAM.h"

FRAM::FRAM(ferroPartNumber t_partNum, byte t_pin):Hackscribble_Ferro (t_partNum, t_pin) {  // Constructor.
  // MB85RS4MT is new 2022 Adafruit 512KB break-out part number
  // MB85RS2MT was our previous 256KB home-built FRAM part number
  // Hackscribble_Ferro library uses standard Arduino SPI pin definitions:  MOSI, MISO, SCK.
  // Above contructor creates an instance of Ferro using the standard Arduino SS pin and FRAM part number.
  // You specify a FRAM part number and SS pin number.  Specify the chip as MB85RS4MT (previously MB85RS2MT, previously MB85RS64.)
  return;
}

void FRAM::begin() {  // Initialize the Hackscribble FRAM.  Must be called in setup() before using.
  ferroResult i = Hackscribble_Ferro::ferroBegin();
  // Here are the possible responses from Hackscribble_Ferro::ferroBegin():
  //    ferroOK = 0
  //    ferroBadStartAddress = 1
  //    ferroBadNumberOfBytes = 2
  //    ferroBadFinishAddress = 3
  //    ferroArrayElementTooBig = 4
  //    ferroBadArrayIndex = 5
  //    ferroBadArrayStartAddress = 6
  //    ferroBadResponse = 7
  //    ferroPartNumberMismatch = 8
  //    ferroUnknownError = 99
  if (i != ferroOK) {
    sprintf(lcdString, "BAD FERRO %i", i); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  // The following check should never fail unless the FRAM is bad.
  // No longer checking TOP and BOTTOM since it's different based on chip.
  if (Hackscribble_Ferro::checkForFRAM() != ferroOK) {
    sprintf(lcdString, "FERRO PROBLEM"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;  // ferroOK
}

ferroResult FRAM::read(unsigned long t_startAddress, byte t_numberOfBytes, byte* t_buffer) {
  return Hackscribble_Ferro::readFerro(t_startAddress, t_numberOfBytes, t_buffer);
}

ferroResult FRAM::write(unsigned long t_startAddress, byte t_numberOfBytes, byte* t_buffer) {
  return Hackscribble_Ferro::writeFerro(t_startAddress, t_numberOfBytes, t_buffer);
}

unsigned long FRAM::bottomAddress() {
  return FRAM::getBottomAddress();
}

unsigned long FRAM::topAddress() {
  return FRAM::getTopAddress();
}

void FRAM::setFRAMRevDate(byte t_month, byte t_day, byte t_year) {
  byte tBuf[3] = {t_month, t_day, t_year};  // i.e. 6, 18, 60
  FRAM::write(FRAM_ADDR_REV_DATE, sizeof(tBuf), tBuf);
}

void FRAM::checkFRAMRevDate() {
  // FRAM address FRAM_ADDR_REV_DATE holds three bytes: Month, Day, Year i.e. 11,27,20.
  // We'll check ferroResult here, and won't bother checking it again when we read/write FRAM.
  byte tBuf[3] = {0, 0, 0};  // Temporary 3-byte array to hold date retrieved from FRAM
  ferroResult i = FRAM::read(FRAM_ADDR_REV_DATE, sizeof(tBuf), tBuf);
  if (i == ferroOK) { sprintf(lcdString, "FRAM OKAY"); pLCD2004->println(lcdString); Serial.println(lcdString); }  // ******************************************************
  if (i != ferroOK) { sprintf(lcdString, "FRAM REV ERR %i", i); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1); }  // FRAM error other than bad date!
  sprintf(lcdString, "FRAM Rev %02i/%02i/%02i", tBuf[0], tBuf[1], tBuf[2]); pLCD2004->println(lcdString); Serial.println(lcdString);
  for (byte i = 0; i <= 2; i++) {  // Compare FRAM rev date versus Train_Consts_Global.h FRAMVERSION[3] { 00, 00, 00 }
    if (tBuf[i] != FRAMVERSION[i]) { sprintf(lcdString, "%.20s", "FRAM WRONG DATE!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1); }
  }
  return;
}

void FRAM::testFRAM() {
  // Write then read a random byte to every possible location.  We'll do this one byte at a time since speed isn't an issue.
  // Typically run this one time only for each new FRAM module.  Then when not compiled into a module it won't use any space.
  // Bottom address will be 0;  // First writable address.
  // Top address will be 524287 (512K - 1); was 262143 (256K - 1) until 6/22
  sprintf(lcdString, "FRAM test begin."); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "ALL DATA OVERWRITTEN"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Bottom addr: %lu", FRAM::getBottomAddress()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Top addr: %lu", FRAM::getTopAddress()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Time: %lums", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Write every addr..."); pLCD2004->println(lcdString); Serial.println(lcdString);
  long seed=1235;
  randomSeed(seed);  // Always start filling and checking with the same random seed
  unsigned long t_startAddress;
  byte byteToWrite = 0;
  byte * ptrByteToWrite = &byteToWrite;
  const byte numberOfBytesToWrite = 1;
  for (t_startAddress = FRAM::getBottomAddress(); t_startAddress <= FRAM::getTopAddress(); t_startAddress++) {
    if ((t_startAddress % 5000) == 0) {  // For every 1000 addresses written
      Serial.print(".");
    }
    byteToWrite = random(256);
    //sprintf(lcdString, "%lu %i", t_startAddress, byteToWrite); Serial.println(lcdString);
    FRAM::write(t_startAddress, numberOfBytesToWrite, ptrByteToWrite);
  }
  Serial.println();
  sprintf(lcdString, "Time: %lums", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Wrote %lu bytes", t_startAddress); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Now read every addr:"); pLCD2004->println(lcdString); Serial.println(lcdString);
  randomSeed(seed);
  byte byteRead = 0;
  byte * ptrByteRead = &byteRead;
  byte byteExpected = 0;
  const byte numberOfBytesToRead = 1;
  for (t_startAddress = FRAM::getBottomAddress(); t_startAddress <= FRAM::getTopAddress(); t_startAddress++) {
    if ((t_startAddress % 5000) == 0) {  // For every 1000 addresses written
      Serial.print(".");
    }
    byteExpected = random(256);
    FRAM::read(t_startAddress, numberOfBytesToRead, ptrByteRead);
    //sprintf(lcdString, "%lu %i %i", t_startAddress, byteExpected, byteRead); Serial.println(lcdString);
    if (byteRead != byteExpected) {
      sprintf(lcdString, "Fail %lu", t_startAddress); pLCD2004->println(lcdString); Serial.println(lcdString);
    }
  }
  Serial.println();
  sprintf(lcdString, "Read %lu bytes", t_startAddress); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Time: %lums", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "All good!"); pLCD2004->println(lcdString); Serial.println(lcdString);
  return;
}

void FRAM::testSRAM() {
  // Write then read a random byte to every possible location.  We only have 8K total so array will be small.
  const unsigned int ARRAY_SIZE = 4096;
  byte testArray[ARRAY_SIZE];
  sprintf(lcdString, "SRAM test begin."); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "ALL DATA OVERWRITTEN"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Write 4K bytes."); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Start: %lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  long seed=1235;
  randomSeed(seed);  // Always start filling and checking with the same random seed
  unsigned long t_Address = 0;
  byte byteToWrite = 0;
  for (t_Address = 0; t_Address < ARRAY_SIZE; t_Address++) {
    byteToWrite = random(256);
    testArray[t_Address] = byteToWrite;
  }
  sprintf(lcdString, "Wrote %lu", t_Address); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Time: %lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Read every addr:"); pLCD2004->println(lcdString); Serial.println(lcdString);
  randomSeed(seed);  // This will "re-start" the seed to exactly match the "random" numbers that were written
  byte byteExpected = 0;
  for (t_Address = 0; t_Address < ARRAY_SIZE; t_Address++) {
    byteExpected = random(256);
    if (testArray[t_Address] != byteExpected) {
      sprintf(lcdString, "Fail %lu", t_Address); pLCD2004->println(lcdString); Serial.println(lcdString);
    }
  }
  sprintf(lcdString, "Read %lu", t_Address); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Finished: %lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "All good!"); pLCD2004->println(lcdString); Serial.println(lcdString);
  return;
}

void FRAM::testQuadRAM(unsigned long t_numBytesToTest) {
  // 06/30/22: Write then read a random byte to every possible location.  We have about 56,650 bytes in QuadRAM heap.
  // The largest value that I've gotten to work is 56,647, which seems AOK.
  // This is really just a heap memory test; if we don't init QuadRAM then we've just got basic heap space
  unsigned long ARRAY_SIZE = t_numBytesToTest / 2;
  byte* pTestArray;
  byte* pTestArray2;
  pTestArray = new byte[ARRAY_SIZE];
  pTestArray2 = new byte[ARRAY_SIZE];
  sprintf(lcdString, "QuadRAM test begin."); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "ALL DATA OVERWRITTEN"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "%lu bytes", t_numBytesToTest); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Start: %lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  unsigned long seed=1235;
  randomSeed(seed);  // Always start filling and checking with the same random seed
  unsigned long t_Address = 0;
  byte byteToWrite = 0;
  for (t_Address = 0; t_Address < ARRAY_SIZE; t_Address++) {
    byteToWrite = random(256);
    pTestArray[t_Address] = byteToWrite;
    pTestArray2[t_Address] = byteToWrite;
  }
  sprintf(lcdString, "Wrote %lu", ARRAY_SIZE * 2); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Time: %lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Read every addr:"); pLCD2004->println(lcdString); Serial.println(lcdString);
  randomSeed(seed);  // This will "re-start" the seed to exactly match the "random" numbers that were written
  byte byteExpected = 0;
  for (t_Address = 0; t_Address < ARRAY_SIZE; t_Address++) {
    byteExpected = random(256);
    if ((pTestArray[t_Address] != byteExpected) || (pTestArray2[t_Address] != byteExpected)) {
      sprintf(lcdString, "Fail %lu", t_Address); pLCD2004->println(lcdString); Serial.println(lcdString);
    }
  }
  sprintf(lcdString, "Read %lu", ARRAY_SIZE * 2); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "Finished: %lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "All good!"); pLCD2004->println(lcdString); Serial.println(lcdString);
  //delete pTestArray;
  //delete pTestArray2;
  return;
}
