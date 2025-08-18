// Rev: 08/17/25.
// Pinball Centipede Shield Library
// Controls MCP23017 16-bit digital I/O chips
// This is the newer 8/28/12 version cleaned up by RDP on 10/14/17.
// RP changed .initialize to .begin for consistency on 10/8/20.
// This newer version supports interrupts by adding portInterrupts(), portCaptureRead(), and portIntPinConfig()

#ifndef Pinball_Centipede_h
#define Pinball_Centipede_h

#include "Arduino.h"
#include <Wire.h>

extern uint8_t CSDataArray[2];

class Pinball_Centipede
{
  public:
    Pinball_Centipede();
    void begin();
    void initializePinsForMaster();
    void initializePinsForSlave();
    void pinMode(int pin, int mode);
    void pinPullup(int pin, int mode);
    void digitalWrite(int pin, int level);
    int  digitalRead(int pin);
    void portMode(int port, int value);
    void portPullup(int port, int value);
    void portWrite(int port, int value);
    int  portRead(int port);
    void portInterrupts(int port, int gpintval, int defval, int intconval);
    int  portCaptureRead(int port);
    void portIntPinConfig(int port, int drain, int polarity);

  //private:
    void WriteRegisters(int port, int startregister, int quantity);
    void ReadRegisters(int port, int startregister, int quantity);
    void WriteRegisterPin(int port, int regpin, int subregister, int level);
};

#endif
