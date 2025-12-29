// PINBALL_FUNCTIONS.CPP Rev: 11/02/25.
// Declares and defines several functions that are global to all (or nearly all) Arduino modules.

#include "Pinball_Functions.h"
#include <Pinball_Centipede.h>

// pShiftRegister is a global defined in the sketches; declare it extern here.
extern Pinball_Centipede* pShiftRegister;

void initScreamoMasterArduinoPins() {

  // Set pin modes for all OUTPUT pins
  digitalWrite(PIN_OUT_LED, LOW);                              // Built-in LED LOW=off
  pinMode(PIN_OUT_LED, OUTPUT);

  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);        // Put RS485 in receive mode
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);                    // HIGH = RS485 transmit, LOW = not transmitting (receive)

  return;
}

void initScreamoSlaveArduinoPins() {

  // Set pin modes for all DIRECT INPUT pins
  pinMode(PIN_IN_SWITCH_CREDIT_EMPTY, INPUT_PULLUP);           // Direct input into Arduino for faster response
  pinMode(PIN_IN_SWITCH_CREDIT_FULL, INPUT_PULLUP);            // Direct input into Arduino for faster response

  // Set pin modes for all OUTPUT pins
  digitalWrite(PIN_OUT_LED, LOW);                              // Built-in LED LOW=off
  pinMode(PIN_OUT_LED, OUTPUT);

  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);        // Put RS485 in receive mode
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);                    // HIGH = RS485 transmit, LOW = not transmitting (receive)

  return;
}

// Force all Centipede outputs to their inactive state (defensive, idempotent).
// Lamps are active-LOW on your hardware, so HIGH = OFF.
void centipedeForceAllOff() {
  if (pShiftRegister == nullptr) {
    return;
  }
  for (int pin = 0; pin < 64; ++pin) {
    pShiftRegister->digitalWrite(pin, HIGH); // HIGH = lamp OFF
  }
}

void haltIfHaltPinPulledLow() {
  while (true) {}  // For a real halt, just loop forever.
  return;
}

void endWithFlashingLED(int t_numFlashes) {  // Works for all modules, including SWT and LEG (requires pointer to Pinball_Centipede)
  pShiftRegister->initScreamoMasterCentipedePins();  // Works for Master or Slave; Centipede 1 is output to relays and this releases them all
  // Infinite loop:
  while (true) {
    for (int i = 1; i <= t_numFlashes; i++) {
      digitalWrite(PIN_OUT_LED, HIGH);
      delay(100);
      digitalWrite(PIN_OUT_LED, LOW);
      delay(250);
    }
    delay(1000);
  }
}

void requestEmergencyStop() {
  return;
}

void chirp() {  // BE AWARE THAT THIS TAKES 10ms TO PERFORM!  SO WHEN USED, IT MAY SLOW AN IMPORTANT FUNCTION.
  return;
}

// BIT FUNCTIONS: These are versions I found online, but Arduino has built-in macros bitSet(), bitClear(), bitRead(), and
// bitWrite().  If mine don't work for some reason, try these.  I don't know why they are different.  I chose to use my own
// versions because 1. Visual Studio gave me a warning that they may not be supported (they are, but I'd rather not have the
// warning), and 2. because I prefer using functions versus "invisible" macros.
// Here are the macro definitions found in arduino.h:
// #define lowByte(w) ((byte) ((w) & 0xff))
// #define highByte(w) ((byte) ((w) >> 8))
// #define bitRead(value, bit) (((value) >> (bit)) & 0x01)
// #define bitSet(value, bit) ((value) |= (1UL << (bit)))
// #define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
// #define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
// #define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

// unsigned setBitAlternate(unsigned t_val, byte t_posn) {  // My version
//   // Set (1) a specific bit in a 16-bit integer.
//   t_val = (t_val & (~(1 << t_posn))) | (1 << t_posn);
//   return t_val;
// }
// unsigned clearBitAlternate(unsigned t_val, byte t_posn) {  // My version
//   // Clear (0) a specific bit in a 16-bit integer.
//   t_val = (t_val & (~(1 << t_posn))) | (0 << t_posn);
//   return t_val;
// }

byte getLowByte(unsigned t_val) {  // Based on Arduino.h macro
  // Returns 8-bit "low" byte of 16-bit unsigned integer.
  return (t_val & 0xff);
}

byte getHighByte(unsigned t_val) {  // Based on Arduino.h macro
  // Returns 8-bit "high" byte of 16-bit unsigned integer.
  return (t_val >> 8);
}

bool readBit(unsigned t_val, byte t_bit) {  // Based on Arduino.h macro
  // Returns true/false, if bit t_bit is set/cleared in 16-bit t_val
  return ((t_val) >> (t_bit) & 0x01);
}

unsigned setBit(unsigned t_val, byte t_bit) {  // Based on Arduino.h macro
  // Set (1) a specific bit in a 16-bit integer.
  return ((t_val) |= (1UL << (t_bit)));
}

unsigned clearBit(unsigned t_val, byte t_bit) {  // Based on Arduino.h macro
  // Clear (0) a specific bit in a 16-bit integer.
  return ((t_val) &= ~(1UL << (t_bit)));
}

unsigned toggleBit(unsigned t_val, byte t_bit) {  // Based on Arduino.h macro
  // Toggle a specific bit in a 16-bit integer.
  return ((t_val) ^= (1UL << (t_bit)));
}

unsigned writeBit(unsigned t_val, byte t_bit, byte t_bitVal) {  // Based on Arduino.h macro
  // Write a 0 or 1, as specified in t_bitVal, to bit number t_bit, in 16-bit integer t_val.
  return ((t_bitVal) ? setBit(t_val, t_bit) : clearBit(t_val, t_bit));
}

