# 1954 Williams Screamo - Modernized Control System Overview
Rev: 12-05-25.

## 1. Introduction

Screamo was originally a 1954 single-player Williams pinball machine, and has since been cosmetically and mechanically restored. This project modernizes the control system while preserving the original gameplay experience as much as possible, plus adding a new Enhanced game mode with modern features.

Originally, all rules, scoring, and features were handled by electromechanical (EM) devices:

- Score Motor: advanced scores in increments of 50,000 or 500,000 points.
- Relay Banks: tracked progress toward features.
- Score Units: mechanical stepper units that advanced and displayed score via illuminated digits in the head.
- Buttons and switches: responded to player and ball actions.
- Playfield lamps: indicated active features and feature progress.
- Coils: moved the ball on the playfield (pop bumper, side kickouts, slingshots, etc.).
- Relays: handled game state and scorekeeping.

This project replaces most of the EM logic with two Arduino Mega 2560 R3 boards (Master and Slave) while preserving the original feel, sounds, and behavior as much as possible, plus adding a new Enhanced game mode and a rich diagnostic system.

---

## 2. High-Level Architecture

- Master Arduino (in cabinet)
  - Owns all game rules and most real-time behavior.
  - Controls playfield and cabinet coils, playfield lamps, shaker motor, Tsunami audio system, ball handling, and Score Motor / EM sound devices.
  - Talks to the Slave via RS-485 messages.
  - Manages Diagnostic mode, multi-player logic in Enhanced mode, and mode selection (Original / Enhanced / Impulse).

- Slave Arduino (in head)
  - Owns score display and head hardware.
  - Controls devices in the head:
    - Score lamps (10K / 100K / 1M lamps).
    - Tilt and GI lamps in the head.
    - Three bells (10K, 100K, Select).
    - 10K Unit (for sound only).
    - Credit Unit (add/deduct credits).
  - Receives score, credit, and state commands and queries via RS-485 from Master.
  - Responsible for realistic timing of score updates and bells, matching the original Score Motor behavior.
  - Has its own 20x4 LCD display for diagnostics.

- Communication
  - Master sends high-level commands to Slave (for example: "add 50K to Player 1 score", "set TILT lamp ON", "set head GI OFF").
  - Slave translates those into stepper pulses, bell rings, and lamp updates, timed to emulate original mechanical behavior.

---

## 3. Original vs Modern Hardware

### 3.1 Original Mechanisms Still Used Normally

These remain and are still used as active game mechanisms:

- Flipper buttons and flippers (mechanically upgraded, but same function) (playfield).
- Pop bumper and dead bumpers (playfield).
- Left and right side kickouts (playfield).
- Left and right slingshots (playfield).
- Credit Unit (head).
- Ball tray release (original coil) (playfield).
- Lamps (playfield and head).
- Most switches (head, playfield and cabinet).

### 3.2 Original Mechanisms Used Only for Sound Effects

These remain physically present and are driven only for sound authenticity (no game logic is derived from them):

- Three scoring bells (10K, 100K, Select) (head).
- 10K Score Unit (head).
- Knocker (cabinet).
- Score Motor (cabinet).
- Selection Unit (cabinet).
- Relay Reset Unit (cabinet).  The original game has two Relay Reset Banks, but we retain only one; we will fire it twice when the original game would have fired both once.

The actual scoring and rule decisions are all handled in software.

### 3.3 New / Modern Hardware

- Two Arduino Mega 2560 R3 boards
  - Master (in cabinet): controls playfield and cabinet devices, game rules, modes, and diagnostics.
  - Slave (in head): controls head devices, score display, and head diagnostics via commands from Master.

- Centipede shift register I/O expanders  
  - Controlled by Arduinos via I2C commands.
  - Input Centipede (cabinet): reads switch status (playfield and cabinet switches).
  - Output Centipede (cabinet): drives 6.3vac playfield lamps (via modern cube relay modules).
  - Output Centipede (head): drives 12vdc head lamps (via modern cube relay modules).

- Relay modules (cabinet and head)
  - Modern relays, digitally controlled by Centipede outputs, in turn provide power to lamps.
  - Pulling a Centipede output LOW effectively turns on a lamp (or group of lamps in the case of head G.I.)
  - Switch 6.3 VAC to playfield lamps (cabinet).
  - Switch 12 V DC to head lamps (head).
  - Software doesn't need to know about relay details; only that controlling Centipede outputs (indirectly) controls lamps.

- MOSFET drivers
  - Controlled by Arduinos PWM and digital pins to fire coils and control motors with adjustable power and timing (head and cabinet).

- Shaker Motor (in cabinet.)
  - Controlled by Master via PWM MOSFET.
  - For tactile feedback in Enhanced mode; speed and duration controlled by PWM (cabinet).

- Audio System (in cabinet.)
  - Controlled by Master via RS-232 serial commands.
  - Tsunami WAV Trigger board (controlled by Master) (cabinet).
  - Amplifier and speaker (cabinet).
  - Plays simultaneous voice, music, and sound effects, primarily in Enhanced mode.

- LCD Displays (on inside of coin door and in head.)
  - Controlled by Arduinos via RS-232 serial commands.
  - Two 20x4 (4 lines of 20 characters each) Digole LCDs:
    - One connected to the Master (inside coin door) for status and diagnostics.
    - One connected to the Slave (in the head) for head diagnostics.

- Diagnostics Module (on inside of coin door.)
  - Read by Master via Centipede inputs.
  - Four diagnostic buttons in the coin door: BACK, LEFT, RIGHT, SELECT.
  - Used in Game Over / Diagnostic mode; also used in-game for Tsunami volume (LEFT/RIGHT).

- New Ball Release Mechanism (from ball trough to ball lift) (below playfield).
  - Up/down post releases individual balls from the trough (below the tray) into the ball lift.
  - Controlled by Master via PWM MOSFET.

- Ball presence switch at base of Ball Lift mechanism (below playfield).
  - Switch read by Master via Input Centipede.

---

## 4. Game Modes

Screamo runs in several modes. Unless otherwise noted, Master is mode authority and informs Slave via RS-485.

### 4.1 Original Mode

- Triggered by: Single press of Start button (with at least one credit).
- Single-player only.
- Rules and scoring match the original Screamo rules.
- Modernized flippers:
  - Independent control of each flipper.
  - Flippers can be held up indefinitely while the button is held.
- EM sound devices used:
  - Score Motor, 10K Unit, Selection Unit, Relay Reset Bank.
- Goal: Make it feel like playing an unmodified original Screamo.
- Shaker motor and Tsunami audio are not used in this mode.

### 4.2 Enhanced Mode

- Triggered by: Double-tap of Start button (with at least one credit).
- A new rule set, uses:
  - Tsunami audio system.
  - Shaker motor.
  - Ball tray release coil and new ball handling features.
- Multi-player support:
  - After starting Enhanced mode, and before any points are scored, the player may press Start up to three more times to add Players 2, 3, and 4 (one credit per player).
- Theme:
  - Roller-coaster / amusement park flavor.
  - Voice, music, and special effects.
  - Musical theme can be toggled between Callipe/Circus music and Surf Rock (settable in Diagnostics Settings).
- Features (planned / current):
  - Up to four players.
  - Special game modes and scoring goals (details to be specified separately).
  - Ball save for first N seconds after the first score on each ball (N defined by a constant).
  - Shaker motor events triggered on certain happenings.
  - Extra voice prompts, music, jackpots, etc. (details to be specified separately).

### 4.3 Impulse Mode

- Triggered by: Triple-tap of Start button (with at least one credit).
- Single-player only.
- Rules identical to Original mode except for flipper behavior:
  - Impulse flippers:
    - Pressing either flipper button causes both flippers to fire briefly and immediately drop.
    - Flippers cannot be held up.
    - A new press is required before flippers can be fired again.
- Intended to exactly reproduce the original 1954 impulse-flipper behavior and gameplay.

### 4.4 Game Over / Attract Mode

- Default mode on power-up and at the end of each game.
- Typical behavior:
  - Head and playfield GI on.
  - TILT lamp off.
  - Prior score displayed (or re-displayed from non-volatile storage).
  - Optionally: an attract-mode show:
    - Flashing score lamps and playfield lamps.
    - Voice, music, and sound effects (if desired).
- In this mode, Master:
  - Waits for Start button presses, coin insertions, knock-off button, or Diagnostic button presses.

### 4.5 Tilt Behavior

- When the game is tilted in any playing mode:
  - Flippers are disabled.
  - All lights momentarily turn off.
  - Any balls in side kickouts are ejected.
  - TILT lamp is turned on and flashes for about 4 seconds.
  - Game waits for all in-play balls to drain.
- After that:
  - If more balls remain for the current player, continue with the next ball.
  - For multi-player Enhanced games, advance to the next player if appropriate.
  - If this was the last ball of the last player, game transitions to Game Over.

### 4.6 Diagnostic Mode (Overview)

- Diagnostic mode is not a normal gameplay mode but is entered from Attract mode:
  - When player presses the SELECT button during MODE_ATTRACT, Master enters Diagnostic mode.
- There are four diagnostic buttons:
  - BACK (exit / go up a level).
  - LEFT and RIGHT (navigate or -/+ depending on context).
  - SELECT (enter/toggle/trigger depending on context).
- The Master LCD shows:
  - ROW 1: "*** DIAGNOSITICS ***"
  - ROW 2: Name of the current test or option.
  - ROW 3: Name of device or setting being tested/adjusted.
  - ROW 4: Current value or instructions.
- Diagnostic sub-tests:
  - "SETTINGS": LEFT and RIGHT buttons move through adjustable settings; must press SELECT to change a setting.
    - VOL MAIN  : Tsunami main volume adjustment (-40dB to 0dB)       i.e. "VOL MAIN: -10dB"
    - SFX OFFSET: Tsunami SFX volume offset (-40dB to +40dB)          i.e. "SFX OFFSET: -10dB"
    - MUS OFFSET: Tsunami Music volume offset (-40dB to +40dB)        i.e. "MUS OFFSET: -10dB"
    - COM OFFSET: Tsunami Commentary volume offset (-40dB to +40dB)   i.e. "COM OFFSET: -10dB"
    - THEME: Calliope vs Surf Rock.                                   i.e. "THEME: CIRCUS" or "THEME: SURF"
    - LAST SONG: Last played Tsunami track on exit.                   i.e. "LAST SONG: 005"
    - BALL SAVE: Set ball save duration in Enhanced mode (in seconds) i.e. "BALL SAVE: 10s"
    - HURRY UP 1-6: Set hurry-up times for Enhanced mode (in seconds) i.e. "HURRY UP 1: 15s"
    - ORIG REPLAY 1-5: Set replay scores for Original mode (0.999)    i.e. "ORIG REPLAY 3: 350"
    - ENH REPLAY 1-5: Set replay scores for Enhanced mode (0.999)     i.e. "ENH REPLAY 2: 500")
  - "LAMP TESTS": LEFT and RIGHT buttons move to previous/next lamp, which is turned on, and previous selection turned back off.  SELECT button does nothing.
    - Must include both playfield and head lamps.
    - i.e. "08 BUMPER S"
  - "SWITCH TESTS": LEFT and RIGHT buttons move to previous/next switch.  SELECT button does nothing.
    - Must include both playfield and head switches.
    - i.e. "37 FLIPPER L" (LCD line 3)
    - When selected switch is closed, LCD line 4 shows "CLOSED" and speaker beeps continuously until released; when open, shows "OPEN".
    - Current switch state shown as "OPEN" or "CLOSED".
  - "COIL/MOTOR TESTS": LEFT and RIGHT buttons move to previous/next coil or motor.  SELECT button fires the coil briefly or runs the motor as long as the SELECT button is being pressed.
    - Must include all coils and motors in cabinet and head.
    - i.e. "03 POP BUMPER"
  - "AUDIO TESTS": LEFT and RIGHT buttons move to previous/next Tsunami 4-digit audio track.  SELECT button plays the selected track once.
    - Audio track descriptions are limited to 15 chars (to fit on LCD) and stored in a lookup table in PROGMEM.
    - i.e. "0005 LETS PLAY SCRMO"

- While in play: During an active game (ENHANCED mode), LEFT and RIGHT buttons are also used to change the Main Volume (and not the offset volumes).  The new value is applied immediately and stored at EEPROM_ADDR_TSUNAMI_GAIN for persistence across power cycles.

---

## 5. Score Motor and Timing Model

The original Score Motor is used for sound and timing reference only. Actual scores are computed in software and displayed via lamps, driven by Slave.

Master should start the Score Motor, and potentially fire the Selection Unit and/or the Relay Reset Bank for the audible/mechanical effect around the time it sends relevant messages to Slave, such as score updates.

Master should wait a realistic motor run duration (varies depending on score digits adjusted and other factors) before stopping motor.  But typically the score motor will run for 0, 1, 2, or 3 full 1/4 revolutions (each 882 ms) depending on the score adjustment being made.

### 5.1 Mechanical Timing

- Score Motor runs at 17 RPM.
- Motor behavior:
  - 1 revolution is divided into 4 quarters.
  - Each 1/4 revolution is 882 ms.
  - Each 1/4 revolution is divided into 6 slots:
    - Slots 1 through 5: potential score advances of 10K or 100K.
    - Slot 6: rest.
  - Each slot is 147 ms (882 ms / 6).

### 5.2 Scoring Behavior (10K and 100K)

Single-unit increments:

- Single 10K increment:
  - Handled as a non-motor pulse.
  - Fire 10K Unit coil once.
  - Fire 10K bell once.
  - NOTE: Some, but not all, 10K increments will also fire the Selection Unit coil via Master for sound.

- Single 100K increment:
  - Also non-motor.
  - Fire 10K unit coil (for sound).
  - Fire 10K bell.
  - Fire 100K bell.

Motor-paced batches:

- For 2 to 5 tens of thousands (20K to 50K):
  - Run one full 1/4 revolution:
    - Up to 5 advances spaced 147 ms apart.
    - One 147 ms rest slot at the end.
- For 2 to 5 hundreds of thousands (200K to 500K):
  - Same pattern, but each advance is +100K.
- For more than 5 tens or hundreds:
  - Repeat full cycles (each cycle is 5 advances plus 1 rest) until remaining units are less than or equal to 1.
  - A remaining single 10K or 100K is then handled as a non-motor single pulse.

### 5.3 Game Start / Relay Reset Timing

- Starting a new game:
  - Requires one 1/4 revolution for preliminaries:
    - Slot 1: Fire Relay Reset Bank.
    - Slot 2: Deduct a credit
    - Slot 3: Open the ball tray
    - Slot 4: Fire Relay Reset Bank again.
    - Slot 5: Release the 1M/100K rapid reset
  - Then one or two additional 1/4 revolutions for tens (0K to 90K) of score reset.
    - If the 10K Unit is already at zero, the Score Motor still runs for two full 1/4 revolutions (10 score steps) to walk the 10K unit back up to zero.
- During game play, firing the Relay Reset Bank consumes a full 1/4 revolution (6 slots, 882 ms) when score adjustments will not take place.
- It is possible that other non-scoring operations may occur in the same 1/4 revolution as Relay Reset.

### 5.4 Software Requirements

- Slave is responsible for:
  - Timing score lamp updates and bell sounds to emulate Score Motor behavior.
  - When instructed to add, for example, 50K, it should:
    - Run 5 score advances at 147 ms intervals.
    - Then a 147 ms rest (silent).
- Master is responsible for:
  - Turning the Score Motor and other devices on and off at the appropriate times, for the appropriate duration, so their sounds match the score updates.

---

## 6. Persistent Score and Reset Logic

Planned or needed functionality:

### 6.1 Persistent Score Storage

- Use non-volatile storage (EEPROM) on Master Arduino to save scores.
- Saving previous score when:
  - A game ends, or
  - Periodically during games (for example every N iterations of the 10 ms loop equal to about 15 to 30 seconds).
- On power-up:
  - Restore the last saved score and display it in Game Over / Attract mode.
- On Game Over:
  - Save the final score(s) to non-volatile storage.
  - For multi-player Enhanced games, display each player's score in sequence.
    - Briefly flash the 1/2/3/4M lamp for player number, followed by their score, for each player.
- Realistic reset to zero when starting a new game:
  - Use 10K and 100K logic to "walk" the score back to zero in a visually and audibly realistic way.
  - 'startNewGame()' should:
    - Reset internal score variables.
    - Command Slave to step the score down to 0 using realistic timing.

### 6.2 Hardware Startup Behavior

- On power-up or hardware reset of Master or Slave:
  - Both Arduinos should initialize their internal state to GAME_OVER mode.
  - Master should read persistent score from EEPROM and command Slave to display it.
  - All outputs (coils, lamps) should be turned off initially.
  - Head lamps (GI and Tilt) should be turned on.
  - TILT lamp should be off.

### 6.3 Hardware Reset Behavior

- At any time during gameplay, game over, or diagnostics:
  - Pressing and holding the KNOCKOFF button for more than 1 second should trigger a hardware reset of both Master and Slave Arduinos:
    - Master sends a reset command to Slave via RS-485.
    - Master and Slave both immediately release all coils, stop all motors, turn off all lamps.
    - Perform software reset.  Result will be as if power had been recycled.
  - Pressing the KNOCKOFF button for less than one second, one or more times, has other functions (for example, adding credits).

---

## 7. Hardware Inventory - Head (Slave Arduino)

The Slave controls devices in the head via MOSFETs, Centipede outputs, and direct inputs.

### 7.1 Coils and MOSFET Outputs

Each is driven by a MOSFET from a PWM pin:

- 'DEV_IDX_CREDIT_UP' - Credit Unit step-up coil  
  One pulse adds one credit.

- 'DEV_IDX_CREDIT_DOWN' - Credit Unit step-down coil  
  One pulse removes one credit.

- 'DEV_IDX_10K_UP' - 10K Unit step-up coil (sound only)  
  One pulse steps the 10K unit one position and should fire whenever 10K lamps change.

- 'DEV_IDX_10K_BELL' - 10K bell coil  
  One pulse rings the 10K bell once, every time 10K score changes.

- 'DEV_IDX_100K_BELL' - 100K bell coil  
  One pulse rings the 100K bell once, every time 100K score changes.

- 'DEV_IDX_SELECT_BELL' - "Select" bell coil  
  Used at various points in the game (not tied directly to score change).

- 'DEV_IDX_LAMP_SCORE' - Score lamp power  
  PWM controls overall brightness of all 27 score lamps (10K / 100K / 1M).

- 'DEV_IDX_LAMP_HEAD_GI_TILT' - Head GI and Tilt lamp power  
  PWM controls brightness for head GI and Tilt.

### 7.2 Head Switch Inputs

Read via direct input pins on Slave:

- 'PIN_IN_SWITCH_CREDIT_EMPTY' - Credit unit empty switch  
  Opens when credits are zero; prevents further step-down; informs Master that credits are empty.

- 'PIN_IN_SWITCH_CREDIT_FULL' - Credit unit full switch  
  Opens when credit unit is full; prevents further step-up.

### 7.3 Head Lamps (via Relay Modules via Centipede Outputs)

Relays switch 12 VDC to the lamps; Centipede outputs control the relays.

- Tilt lamp.
- Head GI lamps.
- Score lamps (27 total):
  - 9 x 10K: 10K, 20K, ..., 90K  
  - 9 x 100K: 100K, 200K, ..., 900K  
  - 9 x 1M: 1M, 2M, ..., 9M

Controlled by Slave based on RS-485 commands from Master.  
Slave must:
- Update score lamps at realistic Score Motor speeds.
- Fire 10K unit and bells appropriately as lamp pattern changes.

### 7.4 Slave LCD

- 20x4 Digole LCD connected to Slave.
- Used for:
  - Head diagnostics.
  - Displaying test names, coil names, lamp names, etc.

---

## 8. Hardware Inventory - Cabinet and Playfield (Master Arduino)

### 8.1 Coils, Motors, and MOSFET Outputs

All controlled by Master via MOSFETs and PWM:

- 'DEV_IDX_POP_BUMPER' - Pop bumper coil.  For "E" bumper only.
- 'DEV_IDX_KICKOUT_LEFT' - Left side kickout coil.
- 'DEV_IDX_KICKOUT_RIGHT' - Right side kickout coil.
- 'DEV_IDX_SLINGSHOT_LEFT' - Left slingshot coil.
- 'DEV_IDX_SLINGSHOT_RIGHT' - Right slingshot coil.
- 'DEV_IDX_FLIPPER_LEFT' - Left flipper coil (supports hold).
- 'DEV_IDX_FLIPPER_RIGHT' - Right flipper coil (supports hold).
- 'DEV_IDX_BALL_TRAY_RELEASE' - Original ball tray release coil (can be held).
- 'DEV_IDX_SELECTION_UNIT' - Selection Unit coil (sound only).
- 'DEV_IDX_RELAY_RESET' - Relay Reset Bank coil (sound only).
- 'DEV_IDX_BALL_TROUGH_RELEASE' - New up/down post ball release coil.
- 'DEV_IDX_MOTOR_SHAKER' - Shaker motor (PWM for speed and duration).
- 'DEV_IDX_KNOCKER' - Knocker coil (one pulse per knock).
- 'DEV_IDX_MOTOR_SCORE' - Score Motor control (sound only, can be held on).

### 8.2 Cabinet Switches (via Centipede Inputs)

All cabinet switch closures, including the flipper buttons, now enter Master through Centipede #2 (switch indexes 64–127). Master still gives the flipper buttons a faster-processing path in software, but no longer wires them directly to Arduino GPIO pins.

- 'SWITCH_IDX_START_BUTTON' – Start button (supports single / double / triple tap detection).
- 'SWITCH_IDX_DIAG_1' to 'SWITCH_IDX_DIAG_4' – Diagnostic buttons (BACK, LEFT, RIGHT, SELECT).
- 'SWITCH_IDX_KNOCK_OFF' – Hidden knock-off switch; can add credits or force reset.
- 'SWITCH_IDX_COIN_MECH' – Coin mech switch; adds credits.
- 'SWITCH_IDX_BALL_PRESENT' – New switch detecting ball at bottom of lift.
- 'SWITCH_IDX_TILT_BOB' – Tilt bob switch.
- 'SWITCH_IDX_FLIPPER_LEFT_BUTTON' / 'SWITCH_IDX_FLIPPER_RIGHT_BUTTON' – Flipper buttons routed through Centipede #2 (indices 37/38 in 'switchParm[]')

### 8.3 Playfield Switches (via Centipede Inputs)

- Bumper switches: 'SWITCH_IDX_BUMPER_S, C, R, E, A, M, O'  
  Close when ball hits corresponding letter bumper.

- Kickout switches detect when ball has landed in kickout saucer:  
  'SWITCH_IDX_KICKOUT_LEFT', 'SWITCH_IDX_KICKOUT_RIGHT'.

- Slingshot switches:  
  'SWITCH_IDX_SLINGSHOT_LEFT', 'SWITCH_IDX_SLINGSHOT_RIGHT'.

- Hat rollover switches:  
  'SWITCH_IDX_HAT_LEFT_TOP', 'SWITCH_IDX_HAT_LEFT_BOTTOM',  
  'SWITCH_IDX_HAT_RIGHT_TOP', 'SWITCH_IDX_HAT_RIGHT_BOTTOM'.

- Side targets:
  - 'SWITCH_IDX_LEFT_SIDE_TARGET_2' to 'SWITCH_IDX_LEFT_SIDE_TARGET_5'
  - 'SWITCH_IDX_RIGHT_SIDE_TARGET_1' to 'SWITCH_IDX_RIGHT_SIDE_TARGET_5'  
  (There is no 'LEFT_SIDE_TARGET_1'.)

- Gobble switch:  
  'SWITCH_IDX_GOBBLE' - ball enters gobble hole and drains.

- Drain switches:  
  'SWITCH_IDX_DRAIN_LEFT', 'SWITCH_IDX_DRAIN_CENTER', 'SWITCH_IDX_DRAIN_RIGHT'.

### 8.4 Playfield Lamps (via Relay Modules via Centipede Outputs)

Relays switch 6.3 VAC to the lamps; Centipede outputs control the relays.

Lamp groups and examples:

- GI lamps ('LAMP_GROUP_GI'):
  - 'LAMP_IDX_GI_LEFT_TOP', etc. (8 individually controlled GI lamps).

- Bumper lamps ('LAMP_GROUP_BUMPER'):
  - 'LAMP_IDX_S, C, R, E, A, M, O' (7).

- White awarded score lamps ('LAMP_GROUP_WHITE'):
  - 'LAMP_IDX_WHITE_1' to 'LAMP_IDX_WHITE_9'.

- Red spotted score lamps ('LAMP_GROUP_RED'):
  - 'LAMP_IDX_RED_1' to 'LAMP_IDX_RED_9'.

- Hat lamps ('LAMP_GROUP_HAT'):
  - 'LAMP_IDX_HAT_LEFT_TOP', etc. (4).

- Kickout lamps ('LAMP_GROUP_KICKOUT'):
  - 'LAMP_IDX_KICKOUT_LEFT', 'LAMP_IDX_KICKOUT_RIGHT'.
  - Indicate "spot number when lit"; awarding spotted number when ball enters lit kickout.

- Special when lit:
  - 'LAMP_IDX_SPECIAL' - lit when gobble hole awards special.

- Gobble score lamps ('LAMP_GROUP_GOBBLE'):
  - 'LAMP_IDX_GOBBLE_1' to 'LAMP_IDX_GOBBLE_5' - indicate number of balls in gobble hole.

- Spot number lamps (side drain rollovers):
  - 'LAMP_IDX_SPOT_NUMBER_LEFT', 'LAMP_IDX_SPOT_NUMBER_RIGHT'.
  - When lit, drain at corresponding rollover awards spotted number.

### 8.5 Master LCD and Audio

- Master 20x4 LCD (inside coin door):
  - Displays current mode, diagnostics screens, device names, and result messages.

- Tsunami WAV Trigger (with amp and speaker):
  - Controlled by Master via RS-232 serial commands.
  - Plays voice, music, and sound effects in Enhanced mode.
  - Volume can be adjusted via LEFT and RIGHT Diagnostic buttons during play.

---

## 9. Software Architecture Overview

### 9.1 Main Loop Timing

- For Original, Enhanced, and Impulse Mode games, we'll use a 10 ms main loop on both Master and Slave:
  - On each loop:
    - Read inputs (Centipede and/or direct pins).
    - Update outputs (coils, lamps, audio commands).
    - Update timers and state machines.
    - Keep track of min and max loop execution time for profiling.
- Between loop iterations:
  - Master Flipper input pins can be polled continuously (or at higher priority) outside the 10 ms loop to minimize flipper latency.

### 9.2 Mode and State Management

- On power-up:
  - Both Master and Slave default to GAME_OVER mode.
- At end of each game:
  - Return to GAME_OVER.
- From Game Over:
  - If coin inserted or knock-off pressed:
    - Increment credits (up to a maximum).
    - Fire knocker.
  - If Diagnostic SELECT button pressed:
    - Enter Diagnostic mode.
  - If Start pressed with credits > 0:
    - Detect single / double / triple tap:
      - Single: MODE_ORIGINAL.
      - Double: MODE_ENHANCED.
        - In Enhanced mode, Start can be pressed up to three more times to add Players 2, 3, and 4 (one credit each).
        - New players may not be added after any points have been scored.
      - Triple: MODE_IMPULSE.
      - Deduct credits appropriately.
      - Single-tapping Start once a point has been scored in Original or Impulse mode ends the game and starts a new one in the same mode (if credits remain).
      - Single-tapping Start in Enhanced mode after a point has been scored has no effect.  Game must be reset to start over.

### 9.3 Multi-Player Indication (Enhanced Mode)

- For multi-player Enhanced games:
  - While waiting for each player to score their first point:
    - Flash the appropriate 1M lamp (1M, 2M, 3M, or 4M) for that player.
    - All other score lamps off.
  - After the first point is scored:
    - Stop flashing; show full score lamp pattern for that player.
- For Enhanced Game Over when only 1 player:
  - Display that player's score as usual.
- For Enhanced Game Over with more than 1 player:
  - Display each player's score in sequence:
    - Flash the appropriate 1/2/3/4M lamp for player number for 1 second.
    - Show that player's score for 2 seconds.
    - Score lamps off for 0.5 seconds.
    - Move to next player until all players have been displayed, and repeat.
---

## 10. Mode Rule Summaries

### 10.1 Original Mode

- Single player only.
- Normal flippers (independent, holdable).
- Classic Screamo scoring and features.
- 5 balls per game; no extra balls.
- Game ends after ball 5 drains.
- 10K Unit, Relay Reset Bank, Selection Unit, and Score Motor are used purely for sound realism.

### 10.2 Impulse Mode

- Same as Original mode, but:
  - Flippers behave as impulse flippers:
    - Press either button: both flippers fire once briefly and then drop.
    - No hold; must release and press again to flip again.

### 10.3 Enhanced Mode

- Enhanced Mode is generally a superset of Original/Impulse mode; i.e. existing Original/Impulse features are preserved where possible, but new features are added.
- Up to 4 players.
- Uses:
  - Tsunami audio (voice, music, sound effects).
  - Shaker motor.
  - New ball save and rule set.
- Rules (high-level, to be elaborated separately):
  - Roller-coaster / Screamo theme integration.
  - Ball save for first N seconds per ball after first score.
  - New scoring goals (for example collecting letters, jackpots, mode progression).
  - Potential use of gobble / special / kickouts / bumpers for modes and bonuses.

---

## 11. Diagnostics and Power Constraints

### 11.1 Lamp Power Budget

To avoid over-current, "All Lamps" test must be split into groups:

- Head (12 V DC, assumed 100 mA per lamp):
  - Head GI + Tilt: 13 x 100 mA = 1.3 A
  - Head digits (score lamps): 27 x 100 mA = 2.7 A
  - Total Head: ~4.0 A

- Playfield (6.3 V AC):
  - Playfield GI: 8 x #44 lamps ~ 2.0 A
  - Playfield bumper lamps: 7 x #51 ~ 1.4 A
  - Red and White inserts: 18 x #44 ~ 2.7 A
  - Other inserts: 14 x #44 ~ 2.1 A
  - Total Playfield: ~8.2 A

Therefore, All Lamps test is implemented as multiple sub-tests:

- HEAD G.I./TILT
- HEAD DIGITS
- P/F G.I.
- P/F BUMPERS
- P/F RED/WHITE
- P/F MISC INSERTS

### 11.2 Diagnostic Tests

Entry:

- From Game Over:
  - Press any Diagnostic button to enter Diagnostic mode.  All lamps turn off.

Navigation:

- BACK: go up a level and eventually exit back to Game Over (G.I. lamps back on).
- MINUS / PLUS: navigate through items at current level.
- SELECT: drill down or activate test.

#### 11.2.1 Lamp Tests

- Grouped lamps (for power management):
  - See groups listed above.
  - Typical behavior: Select to blink or turn on; BACK/MINUS/PLUS to change selection or exit.

- Individual lamps:
  - HEAD:
    - Use MINUS/PLUS buttons to cycle through each lamp; Coin Door LCD displays lamp name; lamp lights as confirmation.
  - PLAYFIELD:
    - Same as above for playfield lamps.

#### 11.2.2 Switch Tests

- As switches change state (open/close), LCD shows:
  - Switch name.
  - New state.

Implementation notes:

- Need to constantly monitor:
  - Master Arduino Flipper digital inputs.
  - Master Arduino Centipede inputs.
  - RS-485 query for head switch status (for example Credits Available).
- Tests must either exclude head Credits Full, or support an RS-485 query for its state.

#### 11.2.3 Coil and Motor Tests

- Use MINUS/PLUS to cycle through coil and motor names (both head and playfield).
- Press SELECT to energize briefly.
  - Score Motor runs until BACK, LEFT, RIGHT, or SELECT is pressed again.
  - Shaker Motor runs at default starting power
    - Press LEFT/RIGHT to adjust power level while running.
    - Press BACK or SELECT to stop.
- Press BACK to exit.

#### 11.2.4 Audio Tests

- Use MINUS/PLUS to cycle through Tsunami tracks.
- Press SELECT to play selected track.
- Playback stops when:
  - Track naturally ends, or
  - BACK/MINUS/PLUS/SELECT is pressed.

---

## 12. Original/Impulse Mode Rule Details

### 12.1 Switch and Lamp Overview

- Each playfield switch has a distinct behavior and score impact, depending on whether it is lit (if can be lit) and the status of the Selection Unit and other simulated electromechanical devices.

 - Original rules call for a five-ball game; no extra balls are awarded.  Because the Ball Trough Release mechanism is new, and because we have disabled an original gate that held balls drained via the Gobble Hole, we will use the new ball release mechanism to release five balls per game, less one if a ball is detected in the ball lift at game start.
   - If there are balls in either kickout at the start of the game, disable slingshot and flipper power, and eject balls and wait to see them in either the Gobble Hole or bottom drains before the first ball is released.
   - If a ball happens to be detected in the Ball Lift at the start of a game, we will call that Ball 1, and only release four additional balls from the trough.
   - If a ball happens to be in the Shooter Lane at the start of a game, we won't be able to see it and the player will effectively start with an extra ball.
 - NOTE: On original game, the 10K Bell is physically tied to the 10K Unit, so every 10K score increment rings the bell.  In this simulation, therefore, we will ring the 10K Bell on every 10K score increment.
 - On every 100K score increment, we will ring the 100K Bell (and also the 10K Bell, since 100K includes a 10K increment).
 - Each time a WHITE INSERT is lit (even if already lit), we will ring the Selection Bell.
 - Each time a CREDIT is added (coin mech or special), we will fire the Credit Unit step-up coil and fire the Knocker.

- Pressing Start button once does the following:
  - Turns on Score Motor.
  - Deducts one credit (if credits > 0). (Motor cycle 1, Slot 1)
  - Fires the Relay Reset Bank coil. (Slot 1)
  - Opens the ball tray. (Slot 1)
  - Fires the Relay Reset Bank coil again. (Slot 4)
  - Releases the 1M/100K rapid reset. (Slot 5)
  - Resets score to zero via Score Motor timing. (Motor cycles 2 and possibly 3)
  - Turns off Score Motor. (After score reset complete)
  - Fires Ball Release coil to release one ball into Ball Lift (if not already present.)
  - NOTE: The ball tray remains open until the first point is scored.

### 12.2 Replays

- There are a few ways to win a free game (add a credit via the Credit Unit step-up coil):
  - Each time the SCORE is updated, we will evaluate to see if a replay score has been achieved:
    - One replay each at 4.5M, 6M, 7M, 8M, and 9M points.
  - If all five balls drain via Gobble Hole (the SPECIAL WHEN LIT insert will be lit on the 5th ball), one replay is awarded.
  - Each time a SPOTTED NUMBER is awarded (lighting the corresponding white insert), we will evaluate the 1-9 Number Matrix for patterns that award replays:
      - Any 3-in-a-row (horizontal, vertical, or diagonal) awards 1 replay (only 1 3-in-a-row scores).
      - Four corners (1, 3, 7, 9) and center (5) awards 5 replays.
      - Making 1-3 awards 20 replays.

- ### 12.3 Scoring Rules

- Scoring Events:
  - Hitting either SLINGSHOT awards 10K points.  This is invariable.
  - Hitting a BUMPER awards 10K points and extinguishes its lamp, whether last lit bumper or not.
  - Hitting the last lit bumper, in addition to awarding 10K and extinguishing the lamp:
    - Simultaneously lights the spotted number (if not already lit).
    - Fires the Relay Reset Bank coil.
    - Re-lights all bumper lamps.
    - Rings the Selection Bell.
    - Awards 500K points via one motor cycle.
  - Hitting any of the nine SIDE TARGETS fires the Selection Unit, which:
    - Awards 10K points (via a switch on the Selection Unit).
    - Advances spotted (Red) number (via the Selection Unit).
    - Sometimes lights or extinguishes the Hat rollover lamps (via the Selection Unit).
      - See Selection Unit details below for the sequence of Red inserts and Hat lamp states.
  - Rolling over any HAT SWITCH awards 10K points, or 100K points if lit.
    - Rolling over a lit Hat does not extinguish it (because the Selection Unit controls Hat lamps, and only the nine Side Targets advance the Selection Unit).
  - Landing in either LEFT or RIGHT KICKOUTs awards 500K points.  If lit, also awards spotted number.
    - Landing in a lit Kickout does not extinguish it (because the 10K Unit controls Kickout lamps, and Kickouts do NOT advance the 10K Unit).
  - Draining via LEFT or RIGHT DRAIN ROLLOVER awards 100K points.  If lit, also awards spotted number.
    - Draining via a lit Drain Rollover does not extinguish it (because the 10K Unit controls Rollover lamps, and Rollovers do NOT advance the 10K Unit).
  - Draining via CENTER DRAIN ROLLOVER awards 500K points.
  - Ball draining via GOBBLE HOLE awards 500K points and (always) awards spotted number.
  - If all five balls drain via Gobble Hole, a replay is also awarded.


- Bumpers: There are seven bumpers, each labeled with a letter: S, C, R, E, A, M, O.  Bumpers are initially lit and extinguised when hit.

- The 1-9 Number Matrix:
  - The red numbered inserts (1-9) indicate the "spotted number" determined by the Selection Unit.
    - There is always exactly one red insert lit at any time.
  - The corresponding white numbered inserts (1-9) indicate the "awarded number", which is lit when various events occur:
    - When the ball hits a lighted "Spots Number When Lit" kickout or drain rollover switch.
    - When the ball drains via the gobble hole.
    - When all seven lit bumpers are hit.
      - When the last bumper is hit, 500K points are also awarded and the Selection Bell is rung.
  - Each time a spotted number is awarded, and the corresponding white insert is lit, the grid is examined for patterns that award replays:
    - When any 3-in-a-row (horizontal, vertical, or diagonal) is completed, a replay is awarded.  Only 1 3-in-a-row scores.
    - Making the four corners (1, 3, 7, 9) and center number (5) awards five additional replays.
    - Making 1-3 scores twenty additional replays.

- The 10K Unit is directly fired by the Bumper switches, the Hat rollover switches, the Slingshot swtiches, and when the Selection Unit is fired.
  - Bumpers, Hats, and Slingshots do NOT advance the Selection Unit.
  - The 10K Unit controls when the Left Kickout/Right Drain and Right Kickout/Left Drain lamps are lit ("Spots Number When Lit").
    - When the 10K Unit is at zero, Left Kickout and Right Drain lamps are lit.
    - When the 10K Unit is at 50K, Right Kickout and Left Drain lamps are lit.
    - These are independent of the Selection Unit.

- The Selection Unit is directly fired only by the 9 Side Target switches (simulated; in fact the Master fires the Selection Unit coil when any 9 Side Target is hit).
  - These devices do NOT fire the 10K Unit directly, but the Selection Unit fires the 10K Unit each time it advances (simulated.)
  - We use the Selection Unit coil for sound effects, but its behavior is simulated in software.
  - The Selection Unit (simulated) has two functions:
    1. Determines which of the red 1-9 numbered "Spot" inserts are lit (one of them is always lit).
       - Various scoring events award the "spotted number", which lights the corresponding white 1-9 numbered insert.
    2. Determines when the four Hat rollover lamps are lit (in Original mode, they always light together).
  - The Selection Unit has 50 steps/states, which equates to five sets of ten steps.  Beginning at a random state the steps are as follows:
    - 1st CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS LIT ON 8.
    - 2nd CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS NOT LIT.
    - 3rd CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS LIT ON 8.
    - 4th CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS NOT LIT.
    - 5th CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS LIT ON 8.
    - Begin 1st Cycle again...
  - Since only the 9 side targets fire the Selection Unit, there are three times in every 50 side target hits when the hats are lit.
    - Hitting any other target does not advance the Selection Unit, so the hats stay lit until a side target is hit again.
    - There is no correlation between the 10K score unit position and the Selection Unit position.

 - The Gobble Hole switch:
   - When any of the first four balls are gobbled, the hole awards 500K points and lights the corresponding 1-4 gobble score lamp.
   - When the fourth ball is gobbled, the Special When Lit lamp is also lit.
   - When the fifth ball is gobbled, a replay is scored.

---

## 13. Enhanced Mode Rule Details

Enhanced Mode rules are based on Original mode rules, with the following additions and changes:

### 13.1 General Features

- Up to four players can play in sequence.
- Shaker Motor is used to provide tactile feedback during certain events.
- Tsunami audio is used for voice, music, and sound effects.
- Ball Save feature gives player a grace period after first scoring a point on each ball.
- Side Kickouts can hold balls temporarily, ejecting them for MULTIBALL play (or when game ends or is tilted).
- Features can either carry over between balls and/or players, or reset at the start of each ball (to be defined).
- Extra balls can be awarded.
- G.I. Lamps, Hat Lamps and Switches may be triggered and controlled independently (not only as a group as with Original mode).
- We are not constrained by the original behaviors of the 10K Unit and Selection Unit.
  - For example, we can light or extinguish Hat lamps independently of the Selection Unit.
  - We can light or extinguish Kickout and Drain Rollover lamps independently of the 10K Unit.
  - We can define new behaviors for these devices as needed.
- All lamps may be lit, flashed, or extinguished independently (to be defined).
  - For example, multiple Red "Spot" lamps may be lit simultaneously or randomly in "motion" (to be defined).
  - All playfield lamps may be extinguished except one group, such as bumpers, during certain modes or events (to be defined).

### 13.2 Shaker Motor

The Shaker Motor shakes the cabinet to enhance gameplay.  It is controlled by Master via PWM.

- Slow shaking begins after ball has been released into ball lift and then the switch opens; i.e. when player pushes ball into shooter lane.
  - This simulates the feeling of climbing the first hill of a rollercoaster.
  - Corresponds with Tsunami sound effect of rollercoaster being hauled up the chain to the top of the first drop.
  - Shaking and sound continues until player scores the first point (hits any target, bumper, gobble, etc.).
  - This feature may be disabled after the first ball so it doesn't become tiresome to the player(s).

- Faster shaking begins then increases then cuts off after the first point is scored, for a few seconds.
  - This simulates the more intense event of dropping down the first hill.
  - Corresponds with Tsunami sound effect of rollercoaster dropping down the first hill and girls screaming.

- Additional shaking events may be defined later to correspond with other modes, scoring events, multi-ball.

### 13.3 Audio

Tsunami WAV Trigger plays voice lines, music tracks, and sound effects.

- Voice lines are played to:
  - Announce start of game, and when players 2 to 4 are added.
  - Announce next player's turn for multi-player games.
  - Announce ball saved.
  - Announce mode starts, completions, and instructions/goals.
  - Announce scoring events such as jackpots, specials, extra balls, replays.
  - Tease player when ball drains and when Game Over.
  - Provide thematic commentary.

- Sound effects are played for:
  - Bumper hits.
  - Target hits.
  - Gobble and drain events.
  - Other scoring events and achievements.
  - Game start (i.e. girl screaming) and end.

- Music tracks play during modes and multi-ball.








---

## 14. Open Items and Implementation Notes

These are good hooks for Copilot to help write code.

- Score persistence
  - Implement non-volatile storage of last game score (player 1 only.)
  - Save periodically during a game or at Game Over.
  - Restore and display on power-up.
  - Perhaps this is handled autonomously by Slave?

- Score Motor timing API
  - Provide function to compute how many Score Motor 1/4 revs a score update will take.

- Mode state machine
  - Clean, centralized state machine for modes:
    - GAME_OVER, DIAGNOSTIC, ORIGINAL, ENHANCED, IMPULSE, TILT, etc.
  - Per-mode handler functions called from the 10 ms loop.

- Input debouncing and edge detection
  - Ensure Start button taps are properly detected (single, double, triple).
  - Debounce switches via time thresholds or sample windows.  Probably by initial edge detection, and then can't be re-detected for N ms.
    - This might entail per-switch countdown timers such as we have implemented in Slave for coils.

- Enhanced mode rule engine
  - Define data structures and functions for:
    - Player progression.
    - Ball save.
    - Feature completion.
    - Voice line selection and timing.
    - Shaker motor patterns.

---

## 15. Suggested Sections to Flesh Out (For Future Docs / Copilot)

The following sections would help Copilot (and future you) a lot if you add more detail:

1. Enhanced Mode Rule Details
   - Exact scoring rules, feature ladders, multi-ball, extra ball, ball save logic.
   - Which combinations of bumpers, targets, gobble events, etc. trigger and end:
     - Voice lines.
     - Sound effects tracks.
     - Music tracks.
     - Shaker events.
   - Definition of "roller coaster" themed modes, if any (for example multiball, hurry-up).

2. Audio Mapping Table
   - A table mapping:
     - Tsunami track numbers to descriptions and when to play.
   - Include priorities (music vs sound effects vs voice), interrupt rules, and any ducking rules.

3. Configuration and Tuning Constants
   - A central header file for all tunables:
     - Shaker motor patterns.
     - Ball save duration.
     - Lamp fade timings (if implemented).
     - Debounce times.

4. Safety and Fault Handling
   - Define behavior on:
     - Stuck switches.
     - Coil on too long (watchdog).
     - Communication timeout between Master and Slave.
   - How to indicate errors to the player (lamps, LCD, audio).
