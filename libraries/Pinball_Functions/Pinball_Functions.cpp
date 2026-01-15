// PINBALL_FUNCTIONS.CPP Rev: 01/14/26.
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
