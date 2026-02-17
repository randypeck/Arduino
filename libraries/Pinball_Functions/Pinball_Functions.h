// PINBALL_FUNCTIONS.H Rev: 02/15/26.
// Not a class, just a group of functions.

#ifndef PINBALL_FUNCTIONS_H
#define PINBALL_FUNCTIONS_H

#include <Arduino.h>  // Allows use of "byte" and other Arduino-specifics
#include <Pinball_Consts.h>

extern const byte THIS_MODULE;  // Will be set to ARDUINO_MAS or ARDUINO_SLV for each module.

// Since every module has a pLCD2004 pointer, we'll just make it global/extern so we don't have to always pass it around.
// Similarly, in lieu of sending an lcdString[] pointer with every call to this class, we'll just "extern" it as a global here.
#include <Pinball_LCD.h>
extern Pinball_LCD* pLCD2004;  // We must still declare/define pLCD2004 to point at the class in the main module.
extern char lcdString[];  // Global array to hold strings sent to Digole 2004 LCD; last char is for null terminator.

// Note: <Wire.h> is already included in <Pinball_Centipede.h> so not needed here.
#include <Pinball_Centipede.h>
extern Pinball_Centipede* pShiftRegister;

void initScreamoMasterArduinoPins();
void initScreamoSlaveArduinoPins();
void centipedeForceAllOff();
void endWithFlashingLED(int t_numFlashes);

#endif
