# CONTRIBUTING

## Overview
Welcome to the Screamo project. This document defines mandatory contribution standards for all source files (.ino, .h, .cpp and ancillary scripts). Follow these rules exactly; project-specific standards override general style guides.

## Coding Standards
- Language Level: C++14.
- Target Platform: Arduino Mega (and related hardware modules).
- File Encoding: UTF-8, but restrict actual characters used to 7-bit ASCII only (no Unicode punctuation or symbols). Use straight quotes (' ") and plain ASCII hyphen (-), etc.
- Line Endings: Use CRLF or LF consistently per your environment; do not mix within a file.
- Indentation: 2 spaces (no tabs) unless an existing file clearly uses a different width; maintain consistency within each file.
- Max Line Length: Prefer <= 120 characters; wrap thoughtfully without breaking readability.
- Braces: K&R style (opening brace on same line) as currently used in .ino files.
- Naming:
  - Constants: UPPER_SNAKE_CASE (e.g. SCORE_MOTOR_STEP_MS).
  - Globals: lowerCamelCase or descriptive snake_case consistent with existing code (e.g. currentScore, resetHighUnitsRemaining).
  - Structs / Classes: PascalCase (e.g. Pinball_Message).
  - Functions: lowerCamelCase (e.g. processScoreReset()).
  - Macros (if any new ones): UPPER_SNAKE_CASE; avoid unless essential.
- Comments:
  - Preserve existing descriptive comments; update only if behavior changes.
  - Use ASCII characters only; do not insert smart quotes, em/en dashes, approximate symbols, or other Unicode glyphs.
  - Explain timing assumptions (e.g. motor cycle ms) when changed.
- Error Handling: For fatal configuration errors in embedded context, it is acceptable to halt (e.g. while(true){}). Provide an LCD or Serial log message before halting.
- Timing: Favor non-blocking patterns (millis()-based) over delay() for runtime logic, except in setup() hardware stabilization or explicit test harness sections.
- EEPROM Writes: Minimize wear—only write when value changes.

## Score Motor & Animation Rules
- A 1/4 motor revolution is treated as 882 ms and subdivided into exactly 6 sub-steps (5 action + 1 rest) of ~147 ms each.
- Batched 10K or 100K adjustments that use motor pacing must respect sub-step timing so that a 5-step batch completes within one 882 ms quarter-cycle followed by one rest sub-step.
- Reset sequences: Do not skip intermediate 100K values when counting down. Each displayed 100K decrement is atomic and occurs at a controlled interval that fits within a single quarter-cycle for the entire high portion if possible.

## RS-485 Messaging
- All new message types must be documented in Pinball_Message.h with a clear one-line purpose.
- Avoid blocking waits for messages; poll within the 10 ms loop.

## ASCII-Only Character Policy (New Rule)
To ensure compatibility with environments limited by codepage settings:
1. Use only ASCII characters (code points 0x20–0x7E) in source and comments.
2. Replace any stylistic punctuation (smart quotes ’ “ ”, dashes — –, approximate sign ?, degree °, etc.) with plain ASCII equivalents (' " - ~) before committing.
3. Avoid invisible Unicode (zero-width space, BOM in the middle of files).
4. When copying from external documents, run a sanitization pass or manually retype punctuation.
5. Commit reviews must reject any non-ASCII characters.

## Testing & Diagnostics
- Temporary test loops (while(true)) are allowed only in dedicated diagnostic sections; label them clearly and remove before integrating gameplay logic.
- Provide LCD output for critical state transitions (reset begin/end, queue full conditions, credit edge cases).

## Performance
- Any change that might increase loop execution beyond the 10 ms tick should be profiled or refactored to remain within timing budget.

## Submitting Changes
1. Ensure code compiles with no new warnings. Particularly avoid unused variable warnings—either use or remove variables; do not leave dead code.
2. Run existing test harness sequences (score increments, resets, credit adjustments) to verify motor pacing correctness.
3. Confirm no Unicode characters by searching for pattern `[\x80-\xFF]` or using an editor "non-ASCII" highlight feature.
4. Update comments when adjusting timing constants.
5. Include brief commit message summarizing functional change and timing impact if any.

## Style Change Requests
- Propose changes to timing constants or pacing algorithms via a documented rationale (e.g. measured coil latency). Include before/after timing table.

## Review Checklist
- [ ] ASCII-only
- [ ] No added blocking delays inside main loop
- [ ] EEPROM writes minimized
- [ ] Score reset shows every 100K decrement
- [ ] Motor-paced batches finish within single quarter-cycle
- [ ] RS-485 messages drained non-blocking
- [ ] Comments accurate & preserved

Thank you for contributing while maintaining consistent behavior and readability.