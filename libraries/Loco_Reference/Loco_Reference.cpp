// LOCO_REFERENCE.CPP Rev: 02/17/24.
// A set of functions to retrieve data from the Locomotive Reference table, which is stored in FRAM.

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

  if ((Loco_Reference::devType(t_locoNum) != DEV_TYPE_LEGACY_ENGINE) &&
      (Loco_Reference::devType(t_locoNum) != DEV_TYPE_LEGACY_TRAIN) &&
      (Loco_Reference::devType(t_locoNum) != DEV_TYPE_TMCC_ENGINE) &&
      (Loco_Reference::devType(t_locoNum) != DEV_TYPE_TMCC_TRAIN)) {
       sprintf(lcdString, "BAD LR D.T. %c", Loco_Reference::devType(t_locoNum)); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);

  }
  Serial.print("TMCC/LEG ENG/TRAIN E|T|N|R: "); Serial.println(Loco_Reference::devType(t_locoNum));

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

  sprintf(lcdString, "Op car %2i, ", Loco_Reference::opCarLocoNum(t_locoNum)); Serial.print(lcdString);

  sprintf(lcdString, "%3i ", Loco_Reference::crawlSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%3i ", Loco_Reference::crawlMmPerSec(t_locoNum)); Serial.print(lcdString);

  sprintf(lcdString, "%3i ", Loco_Reference::lowSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%3i ", Loco_Reference::lowMmPerSec(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%2i ", Loco_Reference::lowSpeedSteps(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i ", Loco_Reference::lowMsStepDelay(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i ", Loco_Reference::lowMmToCrawl(t_locoNum)); Serial.print(lcdString);

  sprintf(lcdString, "%3i ", Loco_Reference::medSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%3i ", Loco_Reference::medMmPerSec(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%2i ", Loco_Reference::medSpeedSteps(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i ", Loco_Reference::medMsStepDelay(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i ", Loco_Reference::medMmToCrawl(t_locoNum)); Serial.print(lcdString);

  sprintf(lcdString, "%3i ", Loco_Reference::highSpeed(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%3i ", Loco_Reference::highMmPerSec(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%2i ", Loco_Reference::highSpeedSteps(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i ", Loco_Reference::highMsStepDelay(t_locoNum)); Serial.print(lcdString);
  sprintf(lcdString, "%4i ", Loco_Reference::highMmToCrawl(t_locoNum)); Serial.print(lcdString);

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
  Serial.println(F("Memory in populateLoco before new: ")); freeMemory();
  unsigned int numRecsToLoad = TOTAL_TRAINS;  // 0..TOTAL_TRAINS - 1; 50 trains.  In case I ever need to do this in batches.
  locoReferenceStruct* lr = new locoReferenceStruct[numRecsToLoad];  // Offset 0 thru (TOTAL_TRAINS - 1)
    //locoReferenceStruct lr[TOTAL_TRAINS];  // This is the way to do it using SRAM instead of heap
  Serial.println(F("Memory in populateLoco after new: ")); freeMemory();
  //      Loco  Active?                            Dev Stm Pas                      Len Op Car Crawl Crawl   Low   Low   Low   Low   Low   Med   Med   Med   Med   Med  High  High  High  High  High
  //       Num  |   1   2   3   4   5   6   7   8  Typ Dsl Frt ...Restrictions...    mm Loco # Leg'y  mm/s Leg'y  mm/s  Step Delay  Dist Leg'y  mm/s  Step Delay  Dist Leg'y  mm/s  Step Delay  Dist
  lr[ 0] = { 1, 1, 'S','H','A','Y',' ',' ',' ',' ','E','D','F','z','x','q','s','r',1510,   0,     20,   47,   38,   93,    2,  320,  603,   64,  186,    2,  320, 2007,   83,  279,    2,  320, 3289};
  lr[ 1] = { 2, 0, 'B','L','G','D','2','9','6','3','T','D','p',' ',' ',' ',' ',' ',1520,   0,     21,   48,   39,   94,    2,  330,  703,   65,  187,    2,  420, 2107,   93,  289,    2,  329, 3189};
  lr[ 2] = { 3, 0, 'C','M','K',' ','3','8','7','2','E','S','P','s','d','f','h','j',1530,   2,     22,   49,   30,   95,    3,  340,  803,   66,  188,    4,  520, 2207,   03,  299,    4,  328, 3089};
  lr[ 3] = { 4, 1, 'S','F',' ','N','W','-','2',' ','E','S','f',' ',' ',' ',' ',' ',1535,   0,     20,   23,   63,   90,    2,  500,  622,  100,  180,    2,  500, 1867,  114,  223,    2,  400, 2096};
  lr[ 4] = { 5, 0, 'D','N','J',' ','4','7','8','1','E','D','F',' ',' ',' ',' ',' ',1540,   0,     24,   41,   32,   97,    2,  360,  003,   83,  180,    2,  720, 2407,   23,  219,    2,  326, 3889};
  lr[ 5] = { 6, 0, 'F','P','G','T','6','5','0','9','T','D','P',' ',' ',' ',' ',' ',1560,   0,     25,   42,   33,   98,    2,  370,  103,   69,  181,    2,  820, 2507,   33,  229,    2,  325, 3789};
  lr[ 6] = { 7, 0, 'G','Q','F','R','7','4','1','8','E','S','P',' ',' ',' ',' ',' ',1570,   0,     26,   43,   34,   99,    2,  380,  203,   60,  182,    2,  920, 2607,   43,  239,    2,  324, 3689};
  lr[ 7] = { 8, 0, 'H','R','D','E','8','3','2','7','T','S','F',' ',' ',' ',' ',' ',1580,   7,     27,   44,   35,   10,    2,  390,  303,   61,  183,    2,  120, 2707,   53,  249,    2,  323, 3589};
  lr[ 8] = { 9, 0, 'I','S','S','W','9','2','3','6','E','D','F',' ',' ',' ',' ',' ',1590,   0,     28,   45,   36,   01,    2,  400,  403,   62,  184,    2,  220, 2807,   63,  259,    2,  322, 3489};
  lr[ 9] = {10, 0, 'J','T','A','Q','0','1','4','5','T','D','P',' ',' ',' ',' ',' ',1600,   0,     29,   46,   37,   02,    2,  410,  503,   63,  185,    2,  320, 2907,   73,  269,    2,  321, 3389};
  lr[10] = {11, 1, 'W','P',' ',' ',' ',' ','5','4','E','D','F',' ',' ',' ',' ',' ',1511,   0,     30,   57,   38,   93,    2,  320,  603,   64,  186,    2,  320, 2007,   83,  279,    2,  320, 3289};
  lr[11] = {12, 0, 'B','L','G','D','2','9','6','3','T','D','P',' ',' ',' ',' ',' ',1521,  14,     31,   58,   39,   94,    2,  330,  703,   65,  187,    2,  420, 2107,   93,  289,    2,  329, 3189};
  lr[12] = {13, 0, 'C','M','K',' ','3','8','7','2','E','S','P',' ',' ',' ',' ',' ',1531,   0,     32,   59,   30,   95,    2,  340,  803,   66,  188,    2,  520, 2207,   03,  299,    2,  328, 3089};
  lr[13] = {14, 0, 'D','N','J',' ','4','7','8','1','T','S','F',' ',' ',' ',' ',' ',1541,   0,     33,   50,   31,   96,    2,  350,  903,   67,  189,    2,  620, 2307,   13,  209,    2,  327, 3989};
  lr[14] = {15, 0, 'E','O','H','Y','5','6','9','0','E','D','F',' ',' ',' ',' ',' ',1551,   0,     34,   51,   32,   97,    2,  360,  003,   68,  180,    2,  720, 2407,   23,  219,    2,  326, 3889};
  lr[15] = {16, 0, 'F','P','G','T','6','5','0','9','T','D','P',' ',' ',' ',' ',' ',1561,   0,     35,   52,   33,   98,    2,  370,  103,   69,  181,    2,  820, 2507,   33,  229,    2,  325, 3789};
  lr[16] = {17, 0, 'G','Q','F','R','7','4','1','8','E','S','P',' ',' ',' ',' ',' ',1571,   0,     36,   53,   34,   99,    2,  380,  203,   60,  182,    2,  920, 2607,   43,  239,    2,  324, 3689};
  lr[17] = {18, 0, 'H','R','D','E','8','3','2','7','T','S','F',' ',' ',' ',' ',' ',1581,   0,     37,   54,   35,   10,    2,  390,  303,   61,  183,    2,  120, 2707,   53,  249,    2,  323, 3589};
  lr[18] = {19, 0, 'I','S','S','W','9','2','3','6','E','D','F',' ',' ',' ',' ',' ',1591,  22,     38,   55,   36,   01,    2,  400,  403,   62,  184,    2,  220, 2807,   63,  259,    2,  322, 3489};
  lr[19] = {20, 0, 'J','T','A','Q','0','1','4','5','T','D','P',' ',' ',' ',' ',' ',1601,   0,     39,   56,   37,   02,    2,  410,  503,   63,  185,    2,  320, 2907,   73,  269,    2,  321, 3389};
  lr[20] = {21, 1, 'W','P',' ',' ',' ',' ','5','4','E','D','F',' ',' ',' ',' ',' ',1512,   0,     40,   67,   38,   93,    2,  320,  603,   64,  186,    2,  320, 2007,   83,  279,    2,  320, 3289};
  lr[21] = {22, 0, 'B','L','G','D','2','9','6','3','T','D','P',' ',' ',' ',' ',' ',1522,   0,     41,   68,   39,   94,    2,  330,  703,   65,  187,    2,  420, 2107,   93,  289,    2,  329, 3189};
  lr[22] = {23, 0, 'C','M','K',' ','3','8','7','2','E','S','P',' ',' ',' ',' ',' ',1532,   0,     42,   69,   30,   95,    2,  340,  803,   66,  188,    2,  520, 2207,   03,  299,    2,  328, 3089};
  lr[23] = {24, 0, 'D','N','J',' ','4','7','8','1','T','S','F',' ',' ',' ',' ',' ',1542,   0,     43,   60,   31,   96,    2,  350,  903,   67,  189,    2,  620, 2307,   13,  209,    2,  327, 3989};
  lr[24] = {25, 0, 'E','O','H','Y','5','6','9','0','E','D','F',' ',' ',' ',' ',' ',1552,   0,     44,   61,   32,   97,    2,  360,  003,   68,  180,    2,  720, 2407,   23,  219,    2,  326, 3889};
  lr[25] = {26, 0, 'F','P','G','T','6','5','0','9','T','D','P',' ',' ',' ',' ',' ',1562,   0,     45,   62,   33,   98,    2,  370,  103,   69,  181,    2,  820, 2507,   33,  229,    2,  325, 3789};
  lr[26] = {27, 0, 'G','Q','F','R','7','4','1','8','E','S','P',' ',' ',' ',' ',' ',1572,   2,     46,   63,   34,   99,    2,  380,  203,   60,  182,    2,  920, 2607,   43,  239,    2,  324, 3689};
  lr[27] = {28, 0, 'H','R','D','E','8','3','2','7','T','S','F',' ',' ',' ',' ',' ',1582,   0,     47,   64,   35,   05,    2,  390,  303,   61,  183,    2,  120, 2707,   53,  249,    2,  323, 3589};
  lr[28] = {29, 0, 'I','S','S','W','9','2','3','6','E','D','F',' ',' ',' ',' ',' ',1592,   0,     48,   65,   36,   01,    2,  400,  403,   62,  184,    2,  220, 2807,   63,  259,    2,  322, 3489};
  lr[29] = {30, 0, 'J','T','A','Q','0','1','4','5','T','D','P',' ',' ',' ',' ',' ',1602,   0,     49,   66,   37,   02,    2,  410,  503,   63,  185,    2,  320, 2907,   73,  269,    2,  321, 3389};
  lr[30] = {31, 1, 'W','P',' ',' ',' ',' ','5','4','E','D','F',' ',' ',' ',' ',' ',1513,   0,     50,   77,   38,   93,    2,  320,  603,   64,  186,    2,  320, 2007,   83,  279,    2,  320, 3289};
  lr[31] = {32, 0, 'B','L','G','D','2','9','6','3','T','D','P',' ',' ',' ',' ',' ',1523,   0,     51,   78,   39,   94,    2,  330,  703,   65,  187,    2,  420, 2107,   93,  289,    2,  329, 3189};
  lr[32] = {33, 0, 'C','M','K',' ','3','8','7','2','E','S','P',' ',' ',' ',' ',' ',1533,   0,     52,   79,   30,   95,    2,  340,  803,   66,  188,    2,  520, 2207,   03,  299,    2,  328, 3089};
  lr[33] = {34, 0, 'D','N','J',' ','4','7','8','1','T','S','F',' ',' ',' ',' ',' ',1543,   0,     53,   70,   31,   96,    2,  350,  903,   67,  189,    2,  620, 2307,   13,  209,    2,  327, 3989};
  lr[34] = {35, 0, 'E','O','H','Y','5','6','9','0','E','D','F',' ',' ',' ',' ',' ',1553,   0,     54,   71,   32,   97,    2,  360,  003,   68,  180,    2,  720, 2407,   23,  219,    2,  326, 3889};
  lr[35] = {36, 0, 'F','P','G','T','6','5','0','9','T','D','P',' ',' ',' ',' ',' ',1563,   0,     55,   72,   33,   98,    2,  370,  103,   69,  181,    2,  820, 2507,   33,  229,    2,  325, 3789};
  lr[36] = {37, 0, 'G','Q','F','R','7','4','1','8','E','S','P',' ',' ',' ',' ',' ',1573,   0,     56,   73,   34,   99,    2,  380,  203,   60,  182,    2,  920, 2607,   43,  239,    2,  324, 3689};
  lr[37] = {38, 0, 'H','R','D','E','8','3','2','7','T','S','F',' ',' ',' ',' ',' ',1583,   0,     57,   74,   35,   05,    2,  390,  303,   61,  183,    2,  120, 2707,   53,  249,    2,  323, 3589};
  lr[38] = {39, 0, 'I','S','S','W','9','2','3','6','E','D','F',' ',' ',' ',' ',' ',1593,   0,     58,   75,   36,   01,    2,  400,  403,   62,  184,    2,  220, 2807,   63,  259,    2,  322, 3489};
  lr[39] = {40, 0, 'J','T','A','Q','0','1','4','5','T','D','P',' ',' ',' ',' ',' ',1603,   0,     59,   76,   37,   02,    2,  410,  503,   63,  185,    2,  320, 2907,   73,  269,    2,  321, 3389};
  lr[40] = {41, 1, 'W','P',' ',' ',' ',' ',' ',' ','E','1','p',' ',' ',' ',' ',' ',1514,   5,     60,   87,   38,   93,    2,  320,  603,   64,  186,    2,  320, 2007,   83,  279,    2,  320, 3289};
  lr[41] = {42, 0, 'B','L','G','D','2','9','6','3','T','2','P',' ',' ',' ',' ',' ',1524,   0,     61,   88,   39,   94,    2,  330,  703,   65,  187,    2,  420, 2107,   93,  289,    2,  329, 3189};
  lr[42] = {43, 0, 'C','M','K',' ','3','8','7','2','E','S','f',' ',' ',' ',' ',' ',1534,   0,     62,   89,   30,   95,    2,  340,  803,   66,  188,    2,  520, 2207,   03,  299,    2,  328, 3089};
  lr[43] = {44, 0, 'D','N','J',' ','4','7','8','1','T','S','F',' ',' ',' ',' ',' ',1544,   0,     63,   80,   31,   96,    2,  350,  903,   67,  189,    2,  620, 2307,   13,  209,    2,  327, 3989};
  lr[44] = {45, 0, 'E','O','H','Y','5','6','9','0','E','D','M',' ',' ',' ',' ',' ',1554,   0,     64,   81,   32,   97,    2,  360,  003,   68,  180,    2,  720, 2407,   23,  219,    2,  326, 3889};
  lr[45] = {46, 0, 'F','P','G','T','6','5','0','9','T','D','P',' ',' ',' ',' ',' ',1564,   0,     65,   82,   33,   98,    2,  370,  103,   69,  181,    2,  820, 2507,   33,  229,    2,  325, 3789};
  lr[46] = {47, 0, 'G','Q','F','R','7','4','1','8','E','S','P',' ',' ',' ',' ',' ',1574,   0,     66,   83,   34,   99,    2,  380,  203,   60,  182,    2,  920, 2607,   43,  239,    2,  324, 3689};
  lr[47] = {48, 0, 'H','R','D','E','8','3','2','7','T','S','F',' ',' ',' ',' ',' ',1584,   0,     67,   84,   35,   05,    2,  390,  303,   61,  183,    2,  120, 2707,   53,  249,    2,  323, 3589};
  lr[48] = {49, 0, 'I','S','S','W','9','2','3','6','E','D','F',' ',' ',' ',' ',' ',1594,   0,     68,   85,   36,   01,    2,  400,  403,   62,  184,    2,  220, 2807,   63,  259,    2,  322, 3489};
  lr[49] = {50, 0, 'J','T','A','Q','0','1','4','5','T','D','P','c','d','e','f','g',1604,  30,     69,   86,   37,   02,    2,  410,  503,   63,  185,    2,  320, 2907,   73,  269,    2,  321, 3389};

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

  Serial.println(F("Memory in populateLoco after writing w/ pFRAMDataBuf, before delete: ")); freeMemory();
  delete[] pFRAMDataBuf;  // IMPORTANT!  Must delete pFRAMDataBuf before lr if we want to free up lr memory; the order is important.
  delete[] lr;  // Free up the heap array memory reserved by "new"
  Serial.println(F("Memory in populateLoco after delete: ")); freeMemory();
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
