// PINBALL_DIAGNOSTICS.H Rev: 01/21/26.
// Handles diagnostic test suites and EEPROM settings management for Screamo pinball
// Simple procedural interface - pass hardware pointers to functions as needed

#ifndef PINBALL_DIAGNOSTICS_H
#define PINBALL_DIAGNOSTICS_H

#include <Arduino.h>
#include <EEPROM.h>
#include <Pinball_LCD.h>
#include <Pinball_Centipede.h>
#include <Tsunami.h>
#include <Pinball_Message.h>
#include <Pinball_Consts.h>
#include <Pinball_Descriptions.h>
#include <Pinball_Audio.h>  // NEW: Use consolidated audio functions

// Forward declarations for hardware classes
class Pinball_LCD;
class Pinball_Centipede;
class Tsunami;
class Pinball_Message;

// *** EEPROM READ/WRITE FUNCTIONS ***
// Simple helper functions for reading/writing game settings to EEPROM

byte diagReadThemeFromEEPROM();
void diagWriteThemeToEEPROM(byte theme);

byte diagReadBallSaveTimeFromEEPROM();
void diagWriteBallSaveTimeToEEPROM(byte seconds);

byte diagReadGameTimeoutFromEEPROM();
void diagWriteGameTimeoutToEEPROM(byte minutes);

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
