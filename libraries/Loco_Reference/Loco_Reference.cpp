// LOCO_REFERENCE.CPP Rev: 07/29/24.
// A set of functions to retrieve data from the Locomotive Reference table, which is stored in FRAM.
// 06/21/24: Added SP 1440 as Engine 40.

#include "Loco_Reference.h"

Loco_Reference::Loco_Reference() {  // Constructor
  // Rev: 01/27/23.
  // Object holds ONE Loco Reference record (not the whole table)
  // Just initialize our internal Loco Reference structure element...
  // Need to initialize locoNum with non-valid value because that's what we look for to see if a record is already loaded.
  // There is no "Loco number 0" in the table, so this will always requre a lookup on the first access.
  // No point in initializing the rest of the fields here.
  m_locoReference.locoNum = 0;  // Actual loco numbers start at 1
  return;
}

void Loco_Reference::begin(FRAM* t_pStorage) {  // Just init the pointer to FRAM
  // Rev: 01/27/23.
  m_pStorage = t_pStorage;  // Pointer to FRAM
  if (m_pStorage == nullptr) {
    sprintf(lcdString, "UN-INIT'd LR PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;
}

// For many of these functions, we will see if the data already in the m_locoReference buffer matches the loco that the module is
// asking about.  If so, no need to do another FRAM get; otherwise, we'll need to do a get before returning the value from the buf.
// Since FRAM retrieval is almost as fast as SRAM, our performance savings is minimal at best, but what the heck.
// All of the following expect t_blockNum, t_locoNum, sensorNum, etc. to start at 1 not 0.  No checking of input ranges!

byte Loco_Reference::locoNum(const byte t_locoNum) {
  // Rev: 01/27/23.
  // Returns Lionel Legacy ID that the operator assigned to this loco/lash-up.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.locoNum;
}

bool Loco_Reference::active(const byte t_locoNum) {
  // Rev: 03/16/23.
  // Active implies ANY legit data in this record, even if not a loco (such as Stationsounds Diner or Crane car.)
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.active;
}

void Loco_Reference::alphaDesc(const byte t_locoNum, char* t_alphaDesc, const byte t_numBytes) {
  // Rev: 01/27/23.
  // Caller better pass a pointer to a char array that's 8 bytes long!  We just populate it.
  if (t_numBytes != ALPHA_WIDTH) {
    sprintf(lcdString, "LRD STR LEN %i", t_numBytes); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  for (byte i = 0; i < t_numBytes; i++) {  // i.e. 0..7
    t_alphaDesc[i] = m_locoReference.alphaDesc[i];
  }
  return;
}

char Loco_Reference::devType(const byte t_locoNum) {  // E|T|N|R but don't call this if PowerMaster or if devType is Accessory.
  // Rev: 05/03/24.
  if ((t_locoNum >= LOCO_ID_POWERMASTER_1) && (t_locoNum <= LOCO_ID_POWERMASTER_4)) {
    return DEV_TYPE_TMCC_ENGINE;
  }
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.devType;
}

char Loco_Reference::steamOrDiesel(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.steamOrDiesel;
}

char Loco_Reference::passOrFreight(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.passOrFreight;
}

void Loco_Reference::restrictions(const byte t_locoNum, char* t_alphaRestrictions, const byte t_numBytes) {
  // Rev: 01/27/23.
  // Caller better pass pointer to char array that's 5 bytes long (can be longer but we'll only populate 5)!
  if (t_numBytes != RESTRICT_WIDTH) {
    sprintf(lcdString, "RSTR STR LEN %i", t_numBytes); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  for (byte i = 0; i < t_numBytes; i++) {
    t_alphaRestrictions[i] = m_locoReference.restrictions[i];
  }
  return;
}

unsigned int Loco_Reference::length(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.length;
}

byte Loco_Reference::opCarLocoNum(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.opCarLocoNum;
}

byte Loco_Reference::crawlSpeed(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.crawlSpeed;
}

unsigned int Loco_Reference::crawlMmPerSec(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.crawlMmPerSec;
}

byte Loco_Reference::lowSpeed(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.lowSpeed;
}

unsigned int Loco_Reference::lowMmPerSec(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.lowMmPerSec;
}

byte Loco_Reference::lowSpeedSteps(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.lowSpeedSteps;
}

unsigned int Loco_Reference::lowMsStepDelay(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.lowMsStepDelay;
}

unsigned int Loco_Reference::lowMmToCrawl(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.lowMmToCrawl;
}

byte Loco_Reference::medSpeed(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.medSpeed;
}

unsigned int Loco_Reference::medMmPerSec(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.medMmPerSec;
}

byte Loco_Reference::medSpeedSteps(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.medSpeedSteps;
}

unsigned int Loco_Reference::medMsStepDelay(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.medMsStepDelay;
}

unsigned int Loco_Reference::medMmToCrawl(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.medMmToCrawl;
}

byte Loco_Reference::highSpeed(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.highSpeed;
}

unsigned int Loco_Reference::highMmPerSec(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.highMmPerSec;
}

byte Loco_Reference::highSpeedSteps(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.highSpeedSteps;
}

unsigned int Loco_Reference::highMsStepDelay(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.highMsStepDelay;
}

unsigned int Loco_Reference::highMmToCrawl(const byte t_locoNum) {
  // Rev: 01/27/23.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  return m_locoReference.highMmToCrawl;
}

void Loco_Reference::getDistanceAndMomentum(const byte t_locoNum, const byte t_currentSpeed, const unsigned int t_sidingLength,
                     byte* t_crawlSpeed, unsigned int* t_msDelayAfterEntry, byte* t_speedSteps, unsigned int* t_msStepDelay) {
  // Rev: 02/03/23.
  // If the Loco Reference record for this loco hasn't already been loaded, get it...
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  *t_crawlSpeed = m_locoReference.crawlSpeed;         // Legacy speed value for this loco at "Crawl."
  if ((t_currentSpeed == m_locoReference.lowSpeed) && (t_sidingLength >= m_locoReference.lowMmToCrawl)) {
    // We have a matching LOW speed *and* there is enough room to slow to Crawl!
    *t_msDelayAfterEntry = ((static_cast<float>(t_sidingLength) - m_locoReference.lowMmToCrawl) / m_locoReference.lowMmPerSec * 1000);
    *t_speedSteps = m_locoReference.lowSpeedSteps;    // Number of Legacy steps to drop at each speed decrease i.e. 5.
    *t_msStepDelay = m_locoReference.lowMsStepDelay;  // ms delay between successive reductions in Legacy speed
    return;
  } else if ((t_currentSpeed == m_locoReference.medSpeed) && (t_sidingLength >= m_locoReference.medMmToCrawl)) {
    // We have a matching MEDIUM speed *and* there is enough room to slow to Crawl!
    *t_msDelayAfterEntry = ((static_cast<float>(t_sidingLength) - m_locoReference.medMmToCrawl) / m_locoReference.medMmPerSec * 1000);
    *t_speedSteps = m_locoReference.medSpeedSteps;
    *t_msStepDelay = m_locoReference.medMsStepDelay;
    return;
  } else if ((t_currentSpeed == m_locoReference.highSpeed) && (t_sidingLength >= m_locoReference.highMmToCrawl)) {
    // We have a matching HIGH speed *and* there is enough room to slow to Crawl!
    *t_msDelayAfterEntry = ((static_cast<float>(t_sidingLength) - m_locoReference.highMmToCrawl) / m_locoReference.highMmPerSec * 1000);
    *t_speedSteps = m_locoReference.highSpeedSteps;
    *t_msStepDelay = m_locoReference.highMsStepDelay;
    return;
  }
  // Either we didn't have a matching L/M/H speed, *or* there isn't enough room for this loco to slow to Crawl.
  sprintf(lcdString, "LOCO %i BAD DATA", t_locoNum); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "SPEED %i", t_currentSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "LENGTH %i", t_sidingLength); pLCD2004->println(lcdString); Serial.println(lcdString);
  endWithFlashingLED(5);
  return;  // Will never execute
}

void Loco_Reference::display(const byte t_locoNum) {
  // Rev: 02/04/23.
  // Display a single Loco Reference record; not the entire table.  For testing and debugging purposes only.
  // Could reduce the amount of memory required by using F() macro in Serial.print, but we only call this for debugging so won't
  // affect the amount of memory required in the final program.
  if (t_locoNum != m_locoReference.locoNum) {
    Loco_Reference::getLocoReference(t_locoNum);
  }
  if ((t_locoNum < 1) || (t_locoNum > TOTAL_TRAINS)) {  // Fatal error (program bug)
    sprintf(lcdString, "BAD DSP REC %i", t_locoNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }

  sprintf(lcdString, "Loco: %3i '", Loco_Reference::locoNum(t_locoNum)); Serial.print(lcdString);

  // Get the 8-char alpha description of this loco
  char alphaDesc[] = {0, 0, 0, 0, 0, 0, 0, 0};  // Init to 8 chars
  char* pAlphaDesc = alphaDesc;
  Loco_Reference::alphaDesc(t_locoNum, pAlphaDesc, ALPHA_WIDTH);
  for (int i = 0; i < ALPHA_WIDTH; i++) {  // 0..7
    Serial.print(alphaDesc[i]);
  }
  Serial.print("', ");

  if (Loco_Reference::devType(t_locoNum) == DEV_TYPE_LEGACY_ENGINE) {
    Serial.print("Legacy Engine, ");
  } else if (Loco_Reference::devType(t_locoNum) == DEV_TYPE_LEGACY_TRAIN) {
    Serial.print("Legacy Train,  ");
  } else if (Loco_Reference::devType(t_locoNum) == DEV_TYPE_TMCC_ENGINE) {
    Serial.print("TMCC Engine,   ");
  } else if (Loco_Reference::devType(t_locoNum) == DEV_TYPE_TMCC_TRAIN) {
    Serial.print("TMCC Train,    ");
  } else {
    sprintf(lcdString, "BAD LR D.T. %c", Loco_Reference::devType(t_locoNum)); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }

  if (Loco_Reference::steamOrDiesel(t_locoNum) == POWER_TYPE_STEAM) {
    Serial.print("Steam,  ");
  } else if (Loco_Reference::steamOrDiesel(t_locoNum) == POWER_TYPE_DIESEL) {
    Serial.print("Diesel, ");
  } else if (Loco_Reference::steamOrDiesel(t_locoNum) == POWER_TYPE_SS_DINER) {
    Serial.print("Diner,  ");
  } else if (Loco_Reference::steamOrDiesel(t_locoNum) == POWER_TYPE_CRANE_BOOM) {
    Serial.print("Crane,  ");
  } else {
    sprintf(lcdString, "BAD S/D %c", Loco_Reference::steamOrDiesel(t_locoNum)); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }

  if (Loco_Reference::passOrFreight(t_locoNum) == LOCO_TYPE_PASS_EXPRESS) {
    Serial.print("Express Passenger , ");
  } else if (Loco_Reference::passOrFreight(t_locoNum) == LOCO_TYPE_PASS_LOCAL) {
    Serial.print("Local Passenger   , ");
  } else if (Loco_Reference::passOrFreight(t_locoNum) == LOCO_TYPE_FREIGHT_EXPRESS) {
    Serial.print("Express Freight   , ");
  } else if (Loco_Reference::passOrFreight(t_locoNum) == LOCO_TYPE_FREIGHT_LOCAL) {
    Serial.print("Local Freight     , ");
  } else if (Loco_Reference::passOrFreight(t_locoNum) == LOCO_TYPE_MOW) {
    Serial.print("Maintenance-Of-Way, ");
  } else {
    sprintf(lcdString, "BAD P/F %c", Loco_Reference::passOrFreight(t_locoNum)); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }

  // Get the 5-char restrictions field.  No need to re-init the chars we used for alphaDesc, they will be overwritten...
  Serial.print(" Restrictions: '");
  Loco_Reference::restrictions(t_locoNum, pAlphaDesc, RESTRICT_WIDTH);
  for (int i = 0; i < RESTRICT_WIDTH; i++) {  // 0..4
    Serial.print(alphaDesc[i]);
  }
  Serial.print("', ");

  sprintf(lcdString, "Length %4i, ", Loco_Reference::length(t_locoNum)); Serial.print(lcdString);

  sprintf(lcdString, "Op car %2i.", Loco_Reference::opCarLocoNum(t_locoNum)); Serial.println(lcdString);

  sprintf(lcdString, "Crawl Legacy %3i, ", Loco_Reference::crawlSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i mm/sec.", Loco_Reference::crawlMmPerSec(t_locoNum)); Serial.println(lcdString);

  sprintf(lcdString, "Low   Legacy %3i, ", Loco_Reference::lowSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i mm/sec, ", Loco_Reference::lowMmPerSec(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "Step %2i, ", Loco_Reference::lowSpeedSteps(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "Delay %4i, ", Loco_Reference::lowMsStepDelay(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i mm to Crawl.", Loco_Reference::lowMmToCrawl(t_locoNum)); Serial.println(lcdString);

  sprintf(lcdString, "Med   Legacy %3i, ", Loco_Reference::medSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i mm/sec, ", Loco_Reference::medMmPerSec(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "Step %2i, ", Loco_Reference::medSpeedSteps(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "Delay %4i, ", Loco_Reference::medMsStepDelay(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i mm to Crawl.", Loco_Reference::medMmToCrawl(t_locoNum)); Serial.println(lcdString);

  sprintf(lcdString, "High  Legacy %3i, ", Loco_Reference::highSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i mm/sec, ", Loco_Reference::highMmPerSec(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "Step %2i, ", Loco_Reference::highSpeedSteps(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "Delay %4i, ", Loco_Reference::highMsStepDelay(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i mm to Crawl.", Loco_Reference::highMmToCrawl(t_locoNum)); Serial.println(lcdString);

  Serial.println();
  return;
}

void Loco_Reference::populate() {
  // Rev: 02/17/24.
  // Populate the Loco Reference table with known constants, and init variable fields.
  // We ONLY need to call this from a utility program, whenever we need to refresh the FRAM Loco Reference table, such as if
  // some data changes (i.e. speed parameters most likely.)
  // Note that loco number 1 corresponds with element 0.
  // Careful don't call this m_locoReference, which would confuse with our "regular" variable that holds just one record.
  // SIDING LENGTH: The SHORTEST siding is 104" = 8' 8" = 2641mm.  Thus, longest HIGH to CRAWL must be LESS THAN 2641mm.
  // Serial.println(F("Memory in populateLoco before new: ")); freeMemory();
  unsigned int numRecsToLoad = TOTAL_TRAINS;  // 0..TOTAL_TRAINS - 1; 50 trains.  In case I ever need to do this in batches.
  locoReferenceStruct* lr = new locoReferenceStruct[numRecsToLoad];  // Offset 0 thru (TOTAL_TRAINS - 1)
  //locoReferenceStruct lr[TOTAL_TRAINS];  // This is the way to do it using SRAM instead of heap
  // Serial.println(F("Memory in populateLoco after new: ")); freeMemory();
  //      Loco  Active?                            Dev Stm Pas                      Len Op Car Crawl Crawl   Low   Low   Low   Low   Low   Med   Med   Med   Med   Med  High  High  High  High  High
  //       Num  |   1   2   3   4   5   6   7   8  Typ Dsl Frt ...Restrictions...    mm Loco # Leg'y  mm/s Leg'y  mm/s  Step Delay  Dist Leg'y  mm/s  Step Delay  Dist Leg'y  mm/s  Step Delay  Dist
  lr[ 0] = { 1, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[ 1] = { 2, 1, 'S','H','A','Y',' ',' ',' ',' ','E','S','p',' ',' ',' ',' ',' ',1000,   0,     19,   23,   37,   46,    1,  500,  295,   63,   93,    1,  320,  762,   84,  140,    1,  320, 1549};  // SPEED & DECEL DATA COMPLETE 7/29/24
  lr[ 2] = { 3, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[ 3] = { 4, 1, 'A','T','S','F',' ','N','W','2','E','D','f',' ',' ',' ',' ',' ',1000,   0,     25,   28,   53,   70,    1,  160,  277,   85,  139,    2,  320,  800,  117,  233,    2,  320, 1772};  // SPEED & DECEL DATA COMPLETE 7/29/24
  lr[ 4] = { 5, 1, 'A','T','S','F','1','4','8','4','E','S','p',' ',' ',' ',' ',' ',1000,   0,      9,   28,   28,   70,    1,  290,  254,   49,  132,    2,  580,  819,   79,  260,    2,  580, 2515};  // SPEED & DECEL DATA COMPLETE 7/29/24
  lr[ 5] = { 6, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[ 6] = { 7, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[ 7] = { 8, 1, 'W','P',' ',' ','8','0','3','A','T','D','F',' ',' ',' ',' ',' ',1000,   0,      7,   23,   37,   94,    3,  350,  191,   93,  330,    3,  350, 1514,  114,  463,    3,  350, 2527};  // SPEED & DECEL DATA COMPLETE 7/29/24
  lr[ 8] = { 9, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[ 9] = {10, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[10] = {11, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[11] = {12, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[12] = {13, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[13] = {14, 0, 'B','I','G',' ','B','O','Y',' ','E','D','F',' ',' ',' ',' ',' ',1000,   0,      8,   23,   26,   95,    3, 1000,    0,   74,  239,    3, 1000,    0,  100,  384,    3, 1000,    0};  // Speeds are good as of 7/29/24 but have yet to determine decel parms
  lr[14] = {15, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[15] = {16, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[16] = {17, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[17] = {18, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[18] = {19, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[19] = {20, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[20] = {21, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[21] = {22, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[22] = {23, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[23] = {24, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[24] = {25, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[25] = {26, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[26] = {27, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[27] = {28, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[28] = {29, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[29] = {30, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[30] = {31, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[31] = {32, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[32] = {33, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[33] = {34, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[34] = {35, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[35] = {36, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[36] = {37, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[37] = {38, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[38] = {39, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[39] = {40, 1, 'S','P',' ',' ','1','4','4','0','E','D','F',' ',' ',' ',' ',' ',1000,   0,      9,   24,   30,   70,    1,  280,  286,   52,  140,    2,  510,  822,   74,  233,    2,  480, 1753};  // SPEED & DECEL DATA COMPLETE 7/29/24
  lr[40] = {41, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[41] = {42, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[42] = {43, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[43] = {44, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[44] = {45, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[45] = {46, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[46] = {47, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[47] = {48, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[48] = {49, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};
  lr[49] = {50, 0, ' ',' ',' ',' ',' ',' ',' ',' ','E','D','F',' ',' ',' ',' ',' ',   0,   0,      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0};

  byte* pFRAMDataBuf = new byte[sizeof(locoReferenceStruct)];
  // byte FRAMDataBuf[sizeof(locoReferenceStruct)];  // This is the way to do it using SRAM instead of heap
  // byte* pFRAMDataBuf = FRAMDataBuf;  // Since FRAMDataBuf is an array, the name is the address.  Same as &FRAMDataBuf[0].

  for (byte locoNum = 1; locoNum <= numRecsToLoad; locoNum++) {  // i.e. 1..50
    memcpy(pFRAMDataBuf, &lr[locoNum - 1], sizeof(locoReferenceStruct));  // Correcting for array element 0 = loco 1
    // memcpy(FRAMDataBuf, &lr[locoNum - 1], sizeof(locoReferenceStruct));  // This is the way to do it using SRAM instead of heap
    m_pStorage->write(Loco_Reference::locoReferenceAddress(locoNum), sizeof(locoReferenceStruct), pFRAMDataBuf);
    // m_pStorage->write(Loco_Reference::locoReferenceAddress(locoNum), sizeof(locoReferenceStruct), FRAMDataBuf);  // SRAM method.

    // for (int i = 1; i < 9; i++) {  // To prove that our memcpy() worked
    //   Serial.print((char)pFRAMDataBuf[i]);
    //   // Serial.print(FRAMDataBuf[i]);  // This is the way to do it using SRAM instead of heap
    // }
    // Serial.println("'");

  }

  // Serial.println(F("Memory in populateLoco after writing w/ pFRAMDataBuf, before delete: ")); freeMemory();
  delete[] pFRAMDataBuf;  // IMPORTANT!  Must delete pFRAMDataBuf before lr if we want to free up lr memory; the order is important.
  delete[] lr;  // Free up the heap array memory reserved by "new"
  // Serial.println(F("Memory in populateLoco after delete: ")); freeMemory();
  m_pStorage->setFRAMRevDate(01, 26, 23);  // ALWAYS UPDATE FRAM DATE IF WE CHANGE A FILE!
  return;
}

// ***** PRIVATE FUNCTIONS ***

void Loco_Reference::getLocoReference(const byte t_locoNum) {
  // Rev: 01/27/23.
  // Expects t_locoNum to start at 1 not 0!
  // FRAM read requires a "byte" pointer to the local data it's going to reading into, so I need to create that via casting.
  // First, create a new byte-type pointer pLocoReferenceElement to our local struct variable pLocoReference.
  byte* pLocoReference = (byte*)&m_locoReference;
  m_pStorage->read(Loco_Reference::locoReferenceAddress(t_locoNum), sizeof(locoReferenceStruct), pLocoReference);
  return;
}

void Loco_Reference::setLocoReference(const byte t_locoNum) {
  // Rev: 01/27/23.
  // Expects t_locoNum to start at 1 not 0!
  // Rather than reading from FRAMDataBuf, I'm reading directly from the local struct variable.
  byte* pLocoReference = (byte*)&m_locoReference;
  m_pStorage->write(Loco_Reference::locoReferenceAddress(t_locoNum), sizeof(locoReferenceStruct), pLocoReference);
  return;
}

unsigned long Loco_Reference::locoReferenceAddress(const byte t_locoNum) {  // Return the starting address of the *record* in the Loco Reference FRAM table for this sensor
  // Rev: 01/27/23.
  // Returns the FRAM byte address of the given record number in the Loco Reference table.
  // Expects t_locoNum to start at 1 not 0!  We correct for that here.
  if ((t_locoNum < 1) || (t_locoNum > TOTAL_TRAINS)) {
    sprintf(lcdString, "BAD LRT LOCO %i", t_locoNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return FRAM_ADDR_LOCO_REF + ((t_locoNum - 1) * sizeof(locoReferenceStruct));  // NOTE: We translate loco number 1 to record 0.
}
