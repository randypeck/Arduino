// PINBALL_AUDIO.CPP Rev: 01/26/26
// Implementation of audio management functions

#include "Pinball_Audio.h"
#include <EEPROM.h>

// ****************************************
// ***** CORE TSUNAMI GAIN FUNCTIONS ******
// ****************************************

void audioApplyMasterGain(int8_t gainDb, Tsunami* pTsunami) {
  if (pTsunami == nullptr) {
    return;
  }
  int gain = (int)gainDb;
  for (int out = 0; out < AUDIO_TSUNAMI_NUM_OUTPUTS; out++) {
    pTsunami->masterGain(out, gain);
  }
}

void audioApplyTrackGain(unsigned int trackNum, int8_t categoryOffset, int8_t masterGain, Tsunami* pTsunami) {
  if (pTsunami == nullptr || trackNum == 0) {
    return;
  }
  // Calculate total gain: master + category offset
  int totalGain = (int)masterGain + (int)categoryOffset;
  // Clamp to valid range
  if (totalGain < -70) totalGain = -70;
  if (totalGain > 10) totalGain = 10;
  // Apply to this specific track
  pTsunami->trackGain((int)trackNum, totalGain);
}

// ****************************************
// ***** EEPROM PERSISTENCE FUNCTIONS *****
// ****************************************

void audioSaveMasterGain(int8_t gainDb) {
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN, (byte)gainDb);
}

void audioLoadMasterGain(int8_t* pGainDb) {
  if (pGainDb == nullptr) {
    return;
  }

  byte raw = EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN);
  int8_t val = (int8_t)raw;

  if (val < TSUNAMI_GAIN_DB_MIN || val > TSUNAMI_GAIN_DB_MAX) {
    *pGainDb = TSUNAMI_GAIN_DB_DEFAULT;
    audioSaveMasterGain(TSUNAMI_GAIN_DB_DEFAULT);
  } else {
    *pGainDb = val;
  }
}

void audioSaveCategoryGains(int8_t voiceGain, int8_t sfxGain, int8_t musicGain) {
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN_VOICE, (byte)voiceGain);
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN_SFX, (byte)sfxGain);
  EEPROM.update(EEPROM_ADDR_TSUNAMI_GAIN_MUSIC, (byte)musicGain);
}

void audioLoadCategoryGains(int8_t* pVoiceGain, int8_t* pSfxGain, int8_t* pMusicGain) {
  if (pVoiceGain == nullptr || pSfxGain == nullptr || pMusicGain == nullptr) {
    return;
  }

  int8_t v = (int8_t)EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN_VOICE);
  int8_t s = (int8_t)EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN_SFX);
  int8_t m = (int8_t)EEPROM.read(EEPROM_ADDR_TSUNAMI_GAIN_MUSIC);

  // Validate and use default if out of range
  if (v < TSUNAMI_CAT_GAIN_MIN || v > TSUNAMI_CAT_GAIN_MAX) v = TSUNAMI_CAT_GAIN_DEFAULT;
  if (s < TSUNAMI_CAT_GAIN_MIN || s > TSUNAMI_CAT_GAIN_MAX) s = TSUNAMI_CAT_GAIN_DEFAULT;
  if (m < TSUNAMI_CAT_GAIN_MIN || m > TSUNAMI_CAT_GAIN_MAX) m = TSUNAMI_CAT_GAIN_DEFAULT;

  *pVoiceGain = v;
  *pSfxGain = s;
  *pMusicGain = m;

  // Save back if any were corrected
  audioSaveCategoryGains(v, s, m);
}

void audioSaveDucking(int8_t duckingDb) {
  EEPROM.update(EEPROM_ADDR_TSUNAMI_DUCK_DB, (byte)duckingDb);
}

void audioLoadDucking(int8_t* pDuckingDb) {
  if (pDuckingDb == nullptr) {
    return;
  }

  int8_t val = (int8_t)EEPROM.read(EEPROM_ADDR_TSUNAMI_DUCK_DB);
  if (val < TSUNAMI_GAIN_DB_MIN || val > TSUNAMI_GAIN_DB_MAX) {
    *pDuckingDb = TSUNAMI_DUCKING_DB_DEFAULT;
    audioSaveDucking(TSUNAMI_DUCKING_DB_DEFAULT);
  } else {
    *pDuckingDb = val;
  }
}

// Load music theme preference from EEPROM
void audioLoadThemePreference(byte* primaryTheme, byte* lastCircusIdx, byte* lastSurfIdx) {
  *primaryTheme = EEPROM.read(EEPROM_ADDR_THEME);
  *lastCircusIdx = EEPROM.read(EEPROM_ADDR_LAST_CIRCUS_SONG_PLAYED);
  *lastSurfIdx = EEPROM.read(EEPROM_ADDR_LAST_SURF_SONG_PLAYED);

  // Sanity check (in case EEPROM uninitialized)
  if (*primaryTheme > 1) {
    *primaryTheme = 0;  // Default to Circus
  }
  if (*lastCircusIdx >= NUM_MUS_CIRCUS) {
    *lastCircusIdx = 0;
  }
  if (*lastSurfIdx >= NUM_MUS_SURF) {
    *lastSurfIdx = 0;
  }
}

// Save music theme preference to EEPROM
void audioSaveThemePreference(byte primaryTheme, byte lastCircusIdx, byte lastSurfIdx) {
  EEPROM.update(EEPROM_ADDR_THEME, primaryTheme);
  EEPROM.update(EEPROM_ADDR_LAST_CIRCUS_SONG_PLAYED, lastCircusIdx);
  EEPROM.update(EEPROM_ADDR_LAST_SURF_SONG_PLAYED, lastSurfIdx);
}

// Start a random music track from the primary theme
bool audioStartPrimaryMusic(byte primaryTheme, byte* lastIdx, Tsunami* tsunami, int8_t musicGainDb, int8_t masterGainDb) {
  if (tsunami == nullptr) {
    return false;
  }

  // Select track array and count based on theme
  const AudioMusTrackDef* trackArray;
  byte trackCount;

  if (primaryTheme == 0) {  // Circus
    trackArray = musTracksCircus;
    trackCount = NUM_MUS_CIRCUS;
  } else {  // Surf
    trackArray = musTracksSurf;
    trackCount = NUM_MUS_SURF;
  }

  // Pick next track (simple sequential for now; can randomize later)
  *lastIdx = (*lastIdx + 1) % trackCount;

  unsigned int trackNum = trackArray[*lastIdx].trackNum;

  // Play track
  tsunami->trackPlayPoly((int)trackNum, 0, false);
  audioApplyTrackGain(trackNum, musicGainDb, masterGainDb, tsunami);

  return true;
}

// ****************************************
// ***** PLAYBACK FUNCTIONS ***************
// ****************************************

void audioPlayTrackWithCategory(unsigned int trackNum, byte category,
  int8_t masterGain, int8_t voiceGain, int8_t sfxGain, int8_t musicGain,
  Tsunami* pTsunami) {
  if (pTsunami == nullptr || trackNum == 0) {
    return;
  }

  // Start playback
  pTsunami->trackPlayPoly((int)trackNum, 0, false);

  // Apply category-specific gain offset
  int8_t offset = 0;
  switch (category) {
  case AUDIO_CATEGORY_VOICE: offset = voiceGain; break;
  case AUDIO_CATEGORY_SFX:   offset = sfxGain; break;
  case AUDIO_CATEGORY_MUSIC: offset = musicGain; break;
  }

  audioApplyTrackGain(trackNum, offset, masterGain, pTsunami);
}

void audioPlayTrack(unsigned int trackNum, int8_t masterGain, int8_t sfxGain, Tsunami* pTsunami) {
  // Convenience function - defaults to SFX category
  audioPlayTrackWithCategory(trackNum, AUDIO_CATEGORY_SFX, masterGain, 0, sfxGain, 0, pTsunami);
}

void audioStopTrack(unsigned int trackNum, Tsunami* pTsunami) {
  if (pTsunami == nullptr || trackNum == 0) {
    return;
  }
  pTsunami->trackStop((int)trackNum);
}

// ****************************************
// ***** UTILITY FUNCTIONS ****************
// ****************************************

int audioRandomInt(int maxExclusive) {
  if (maxExclusive <= 0) {
    return 0;
  }
  return (int)(millis() % (unsigned long)maxExclusive);
}