// PINBALL_LAMP_EFFECTS.H Rev: 03/11/26.
// All 47 lamps have X/Y coordinates (mm from left edge and top edge).
// Effects are non-blocking: call processLampEffectTick() once per 10ms loop tick.
// The engine saves/restores real lamp state so gameplay lamps are not corrupted.

#ifndef PINBALL_LAMP_EFFECTS_H
#define PINBALL_LAMP_EFFECTS_H

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <Pinball_Consts.h>

// Forward declaration needed by cancelLampEffect() and processLampEffectTick().
class Pinball_Centipede;

// *** EFFECT TYPE CONSTANTS ***
const byte LAMP_EFFECT_NONE           = 0;
const byte LAMP_EFFECT_SWEEP_TOP_DOWN = 1;  // Top of playfield to bottom (increasing Y from top)
const byte LAMP_EFFECT_SWEEP_BOT_UP   = 2;  // Bottom to top (decreasing Y from bottom)
const byte LAMP_EFFECT_SWEEP_LEFT     = 3;  // Left to right (increasing X)
const byte LAMP_EFFECT_SWEEP_RIGHT    = 4;  // Right to left (decreasing X)
const byte LAMP_EFFECT_SWEEP_DOWN_UP  = 5;  // Top-down sweep, then bottom-up sweep (two phases)
const byte LAMP_EFFECT_SWEEP_UP_DOWN  = 6;  // Bottom-up sweep, then top-down sweep (two phases)
const byte LAMP_EFFECT_SWEEP_LR_RL    = 7;  // Left-right sweep, then right-left sweep (two phases)
const byte LAMP_EFFECT_SWEEP_RL_LR    = 8;  // Right-left sweep, then left-right sweep (two phases)
// Fill-then-drain effects: phase 1 lights lamps progressively, phase 2 turns them OFF progressively.
// The visual is a wave that fills the playfield, then recedes.
const byte LAMP_EFFECT_FILL_UP_DRAIN_DOWN  =  9;  // Fill bottom-to-top, drain top-to-bottom
const byte LAMP_EFFECT_FILL_DOWN_DRAIN_UP  = 10;  // Fill top-to-bottom, drain bottom-to-top
const byte LAMP_EFFECT_FILL_LR_DRAIN_RL    = 11;  // Fill left-to-right, drain right-to-left
const byte LAMP_EFFECT_FILL_RL_DRAIN_LR    = 12;  // Fill right-to-left, drain left-to-right

// *** LAMP POSITION STRUCTURE (stored in PROGMEM) ***
// Coordinates in millimeters from playfield edges:
//   x_mm: distance from LEFT edge of playfield
//   y_mm: distance from TOP edge of playfield (shooter end = small Y, drain end = large Y)
struct LampPositionDef {
  unsigned int x_mm;
  unsigned int y_mm;
};

// PROGMEM position table (indexed by lamp index 0..46)
extern const LampPositionDef lampPositions[NUM_LAMPS_MASTER] PROGMEM;

// *** EFFECT ENGINE API ***

// Start a lamp effect. All playfield lamps are saved and turned off, then each
// lamp turns on at a time proportional to its position along the sweep axis.
// sweepMs = total duration from first lamp ON to last lamp ON (per phase).
// holdMs = how long all lamps stay on (or off for drain phase) before restoring.
//   For single-phase effects: holdMs is the pause after all lamps are lit.
//   For two-phase sweep effects (DOWN_UP, etc.): holdMs applies after phase 2.
//   For fill-then-drain effects (FILL_UP_DRAIN_DOWN, etc.): holdMs applies
//     after phase 2 finishes (all lamps off), before restoring original state.
// sweepMs applies to EACH phase independently.
// If an effect is already running, it is replaced by the new one.
void startLampEffect(byte effectType, unsigned long sweepMs, unsigned long holdMs);

// Cancel any running effect and immediately restore the saved lamp state.
// Pass the Centipede pointer so the engine can write back the saved port state.
// If no effect is running, this is a no-op.
// IMPORTANT: Call this before any code that directly writes lamp state
// (handleFullTilt, teardownGame, setAttractLamps, setInitialGameplayLamps, etc.)
// to prevent the effect engine from overwriting the new lamp state on its next tick.
void cancelLampEffect(Pinball_Centipede* pShift);

// Returns true if a lamp effect is currently running.
bool isLampEffectActive();

// Call once per 10ms tick from loop(). Drives the effect state machine.
void processLampEffectTick(Pinball_Centipede* pShift, LampParmStruct* pLampParm);

#endif