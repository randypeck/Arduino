// Rev: 12-18-20 by RDP to display start, midpoint, and end times on serial monitor.

// Here are the results of write/read tests for SRAM, FRAM, and QuadRAM (tested 12/18/10):
//
// SRAM    reads    4,096 bytes in  0.404 seconds = 10.14 bytes/msec = 0.09862 msec/byte - 0.05760 = 0.04102 msec/byte "pure" read time  (24.4 bytes/msec)
// QuadRAM reads  524,288 bytes in 74.150 seconds =  7.07 bytes/msec = 0.14144 msec/byte - 0.10081 = 0.04063 msec/byte "pure" read time  (24.6 bytes/msec)
// FRAM    reads  262,144 bytes in 28.796 seconds =  9.10 bytes/msec = 0.10989 msec/byte - 0.05760 = 0.05229 msec/byte "pure" read time  (19.1 bytes/msec)
//
// SRAM    writes   4,096 bytes in  0.390 seconds = 10.50 bytes/msec = 0.09524 msec/byte - 0.05568 = 0.03956 msec/byte "pure" write time (25.3 bytes/msec)
// QuadRAM writes 524,288 bytes in 70.656 seconds =  7.42 bytes/msec = 0.13477 msec/byte - 0.09461 = 0.04016 msec/byte "pure" write time (24.9 bytes/msec)
// FRAM    writes 262,144 bytes in 30.518 seconds =  8.59 bytes/msec = 0.11641 msec/byte - 0.05568 = 0.06073 msec/byte "pure" write time (16.5 bytes/msec)
//
// SRAM loop with NO read or write array element (to subtract the overhead from the actual read/write time):
// SRAM    writes   4,096 bytes in  0.228 seconds = 17.96 bytes/msec = 0.05568 msec/byte
// SRAM    reads    4,096 bytes in  0.236 seconds = 17.36 bytes/msec = 0.05760 msec/byte
// QuadRAM loop with NO read or write element (to subtract the overhead from the actual read/write time):
// QuadRAM writes 524,288 bytes in 49.606 seconds = 10.57 bytes/msec = 0.09461 msec/byte
// QuadRAM reads  524,288 bytes in 52.839 seconds =  9.92 bytes/msec = 0.10081 msec/byte
  
/* This QuadRAM application is a production test that verifies that
 * all RAM locations are unique and can store values.
 *
 * Assumptions:
 *
 *   - QuadRAM plugged in to Arduino Mega 1280 or Mega 2560
 *
 * The behavior is as follows:
 *
 *   - Each one of 16 banks of 32 kbytes each is filled with a pseudorandom sequence of numbers
 *   - The 16 banks are read back to verify the pseudorandom sequence
 *   - On success, the Arduino's LED is turned on steadily, with a brief pulse off every 3 seconds
 *   - On failure, the Arduino's LED blinks rapidly
 *   - The serial port is used for status information (38400 bps)
 *
 * This software is licensed under the GNU General Public License (GPL) Version
 * 3 or later. This license is described at
 * http://www.gnu.org/licenses/gpl.html
 *
 * Application Version 1.0 -- October 2011 Rugged Circuits LLC
 * http://www.ruggedcircuits.com/html/quadram.html
 */

#define NUM_BANKS 16

void setup(void)
{
  DDRL  = 0xE0;  // Bits PL7, PL6, PL5 select the bank
  PORTL = 0x1F;  // Select bank 0, pullups on everything else

  /* Enable XMEM interface:

         SRE    (7)   : Set to 1 to enable XMEM interface
         SRL2-0 (6-4) : Set to 00x for wait state sector config: Low=N/A, High=0x2200-0xFFFF
         SRW11:0 (3-2) : Set to 00 for no wait states in upper sector
         SRW01:0 (1-0) : Set to 00 for no wait states in lower sector
   */
  XMCRA = 0x80;

  // Bus keeper, lower 7 bits of Port C used for upper address bits, bit 7 is under PIO control
  XMCRB = _BV(XMBK) | _BV(XMM0);
  DDRC |= _BV(7);
  PORTC = 0x7F; // Enable pullups on port C, select bank 0

  // PD7 is RAM chip enable, active low. Enable it now, and enable pullups on Port D.
  DDRD  = _BV(7);
  PORTD = 0x7F;

  // Open up a serial channel
  Serial.begin(115200); //38400);

  // Enable on-board LED
  pinMode(13, OUTPUT);

  Serial.println("QuadRAM test begin...");
}

// Blink the Arduino LED quickly to indicate a memory test failure
void blinkfast(void)
{
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
}

// Provide information on which address failed. Then, sit in an infinite loop blinking
// the LED quickly.
unsigned long gTotalFailures;
void fail(uint8_t *addr, uint8_t expect, uint8_t got, uint8_t lastGood)
{
  Serial.print("At address "); Serial.print((uint16_t)addr,HEX);
  Serial.print(" expected "); Serial.print(expect, HEX);
  Serial.print(" got "); Serial.print(got, HEX);
  Serial.print(" last good was "); Serial.print(lastGood, HEX);
  Serial.println();
  gTotalFailures++;

  while (1) {
    blinkfast();
  }
}

#define STARTADDR ((uint8_t *)0x8000)  // 0x8000 = 32,768
#define COUNT (0x8000U-1)

long seed=1234;

// Select one of 16 32-kilobyte banks
void banksel(uint8_t bank)
{
  // The 16-bit 'bank' parameter can be thought of as a 4-bit field, with the upper
  // 3 bits to be written to PL7:PL5 and the lowest 1 bit to PC7.
  if (bank & 1) { // Take care of lowest 1 bit
    PORTC |= _BV(7);
  } else {
    PORTC &= ~_BV(7);
  }

  // Now shift upper 3 bits (of 4-bit value in 'bank') to PL7:PL5
  PORTL = (PORTL & 0x1F) | (((bank>>1)&0x7) << 5);
}

// Fill the currently selected 32 kilobyte bank of memory with a random sequence
void bankfill(void)
{
  uint8_t *addr;
  uint8_t val;
  uint16_t count;

  for (addr=STARTADDR, count=COUNT; count; addr++, count--) {
    val = (uint8_t)random(256);
    *addr = val;
    if ((count & 0xFFFU) == 0) blinkfast();
  }
}

// Check the currently selected 32 kilobyte bank of memory against expected values
// in each memory cell.
void bankcheck(void)
{
  uint8_t expect;
  uint8_t *addr;
  uint16_t count;
  uint8_t lastGood=0xEE;

  for (addr=STARTADDR, count=COUNT; count; addr++, count--) {
    expect = random(256);
    if (*addr != expect) {
        fail(addr,expect,*addr,lastGood);
    }
    lastGood = expect;
    
    if ((count & 0xFFFU) == 0x123) blinkfast();
  }
}

void loop(void)
{
  uint8_t bank;

  // Always start filling and checking with the same random seed
  randomSeed(seed);
  gTotalFailures=0;
  Serial.print("QuadRAM test begins writing 524,288 bytes at "); Serial.println(millis());
  // Fill all 16 banks with random numbers
  for (bank=0; bank < NUM_BANKS; bank++) {
    Serial.print("   Filling bank "); Serial.print(bank, DEC); Serial.println(" ...");
    banksel(bank); bankfill();
  }
  Serial.print("QuadRAM done writing 524,288 bytes at "); Serial.println(millis());

  // Restore the initial random seed, then check all 16 banks against expected random numbers
  randomSeed(seed);
  for (bank=0; bank < NUM_BANKS; bank++) {
    Serial.print("  Checking bank "); Serial.print(bank, DEC); Serial.println(" ...");
    banksel(bank); bankcheck();
  }

  // If we got this far, there were no failures
  if (gTotalFailures) {
    Serial.print(gTotalFailures); Serial.println(" total failures");
  } else {
    Serial.print("QuadRAM test done reading at "); Serial.println(millis());
  }
  Serial.println();

  // Keep the LED on mostly, and pulse off briefly every 3 seconds
  while (1) {
    digitalWrite(13, HIGH);
    delay(3000);
    digitalWrite(13, LOW);
    delay(500);
  }
}
// vim: syntax=cpp ai ts=2 sw=2 cindent expandtab
