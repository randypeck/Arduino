// PINBALL_DIAGNOSTICS.H Rev: 01/21/26.
// Handles diagnostic test suites and EEPROM settings management for Screamo pinball
// Simple procedural interface - pass hardware pointers to functions as needed

#ifndef PINBALL_DIAGNOSTICS_H
#define PINBALL_DIAGNOSTICS_H

#include <Arduino.h>

// Forward declarations for hardware classes
class Pinball_LCD;
class Pinball_Centipede;
class Tsunami;
class Pinball_Message;

// EEPROM address constants (from Screamo_Master.ino)
const int EEPROM_ADDR_LAST_SCORE              =  0;
const int EEPROM_ADDR_TSUNAMI_GAIN            = 10;
const int EEPROM_ADDR_TSUNAMI_GAIN_VOICE      = 11;
const int EEPROM_ADDR_TSUNAMI_GAIN_SFX        = 12;
const int EEPROM_ADDR_TSUNAMI_GAIN_MUSIC      = 13;
const int EEPROM_ADDR_TSUNAMI_DUCK_DB         = 14;
const int EEPROM_ADDR_THEME                   = 20;
const int EEPROM_ADDR_LAST_CIRCUS_SONG_PLAYED = 21;
const int EEPROM_ADDR_LAST_SURF_SONG_PLAYED   = 22;
const int EEPROM_ADDR_LAST_MODE_PLAYED        = 25;
const int EEPROM_ADDR_BALL_SAVE_TIME          = 30;
const int EEPROM_ADDR_MODE_1_TIME             = 31;
const int EEPROM_ADDR_MODE_2_TIME             = 32;
const int EEPROM_ADDR_MODE_3_TIME             = 33;
const int EEPROM_ADDR_MODE_4_TIME             = 34;
const int EEPROM_ADDR_MODE_5_TIME             = 35;
const int EEPROM_ADDR_MODE_6_TIME             = 36;
const int EEPROM_ADDR_ORIGINAL_REPLAY_1       = 40;
const int EEPROM_ADDR_ORIGINAL_REPLAY_2       = 42;
const int EEPROM_ADDR_ORIGINAL_REPLAY_3       = 44;
const int EEPROM_ADDR_ORIGINAL_REPLAY_4       = 46;
const int EEPROM_ADDR_ORIGINAL_REPLAY_5       = 48;
const int EEPROM_ADDR_ENHANCED_REPLAY_1       = 50;
const int EEPROM_ADDR_ENHANCED_REPLAY_2       = 52;
const int EEPROM_ADDR_ENHANCED_REPLAY_3       = 54;
const int EEPROM_ADDR_ENHANCED_REPLAY_4       = 56;
const int EEPROM_ADDR_ENHANCED_REPLAY_5       = 58;

// *** EEPROM READ/WRITE FUNCTIONS ***
// Simple helper functions for reading/writing game settings to EEPROM

byte diagReadThemeFromEEPROM();
void diagWriteThemeToEEPROM(byte theme);

byte diagReadBallSaveTimeFromEEPROM();
void diagWriteBallSaveTimeToEEPROM(byte seconds);

byte diagReadModeTimeFromEEPROM(byte modeNum);
void diagWriteModeTimeToEEPROM(byte modeNum, byte seconds);

unsigned int diagReadReplayScoreFromEEPROM(bool enhanced, byte replayNum);
void diagWriteReplayScoreToEEPROM(bool enhanced, byte replayNum, unsigned int score);

// *** DIAGNOSTIC SUITE HANDLERS ***
// Each function runs a complete diagnostic suite, returning when user exits.
// Pass pointers to hardware objects needed by each suite.

void diagRunVolume(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Tsunami* pTsunami,
  int8_t* pTsunamiGainDb, int8_t* pVoiceGainDb, int8_t* pSfxGainDb,
  int8_t* pMusicGainDb, int8_t* pDuckingDb,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew);

void diagRunLamps(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Pinball_Message* pMsg,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew);

void diagRunSwitches(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Tsunami* pTsunami,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew);

void diagRunCoils(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Pinball_Message* pMsg,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew);

void diagRunAudio(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Tsunami* pTsunami,
  int8_t masterGainDb, int8_t voiceGainDb, int8_t sfxGainDb, int8_t musicGainDb,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew);

void diagRunSettings(Pinball_LCD* pLCD, Pinball_Centipede* pShift,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew);

// ****************************************
// ***** REMOVED: Audio functions now in Pinball_Audio.h/.cpp
// ****************************************
// The following function declarations have been REMOVED and moved to Pinball_Audio.h:
// - diagAdjustMasterGain()
// - diagAdjustMasterGainQuiet()
// - diagApplyMasterGain() -> audioApplyMasterGain()
// - diagApplyTrackGain() -> audioApplyTrackGain()
// - diagSaveMasterGain() -> audioSaveMasterGain()
// - diagLoadMasterGain() -> audioLoadMasterGain()
// - diagSaveCategoryGains() -> audioSaveCategoryGains()
// - diagLoadCategoryGains() -> audioLoadCategoryGains()
// - diagSaveDucking() -> audioSaveDucking()
// - diagLoadDucking() -> audioLoadDucking()
// - diagPlayTrackWithCategory() -> audioPlayTrackWithCategory()
//
// Use #include <Pinball_Audio.h> instead

#endif // PINBALL_DIAGNOSTICS_H
