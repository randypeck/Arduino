// TRAIN_FUNCTIONS.CPP Rev: 05/23/24.
// Declares and defines several functions that are global to all (or nearly all) Arduino modules.
// 05/23/24: Always digitalWrite(pin, LOW) before pinMode(pin, OUTPUT) else will write high briefly.
// 04/15/24: Increased the False Halt delay from 1ms to 5ms; was getting too many false halts when pressing turnout buttons.
// 06/30/22: Removed pinMode and digitalWrite for FRAM; we'll do that in Hackscribble_Ferro class.

#include "Train_Functions.h"

void initializePinIO() {

  // First we'll set up pins that are appropriate for all modules.
  digitalWrite(PIN_IO_HALT, HIGH);      // Monitor if gets pulled LOW it means someone tripped HALT.
  pinMode(PIN_IO_HALT, INPUT_PULLUP);   // Or change pinMode to OUTPUT and pull low to trigger HALT from here.
  digitalWrite(PIN_OUT_SPEAKER, HIGH);  // Piezo buzzer connects positive here
  pinMode(PIN_OUT_SPEAKER, OUTPUT);
  digitalWrite(PIN_OUT_LED, LOW);       // Built-in LED LOW=off
  pinMode(PIN_OUT_LED, OUTPUT);
  digitalWrite(PIN_OUT_RS485_TX_ENABLE, RS485_RECEIVE);  // Put RS485 in receive mode
  pinMode(PIN_OUT_RS485_TX_ENABLE, OUTPUT);              // HIGH = RS485 transmit, LOW = not transmitting (receive)
  digitalWrite(PIN_OUT_RS485_TX_LED, LOW);               // Turn off the BLUE transmit LED
  pinMode(PIN_OUT_RS485_TX_LED, OUTPUT);                 // Set HIGH to turn on BLUE LED when RS485 is TRANSMITTING data
  digitalWrite(PIN_OUT_RS485_RX_LED, LOW);               // Turn off the YELLOW receive LED
  pinMode(PIN_OUT_RS485_RX_LED, OUTPUT);                 // Set HIGH to turn on YELLOW when RS485 is RECEIVING data

  // Now set up pins that are unique to MAS.
  if (THIS_MODULE == ARDUINO_MAS) {
    pinMode(PIN_IN_REQ_TX_LEG, INPUT_PULLUP);      // MAS input pulled LOW by LEG when it wants to send MAS a message.
    pinMode(PIN_IN_REQ_TX_SNS, INPUT_PULLUP);      // MAS input pulled LOW by SNS when it wants to send MAS an occ sensor chg msg.
    pinMode(PIN_IN_REQ_TX_BTN, INPUT_PULLUP);      // MAS input pulled LOW by BTN to signal MAS a turnout btn has been pressed.
    pinMode(PIN_IN_ROTARY_MANUAL, INPUT_PULLUP);   // Pulled LOW when selected on control panel.  Same with following...
    pinMode(PIN_IN_ROTARY_REGISTER, INPUT_PULLUP);
    pinMode(PIN_IN_ROTARY_AUTO, INPUT_PULLUP);
    pinMode(PIN_IN_ROTARY_PARK, INPUT_PULLUP);
    pinMode(PIN_IN_ROTARY_POV, INPUT_PULLUP);
    digitalWrite(PIN_OUT_ROTARY_LED_MANUAL, HIGH); // Pulled LOW to turn on LED.  Same with following...
    pinMode(PIN_OUT_ROTARY_LED_MANUAL, OUTPUT);
    digitalWrite(PIN_OUT_ROTARY_LED_REGISTER, HIGH);
    pinMode(PIN_OUT_ROTARY_LED_REGISTER, OUTPUT);
    digitalWrite(PIN_OUT_ROTARY_LED_AUTO, HIGH);
    pinMode(PIN_OUT_ROTARY_LED_AUTO, OUTPUT);
    digitalWrite(PIN_OUT_ROTARY_LED_PARK, HIGH);
    pinMode(PIN_OUT_ROTARY_LED_PARK, OUTPUT);
    digitalWrite(PIN_OUT_ROTARY_LED_POV, HIGH);    // Unsupported mode, will stay dark (high)
    pinMode(PIN_OUT_ROTARY_LED_POV, OUTPUT);
    pinMode(PIN_IN_ROTARY_START, INPUT_PULLUP);    // Pulled LOW when button pressed...
    pinMode(PIN_IN_ROTARY_STOP, INPUT_PULLUP);
    digitalWrite(PIN_OUT_ROTARY_LED_START, HIGH);  // Pull LOW to turn on LED.
    pinMode(PIN_OUT_ROTARY_LED_START, OUTPUT);
    digitalWrite(PIN_OUT_ROTARY_LED_STOP, HIGH);
    pinMode(PIN_OUT_ROTARY_LED_STOP, OUTPUT);
  }

  // Now set up pins that are unique to LEG.
  if (THIS_MODULE == ARDUINO_LEG) {
    pinMode(PIN_OUT_REQ_TX_LEG, INPUT_PULLUP);     // LEG change to output and pull LOW to send MAS a Legacy-related message
    pinMode(PIN_IN_PANEL_BROWN_ON, INPUT_PULLUP);  // Pulled LOW when selected on control panel.  Same with following...
    pinMode(PIN_IN_PANEL_BROWN_OFF, INPUT_PULLUP);
    pinMode(PIN_IN_PANEL_BLUE_ON, INPUT_PULLUP);
    pinMode(PIN_IN_PANEL_BLUE_OFF, INPUT_PULLUP);
    pinMode(PIN_IN_PANEL_RED_ON, INPUT_PULLUP);
    pinMode(PIN_IN_PANEL_RED_OFF, INPUT_PULLUP);
    pinMode(PIN_IN_PANEL_4_ON, INPUT_PULLUP);
    pinMode(PIN_IN_PANEL_4_OFF, INPUT_PULLUP);
  }

  // Now set up pins that are unique to BTN.
  if (THIS_MODULE == ARDUINO_BTN) {
    digitalWrite(PIN_OUT_REQ_TX_BTN, HIGH);
    pinMode(PIN_OUT_REQ_TX_BTN, OUTPUT);           // When a button has been pressed, tell MAS by pulling this pin LOW
  }

  // Now set up pins that are unique to SNS.
  if (THIS_MODULE == ARDUINO_SNS) {
    digitalWrite(PIN_OUT_REQ_TX_SNS, HIGH);
    pinMode(PIN_OUT_REQ_TX_SNS, OUTPUT);           // When a sensor has been tripped or cleared, tell MAS by pulling this pin LOW
  }

  // Now set up pins that are unique to OCC.
  if (THIS_MODULE == ARDUINO_OCC) {
    pinMode(PIN_IN_WAV_STATUS, INPUT_PULLUP);      // LOW when OCC WAV Trigger track is playing, else HIGH.
    pinMode(PIN_IN_ROTARY_1, INPUT_PULLUP);        // Input: Rotary Encoder pin 1 of 2 (plus Select)
    pinMode(PIN_IN_ROTARY_2, INPUT_PULLUP);        // Input: Rotary Encoder pin 2 of 2 (plus Select)
    pinMode(PIN_IN_ROTARY_PUSH, INPUT_PULLUP);     // Input: Rotary Encoder "Select" (pushbutton) pin
  }
  return;
}

void initializeQuadRAM() {
  // *** QUADRAM EXTERNAL SRAM MODULE ***
  // 05/24/24: Changed four pinMode/digitalWrite commands to write LOW *before* setting pinMode; i.e. reversed order of commands.
  //           This is because if you set pinMode(pin, OUTPUT) before digitalWrite(pin, LOW), you'll output an active high briefly.
  // Currently used as heap memory, only on MAS, OCC, and LEG, to free up some SRAM space.
  // QuadRAM: Enable external memory with no wait states.  https://www.rugged-circuits.com/quadram-tech
  // When external RAM is enabled, pins PA0-PA7 (digital pins D22 through D29), PC0-PC7 (digital pins D30 through D37),
  // PG0-PG2 (digital pins D39 through D41), PL5-PL7 (digital pins D44 through D42) and PD7 (digital pin D38) are reserved for the
  // external memory interface.
  // MEMORY MAP:
  // External SRAM             : 0x2200 - 0xFFFF (8,704 - 65,535) (56,832 bytes)
  // Internal SRAM             : 0x0200 - 0x21FF (  512 -  8,703) ( 8,191 bytes)
  // 416 External I/O Registers: 0x0060 - 0x01FF (   96 -    511)
  // 64 I/O Registers          : 0x0020 - 0x005F (   32 -     95)
  // 32 Registers              : 0x0000 - 0x001F (    0 -     31)
  // For QuadRAM: Define external SRAM start and end.  Total size = 56,831 bytes.
  #define XMEM_START   ( (void *) 0x2200 )   // Start of QuadRAM External SRAM memory space = decimal 8,704.
  #define XMEM_END     ( (void *) 0xFFFF )   // End of QuadRAM External SRAM memory space = decimal 65,535.
  // Set SRE bit in the XMCRA (External Memory Control Register A).
  // QuadRAM is fast enough to support zero-wait-state operation so all other bits in the XMCRA register can be set to 0.
  XMCRA = _BV(SRE);
  // QuadRAM: Disable high memory masking (don't need >56kbytes in this app):
  XMCRB = 0;
  // QuadRAM requires four pinMode() and digitalWrite() statements.
  // QuadRAM has to be enabled by setting PD7 (Arduino digital pin D38) low:
  digitalWrite(PIN_OUT_XMEM_ENABLE, LOW); pinMode(PIN_OUT_XMEM_ENABLE, OUTPUT);  // PIN_OUT_XMEM_ENABLE = 38
  // QuadRAM: Enable bank select outputs and select bank 0. Don't need multiple banks in this app.
  // Pins PL5 through PL7 (digital pins D44 through D42) are used to select between eight 64 kilobyte RAM banks, which together
  // form the 512 kilobyte external RAM.
  digitalWrite(PIN_OUT_XMEM_BANK_BIT_0, LOW); pinMode(PIN_OUT_XMEM_BANK_BIT_0, OUTPUT);  // PIN_OUT_XMEM_BANK_BIT_0 = 42 (PL7 or D42)
  digitalWrite(PIN_OUT_XMEM_BANK_BIT_1, LOW); pinMode(PIN_OUT_XMEM_BANK_BIT_1, OUTPUT);  // PIN_OUT_XMEM_BANK_BIT_1 = 43 (PL6 or D43)
  digitalWrite(PIN_OUT_XMEM_BANK_BIT_2, LOW); pinMode(PIN_OUT_XMEM_BANK_BIT_2, OUTPUT);  // PIN_OUT_XMEM_BANK_BIT_2 = 44 (PL5 or D44)
  // QuadRAM: Relocate the heap to external SRAM.
  // This is not required, but frees more of the 8K SRAM so definitely do this.
  __malloc_heap_start =  (char *) ( XMEM_START );   // Reallocate start of heap area = 0x2200 = dec 8,704.
  __malloc_heap_end   =  (char *) ( XMEM_END );     // Reallocate end of heap area = 0xFFFF = dec 65,535.
  return;
}

void haltIfHaltPinPulledLow() {
  // Check to see if another Arduino, or Emergency Stop button, pulled the HALT pin low.
  // If we get a false HALT due to turnout solenoid EMF, it only lasts for about 8 microseconds,
  // but EMF double spike causes rare false trips.  So we upped from 50 microseconds to 1 ms (and 4/15/24 to 5ms.)
  // 04/16/24: Still getting false halts occasionally at 5ms delay when pressing turnout buttons in Manual mode.
  if (digitalRead(PIN_IO_HALT) == LOW) {    // See if it lasts a while
    delay(10);  // Pause to let a spike resolve.  Had to increase from 5us ultimately to 10ms due to too many false halts.
    if (digitalRead(PIN_IO_HALT) == LOW) {  // If still low, we have a legit Halt, not a transient voltage spike
      if (THIS_MODULE == ARDUINO_LEG) {     // LEG requires some extra logic:
        pShiftRegister->initializePinsForOutput();  // LEG: Release all relay coils that might be holding accessories on
        requestLegacyHalt();
      }
      if (THIS_MODULE == ARDUINO_SWT) {             // SWT requires some extra logic:
        pShiftRegister->initializePinsForOutput();  // SWT: Release all relay coils that might be activating turnout solenoids
        wdt_disable();  // No longer need WDT since solenoids are released and we are terminating program here.
      }
      chirp();
      sprintf(lcdString, "HALT PIN LOW!");
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
      while (true) {}  // For a real halt, just loop forever.
    } else {
      chirp();
      sprintf(lcdString, "FALSE HALT DETECT!");
      pLCD2004->println(lcdString);
      Serial.println(lcdString);
    }
  }
  return;
}

void endWithFlashingLED(int t_numFlashes) {  // Works for all modules, including SWT and LEG (requires pointer to Centipede)
  if (THIS_MODULE == ARDUINO_SWT) {             // SWT requires some extra logic:
    pShiftRegister->initializePinsForOutput();  // SWT: Release all relay coils that might be activating turnout solenoids
  }
  if (THIS_MODULE == ARDUINO_LEG) {             // LEG requires some extra logic:
    pShiftRegister->initializePinsForOutput();  // LEG: Release all relay coils that might be holding accessories on
    requestLegacyHalt();                        // LEG: Stop all trains, turn off all track power.
  }
  // The rest of the function applies to all modules including MAS, SNS, OCC, LED, and BTN:
  requestEmergencyStop();
  // Infinite loop:
  while (true) {
    for (int i = 1; i <= t_numFlashes; i++) {
      digitalWrite(PIN_OUT_LED, HIGH);
      chirp();
      digitalWrite(PIN_OUT_LED, LOW);
      delay(250);
    }
    delay(1000);
  }
}

void requestEmergencyStop() {
  // Rev: 05/23/24 to digitalWrite(pin, LOW) *before* setting pinMode(pin, OUTPUT), for best practices (not really a problem here.)
  // Rev: 10/05/16
  // The reason why we only pull the pin low for 1 second is because all of the external resistors will create a load which may
  // require more current than the Arduino is rated, so just pull low momentarily rather than leave the current sink on. 
  // This means that every module needs to check if the halt line is pulled low AT LEAST EVERY 1 SECOND.  Should not be a problem
  // since it should happen every time through every loop - but we could always extend the time held low if needed.
  // Of course there are some cases where blocking will prevent checking, such as when waiting for the operator to make selections
  // on the rotary encoder during Registration.  No big deal -- the most important place to constantly be checking the halt line
  // is in LEG, where we will want to turn off all track power.
  digitalWrite(PIN_IO_HALT, LOW);   // Pulling this low tells all Arduinos to HALT
  pinMode(PIN_IO_HALT, OUTPUT);
  delay(1000);
  digitalWrite(PIN_IO_HALT, HIGH);  // Well this seems unnecessary...why not just leave it pulling low?
  pinMode(PIN_IO_HALT, INPUT_PULLUP);
  return;
}

void chirp() {  // BE AWARE THAT THIS TAKES 10ms TO PERFORM!  SO WHEN USED, IT MAY SLOW AN IMPORTANT FUNCTION.
  // Rev 10/05/16
  digitalWrite(PIN_OUT_SPEAKER, LOW);  // turn on piezo
  delay(10);
  digitalWrite(PIN_OUT_SPEAKER, HIGH);
  return;
}

void requestLegacyHalt() {
  // Needed by endWithFlashingLED() and haltIfHaltPinPulledLow() when called by LEG
  // Rev 08/21/17: Pulled code out of requestEmergencyStop and checkIfHaltPinPulledLow
  // We are going to force this rather than using the Legacy command buffer, because this is an emergency stop.
  for (int i = 0; i <= 2; i++) {   // Send Halt to Legacy (3 times for good measure)
    byte legacy11 = 0xFE;
    byte legacy12 = 0xFF;
    byte legacy13 = 0xFF;
    Serial3.write(legacy11);
    Serial3.write(legacy12);
    Serial3.write(legacy13);
    delay(25);  // Brief (25ms) pause between bursts of 3-byte commands
  }
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
