# 1954 Williams Screamo - Modernized Control System Overview
Rev: 01-26-26.  Revised by Claude Opus 4.5.
IMPORTANT: This document refers to multi-player Enhanced games, HOWEVER, for the time being we will only support single players per game.  We may add multi-player support later.

## 1. Introduction

Screamo was originally a 1954 single-player Williams pinball machine, and has since been cosmetically and mechanically restored. This project modernizes the control system while preserving the original gameplay experience as much as possible, plus adding a new Enhanced game style with modern features.

Originally, all rules, scoring, and features were handled by electromechanical (EM) devices:

- Score Motor: advanced scores in increments of 50,000 or 500,000 points.
- Relay Banks: tracked progress toward features.
- Score Units: mechanical stepper units that advanced and displayed score via illuminated digits in the head, in increments of 10,000 points.
- Buttons and switches: responded to player and ball actions.
- Playfield lamps: indicated active features and feature progress.
- Coils: moved the ball on the playfield (pop bumper, side kickouts, slingshots, etc.) and controlled mechanisms (ball tray release, stepper units, etc.)
- Relays: handled game state and scorekeeping.

This project replaces most of the EM logic with two Arduino Mega 2560 R3 boards (Master and Slave) while preserving the original feel, sounds, and behavior as much as possible, plus adding a new Enhanced game style and a rich diagnostic system.

NOTE: The score is displayed to the player via lamps that illuminate numbers in the head, rather than a digital display.  Possible scores range from 10,000 to 9,990,000 points, in increments of 10,000 points.  Thus, there are 999 possible score values, and internally we represent the score as an integer from 1 to 999, where each unit represents 10,000 points.

---

## 2. High-Level Architecture

- Master Arduino (in cabinet)
  - Owns all game rules and most real-time behavior.
  - Controls playfield and cabinet coils, playfield lamps, shaker motor, Tsunami audio system, ball handling, and Score Motor / EM sound devices.
  - Talks to the Slave via RS-485 messages.
  - Manages Diagnostic mode, multi-player logic in Enhanced game style, and style selection (Original / Enhanced / Impulse).

- Slave Arduino (in head)
  - Owns lit-from-behind-backglass score display (including Tilt lamp and head's G.I. lamps) and head hardware.
  - Controls devices in the head:
    - Score lamps (Nine each of 10K, 100K, and 1M lamps).
    - Tilt and GI lamps in the head.
    - Three bells (10K, 100K, Select).
    - 10K Unit (for mechanical sound only).
    - Credit Unit (add/deduct credits; credits visible to player through a mechanical display).
  - Receives score, credit, and state commands and queries via RS-485 from Master.
  - Responsible for realistic timing of score updates and bells, matching the original Score Motor behavior.
  - Has its own 20x4 LCD display for diagnostics.  The LCD is only visible to the operator when the back door of the head is removed; not visible to players.

- Communication
  - Master sends high-level commands to Slave (for example: "add 50K to Player 1 score", "set TILT lamp ON", "set head GI OFF").
  - Slave translates those into stepper pulses, bell rings, and lamp updates, timed to emulate original mechanical behavior.
  - Master can also query Slave for credit status, but only whether one or more credits are available, not how many there are.
  - Master can also query Slave for the current score, but only for diagnostic purposes; Master is the authority on score.
  - Further RS485 commands can be established between Master and Slave should they be needed.

---

## 3. Original vs Modern Hardware

### 3.1 Original Mechanisms Still Used Normally

These remain and are still used as active game mechanisms:

- Flipper buttons and flippers (mechanically upgraded, but same function) (playfield).
- Pop bumper and dead bumpers (playfield).
- Left and right side kickouts (playfield).
- Left and right slingshots (playfield).
- Credit Unit (head).
- Ball Tray Release (original coil) (playfield).
- Lamps (playfield and head).
- Most switches (head, playfield and cabinet) including the coin mechanism switch and the Start button.

### 3.2 Original Mechanisms Used Only for Sound Effects

These remain physically present and are driven only for sound authenticity (no game logic is derived from them):

- Three scoring bells (10K, 100K, Select) (head).
- 10K Score Unit (head).  In order to "reset" the 10K unit to zero, it must advance all the way around back to zero even if it is already at zero.
- Knocker (cabinet).
- Score Motor (cabinet).
- Selection Unit (underside of playfield).
- Relay Reset Unit (underside of playfield).  The original game has two Relay Reset Banks, but we retain only one; we will fire it twice when the original game would have fired both once.

The actual scoring and rule decisions are all handled in software.

### 3.3 New / Modern Hardware

- Two Arduino Mega 2560 R3 boards
  - Master (in cabinet): controls playfield and cabinet devices, game rules, modes, and diagnostics.
  - Slave (in head): controls head devices, score display, and head diagnostics via commands from Master.

- Centipede shift register I/O expanders  
  - Controlled by Arduinos via I2C commands.
  - Input Centipede (cabinet): reads switch status (playfield and cabinet switches).
  - Output Centipede (cabinet): drives 6.3vac incandescent playfield lamps (via modern cube relay modules).
  - Output Centipede (head): drives 12vdc LED head lamps (via modern cube relay modules).

- Relay modules (cabinet and head)
  - Modern relays, digitally controlled by Centipede outputs, in turn provide power to lamps.
  - Pulling a Centipede output LOW effectively turns on a lamp (or group of lamps in the case of head G.I.)
  - Switch 6.3 VAC to playfield lamps (cabinet).
  - Switch 12 V DC to head lamps (head).
  - Software doesn't need to know about relay details; only that controlling Centipede outputs (indirectly) controls lamps.

- MOSFET drivers (cabinet and head)
  - Controlled by Arduinos PWM and digital pins to fire coils and control the shaker motor with adjustable power and timing.

- Solid-state relay (in cabinet)
  - Controlled by Master via digital pins.
  - Switches 50 VAC to Score Motor (on/off control only; no speed control).
  - For sound effect only; Master controls timing to match score updates.

- Shaker Motor (in cabinet.)
  - Controlled by Master via PWM MOSFET.
  - For tactile feedback in Enhanced game style; speed and duration controlled by PWM (cabinet).

- Audio System (in cabinet.)
  - Controlled by Master via RS-232 serial commands.
  - Tsunami WAV Trigger board (controlled by Master) (cabinet).
  - Amplifier and speaker (cabinet).
  - Plays simultaneous voice, music, and sound effects, primarily in Enhanced style.

- LCD Displays (on inside of coin door and in head.)
  - Controlled by Arduinos via RS-232 serial commands.
  - Two 20x4 (4 lines of 20 characters each) Digole LCDs:
    - One connected to the Master (inside coin door) for status, diagnostics, and settings.
    - One connected to the Slave (in the head) for head diagnostics.
    - Neither is visible to players during normal play.

- Diagnostics 4-Pushbutton Module (on inside of coin door.)
  - Four diagnostic buttons in the coin door: BACK, LEFT, RIGHT, SELECT.
  - Used in Game Over / Diagnostic mode; also used in-game for Tsunami volume (LEFT/RIGHT).

- New Ball Trough Release Mechanism (from Ball Trough to Ball Lift) (below playfield).
  - Up/down post releases individual balls from the Ball Trough into the Ball Lift.
  - Controlled by Master via PWM MOSFET.
  - See Section 3.4 for Ball Flow details.

- Ball In Lift Switch at base of Ball Lift mechanism (below playfield).
  - 'SWITCH_IDX_BALL_IN_LIFT' - Switch read by Master via Input Centipede.
  - Detects when a ball is present at the base of the Ball Lift, waiting to be pushed up into the Shooter Lane.

- 5 Balls In Trough Switch in Ball Trough (below playfield).
  - 'SWITCH_IDX_5_BALLS_IN_TROUGH' - Switch read by Master via Input Centipede.
  - Detects when five balls are present in the Ball Trough.
  - When this switch is OPEN, fewer than five balls are in the trough.

### 3.4 Ball Flow and Ball Handling Mechanisms

Understanding how the ball moves through the machine is critical for game logic:

**Physical Areas (from playfield to shooter lane):**

1. **Playfield**: Where the ball is in play. Balls can drain from the playfield only two ways:
   - Via one of the three bottom Drain Rollovers (left, center, right; each of which has a rollover switch that closes whenever a ball drains over it).
   - Via the Gobble Hole (a hole near the center of the playfield; this hole also has a switch that closes whenever a ball drains through it).

2. **Ball Tray**: A collection area at the very bottom of the playfield that catches balls draining via the three bottom Drain Rollovers (left, center, right). The Ball Tray holds balls until the Ball Tray Release coil is fired, which opens the tray and allows balls to drop into the Ball Trough below.

3. **Ball Trough**: A holding area below the Ball Tray that can hold up to five balls. Balls enter the Ball Trough in two ways:
   - From the Ball Tray (when the Ball Tray Release coil is fired).
   - Directly from the Gobble Hole (balls draining via the Gobble Hole bypass the Ball Tray entirely and drop directly into the Ball Trough).
   
   The Ball Trough has a switch ('SWITCH_IDX_5_BALLS_IN_TROUGH') that closes when five balls are present.

4. **Ball Trough Release**: A new up/down post mechanism ('DEV_IDX_BALL_TROUGH_RELEASE') that releases one ball at a time from the Ball Trough into the Ball Lift. Each pulse releases exactly one ball.

5. **Ball Lift**: A vertical channel where balls wait after being released from the Ball Trough. The Ball Lift has a switch (`SWITCH_IDX_BALL_IN_LIFT`) at its base that detects when a ball is present. The player manually pushes the ball up through the Ball Lift using the Ball Lift Rod (a manually-operated plunger), which moves the ball from the base of the lift into the Shooter Lane. **The ball lift does NOT automatically launch balls.**

   **Ball Lift Switch Behavior:**
   - Switch CLOSED: A ball is resting at the base of the Ball Lift, ready for the player to push up.
   - Switch OPEN: Either no ball is present, OR the player is actively pushing the Ball Lift Rod (which lifts any ball out of the switch detection area).
   
   **Important Ball Lift Considerations:**
   - If the Ball Lift Rod is partially or fully pressed when we fire `DEV_IDX_BALL_TROUGH_RELEASE`, the released ball will be blocked and cannot drop into the base of the lift until the rod is fully released.
   - If a ball is already at the base of the lift (`SWITCH_IDX_BALL_IN_LIFT` closed), we should NOT release another ball - doing so could cause a jam.
   - During Multiball or Modes, if we release a ball to the lift but the player ignores it (focused on other balls), that ball remains at the base of the lift. When we later need to give them their next ball, we simply use the ball already waiting there rather than releasing another.
   - While physically possible for two balls to occupy the Ball Lift simultaneously, this would cause a jam. If this occurs, the player can clear it by pressing the Ball Lift Rod fully to let balls drop into the Shooter Lane. Our software should avoid this situation by checking `SWITCH_IDX_BALL_IN_LIFT` before releasing a ball.

6. **Shooter Lane**: The lane at the top of the Ball Lift where the ball rests before the player uses the shooter (main plunger) to launch it onto the playfield. **The player must pull and release the shooter to launch the ball.**

**Ball Flow Summary:**
~~~
Playfield
    |
    +---> Drain Rollovers (L/C/R) ---> Ball Tray ---> [Ball Tray Release coil] ---> Ball Trough
    |                                                                                    |
    |                                                                        [Ball Trough Release coil]
    |                                                                                    |
    |                                                                                    v
    |                                                                              Ball Lift (SWITCH_IDX_BALL_IN_LIFT)
    |                                                                                    |
    |                                                                        [Player pushes Ball Lift Rod]
    |                                                                                    |
    |                                                                                    v
    |                                                                              Shooter Lane
    |                                                                                    |
    |                                                                           [Player shoots]
    |                                                                                    |
    +<-----------------------------------------------------------------------------------+

Playfield
    |
    +---> Gobble Hole -------------------------------------------------directly---> Ball Trough
                                                                                         |
                                                                             [Ball Trough Release coil]
                                                                                         |
                                                                                         v
                                                                                   Ball Lift
                                                                                         |
                                                                             [Player pushes Ball Lift Rod]
                                                                                         |
                                                                                         v
                                                                                   Shooter Lane
                                                                                         |
                                                                                  [Player shoots]
                                                                                         |
    +<-----------------------------------------------------------------------------------+
~~~

**Switch Reliability Notes:**

- The `SWITCH_IDX_GOBBLE` (Gobble Hole) switch closes reliably every time a ball enters the Gobble Hole. With our 10ms main loop checking switch states, it is extremely unlikely to miss a ball. However, our ball tracking logic should be resilient - if we somehow miss detecting a Gobble drain, the worst consequence should be the player not receiving points for that drain, not a corrupted ball count that breaks the game.

- The `SWITCH_IDX_5_BALLS_IN_TROUGH` switch reliably indicates when exactly five balls are present in the Ball Trough. When OPEN, we know at least one ball is elsewhere (playfield, Ball Tray, Ball Lift, Shooter Lane, or stuck somewhere).

**Key Software Implications:**
- When balls drain via Drain Rollovers, the Ball Tray collects them. Software must fire 'DEV_IDX_BALL_TRAY_RELEASE' to drop them into the Ball Trough before they can be dispensed.
- When balls drain via Gobble Hole, they go directly to the Ball Trough (no Ball Tray Release needed).
- To dispense a ball to the player, when the Ball Lift switch is open (no ball present in lift), fire 'DEV_IDX_BALL_TROUGH_RELEASE' to release one ball from the Ball Trough into the Ball Lift.
- The 'SWITCH_IDX_BALL_IN_LIFT' switch tells software when a ball is ready for the player to push into the Shooter Lane.
- The 'SWITCH_IDX_5_BALLS_IN_TROUGH' switch tells software whether all five balls are accounted for in the Ball Trough.

### 3.5 Ball Tracking

Master tracks all five balls at all times during gameplay.

#### 3.5.1 Ball Locations

Each ball can be in one of these locations:
- **TRAY**: In the ball tray (above trough, drains when tray opens)
- **TROUGH**: In the ball trough (ready to dispense)
- **LIFT**: At the base of the ball lift (waiting for player to lift)
- **PLAYFIELD**: On the playfield (in active play), including shooter lane
- **KICKOUT_LEFT**: Captured in left kickout hole
- **KICKOUT_RIGHT**: Captured in right kickout hole

#### 3.5.2 Ball Counting Rules

- **5-balls-in-trough switch**: Only indicates all 5 balls are in trough; does not provide count of fewer balls
- **Ball-in-lift switch**: Indicates exactly one ball is at lift base
- **Kickout switches**: Each indicates one ball captured in that hole

Ball counts are derived from:
1. Initial state: All 5 balls assumed in tray or trough at game start
2. Dispense event: Decrement trough count when ball released to lift
3. Lift cleared (ball-in-lift switch opens after being closed): Ball now on playfield/shooter lane
4. Drain detected: Ball returning to tray (bottom rollovers) or trough (gobble hole)
5. Kickout capture: Ball in kickout (detected by kickout switch)
6. Kickout eject: Ball returns to playfield

#### 3.5.3 Sanity Checks

Master periodically validates ball counts:
- If 5-balls-in-trough switch is closed, ballsInTrough must be 5
- Total of all locations must equal 5
- If mismatch detected, log warning but do not interrupt gameplay

### 3.6 Ball Tray Management

The Ball Tray holds balls that have drained via the three bottom rollovers (left, center, right). Balls that drain via the Gobble Hole bypass the tray and go directly to the trough.

#### 3.6.1 Ball Tray Solenoid Characteristics

- **Initial Power**: 200 (PWM 0-255)
- **Hold Power**: 40 (PWM 0-255) - sufficient to keep tray open, minimizes heat
- **Activation Time**: 200ms at initial power before reducing to hold power

**Important:** While the solenoid can be held indefinitely at hold power, it will warm up over time. Minimize hold duration when possible.

#### 3.6.2 Ball Tray States by Game Phase

| Phase                               | Original/Impulse | Enhanced    |
|-------------------------------------|------------------|-------------|
| Attract                             | Closed           | Closed      |
| Ball Recovery (waiting for 5 balls) | Open (held)      | Open (held) |
| Gameplay (before first score)       | Open (held)      | Open (held) |
| Gameplay (after first score)        | Closed           | See 3.6.3   |
| Game Over                           | Closed           | Closed      |

#### 3.6.3 Enhanced Mode Ball Tray During Gameplay

In Enhanced mode, multiple balls may be in play or locked. When a ball drains via a bottom rollover:

1. Open Ball Tray immediately
2. Hold open for `BALL_TRAY_DRAIN_HOLD_SECONDS` (configurable constant, default 8 seconds)
3. Close Ball Tray
4. This allows drained balls to reach the trough for potential re-dispensing

**Configurable Constant (defined at top of Screamo_Master.ino):**
const byte BALL_TRAY_DRAIN_HOLD_SECONDS = 8;  // Seconds to hold tray open after drain

If testing shows balls take longer to reach trough, increase this value.

---

## 4. Game Styles

Screamo runs in several styles. Unless otherwise noted, Master is style authority and informs Slave via RS-485.

### 4.1 Original Style

- Triggered by: Single press of Start button (with at least one credit).
- Single-player only.
- Rules and scoring match the original Screamo rules.
- Modernized flippers (a slight departure from original)
  - Independent control of each flipper.
  - Flippers can be held up indefinitely while the button is held.
- EM sound devices used:
  - Bells, Score Motor, 10K Unit, Selection Unit, Relay Reset Bank, Knocker.
- Goal: Make it feel like playing an unmodified original Screamo (except for the flipper operation.)
- Shaker motor and Tsunami audio are not used in this style.

### 4.2 Enhanced Style

- Triggered by: Double-tap of Start button (two presses within 500ms, with at least one credit).
- A new rule set, uses:
  - Tsunami audio system.
  - Shaker motor.
  - EM sound devices (bells, Score Motor, 10K Unit, Selection Unit, Relay Reset Bank) during NORMAL Enhanced gameplay.
  - Tsunami-only sound effects during MODES and MULTIBALL (no EM bells/devices during these special periods).
- Multi-player support:
  - After starting Enhanced style, and before any points are scored, the player may press Start up to three more times to add Players 2, 3, and 4 (one credit per player).
  - Audio cue: "Press Start again if ya wanna admit any of your little friends."
- Theme:
  - Roller-coaster / amusement park flavor.
  - Voice, music, and special effects.
  - Musical theme can be toggled between Calliope/Circus music and Surf Rock (settable in Diagnostics Settings).
  - One of the two themes is used for music during regular Enhanced play, and the other theme's music is played during modes and Multiball.
- Features (planned / current):
  - Up to four players.
  - Special game modes and scoring goals (details to be specified separately).
  - Ball save for first N seconds after the first score on each ball (time defined in Settings) OR if we drain the ball before hitting any targets.
  - Shaker motor events triggered on certain happenings.
  - Extra voice prompts, music, jackpots, etc. (details to be specified separately).

### 4.3 Impulse Style

- Triggered by: Second press of Start button MORE than 500ms after the first press (with at least one credit).
  - This allows players familiar with the hidden style to deliberately access it by pressing Start, waiting a beat, then pressing Start again.
- Single-player only.
- Rules identical to Original style except for flipper behavior:
  - Impulse flippers:
    - Pressing either flipper button causes both flippers to fire briefly and immediately drop.
    - Flippers cannot be held up.
    - A new press is required before flippers can be fired again.
- Intended to exactly reproduce the original 1954 impulse-flipper behavior and gameplay.
- EM sound devices used (same as Original style).
- Shaker motor and Tsunami audio are not used in this style.

### 4.4 Start Button Tap Detection Summary

**Window Timing:**
- Double-tap window: 500ms (configurable via 'START_STYLE_DETECT_WINDOW_MS' in code)
- Player addition window: Remains open until first point is scored (Enhanced style only)

**Three Start Patterns:**

| Pattern     | Timing                                     | Credits Deducted          | Result                                                                               |
|-------------|--------------------------------------------|---------------------------|--------------------------------------------------------------------------------------|
| Single tap  | No 2nd press within 500ms                  | 1 credit                  | Original Style; Game starts immediately after 500ms expires                          |
| Delayed tap | 2nd press > 500ms but before 1st pt scored | 0 additional credits      | Impulse Style; Switches from Original to Impulse mode, no add'l credit deducted      |
| Double tap  | 2nd press within 500ms                     | 1 credit + 1/add'l player | Enhanced Style; Game starts, add'l Start presses add players 2-4 until 1st pt scored |

**Detailed Behavior:**

**Original Style (Single Tap; 500ms expires):**

T =   0ms: Start pressed (1st time) Start 500ms timer; DO NOT deduct credit yet
           DO NOT start game yet
           Monitor for 2nd Start press
T = 500ms: Timer expires, no 2nd press detected
           Check credits:
             IF credits: Deduct 1 credit Start Original Style game immediately (mechanical sounds only)
             IF no credits: Do nothing, stay in Attract mode

**Impulse Style (Delayed Tap - after 500ms):**

T =   0ms: Start pressed (1st time) Start 500ms timer; DO NOT deduct credit yet
           DO NOT start game yet
           Monitor for 2nd Start press
T = 500ms: Timer expires, no 2nd press
           Check credits:
             IF credits: Deduct 1 credit Start Original Style game immediately (mechanical sounds only)
             IF no credits: Do nothing, stay in Attract mode
T = 700ms: Start pressed (2nd time, AFTER 500ms expired and BEFORE first point scored) Original Style -> Impulse Style
           NO additional credit deducted
           Switch flipper behavior to Impulse mode
           Continue play (already started as Original)

**Enhanced Style (Double Tap - within 500ms):**

T =   0ms: Start pressed (1st time) Start 500ms timer; DO NOT deduct credit yet
           DO NOT start game yet
           Monitor for 2nd Start press
T = 200ms: Start pressed (2nd time, within 500ms) Enhanced Style detected
           Check credits:
             IF credits: Deduct 1 credit Play SCREAM sound effect Start Enhanced game sequence Player 1 added Enter player addition window (until first point scored)
             Monitor for: coin inserts, knockoff presses, Start presses
             IF no credits: Play Aoooga horn + "no credit" announcement Stay in Attract mode
T = 200ms+: Player Addition Window (Enhanced only, UNTIL FIRST POINT SCORED) monitor continuously for:
           A) Start pressed again:
              IF players < 4 AND credits:
                Deduct 1 credit
                Add player (2, 3, or 4)
                Duck music, play announcement ("Second guest, c'mon in!")
                Continue monitoring
              IF players < 4 AND no credits:
                Play Aoooga horn + random "no credit" announcement
                Continue monitoring (can still add coins and retry)
              IF players = 4:
                Play "park's full" announcement
                Continue monitoring
           B) Coin inserted OR Knockoff pressed:
              Add 1 credit
              Fire knocker
              Continue monitoring
              (Player can now press Start to add another player)       
           C) First point scored:
              Close player addition window
              Start normal Enhanced gameplay

**Key Points:**
1. **First credit deduction happens:**
   - Original: When 500ms timer expires (if credits available)
   - Enhanced: Immediately on 2nd press within 500ms (if credits available)
   - Impulse: Same as Original (no additional deduction on 2nd press)

2. **Additional credits deducted (Enhanced only):**
   - One credit per additional player (players 2-4)
   - Only until first point is scored

3. **Zero credits behavior:**
   - Original/Impulse: Silent, stay in Attract mode
   - Enhanced (initial): Aoooga horn + announcement, then Attract mode
   - Enhanced (adding players): Aoooga horn + announcement, stay in player addition window

4. **Continuous monitoring (Enhanced player addition window):**
   - Coin inserts and Knockoff presses always add credits
   - Start presses attempt to add players if < 4 (success depends on credits)
   - Window closes when first point is scored

5. **No audio until style is determined:**
   - Keeps Enhanced/Impulse styles hidden from casual players
   - Original/Impulse: Only mechanical EM sounds
   - Enhanced: Scream + voice lines + music

**Start button during gameplay:**
- **Before first point scored:**
  - Original/Impulse: Second press changes to Impulse; additional Start presses are ignored.
  - Enhanced: Start presses attempt to add players 2-4 (see above).
- **After first point scored (any style) and if credits:**
  - Pressing Start immediately ends the current game and begins tap detection for a new game.
  - This matches the behavior of an original, unmodified 1954 Screamo.
- **After first point scored (any style) and if no credits:**
  - Do nothing; stay in current game until it ends normally.

### 4.5 Game Over / Attract Mode

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

### 4.6 Tilt Behavior

Tilting always affects only the CURRENT BALL; it never ends the game prematurely (unless it was the last ball of the last player).

- When the game is tilted in any playing style:
  - Flippers are disabled.
  - All playfield lights momentarily turn off.
  - Any balls in side kickouts are retained (locked balls always carry over to the next ball and to the next player, if any).
  - TILT lamp is turned on and flashes for about 4 seconds.
  - Game waits for all in-play balls to drain. (This may be difficult to know if there is a ball in the Shooter Lane and/or the base of the Ball Lift.)
- After all balls have drained:
  - If more balls remain for the current player:
    - Continue with the next ball for this player.
  - If this was the last ball for the current player in a multi-player Enhanced game:
    - Advance to the next player's first ball.
  - If this was the last ball of the last player:
    - Game transitions to Game Over / Attract mode.

**Tilt warning count resets at the start of each ball** (not cumulative across balls or players). See Section 14.1 for Enhanced mode's two-warning tilt behavior.

### 4.7 Diagnostic Mode (Overview)

Diagnostic mode provides access to hardware testing and game settings. It is entered from Attract mode by pressing the SELECT button.

#### Entry and Navigation

- **Entry**: Press SELECT button during MODE_ATTRACT to enter Diagnostic mode.
- **Volume Shortcut**: During ATTRACT or ENHANCED style gameplay, LEFT and RIGHT buttons adjust Main Volume directly (persisted to EEPROM).
- **Four Diagnostic Buttons**:
  - BACK: Exit current test / go up a level / return to Attract mode.
  - LEFT (-): Navigate to previous item or decrease value.
  - RIGHT (+): Navigate to next item or increase value.
  - SELECT: Enter a sub-menu, activate a test, or confirm a selection.

#### LCD Display Format

- **Row 1**: Suite/test title (e.g., "DIAGNOSTICS", "VOLUME ADJUST", "LAMP TEST")
- **Row 2**: Current item name or parameter
- **Row 3**: Additional info (item counter, value, or instructions)
- **Row 4**: Status or navigation hints

#### Diagnostic Suites (Implemented)

**1. VOLUME** COMPLETE
- Two-level navigation: LEFT/RIGHT selects parameter, SELECT enters adjustment mode.
- Parameters:
  - **Master**: Overall Tsunami gain (-40dB to 0dB). Plays music sample when adjusted.
  - **Voice Offset**: Voice category gain offset (-20dB to +20dB). Plays voice sample.
  - **SFX Offset**: Sound effects gain offset (-20dB to +20dB). Plays SFX sample.
  - **Music Offset**: Music category gain offset (-20dB to +20dB). Plays music sample.
  - **Duck Offset**: Ducking level when voice plays over music/SFX (-40dB to 0dB). Demonstrates ducking.
- All values persisted to EEPROM immediately on change.

**2. LAMP TESTS** COMPLETE
- LEFT/RIGHT cycles through all lamps (Master playfield + Slave head).
- Current lamp is ON; previous lamp turns OFF automatically.
- Displays: lamp index, name (from PROGMEM), "Lamp X of Y", "ON".
- Includes 47 Master lamps + 29 Slave lamps (score digits, GI, TILT).
- BACK restores attract lamp state and exits.

**3. SWITCH TESTS** COMPLETE
- Passive monitoring: displays any closed switch automatically.
- Plays 1000Hz tone on switch close, 500Hz tone on switch open.
- Displays: switch index, name (from PROGMEM), pin number.
- Covers all 40 switches including flipper buttons (via Centipede).
- Does NOT include Slave head switches (credit full/empty).
- BACK exits to main menu.

**4. COIL/MOTOR TESTS** COMPLETE
- LEFT/RIGHT cycles through all coils and motors (Master + Slave).
- SELECT fires coil briefly OR runs motor while held.
  - Shaker Motor: runs at minimum power while SELECT held.
  - Score Motor: runs while SELECT held.
  - Other coils: fire once with configured power/duration.
- Includes 14 Master devices + 6 Slave devices.
- BACK stops any running motor and exits.

**5. AUDIO TESTS** COMPLETE
- LEFT/RIGHT cycles through all audio tracks sequentially.
- SELECT plays the current track.
- Displays: track number, category, description (from PROGMEM).
- Covers all COM (voice), SFX, and MUS tracks.
- BACK stops playback and exits.

#### Settings to Implement (Future)

The following settings are defined in EEPROM:

**Game Settings** (TODO)
- **THEME**: Select Circus or Surf Rock music as the PRIMARY theme.  The other theme is used during Modes and Multiball.
  - EEPROM_ADDR_THEME (address 20)
- **BALL SAVE**: Ball save duration in seconds (0=off, 1-30).
  - EEPROM_ADDR_BALL_SAVE_TIME (address 30)
- **MODE 1-6**: Mode time limits in seconds.
  - EEPROM_ADDR_MODE_1_TIME through EEPROM_ADDR_MODE_6_TIME (addresses 31-36)

**Replay Scores** (TODO)
- **ORIG REPLAY 1-5**: Replay scores for Original/Impulse style (0-999, representing 10K increments).
  - EEPROM_ADDR_ORIGINAL_REPLAY_1 through EEPROM_ADDR_ORIGINAL_REPLAY_5 (addresses 40-48, 2 bytes each)
- **ENH REPLAY 1-5**: Replay scores for Enhanced style.
  - EEPROM_ADDR_ENHANCED_REPLAY_1 through EEPROM_ADDR_ENHANCED_REPLAY_5 (addresses 50-58, 2 bytes each)

**Track History** (TODO)
- ** LAST SCORE **: Score at (approximately, not exactly) end of last game played (1-999, representing 10K increments).
  - EEPROM_ADDR_LAST_SCORE (address 0)
- **LAST CIRCUS SONG**: Most recent Circus track played.
  - EEPROM_ADDR_LAST_CIRCUS_SONG_PLAYED (address 21)
- **LAST SURF SONG**: Most recent Surf Rock track played.
  - EEPROM_ADDR_LAST_SURF_SONG_PLAYED (address 22)

**Mode History** (TODO)
- **LAST GAME MODE**: Most recently-played game mode BUMPER CARS, ROLL-A-BALL, GOBBLE HOLE
  - EEPROM_ADDR_LAST_MODE_PLAYED (address 25)

#### Implementation Notes

- All lamp, switch, coil, and audio descriptions are stored in PROGMEM (see 'Pinball_Descriptions.h').
- Dirty-flag rendering minimizes LCD updates.
- Switch state arrays ('switchOldState[]', 'switchNewState[]') are updated each 10ms tick.
- Edge detection ('switchPressedEdge()') prevents button repeat.

---

## 5. Score Motor and Timing Model

The original Score Motor is used for sound and timing reference only. Actual scores are computed in software and displayed via lamps, driven by Slave.

**EM Sound Device Usage by Style:**
- **Original Style**: All EM sound devices used (Score Motor, 10K Unit, Selection Unit, Relay Reset Bank, three Bells, Knocker).
- **Impulse Style**: Same as Original style.
- **Enhanced Style (normal gameplay)**: All EM sound devices used, same as Original style. This provides familiar mechanical feedback during regular play.
- **Enhanced Style (during Modes and Multiball)**: EM sound devices are NOT used. Instead, Tsunami audio provides all sound effects. This makes these special periods feel distinctly different and more exciting.
Note that we will use the Credit Unit in all styles, as one credit is required to start any game (and for each player in Enhanced style.)

Master should start the Score Motor, and potentially fire the Selection Unit and/or the Relay Reset Bank for the audible/mechanical effect around the time it sends relevant messages to Slave, such as score updates.  Operation of the original Selection Unit and Relay Reset Bank will be detailed below.

Master should wait a realistic motor run duration (varies depending on score digits adjusted and other factors) before stopping motor.  But typically, if the score motor runs, it will be for 0, 1, 2, or 3 full 1/4 revolutions (each 882 ms) depending on the score adjustment being made.  Sometimes the Score Motor does not run, such as for individual 10K or 100K score increments.

### 5.1 Mechanical Timing

- Score Motor runs at 17 RPM.
- Motor behavior:
  - 1 revolution is divided into 4 quarters.
  - Each 1/4 revolution is 882 ms.
  - Each 1/4 revolution is divided into 6 slots:
    - Slots 1 through 5: potential score advances of 10K or 100K.
    - Slot 6: rest.
  - Each slot is 147 ms (882 ms / 6).
- Approximation is acceptable for Score Motor timing; exact 147ms slots are not required.

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
    - Slot 3: Open the Ball Tray
    - Slot 4: Fire Relay Reset Bank again.
    - Slot 5: Release the 1M/100K rapid reset
  - Then one or two additional 1/4 revolutions for tens (0K to 90K) of score reset.
    - If the 10K Unit is already at zero, the Score Motor still runs for two full 1/4 revolutions (10 score steps) to walk the 10K unit back up to zero.
- During game play, firing the Relay Reset Bank consumes a full 1/4 revolution (6 slots, 882 ms) when score adjustments will not take place.
- It is possible that other non-scoring operations may occur in the same 1/4 revolution as Relay Reset.

### 5.4 Score Motor Usage Summary

| Operation                            | Original | Impulse | Enhanced |
|--------------------------------------|----------|---------|----------|
| Game start (score reset)             | Yes      | Yes     | No       |
| Small score increment (10K or 100K)  | No       | No      | No       |
| Large score increment (50K, 500K)    | Yes      | Yes     | No       |
| Score decrement (not used currently) | Yes      | Yes     | No       |

### 5.5 Future Enhancement: Score Reset Duration Calculation

Currently, only Slave knows the "last score" from previous game/power-up. Master cannot calculate exact reset duration.

**Future implementation:**
1. Master stores last score in EEPROM
2. On power-up, Master tells Slave what "last score" to display
3. Both Arduinos know starting score
4. Master calculates reset duration based on number of reel decrements needed
5. Master runs Score Motor for exactly that duration

**For now:** Master assumes worst-case reset time of 2 full motor cycles (~1764ms).

### 5.6 Software Requirements

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

- Use non-volatile storage (EEPROM) on Slave Arduino to save most-recent score.  Needn't be exact, just approximate, so updates can be infrequent.
  - Saving previous score when:
    - A game ends, or
    - Periodically during games (for example every N iterations of the 10 ms loop equal to about 15 to 30 seconds).
    - This is to approximate the original EM game's behavior of retaining score across power cycles, but exact previous score is NOT critical.
- On power-up:
  - Slave: Restore the last saved score and display it in Game Over / Attract mode.
- On Game Over:
  - Slave saves the final score (player 1 only) to non-volatile storage, and continues to display that score until next power cycle or new game start.
  - For multi-player Enhanced games, when the game ends, display each player's score in sequence:
    - Briefly flash the 1/2/3/4M lamp for player number, followed by their score, and then a brief interval with score lamps off, for each player.
  - Continue to display the last player's score(s) until next power cycle or new game start.
- Realistic reset to zero when starting a new game:
  - Use 10K and 100K logic to "walk" the score back to zero in a visually and audibly realistic way.
  - 'startNewGame()' should:
    - Reset internal score variables.
    - Command Slave to step the score down to 0 using realistic timing.

### 6.2 Hardware Startup Behavior

- On power-up or hardware reset of Master or Slave:
  - Both Arduinos should initialize their internal state to GAME_OVER mode.
  - Slave should read persistent score from EEPROM and display it.
  - All outputs (coils, lamps) should be turned off initially.
  - Head lamps (GI and Tilt) should be turned on.
  - TILT lamp should be off.
  - Playfield GI should be on.

### 6.3 Hardware Reset Behavior

- At any time during gameplay, game over, or diagnostics:
  - Pressing and holding the KNOCKOFF button for more than 1 second should trigger a software reset of both Master and Slave Arduinos:
    - Master sends a reset command to Slave via RS-485.
    - Master and Slave both immediately release all coils, stop all motors, turn off all lamps.
    - Perform software reset.  Result will be as if power had been recycled.
  - Pressing the KNOCKOFF button for less than one second, one or more times, will command Master to send a message to Slave to add a credit, for each press.

---

## 7. Error Handling

### 7.1 Critical Errors

When Master detects a critical error, it calls `criticalError(line1, line2, line3)` which:

1. Clears LCD and displays the three-line error message
2. Displays "HOLD KNOCKOFF=RST" on line 4
3. Plays tilt buzzer sound
4. Plays verbal error announcement (if applicable)
5. Disables all coils and gameplay
6. Waits for 1-second Knockoff button hold to reset system

**Critical Error Conditions:**
- RS485 communication timeout (no response from Slave within 100ms)
- Hardware initialization failure
- Ball count impossible state (more than 5 balls detected)

### 7.2 Recoverable Errors

Some error conditions allow gameplay to continue:

- **Missing ball during startup**: Wait for player to recover balls (see Section 12)
- **Ball stuck in kickout**: Automatic retry after timeout, then continue
- **Score motor timeout**: Log warning, continue gameplay

### 7.3 Debug Logging

When `debugOn = true`, Master logs to Serial:
- Slow loop warnings (>9ms)
- Ball location changes
- State machine transitions
- Switch edge detections

---

## 8. Hardware Inventory - Head (Slave Arduino)

The Slave controls devices in the head via MOSFETs, Centipede outputs, and direct inputs.

### 8.1 Coils and MOSFET Outputs

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
  PWM controls overall brightness of all 27 score lamps (10K / 100K / 1M). The lamps themselves are turned on and off via Centipede outputs/relays.

- 'DEV_IDX_LAMP_HEAD_GI_TILT' - Head GI and Tilt lamp power  
  PWM controls brightness for head GI and Tilt. The lamps themselves are turned on and off via Centipede outputs/relays.

### 8.2 Head Switch Inputs

Read via direct input pins on Slave:

- 'PIN_IN_SWITCH_CREDIT_EMPTY' - Credit unit empty switch  
  Opens when credits are zero; prevents further step-down; informs Master that credits are empty.

- 'PIN_IN_SWITCH_CREDIT_FULL' - Credit unit full switch  
  Opens when credit unit is full; prevents further step-up.

### 8.3 Head Lamps (via Relay Modules via Centipede Outputs)

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

### 8.4 Slave LCD

- 20x4 Digole LCD connected to Slave.
- Used for:
  - Head diagnostics.
  - Displaying test names, coil names, lamp names, etc.

---

## 9. Hardware Inventory - Cabinet and Playfield (Master Arduino)

### 9.1 Coils, Motors, and MOSFET Outputs

All controlled by Master via MOSFETs and PWM:

- 'DEV_IDX_POP_BUMPER' - Pop bumper coil.  For "E" bumper only.
- 'DEV_IDX_KICKOUT_LEFT' - Left side kickout coil.
- 'DEV_IDX_KICKOUT_RIGHT' - Right side kickout coil.
- 'DEV_IDX_SLINGSHOT_LEFT' - Left slingshot coil.
- 'DEV_IDX_SLINGSHOT_RIGHT' - Right slingshot coil.
- 'DEV_IDX_FLIPPER_LEFT' - Left flipper coil (supports hold).
- 'DEV_IDX_FLIPPER_RIGHT' - Right flipper coil (supports hold).
- 'DEV_IDX_BALL_TRAY_RELEASE' - Original Ball Tray Release coil (can be held). Opens the Ball Tray to drop balls into the Ball Trough.
- 'DEV_IDX_SELECTION_UNIT' - Selection Unit coil (sound only).
- 'DEV_IDX_RELAY_RESET' - Relay Reset Bank coil (sound only).
- 'DEV_IDX_BALL_TROUGH_RELEASE' - New up/down post Ball Trough Release coil. Releases one ball from Ball Trough into Ball Lift.
- 'DEV_IDX_MOTOR_SHAKER' - Shaker motor (PWM for speed and duration).
- 'DEV_IDX_KNOCKER' - Knocker coil (one pulse per knock).
- 'DEV_IDX_MOTOR_SCORE' - Score Motor control (sound only, can be held on).

### 9.2 Cabinet Switches (via Centipede Inputs)

All cabinet switch closures, including the flipper buttons, now enter Master through Centipede #2 (switch indexes 64-127). Master still gives the flipper buttons a faster-processing path in software, but no longer wires them directly to Arduino GPIO pins.

- 'SWITCH_IDX_START_BUTTON' - Start button (supports single / double / delayed-second tap detection).
- 'SWITCH_IDX_DIAG_1' to 'SWITCH_IDX_DIAG_4' - Diagnostic buttons (BACK, LEFT, RIGHT, SELECT).
- 'SWITCH_IDX_KNOCK_OFF' - Hidden knock-off switch; can add credits or force reset.
- 'SWITCH_IDX_COIN_MECH' - Coin mech switch; adds credits.
- 'SWITCH_IDX_5_BALLS_IN_TROUGH' - Ball Trough switch; closes when 5 balls are present in the Ball Trough.
- 'SWITCH_IDX_BALL_IN_LIFT' - Ball Lift switch; closes when a ball is present at the base of the Ball Lift.
- 'SWITCH_IDX_TILT_BOB' - Tilt bob switch.
- 'SWITCH_IDX_FLIPPER_LEFT_BUTTON' - Left flipper button (routed through Centipede #2).
- 'SWITCH_IDX_FLIPPER_RIGHT_BUTTON' - Right flipper button (routed through Centipede #2).

### 9.3 Playfield Switches (via Centipede Inputs)

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
  'SWITCH_IDX_GOBBLE' - ball enters gobble hole and drains directly to Ball Trough.

- Drain switches:  
  'SWITCH_IDX_DRAIN_LEFT', 'SWITCH_IDX_DRAIN_CENTER', 'SWITCH_IDX_DRAIN_RIGHT'.
  These detect balls draining via the three bottom rollovers into the Ball Tray.

### 9.4 Playfield Lamps (via Relay Modules via Centipede Outputs)

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

### 9.5 Master LCD and Audio

- Master 20x4 LCD (inside coin door):
  - Displays current mode, diagnostics screens, device names, and result messages.

- Tsunami WAV Trigger (with amp and speaker):
  - Controlled by Master via RS-232 serial commands.
  - Plays voice, music, and sound effects in Enhanced style.
  - Volume can be adjusted via LEFT and RIGHT Diagnostic buttons during play.

---

## 10. Software Architecture Overview

### 10.1 Main Loop Timing

- For Original, Enhanced, and Impulse Style games, we use a 10 ms main loop on both Master and Slave:
  - On each loop:
    - Read all switch inputs once (Centipede ports).
    - Compute switch edge detection flags (just-closed, just-opened).
    - Update device timers and process queued coil activations.
    - Process flippers (immediate response).
    - Dispatch to current game phase handler.
    - Update outputs (coils, motors, lamps, audio commands).
    - Keep track of min and max loop execution time for profiling.
- All game logic runs within the 10ms tick - **no blocking operations (delay()) during gameplay**.
- All coils must include a software watchdog so they are not left turned on more than an expected maximum amount of time.  An exception would be the playfield Ball Tray Release Coil, which can be held open indefinitely.

### 10.2 Non-Blocking Architecture

**Critical Design Principle:** All operations during gameplay must be non-blocking. The main loop must complete within 10ms every tick.

**Allowed blocking:**
- During `setup()` for hardware initialization (delays for LCD, Tsunami, etc.)
- During Diagnostic mode for certain tests

**Not allowed during gameplay:**
- `delay()` calls
- Busy-wait loops for switch changes
- Any operation that takes more than a few milliseconds

**Implementation approach:**
- Use state machines with tick counters instead of delays
- Use device timer system for all coil activations (never direct analogWrite during gameplay)
- Use phase/sub-phase enums to track multi-step operations

### 10.3 Game Phase State Machine

Master uses a game phase state machine to track the overall game state:

PHASE_ATTRACT       - Waiting for Start button, processing coins 
PHASE_STARTUP       - Ball recovery and game initialization (non-blocking sub-states) 
PHASE_BALL_READY    - Ball in lift, waiting for player to launch 
PHASE_BALL_IN_PLAY  - Ball on playfield, processing scoring switches 
PHASE_BALL_ENDED    - Ball drained, advancing to next ball/player 
PHASE_GAME_OVER     - Game finished, transitioning to Attract 
PHASE_TILT          - Tilted, waiting for ball(s) to drain 
PHASE_DIAGNOSTIC    - Diagnostic mode active

### 10.4 Switch Processing

Switch processing happens once per tick, at the start of the main loop:

1. Read all Centipede input ports into `switchNewState[]`
2. For each switch, compute:
   - `switchJustClosedFlag[]` - true if switch just closed this tick (with debounce)
   - `switchJustOpenedFlag[]` - true if switch just opened this tick
3. Decrement any active debounce counters
4. Game logic checks flags (not raw switch state) for scoring switches

This ensures consistent switch state throughout the tick and prevents re-triggering.

### 10.5 Mode/Style and State Management

- On power-up:
  - Both Master and Slave default to GAME_OVER / Attract mode.
- At end of each game:
  - Return to GAME_OVER / Attract mode.
- From Attract mode:
  - If coin inserted or knock-off pressed:
    - Increment credits (up to a maximum).
    - Fire knocker.
  - If Diagnostic LEFT/RIGHT pressed:
    - Adjust Tsunami volume (persisted).
  - If Diagnostic SELECT button pressed:
    - Enter Diagnostic mode.
  - If Start pressed with credits > 0:
    - Begin Start button tap detection (see Section 4.4):
      - Single tap (no second press within 500ms): MODE_ORIGINAL
      - Double tap (second press within 500ms): MODE_ENHANCED
      - Delayed second tap (second press after 500ms): MODE_IMPULSE
    - In Enhanced style only:
      - Start can be pressed up to three more times to add Players 2, 3, and 4 (one credit each).
      - New players may not be added after any points have been scored.
    - Deduct credits appropriately.
  - If Start pressed with credits = 0:
    - In Original/Impulse detection: Do nothing, remain in Attract mode.
    - In Enhanced detection: Play Aoooga horn sound effect and random "no credit" announcement, then return to Attract mode.

### 10.6 Multi-Player Indication (Enhanced Style)

- For multi-player Enhanced games:
  - While waiting for each player to score their first point:
    - Flash the appropriate 1M lamp (1M, 2M, 3M, or 4M) for that player.
    - All other score lamps off.
    - We will also provide audio cues including "Player 2, you're up next", etc.
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

## 11. Style Rule Summaries

### 11.1 Original Style

- Single player only.
- Normal flippers (independent, holdable).
- Classic Screamo scoring and features.
- 5 balls per game; no extra balls.
- Game ends after ball 5 drains.
- All EM sound devices used (10K Unit, Relay Reset Bank, Selection Unit, Score Motor, three Bells, Knocker).
- No Tsunami audio or shaker motor.

### 11.2 Impulse Style

- Same as Original style, but:
  - Flippers behave as impulse flippers:
    - Press either button: both flippers fire once briefly and then drop.
    - No hold; must release and press again to flip again.

### 11.3 Enhanced Style

- Enhanced Style is generally a superset of Original/Impulse style; i.e. existing Original/Impulse features are preserved where possible, but new features are added.
- Up to 4 players.
- Uses:
  - Tsunami audio (voice, music, sound effects).
  - Shaker motor.
  - EM sound devices during normal gameplay (same as Original style).
  - Tsunami-only sound effects during Modes and Multiball (no EM bells/devices).
- Rules (high-level, to be elaborated separately):
  - Roller-coaster / Screamo theme integration.
  - Ball save for first N seconds per ball after first score, or if ball drains before hitting any targets (even if >N seconds).
  - New scoring goals (for example collecting letters, jackpots, mode progression).
  - Potential use of gobble / special / kickouts / bumpers for modes and bonuses.

---

## 12. Starting a Game

### 12.1 Pre-Start Ball Recovery (All Game Modes)

**Critical Requirement:** Before any game can begin (Original, Impulse, or Enhanced), **all five balls must be detected in the ball trough** (`SWITCH_IDX_5_BALLS_IN_TROUGH` switch closed). This applies to all game modes equally.

Upon turning on the game, whenever the machine is in Attract mode, the pop bumper, flippers, and slingshots are disabled, and if a ball is detected in either side kickout, it is ejected immediately.

At any time, whether in a game or not, inserting a coin or pressing the KNOCKOFF button for less than one second will add one credit (if not full) and fire the knocker.

**Start button during a game:**
- Before first point scored: Extra Start presses are ignored in Original/Impulse style; in Enhanced style, they attempt to add players (see Section 12.3).
- After first point scored: 
  - If the game has a credit: Pressing Start immediately ends the current game and begins tap detection for a new game (matching original 1954 behavior).
  - If the game has no credits: Pressing Start does nothing (remains in current game).

**Ball Recovery Behavior (All Modes):**

1. **When Start is pressed**, Master immediately:
   - Opens the Ball Tray (to allow any balls in the tray to drop into the trough)
   - Ejects any balls from the left and right kickout holes (if switch indicates ball present)
   - **Disables flippers, pop bumper, and slingshots** - they will not respond to switch closures
   - Waits for the 5-balls-in-trough switch to close

2. **If 5 balls are NOT detected within 10 seconds:**
   - **Enhanced Mode**: Play announcement "There's a ball missing somewhere. Find it and we'll get this coaster rolling!" Continue waiting up to 30 more seconds. If still missing, abort game start, refund credit(s), return to Attract mode.
   - **Original/Impulse Mode**: Wait silently. The player will intuitively realize the game hasn't started because:
     - Flippers do not respond
     - Pop bumper and slingshots do not fire
     - No points are scored
     - Any balls on playfield will drain naturally into the trough
   - Continue waiting indefinitely until 5 balls detected or Knockoff button resets system.

3. **Once 5 balls are detected**, proceed with mode-specific startup sequence.

**Rationale:** This design prevents starting a game with missing balls (which would confuse ball counting) and provides intuitive feedback to players. A player who walks up to a machine with balls stuck on the playfield will see them drain harmlessly, then the game will start normally.

### 12.2 Original/Impulse Style Start Sequence

Original style is started when the Start button tap detection window (500ms, configurable) expires without a second press.
Impulse style is started by a delayed second tap (second press MORE than 500ms after the first).

**Tap detection behavior:**
- On first Start press with credits >= 1:
  - Start the 500ms tap detection timer.
  - Do NOT begin the startup sequence yet.
  - Wait for either:
    - Second press within 500ms: Enhanced style selected.
    - Second press after 500ms: Impulse style selected.
    - Timer expires with no second press: Original style selected.
- On first Start press with credits = 0:
  - Nothing happens (no audio feedback to avoid revealing hidden styles).
  - Return to Attract mode.

**Once Original or Impulse style is selected (and 5 balls in trough and at least 1 credit available):**

  - **Startup sequence** (with Score Motor timing per Section 5.3):
    - Turn on Score Motor.
    - Deduct one credit (Motor cycle 1, Slot 2).
    - Fire the Relay Reset Bank coil (Slot 1).
    - Open the Ball Tray via `DEV_IDX_BALL_TRAY_RELEASE` (Slot 3) - **Ball Tray stays open until first point scored**.
    - Fire the Relay Reset Bank coil again (Slot 4).
    - Release the 1M/100K rapid reset (Slot 5) (Handled by Slave).
    - Reset 10K score to zero via Score Motor timing (Motor cycles 2 and possibly 3) (Handled by Slave).
    - Turn off Score Motor (after score reset complete).
  
  - **First ball dispensing**:
    - Check `SWITCH_IDX_BALL_IN_LIFT`:
      - If CLOSED (ball already in lift): Throw critical error. This should never happen with Ball 1 since we know we have 5 balls in the trough.
      - If OPEN (no ball in lift): Fire `DEV_IDX_BALL_TROUGH_RELEASE` to dispense one ball.
    - Wait briefly (~2 seconds max) for `SWITCH_IDX_BALL_IN_LIFT` to close.
    - If switch doesn't close:
      - Try dispensing again.
      - If still no ball detected, throw critical error.
  
  - **Enable gameplay devices**: Pop bumper, flipper, and slingshot solenoids now respond to corresponding switch closures. All playfield switches are monitored for scoring.
  
  - **During gameplay - ball drain handling**:
    - When any Drain Rollover (`SWITCH_IDX_DRAIN_LEFT/CENTER/RIGHT`) or Gobble Hole (`SWITCH_IDX_GOBBLE`) switch closes:
      - Ball has drained.
      - If balls remain to dispense:
        - Fire `DEV_IDX_BALL_TROUGH_RELEASE` to dispense next ball.
        - If `SWITCH_IDX_BALL_IN_LIFT` doesn't close within ~2 seconds:
          - Try dispensing again.
          - If still no ball detected, throw critical error.
      - If this was ball 5:
        - Game ends.
  
  - **Ball Tray behavior**:
    - Ball Tray stays OPEN from game start until first point is scored.
    - On first point scored: Close Ball Tray via releasing `DEV_IDX_BALL_TRAY_RELEASE`.
    - After closing, Ball Tray stays closed for rest of game.
    - Balls draining via Drain Rollovers now accumulate in Ball Tray.
  
  - **Impulse style switch (delayed second Start press)**:
    - If Start is pressed a second time AFTER 500ms but BEFORE first point scored:
      - Switch from Original to Impulse style.
      - NO additional credit deducted.
      - Only change is flipper behavior (both fire together, no hold).
      - Ball counting and all other logic remains the same.
  
  - **Start button during gameplay**:
    - Before first point scored: Additional Start presses are ignored (except for Impulse switch above).
    - After first point scored: Pressing Start (with credits) immediately ends game and begins tap detection for new game.

### 12.3 Enhanced Style Start Sequence

Enhanced style is started by a double-tap of Start (second press within 500ms of the first).

**Initial credit check (before game starts):**
  - If credits = 0:
    - Play Aoooga horn sound effect.
    - Play a random "no credit" announcement (e.g., "No coin, no joyride!").
    - Return to Attract mode.

**Ball recovery (required before game starts):**
  - See Section 12.1 - all 5 balls must be in trough before proceeding.

**Game startup sequence (once 5 balls present and credit available):**

Enhanced style uses a **silent score reset** - no Score Motor, Relay Reset Bank, or other EM sound devices during startup. This allows the Tsunami audio to provide a clean, impactful startup experience.

  - Play Scream sound effect immediately (for a fun shock to the player).
  - Deduct one credit (silent - no motor timing).
  - Send silent score reset command to Slave (Slave resets lamps immediately, no bells or 10K unit).
  - Since 5 balls are in the trough, Ball Tray can be closed at any time; no need to hold open as we're not trying to emulate an Original game.
  - Wait briefly for score reset to complete.
  - Play "Hey gang, let's ride the Screamo!" announcement.
  - Enable pop bumper, flippers, and slingshots.
  - Play "Okay kid, you're in. Press Start again if ya wanna admit any of your little friends." announcement.
  - Release the first ball into Ball Lift:
    - First check `SWITCH_IDX_BALL_IN_LIFT`:
      - If CLOSED: A ball is already at the base of the lift. Do NOT release another ball. This would normally never happen, but for multi-player games, if a previous player failed to launch their ball, it could be left there.
      - If OPEN: Fire `DEV_IDX_BALL_TROUGH_RELEASE` to release one ball into the Ball Lift.
    - Wait up to ~2 seconds for `SWITCH_IDX_BALL_IN_LIFT` to close (confirming ball arrived).
    - If still no ball detected, throw critical error.

**Player addition window (until first point is scored):**

During this window, Master monitors for the following events. These events are not mutually exclusive; multiple can occur and be handled in the same loop iteration.

  1. **Coin inserted or Knockoff button pressed:**
     - Add one credit (if not full).
     - Fire knocker.
     - Continue waiting.

  2. **Start button pressed:**
     - If < 4 players and credits > 0:
       - Deduct one credit.
       - Duck hill climb audio (if playing) while announcement plays.
       - Play appropriate announcement:
         - Player 2: "Second guest, c'mon in!"
         - Player 3: "Third guest, you're in!"
         - Player 4: "Fourth guest, through the turnstile!"
       - Continue waiting.
     - If < 4 players but credits = 0:
       - Duck hill climb audio (if playing) while announcement plays.
       - Play Aoooga horn sound effect.
       - Play random "no credit" announcement (e.g., "No free admission today!").
       - Continue waiting (player can still add coins or shoot the ball).
     - If 4 players already:
       - Duck hill climb audio (if playing) while announcement plays.
       - Play "The park's full, no more guests can join!" announcement.
       - Continue waiting.

  3. **Ball Lift switch opens (player pushes ball lift rod) - first time only:**
     - Start Ball 1 Lift sounds (looping chain-lift sound effect).
     - Start Shaker Motor at low speed.
     - Start 30-second hill climb timeout timer.
     - Mark that hill climb has started (ignore subsequent Ball Lift switch changes).
     - **Player addition window remains open** - players 2-4 can still be added.
     - Continue waiting for first point.

  4. **Hill climb timeout (30 seconds elapsed since hill climb started):**
     - Stop Ball 1 Lift sounds.
     - Stop Shaker Motor.
     - Play non-urgent "shoot the ball" reminder (e.g., "C'mon, shoot the ball already!").
     - Start periodic reminder timer (e.g., every 15 seconds).
     - Continue waiting for first point.

  5. **Periodic reminder (while waiting for first point, after hill climb timeout):**
     - Play non-urgent "shoot the ball" reminder.
     - Continue waiting.

  6. **First point scored:**
     - **Player addition window closes** - no more players can be added.
     - Close Ball Tray.
     - Stop Ball 1 Lift sounds (if still playing).
     - Play First Hill Screaming sounds.
     - Increase Shaker Motor speed (or restart if timed out), continue for several seconds, then turn off.
     - Start the first music track.
     - Enhanced style gameplay begins.

**Ball Tray strategy during Enhanced gameplay:**

Unlike Original/Impulse style where the Ball Tray closes after the first point and stays closed, Enhanced style keeps the Ball Trough "stocked" by opening the Ball Tray whenever a ball drains via Drain Rollovers:

- When `SWITCH_IDX_DRAIN_LEFT`, `SWITCH_IDX_DRAIN_CENTER`, or `SWITCH_IDX_DRAIN_RIGHT` closes:
  - Open Ball Tray via `DEV_IDX_BALL_TRAY_RELEASE`.
  - Hold open for `BALL_TRAY_DRAIN_HOLD_SECONDS` (configurable, default 8 seconds).
  - Close Ball Tray.
  - This ensures balls are available for instant dispensing (Ball Save, Multiball, Mode replacement).

- When `SWITCH_IDX_GOBBLE` closes:
  - Ball goes directly to Ball Trough (no Ball Tray involvement).
  - No action needed on Ball Tray.

**Start button handling during Enhanced gameplay:**
  - Before first point scored: Start presses attempt to add players (handled above).
  - After first point scored: Pressing Start ends the game and begins tap detection for a new game.

**Game end:**
  - Enhanced style gameplay ends when all five balls have drained for all players, or when Start is pressed after first point (with credits).

---

## 13. Original/Impulse Style Rule Details

### 13.1 Switch and Lamp Overview

- Each playfield switch has a distinct behavior and score impact, depending on whether it is lit (if it can be lit) and the status of the simulated Selection Unit and other simulated electromechanical devices.
- NOTE: When we refer to the Selection Unit and the 10K Unit, these are physical units that fire for sound only; their behavior is simulated in software.

  - Inserting a coin into the coin mech adds one credit (if not full) and fires the Knocker.
  - Original rules call for a five-ball game; no extra balls are awarded.
  - NOTE: On original game, the 10K Bell is physically tied to the 10K Unit, so every 10K score increment rings the bell. In this simulation, therefore, we will ring the 10K Bell on every 10K score increment.
  - On every 100K score increment, we will ring the 100K Bell (and also the 10K Bell, since 100K includes a 10K increment).
  - Each time a WHITE INSERT is lit (even if already lit), we will ring the Selection Bell.
  - Each time a CREDIT is added (coin mech or special), we will fire the Credit Unit step-up coil and fire the Knocker.

### 13.2 Replays

- There are a few ways to win a free game (add a credit via the Credit Unit step-up coil):
  - Each time the SCORE is updated, we will evaluate to see if a replay score has been achieved:
    - One replay each at 4.5M, 6M, 7M, 8M, and 9M points (or whatever is configured in EEPROM).
  - If all five balls drain via Gobble Hole (the SPECIAL WHEN LIT insert will be lit on the 5th ball), one replay is awarded.
  - Each time a SPOTTED NUMBER is awarded (lighting the corresponding white insert), we will evaluate the 1-9 Number Matrix for patterns that award replays:
      - Any 3-in-a-row (horizontal, vertical, or diagonal) awards 1 replay (only 1 3-in-a-row scores).
      - Four corners (1, 3, 7, 9) and center (5) awards 5 replays.
      - Making 1-3 awards 20 replays.

### 13.3 Scoring Rules

- Scoring Events:
  - Hitting either SLINGSHOT awards 10K points. This is invariable.
  - Hitting a BUMPER awards 10K points and extinguishes its lamp.
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
  - Landing in either LEFT or RIGHT KICKOUTs awards 500K points. If lit, also awards spotted number.
    - Landing in a lit Kickout does not extinguish it (because the 10K Unit controls Kickout lamps, and Kickouts do NOT advance the 10K Unit).
  - Draining via LEFT or RIGHT DRAIN ROLLOVER awards 100K points. If lit, also awards spotted number.
    - Draining via a lit Drain Rollover does not extinguish it (because the 10K Unit controls Rollover lamps, and Rollovers do NOT advance the 10K Unit).
  - Draining via CENTER DRAIN ROLLOVER awards 500K points.
  - Ball draining via GOBBLE HOLE awards 500K points and always awards spotted number.
  - If all five balls drain via Gobble Hole, a replay is also awarded.
  - Tilting the machine immediately ends the current ball (no score awarded for that ball) but does not end the game.
    - The ball is immediately drained; if any balls remain, the next ball is released.
    - If no balls remain, the game ends.

- Bumpers: There are seven bumpers, each labeled with a letter: S, C, R, E, A, M, O. Bumpers are initially lit and extinguished when hit.
  - The 'E' bumper is a pop bumper; all others are dead bumpers.

- The 1-9 Number Matrix:
  - The red numbered inserts (1-9) indicate the "spotted number" determined by the Selection Unit.
    - There is always exactly one red insert lit at any time.
  - The corresponding white numbered inserts (1-9) indicate the "awarded number", which is lit when various events occur:
    - When the ball hits a lighted "Spots Number When Lit" kickout or drain rollover switch.
    - When the ball drains via the gobble hole.
    - When all seven lit bumpers are hit.
      - When the last bumper is hit, 500K points are also awarded and the Selection Bell is rung.
  - Each time a spotted number is awarded, and the corresponding white insert is lit, the grid is examined for patterns that award replays:
    - When any 3-in-a-row (horizontal, vertical, or diagonal) is completed, a replay is awarded. Only 1 3-in-a-row scores.
    - Making the four corners (1, 3, 7, 9) and center number (5) awards five additional replays.
    - Making 1-3 scores twenty additional replays.

- The 10K Unit is directly fired by the Bumper switches, the Hat rollover switches, the Slingshot switches, and when the Selection Unit is fired (by hitting one of the nine side targets).
  - Bumpers, Hats, and Slingshots do NOT advance the Selection Unit.
  - The 10K Unit controls when the Left Kickout/Right Drain and Right Kickout/Left Drain lamps are lit ("Spots Number When Lit").
    - When the 10K Unit is at zero, Left Kickout and Right Drain lamps are lit.
    - When the 10K Unit is at 50K, Right Kickout and Left Drain lamps are lit.
    - These are independent of the Selection Unit.

- The Selection Unit is directly fired only by the 9 Side Target switches (simulated; in fact the Master fires the Selection Unit coil when any 9 Side Target is hit).
  - These devices do NOT fire the 10K Unit directly, but the Selection Unit fires the 10K Unit each time it advances (simulated).
  - We use the Selection Unit coil for sound effects, but its behavior is simulated in software.
  - The Selection Unit (simulated) has two functions:
    1. Determines which of the red 1-9 numbered "Spot" inserts are lit (one of them is always lit).
       - Various scoring events award the "spotted number", which lights the corresponding white 1-9 numbered insert.
    2. Determines when the four Hat rollover lamps are lit (in Original style, they always light and extinguish together).
  - The Selection Unit has 50 steps/states, which equates to five sets of ten steps. Beginning at a random state the steps are as follows:
    - 1st CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS LIT ON 8.
    - 2nd CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS NOT LIT.
    - 3rd CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS LIT ON 8.
    - 4th CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS NOT LIT.
    - 5th CYCLE RED INSERT LIT: 8, 3, 4, 9, 2, 5, 7, 2, 1, 6   HATS LIT ON 8.
    - Begin 1st Cycle again...
  - Since only the 9 side targets fire the Selection Unit, there are three times in every 50 side target hits when the Hats are lit.
    - Hitting any other target does not advance the Selection Unit, so the Hats stay lit until a side target is hit again.
    - There is no correlation between the 10K score unit position and the Selection Unit position.

 - The Gobble Hole switch:
   - When any of the first four balls are gobbled, the hole awards 500K points and lights the next un-lit gobble score lamp.
   - When the fourth ball is gobbled, gobble lamps 1 thru 4 will be lit, plus the Special When Lit lamp is also lit.
   - When the fifth ball is gobbled, gobble lamp 5 lights and a replay is scored (in addition to normal gobble hole features 500K plus awarding spotted number).

---

## 14. Enhanced Style Rule Details

Enhanced Style rules are based on Original style rules, with the following additions and changes:

### 14.1 General Features
- Up to four players can play in sequence.
- Shaker Motor is used to provide tactile feedback during certain events.
- Tsunami audio is used for voice, music, and sound effects.
  - There are two music themes: CIRCUS and SURF. An option in Settings determines which plays during normal play and which plays during Modes and Multiball.
  - For instance, if CIRCUS is selected as normal play music, SURF plays during Modes and Multiball.
  - At the end of each Mode or Multiball, the next normal play music track resumes from the beginning.
  - "Last track played" for both Circus and Surf themes are stored in EEPROM.
- **EM Sound Devices**:
  - During NORMAL Enhanced gameplay: All EM sound devices are used (bells, Score Motor, 10K Unit, Selection Unit, Relay Reset Bank), same as Original style.
  - During MODES and MULTIBALL: EM sound devices are NOT used. Tsunami audio provides all sound effects. This makes these special periods feel distinctly different and more exciting.
- Ball Save feature gives player a grace period (time stored in EEPROM) after first scoring a point on each ball.
  - If a ball drains without hitting any targets, even after the grace period has expired, the ball is saved and re-released.
- Side Kickouts can "lock" balls temporarily, ejecting them for MULTIBALL play (or when game ends or is tilted).
- Features can either carry over between balls and/or players, or reset at the start of each ball (to be defined).
    - If one or two balls are locked in the side kickouts, those carry over between balls and players.
- Extra balls can be awarded via yet-to-be-defined scoring events (to be defined).
- G.I. Lamps, Hat Lamps and Switches may be triggered and controlled independently (not only as a group as with Original style).
- We are not constrained by the original behaviors of the 10K Unit and Selection Unit.
  - For example, we can light or extinguish Hat lamps independently of the Selection Unit.
  - We can light or extinguish Kickout and Drain Rollover lamps independently of the 10K Unit.
  - We can define new behaviors for these devices as needed.
- All lamps may be lit, flashed, or extinguished independently (to be defined).
  - For example, multiple Red "Spot" lamps may be lit simultaneously or randomly in "motion" (to be defined).
  - All playfield lamps may be extinguished except one group, such as bumpers, during certain modes or events (to be defined).
- **Tilt behavior** (Enhanced style has a two-warning system):
  - The first time the Tilt Bob is triggered during a ball, the game plays a teasing audio line such as "Watch it!" but does NOT end the ball.
  - The second time the Tilt Bob is triggered during the SAME ball:
    - The Tilt Buzzer sound effect is played.
    - A verbal message is played such as "Nice goin', you just ruined your ride! Ball's toast."
    - The current ball immediately ends (no score awarded for that ball).
  - **Tilt warning count resets at the start of each ball.** It is not cumulative across balls or players.
  - Tilting does not end the game; play continues with the next ball (or next player's ball in multiplayer).

### 14.2 Shaker Motor

The Shaker Motor shakes the cabinet to enhance gameplay. It is controlled by Master via PWM.

- Slow shaking begins after ball has been released into Ball Lift and then the 'SWITCH_IDX_BALL_IN_LIFT' switch opens; i.e. when player pushes ball into Shooter Lane.
  - This simulates the feeling of climbing the first hill of a rollercoaster.
  - Corresponds with Tsunami sound effect of rollercoaster being hauled up the chain to the top of the first drop.
  - Shaking and sound continues until player scores the first point (hits any target, bumper, gobble, etc.).
  - This feature may be disabled after each player's first ball so it doesn't become tiresome to the player(s).

- Faster shaking begins then increases then cuts off after the first point is scored, for a few seconds.
  - This simulates the more intense event of dropping down the first hill.
  - Corresponds with Tsunami sound effect of rollercoaster dropping down the first hill and girls screaming.

- Additional shaking events may be defined later to correspond with other modes, scoring events, multiball.

### 14.3 Audio

Tsunami WAV Trigger plays voice lines, music tracks, and sound effects.
The three physical bells in the head are also used during normal Enhanced gameplay, but generally NOT when in a Mode or Multiball (targets have their own sound effects generated by the WAV Trigger).

- The file 'filenames.txt' lists all audio files on the WAV Trigger SD card, with their track numbers and descriptions.

- Voice lines are played to:
  - Announce start of game, and when players 2 to 4 are added.
  - Announce next player's turn for multi-player games.
  - Announce ball saved.
  - Announce ball tilt warnings and ball end on tilt.
  - Announce balls locked for multiball and multiball start.
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
  - IMPORTANT TODO: We need SFX for various target hits when not in a Mode!
    - Original and Impulse styles have bell(s) for every target hit, but Enhanced style DOES use the mechanical bells during normal gameplay (see Section 14.1).
    - Only during Modes and Multiball do we switch to Tsunami-only sound effects.
    - For regular Enhanced game play, target hit sounds come from the physical bells.
    - For Mode/Multiball play, target hit sounds should be special and exciting:
    - SIDE TARGETS: (to be defined)
    - BUMPERS: (to be defined)
    - GOBBLE HOLE: Tiny crowd "oohh..." as in "that's too bad" (regular play uses bells; this is for Modes)
    - HAT ROLLOVERS: (to be defined)
    - SLINGSHOTS: whip crack (for Modes)
    - 3 DRAIN ROLLOVERS: Random voice insult i.e. "Did ya forget where the flipper buttons are?"

- Primary music theme (Circus or Surf) music tracks play during regular game play.
- Secondary music theme (whichever isn't primary) music tracks play during modes and multiball.

### 14.4 Starting a New Game

- From Game Over:
  - Inserting a coin adds a credit (up to maximum) and fires knocker.
  - **Start button tap detection** (see Section 4.4 for timing details):
    - Single tap (no second press within 500ms): Original Style
    - Double tap (second press within 500ms): Enhanced Style
    - Delayed second tap (second press after 500ms): Impulse Style
  - The startup sequence does NOT begin until the style is determined (after 500ms or second press).
  - For Enhanced style:
    - Additional Start presses (before first point scored) add Players 2, 3, and 4 (one credit each).
    - New players may not be added after any points have been scored.
  - Deduct credits appropriately.
  - See Section 12.3 for detailed Enhanced style startup sequence including player addition, audio feedback, and first ball motor/audio sequence.

### 14.5 Ball Save

- For non-Multiball, non-Mode game play, after first point is scored on each ball, Ball Save is active for N seconds (N stored in EEPROM).
  - If ball drains during Ball Save period:
    - Play non-urgent "ball saved" audio line such as "Ball saved! Ride again!"
    - Release another ball into Ball Lift via 'DEV_IDX_BALL_TROUGH_RELEASE' (if balls remain in Ball Trough).
    - Only one "ball save" allowed per ball in non-multiball and non-Mode game play.
   - Since the ball save timer doesn't start until the first point is scored, it's possible for the player to drain the ball before scoring any other points; in this case, the ball is still saved.

### 14.6 Multi-Ball

- Whenever a ball lands in a side kickout, that ball is "locked" for multiball.
  - Balls can be locked during regular play and also during Mode play.
  - Upon locking the first ball:
    - Play ball locked audio line.
    - Release a new ball into the Ball Lift.
  - Upon locking the second ball:
    - Release a new ball into the Ball Lift.
  - Upon scoring the first point after two balls are locked (and NOT in a Mode):
    - Start multiball play.
    - End regular music playing.
    - Release locked balls into playfield.
    - Play multiball start audio line.
    - Start the next Mode music track (used for both multiball and Mode play).
    - **Switch from EM bells to Tsunami-only sound effects.**
  - During multiball:
    - All targets score as usual, just with more balls on the playfield.
    - Any balls that land in the side kickouts will be immediately ejected.
    - Any balls that drain within the ball-save time period (stored in EEPROM, and applies to regular balls as well) are returned to the player, along with an "urgent" ball-saved verbal announcement such as "Ball saved! Shoot it now!"
  - Multiball ends when all but one ball has drained:
    - Stop multiball music track.
    - Resume normal play music track.
    - **Switch back to EM bells for sound effects.**
  - When a second ball is locked during a Mode (Bumper Cars, etc.), the Mode continues with a new ball but multiball does NOT start until AFTER the Mode ends and the first point is scored.
 
### 14.7 Modes

- There are currently three modes (besides Multi-Ball): BUMPER CARS, ROLL-A-BALL, and GOBBLE HOLE SHOOTING GALLERY
- Each mode has a combination of shared and unique sound effects and voice prompts.
- **During all Modes: EM sound devices (bells, 10K Unit, etc.) are NOT used. Tsunami audio provides all sound effects.**
- A mode is started each of the first three times during a game (by a given player) that all bumper lamps are extinguished (and a white insert is lit.)
  - The mode is selected based on the next mode following the "last mode started" which is stored in EEPROM. This carries over for multiple players, so if one player plays Roll-A-Ball, the next mode regardless of who the player is will be Gobble Hole.
  - It's possible that with more than one player, a given player will get the same mode two times in a row (if the intervening player(s) played two modes.)
- Each mode has specific goals, scoring rules, audio lines, and shaker motor patterns (to be defined).
- Modes interrupt normal play music with mode-specific music tracks.
  - When a mode starts, we always play the same "mode-start stinger" which is a school bell sound effect.
  - The stinger is always followed by an announcement of the mode name and instructions/goals.
  - Then the next mode music track plays (a Circus or Surf track, whichever is not the normal play theme).
  - When 10 seconds are remaining, play a standard "Ten seconds remaining" audio line, and start a ten-second countdown sound effect.
  - When the mode ends, stop the mode music track, play "Time!" audio track, and play the "mode-end stinger" (always a factory whistle sound effect).
  - Since drained balls are always replaced during modes, we'll allow a grace period of a few seconds: if a ball drains within three seconds of mode end, we replace it and let the player continue with regular play.
  - The next regular music track plays from the beginning.
  - **Switch back to EM bells for sound effects after mode ends.**

#### 14.7.1

- BUMPER CARS MODE:
  - Mode starts after the all seven bumpers have been hit (and thus their lamps extinguished and a spotted number awarded). Rotating modes sequenced from BUMPER CARS to ROLL-A-BALL to GOBBLE HOLE modes (last-played mode is stored in EEPROM.)
  - Pre-mode features are remembered and restored after mode ends. If there are one or two balls locked in side kickouts, they remain locked during the mode and are available for multiball after the mode ends.
    - Note that it's possible to have two balls locked in side kickouts when the mode starts, and the first shot of the next ball happens to extinguish the last bumper lamp, thereby starting the mode with two balls locked. In this case, the next point scored AFTER the mode ends will start multiball.
  - Stop regular-play music, make an announcement "Let's ride the bumper cars...", then play the all-mode start stinger "school bell" sound effect, then start a new mode song.
  - Player must hit bumpers in S-C-R-E-A-M-O sequence for 1 Million points jackpot.
  - NO TARGETS COUNT DURING THIS MODE, not even bumpers until the last in the sequence ("O") is hit.
  - Mode starts with all playfield lights off except all seven bumper lamps are all lit.
  - Bumper hits, lit or not, trigger a car honk sound effect (Tsunami, not bells).
  - Each bumper hit in sequence turns off that bumper's lamp and plays "good shot" audio line.
  - Any of the nine side targets hit triggers a random car crash sound effect.
  - None of the targets, kickouts, rollovers, or gobble hole score any points during this mode.
  - If a ball drains during the mode but before time runs out, the countdown timer pauses, a new ball is released, an announcement is made i.e. "Here's another ball; shoot it now!" and the timer resumes when the next point is scored. This way the player is not penalized in time for the time it takes to raise and shoot a fresh ball.
  - Successful completion awards 1 Million points, the audio "Jackpot!", and the audio SFX "Ding ding ding".
  - After ten seconds, play the reminder "Keep aiming for those bumpers!" audio line.
  - Ten seconds before the mode ends, announce "Ten seconds!" and start the ten-second "mechanical timer" sound effect.
  - If the goal is completed before time runs out, the bumpers re-light and the mode restarts but with only the remaining time.
  - Failure to complete within time limit ends mode with no reward.
  - When time is up, stop mode music, play the all mode stop stinger "factory whistle" sound effect, play the all-mode "Time!" track.
  - Playfield lamps return to their previous states (i.e. if any hats or kickouts were lit before the mode started, they are re-lit).

#### 14.7.2

- ROLL-A-BALL MODE:
  - Mode starts after the all seven bumpers have been hit (and thus their lamps extinguished and a spotted number awarded). Rotating modes sequenced from BUMPER CARS to ROLL-A-BALL to GOBBLE HOLE modes (last-played mode is stored in EEPROM.)
  - Pre-mode game state is preserved and then restored when the mode ends, much as with BUMPER CARS mode.
  - Player must hit any of the four Hat Rollovers as many times as possible within the time limit.
  - Each hit awards 100K points, plays a random "bowling strike" sound effect (Tsunami), and plays a random compliment such as "good shot!"
  - Any of the nine side targets hit triggers a random glass-break sound effect.
  - ONLY HAT ROLLOVERS COUNT TOWARDS SCORE DURING THIS MODE.
  - After ten seconds, play the reminder "Keep rolling over those hats!" audio line.
  - If a ball drains during the mode but before time runs out, the countdown timer pauses, a new ball is released, and the timer resumes when the next point is scored.
  - Successful completion awards jackpot score and audio line.
  - Failure to complete within time limit ends mode with no reward, but the player does get a new ball.
  - Playfield lamps return to their previous states (i.e. if any hats or kickouts were lit before the mode started, they are re-lit).

#### 14.7.3

- GOBBLE HOLE SHOOTING GALLERY MODE:
  - Mode starts after the all seven bumpers have been hit (and thus their lamps extinguished and a spotted number awarded). Rotating modes sequenced from BUMPER CARS to ROLL-A-BALL to GOBBLE HOLE modes (last-played mode is stored in EEPROM.)
  - Player must hit the Gobble Hole as many times as possible within the time limit.
  - Each hit awards points and plays "slide whistle" and "applause" audio sound effects (Tsunami).
  - ONLY GOBBLE HOLE COUNTS DURING THIS MODE.
  - At some point we play the reminder "Keep aiming for the Gobble Hole!" audio line.
  - If a ball drains during the mode but before time runs out, the countdown timer pauses, a new ball is released, and the timer resumes when the next point is scored.
  - Every time the Gobble Hole is hit, awards jackpot score, turns on Shaker Motor for a few seconds, plays audio line, and provides a new ball to keep trying (as when a ball is drained during the mode).
  - Failure to complete within time limit ends mode with no reward, but the player does get a new ball.

### 14.8 Ball Drains

- When a ball drains (not in a mode, where a ball is always returned):
  - Play a teasing audio line.
  - Turn on Shaker Motor for a few seconds.
- If Ball Save is active, save the ball as described above, including audio line.
- If Ball Save is not active, proceed to next ball or Game Over as appropriate.
- If multiball is active and more than one ball remains in play, continue multiball.
- If multiball is not active, proceed to next ball or Game Over as appropriate.
- If Game Over:
  - Play Game Over audio line.
  - Display each player's score in sequence on score lamps as described above (see Section 10.6).
  - Resume Attract mode.

---

## 15. Open Items and Implementation Notes

- Score Motor timing API
  - Provide function to compute how many Score Motor 1/4 revs a score update will take, so Master can keep Score Motor running in sync with Slave's score lamp updates.

- Mode state machine
  - Clean, centralized state machine for modes/styles:
    - GAME_OVER, DIAGNOSTIC, ORIGINAL, ENHANCED, IMPULSE, TILT, etc.
  - Per-mode handler functions called from the 10 ms loop.

- Input debouncing and edge detection
  - Ensure Start button taps are properly detected (single, double, delayed-second).
  - Debounce switches via time thresholds or sample windows. Probably by initial edge detection, and then can't be re-detected for N ms.
    - This might entail per-switch countdown timers such as we have implemented in Slave for coils.

- Enhanced style rule engine
  - Define data structures and functions for:
    - Player progression.
    - Ball save.
    - Feature completion.
    - Voice line selection and timing.
    - Shaker motor patterns.

---

## 16. Suggested Sections to Flesh Out (For Future Docs / Copilot)

The following sections would help Copilot (and future you) a lot if you add more detail:

1. Enhanced Style Rule Details
   - Exact scoring rules, feature ladders, multiball, extra ball, ball save logic.
   - Which combinations of bumpers, targets, gobble events, etc. trigger and end:
     - Voice lines.
     - Sound effects tracks.
     - Music tracks.
     - Shaker events.
   - Definition of "roller coaster" themed modes, if any (for example multiball).

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
