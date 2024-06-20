// SENSOR_BLOCK.CPP Rev: 06/18/24.  TESTED AND WORKING.
// A set of functions to retrieve data from the Sensor Block cross reference table, which is stored in FRAM.
// Changed name from setStatus/getStatus to setSensorStatus/getSensorStatus

#include <Sensor_Block.h>

Sensor_Block::Sensor_Block() {  // Constructor
  // Rev: 01/25/23.
  // Object holds ONE Sensor-Block record (not the whole table)
  // Just initialize our internal sensor-block structure element...
  m_sensorBlock.sensorNum = 0;  // Zero is *not* a real sensor number so this will never be legal.
  m_sensorBlock.blockNum  = 0;
  m_sensorBlock.whichEnd  = SENSOR_END_WEST;
  m_sensorBlock.status    = SENSOR_STATUS_CLEARED;
  return;
}

void Sensor_Block::begin(FRAM* t_pStorage) {  // Just init the pointer to FRAM
  // Rev: 01/25/23.
  m_pStorage = t_pStorage;  // Pointer to FRAM
  if (m_pStorage == nullptr) {
    sprintf(lcdString, "UN-INIT'd SB PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;
}

// For these functions, we should see if the data already in the m_sensorBlock buffer matches the sensor that the module
// is asking about.  If so, no need to do another FRAM get; otherwise, do a get before returning the value from the buffer.

byte Sensor_Block::getSensorNumber(const byte t_sensorNum) {
  // Rev: 01/26/23.
  // Gets the sensor number from the table, which darn well better match t_sensorNum!
  // Expects t_sensorNum to start at 1 not 0.
  if (t_sensorNum != m_sensorBlock.sensorNum) {
    Sensor_Block::getSensorBlock(t_sensorNum);
  }
  return m_sensorBlock.sensorNum;
}

byte Sensor_Block::whichBlock(const byte t_sensorNum) {
  // Rev: 01/25/23.
  // Returns block number that this sensor is in, 1..26 (not 0..25)
  // Expects t_sensorNum to start at 1 not 0!
  if (t_sensorNum != m_sensorBlock.sensorNum) {
    Sensor_Block::getSensorBlock(t_sensorNum);
  }
  return m_sensorBlock.blockNum;
}

char Sensor_Block::whichEnd(const byte t_sensorNum) {
  // Rev: 01/25/23.
  // Returns SENSOR_END_EAST or SENSOR_END_WEST ('E' or 'W'), depending on which end of it's block this sensor is located.
  // Expects t_sensorNum to start at 1 not 0!
  if (t_sensorNum != m_sensorBlock.sensorNum) {
    Sensor_Block::getSensorBlock(t_sensorNum);
  }
  return m_sensorBlock.whichEnd;  // SENSOR_BLOCK_EAST == 'E', or SENSOR_BLOCK_WEST == 'W'
}

char Sensor_Block::getSensorStatus(const byte t_sensorNum) {
  // Rev: 01/25/23.
  // Returns field status = SENSOR_STATUS_TRIPPED or SENSOR_STATUS_CLEARED.
  // This reflects what is stored in the Sensor Block table, *not* necessarily the actual status of the sensor.
  if (t_sensorNum != m_sensorBlock.sensorNum) {
    Sensor_Block::getSensorBlock(t_sensorNum);
  }
  return m_sensorBlock.status;
}

void Sensor_Block::setSensorStatus(const byte t_sensorNum, const char t_sensorStatus) {
  // Rev: 01/25/23.
  // Sets a particular sensor's occupancy status to either Tripped or Cleared [T|C]
  if ((t_sensorStatus != SENSOR_STATUS_TRIPPED) && (t_sensorStatus != SENSOR_STATUS_CLEARED)) {
    sprintf(lcdString, "BAD SET S%i %c", t_sensorNum, t_sensorStatus); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  if (t_sensorNum != m_sensorBlock.sensorNum) {
    Sensor_Block::getSensorBlock(t_sensorNum);
  }
  m_sensorBlock.status = t_sensorStatus;
  Sensor_Block::setSensorBlock(t_sensorNum);
  return;
}

void Sensor_Block::display(const byte t_sensorNum) {
  // Rev: 01/26/23.
  // Display a single Sensor Block record; not the entire table.  For testing and debugging purposes only.
  // Could reduce the amount of memory required by using F() macro in Serial.print, but we only call this for debugging so won't
  // affect the amount of memory required in the final program.
  if (t_sensorNum != m_sensorBlock.sensorNum) {
    Sensor_Block::getSensorBlock(t_sensorNum);
  }
  sprintf(lcdString, "Sensor: %2i ", Sensor_Block::getSensorNumber(t_sensorNum)); Serial.print(lcdString);
  if (Sensor_Block::whichEnd(t_sensorNum) == SENSOR_END_WEST) {
    Serial.print(" WEST end of Block ");
  } else if (Sensor_Block::whichEnd(t_sensorNum) == SENSOR_END_EAST) {
    Serial.print(" EAST end of Block ");
  } else {
    sprintf(lcdString, "BAD END SENSOR %i", t_sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  sprintf(lcdString, "%2i, Currently ", Sensor_Block::whichBlock(t_sensorNum)); Serial.print(lcdString);
  if (Sensor_Block::getSensorStatus(t_sensorNum) == SENSOR_STATUS_CLEARED) {
    Serial.println("Cleared.");
  }
  else if (Sensor_Block::getSensorStatus(t_sensorNum) == SENSOR_STATUS_TRIPPED) {
    Serial.println("TRIPPED.");
  }
  else {
    sprintf(lcdString, "BAD T/C SENSOR %i", t_sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;
}

void Sensor_Block::populate() {  // Populate the Sensor Block table with known constants, and init variable fields.
  // Rev: 01/25/23.
  // We ONLY need to call this from a utility program, whenever we need to refresh the FRAM Sensor Block table, such as if
  // some data changes (which it shouldn't unless we made an error, or re-design the layout.)
  // Note that sensor number 1..52 corresponds with element 0..51.
  // Careful don't call this m_sensorBlock, which would confuse with our "regular" variable that holds just one record.
  // Technically I should change W and E to SENSOR_END_WEST and SENSOR_END_EAST, but they are defined as char W and E so no prob.
  // Also I should change C to SENSOR_STATUS_CLEARED, but it's defined as char C so no big deal here.
  sensorBlockStruct sensorBlock[TOTAL_SENSORS] = {   // 52 sensors, offset 0 thru 51
    { 1,  1, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    { 2,  1, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    { 3,  2, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    { 4,  2, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    { 5,  3, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    { 6,  3, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    { 7,  4, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    { 8,  4, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    { 9,  5, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {10,  5, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {11,  6, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {12,  6, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {13, 14, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {14, 14, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {15, 15, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {16, 15, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {17, 13, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {18, 13, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {19, 19, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {20, 19, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {21, 20, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {22, 20, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {23, 23, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {24, 23, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {25, 24, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {26, 24, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {27, 25, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {28, 25, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {29, 26, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {30, 26, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {31,  7, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {32,  7, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {33,  8, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {34,  8, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {35,  9, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {36,  9, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {37, 10, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {38, 10, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {39, 11, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {40, 11, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {41, 12, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {42, 12, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {43, 16, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {44, 16, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {45, 17, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {46, 17, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {47, 18, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {48, 18, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {49, 22, SENSOR_END_EAST, SENSOR_STATUS_CLEARED},
    {50, 22, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {51, 21, SENSOR_END_WEST, SENSOR_STATUS_CLEARED},
    {52, 21, SENSOR_END_EAST, SENSOR_STATUS_CLEARED}
  };
  byte FRAMDataBuf[sizeof(sensorBlockStruct)];
  for (byte sensorNum = 1; sensorNum <= TOTAL_SENSORS; sensorNum++) {
    memcpy(FRAMDataBuf, &sensorBlock[sensorNum - 1], sizeof(sensorBlockStruct));  // Correcting for array element 0 = sensor 1
    m_pStorage->write(Sensor_Block::sensorBlockAddress(sensorNum), sizeof(sensorBlockStruct), FRAMDataBuf);
  }
  m_pStorage->setFRAMRevDate(01, 26, 23);  // ALWAYS UPDATE FRAM DATE IF WE CHANGE A FILE!
  return;
}

// ***** PRIVATE FUNCTIONS ***

void Sensor_Block::getSensorBlock(const byte t_sensorNum) {
  // Expects t_sensorNum to start at 1 not 0!
  // Rather than writing into FRAMDataBuf and then copying to the structure, I'm writinging directly into the local struct variable.
  // However, FRAM.read requires a "byte" pointer to the local data it's going to be writing into, so I need to create that variable.
  // Shown below are two possible ways to cast a pointer to the Sensor Block structure so FRAM.read can work with it.  Both confirmed work.
  // byte* pSensorBlock = (byte*)&m_sensorBlock;  // FRAM "read" expects pointer to byte, not pointer to structure element, so need to cast
  sensorBlockStruct* pSensorBlockElement = &m_sensorBlock;  // FRAM.read needs a pointer to the Sensor Block structure variable.
  byte* pSensorBlock = reinterpret_cast<byte*>(pSensorBlockElement);  // But FRAM.read expects pointer to byte, so need to cast.
  m_pStorage->read(Sensor_Block::sensorBlockAddress(t_sensorNum), sizeof(sensorBlockStruct), pSensorBlock);  // Pass "real" sensor num starts at 1
  return;
}

void Sensor_Block::setSensorBlock(const byte t_sensorNum) {
  // Expects t_sensorNum to start at 1 not 0!
  sensorBlockStruct* pSensorBlockElement = &m_sensorBlock;  // FRAM.write needs a pointer to the structure variable.
  byte* pSensorBlock = reinterpret_cast<byte*>(pSensorBlockElement);  // But FRAM.write expects pointer to byte, so need to cast.
  m_pStorage->write(Sensor_Block::sensorBlockAddress(t_sensorNum), sizeof(sensorBlockStruct), pSensorBlock);  // Pass "real" sensor num starts at 1
  return;
}

unsigned long Sensor_Block::sensorBlockAddress(const byte t_sensorNum) {  // Return the starting address of the *record* in the Sensor Block X-ref FRAM table for this sensor
  // Expects t_sensorNum to start at 1 not 0!  We correct for that here.
  if ((t_sensorNum < 1) || (t_sensorNum > TOTAL_SENSORS)) {
    sprintf(lcdString, "BAD SBX SENSOR %i", t_sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return FRAM_ADDR_SNS_BLK_XREF + ((t_sensorNum - 1) * sizeof(sensorBlockStruct));  // NOTE: We translate block number 1 to record 0.
}
