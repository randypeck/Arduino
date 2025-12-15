# CONTRIBUTING

## Overview
Welcome to the Screamo project. This document defines mandatory contribution standards for all source files (.ino, .h, .cpp and ancillary scripts). Follow these rules exactly; project-specific standards override general style guides.

## Coding Standards
- Language Level: C++14.
- Target Platform: Arduino Mega (and related hardware modules).
- File Encoding: UTF-8, but restrict actual characters used to 7-bit ASCII only (no Unicode punctuation or symbols). Use straight single and double quotes (\' and \") and plain ASCII hyphen (-). Do not use smart quotes, en/em dashes, ellipses, or other Unicode punctuation in source files or comments.
- Quoting preference in source and comments: prefer single quote (') when referring to single characters and straight double quote (") for strings. Avoid using the backtick (`) in source comments; backticks are acceptable in Markdown documentation but should not be used inside .ino, .h, .cpp comments or identifiers.
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
  - Preserve existing descriptive comments. Do not remove or trim comments unless they are factually incorrect or describe behavior that no longer exists. When modifying code paths, update the affected comments instead of deleting them.
  - Add new comments when introducing functionality that might not be immediately obvious.
  - Use ASCII characters only; do not insert smart quotes, en/em dashes, approximate symbols, or other Unicode glyphs in source comments.
- Testing: Describe any hardware dependencies or assumptions when submitting changes.

## Project-specific C++ Usage Preferences

These rules override general C++ style recommendations where they conflict.

### Consts vs enums

For indices, categories, and flags, prefer `const` integral values instead of C++ `enum` / `enum class`:

- Use `const byte SOME_IDX = 0;` or `const uint8_t SOME_FLAG = 0x01;` for:
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
  - Plain values for small POD types (e.g. `uint8_t`, `int`, `bool`).
  - Raw pointers (`T*`, `const T*`) where shared objects are needed.
  - Existing globals where that matches current patterns (e.g. `pShiftRegister`, `pTsunami`, `pMessage`, `pLCD2004`).

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
- Prefer fixed-width integer types (`uint8_t`, `int16_t`, etc.) for values sent over the wire or stored in tables.
- When using `sizeof` for array lengths, store the result in `const` variables (e.g. `const uint8_t NUM_ITEMS = (uint8_t)(sizeof(items) / sizeof(items[0]));`).

## Testing and Safety

- Always ensure coils, motors, and high-power outputs are driven to a safe OFF state on startup and before any software reset.
- When modifying timing or power parameters for coils and motors, test on real hardware at low duty first.
- Any change that affects score handling, credits, or ball handling must be tested in both Original and Enhanced modes.

## Helpers around tables (devices, lamps, switches, audio)

- Access table entries by index (`table[idx]`) using named `const` indices.
- It is acceptable (and preferred) to read fields directly from the table into local scalars:
  - Example: `uint16_t trackNum = audioTracks[idx].trackNum;`
- Avoid clever abstractions (templates, heavy OO, RAII wrappers) that make the control flow less obvious.

## Documentation

- Update `Screamo Overview.md` when changing high-level behavior (modes, scoring rules, hardware usage).
- Keep function and file headers in `.ino` / `.cpp` files up to date with revision date and a brief summary of changes.