// PINBALL_AUDIO.H Rev: 01/21/26
// Audio management functions for Tsunami WAV Trigger
// Handles gain control, EEPROM persistence, and track playback

#ifndef PINBALL_AUDIO_H
#define PINBALL_AUDIO_H

#include <Arduino.h>

// Forward declaration for Tsunami class
class Tsunami;

// EEPROM addresses for audio settings (must match Pinball_Diagnostics.h)
const int AUDIO_EEPROM_ADDR_TSUNAMI_GAIN       = 10;
const int AUDIO_EEPROM_ADDR_TSUNAMI_GAIN_VOICE = 11;
const int AUDIO_EEPROM_ADDR_TSUNAMI_GAIN_SFX   = 12;
const int AUDIO_EEPROM_ADDR_TSUNAMI_GAIN_MUSIC = 13;
const int AUDIO_EEPROM_ADDR_TSUNAMI_DUCK_DB    = 14;

// Tsunami hardware constants
// NOTE: Tsunami.h defines TSUNAMI_NUM_OUTPUTS as 8, but we only use 4 outputs
const byte AUDIO_TSUNAMI_NUM_OUTPUTS = 4;

// Gain range constants
const int8_t TSUNAMI_GAIN_DB_DEFAULT = -10;
const int8_t TSUNAMI_GAIN_DB_MIN     = -40;
const int8_t TSUNAMI_GAIN_DB_MAX     =   0;
const int8_t TSUNAMI_CAT_GAIN_MIN    = -20;
const int8_t TSUNAMI_CAT_GAIN_MAX    =  20;

// Audio category constants
const byte AUDIO_CATEGORY_VOICE = 0;
const byte AUDIO_CATEGORY_SFX   = 1;
const byte AUDIO_CATEGORY_MUSIC = 2;

// *** CORE TSUNAMI GAIN FUNCTIONS ***
// Apply master gain to all Tsunami outputs
void audioApplyMasterGain(int8_t gainDb, Tsunami* pTsunami);

// Apply category-specific gain offset to a playing track
void audioApplyTrackGain(unsigned int trackNum, int8_t categoryOffset, int8_t masterGain, Tsunami* pTsunami);

// *** EEPROM PERSISTENCE FUNCTIONS ***
// Save and load master gain
void audioSaveMasterGain(int8_t gainDb);
void audioLoadMasterGain(int8_t* pGainDb);

// Save and load category gains (voice, SFX, music)
void audioSaveCategoryGains(int8_t voiceGain, int8_t sfxGain, int8_t musicGain);
void audioLoadCategoryGains(int8_t* pVoiceGain, int8_t* pSfxGain, int8_t* pMusicGain);

// Save and load ducking level
void audioSaveDucking(int8_t duckingDb);
void audioLoadDucking(int8_t* pDuckingDb);

// *** PLAYBACK FUNCTIONS ***
// Play a track with category-specific gain applied
void audioPlayTrackWithCategory(unsigned int trackNum, byte category,
  int8_t masterGain, int8_t voiceGain, int8_t sfxGain, int8_t musicGain,
  Tsunami* pTsunami);

// Simple play (defaults to SFX category)
void audioPlayTrack(unsigned int trackNum, int8_t masterGain, int8_t sfxGain, Tsunami* pTsunami);

// Stop a specific track
void audioStopTrack(unsigned int trackNum, Tsunami* pTsunami);

// *** UTILITY FUNCTIONS ***
// Pseudo-random number generator for track selection
int audioRandomInt(int maxExclusive);

#endif // PINBALL_AUDIO_H