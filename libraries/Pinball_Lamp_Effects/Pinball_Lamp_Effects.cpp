// PINBALL_LAMP_EFFECTS.CPP Rev: 03/11/26.
// Position-based lamp effect engine for Screamo playfield lamps.
// See Pinball_Lamp_Effects.h for API documentation.

#include <Pinball_Lamp_Effects.h>
#include <Pinball_Centipede.h>

// ************************************************************************
// ************************ TUNING CONSTANTS ******************************
// ************************************************************************

// For fill-then-drain effects, pause with all lamps OFF before the fill phase begins,
// and pause with all lamps ON between the fill and drain phases.
// Both pauses use the same percentage of sweepMs. For example, with sweepMs=3000 and
// FILL_DRAIN_PAUSE_PERCENT=30, both pauses are 900ms: the player sees a 900ms blackout,
// then the fill sweep, then 900ms fully lit, then the drain sweep.
// Set to 0 to disable both pauses entirely.
const byte FILL_DRAIN_PAUSE_PERCENT = 30;

// ************************************************************************
// ******************** LAMP POSITION TABLE (PROGMEM) *********************
// ************************************************************************
// Indexed by lamp index 0..46 (matches lampParm[] order in Screamo_Master.ino).
// x_mm = distance from left edge, y_mm = distance from top edge of playfield.
// Coordinates are approximate (+/- 20mm) -- sufficient for smooth sweep timing.

const LampPositionDef lampPositions[NUM_LAMPS_MASTER] PROGMEM = {
  //  x_mm, y_mm
  {   63,  183 },  //  0  GI_LEFT_TOP
  {   47,  476 },  //  1  GI_LEFT_CENTER_1
  {   43,  616 },  //  2  GI_LEFT_CENTER_2
  {  103,  848 },  //  3  GI_LEFT_BOTTOM
  {  416,  183 },  //  4  GI_RIGHT_TOP
  {  424,  476 },  //  5  GI_RIGHT_CENTER_1
  {  428,  616 },  //  6  GI_RIGHT_CENTER_2
  {  365,  848 },  //  7  GI_RIGHT_BOTTOM
  {  173,  194 },  //  8  BUMPER_S
  {  300,  194 },  //  9  BUMPER_C
  {  110,  306 },  // 10  BUMPER_R
  {  236,  306 },  // 11  BUMPER_E
  {  363,  306 },  // 12  BUMPER_A
  {  172,  417 },  // 13  BUMPER_M
  {  298,  417 },  // 14  BUMPER_O
  {  165,  558 },  // 15  WHITE_1
  {  234,  558 },  // 16  WHITE_2
  {  304,  558 },  // 17  WHITE_3
  {  165,  629 },  // 18  WHITE_4
  {  234,  629 },  // 19  WHITE_5
  {  304,  629 },  // 20  WHITE_6
  {  165,  698 },  // 21  WHITE_7
  {  234,  698 },  // 22  WHITE_8
  {  304,  698 },  // 23  WHITE_9
  {  143,  581 },  // 24  RED_1
  {  212,  581 },  // 25  RED_2
  {  281,  581 },  // 26  RED_3
  {  143,  651 },  // 27  RED_4
  {  212,  651 },  // 28  RED_5
  {  281,  651 },  // 29  RED_6
  {  143,  721 },  // 30  RED_7
  {  212,  721 },  // 31  RED_8
  {  281,  721 },  // 32  RED_9
  {  129,  493 },  // 33  HAT_LEFT_TOP
  {  109,  629 },  // 34  HAT_LEFT_BOTTOM
  {  341,  493 },  // 35  HAT_RIGHT_TOP
  {  359,  629 },  // 36  HAT_RIGHT_BOTTOM
  {   39,  545 },  // 37  KICKOUT_LEFT
  {  432,  545 },  // 38  KICKOUT_RIGHT
  {  236,  409 },  // 39  SPECIAL
  {  188,  494 },  // 40  GOBBLE_1
  {  202,  461 },  // 41  GOBBLE_2
  {  236,  447 },  // 42  GOBBLE_3
  {  269,  461 },  // 43  GOBBLE_4
  {  283,  494 },  // 44  GOBBLE_5
  {   44,  747 },  // 45  SPOT_NUMBER_LEFT
  {  425,  747 }   // 46  SPOT_NUMBER_RIGHT
};

// ************************************************************************
// ********************* EFFECT ENGINE STATE ******************************
// ************************************************************************

static byte  effectType = LAMP_EFFECT_NONE;
static unsigned long effectStartMs = 0;       // millis() when current phase started
static unsigned long effectSweepMs = 0;       // Sweep duration per phase
static unsigned long effectHoldMs = 0;        // Hold time after final phase completes
static bool  effectSaveComplete = false;      // True once lamp state has been saved

// Two-phase support: the "requested" effect type is stored in effectType.
// effectPhase tracks which phase we are currently executing (1 or 2).
// effectPhase1Type and effectPhase2Type hold the simple sweep type for each phase.
static byte effectPhase = 0;                  // 0 = not started, 1 = first phase, 2 = second phase
static byte effectPhase1Type = LAMP_EFFECT_NONE;
static byte effectPhase2Type = LAMP_EFFECT_NONE;

// Phase mode: false = turn lamps ON as activation time arrives (normal sweep/fill).
//             true  = turn lamps OFF as activation time arrives (drain phase).
static bool effectPhaseDrain = false;

// Leading blackout pause (fill-drain effects only): after all lamps are turned off on the
// first tick, hold the blackout for a duration so the player sees the contrast before the
// fill sweep begins. 0 = no blackout pause active.
static unsigned long effectBlackoutEndMs = 0;

// Fill-drain mid-point pause: when fill phase completes, effectPauseEndMs is set to
// millis() + pause duration. The engine holds all lamps on until the pause expires,
// then transitions to the drain phase. 0 = no pause active.
static unsigned long effectPauseEndMs = 0;

// Saved lamp state: 4 x 16-bit words covering Centipede output ports 0..3 (pins 0..63).
// Read via portRead() before the effect starts; written back via portWrite() when it ends.
static unsigned int savedPortState[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

// Per-lamp activation time in milliseconds relative to effectStartMs.
// Computed once per phase when startLampEffect() or phase transition is called.
// 47 x 2 = 94 bytes RAM.
static unsigned int lampActivateMs[NUM_LAMPS_MASTER];

// Bitfield tracking which lamps have been processed during the current phase.
// For fill phases: bit N = lamp N has been turned ON.
// For drain phases: bit N = lamp N has been turned OFF.
// 47 lamps = 6 bytes.
static byte lampsProcessedBits[6];

// ************************************************************************
// ****************** HELPER: READ PROGMEM POSITION ***********************
// ************************************************************************

static unsigned int readLampX(byte lampIdx) {
  return pgm_read_word(&lampPositions[lampIdx].x_mm);
}

static unsigned int readLampY(byte lampIdx) {
  return pgm_read_word(&lampPositions[lampIdx].y_mm);
}

// ************************************************************************
// ****************** HELPER: GET POSITION ALONG AXIS *********************
// ************************************************************************

static unsigned int getAxisPosition(byte lampIdx, byte effect) {
  if (effect == LAMP_EFFECT_SWEEP_LEFT || effect == LAMP_EFFECT_SWEEP_RIGHT) {
    return readLampX(lampIdx);
  }
  return readLampY(lampIdx);  // Top-down and bottom-up both use Y
}

// ************************************************************************
// ****************** HELPER: BITFIELD OPERATIONS *************************
// ************************************************************************

static bool isLampProcessed(byte lampIdx) {
  return (lampsProcessedBits[lampIdx >> 3] & (1 << (lampIdx & 7))) != 0;
}

static void markLampProcessed(byte lampIdx) {
  lampsProcessedBits[lampIdx >> 3] |= (1 << (lampIdx & 7));
}

// ************************************************************************
// ****************** HELPER: EFFECT CLASSIFICATION ***********************
// ************************************************************************

static bool isTwoPhaseEffect(byte effect) {
  return (effect >= LAMP_EFFECT_SWEEP_DOWN_UP && effect <= LAMP_EFFECT_SWEEP_RL_LR);
}

static bool isFillDrainEffect(byte effect) {
  return (effect >= LAMP_EFFECT_FILL_UP_DRAIN_DOWN && effect <= LAMP_EFFECT_FILL_RL_DRAIN_LR);
}

// ************************************************************************
// ****************** HELPER: COMPUTE PAUSE DURATION **********************
// ************************************************************************

// Returns the pause duration in milliseconds for fill-drain effects, computed from
// FILL_DRAIN_PAUSE_PERCENT and the current effectSweepMs. Returns 0 if the pause
// percentage is 0 or the effect is not a fill-drain type.
static unsigned long fillDrainPauseMs() {
  if (FILL_DRAIN_PAUSE_PERCENT == 0) {
    return 0;
  }
  return ((unsigned long)FILL_DRAIN_PAUSE_PERCENT * effectSweepMs) / 100UL;
}

// Maps a two-phase sweep effect constant to its two simple sweep phases.
// Both phases are "fill" (turn lamps on).
static void getPhaseTypes(byte effect, byte* phase1, byte* phase2) {
  switch (effect) {
  case LAMP_EFFECT_SWEEP_DOWN_UP:
    *phase1 = LAMP_EFFECT_SWEEP_TOP_DOWN;
    *phase2 = LAMP_EFFECT_SWEEP_BOT_UP;
    break;
  case LAMP_EFFECT_SWEEP_UP_DOWN:
    *phase1 = LAMP_EFFECT_SWEEP_BOT_UP;
    *phase2 = LAMP_EFFECT_SWEEP_TOP_DOWN;
    break;
  case LAMP_EFFECT_SWEEP_LR_RL:
    *phase1 = LAMP_EFFECT_SWEEP_LEFT;
    *phase2 = LAMP_EFFECT_SWEEP_RIGHT;
    break;
  case LAMP_EFFECT_SWEEP_RL_LR:
    *phase1 = LAMP_EFFECT_SWEEP_RIGHT;
    *phase2 = LAMP_EFFECT_SWEEP_LEFT;
    break;
  default:
    *phase1 = effect;
    *phase2 = LAMP_EFFECT_NONE;
    break;
  }
}

// Maps a fill-then-drain effect constant to its two simple sweep phases.
// Phase 1 fills (turn lamps on), phase 2 drains (turn lamps off).
// The simple sweep type controls direction; effectPhaseDrain controls on/off behavior.
static void getFillDrainPhaseTypes(byte effect, byte* phase1, byte* phase2) {
  switch (effect) {
  case LAMP_EFFECT_FILL_UP_DRAIN_DOWN:
    *phase1 = LAMP_EFFECT_SWEEP_BOT_UP;    // Fill: bottom to top
    *phase2 = LAMP_EFFECT_SWEEP_TOP_DOWN;   // Drain: top to bottom
    break;
  case LAMP_EFFECT_FILL_DOWN_DRAIN_UP:
    *phase1 = LAMP_EFFECT_SWEEP_TOP_DOWN;   // Fill: top to bottom
    *phase2 = LAMP_EFFECT_SWEEP_BOT_UP;     // Drain: bottom to top
    break;
  case LAMP_EFFECT_FILL_LR_DRAIN_RL:
    *phase1 = LAMP_EFFECT_SWEEP_LEFT;       // Fill: left to right
    *phase2 = LAMP_EFFECT_SWEEP_RIGHT;      // Drain: right to left
    break;
  case LAMP_EFFECT_FILL_RL_DRAIN_LR:
    *phase1 = LAMP_EFFECT_SWEEP_RIGHT;      // Fill: right to left
    *phase2 = LAMP_EFFECT_SWEEP_LEFT;       // Drain: left to right
    break;
  default:
    *phase1 = LAMP_EFFECT_SWEEP_BOT_UP;
    *phase2 = LAMP_EFFECT_SWEEP_TOP_DOWN;
    break;
  }
}

// ************************************************************************
// ****************** COMPUTE ACTIVATION TIMES ****************************
// ************************************************************************
// For each lamp, compute when it turns on/off (ms offset from phase start).
// Timing is linearly proportional to position along the sweep axis:
//   lamps close together activate nearly simultaneously,
//   lamps far apart have proportionally more delay.
//
// For "forward" effects (top-down, left-right): min position = time 0, max = sweepMs.
// For "reverse" effects (bottom-up, right-left): max position = time 0, min = sweepMs.

static void computeActivationTimes(byte simpleEffect, unsigned long sweepMs) {
  // Find min and max axis coordinates across all 47 lamps
  unsigned int minPos = 0xFFFF;
  unsigned int maxPos = 0;
  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    unsigned int pos = getAxisPosition(i, simpleEffect);
    if (pos < minPos) minPos = pos;
    if (pos > maxPos) maxPos = pos;
  }

  unsigned int range = maxPos - minPos;
  if (range == 0) range = 1;  // Prevent division by zero

  bool reverse = (simpleEffect == LAMP_EFFECT_SWEEP_BOT_UP
    || simpleEffect == LAMP_EFFECT_SWEEP_RIGHT);

  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    unsigned int pos = getAxisPosition(i, simpleEffect);
    unsigned long timeMs = ((unsigned long)(pos - minPos) * sweepMs) / range;

    if (reverse) {
      timeMs = sweepMs - timeMs;
    }

    lampActivateMs[i] = (unsigned int)(timeMs > 65535UL ? 65535U : timeMs);
  }
}

// ************************************************************************
// ************************** PUBLIC API **********************************
// ************************************************************************

void startLampEffect(byte effect, unsigned long sweepMs, unsigned long holdMs) {
  if (effect == LAMP_EFFECT_NONE) {
    // Caller passed NONE -- just clear internal state without restoring
    // (caller does not have pShift here; use cancelLampEffect(pShift) to restore).
    effectType = LAMP_EFFECT_NONE;
    effectSaveComplete = false;
    effectPhase = 0;
    effectPhaseDrain = false;
    effectBlackoutEndMs = 0;
    effectPauseEndMs = 0;
    return;
  }

  effectType = effect;
  effectSweepMs = sweepMs;
  effectHoldMs = holdMs;
  effectSaveComplete = false;
  effectPhaseDrain = false;
  effectBlackoutEndMs = 0;
  effectPauseEndMs = 0;
  memset(lampsProcessedBits, 0, sizeof(lampsProcessedBits));

  // Determine phase structure
  if (isFillDrainEffect(effect)) {
    getFillDrainPhaseTypes(effect, &effectPhase1Type, &effectPhase2Type);
    effectPhase = 1;
    effectPhaseDrain = false;  // Phase 1 is always fill (turn lamps on)
    computeActivationTimes(effectPhase1Type, sweepMs);
  } else if (isTwoPhaseEffect(effect)) {
    getPhaseTypes(effect, &effectPhase1Type, &effectPhase2Type);
    effectPhase = 1;
    computeActivationTimes(effectPhase1Type, sweepMs);
  } else {
    effectPhase1Type = effect;
    effectPhase2Type = LAMP_EFFECT_NONE;
    effectPhase = 1;
    computeActivationTimes(effect, sweepMs);
  }

  // Record start time AFTER computation so first tick timing is accurate
  effectStartMs = millis();
}

void cancelLampEffect(Pinball_Centipede* pShift) {
  if (effectType == LAMP_EFFECT_NONE) {
    return;  // No effect running -- nothing to do
  }

  // If the effect had already saved lamp state (effectSaveComplete == true),
  // restore it now so the lamps return to their pre-effect state.
  // The caller is about to write new lamp state (tilt blackout, attract lamps,
  // gameplay lamps, etc.), so restoring first gives them a clean baseline.
  if (effectSaveComplete && pShift != nullptr) {
    for (byte p = 0; p < 4; p++) {
      pShift->portWrite(p, savedPortState[p]);
    }
  }

  effectType = LAMP_EFFECT_NONE;
  effectSaveComplete = false;
  effectPhase = 0;
  effectPhaseDrain = false;
  effectBlackoutEndMs = 0;
  effectPauseEndMs = 0;
}

bool isLampEffectActive() {
  return (effectType != LAMP_EFFECT_NONE);
}

void processLampEffectTick(Pinball_Centipede* pShift, LampParmStruct* pLampParm) {
  if (effectType == LAMP_EFFECT_NONE || pShift == nullptr) {
    return;
  }

  // If a leading blackout pause is active, wait for it to expire.
  // All lamps are off (from the save/all-off block) and we hold them off so the
  // player sees the contrast before the fill sweep begins.
  if (effectBlackoutEndMs != 0) {
    if ((long)(millis() - effectBlackoutEndMs) < 0) {
      return;  // Still in blackout
    }
    // Blackout expired -- reset the sweep clock so activation times start from now
    effectBlackoutEndMs = 0;
    effectStartMs = millis();
  }

  // If a mid-point pause is active, wait for it to expire before continuing.
  if (effectPauseEndMs != 0) {
    if ((long)(millis() - effectPauseEndMs) < 0) {
      return;  // Still pausing with all lamps on
    }
    // Pause expired -- transition to drain phase now
    effectPauseEndMs = 0;
    effectPhase = 2;
    memset(lampsProcessedBits, 0, sizeof(lampsProcessedBits));
    computeActivationTimes(effectPhase2Type, effectSweepMs);
    effectPhaseDrain = true;
    effectStartMs = millis();
    return;
  }

  unsigned long elapsed = millis() - effectStartMs;

  // On the very first tick, save current lamp output state and turn all lamps off.
  // Uses portRead/portWrite (4 I2C transactions each) instead of 47 individual
  // digitalWrite calls (~9.4ms) to stay well within the 10ms tick budget.
  if (!effectSaveComplete) {
    for (byte p = 0; p < 4; p++) {
      savedPortState[p] = (unsigned int)pShift->portRead(p);
    }
    // Turn all lamps off by setting all output bits HIGH (HIGH = lamp OFF).
    // This affects ALL 64 pins on Centipede #1 (ports 0..3), not just the 47 lamp
    // pins. Non-lamp pins on Centipede #1 are unused outputs, so setting them HIGH
    // is harmless. The restore phase writes back the exact saved state.
    for (byte p = 0; p < 4; p++) {
      pShift->portWrite(p, 0xFFFF);
    }
    effectSaveComplete = true;

    // For fill-drain effects, start a leading blackout pause so the player sees
    // all lamps go dark before the fill sweep begins. Without this, lamps that
    // were already on (e.g. GI at the bottom) appear to stay on when the fill
    // starts from their direction -- the transition is invisible.
    if (isFillDrainEffect(effectType)) {
      unsigned long pauseMs = fillDrainPauseMs();
      if (pauseMs > 0) {
        effectBlackoutEndMs = millis() + pauseMs;
        return;  // Hold all lamps off; fill begins when blackout expires
      }
    }
  }

  // Process lamps whose activation time has been reached.
  // The bitfield prevents redundant I2C writes on subsequent ticks.
  // In fill mode (effectPhaseDrain == false): turn lamps ON.
  // In drain mode (effectPhaseDrain == true): turn lamps OFF.
  bool allDone = true;
  byte targetLevel = effectPhaseDrain ? HIGH : LOW;  // HIGH = OFF, LOW = ON
  for (byte i = 0; i < NUM_LAMPS_MASTER; i++) {
    if (!isLampProcessed(i)) {
      if (elapsed >= (unsigned long)lampActivateMs[i]) {
        pShift->digitalWrite(pLampParm[i].pinNum, targetLevel);
        markLampProcessed(i);
      } else {
        allDone = false;
      }
    }
  }

  // Once all lamps have been processed for this phase, decide what to do next.
  if (allDone) {
    // Two-phase effect (sweep or fill-drain): transition from phase 1 to phase 2
    if (effectPhase == 1 && effectPhase2Type != LAMP_EFFECT_NONE) {

      if (isFillDrainEffect(effectType)) {
        // Fill-drain: all lamps are now on. Start a mid-point pause so the player
        // can appreciate the fully-lit playfield before the drain phase begins.
        unsigned long pauseMs = fillDrainPauseMs();
        if (pauseMs > 0) {
          effectPauseEndMs = millis() + pauseMs;
          return;  // Hold all lamps on; drain transition happens when pause expires
        }
        // Zero pause: transition immediately (same as before)
        effectPhase = 2;
        memset(lampsProcessedBits, 0, sizeof(lampsProcessedBits));
        computeActivationTimes(effectPhase2Type, effectSweepMs);
        effectPhaseDrain = true;
      } else {
        // Two-phase sweep: phase 2 is another fill. Turn all lamps off first.
        effectPhase = 2;
        memset(lampsProcessedBits, 0, sizeof(lampsProcessedBits));
        computeActivationTimes(effectPhase2Type, effectSweepMs);
        for (byte p = 0; p < 4; p++) {
          pShift->portWrite(p, 0xFFFF);
        }
      }

      effectStartMs = millis();
      return;
    }

    // Single-phase effect, or phase 2 complete: wait for hold time, then restore.
    if (elapsed >= effectSweepMs + effectHoldMs) {
      for (byte p = 0; p < 4; p++) {
        pShift->portWrite(p, savedPortState[p]);
      }
      effectType = LAMP_EFFECT_NONE;
      effectSaveComplete = false;
      effectPhase = 0;
      effectPhaseDrain = false;
    }
  }
}