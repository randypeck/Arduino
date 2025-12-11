# MASTER_DESIGN

Key functions and invariants (Slave)
- 'requestScoreAdjust(int delta10Ks)'
  - Clamps magnitude and adjusts delta so 'targetScore' stays in '0..999'.
  - Enqueues delta into 'scoreCmdQueue' unless full (drop policy).
  - If idle, dequeues first command and calls 'startBatchFromCmd()'.
- 'dequeueScoreCmdIfAny(int* outCmd)'
  - Centralized circular-buffer dequeue; used by starter and finish logic.
- 'startBatchFromCmd(int cmd)'
  - Decomposes a signed delta into hundreds (100K) or tens (10K) batch counts.
  - Initializes 'scoreAdjustActive', 'scoreAdjustMotorMode', 'cycleActionLimit', timestamps and persistence flags.
- 'processScoreAdjust()'
  - Runs every Slave tick; advances 'currentScore' in motor or single-pulse mode and fires coils/bells via 'activateDevice()'.
- 'scoreAdjustFinishIfDone()'
  - Waits the required rest window (motor vs single pulse) then starts next queued command or persists score.
- 'pruneQueuedAtBoundary(int dir)'
  - Removes queued commands that would continue moving toward a saturated boundary.

Timing responsibilities (Master <-> Slave)
- Slave enforces the 10ms cooperative scheduler tick for device timers.
- Master should:
  - Start score motor for the audible/mechanical effect around the time it sends the score delta messages.
  - Wait a realistic motor run duration (approx multiples of 'SCORE_MOTOR_STEP_MS' or 'SCORE_MOTOR_CYCLE_MS') before stopping motor.
  - For example, to let Slave produce 4 advances, run motor ~4 * 147ms = 588ms (Master code currently uses this).

9) Diagnostics and instrumentation to add (recommended)
- Slave-side counters:
  - 'droppedScoreCmdCount' — increments when queue is full and a request is dropped.
  - 'crcFailCount', 'rs485OverflowCount' — serial errors.
  - Expose via periodic SLV->MAS diagnostics or show on LCD.
- Master-side test cases (implement in 'runBasicDiagnostics()' or new test harness):
  - Coalesced vs burst tests (verify Slave behavior).
  - Flood test with pacing and without to measure lost commands.
  - Verify that coils clear when Slave receives malformed messages (no infinite halt).

