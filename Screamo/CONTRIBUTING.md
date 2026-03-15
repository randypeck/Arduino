# CONTRIBUTING

## Overview
Welcome to the Screamo project. This document defines mandatory contribution standards for all source files (.ino, .h, .cpp and ancillary scripts). Follow these rules exactly; project-specific standards override general style guides.

## Architecture and Runtime Constraints

This section captures the universal facts about the project that affect every coding decision. For full details, see Screamo_Overview.md.

### Hardware Platform
- **MCU**: Two Arduino Mega 2560 R3 boards (ATmega2560, 16 MHz, 8 KB SRAM, 256 KB flash, 4 KB EEPROM).
  - **Master** (in cabinet): owns all game rules, playfield/cabinet coils and lamps, shaker motor, audio, ball handling, Score Motor, and diagnostics.
  - **Slave** (in head): owns score display (illuminated backglass lamps), head GI/Tilt lamps, three bells, 10K Unit, and Credit Unit. Receives commands from Master via RS-485.
- **I/O expansion**: Centipede shift registers on I2C. One input Centipede reads all switches (pins 64-127). One output Centipede drives all playfield lamps (pins 0-63) via relay modules.
- **Coils and motors**: driven by Arduino PWM pins through MOSFET drivers. Power level (0-255) and pulse duration (in 10ms ticks) are defined per device in `deviceParm[]`.
- **Score Motor**: original 50 VAC motor controlled via a solid-state relay (on/off only). Used for sound authenticity; all scoring logic is in software.
- **Audio**: Tsunami WAV Trigger board on Serial3 (9600 baud). Supports polyphonic playback across COM (voice), SFX, and Music channels. Controlled via the Tsunami library.
- **LCD**: Two 20x4 Digole LCDs on Serial1 (Master) and Serial1 (Slave), 115200 baud. Not player-facing; used for operator diagnostics.
- **Communication**: RS-485 half-duplex bus on Serial2 (115200 baud) between Master and Slave. Master sends score increments, lamp commands, credit operations; Slave responds to queries.

### Memory Model
- **No dynamic allocation during gameplay.** All objects (`pShiftRegister`, `pTsunami`, `pMessage`, `pLCD2004`) are allocated once in `setup()` via `new` and never freed. No `malloc`, `realloc`, or `String` objects.
- **Globals over locals** for any state that persists across ticks or is shared between functions. Game state, ball tracking, audio tracking, mode state, etc. are all file-scope globals in `Screamo_Master.ino`.
- **PROGMEM** for large constant arrays (audio track tables, lamp position tables, diagnostic descriptions). Read via `pgm_read_*` helpers.
- **EEPROM** for persistent settings (volume, theme, ball save time, replay thresholds, Selection Unit position, WHITE insert mask, gobble count). Read at startup, written periodically or on change.
- **Stack usage**: avoid deep recursion and large local arrays. The ATmega2560 has only 8 KB SRAM shared between globals, heap (minimal), and stack.

### Main Loop Timing
- **Tick-based, non-blocking main loop.** `loop()` runs every 10ms (`LOOP_TICK_MS = 10`). All game logic executes within this tick. A "slow loop" warning fires if a tick exceeds 15ms.
- **No `delay()` during active gameplay** (PHASE_BALL_IN_PLAY, PHASE_BALL_READY, PHASE_TILT). Blocking delays are acceptable only during setup sequences (PHASE_STARTUP audio playback), diagnostics, and game-over teardown where responsiveness is not critical.
- **Timer-based scheduling**: use `millis()` deadlines and per-tick countdown variables instead of blocking waits. Examples: `shakerDropStartMs`, `multiballSaveEndMs`, `ballSave.endMs`, `replayNextKnockMs`.
- **Coil timing**: `deviceParm[].countdown` counts down in ticks (not ms). Negative values represent a mandatory rest period between activations.

### Switch Processing
- **Per-tick edge detection.** `updateAllSwitchStates()` runs once per tick, reading all 40 switches from the input Centipede. It computes `switchJustClosedFlag[]` and `switchJustOpenedFlag[]` arrays.
- **Debounce**: `switchDebounceCounter[]` suppresses edge detection for `SWITCH_DEBOUNCE_TICKS` (5 ticks = 50ms) after a closure. During debounce, no edges are reported and `switchLastState[]` is frozen to prevent phantom re-triggers.
- **Extended suppression**: some switches (kickouts, drains during specific scenarios) use longer suppression periods set directly on `switchDebounceCounter[]`.

### Ball Tracking
- **Counter-based**, not position-based. `ballsInPlay` (0-3) tracks balls actively on the playfield. `ballInLift` tracks whether a ball is at the lift base. `ballsLocked` (0-2) tracks balls held in kickouts for multiball.
- **Five balls total.** The machine has exactly 5 pinballs. The 5-balls-in-trough switch (`SWITCH_IDX_5_BALLS_IN_TROUGH`) detects when all 5 are in the trough. It requires a 1-second stability check (`BALL_DETECTION_STABILITY_MS`) before trusting it, because balls rolling over the switch cause transient closures.
- **Ball dispensing**: always check `SWITCH_IDX_BALL_IN_LIFT` before firing `DEV_IDX_BALL_TROUGH_RELEASE`. If a ball is already at the lift, skip the trough release and treat the existing ball as the newly dispensed one.

### Audio Architecture
- **Three concurrent channels**: COM (voice/announcements), SFX (sound effects), and Music (background music). Each tracked by `audioCurrentComTrack`, `audioCurrentSfxTrack`, `audioCurrentMusicTrack`.
- **Music ducking**: when a COM track plays, `audioDuckMusicForVoice()` reduces music volume by `tsunamiDuckingDb` (default -20 dB). Un-ducked when COM finishes.
- **Deferred announcements**: `scheduleComTrack()` queues a COM track to play after a delay (e.g., after scoring bells finish). `processAudioTick()` fires it when the deadline arrives.
- **Per-category gain**: master gain (`tsunamiGainDb`) plus per-category offsets (`tsunamiVoiceGainDb`, `tsunamiSfxGainDb`, `tsunamiMusicGainDb`). All persisted in EEPROM.

### Game Phases
- `PHASE_ATTRACT` -> `PHASE_STARTUP` -> `PHASE_BALL_READY` -> `PHASE_BALL_IN_PLAY` -> (drain) -> `PHASE_STARTUP` (next ball) or game over -> `PHASE_ATTRACT`.
- `PHASE_TILT` is entered from `PHASE_BALL_IN_PLAY` on a full tilt; waits for all balls to drain, then advances to next ball or game over.
- `PHASE_BALL_SEARCH` is entered from `PHASE_STARTUP` if ball recovery times out (Enhanced only).
- `PHASE_DIAGNOSTIC` is entered from attract mode via the SELECT button.

### Game Styles
- **Original**: emulates the 1954 EM game. Score Motor drives timing. No audio beyond mechanical sounds. No ball save, no modes, no multiball.
- **Impulse**: like Original but both flippers fire together on either button press (brief pulse, no hold). A novelty mode.
- **Enhanced**: full modern feature set. Audio announcements, background music, shaker motor, ball save, multiball (lock 2 balls in kickouts, 3-ball multiball on next hit), timed game-within-a-game modes (Bumper Cars, Roll-A-Ball, Gobble Hole Shooting Gallery), extra ball, and replay score thresholds.

### Key Behavioral Rules
- **Non-blocking during gameplay** is the #1 constraint. Any new feature must work within the 10ms tick budget using timers and state machines, not delays.
- **All scoring goes through `addScore()`** which handles the score increment, RS-485 message to Slave, Score Motor timing, and replay threshold checks.
- **Lamp effects** are driven by a non-blocking engine (`processLampEffectTick()`) that saves/restores lamp state. Only one effect runs at a time.
- **Coil safety**: `updateDeviceTimers()` runs every tick and enforces maximum-on times and mandatory rest periods. A watchdog scan detects corrupted countdown values.
- **RS-485 is synchronous and blocking** (~2.5ms per message send). Minimize the number of RS-485 sends per tick.

## Coding Standards
- I much prefer simple and easy-to-understand code over optimized code, or code that uses more advanced C++ features.  Try to keep things as close to C as possible, while still using C++ features where they make sense.
- It's definitely okay to use C++ classes and objects where they make sense, but try to keep the interface simple and straightforward, and avoid complex patterns or abstractions that make the code harder to read or understand.
- Language Level: C++14.
- Target Platform: Arduino Mega (and related hardware modules).
- File Encoding: UTF-8, but restrict actual characters used to 7-bit ASCII only (no Unicode punctuation or symbols). Use straight single and double quotes (\' and \") and plain ASCII hyphen (-). Do not use smart quotes, en/em dashes, ellipses, or other Unicode punctuation in source files or comments.
- Quoting preference in source and comments: prefer single quote (') when referring to single characters and straight double quote (") for strings. Avoid using the backtick (`) in source comments; backticks are acceptable in Markdown documentation but should not be used inside .ino, .h, .cpp comments or identifiers.
- Line Endings: Use CRLF or LF consistently per your environment; do not mix within a file.
- Indentation: 2 spaces (no tabs) unless an existing file clearly uses a different width; maintain consistency within each file.
- Max Line Length: Prefer <= 120 characters; wrap thoughtfully without breaking readability.
- Braces:
  - K&R style: opening brace on the same line as the control statement or function definition.
- 'if' / 'else' layout:
  - Place the 'else' or 'else if' on the **same line** as the closing brace of the previous 'if' / 'else' block.
  - Preferred:

    '''cpp
    if (newVal < TSUNAMI_GAIN_DB_MIN) {
      newVal = TSUNAMI_GAIN_DB_MIN;
    } else if (newVal > TSUNAMI_GAIN_DB_MAX) {
      newVal = TSUNAMI_GAIN_DB_MAX;
    } else {
      // default handling
    }
    '''

  - Avoid:

    '''cpp
    if (newVal < TSUNAMI_GAIN_DB_MIN) {
      newVal = TSUNAMI_GAIN_DB_MIN;
    }
    else if (newVal > TSUNAMI_GAIN_DB_MAX) {
      newVal = TSUNAMI_GAIN_DB_MAX;
    }
    '''

  - Pointer and Reference Syntax:
  - Prefer **pointer syntax** over C++ reference syntax when accessing array elements or modifying struct/class members.
  - Use `Type* pVar = array + idx;` style (pointer arithmetic) for array element access.
  - Prefix pointer variable names with 'p' (e.g., pDev, pLamp, pSwitch).
  - Example (preferred):

    '''cpp
    DeviceParmStruct* pDev = deviceParm + t_devIdx;
    pDev->countdown = pDev->timeOn;
    '''

  - Avoid:

    '''cpp
    DeviceParmStruct& dev = deviceParm[t_devIdx];
    dev.countdown = dev.timeOn;
    '''
  
- Naming:
  - Constants: UPPER_SNAKE_CASE (e.g. SCORE_MOTOR_STEP_MS).
  - Globals: lowerCamelCase or descriptive snake_case consistent with existing code (e.g. currentScore, resetHighUnitsRemaining).
  - Structs / Classes: PascalCase (e.g. Pinball_Message).
  - Functions: lowerCamelCase (e.g. processScoreReset()).
  - Macros (if any new ones): UPPER_SNAKE_CASE; avoid unless essential.
- Comments:
  - Preserve existing descriptive comments. Do not remove or trim comments unless they are factually incorrect or describe behavior that no longer exists. When modifying code paths, update the affected comments instead of deleting them.
  - Add new comments when introducing functionality that might not be immediately obvious.
  - Use ASCII characters only; do not insert smart quotes, en/em dashes, approximate symbols, or other Unicode glyphs in source comments.
- Testing: Describe any hardware dependencies or assumptions when submitting changes.
- Regression Vigilance: Every time you suggest code changes or additions, review the full change set to verify no new bugs are introduced. Trace all affected code paths -- including edge cases, phase transitions, and timer/flag state -- before finalizing a suggestion.

## Bug Fix Policy

When asked to diagnose or fix a bug:

- **Trace the root cause with certainty** before proposing any code change. Read all relevant code paths, follow the data flow, and confirm that the proposed explanation fully accounts for the observed behavior.
- **If the root cause cannot be determined from the code alone**, say so explicitly. Do not guess. Instead, provide targeted debug instrumentation (e.g. `Serial.print` statements, LCD output, or flag tracking) that will capture the specific missing information needed to identify the root cause.
- **Speculative fixes are not acceptable.** A fix that "might work" or "should help" without a confirmed root cause risks making things worse, masking the real problem, or introducing new bugs. The cost of a wrong fix is higher than the cost of another debug cycle.
- **When proposing a fix, explain the chain of causation**: what state the system is in, what event triggers the bug, what the code does wrong, and exactly how the proposed change corrects it.
- This policy applies to all bug fix requests, now and in the future.

## Project Practices
- Keep behavior changes focused and well-documented in commit messages.
- When adding new hardware interactions (coils, lamps, switches, audio tracks), prefer extending the existing tables/structs rather than introducing parallel mechanisms.
- Diagnostics and test code should be clearly separated from production gameplay logic and guarded so it does not run in normal operation.
- When in doubt, match the surrounding style and structure in the file you are editing.

## Project-specific C++ Usage Preferences

These rules override general C++ style recommendations where they conflict.

### Consts vs enums

For indices, categories, and flags, prefer `const` integral values instead of C++ `enum` / `enum class`:

- Use `const byte SOME_IDX = 0;` or `const byte SOME_FLAG = 0x01;` for:
  - Device indices (coils / motors)
  - Lamp indices and groups
  - Switch indices
  - Audio categories and subcategories
- Reasons:
  - Keeps all index-style identifiers visually consistent across the codebase.
  - Avoids IntelliSense / Arduino parser quirks seen with enums in `.ino` files.

Existing enums may remain, but new code should use `const` integral values for this kind of identifier.

### References vs pointers

Avoid C++ references in this project:

- Do not introduce new function parameters or variables using `T&` or `const T&`.
- Prefer:
  - Plain values for small POD types (e.g. `byte`, `int`, `bool`).
  - Prefer 'byte' over 'uint8_t'
  - Prefer 'char' over 'int8_t'
  - Prefer 'unsigned int' over 'uint16_t'
  - Prefer 'int' over 'int16_t'
  - Prefer 'unsigned long' over 'uint32_t'
  - Prefer 'long' over 'int32_t'
  - Raw pointers (`T*`, `const T*`) where shared objects are needed.
  - Existing globals where that matches current patterns (e.g. `pShiftRegister`, `pTsunami`, `pMessage`, `pLCD2004`)

This keeps indirection explicit, avoids hidden aliasing, and matches the existing style of simple structs + tables + helper functions.

### LCD string buffer (`lcdString`)

`lcdString` is a global buffer used for formatting text sent to the Digole 20x4 LCD. Its size is fixed at `LCD_WIDTH + 1`, where `LCD_WIDTH` is 20 characters:

- The buffer holds **exactly 20 display characters plus a terminating null**.
- Any use of `sprintf`, `snprintf`, or similar functions **must guarantee** that the resulting string (excluding the terminating `\0`) is at most 20 characters long.
- Do not write longer strings and rely on truncation; overflowing `lcdString` will corrupt memory on the Arduino.

Guidelines when formatting into `lcdString`:

- Always consider the maximum value of numeric fields when designing format strings.
  - Example: `sprintf(lcdString, "Diag Page %d", page);` is safe only if the formatted result never exceeds 20 characters.
- If you change a format string or add new fields, re-check the worst-case total length against 20 characters.
- Prefer shorter labels if needed to stay within the 20-character limit.

## Arduino / AVR specifics

- Be mindful of Arduino core and AVR constraints.
- Prefer fixed-width integer types (`byte`, `int`, etc.) for values sent over the wire or stored in tables.
- When using `sizeof` for array lengths, store the result in `const` variables (e.g. `const unsigned int NUM_ITEMS = (unsigned int)(sizeof(items) / sizeof(items[0]));`).

## Testing and Safety

- Always ensure coils, motors, and high-power outputs are driven to a safe OFF state on startup and before any software reset.
- When modifying timing or power parameters for coils and motors, test on real hardware at low duty first.
- Any change that affects score handling, credits, or ball handling must be tested in both Original and Enhanced modes.

## Helpers around tables (devices, lamps, switches, audio)

- Access table entries by index (`table[idx]`) using named `const` indices.
- It is acceptable (and preferred) to read fields directly from the table into local scalars:
  - Example: `unsigned int trackNum = audioTracks[idx].trackNum;`
- Avoid clever abstractions (templates, heavy OO, RAII wrappers) that make the control flow less obvious.

## Documentation

- Update `Screamo Overview.md` when changing high-level behavior (modes, scoring rules, hardware usage).
- Keep function and file headers in `.ino` / `.cpp` files up to date with revision date and a brief summary of changes.