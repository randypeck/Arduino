// FRAM.H Rev: 11/12/22.
// 11/12/20: begin() no longer needs to pass Display_2004* t_pLCD2004 as a parm, so long as we #include <Train_Functions.h>
//           The original begin was: void FRAM::begin(Display_2004* t_pLCD2004)
// 06/30/22: Added chip select pin to FRAM and Hackscribble_Ferro classes, so we can one again use more than one (for copy util.)
// 12/17/20: Added FRAM and SRAM speed tests to basic FRAM integrity test.

// FRAM handles access to/from the FRAM memory chip used to store our tables -- behaves like a direct-access data file.
// It simplifies use of the FRAM chip by encapsulating all of the initialization and access logic within the class.
// FRAM accesses the chip via the Hackscribble class library.
// FRAM is a child class of parent Hackscribble_Ferro, in order to be able to make both of them static, via a
// single instantiation from the main program.
// The byte buffer t_buffer can be up to 255 (not 256) bytes long (limited because t_numberOfBytes is a byte value.)

// As of 12/09/22, ONLY MAS, LEG, and OCC need FRAMand QuadRAM.  No other modules need either one.
// LED(green turnouts) does not need anything in FRAM or QuadRAM - it just illuminates turnout orientation by monitoring RS-485
//   messages to SWT and using built - in table.
// SNS(occupancy sensors) don't need anything in FRAM or QuadRAM - it just reports trips and clears via RS-485 messages.
// BTN(button presses) doesn't need to look anything up - it just reports presses (based on mode/state) via RS-485 messages.
// SWT(throw turnouts) doesn't need to look anything up - it just throws turnouts blindly per incoming RS-485 messages.
// FRAM: Anyone who needs Sensor, Turnout/Block Res'n/Ref, Deadlocks, Loco Ref, or Route Ref: MAS, LEG, and OCC only.
// QuadRAM(Heap) : Anyone who needs Train Progress and Delayed Action: MAS, LEG, and OCC only.
//   Maybe SNS if needed during reg'n but I don't see how???  But NOT LED or SNS for TRAIN PROGRESS; don't need the features I was
//   considering.  I *did* modify common objects to reside on the heap : LCD, FRAM, RS-485 in addition to Loco Ref, Block & Turnout
//   Res'n, Route Ref, Deadlocks, etc.  But may still not need QuadRAM on every Arduino, because as long as we don't init QuadRAM,
//   it will just be placed on original heap space.  So until / unless those modules run out of SRAM in other than MAS/LEG/OCC, we
//   won't need QuadRAM for those modules.
//   HOWEVER, if LED, SNS, SWT, or BTN ever do have any memory problems, we can simply install a QuadRAM and add ONE LINE to
//   initialize it in setup() and all original heap space will be freed up for more SRAM.

// Here are the results of write/read tests for SRAM, FRAM, and QuadRAM (tested 12/18/20).
// For each test, I also tested the speed of the test loop without doing the actual read or write, to try to establish the actual
// reading and writing time without the overhead of the loop -- which would be a more realistic number if writing blocks of
// multiple blocks at a time.  So the final number on the right, in bytes/msec, is my best guess of each media's approx. speed.
// CONCLUSION: QuadRAM and FRAM are so fast, they are virtually the same speed as SRAM.  So do what's easiest, not what's fastest.
//
// SRAM    reads    4,096 bytes in  0.404 seconds = (10.14 bytes/msec) = 0.09862 msec/byte - 0.05760 = 0.04102 msec/byte "pure" read time  (24.4 bytes/msec)
// QuadRAM reads  524,288 bytes in 74.150 seconds = ( 7.07 bytes/msec) = 0.14144 msec/byte - 0.10081 = 0.04063 msec/byte "pure" read time  (24.6 bytes/msec)
// FRAM    reads  262,144 bytes in 28.796 seconds = ( 9.10 bytes/msec) = 0.10989 msec/byte - 0.05760 = 0.05229 msec/byte "pure" read time  (19.1 bytes/msec)

// SRAM    writes   4,096 bytes in  0.390 seconds = (10.50 bytes/msec) = 0.09524 msec/byte - 0.05568 = 0.03956 msec/byte "pure" write time (25.3 bytes/msec)
// QuadRAM writes 524,288 bytes in 70.656 seconds = ( 7.42 bytes/msec) = 0.13477 msec/byte - 0.09461 = 0.04016 msec/byte "pure" write time (24.9 bytes/msec)
// FRAM    writes 262,144 bytes in 30.518 seconds = ( 8.59 bytes/msec) = 0.11641 msec/byte - 0.05568 = 0.06073 msec/byte "pure" write time (16.5 bytes/msec)
//
// SRAM loop with NO read or write array element (to subtract the overhead from the actual read/write time):
// SRAM    writes   4,096 bytes in  0.228 seconds = 17.96 bytes/msec = 0.05568 msec/byte
// SRAM    reads    4,096 bytes in  0.236 seconds = 17.36 bytes/msec = 0.05760 msec/byte
// QuadRAM loop with NO read or write element (to subtract the overhead from the actual read/write time):
// QuadRAM writes 524,288 bytes in 49.606 seconds = 10.57 bytes/msec = 0.09461 msec/byte
// QuadRAM reads  524,288 bytes in 52.839 seconds =  9.92 bytes/msec = 0.10081 msec/byte

#ifndef FRAM_H
#define FRAM_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <SPI.h>                          // FRAM is connected via SPI interface
#include <Hackscribble_Ferro.h>           // This is the library that manages the FRAM hardware

class FRAM : private Hackscribble_Ferro {

  public:

    FRAM(ferroPartNumber t_partNum, byte t_pin);  // Constructor just needs the CS/SS pin number for the FRAM chip
    void begin();  // Initialize the Hackscribble FRAM.  Must be called in setup() before using.
    ferroResult read(unsigned long t_startAddress, byte t_numberOfBytes, byte* t_buffer);   // General FRAM read, max 255 bytes.
    ferroResult write(unsigned long t_startAddress, byte t_numberOfBytes, byte* t_buffer);  // General FRAM write, max 255 bytes.
    unsigned long bottomAddress();
    unsigned long topAddress();
    void setFRAMRevDate(byte t_month, byte t_day, byte t_year);
    void checkFRAMRevDate();
    void testFRAM();  // Write then read random byte to every location.
    void testSRAM();  // Write then read random byte to SRAM just to compare read/write speeds to FRAM and QuadRAM
    void testQuadRAM(unsigned long t_numBytesToTest);  // Maximum about 56,647 "testable" bytes with QuadRAM

  protected:

  private:

};

#endif

