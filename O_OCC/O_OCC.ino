// O_OCC.INO Rev: 04/16/24.  FINISHED BUT NOT TESTED.
// OCC paints the WHITE Occupancy Sensor LEDs and RED/BLUE Block Occupancy LEDs on the Control Panel.
// In Registration mode, OCC also prompts operator for initial data, using the Control Panel's Rotary Encoder and 8-Char display.
// In Auto/Park modes, OCC also autonomously sends arrival and departure announcements to various stations around the layout.

// OCC will maintain its own Block Res'n table exclusively to use as the default locoNum for occupied blocks during the next
// Registration.  If the operator didn't end the last session by organically stopping in Auto or Park mode, there could be any
// number of Block Res'n records that show reserved for a given loco -- but we don't care.  Even if multiple blocks still remain as
// reserved for a single loco, the loco will still be the last-known loco in that block -- our best guess.
// Block Res'n can be kept up to date during Auto/Park mode in two possible ways:
// 1. By re-scanning the entire Train Progress table for every loco, each time a Route is added and each time a sensor is tripped
// or cleared.  This is not the preferred method but it might be the easiest method.
// 2. By updating ONLY the Block Reservation records each time a Route is received and each time a sensor is tripped or cleared.
// We'll try to do it this way.

// STILL TO BE DONE:
//   Add code to autonomously manage crossing gate lower/raise (not part of Routes.)
//   Add code to autonomously manage the P.A. Announcements, when running in Auto mode (and optionally Park mode.)
//   Both of the above could potentially be controlled by the Sensor Action Table.
//   Add code to support DONE as a choice for rotary encoder when selecting occupied blocks (to default to all remaining = STATIC.)
//     This is actually a bit more complicated than I thought, so I've taken out the early code I had to support it.

// In Registration Mode, OCC prompts the operator for initialization questions such as Smoke on/off, and prompts the operator (via
// the Rotary Encoder and 8-Char Alphanumeric Display on the Control Panel) for ID of every loco or piece of equipment occupying
// every occupied sensor/block (occupied blocks default to STATIC so only non-static equipment is sent via RS485 to MAS and LEG.)
// In Auto and Park Modes, OCC also sends station announcements (such as arrival and departure) (via the attached WAV Trigger) to
// various stations on the layout, autonomously based on the Train Progress table (different than Legacy loco/tower announcements.)

// OCC paints the WHITE Sensor Occupany LEDs on the control panel as follows:
//   In All modes when State is Stopped, all WHITE LEDs will be off.
//   In Manual/Auto/Park, Running/Stopping, WHITE LEDs will be lit when equipment is tripping a given sensor.
//   In Register, Running/Stopping, there will be ONE WHITE LED lit as the prompt for which loco is occupying corresponding block.
// OCC paints the RED and BLUE Block Occupancy LEDs on the control panel as follows:
//   RED/BLUE LEDs are only illuminated during Registration, Auto, and Park modes.  They are dark in Manual mode.
//   In All modes when State is Stopped, all RED and BLUE LEDs will be off.
//   In Manual, All States, RED and BLUE LEDs are dark; we can't know of a train between Occupany Sensors so we won't try.
//   In Register, Running/Stopping, when prompting for which loco is occupying a given block, there will be ONE RED LED lit as the
//     prompt for which loco is occupying that block (white occupancy sensor will also be lit to show which end is occupied.)
//   In Auto/Park, Running/Stopping, BLUE LED indicates block is reserved, RED LED indicates block is occupied.
//     This requires OCC to maintain both Train Progress and Block Res'n tables (but not a Turnout Res'n table for OCC.)
//     BLUE/RED LEDs can change in Auto/Park mode when a sensor is Tripped or Cleared, and when a new Route is received.
//     IMPORTANT! Don't forget to illuminate STATIC trains (not part of Train Progress) as RED/OCCUPIED!
//   OCC needs to track Block Res'ns for two reasons:
//     1. To illuminate Red/Blue Block Occupancy LEDs appropriately (although this can be gleaned by scanning Train Progress only.)
//     2. To keep track of each loco's last-known block (location) for next-operating-session Registration defaults.
//     NOTE: We have a rule for Routes that we must never traverse the same turnout more than once in a Route, unless there is a
//     stop or direction-change command between the Turnout commands.  This makes it possible for the above logic to work properly,
//     and is not a problem unless we want to eventually implement a reverse loop route that does not include any intervening
//     turnouts (such as perhaps with a trolly/streetcar.)  We can update the logic at that point -- only in that we don't want to
//     throw the turnout twice but rather throw it once, and let the train return via the other leg and no problem to traverse a
//     turnout when converging no matter which way it's set.

// There is no support for blinking WHITE, RED, or BLUE LEDs on the Control Panel.  Could always add support in the future.
// In Auto/Park modes, OCC may autonomously send audio station announcements to a WAV Trigger, by following Train Progress.

// *** REGARDING AUTONOMOUS OCC STATION P.A. ANNOUNCEMENTS ***
// OCC can handle PA announcements autonomously based on Train Progress.
//   Train Progress routes (coming from MAS) says i.e. that a train will depart AND WHEN based on a delay, so OCC should be able
//   to determine fairly accurately when to make station departure announcements.
//   Train Progress maintains a timeToStart field that may be kept up to date as initial and Extension routes are received.
// Also, WAV Trigger announcements are long, and I'm not sure what to do when i.e. three trains depart or arrive at sidings at the
// same time.  The WAV Trigger does have the ability to play multiple tracks at the same time, but this would be confusing to
// listen to and difficult to track timing on.  I suspect that  if a track is already playing from a previous command, just ignore
// the new command.  OCC can check an input pin, coming from the WAV trigger, to know if a sound is currently playing or not -- so
// that should work fine.  When each phrase is playing on the WAV Trigger, it sets the Play Status pin low.  Then when the track is
// finished playing, the pin goes to 3.3 volts (not 5 volts.)  That is how OCC will know when to send the next section of the
// announcement to the WAV Trigger.

// *** REGARDING BLOCKS RESERVED FOR STATIC EQUIPMENT ***
// In order to properly paint the RED/BLUE BLOCK LEDs during AUTO/PARK modes, in addition to examining Train Progress for each
// active train and the blocks that are part of its route, OCC will need to know which blocks are occuped by STATIC equipment.
// We thus maintain a private array in the Occupancy LED class that has true/false as STATIC for each block, defined during
// Registration.  We don't use Block Reservation at all when painting occupancy LEDs.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_OCC;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "OCC 04/15/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorage = nullptr;

// *** RS485/DIGITAL MESSAGE CLASS ***
#include <Message.h>
Message* pMessage = nullptr;

// *** CENTIPEDE SHIFT REGISTER CLASS ***
// #include <Wire.h> (required by Centipede) is already in <Centipede.h> so not needed here.
// #include <Centipede.h> is already in <Train_Functions.h> so not needed here.
Centipede* pShiftRegister = nullptr;  // Only need ONE object for one or two Centipede boards (SNS only has ONE Centipede.)

// *** TURNOUT RESERVATION TABLE CLASS (IN FRAM) ***
// OCC needs to track Turnout Res'ns in order to know when it can release Block Res'ns and thus keep Control Panel LEDs accurate.
// #include <Turnout_Reservation.h>
// Turnout_Reservation* pTurnoutReservation = nullptr;

// *** SENSOR-BLOCK CROSS REFERENCE TABLE CLASS (IN FRAM) ***
#include <Sensor_Block.h>
Sensor_Block* pSensorBlock = nullptr;

// *** BLOCK RESERVATION AND LOOKUP TABLE CLASS (IN FRAM) ***
#include <Block_Reservation.h>
Block_Reservation* pBlockReservation = nullptr;

// *** LOCOMOTIVE REFERENCE LOOKUP TABLE CLASS (IN FRAM) ***
#include <Loco_Reference.h>  // Needed so we can lookup loco A/N descriptions during Registration
Loco_Reference* pLoco = nullptr;

// *** ROUTE REFERENCE TABLE CLASS (IN FRAM) ***
#include <Route_Reference.h>
Route_Reference* pRoute = nullptr;

// *** TRAIN PROGRESS TABLE CLASS (ON HEAP) ***
#include <Train_Progress.h>
Train_Progress* pTrainProgress = nullptr;

// *** 8-CHAR ALPHANUMERIC LED ARRAY CLASS ***
#include <QuadAlphaNum.h>   // Required for pair of 4-char alphanumeric displays on the control panel.
QuadAlphaNum* pAlphaNumDisplay = nullptr;
char alphaString[ALPHA_WIDTH + 1] = "        ";  // Global array to hold strings sent to alpha display.  8 chars plus \0 newline.

// *** ROTARY ENCODER ***
#include <Rotary_Prompt.h>  // Includes definition of rotaryEncoderPromptStruct data type
Rotary_Prompt* pRotaryEncoderPrompter = nullptr;

// *** CONTROL PANEL OCCUPANCY LEDS (WHITE and RED/BLUE) ***
#include <Occupancy_LEDs.h>
Occupancy_LEDs* pOccupancyLEDs = nullptr;

// *** MISC CONSTANTS AND GLOBALS NEEDED BY OCC ***

// The Rotary Prompt array is used for both various Y/N pre-registration prompts, as well as for registering trains.
// Simple prompts such as SMOKE Y/N will use only the first couple of elements.
// Loco Registration could potentially use TOTAL_TRAINS + 1 elements, which can hold the names of all 50 possible train names, plus
// 'STATIC'.  If we ever add support for 'DONE' then dimension to TOTAL_TRAINS + 2.
const byte MAX_ROTARY_PROMPTS = TOTAL_TRAINS + 1;  // + 1 allows up to every train plus STATIC; the most we'd ever need.
rotaryEncoderPromptStruct rotaryPrompt[MAX_ROTARY_PROMPTS];  // expired (bool), referenceNum, and alphaPrompt[9]
byte numRotaryPrompts   = 0;  // Gets passed to pRotaryEncoderPrompter->getSelection() as number of elements to use.

// Variables that come with new Route messages
byte         locoNum          = 0;
char         extOrCont        = ROUTE_TYPE_EXTENSION;  // or ROUTE_TYPE_CONTINUATION (E/C)
unsigned int routeRecNum      = 0;    // Record number in FRAM, 0..n
unsigned int countdown        = 0;    // Number of seconds to delay before starting train on an Extension route
byte         sensorNum        = 0;
char         trippedOrCleared = SENSOR_STATUS_CLEARED;  // or SENSOR_STATUS_TRIPPED (C/T)

byte         modeCurrent      = MODE_UNDEFINED;
byte         stateCurrent     = STATE_UNDEFINED;
char         msgType          = ' ';

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();

  // *** QUADRAM HEAP MEMORY MODULE ***
  initializeQuadRAM();  // Add-on memory board provides 56,832 bytes for heap

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(SERIAL0_SPEED);   // PC serial monitor window 115200.  Change if using thermal mini printer.
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(SERIAL2_SPEED);  // RS485 115200.
  Serial3.begin(SERIAL3_SPEED);  // WAV Trigger 9600 baud

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);

  // *** INITIALIZE FRAM CLASS AND OBJECT *** (Heap uses 76 bytes)
  // Must pass FRAM part num and Arduino pin num as parms to constructor (vs begin) because needed by parent Hackscribble_Ferro.
  // MB85RS4MT is 512KB (4Mb) SPI FRAM.  Our original FRAM was only 8KB SPI.
  pStorage = new FRAM(MB85RS4MT, PIN_IO_FRAM_CS);  // Instantiate the object and assign the global pointer
  pStorage->begin();  // Will crash on its own if there is any problem with the FRAM
  //pStorage->setFRAMRevDate(6, 18, 60);  // Available function for testing only.
  //pStorage->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
  //pStorage->testFRAM();         while (true) { }  // Writes then reads random data to entire FRAM.
  //pStorage->testSRAM();         while (true) { }  // Test 4K of SRAM just for comparison.
  //pStorage->testQuadRAM(56647); while (true) { }  // (unsigned long t_numBytesToTest); max about 56,647 testable bytes.

  // *** INITIALIZE RS485 MESSAGE CLASS AND OBJECT *** (Heap uses 30 bytes)
  // WARNING: Instantiating Message class hangs the system if hardware is not connected.
  pMessage = new Message;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pMessage->begin(&Serial2, SERIAL2_SPEED);

  // *** INITIALIZE CENTIPEDE SHIFT REGISTER ***
  // WARNING: Instantiating Centipede class hangs the system if hardware is not connected.
  Wire.begin();                               // Join the I2C bus as a master for Centipede shift register.
  pShiftRegister = new Centipede;             // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pShiftRegister->begin();                    // Set all registers to default.
  pShiftRegister->initializePinsForOutput();  // Set all Centipede shift register pins to OUTPUT for Occupancy LEDs
  
  // *** INITIALIZE TURNOUT RESERVATION CLASS AND OBJECT ***  (Heap uses 9 bytes)
  // pTurnoutReservation = new Turnout_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  // pTurnoutReservation->begin(pStorage);

  // *** INITIALIZE SENSOR-BLOCK CROSS REFERENCE CLASS AND OBJECT *** (Heap uses 9 bytes)
  pSensorBlock = new Sensor_Block;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pSensorBlock->begin(pStorage);

  // *** INITIALIZE BLOCK RESERVATION CLASS AND OBJECT *** (Heap uses 26 bytes)
  pBlockReservation = new Block_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pBlockReservation->begin(pStorage);

  // *** INITIALIZE LOCOMOTIVE REFERENCE TABLE CLASS AND OBJECT *** (Heap uses 81 bytes)
  pLoco = new Loco_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pLoco->begin(pStorage);

  // *** INITIALIZE ROUTE REFERENCE CLASS AND OBJECT ***  (Heap uses 177 bytes)
  pRoute = new Route_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pRoute->begin(pStorage);

  // *** INITIALIZE TRAIN PROGRESS CLASS AND OBJECT ***  (Heap uses 14,552 bytes)
  // WARNING: TRAIN PROGRESS MUST BE INSTANTIATED *AFTER* BLOCK RESERVATION AND ROUTE REFERENCE.
  pTrainProgress = new Train_Progress;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pTrainProgress->begin(pBlockReservation, pRoute);  // Unnecessary here?  We'll need to call each time we begin Registration.

  // *** INITIALIZE 8-CHAR ALPHANUMERIC LED DISPLAY CLASS AND OBJECT ***
  pAlphaNumDisplay = new QuadAlphaNum;
  // I2C Address of left-hand  4-char LED backpack display = 0x70;
  // I2C Address of right-hand 4-char LED backpack display = 0x71;
  pAlphaNumDisplay->begin(0x70, 0x71);
  sprintf(alphaString, "12345678");
  pAlphaNumDisplay->writeToBackpack(alphaString); delay(1000);  // Test message at startup for just one second
  sprintf(alphaString, "        ");
  pAlphaNumDisplay->writeToBackpack(alphaString);  // Clear 8-char a/n LED display

  // *** INITIALIZE ROTARY PROMPT CLASS THAT INCLUDES THE ROTARY ENCODER AND USES THE 8-CHAR ALPHA DISPLAY ***
  pRotaryEncoderPrompter = new Rotary_Prompt;
  pRotaryEncoderPrompter->begin(pAlphaNumDisplay);  // Pass this class a pointer to the 8-char alphanumeric LED display.
  // begin() inits our Static Block array, Sensor Status array, and Block Status arrays.

  // *** INITIALIZE OCCUPANY LED CLASS FOR WHITE AND RED/BLUE CONTROL PANEL LEDs ***
  // WARNING: OCCUPANCY LED MUST BE INSTANTIATED *AFTER* TRAIN PROGRESS
  pOccupancyLEDs = new Occupancy_LEDs;
  pOccupancyLEDs->begin(pTrainProgress);

}  // End of Setup

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop

  // **************************************************************
  // ***** SUMMARY OF MESSAGES RECEIVED & SENT BY THIS MODULE *****
  // ***** REV: 02/22/24                                      *****
  // **************************************************************
  //
  // ********** OUTGOING MESSAGES THAT OCC WILL TRANSMIT: *********
  //
  // OCC-to-LEG: 'F' Fast startup Fast/Slow:  REGISTRATION MODE ONLY.
  // pMessage->sendOCCtoLEGFastOrSlow(char fastOrSlow);
  //
  // OCC-to-LEG: 'K' Smoke/No smoke:  REGISTRATION MODE ONLY.
  // pMessage->sendOCCtoLEGSmokeOn(char smokeOrNoSmoke);
  //
  // OCC-to-LEG: 'A' Audio/No audio:  REGISTRATION MODE ONLY.  (Will also be used by OCC)
  // pMessage->sendOCCtoLEGAudioOn(char audioOrNoAudio);
  // 
  // OCC-to-LEG: 'D' Debug/No debug:  REGISTRATION MODE ONLY.  (Will also be seen/used my MAS)
  // pMessage->sendOCCtoLEGDebugOn(char debugOrNoDebug);
  //
  // OCC-to-ALL: 'L' Location of just-registered train: REGISTRATION MODE ONLY.  One rec/occupied sensor, real and static.
  // pMessage->sendOCCtoALLTrainLocation(byte locoNum, routeElement locoBlock);
  //
  // ********** INCOMING MESSAGES THAT OCC WILL RECEIVE: **********
  //
  // MAS-to-ALL: 'M' Mode/State change message:
  // pMessage->getMAStoALLModeState(byte &mode, byte &state);
  //
  // SNS-to-ALL: 'S' Sensor status (num [1..52] and Tripped|Cleared)
  // pMessage->getSNStoALLSensorStatus(byte &sensorNum, char &trippedOrCleared);
  //
  // MAS-to-ALL: 'R' Route sent by FRAM Route Record Number:  ONLY DURING AUTO/PARK MODES RUNNING/STOPPING STATE.
  // pMessage->getMAStoALLRoute(byte &locoNum, char &extOrCont, unsigned int &routeRecNum, unsigned int &countdown);
  // 
  // **************************************************************
  // **************************************************************
  // **************************************************************

  // See if there is an incoming message that we care about.
  // If we receive a new Mode/State message, we will always assume it's a legal transition based on MAS mode/state rules.
  // Blank means no message; otherwise need to call appropriate "pMessage->get" function to retrieve incoming data.
  // We will wait until a mode starts (MANUAL, REGISTER, or AUTO/PARK) and then process that mode until the mode ends.
  msgType = pMessage->available();
  switch (msgType) {

    case ' ':  // No message; move along, nothing to see here.
    {
      break;
    }

    case 'M':  // New mode/state message in incoming RS485 buffer *** VALID IN ANY MODE ***
    {
      pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);

      if ((modeCurrent == MODE_MANUAL) && (stateCurrent == STATE_RUNNING)) {

        // Run in Manual mode until stopped.
        OCCManualMode();
        // Now, modeCurrent == MODE_MANUAL and stateCurrent == STATE_STOPPED

      }  // *** MANUAL MODE COMPLETE! ***

      if ((modeCurrent == MODE_REGISTER) && (stateCurrent == STATE_RUNNING)) {

        // Run in Register mode until complete.
        OCCRegistrationMode();
        // Now, modeCurrent == MODE_REGISTER and stateCurrent == STATE_STOPPED

      }  // *** REGISTER MODE COMPLETE! ***

      if (((modeCurrent == MODE_AUTO) || (modeCurrent == MODE_PARK)) && (stateCurrent == STATE_RUNNING)) {

        // Run in Auto/Park mode until complete.
        OCCAutoParkMode();
        // Now, modeCurrent == MODE_AUTO or MODE_PARK, and stateCurrent == STATE_STOPPED

      }  // *** AUTO/PARK MODE COMPLETE! ***
      break;
    }

    default:  // *** ANY OTHER MESSAGE WE SEE HERE IS A BUG ***
    {
      sprintf(lcdString, "ERR MSG TYPE %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

  }
}

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

// ********************************************************************************************************************************
// **************************************************** OPERATE IN MANUAL MODE ****************************************************
// ********************************************************************************************************************************

void OCCManualMode() {

  // We will only enter this block if we just entered MANUAL RUNNING.
  // When starting MANUAL mode, MAS/SNS will ALWAYS immediately send status of every sensor.
  // Standby and recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {  // Each record should be sensorNum 1..TOTAL_SENSORS in sequence.
      sprintf(lcdString, "SNS NUM MISMATCH M"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);  // Update status but don't paint
  }

  // Okay, we've received all TOTAL_SENSORS sensor status updates, and updated the Occupancy_LEDs class.
  // Now go ahead and paint all the WHITE LEDs.
  pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
  // In Manual, all RED/BLUE Block Occupancy LEDs must be turned off.
  pOccupancyLEDs->paintOneBlockOccupancyLED(0);  // Sending 0 will turn off all RED/BLUE LEDs

  // Now operate in Manual until mode stopped
  // Just watch the Emergency Stop (halt) line, and monitor for messages:
  // * Sensor updates, which require us to refresh the Control Panel WHITE LEDs.
  // * Mode update, in which case we're done and return to the main loop.
  do {

    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, release relays and just stop

    msgType = pMessage->available();  // Could be ' ', 'S', or 'M'
    if (msgType == 'S') {  // Got a sensor message in Manual mode; update the WHITE LEDs
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
      pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);
      pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
      if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
        sprintf(lcdString, "Sensor %2d Tripped.", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      } else {
        sprintf(lcdString, "Sensor %2d Cleared.", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      }
    } else if (msgType == 'M') {  // If we get a Mode message it can only be Manual Stopped.
      pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
      if ((modeCurrent != MODE_MANUAL) || (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "MAN MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to stop Manual mode.
      // When MANUAL mode is stopped, all there is to do is turn off all the Control Panel LEDs.
      pOccupancyLEDs->paintOneOccupancySensorLED(0);  // Sending 0 will turn off all WHITE LEDs
      pOccupancyLEDs->paintOneBlockOccupancyLED(0);   // Sending 0 will turn off all RED/BLUE LEDs
    } else if (msgType != ' ') {
      // WE HAVE A BIG PROBLEM with an unexpected message type at this point
      sprintf(lcdString, "MAN MODE MSG ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
  } while (stateCurrent != STATE_STOPPED);  // We'll just assume mode is still Manual
}

// ********************************************************************************************************************************
// ************************************************ OPERATE IN REGISTRATION MODE **************************************************
// ********************************************************************************************************************************

void OCCRegistrationMode() {

  // We will only enter this code block if we just entered REGISTRATION RUNNING.
  // Each time we (re)start Registration, we'll call Occupancy_LEDs::begin() to reset the Static-Equipment Block, Sensor
  // Status, and internal Block status arrays.  No harm in calling begin() and passing the pointer more than once.
  // Do this BEFORE RECEIVE SENSOR STATUS MESSAGES, because it resets the local sensor status array that we'll be updating!
  pOccupancyLEDs->begin(pTrainProgress);  // This only initializes the OccupancyLEDs() local arrays.
  // Now paint all Control Panel Occupancy and Block LEDs dark.
  pOccupancyLEDs->paintOneOccupancySensorLED(0);  // Sending 0 will turn off all WHITE LEDs
  pOccupancyLEDs->paintOneBlockOccupancyLED(0);   // Sending 0 will turn off all RED/BLUE LEDs
  // NOTE: The following Block Reservation read (and then release) MUST take place before we receive sensor status messages:
  // Let's take advantage of the fact that we *might* have a still-accurate Block Res'n table in FRAM.  This will occur if
  // the previous operating session ended in Auto or Park mode with all locos stopped.  This can provide us with our best
  // guess of what loco might be sitting in each occupied block, when we prompt for that during Registration.  Worst case is
  // our default loco will be incorrect and the user will just need to scroll the rotary encoder to choose the right loco.
  byte defaultLocoInThisBlock[TOTAL_BLOCKS];  // Just a temporary array until we've completed Registration
  for (byte blockNum = 1; blockNum <= TOTAL_BLOCKS; blockNum++) {
    defaultLocoInThisBlock[blockNum - 1] = pBlockReservation->reservedForTrain(blockNum);
    // Could only legitimately be LOCO_ID_NULL, LOCO_ID_STATIC, or a valid locoNum 1..TOTAL_TRAINS; let's be certain.
    if ((defaultLocoInThisBlock[blockNum - 1] != LOCO_ID_NULL) &&
      (defaultLocoInThisBlock[blockNum - 1] != LOCO_ID_STATIC) &&
      ((defaultLocoInThisBlock[blockNum - 1] < 1) || (defaultLocoInThisBlock[blockNum - 1] > TOTAL_TRAINS))) {
      sprintf(lcdString, "BLK RES LOCO ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
  }
  // Now we've preserved the possible loco occupying each occupied block in defaultLocoInThisBlock[]!
  // Okay to clear the Block Res'n table...
  pBlockReservation->releaseAllBlocks();  // No blocks are reserved now.

  // When starting REGISTRATION mode, MAS/SNS will ALWAYS send status of every sensor (after a short pause to let us init.)
  // *** GET THE TRIP/CLEAR STATUS OF EVERY OCCUPANCY SENSOR ***
  // No need to clear first as we'll get them all.
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') { }  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {  // Each record should be sensorNum 1..TOTAL_SENSORS in sequence.
      sprintf(lcdString, "SNS NUM MISMATCH R"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);  // Update status array but don't paint
    // Since we know the associated block is occupied by *something*, let's reserve it for STATIC.  Then as we prompt the
    // user for the loco ID of each occupied block, we'll update the block reservation for each non-STATIC train.
    pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BE, LOCO_ID_STATIC);  // Direction is n/a
  }
  // Okay, we've received all TOTAL_SENSORS sensor status updates, updated our local array, and updated Block Res'n

  // Clear the Train Progress table and whatever other tables we might be using
  pTrainProgress->begin(pBlockReservation, pRoute);  // Reset the Train Progress table to no trains anywhere

  // *** PROMPT OPERATOR FOR STARTUP FAST/SLOW, SMOKE ON/OFF, AUDIO ON/OFF, DEBUG ON/OFF ***
  // Prompt for Fast or Slow loco power up (with or without startup dialogue.)
  rotaryPrompt[0].expired = false;
  rotaryPrompt[0].referenceNum = 0;
  strcpy(rotaryPrompt[0].alphaPrompt, "FAST PWR");
  rotaryPrompt[1].expired = false;
  rotaryPrompt[1].referenceNum = 0;
  strcpy(rotaryPrompt[1].alphaPrompt, "SLOW PWR");
  byte promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 2, 0);
  if (promptSelected == 0) {
    pMessage->sendOCCtoLEGFastOrSlow('F');  // Fast
  }
  else {
    pMessage->sendOCCtoLEGFastOrSlow('S');  // Slow
  }
  // Prompt for Smoke on or off.  Note: we may want LEG to always turn off smoke after 10 minutes automatically.
  strcpy(rotaryPrompt[0].alphaPrompt, "SMOKE N");
  strcpy(rotaryPrompt[1].alphaPrompt, "SMOKE Y");
  promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 2, 0);
  if (promptSelected == 0) {
    pMessage->sendOCCtoLEGSmokeOn('N');  // No smoke
  }
  else {
    pMessage->sendOCCtoLEGSmokeOn('S');  // Smoke
  }
  // Prompt for Audio on or off (can apply to OCC station announcements and/or LEG loco dialogue
  strcpy(rotaryPrompt[0].alphaPrompt, "AUDIO N");
  strcpy(rotaryPrompt[1].alphaPrompt, "AUDIO Y");
  promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 2, 0);
  if (promptSelected == 0) {
    pMessage->sendOCCtoLEGAudioOn('N');  // No audio
  }
  else {
    pMessage->sendOCCtoLEGAudioOn('A');  // Audio on
  }
  // Prompt for Debug Mode on or off.
  strcpy(rotaryPrompt[0].alphaPrompt, "DEBUG N");
  strcpy(rotaryPrompt[1].alphaPrompt, "DEBUG Y");
  promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 2, 0);
  if (promptSelected == 0) {
    pMessage->sendOCCtoLEGDebugOn('N');  // No debug
  }
  else {
    pMessage->sendOCCtoLEGDebugOn('D');  // Debug
  }
  // Finished prompting operator for FAST/SLOW, SMOKE/NO SMOKE, AUDIO ON/OFF, DEBUG ON/OFF.

  // Paint all the WHITE LEDs, to show where the system sees all occupancy, to help operator with next step.
  pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);

  // *** PROMPT OPERATOR FOR LOCO ID OF EVERY OCCUPIED BLOCK, ONE AT A TIME ***

  // Populate local array for the set of Loco Description prompts to use with the 8-char A/N display and Rotary Encoder.
  // The first choice for the operator will be "STATIC"
  numRotaryPrompts = 0;
  rotaryPrompt[numRotaryPrompts].expired = false;
  rotaryPrompt[numRotaryPrompts].referenceNum = LOCO_ID_STATIC;  // ID is 99, but will go in element 0.
  strcpy(rotaryPrompt[numRotaryPrompts].alphaPrompt, "STATIC");
  // Scan the entire Loco Ref table and ADD EVERY ACTIVE LOCO IN LOCOREF to the rotaryPrompt[] array.
  for (byte locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {
    if (pLoco->active(locoNum) == true) {  // If this is a Loco Ref record with an actual loco's data in it
      numRotaryPrompts++;
      rotaryPrompt[numRotaryPrompts].expired = false;
      if (pLoco->locoNum(locoNum) != locoNum) {
        sprintf(lcdString, "UNEXPECTED LOCONUM"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      rotaryPrompt[numRotaryPrompts].referenceNum = locoNum;
      pLoco->alphaDesc(locoNum, rotaryPrompt[numRotaryPrompts].alphaPrompt, 8);  // Returns text via pointer (array)
    }
  }
  // Now we have a Rotary Encoder array loaded with every active locoRef name, plus STATIC.
  // Our index into rotaryPrompt[] will be 0..(numRotaryPrompts - 1)

  // For each occuped sensor, ask operator what loco is there.
  for (sensorNum = 1; sensorNum <= TOTAL_SENSORS; sensorNum++) {  // sensorNum variable declared above setup()
    if (pOccupancyLEDs->getSensorStatus(sensorNum) == SENSOR_STATUS_TRIPPED) {  // We have an occupied block
      byte blockNum = pSensorBlock->whichBlock(sensorNum);
      char whichEnd = pSensorBlock->whichEnd(sensorNum);  // LOCO_DIRECTION_EAST == 'E', or LOCO_DIRECTION_WEST == 'W'
      // So we know that block blockNum is occupied and at which end (thus, which direction the equipment is facing.)

      // Create a temp route element for the route's starting block/dir that we can use to pass to MAS and LEG via RS485, and
      // to populate our local Train Progress table's initial route for this loco (only used if the loco is not STATIC.)
      routeElement initialBlock;
      initialBlock.routeRecVal = blockNum;
      if (whichEnd == LOCO_DIRECTION_EAST) {
        initialBlock.routeRecType = BE;
      }
      else {  // Better be LOCO_DIRECTION_WEST!
        initialBlock.routeRecType = BW;
      }
      // Illuminate the RED block-occupancy LED we want to prompt user about.  Turns all others off automatically.
      pOccupancyLEDs->paintOneBlockOccupancyLED(blockNum);

      // Recall that our best-guess locoNum that may be occupying blockNum is defaultLocoInThisBlock[blockNum - 1]
      // Since we've already populated the rotaryPrompt[] array with EVERY active locoNum, we're guaranteed that the loco
      // that was previously using this block will be found in our rotaryPrompt[] array (even if it's STATIC.)
      // Remember, we're not passing the locoNum or blockNum as the default rotaryPrompt[] array element, but rather we're
      // going to have to pass the rotaryPrompt[] array index that contains the locoNum and description for our default loco.
      // So scan rotaryPrompt[].referencNum (locoNum) until we find defaultLocoInThisBlock[blockNum - 1]
      byte startElement = 0;  // We'll default to rotary prompt element 0 = STATIC
      for (byte rotaryElementToCheck = 0; rotaryElementToCheck < numRotaryPrompts; rotaryElementToCheck++) {
        if (rotaryPrompt[rotaryElementToCheck].referenceNum == defaultLocoInThisBlock[blockNum - 1]) {
          startElement = rotaryElementToCheck;
          break;
        }
      }
      // Now if this loco was previously found in the Block Res'n table, we'll use it as our default choice for the user.
      // Prompt user for loco, starting with startElement (could be STATIC or the last-known loco in this block.)
      // getSelection() returns the offset into the rotary array that the user selected.
      byte rotaryArrayElement = pRotaryEncoderPrompter->getSelection(rotaryPrompt, numRotaryPrompts, startElement);
      // rotaryArrayElement will be the rotaryPrompt[] array index the user chose, 0..numRotaryPrompts - 1.
      byte locoSelected = rotaryPrompt[rotaryArrayElement].referenceNum;
      Serial.print("Loco selected array offset = "); Serial.println(rotaryArrayElement);
      Serial.print("Ref Num = "); Serial.println(locoSelected);  // locoNum or LOCO_ID_STATIC
      Serial.println("Item desc = "); Serial.println(rotaryPrompt[rotaryArrayElement].alphaPrompt);  // locoDesc or STATC
      // At this point, we have already updated the Block Res'n table showing every occupied block is reserved for STATIC,
      // and MAS and LEG also know about all of the STATIC blocks, so we'll only need to update our own Block Res'n and send
      // RS485 messages to MAS and LEG if it's occupied by something other than STATIC i.e. a real loco.
      if (rotaryPrompt[locoSelected].referenceNum == LOCO_ID_STATIC) {
        Serial.print("Setting block STATIC: "); Serial.println(blockNum);
        // Add this to the Occupancy_LEDs static block array (all were initialized above to not-STATIC.)  This is ONLY used
        // when painting Block Occupancy RED/BLUE LEDs, since we can't derive this from Train Progress.
        pOccupancyLEDs->setBlockStatic(blockNum);
        // No need to update Block Res'n or RS485, since they'll already have flagged all occupied blocks as STATIC, until
        // told otherwise.
      }
      else {  // Operator chose an actual loco that is occupying the block
        // First flag their selection as expired...
        rotaryPrompt[locoSelected].expired = true;
        // Now notify everyone that we have a real loco to register!  rotaryPrompt[locoSelected].referenceNum is locoNum.
        // MAS and LEG will use this to establish initial Train Progress Route, and LEG will startup loco etc.
        pMessage->sendOCCtoALLTrainLocation(rotaryPrompt[locoSelected].referenceNum, initialBlock);
        // Now take the same action ourselves when a new train is registered: establish new Train Progress rec for this loco.
        pTrainProgress->setInitialRoute(rotaryPrompt[locoSelected].referenceNum, initialBlock);
        // And update our local Block Reservation table to change the block status reservation from STATIC to this loco.
        pBlockReservation->reserveBlock(blockNum, initialBlock.routeRecType, rotaryPrompt[locoSelected].referenceNum);
      }
    }
  }
  // All occupied sensors are accounted for.
  // A Train Progress table has been initialized for this loco, with it sitting in it's originating block.
  // Block reservations are set up for every occupied block (as either STATIC or a real loco.)

  // Send a "train location" message with locoNum 0 to signal we're done.
  routeElement tempElement;
  tempElement.routeRecVal = 0;    // Block 0
  tempElement.routeRecType = BE;  // n/a
  pMessage->sendOCCtoALLTrainLocation(0, tempElement);  // locoNum 0, block BE00.

  // *** REGISTRATION COMPLETE! ***
  // Now just wait for MAS to send us a Registration mode STOPPED message.
  do {
    msgType = pMessage->available();
  } while (msgType != ' ');  // Wait for a message

  // We received a message.  It had better be Register mode Stopped!
  if (msgType != 'M') {
    // WE HAVE A BIG PROBLEM because we would only expect a Register Mode Stopped message at this point
    sprintf(lcdString, "REG MODE MSG ERR 1!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  // Okay we've received a Mode message.  So far, so good.
  pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
  // It had better be Register mode stopped, nothing else makes sense.
  if ((modeCurrent != MODE_REGISTER) || (stateCurrent != STATE_STOPPED)) {
    sprintf(lcdString, "REG MODE UPDT ERR 2!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
  }
  // Okay they want to stop Register mode.
  // When REGISTER mode is stopped, all there is to do is turn off all the Control Panel LEDs.
  pOccupancyLEDs->paintOneOccupancySensorLED(0);  // Sending 0 will turn off all WHITE LEDs
  pOccupancyLEDs->paintOneBlockOccupancyLED(0);  // Sending 0 will turn off all RED/BLUE LEDs

}

// ********************************************************************************************************************************
// ************************************************** OPERATE IN AUTO/PARK MODE ***************************************************
// ********************************************************************************************************************************

void OCCAutoParkMode() {

  // Upon entry here, modeCurrent == AUTO or PARK, and stateCurrent == RUNNING.
  // Keep the Train Progress table, Block Reservation Table, and the Control Panel white and red/blue LEDs up to date.

  // We may receive new Route (Extension/Continuation) messages to be added to Train Progress and Block Reservation.
  //   When a new Route message is received (extension or continuation), re-paint the Control Panel RED/BLUE BLOCK OCCUPANCY LEDs.
  //   No need to re-paint the white occupancy sensor LEDs when a new Route is received as they won't have changed.
  //   Also update the Block Reservation table to include the new blocks as "Reserved."
  // We may receive Sensor Trip/Clear messages to update Train Progress and release Block Reservations.
  //   When a new Sensor trip/clear message is received, re-paint the Control Panel WHITE OCCUPANCY SENSOR LEDs and also the
  //   RED/BLUE BLOCK OCCUPANCY LEDs.
  //   When a sensor clears, also update the Block Reservation table to release any blocks behind the cleared sensor, unless they
  //     occur again farther ahead in the route.

  // Upon starting Auto/Park mode, all white and red/blue LEDs will be dark, so paint them here.
  pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
  pOccupancyLEDs->paintAllBlockOccupancyLEDs();

  // State may change from STATE_RUNNING to STATE_STOPPING but that won't affect us and OCC won't see those RS485 messages.
  // So just run in Auto/Park mode until we receive a Mode message which CAN ONLY BE Auto or Park with (new) state Stopped.
  do {  // Operate in Auto/Park until mode stopped
    msgType = pMessage->available();  // Could be ' ', Sensor, Route, or Mode message (others ignored)
    // Note: If there is no message incoming for this module, msgType will be set to ' ' (blank).

    if (msgType == 'S') {  // Got a Sensor-change message in Auto/Park mode
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);  // Get the Sensor trip/clear message
      pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);   // Update internal occupancy sensor status tracker
      // Update the corresponding WHITE LED to reflect the change...
      pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
      // We can't yet update the RED/BLUE block-occupancy LEDs until we look at the Train Progress table.

      if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
        locoNum = pTrainProgress->locoThatTrippedSensor(sensorNum);

        // Advance Train Progress pointer from current Next-To-Trip each next sensor (if any) until we find the next sensor (which
        // will become the new Next-To-Trip sensor.  As we move our pointer forward, handle each route element as necessary.
        // In the event we have tripped the final sensor in the route (by checking pTrainProgress->atEndOfRoute()), then obviously
        // don't advance the pointer as we will be at the end of the route (until/unless we are assigned a new Extension route.)
        // We'll want to flag any Block records that we encounter as Occupied (which will have been previously only Reserved,)
        // which will be done automatically when we call OccupancyLEDs->paintAllBlockOccupancyLEDs().
        // First check if we're at the end of the Route (i.e. just tripped the STOP sensor) in which case OCC has nothing to do.
        if ((pTrainProgress->atEndOfRoute(locoNum) == false)) {  // If not at end of route

          byte tempTPPointer = pTrainProgress->nextToTripPtr(locoNum);  // Element number, not a sensor number, just tripped
          routeElement tempTPElement;  // Working/scratch Train Progress element

          do {
            // Advance pointer to the next element in this loco's Train Progress table...
            tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);
            tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);
            // If there was anything to do with any Train Progress elements, we would do it here.
          } while (tempTPElement.routeRecType != SN);  // Watching for the new nextToTrip sensor.
          pTrainProgress->setNextToTripPtr(locoNum, tempTPPointer);
        }
        // Now the next-to-trip pointer is up to date; whether it was already at the end of the route, or if we advanced it.

        // We *could* have tracked all blocks between the old Next-To-Trip and the new Next-To-Trip and changed from Reserved to
        // Occuped, but instead we'll let paintAllBlockOccupancyLEDs() do that by scanning the whole table.
        // Similarly, MAS *could* have thrown turnouts between the old Next-To-Trip and the new Next-To-Trip, but again, I think it
        // might be easier just to scan the whole Train Progress table for this loco and throw as needed.

        // Not necessary to change the status of the block(s) ahead of the old next-to-trip from RESERVED to OCCUPIED; they're
        // already reserved, and the paintAllBlockOccupancyLEDs() function automatically illuminates all STATIC blocks as
        // Red/Occupied, then scans Train Progress and automatically figures out which blocks should be flagged as Blue/Reserved
        // and Red/Occupied.
        // Also, the Block Reservation table isn't affected when a sensor is tripped, only possibly when a sensor is cleared,
        // because Block Res'n doesn't differentiate between RESERVED and OCCUPIED, only RESERVED and NOT_RESERVED.
      } else {  // Loco Cleared a sensor
        locoNum = pTrainProgress->locoThatClearedSensor(sensorNum);
  
        // When we clear the Next-To-Clear sensor there is much to do:
        // * All blocks between the old Tail and the old Next-To-Clear can be released IFF they don't recur again farther ahead in
        //   this route.
        // * All turnouts between the old Tail and the old Next-To-Clear can be released IFF they don't recur again farther ahead
        //   in this route (LEG only.)
        // * Update the Tail to be equal to the old Next-To-Clear.
        // * Update the Next-To-Clear to be next Sensor record ahead in the route.
        //   Note that there will always be a sensor record ahead of any sensor that is cleared, even at the end of a route.

        byte tempTPPointer = pTrainProgress->tailPtr(locoNum);  // Element number, not a sensor number
        routeElement tempTPElement;  // Working/scratch Train Progress element

        do {
          // Advance pointer to the next element in this loco's Train Progress table...
          tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);
          tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);
          // See what it is and decide if any action is necessary.
          if ((tempTPElement.routeRecType == BE) || (tempTPElement.routeRecType == BW)) {  // Block can maybe be released
            // If block does *not* recur ahead in this TP route, we can release its reservation
            if (pTrainProgress->blockOccursAgainInRoute(locoNum, tempTPElement.routeRecVal, tempTPPointer) == false) {
              pBlockReservation->releaseBlock(tempTPElement.routeRecVal);
            }
          } else if ((tempTPElement.routeRecType == TN) || (tempTPElement.routeRecType == TR)) {  // Turnout can maybe be released
              // If turnout does *not* recur ahead in this TP route, we can release its reservation (n/a for OCC)
            if (pTrainProgress->turnoutOccursAgainInRoute(locoNum, tempTPElement.routeRecVal, tempTPPointer) == false) {
              // We don't need to release Turnout reservations in OCC
            }
          }
        } while (tempTPElement.routeRecType != SN);  // Watching for the new nextToTrip sensor.
        // Now our tempTPPointer is pointing at the new tail / old next-to-clear.  Let's be sure!
        if (tempTPPointer != pTrainProgress->nextToClearPtr(locoNum)) {  // Fatal error
          sprintf(lcdString, "NTC POINTER ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
        }
        // Assign the new Tail pointer value
        pTrainProgress->setTailPtr(locoNum, pTrainProgress->nextToClearPtr(locoNum));
        // Now assign the new Next-To-Clear pointer value by traversing T.P. forward until we find the next SN sensor record.
        tempTPPointer = pTrainProgress->tailPtr(locoNum);  // Start at new Tail
        do {
          // Advance pointer to the next element in this loco's Train Progress table...
          tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);
          tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);  // We'll be searching for record type SN
        } while (tempTPElement.routeRecType != SN);  // Watching for the next sensor.
        // Found a sensor record - this will be the new Next-To-Clear.
        pTrainProgress->setNextToClearPtr(locoNum, tempTPPointer);
      }
      // Whether sensor tripped or cleared, paint the RED/BLUE Block Occupancy LEDs to reflect the loco's updated location.
      pOccupancyLEDs->paintAllBlockOccupancyLEDs();
    }  // End of "if we received a Senor tripped or cleared message

    else if (msgType == 'R') {  // We've got a new Route to add to Train Progress.  Could be Continuation or Extension.

      // 04/11/24: Add the new Route to Train Progress, update Block (and Turnout reservations if MAS), (and re-paint red/blue
      // sensors if OCC).
      // First, get the incoming Route message which we know is waiting for us...
      pMessage->getMAStoALLRoute(&locoNum, &extOrCont, &routeRecNum, &countdown);
      // Add Route to Train Progress, update reserved Blocks and Turnouts, then re-paint the Red/Blue LEDs on the Control Panel.
      if (extOrCont == ROUTE_TYPE_EXTENSION) {
        // Add this extension route to Train Progress (our train is currently stopped.)
        pTrainProgress->setExtensionRoute(locoNum, routeRecNum, countdown);
      } else if (extOrCont == ROUTE_TYPE_CONTINUATION) {
        // Add this continuation route to Train Progress (our train is currently moving.)
        pTrainProgress->setContinuationRoute(locoNum, routeRecNum);
      }
      // We'll also need to Reserve every block (and turnout, if MAS) that's part of the Route just added.
      // We've already added the elements to Train Progress and updated those pointers, so probably easiest to just scan the entire
      // Train Progress table from Tail to Head for this loco and (re)reserve the blocks and turnouts.
      byte tempTPPointer = pTrainProgress->tailPtr(locoNum);  // Element number, not a sensor number
      routeElement tempTPElement;  // Working/scratch Train Progress element
      while (tempTPPointer != pTrainProgress->stopPtr(locoNum)) {  // Stop pointer is always two elements before head, so AOK
        tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);
        // See what it is and decide if any action is necessary.
        if ((tempTPElement.routeRecType == BE) || (tempTPElement.routeRecType == BW)) {  // Block must be reserved
          pBlockReservation->reserveBlock(tempTPElement.routeRecVal, tempTPElement.routeRecType, locoNum);
        } else if ((tempTPElement.routeRecType == TN) || (tempTPElement.routeRecType == TR)) {  // Turnout must be reserved
          // We don't need to reserve Turnout reservations in OCC
        }
        // Advance pointer to the next element in this loco's Train Progress table...
        tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);
      }  // We've reached the end of the Route.  All Blocks and Turnouts (re)reserved.
      // The White sensor LEDs aren't affected when we add a new Route, but the Red/Blue Block LEDs need to be updated.
      pOccupancyLEDs->paintAllBlockOccupancyLEDs();
    }

    else if (msgType == 'M') {  // If we get a Mode message it can only be Auto/Park Stopped and we're done here.
      // Message class will have filtered out Mode Auto/Park, State STOPPING for OCC because it's irrelevant.
      // We'll just check to be sure...
      if (((modeCurrent != MODE_AUTO) && (modeCurrent != MODE_PARK)) ||
        (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "AUTO MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to stop Auto/Park mode.  It must mean all locos are stopped.
      // We will just fall out of the loop below since stateCurrent is now STATE_STOPPED
    }

    else if (msgType != ' ') {  // AT this point, the only other valid response from pMessage->available() is BLANK (no message.)
      // Any message type other than S, R, M, or Blank means a serious bug.
      sprintf(lcdString, "AUTO MODE MSG ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

  } while (stateCurrent != STATE_STOPPED);

  // Okay, Auto/Park mode has organically stopped!
  // When AUTO/PARK mode is stopped, turn off all the Control Panel LEDs to make it clear that Auto/Park mode has stopped.
  pOccupancyLEDs->paintOneOccupancySensorLED(0);  // Sending 0 will turn off all WHITE LEDs
  pOccupancyLEDs->paintOneBlockOccupancyLED(0);   // Sending 0 will turn off all RED/BLUE LEDs

}

// ********************************************************************************************************************************
// ********************************************************************************************************************************
// ********************************************************************************************************************************



// These are the possible values of any given Train Progress record that we may want to deal with...
//      const byte ER =  2;  // End-Of-Route
//      const byte SN =  3;  // Sensor
//      const byte BE =  4;  // Block Eastbound
//      const byte BW =  5;  // Block Westbound
//      const byte TN =  6;  // Turnout Normal
//      const byte TR =  7;  // Turnout Reverse
//      const byte FD =  8;  // Direction Forward
//      const byte RD =  9;  // Direction Reverse
//      const byte VL = 10;  // Velocity (NOTE: "SP" IS RESERVED IN ARDUINO) (values can be SPEED_STOP = 0, thru SPEED_HIGH = 4.)
//      const byte TD = 11;  // Time Delay in seconds (not ms)
//      const byte BX = 12;  // Deadlock table only.  Block either direction is a threat.
//      const byte SC = 13;  // Sensor Clear (not implemented yet, as of 1/23)
