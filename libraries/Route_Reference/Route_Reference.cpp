// ROUTE_REFERENCE.CPP Rev: 03/01/23.  TESTED AND WORKING.
// 03/01/23: Removed levels field.
// 01/24/23: Constructor needs to set initial value of index field (recNum) to a NON-ZERO, impossible value.  If we initialized it
//           to 0 and our first lookup was for record 0, our code would think the real record had already been loaded and return
//           whatever starting values our constructor assigned, rather than the actual contents of record 0!
// 01/23/23: Using all stack variables, rather than heap variables, since the object will be on the heap based on the fact that a
//           pointer to the object gets declared above setup(), and an object is created on the heap in setup().  This implies that
//           ALL DATA LOCAL TO THIS CLASS OBJECT will reside on the heap.  No need to use pointers on local class data.
// 12/21/22: Using FRAM_ADDR_ROUTE_REF as starting address for EB routes, then calculating starting address for WB routes using:
//           FRAM_ADDR_ROUTE_REF + (FRAM_RECS_ROUTE_EAST * sizeof(routeReferenceStruct))
// 12/20/22: Using actual FRAM record number starting at zero for Rec Num; vs Route ID which is only used for spreadsheet sorting.
// 11/30/22: Changed char Auto/Park A|P to bool Park = true|false (since anything that isn't Park is Auto.)

// FRAM Record Numbers always start at zero
// Route IDs are always 1 or greater and are only used to reference back to original spreadsheet row.
// First EASTBOUND record FRAM address is FRAM_ADDR_ROUTE_REF
// First WESTBOUND record FRAM address is FRAM_ADDR_ROUTE_REF + (FRAM_RECS_ROUTE_EAST * sizeof(routeReferenceStruct))

#include "Route_Reference.h"

Route_Reference::Route_Reference() {  // Constructor
  // Rev: 03/01/23.
  // Object holds ONE Route Reference record (not the whole table) (though it includes all elements in the Route.)
  // Just initialize our internal structure element...
  m_routeReference.recNum                   = 999;  // Physical FRAM record number will always be 0 or higher.
  // WE MUST ASSIGN AN "IMPOSSIBLE" INITIAL VALUE TO THE INDEX FIELD, and 0 is a valid record number so don't init to that!
  // If we assign the inital value of recNum = 0, and if our first "get" happens to be for record 0, our code below will think
  // that record 0 is already in memory and return the following values, rather than what's actually at record zero!
  m_routeReference.routeID                  = 0;  // Spreadsheet reference identifier will always be greater than zero.
  m_routeReference.origin.routeRecType      = ER;
  m_routeReference.origin.routeRecVal       = 0;
  m_routeReference.destination.routeRecType = ER;
  m_routeReference.destination.routeRecVal  = 0;
  m_routeReference.park                     = false;
  m_routeReference.priority                 = 0;
  for (int routeSegment = 0; routeSegment < FRAM_SEGMENTS_ROUTE_REF; routeSegment++) {
    m_routeReference.route[routeSegment].routeRecType = ER;
    m_routeReference.route[routeSegment].routeRecVal = 0;
  }
  return;
}

void Route_Reference::begin(FRAM* t_pStorage) {
  // Rev: 12/22/22.
  m_pStorage = t_pStorage;  // Pointer to FRAM so we can access our table.
  if (m_pStorage == nullptr) {
    sprintf(lcdString, "UN-INIT'd RR PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;
}

// Calling module MUST keep track of FRAM Record Number (vs. Route ID) since records are ordered that way, thus we can keep getting
// the "next" physical record until we no longer have a match for origin.
// We could just say "get next" and let this class decide based on what is currently loaded, but if we make another call to this
// class that loads some other Route record, we'll have lost our position.  So caller better keep track of FRAM Record Number.

// For functions that retrieve fields from a Route Reference record, we'll see if the data already in the m_routeReference buffer
// matches the Route's Record Number the module is asking about by comparing t_recNum.  If so, no need to do another FRAM get;
// otherwise, we'll need to do a FRAM lookup before returning the value from the buffer.

unsigned int Route_Reference::getRecNum(const unsigned int t_recNum) {
  // Rev: 12/22/22.
  // Returns route's FRAM record number (0..n) for whatever is in memory (m_routeReference) at this moment.
  // We darn well better retrieve the recNum equal to the t_recNum!
  // We won't bother using this function inside of this class (we'll just use m_RouteReference.recNum), but since that's a private
  // variable, and the calling program might sometime want to know the current record's recNum, we have this function.
  if (t_recNum != m_routeReference.recNum) {  // Only do the FRAM retrieval if the record isn't already in memory.
    Route_Reference::getRouteReference(t_recNum);
  }
  return m_routeReference.recNum;
}

unsigned int Route_Reference::getRouteID(const unsigned int t_recNum) {
  // Rev: 12/22/22.
  // Returns Route ID from spreadsheet, only used to cross reference when debugging.
  // Route ID is normally irrelevant to the caller; only needed for reporting and debugging.  But we'll keep this function public
  // just in case we need it outside of the class at some point.
  if (t_recNum != m_routeReference.recNum) {  // Only do the FRAM retrieval if the record isn't already in memory.
    Route_Reference::getRouteReference(t_recNum);
  }
  return m_routeReference.routeID;
}

unsigned int Route_Reference::getFirstMatchingOrigin(const routeElement t_origin) {
  // Rev: 12/21/22.  READY FOR TESTING.  Be sure to try searching for an impossible route such as BE70.
  // Returns FRAM Rec Num of first Route Record matching t_origin, else fatal error if none found.
  // 12/21/22.  Updated with FRAM Route Index starts at 0, FRAM EB and WB records are contiguous.
  // Can probably get rid of some of the invalid field error checking once this is working properly.
  // Given a route element i.e. BW03, return the Route Reference FRAM record number for the first match.  This function is
  // typically used by Dispatcher when searching for an appropriate route for a loco, given it's current (or future) location.
  // Since there is at least one Route for every valid origin, we will ALWAYS return a valid rec num (including 0 for BE01.)
  // Of course, this doesn't mean that this route is suitable for a given train; it's just the first step.
  // Scan each Route Reference table record, one at a time, starting at the first record EB or WB.
  // WHEN (not if) we find a match, return FRAM record number 0..n.
  // If we don't find a match, this would be a fatal error because there must always be at least one route for each origin.
  // WE CAN SPEED THIS UP (EVENTUALLY) IF WE WANT by doing a binary search within East or West routes, versus searching every
  //   EB route sequentially starting with Rec 0, or every WB route sequentially starting immediately following EB routes.
  //   Which could be slow once we have 2 levels with over 150 routes in each direction, but searching FRAM is pretty dang fast.
  //   FRAM reads 8192 bytes ONE byte at a time: 206ms
  //   FRAM reads 8192 bytes 64 bytes at a time : 24ms = 128 multi-byte reads in 24ms = 5.3 multi-byte reads per millisecond.
  //   So just spitballing, around 30ms MAXIMUM to find a route when there are 150 routes in each direction.
  //   Sequential searching via getFirstMatchingOrigin() is ONLY needed when MAS wants to find a new extension or continuation
  //   route. Once a route is found, all other modules will receive the route's FRAM Record Number to look it up, which is an
  //   almost instant lookup.  So it would seem very unnecessary to complicate this code with a binary or hash lookup, since MAS
  //   can likely afford to "waste" 30ms for a lookup that only happens once each time a train needs a new route.
  // First, determine if this is an EB or WB origin, as that will eliminate half the possibilities and tell us where to start.
  unsigned int recNum = 0;
  unsigned int maxRecs = 0;
  if (t_origin.routeRecType == BE) {
    recNum = 0;  // EB routes always start at FRAM Route Table record 0.
    maxRecs = FRAM_RECS_ROUTE_EAST;  // 38 as of 01/16/23, until we add more routes and/or the second-level routes.
  } else if (t_origin.routeRecType == BW) {
    recNum = FRAM_RECS_ROUTE_EAST;   // First WB rec; for example, 38 if EB routes are 0..37, then WB is 38..n
    maxRecs = FRAM_RECS_ROUTE_WEST;  // 36 as of 01/16/23, until we add more routes and/or the second-level routes.
  } else {  // fatal error if the origin is not BE or BW!
    sprintf(lcdString, "BAD FMOa %i %i", t_origin.routeRecType, t_origin.routeRecVal); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  for (unsigned int checkIndex = recNum; checkIndex < (recNum + maxRecs); checkIndex++) {  // i.e. 0..29, or 30..51
    if (Route_Reference::getOrigin(checkIndex).routeRecVal == t_origin.routeRecVal) {  // Already know that BE or BW will match
      return checkIndex;
    }
  }
  // If we drop through to here it's fatal because we couldn't even find one match!  Impossible!
  sprintf(lcdString, "BAD FMOb %i %i", t_origin.routeRecType, t_origin.routeRecVal); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  return 0;  // Will never be executed, be eliminates a compiler warning message
}

unsigned int Route_Reference::getNextMatchingOrigin(const routeElement t_origin, const unsigned int t_recNum) {
  // Rev: 12/22/22.  READY FOR TESTING.
  // Returns FRAM Rec Num of next Route Record matching t_origin starting at t_recNum, else returns 0 if no more matches.
  // 12/22/22.  Updated with FRAM Route Index starts at 0, FRAM EB and WB records are contiguous.
  // Can probably get rid of some of the invalid field error checking once this is working properly.
  // Given an existing FRAM Route Reference record number, returns the next record number if it has the same origin i.e. BE14; else
  // returns 0.  If the next record retrieved matches t_origin, return value will ALWAYS be t_recNum (passed by the caller) + 1.
  // If there are no more records in the table, or if the next record is a different origin, returns 0.
  // Note that there is no possibility of a legitimate "next" record having a record number of 0; thus we can use return value of 0
  // to indicate no matching "next" record was found.
  if (t_origin.routeRecType == BE) {
    if ((t_recNum < 0) || (t_recNum > FRAM_RECS_ROUTE_EAST - 1)) {  // Invalid rec num
      sprintf(lcdString, "BAD NMOe %i", t_recNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
    } else if (t_recNum == FRAM_RECS_ROUTE_EAST - 1) {  // Already looking at last EB route; can't be a "next."
      return 0;
    }
  } else if (t_origin.routeRecType == BW) {
    if ((t_recNum < FRAM_RECS_ROUTE_EAST) || (t_recNum > FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST - 1)) {  // Invalid rec num
      sprintf(lcdString, "BAD NMOw %i", t_recNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
    } else if (t_recNum == (FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST - 1)) {  // Already looking at last WB route; no "next."
      return 0;
    }
  } else {  // fatal error if the origin is not BE or BW!
    sprintf(lcdString, "BAD NMOc %i %i %i", t_origin.routeRecType, t_origin.routeRecVal, t_recNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  // What we know now is that we were passed a valid Route Origin (i.e. BE03) and Route Rec Num  (i.e. 12), and there is at least
  // one "next" record available in that direction to examine.  But is it also the same block num?
  // Retrieve the next record following t_recNum and see if it matches t_origin (specifically it's block number since we already
  // know it matches direction) or not.
  if (Route_Reference::getOrigin(t_recNum + 1).routeRecVal == t_origin.routeRecVal) {  // Already know that BE or BW will match
    return t_recNum + 1;
  } else {
    return 0;
  }
}

unsigned int Route_Reference::searchRouteID(const unsigned int t_routeID) {
  // Rev: 01/17/23.  READY FOR TESTING.  Try searching for a non-existent Route ID (i.e. BE50) to confirm it crashes.
  // Brute force sequential search returns the unique FRAM Route Rec Num that matches a given (also unique) Route ID.
  // Used for testing purposes to run a train through a given route, since the operator will only be able to identify a route by
  // Route ID from the spreadsheet.  Recall that FRAM Rec Num is not known in the spreadsheet; rather, it's assigned only after all
  // routes have been sorted (by Origin + Priority + Destination,) just before populating FRAM Route Reference.
  for (unsigned int checkRec = 0; checkRec < (FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST); checkRec++) {
    // For each record in FRAM Route Reference, see if Route ID matches or keep searching
    if (Route_Reference::getRouteID(checkRec) == t_routeID) {
      return checkRec;
    }
  }
  // If we drop through to here it's fatal because we couldn't even find a match!  Impossible!
  sprintf(lcdString, "BAD ROUTE %i", t_routeID); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  return 0;  // Will never get executed; will either return valid value above or fatal error
}

routeElement Route_Reference::getOrigin(const unsigned int t_recNum) {
  // Rev: 01/24/23.
  // Returns route t_recNum's "origin" field i.e. BW03
  if (t_recNum != m_routeReference.recNum) {
    Route_Reference::getRouteReference(t_recNum);
  }
  return m_routeReference.origin;
}

routeElement Route_Reference::getDest(const unsigned int t_recNum) {
  // Rev: 01/24/23.
  // Returns route t_recNum's "destination" field i.e. BE17
  if (t_recNum != m_routeReference.recNum) {
    Route_Reference::getRouteReference(t_recNum);
  }
  return m_routeReference.destination;
}

bool Route_Reference::getPark(const unsigned int t_recNum) {
  // Rev: 01/24/23.
  // Returns route t_recNum's Park field true or false
  if (t_recNum != m_routeReference.recNum) {
    Route_Reference::getRouteReference(t_recNum);
  }
  return m_routeReference.park;
}

byte Route_Reference::getPriority(const unsigned int t_recNum) {
  // Rev: 01/24/23.
  // Returns route t_recNum's Priority field 1..5
  if (t_recNum != m_routeReference.recNum) {
    Route_Reference::getRouteReference(t_recNum);
  }
  return m_routeReference.priority;
}

routeElement Route_Reference::getElement(const unsigned int t_recNum, const byte t_elementNum) {
  // Rev: 01/26/23.
  // Returns a single route element within a route i.e. TR12.
  // t_recNum must be 0..((FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST) - 1)
  // t_elementNum must be 0..(FRAM_SEGMENTS_ROUTE_REF - 1) i.e. 0..79 since there are 80 elements/route record.
  // t_recNum is range checked when we retrieve the record, so no need to check here.
  // t_elementNum is byte so can't be negative, but we'll confirm it's within positive range.
  if ((t_elementNum > (FRAM_SEGMENTS_ROUTE_REF - 1))) {  // Fatal error (program bug)
    sprintf(lcdString, "BAD GET ELEMENT %i", t_elementNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  if (t_recNum != m_routeReference.recNum) {  // Only retrieve route record from FRAM if it's not already in memory.
    Route_Reference::getRouteReference(t_recNum);
  }
  return m_routeReference.route[t_elementNum];
}

void Route_Reference::display(const unsigned int t_recNum) {
  // Rev: 01/24/23.
  // Display a single route; not the entire table.  For testing and debugging purposes only.
  // Could reduce the amount of memory required by using F() macro in Serial.print, but we only call this for debugging so won't
  // affect the amount of memory required in the final program.
  if ((t_recNum < 0) || (t_recNum > ((FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST) - 1))) {  // Fatal error (program bug)
    sprintf(lcdString, "BAD DSP REC %i", t_recNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  sprintf(lcdString, "Rec: %3i ", Route_Reference::getRecNum(t_recNum)); Serial.print(lcdString);
  sprintf(lcdString, "Route: %3i ", Route_Reference::getRouteID(t_recNum)); Serial.print(lcdString);
  Serial.print("From ");
  if (Route_Reference::getOrigin(t_recNum).routeRecType == BE) {
    Serial.print("BE");
  } else if (Route_Reference::getOrigin(t_recNum).routeRecType == BW) {
    Serial.print("BW");
  } else {
    sprintf(lcdString, "BAD DSP ORG %i", t_recNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  Serial.print(Route_Reference::getOrigin(t_recNum).routeRecVal); Serial.print(" To ");
  if (Route_Reference::getDest(t_recNum).routeRecType == BE) {
    Serial.print("BE");
  } else if (Route_Reference::getDest(t_recNum).routeRecType == BW) {
    Serial.print("BW");
  } else {
    sprintf(lcdString, "BAD DSP DST %i", t_recNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  Serial.print(Route_Reference::getDest(t_recNum).routeRecVal); Serial.print(" ");
  Serial.print("Park: ");
  Serial.print(Route_Reference::getPark(t_recNum)); Serial.print(" ");
  Serial.print("Priority: ");
  Serial.println(Route_Reference::getPriority(t_recNum)); Serial.print(" ");
  for (byte routeSegment = 0; routeSegment < FRAM_SEGMENTS_ROUTE_REF; routeSegment++) {
    routeElement e = Route_Reference::getElement(t_recNum, routeSegment);
    if (e.routeRecType == ER) {
      Serial.print("ER");
      break;
    } else if (e.routeRecType == SN) {
      Serial.print("SN");
    } else if (e.routeRecType == BE) {
      Serial.print("BE");
    } else if (e.routeRecType == BW) {
      Serial.print("BW");
    } else if (e.routeRecType == TN) {
      Serial.print("TN");
    } else if (e.routeRecType == TR) {
      Serial.print("TR");
    } else if (e.routeRecType == FD) {
      Serial.print("FD");
    } else if (e.routeRecType == RD) {
      Serial.print("RD");
    } else if (e.routeRecType == VL) {
      Serial.print("VL");
    } else if (e.routeRecType == TD) {
      Serial.print("TD");
    } else if (e.routeRecType == SC) {
      Serial.print("SC");
    } else {
      Serial.print(e.routeRecType); Serial.print("-"); Serial.println(e.routeRecVal);
      sprintf(lcdString, "BAD DSP RE %i %i", t_recNum, routeSegment); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
    }
    sprintf(lcdString, "%2i ", e.routeRecVal); Serial.print(lcdString);
  }
  Serial.println();
}

void Route_Reference::populate() {  // Populate the Route Reference table.
  // Rev: 01/25/23.
  // We ONLY need to call this from a utility program, whenever we need to refresh the FRAM Route Reference table.
  // NOTES REGARDING MEMORY USAGE: We can populate an absolute maximum of 39 Route Reference records at a time; 40 blows up.
  // I'll break the Route Reference elements into groups of 25 elements each.
  // Just comment in/out GROUP_1, GROUP_2, etc. one at a time as we run and re-run the populate() utility.
  // Careful don't call this m_routeReference, which would confuse with our "regular" variable that holds just one record.
  // The Route Reference table is composed of the following:
  // When stored in FRAM, routes are sorted by ORIGIN + PRIORITY + DESTINATION.
  // recNum is the record number in FRAM for this record.  So we have a unique key that can be used for instant lookup.
  // routeID is the unique Route Identifier cross reference back to the spreadsheet.  These are not in any particular sequence.
  // Here is how we figure how much heap storage to reserve for populating this file:
  // sizeof(routeReferenceStruct) * number_of_records_to_load = 170 * 74 = 12,580.
  // If we use a conventional (non-heap) array, we'll use twice as much SRAM as the array requires.
  // If we use a heap array (with "new"), we'll use the amount of SRAM that the array requires.  So "new" is better.
  // Using malloc or new with the QuadRAM heap space uses about half as much of the lower SRAM as
  // using a regular array to store the Routes, but we still consume about 118 bytes per Route in lower SRAM even with malloc/new.

  Serial.println(F("Memory before new: ")); freeMemory();

  // The following routes must be sorted by ORIGIN + PRIORITY + DEST (*not* by Route Number.)

  // If we add or remove any routes, BEFORE POPULATING, be sure to update Train_Consts_Global.h with new values for
  // FRAM_RECS_ROUTE_EAST and FRAM_RECS_ROUTE_WEST.
  // If we need to expand the number of elements per Route (currently 80) update FRAM_SEGMENTS_ROUT_REF in Train_Consts_Global.h.

  // WE MUST SEARCH AND REPLACE "08" and "09" with " 8" and " 9" IN SELECTION ONLY (NOT CURRENT DOCUMENT OR ALL DOCUMENTS.)
  // Else software thinks these are OCTAL numbers.  Could also do 0-7 for cosmetics but would not change anything.

  //                                  P  P  ORG     ORG     DIR     VEL
  //          Rec Rt                  R  R  BLK     SNS     CMD     CMD
  //          Num ID  Orig    Dest    K  I  R 1     R 2     R 3     R 4     R 5     R 6     R 7     R 8     R 9     R10     R11     R12     R13     R14     R15     R16     R17     R18     R19     R20     R21     R22     R23     R24     R25     R26     R27     R28     R29     R30     R31     R32     R33     R34     R35     R36     R37     R38     R39     R40     R41     R42     R43     R44     R45     R46     R47     R48     R49     R50     R51     R52     R53     R54     R55     R56     R57     R58     R59     R60     R61     R62     R63     R64     R65     R66     R67     R68     R69     R70     R71     R72     R73     R74     R75     R76     R77     R78     R79     R80

#define GROUP_1  // Set to GROUP_1, GROUP_2, etc. to populate one group at a time.

#ifdef GROUP_1
  unsigned int numRecsToProcess = 25; // 0..24 = Num of array elements we'll transfer to FRAM at this time.
  routeReferenceStruct* data = new routeReferenceStruct[numRecsToProcess];

  data[ 0] = {  0,  1, BE, 01, BW, 13, 0, 1, BE, 01, SN, 02, FD, 00, VL, 02, TN, 06, SN, 51, VL, 03, BE, 21, SN, 52, TN, 21, SN, 18, VL, 01, BW, 13, SN, 17, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 1] = {  1,  2, BE, 01, BW, 14, 1, 1, BE, 01, SN, 02, FD, 00, VL, 02, TN, 06, SN, 51, VL, 03, BE, 21, SN, 52, TR, 21, SN, 44, BW, 16, SN, 43, TN, 20, TN, 19, SN, 14, VL, 01, BW, 14, SN, 13, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 2] = {  2,  3, BE, 01, BW, 15, 1, 2, BE, 01, SN, 02, FD, 00, VL, 02, TN, 06, SN, 51, VL, 03, BE, 21, SN, 52, TR, 21, SN, 44, BW, 16, SN, 43, TR, 20, SN, 16, VL, 01, BW, 15, SN, 15, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 3] = {  3, 47, BE, 01, BE, 02, 1, 4, BE, 01, SN, 02, FD, 00, VL, 02, TN, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, RD, 00, VL, 02, BW, 21, SN, 51, TR, 06, TR, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, FD, 00, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 4] = {  4, 48, BE, 01, BE, 03, 1, 4, BE, 01, SN, 02, FD, 00, VL, 02, TN, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, RD, 00, VL, 02, BW, 21, SN, 51, TR, 06, TR, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, FD, 00, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 5] = {  5,  7, BE, 02, BW, 13, 0, 1, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TR, 07, TR, 06, SN, 51, VL, 03, BE, 21, SN, 52, TN, 21, SN, 18, VL, 01, BW, 13, SN, 17, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 6] = {  6,  8, BE, 02, BW, 14, 1, 1, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TR, 07, TR, 06, SN, 51, VL, 03, BE, 21, SN, 52, TR, 21, SN, 44, BW, 16, SN, 43, TN, 20, TN, 19, SN, 14, VL, 01, BW, 14, SN, 13, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 7] = {  7,  5, BE, 02, BW, 05, 1, 2, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TR, 15, TR, 14, SN, 10, VL, 01, BW, 05, SN,  9, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 8] = {  8,  6, BE, 02, BW, 06, 1, 2, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TR, 15, TN, 14, SN, 12, VL, 01, BW, 06, SN, 11, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 9] = {  9,  9, BE, 02, BW, 15, 1, 2, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TR, 07, TR, 06, SN, 51, VL, 03, BE, 21, SN, 52, TR, 21, SN, 44, BW, 16, SN, 43, TR, 20, SN, 16, VL, 01, BW, 15, SN, 15, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[10] = { 10, 49, BE, 02, BE, 01, 1, 4, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TR, 07, TR, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, RD, 00, VL, 02, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, FD, 00, VL, 01, BE, 01, SN, 02, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[11] = { 11, 50, BE, 02, BE, 03, 1, 4, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TN, 07, SN, 31, VL, 01, BE, 07, SN, 32, VL, 00, RD, 00, VL, 02, BW, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, FD, 00, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[12] = { 12,  4, BE, 02, BW, 04, 0, 5, BE, 02, SN, 04, FD, 00, VL, 02, TN,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TN, 15, TR,  9, SN,  8, VL, 01, BW, 04, SN, 07, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[13] = { 13, 13, BE, 03, BW, 13, 0, 1, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TR, 07, TR, 06, SN, 51, VL, 03, BE, 21, SN, 52, TN, 21, SN, 18, VL, 01, BW, 13, SN, 17, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[14] = { 14, 14, BE, 03, BW, 14, 1, 1, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TR, 07, TR, 06, SN, 51, VL, 03, BE, 21, SN, 52, TR, 21, SN, 44, BW, 16, SN, 43, TN, 20, TN, 19, SN, 14, VL, 01, BW, 14, SN, 13, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[15] = { 15, 11, BE, 03, BW, 05, 1, 2, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TR, 15, TR, 14, SN, 10, VL, 01, BW, 05, SN,  9, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[16] = { 16, 12, BE, 03, BW, 06, 1, 2, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TR, 15, TN, 14, SN, 12, VL, 01, BW, 06, SN, 11, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[17] = { 17, 15, BE, 03, BW, 15, 1, 2, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TR, 07, TR, 06, SN, 51, VL, 03, BE, 21, SN, 52, TR, 21, SN, 44, BW, 16, SN, 43, TR, 20, SN, 16, VL, 01, BW, 15, SN, 15, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[18] = { 18, 52, BE, 03, BE, 02, 1, 4, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TN, 07, SN, 31, VL, 01, BE, 07, SN, 32, VL, 00, RD, 00, VL, 02, BW, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, FD, 00, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[19] = { 19, 51, BE, 03, BE, 01, 1, 5, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TR, 07, TR, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, RD, 00, VL, 02, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, FD, 00, VL, 01, BE, 01, SN, 02, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[20] = { 20, 53, BE, 03, BE, 04, 0, 5, BE, 03, SN, 06, RD, 00, VL, 02, BW, 03, SN, 05, TN, 05, TN, 04, SN, 37, VL, 01, BW, 10, SN, 38, VL, 00, FD, 00, VL, 02, BE, 10, SN, 37, TR, 04, TR,  9, SN, 07, VL, 01, BE, 04, SN,  8, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[21] = { 21, 10, BE, 03, BW, 04, 0, 5, BE, 03, SN, 06, FD, 00, VL, 02, TR,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TN, 15, TR,  9, SN,  8, VL, 01, BW, 04, SN, 07, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[22] = { 22, 54, BE, 04, BE, 03, 1, 3, BE, 04, SN,  8, RD, 00, VL, 02, BW, 04, SN, 07, TR, 04, SN, 37, VL, 01, BW, 10, SN, 38, VL, 00, FD, 00, VL, 02, BE, 10, SN, 37, TN, 04, TN, 05, SN, 05, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[23] = { 23, 16, BE, 04, BW, 02, 1, 4, BE, 04, SN,  8, FD, 00, VL, 02, TR,  9, TN, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[24] = { 24, 17, BE, 04, BW, 03, 1, 5, BE, 04, SN,  8, FD, 00, VL, 02, TR,  9, TN, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
#endif  // GROUP_1

#ifdef GROUP_2
  unsigned int numRecsToProcess = 25;  // 25..49 = Number of array elements we'll transfer to FRAM at this time.
  routeReferenceStruct* data = new routeReferenceStruct[numRecsToProcess];

  data[ 0] = { 25, 18, BE, 05, BW, 02, 1, 1, BE, 05, SN, 10, FD, 00, VL, 02, TR, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 1] = { 26, 19, BE, 05, BW, 03, 1, 1, BE, 05, SN, 10, FD, 00, VL, 02, TR, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 2] = { 27, 20, BE, 06, BW, 02, 1, 1, BE, 06, SN, 12, FD, 00, VL, 02, TN, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 3] = { 28, 21, BE, 06, BW, 03, 1, 1, BE, 06, SN, 12, FD, 00, VL, 02, TN, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 4] = { 29, 22, BE, 13, BW, 01, 1, 4, BE, 13, SN, 18, FD, 00, VL, 02, TN, 21, SN, 52, VL, 03, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 5] = { 30, 23, BE, 13, BW, 02, 1, 4, BE, 13, SN, 18, FD, 00, VL, 02, TN, 21, SN, 52, VL, 03, BW, 21, SN, 51, TR, 06, TR, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 6] = { 31, 24, BE, 13, BW, 03, 1, 4, BE, 13, SN, 18, FD, 00, VL, 02, TN, 21, SN, 52, VL, 03, BW, 21, SN, 51, TR, 06, TR, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 7] = { 32, 25, BE, 14, BW, 01, 1, 1, BE, 14, SN, 14, FD, 00, VL, 02, TN, 19, TN, 20, SN, 43, BE, 16, SN, 44, TR, 21, SN, 52, VL, 03, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 8] = { 33, 26, BE, 14, BW, 02, 1, 1, BE, 14, SN, 14, FD, 00, VL, 02, TN, 19, TN, 20, SN, 43, BE, 16, SN, 44, TR, 21, SN, 52, VL, 03, BW, 21, SN, 51, TR, 06, TR, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 9] = { 34, 27, BE, 14, BW, 03, 1, 1, BE, 14, SN, 14, FD, 00, VL, 02, TN, 19, TN, 20, SN, 43, BE, 16, SN, 44, TR, 21, SN, 52, VL, 03, BW, 21, SN, 51, TR, 06, TR, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[10] = { 35, 28, BE, 15, BW, 01, 1, 1, BE, 15, SN, 16, FD, 00, VL, 02, TR, 20, SN, 43, BE, 16, SN, 44, TR, 21, SN, 52, VL, 03, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[11] = { 36, 29, BE, 15, BW, 02, 1, 1, BE, 15, SN, 16, FD, 00, VL, 02, TR, 20, SN, 43, BE, 16, SN, 44, TR, 21, SN, 52, VL, 03, BW, 21, SN, 51, TR, 06, TR, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[12] = { 37, 30, BE, 15, BW, 03, 1, 1, BE, 15, SN, 16, FD, 00, VL, 02, TR, 20, SN, 43, BE, 16, SN, 44, TR, 21, SN, 52, VL, 03, BW, 21, SN, 51, TR, 06, TR, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[13] = { 38, 31, BW, 01, BE, 13, 0, 1, BW, 01, SN, 01, FD, 00, VL, 02, TN, 01, SN, 33, VL, 03, BW,  8, SN, 34, TN, 16, SN, 17, VL, 01, BE, 13, SN, 18, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[14] = { 39, 32, BW, 01, BE, 14, 1, 1, BW, 01, SN, 01, FD, 00, VL, 02, TN, 01, SN, 33, VL, 03, BW,  8, SN, 34, TR, 16, SN, 41, BE, 12, SN, 42, TN, 17, TR, 18, TN, 19, SN, 13, VL, 01, BE, 14, SN, 14, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[15] = { 40, 33, BW, 01, BE, 15, 1, 1, BW, 01, SN, 01, FD, 00, VL, 02, TN, 01, SN, 33, VL, 03, BW,  8, SN, 34, TR, 16, SN, 41, BE, 12, SN, 42, TR, 17, SN, 15, VL, 01, BE, 15, SN, 16, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[16] = { 41, 55, BW, 01, BW, 02, 1, 4, BW, 01, SN, 01, RD, 00, VL, 02, BE, 01, SN, 02, TN, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, FD, 00, VL, 02, BW, 21, SN, 51, TR, 06, TR, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[17] = { 42, 56, BW, 01, BW, 03, 1, 5, BW, 01, SN, 01, RD, 00, VL, 02, BE, 01, SN, 02, TN, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, FD, 00, VL, 02, BW, 21, SN, 51, TR, 06, TR, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[18] = { 43, 57, BW, 02, BE, 03, 1, 1, BW, 02, SN, 03, RD, 00, VL, 02, BE, 02, SN, 04, TN,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TN, 15, TR,  9, SN,  8, BW, 04, SN, 07, TR, 04, SN, 37, VL, 01, BW, 10, SN, 38, VL, 00, FD, 00, VL, 03, BE, 10, SN, 37, TN, 04, TN, 05, SN, 05, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[19] = { 44, 34, BW, 02, BE, 13, 0, 1, BW, 02, SN, 03, FD, 00, VL, 02, TR, 03, TR, 01, SN, 33, VL, 03, BW,  8, SN, 34, TN, 16, SN, 17, VL, 01, BE, 13, SN, 18, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[20] = { 45, 35, BW, 02, BE, 14, 1, 1, BW, 02, SN, 03, FD, 00, VL, 02, TR, 03, TR, 01, SN, 33, VL, 03, BW,  8, SN, 34, TR, 16, SN, 41, BE, 12, SN, 42, TN, 17, TR, 18, TN, 19, SN, 13, VL, 01, BE, 14, SN, 14, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[21] = { 46, 36, BW, 02, BE, 15, 1, 1, BW, 02, SN, 03, FD, 00, VL, 02, TR, 03, TR, 01, SN, 33, VL, 03, BW,  8, SN, 34, TR, 16, SN, 41, BE, 12, SN, 42, TR, 17, SN, 15, VL, 01, BE, 15, SN, 16, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[22] = { 47, 59, BW, 02, BW, 03, 1, 4, BW, 02, SN, 03, RD, 00, VL, 02, BE, 02, SN, 04, TN,  8, TN, 07, SN, 31, VL, 01, BW, 07, SN, 32, VL, 00, FD, 00, VL, 02, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[23] = { 48, 58, BW, 02, BW, 01, 1, 5, BW, 02, SN, 03, RD, 00, VL, 02, BE, 02, SN, 04, TN,  8, TR, 07, TR, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, FD, 00, VL, 02, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[24] = { 49, 60, BW, 03, BE, 02, 1, 1, BW, 03, SN, 05, FD, 00, VL, 02, TN, 05, TN, 04, SN, 37, VL, 01, BW, 10, SN, 38, VL, 00, RD, 00, VL, 03, BE, 10, SN, 37, TR, 04, SN, 07, BE, 04, TR,  9, SN,  8, TN, 15, SN, 32, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, FD, 00, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
#endif  // GROUP_2

#ifdef GROUP_3
  unsigned int numRecsToProcess = 24;  // 50..73 = Number of array elements we'll transfer to FRAM at this time.
  routeReferenceStruct* data = new routeReferenceStruct[numRecsToProcess];

  data[ 0] = { 50, 61, BW, 03, BE, 03, 1, 1, BW, 03, SN, 05, RD, 00, VL, 02, BE, 03, SN, 06, TR,  8, TN, 07, SN, 31, VL, 03, BW, 07, SN, 32, TN, 15, TR,  9, SN,  8, BW, 04, SN, 07, TR, 04, SN, 37, VL, 01, BW, 10, SN, 38, VL, 00, FD, 00, VL, 03, BE, 10, SN, 37, TN, 04, TN, 05, SN, 05, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 1] = { 51, 62, BW, 03, BE, 03, 1, 2, BW, 03, SN, 05, FD, 00, VL, 02, TN, 05, TN, 04, SN, 37, VL, 01, BW, 10, SN, 38, VL, 00, RD, 00, VL, 03, BE, 10, SN, 37, TR, 04, SN, 07, BE, 04, TR,  9, SN,  8, TN, 15, SN, 32, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, FD, 00, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 2] = { 52, 63, BW, 03, BW, 01, 1, 4, BW, 03, SN, 05, RD, 00, VL, 02, BE, 03, SN, 06, TR,  8, TR, 07, TR, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, FD, 00, VL, 02, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 3] = { 53, 64, BW, 03, BW, 02, 1, 4, BW, 03, SN, 05, RD, 00, VL, 02, BE, 03, SN, 06, TR,  8, TN, 07, SN, 31, VL, 01, BW, 07, SN, 32, VL, 00, FD, 00, VL, 02, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 4] = { 54, 65, BW, 04, BE, 02, 1, 1, BW, 04, SN, 07, RD, 00, VL, 02, BE, 04, TR,  9, SN,  8, VL, 03, TN, 15, SN, 32, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, FD, 00, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 5] = { 55, 66, BW, 04, BE, 03, 1, 1, BW, 04, SN, 07, RD, 00, VL, 02, BE, 04, TR,  9, SN,  8, VL, 03, TN, 15, SN, 32, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, FD, 00, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 6] = { 56, 68, BW, 04, BW, 02, 1, 3, BW, 04, SN, 07, FD, 00, VL, 01, TR, 04, SN, 37, BW, 10, SN, 38, VL, 00, RD, 00, VL, 03, BE, 10, SN, 37, TN, 04, TN, 05, SN, 05, BE, 03, SN, 06, TR,  8, TN, 07, SN, 31, VL, 01, BW, 07, SN, 32, VL, 00, FD, 00, VL, 02, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 7] = { 57, 67, BW, 04, BW, 01, 1, 4, BW, 04, SN, 07, FD, 00, VL, 01, TR, 04, SN, 37, BW, 10, SN, 38, VL, 00, RD, 00, VL, 03, BE, 10, SN, 37, TN, 04, TN, 05, SN, 05, BE, 03, SN, 06, TR,  8, TR, 07, TR, 06, SN, 51, VL, 01, BE, 21, SN, 52, VL, 00, FD, 00, VL, 02, BW, 21, SN, 51, TN, 06, SN, 02, VL, 01, BW, 01, SN, 01, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 8] = { 58, 69, BW, 04, BW, 03, 1, 4, BW, 04, SN, 07, FD, 00, VL, 01, TR, 04, SN, 37, BW, 10, SN, 38, VL, 00, RD, 00, VL, 03, BE, 10, SN, 37, TN, 04, TN, 05, SN, 05, VL, 01, BE, 03, SN, 06, VL, 00, FD, 00, VL, 01, BW, 03, SN, 05, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[ 9] = { 59, 37, BW, 05, BE, 14, 1, 1, BW, 05, SN,  9, FD, 00, VL, 02, TR, 12, TR, 11, SN, 39, VL, 03, BW, 11, SN, 40, TN, 18, TN, 19, SN, 13, VL, 01, BE, 14, SN, 14, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[10] = { 60, 70, BW, 05, BE, 02, 1, 5, BW, 05, SN,  9, RD, 00, VL, 02, BE, 05, SN, 10, TR, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, FD, 00, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[11] = { 61, 71, BW, 05, BE, 03, 1, 5, BW, 05, SN,  9, RD, 00, VL, 02, BE, 05, SN, 10, TR, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, FD, 00, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[12] = { 62, 38, BW, 06, BE, 14, 1, 1, BW, 06, SN, 11, FD, 00, VL, 02, TR, 13, TN, 12, TR, 11, SN, 39, VL, 03, BW, 11, SN, 40, TN, 18, TN, 19, SN, 13, VL, 01, BE, 14, SN, 14, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[13] = { 63, 72, BW, 06, BE, 02, 1, 5, BW, 06, SN, 11, RD, 00, VL, 02, BE, 06, SN, 12, TN, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TN,  8, SN, 04, VL, 01, BW, 02, SN, 03, VL, 00, FD, 00, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[14] = { 64, 73, BW, 06, BE, 03, 1, 5, BW, 06, SN, 11, RD, 00, VL, 02, BE, 06, SN, 12, TN, 14, TR, 15, SN, 32, VL, 03, BE, 07, SN, 31, TN, 07, TR,  8, SN, 06, VL, 01, BW, 03, SN, 05, VL, 00, FD, 00, VL, 01, BE, 03, SN, 06, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[15] = { 65, 39, BW, 13, BE, 01, 1, 4, BW, 13, SN, 17, FD, 00, VL, 02, TN, 16, SN, 34, VL, 03, BE,  8, SN, 33, TN, 01, SN, 01, VL, 01, BE, 01, SN, 02, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[16] = { 66, 40, BW, 13, BE, 02, 1, 4, BW, 13, SN, 17, FD, 00, VL, 02, TN, 16, SN, 34, VL, 03, BE,  8, SN, 33, TR, 01, TR, 03, SN, 03, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[17] = { 67, 41, BW, 14, BE, 01, 1, 1, BW, 14, SN, 13, FD, 00, VL, 02, TN, 19, TR, 18, TN, 17, SN, 42, VL, 03, BW, 12, SN, 41, TR, 16, SN, 34, BE,  8, SN, 33, TN, 01, SN, 01, VL, 01, BE, 01, SN, 02, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[18] = { 68, 42, BW, 14, BE, 02, 1, 1, BW, 14, SN, 13, FD, 00, VL, 02, TN, 19, TR, 18, TN, 17, SN, 42, VL, 03, BW, 12, SN, 41, TR, 16, SN, 34, BE,  8, SN, 33, TR, 01, TR, 03, SN, 03, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[19] = { 69, 43, BW, 14, BE, 05, 1, 1, BW, 14, SN, 13, FD, 00, VL, 02, TN, 19, TN, 18, SN, 40, VL, 03, BE, 11, SN, 39, TR, 11, TR, 12, SN,  9, VL, 01, BE, 05, SN, 10, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[20] = { 70, 44, BW, 14, BE, 06, 1, 1, BW, 14, SN, 13, FD, 00, VL, 02, TN, 19, TN, 18, SN, 40, VL, 03, BE, 11, SN, 39, TR, 11, TN, 12, TR, 13, SN, 11, VL, 01, BE, 06, SN, 12, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[21] = { 71, 45, BW, 15, BE, 01, 1, 1, BW, 15, SN, 15, FD, 00, VL, 02, TR, 17, SN, 42, VL, 03, BW, 12, SN, 41, TR, 16, SN, 34, BE,  8, SN, 33, TN, 01, SN, 01, VL, 01, BE, 01, SN, 02, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[22] = { 72, 46, BW, 15, BE, 02, 1, 1, BW, 15, SN, 15, FD, 00, VL, 02, TR, 17, SN, 42, VL, 03, BW, 12, SN, 41, TR, 16, SN, 34, BE,  8, SN, 33, TR, 01, TR, 03, SN, 03, VL, 01, BE, 02, SN, 04, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
  data[23] = { 73, 74, BW, 15, BW, 14, 1, 5, BW, 15, SN, 15, FD, 00, VL, 02, TR, 17, SN, 42, VL, 01, BW, 12, SN, 41, VL, 00, RD, 00, VL, 02, BE, 12, SN, 42, TN, 17, TR, 18, SN, 13, VL, 01, BE, 14, TN, 19, SN, 14, VL, 00, FD, 00, VL, 01, BW, 14, SN, 13, VL, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00, ER, 00 };
#endif  // GROUP_3

  Serial.println(F("Memory after new: ")); freeMemory();
  byte FRAMDataBuf[sizeof(routeReferenceStruct)];
  for (unsigned int arrayIndex = 0; arrayIndex < numRecsToProcess; arrayIndex++) {
    memcpy(FRAMDataBuf, &data[arrayIndex], sizeof(routeReferenceStruct));
    // Now that we have the data in FRAMDataBuf, need to figure out where in FRAM to write it.
    unsigned int recToWrite = data[arrayIndex].recNum;
    m_pStorage->write(Route_Reference::routeReferenceAddress(recToWrite), sizeof(routeReferenceStruct), FRAMDataBuf);
  }
  delete[] data;  // Free up the data[] array memory reserved by "new"
  Serial.println(F("Memory after delete: ")); freeMemory();
  //m_pStorage->setFRAMRevDate(01, 26, 23);  // ALWAYS UPDATE FRAM DATE IF WE CHANGE A FILE!
  return;
}

// ***** PRIVATE FUNCTIONS ***

void Route_Reference::getRouteReference(const unsigned int t_recNum) {  // Populates m_routeReference data for FRAM t_recNum.
  // Rev: 01/24/23.
  // t_recNum must be 0..((FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST) - 1)
  // FRAM read requires a "byte" pointer to the local data it's going to read into, so I need to create that via casting.
  // Here are ways to cast a pointer to our local m_routeReference struct variable so FRAM.read can use it.
  // METHOD 1 uses C-style cast:
  //   byte* pRouteReference = (byte*)&m_routeReference;
  // METHOD 2 uses C++ style cast.  This splits it into two lines, I'm not sure why:
  //   routeReferenceStruct* ptrRouteReferenceElement = &m_routeReference;
  //   byte* pRouteReference = reinterpret_cast<byte*> (ptrRouteReferenceElement);
  // METHOD 3 using C++ style cast but done in a single step:
  //   byte* pRouteReference = reinterpret_cast<byte*> (&m_routeReference);
  // We'll use the C-style cast because it's simple and works; we don't need the features that the C++ version offers.

  // First, create a new byte-type pointer pRouteReference to our local struct variable m_routeReference.
  // Note that we could define this variable in Route_Reference.h just below where we define m_routeReference, so we wouldn't need
  // to create pRouteReference every time we entered this function, but it's the only place we need this pointer, and we'll defer
  // to the rule-of-thumb that says to define variables as close as possible to where you'll use them.
  byte* pRouteReference = (byte*) (&m_routeReference);
  // Now load our local m_routeReference buffer (pointed at by pRouteReference) with data from FRAM.
  m_pStorage->read(Route_Reference::routeReferenceAddress(t_recNum), sizeof(routeReferenceStruct), pRouteReference);
  return;
}

unsigned long Route_Reference::routeReferenceAddress(const unsigned int t_recNum) {
  // Rev: 01/24/23.
  // Returns the FRAM byte address of the given record number 0..n in the Route Reference table.
  // t_recNum 0 will be FRAM_ADDR_ROUTE_REF i.e. 19114.  Subsequent records will jump by sizeof(routeReferenceStruct) bytes/record.
  // Since t_recNum is an unsigned int, no need to check if negative (and 0 is a valid record number.)
  if (t_recNum > (FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST - 1)) {
    sprintf(lcdString, "BAD RT REF REC %i", t_recNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return FRAM_ADDR_ROUTE_REF + (t_recNum * sizeof(routeReferenceStruct));
}
