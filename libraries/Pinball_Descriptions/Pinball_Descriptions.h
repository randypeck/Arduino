// PINBALL_DESCRIPTIONS.H Rev: 01/02/26.
// Strings stored in PROGMEM to save RAM, used for diagnostic display of lamp, switch, coil, and WAV names.

#ifndef PINBALL_DESCRIPTIONS_H
#define PINBALL_DESCRIPTIONS_H

#include <Arduino.h>
#include <avr/pgmspace.h>

// ***************************************
// ***** LAMP DESCRIPTIONS (PROGMEM) *****
// ***************************************
// Format: Max 17 chars; ALL CAPS; combined playfield, cabinet, and head lamps
// Display as: "NN DESCRIPTION" (NN = 2-digit index, space, then description)
// PLAYFIELD LAMPS (indices 0-46), HEAD LAMPS (indices 47-73)

const char diagLampName0[]  PROGMEM = "GI LEFT TOP";
const char diagLampName1[]  PROGMEM = "GI LEFT CTR 1";
const char diagLampName2[]  PROGMEM = "GI LEFT CTR 2";
const char diagLampName3[]  PROGMEM = "GI LEFT BOTTOM";
const char diagLampName4[]  PROGMEM = "GI RIGHT TOP";
const char diagLampName5[]  PROGMEM = "GI RIGHT CTR 1";
const char diagLampName6[]  PROGMEM = "GI RIGHT CTR 2";
const char diagLampName7[]  PROGMEM = "GI RIGHT BOTTOM";
const char diagLampName8[]  PROGMEM = "BUMPER S";
const char diagLampName9[]  PROGMEM = "BUMPER C";
const char diagLampName10[] PROGMEM = "BUMPER R";
const char diagLampName11[] PROGMEM = "BUMPER E";
const char diagLampName12[] PROGMEM = "BUMPER A";
const char diagLampName13[] PROGMEM = "BUMPER M";
const char diagLampName14[] PROGMEM = "BUMPER O";
const char diagLampName15[] PROGMEM = "WHITE 1";
const char diagLampName16[] PROGMEM = "WHITE 2";
const char diagLampName17[] PROGMEM = "WHITE 3";
const char diagLampName18[] PROGMEM = "WHITE 4";
const char diagLampName19[] PROGMEM = "WHITE 5";
const char diagLampName20[] PROGMEM = "WHITE 6";
const char diagLampName21[] PROGMEM = "WHITE 7";
const char diagLampName22[] PROGMEM = "WHITE 8";
const char diagLampName23[] PROGMEM = "WHITE 9";
const char diagLampName24[] PROGMEM = "RED 1";
const char diagLampName25[] PROGMEM = "RED 2";
const char diagLampName26[] PROGMEM = "RED 3";
const char diagLampName27[] PROGMEM = "RED 4";
const char diagLampName28[] PROGMEM = "RED 5";
const char diagLampName29[] PROGMEM = "RED 6";
const char diagLampName30[] PROGMEM = "RED 7";
const char diagLampName31[] PROGMEM = "RED 8";
const char diagLampName32[] PROGMEM = "RED 9";
const char diagLampName33[] PROGMEM = "HAT LEFT TOP";
const char diagLampName34[] PROGMEM = "HAT LEFT BOTTOM";
const char diagLampName35[] PROGMEM = "HAT RIGHT TOP";
const char diagLampName36[] PROGMEM = "HAT RIGHT BOTTOM";
const char diagLampName37[] PROGMEM = "KICKOUT LEFT";
const char diagLampName38[] PROGMEM = "KICKOUT RIGHT";
const char diagLampName39[] PROGMEM = "SPECIAL";
const char diagLampName40[] PROGMEM = "GOBBLE 1";
const char diagLampName41[] PROGMEM = "GOBBLE 2";
const char diagLampName42[] PROGMEM = "GOBBLE 3";
const char diagLampName43[] PROGMEM = "GOBBLE 4";
const char diagLampName44[] PROGMEM = "GOBBLE 5";
const char diagLampName45[] PROGMEM = "SPOT # LEFT";
const char diagLampName46[] PROGMEM = "SPOT # RIGHT";
// HEAD LAMPS (Slave-controlled)
const char diagLampName47[] PROGMEM = "HEAD GI";
const char diagLampName48[] PROGMEM = "TILT";
const char diagLampName49[] PROGMEM = "10K";
const char diagLampName50[] PROGMEM = "20K";
const char diagLampName51[] PROGMEM = "30K";
const char diagLampName52[] PROGMEM = "40K";
const char diagLampName53[] PROGMEM = "50K";
const char diagLampName54[] PROGMEM = "60K";
const char diagLampName55[] PROGMEM = "70K";
const char diagLampName56[] PROGMEM = "80K";
const char diagLampName57[] PROGMEM = "90K";
const char diagLampName58[] PROGMEM = "100K";
const char diagLampName59[] PROGMEM = "200K";
const char diagLampName60[] PROGMEM = "300K";
const char diagLampName61[] PROGMEM = "400K";
const char diagLampName62[] PROGMEM = "500K";
const char diagLampName63[] PROGMEM = "600K";
const char diagLampName64[] PROGMEM = "700K";
const char diagLampName65[] PROGMEM = "800K";
const char diagLampName66[] PROGMEM = "900K";
const char diagLampName67[] PROGMEM = "1M";
const char diagLampName68[] PROGMEM = "2M";
const char diagLampName69[] PROGMEM = "3M";
const char diagLampName70[] PROGMEM = "4M";
const char diagLampName71[] PROGMEM = "5M";
const char diagLampName72[] PROGMEM = "6M";
const char diagLampName73[] PROGMEM = "7M";
const char diagLampName74[] PROGMEM = "8M";
const char diagLampName75[] PROGMEM = "9M";

const char* const diagLampNames[] PROGMEM = {
  diagLampName0, diagLampName1, diagLampName2, diagLampName3, diagLampName4,
  diagLampName5, diagLampName6, diagLampName7, diagLampName8, diagLampName9,
  diagLampName10, diagLampName11, diagLampName12, diagLampName13, diagLampName14,
  diagLampName15, diagLampName16, diagLampName17, diagLampName18, diagLampName19,
  diagLampName20, diagLampName21, diagLampName22, diagLampName23, diagLampName24,
  diagLampName25, diagLampName26, diagLampName27, diagLampName28, diagLampName29,
  diagLampName30, diagLampName31, diagLampName32, diagLampName33, diagLampName34,
  diagLampName35, diagLampName36, diagLampName37, diagLampName38, diagLampName39,
  diagLampName40, diagLampName41, diagLampName42, diagLampName43, diagLampName44,
  diagLampName45, diagLampName46, diagLampName47, diagLampName48, diagLampName49,
  diagLampName50, diagLampName51, diagLampName52, diagLampName53, diagLampName54,
  diagLampName55, diagLampName56, diagLampName57, diagLampName58, diagLampName59,
  diagLampName60, diagLampName61, diagLampName62, diagLampName63, diagLampName64,
  diagLampName65, diagLampName66, diagLampName67, diagLampName68, diagLampName69,
  diagLampName70, diagLampName71, diagLampName72, diagLampName73, diagLampName74,
  diagLampName75
};

const byte NUM_DIAG_LAMPS = 76;

// *****************************************
// ***** SWITCH DESCRIPTIONS (PROGMEM) *****
// *****************************************
// Format: Max 17 chars; ALL CAPS; combined playfield, cabinet, and head switches
// Display as: "NN DESCRIPTION"
// CABINET/PLAYFIELD SWITCHES (indices 0-39), HEAD SWITCHES (indices 40-41)

const char diagSwitchName0[]  PROGMEM = "START BUTTON";
const char diagSwitchName1[]  PROGMEM = "DIAG BACK";
const char diagSwitchName2[]  PROGMEM = "DIAG MINUS";
const char diagSwitchName3[]  PROGMEM = "DIAG PLUS";
const char diagSwitchName4[]  PROGMEM = "DIAG SELECT";
const char diagSwitchName5[]  PROGMEM = "KNOCK OFF";
const char diagSwitchName6[]  PROGMEM = "COIN MECH";
const char diagSwitchName7[]  PROGMEM = "5 BALLS IN TROUGH";
const char diagSwitchName8[]  PROGMEM = "BALL IN LIFT";
const char diagSwitchName9[]  PROGMEM = "TILT BOB";
const char diagSwitchName10[] PROGMEM = "BUMPER S";
const char diagSwitchName11[] PROGMEM = "BUMPER C";
const char diagSwitchName12[] PROGMEM = "BUMPER R";
const char diagSwitchName13[] PROGMEM = "BUMPER E";
const char diagSwitchName14[] PROGMEM = "BUMPER A";
const char diagSwitchName15[] PROGMEM = "BUMPER M";
const char diagSwitchName16[] PROGMEM = "BUMPER O";
const char diagSwitchName17[] PROGMEM = "KICKOUT LEFT";
const char diagSwitchName18[] PROGMEM = "KICKOUT RIGHT";
const char diagSwitchName19[] PROGMEM = "SLINGSHOT LEFT";
const char diagSwitchName20[] PROGMEM = "SLINGSHOT RIGHT";
const char diagSwitchName21[] PROGMEM = "HAT LEFT TOP";
const char diagSwitchName22[] PROGMEM = "HAT LEFT BOT";
const char diagSwitchName23[] PROGMEM = "HAT RIGHT TOP";
const char diagSwitchName24[] PROGMEM = "HAT RIGHT BOT";
const char diagSwitchName25[] PROGMEM = "TARGET LEFT 2";
const char diagSwitchName26[] PROGMEM = "TARGET LEFT 3";
const char diagSwitchName27[] PROGMEM = "TARGET LEFT 4";
const char diagSwitchName28[] PROGMEM = "TARGET LEFT 5";
const char diagSwitchName29[] PROGMEM = "TARGET RIGHT 1";
const char diagSwitchName30[] PROGMEM = "TARGET RIGHT 2";
const char diagSwitchName31[] PROGMEM = "TARGET RIGHT 3";
const char diagSwitchName32[] PROGMEM = "TARGET RIGHT 4";
const char diagSwitchName33[] PROGMEM = "TARGET RIGHT 5";
const char diagSwitchName34[] PROGMEM = "GOBBLE HOLE";
const char diagSwitchName35[] PROGMEM = "DRAIN LEFT";
const char diagSwitchName36[] PROGMEM = "DRAIN CENTER";
const char diagSwitchName37[] PROGMEM = "DRAIN RIGHT";
const char diagSwitchName38[] PROGMEM = "FLIPPER LEFT";
const char diagSwitchName39[] PROGMEM = "FLIPPER RIGHT";
// HEAD SWITCHES (Slave-controlled)
const char diagSwitchName40[] PROGMEM = "CREDIT EMPTY";
const char diagSwitchName41[] PROGMEM = "CREDIT FULL";

const char* const diagSwitchNames[] PROGMEM = {
  diagSwitchName0, diagSwitchName1, diagSwitchName2, diagSwitchName3, diagSwitchName4,
  diagSwitchName5, diagSwitchName6, diagSwitchName7, diagSwitchName8, diagSwitchName9,
  diagSwitchName10, diagSwitchName11, diagSwitchName12, diagSwitchName13, diagSwitchName14,
  diagSwitchName15, diagSwitchName16, diagSwitchName17, diagSwitchName18, diagSwitchName19,
  diagSwitchName20, diagSwitchName21, diagSwitchName22, diagSwitchName23, diagSwitchName24,
  diagSwitchName25, diagSwitchName26, diagSwitchName27, diagSwitchName28, diagSwitchName29,
  diagSwitchName30, diagSwitchName31, diagSwitchName32, diagSwitchName33, diagSwitchName34,
  diagSwitchName35, diagSwitchName36, diagSwitchName37, diagSwitchName38, diagSwitchName39,
  diagSwitchName40, diagSwitchName41
};

const byte NUM_DIAG_SWITCHES = 42;
// *********************************************
// ***** COIL/MOTOR DESCRIPTIONS (PROGMEM) *****
// *********************************************
// Format: Max 17 chars; ALL CAPS; combined cabinet/playfield and head coils/motors
// Display as: "NN DESCRIPTION"
// CABINET COILS (indices 0-13), HEAD COILS (indices 14-20)

const char diagCoilName0[]  PROGMEM = "POP BUMPER";
const char diagCoilName1[]  PROGMEM = "KICKOUT LEFT";
const char diagCoilName2[]  PROGMEM = "KICKOUT RIGHT";
const char diagCoilName3[]  PROGMEM = "SLINGSHOT LEFT";
const char diagCoilName4[]  PROGMEM = "SLINGSHOT RIGHT";
const char diagCoilName5[]  PROGMEM = "FLIPPER LEFT";
const char diagCoilName6[]  PROGMEM = "FLIPPER RIGHT";
const char diagCoilName7[]  PROGMEM = "BALL TRAY REL";
const char diagCoilName8[]  PROGMEM = "SELECTION UNIT";
const char diagCoilName9[]  PROGMEM = "RELAY RESET";
const char diagCoilName10[] PROGMEM = "BALL TROUGH POST";
const char diagCoilName11[] PROGMEM = "SHAKER MOTOR";
const char diagCoilName12[] PROGMEM = "KNOCKER";
const char diagCoilName13[] PROGMEM = "SCORE MOTOR";
// HEAD COILS (Slave-controlled)
const char diagCoilName14[] PROGMEM = "CREDIT ADD";
const char diagCoilName15[] PROGMEM = "CREDIT DEDUCT";
const char diagCoilName16[] PROGMEM = "10K UNIT";
const char diagCoilName17[] PROGMEM = "10K BELL";
const char diagCoilName18[] PROGMEM = "100K BELL";
const char diagCoilName19[] PROGMEM = "SELECT BELL";
const char diagCoilName20[] PROGMEM = "LAMP SCORE";
const char diagCoilName21[] PROGMEM = "LAMP GI/TILT";

const char* const diagCoilNames[] PROGMEM = {
  diagCoilName0, diagCoilName1, diagCoilName2, diagCoilName3, diagCoilName4,
  diagCoilName5, diagCoilName6, diagCoilName7, diagCoilName8, diagCoilName9,
  diagCoilName10, diagCoilName11, diagCoilName12, diagCoilName13, diagCoilName14,
  diagCoilName15, diagCoilName16, diagCoilName17, diagCoilName18, diagCoilName19,
  diagCoilName20, diagCoilName21
};

const byte NUM_DIAG_COILS = 22;

// ************************************************************************************
// ***** AUDIO TRACK DESCRIPTIONS (PROGMEM) *****
// ************************************************************************************
// Format: Max 15 chars; ALL CAPS (to fit with "NNNN " prefix = 20 chars total)
// Display as: "NNNN DESCRIPTION" where NNNN is 4-digit track number 0001..4095

const char diagAudioName0[]  PROGMEM = "GAME START";
const char diagAudioName1[]  PROGMEM = "PLAYER UP";
const char diagAudioName2[]  PROGMEM = "BALL SAVED 1";
const char diagAudioName3[]  PROGMEM = "BALL SAVED 2";
const char diagAudioName4[]  PROGMEM = "BAD PLAY 1";
const char diagAudioName5[]  PROGMEM = "BAD PLAY 2";
const char diagAudioName6[]  PROGMEM = "COASTER CLIMB";
const char diagAudioName7[]  PROGMEM = "COASTER DROP";
const char diagAudioName8[]  PROGMEM = "SCREAM";
const char diagAudioName9[]  PROGMEM = "GOBBLE";
const char diagAudioName10[] PROGMEM = "MUSIC MAIN";
const char diagAudioName11[] PROGMEM = "MUSIC MULTI";
const char diagAudioName12[] PROGMEM = "MUSIC ATTRACT";

const char* const diagAudioNames[] PROGMEM = {
  diagAudioName0, diagAudioName1, diagAudioName2, diagAudioName3, diagAudioName4,
  diagAudioName5, diagAudioName6, diagAudioName7, diagAudioName8, diagAudioName9,
  diagAudioName10, diagAudioName11, diagAudioName12
};

const byte NUM_DIAG_AUDIO = 13;

// ************************************************************************************
// ***** HELPER FUNCTION TO RETRIEVE PROGMEM STRINGS *****
// ************************************************************************************

// Helper: copy a PROGMEM string from a PROGMEM array of pointers into a RAM buffer.
// This safely handles the double indirection required for arrays of const char* PROGMEM.
// Truncates to (t_destSize - 1) characters and null-terminates.
void diagCopyProgmemString(const char* const* t_table, byte t_index, 
                           char* t_dest, byte t_destSize) {
  // Get the pointer to the nth string (itself stored in PROGMEM)
  const char* pStr = (const char*)pgm_read_word(&t_table[t_index]);
  
  // Copy the string from PROGMEM into RAM buffer
  byte i = 0;
  for (; i < (byte)(t_destSize - 1); i++) {
    char c = pgm_read_byte(pStr + i);
    if (c == '\0') {
      break;
    }
    t_dest[i] = c;
  }
  t_dest[i] = '\0';
}

#endif