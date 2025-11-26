Screamo\_Slave\_Logic.md

\# Screamo Slave Logic Overview



This document explains the timing, queuing, and device control logic implemented in `Screamo\_Slave.ino` for realistic score motor behavior and score lamp updates.



\## Goals

\- Match real score motor timing: 1/4 revolution = 882ms with 6 sub-steps (147ms each): up to 5 advances + 1 rest.

\- Use motor cycles for multi-unit steps (2..5 tens or hundreds per cycle). Larger batches repeat cycles.

\- Handle single 10K or single 100K advances without motor pacing (fast single pulses).

\- Preserve cycle “rest” behavior so sequences sound and look mechanical.

\- Prevent score saturation (0..999), prune queued work that would exceed boundaries.

\- Keep all operations non-blocking and paced via `millis()`.



\## Key Concepts and Variables



\- Timing constants:

&nbsp; - `SCORE\_MOTOR\_CYCLE\_MS = 882`

&nbsp; - `SCORE\_MOTOR\_STEPS = 6` (5 advances + 1 rest)

&nbsp; - `SCORE\_MOTOR\_STEP\_MS = 147`



\- Command queue:

&nbsp; - `scoreCmdQueue\[8]`, `scoreCmdHead`, `scoreCmdTail` a FIFO of requested score deltas in 10K units (signed).

&nbsp; - `requestScoreAdjust(delta10Ks)` enqueues; if idle, begins execution immediately.



\- Batch decomposition:

&nbsp; - `batchHundredsRemaining`: number of 100K steps (each = +10 units) still to process.

&nbsp; - `batchTensRemaining`: number of 10K steps still to process.

&nbsp; - Initial decomposition rule:

&nbsp;   - If absolute magnitude divisible by 10: treat as hundreds-only (e.g., 20 => 2×100K).

&nbsp;   - Otherwise tens-only (e.g., 7 => 7×10K).



\- Motor cycle state:

&nbsp; - `scoreAdjustActive`: true while executing a command.

&nbsp; - `scoreAdjustMotorMode`: true when pacing under score motor rules.

&nbsp; - `scoreCycleHundred`: true when cycle advances are 100K steps, false for 10K steps.

&nbsp; - `cycleActionLimit`: how many advances occur in the current cycle (1..5).

&nbsp; - `scoreAdjustCycleIndex`: sub-step index 0..5 within a cycle.

&nbsp; - `scoreAdjustLastMs`: last timed step/pulse timestamp.

&nbsp; - `motorCycleStartMs`: timestamp marking the beginning of the current cycle or single-pulse rest window.

&nbsp; - `lastCommandWasMotor`: whether the current command used motor pacing (affects required rest time).

&nbsp; - `motorPersistent`: set true for commands that start with multi-unit (>1). Forces a final single remainder to also run a full cycle.



\- Score and bounds:

&nbsp; - `currentScore` and `targetScore` are 0..999 (representing 0..9,990,000).

&nbsp; - `pruneQueuedAtBoundary(dir)` removes queued commands in a saturated direction (up when at 999, down when at 0).



\## Device Timing



\- Rapid-fire devices (10K unit coil and bells) are paced to fit within a 147ms window:

&nbsp; - 10K Unit: `timeOn = 6` ticks (60ms), rest `-8` ticks (80ms) => ~140ms.

&nbsp; - Bells: `timeOn = 5` ticks (50ms), rest `-8` ticks (80ms) => ~130ms.

&nbsp; - This ensures each scoring advance can occur at most once per motor sub-step.



\- `updateDeviceTimers()` handles the coil “on” countdown, switches to rest, and re-fires queued requests when ready.



\## Message Handling



`processMessage()`:

\- Mode, GI/Tilt, bell pulses, and 10K unit tests are handled immediately.

\- `RS485\_TYPE\_MAS\_TO\_SLV\_SCORE\_INC\_10K`:

&nbsp; - Reads signed `incAmount` (-999..999).

&nbsp; - Clamps to prevent exceeding 0..999 after considering `targetScore`.

&nbsp; - Enqueues via `requestScoreAdjust(incAmount)`.



\- `RS485\_TYPE\_MAS\_TO\_SLV\_SCORE\_RESET`:

&nbsp; - Performs a realistic reset sequence:

&nbsp;   - Rapid high reversal (millions + hundredK only), paced by a very short fixed interval.

&nbsp;   - Then one or two motor-paced tensK cycles (up to 5 advances per cycle) with a rest slot.



\## Command Start Logic



`requestScoreAdjust(delta10Ks)`:

\- Enqueues the delta and updates `targetScore`.

\- If idle:

&nbsp; - Decomposes into `batchHundredsRemaining` or `batchTensRemaining`.

&nbsp; - Determines initial pacing mode:

&nbsp;   - Multi-unit (2..5): `scoreAdjustMotorMode = true`, set `cycleActionLimit = min(remaining, 5)`, `motorPersistent = true`, `lastCommandWasMotor = true`.

&nbsp;   - Single-only (exactly 1): non-motor; `lastCommandWasMotor = false`.

&nbsp; - Sets `motorCycleStartMs = millis()` and primes `scoreAdjustLastMs` so the first sub-step can run immediately.



\## Execution and Cycle Logic



`processScoreAdjust()` runs once per 10ms loop (after 147ms spacing and when coils are idle):



\- Non-motor singles:

&nbsp; - Single 100K (exactly one hundred): fire 10K unit + 10K bell + 100K bell; update lamps; mark `lastCommandWasMotor = false`.

&nbsp; - Single 10K: fire 10K unit + 10K bell; ring 100K bell only if a boundary is crossed; mark `lastCommandWasMotor = false`.

&nbsp; - After a single pulse, set `motorCycleStartMs = millis()` to start a 147ms rest window before the next queued single starts.



\- Motor-paced cycles (6 sub-steps per cycle):

&nbsp; - For sub-steps `0..cycleActionLimit-1`: perform advances (10K or 100K) each step, firing 10K unit coil and bells appropriately.

&nbsp; - For sub-steps `cycleActionLimit..5`: rest (silent).

&nbsp; - Decrement `batchHundredsRemaining` or `batchTensRemaining` only on advance steps.



\- Cycle completion:

&nbsp; - When `scoreAdjustCycleIndex` reaches 6:

&nbsp;   - If more work remains:

&nbsp;     - Set `cycleActionLimit = min(remaining, 5)`.

&nbsp;     - If remainder is 1 and `motorPersistent = true`, keep motor mode and run one final cycle with `cycleActionLimit = 1` (advance once, then 5 rests).

&nbsp;     - If remainder is 1 and `motorPersistent = false`, drop to single non-motor pulse mode for the remainder.

&nbsp;     - Reset `motorCycleStartMs = millis()` when starting a new cycle.

&nbsp;   - If no work remains:

&nbsp;     - Clear `scoreAdjustMotorMode` and let finish logic decide next command.



\## Finish Logic and Rest Windows



`scoreAdjustFinishIfDone()`:

\- Only runs when both `batchHundredsRemaining` and `batchTensRemaining` are zero and `scoreAdjustMotorMode` is false.

\- Enforces a rest window before starting the next command:

&nbsp; - `requiredMs = lastCommandWasMotor ? 882 : 147`

&nbsp; - Waits until `millis() - motorCycleStartMs >= requiredMs`.

\- If queue not empty: begin the next command, set pacing mode, persistence, and timestamps.

\- If queue empty: finalize and persist score to EEPROM.



\## Persistence Rules



\- Single-only commands (initial magnitude exactly 1 or exactly 10):

&nbsp; - Run as single pulses with 147ms rest windows (no motor cycle).



\- Commands that start as multi-unit (initial tens or hundreds >1):

&nbsp; - Always consume full motor cycles for all units, including a final remainder of 1:

&nbsp;   - Example `Inc10K(6)` pattern:

&nbsp;     - Cycle 1: on,on,on,on,on,rest

&nbsp;     - Cycle 2: on,rest,rest,rest,rest,rest

&nbsp; - This is controlled by `motorPersistent = true`.



\## Examples



\- Inc10K(2) then Inc10K(3) then Inc10K(4) then Inc10K(5):

&nbsp; - Four distinct motor cycles:

&nbsp;   - 2 advances + 4 rests

&nbsp;   - 3 advances + 3 rests

&nbsp;   - 4 advances + 2 rests

&nbsp;   - 5 advances + 1 rest



\- Inc10K(10) repeated 7 times (single 100K pulses):

&nbsp; - Each runs as a single pulse with a ~147ms pacing (not 882ms).



\- Inc10K(20), Inc10K(3), Inc10K(-12):

&nbsp; - 200K: 2 hundred advances + 4 rests (motor)

&nbsp; - 30K: 3 tens advances + 3 rests (motor)

&nbsp; - -120K: three motor cycles of tens: 5 + rest, 5 + rest, 2 + 4 rests



\## Reset Sequence (Score Reset)

\- Phase 0 (REV\_HIGH): rapidly decrement high digits (millions + hundredK) without skipping values; lamps updated each step; then wait a full 882ms to mimic a cycle.

\- Phase 1/2 (10K cycles): motor-paced tensK advances across one or two cycles (up to 10 steps), ringing 100K bell at tens boundary without changing hundredK/million lamps.



\## Safety and Bounds

\- Commands that would exceed 0..999 after considering queued work are clamped and/or pruned.

\- Invalid device index or message type halts with an error (embedded safe-fail behavior).



\## Why This Works

\- Singles are fast and independent of the motor: one sub-step window per pulse.

\- Motor cycles enforce the full cam sequence: advances then rests.

\- Final single remainders in multi-unit commands are consumed within a full cycle for authenticity.

\- All pacing is non-blocking and synchronized with coil availability and 147ms step spacing.



