# MASTER_DESIGN

Purpose
- Compact design summary for the Master program and the Master<->Slave protocol.
- Captures important decisions made while diagnosing and fixing 'Screamo_Slave.ino'.
- Store this file in the repo so new Copilot Chats or developers can resume work without rerunning debugging history.

1) High-level overview
- Master issues RS-485 requests to Slave to change score, ring bells, or control lamps.
- Slave is authoritative for display timing and hardware pacing of score advances (matches score motor slots).
- Master must respect Slave pacing and avoid flooding Slave's RS-485 receive queue.

2) Important message types (callers / payload)
- 'RS485_TYPE_MAS_TO_SLV_SCORE_INC_10K'
  - Payload: signed int (10K units). Example: '+1' = +10K, '+10' = +100K.
  - Master should coalesce when possible (send larger delta instead of many small ones).
- 'RS485_TYPE_MAS_TO_SLV_SCORE_ABS'
  - Payload: single int (0..999). Sets 'currentScore' and 'targetScore' immediately.
- 'RS485_TYPE_MAS_TO_SLV_SCORE_RESET'
  - No payload. Starts Slave reset state machine that preempts adjusts.
- Bells, lamps, credit messages: smaller, single-command messages (no payload or single byte payloads).

3) Slave-side semantics Master must know
- 'targetScore' immediately reflects all accepted queued deltas and is clamped into '0..999'.
- 'currentScore' moves toward 'targetScore' over time via Slave's state machine.
- Slave uses a FIFO 'scoreCmdQueue' with capacity 'SCORE_CMD_QUEUE_SIZE' (currently 8).
  - Policy: when queue is full the Slave drops the incoming request and logs "Score Q FULL drop X".
  - Master should assume occasional drops if it sends bursts; prefer coalescing.
- Enqueue API: Master sends 'sendMAStoSLVScoreInc10K(value)'; Slave uses 'requestScoreAdjust(value)' to clamp/enqueue and update 'targetScore'.
- Motor pacing: Slave groups multi-unit requests into motor-paced cycles (up to 5 advances per 882ms/6 slots).
  - Sub-step interval: 'SCORE_MOTOR_STEP_MS' (147ms).
  - Full cycle: 'SCORE_MOTOR_CYCLE_MS' (882ms).

4) Key functions and invariants (Slave)
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

5) RS-485 robustness changes (applied to Slave 'Pinball_Message::getMessageRS485')
- Previously fatal paths (calls to 'endWithFlashingLED' / infinite loop) were converted to non-fatal handling:
  - Input buffer near-overflow is logged and the serial buffer is drained so main loop continues.
  - Too-short / too-long messages and bad CRC are logged and discarded instead of halting.
- Rationale: fatal halts stopped 'updateDeviceTimers()' and left coils latched ON. Non-fatal handling preserves hardware safety.

6) Queue and flood behavior - guidance for Master
- Avoid sending long bursts of individual '+1' commands. Either:
  - Coalesce into larger deltas (e.g. send '+6' instead of six separate '+1' in rapid succession), or
  - Insert small pacing delays between successive 'sendMAStoSLVScoreInc10K()' calls.
- Suggested safe pacing:
  - Prefer grouping changes that map to the same motor cycle when possible.
  - If Master starts the score motor (sound/visual effect), ensure it holds the motor ON for the expected window and then turns it OFF.
- Diagnostic: if Slave logs "Score Q FULL drop X", slow Master or increase queue capacity (tradeoff: SRAM).

7) Score motor control (Master hardware note)
- The score motor drive is an SSR (active ON/OFF), not a PWM-driven MOSFET. Use 'digitalWrite(pin, HIGH)' to start and 'digitalWrite(pin, LOW)' to stop.
- Avoid 'analogWrite(pin, HIGH)' or 'analogWrite(pin, 255)' for start/stop ambiguity. Use 'digitalWrite' for explicit semantics.

8) Timing responsibilities (Master <-> Slave)
- Slave enforces the 10ms cooperative scheduler tick for device timers.
- Master should:
  - Start score motor for the audible/mechanical effect around the time it sends the score delta messages.
  - Wait a realistic motor run duration (approx multiples of 'SCORE_MOTOR_STEP_MS' or 'SCORE_MOTOR_CYCLE_MS') before stopping motor.
  - For example, to let Slave produce 4 advances, run motor ~4 * 147ms = 588ms (Master code currently uses this).
- If Master must synchronize tightly with Slave, consider adding an ACK message from Slave (not yet implemented).

9) Diagnostics and instrumentation to add (recommended)
- Slave-side counters:
  - 'droppedScoreCmdCount' — increments when queue is full and a request is dropped.
  - 'crcFailCount', 'rs485OverflowCount' — serial errors.
  - Expose via periodic SLV->MAS diagnostics or show on LCD.
- Master-side test cases (implement in 'runBasicDiagnostics()' or new test harness):
  - Coalesced vs burst tests (verify Slave behavior).
  - Flood test with pacing and without to measure lost commands.
  - Verify that coils clear when Slave receives malformed messages (no infinite halt).

10) Short TODO list
- Consider ACK/NACK for critical messages (score deltas) so Master can retry or coalesce on NACK.
- Add optional flow-control: Slave could send 'RS485_TYPE_SLV_TO_MAS_BUSY' when its input buffer/queue exceeds threshold.
- Expose diagnostic counters on LCD via a simple command sequence.
- Replace any remaining 'analogWrite(pin, HIGH)' uses with explicit 'digitalWrite' for non-PWM SSR pins.

11) Quick reproducible tests (how to validate)
- Reproduce previous failure:
  - From Master, send a burst of 6 'sendMAStoSLVScoreInc10K(1)' with no delay. Confirm Slave no longer halts.
  - Inspect Slave LCD messages for "Score Q FULL drop ..." and check coils are not locked.
- Validate motor control:
  - Use 'sendMAStoSLVScoreInc10K(50)' and ensure Master turns on score motor (digital HIGH) for ~588ms and then off (LOW).
  - Confirm Slave displays and bells fire in expected spacing (~147ms).

12) Contacts and references
- Key files:
  - Slave logic: 'Screamo_Slave.ino' ('requestScoreAdjust', 'startBatchFromCmd', 'processScoreAdjust', 'scoreAdjustFinishIfDone')
  - Messaging: 'libraries/Pinball_Message/Pinball_Message.cpp' ('getMessageRS485', send/get helpers)
  - Master test harness: 'Screamo_Master.ino' ('runBasicDiagnostics', kickout handlers')

Notes
- Keep Master changes incremental and test after each pacing or queue-related change.
- If you want, I can:
  - Generate a compact ACK/NACK protocol patch for 'Pinball_Message'.
  - Add diagnostic counters and an RS-485 diagnostic message type.
  - Produce a minimal Master helper to coalesce score deltas before sending.

Prepared by: GitHub Copilot