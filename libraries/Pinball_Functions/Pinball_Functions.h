// PINBALL_FUNCTIONS.H
// Not a class, just a group of functions.

// 02/20/23: Eliminated pLCD2004 and pShiftRegister from parms being passed to classes, *and* as "extern" in class.h files,
// because that's redundant. They're declared extern here in Train_Functions.h, which is #included in every .ini and .h file.
// Also don't need any local m_pLCD2004 or m_pShiftRegister variables in class .h files.

// Header (.h) files should contain:
// * Header guards #ifndef/#define/#endif
// * Source code documentation i.e. purpose of parms, and return values of functions.
// * Type and constant DEFINITIONS.  Note this does not consume memory if not used.
// * External variable, structure, function, and object DECLARATIONS
// Large numbers of structure declarations should be split into separate headers.
// Struct declarations do not take up any memory, so no harm done if they get included with code that never defines them.
// Structure definitions can be in a corresponding .cpp file, *or* in the main source file (probably better for me.)
// enum and constants should be DEFINED not just declared in the .h file.

#ifndef PINBALL_FUNCTIONS_H
#define PINBALL_FUNCTIONS_H

#include <Arduino.h>  // Allows use of "byte" and other Arduino-specifics
#include <Pinball_Consts.h>

extern const byte THIS_MODULE;  // Will be set to O_MAS, O_LEG, O_SWT, etc. for each module.

// Since every module has a pLCD2004 pointer, we'll just make it global/extern so we don't have to always pass it around.
// Similarly, in lieu of sending an lcdString[] pointer with every call to this class, we'll just "extern" it as a global here.
// I am *not* doing a similar extern for FRAM because not every module needs FRAM and thus won't have a pStorage pointer.
#include <Pinball_LCD.h>
extern Pinball_LCD* pLCD2004;  // We must still declare/define pLCD2004 to point at the class in the main module i.e. O_LEG.
extern char lcdString[];  // Global array to hold strings sent to Digole 2004 LCD; last char is for null terminator.

// Note: <Wire.h> is already included in <Pinball_Centipede.h> so not needed here.
#include <Pinball_Centipede.h>
extern Pinball_Centipede* pShiftRegister;

extern void wdt_disable();  // Used by SWT to release all turnout solenoids in haltIfHaltPinPulledLow()

void initScreamoMasterPins();
void initScreamoSlavePins();
void haltIfHaltPinPulledLow();
void endWithFlashingLED(int t_numFlashes);
void requestEmergencyStop();
void chirp();
void requestLegacyHalt();

void initializeQuadRAM();
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

// BIT FUNCTIONS: I found a couple alternates online, but Arduino.h has built-in macros for all kinds of bit manipulation.
// I built functions based on the Arduino.h macros, because: 1. Visual Studio gave me a warning that they may not be supported
// (they are, but I'd rather not have the warning), and 2. because I prefer using functions versus "invisible" macros.
// Here are the macro definitions found in arduino.h:
// #define lowByte(w) ((uint8_t) ((w) & 0xff))
// #define highByte(w) ((uint8_t) ((w) >> 8))
// #define bitRead(value, bit) (((value) >> (bit)) & 0x01)
// #define bitSet(value, bit) ((value) |= (1UL << (bit)))
// #define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
// #define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
// #define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))
byte getLowByte(unsigned t_val);
byte getHighByte(unsigned t_val);
bool readBit(unsigned t_val, byte t_bit);
unsigned setBit(unsigned t_val, byte t_bit);
unsigned clearBit(unsigned t_val, byte t_bit);
unsigned toggleBit(unsigned t_val, byte t_bit);
unsigned writeBit(unsigned t_val, byte t_bit, byte t_bitVal);

unsigned int freeMemory ();  // Excellent little utility that returns the amount of unused SRAM.

#endif
