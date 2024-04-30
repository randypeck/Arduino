// BLOCK_RESERVATION.CPP Rev: 02/27/23.  TESTED AND WORKING.
// A set of functions to read and update the Block Reservation table, which is stored in FRAM.
// 02/09/23: Eliminated possibility of having ER as optional direction; must always be either BE or BW even if not reserved.
// 01/17/23: Rearranging/updating some of the structure fields, updated block lengths for 1st level (2nd level unknown.)
// 11/27/22: Added default Eastbound and Westbound block speeds for use by Train Progress when needed for final block of a Cont'n
//           Route -- when we need to assign a speed to the no-longer-a-Destination block.
// 12/26/20: Added station type P/F/B/N field; reduced "future" field from 6 to 5 bytes.
// 12/18/20: Added isParkingSiding bool for use by Dispatcher.

#include "Block_Reservation.h"

Block_Reservation::Block_Reservation() {  // Constructor
  // Rev: 01/17/23.
  // Object holds ONE Block Reservation record (not the whole table)
  // Just initialize our internal block-reservation structure element...
  m_blockReservation.blockNum             = 0;   // Zero is *not* a real block so this will never be legal.
  m_blockReservation.reservedForTrain     = LOCO_ID_NULL;  // Real trains start at locoNum 1
  m_blockReservation.reservedForDirection = BW;  // Only relevant if reservedForTrain non-zero, but always must be BE or BW.
  m_blockReservation.westSensor           = 0;   // Zero is *not* a real sensor number so this will never be legal.
  m_blockReservation.eastSensor           = 0;   // Zero is *not* a real sensor number so this will never be legal.
  m_blockReservation.westboundSpeed       = LOCO_SPEED_STOP;
  m_blockReservation.eastboundSpeed       = LOCO_SPEED_STOP;
  m_blockReservation.length               = 0;
  m_blockReservation.sidingType           = SIDING_NOT_A_SIDING;
  m_blockReservation.isParkingSiding      = false;
  m_blockReservation.stationType          = STATION_NOT_A_STATION;
  m_blockReservation.forbidden            = FORBIDDEN_NONE;
  m_blockReservation.isTunnel             = false;
  m_blockReservation.gradeDirection       = GRADE_NOT_A_GRADE;
  return;
}

void Block_Reservation::begin(FRAM* t_pStorage) {
  // Rev: 01/22/23.
  // We do NOT call releaseAllBlocks() yet as we may want to preserve reservedForTrain for Registrar to read and use as the default
  // location as trains are registered (though as of 3/3/23, it looks like we'll use Loco Ref to store each loco's last-known loc.)
  // Regardless, Registrar must call releaseAllBlocks() when it's ready to do so.
  m_pStorage = t_pStorage;  // Pointer to FRAM so we can access our table.
  if (m_pStorage == nullptr) {
    sprintf(lcdString, "UN-INIT'd BR PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;
}

// For many of these functions, we will see if the data already in the m_blockReservation buffer matches the block that the module
// is asking about.  If so, no need to do another FRAM get; otherwise, do a get before returning the value from the buffer.
// All of the following expect t_blockNum, t_locoNum, sensorNum, etc. to start at 1 not 0.  No checking of input ranges!

void Block_Reservation::reserveBlock(const byte t_blockNum, const byte t_direction, const byte t_locoNum) {
  // Rev: 01/24/23.
  // Retrieve entire Block Res'n record (if not already loaded.)
  // FATAL ERROR if previously reserved, but this is optional, maybe overkill or even a problem.
  // Because "direction" can be confusing, let's make sure it's either BE or BW per our global consts...
  // We don't need to check the range of t_blockNum here because it's handled by getBlockReservation()/blockReservationAddress().
  // if ((t_blockNum < 1) || (t_blockNum > TOTAL_BLOCKS)) {
  //   sprintf(lcdString, "FATAL BLK NUM %i", t_direction); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  // }
  if ((t_direction != BE) && (t_direction != BW)) {
    sprintf(lcdString, "FATAL BLK DIR %i", t_direction); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  if (((t_locoNum < 1) || (t_locoNum > TOTAL_TRAINS)) && (t_locoNum != LOCO_ID_STATIC)) {
    sprintf(lcdString, "FATAL TRN NUM %i", t_direction); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);  // This will also check t_blockNum is valid
  }
  // Confirm not already reserved (even for this train; it should just never happen)
  if ((m_blockReservation.reservedForTrain != LOCO_ID_NULL) &&
      (m_blockReservation.reservedForTrain != LOCO_ID_STATIC)) {  // Whoops!  Already reserved (unless STATIC for 2nd level)
    sprintf(lcdString, "FATAL BLK %i RES'D!", t_blockNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  // Update the two fields, and then write the record back to FRAM.
  m_blockReservation.reservedForTrain = t_locoNum;
  m_blockReservation.reservedForDirection = t_direction;  // BE or BW
  Block_Reservation::setBlockReservation(t_blockNum);
  return;
}

void Block_Reservation::releaseBlock(const byte t_blockNum) {  // Could return bool if want to confirm it was previously reserved.
  // Rev: 01/17/23.
  // Retrieve entire Block Reservation record if not already loaded, update the two fields, then write back to FRAM.
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  m_blockReservation.reservedForTrain = LOCO_ID_NULL;
  m_blockReservation.reservedForDirection = BW;  // Doesn't matter which direction but MUST be BE or BW.
  Block_Reservation::setBlockReservation(t_blockNum);
  return;
}

void Block_Reservation::releaseAllBlocks() {
  // Rev: 01/17/23.
  // Should be called by Registrar *after* first reading in reservedForTrain, as default location for each train found.
  for (byte blockNum = 1; blockNum <= TOTAL_BLOCKS; blockNum++) {
    Block_Reservation::releaseBlock(blockNum);
  }
  return;
}

byte Block_Reservation::reservedForTrain(const byte t_blockNum) {
  // Rev: 01/17/23.
  // Returns train number this block is currently reserved for, including LOCO_ID_NULL and LOCO_ID_STATIC.
  // Expects t_blockNum to start at 1 not 0!  Returns train number starting at 1 (if a real train).
  // LOCO_ID_NULL = 0 = false, means not reserved; any other value is either a real or static loco (either way, it's reserved.)
  // Note: This function can be used as bool since "unreserved" is Train 0.
  // i.e. "if (BlockReservation.reservedForTrain(b))" then some train (including STATIC) has this block reserved.
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.reservedForTrain;
}

byte Block_Reservation::reservedDirection(const byte t_blockNum) {  // Returns const BE or BW if reserved, else undefined.
  // Rev: 01/17/23.
  // Note: Returns the contents of the "Reserved Direction" field whether the block is actually reserved or not.
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.reservedForDirection;
}

byte Block_Reservation::westSensor(const byte t_blockNum) {  // Returns sensor number 1..n
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.westSensor;
}

byte Block_Reservation::eastSensor(const byte t_blockNum) {  // Returns sensor number 1..n
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.eastSensor;
}

byte Block_Reservation::westboundSpeed(const byte t_blockNum) {  // Returns const speed 1-4 i.e. LOCO_SPEED_LOW.
  // Expects t_blockNum to start at 1 not 0!
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.westboundSpeed;
}

byte Block_Reservation::eastboundSpeed(const byte t_blockNum) {  // Returns const speed 1-4 i.e. LOCO_SPEED_LOW.
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.eastboundSpeed;
}

unsigned int Block_Reservation::length(const byte t_blockNum) {  // Returns block length in mm
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.length;
}

char Block_Reservation::sidingType(const byte t_blockNum) {  // Returns const i.e. SIDING_SINGLE_ENDED etc.  Useful for ???
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.sidingType;
}

bool Block_Reservation::isParkingSiding(const byte t_blockNum) {  // Returns true if we can park a train here in Park mode
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.isParkingSiding;
}

char Block_Reservation::stationType(const byte t_blockNum) {  // Returns const i.e. STATION_NOT_A_STATION
  // Expects t_blockNum to start at 1 not 0!
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.stationType;
}

char Block_Reservation::forbidden(const byte t_blockNum) {  // Returns const i.e. FORBIDDEN_NONE
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.forbidden;
}

bool Block_Reservation::isTunnel(const byte t_blockNum) {  // Returns true if block is in a tunnel at least part of the time.
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.isTunnel;
}

char Block_Reservation::gradeDirection(const byte t_blockNum) {  // Returns const i.e. GRADE_EASTBOUND
  if (t_blockNum != m_blockReservation.blockNum) {
    Block_Reservation::getBlockReservation(t_blockNum);
  }
  return m_blockReservation.gradeDirection;
}

void Block_Reservation::display(const byte t_blockNum) {
  // Rev: 01/24/23.
  // Display a single Block Reservation record; not the entire table.  For testing and debugging purposes only.
  // Could reduce the amount of memory required by using F() macro in Serial.print, but we only call this for debugging so won't
  // affect the amount of memory required in the final program.
  if ((t_blockNum < 1) || (t_blockNum > TOTAL_BLOCKS)) {  // Fatal error (program bug)
    sprintf(lcdString, "BAD DSP REC %i", t_blockNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  sprintf(lcdString, "Block: %3i ", t_blockNum); Serial.print(lcdString);
  sprintf(lcdString, "Loco: %3i ", Block_Reservation::reservedForTrain(t_blockNum)); Serial.print(lcdString);
  Serial.print("Dir ");
  if (Block_Reservation::reservedDirection(t_blockNum) == BE) {
    Serial.print("BE");
  }
  else if (Block_Reservation::reservedDirection(t_blockNum) == BW) {
    Serial.print("BW");
  }
  else {
    sprintf(lcdString, "BAD DIR %i", t_blockNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  sprintf(lcdString, ", W Sns %2i, ", Block_Reservation::westSensor(t_blockNum)); Serial.print(lcdString);
  sprintf(lcdString, "E Sns %2i, ", Block_Reservation::eastSensor(t_blockNum)); Serial.print(lcdString);
  sprintf(lcdString, "W Spd %2i, ", Block_Reservation::westboundSpeed(t_blockNum)); Serial.print(lcdString);
  sprintf(lcdString, "E Spd %2i, ", Block_Reservation::eastboundSpeed(t_blockNum)); Serial.print(lcdString);
  sprintf(lcdString, "Len %4i, ", Block_Reservation::length(t_blockNum)); Serial.print(lcdString);

  if (Block_Reservation::sidingType(t_blockNum) == SIDING_NOT_A_SIDING) {
    Serial.print(" S Type SIDING_NOT_A_SIDING, ");
  } else if (Block_Reservation::sidingType(t_blockNum) == SIDING_DOUBLE_ENDED) {
    Serial.print(" S Type SIDING_DOUBLE_ENDED, ");
  } else if (Block_Reservation::sidingType(t_blockNum) == SIDING_SINGLE_ENDED) {
    Serial.print(" S Type SIDING_SINGLE_ENDED, ");
  } else {
    Serial.print(" SIDING TYPE ERROR! ");Serial.println(Block_Reservation::sidingType(t_blockNum)); endWithFlashingLED(5);
  }

  if (Block_Reservation::isParkingSiding(t_blockNum)) {
    Serial.print(" IS Parking Siding,     ");
  } else {
    Serial.print(" Is NOT Parking Siding, ");
  }

  if (Block_Reservation::stationType(t_blockNum) == STATION_PASSENGER) {
    Serial.print(" Passenger station, ");
  }
  else if (Block_Reservation::stationType(t_blockNum) == STATION_FREIGHT) {
    Serial.print(" Freight station,   ");
  }
  else if (Block_Reservation::stationType(t_blockNum) == STATION_ANY_TYPE) {
    Serial.print(" Pass/Frt station,  ");
  }
  else if (Block_Reservation::stationType(t_blockNum) == STATION_NOT_A_STATION) {
    Serial.print(" NOT A STATION,     ");
  } else {
    Serial.print(" STATION TYPE ERROR! "); Serial.println(Block_Reservation::stationType(t_blockNum)); endWithFlashingLED(5);
  }

  if (Block_Reservation::forbidden(t_blockNum) == FORBIDDEN_NONE) {
    Serial.print(" FORBIDDEN_NONE,      ");
  }
  else if (Block_Reservation::forbidden(t_blockNum) == FORBIDDEN_PASSENGER) {
    Serial.print(" FORBIDDEN_PASSENGER, ");
  }
  else if (Block_Reservation::forbidden(t_blockNum) == FORBIDDEN_FREIGHT) {
    Serial.print(" FORBIDDEN_FREIGHT,   ");
  }
  else if (Block_Reservation::forbidden(t_blockNum) == FORBIDDEN_LOCAL) {
    Serial.print(" FORBIDDEN_LOCAL,     ");
  }
  else if (Block_Reservation::forbidden(t_blockNum) == FORBIDDEN_THROUGH) {
    Serial.print(" FORBIDDEN_THROUGH,   ");
  } else {
    Serial.print(" FORBIDDEN TYPE ERROR! "); Serial.println(Block_Reservation::forbidden(t_blockNum)); endWithFlashingLED(5);
  }

  if (Block_Reservation::isTunnel(t_blockNum)) {
    Serial.print(" Is Tunnel,     ");
  }
  else {
    Serial.print(" Is NOT Tunnel, ");
  }

  if (Block_Reservation::gradeDirection(t_blockNum) == GRADE_NOT_A_GRADE) {
    Serial.print(" GRADE_NOT_A_GRADE. ");
  } else if (Block_Reservation::gradeDirection(t_blockNum) == GRADE_EASTBOUND) {
    Serial.print(" GRADE_EASTBOUND.   ");
  } else if (Block_Reservation::gradeDirection(t_blockNum) == GRADE_WESTBOUND) {
    Serial.print(" GRADE_WESTBOUND.   ");
  } else {
    Serial.print(" GRADE TYPE ERROR! "); Serial.println(Block_Reservation::gradeDirection(t_blockNum)); endWithFlashingLED(5);
  }

  Serial.println();
}

void Block_Reservation::populate() {  // Populate the Block Reservation table with known constants, and init variable fields.
  // Rev: 01/25/23.
  // We ONLY need to call this from a utility program, whenever we need to refresh the FRAM Block Reservation table, such as if
  // some data changes (i.e. siding length, restrictions.)
  // NOTE: Block number 1..26 corresponds with FRAM element 0..25.
  // Careful don't call this m_blockReservation, which would confuse with our "regular" variable that holds just one record.

  // If we use a conventional (non-heap) array, we'll use twice as much SRAM as the array requires.
  // If we use a heap array (with "new"), we'll use the amount of SRAM that the array requires.  So "new" is better.
  // blockReservationStruct blockReservation[TOTAL_BLOCKS];   // 26 blocks, offset 0 thru 25
  blockReservationStruct* blockReservation = new blockReservationStruct[TOTAL_BLOCKS];

  //                      Blk  Reserved for Res   W   E  WB                 EB                  Len  Sid'g               Park?   St'n Type              Forbidden          Tnl?   Grade Up
  //                      Num  Train Num    Dir Sns Sns  Speed              Speed                mm  NDS                 TF      PFBN                   bPFLT              TF     Direction

  blockReservation[ 0] = {  1, LOCO_ID_NULL, BW,  1,  2, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM, 2629, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // Assuming using west-most sensor.
  blockReservation[ 1] = {  2, LOCO_ID_NULL, BW,  3,  4, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM, 2629, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // 
  blockReservation[ 2] = {  3, LOCO_ID_NULL, BW,  5,  6, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM, 2629, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // 
  blockReservation[ 3] = {  4, LOCO_ID_NULL, BW,  7,  8, LOCO_SPEED_LOW   , LOCO_SPEED_LOW   , 3061, SIDING_DOUBLE_ENDED, false, STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_WESTBOUND };  // Longer than 1-3 as it ends at bottom of grade, short of Turnout 09.
  blockReservation[ 4] = {  5, LOCO_ID_NULL, BW,  9, 10, LOCO_SPEED_LOW   , LOCO_SPEED_LOW   , 3353, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_THROUGH, false, GRADE_NOT_A_GRADE };  // Sidings 5 and 6 should start and stop parallel; should be 127" long each.
  blockReservation[ 5] = {  6, LOCO_ID_NULL, BW, 11, 12, LOCO_SPEED_LOW   , LOCO_SPEED_LOW   , 3353, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // Sidings 5 and 6 should start and stop parallel; should be 127" long each.
  blockReservation[ 6] = {  7, LOCO_ID_NULL, BW, 32, 31, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , true , GRADE_NOT_A_GRADE };  // 
  blockReservation[ 7] = {  8, LOCO_ID_NULL, BW, 34, 33, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // 
  blockReservation[ 8] = {  9, LOCO_ID_NULL, BW, 36, 35, LOCO_SPEED_LOW   , LOCO_SPEED_HIGH  ,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , false, GRADE_WESTBOUND };  // 
  blockReservation[ 9] = { 10, LOCO_ID_NULL, BW, 38, 37, LOCO_SPEED_LOW   , LOCO_SPEED_HIGH  ,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , false, GRADE_WESTBOUND };  // 
  blockReservation[10] = { 11, LOCO_ID_NULL, BW, 40, 39, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // 
  blockReservation[11] = { 12, LOCO_ID_NULL, BW, 41, 42, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , true , GRADE_NOT_A_GRADE };  // 
  blockReservation[12] = { 13, LOCO_ID_NULL, BW, 17, 18, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM, 9208, SIDING_DOUBLE_ENDED, false, STATION_ANY_TYPE     , FORBIDDEN_NONE   , true , GRADE_NOT_A_GRADE };  // This is an accurate measurement from start of sensor to start of sensor.
  blockReservation[13] = { 14, LOCO_ID_NULL, BW, 13, 14, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM, 3391, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // Sidings 14 and 15 should start and stop parallel; should be 122.5" long each.
  blockReservation[14] = { 15, LOCO_ID_NULL, BW, 15, 16, LOCO_SPEED_LOW   , LOCO_SPEED_LOW   , 3442, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // Sidings 14 and 15 should start and stop parallel; should be 122.5" long each.
  blockReservation[15] = { 16, LOCO_ID_NULL, BW, 43, 44, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , true , GRADE_NOT_A_GRADE };  // 
  blockReservation[16] = { 17, LOCO_ID_NULL, BW, 45, 46, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // 
  blockReservation[17] = { 18, LOCO_ID_NULL, BW, 47, 48, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // 
  blockReservation[18] = { 19, LOCO_ID_NULL, BW, 19, 20, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM, 3480, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // Length is just a guess.
  blockReservation[19] = { 20, LOCO_ID_NULL, BW, 21, 22, LOCO_SPEED_LOW   , LOCO_SPEED_LOW   , 3632, SIDING_DOUBLE_ENDED, true , STATION_ANY_TYPE     , FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // Length is just a guess.
  blockReservation[20] = { 21, LOCO_ID_NULL, BW, 51, 52, LOCO_SPEED_MEDIUM, LOCO_SPEED_MEDIUM,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , true , GRADE_NOT_A_GRADE };  // 
  blockReservation[21] = { 22, LOCO_ID_NULL, BW, 50, 49, LOCO_SPEED_LOW   , LOCO_SPEED_LOW   ,    0, SIDING_NOT_A_SIDING, false, STATION_NOT_A_STATION, FORBIDDEN_NONE   , false, GRADE_NOT_A_GRADE };  // 
  blockReservation[22] = { 23, LOCO_ID_NULL, BW, 23, 24, LOCO_SPEED_CRAWL , LOCO_SPEED_CRAWL , 2540, SIDING_SINGLE_ENDED, true , STATION_PASSENGER    , FORBIDDEN_FREIGHT, false, GRADE_NOT_A_GRADE };  // Length is just a guess.
  blockReservation[23] = { 24, LOCO_ID_NULL, BW, 25, 26, LOCO_SPEED_CRAWL , LOCO_SPEED_CRAWL , 2540, SIDING_SINGLE_ENDED, true , STATION_PASSENGER    , FORBIDDEN_FREIGHT, false, GRADE_NOT_A_GRADE };  // Length is just a guess.
  blockReservation[24] = { 25, LOCO_ID_NULL, BW, 27, 28, LOCO_SPEED_CRAWL , LOCO_SPEED_CRAWL , 2540, SIDING_SINGLE_ENDED, true , STATION_PASSENGER    , FORBIDDEN_FREIGHT, false, GRADE_NOT_A_GRADE };  // Length is just a guess.
  blockReservation[25] = { 26, LOCO_ID_NULL, BW, 29, 30, LOCO_SPEED_CRAWL , LOCO_SPEED_CRAWL , 2540, SIDING_SINGLE_ENDED, true , STATION_PASSENGER    , FORBIDDEN_FREIGHT, false, GRADE_NOT_A_GRADE };  // Length is just a guess.

  byte FRAMDataBuf[sizeof(blockReservationStruct)];
  for (byte blockNum = 1; blockNum <= TOTAL_BLOCKS; blockNum++) {
    memcpy(FRAMDataBuf, &blockReservation[blockNum - 1], sizeof(blockReservationStruct));  // Correcting for array element 0 = block 1
    m_pStorage->write(Block_Reservation::blockReservationAddress(blockNum), sizeof(blockReservationStruct), FRAMDataBuf);
  }
  delete[] blockReservation;  // Free up the heap array memory reserved by "new"
  m_pStorage->setFRAMRevDate(01, 26, 23);  // ALWAYS UPDATE FRAM DATE IF WE CHANGE A FILE!
  return;
}

// ***** PRIVATE FUNCTIONS ***

void Block_Reservation::getBlockReservation(const byte t_blockNum) {
  // Rev: 01/24/23.
  // Expects t_blockNum to start at 1 not 0!
  // FRAM read requires a "byte" pointer to the local data it's going to reading into, so I need to create that via casting.
  // First, create a new byte-type pointer pBlockReservation to our local struct variable m_blockReservation.
  byte* pBlockReservation = (byte*)(&m_blockReservation);
  m_pStorage->read(Block_Reservation::blockReservationAddress(t_blockNum), sizeof(blockReservationStruct), pBlockReservation);
  return;
}

void Block_Reservation::setBlockReservation(const byte t_blockNum) {
  // Rev: 01/24/23.
  // Expects t_blockNum to start at 1 not 0!
  byte* pBlockReservation = (byte*)(&m_blockReservation);
  m_pStorage->write(Block_Reservation::blockReservationAddress(t_blockNum), sizeof(blockReservationStruct), pBlockReservation);
  return;
}

unsigned long Block_Reservation::blockReservationAddress(const byte t_blockNum) {
  // Rev: 01/24/23.
  // Returns the FRAM byte address of the given record number in the Block Reservation table.
  // Expects t_blockNum to start at 1 not 0!  We correct for that here as FRAM starts reading at record 0.
  if ((t_blockNum < 1) || (t_blockNum > TOTAL_BLOCKS)) {
    sprintf(lcdString, "BAD BRA BLOCK %i", t_blockNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return FRAM_ADDR_BLOCK_RESN + ((t_blockNum - 1) * sizeof(blockReservationStruct));  // NOTE: We translate block num 1 to rec 0.
}
