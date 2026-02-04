// PINBALL_DIAGNOSTICS.CPP Rev: 01/21/26.
// Implementation of diagnostic test suites and EEPROM settings management

#include "Pinball_Diagnostics.h"

// External references to arrays defined in Screamo_Master.ino
extern struct LampParmStruct {
  byte pinNum;
  byte groupNum;
  bool stateOn;
} lampParm[];

extern struct SwitchParmStruct {
  byte pinNum;
  byte loopConst;
  byte loopCount;
} switchParm[];

extern struct DeviceParmStruct {
  byte pinNum;
  byte powerInitial;
  byte timeOn;
  byte powerHold;
  int8_t countdown;
  byte queueCount;
} deviceParm[];

extern const byte NUM_LAMPS_MASTER;
extern const byte NUM_SWITCHES_MASTER;
extern const byte NUM_DEVS_MASTER;
extern const byte MOTOR_SHAKER_POWER_MIN;
extern const byte DEV_IDX_MOTOR_SHAKER;
extern const byte DEV_IDX_MOTOR_SCORE;

extern const byte LAMP_IDX_S;
extern const byte LAMP_IDX_O;
extern const byte LAMP_GROUP_GI;
extern const byte LAMP_GROUP_BUMPER;

extern const byte SWITCH_IDX_DIAG_1;
extern const byte SWITCH_IDX_DIAG_4;

extern const char* const settingsCategoryNames[];
extern const char* const gameSettingNames[];

// ****************************************
// ***** EEPROM READ/WRITE FUNCTIONS ******
// ****************************************

byte diagReadThemeFromEEPROM() {
  byte theme = EEPROM.read(EEPROM_ADDR_THEME);
  if (theme > 1) {
    theme = THEME_CIRCUS;
  }
  return theme;
}

void diagWriteThemeToEEPROM(byte theme) {
  if (theme <= 1) {
    EEPROM.update(EEPROM_ADDR_THEME, theme);
  }
}

byte diagReadBallSaveTimeFromEEPROM() {
  byte ballSave = EEPROM.read(EEPROM_ADDR_BALL_SAVE_TIME);
  if (ballSave > 30) {
    ballSave = 10;
  }
  return ballSave;
}

void diagWriteBallSaveTimeToEEPROM(byte seconds) {
  if (seconds <= 30) {
    EEPROM.update(EEPROM_ADDR_BALL_SAVE_TIME, seconds);
  }
}

byte diagReadGameTimeoutFromEEPROM() {
  byte timeout = EEPROM.read(EEPROM_ADDR_GAME_TIMEOUT);
  if (timeout == 0 || timeout > 60) {  // 0 or >60 minutes invalid
    timeout = 5;  // Default 5 minutes
  }
  return timeout;
}

void diagWriteGameTimeoutToEEPROM(byte minutes) {
  if (minutes > 0 && minutes <= 60) {  // Valid range: 1-60 minutes
    EEPROM.update(EEPROM_ADDR_GAME_TIMEOUT, minutes);
  }
}

byte diagReadModeTimeFromEEPROM(byte modeNum) {
  if (modeNum < 1 || modeNum > 6) {
    return 60;
  }
  int addr = EEPROM_ADDR_MODE_1_TIME + (modeNum - 1);
  byte modeTime = EEPROM.read(addr);
  if (modeTime == 0 || modeTime > 250) {
    modeTime = 60;
  }
  return modeTime;
}

void diagWriteModeTimeToEEPROM(byte modeNum, byte seconds) {
  if (modeNum < 1 || modeNum > 6) {
    return;
  }
  if (seconds == 0 || seconds > 250) {
    return;
  }
  int addr = EEPROM_ADDR_MODE_1_TIME + (modeNum - 1);
  EEPROM.update(addr, seconds);
}

unsigned int diagReadReplayScoreFromEEPROM(bool enhanced, byte replayNum) {
  if (replayNum < 1 || replayNum > 5) {
    return 450;
  }

  int baseAddr = enhanced ? EEPROM_ADDR_ENHANCED_REPLAY_1 : EEPROM_ADDR_ORIGINAL_REPLAY_1;
  int addr = baseAddr + ((replayNum - 1) * 2);

  unsigned int score = 0;
  EEPROM.get(addr, score);

  if (score > 999) {
    if (enhanced) {
      score = 450 + (replayNum * 50) + 50;
      if (replayNum == 2) score = 650;
      if (replayNum == 3) score = 750;
      if (replayNum == 4) score = 850;
      if (replayNum == 5) score = 950;
    } else {
      score = 400 + (replayNum * 50) + 50;
      if (replayNum == 2) score = 600;
      if (replayNum == 3) score = 700;
      if (replayNum == 4) score = 800;
      if (replayNum == 5) score = 900;
    }
  }
  return score;
}

void diagWriteReplayScoreToEEPROM(bool enhanced, byte replayNum, unsigned int score) {
  if (replayNum < 1 || replayNum > 5) {
    return;
  }
  if (score > 999) {
    return;
  }

  int baseAddr = enhanced ? EEPROM_ADDR_ENHANCED_REPLAY_1 : EEPROM_ADDR_ORIGINAL_REPLAY_1;
  int addr = baseAddr + ((replayNum - 1) * 2);

  EEPROM.put(addr, score);
}

// ****************************************
// ***** REMOVED: Audio functions now in Pinball_Audio.h/.cpp
// ****************************************
// The following functions have been REMOVED and moved to Pinball_Audio library:
// - diagSaveMasterGain() -> audioSaveMasterGain()
// - diagLoadMasterGain() -> audioLoadMasterGain()
// - diagSaveCategoryGains() -> audioSaveCategoryGains()
// - diagLoadCategoryGains() -> audioLoadCategoryGains()
// - diagSaveDucking() -> audioSaveDucking()
// - diagLoadDucking() -> audioLoadDucking()
// - diagApplyMasterGain() -> audioApplyMasterGain()
// - diagApplyTrackGain() -> audioApplyTrackGain()
// - diagPlayTrackWithCategory() -> audioPlayTrackWithCategory()

// ****************************************
// ***** INTERNAL HELPER FUNCTIONS ********
// ****************************************

static bool diagSwitchClosed(byte switchIdx, unsigned int* pSwitchNew) {
  if (switchIdx >= NUM_SWITCHES_MASTER) {
    return false;
  }
  byte pin = switchParm[switchIdx].pinNum;
  pin = pin - 64;
  byte portIndex = pin / 16;
  byte bitNum = pin % 16;
  return ((pSwitchNew[portIndex] & (1 << bitNum)) == 0);
}

static bool diagSwitchPressed(byte diagIdx, byte* pLastState, unsigned int* pSwitchOld, unsigned int* pSwitchNew) {
  byte switchIdx = SWITCH_IDX_DIAG_1 + diagIdx;
  bool closedNow = diagSwitchClosed(switchIdx, pSwitchNew);

  // Only register a press if: was open (0), now closed (1)
  bool pressed = (closedNow && (pLastState[diagIdx] == 0));

  // Update state: 1 = closed, 0 = open
  pLastState[diagIdx] = closedNow ? 1 : 0;

  return pressed;
}

static void diagLcdPrintRow(byte row, const char* text, Pinball_LCD* pLCD) {
  if (pLCD == nullptr || row < 1 || row > 4) {
    return;
  }
  char buf[LCD_WIDTH + 1];
  memset(buf, ' ', LCD_WIDTH);
  buf[LCD_WIDTH] = '\0';
  byte len = strlen(text);
  if (len > LCD_WIDTH) len = LCD_WIDTH;
  memcpy(buf, text, len);
  pLCD->printRowCol(row, 1, buf);
}

static void diagCopyStringFromProgmem(const char* const* table, unsigned int index, char* dest, byte maxLen) {
  const char* src = (const char*)pgm_read_word(&table[index]);
  strncpy_P(dest, src, maxLen - 1);
  dest[maxLen - 1] = '\0';
}

// ****************************************
// ***** DIAGNOSTIC SUITE HANDLERS ********
// ****************************************

void diagRunVolume(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Tsunami* pTsunami,
  int8_t* pTsunamiGainDb, int8_t* pVoiceGainDb, int8_t* pSfxGainDb,
  int8_t* pMusicGainDb, int8_t* pDuckingDb,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew) {

  byte paramIdx = 0;
  const byte NUM_PARAMS = 5;
  const char* paramNames[NUM_PARAMS] = {
    "Master",
    "Voice Offset",
    "SFX Offset",
    "Music Offset",
    "Duck Offset"
  };

  bool adjusting = false;
  char buf[21];
  byte diagButtonState[4] = { 0, 0, 0, 0 };

  // CRITICAL FIX: Sync button states to current reality before starting
  for (int i = 0; i < 4; i++) {
    pSwitchNew[i] = pShift->portRead(4 + i);
  }
  for (byte i = 0; i < 4; i++) {
    byte switchIdx = SWITCH_IDX_DIAG_1 + i;
    byte pin = switchParm[switchIdx].pinNum - 64;
    byte portIndex = pin / 16;
    byte bitNum = pin % 16;
    bool closedNow = ((pSwitchNew[portIndex] & (1 << bitNum)) == 0);
    diagButtonState[i] = closedNow ? 1 : 0;
  }

  int8_t lastDisplayedValue = 0;
  byte lastDisplayedParam = 0xFF;
  bool needsFullRedraw = true;

  while (true) {
    for (int i = 0; i < 4; i++) {
      pSwitchOld[i] = pSwitchNew[i];
      pSwitchNew[i] = pShift->portRead(4 + i);
    }

    int8_t currentVal = 0;
    switch (paramIdx) {
    case 0: currentVal = *pTsunamiGainDb; break;
    case 1: currentVal = *pVoiceGainDb; break;
    case 2: currentVal = *pSfxGainDb; break;
    case 3: currentVal = *pMusicGainDb; break;
    case 4: currentVal = *pDuckingDb; break;
    }

    if (needsFullRedraw || lastDisplayedParam != paramIdx) {
      diagLcdPrintRow(1, "VOLUME ADJUST", pLCD);
      diagLcdPrintRow(2, paramNames[paramIdx], pLCD);

      if (!adjusting) {
        diagLcdPrintRow(3, "-/+ param SEL=enter", pLCD);
        diagLcdPrintRow(4, "BACK=exit", pLCD);
      } else {
        sprintf(buf, "Value: %4d dB", (int)currentVal);
        diagLcdPrintRow(3, buf, pLCD);
        diagLcdPrintRow(4, "-/+ value  BACK=done", pLCD);
        lastDisplayedValue = currentVal;
      }

      needsFullRedraw = false;
      lastDisplayedParam = paramIdx;
    } else if (adjusting && currentVal != lastDisplayedValue) {
      sprintf(buf, "Value: %4d dB", (int)currentVal);
      diagLcdPrintRow(3, buf, pLCD);
      lastDisplayedValue = currentVal;
    }

    if (diagSwitchPressed(0, diagButtonState, pSwitchOld, pSwitchNew)) {  // BACK
      if (adjusting) {
        if (pTsunami != nullptr) {
          pTsunami->stopAllTracks();
        }
        adjusting = false;
        needsFullRedraw = true;
      } else {
        return;
      }
    }

    if (diagSwitchPressed(1, diagButtonState, pSwitchOld, pSwitchNew)) {  // Minus
      if (!adjusting) {
        if (paramIdx == 0) {
          paramIdx = NUM_PARAMS - 1;
        } else {
          paramIdx--;
        }
        needsFullRedraw = true;
      } else {
        switch (paramIdx) {
        case 0:
          *pTsunamiGainDb = constrain(*pTsunamiGainDb - 1, -40, 0);
          audioApplyMasterGain(*pTsunamiGainDb, pTsunami);  // CHANGED: Use Pinball_Audio
          audioSaveMasterGain(*pTsunamiGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(2001, AUDIO_CATEGORY_MUSIC, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 1:
          *pVoiceGainDb = constrain(*pVoiceGainDb - 1, -20, 20);
          audioSaveCategoryGains(*pVoiceGainDb, *pSfxGainDb, *pMusicGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(351, AUDIO_CATEGORY_VOICE, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 2:
          *pSfxGainDb = constrain(*pSfxGainDb - 1, -20, 20);
          audioSaveCategoryGains(*pVoiceGainDb, *pSfxGainDb, *pMusicGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(1121, AUDIO_CATEGORY_SFX, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 3:
          *pMusicGainDb = constrain(*pMusicGainDb - 1, -20, 20);
          audioSaveCategoryGains(*pVoiceGainDb, *pSfxGainDb, *pMusicGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(2001, AUDIO_CATEGORY_MUSIC, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 4:
          *pDuckingDb = constrain(*pDuckingDb - 1, -40, 0);
          audioSaveDucking(*pDuckingDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(2001, AUDIO_CATEGORY_MUSIC, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
            delay(100);
            int duckGain = (int)(*pTsunamiGainDb) + (int)(*pMusicGainDb) + (int)(*pDuckingDb);
            if (duckGain < -70) duckGain = -70;
            pTsunami->trackGain(2001, duckGain);
            delay(100);
            audioPlayTrackWithCategory(351, AUDIO_CATEGORY_VOICE, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        }
      }
    }

    if (diagSwitchPressed(2, diagButtonState, pSwitchOld, pSwitchNew)) {  // Plus
      if (!adjusting) {
        paramIdx++;
        if (paramIdx >= NUM_PARAMS) {
          paramIdx = 0;
        }
        needsFullRedraw = true;
      } else {
        switch (paramIdx) {
        case 0:
          *pTsunamiGainDb = constrain(*pTsunamiGainDb + 1, -40, 0);
          audioApplyMasterGain(*pTsunamiGainDb, pTsunami);  // CHANGED: Use Pinball_Audio
          audioSaveMasterGain(*pTsunamiGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(2001, AUDIO_CATEGORY_MUSIC, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 1:
          *pVoiceGainDb = constrain(*pVoiceGainDb + 1, -20, 20);
          audioSaveCategoryGains(*pVoiceGainDb, *pSfxGainDb, *pMusicGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(351, AUDIO_CATEGORY_VOICE, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 2:
          *pSfxGainDb = constrain(*pSfxGainDb + 1, -20, 20);
          audioSaveCategoryGains(*pVoiceGainDb, *pSfxGainDb, *pMusicGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(1121, AUDIO_CATEGORY_SFX, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 3:
          *pMusicGainDb = constrain(*pMusicGainDb + 1, -20, 20);
          audioSaveCategoryGains(*pVoiceGainDb, *pSfxGainDb, *pMusicGainDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(2001, AUDIO_CATEGORY_MUSIC, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        case 4:
          *pDuckingDb = constrain(*pDuckingDb + 1, -40, 0);
          audioSaveDucking(*pDuckingDb);  // CHANGED: Use Pinball_Audio
          if (pTsunami != nullptr) {
            pTsunami->stopAllTracks();
            delay(20);
            audioPlayTrackWithCategory(2001, AUDIO_CATEGORY_MUSIC, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
            delay(100);
            audioPlayTrackWithCategory(351, AUDIO_CATEGORY_VOICE, *pTsunamiGainDb, *pVoiceGainDb, *pSfxGainDb, *pMusicGainDb, pTsunami);  // CHANGED
          }
          break;
        }
      }
    }

    if (diagSwitchPressed(3, diagButtonState, pSwitchOld, pSwitchNew)) {  // SELECT
      if (!adjusting) {
        adjusting = true;
        needsFullRedraw = true;
      }
    }

    delay(10);
  }
}

void diagRunLamps(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Pinball_Message* pMsg,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew) {

  // Turn all lamps off initially
  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    pShift->digitalWrite(lampParm[i].pinNum, HIGH);
  }
  pMsg->sendMAStoSLVScoreAbs(0);
  pMsg->sendMAStoSLVGILamp(false);
  pMsg->sendMAStoSLVTiltLamp(false);

  const byte NUM_TOTAL_LAMPS = NUM_LAMPS_MASTER + NUM_LAMPS_SLAVE;
  byte lampIdx = 0;
  char buf[21];
  byte diagButtonState[4] = { 0, 0, 0, 0 };

  // Turn on first lamp
  if (lampIdx < NUM_LAMPS_MASTER) {
    pShift->digitalWrite(lampParm[lampIdx].pinNum, LOW);
  }

  while (true) {
    // Update switches
    for (int i = 0; i < 4; i++) {
      pSwitchOld[i] = pSwitchNew[i];
      pSwitchNew[i] = pShift->portRead(4 + i);
    }

    // Display current lamp
    diagLcdPrintRow(1, "LAMP TEST", pLCD);

    if (lampIdx < NUM_LAMPS_MASTER) {
      char lampName[17];
      diagCopyStringFromProgmem(diagLampNames, lampIdx, lampName, sizeof(lampName));
      sprintf(buf, "M%02d: %.15s", lampIdx, lampName);
    } else {
      byte slaveLampIdx = lampIdx - NUM_LAMPS_MASTER;
      if (slaveLampIdx < 9) {
        sprintf(buf, "S%02d: %dK", slaveLampIdx, (slaveLampIdx + 1) * 10);
      } else if (slaveLampIdx < 18) {
        sprintf(buf, "S%02d: %dK", slaveLampIdx, (slaveLampIdx - 8) * 100);
      } else if (slaveLampIdx < 27) {
        sprintf(buf, "S%02d: %dM", slaveLampIdx, (slaveLampIdx - 17));
      } else if (slaveLampIdx == 27) {
        sprintf(buf, "S27: GI");
      } else {
        sprintf(buf, "S28: TILT");
      }
    }
    diagLcdPrintRow(2, buf, pLCD);

    sprintf(buf, "Lamp %d of %d", lampIdx + 1, NUM_TOTAL_LAMPS);
    diagLcdPrintRow(3, buf, pLCD);
    diagLcdPrintRow(4, "ON", pLCD);

    // Button handling
    if (diagSwitchPressed(0, diagButtonState, pSwitchOld, pSwitchNew)) {  // BACK
      // Turn off current lamp
      if (lampIdx < NUM_LAMPS_MASTER) {
        pShift->digitalWrite(lampParm[lampIdx].pinNum, HIGH);
      } else {
        byte slaveLampIdx = lampIdx - NUM_LAMPS_MASTER;
        if (slaveLampIdx < 27) {
          pMsg->sendMAStoSLVScoreAbs(0);
        } else if (slaveLampIdx == 27) {
          pMsg->sendMAStoSLVGILamp(false);
        } else {
          pMsg->sendMAStoSLVTiltLamp(false);
        }
      }
      // Restore attract lamps (GI + bumpers)
      for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
        if (lampParm[i].groupNum == LAMP_GROUP_GI) {
          pShift->digitalWrite(lampParm[i].pinNum, LOW);
        }
      }
      for (byte i = LAMP_IDX_S; i <= LAMP_IDX_O; i++) {
        pShift->digitalWrite(lampParm[i].pinNum, LOW);
      }
      pMsg->sendMAStoSLVGILamp(true);
      return;
    }

    if (diagSwitchPressed(1, diagButtonState, pSwitchOld, pSwitchNew)) {  // Prev
      // Turn off current
      if (lampIdx < NUM_LAMPS_MASTER) {
        pShift->digitalWrite(lampParm[lampIdx].pinNum, HIGH);
      } else {
        byte slaveLampIdx = lampIdx - NUM_LAMPS_MASTER;
        if (slaveLampIdx < 27) {
          pMsg->sendMAStoSLVScoreAbs(0);
        } else if (slaveLampIdx == 27) {
          pMsg->sendMAStoSLVGILamp(false);
        } else {
          pMsg->sendMAStoSLVTiltLamp(false);
        }
      }

      // Move to previous
      if (lampIdx == 0) lampIdx = NUM_TOTAL_LAMPS - 1;
      else lampIdx--;

      // Turn on new lamp
      if (lampIdx < NUM_LAMPS_MASTER) {
        pShift->digitalWrite(lampParm[lampIdx].pinNum, LOW);
      } else {
        byte slaveLampIdx = lampIdx - NUM_LAMPS_MASTER;
        if (slaveLampIdx < 9) {
          pMsg->sendMAStoSLVScoreAbs(slaveLampIdx + 1);
        } else if (slaveLampIdx < 18) {
          pMsg->sendMAStoSLVScoreAbs((slaveLampIdx - 8) * 10);
        } else if (slaveLampIdx < 27) {
          pMsg->sendMAStoSLVScoreAbs((slaveLampIdx - 17) * 100);
        } else if (slaveLampIdx == 27) {
          pMsg->sendMAStoSLVGILamp(true);
        } else {
          pMsg->sendMAStoSLVTiltLamp(true);
        }
      }
    }

    if (diagSwitchPressed(2, diagButtonState, pSwitchOld, pSwitchNew)) {  // Next
      // Turn off current
      if (lampIdx < NUM_LAMPS_MASTER) {
        pShift->digitalWrite(lampParm[lampIdx].pinNum, HIGH);
      } else {
        byte slaveLampIdx = lampIdx - NUM_LAMPS_MASTER;
        if (slaveLampIdx < 27) {
          pMsg->sendMAStoSLVScoreAbs(0);
        } else if (slaveLampIdx == 27) {
          pMsg->sendMAStoSLVGILamp(false);
        } else {
          pMsg->sendMAStoSLVTiltLamp(false);
        }
      }

      // Move to next
      lampIdx++;
      if (lampIdx >= NUM_TOTAL_LAMPS) lampIdx = 0;

      // Turn on new lamp
      if (lampIdx < NUM_LAMPS_MASTER) {
        pShift->digitalWrite(lampParm[lampIdx].pinNum, LOW);
      } else {
        byte slaveLampIdx = lampIdx - NUM_LAMPS_MASTER;
        if (slaveLampIdx < 9) {
          pMsg->sendMAStoSLVScoreAbs(slaveLampIdx + 1);
        } else if (slaveLampIdx < 18) {
          pMsg->sendMAStoSLVScoreAbs((slaveLampIdx - 8) * 10);
        } else if (slaveLampIdx < 27) {
          pMsg->sendMAStoSLVScoreAbs((slaveLampIdx - 17) * 100);
        } else if (slaveLampIdx == 27) {
          pMsg->sendMAStoSLVGILamp(true);
        } else {
          pMsg->sendMAStoSLVTiltLamp(true);
        }
      }
    }

    delay(10);
  }
}

void diagRunSwitches(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Tsunami* pTsunami,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew) {

  byte lastSwitch = 0xFF;
  bool lastSwitchWasClosed = false;
  char buf[21];
  byte diagButtonState[4] = { 0, 0, 0, 0 };

  diagLcdPrintRow(1, "SWITCH TEST", pLCD);
  diagLcdPrintRow(2, "Waiting...", pLCD);
  diagLcdPrintRow(3, "", pLCD);
  diagLcdPrintRow(4, "", pLCD);

  while (true) {
    // Update switches
    for (int i = 0; i < 4; i++) {
      pSwitchOld[i] = pSwitchNew[i];
      pSwitchNew[i] = pShift->portRead(4 + i);
    }

    if (diagSwitchPressed(0, diagButtonState, pSwitchOld, pSwitchNew)) {
      return;
    }

    // Check for any closed switch
    byte closedSwitch = 0xFF;
    for (byte sw = 0; sw < NUM_SWITCHES_MASTER; sw++) {
      if (diagSwitchClosed(sw, pSwitchNew)) {
        closedSwitch = sw;
        break;
      }
    }

    if (closedSwitch != lastSwitch) {
      if (closedSwitch == 0xFF) {
        if (lastSwitchWasClosed) {
          if (pTsunami != nullptr) {
            pTsunami->trackPlayPoly(105, 0, false);  // Switch open tone
          }
          lastSwitchWasClosed = false;
        }
        diagLcdPrintRow(2, "Waiting...", pLCD);
        diagLcdPrintRow(3, "", pLCD);
        diagLcdPrintRow(4, "", pLCD);
      } else {
        char swName[17];
        diagCopyStringFromProgmem(diagSwitchNames, closedSwitch, swName, sizeof(swName));
        sprintf(buf, "SW%02d: %.14s", closedSwitch, swName);
        diagLcdPrintRow(2, buf, pLCD);
        sprintf(buf, "Switch %d of %d", closedSwitch + 1, NUM_SWITCHES_MASTER);
        diagLcdPrintRow(3, buf, pLCD);
        sprintf(buf, "Pin: %d", switchParm[closedSwitch].pinNum);
        diagLcdPrintRow(4, buf, pLCD);
        if (pTsunami != nullptr) {
          pTsunami->trackPlayPoly(104, 0, false);  // Switch close tone
        }
        lastSwitchWasClosed = true;
      }
      lastSwitch = closedSwitch;
    }
    delay(10);
  }
}

void diagRunCoils(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Pinball_Message* pMsg,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew) {

  const byte NUM_SLAVE_DEVS = 6;
  const byte NUM_TOTAL_DEVS = NUM_DEVS_MASTER + NUM_SLAVE_DEVS;
  byte devIdx = 0;
  char buf[21];
  bool motorRunning = false;
  byte diagButtonState[4] = { 0, 0, 0, 0 };

  // CRITICAL FIX: Sync button states to prevent carry-over
  for (int i = 0; i < 4; i++) {
    pSwitchNew[i] = pShift->portRead(4 + i);
  }
  for (byte i = 0; i < 4; i++) {
    byte switchIdx = SWITCH_IDX_DIAG_1 + i;
    if (switchIdx >= NUM_SWITCHES_MASTER) continue;
    byte pin = switchParm[switchIdx].pinNum - 64;
    byte portIndex = pin / 16;
    byte bitNum = pin % 16;
    bool closedNow = ((pSwitchNew[portIndex] & (1 << bitNum)) == 0);
    diagButtonState[i] = closedNow ? 1 : 0;
  }

  while (true) {
    // Update switches
    for (int i = 0; i < 4; i++) {
      pSwitchOld[i] = pSwitchNew[i];
      pSwitchNew[i] = pShift->portRead(4 + i);
    }

    // Display
    diagLcdPrintRow(1, "COIL/MOTOR TEST", pLCD);

    if (devIdx < NUM_DEVS_MASTER) {
      char devName[17];
      diagCopyStringFromProgmem(diagCoilNames, devIdx, devName, sizeof(devName));
      sprintf(buf, "M%02d: %.15s", devIdx, devName);
    } else {
      byte slaveDevIdx = devIdx - NUM_DEVS_MASTER;
      const char* slaveDevNames[6] = {
        "Credit Up", "Credit Down", "10K Up", "10K Bell", "100K Bell", "Select Bell"
      };
      sprintf(buf, "S%02d: %.13s", slaveDevIdx, slaveDevNames[slaveDevIdx]);
    }
    diagLcdPrintRow(2, buf, pLCD);

    sprintf(buf, "Device %d of %d", devIdx + 1, NUM_TOTAL_DEVS);
    diagLcdPrintRow(3, buf, pLCD);

    if (motorRunning) {
      diagLcdPrintRow(4, "RUNNING", pLCD);
    } else {
      diagLcdPrintRow(4, "Ready", pLCD);
    }

    // Button handling
    if (diagSwitchPressed(0, diagButtonState, pSwitchOld, pSwitchNew)) {  // BACK
      if (motorRunning) {
        if (devIdx == DEV_IDX_MOTOR_SHAKER) {
          analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
        } else if (devIdx == DEV_IDX_MOTOR_SCORE) {
          digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, LOW);
        }
        motorRunning = false;
      }
      return;
    }

    if (diagSwitchPressed(1, diagButtonState, pSwitchOld, pSwitchNew)) {  // Prev
      if (motorRunning) {
        if (devIdx == DEV_IDX_MOTOR_SHAKER) {
          analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
        } else if (devIdx == DEV_IDX_MOTOR_SCORE) {
          digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, LOW);
        }
        motorRunning = false;
      }
      if (devIdx == 0) devIdx = NUM_TOTAL_DEVS - 1;
      else devIdx--;
    }

    if (diagSwitchPressed(2, diagButtonState, pSwitchOld, pSwitchNew)) {  // Next
      if (motorRunning) {
        if (devIdx == DEV_IDX_MOTOR_SHAKER) {
          analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
        } else if (devIdx == DEV_IDX_MOTOR_SCORE) {
          digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, LOW);
        }
        motorRunning = false;
      }
      devIdx++;
      if (devIdx >= NUM_TOTAL_DEVS) devIdx = 0;
    }

    if (diagSwitchPressed(3, diagButtonState, pSwitchOld, pSwitchNew)) {  // Fire/Start
      if (devIdx == DEV_IDX_MOTOR_SHAKER) {
        analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, MOTOR_SHAKER_POWER_MIN);
        motorRunning = true;
      } else if (devIdx == DEV_IDX_MOTOR_SCORE) {
        digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, HIGH);
        motorRunning = true;
      } else if (devIdx < NUM_DEVS_MASTER) {
        analogWrite(deviceParm[devIdx].pinNum, deviceParm[devIdx].powerInitial);
        delay(deviceParm[devIdx].timeOn * 10);
        analogWrite(deviceParm[devIdx].pinNum, 0);
      } else {
        byte slaveDevIdx = devIdx - NUM_DEVS_MASTER;
        switch (slaveDevIdx) {
        case 0: pMsg->sendMAStoSLVCreditInc(1); break;
        case 1: pMsg->sendMAStoSLVCreditDec(); break;
        case 2: pMsg->sendMAStoSLV10KUnitPulse(); break;
        case 3: pMsg->sendMAStoSLVBell10K(); break;
        case 4: pMsg->sendMAStoSLVBell100K(); break;
        case 5: pMsg->sendMAStoSLVBellSelect(); break;
        }
      }
    }

    // Stop motors if SELECT button released
    if (motorRunning && !diagSwitchClosed(SWITCH_IDX_DIAG_4, pSwitchNew)) {
      if (devIdx == DEV_IDX_MOTOR_SHAKER) {
        analogWrite(deviceParm[DEV_IDX_MOTOR_SHAKER].pinNum, 0);
      } else if (devIdx == DEV_IDX_MOTOR_SCORE) {
        digitalWrite(deviceParm[DEV_IDX_MOTOR_SCORE].pinNum, LOW);
      }
      motorRunning = false;
    }

    delay(10);
  }
}

void diagRunAudio(Pinball_LCD* pLCD, Pinball_Centipede* pShift, Tsunami* pTsunami,
  int8_t masterGainDb, int8_t voiceGainDb, int8_t sfxGainDb, int8_t musicGainDb,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew) {

  unsigned int trackIdx = 0;
  byte diagButtonState[4] = { 0, 0, 0, 0 };

  // CRITICAL FIX: Sync button states to prevent carry-over
  for (int i = 0; i < 4; i++) {
    pSwitchNew[i] = pShift->portRead(4 + i);
  }
  for (byte i = 0; i < 4; i++) {
    byte switchIdx = SWITCH_IDX_DIAG_1 + i;
    if (switchIdx >= NUM_SWITCHES_MASTER) continue;
    byte pin = switchParm[switchIdx].pinNum - 64;
    byte portIndex = pin / 16;
    byte bitNum = pin % 16;
    bool closedNow = ((pSwitchNew[portIndex] & (1 << bitNum)) == 0);
    diagButtonState[i] = closedNow ? 1 : 0;
  }

  while (true) {
    // Update switches
    for (int i = 0; i < 4; i++) {
      pSwitchOld[i] = pSwitchNew[i];
      pSwitchNew[i] = pShift->portRead(4 + i);
    }

    if (trackIdx >= NUM_DIAG_AUDIO_TRACKS) {
      trackIdx = 0;
    }

    unsigned int testTrack = pgm_read_word(&diagAudioTrackNums[trackIdx]);

    char line1[21];
    char line2[21];
    char buf[21];

    diagCopyStringFromProgmem(diagAudioLine1, trackIdx, line1, sizeof(line1));
    diagCopyStringFromProgmem(diagAudioLine2, trackIdx, line2, sizeof(line2));

    diagLcdPrintRow(1, line1, pLCD);
    diagLcdPrintRow(2, line2, pLCD);

    sprintf(buf, "Track %u of %u", (unsigned)(trackIdx + 1), (unsigned)NUM_DIAG_AUDIO_TRACKS);
    diagLcdPrintRow(3, buf, pLCD);
    diagLcdPrintRow(4, "Ready", pLCD);

    if (diagSwitchPressed(0, diagButtonState, pSwitchOld, pSwitchNew)) {
      if (pTsunami != nullptr) {
        pTsunami->stopAllTracks();
      }
      return;
    }

    if (diagSwitchPressed(1, diagButtonState, pSwitchOld, pSwitchNew)) {
      if (trackIdx > 0) {
        trackIdx--;
      } else {
        trackIdx = NUM_DIAG_AUDIO_TRACKS - 1;
      }
    }

    if (diagSwitchPressed(2, diagButtonState, pSwitchOld, pSwitchNew)) {
      trackIdx++;
      if (trackIdx >= NUM_DIAG_AUDIO_TRACKS) {
        trackIdx = 0;
      }
    }

    if (diagSwitchPressed(3, diagButtonState, pSwitchOld, pSwitchNew)) {
      if (pTsunami != nullptr) {
        pTsunami->stopAllTracks();
        delay(50);

        // Determine category and apply appropriate gain
        byte category = AUDIO_CATEGORY_SFX; // Default SFX
        if (testTrack >= 2001 && testTrack <= 2068) {
          category = AUDIO_CATEGORY_MUSIC; // Music
        } else if (testTrack >= 101 && testTrack <= 999) {
          category = AUDIO_CATEGORY_VOICE; // Voice/COM
        }

        // CHANGED: Use Pinball_Audio function
        audioPlayTrackWithCategory(testTrack, category, masterGainDb, voiceGainDb, sfxGainDb, musicGainDb, pTsunami);

        diagLcdPrintRow(4, "Playing...", pLCD);
        delay(200);
      } else {
        diagLcdPrintRow(4, "Tsunami NULL!", pLCD);
      }
    }

    delay(10);
  }
}

void diagRunSettings(Pinball_LCD* pLCD, Pinball_Centipede* pShift,
  unsigned int* pSwitchOld, unsigned int* pSwitchNew) {

  // Three-level navigation: category -> parameter -> value
  byte categoryIdx = 0;
  byte paramIdx = 0;
  byte level = 0;  // 0=category select, 1=param select, 2=value adjust

  const byte NUM_CATEGORIES = 3;
  const char* categoryNames[NUM_CATEGORIES] = {
    "GAME SETTINGS",
    "ORIGINAL REPLAYS",
    "ENHANCED REPLAYS"
  };

  char buf[21];
  byte diagButtonState[4] = { 0, 0, 0, 0 };
  bool needsRedraw = true;

  // CRITICAL FIX: Sync button states to prevent carry-over
  for (int i = 0; i < 4; i++) {
    pSwitchNew[i] = pShift->portRead(4 + i);
  }
  for (byte i = 0; i < 4; i++) {
    byte switchIdx = SWITCH_IDX_DIAG_1 + i;
    if (switchIdx >= NUM_SWITCHES_MASTER) continue;
    byte pin = switchParm[switchIdx].pinNum - 64;
    byte portIndex = pin / 16;
    byte bitNum = pin % 16;
    bool closedNow = ((pSwitchNew[portIndex] & (1 << bitNum)) == 0);
    diagButtonState[i] = closedNow ? 1 : 0;
  }

  while (true) {
    // Update switches
    for (int i = 0; i < 4; i++) {
      pSwitchOld[i] = pSwitchNew[i];
      pSwitchNew[i] = pShift->portRead(4 + i);
    }

    // Display current screen
    if (needsRedraw) {
      if (level == 0) {
        // Category selection
        diagLcdPrintRow(1, "SETTINGS", pLCD);
        diagLcdPrintRow(2, categoryNames[categoryIdx], pLCD);
        diagLcdPrintRow(3, "-/+ category SEL=enter", pLCD);
        diagLcdPrintRow(4, "BACK=exit", pLCD);

      } else if (level == 1) {
        // Parameter selection within category
        diagLcdPrintRow(1, categoryNames[categoryIdx], pLCD);

        if (categoryIdx == SETTINGS_CAT_GAME) {
          const char* paramNamePtr = (const char*)pgm_read_word(&gameSettingNames[paramIdx]);
          diagLcdPrintRow(2, paramNamePtr, pLCD);
        } else {
          snprintf(buf, sizeof(buf), "Replay %d", paramIdx + 1);
          diagLcdPrintRow(2, buf, pLCD);
        }

        diagLcdPrintRow(3, "-/+ param SEL=enter", pLCD);
        diagLcdPrintRow(4, "BACK=exit category", pLCD);

      } else {
        // Level 2: Value adjustment
        diagLcdPrintRow(1, "ADJUST VALUE", pLCD);

        if (categoryIdx == SETTINGS_CAT_GAME) {
          const char* paramNamePtr = (const char*)pgm_read_word(&gameSettingNames[paramIdx]);
          diagLcdPrintRow(2, paramNamePtr, pLCD);
          // Display current value
          if (paramIdx == GAME_SETTING_THEME) {
            byte theme = diagReadThemeFromEEPROM();
            snprintf(buf, sizeof(buf), "Value: %s", theme == THEME_CIRCUS ? "Circus" : "Surf");
          } else if (paramIdx == GAME_SETTING_BALL_SAVE) {
            byte ballSave = diagReadBallSaveTimeFromEEPROM();
            snprintf(buf, sizeof(buf), "Value: %d sec", ballSave);
          } else if (paramIdx == GAME_SETTING_TIMEOUT) {  // NEW
            byte timeout = diagReadGameTimeoutFromEEPROM();
            snprintf(buf, sizeof(buf), "Value: %d min", timeout);
          } else {
            byte modeTime = diagReadModeTimeFromEEPROM(paramIdx - GAME_SETTING_MODE_1 + 1);
            snprintf(buf, sizeof(buf), "Value: %d sec", modeTime);
          }
        } else {
          bool enhanced = (categoryIdx == SETTINGS_CAT_ENH_REPLAY);
          snprintf(buf, sizeof(buf), "Replay %d", paramIdx + 1);
          diagLcdPrintRow(2, buf, pLCD);

          unsigned int score = diagReadReplayScoreFromEEPROM(enhanced, paramIdx + 1);
          snprintf(buf, sizeof(buf), "Score: %d.%dM", score / 100, (score / 10) % 10);
        }

        diagLcdPrintRow(3, buf, pLCD);
        diagLcdPrintRow(4, "-/+ value  BACK=done", pLCD);
      }

      needsRedraw = false;
    }

    // Button handling
    if (diagSwitchPressed(0, diagButtonState, pSwitchOld, pSwitchNew)) {  // BACK
      if (level == 2) {
        level = 1;
        needsRedraw = true;
      } else if (level == 1) {
        level = 0;
        paramIdx = 0;
        needsRedraw = true;
      } else {
        return;
      }
    }

    if (diagSwitchPressed(1, diagButtonState, pSwitchOld, pSwitchNew)) {  // Minus
      if (level == 0) {
        if (categoryIdx == 0) categoryIdx = NUM_CATEGORIES - 1;
        else categoryIdx--;
        needsRedraw = true;
      } else if (level == 1) {
        byte maxParams = (categoryIdx == SETTINGS_CAT_GAME) ? NUM_GAME_SETTINGS : 5;
        if (paramIdx == 0) paramIdx = maxParams - 1;
        else paramIdx--;
        needsRedraw = true;
      } else {
        if (categoryIdx == SETTINGS_CAT_GAME) {
          if (paramIdx == GAME_SETTING_THEME) {
            byte theme = diagReadThemeFromEEPROM();
            theme = (theme == THEME_CIRCUS) ? THEME_SURF : THEME_CIRCUS;
            diagWriteThemeToEEPROM(theme);
          } else if (paramIdx == GAME_SETTING_BALL_SAVE) {
            byte ballSave = diagReadBallSaveTimeFromEEPROM();
            if (ballSave > 0) ballSave--;
            diagWriteBallSaveTimeToEEPROM(ballSave);
          } else if (paramIdx == GAME_SETTING_TIMEOUT) {  // NEW
            byte timeout = diagReadGameTimeoutFromEEPROM();
            if (timeout > 1) timeout--;  // Min 1 minute
            diagWriteGameTimeoutToEEPROM(timeout);
          } else {
            byte modeTime = diagReadModeTimeFromEEPROM(paramIdx - GAME_SETTING_MODE_1 + 1);
            if (modeTime > 1) modeTime--;
            diagWriteModeTimeToEEPROM(paramIdx - GAME_SETTING_MODE_1 + 1, modeTime);
          }
        } else {
          bool enhanced = (categoryIdx == SETTINGS_CAT_ENH_REPLAY);
          unsigned int score = diagReadReplayScoreFromEEPROM(enhanced, paramIdx + 1);
          if (score > 0) score--;
          diagWriteReplayScoreToEEPROM(enhanced, paramIdx + 1, score);
        }
        needsRedraw = true;
      }
    }

    if (diagSwitchPressed(2, diagButtonState, pSwitchOld, pSwitchNew)) {  // Plus
      if (level == 0) {
        categoryIdx++;
        if (categoryIdx >= NUM_CATEGORIES) categoryIdx = 0;
        needsRedraw = true;
      } else if (level == 1) {
        byte maxParams = (categoryIdx == SETTINGS_CAT_GAME) ? NUM_GAME_SETTINGS : 5;
        paramIdx++;
        if (paramIdx >= maxParams) paramIdx = 0;
        needsRedraw = true;
      } else {
        if (categoryIdx == SETTINGS_CAT_GAME) {
          if (paramIdx == GAME_SETTING_THEME) {
            byte theme = diagReadThemeFromEEPROM();
            theme = (theme == THEME_CIRCUS) ? THEME_SURF : THEME_CIRCUS;
            diagWriteThemeToEEPROM(theme);
          } else if (paramIdx == GAME_SETTING_BALL_SAVE) {
            byte ballSave = diagReadBallSaveTimeFromEEPROM();
            if (ballSave < 30) ballSave++;
            diagWriteBallSaveTimeToEEPROM(ballSave);
          } else if (paramIdx == GAME_SETTING_TIMEOUT) {  // NEW
            byte timeout = diagReadGameTimeoutFromEEPROM();
            if (timeout < 60) timeout++;  // Max 60 minutes
            diagWriteGameTimeoutToEEPROM(timeout);
          } else {
            byte modeTime = diagReadModeTimeFromEEPROM(paramIdx - GAME_SETTING_MODE_1 + 1);
            if (modeTime < 250) modeTime++;
            diagWriteModeTimeToEEPROM(paramIdx - GAME_SETTING_MODE_1 + 1, modeTime);
          }
        } else {
          bool enhanced = (categoryIdx == SETTINGS_CAT_ENH_REPLAY);
          unsigned int score = diagReadReplayScoreFromEEPROM(enhanced, paramIdx + 1);
          if (score < 999) score++;
          diagWriteReplayScoreToEEPROM(enhanced, paramIdx + 1, score);
        }
        needsRedraw = true;
      }
    }

    if (diagSwitchPressed(3, diagButtonState, pSwitchOld, pSwitchNew)) {  // SELECT
      if (level == 0) {
        level = 1;
        paramIdx = 0;
        needsRedraw = true;
      } else if (level == 1) {
        level = 2;
        needsRedraw = true;
      }
    }

    delay(10);
  }
}