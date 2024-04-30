// DEADLOCK.CPP Rev: 02/27/23.
// Part of O_MAS.
// Determine if a proposed next-route's destination could create a Deadlock condition for a given train.
// 01/14/23: Although the list of threat routes can include BE and BW block types, we're going to be more conservative and list
//           only threats that one must be completely empty; i.e. all threats are type BXnn now.
// 01/12/23: For simplicity, we are going to assume that we have EXACTLY ONE Deadlock record for every siding/direction.  If there
//           are no threats (such as for sidings 23-26 in each direction) then simply populate the threat list with all "ER00."
//           There must NOT be a siding/direction without a Deadlock record, and there must NOT be more than one Deadlock record
//           per siding/direction.
//           Also we aren't checking if t_candidateDestination or t_locoNum are valid values; they'd better be or we'll crash.
// 12/27/22: We don't care about levels anymore -- for single-level operation, we'll just reserve blocks 19-20 and 23-26 as
//           Reserved for Static equipment.  Static needs to be treated as facing both BE and BW so we'll use BX.
// 12/27/22: Besides our second level, there could be other track plans where a threat would exist whether equipment is facing east
//           or west.  So the Deadlock table will indicate "BX" for threats when we want to indicate block occupancy for either
//           direction.  IMPORTANT: There will never be a Route block BX, or Deadlock Dest block BX; only Threat blocks can use BX.
//           Trivial feature but saves us from having to check i.e. BE03 *and* BW03, esp. since not relevant for static equipment.

#include <Deadlock.h>

Deadlock_Reference::Deadlock_Reference() {  // Constructor
  // Rev: 01/26/23.
  // Object holds ONE Deadlock record (not the whole table)
  // Just initialize our internal turnout-reservation structure element...
  // m_deadlock is our local working variable of type deadlockStruct, so it holds one Deadlock table entry (22 bytes.)
  m_deadlock.deadlockNum = 0;  // 0 is never a valid value for deadlock (record) number, which begins with zero.
  m_deadlock.destination.routeRecType = ER;  // End-Of-Record is *not* a real destination so this will never be legal.
  m_deadlock.destination.routeRecVal = 0;
  for (byte threatNum = 0; threatNum < FRAM_FIELDS_DEADLOCK; threatNum++) {  // 11 fields in the threat list as of 1/15/23.
    m_deadlock.threatList[threatNum].routeRecType = ER;  // end-of-record
    m_deadlock.threatList[threatNum].routeRecVal = 0;
  }
  return;
}

void Deadlock_Reference::begin(FRAM* t_pStorage, Block_Reservation* t_pBlockReservation, Loco_Reference* t_pLoco) {
  // Rev: 01/26/23.
  m_pStorage = t_pStorage;                    // Pointer to FRAM
  m_pBlockReservation = t_pBlockReservation;  // Pointer to Block Reservation table
  m_pLoco = t_pLoco;                          // Pointer to Loco Ref table
  if ((m_pStorage == nullptr) || (m_pBlockReservation == nullptr) || (m_pLoco == nullptr)) {
    sprintf(lcdString, "UN-INIT'd DL PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;
}

bool Deadlock_Reference::deadlockExists(const routeElement t_candidateDestination, const byte t_locoNum) {
  // Rev: 02/09/23.
  // Returns true if t_candidateDestination would create a deadlock scenario if we moved t_locoNum into it; else false.
  // t_locoNum is expected to be 1..50 (not 0..49) but there is no data validation on it.
  // t_candidateDestination is expected to be a valid route element BE/BW (not BX) + 01..26, but there is no data validation on it.
  // AFTER Dispatcher finds a candidate Route that it likes (candidate destination available, all turnouts on the route available,
  // long enough to hold this train, and of a type that doesn't restrict this train's type) it must call this function to see if
  // the proposed route's destination block and direction might create a deadlock with other trains.  This basically boils down to
  // ensuring that this loco will have a subsequent next station (following our candidate destination station) that is also
  // currently available and appropriate length and type to use when the time comes.
  // If *any* block in the threat list for the Deadlock record matching t_candidateDestination is *not* reserved in the
  //   direction/level we fear, and if the train is allowed to enter that block based on it's type and length, then there will be
  //   an escape route for this train, once it reaches this destination, and we will consider the destination to not create a
  //   deadlock; it may safely be used as this train's next destination and thus there is no need to check any further and we'll
  //   return false.
  // Only if *every* siding on the threat list is either occupied (potentially in the direction we don't like) or too short for our
  //   train, or of a type that prohibits trains of our type (i.e. pass or frt) will we return "true," which means that the
  //   candidate destination should NOT be selected by the caller (i.e. Dispatcher) as the next destination siding for this train.
  // IMPORTANT: When checking for block occupancy in the threat list, we must also consider if the "threat" siding is too short for
  //   t_locoNum, and also check if this loco type is forbidden in that siding (i.e. freight loco no allowed in passenger siding.)
  //   In either case, we'll consider that threat siding to be effectively occupied (unavailable) even if there was no train in it.
  // IMPORTANT: If we find a block in a Deadlock record's threat list that is reserved for t_locoNum (i.e. us,) we must disregard
  //   and consider that block to be unreserved.  This is because we may possibly be sitting in that block, but will clear it if we
  //   proceed to t_candidateDestination.  For example, if we are in BE03 (or enroute to it, either way it will be Reserved for us)
  //   and Dispatcher is looking at BE04 as a possible next destination (by backing into BX10 and then forward into BE04,) BE03 is
  //   in the BE04 deadlock conflict list!  But we will move out of BE03 (and, if we are moving, any other blocks that were still
  //   reserved until we reach BE03) so it will not be a conflict in this case.
  // NOTE: For "Reserved Static" blocks, such as will be used for 2nd level sidings before it is constructed, Registration will
  //   reserve them in either direction (doesn't matter) for loco 99 (STATIC.)  So this code needs to check two cases:
  //   1. If a block is reserved for a real train 1..50, then look at direction i.e. BE14 (though as of 1/26/23, every threat
  //      entry is for direction BX which means that we don't want a train facing EITHER direction in that siding.)
  //   2. If a block is reserved for STATIC train 99, then consider it occupied in both directions i.e. BE and BW.
  // IMPORTANT NOTES REGARDING HOW TO AVOID A DEADLOCK, and why Threat directions are all BX, and not BE or BW:
  // Until Jan 2023, I thought we would flag a potential deadlock only if all subsequent sidings were occupied *and* those trains
  // were either STATIC or were facing our train.  I.e. even if a subsequent siding was occupied, if the train was facing away from
  // us, it would be able to escape somehow, and THEN we could occupy that siding.  However, this could still result in deadlocks
  // in even simple scenarios such as if all first-level sidings are occupied except BE02.  In this example, suppose other trains
  // are in BE01 and BW13, and we are considering dropping down from the second level into BE02.  That would seem not to be a
  // potential deadlock since BW13 was facing away from us -- but that train could only enter BE01 or BE02, both of which would be
  // occupied if we were to use BE02 for our loco.  In that scenario, we could only be certain of not risking a deadlock if both
  // our proposed destination (BE02) *and* at least one subsequent destination (BX13) was totally unoccupied (i.e. couldn't even be
  // facing away from us.)  So I'm changing the Deadlock table to reflect this new insight.
  // FUTURE: At some point we may need more than one Deadlock record per destination block/direction, but not as of Jan 2023.
  //         So to keep our code as simple as possible, we only allow for EXACTLY ONE Deadlock record per siding/direction.
  //         This also means that there must not be zero Deadlock records for any siding.
  // FUTURE: May want to confirm that t_candidateDestination is a valid routeElement record i.e. BE/BW (not BX), 01..nn.  We
  //         should really call Route_Reference::outOfRangeRouteReference(t_candidateDestination) but we'd need a pointer to the
  //         Route Reference table to do that, which we don't currently have as of 1/11/23.  And Route Reference doesn't have that
  //         function yet.  So we'll rely on the caller to ensure it passes valid parameters.

  // Feel free to delete this debug output to the Serial monitor...
  Serial.print(F("DEADLOCK CHECK for loco ")); Serial.print(t_locoNum); Serial.print(F(", candidate destination "));
  if (t_candidateDestination.routeRecType == BE) {
    Serial.print(F("BE"));
  }
  else if (t_candidateDestination.routeRecType == BW) {
    Serial.print(F("BW"));
  }
  else {
    sprintf(lcdString, "BAD DDLK DIR %i", t_candidateDestination.routeRecType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  Serial.println(t_candidateDestination.routeRecVal);
  Serial.print(F("Train length: ")); Serial.print(m_pLoco->length(t_locoNum)); Serial.print(F(", Train Type: ")); Serial.println(m_pLoco->passOrFreight(t_locoNum));

  // Scan every Deadlock record sequentially until we find the one-and-only with a matching t_candidateDestination i.e. BW04.
  // Deadlock table records don't need to be in any particular order; we'll find ours by scanning sequentially until found.
  byte deadlockRecord = 0;  // Records start at 1 but this counter will be incremented before it's used.
  do {
    deadlockRecord++;
    if (deadlockRecord > FRAM_RECS_DEADLOCK) {  // Whoops!  Ran thru all records without finding our match - should never happen!
      sprintf(lcdString, "BAD DEADLOCK REC %i", deadlockRecord); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
    }
    // Populate our local m_deadlock struct with the contents of Deadlock rec num deadlockRecord (1..FRAM_RECS_DEADLOCK.)
    Deadlock_Reference::getDeadlockRecord(deadlockRecord);  // Retrieve contents of Deadlock record into m_deadlock local struct
  } while ((m_deadlock.destination.routeRecType != t_candidateDestination.routeRecType) ||  // Block direction (BX is n/a here.)
           (m_deadlock.destination.routeRecVal  != t_candidateDestination.routeRecVal));   // Block number 1..26
  // When we get here, we have successfully loaded the one-and-only Deadlock record that matches our candidate destination!  Yay!

  // Special condition checks if there are NO deadlock threat records for t_candidateDestination (such as with blocks 23-26).
  if (m_deadlock.threatList[0].routeRecType == ER) {  // No possible threats if the first one is ER.
    Serial.println(F("No threats exist for this siding; nothing to do."));
    return false;  // Occupying t_candidateDestination won't create a deadlock; good to go!
  }

  // If we get here, we have found our one-and-only Deadlock record and we're confident there is AT LEAST ONE "threat" field.
  // Let's examine each "threat" field in this record and see if we can find JUST ONE that our train could enter.  That's all we
  // need to find in order to consider this destination as not-a-deadlock-risk.
  // 1. Is this threat siding reserved for a train OTHER THAN OURSELF facing either towards us, or either way if BX, OR
  // 2. Is t_locoNum too long to fit into this threat siding, OR
  // 3. Is t_locoNum of a type (i.e. freight) that is not allowed to enter this threat siding?
  // Any one of the above conditions indicates that the threat siding is not appropriate; see if there might be another one.
  // Or perhaps it's easier to look at this in reverse, to decide if we have a threat siding that's okay for us to use:
  // 1. Is this threat siding either un-reserved, or reserved for at train other than t_locoNum that's not facing in an
  //    objectional direction; AND
  // 2. Is t_locoNum short enough to fit into this threat siding; AND
  // 3. is t_locoNum of a type (i.e. freight) that IS ALLOWED to use this threat siding?
  // If a threat siding satisfies all three of the above criteria, then we can use it and thus exit this function, returning false!
  // NOTE: Don't get confused that we're not considering if t_candidateDestination itself might not be long enough or of an
  // appropriate type for this loco; we only check the threat blocks.  It's up to the caller (presumably, Dispatcher) to have
  // already confirmed that the candidate destination is even an option for this train.
  // If *any* non-ER00 threat element allows us to enter, then we can exit this function with deadlockExists == false.

  // So, for each possible threat block for this Deadlock record...
  for (byte threatNum = 0; threatNum < FRAM_FIELDS_DEADLOCK; threatNum++) {
    // We are guaranteed to always enter this loop with at least one threat to check, because we've already accounted for the case
    // where the first threat for a given Deadlock record was ER00, above.
    // If this is an ER-type record, we've reached the end of the actual threats without finding an exit; thus it's a deadlock.
    if (m_deadlock.threatList[threatNum].routeRecType == ER) {
      Serial.println(F("No more threats to check; it's a deadlock."));
      return true;  // It's a deadlock
    }

    // Okay, we have a fresh threat to check.  Is it available as a future destination to avoid deadlock?
    const byte threatBlockNum = m_deadlock.threatList[threatNum].routeRecVal;
    const byte threatBlockDir = m_deadlock.threatList[threatNum].routeRecType;
    Serial.print(F("Checking threat #")); Serial.print(threatNum); Serial.print(F(", block "));
    if (threatBlockDir == BE) {
      Serial.print(F("BE"));
    } else if (threatBlockDir == BW) {
      Serial.print(F("BW"));
    } else if (threatBlockDir == BX) {
      Serial.print(F("BX"));
    } else {
      sprintf(lcdString, "BAD DDLK TN"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
    }
    Serial.println(threatBlockNum);

    // Let's first see if this threat siding will even allow our length and type of train, before we see if it's occupied...
    if (m_pLoco->length(t_locoNum) > m_pBlockReservation->length(threatBlockNum)) {  // Train too long; forget this threat block as a possibility
      Serial.println(F("Loco is too long for this block.  We'll keep trying if we can..."));
      continue;  // Go back up to the top of this for-loop and try the next threat
    } else {
      Serial.print("Loco fits. ");
    }
    // Okay, we know that our train is long enough to fit in the threat siding.  How about restrictions?  We have:
    //   m_pLoco->passOrFreight(t_locoNum) = P/p/F/f/M (M.O.W.)
    //   m_pBlockReservation->forbidden(t_blockNum) = P/F/L/T = Pass, Freight, Local, Through
    // Rev: 02/09/23 Tested all combinations of Loco type P/p/F/f vs. Forbidden Pass/Freight/Through/Local all work AOK.
    if ((((m_pLoco->passOrFreight(t_locoNum) == 'P') || (m_pLoco->passOrFreight(t_locoNum) == 'F')) && (m_pBlockReservation->forbidden(threatBlockNum) == FORBIDDEN_THROUGH)) ||
        (((m_pLoco->passOrFreight(t_locoNum) == 'p') || (m_pLoco->passOrFreight(t_locoNum) == 'f')) && (m_pBlockReservation->forbidden(threatBlockNum) == FORBIDDEN_LOCAL)) ||
        (((m_pLoco->passOrFreight(t_locoNum) == 'p') || (m_pLoco->passOrFreight(t_locoNum) == 'P')) && (m_pBlockReservation->forbidden(threatBlockNum) == FORBIDDEN_PASSENGER)) ||
        (((m_pLoco->passOrFreight(t_locoNum) == 'f') || (m_pLoco->passOrFreight(t_locoNum) == 'F')) && (m_pBlockReservation->forbidden(threatBlockNum) == FORBIDDEN_FREIGHT))) {
      Serial.print(F("Loco type ")); Serial.print(m_pLoco->passOrFreight(t_locoNum)); Serial.println(F(" forbidden in Block.  We'll keep trying if we can..."));
      continue;  // Go back up to the top of this for-loop and try the next threat
    } else {
      Serial.print("Loco type P/p/F/f is okay. ");
    }

    // Well, our train is long enough and not forbidden to enter the threat siding.  Is it reserved for some other train though?
    const byte blockReservedForLocoNum = m_pBlockReservation->reservedForTrain(m_deadlock.threatList[threatNum].routeRecVal);
    const byte blockReservedForDirection = m_pBlockReservation->reservedDirection(m_deadlock.threatList[threatNum].routeRecType);
    // Note that blockReservedForDirection must be BE or BW; BX is only used in the Deadlock table.
    // HOWEVER, if blockReservedForLocoNum = LOCO_ID_STATIC, then we must consider the loco to be reserved for BOTH directions.
    // OTOH, if the block is reserved for LOCO_ID_NULL *or* ourself (either direction), then we are okay to use it!
    // So, is the block either unreserved, or reserved for us?
    if (blockReservedForLocoNum == LOCO_ID_NULL) {  // Great!  It's available; not reserved for anyone.
      Serial.println(F("Threat block is not reserved, so okay to occupy; no deadlock!"));
      return false;  // It's not a deadlock
    } else if (blockReservedForLocoNum == t_locoNum) {  // Great!  It's already reserved for us!
      Serial.println(F("Threat block is already reserved FOR US! Okay to occupy; no deadlock!"));
      return false;  // It's not a deadlock
    }
    // Okay we can fit in the siding, and are allowed, BUT it is FOR SURE ALREADY RESERVED for some OTHER train (or STATIC.)
    // Our last hope for this threat is if it specifies a direction, and the block is reserved in another direction.
    // Note that the threat could be BX (either direction.)  But if the threat is BE and the block is reserved for BW, or vice-
    // versa, we're good to go.  Blocks must ALWAYS be reserved either BE or BW (never BX or ER) even if not reserved.
    Serial.print("Threat block is reserved for loco "); Serial.print(blockReservedForLocoNum); Serial.print(" facing ");
    if (blockReservedForDirection == BE) {
      Serial.println(" East.");
    } else if (blockReservedForDirection == BW) {
      Serial.println(" West.");
    } else {
      sprintf(lcdString, "BAD DIR %i", blockReservedForDirection); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
    }
    if (threatBlockDir == BX) {
      // Threat says if there is a train reserved in this block (which we now know there is) for either direction, it's no good.
      Serial.print(F("Threat block ")); Serial.print(threatBlockNum);
      Serial.println(F(" is type BX so no-go.  We'll keep trying if we can..."));
      continue;  // Go back up to the top of this for-loop and try the next threat
    }
    if (blockReservedForLocoNum == LOCO_ID_STATIC) {
      // Static trains can't exit from either side, so this threat siding won't work for us.
      Serial.print(F("Threat block ")); Serial.print(threatBlockNum);
      Serial.println(F(" is STATIC so no-go.  We'll keep trying if we can..."));
      continue;  // Go back up to the top of this for-loop and try the next threat
    }
    // Well, at this point threatBlockDir indicates a direction (BE or BW, not BX) and the block is reserved in a particular
    // direction.  If they match, we're screwed; if not, we're good.
    if (threatBlockDir == blockReservedForDirection) {
      Serial.print(F("Threat block ")); Serial.print(threatBlockNum);
      Serial.println(F(" direction matches block reservation, so no go.  We'll keep trying if we can..."));
      continue;  // Go back up to the top of this for-loop and try the next threat
    }
    // Well heck, we've passed all restrictions for this threat block.  Therefore, it's available to our loco as a subsequent
    // destination following t_candidateDestination and thus entering t_candidateDestination won't create a deadlock!
    Serial.print(F("Threat block ")); Serial.print(threatBlockNum);
    Serial.println(F(" is reserved but in the opposite direction of the threat, so okay to occupy; no deadlock!"));
    return false;  // It's not a deadlock
  }
  // We should NEVER fall thorough the above loop to here; it means we overlooked a possible scenario.
  sprintf(lcdString, "DEADLOCKEXISTS BUG!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  return true;  // <- just added return to stop the compiler error; I can't ever fall through to here.
}

void Deadlock_Reference::display() {  // Test code to dump the contents of the Deadlock table
  // Rev: 01/26/23.
  // Display the entire Deadlock table.  For testing and debugging purposes only.
  // Could reduce the amount of memory required by using F() macro in Serial.print, but we only call this for debugging so won't
  // affect the amount of memory required in the final program.
  // Expects t_deadlockNum to start at 1 not 0!
  for (byte deadlockNum = 1; deadlockNum <= FRAM_RECS_DEADLOCK; deadlockNum++) {
    Deadlock_Reference::getDeadlockRecord(deadlockNum);  // Put Deadlock record into m_deadlock
    sprintf(lcdString, "Num: %2i  ", m_deadlock.deadlockNum); Serial.print(lcdString);
    Serial.print(F("Dest: "));
    if (m_deadlock.destination.routeRecType == BE) {
      Serial.print(F("BE"));
    }
    else if ((m_deadlock.destination.routeRecType) == BW) {
      Serial.print(F("BW"));
    }
    else {  // Note: BX is not applicable for Destination Block field
      Serial.print(F("??"));
    }
    sprintf(lcdString,"%2i", m_deadlock.destination.routeRecVal); Serial.print(lcdString);
    for (byte threatNum = 0; threatNum < FRAM_FIELDS_DEADLOCK; threatNum++) {  // Zero offset (Sub-field, not deadlock record number)
      if ((m_deadlock.threatList[threatNum].routeRecType) == ER) {
        break; //Serial.print(F("ER"));
      }
      sprintf(lcdString,"  T%i: ", threatNum + 1); Serial.print(lcdString); 
      if ((m_deadlock.threatList[threatNum].routeRecType) == BE) {
        Serial.print(F("BE"));
      }
      else if ((m_deadlock.threatList[threatNum].routeRecType) == BW) {
        Serial.print(F("BW"));
      }
      else if ((m_deadlock.threatList[threatNum].routeRecType) == BX) {
        Serial.print(F("BX"));
      }
      else {
        Serial.print(F("??"));
      }
      sprintf(lcdString, "%2i  ", m_deadlock.threatList[threatNum].routeRecVal); Serial.print(lcdString);
    }
    Serial.println();
  }
  return;
}

void Deadlock_Reference::populate() {
  // Rev: 01/26/23.
  // Populate the Deadlock table with known constants.
  // IMPORTANT: This initial Deadlock table is set up for SINGLE-LEVEL OPERATION ONLY.  We will need to make changes to this table
  //            once we start operating on both levels.
  // Deadlock Number is 1..FRAM_RECS_DEADLOCK.  FRAM record number is 0..(FRAM_RECS_DEADLOCK - 1), accounted for here.
  // We ONLY need to call populate() from a utility program, whenever we need to refresh the FRAM Deadlock table, such as if we
  // discover a problem with an existing Deadlock list, or when we start operating on the second level (i.e. see BW04.)
  // 01/14/23: Realized our logic of "not a threat if facing away from us" would not work in some scenarios.  Modified threats to
  // be more conservative: We must have at least one *vacant* subsequent destination upon arriving at our candidate destination.
  // Careful don't call this m_deadlock, which would confuse with our "regular" variable that holds just one record.
  // 12/22/22: Eliminated levels; MAS Registration will simply tag those blocks as Reserved for STATIC equipment.  However, there
  // will still be some changes to the Deadlock table depending on if we are using one versus both levels (details below.)

  deadlockStruct* deadlock = new deadlockStruct[FRAM_RECS_DEADLOCK];

  //  Rec_Num    DDLK   Dest     Threat_1 Threat_2 Threat_3 Threat_4 Threat_5 Threat_6 Threat_7 Threat_8 Threat_9 Threat_10 Threat_11

  // BE01: Be sure we have low-priority escape Routes to BE02 and BE03 in order for this to work:
  deadlock[ 0] = {  1,  BE,01,      BX,02,   BX,03,   BX,13,   BX,14,   BX,15,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BE02: Be sure we have low-priority escape Routes to BE01, BE03 and BW04 in order for this to work:
  deadlock[ 1] = {  2,  BE,02,      BX,01,   BX,03,   BX,04,   BX,05,   BX,06,   BX,13,   BX,14,   BX,15,   ER,00,   ER,00,   ER,00 };
  // BE03: Be sure we have low-priority escape Routes to BE01, BE02 and BW04 in order for this to work:
  deadlock[ 2] = {  3,  BE,03,      BX,01,   BX,02,   BX,04,   BX,05,   BX,06,   BX,13,   BX,14,   BX,15,   ER,00,   ER,00,   ER,00 };
  // BE04: In a pinch we could add escape routes to blocks 5 and 6, but for now we won't.
  deadlock[ 3] = {  4,  BE,04,      BX,02,   BX,03,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BE05, BE06: It really only makes sense to proceed into block 2 or 3.
  deadlock[ 4] = {  5,  BE,05,      BX,02,   BX,03,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[ 5] = {  6,  BE,06,      BX,02,   BX,03,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BE13, BE14, BE15: Really only makes sense to enter blocks 1, 2, or 3.
  deadlock[ 6] = {  7,  BE,13,      BX,01,   BX,02,   BX,03,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[ 7] = {  8,  BE,14,      BX,01,   BX,02,   BX,03,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[ 8] = {  9,  BE,15,      BX,01,   BX,02,   BX,03,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BE19, BE20: Ahead into BE03 or BE04, and be sure to add reversing routes to back into BE23-BE26:
  deadlock[ 9] = { 10,  BE,19,      BX,03,   BX,04,   BX,23,   BX,24,   BX,25,   BX,26,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[10] = { 11,  BE,20,      BX,03,   BX,04,   BX,23,   BX,24,   BX,25,   BX,26,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BE23-BE26 single-ended sidings can never trigger a deadlock because they don't block any other train's egress.  Thus, they
  // don't need to have potential next destinations in order to get out of someone else's way.
  deadlock[11] = { 12,  BE,23,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[12] = { 13,  BE,24,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[13] = { 14,  BE,25,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[14] = { 15,  BE,26,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BW01: Should we have low-priority escape Routes to BW02 and BW03?  Frankly I can't think of a scenario where moving from
  // BW01 to BW02 or BW03 could get out of anyone's way and solve a deadlock, but I created those routes so what the heck.
  deadlock[15] = { 16,  BW,01,      BX,02,   BX,03,   BX,13,   BX,14,   BX,15,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BW02: Should we have low-priority escape Routes to BW01 and BW03?  Again, I can't think of a scenario where moving from
  // BW02 to BW01 or BW03 could get out of anyone's way and solve a deadlock, but I created those routes anyway.
  // Could also add escape routes backing into BE04, BE05, and BE06 but the only subsequent destinations would be right back into
  //  BW02 or BW03.  Plus, there are already tons of escape routes for BW02:
  deadlock[16] = { 17,  BW,02,      BX,01,   BX,03,   BX,13,   BX,14,   BX,15,   BX,19,   BX,20,   BX,23,   BX,24,   BX,25,   BX,26 };
  // BW03: Tons of options with two-level operation, but for single level we need to have low-priority escape routes to BW01 and
  // BW02.  Could also add escape routes backing into BE04, BE05, and BE06 but would then only put us back where we started.
  // Not much point in adding BE04/BE05/BE06 escape Routes unless I include them here in Deadlocks, since not including them here
  // makes it impossible that a train won't have a better option: BX01/BX02/BX19-20/BX23-26 as included here:
  deadlock[17] = { 18,  BW,03,      BX,01,   BX,02,   BX,19,   BX,20,   BX,23,   BX,24,   BX,25,   BX,26,   ER,00,   ER,00,   ER,00 };
  // BW04: We can enter BW04 even if the entire second level is reserved, by providing escape routes to BE02, BE03, and BW03.
  // Not much point in reversing back into BE02/BE03 because that's where we would have come from, but we could shuffle into
  // BW03 (if the entire second level was full or absent.)  Of course, reversing back into BE02/BE03 would allow us to continue
  // towards either BW13/BW14/BW15 or BW05/BW06.  Not sure why we wouldn't have just done that in the first place, though.
  // BW04 should be a very low priority in the Route Reference table when in single level operation, but a high priority when in
  // two-level operation (since it will send us back up to the second level.)
  // For single-level operation, we're not going to completely stop trains from entering BW04 because it's interesting.
  // Here is a more appropriate BW04 deadlock array, for use with two-level operation:
  // { 19,  BW,04,      BX,19,   BX,20,   BX,23,   BX,24,   BX,25,   BX,26,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00},
  // And here is a version for the BW04 deadlock array that allows extra emergency exits, for use with single-level operation:
  deadlock[18] = { 19,  BW,04,      BX,02,   BX,03,   BX,19,   BX,20,   BX,23,   BX,24,   BX,25,   BX,26,   ER,00,   ER,00,   ER,00 };
  // BW05 and BW06: Could potentially have emergency exit routes by backing into BE02/BE03, but very low priority (especially
  // since that's where we must have just come from!)  But since there is only one option when going forward (BE14) if that's
  // occupied we should have an emergency egress (or two.)
  deadlock[19] = { 20,  BW,05,      BX,02,   BX,03,   BX,14,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[20] = { 21,  BW,06,      BX,02,   BX,03,   BX,14,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BW13: There are really no good options other than straight on into either BE01 or BE02.  As a very very very last resort, we
  // could back down into BW21 and then up into BW14, but I'm not going to consider that yet as it's a terrible route.
  deadlock[21] = { 22,  BW,13,      BX,01,   BX,02,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BW14: Pretty straightforward -- ahead into blocks 1, 2, 5, or 6.
  deadlock[22] = { 23,  BW,14,      BX,01,   BX,02,   BX,05,   BX,06,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BW15: Only good options are BE01 or BE02; but adding drop into BW14 as last-ditch deadlock escape (then into BE11.)
  deadlock[23] = { 24,  BW,15,      BX,01,   BX,02,   BX,14,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BW19, BW20: Nothing complicated here.
  deadlock[24] = { 25,  BW,19,      BX,02,   BX,03,   BX,04,   BX,23,   BX,24,   BX,25,   BX,26,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[25] = { 26,  BW,20,      BX,02,   BX,03,   BX,04,   BX,23,   BX,24,   BX,25,   BX,26,   ER,00,   ER,00,   ER,00,   ER,00 };
  // BW23-BW26 single-ended sidings can never trigger a deadlock because they don't block any other train's egress.
  deadlock[26] = { 27,  BW,23,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[27] = { 28,  BW,24,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[28] = { 29,  BW,25,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };
  deadlock[29] = { 30,  BW,26,      ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00,   ER,00 };

  byte FRAMDataBuf[sizeof(deadlockStruct)];
  for (byte recNum = 1; recNum <= FRAM_RECS_DEADLOCK; recNum++) {
    Serial.print(F("Writing record ")); Serial.print(recNum); Serial.print(F(" to address ")); Serial.println(Deadlock_Reference::deadlockAddress(recNum));
    memcpy(FRAMDataBuf, &deadlock[recNum - 1], sizeof(deadlockStruct));
    m_pStorage->write(Deadlock_Reference::deadlockAddress(recNum), sizeof(deadlockStruct), FRAMDataBuf);
  }
  delete[] deadlock;
  m_pStorage->setFRAMRevDate(01, 26, 23);  // ALWAYS UPDATE FRAM DATE IF WE CHANGE A FILE!
  return;
}

// ***** PRIVATE FUNCTIONS ***

void Deadlock_Reference::getDeadlockRecord(const byte t_deadlockNum) {  // Retrieve a 22-byte Deadlock table record from FRAM
  // Rev: 01/26/23.
  // Retrieves a Deadlock record and puts the contents into our locally-global deadlock element m_deadlock
  // Expects t_deadlockNum to start at 1 not 0!  So 1..FRAM_RECS_DEADLOCK.
  // If t_deadlockNum is out of range, it will be fatal error in deadlockAddress() (no need to check here.)
  // Rather than writing into FRAMDataBuf and then copying to the structure, I'm writing directly into the local struct variable.
  // However, FRAM.read requires a "byte" pointer to the local data it's going to be writing into, so I need to create that variable.
  // Shown below are two possible ways to cast a pointer to the structure so FRAM.read can work with it.  Both confirmed work.
  // byte* pDeadlock = (byte*)&m_deadlock;  // FRAM "read" expects pointer to byte, not pointer to structure element, so need to cast
  // deadlockStruct* pDeadlockElement = &m_deadlock;  // FRAM.read needs a pointer to the structure variable.
  // byte* pDeadlock = reinterpret_cast<byte*>(pDeadlockElement);  // But FRAM.read expects pointer to byte, so need to cast.
  byte* pDeadlock = (byte*)(&m_deadlock);
  m_pStorage->read(Deadlock_Reference::deadlockAddress(t_deadlockNum), sizeof(deadlockStruct), pDeadlock);
  return;
}

unsigned long Deadlock_Reference::deadlockAddress(const byte t_deadlockNum) {  // Return the FRAM address of the *record* in the Deadlock table for the 'n'th record
  // Rev: 01/09/23.
  // Expects t_deadlockNum to start at 1!  We account for that here.
  if ((t_deadlockNum < 1) || (t_deadlockNum > FRAM_RECS_DEADLOCK)) {
    sprintf(lcdString, "BAD DEADLOCK ADR %i", t_deadlockNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return FRAM_ADDR_DEADLOCK + ((t_deadlockNum - 1) * sizeof(deadlockStruct));
}
