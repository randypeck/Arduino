// PINBALL_DESCRIPTIONS.H Rev: 01/19/26.
// Strings stored in PROGMEM to save RAM, used for diagnostic display of lamp, switch, coil, and WAV names.

#ifndef PINBALL_DESCRIPTIONS_H
#define PINBALL_DESCRIPTIONS_H

#include <Arduino.h>
#include <avr/pgmspace.h>

// Name arrays used for diagnostic displays
const char* const settingsCategoryNames[] PROGMEM = {
  "Game Settings",
  "Original Replays",
  "Enhanced Replays"
};

const char* const gameSettingNames[] PROGMEM = {
  "Music Theme",
  "Ball Save Time",
  "Mode 1: Bumper",
  "Mode 2: Roll-A-Ball",
  "Mode 3: Gobble",
  "Mode 4: Reserved",
  "Mode 5: Reserved",
  "Mode 6: Reserved"
};

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
// Format: Two lines per track for LCD display
// Line 1: Track number + type + category (max 20 chars)
// Line 2: Short description (max 20 chars)
// Tracks indexed 0-298; actual track numbers in string

// *** LINE 1: Track number + type + category ***
const char diagAudioL1_0[]   PROGMEM = "0101 COM DIAG";
const char diagAudioL1_1[]   PROGMEM = "0102 COM DIAG";
const char diagAudioL1_2[]   PROGMEM = "0103 SFX DIAG";
const char diagAudioL1_3[]   PROGMEM = "0104 SFX DIAG";
const char diagAudioL1_4[]   PROGMEM = "0105 SFX DIAG";
const char diagAudioL1_5[]   PROGMEM = "0111 COM DIAG";
const char diagAudioL1_6[]   PROGMEM = "0112 COM DIAG";
const char diagAudioL1_7[]   PROGMEM = "0113 COM DIAG";
const char diagAudioL1_8[]   PROGMEM = "0114 COM DIAG";
const char diagAudioL1_9[]   PROGMEM = "0115 COM DIAG";
const char diagAudioL1_10[]  PROGMEM = "0116 COM DIAG";
const char diagAudioL1_11[]  PROGMEM = "0117 COM DIAG";
const char diagAudioL1_12[]  PROGMEM = "0201 COM TILT WARN";
const char diagAudioL1_13[]  PROGMEM = "0202 COM TILT WARN";
const char diagAudioL1_14[]  PROGMEM = "0203 COM TILT WARN";
const char diagAudioL1_15[]  PROGMEM = "0204 COM TILT WARN";
const char diagAudioL1_16[]  PROGMEM = "0205 COM TILT WARN";
const char diagAudioL1_17[]  PROGMEM = "0211 SFX TILT";
const char diagAudioL1_18[]  PROGMEM = "0212 COM TILT";
const char diagAudioL1_19[]  PROGMEM = "0213 COM TILT";
const char diagAudioL1_20[]  PROGMEM = "0214 COM TILT";
const char diagAudioL1_21[]  PROGMEM = "0215 COM TILT";
const char diagAudioL1_22[]  PROGMEM = "0216 COM TILT";
const char diagAudioL1_23[]  PROGMEM = "0301 COM BALL MISS";
const char diagAudioL1_24[]  PROGMEM = "0302 COM BALL MISS";
const char diagAudioL1_25[]  PROGMEM = "0303 COM BALL MISS";
const char diagAudioL1_26[]  PROGMEM = "0304 COM BALL MISS";
const char diagAudioL1_27[]  PROGMEM = "0311 SFX START REJ";
const char diagAudioL1_28[]  PROGMEM = "0312 COM START REJ";
const char diagAudioL1_29[]  PROGMEM = "0313 COM START REJ";
const char diagAudioL1_30[]  PROGMEM = "0314 COM START REJ";
const char diagAudioL1_31[]  PROGMEM = "0315 COM START REJ";
const char diagAudioL1_32[]  PROGMEM = "0316 COM START REJ";
const char diagAudioL1_33[]  PROGMEM = "0317 COM START REJ";
const char diagAudioL1_34[]  PROGMEM = "0318 COM START REJ";
const char diagAudioL1_35[]  PROGMEM = "0319 COM START REJ";
const char diagAudioL1_36[]  PROGMEM = "0320 COM START REJ";
const char diagAudioL1_37[]  PROGMEM = "0321 COM START REJ";
const char diagAudioL1_38[]  PROGMEM = "0322 COM START REJ";
const char diagAudioL1_39[]  PROGMEM = "0323 COM START REJ";
const char diagAudioL1_40[]  PROGMEM = "0324 COM START REJ";
const char diagAudioL1_41[]  PROGMEM = "0325 COM START REJ";
const char diagAudioL1_42[]  PROGMEM = "0326 COM START REJ";
const char diagAudioL1_43[]  PROGMEM = "0327 COM START REJ";
const char diagAudioL1_44[]  PROGMEM = "0328 COM START REJ";
const char diagAudioL1_45[]  PROGMEM = "0329 COM START REJ";
const char diagAudioL1_46[]  PROGMEM = "0330 COM START REJ";
const char diagAudioL1_47[]  PROGMEM = "0351 COM START";
const char diagAudioL1_48[]  PROGMEM = "0352 COM START";
const char diagAudioL1_49[]  PROGMEM = "0353 COM START";
const char diagAudioL1_50[]  PROGMEM = "0354 COM START";
const char diagAudioL1_51[]  PROGMEM = "0355 COM START";
const char diagAudioL1_52[]  PROGMEM = "0356 COM START";
const char diagAudioL1_53[]  PROGMEM = "0357 COM START";
const char diagAudioL1_54[]  PROGMEM = "0401 SFX START";
const char diagAudioL1_55[]  PROGMEM = "0402 COM START";
const char diagAudioL1_56[]  PROGMEM = "0403 SFX START";
const char diagAudioL1_57[]  PROGMEM = "0404 SFX START";
const char diagAudioL1_58[]  PROGMEM = "0451 COM PLAYER";
const char diagAudioL1_59[]  PROGMEM = "0452 COM PLAYER";
const char diagAudioL1_60[]  PROGMEM = "0453 COM PLAYER";
const char diagAudioL1_61[]  PROGMEM = "0454 COM PLAYER";
const char diagAudioL1_62[]  PROGMEM = "0461 COM BALL";
const char diagAudioL1_63[]  PROGMEM = "0462 COM BALL";
const char diagAudioL1_64[]  PROGMEM = "0463 COM BALL";
const char diagAudioL1_65[]  PROGMEM = "0464 COM BALL";
const char diagAudioL1_66[]  PROGMEM = "0465 COM BALL";
const char diagAudioL1_67[]  PROGMEM = "0511 COM BALL1 COM";
const char diagAudioL1_68[]  PROGMEM = "0512 COM BALL1 COM";
const char diagAudioL1_69[]  PROGMEM = "0513 COM BALL1 COM";
const char diagAudioL1_70[]  PROGMEM = "0514 COM BALL1 COM";
const char diagAudioL1_71[]  PROGMEM = "0515 COM BALL1 COM";
const char diagAudioL1_72[]  PROGMEM = "0516 COM BALL1 COM";
const char diagAudioL1_73[]  PROGMEM = "0517 COM BALL1 COM";
const char diagAudioL1_74[]  PROGMEM = "0518 COM BALL1 COM";
const char diagAudioL1_75[]  PROGMEM = "0519 COM BALL1 COM";
const char diagAudioL1_76[]  PROGMEM = "0531 COM BALL5 COM";
const char diagAudioL1_77[]  PROGMEM = "0532 COM BALL5 COM";
const char diagAudioL1_78[]  PROGMEM = "0533 COM BALL5 COM";
const char diagAudioL1_79[]  PROGMEM = "0534 COM BALL5 COM";
const char diagAudioL1_80[]  PROGMEM = "0535 COM BALL5 COM";
const char diagAudioL1_81[]  PROGMEM = "0536 COM BALL5 COM";
const char diagAudioL1_82[]  PROGMEM = "0537 COM BALL5 COM";
const char diagAudioL1_83[]  PROGMEM = "0538 COM BALL5 COM";
const char diagAudioL1_84[]  PROGMEM = "0539 COM BALL5 COM";
const char diagAudioL1_85[]  PROGMEM = "0540 COM BALL5 COM";
const char diagAudioL1_86[]  PROGMEM = "0551 COM GAME OVER";
const char diagAudioL1_87[]  PROGMEM = "0552 COM GAME OVER";
const char diagAudioL1_88[]  PROGMEM = "0553 COM GAME OVER";
const char diagAudioL1_89[]  PROGMEM = "0554 COM GAME OVER";
const char diagAudioL1_90[]  PROGMEM = "0555 COM GAME OVER";
const char diagAudioL1_91[]  PROGMEM = "0556 COM GAME OVER";
const char diagAudioL1_92[]  PROGMEM = "0557 COM GAME OVER";
const char diagAudioL1_93[]  PROGMEM = "0558 COM GAME OVER";
const char diagAudioL1_94[]  PROGMEM = "0559 COM GAME OVER";
const char diagAudioL1_95[]  PROGMEM = "0560 COM GAME OVER";
const char diagAudioL1_96[]  PROGMEM = "0561 COM GAME OVER";
const char diagAudioL1_97[]  PROGMEM = "0562 COM GAME OVER";
const char diagAudioL1_98[]  PROGMEM = "0563 COM GAME OVER";
const char diagAudioL1_99[]  PROGMEM = "0564 COM GAME OVER";
const char diagAudioL1_100[] PROGMEM = "0565 COM GAME OVER";
const char diagAudioL1_101[] PROGMEM = "0566 COM GAME OVER";
const char diagAudioL1_102[] PROGMEM = "0567 COM GAME OVER";
const char diagAudioL1_103[] PROGMEM = "0568 COM GAME OVER";
const char diagAudioL1_104[] PROGMEM = "0569 COM GAME OVER";
const char diagAudioL1_105[] PROGMEM = "0570 COM GAME OVER";
const char diagAudioL1_106[] PROGMEM = "0571 COM GAME OVER";
const char diagAudioL1_107[] PROGMEM = "0572 COM GAME OVER";
const char diagAudioL1_108[] PROGMEM = "0573 COM GAME OVER";
const char diagAudioL1_109[] PROGMEM = "0574 COM GAME OVER";
const char diagAudioL1_110[] PROGMEM = "0575 COM GAME OVER";
const char diagAudioL1_111[] PROGMEM = "0576 COM GAME OVER";
const char diagAudioL1_112[] PROGMEM = "0577 COM GAME OVER";
const char diagAudioL1_113[] PROGMEM = "0611 COM SHOOT";
const char diagAudioL1_114[] PROGMEM = "0612 COM SHOOT";
const char diagAudioL1_115[] PROGMEM = "0613 COM SHOOT";
const char diagAudioL1_116[] PROGMEM = "0614 COM SHOOT";
const char diagAudioL1_117[] PROGMEM = "0615 COM SHOOT";
const char diagAudioL1_118[] PROGMEM = "0616 COM SHOOT";
const char diagAudioL1_119[] PROGMEM = "0617 COM SHOOT";
const char diagAudioL1_120[] PROGMEM = "0618 COM SHOOT";
const char diagAudioL1_121[] PROGMEM = "0619 COM SHOOT";
const char diagAudioL1_122[] PROGMEM = "0620 COM SHOOT";
const char diagAudioL1_123[] PROGMEM = "0631 COM BALL SAVED";
const char diagAudioL1_124[] PROGMEM = "0632 COM BALL SAVED";
const char diagAudioL1_125[] PROGMEM = "0633 COM BALL SAVED";
const char diagAudioL1_126[] PROGMEM = "0634 COM BALL SAVED";
const char diagAudioL1_127[] PROGMEM = "0635 COM BALL SAVED";
const char diagAudioL1_128[] PROGMEM = "0636 COM BALL SAVED";
const char diagAudioL1_129[] PROGMEM = "0641 COM BALL SAVED";
const char diagAudioL1_130[] PROGMEM = "0651 COM SAVED URG";
const char diagAudioL1_131[] PROGMEM = "0652 COM SAVED URG";
const char diagAudioL1_132[] PROGMEM = "0653 COM SAVED URG";
const char diagAudioL1_133[] PROGMEM = "0654 COM SAVED URG";
const char diagAudioL1_134[] PROGMEM = "0655 COM SAVED URG";
const char diagAudioL1_135[] PROGMEM = "0656 COM SAVED URG";
const char diagAudioL1_136[] PROGMEM = "0657 COM SAVED URG";
const char diagAudioL1_137[] PROGMEM = "0658 COM SAVED URG";
const char diagAudioL1_138[] PROGMEM = "0659 COM SAVED URG";
const char diagAudioL1_139[] PROGMEM = "0660 COM SAVED URG";
const char diagAudioL1_140[] PROGMEM = "0661 COM SAVED URG";
const char diagAudioL1_141[] PROGMEM = "0662 COM SAVED URG";
const char diagAudioL1_142[] PROGMEM = "0671 COM MULTIBALL";
const char diagAudioL1_143[] PROGMEM = "0672 COM MULTIBALL";
const char diagAudioL1_144[] PROGMEM = "0673 COM MULTIBALL";
const char diagAudioL1_145[] PROGMEM = "0674 COM MULTIBALL";
const char diagAudioL1_146[] PROGMEM = "0675 COM MULTIBALL";
const char diagAudioL1_147[] PROGMEM = "0701 COM COMPLIMENT";
const char diagAudioL1_148[] PROGMEM = "0702 COM COMPLIMENT";
const char diagAudioL1_149[] PROGMEM = "0703 COM COMPLIMENT";
const char diagAudioL1_150[] PROGMEM = "0704 COM COMPLIMENT";
const char diagAudioL1_151[] PROGMEM = "0705 COM COMPLIMENT";
const char diagAudioL1_152[] PROGMEM = "0706 COM COMPLIMENT";
const char diagAudioL1_153[] PROGMEM = "0707 COM COMPLIMENT";
const char diagAudioL1_154[] PROGMEM = "0708 COM COMPLIMENT";
const char diagAudioL1_155[] PROGMEM = "0709 COM COMPLIMENT";
const char diagAudioL1_156[] PROGMEM = "0710 COM COMPLIMENT";
const char diagAudioL1_157[] PROGMEM = "0711 COM COMPLIMENT";
const char diagAudioL1_158[] PROGMEM = "0712 COM COMPLIMENT";
const char diagAudioL1_159[] PROGMEM = "0713 COM COMPLIMENT";
const char diagAudioL1_160[] PROGMEM = "0714 COM COMPLIMENT";
const char diagAudioL1_161[] PROGMEM = "0721 COM DRAIN";
const char diagAudioL1_162[] PROGMEM = "0722 COM DRAIN";
const char diagAudioL1_163[] PROGMEM = "0723 COM DRAIN";
const char diagAudioL1_164[] PROGMEM = "0724 COM DRAIN";
const char diagAudioL1_165[] PROGMEM = "0725 COM DRAIN";
const char diagAudioL1_166[] PROGMEM = "0726 COM DRAIN";
const char diagAudioL1_167[] PROGMEM = "0727 COM DRAIN";
const char diagAudioL1_168[] PROGMEM = "0728 COM DRAIN";
const char diagAudioL1_169[] PROGMEM = "0729 COM DRAIN";
const char diagAudioL1_170[] PROGMEM = "0730 COM DRAIN";
const char diagAudioL1_171[] PROGMEM = "0731 COM DRAIN";
const char diagAudioL1_172[] PROGMEM = "0732 COM DRAIN";
const char diagAudioL1_173[] PROGMEM = "0733 COM DRAIN";
const char diagAudioL1_174[] PROGMEM = "0734 COM DRAIN";
const char diagAudioL1_175[] PROGMEM = "0735 COM DRAIN";
const char diagAudioL1_176[] PROGMEM = "0736 COM DRAIN";
const char diagAudioL1_177[] PROGMEM = "0737 COM DRAIN";
const char diagAudioL1_178[] PROGMEM = "0738 COM DRAIN";
const char diagAudioL1_179[] PROGMEM = "0739 COM DRAIN";
const char diagAudioL1_180[] PROGMEM = "0740 COM DRAIN";
const char diagAudioL1_181[] PROGMEM = "0741 COM DRAIN";
const char diagAudioL1_182[] PROGMEM = "0742 COM DRAIN";
const char diagAudioL1_183[] PROGMEM = "0743 COM DRAIN";
const char diagAudioL1_184[] PROGMEM = "0744 COM DRAIN";
const char diagAudioL1_185[] PROGMEM = "0745 COM DRAIN";
const char diagAudioL1_186[] PROGMEM = "0746 COM DRAIN";
const char diagAudioL1_187[] PROGMEM = "0747 COM DRAIN";
const char diagAudioL1_188[] PROGMEM = "0748 COM DRAIN";
const char diagAudioL1_189[] PROGMEM = "0811 COM AWARD";
const char diagAudioL1_190[] PROGMEM = "0812 COM AWARD";
const char diagAudioL1_191[] PROGMEM = "0821 COM AWARD";
const char diagAudioL1_192[] PROGMEM = "0822 COM AWARD";
const char diagAudioL1_193[] PROGMEM = "0823 COM AWARD";
const char diagAudioL1_194[] PROGMEM = "0824 COM AWARD";
const char diagAudioL1_195[] PROGMEM = "0831 COM AWARD";
const char diagAudioL1_196[] PROGMEM = "0841 COM AWARD";
const char diagAudioL1_197[] PROGMEM = "0842 COM AWARD";
const char diagAudioL1_198[] PROGMEM = "1001 SFX MODE";
const char diagAudioL1_199[] PROGMEM = "1002 COM MODE";
const char diagAudioL1_200[] PROGMEM = "1003 COM MODE";
const char diagAudioL1_201[] PROGMEM = "1004 SFX MODE";
const char diagAudioL1_202[] PROGMEM = "1005 COM MODE";
const char diagAudioL1_203[] PROGMEM = "1006 SFX MODE";
const char diagAudioL1_204[] PROGMEM = "1101 COM BUMPER";
const char diagAudioL1_205[] PROGMEM = "1111 COM BUMPER";
const char diagAudioL1_206[] PROGMEM = "1121 SFX BUMPER HIT";
const char diagAudioL1_207[] PROGMEM = "1122 SFX BUMPER HIT";
const char diagAudioL1_208[] PROGMEM = "1123 SFX BUMPER HIT";
const char diagAudioL1_209[] PROGMEM = "1124 SFX BUMPER HIT";
const char diagAudioL1_210[] PROGMEM = "1125 SFX BUMPER HIT";
const char diagAudioL1_211[] PROGMEM = "1126 SFX BUMPER HIT";
const char diagAudioL1_212[] PROGMEM = "1127 SFX BUMPER HIT";
const char diagAudioL1_213[] PROGMEM = "1128 SFX BUMPER HIT";
const char diagAudioL1_214[] PROGMEM = "1129 SFX BUMPER HIT";
const char diagAudioL1_215[] PROGMEM = "1130 SFX BUMPER HIT";
const char diagAudioL1_216[] PROGMEM = "1131 SFX BUMPER HIT";
const char diagAudioL1_217[] PROGMEM = "1132 SFX BUMPER HIT";
const char diagAudioL1_218[] PROGMEM = "1133 SFX BUMPER HIT";
const char diagAudioL1_219[] PROGMEM = "1141 SFX BUMPER MIS";
const char diagAudioL1_220[] PROGMEM = "1142 SFX BUMPER MIS";
const char diagAudioL1_221[] PROGMEM = "1143 SFX BUMPER MIS";
const char diagAudioL1_222[] PROGMEM = "1144 SFX BUMPER MIS";
const char diagAudioL1_223[] PROGMEM = "1145 SFX BUMPER MIS";
const char diagAudioL1_224[] PROGMEM = "1146 SFX BUMPER MIS";
const char diagAudioL1_225[] PROGMEM = "1147 SFX BUMPER MIS";
const char diagAudioL1_226[] PROGMEM = "1148 SFX BUMPER MIS";
const char diagAudioL1_227[] PROGMEM = "1197 SFX BUMPER ACH";
const char diagAudioL1_228[] PROGMEM = "1201 COM RAB";
const char diagAudioL1_229[] PROGMEM = "1211 COM RAB";
const char diagAudioL1_230[] PROGMEM = "1221 SFX RAB HIT";
const char diagAudioL1_231[] PROGMEM = "1222 SFX RAB HIT";
const char diagAudioL1_232[] PROGMEM = "1223 SFX RAB HIT";
const char diagAudioL1_233[] PROGMEM = "1224 SFX RAB HIT";
const char diagAudioL1_234[] PROGMEM = "1225 SFX RAB HIT";
const char diagAudioL1_235[] PROGMEM = "1241 SFX RAB MISS";
const char diagAudioL1_236[] PROGMEM = "1242 SFX RAB MISS";
const char diagAudioL1_237[] PROGMEM = "1243 SFX RAB MISS";
const char diagAudioL1_238[] PROGMEM = "1244 SFX RAB MISS";
const char diagAudioL1_239[] PROGMEM = "1245 SFX RAB MISS";
const char diagAudioL1_240[] PROGMEM = "1246 SFX RAB MISS";
const char diagAudioL1_241[] PROGMEM = "1247 SFX RAB MISS";
const char diagAudioL1_242[] PROGMEM = "1248 SFX RAB MISS";
const char diagAudioL1_243[] PROGMEM = "1249 SFX RAB MISS";
const char diagAudioL1_244[] PROGMEM = "1250 SFX RAB MISS";
const char diagAudioL1_245[] PROGMEM = "1251 SFX RAB MISS";
const char diagAudioL1_246[] PROGMEM = "1252 SFX RAB MISS";
const char diagAudioL1_247[] PROGMEM = "1253 SFX RAB MISS";
const char diagAudioL1_248[] PROGMEM = "1254 SFX RAB MISS";
const char diagAudioL1_249[] PROGMEM = "1297 SFX RAB ACH";
const char diagAudioL1_250[] PROGMEM = "1301 COM GOBBLE";
const char diagAudioL1_251[] PROGMEM = "1311 COM GOBBLE";
const char diagAudioL1_252[] PROGMEM = "1321 SFX GOBBLE HIT";
const char diagAudioL1_253[] PROGMEM = "1341 SFX GOBBLE MIS";
const char diagAudioL1_254[] PROGMEM = "1342 SFX GOBBLE MIS";
const char diagAudioL1_255[] PROGMEM = "1343 SFX GOBBLE MIS";
const char diagAudioL1_256[] PROGMEM = "1344 SFX GOBBLE MIS";
const char diagAudioL1_257[] PROGMEM = "1345 SFX GOBBLE MIS";
const char diagAudioL1_258[] PROGMEM = "1346 SFX GOBBLE MIS";
const char diagAudioL1_259[] PROGMEM = "1347 SFX GOBBLE MIS";
const char diagAudioL1_260[] PROGMEM = "1348 SFX GOBBLE MIS";
const char diagAudioL1_261[] PROGMEM = "1397 SFX GOBBLE ACH";
const char diagAudioL1_262[] PROGMEM = "2001 MUS CIRCUS";
const char diagAudioL1_263[] PROGMEM = "2002 MUS CIRCUS";
const char diagAudioL1_264[] PROGMEM = "2003 MUS CIRCUS";
const char diagAudioL1_265[] PROGMEM = "2004 MUS CIRCUS";
const char diagAudioL1_266[] PROGMEM = "2005 MUS CIRCUS";
const char diagAudioL1_267[] PROGMEM = "2006 MUS CIRCUS";
const char diagAudioL1_268[] PROGMEM = "2007 MUS CIRCUS";
const char diagAudioL1_269[] PROGMEM = "2008 MUS CIRCUS";
const char diagAudioL1_270[] PROGMEM = "2009 MUS CIRCUS";
const char diagAudioL1_271[] PROGMEM = "2010 MUS CIRCUS";
const char diagAudioL1_272[] PROGMEM = "2011 MUS CIRCUS";
const char diagAudioL1_273[] PROGMEM = "2012 MUS CIRCUS";
const char diagAudioL1_274[] PROGMEM = "2013 MUS CIRCUS";
const char diagAudioL1_275[] PROGMEM = "2014 MUS CIRCUS";
const char diagAudioL1_276[] PROGMEM = "2015 MUS CIRCUS";
const char diagAudioL1_277[] PROGMEM = "2016 MUS CIRCUS";
const char diagAudioL1_278[] PROGMEM = "2017 MUS CIRCUS";
const char diagAudioL1_279[] PROGMEM = "2018 MUS CIRCUS";
const char diagAudioL1_280[] PROGMEM = "2019 MUS CIRCUS";
const char diagAudioL1_281[] PROGMEM = "2051 MUS SURF";
const char diagAudioL1_282[] PROGMEM = "2052 MUS SURF";
const char diagAudioL1_283[] PROGMEM = "2053 MUS SURF";
const char diagAudioL1_284[] PROGMEM = "2054 MUS SURF";
const char diagAudioL1_285[] PROGMEM = "2055 MUS SURF";
const char diagAudioL1_286[] PROGMEM = "2056 MUS SURF";
const char diagAudioL1_287[] PROGMEM = "2057 MUS SURF";
const char diagAudioL1_288[] PROGMEM = "2058 MUS SURF";
const char diagAudioL1_289[] PROGMEM = "2059 MUS SURF";
const char diagAudioL1_290[] PROGMEM = "2060 MUS SURF";
const char diagAudioL1_291[] PROGMEM = "2061 MUS SURF";
const char diagAudioL1_292[] PROGMEM = "2062 MUS SURF";
const char diagAudioL1_293[] PROGMEM = "2063 MUS SURF";
const char diagAudioL1_294[] PROGMEM = "2064 MUS SURF";
const char diagAudioL1_295[] PROGMEM = "2065 MUS SURF";
const char diagAudioL1_296[] PROGMEM = "2066 MUS SURF";
const char diagAudioL1_297[] PROGMEM = "2067 MUS SURF";
const char diagAudioL1_298[] PROGMEM = "2068 MUS SURF";

// *** LINE 2: Short descriptions ***
const char diagAudioL2_0[]   PROGMEM = "Entering diagnostics";
const char diagAudioL2_1[]   PROGMEM = "Exiting diagnostics";
const char diagAudioL2_2[]   PROGMEM = "Continuous tone";
const char diagAudioL2_3[]   PROGMEM = "Switch close 1000hz";
const char diagAudioL2_4[]   PROGMEM = "Switch open 500hz";
const char diagAudioL2_5[]   PROGMEM = "Lamps";
const char diagAudioL2_6[]   PROGMEM = "Switches";
const char diagAudioL2_7[]   PROGMEM = "Coils";
const char diagAudioL2_8[]   PROGMEM = "Motors";
const char diagAudioL2_9[]   PROGMEM = "Music";
const char diagAudioL2_10[]  PROGMEM = "Sound Effects";
const char diagAudioL2_11[]  PROGMEM = "Comments";
const char diagAudioL2_12[]  PROGMEM = "Shake it again...";
const char diagAudioL2_13[]  PROGMEM = "Easy there Hercules";
const char diagAudioL2_14[]  PROGMEM = "Whoa there King Kong";
const char diagAudioL2_15[]  PROGMEM = "Careful";
const char diagAudioL2_16[]  PROGMEM = "Watch it";
const char diagAudioL2_17[]  PROGMEM = "Buzzer";
const char diagAudioL2_18[]  PROGMEM = "Nice goin Hercules";
const char diagAudioL2_19[]  PROGMEM = "Congrats King Kong";
const char diagAudioL2_20[]  PROGMEM = "Ya broke it palooka";
const char diagAudioL2_21[]  PROGMEM = "Aint bumper cars";
const char diagAudioL2_22[]  PROGMEM = "Tilt! Roughhousing";
const char diagAudioL2_23[]  PROGMEM = "Ball missing";
const char diagAudioL2_24[]  PROGMEM = "Press lift rod";
const char diagAudioL2_25[]  PROGMEM = "Shoot and drain";
const char diagAudioL2_26[]  PROGMEM = "Still missing";
const char diagAudioL2_27[]  PROGMEM = "Aoooga horn";
const char diagAudioL2_28[]  PROGMEM = "No free admission";
const char diagAudioL2_29[]  PROGMEM = "Ask yer mommy";
const char diagAudioL2_30[]  PROGMEM = "I told yas SCRAM";
const char diagAudioL2_31[]  PROGMEM = "Aint gettin in";
const char diagAudioL2_32[]  PROGMEM = "Got a cigarette";
const char diagAudioL2_33[]  PROGMEM = "Couch cushions";
const char diagAudioL2_34[]  PROGMEM = "No flippers";
const char diagAudioL2_35[]  PROGMEM = "Drop a coin";
const char diagAudioL2_36[]  PROGMEM = "Santee Claus";
const char diagAudioL2_37[]  PROGMEM = "No ticket no tumble";
const char diagAudioL2_38[]  PROGMEM = "Pressin coins";
const char diagAudioL2_39[]  PROGMEM = "Losin money";
const char diagAudioL2_40[]  PROGMEM = "Sell a balloon";
const char diagAudioL2_41[]  PROGMEM = "After ya pay";
const char diagAudioL2_42[]  PROGMEM = "Soup kitchen";
const char diagAudioL2_43[]  PROGMEM = "Aint a charity";
const char diagAudioL2_44[]  PROGMEM = "Sneak side gate";
const char diagAudioL2_45[]  PROGMEM = "Somethin shiney";
const char diagAudioL2_46[]  PROGMEM = "No coin no joyride";
const char diagAudioL2_47[]  PROGMEM = "Okay kid youre in";
const char diagAudioL2_48[]  PROGMEM = "Wilder ride chicken";
const char diagAudioL2_49[]  PROGMEM = "More friends";
const char diagAudioL2_50[]  PROGMEM = "Second guest";
const char diagAudioL2_51[]  PROGMEM = "Third guest";
const char diagAudioL2_52[]  PROGMEM = "Fourth guest";
const char diagAudioL2_53[]  PROGMEM = "Parks full";
const char diagAudioL2_54[]  PROGMEM = "Scream";
const char diagAudioL2_55[]  PROGMEM = "Hey Gang Screamo";
const char diagAudioL2_56[]  PROGMEM = "Lift loop 120s";
const char diagAudioL2_57[]  PROGMEM = "First drop screams";
const char diagAudioL2_58[]  PROGMEM = "Guest 1";
const char diagAudioL2_59[]  PROGMEM = "Guest 2";
const char diagAudioL2_60[]  PROGMEM = "Guest 3";
const char diagAudioL2_61[]  PROGMEM = "Guest 4";
const char diagAudioL2_62[]  PROGMEM = "Ball 1";
const char diagAudioL2_63[]  PROGMEM = "Ball 2";
const char diagAudioL2_64[]  PROGMEM = "Ball 3";
const char diagAudioL2_65[]  PROGMEM = "Ball 4";
const char diagAudioL2_66[]  PROGMEM = "Ball 5";
const char diagAudioL2_67[]  PROGMEM = "Climb aboard";
const char diagAudioL2_68[]  PROGMEM = "Explore the park";
const char diagAudioL2_69[]  PROGMEM = "Fire away";
const char diagAudioL2_70[]  PROGMEM = "Launch it";
const char diagAudioL2_71[]  PROGMEM = "Lets find a ride";
const char diagAudioL2_72[]  PROGMEM = "Show em how";
const char diagAudioL2_73[]  PROGMEM = "Youre up";
const char diagAudioL2_74[]  PROGMEM = "Your turn to ride";
const char diagAudioL2_75[]  PROGMEM = "Lets ride";
const char diagAudioL2_76[]  PROGMEM = "Dont embarrass";
const char diagAudioL2_77[]  PROGMEM = "Now or never";
const char diagAudioL2_78[]  PROGMEM = "Last ride of day";
const char diagAudioL2_79[]  PROGMEM = "Make it flashy";
const char diagAudioL2_80[]  PROGMEM = "No pressure";
const char diagAudioL2_81[]  PROGMEM = "This ball decides";
const char diagAudioL2_82[]  PROGMEM = "This is it";
const char diagAudioL2_83[]  PROGMEM = "Last ticket";
const char diagAudioL2_84[]  PROGMEM = "Last ball";
const char diagAudioL2_85[]  PROGMEM = "Make it count";
const char diagAudioL2_86[]  PROGMEM = "End of ride";
const char diagAudioL2_87[]  PROGMEM = "Enjoyed now scram";
const char diagAudioL2_88[]  PROGMEM = "Curtains for you";
const char diagAudioL2_89[]  PROGMEM = "Game over dude";
const char diagAudioL2_90[]  PROGMEM = "No more rides";
const char diagAudioL2_91[]  PROGMEM = "No more tickets";
const char diagAudioL2_92[]  PROGMEM = "Out of balls";
const char diagAudioL2_93[]  PROGMEM = "Randy warned me";
const char diagAudioL2_94[]  PROGMEM = "Move along";
const char diagAudioL2_95[]  PROGMEM = "Closed come again";
const char diagAudioL2_96[]  PROGMEM = "Step right down";
const char diagAudioL2_97[]  PROGMEM = "All she wrote";
const char diagAudioL2_98[]  PROGMEM = "Fat lady sung";
const char diagAudioL2_99[]  PROGMEM = "Closed pal";
const char diagAudioL2_100[] PROGMEM = "Out of running";
const char diagAudioL2_101[] PROGMEM = "Shows over hotshot";
const char diagAudioL2_102[] PROGMEM = "Out of tickets";
const char diagAudioL2_103[] PROGMEM = "End of line";
const char diagAudioL2_104[] PROGMEM = "Park Closed Exit";
const char diagAudioL2_105[] PROGMEM = "Parked disembark";
const char diagAudioL2_106[] PROGMEM = "Hit the brakes";
const char diagAudioL2_107[] PROGMEM = "End of midway";
const char diagAudioL2_108[] PROGMEM = "Gave it a whirl";
const char diagAudioL2_109[] PROGMEM = "Park Now Closed";
const char diagAudioL2_110[] PROGMEM = "Tickets punched";
const char diagAudioL2_111[] PROGMEM = "Safety Bar";
const char diagAudioL2_112[] PROGMEM = "Better luck";
const char diagAudioL2_113[] PROGMEM = "Press lift rod";
const char diagAudioL2_114[] PROGMEM = "More fun shoot";
const char diagAudioL2_115[] PROGMEM = "Recommend shooting";
const char diagAudioL2_116[] PROGMEM = "Dont be afraid";
const char diagAudioL2_117[] PROGMEM = "For Gods sake";
const char diagAudioL2_118[] PROGMEM = "No dilly dallying";
const char diagAudioL2_119[] PROGMEM = "Good time shoot";
const char diagAudioL2_120[] PROGMEM = "Excellent time";
const char diagAudioL2_121[] PROGMEM = "Shoot the Ball";
const char diagAudioL2_122[] PROGMEM = "Excellent (annoyed)";
const char diagAudioL2_123[] PROGMEM = "Launch again";
const char diagAudioL2_124[] PROGMEM = "Keep shooting";
const char diagAudioL2_125[] PROGMEM = "Shoot again";
const char diagAudioL2_126[] PROGMEM = "Ride again";
const char diagAudioL2_127[] PROGMEM = "Shoot again";
const char diagAudioL2_128[] PROGMEM = "Back on ride";
const char diagAudioL2_129[] PROGMEM = "Ball back mode end";
const char diagAudioL2_130[] PROGMEM = "Send it";
const char diagAudioL2_131[] PROGMEM = "Fire at will";
const char diagAudioL2_132[] PROGMEM = "Fire away";
const char diagAudioL2_133[] PROGMEM = "Quick shoot it";
const char diagAudioL2_134[] PROGMEM = "Shoot it now";
const char diagAudioL2_135[] PROGMEM = "Hurry shoot";
const char diagAudioL2_136[] PROGMEM = "Quick shoot again";
const char diagAudioL2_137[] PROGMEM = "Shoot another";
const char diagAudioL2_138[] PROGMEM = "Keep shooting";
const char diagAudioL2_139[] PROGMEM = "Shoot again";
const char diagAudioL2_140[] PROGMEM = "For Gods sake";
const char diagAudioL2_141[] PROGMEM = "Quick another";
const char diagAudioL2_142[] PROGMEM = "First ball locked";
const char diagAudioL2_143[] PROGMEM = "Second ball locked";
const char diagAudioL2_144[] PROGMEM = "Multiball";
const char diagAudioL2_145[] PROGMEM = "All rides running";
const char diagAudioL2_146[] PROGMEM = "Bumpers double";
const char diagAudioL2_147[] PROGMEM = "Good shot";
const char diagAudioL2_148[] PROGMEM = "Awesome great work";
const char diagAudioL2_149[] PROGMEM = "Great job";
const char diagAudioL2_150[] PROGMEM = "Great shot";
const char diagAudioL2_151[] PROGMEM = "Youve done it";
const char diagAudioL2_152[] PROGMEM = "Mission accomplished";
const char diagAudioL2_153[] PROGMEM = "Youve done it";
const char diagAudioL2_154[] PROGMEM = "You did it";
const char diagAudioL2_155[] PROGMEM = "Amazing";
const char diagAudioL2_156[] PROGMEM = "Great job";
const char diagAudioL2_157[] PROGMEM = "Nicely done";
const char diagAudioL2_158[] PROGMEM = "Well done";
const char diagAudioL2_159[] PROGMEM = "Great work";
const char diagAudioL2_160[] PROGMEM = "Great shot";
const char diagAudioL2_161[] PROGMEM = "Forgot flippers";
const char diagAudioL2_162[] PROGMEM = "Gravity answered";
const char diagAudioL2_163[] PROGMEM = "Eye doctor";
const char diagAudioL2_164[] PROGMEM = "Pretend I didnt";
const char diagAudioL2_165[] PROGMEM = "Gravity wins";
const char diagAudioL2_166[] PROGMEM = "Sorry for loss";
const char diagAudioL2_167[] PROGMEM = "That was quick";
const char diagAudioL2_168[] PROGMEM = "Eyes open";
const char diagAudioL2_169[] PROGMEM = "Nice drain";
const char diagAudioL2_170[] PROGMEM = "Didnt see coming";
const char diagAudioL2_171[] PROGMEM = "Thats humiliating";
const char diagAudioL2_172[] PROGMEM = "So long ball";
const char diagAudioL2_173[] PROGMEM = "Thats gotta hurt";
const char diagAudioL2_174[] PROGMEM = "Saddest thing";
const char diagAudioL2_175[] PROGMEM = "Yikes";
const char diagAudioL2_176[] PROGMEM = "Hurt to watch";
const char diagAudioL2_177[] PROGMEM = "Just terrible";
const char diagAudioL2_178[] PROGMEM = "Home sweet home";
const char diagAudioL2_179[] PROGMEM = "Whoopsie daisy";
const char diagAudioL2_180[] PROGMEM = "Get outta there";
const char diagAudioL2_181[] PROGMEM = "Hey wrong way";
const char diagAudioL2_182[] PROGMEM = "Kid not that way";
const char diagAudioL2_183[] PROGMEM = "Leaving so soon";
const char diagAudioL2_184[] PROGMEM = "No no no";
const char diagAudioL2_185[] PROGMEM = "Not that way";
const char diagAudioL2_186[] PROGMEM = "Not the exit";
const char diagAudioL2_187[] PROGMEM = "Oh boy";
const char diagAudioL2_188[] PROGMEM = "Where ya goin kid";
const char diagAudioL2_189[] PROGMEM = "Special is lit";
const char diagAudioL2_190[] PROGMEM = "Reeee-plaaay";
const char diagAudioL2_191[] PROGMEM = "Three in a row";
const char diagAudioL2_192[] PROGMEM = "Four corners";
const char diagAudioL2_193[] PROGMEM = "One two three";
const char diagAudioL2_194[] PROGMEM = "Five Gobble Hole";
const char diagAudioL2_195[] PROGMEM = "Spelled SCREAMO";
const char diagAudioL2_196[] PROGMEM = "EXTRA BALL";
const char diagAudioL2_197[] PROGMEM = "Send it";
const char diagAudioL2_198[] PROGMEM = "School Bell";
const char diagAudioL2_199[] PROGMEM = "Jackpot";
const char diagAudioL2_200[] PROGMEM = "Ten seconds left";
const char diagAudioL2_201[] PROGMEM = "Timer countdown";
const char diagAudioL2_202[] PROGMEM = "Time";
const char diagAudioL2_203[] PROGMEM = "Factory whistle";
const char diagAudioL2_204[] PROGMEM = "Bumper Cars intro";
const char diagAudioL2_205[] PROGMEM = "Keep smashing";
const char diagAudioL2_206[] PROGMEM = "Car honk";
const char diagAudioL2_207[] PROGMEM = "La cucaracha";
const char diagAudioL2_208[] PROGMEM = "Car honk 2x";
const char diagAudioL2_209[] PROGMEM = "Car honk 2x";
const char diagAudioL2_210[] PROGMEM = "Car honk 2x";
const char diagAudioL2_211[] PROGMEM = "Car honk 2x";
const char diagAudioL2_212[] PROGMEM = "Car honk 2x";
const char diagAudioL2_213[] PROGMEM = "Car honk rubber";
const char diagAudioL2_214[] PROGMEM = "Car honk rubber";
const char diagAudioL2_215[] PROGMEM = "Car honk aoooga";
const char diagAudioL2_216[] PROGMEM = "Diesel train";
const char diagAudioL2_217[] PROGMEM = "Car honk";
const char diagAudioL2_218[] PROGMEM = "Truck air horn";
const char diagAudioL2_219[] PROGMEM = "Car crash";
const char diagAudioL2_220[] PROGMEM = "Car crash";
const char diagAudioL2_221[] PROGMEM = "Car crash";
const char diagAudioL2_222[] PROGMEM = "Car crash";
const char diagAudioL2_223[] PROGMEM = "Car crash";
const char diagAudioL2_224[] PROGMEM = "Cat screech";
const char diagAudioL2_225[] PROGMEM = "Car crash";
const char diagAudioL2_226[] PROGMEM = "Car crash";
const char diagAudioL2_227[] PROGMEM = "Bell ding ding";
const char diagAudioL2_228[] PROGMEM = "Roll-A-Ball intro";
const char diagAudioL2_229[] PROGMEM = "Keep rolling hats";
const char diagAudioL2_230[] PROGMEM = "Bowling strike";
const char diagAudioL2_231[] PROGMEM = "Bowling strike";
const char diagAudioL2_232[] PROGMEM = "Bowling strike";
const char diagAudioL2_233[] PROGMEM = "Bowling strike";
const char diagAudioL2_234[] PROGMEM = "Bowling strike";
const char diagAudioL2_235[] PROGMEM = "Glass break";
const char diagAudioL2_236[] PROGMEM = "Glass break";
const char diagAudioL2_237[] PROGMEM = "Glass break";
const char diagAudioL2_238[] PROGMEM = "Glass break";
const char diagAudioL2_239[] PROGMEM = "Glass break";
const char diagAudioL2_240[] PROGMEM = "Glass break";
const char diagAudioL2_241[] PROGMEM = "Glass break";
const char diagAudioL2_242[] PROGMEM = "Glass break";
const char diagAudioL2_243[] PROGMEM = "Glass break";
const char diagAudioL2_244[] PROGMEM = "Glass break";
const char diagAudioL2_245[] PROGMEM = "Glass break";
const char diagAudioL2_246[] PROGMEM = "Glass break";
const char diagAudioL2_247[] PROGMEM = "Glass break";
const char diagAudioL2_248[] PROGMEM = "Goat sound";
const char diagAudioL2_249[] PROGMEM = "Ta da";
const char diagAudioL2_250[] PROGMEM = "Shooting Gallery";
const char diagAudioL2_251[] PROGMEM = "Keep shooting";
const char diagAudioL2_252[] PROGMEM = "Slide whistle";
const char diagAudioL2_253[] PROGMEM = "Ricochet";
const char diagAudioL2_254[] PROGMEM = "Ricochet";
const char diagAudioL2_255[] PROGMEM = "Ricochet";
const char diagAudioL2_256[] PROGMEM = "Ricochet";
const char diagAudioL2_257[] PROGMEM = "Ricochet";
const char diagAudioL2_258[] PROGMEM = "Ricochet";
const char diagAudioL2_259[] PROGMEM = "Ricochet";
const char diagAudioL2_260[] PROGMEM = "Ricochet";
const char diagAudioL2_261[] PROGMEM = "Applause";
const char diagAudioL2_262[] PROGMEM = "Barnum Bailey";
const char diagAudioL2_263[] PROGMEM = "Rensaz Race March";
const char diagAudioL2_264[] PROGMEM = "Twelfth Street Rag";
const char diagAudioL2_265[] PROGMEM = "Chariot Ben Hur";
const char diagAudioL2_266[] PROGMEM = "Organ Grinder";
const char diagAudioL2_267[] PROGMEM = "Hands Across Sea";
const char diagAudioL2_268[] PROGMEM = "Field Artillery";
const char diagAudioL2_269[] PROGMEM = "You Flew Over";
const char diagAudioL2_270[] PROGMEM = "Double Eagle";
const char diagAudioL2_271[] PROGMEM = "Ragtime Cowboy";
const char diagAudioL2_272[] PROGMEM = "Billboard March";
const char diagAudioL2_273[] PROGMEM = "El Capitan";
const char diagAudioL2_274[] PROGMEM = "Smiles";
const char diagAudioL2_275[] PROGMEM = "Spirit St Louis";
const char diagAudioL2_276[] PROGMEM = "The Free Lance";
const char diagAudioL2_277[] PROGMEM = "The Roxy March";
const char diagAudioL2_278[] PROGMEM = "Stars Stripes";
const char diagAudioL2_279[] PROGMEM = "Washington Post";
const char diagAudioL2_280[] PROGMEM = "Bombasto";
const char diagAudioL2_281[] PROGMEM = "Miserlou";
const char diagAudioL2_282[] PROGMEM = "Bumble Bee Stomp";
const char diagAudioL2_283[] PROGMEM = "Wipe Out";
const char diagAudioL2_284[] PROGMEM = "Banzai Washout";
const char diagAudioL2_285[] PROGMEM = "Hava Nagila";
const char diagAudioL2_286[] PROGMEM = "Sabre Dance";
const char diagAudioL2_287[] PROGMEM = "Malaguena";
const char diagAudioL2_288[] PROGMEM = "Wildfire";
const char diagAudioL2_289[] PROGMEM = "The Wedge";
const char diagAudioL2_290[] PROGMEM = "Exotic";
const char diagAudioL2_291[] PROGMEM = "The Victor";
const char diagAudioL2_292[] PROGMEM = "Mr Eliminator";
const char diagAudioL2_293[] PROGMEM = "Night Rider";
const char diagAudioL2_294[] PROGMEM = "The Jester";
const char diagAudioL2_295[] PROGMEM = "Pressure";
const char diagAudioL2_296[] PROGMEM = "Shootin Beavers";
const char diagAudioL2_297[] PROGMEM = "Riders in Sky";
const char diagAudioL2_298[] PROGMEM = "Bumble Bee Boogie";

// *** POINTER ARRAYS ***
const char* const diagAudioLine1[] PROGMEM = {
  diagAudioL1_0,   diagAudioL1_1,   diagAudioL1_2,   diagAudioL1_3,   diagAudioL1_4,
  diagAudioL1_5,   diagAudioL1_6,   diagAudioL1_7,   diagAudioL1_8,   diagAudioL1_9,
  diagAudioL1_10,  diagAudioL1_11,  diagAudioL1_12,  diagAudioL1_13,  diagAudioL1_14,
  diagAudioL1_15,  diagAudioL1_16,  diagAudioL1_17,  diagAudioL1_18,  diagAudioL1_19,
  diagAudioL1_20,  diagAudioL1_21,  diagAudioL1_22,  diagAudioL1_23,  diagAudioL1_24,
  diagAudioL1_25,  diagAudioL1_26,  diagAudioL1_27,  diagAudioL1_28,  diagAudioL1_29,
  diagAudioL1_30,  diagAudioL1_31,  diagAudioL1_32,  diagAudioL1_33,  diagAudioL1_34,
  diagAudioL1_35,  diagAudioL1_36,  diagAudioL1_37,  diagAudioL1_38,  diagAudioL1_39,
  diagAudioL1_40,  diagAudioL1_41,  diagAudioL1_42,  diagAudioL1_43,  diagAudioL1_44,
  diagAudioL1_45,  diagAudioL1_46,  diagAudioL1_47,  diagAudioL1_48,  diagAudioL1_49,
  diagAudioL1_50,  diagAudioL1_51,  diagAudioL1_52,  diagAudioL1_53,  diagAudioL1_54,
  diagAudioL1_55,  diagAudioL1_56,  diagAudioL1_57,  diagAudioL1_58,  diagAudioL1_59,
  diagAudioL1_60,  diagAudioL1_61,  diagAudioL1_62,  diagAudioL1_63,  diagAudioL1_64,
  diagAudioL1_65,  diagAudioL1_66,  diagAudioL1_67,  diagAudioL1_68,  diagAudioL1_69,
  diagAudioL1_70,  diagAudioL1_71,  diagAudioL1_72,  diagAudioL1_73,  diagAudioL1_74,
  diagAudioL1_75,  diagAudioL1_76,  diagAudioL1_77,  diagAudioL1_78,  diagAudioL1_79,
  diagAudioL1_80,  diagAudioL1_81,  diagAudioL1_82,  diagAudioL1_83,  diagAudioL1_84,
  diagAudioL1_85,  diagAudioL1_86,  diagAudioL1_87,  diagAudioL1_88,  diagAudioL1_89,
  diagAudioL1_90,  diagAudioL1_91,  diagAudioL1_92,  diagAudioL1_93,  diagAudioL1_94,
  diagAudioL1_95,  diagAudioL1_96,  diagAudioL1_97,  diagAudioL1_98,  diagAudioL1_99,
  diagAudioL1_100, diagAudioL1_101, diagAudioL1_102, diagAudioL1_103, diagAudioL1_104,
  diagAudioL1_105, diagAudioL1_106, diagAudioL1_107, diagAudioL1_108, diagAudioL1_109,
  diagAudioL1_110, diagAudioL1_111, diagAudioL1_112, diagAudioL1_113, diagAudioL1_114,
  diagAudioL1_115, diagAudioL1_116, diagAudioL1_117, diagAudioL1_118, diagAudioL1_119,
  diagAudioL1_120, diagAudioL1_121, diagAudioL1_122, diagAudioL1_123, diagAudioL1_124,
  diagAudioL1_125, diagAudioL1_126, diagAudioL1_127, diagAudioL1_128, diagAudioL1_129,
  diagAudioL1_130, diagAudioL1_131, diagAudioL1_132, diagAudioL1_133, diagAudioL1_134,
  diagAudioL1_135, diagAudioL1_136, diagAudioL1_137, diagAudioL1_138, diagAudioL1_139,
  diagAudioL1_140, diagAudioL1_141, diagAudioL1_142, diagAudioL1_143, diagAudioL1_144,
  diagAudioL1_145, diagAudioL1_146, diagAudioL1_147, diagAudioL1_148, diagAudioL1_149,
  diagAudioL1_150, diagAudioL1_151, diagAudioL1_152, diagAudioL1_153, diagAudioL1_154,
  diagAudioL1_155, diagAudioL1_156, diagAudioL1_157, diagAudioL1_158, diagAudioL1_159,
  diagAudioL1_160, diagAudioL1_161, diagAudioL1_162, diagAudioL1_163, diagAudioL1_164,
  diagAudioL1_165, diagAudioL1_166, diagAudioL1_167, diagAudioL1_168, diagAudioL1_169,
  diagAudioL1_170, diagAudioL1_171, diagAudioL1_172, diagAudioL1_173, diagAudioL1_174,
  diagAudioL1_175, diagAudioL1_176, diagAudioL1_177, diagAudioL1_178, diagAudioL1_179,
  diagAudioL1_180, diagAudioL1_181, diagAudioL1_182, diagAudioL1_183, diagAudioL1_184,
  diagAudioL1_185, diagAudioL1_186, diagAudioL1_187, diagAudioL1_188, diagAudioL1_189,
  diagAudioL1_190, diagAudioL1_191, diagAudioL1_192, diagAudioL1_193, diagAudioL1_194,
  diagAudioL1_195, diagAudioL1_196, diagAudioL1_197, diagAudioL1_198, diagAudioL1_199,
  diagAudioL1_200, diagAudioL1_201, diagAudioL1_202, diagAudioL1_203, diagAudioL1_204,
  diagAudioL1_205, diagAudioL1_206, diagAudioL1_207, diagAudioL1_208, diagAudioL1_209,
  diagAudioL1_210, diagAudioL1_211, diagAudioL1_212, diagAudioL1_213, diagAudioL1_214,
  diagAudioL1_215, diagAudioL1_216, diagAudioL1_217, diagAudioL1_218, diagAudioL1_219,
  diagAudioL1_220, diagAudioL1_221, diagAudioL1_222, diagAudioL1_223, diagAudioL1_224,
  diagAudioL1_225, diagAudioL1_226, diagAudioL1_227, diagAudioL1_228, diagAudioL1_229,
  diagAudioL1_230, diagAudioL1_231, diagAudioL1_232, diagAudioL1_233, diagAudioL1_234,
  diagAudioL1_235, diagAudioL1_236, diagAudioL1_237, diagAudioL1_238, diagAudioL1_239,
  diagAudioL1_240, diagAudioL1_241, diagAudioL1_242, diagAudioL1_243, diagAudioL1_244,
  diagAudioL1_245, diagAudioL1_246, diagAudioL1_247, diagAudioL1_248, diagAudioL1_249,
  diagAudioL1_250, diagAudioL1_251, diagAudioL1_252, diagAudioL1_253, diagAudioL1_254,
  diagAudioL1_255, diagAudioL1_256, diagAudioL1_257, diagAudioL1_258, diagAudioL1_259,
  diagAudioL1_260, diagAudioL1_261, diagAudioL1_262, diagAudioL1_263, diagAudioL1_264,
  diagAudioL1_265, diagAudioL1_266, diagAudioL1_267, diagAudioL1_268, diagAudioL1_269,
  diagAudioL1_270, diagAudioL1_271, diagAudioL1_272, diagAudioL1_273, diagAudioL1_274,
  diagAudioL1_275, diagAudioL1_276, diagAudioL1_277, diagAudioL1_278, diagAudioL1_279,
  diagAudioL1_280, diagAudioL1_281, diagAudioL1_282, diagAudioL1_283, diagAudioL1_284,
  diagAudioL1_285, diagAudioL1_286, diagAudioL1_287, diagAudioL1_288, diagAudioL1_289,
  diagAudioL1_290, diagAudioL1_291, diagAudioL1_292, diagAudioL1_293, diagAudioL1_294,
  diagAudioL1_295, diagAudioL1_296, diagAudioL1_297, diagAudioL1_298
};

const char* const diagAudioLine2[] PROGMEM = {
  diagAudioL2_0,   diagAudioL2_1,   diagAudioL2_2,   diagAudioL2_3,   diagAudioL2_4,
  diagAudioL2_5,   diagAudioL2_6,   diagAudioL2_7,   diagAudioL2_8,   diagAudioL2_9,
  diagAudioL2_10,  diagAudioL2_11,  diagAudioL2_12,  diagAudioL2_13,  diagAudioL2_14,
  diagAudioL2_15,  diagAudioL2_16,  diagAudioL2_17,  diagAudioL2_18,  diagAudioL2_19,
  diagAudioL2_20,  diagAudioL2_21,  diagAudioL2_22,  diagAudioL2_23,  diagAudioL2_24,
  diagAudioL2_25,  diagAudioL2_26,  diagAudioL2_27,  diagAudioL2_28,  diagAudioL2_29,
  diagAudioL2_30,  diagAudioL2_31,  diagAudioL2_32,  diagAudioL2_33,  diagAudioL2_34,
  diagAudioL2_35,  diagAudioL2_36,  diagAudioL2_37,  diagAudioL2_38,  diagAudioL2_39,
  diagAudioL2_40,  diagAudioL2_41,  diagAudioL2_42,  diagAudioL2_43,  diagAudioL2_44,
  diagAudioL2_45,  diagAudioL2_46,  diagAudioL2_47,  diagAudioL2_48,  diagAudioL2_49,
  diagAudioL2_50,  diagAudioL2_51,  diagAudioL2_52,  diagAudioL2_53,  diagAudioL2_54,
  diagAudioL2_55,  diagAudioL2_56,  diagAudioL2_57,  diagAudioL2_58,  diagAudioL2_59,
  diagAudioL2_60,  diagAudioL2_61,  diagAudioL2_62,  diagAudioL2_63,  diagAudioL2_64,
  diagAudioL2_65,  diagAudioL2_66,  diagAudioL2_67,  diagAudioL2_68,  diagAudioL2_69,
  diagAudioL2_70,  diagAudioL2_71,  diagAudioL2_72,  diagAudioL2_73,  diagAudioL2_74,
  diagAudioL2_75,  diagAudioL2_76,  diagAudioL2_77,  diagAudioL2_78,  diagAudioL2_79,
  diagAudioL2_80,  diagAudioL2_81,  diagAudioL2_82,  diagAudioL2_83,  diagAudioL2_84,
  diagAudioL2_85,  diagAudioL2_86,  diagAudioL2_87,  diagAudioL2_88,  diagAudioL2_89,
  diagAudioL2_90,  diagAudioL2_91,  diagAudioL2_92,  diagAudioL2_93,  diagAudioL2_94,
  diagAudioL2_95,  diagAudioL2_96,  diagAudioL2_97,  diagAudioL2_98,  diagAudioL2_99,
  diagAudioL2_100, diagAudioL2_101, diagAudioL2_102, diagAudioL2_103, diagAudioL2_104,
  diagAudioL2_105, diagAudioL2_106, diagAudioL2_107, diagAudioL2_108, diagAudioL2_109,
  diagAudioL2_110, diagAudioL2_111, diagAudioL2_112, diagAudioL2_113, diagAudioL2_114,
  diagAudioL2_115, diagAudioL2_116, diagAudioL2_117, diagAudioL2_118, diagAudioL2_119,
  diagAudioL2_120, diagAudioL2_121, diagAudioL2_122, diagAudioL2_123, diagAudioL2_124,
  diagAudioL2_125, diagAudioL2_126, diagAudioL2_127, diagAudioL2_128, diagAudioL2_129,
  diagAudioL2_130, diagAudioL2_131, diagAudioL2_132, diagAudioL2_133, diagAudioL2_134,
  diagAudioL2_135, diagAudioL2_136, diagAudioL2_137, diagAudioL2_138, diagAudioL2_139,
  diagAudioL2_140, diagAudioL2_141, diagAudioL2_142, diagAudioL2_143, diagAudioL2_144,
  diagAudioL2_145, diagAudioL2_146, diagAudioL2_147, diagAudioL2_148, diagAudioL2_149,
  diagAudioL2_150, diagAudioL2_151, diagAudioL2_152, diagAudioL2_153, diagAudioL2_154,
  diagAudioL2_155, diagAudioL2_156, diagAudioL2_157, diagAudioL2_158, diagAudioL2_159,
  diagAudioL2_160, diagAudioL2_161, diagAudioL2_162, diagAudioL2_163, diagAudioL2_164,
  diagAudioL2_165, diagAudioL2_166, diagAudioL2_167, diagAudioL2_168, diagAudioL2_169,
  diagAudioL2_170, diagAudioL2_171, diagAudioL2_172, diagAudioL2_173, diagAudioL2_174,
  diagAudioL2_175, diagAudioL2_176, diagAudioL2_177, diagAudioL2_178, diagAudioL2_179,
  diagAudioL2_180, diagAudioL2_181, diagAudioL2_182, diagAudioL2_183, diagAudioL2_184,
  diagAudioL2_185, diagAudioL2_186, diagAudioL2_187, diagAudioL2_188, diagAudioL2_189,
  diagAudioL2_190, diagAudioL2_191, diagAudioL2_192, diagAudioL2_193, diagAudioL2_194,
  diagAudioL2_195, diagAudioL2_196, diagAudioL2_197, diagAudioL2_198, diagAudioL2_199,
  diagAudioL2_200, diagAudioL2_201, diagAudioL2_202, diagAudioL2_203, diagAudioL2_204,
  diagAudioL2_205, diagAudioL2_206, diagAudioL2_207, diagAudioL2_208, diagAudioL2_209,
  diagAudioL2_210, diagAudioL2_211, diagAudioL2_212, diagAudioL2_213, diagAudioL2_214,
  diagAudioL2_215, diagAudioL2_216, diagAudioL2_217, diagAudioL2_218, diagAudioL2_219,
  diagAudioL2_220, diagAudioL2_221, diagAudioL2_222, diagAudioL2_223, diagAudioL2_224,
  diagAudioL2_225, diagAudioL2_226, diagAudioL2_227, diagAudioL2_228, diagAudioL2_229,
  diagAudioL2_230, diagAudioL2_231, diagAudioL2_232, diagAudioL2_233, diagAudioL2_234,
  diagAudioL2_235, diagAudioL2_236, diagAudioL2_237, diagAudioL2_238, diagAudioL2_239,
  diagAudioL2_240, diagAudioL2_241, diagAudioL2_242, diagAudioL2_243, diagAudioL2_244,
  diagAudioL2_245, diagAudioL2_246, diagAudioL2_247, diagAudioL2_248, diagAudioL2_249,
  diagAudioL2_250, diagAudioL2_251, diagAudioL2_252, diagAudioL2_253, diagAudioL2_254,
  diagAudioL2_255, diagAudioL2_256, diagAudioL2_257, diagAudioL2_258, diagAudioL2_259,
  diagAudioL2_260, diagAudioL2_261, diagAudioL2_262, diagAudioL2_263, diagAudioL2_264,
  diagAudioL2_265, diagAudioL2_266, diagAudioL2_267, diagAudioL2_268, diagAudioL2_269,
  diagAudioL2_270, diagAudioL2_271, diagAudioL2_272, diagAudioL2_273, diagAudioL2_274,
  diagAudioL2_275, diagAudioL2_276, diagAudioL2_277, diagAudioL2_278, diagAudioL2_279,
  diagAudioL2_280, diagAudioL2_281, diagAudioL2_282, diagAudioL2_283, diagAudioL2_284,
  diagAudioL2_285, diagAudioL2_286, diagAudioL2_287, diagAudioL2_288, diagAudioL2_289,
  diagAudioL2_290, diagAudioL2_291, diagAudioL2_292, diagAudioL2_293, diagAudioL2_294,
  diagAudioL2_295, diagAudioL2_296, diagAudioL2_297, diagAudioL2_298
};

const unsigned int NUM_DIAG_AUDIO = 299;

// ************************************************************************************
// ***** VALID AUDIO TRACK NUMBERS FOR DIAGNOSTICS *****
// ************************************************************************************
// Track numbers that have corresponding WAV files on the Tsunami SD card.
// Used by diagRunAudio() to skip non-existent track numbers.

const unsigned int diagAudioTrackNums[] PROGMEM = {
  101, 102, 103, 104, 105, 111, 112, 113, 114, 115, 116, 117,
  201, 202, 203, 204, 205, 211, 212, 213, 214, 215, 216,
  301, 302, 303, 304, 311, 312, 313, 314, 315, 316, 317, 318, 319,
  320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330,
  351, 352, 353, 354, 355, 356, 357,
  401, 402, 403, 404,
  451, 452, 453, 454, 461, 462, 463, 464, 465,
  511, 512, 513, 514, 515, 516, 517, 518, 519,
  531, 532, 533, 534, 535, 536, 537, 538, 539, 540,
  551, 552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574, 575, 576, 577,
  611, 612, 613, 614, 615, 616, 617, 618, 619, 620,
  631, 632, 633, 634, 635, 636, 641,
  651, 652, 653, 654, 655, 656, 657, 658, 659, 660, 661, 662,
  671, 672, 673, 674, 675,
  701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 711, 712, 713, 714,
  721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748,
  811, 812, 821, 822, 823, 824, 831, 841, 842,
  1001, 1002, 1003, 1004, 1005, 1006,
  1101, 1111, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1129, 1130, 1131, 1132, 1133,
  1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1197,
  1201, 1211, 1221, 1222, 1223, 1224, 1225,
  1241, 1242, 1243, 1244, 1245, 1246, 1247, 1248, 1249, 1250, 1251, 1252, 1253, 1254, 1297,
  1301, 1311, 1321, 1341, 1342, 1343, 1344, 1345, 1346, 1347, 1348, 1397,
  2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
  2051, 2052, 2053, 2054, 2055, 2056, 2057, 2058, 2059, 2060, 2061, 2062, 2063, 2064, 2065, 2066, 2067, 2068
};
const unsigned int NUM_DIAG_AUDIO_TRACKS = sizeof(diagAudioTrackNums) / sizeof(diagAudioTrackNums[0]);

#endif
