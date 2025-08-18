// TRAIN_FUNCTIONS.CPP Rev: 05/23/24.
// Declares and defines several functions that are global to all (or nearly all) Arduino modules.
// 05/23/24: Always digitalWrite(pin, LOW) before pinMode(pin, OUTPUT) else will write high briefly.
// 04/15/24: Increased the False Halt delay from 1ms to 5ms; was getting too many false halts when pressing turnout buttons.
// 06/30/22: Removed pinMode and digitalWrite for FRAM; we'll do that in Hackscribble_Ferro class.

#include "Pinball_Functions.h"

void initScreamoMasterPins() {
  // First we'll set up pins that are appropriate for all modules.
  digitalWrite(PIN_OUT_LED, LOW);       // Built-in LED LOW=off
  pinMode(PIN_OUT_LED, OUTPUT);
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Put RS485 in receive mode
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);              // HIGH = RS485 transmit, LOW = not transmitting (receive)
  digitalWrite(PIN_OUT_RS485_TX_LED, LOW);               // Turn off the BLUE transmit LED
  pinMode(PIN_OUT_RS485_TX_LED, OUTPUT);                 // Set HIGH to turn on BLUE LED when RS485 is TRANSMITTING data
  digitalWrite(PIN_OUT_RS485_RX_LED, LOW);               // Turn off the YELLOW receive LED
  pinMode(PIN_OUT_RS485_RX_LED, OUTPUT);                 // Set HIGH to turn on YELLOW when RS485 is RECEIVING data
  return;
}

void initScreamoSlavePins() {
  // First we'll set up pins that are appropriate for all modules.
  digitalWrite(PIN_OUT_LED, LOW);       // Built-in LED LOW=off
  pinMode(PIN_OUT_LED, OUTPUT);
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Put RS485 in receive mode
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);              // HIGH = RS485 transmit, LOW = not transmitting (receive)
  digitalWrite(PIN_OUT_RS485_TX_LED, LOW);               // Turn off the BLUE transmit LED
  pinMode(PIN_OUT_RS485_TX_LED, OUTPUT);                 // Set HIGH to turn on BLUE LED when RS485 is TRANSMITTING data
  digitalWrite(PIN_OUT_RS485_RX_LED, LOW);               // Turn off the YELLOW receive LED
  pinMode(PIN_OUT_RS485_RX_LED, OUTPUT);                 // Set HIGH to turn on YELLOW when RS485 is RECEIVING data
  return;
}

void haltIfHaltPinPulledLow() {
  while (true) {}  // For a real halt, just loop forever.
  return;
}

void endWithFlashingLED(int t_numFlashes) {  // Works for all modules, including SWT and LEG (requires pointer to Pinball_Centipede)
  pShiftRegister->initializePinsForMaster();  // Works for Master or Slave; Centipede 1 is output to relays and this releases them all
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
// #define lowByte(w) ((uint8_t) ((w) & 0xff))
// #define highByte(w) ((uint8_t) ((w) >> 8))
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

unsigned int freeMemory () {  // Excellent little utility that returns the amount of unused SRAM.
  // Arduino memory:
  // 0x8FF: Top of SRAM
  // Stack grows down
  //                 <- SP
  // (free memory)
  //                 <- *__brkval
  // Heap (relocated)
  //                 <- &__bss_end
  // .bss  = not-yet-initialized global variables
  // .data = initialized global variables
  // 0x0100: Bottom of SRAM
  // SP will be replaced with: *((uint16_t volatile *) (0x3D))  // Bottom of stack (grows down)
  // extern unsigned int __heap_start, __heap_end;
  extern unsigned int __bss_start, __bss_end;  // __bss_end is lowest available address above globals
  extern unsigned int __data_start, __data_end;
  extern void *__brkval;          // Top of heap; should match __bss_end after moving heap.
 
  Serial.println(F("======================================================"));
  Serial.print  (F("__malloc_heap_end             should be 65,535 = ")); Serial.println(uint16_t(__malloc_heap_end));
  Serial.print  (F("__brkval (top of heap)                         = ")); Serial.println((unsigned int)__brkval);
  Serial.print  (F("__malloc_heap_start (base of heap shd be 8,704 = ")); Serial.println(uint16_t(__malloc_heap_start));
  Serial.print  (F("  HEAP SPACE USED -----------------------------> ")); 
    if (__brkval == 0) {
      Serial.println(0);
    } else {
      Serial.println(((int)__brkval) - uint16_t(__malloc_heap_start));
    }
  Serial.print  (F("  HEAP SPACE REMAINING ------------------------> "));
    Serial.println(uint16_t(__malloc_heap_end) - (unsigned int)__brkval);
  Serial.print  (F("Stack pointer (bottom of stack)    below 8,700 = ")); Serial.println((int)SP);
  Serial.print  (F("__bss_end (top of global data)                 = ")); Serial.println((int)&__bss_end);
  Serial.print  (F("__brkval boundary betw stack & orig heap       = ")); Serial.println((unsigned int)&__brkval);
  Serial.print  (F("__bss_start                                    = ")); Serial.println((unsigned int)&__bss_start);
  Serial.print  (F("__data_end                                     = ")); Serial.println((unsigned int)&__data_end);
  Serial.print  (F("__data_start                                   = ")); Serial.println((unsigned int)&__data_start);
  Serial.print  (F("  FREE MEMORY ---------------------------------> ")); Serial.println(SP - (int)&__bss_end);
  Serial.println(F("======================================================"));

  return (unsigned int)(SP - (int)&__bss_end);
}
