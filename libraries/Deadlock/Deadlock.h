// DEADLOCK.H Rev: 01/27/23.
// Part of O_MAS.
// Determine if a proposed next-route's destination could create a Deadlock condition for a given train.
// deadlockExists() assumes that it's being passed a valid t_candidateDestination and t_locoNum; no error checking on those.
// 01/26/23: IMPORTANT NOTE REGARDING IF TRAIN IS TOO LONG, OR FORBIDDEN TYPE (i.e. Pass/Frt) FOR A GIVEN SIDING.
// A train may not be allowed to use a siding if it is either:
//   1. Too long to fit in the siding; or
//   2. Of a type that is not allowed in the siding, such as Passenger or Freight.
// You need to compare the Loco Ref's Length and Type against the Block Reservation's Length and Forbidden fields.
// Don't get confused that we have a CANDIDATE DESTINATION, which we want to know if there is a potential Deadlock, but we also
// have a list of THREAT SIDINGS which the train must also be able to potentially fit in.  The caller (assumed to be the
// Dispatcher) should be responsible for ensuring that the Candidate Destination is even an option for the given train before
// calling this class to see if a potential Deadlock exists.  HOWEVER (and he's the sticky part) because the caller doesn't
// know which Threat Blocks make up a Deadlock record for a given siding, and this class doesn't return such information to the
// caller, the caller won't be able to determine if any of the Threat sidings are inappropriate for this train.
// Thus, THIS CLASS must check if each Threat siding is of an appropriate length and allows this type of train, IN ADDITION to
// confirming that the Threat siding is unoccupied (or occupied in a direction that is not a threat.)
// So even though it feels sloppy, we'll be sure a threat is appropriate for the length and type of this train before deciding
// that the threat siding provides a possible escape from Deadlock.  Ugh.
// 01/09/23: Eliminated levels logic.  For single-level operation, just flag upper-level blocks as occupied STATIC.
// 11/16/20: Finished and tested, but then re-wrote in Jan 2023 so need to test again.
// Given a "Candidate Destination" i.e. BE04, check to see if *all* blocks in the "threat list" are either:
//   1. Reserved (for other trains not including ours, or for STATIC equipment), in corresponding direction if applicable; or
//   2. Ineligible as a destination for this train due to siding length or type.
//      For this, we will need to examine both Loco Ref and Block Res'n.
// For single-level operation, MAS must have already flagged all upper-level sidings as Reserved for STATIC.
// We will probably want to tweak some of the deadlocks once we start running on both levels, such as BW04.

#ifndef DEADLOCK_H
#define DEADLOCK_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <FRAM.h>
#include <Block_Reservation.h>   // Needed to look up reservations, block length, and block restrictions.
#include <Loco_Reference.h>      // Needed to look up train length and type.

class Deadlock_Reference {

  public:

    Deadlock_Reference();  // Constructor must be called above setup() so the object will be global to the module.
    void begin(FRAM* t_pStorage, Block_Reservation* t_pBlockReservation, Loco_Reference* t_pLoco);
    bool deadlockExists(const routeElement t_candidateDestination, const byte t_locoNum);  // True if deadlock found, else false.

    void display();  // Display the entire table to Serial COM.
    void populate();  // Special utility reads hard-coded data, writes records to FRAM.

  private:

    void getDeadlockRecord(const byte t_deadlockNum);    // Simply retrieve a Deadlock table record 1..n into m_deadlock
    unsigned long deadlockAddress(const byte t_recNum);  // Return FRAM address of Deadlock table record t_recNum

    // DEADLOCK REFERENCE TABLE.  This struct is only needed inside this class.
    struct deadlockStruct {
      byte         deadlockNum;  // Record number 1..FRAM_RECS_DEADLOCK for in-class housekeeping reference.
      routeElement destination;  // i.e. BE04
      routeElement threatList[FRAM_FIELDS_DEADLOCK];  // i.e. BE02, BE03, ER00...
    };
    deadlockStruct m_deadlock;  // Could save 25 bytes (less 2 bytes for pointer) if I make this a pointer to heap

    FRAM* m_pStorage;           // Pointer to the FRAM memory module
    Loco_Reference* m_pLoco;
    Block_Reservation* m_pBlockReservation;

};

#endif
