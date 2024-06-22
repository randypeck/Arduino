// O_SNS.INO Rev: 06/22/24.  Finished but not tested.
// SNS reads occupancy sensors and forwards them along to MAS and anyone else who cares to listen (mode/state permitting.)
// MAS can also *request* a sensor status regardless of mode or state; we won't argue.

// DEBUG CODE: We are chirping each time there is a sensor trip that we want to send out; eliminate once established working.

// MANUAL/AUTO/PARK, RUNNING/STOPPING: Will send message to MAS whenever sensor status changes.
// REGISTRATION/ANY STATE: If a sensor is tripped or cleared, we'll trip an Emergency Stop as that is not okay!
// Otherwise will not send sensor-change messages to MAS.

// NOTE regarding sensor numbers: Sensor numbers 1..52 correspond to Centipede pins 0..51.
// All modules outside of SNS refer to Sensor numbers starting at 1, not 0.

// NOTE: An 18" passenger car measures 9.25" between wheels, so every isolated track section should have a rail AT LEAST that long.
// Should go with 10" for safety, but as little as 9.5".  (I confirmed this on 10/17/15 -- space between wheels on track is 9", or
// 9.25" on the outside rail if on a curve.  So could be safe with as little as 9.5" sensor rail between gaps.)
// NOTE: Do not cut a gap near a track joint.  The joint tie plus one more tie is okay, but the joint tie plus two more ties before
// the gap is better.  See the lift gate, south end of track 3, for an example with just one tie beyond the joint tie.  Some
// superglue would help if this must be done.

// NOTES REGARDING TIME-DELAY RELAYS, SPANNING MULTIPLE SENSORS, COPPER FOIL SENSOR LENGTH, ETC.  FOR ARCHIVE REFERENCE:
// Note that it's possible to have up to 4 sensors tripped at the same time (i.e. 14, 43, 44, and 52 with a long train moving
//   slowly), especially considering the time-delay sensor relays.
// IF THE RELAY TIME DELAY ON TWO CONSECUTIVE SENSORS IS MUCH DIFFERENT, WE COULD SEE TWO SENSORS RELEASE IN THE WRONG ORDER!
// For example, sensors that are very close together such as 14, 43, 44, and 52 -- but really any nearby block exit sensor to
//   the next block's entry sensor.
// THIS IS AN ARGUMENT TO KEEP TIME-DELAY RELAY TIMES VERY PRECISELY IDENTICAL, and delay length is less critical.
// ALSO we must ensure that no copper-foil sensor strip is shorter than the distance between the farthest-apart pair of trucks.
//   The distance between front and rear wheels on an 18" passenger car is 10-3/4", so this should be the absolute minimum
//   length of each copper-foil sensor strip, if the time-delay relays are set to near-zero delay.  However as of Jan 2021,
//   all sensor strips are 10" long, and thus short enough to be briefly spanned by passenger car wheelsets.
// MY BEST GUESS IS TO TRY TO SET EVERY TIME-DELAY RELAY TO AS CLOSE TO EXACTLY TWO SECONDS AS POSSIBLE.
//   Longer is better, due to some goofy sensor strips in blocks 14 and 16.  Two seconds should be more than enough, and
//   the time could be even longer as long as all relays (especially critical ones like 14/43/44/52) are timed the same.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_SNS;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "SNS 06/22/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage = nullptr;

// *** CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Centipede) is already in <Centipede.h> so not needed here.
// #include <Centipede.h> is already in <Train_Functions.h> so not needed here.
Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (SNS only has ONE Centipede.)

// *** CONSTANTS NEEDED TO TRACK MODE AND STATE ***
byte modeCurrent    = MODE_UNDEFINED;
byte stateCurrent   = STATE_UNDEFINED;

// *** MISC CONSTANTS AND GLOBALS NEEDED BY SNS ***

byte sensorNum = 0;
char sensorStatus = SENSOR_STATUS_CLEARED;  // Can be T|C for Tripped or Cleared

// *** SENSOR STATE TABLE: Arrays contain 4 elements (unsigned ints) of 16 bits each = 64 bits = 1 Centipede
unsigned int sensorOldState[] = {65535,65535,65535,65535};
unsigned int sensorNewState[] = {65535,65535,65535,65535};

// *** SENSOR STATUS UPDATE TABLE: Store the sensor number and change type for an individual sensor change, to pass to functions etc.
struct sensorUpdateStruct {
  byte sensorNum;
  char sensorStatus;
};
sensorUpdateStruct sensorUpdate = {0, SENSOR_STATUS_CLEARED};

// We need a delay between sensor updates on RS485 so we don't overflow the incoming buffer of other Arduinos.
// OCC overflows if delay is 30ms or less, and it looks cool having it at the same 100ms delay as turnouts
// are thrown, so we'll use 100ms.
// This is relevant when we first start, sending sensor messages to MAS for all currently-occupied blocks.
const unsigned long SENSOR_DELAY_MS = 100;       // milliseconds.
unsigned long sensorTimeUpdatedMS   = millis();  // So we don't send sensor updates more frequently than SENSOR_DELAY_MS.

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // *** INITIALIZE CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Centipede class hangs the system if hardware is not connected.
  Wire.begin();                               // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Centipede;             // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initializePinsForInput();   // Set all Centipede shift register pins to INPUT for Sensors.

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop

  // **************************************************************
  // ***** SUMMARY OF MESSAGES RECEIVED & SENT BY THIS MODULE *****
  // ***** REV: 03/10/23                                      *****
  // **************************************************************
  //
  // ********** OUTGOING MESSAGES THAT SNS WILL TRANSMIT: *********
  //
  // SNS-to-ALL: Sending 'S' sensor status byte sensorNum [1..52], and char sensorStatus [Tripped|Cleared]
  // This can be either as a response to a request from MAS for a sensor status, or we initiate after detecting a sensor change.
  // pMessage->sendSNStoALLSensorStatus(byte sensorNum, char sensorStatus);
  //
  // ********** INCOMING MESSAGES THAT SNS WILL RECEIVE: **********
  //
  // MAS-to-ALL: 'M' Mode/State change message:
  // pMessage->getMAStoALLModeState(byte &mode, byte &state);
  //
  // MAS-to-SNS: Request for SNS to 'S' Send the status of sensorNum:
  // pMessage->getMAStoSNSRequestSensor(byte &sensorNum);
  //
  // **************************************************************
  // **************************************************************
  // **************************************************************

  // See if there is an incoming message for us...
  char msgType = pMessage->available();

  // msgType ' ' (blank) means there was no message waiting for us.
  // msgType 'M' means this is a Mode/State update message.
  // msgType 'S' means MAS wants us to send the status of a sensor
  // For any message, we'll need to call the "pMessage->get" function to retrieve the actual contents of the message.

  while (msgType != ' ') {
    switch(msgType) {
      case 'M' :  // New mode/state message in incoming RS485 buffer.
        pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
        // Just calling the function updates modeCurrent and modeState ;-)
        sprintf(lcdString, "M %i S %i", modeCurrent, stateCurrent); Serial.println(lcdString);
        break;
      case 'S' :  // Request from MAS to send the status of a specific sensor.
        // So, which sensor number does MAS want the status of?  It better be 1..52, and *not* 0..51.
        pMessage->getMAStoSNSRequestSensor(&sensorNum);
        // Look at the Centipede shift register to find the status of the specified sensor in stateOfSensor().
        // We may immediately transmit that information back to MAS -- permission to xmit was implicit with the message
        pMessage->sendSNStoALLSensorStatus(sensorNum, stateOfSensor(sensorNum));
        if (stateOfSensor(sensorNum) == SENSOR_STATUS_TRIPPED) {
          sprintf(lcdString, "Sensor %i Tripped", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
        } else {
          sprintf(lcdString, "Sensor %i Cleared", sensorNum); Serial.println(lcdString);  // Not to LCD
        }
        break;
      default:
        sprintf(lcdString, "MSG TYPE ERROR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    msgType = pMessage->available();
  }

  // We have handled any incoming message.

  // See if a train tripped or cleared an occupancy sensor on the layout, and if mode/state allows, send to MAS.
  // IMPORTANT: Centipede pin 0 = sensor 1.  Which means that throughout this system, except when reading and writing Centipede
  // pins, we use Sensors 1..52.  IN THIS MODULE ONLY, we will translate to/from Centipede pin numbers and sensor numbers.

  // Read the status of the Centipede shift register and determine if there have been any new changes...
  // unsigned int sensorOldState[] = {65535,65535,65535,65535};  // Bit = 1 means sensor NOT tripped, so default all unoccupied.
  // unsigned int sensorNewState[] = {65535,65535,65535,65535};  // A bit changes to zero when it is occupied.
  // First time into loop, we will see bits set for all occupied sensors - probably several.
  // Use: pShiftRegister->portRead([0...7]) - Reads 16-bit value from one port (chip)

  // Need to keep track of "before" and "after" values.
  bool stateChange = false;
  for (byte pinBank = 0; pinBank <= 3; pinBank++) {  // This is for ONE Centipede shift register board, with four 16-bit chips.
    sensorNewState[pinBank] = pShiftRegister->portRead(pinBank);  // Populate with 4 16-bit values
    if (sensorOldState[pinBank] != sensorNewState[pinBank]) {
      stateChange = true;
      // Figure which sensor...
      // Centipede input 0 = sensor #1.
      // changedBits uses Exclusive OR (^) to find changed bits in the current long int.
      // Note that if a Centipede bit is 1, it means the sensor is NOT tripped,
      // and if the Centipede bit is 0, it means the sensor has been tripped i.e. grounded.
      unsigned int changedBits = (sensorOldState[pinBank] ^ sensorNewState[pinBank]);
      for (byte pinBit = 0; pinBit <= 15; pinBit++) {  // For each bit in this 16-bit integer (of 4)
        if ((bitRead(changedBits, pinBit)) == 1) {     // Found a bit that has changed, one way or the other - set or cleared
          // No train should be moving if we are in Registration mode, so that will be a critical error.
          if (modeCurrent == MODE_REGISTER) {
           sprintf(lcdString, "SENS TRIP IN REG!"); pLCD2004->println(lcdString); endWithFlashingLED(5);  // Bug or operator error
          }
          byte sensorNum = ((pinBank * 16) + pinBit + 1);  // Add 1 to translate from Centipede pin num to Sensor num
          // The following is a bit counter-intuitive because Centipede bit will be opposite of how we set sensorUpdate[] bit.
          // We only do a test here (rather than do it in a single clever statement) so we can display English on the LCD.
          if (bitRead(sensorNewState[pinBank], pinBit) == 1) {     // Bit changed to 1 means it is clear (not grounded)
            sensorUpdate.sensorStatus = SENSOR_STATUS_CLEARED;     // Cleared
            sprintf(lcdString, "Sensor %i Cleared", sensorNum);  // Always displays, but will only send msg if mode/state allows.
          } else {                                                 // Bit changed to 0 means it was tripped (i.e. grounded)
            sensorUpdate.sensorStatus = SENSOR_STATUS_TRIPPED;     // Tripped
            sprintf(lcdString, "Sensor %i Tripped", sensorNum);
          }
          sensorUpdate.sensorNum = sensorNum;
          // We need a slight delay in the event that we have a flurry of sensor changes to transmit to MAS, to give the
          // other modules a chance to keep up.  No delay overflowed OCC input RS485 buffer when we started a mode.
          // Note that we impose a delay ONLY if we just already sent an update, normally there will be no delay.
          while ((millis() - sensorTimeUpdatedMS) < SENSOR_DELAY_MS) {}   // Wait a moment
          sensorTimeUpdatedMS = millis();    // Refresh the timer
          // Now that we have a known sensor change, let's check to see if the mode indicates we should send it to MAS.
          // The combination of Modes and States where are are allowed (and not allowed) to send sensor updates is rather complex,
          //  so we'll put that logic in a boolean function call: modeAndStateAllowSensorUpdate().
          if ((modeAndStateAllowSensorUpdate() == true) && (sensorNum <= (TOTAL_SENSORS))) {
            // Okay, we have detected a sensor change *and* the mode/state tells us that MAS wants to know about it.
            // The Message class will automatically handle pulling the digital line low, waiting for permission to send from MAS,
            // and sending the Sensor Tripped message via RS485.  If we receive some other relevant message from MAS before we
            // receive permission to transmit the sensor update (such as a Mode Change message), the systme will halt and display
            // a messag on the LCD indicating "unexpected message."  Same general behavior for BTN, OCC, and LEG when they need to
            // send a message to MAS.
            pMessage->sendSNStoALLSensorStatus(sensorUpdate.sensorNum, sensorUpdate.sensorStatus);
            pLCD2004->println(lcdString);  // Don't display a message on the LCD until after the change has been sent to MAS
            Serial.println(lcdString);
            chirp();  // ************************************************* DEBUG CODE SO WE CAN SEE HOW LONG BEFORE A TRAIN STARTS SLOWING AFTER HITTING SENSOR ******************
          }
        }
      }
    }
  }
  // If there has been a change in sensor status, we will reset our old sensor states to reflect what was sent...
  // We do this independent of the above "check every sensor" loop because there could have been multiple sensor changes in that
  // loop, and we only want to reset the state of the sensor that was transmitted to MAS.
  if (stateChange) {
    // Set "old" Centipede shift register value to the latest value, so only new changes will be seen...
    for (byte pinBank = 0; pinBank <= 3; pinBank++) {
      sensorOldState[pinBank] = sensorNewState[pinBank];
    }
  }
}  // End of loop()

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

bool modeAndStateAllowSensorUpdate() {
  // Is the current Mode and State a condition that allows us to send a sensor change, if one is detected?
  // We will not send sensor updates if mode is REGISTER or UNDEFINED, or if state is STOPPED or UNDEFINED.
  // So we'll feel free to ask to send updates whenever we see a sensor status change (tripped/cleared) in these modes/states:
  // Undefined Mode and/or Undefined State: Not allowed.
  // Manual, Auto, and Park Mode: Running or Stopping State (though there isn't a Stopping State in Manual Mode.)
  // Registration Mode: Not allowed; in fact if a sensor changes status during Registration mode, we trigger an emergency stop.
  if (((modeCurrent == MODE_MANUAL) || (modeCurrent == MODE_AUTO) || (modeCurrent == MODE_PARK))
        &&
      ((stateCurrent == STATE_RUNNING) || (stateCurrent == STATE_STOPPING))) {
    return true;
  }
  return false;
}

char stateOfSensor(const byte t_sensorNum) {
  // This function will return the state, Tripped or Cleared, of a specified sensor number 1..52 (NOT 0..51!)
  // Since we don't call this as a result of a detected change, we don't need to update sensorOldState/sensorNewstate arrays.
  // Centipede input 0 = sensor #1.
  if ((t_sensorNum < 1) || (t_sensorNum > TOTAL_SENSORS)) {
    sprintf(lcdString, "SENSOR NUM BAD!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(6);
  }
  if (pShiftRegister->digitalRead(t_sensorNum - 1) == LOW) {
    return SENSOR_STATUS_TRIPPED;
  }
  // The only other possibility is HIGH...
  return SENSOR_STATUS_CLEARED;
}
