// TURNOUT_RESERVATION.CPP Rev: 04/14/24.  FINISHED.
// A set of functions to read and update the Turnout Reservation table, which is stored in FRAM.

#include <Turnout_Reservation.h>

Turnout_Reservation::Turnout_Reservation() {  // Constructor
  // Rev: 01/25/23.
  // Object holds ONE Turnout Reservation record (not the whole table)
  // Just initialize our internal turnout-reservation structure element...
  m_turnoutReservation.turnoutNum       = 0;    // Zero is *not* a real turnout so this will never be legal.
  m_turnoutReservation.lastPosition     = TURNOUT_DIR_NORMAL;  // This won't clobber anything; just giving it a value.
  m_turnoutReservation.reservedForTrain = LOCO_ID_NULL;
  return;
}

void Turnout_Reservation::begin(FRAM* t_pStorage) {
  // Rev: 01/25/23.
  // Sets the first two fields of every record to 0, blank.  Called at beginning of registration.
  m_pStorage = t_pStorage;  // Pointer to FRAM
  if (m_pStorage == nullptr) {
    sprintf(lcdString, "UN-INIT'd TR PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  Turnout_Reservation::releaseAll();
  return;
}

// For these functions, we should see if the data already in the m_turnoutReservation buffer matches the turnout that the module
// is asking about.  If so, no need to do another FRAM get; otherwise, do a get before returning the value from the buffer.

void Turnout_Reservation::setLastOrientation(const byte t_turnoutNum, const char t_position) {
  // Rev: 04/14/24.
  // Retrieve the entire Turnout Reservation record (if not already loaded), update the "lastPosition" field to 'N'ormal or
  // 'R'everse (t_position, not error checked,), then write the record back to FRAM w/o affecting which train is reserved for.
  // Expects t_turnoutNum to start at 1 not 0!  t_position better be 'N' or 'R'; no check for error.
  // We need this so that in Manual mode, when the operator presses a turnout button on the control panel, we'll know to throw it
  // in the opposite orientation that it's currently set.  No need to track outside of Manual mode; we'll initialize each turnout's
  // orientation each time we start Manual mode.
  if (t_turnoutNum != m_turnoutReservation.turnoutNum) {
    Turnout_Reservation::getReservation(t_turnoutNum);
  }
  m_turnoutReservation.lastPosition = t_position;
  Turnout_Reservation::setReservation(t_turnoutNum);
  return;
}

char Turnout_Reservation::getLastOrientation(const byte t_turnoutNum) {
  // Rev: 04/14/24.
  // Get and return a turnout's "last-known" orientation: 'N'ormal or 'R'everse (not guaranteed to reflect actual position.)
  // Expects t_turnoutNum to start at 1 not 0!  Returns whatever is in the "lastPosition" field, even if it's not 'N' or 'R'.
  // This is only valid data in Manual mode as it's not tracked in other modes.
  if (t_turnoutNum != m_turnoutReservation.turnoutNum) {
    Turnout_Reservation::getReservation(t_turnoutNum);
  }
  return m_turnoutReservation.lastPosition;
}

void Turnout_Reservation::reserveTurnout(const byte t_turnoutNum, const byte t_locoNum) {
  // Rev: 01/25/23.
  // Retrieve the entire Turnout Reservation record (if not already loaded), update the "reservedForTrain" field, and then write
  // the record back to FRAM.
  // Reserving a turnout DOES NOT update the last-known orientation -- so we can use last-known even after released.
  // Expects t_turnoutNum to start at 1 not 0!  Expects real train numbers i.e. starting at 1 not 0.
  // Could return bool if we want to confirm not previously reserved.
  // NOTE: If we try to reserve a turnout for a train that's already reserved for another train, we're calling that a fatal error.
  //       This can easily be changed here if we want to make reserving an already-reserved turnout AOK.
  // 12/12/20: PROBLEM RESERVING TURNOUT #17.  I have no idea why but it repeated several times, then I did a memory test and
  // re-populated FRAM, and now it works just fine.  Ugh.  This could mean problems in FRAM not behaving properly without knowning
  // why.  However I have not been able to repeat this problem, as recently as 1/25/23, so maybe I had a memory leak somewhere?
  if (t_turnoutNum != m_turnoutReservation.turnoutNum) {
    Turnout_Reservation::getReservation(t_turnoutNum);
  }
  if ((Turnout_Reservation::reservedForTrain(t_turnoutNum) != LOCO_ID_NULL) &&
    (Turnout_Reservation::reservedForTrain(t_turnoutNum) != t_locoNum)) {
    // We'll allow us to reserve it if it's already reserved for us, but not if it's already reserved for an other loco
    sprintf(lcdString, "T %2i RES'D FOR %2i!", t_turnoutNum, Turnout_Reservation::reservedForTrain(t_turnoutNum)); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  m_turnoutReservation.reservedForTrain = t_locoNum;
  Turnout_Reservation::setReservation(t_turnoutNum);
  return;
}

void Turnout_Reservation::release(const byte t_turnoutNum) {
  // Rev: 01/25/23.
  // Retrieve the entire Turnout Reservation record (if not already loaded), update the "reservedForTrain" field, and then write
  // the record back to FRAM.
  // Releasing a turnout DOES NOT update the last-known orientation -- so we can use last-known even after released.
  // Expects t_turnoutNum to start at 1 not 0!
  // Could return bool if want to confirm it was previously reserved.
  if (t_turnoutNum != m_turnoutReservation.turnoutNum) {
    Turnout_Reservation::getReservation(t_turnoutNum);
  }
  m_turnoutReservation.reservedForTrain = LOCO_ID_NULL;
  Turnout_Reservation::setReservation(t_turnoutNum);
  return;
}

void Turnout_Reservation::releaseAll() {
  // Rev: 01/25/23.
  // Releasing a turnout DOES NOT update the last-known orientation -- so we can use last-known even after released.
  for (byte turnoutNum = 1; turnoutNum <= TOTAL_TURNOUTS; turnoutNum++) {
    Turnout_Reservation::release(turnoutNum);
  }
  return;
}

byte Turnout_Reservation::reservedForTrain(const byte t_turnoutNum) {
  // Rev: 01/25/23.
  // Returns train number this turnout is currently reserved for, incl. LOCO_ID_NULL and LOCO_ID_STATIC.  0 means not reserved.
  // Note: This function can be used as bool since "unreserved" is Train 0.
  // i.e. "if (TurnoutReservation.reservedForTrain(b))" can be interpreted as True or False if you just want to know if *any* train
  // (including STATIC) has this turnout reserved.
  // Expects t_turnoutNum to start at 1 not 0!  Returns train number starting at 1 (if a real train).
  if (t_turnoutNum != m_turnoutReservation.turnoutNum) {
    Turnout_Reservation::getReservation(t_turnoutNum);
  }
  return m_turnoutReservation.reservedForTrain;
}

void Turnout_Reservation::display(const byte t_turnoutNum) {
  // Rev: 01/25/23.
  // Display a single Turnout Reservation record; not the entire table.  For testing and debugging purposes only.
  // Could reduce the amount of memory required by using F() macro in Serial.print, but we only call this for debugging so won't
  // affect the amount of memory required in the final program.
  if (t_turnoutNum != m_turnoutReservation.turnoutNum) {
    Turnout_Reservation::getReservation(t_turnoutNum);
  }
  sprintf(lcdString, "Turnout: %2i ", t_turnoutNum); Serial.print(lcdString);
  sprintf(lcdString, "Orientation: %c ", Turnout_Reservation::getLastOrientation(t_turnoutNum)); Serial.print(lcdString);
  sprintf(lcdString, "Reserved for: %2i ", Turnout_Reservation::reservedForTrain(t_turnoutNum)); Serial.print(lcdString);
  Serial.println();
}

void Turnout_Reservation::populate() {
  // Rev: 01/25/23.
  // RARELY CALLED function that just populates a new FRAM.  No reason to use after that.
  for (byte turnoutNum = 1; turnoutNum <= TOTAL_TURNOUTS; turnoutNum++) {
    m_turnoutReservation.turnoutNum = turnoutNum;
    m_turnoutReservation.lastPosition = TURNOUT_DIR_NORMAL;  // Random but valid ('N' or 'R' would be equally valid)
    m_turnoutReservation.reservedForTrain = LOCO_ID_NULL;  // Not reserved
    Turnout_Reservation::setReservation(turnoutNum);
  }
  m_pStorage->setFRAMRevDate(01, 26, 23);  // ALWAYS UPDATE FRAM DATE IF WE CHANGE A FILE!
  return;
}

// ***** PRIVATE FUNCTIONS ***

void Turnout_Reservation::getReservation(const byte t_turnoutNum) {
  // Rev: 01/25/23.
  // Expects t_turnoutNum to start at 1 not 0!
  // FRAM read requires a "byte" pointer to the local data it's going to reading into, so I need to create that via casting.
  // Create a new byte-type pointer pTurnoutReservation and cast the address of our local struct variable m_turnoutReservation.
  byte* pTurnoutReservation = (byte*)(&m_turnoutReservation);
  m_pStorage->read(Turnout_Reservation::turnoutReservationAddress(t_turnoutNum), sizeof(turnoutReservationStruct), pTurnoutReservation);
  return;
}

void Turnout_Reservation::setReservation(const byte t_turnoutNum) {
  // Rev: 01/25/23.
  // Expects t_turnoutNum to start at 1 not 0!
  // FRAM.write needs a BYTE pointer to the Turnout Reservation structure variable, so we'll create and cast it here.
  byte* pTurnoutReservation = (byte*)(&m_turnoutReservation);
  m_pStorage->write(Turnout_Reservation::turnoutReservationAddress(t_turnoutNum), sizeof(turnoutReservationStruct), pTurnoutReservation);  // Pass "real" turnout num starts at 1
  return;
}

unsigned long Turnout_Reservation::turnoutReservationAddress(const byte t_turnoutNum) {  // Return the starting address of the *record* in the Turnout Reservation FRAM table for this turnout
  // Rev: 01/25/23.
  // Expects t_turnoutNum to start at 1 not 0!  We correct for that here.
  if ((t_turnoutNum < 1) || (t_turnoutNum > TOTAL_TURNOUTS)) {
    sprintf(lcdString, "BAD TRA TURNOUT %i", t_turnoutNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return FRAM_ADDR_TURNOUT_RESN + ((t_turnoutNum - 1) * sizeof(turnoutReservationStruct));  // NOTE: We translate turnout number 1 to record 0.
}
