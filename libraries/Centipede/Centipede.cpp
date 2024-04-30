// Rev: 10/09/20.
// Centipede Shield Library
// Controls MCP23017 16-bit digital I/O chips
// This is the newer 8/28/12 version cleaned up by RDP on 10/14/17
// RP changed .initialize to .begin for consistency on 10/8/20.
// This newer version supports interrupts by adding portInterrupts(), portCaptureRead(), and portIntPinConfig()

#include <Centipede.h>

uint8_t CSDataArray[2] = {0};

#define CSAddress 0b0100000

Centipede::Centipede() {  // no constructor tasks yet
}

// Set device to default values
void Centipede::begin() {
  for (int j = 0; j < 7; j++) {
    CSDataArray[0] = 255;
    CSDataArray[1] = 255;
    WriteRegisters(j, 0x00, 2);
    CSDataArray[0] = 0;
    CSDataArray[1] = 0;
    for (int k = 2; k < 0x15; k+=2) {
      WriteRegisters(j, k, 2);
    }
  }
}

// *** THIS VERSION FROM BTN, SNS (Buttons and Sensors are inputs) ***
void Centipede::initializePinsForInput() {  // Currently only supports all pins for input, not mixed
  // Sets all pins for two boards; works fine even if only one board is installed.
  // Will need to re-write or add new function if we ever want some pins for input and others for output.
  for (int i = 0; i < 8; i++) {         // For each of 4 chips per board / 2 boards = 8 chips @ 16 bits/chip
    portMode(i, 0b1111111111111111);   // 0 = OUTPUT, 1 = INPUT.  Sets all pins on chip to INPUT
    portPullup(i, 0b1111111111111111); // 0 = NO PULLUP, 1 = PULLUP.  Sets all pins on chip to PULLUP
  }
}

// *** THIS VERSION FOR SWT, LED, LEG, OCC (Turnouts, LEDs and Acc'ys are outputs) ******
void Centipede::initializePinsForOutput() {  // Currently only supports all pins for output, not mixed
  // Sets all pins for two boards; works fine even if only one board is installed.
  // Will need to re-write or add new function if we ever want some pins for input and others for output.
  for (int i = 0; i < 8; i++) {         // For each of 4 chips per board / 2 boards = 8 chips @ 16 bits/chip
    // Set outputs high before setting pin mode to output, to prevent brief low being sent to Centipede shift register outputs
    portWrite(i, 0b1111111111111111);  // Set all outputs HIGH (pull LOW to turn on an LED)
    portMode(i, 0b0000000000000000);   // Set all pins on this chip to OUTPUT
  }
}

void Centipede::WriteRegisters(int port, int startregister, int quantity) {
  Wire.beginTransmission(CSAddress + port);
  Wire.write((byte)startregister);
  for (int i = 0; i < quantity; i++) {
    Wire.write((byte)CSDataArray[i]);
  }
  Wire.endTransmission();
}

void Centipede::ReadRegisters(int port, int startregister, int quantity) {
  Wire.beginTransmission(CSAddress + port);
  Wire.write((byte)startregister);
  Wire.endTransmission();
  Wire.requestFrom(CSAddress + port, quantity);
  for (int i = 0; i < quantity; i++) {
    CSDataArray[i] = Wire.read();
  }
}

void Centipede::WriteRegisterPin(int port, int regpin, int subregister, int level) {
  ReadRegisters(port, subregister, 1); 
  if (level == 0) {
    CSDataArray[0] &= ~(1 << regpin);
  }
  else {
    CSDataArray[0] |= (1 << regpin);
  }
  WriteRegisters(port, subregister, 1);
}

void Centipede::pinMode(int pin, int mode) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  int regpin = pin - ((port << 1) + subregister)*8;
  WriteRegisterPin(port, regpin, subregister, mode ^ 1);
}

void Centipede::pinPullup(int pin, int mode) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  int regpin = pin - ((port << 1) + subregister)*8;
  WriteRegisterPin(port, regpin, 0x0C + subregister, mode);
}


void Centipede::digitalWrite(int pin, int level) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  int regpin = pin - ((port << 1) + subregister)*8;
  WriteRegisterPin(port, regpin, 0x12 + subregister, level);
}

int Centipede::digitalRead(int pin) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  ReadRegisters(port, 0x12 + subregister, 1);
  int returnval = (CSDataArray[0] >> (pin - ((port << 1) + subregister)*8)) & 1;
  return returnval;
}

void Centipede::portMode(int port, int value) {
  CSDataArray[0] = value;
  CSDataArray[1] = value>>8;
  WriteRegisters(port, 0x00, 2);
}

void Centipede::portWrite(int port, int value) {
  CSDataArray[0] = value;
  CSDataArray[1] = value>>8;
  WriteRegisters(port, 0x12, 2);
}

void Centipede::portInterrupts(int port, int gpintval, int defval, int intconval) {
  WriteRegisterPin(port, 6, 0x0A, 1);
  WriteRegisterPin(port, 6, 0x0B, 1);
  CSDataArray[0] = gpintval;
  CSDataArray[1] = gpintval>>8;
  WriteRegisters(port, 0x04, 2);
  CSDataArray[0] = defval;
  CSDataArray[1] = defval>>8;
  WriteRegisters(port, 0x06, 2);
  CSDataArray[0] = intconval;
  CSDataArray[1] = intconval>>8;
  WriteRegisters(port, 0x08, 2);
}

int Centipede::portCaptureRead(int port) {
  ReadRegisters(port, 0x10, 2);
  int receivedval = CSDataArray[0];
  receivedval |= CSDataArray[1] << 8;
  return receivedval;  
}

void Centipede::portIntPinConfig(int port, int drain, int polarity) {
  WriteRegisterPin(port, 1, 0x0A, drain);
  WriteRegisterPin(port, 1, 0x0B, drain);
  WriteRegisterPin(port, 0, 0x0A, polarity);
  WriteRegisterPin(port, 0, 0x0B, polarity);
}

void Centipede::portPullup(int port, int value) {
  CSDataArray[0] = value;
  CSDataArray[1] = value>>8;
  WriteRegisters(port, 0x0C, 2);
}

int Centipede::portRead(int port) {
  ReadRegisters(port, 0x12, 2);
  int receivedval = CSDataArray[0];
  receivedval |= CSDataArray[1] << 8;
  return receivedval;  
}