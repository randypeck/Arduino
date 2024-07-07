// 3x4 Membrane Keypad test code, based on Arduino Keypad and Key libraries.
// Rev: 06-30-24 by RDP

// *** NUMERIC KEYPAD ***
#include <Keypad.h>  // Also includes Key.h
const byte KEYPAD_ROWS = 4; //four KEYPAD_ROWS
const byte KEYPAD_COLS = 3; //three columns
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// KEYPAD PINS: KEYPAD_ROWS are A3..A6 (57..60), KEYPAD_COLS are A0..A2 (54..56)
// Note: Analog pins A0, A1, etc. are digital pins 54, 55, etc.
byte keypadRowPins[KEYPAD_ROWS] = {60, 59, 58, 57}; // Connect to the row pinouts of the keypad
byte keypadColPins[KEYPAD_COLS] = {56, 55, 54};     // Connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), keypadRowPins, keypadColPins, KEYPAD_ROWS, KEYPAD_COLS );

void setup(){
  Serial.begin(9600);
  while (!Serial) {}  // Wait for serial port to connect; needed for native USB port only
  Serial.println();
  Serial.print("Enter locoNum: ");
  unsigned int locoNum = getKeypadValue();
  Serial.println(locoNum);
  Serial.print("Enter Legacy speed: ");
  unsigned int legacySpeed = getKeypadValue();
  Serial.println(legacySpeed);
}
  
void loop(){
}

unsigned int getKeypadValue() {
  // Returns an unsigned int, terminated by * or #, from a 3x4 membrane keypad
  // Rev: 06-27-24 by RDP
  String stringValue = "0";
  while (true) {
    char charValue = getKeypadPress();  // 0..9 or * or #
    if ((charValue == '#') || (charValue == '*')) {  // Pressed "Enter"
      return stringValue.toInt();  // Converts an ASCII string with numberic value to an integer
    } else {  // Otherwise they hit a digit
      stringValue += charValue;
    }
  }
}

char getKeypadPress() {
  // Returns a single key press as a char from a 3x4 membrane keypad.
  // Rev: 06-27-24 by RDP
  char key = 0;
  while (key == 0) {
    key = keypad.getKey();
  }
  return key;
}

