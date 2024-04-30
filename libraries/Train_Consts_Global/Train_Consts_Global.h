// TRAIN_CONSTS_GLOBAL.H Rev: 02/17/24.
// 02/17/23: Updated pin number for OCC WAV Trigger status input
// 03/21/23: Added RS485_MAS_ALL_ROUTE_EXT_CONT_OFFSET for send/getMAStoALLRoute()
// 03/02/23: Added LOCO_ID_POWERMASTER_n consts for use by LEG.
// 02/15/23: Changed CONTROL_TYPE_xx and LOCO_TYPE_xx to DEV_TYPE_xx
// 02/03/23: Added const chars for Loco Ref fields i.e. CONTROL_TYPE_LEGACY vs CONTROL_TYPE_TMCC
// 01/15/23: Changed FRAM_FIELDS_DEADLOCK from 9 to 11 (needed by Deadlock BW02.)
// 12/27/22: Added BX as Block Direction (in addition to BE and BW) for Deadlocks to indicate either direction is a risk.
//           Note that BX will *only* be used in the Deadlocks table, and never in Routes.
// 12/21/22: Changed FRAM Route EB and WB records to be contiguous, so eliminated some global consts used by Route Reference.
// Declares and defines all constants that are global to all (or nearly all) Arduino modules.
// Any non-const global variables should be declared here as extern, and defined in the .cpp file.
// 06/17/21 Removed Train Progress and Delayed Action constants from FRAM addresses, since they're going to be stored on the heap!
// 07/09/22 Added BR = Beginnnig-of-Route as a Route Element (for messages only; not part of Route Reference or Train Progress.)
//          12/21/22: Probably don't need BR since we're sending routes via Rec Num rather than piecemeal via RS-485.

// Header (.h) files should contain:
// * Header guards #ifndef/#define/#endif
// * Source code documentation i.e. purpose of parms, and return values of functions.
// * Type and constant DEFINITIONS.  Note this does not consume memory if not used.
// * External variable, structure, function, and object DECLARATIONS
// Large numbers of structure declarations should be split into separate headers.
// Struct declarations do not take up any memory, so no harm done if they get included with code that never defines them.
// Structure definitions can be in a corresponding .cpp file, *or* in the main source file (probably better for me.)
// enum and constants should be DEFINED not just declared in the .h file.

#ifndef TRAIN_CONSTS_GLOBAL_H
#define TRAIN_CONSTS_GLOBAL_H

#include <Arduino.h>  // Allows use of "byte" and other Arduino-specifics (eliminates 'byte' does not name a type compiler error.)

// *** ARDUINO DEVICE CONSTANTS: Here are all the different Arduinos and their "addresses" (ID numbers) for communication.
const byte ARDUINO_NUL =  0;              // Use this to initialize etc.
const byte ARDUINO_MAS =  1;              // Master Arduino (Main controller)
const byte ARDUINO_LEG =  2;              // Output Legacy interface and accessory relay control
const byte ARDUINO_SNS =  3;              // Input reads reads status of isolated track sections on layout
const byte ARDUINO_BTN =  4;              // Input reads button presses by operator on control panel
const byte ARDUINO_SWT =  5;              // Output throws turnout solenoids (Normal/Reverse) on layout
const byte ARDUINO_LED =  6;              // Output controls the Green turnout indication LEDs on control panel
const byte ARDUINO_OCC =  7;              // Output controls the Red/Green and White occupancy LEDs on control panel
const byte ARDUINO_ALL = 99;              // Master broadcasting to all i.e. mode change

// *** OPERATING MODES AND STATES:
const byte MODE_TOTAL      = 5;           // Five modes: MANUAL, REGISTER, AUTO, PARK, and POV
const byte MODE_UNDEFINED  = 0;
const byte MODE_MANUAL     = 1;
const byte MODE_REGISTER   = 2;
const byte MODE_AUTO       = 3;
const byte MODE_PARK       = 4;
const byte MODE_POV        = 5;           // Not yet supported
const byte STATE_UNDEFINED = 0;
const byte STATE_RUNNING   = 1;
const byte STATE_STOPPING  = 2;
const byte STATE_STOPPED   = 3;

// *** VARIOUS MAXIMUM VALUES ***
const byte TOTAL_BLOCKS           =  26;
const byte TOTAL_TURNOUTS         =  32;  // 30 connected, but 32 relays and we're using 32 bits in last-known.
const byte TOTAL_SENSORS          =  52;
const byte TOTAL_TRAINS           =  50;
const byte TOTAL_LEG_ACCY_RELAYS  =  16;  // 0..15 LEG accessory relays so far.

const byte LCD_WIDTH      = 20;  // 2004 (20 char by 4 lines) LCD display
const byte ALPHA_WIDTH    =  8;  // Width of 8-char alphanumeric display and of fields that use it such as train names, questions, etc.
const byte RESTRICT_WIDTH =  5;  // Locomotive 'Restrictions' field length used by Loco_Reference.h/.cpp

// *** FRAM-RELATED CONSTS AND VALUES ***
// Now we will define all of the constants that tell us where various data and tables are located in FRAM.
// Addresses within the FRAM should be unsigned long integers.
// Maximum buffer length that can be read/written in FRAM will be 255, not 256, because we must pass number-of-bytes-to-read/write
// as a byte value (0..255.)
const byte FRAMVERSION[3] { 06, 18, 60 };     // If used, this must match the version date stored in the FRAM control block.
// const unsigned long FRAM_BOTTOM =      0;  // First writable address.
// const unsigned long FRAM_TOP    = 524287;  // Was 262143 until 6/22. Top writable address; should be 524287 (512K - 1)
// We're not going to need a FRAM I/O byte buffer; we'll create buffers as needed in each class.
// *** OFFSETS OF VARIOUS TABLES IN FRAM.  I.E. STARTING ADDRESSES. ***
// Last-known turnout orientation (MAS only) is stored in the Turnout Reservation file.
// Last-known block and dir of each train (MAS only) is stored in the Block Reservation file.
const unsigned long FRAM_ADDR_REV_DATE       =      0;  // Three bytes: Month, Day, Year i.e. 11,27,20.
const unsigned long FRAM_ADDR_TURNOUT_RESN   =    256;  // Turnout res'ervatio'ns for 32 turnouts.
//                  TOTAL_TURNOUTS = 32 (defined above) number of Turnout Reservation table records.
const unsigned long FRAM_ADDR_SNS_BLK_XREF   =    512;  // Sensor-Block Xref for 52 sensors.  MAS, OCC, and LEG.
//                  TOTAL_SENSORS  = 52 (defined above) number of Sensor-Block Xref table records.
const unsigned long FRAM_ADDR_BLOCK_RESN     =   1024;  // Block Reservation for 26 blocks.  MAS, OCC, and LEG.  MAS, OCC, and LEG.
//                  TOTAL_BLOCKS   = 26 (defined above) number of Block Reservation table records.
const unsigned long FRAM_ADDR_DEADLOCK       =   3072;  // Deadlock table; currently 32 deadlocks @ 23 bytes each.  MAS only.
const byte          FRAM_FIELDS_DEADLOCK     =     11;  // Max possible num facing blocks to constitute a deadlock, per Deadlocks table.
const unsigned int  FRAM_RECS_DEADLOCK       =     30;  // Deadlock table NUMBER OF RECORDS.
const unsigned long FRAM_ADDR_LOCO_REF       =   5120;  // Loco (train) Reference table.  MAS, OCC, and LEG.
//                  TOTAL_TRAINS   = 50 (defined above) number of Loco Reference table records.
const unsigned long FRAM_ADDR_SENSOR_ACTION  =  12714;  // Sensor Action table, for sure LEG, maybe also OCC for PA Announcements?
const unsigned long FRAM_RECS_SENSOR_ACTION  =    200;  // Absolutely NO IDEA how many records, if any, will be in this file yet.

const unsigned long FRAM_ADDR_ROUTE_REF      =  19114;  // Route Reference EB routes starting address, followed by WB. MAS/OCC/LEG.
const unsigned int  FRAM_RECS_ROUTE_EAST     =     38;  // 38 "level 1 only" EB routes as of 01/16/22.
const unsigned int  FRAM_RECS_ROUTE_WEST     =     36;  // 36 "level 1 only" WB routes as of 01/16/22.
// 12/21/22: Rather than define a fixed FRAM address where the WB route records start, we'll just calculate it in the
// Route_Reference class, using: FRAM_ADDR_ROUTE_REF + (FRAM_RECS_ROUTE_EAST + sizeof(routeReferenceStruct)).
// const unsigned int  FRAM_FIRST_EAST_ROUTE    =      0;  // Index (starting at 0) into FRAM Route Reference table where the first Eastbound route can be found.
// const unsigned int  FRAM_FIRST_WEST_ROUTE    =    326;  // Index (starting at 326) into FRAM Route Reference table where the first Westbound route can be found.
const byte          FRAM_SEGMENTS_ROUTE_REF  =     80;  // Route Reference max number of "routeElement" segments per route.  Used when defining routeReferenceStruct.
const unsigned long FRAM_ADDR_MAS_LOG_FILE   = 186026;  // Address of the two-byte "next byte to write" in the log file.  Each record will need to include locoNum + routeElement.

// *** DELAYED-ACTION-RELATED CONSTS ***
const          int  HEAP_RECS_DELAYED_ACTION =   1000;  // int vs unsigned int because we compare it to values that can be negative; eliminates compiler warnings

// *** ROUTE REFERENCE CONSTS ***

// Define the byte values of each of the various commands that can be part of a Route i.e. CN, BE, etc.
// Note: SP is a reserved const in Arduino, so can't use it for "Speed"
// It would be better, but less convenient when defining routes, to use a prefix with these constants i.e. ROUTE_SN, ROUTE_BE, etc.
// But that would require us to use those longer labels in the spreadsheet (on which routes are based) thus too much trouble.
//const byte BR =  1;  // Beginning-of-Route.  Used only as message from MAS to LEG; not part of Route Reference or Train Progress.
const byte ER =  2;  // End-Of-Route
const byte SN =  3;  // Sensor
const byte BE =  4;  // Block Eastbound
const byte BW =  5;  // Block Westbound
const byte TN =  6;  // Turnout Normal
const byte TR =  7;  // Turnout Reverse
const byte FD =  8;  // Direction Forward
const byte RD =  9;  // Direction Reverse
const byte VL = 10;  // Velocity (NOTE: "SP" IS RESERVED IN ARDUINO) (values can be SPEED_STOP = 0, thru SPEED_HIGH = 4.)
const byte TD = 11;  // Time Delay in seconds (not ms)
const byte BX = 12;  // Deadlock table only.  Block either direction is a threat.
const byte SC = 13;  // Sensor Clear (not implemented yet, as of 1/23)

// routeReferenceStruct is only used in Route_Reference.h/.cpp, but routeElement is used in several modules so global here.
struct routeElement {
  byte routeRecType;  // i.e. 2 = BW = "Block West"
  byte routeRecVal;   // i.e. 4 (if BW04, for example)
};

// *** TRAIN PROGRESS CONSTS ***
const unsigned int  HEAP_RECS_TRAIN_PROGRESS =    140;  // Train Progress table NUM ROUTE ELEMENTS/TRAIN (circular buf size.)

// *** DELAYED-ACTION TABLE CONSTS ***

// SUMMARY OF ALL LEGACY/TMCC COMMANDS THAT CAN BE POPULATED IN THE DELAYED-ACTION TABLE as Device Command in Delayed Action:
// Rev: 02/22/23
// THESE CONSTS ARE NEEDED BY THE CONDUCTOR, DELAYED ACTION, AND ENGINEER CLASSES, SO THEY ARE DEFINED GLOBAL HERE.
// Station PA Announcements aren't part of Delayed Action, because OCC controls the WAV Trigger.
//   Don't confuse this with Loco and StationSounds Diner announcements, which ARE handled by Delayed Action.
const byte LEGACY_ACTION_NULL          =  0;
const byte LEGACY_ACTION_STARTUP_SLOW  =  1;  // 3-byte
const byte LEGACY_ACTION_STARTUP_FAST  =  2;  // 3-byte
const byte LEGACY_ACTION_SHUTDOWN_SLOW =  3;  // 3-byte
const byte LEGACY_ACTION_SHUTDOWN_FAST =  4;  // 3-byte
const byte LEGACY_ACTION_SET_SMOKE     =  5;  // 9-byte.  0x7C FX Control Trigger requires parm 0x00..0x03 (Off/Low/Med/Hi)
const byte LEGACY_ACTION_EMERG_STOP    =  6;  // 3-byte
const byte LEGACY_ACTION_ABS_SPEED     =  7;  // 3-byte.  Requires Parm1 0..199
const byte LEGACY_ACTION_MOMENTUM_OFF  =  8;  // 3-byte.  We will never set Momentum to any value but zero.
const byte LEGACY_ACTION_STOP_IMMED    =  9;  // 3-byte.  Prefer over Abs Spd 0 b/c overrides momentum if instant stop desired.
const byte LEGACY_ACTION_FORWARD       = 10;  // 3-byte.  Delayed Action table command types
const byte LEGACY_ACTION_REVERSE       = 11;  // 3-byte.  
const byte LEGACY_ACTION_FRONT_COUPLER = 12;  // 3-byte.  
const byte LEGACY_ACTION_REAR_COUPLER  = 13;  // 3-byte.  
const byte LEGACY_ACTION_ACCESSORY_ON  = 14;  // Not part of Legacy/TMCC; Relays managed by OCC and LEG
const byte LEGACY_ACTION_ACCESSORY_OFF = 15;  // Not part of Legacy/TMCC; Relays managed by OCC and LEG
const byte PA_SYSTEM_ANNOUNCE          = 16;  // Not part of Legacy/TMCC; WAV Trigger managed by OCC.
const byte LEGACY_SOUND_HORN_NORMAL    = 20;  // 3-byte.
const byte LEGACY_SOUND_HORN_QUILLING  = 21;  // 3-byte.  Requires intensity 0..15
const byte LEGACY_SOUND_REFUEL         = 22;  // 3-byte.  With minor dialogue.  Nothing on SP 1440.  Steam only??? *******************
const byte LEGACY_SOUND_DIESEL_RPM     = 23;  // 3-byte.  Diesel only. Requires Parm 0..7
const byte LEGACY_SOUND_WATER_INJECT   = 24;  // 3-byte.  Steam only.
const byte LEGACY_SOUND_ENGINE_LABOR   = 25;  // 3-byte.  Diesel only?  Barely discernable on SP 1440. *******************
const byte LEGACY_SOUND_BELL_OFF       = 26;  // 3-byte.  
const byte LEGACY_SOUND_BELL_ON        = 27;  // 3-byte.  
const byte LEGACY_SOUND_BRAKE_SQUEAL   = 28;  // 3-byte.  Only works when moving.  Short chirp.
const byte LEGACY_SOUND_AUGER          = 29;  // 3-byte.  Steam only.
const byte LEGACY_SOUND_AIR_RELEASE    = 30;  // 3-byte.  Can barely hear.  Not sure if it works on steam as well as diesel? **************
const byte LEGACY_SOUND_LONG_LETOFF    = 31;  // 3-byte.  Diesel and (presumably) steam. Long hiss. *****************
const byte LEGACY_SOUND_MSTR_VOL_UP    = 32;  // 9-byte.  There are 10 possible volumes (9 excluding "off") i.e. 0..9.
const byte LEGACY_SOUND_MSTR_VOL_DOWN  = 33;  // 9-byte.  No way to go directly to a master vol level, so we must step up/down as desired.
const byte LEGACY_SOUND_BLEND_VOL_UP   = 34;  // 9-byte.  Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
const byte LEGACY_SOUND_BLEND_VOL_DOWN = 35;  // 9-byte.  Tried this but wasn't able to get any response (same with 9-byte mstr vol up/down)
const byte LEGACY_SOUND_COCK_CLEAR_ON  = 36;  // 9-byte.  Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
const byte LEGACY_SOUND_COCK_CLEAR_OFF = 37;  // 9-byte.  Steam only. This is a 9-byte 0x74 Legacy Railsounds FX Trigger
const byte LEGACY_DIALOGUE             = 41;  // Separate function.  For any of the 38 Legacy Dialogues (9-byte 0x72), which will be passed as a parm.
const byte TMCC_DIALOGUE               = 42;  // Separate function.  For any of the 6 TMCC StationSounds Diner cmds, which will be passed as a parm.
const byte LEGACY_NUMERIC_PRESS        = 43;  // 3-byte.  Need for vol up/down

// LEGACY HORN COMMAND *PATTERNS* (used by Conductor class to pass horn patterns to Delayed_Action class.)
const byte LEGACY_PATTERN_SHORT_TOOT   =  1;  // S    (Used informally i.e. to tell operator "I'm here" when registering)
const byte LEGACY_PATTERN_LONG_TOOT    =  2;  // L    (Used informally)
const byte LEGACY_PATTERN_STOPPED      =  3;  // S    (Applying brakes)
const byte LEGACY_PATTERN_APPROACHING  =  4;  // L    (Approaching PASSENGER station -- else not needed.  Some use S.)
const byte LEGACY_PATTERN_DEPARTING    =  5;  // L-L  (Releasing air brakes. Some railroads that use L-S, but I prefer L-L)
const byte LEGACY_PATTERN_BACKING      =  6;  // S-S-S
const byte LEGACY_PATTERN_CROSSING     =  7;  // L-L-S-L

// LEGACY RAILSOUND DIALOGUE COMMAND *PARAMETERS* (specified as Parm1 in Delayed Action, by Conductor, also used by Engineer)
// ONLY VALID WHEN Device Command = LEGACY_DIALOGUE.
// Part of 9-byte 0x72 Legacy commands i.e. three 3-byte "words"
// Rev: 06/11/22
// So the main command will be LEGACY_DIALOGUE and Parm 1 will be one of the following.
// Note: The following could just be random bytes same as above, but eventually the Conductor or Engineer will need to know the
// hex value (0x##) to trigger each one -- so they might just as well be the same as the hex codes, right?
const byte LEGACY_DIALOGUE_T2E_STARTUP                   = 0x06;  // 06 Tower tells engineer to startup
const byte LEGACY_DIALOGUE_E2T_DEPARTURE_NO              = 0x07;  // 07 Engineer asks okay to depart, tower says no
const byte LEGACY_DIALOGUE_E2T_DEPARTURE_YES             = 0x08;  // 08 Engineer asks okay to depart, tower says yes
const byte LEGACY_DIALOGUE_E2T_HAVE_DEPARTED             = 0x09;  // 09 Engineer tells tower "here we come" ?
const byte LEGACY_DIALOGUE_E2T_CLEAR_AHEAD_YES           = 0x0A;  // 10 Engineer asks tower "Am I clear?", Yardmaster says yes
const byte LEGACY_DIALOGUE_T2E_NON_EMERG_STOP            = 0x0B;  // 11 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_T2E_RESTRICTED_SPEED          = 0x0C;  // 12 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_T2E_SLOW_SPEED                = 0x0D;  // 13 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_T2E_MEDIUM_SPEED              = 0x0E;  // 14 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_T2E_LIMITED_SPEED             = 0x0F;  // 15 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_T2E_NORMAL_SPEED              = 0x10;  // 16 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_T2E_HIGH_BALL_SPEED           = 0x11;  // 17 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_E2T_ARRIVING                  = 0x12;  // 18 Arriving, E2T asks if lead is clear, tower says yes
const byte LEGACY_DIALOGUE_E2T_HAVE_ARRIVED              = 0x13;  // 19 E2T in the clear and ready; T2E set brakes
const byte LEGACY_DIALOGUE_E2T_SHUTDOWN                  = 0x14;  // 20 Dialogue used in long shutdown "goin' to beans"
const byte LEGACY_DIALOGUE_T2E_STANDBY                   = 0x22;  // 34 Tower says "please hold" i.e. after req by eng to proceed
const byte LEGACY_DIALOGUE_T2E_TAKE_THE_LEAD             = 0x23;  // 35 T2E okay to proceed; "take the lead", clear to pull
const byte LEGACY_DIALOGUE_CONDUCTOR_ALL_ABOARD          = 0x30;  // 48 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_FUEL_LEVEL     = 0x3D;  // 61 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_FUEL_REFILLED  = 0x3E;  // 62 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_SPEED          = 0x3F;  // 63 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_WATER_LEVEL    = 0x40;  // 64 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_ENGINEER_SPEAK_WATER_REFILLED = 0x41;  // 65 Not implemented on SP 1440
const byte LEGACY_DIALOGUE_SS_CONDUCTOR_NEXT_STOP        = 0x68;  // 104 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_CONDUCTOR_WATCH_YOUR_STEP  = 0x69;  // 105 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_CONDUCTOR_ALL_ABOARD       = 0x6A;  // 106 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_CONDUCTOR_TICKETS_PLEASE   = 0x6B;  // 107 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_CONDUCTOR_PREMATURE_STOP   = 0x6C;  // 108 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STEWARD_WELCOME_ABOARD     = 0x6D;  // 109 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STEWARD_FIRST_SEATING      = 0x6E;  // 110 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STEWARD_SECOND_SEATING     = 0x6F;  // 111 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STEWARD_LOUNGE_CAR_OPEN    = 0x70;  // 112 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_ARRIVING  = 0x71;  // 113 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_ARRIVED   = 0x72;  // 114 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_BOARDING  = 0x73;  // 115 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STATION_PA_TRAIN_DEPARTED  = 0x74;  // 116 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STATION_PASS_CAR_STARTUP   = 0x75;  // 117 Don't have any Legacy StationSounds diners, so n/a for now.
const byte LEGACY_DIALOGUE_SS_STATION_PASS_CAR_SHUTDOWN  = 0x76;  // 118 Don't have any Legacy StationSounds diners, so n/a for now.

// TMCC STATIONSOUNDS DINER DIALOGUE COMMAND *PARAMETERS* (specified as Parm1 in Delayed Action, by Conductor, also used by Engineer)
// ONLY VALID WHEN Device Command = TMCC_DIALOGUE.
// Rev: 06/11/22.
// All of my StationSounds Diners are TMCC; no need for Legacy StationSounds support but we have it.
// These are either single-digit (single 3-byte) commands, or Aux-1 + digit commands (total of 6 bytes.)
// Dialogue commands depend on if the train is stopped or moving, and sometimes what announcements were already made.
//   Although it's possible to assign a Stationsounds Diner as part of a Legacy Train (vs Engine), I think it will be easier to
//   just give each SS Diner a unique Engine ID and associate it with a particular Engine/Train in the Loco Ref table.  Then I can
//   just send discrete TMCC Stationsounds commands to the diner when I know the train is doing whatever is applicable.
//   IMPORTANT: May need to STARTUP (and SHUTDOWN) the StationSounds Diner before using it, so somehow part of train startup.
const byte TMCC_DIALOGUE_STATION_ARRIVAL         = 1;  // 6 bytes: Aux1+7. When STOPPED: PA announcement i.e. "The Daylight is now arriving."
const byte TMCC_DIALOGUE_STATION_DEPARTURE       = 2;  // 3 bytes: 7.      When STOPPED: PA announcement i.e. "The Daylight is now departing."
const byte TMCC_DIALOGUE_CONDUCTOR_ARRIVAL       = 3;  // 6 bytes: Aux1+2. When STOPPED: Conductor says i.e. "Watch your step."
const byte TMCC_DIALOGUE_CONDUCTOR_DEPARTURE     = 4;  // 3 bytes: 2.      When STOPPED: Conductor says i.e. "Watch your step." then "All aboard!"
const byte TMCC_DIALOGUE_CONDUCTOR_TICKETS_DINER = 5;  // 3 bytes: 2.      When MOVING: Conductor says "Welcome aboard/Tickets please/1st seating."
const byte TMCC_DIALOGUE_CONDUCTOR_STOPPING      = 6;  // 6 bytes: Aux1+2. When MOVING: Conductor says "Next stop coming up."

const byte LEGACY_CMD_DELAY           =  30;  // ms for Engineer to wait between successive 3-byte Legacy or TMCC cmds going out the serial port.
const byte LEGACY_CMD_BYTES           =   9;  // Element size is always 9 bytes to hold largest possible Legacy command.
const int  LEGACY_CMD_HEAP_RECS       = 100;  // How many struct elements in the Engineer Legacy Command buffer (9 bytes/element)
const byte LOCO_ID_NULL               =   0;  // Used for "no train."
const byte LOCO_ID_POWERMASTER_1      =  91;  // This is the Legacy Engine Number needed by Legacy to turn PowerMasters on and off.
const byte LOCO_ID_POWERMASTER_2      =  92;  // These can be changed by re-programming the PowerMasters and changing these consts.
const byte LOCO_ID_POWERMASTER_3      =  93;  // Address their Engine num and set Abs Speed to 1 (on) or 0 (off.)
const byte LOCO_ID_POWERMASTER_4      =  94;  // Not using this one as of Jan 2017.
const byte LOCO_ID_STATIC             =  99;  // This train number is a static train. 99 is ALSO the UNIVERSAL num that all locos respond to.
const byte LOCO_ID_DONE               = 100;  // Used when selecting "done" on rotary encoder
const char LOCO_DIRECTION_NONE        = ' ';
const char LOCO_DIRECTION_EAST        = 'E';
const char LOCO_DIRECTION_WEST        = 'W';
const byte LOCO_SPEED_STOP            =   0;  // NOTE: "SS" IS RESERVED IN ARDUINO
const byte LOCO_SPEED_CRAWL           =   1;
const byte LOCO_SPEED_LOW             =   2;
const byte LOCO_SPEED_MEDIUM          =   3;
const byte LOCO_SPEED_HIGH            =   4;  // NOTE: "HIGH" IS RESERVED IN ARDUINO
const char SIDING_NOT_A_SIDING        = 'N';
const char SIDING_DOUBLE_ENDED        = 'D';
const char SIDING_SINGLE_ENDED        = 'S';
const char STATION_PASSENGER          = 'P';  // Passenger trains only allowed at this station
const char STATION_FREIGHT            = 'F';  // Freight trains only allowed at this station
const char STATION_ANY_TYPE           = 'B';  // Both; can be used as both a Passenger and a Freight station
const char STATION_NOT_A_STATION      = 'N';  // This block is not a station; nobody may stop here
const char FORBIDDEN_NONE             = ' ';
const char FORBIDDEN_PASSENGER        = 'P';
const char FORBIDDEN_FREIGHT          = 'F';
const char FORBIDDEN_LOCAL            = 'L';
const char FORBIDDEN_THROUGH          = 'T';
const char GRADE_NOT_A_GRADE          = ' ';
const char GRADE_EASTBOUND            = 'E';
const char GRADE_WESTBOUND            = 'W';
const char SENSOR_END_EAST            = 'E';  // Used by Sensor_Block::whichEnd();
const char SENSOR_END_WEST            = 'W';  // Used by Sensor_Block::whichEnd();
const char SENSOR_STATUS_TRIPPED      = 'T';
const char SENSOR_STATUS_CLEARED      = 'C';
const char DEV_TYPE_LEGACY_ENGINE     = 'E';
const char DEV_TYPE_LEGACY_TRAIN      = 'T';
const char DEV_TYPE_TMCC_ENGINE       = 'N';
const char DEV_TYPE_TMCC_TRAIN        = 'R';
const char DEV_TYPE_ACCESSORY         = 'A';
const char POWER_TYPE_STEAM           = 'S';
const char POWER_TYPE_DIESEL          = 'D';
const char POWER_TYPE_SS_DINER        = '1';
const char POWER_TYPE_CRANE_BOOM      = '2';
const char LOCO_TYPE_PASS_EXPRESS     = 'P';
const char LOCO_TYPE_PASS_LOCAL       = 'p';
const char LOCO_TYPE_FREIGHT_EXPRESS  = 'F';
const char LOCO_TYPE_FREIGHT_LOCAL    = 'f';
const char LOCO_TYPE_MOW              = 'M';
const char ROUTE_TYPE_NULL            = 'N';  // Fn return value when neither an Extension or Continuation route is an option.
const char ROUTE_TYPE_EXTENSION       = 'E';  // This is a route that will be started with the loco stopped (initial or add-on)
const char ROUTE_TYPE_CONTINUATION    = 'C';  // This is a route that will be added while the loco is still moving towards it.

const char TURNOUT_DIR_NORMAL         = 'N';
const char TURNOUT_DIR_REVERSE        = 'R';

// Serial port speed
// 6/5/22: These should really be i.e. SERIAL_SPEED_MONITOR, SERIAL_SPEED_2004LCD, SERIAL_SPEED_RS485, SERIAL_SPEED_LEGACY_BASE
const long unsigned int SERIAL0_SPEED = 115200;  // Serial port 0 is used for serial monitor
const long unsigned int SERIAL1_SPEED = 115200;  // Serial port 1 is Digole 2004 LCD display
const long unsigned int SERIAL2_SPEED = 115200;  // Serial port 2 is the RS485 communications bus
const long unsigned int SERIAL3_SPEED =   9600;  // Serial port 3 is LEG Legacy xface, and OCC WAV Trigger xface; both 9600.

// Note that the serial input buffer is only 64 bytes, which means that we need to keep emptying it since there
// will be many commands between Arduinos, even though most may not be for THIS Arduino.  If the buffer overflows,
// then we will be totally screwed up (but it will be apparent in the checksum.)
const byte RS485_TRANSMIT    = HIGH;      // HIGH = 0x1.  How to set TX_CONTROL pin when we want to transmit RS485
const byte RS485_RECEIVE     = LOW;       // LOW = 0x0.  How to set TX_CONTROL pin when we want to receive (or NOT transmit) RS485
const byte RS485_MAX_LEN     = 20;        // buf len to hold the longest possible RS485 msg incl to, from, CRC.  16 as of 3/3/23.
const byte RS485_LEN_OFFSET  =  0;        // first byte of message is always total message length in bytes
const byte RS485_FROM_OFFSET =  1;        // second byte of message is the ID of the Arduino the message is coming from
const byte RS485_TO_OFFSET   =  2;        // third byte of message is the ID of the Arduino the message is addressed to
const byte RS485_TYPE_OFFSET =  3;        // fourth byte of message is the type of message such as M for Mode, S for Smoke, etc.
// Note also that the LAST byte of every message is a CRC8 checksum of all bytes except the last.

// These RS485 message offsets are specific to individual modules (though all are used by O-MAS).
// The 3-char name refers to FROM, TO, and MESSAGE TYPE.
// Confirmed okay as of 03/03/23.
const byte RS485_MAS_ALL_MODE_OFFSET               =  4;  // const byte i.e. MODE_MANUAL
const byte RS485_MAS_ALL_STATE_OFFSET              =  5;  // const byte i.e. STATE_RUNNING
const byte RS485_OCC_LEG_FAST_SLOW_OFFSET          =  4;  // char F|S
const byte RS485_OCC_LEG_SMOKE_ON_OFF_OFFSET       =  4;  // char S|N
const byte RS485_OCC_LEG_AUDIO_ON_OFF_OFFSET       =  4;  // char A|N
const byte RS485_OCC_LEG_DEBUG_ON_OFF_OFFSET       =  4;  // char D|N
const byte RS485_OCC_ALL_REGISTER_LOCO_NUM_OFFSET  =  4;  // byte 1..50
const byte RS485_OCC_ALL_REGISTER_BLOCK_NUM_OFFSET =  5;  // byte 1..26
const byte RS485_OCC_ALL_REGISTER_BLOCK_DIR_OFFSET =  6;  // const byte BE or BW
const byte RS485_MAS_ALL_ROUTE_LOCO_NUM_OFFSET     =  4;  // byte 1..50
const byte RS485_MAS_ALL_ROUTE_EXT_CONT_OFFSET     =  5;  // char E|C
const byte RS485_MAS_ALL_ROUTE_REC_NUM_OFFSET      =  6;  // byte 0..(FRAM_RECS_ROUTE_EAST + FRAM_RECS_ROUTE_WEST - 1)
const byte RS485_MAS_ALL_ROUTE_TIME_DELAY_OFFSET   =  8;  // 2-byte unsigned int SECONDS (not ms) to delay before starting Extension (stopped) route.
const byte RS485_BTN_MAS_BUTTON_NUM_OFFSET         =  4;  // byte 1..32 (30 connected turnouts, but 32 relays)
const byte RS485_MAS_ALL_SET_TURNOUT_NUM_OFFSET    =  4;  // byte 1..30 (30 connected turnouts, but 32 relays)
const byte RS485_MAS_ALL_SET_TURNOUT_DIR_OFFSET    =  5;  // char N|R
const byte RS485_MAS_SNS_SENSOR_NUM_OFFSET         =  4;  // byte 1..52
const byte RS485_SNS_ALL_SENSOR_NUM_OFFSET         =  4;  // byte 1..52
const byte RS485_SNS_ALL_SENSOR_TRIP_CLEAR_OFFSET  =  5;  // char T|C

// *** ARDUINO PIN NUMBERS:
// *** STANDARD I/O PORT PIN NUMBERS ***
// const byte PIN_IN_MEGA_RX0             =  0;  // Serial 0 receive PC Serial monitor
// const byte PIN_OUT_MEGA_TX0            =  1;  // Serial 0 transmit PC Serial monitor
// const byte PIN_IN_MEGA_RX1             = 19;  // Serial 1 receive NOT NEEDED FOR 2004 LCD (previosly used for FRAM3)
// const byte PIN_OUT_MEGA_TX1            = 18;  // Serial 1 transmit 2004 LCD
// const byte PIN_IN_MEGA_RX2             = 17;  // Serial 2 receive RS485 network
// const byte PIN_OUT_MEGA_TX2            = 16;  // Serial 2 transmit RS485 network
// const byte PIN_IN_MEGA_RX3             = 15;  // Serial 3 receive A_LEG Legacy, A_OCC WAV Trigger
// const byte PIN_OUT_MEGA_TX3            = 14;  // Serial 3 transmit A_LEG Legacy, A_OCC WAV Trigger
// const byte PIN_IO_MEGA_SDA             = 20;  // I2C (also pin 44)
// const byte PIN_IO_MEGA_SCL             = 21;  // I2C (also pin 43)
// const byte PIN_IO_MEGA_MISO            = 50;  // SPI Master In Slave Out (MISO) a.k.a. Controller In Peripheral Out (CIPO)
// const byte PIN_IO_MEGA_MOSI            = 51;  // SPI Master Out Slave In (MOSI) a.k.a. Controller Out Peripheral In (COPI)
// const byte PIN_IO_MEGA_SCK             = 52;  // SPI Clock
// const byte PIN_IO_MEGA_SS              = 53;  // SPI Slave Select (SS) a.k.a. Chip Select (CS).  Used for the 256KB/512KB FRAM chip.

// *** COMMUNICATION PIN NUMBERS ***
const byte PIN_OUT_RS485_RX_LED        =  6;  // Output: set HIGH to turn on YELLOW when RS485 is RECEIVING data
const byte PIN_OUT_RS485_TX_ENABLE     =  4;  // Output: set HIGH when in RS485 transmit mode, LOW when not transmitting
const byte PIN_OUT_RS485_TX_LED        =  5;  // Output: set HIGH to turn on BLUE LED when RS485 is TRANSMITTING data
const byte PIN_OUT_REQ_TX_BTN          =  8;  // O_BTN output pin pulled LOW when it wants to tell A-MAS a turnout button has been pressed.
const byte PIN_IN_REQ_TX_BTN           =  8;  // O_MAS input pin pulled LOW by A-BTN when it wants to tell A-MAS a turnout button has been pressed.
const byte PIN_OUT_REQ_TX_LEG          =  8;  // O_LEG output pin pulled LOW when it wants to send A_MAS a message (unused as of 2/24.)
const byte PIN_IN_REQ_TX_LEG           =  2;  // O_MAS input pin pulled LOW by A_LEG when it wants to send A_MAS a message (unused as of 2/24.)
const byte PIN_OUT_REQ_TX_SNS          =  8;  // O_SNS output pin pulled LOW when it wants to send A_MAS an occupancy sensor change message.
const byte PIN_IN_REQ_TX_SNS           =  3;  // O_MAS input pin pulled LOW by A_SNS when it wants to send A_MAS an occupancy sensor change message.

// *** QuadMEM HEAP MEMORY MODULE PIN NUMBERS ***
const byte PIN_OUT_XMEM_ENABLE         = 38;
const byte PIN_OUT_XMEM_BANK_BIT_0     = 42;
const byte PIN_OUT_XMEM_BANK_BIT_1     = 43;
const byte PIN_OUT_XMEM_BANK_BIT_2     = 44;

// *** MAS CONTROL PANEL MAS MODE CONTROL PINS ***
// Need to move pins along back row to make room for QuadRAM card.
// 6/29/22 CHANGED THESE 14 PINS FROM D23-D49 to ANALOG PINS A02-A15 == DIGITAL PINS D56-D69
const byte PIN_IN_ROTARY_AUTO          = 67;  // Changed D27 to A13 = D67. Input : MAS Rotary mode "Auto."  Pulled LOW
const byte PIN_IN_ROTARY_MANUAL        = 69;  // Changed D23 to A15 = D69. Input : MAS Rotary mode "Manual."  Pulled LOW
const byte PIN_IN_ROTARY_PARK          = 66;  // Changed D29 to A12 = D66. Input : MAS Rotary mode "Park."  Pulled LOW
const byte PIN_IN_ROTARY_POV           = 65;  // Changed D31 to A11 = D65. Input : MAS Rotary mode "P.O.V."  Pulled LOW
const byte PIN_IN_ROTARY_REGISTER      = 68;  // Changed D25 to A14 = D68. Input : MAS Rotary mode "Register."  Pulled LOW
const byte PIN_IN_ROTARY_START         = 64;  // Changed D33 to A10 = D64. Input : MAS Rotary "Start" button.  Pulled LOW
const byte PIN_IN_ROTARY_STOP          = 63;  // Changed D35 to A09 = D63. Input : MAS Rotary "Stop" button.  Pulled LOW
const byte PIN_OUT_ROTARY_LED_AUTO     = 58;  // Changed D45 to A04 = D58. Output: MAS Rotary Auto position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_MANUAL   = 60;  // Changed D41 to A06 = D60. Output: MAS Rotary Manual position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_PARK     = 57;  // Changed D47 to A03 = D57. Output: MAS Rotary Park position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_POV      = 56;  // Changed D49 to A02 = D56. Output: MAS Rotary P.O.V. position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_REGISTER = 59;  // Changed D43 to A05 = D59. Output: MAS Rotary Register position LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_START    = 62;  // Changed D37 to A08 = D62. Output: MAS Rotary Start button internal LED.  Pull LOW to turn on.
const byte PIN_OUT_ROTARY_LED_STOP     = 61;  // Changed D39 to A07 = D61. Output: MAS Rotary Stop button internal LED.  Pull LOW to turn on.

// *** OCC CONTROL PANEL OCC ROTARY ENCODER PIN NUMBERS ***
const byte PIN_IN_ROTARY_1             =  2;  // Input: OCC Rotary Encoder pin 1 of 2 (plus Select)
const byte PIN_IN_ROTARY_2             =  3;  // Input: OCC Rotary Encoder pin 2 of 2 (plus Select)
const byte PIN_IN_ROTARY_PUSH          = 19;  // Input: OCC Rotary Encoder "Select" (pushbutton) pin

// *** LEG CONTROL PANEL TOGGLE SWITCH PIN NUMBERS ***
// Need to move pins along back row to make room for QuadRAM card.
// 6/29/22 CHANGED THESE 8 PINS FROM D23-D37 TO ANALOG PINS A8-A15 == DIGITAL PINS D62-D69
const byte PIN_IN_PANEL_4_OFF          = 62;  // Change D37 to A08 = D62. Input: LEG Control panel #4 PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_4_ON           = 63;  // Change D35 to A09 = D63. Input: LEG Control panel #4 PowerMaster toggled up.  Pulled LOW.
const byte PIN_IN_PANEL_BLUE_OFF       = 66;  // Change D29 to A12 = D66. Input: LEG Control panel "Blue" PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_BLUE_ON        = 67;  // Change D27 to A13 = D67. Input: LEG Control panel "Blue" PowerMaster toggled up.  Pulled LOW.
const byte PIN_IN_PANEL_BROWN_OFF      = 68;  // Change D25 to A14 = D68. Input: LEG Control panel "Brown" PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_BROWN_ON       = 69;  // Change D23 to A15 = D69. Input: LEG Control panel "Brown" PowerMaster toggled up.  Pulled LOW.
const byte PIN_IN_PANEL_RED_OFF        = 64;  // Change D33 to A10 = D64. Input: LEG Control panel "Red" PowerMaster toggled down.  Pulled LOW.
const byte PIN_IN_PANEL_RED_ON         = 65;  // Change D31 to A11 = D65. Input: LEG Control panel "Red" PowerMaster toggled up.  Pulled LOW.

// *** MISC HARDWARE CONNECTION PINS ***
const byte PIN_IO_HALT                 =  9;  // Output: Pull low to tell A-LEG to issue Legacy Emergency Stop FE FF FF
                                              // Input: Check if pulled low
const byte PIN_OUT_SPEAKER             =  7;  // Output: Piezo buzzer connects positive here
const byte PIN_IN_WAV_STATUS           = 10;  // Input: LOW when OCC WAV Trigger track is playing, else HIGH.
const byte PIN_OUT_LED                 = 13;  // Built-in LED always pin 13
const byte PIN_IO_FRAM_CS              = 53;  // Output: FRAM  chip select.

#endif
