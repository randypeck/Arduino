// TURNOUT_RESERVATION.H Rev: 04/14/24.  FINISHED.
// A set of functions to read and update the Turnout Reservation table, which is stored in FRAM.
// For modules *other than* SWT and LED.  I.e. for MAS, and possibly OCC and LEG (for use in tracking occupancy).
// 04/14/24: Started using TURNOUT_DIR_NORMAL and TURNOUT_DIR_REVERSE instead of 'N' and 'R'
// 04/14/24: Removed getTurnoutNumber as it was pointless -- you passed it the turnout number you wanted to retrieve!
// 03/05/23: Renamed reserve() to reserveTurnout()

// 12/27/20: I may want to add THROW commands here, in which case I should store the pin numbers (two per turnout: Normal, Reverse) needed to throw it.
// On the other hand, only MAS cares about Turnout Reservations, and only SWT cares about actually throwing turnouts.
// Still, since SWT and LED both need to know hardward details, it might be useful to store it in this table.
// On the other hand, by hard-coding that info into SWT and LED, we don't need to store that info and it works already, so why mess with it?

#ifndef TURNOUT_RESERVATION_H
#define TURNOUT_RESERVATION_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <FRAM.h>

class Turnout_Reservation {

  public:

    Turnout_Reservation();  // Constructor must be called above setup() so the object will be global to the module.
    void begin(FRAM* t_pStorage);  // Called at beginning of registration.  Unreserves all turnouts.

    // These functions expect turnoutNum to start at 1 and return locoNums starting at 1 (except "unreserved" locoNum is zero.)

    // Set last-known orientation, 'N'ormal or 'R'everse, without affecting which train is might be reserved for.
    // We need this so that in Manual mode, when the operator presses a turnout button on the control panel, we'll know to throw it
    // in the opposite orientation that it's currently set.  No need to track outside of Manual mode; we'll initialize each
    // turnout's orientation each time we start Manual mode.
    void setLastOrientation(const byte t_turnoutNum, const char t_position);

    // Get last-known orientation, 'N'ormal or 'R'everse, whether it's reserved for not.
    // This is only valid data in Manual mode as it's not tracked in other modes.
    char getLastOrientation(const byte t_turnoutNum);

    // Reserve a turnout for a particular train, but don't worry about orientation.
    // Could return bool if want to confirm if previously reserved.
    void reserveTurnout(const byte t_turnoutNum, const byte t_locoNum);

    // Set turnout's "Reserved For Train" field to zero, without affecting it's last-known orientation.
    void release(const byte t_turnoutNum);  // Could return bool if we want to confirm it was previously reserved.

    void releaseAll();  // Just a time-saver.  Does what .begin() does except doesn't initialize LCD and FRAM pointers.

    // Return train number this turnout is currently reserved for, including LOCO_ID_NULL and LOCO_ID_STATIC.
    byte reservedForTrain(const byte t_turnoutNum);

    void display(const byte t_turnoutNum);  // Display a single record to Serial COM.
    void populate();  // Special utility reads hard-coded data, writes records to FRAM.

  private:

    void getReservation(const byte t_turnoutNum);
    void setReservation(const byte t_turnoutNum);

    // Return the FRAM address of the *record* in the Turnout Reservation table for this turnout
    unsigned long turnoutReservationAddress(const byte t_turnoutNum);

    // TURNOUT RESERVATION TABLE.  This struct is only known inside the class; calling routines use getters and setters, never direct struct access.
    struct turnoutReservationStruct {
      byte turnoutNum;            // Actual turnout number 1..30 (not 0..29)
      char lastPosition;          // TURNOUT_DIR_NORMAL = 'N'ormal or TURNOUT_DIR_REVERSE = 'R'everse.
      byte reservedForTrain;      // Variable: 0 = unreserved, TRAIN_STATIC = permanently reserved, 1..8 = train number
    };
    turnoutReservationStruct m_turnoutReservation;

    FRAM* m_pStorage;           // Pointer to the FRAM memory module

};

#endif
