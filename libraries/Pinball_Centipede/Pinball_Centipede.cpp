// Rev: 03/16/26.
// Pinball Centipede Shield Library
// Controls MCP23017 16-bit digital I/O chips
// This is the newer 8/28/12 version cleaned up by RDP on 10/14/17
// RP changed .initialize to .begin for consistency on 10/8/20.
// This newer version supports interrupts by adding portInterrupts(), portCaptureRead(), and portIntPinConfig()
// 03/16/26: Added read-twice-compare with majority vote to WriteRegisterPin() to protect against I2C corruption from solenoid PWM.

#include <Pinball_Centipede.h>

byte CSDataArray[2] = {0};

#define CSAddress 0b0100000  // 0b010000 = 0x20, which is the address of the first (of 4 or 8) chips on the Centipede

Pinball_Centipede::Pinball_Centipede() {  // no constructor tasks yet
}

// Set device to default values
void Pinball_Centipede::begin() {
  // Ensure output latches default to HIGH (inactive for active-LOW relays)
  // Write OLAT (0x12) = 0xFFFF for all ports to avoid transient ON
  for (int port = 0; port < 8; ++port) {
    CSDataArray[0] = 0xFF;
    CSDataArray[1] = 0xFF;
    WriteRegisters(port, 0x12, 2); // OLAT registers
  }
  // Continue with existing default configuration
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

void Pinball_Centipede::initScreamoMasterCentipedePins() {  // First board is output, Second board is input
  // Sets all pins for two boards; works fine even if only one board is installed.
  // For chips (ports) 0..7, set to input or output per chip as desired
  // MASTER has first 4 ports as OUTPUT (to lamp relays)
  for (int i = 0; i < 4; i++) {         // For each of the 4 chips on Centipede 1 of 2...
    // Set outputs high before setting pin mode to output, to prevent brief low being sent to Centipede shift register outputs
    portWrite(i, 0b1111111111111111);  // Set all outputs HIGH (pull LOW to turn on)
    portMode(i, 0b0000000000000000);   // 0 = OUTPUT, 1 = INPUT.  Sets all pins on this chip to OUTPUT
  }
  // MASTER has second 4 ports as INPUT (from switches)
  for (int i = 4; i < 8; i++) {         // For each of the 4 chips on Centipede 2 of 2...
    portMode(i, 0b1111111111111111);   // 0 = OUTPUT, 1 = INPUT.  Sets all pins on chip to INPUT
    portPullup(i, 0b1111111111111111); // 0 = NO PULLUP, 1 = PULLUP.  Sets all pins on chip to PULLUP
  }
}

void Pinball_Centipede::initScreamoSlaveCentipedePins() {  // Just one board on Slave which is output to lamp relays
  // Sets all pins for two boards; works fine even if only one board is installed.
  for (int i = 0; i < 8; i++) {         // For each of 4 chips per board / 2 boards = 8 chips @ 16 bits/chip
    // Set outputs high before setting pin mode to output, to prevent brief low being sent to Centipede shift register outputs
    portWrite(i, 0b1111111111111111);  // Set all outputs HIGH (pull LOW to turn on)
    portMode(i, 0b0000000000000000);   // Set all pins on this chip to OUTPUT
  }
}

void Pinball_Centipede::WriteRegisters(int port, int startregister, int quantity) {
  Wire.beginTransmission(CSAddress + port);
  Wire.write((byte)startregister);
  for (int i = 0; i < quantity; i++) {
    Wire.write((byte)CSDataArray[i]);
  }
  Wire.endTransmission();
}

void Pinball_Centipede::ReadRegisters(int port, int startregister, int quantity) {
  Wire.beginTransmission(CSAddress + port);
  Wire.write((byte)startregister);
  Wire.endTransmission();
  Wire.requestFrom(CSAddress + port, quantity);
  for (int i = 0; i < quantity; i++) {
    CSDataArray[i] = Wire.read();
  }
}

void Pinball_Centipede::WriteRegisterPin(int port, int regpin, int subregister, int level) {
  // ***** WHY THIS PROTECTION EXISTS *****
  // This function changes ONE bit in an 8-bit MCP23017 register. To do that,
  // it must READ the current register value, MODIFY the target bit, then WRITE
  // the result back. This is called a "read-modify-write" cycle.
  //
  // The problem: flipper hold-power PWM (~4-8 kHz) on the same I2C bus can
  // corrupt individual I2C transactions. If the READ returns garbage, we modify
  // one bit of that garbage and write it back -- silently turning on or off up
  // to 7 OTHER lamps that share the same 8-bit register. For example, turning
  // off bumper "M" could accidentally turn on bumper "E" if the read was wrong.
  //
  // ***** WHAT WE DO *****
  // Read the register TWICE and compare. If both reads agree, we trust the
  // result (the odds of identical corruption on back-to-back reads ~0.13ms
  // apart are negligible). If they disagree, one of the reads was corrupted --
  // do a third read and take the majority vote (2-out-of-3 wins).

  // First read of the current register value
  ReadRegisters(port, subregister, 1);
  uint8_t read1 = CSDataArray[0];

  // Second read -- should match if I2C bus was clean
  ReadRegisters(port, subregister, 1);
  uint8_t read2 = CSDataArray[0];

  if (read1 != read2) {
    // Reads disagree: at least one was corrupted. Tiebreaker read.
    ReadRegisters(port, subregister, 1);
    uint8_t read3 = CSDataArray[0];
    // Use whichever value appeared at least twice
    CSDataArray[0] = (read1 == read3) ? read1 : read2;
  } else {
    // Both reads agree -- trust this value
    CSDataArray[0] = read1;
  }

  // ***** MODIFY the target bit *****
  // Now CSDataArray[0] holds a verified copy of the register's current state.
  // Set or clear the one bit we were asked to change.
  if (level == 0) {
    CSDataArray[0] &= ~(1 << regpin);  // Clear bit (e.g. lamp ON for active-low)
  } else {
    CSDataArray[0] |= (1 << regpin);   // Set bit (e.g. lamp OFF for active-low)
  }

  // ***** WRITE the modified register back *****
  WriteRegisters(port, subregister, 1);

  // NOTE: The write itself could theoretically be corrupted on the I2C bus,
  // but corrupted writes are self-correcting -- the next portWrite() or
  // writeLampsBulk() call will overwrite the entire register with correct data.
  // Adding write-verify here would double I2C traffic for marginal benefit.
}

void Pinball_Centipede::pinMode(int pin, int mode) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  int regpin = pin - ((port << 1) + subregister)*8;
  WriteRegisterPin(port, regpin, subregister, mode ^ 1);
}

void Pinball_Centipede::pinPullup(int pin, int mode) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  int regpin = pin - ((port << 1) + subregister)*8;
  WriteRegisterPin(port, regpin, 0x0C + subregister, mode);
}


void Pinball_Centipede::digitalWrite(int pin, int level) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  int regpin = pin - ((port << 1) + subregister)*8;
  WriteRegisterPin(port, regpin, 0x12 + subregister, level);
}

int Pinball_Centipede::digitalRead(int pin) {
  int port = pin >> 4;
  int subregister = (pin & 8) >> 3;
  ReadRegisters(port, 0x12 + subregister, 1);
  int returnval = (CSDataArray[0] >> (pin - ((port << 1) + subregister)*8)) & 1;
  return returnval;
}

void Pinball_Centipede::portMode(int port, int value) {
  CSDataArray[0] = value;
  CSDataArray[1] = value>>8;
  WriteRegisters(port, 0x00, 2);
}

void Pinball_Centipede::portWrite(int port, int value) {
  CSDataArray[0] = value;
  CSDataArray[1] = value>>8;
  WriteRegisters(port, 0x12, 2);
}

void Pinball_Centipede::portInterrupts(int port, int gpintval, int defval, int intconval) {
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

int Pinball_Centipede::portCaptureRead(int port) {
  ReadRegisters(port, 0x10, 2);
  int receivedval = CSDataArray[0];
  receivedval |= CSDataArray[1] << 8;
  return receivedval;  
}

void Pinball_Centipede::portIntPinConfig(int port, int drain, int polarity) {
  WriteRegisterPin(port, 1, 0x0A, drain);
  WriteRegisterPin(port, 1, 0x0B, drain);
  WriteRegisterPin(port, 0, 0x0A, polarity);
  WriteRegisterPin(port, 0, 0x0B, polarity);
}

void Pinball_Centipede::portPullup(int port, int value) {
  CSDataArray[0] = value;
  CSDataArray[1] = value>>8;
  WriteRegisters(port, 0x0C, 2);
}

int Pinball_Centipede::portRead(int port) {
  ReadRegisters(port, 0x12, 2);
  int receivedval = CSDataArray[0];
  receivedval |= CSDataArray[1] << 8;
  return receivedval;  
}