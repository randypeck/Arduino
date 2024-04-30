// BLOCK_RESERVATION.H Rev: 01/24/23.  TESTED AND WORKING.
// A set of functions to read and update the Block Reservation table, which is stored in FRAM.

// 01/25/23: Changed order of parms in reserveBlock() to more intuitive Block + Direction + LocoNum
// 01/17/23: Rearranging/updating some of the struct fields, updated block lengths for 1st level (2nd level unknown.)
//           Modified spreadsheet Block Res'n data to match.
// 08/12/22: Basically finished, but need to confirm actual lengths for each block.
// Block lengths are in mm, and must reflect the distance from tripping the entry sensor to tripping the exit sensor, regardless of
// which direction the train is moving.  This implies that both sensors must be the same length (we are shooting for about 10".)
// 08/12/22: Added siding speeds back in, to be used when Train Progress needs to add a not-stopping continuation route, to know
//           what speed to use in the previous destination block, since we won't want to slow to VL01.

// IMPORTANT: Until we add the second level, be sure Dispatcher automatically reserves blocks 19-20 and 23-26 for STATIC trains.

#ifndef BLOCK_RESERVATION_H
#define BLOCK_RESERVATION_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <FRAM.h>

class Block_Reservation {

  public:

    Block_Reservation();  // Constructor must be called above setup() so the object will be global to the module.
    void begin(FRAM* t_pStorage);  // Initialized FRAM pointer and un-reserves all blocks.

    // These functions expect blockNum to start at 1!  And return train nums starting at 1 (except "unreserved" train 0.)

    void reserveBlock(const byte t_blockNum, const byte t_direction, const byte t_locoNum);  // Fatal error if already reserved.
    void releaseBlock(const byte t_blockNum);
    void releaseAllBlocks();  // Un-reserves all blocks by setting reservedForTrain=0 and reservedForDirection=ER

    byte reservedForTrain(const byte t_blockNum);   // 1..n
    byte reservedDirection(const byte t_blockNum);  // BE/BW
    byte westSensor(const byte t_blockNum);         // 1..n
    byte eastSensor(const byte t_blockNum);         // 1..n
    byte westboundSpeed(const byte t_blockNum);     // 0..4
    byte eastboundSpeed(const byte t_blockNum);     // 0..4
    unsigned int length(const byte t_blockNum);     // in mm
    char sidingType(const byte t_blockNum);         // Not, Double-Ended, or Single-Ended
    bool isParkingSiding(const byte t_blockNum);    // True/False
    char stationType(const byte t_blockNum);        // Passenger/Freight/Both/Neither
    char forbidden(const byte t_blockNum);          // (blank)/Pass/Freight/Local/Through
    bool isTunnel(const byte t_blockNum);           // True/False
    char gradeDirection(const byte t_blockNum);     // Not-a-grade/Eastbound/Westbound rising

    void display(const byte t_blockNum);  // Display a single record to Serial COM.
    void populate();  // Special utility reads hard-coded data, writes records to FRAM.

  private:

    void getBlockReservation(const byte t_blockNum);  // Loads the whole record into struct variable m_blockReservation from FRAM
    void setBlockReservation(const byte t_blockNum);  // Stores the struct variable m_blockReservation into FRAM

    // Returns FRAM byte address in Block Res'n for this block.
    unsigned long blockReservationAddress(const byte t_blockNum);

    // BLOCK RESERVATION TABLE.  This struct is known only within the class.
    struct blockReservationStruct {
      byte blockNum;              // Const: Actual block num 1..26 (not 0..25, which would be the FRAM record num.)
      byte reservedForTrain;      // Variable: 0 = unreserved, TRAIN_STATIC = permanently reserved, else RESERVED for this train.
      byte reservedForDirection;  // Variable from global const: BE, BW or ER.  Not used; possible future use with signaling?
      byte westSensor;            // Const: 1..52.
      byte eastSensor;            // Const: 1..52.
      byte westboundSpeed;        // Const: 0..4 = LOCO_SPEED_STOP, _CRAWL, _LOW, _MEDIUM, and _HIGH
      byte eastboundSpeed;        // Const: 0..4 = LOCO_SPEED_STOP, _CRAWL, _LOW, _MEDIUM, and _HIGH
      unsigned int length;        // Const: Block length in mm.  Only used for siding blocks.  Used for slowing-down calcs.
      char sidingType;            // Const: SIDING_NOT_A_SIDING, SIDING_DOUBLE_ENDED, or SIDING_SINGLE_ENDED
      bool isParkingSiding;       // Const: True if it's a siding we can "Park" a train in, else false.
      char stationType;           // Const: STATION_NOT_A_STATION, STATION_PASSENGER, STATION_FREIGHT, or STATION_ANY_TYPE
      char forbidden;             // Const: FORBIDDEN_NONE, FORBIDDEN_PASSENGER, FORBIDDEN_FREIGHT, FORBIDDEN_LOCAL, FORBIDDEN_THROUGH
      // Forbidden is used by Deadlocks but not really worked out as of 2/8/23, i.e. how to forbid THROUGH trains, but allow both
      // local freight and local passenger?
      //        Not yet used, but should correspond to Train table value.
      bool isTunnel;              // Const: True if tunnel vs not a tunnel
      char gradeDirection;        // Const: GRADE_NOT_A_GRADE, GRADE_EASTBOUND, GRADE_WESTBOUND
      //        Which direction is going up?  Not currently used since Route dictates speed.
    };
    blockReservationStruct m_blockReservation;  // Could save 15 bytes (less 2 for ptr) if made this into ptr to heap.

    FRAM* m_pStorage;           // Pointer to the FRAM memory module.

};

#endif
