// PINBALL_AUDIO.H Rev: 01/26/26
// Audio management functions for Tsunami WAV Trigger
// Handles gain control, EEPROM persistence, and track playback

#ifndef PINBALL_AUDIO_H
#define PINBALL_AUDIO_H

#include <Arduino.h>
#include <Tsunami.h>
#include <Pinball_Consts.h>        // For EEPROM addresses
#include <Pinball_Audio_Tracks.h>  // For music track arrays and counts

// Forward declaration for Tsunami class
// class Tsunami;   *************************** NOT NEEDED CONFIRM

// Tsunami hardware constants
// NOTE: Tsunami.h defines TSUNAMI_NUM_OUTPUTS as 8, but we only use 4 outputs
const byte AUDIO_TSUNAMI_NUM_OUTPUTS = 4;

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

void audioLoadThemePreference(byte* primaryTheme, byte* lastCircusIdx, byte* lastSurfIdx);
void audioSaveThemePreference(byte primaryTheme, byte lastCircusIdx, byte lastSurfIdx);
bool audioStartPrimaryMusic(byte primaryTheme, byte* lastIdx, Tsunami* tsunami, int8_t musicGainDb, int8_t masterGainDb);

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