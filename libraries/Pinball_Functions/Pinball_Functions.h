// PINBALL_FUNCTIONS.H Rev: 08/18/25.
// Not a class, just a group of functions.

// 02/20/23: Eliminated pLCD2004 and pShiftRegister from parms being passed to classes, *and* as "extern" in class.h files,
// because that's redundant. They're declared extern here in Train_Functions.h, which is #included in every .ini and .h file.
// Also don't need any local m_pLCD2004 or m_pShiftRegister variables in class .h files.

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

void initScreamoMasterArduinoPins();
void initScreamoSlaveArduinoPins();
void centipedeForceAllOff();
void haltIfHaltPinPulledLow();
void endWithFlashingLED(int t_numFlashes);
void requestEmergencyStop();
void chirp();

// BIT FUNCTIONS: I found a couple alternates online, but Arduino.h has built-in macros for all kinds of bit manipulation.
// I built functions based on the Arduino.h macros, because: 1. Visual Studio gave me a warning that they may not be supported
// (they are, but I'd rather not have the warning), and 2. because I prefer using functions versus "invisible" macros.
// Here are the macro definitions found in arduino.h:
// #define lowByte(w) ((unsigned int) ((w) & 0xff))
// #define highByte(w) ((unsigned int) ((w) >> 8))
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

#endif
