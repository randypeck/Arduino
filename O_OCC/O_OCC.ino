// O_OCC.INO Rev: 09/27/24.
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
//     Both of the above could potentially be controlled by a Sensor Action Table.
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
//     1. To illuminate Red/Blue Block Occupancy LEDs appropriately.  Although active train locations, both reserved and occupied,
//        can and are determined by scanning Train Progress, the Block Res'n table shows us blocks that were reserved for STATIC
//        equipment during Registration.
// We also maintain a private array in the Occupancy LED class that has true/false as STATIC for each block, defined during Reg'n. ****************************

// Clarify the above -- do we find static equipment via Block Res'n or our private array?  When do we define the contents of the
// private array?  During Reg'n and it's held over into Auto/Park???? *****************************************************************************************
// Occupancy_LEDs class maintains bool m_staticBlock[TOTAL_BLOCKS] true or false.


//     2. To keep track of each loco's last-known block (location) for next-operating-session Registration defaults.

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

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_OCC;  // Global needed by Train_Functions.cpp and Message.cpp functions.
char lcdString[LCD_WIDTH + 1] = "OCC 09/27/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

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
// OCC doesn't care about Turnout Reservations for any purpose.
#include <Turnout_Reservation.h>
Turnout_Reservation* pTurnoutReservation = nullptr;

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

// Variables that come with new Route messages
byte         locoNum          = 0;
char         extOrCont        = ROUTE_TYPE_EXTENSION;  // or ROUTE_TYPE_CONTINUATION (E/C)
unsigned int routeRecNum      = 0;  // Record number in FRAM, 0..n
unsigned int countdown        = 0;  // Number of seconds to delay before starting train on an Extension route
byte         sensorNum        = 0;
char         trippedOrCleared = SENSOR_STATUS_CLEARED;  // or SENSOR_STATUS_TRIPPED (C/T)

byte         modeCurrent      = MODE_UNDEFINED;
byte         stateCurrent     = STATE_UNDEFINED;
char         msgType          = ' ';

char fastOrSlow = ' ';  // Loco startup can be F|S
char smokeOn    = ' ';  // Smoke can be S|N
char audioOn    = ' ';  // Audio can be A|N
bool debugOn    = false;

// The Rotary Prompt array is used for both various Y/N pre-registration prompts, as well as for registering trains.
// Simple prompts such as SMOKE Y/N will use only the first couple of elements.
// Loco Registration could potentially use TOTAL_TRAINS + 1 elements, which can hold the names of all 50 possible train names, plus
// 'STATIC'.  If we ever add support for 'DONE' then dimension to TOTAL_TRAINS + 2.
const byte MAX_ROTARY_PROMPTS = TOTAL_TRAINS + 1;  // + 1 allows up to every train plus STATIC; the most we'd ever need.
rotaryEncoderPromptStruct rotaryPrompt[MAX_ROTARY_PROMPTS];  // expired (bool), referenceNum, and alphaPrompt[9]
byte topRotaryElement = 0;  // Gets passed to pRotaryEncoderPrompter->getSelection() as number of elements to use.

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
  //pStorage->testFRAM();         while (true) {}  // Writes then reads random data to entire FRAM.
  //pStorage->testSRAM();         while (true) {}  // Test 4K of SRAM just for comparison.
  //pStorage->testQuadRAM(56647); while (true) {}  // (unsigned long t_numBytesToTest); max about 56,647 testable bytes.

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
   pTurnoutReservation = new Turnout_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
   pTurnoutReservation->begin(pStorage);

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
  pTrainProgress->begin(pBlockReservation, pRoute);

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

}  // End of setup()

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, just stop

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
  // MAS-to-ALL: 'R' Route sent by FRAM Route Record Number:  ONLY DURING AUTO/PARK MODES.
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
        sprintf(lcdString, "BEGIN MAN MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        OCCManualMode();
        sprintf(lcdString, "END MAN MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Now, modeCurrent == MODE_MANUAL and stateCurrent == STATE_STOPPED

      }  // *** MANUAL MODE COMPLETE! ***

      if ((modeCurrent == MODE_REGISTER) && (stateCurrent == STATE_RUNNING)) {

        // Run in Register mode until complete.
        sprintf(lcdString, "BEGIN REG MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        OCCRegistrationMode();
        sprintf(lcdString, "END REG MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Now, modeCurrent == MODE_REGISTER and stateCurrent == STATE_STOPPED

      }  // *** REGISTER MODE COMPLETE! ***

      if (((modeCurrent == MODE_AUTO) || (modeCurrent == MODE_PARK)) && (stateCurrent == STATE_RUNNING)) {

        // Run in Auto/Park mode until complete.
        sprintf(lcdString, "BEGIN AUTO MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        OCCAutoParkMode();
        sprintf(lcdString, "END AUTO MODE"); pLCD2004->println(lcdString); Serial.println(lcdString);
        // Now, modeCurrent == MODE_AUTO or MODE_PARK, and stateCurrent == STATE_STOPPED

      }  // *** AUTO/PARK MODE COMPLETE! ***
      break;
    }

    default:  // *** ANY OTHER MESSAGE WE SEE HERE IS A BUG ***
    {
      sprintf(lcdString, "ERR MSG TYPE %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

  }
}  // End of loop()

// ********************************************************************************************************************************
// ********************************************************* FUNCTIONS ************************************************************
// ********************************************************************************************************************************

// ********************************************************************************************************************************
// **************************************************** OPERATE IN MANUAL MODE ****************************************************
// ********************************************************************************************************************************

void OCCManualMode() {  // CLEAN THIS UP SO THAT MAS/OCC/LEG ARE MORE CONSISTENT WHEN DOING THE SAME THING I.E. RETRIEVING SENSORS AND UPDATING SENSOR-BLOCK *******************************

  // When starting MANUAL mode, MAS/SNS will always immediately send status of every sensor.
  // Recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {
      sprintf(lcdString, "SNS %i %i NUM ERR", i, sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", i, trippedOrCleared); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
    if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
      sprintf(lcdString, "Sensor %i Tripped", i); pLCD2004->println(lcdString); Serial.println(lcdString);
    } else {
      sprintf(lcdString, "Sensor %i Cleared", i); Serial.println(lcdString);  // Not to LCD
    }
    pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);  // Update occupancy sensor status array but don't paint
//    pSensorBlock->setSensorStatus(sensorNum, trippedOrCleared);  NOTE 8/2/24: THIS CALL SEEMS UNNECESSARY - WE DON'T EVER CALL GETSENSORSTATUS WHEN RUNNING IN MANUAL MODE; CONFIRM MANUAL STILL WORKS WITH THIS COMMENTED OUT ***************************
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
    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, just stop
    msgType = pMessage->available();  // Could be ' ', 'S', or 'M'
    if (msgType == 'S') {  // Got a sensor message in Manual mode; update the WHITE LEDs
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
      pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);
//      pSensorBlock->setSensorStatus(sensorNum, trippedOrCleared);  NOTE 8/2/24: THIS CALL SEEMS UNNECESSARY - WE DON'T EVER CALL GETSENSORSTATUS WHEN RUNNING IN MANUAL MODE; CONFIRM MANUAL STILL WORKS WITH THIS COMMENTED OUT ***************************
      pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
      if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
        sprintf(lcdString, "Sensor %i Tripped", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      } else {
        sprintf(lcdString, "Sensor %i Cleared", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      }
    } else if (msgType == 'M') {  // If we get a Mode message it can only be Manual Stopped.
      pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
      if ((modeCurrent != MODE_MANUAL) || (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "MAN MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to stop Manual mode.
      // When MANUAL mode is stopped, all there is to do is turn off all the Control Panel LEDs.
      pOccupancyLEDs->darkenAllOccupancySensorLEDs();
      pOccupancyLEDs->paintOneBlockOccupancyLED(0);   // Sending 0 will turn off all RED/BLUE LEDs
    } else if (msgType != ' ') {  // Remember, pMessage->available() returns ' ' if there is no message.
      sprintf(lcdString, "MAN MSG ERR %c", msgType); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
  } while (stateCurrent != STATE_STOPPED);  // We'll just assume mode is still Manual
  return;
}

// ********************************************************************************************************************************
// ************************************************ OPERATE IN REGISTRATION MODE **************************************************
// ********************************************************************************************************************************

void OCCRegistrationMode() {

  // Rev: 08-04-24.
  // Note that although OCC uses Block Res'n to preserve data for the next Registration loco default location, it does not otherwise
  // care about Block Reservations, and doesn't care at all about Turnout Reservations.

  sprintf(alphaString, "WAIT...");
  pAlphaNumDisplay->writeToBackpack(alphaString);

  // Initialize and clear the Control Panel Occupancy Sensor and Block Occupancy LEDs.
  // Each time we (re)start Registration, we'll call Occupancy_LEDs::begin() to reset the Static-Equipment Block, Sensor
  // Status, and internal Block status arrays.  No harm in calling begin() and passing the pointer more than once.
  // Do this BEFORE RECEIVE SENSOR STATUS MESSAGES, because it resets the local sensor status array that we'll be updating!
  pOccupancyLEDs->begin(pTrainProgress);  // This only initializes the OccupancyLEDs() local arrays.
  // Now paint all Control Panel Occupancy and Block LEDs dark.
  pOccupancyLEDs->darkenAllOccupancySensorLEDs(); // Turn off all WHITE LEDs
  pOccupancyLEDs->paintOneBlockOccupancyLED(0);   // Sending 0 will turn off all RED/BLUE LEDs

  // For each block, read Block Reservation record and store locoNum field in defaultLocoInThisBlock[0..TOTAL_BLOCKS - 1]
  //   This will preserve left-over last-known locos in each block, to hopefully use as the default loco for each occupied block.
  // NOTE: The following Block Reservation read (and then release) MUST take place before we receive sensor status messages.
  // Let's take advantage of the fact that we *might* have a still-accurate Block Res'n table in FRAM.  This will occur if
  // the previous operating session ended in Auto or Park mode with all locos stopped.  This can provide us with our best
  // guess of what loco might be sitting in each occupied block, when we prompt for that during Registration.  Worst case is
  // our default loco will be incorrect and the user will just need to scroll the rotary encoder to choose the right loco.
  sprintf(lcdString, "Loading defaults..."); pLCD2004->println(lcdString); Serial.println(lcdString);
  byte defaultLocoInThisBlock[TOTAL_BLOCKS];  // Just a temporary array until we've completed Registration
  for (byte blockNum = 1; blockNum <= TOTAL_BLOCKS; blockNum++) {
    defaultLocoInThisBlock[blockNum - 1] = pBlockReservation->reservedForTrain(blockNum);
    sprintf(lcdString, "Block %i Loco %i", blockNum, defaultLocoInThisBlock[blockNum - 1]); pLCD2004->println(lcdString); Serial.println(lcdString);
    // Could only legitimately be LOCO_ID_NULL, LOCO_ID_STATIC, or a valid locoNum 1..TOTAL_TRAINS; let's be certain.
    if ((defaultLocoInThisBlock[blockNum - 1] != LOCO_ID_NULL) &&
      (defaultLocoInThisBlock[blockNum - 1] != LOCO_ID_STATIC) &&
      ((defaultLocoInThisBlock[blockNum - 1] < 1) || (defaultLocoInThisBlock[blockNum - 1] > TOTAL_TRAINS))) {
      sprintf(lcdString, "BLK RES LOCO ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
  }

  // Release all block reservations.
  pBlockReservation->releaseAllBlocks();

  // Initialize the Train Progress table for every possible train (1..TOTAL_TRAINS)
  for (locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {  // i.e. 1..50 trains
    pTrainProgress->resetTrainProgress(locoNum);  // Initialize the header and no route for a single train.
  }

  // When starting REGISTRATION mode, SNS will always immediately send status of every sensor.
  // Receive the status T/C of each sensor. For each sensor retrieved:
  //   Store the sensor status in the Occupany LED class, as LED_OFF or LED_SENSOR_OCCUPIED
  //   Also store sensor status T/C in Sensor-Block table for later use.
  //   If sensor is Tripped, reserve sensor's block for LOCO_ID_STATIC in Block Res'n table.
  // Standby and recieve a Sensor Status from every sensor.  No need to clear first as we'll get them all.
  for (int i = 1; i <= TOTAL_SENSORS; i++) {
    // Wait for a Sensor message (there better not be any other type that comes in)...
    while (pMessage->available() != 'S') {}  // Do nothing until we have a new message
    pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);
    if (i != sensorNum) {
      sprintf(lcdString, "SNS %i %i NUM ERR", i, sensorNum); pLCD2004->println(lcdString);  Serial.println(lcdString); endWithFlashingLED(1);
    }
    if ((trippedOrCleared != SENSOR_STATUS_TRIPPED) && (trippedOrCleared != SENSOR_STATUS_CLEARED)) {
      sprintf(lcdString, "SNS %i %c T|C ERR", i, trippedOrCleared); pLCD2004->println(lcdString);  Serial.println(lcdString); endWithFlashingLED(1);
    }
    if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {
      sprintf(lcdString, "Sensor %i Tripped", i); pLCD2004->println(lcdString); Serial.println(lcdString);
    } else {
      sprintf(lcdString, "Sensor %i Cleared", i); Serial.println(lcdString);  // Not to LCD
    }

    // Update sensor status in the Occupancy LED class but don't paint
    pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);

    // Store the sensor status T/C in the Sensor Block table.
    pSensorBlock->setSensorStatus(sensorNum, trippedOrCleared);

    // If the sensor is TRIPPED, then reserve the corresponding block as STATIC in the Block Reservation table. Later, as we prompt
    // the user for the loco ID of each occupied block, we'll update the block reservation for each non-STATIC train.
    if (trippedOrCleared == 'T') {
      if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_EAST) {  // 'E'
        pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BE, LOCO_ID_STATIC);
        sprintf(lcdString, "  Res Block %i E", pSensorBlock->whichBlock(sensorNum)); pLCD2004->println(lcdString); Serial.println(lcdString);
      } else if (pSensorBlock->whichEnd(sensorNum) == LOCO_DIRECTION_WEST) {  // 'W'
        pBlockReservation->reserveBlock(pSensorBlock->whichBlock(sensorNum), BW, LOCO_ID_STATIC);
        sprintf(lcdString, "  Res Block %i W", pSensorBlock->whichBlock(sensorNum)); pLCD2004->println(lcdString); Serial.println(lcdString);
      } else {  // Not E or W, we have a problem!
        sprintf(lcdString, "BLK END %i %c", sensorNum, pSensorBlock->whichEnd(sensorNum)); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
    }
  }

  // *** PROMPT OPERATOR FOR STARTUP FAST/SLOW, SMOKE ON/OFF, AUDIO ON/OFF, DEBUG ON/OFF ***
  // Prompt for Fast or Slow loco power up (with or without startup dialogue.)
  rotaryPrompt[0].expired = false;
  rotaryPrompt[0].referenceNum = 0;
  strcpy(rotaryPrompt[0].alphaPrompt, "FAST PWR");
  rotaryPrompt[1].expired = false;
  rotaryPrompt[1].referenceNum = 0;
  strcpy(rotaryPrompt[1].alphaPrompt, "SLOW PWR");
  byte promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 1, 0);
  if (promptSelected == 0) {
    fastOrSlow = 'F';
  } else {
    fastOrSlow = 'S';
  }
  pMessage->sendOCCtoLEGFastOrSlow(fastOrSlow);

  // Prompt for Smoke on or off.  Note: we may want LEG to always turn off smoke after 10 minutes automatically.
  strcpy(rotaryPrompt[0].alphaPrompt, "SMOKE N");
  strcpy(rotaryPrompt[1].alphaPrompt, "SMOKE Y");
  promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 1, 0);
  if (promptSelected == 0) {
    smokeOn = 'N';
  } else {
    smokeOn = 'S';
  }
  pMessage->sendOCCtoLEGSmokeOn(smokeOn);

  // Prompt for Audio on or off (can apply to OCC station announcements and/or LEG loco dialogue
  strcpy(rotaryPrompt[0].alphaPrompt, "AUDIO N");
  strcpy(rotaryPrompt[1].alphaPrompt, "AUDIO Y");
  promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 1, 0);
  if (promptSelected == 0) {
    audioOn = 'N';
  } else {
    audioOn = 'A';
  }
  pMessage->sendOCCtoLEGAudioOn(audioOn);

  // Prompt for Debug Mode on or off.
  strcpy(rotaryPrompt[0].alphaPrompt, "DEBUG N");
  strcpy(rotaryPrompt[1].alphaPrompt, "DEBUG Y");
  promptSelected = pRotaryEncoderPrompter->getSelection(rotaryPrompt, 1, 0);
  if (promptSelected == 0) {
    pMessage->sendOCCtoLEGDebugOn('N');  // No debug
    debugOn = false;
  } else {
    pMessage->sendOCCtoLEGDebugOn('D');  // Debug
    debugOn = true;
  }
  // Finished prompting operator for FAST/SLOW, SMOKE/NO SMOKE, AUDIO ON/OFF, DEBUG ON/OFF.
  pRotaryEncoderPrompter->clearDisplay();

  // *** PROMPT OPERATOR FOR LOCO ID OF EVERY OCCUPIED BLOCK, ONE AT A TIME ***
  // Paint all the WHITE LEDs, to show where the system sees all occupancy, to help operator with next step.
  pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
  // Populate local array for the set of Loco Description prompts to use with the 8-char A/N display and Rotary Encoder.
  // The first choice for the operator will be "STATIC"
  topRotaryElement = 0;
  rotaryPrompt[topRotaryElement].expired = false;
  rotaryPrompt[topRotaryElement].referenceNum = LOCO_ID_STATIC;  // ID is 99, but will go in element 0.
  strcpy(rotaryPrompt[topRotaryElement].alphaPrompt, "STATIC");
  // Scan the entire Loco Ref table and ADD EVERY ACTIVE LOCO IN LOCOREF to the rotaryPrompt[] array.
  for (byte locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {
    if (pLoco->active(locoNum) == true) {  // If this is a Loco Ref record with an actual loco's data in it
      topRotaryElement++;
      rotaryPrompt[topRotaryElement].expired = false;
      if (pLoco->locoNum(locoNum) != locoNum) {
        sprintf(lcdString, "UNEXPECTED LOCONUM"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      rotaryPrompt[topRotaryElement].referenceNum = locoNum;
      pLoco->alphaDesc(locoNum, rotaryPrompt[topRotaryElement].alphaPrompt, 8);  // Returns text via pointer (array)
      sprintf(lcdString, "Found loco %i", locoNum); pLCD2004->println(lcdString); Serial.println(lcdString);
    }
  }
  sprintf(lcdString, "Done finding locos."); Serial.println(lcdString);
  // Now we have a Rotary Encoder array loaded with every active locoRef name, plus STATIC.
  // Our index into rotaryPrompt[] will be 0..(topRotaryElement - 1)
  // For each occuped sensor, ask operator what loco is there.
  // Occupied sensors/blocks could be a real loco or STATIC equipment.
  for (sensorNum = 1; sensorNum <= TOTAL_SENSORS; sensorNum++) {  // sensorNum variable declared above setup()
    sprintf(lcdString, "Sensor %i %c", sensorNum, pSensorBlock->getSensorStatus(sensorNum));  pLCD2004->println(lcdString); Serial.println(lcdString);
    if (pSensorBlock->getSensorStatus(sensorNum) == SENSOR_STATUS_TRIPPED) {  // We have an occupied block
      sprintf(lcdString, "Tripped sensor %i", sensorNum); pLCD2004->println(lcdString); Serial.println(lcdString);
      byte blockNum = pSensorBlock->whichBlock(sensorNum);
      char whichEnd = pSensorBlock->whichEnd(sensorNum);  // LOCO_DIRECTION_EAST == 'E', or LOCO_DIRECTION_WEST == 'W'
      sprintf(lcdString, "Block %i ", blockNum); pLCD2004->println(lcdString); Serial.print(lcdString);
      // So we know that block blockNum is occupied and at which end (thus, which direction the equipment is facing.)
      // Create a temp route element to store the block/dir (if it's a non-STATIC train) that MAS/OCC/LEG can use to populate the
      // Train Progress table's initial route for this loco (will disregard if this is ID'd as a STATIC train.)
      routeElement initialBlock;
      initialBlock.routeRecVal = blockNum;
      if (whichEnd == LOCO_DIRECTION_EAST) {
        initialBlock.routeRecType = BE;
        sprintf(lcdString, "East"); pLCD2004->println(lcdString); Serial.println(lcdString);
      } else {  // Better be LOCO_DIRECTION_WEST!
        initialBlock.routeRecType = BW;
        sprintf(lcdString, "West"); pLCD2004->println(lcdString); Serial.println(lcdString);
      }
      // Illuminate the RED block-occupancy LED we want to prompt user about.  Turns all others off automatically.
      pOccupancyLEDs->paintOneBlockOccupancyLED(blockNum);
      // Recall that our best-guess locoNum that may be occupying blockNum is defaultLocoInThisBlock[blockNum - 1] (because our
      //   little local array defaultLocoInThisBlock[] goes from 0..TOTAL_BLOCKS - 1)
      // Since we've already populated the rotaryPrompt[] array with EVERY active locoNum, we're guaranteed that the loco
      // that was previously using this block will be found in our rotaryPrompt[] array (even if it's STATIC.)
      // Remember, we're not passing the locoNum or blockNum as the default rotaryPrompt[] array element, but rather we're
      // going to have to pass the rotaryPrompt[] array index that contains the locoNum and description for our default loco.
      // So scan rotaryPrompt[].referencNum (locoNum) until we find defaultLocoInThisBlock[blockNum - 1]
      byte startElement = 0;  // We'll default to rotary prompt element 0 = STATIC, but try to find a previously-occupying loco.
      for (byte rotaryElementToCheck = 0; rotaryElementToCheck <= topRotaryElement; rotaryElementToCheck++) {
        sprintf(lcdString, "Check element %i", rotaryElementToCheck); pLCD2004->println(lcdString); Serial.println(lcdString);
        sprintf(lcdString, "Ref num %i", rotaryPrompt[rotaryElementToCheck].referenceNum); pLCD2004->println(lcdString); Serial.println(lcdString);
        sprintf(lcdString, "Def loco %i", defaultLocoInThisBlock[blockNum - 1]); pLCD2004->println(lcdString); Serial.println(lcdString);
        if (rotaryPrompt[rotaryElementToCheck].referenceNum == defaultLocoInThisBlock[blockNum - 1]) {
          startElement = rotaryElementToCheck;
          sprintf(lcdString, "Found it!"); pLCD2004->println(lcdString); Serial.println(lcdString);
          break;
        }
      }
      sprintf(lcdString, "startElement %i", startElement); pLCD2004->println(lcdString); Serial.println(lcdString);
      // Now if this loco was previously found in the Block Res'n table, we'll use it as our default choice for the user.
      // Prompt user for loco, starting with startElement (could be STATIC or the last-known loco in this block.)
      // getSelection() returns the offset into the rotary array that the user selected.
      byte rotaryArrayElement = pRotaryEncoderPrompter->getSelection(rotaryPrompt, topRotaryElement, startElement);
      // rotaryArrayElement will be the rotaryPrompt[] array index the user chose, 0..topRotaryElement - 1.
      byte locoSelected = rotaryPrompt[rotaryArrayElement].referenceNum;
      // Before we forget, flag their selection as expired as we won't ever say a loco is on two blocks at the same time!
      if (rotaryArrayElement != 0) {  // Don't expire STATAIC
        rotaryPrompt[rotaryArrayElement].expired = true;
      }
      Serial.print("Array offset = "); Serial.println(rotaryArrayElement);
      Serial.print("Ref Num = "); Serial.println(locoSelected);  // locoNum or LOCO_ID_STATIC
      Serial.print("Desc = "); Serial.println(rotaryPrompt[rotaryArrayElement].alphaPrompt);  // locoDesc or STATC
      // We have previously updated the Block Res'n table showing every occupied block is reserved for STATIC, and also sent that
      // to MAS and LEG.  But if the operator chose anything other than STATIC for this occupied block (i.e. a real loco), then we
      // and MAS and LEG will all need to update the Block Reservation table to reflect this loco being in that block.
      if (locoSelected == LOCO_ID_STATIC) {
        // Add this to the Occupancy_LEDs static block array (all were initialized above to not-STATIC.)  This is ONLY used
        // when painting Block Occupancy RED/BLUE LEDs, since we can't derive this from Train Progress.
        pOccupancyLEDs->setBlockStatic(blockNum);
        // No need to update Block Res'n or RS485, since they'll already have flagged all occupied blocks as STATIC, until
        // told otherwise.
      } else {  // Operator chose an actual loco that is occupying the block, NOT STATIC
        // Notify everyone that we have a real loco to register!  rotaryPrompt[locoSelected].referenceNum is locoNum.
        // MAS and LEG will use this to establish initial Train Progress Route, and LEG will startup loco etc.
        pMessage->sendOCCtoALLTrainLocation(locoSelected, initialBlock);
        // Now take the same action ourselves when a new train is registered: establish new Train Progress rec for this loco.
        pTrainProgress->setInitialRoute(locoSelected, initialBlock);
        // And update our local Block Reservation table to change the block status reservation from STATIC to this loco.
        pBlockReservation->reserveBlock(blockNum, initialBlock.routeRecType, locoSelected);  // blockNum, direction, locoNum
        // I don't *think* we need to do anything with pOccupancyLEDs...
      }
    }
  }
  pRotaryEncoderPrompter->clearDisplay();
  // All occupied sensors are accounted for.
  // A Train Progress table has been initialized for this loco, with it sitting in it's originating block.
  // Block reservations are set up for every occupied block (as either STATIC or a real loco.)

  // Now that we're done, turn off all the Control Panel LEDs.
  pOccupancyLEDs->darkenAllOccupancySensorLEDs();  // Sending 0 will turn off all WHITE LEDs
  pOccupancyLEDs->paintOneBlockOccupancyLED(0);  // Sending 0 will turn off all RED/BLUE LEDs

  // Send a "train location" message with locoNum 0 to signal we're done.
  routeElement tempTPElement;
  tempTPElement.routeRecVal = 0;    // Block 0
  tempTPElement.routeRecType = BE;  // n/a
  pMessage->sendOCCtoALLTrainLocation(LOCO_ID_NULL, tempTPElement);  // locoNum 0 means we're done with Registration

  // *** REGISTRATION COMPLETE! ***
  // We're done, so just wait for the mode-change message from MAS to exit back to main loop...
  do {
    haltIfHaltPinPulledLow();  // If someone has pulled the Halt pin low, just stop
    msgType = pMessage->available();  // Could only be ' ' or 'M' i.e. nothing or MODE message
    if (msgType == 'M') {  // If we get a Mode message it can only be Registration Stopped.
      pMessage->getMAStoALLModeState(&modeCurrent, &stateCurrent);
      if ((modeCurrent != MODE_REGISTER) || (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "REG MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to stop Registration mode.
    }
    else if (msgType != ' ') {  // Remember, pMessage->available() returns ' ' if there is no message.
      // WE HAVE A BIG PROBLEM with an unexpected message type at this point
      sprintf(lcdString, "REG MODE MSG ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }
  } while (stateCurrent != STATE_STOPPED);  // We'll just assume mode is still Register
  return;
}

// ********************************************************************************************************************************
// ************************************************** OPERATE IN AUTO/PARK MODE ***************************************************
// ********************************************************************************************************************************

void OCCAutoParkMode() {

  // Rev: 09-08-24.
  // NOTE 09-02-24: MAS/OCC/LEG need to pay attention to how/when isParked, isStopped, timeStopped, timeToStart, currentSpeed,
  //   currentSpeedTime, and expectedStopTime are updated -- as I'm not yet confident they're all being updated appropriately.
  // Upon entry here, modeCurrent == AUTO or PARK, and stateCurrent == RUNNING.
  // Mode may change from AUTO to PARK, and state may change from RUNNING to STOPPING, but OCC and LEG don't care and won't even
  // see those RS-485 messages.
  // So just run until we receive a Mode message (will still be AUTO or PARK) with a new state of STOPPED.

  // Note that when we're stopped, either in our initial Registration block or at the end of a route, before a train can start
  // moving, MAS must first obviously send a new Extension Route message - but that alone won't cause the train to move, and in
  // fact it will not be appropriate for a train to move until its Train Progress timeToStart field has expired.  So in addition
  // to sending a new Route message (which includes timeToStart,) MAS must regularly check the timeToStart field in its own Train
  // Progress header.  When it's time to get a train moving, MAS (not LEG) triggers the movement by sending a "fake" Sensor Trip
  // message to OCC and LEG for the sensor the loco is stopped on (which will always be Next-To-Trip.)
  // OCC and LEG will get the message that the train is "moving" and LEG will respond to the "trip" by sending FD/RD and VL##
  // commands to loco, based on subsequent Train Progress route elements.
  // So outside of the "check for messages" code, MAS, OCC, and LEG should all monitor Train Progress timeToStart:
  //   MAS uses this to know when to send a "fake" sensor trip message to get a train moving.
  //   OCC and LEG can use it to make pre-departure station announcements and tower comms seconds before scheduled departure.

  // OCC's only job here is to keep the WHITE Occupancy Sensor LEDs and RED/BLUE Block Occupancy LEDs correctly illuminated.
  // To do this, it must keep track of incoming Routes and Sensor Trips/Clears, and use that to keep the Train Progress table and
  // Block Reservation table up to date.
  // OCC does NOT use the Block Res'n table when determining/painting RED/BLUE Block Occupancy LEDs -- rather, it maintains an
  // internal array for Static blocks (always "Occupied") and then scans the entire Train Progress table to determine which blocks
  // are reserved and occupied.
  // However, OCC DOES keep the Block Res'n table up to date so it can have useful default locoNums occupying blocks during Reg'n.

  // THERE ARE ONLY THREE INCOMING MESSAGES OCC WILL BE SEEING:

  // 'R' Route: Extension (stopped) or Continuation (moving.)  Rev: 09-08-24.

  //   Get new Route (locoNum, extOrCont, routeRecNum, countdown.)
  //   Create a temp variable to note value of stopPtr.  We'll need it later.
  //   Call Train Progress addExtensionRoute() or addContinuationRoute() to add elements of the new Route to Train Progress (incl.
  //     update pointers, isParked, isStopped, timeToStart.)
  //   For every element from our old stopPtr (noted above) through our new stopPtr:
  //     MAS & OCC: Reserve each new Block in the Route in Block Reservation as Reserved for this loco.
  //       Reminder: OCC only tracks block res'ns for Reg'n to use as default loco for occupied blocks.
  //     MAS: Reserve each new Turnout in the Route in Turnout Reservation as Reserved for this loco.
  //   OCC: Re-paint Control Panel RED/BLUE BLOCK OCCUPANCY LEDs to show the new Blocks as RED/Reserved or BLUE/Occupied.
  //     No need to re-paint the white occupancy sensor LEDs when a new Route is received as they won't have changed.

  // 'S' Sensor Trip/Clear.

  //   Get the Sensor Trip/Clear message: sensorNum and Tripped/Cleared.

  //   *** SENSOR TRIP ***  Rev: 09-08-24.

  //     Get the locoNum that tripped the sensor.

  //     IF WE TRIPPED THE STOP SENSOR (END OF ROUTE):

  //       None of the 8 pointers need to be updated (CONT, STATION, CRAWL, STOP, TAIL, CLEAR, TRIP, or HEAD.
  //       Next element will always be VL00.
  //       LEG: Populate Delayed Action to stop the loco (in 3 seconds, from Crawl.)
  //       LEG: Delay 3 seconds, turn off bell, delay .5 seconds, blow horn LEGACY_PATTERN_STOPPED
  //       OCC/LEG: Delay as needed, make "has arrived" announcement.
  //       MAS/OCC/LEG: Update Train Progress isParked = true IFF block is Block Res'n "parking siding."
  //       MAS/OCC/LEG: Update Train Progress isStopped = true @ millis() + 3000 (due to 3-second stop time from tripping sensor.)
  //       LEG: Train Progress currentSpeed and currentSpeedTime will be written automatically by Engineer.

  //     ELSE (IF WE TRIPPED ANY SENSOR OTHER THAN STOP SENSOR (END-OF-ROUTE)):

  //       Since we know we didn't just trip the STOP sensor, we are guaranteed to have another sensor ahead in the route, and
  //         this will become the new Next-To-Trip sensor.
  //       IF WE TRIPPED THE CONTINUATION sensor: (we may or may not stop ahead)
  //         MAS: Decide if we want to assign a Continuation (not stopping) route for this train; do so if desired.
  //         OCC & LEG: Ignore.
  //       IF WE TRIPPED THE STATION STOP sensor: (means we are for sure stopping)
  //         OCC and LEG: Make announcement (via PA, via loco.)  Or wait until we trip the CRAWL sensor.
  //       IF WE TRIPPED THE CRAWL sensor:
  //         LEG: Turn on bell, make arrival announcement on loco.
  //         LEG: Blow horn LEGACY_PATTERN_APPROACHING
  //         OCC: Make arrival announcement at station.

  //       Now, FOR ALL SENSOR TRIPS EXCEPT STOP SENSOR, including the above CONT, STATION, CRAWL:
  //       Process each T.P. route element from first element following just-tripped sensor to next SN sensor record:
  //         FD00/RD00: LEG: Confirm we are stopped or will be stopped within 3 seconds (fatal error if not,) add delay to allow
  //                         stop time, then set loco direction via Delayed Action.
  //         VL00:    : LEG: Populate D.A. slowToStop (3-second stop.)
  //         VL01:    : LEG: If SLOWING to VL01, populate D.A. to change loco speed, but consider block length.
  //                         If STOPPED and FD00, blow horn LEGACY_PATTERN_DEPARTING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //                         If STOPPED and RD00, blow horn LEGACY_PATTERN_BACKING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //         VL02..04 : LEG: If STOPPED and FD00, blow horn LEGACY_PATTERN_DEPARTING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //                         ELSE If STOPPED and RD00, blow horn LEGACY_PATTERN_BACKING, add delay to allow horn time, then
  //                         populateLocoSpeedChange at Med momentum.
  //                         ELSE If NOT STOPPED, populateLocoSpeedChange at Med momentum.
  //         BE##/BW##: OCC: Change status of block from Reserved to Occupied (automatic in paintAllBlockOccupancyLEDs.)
  //                    LEG: May reference for slowToCrawl() when appropriate.
  //         TN##/TR##: MAS: Throw the turnout (per new rule as of 09-08-24.)
  //         SN##     : ALL: Set Next-To-Trip sensor to this sensor element number.
  //       Note regarding VL00 + FD00/RD00 + VL##: It's possible we'll encounter a VL00 to stop the loco, then RD or FD to reverse,
  //         then VL## to accelerate.  LEG won't be able to rely on the Train Progress currentSpeed when assigning the second set
  //         of speed commands!  However, we should always be at Crawl when encountering the VL00 command that preceeds the FD/RD
  //         command, so we'll assume it will always take 3 seconds for the loco to slow from Crawl to Stop, and maybe add a little
  //         more time and throw in a whistle/horn "reverse" or "forward" pattern.
  //       ALL: Advance Next-To-Trip pointer to next SN sensor record.
  //       ALL: Update Train Progress isParked = false.
  //       ALL: Update Train Progress isStopped = false.

  //   *** SENSOR CLEAR ***  Rev: 09-06-24.

  //     Get the locoNum that cleared the sensor.
  //       Note there will always be a subsequent sensor ahead of any sensor just cleared, even at the end of a route.

  //     Process each T.P. route element from "old" Tail through "old" (just cleared) Next-To-Clear sensor:
  //       FD00/RD00: ALL: Ignore.
  //       VL##     : ALL: Ignore.
  //       BE##/BW##: MAS, OCC: IFF block does not recur ahead in route, release block reservation.
  //       TN##/TR##: MAS: IFF turnout does not recur ahead in route, release turnout reservation.
  //       SN##     : ALL: Update T.P. Tail = old Next-To-Clear.
  //                       Update T.P. Next-To-Clear = sensorNum just encountered.

  //   *** EITHER TRIP OR CLEAR ***  Rev: 09-06-24.

  //     OCC: Update internal Occupancy LED array that keeps track of current sensor status, to reflect the change.
  //     OCC: Re-paint the Control Panel WHITE OCCUPANCY SENSOR LEDs
  //     OCC: Re-paint the Control Panel RED/BLUE BLOCK OCCUPANCY LEDs.

  // 'M' Mode/State change.  Rev: 09-06-24.
  //     Which for us will only be changing from Auto or Park Running or Stopping to Auto or Park Stopped.

  //   If new mode != MODE_AUTO and new mode != MODE_PARK, or new state != STATE_STOPPED, then fatal error.
  //   Else exit back to main loop.

  // ******************************************************************************************************************************

  // Perform any tasks that must be done ONCE, *before* starting our main Auto/Park loop...

  // OCC: Upon starting Auto/Park, all white and red/blue LEDs will be dark, so paint them now to reflect what we know from Reg'n:
  pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
  pOccupancyLEDs->paintAllBlockOccupancyLEDs();

  do {  // Operate in Auto/Park until mode == STOPPED

    // Perform any tasks that must be done REGULARLY, *outside* of specific Message commands (Route, Sensor, Mode):

    // ALL: If someone has pulled the Halt pin low, just stop
    haltIfHaltPinPulledLow();

    // See if there is an incoming message for us...could be ' ', Sensor, Route, or Mode message (others ignored)
    msgType = pMessage->available();  // msgType ' ' (blank) indicates no message

    // ***** ROUTE MESSAGE *****

    if (msgType == 'R') {  // Rev: 09-08-24.  We've got a new Route to add to Train Progress!

// Test with a new route that begins in Reverse as well as with a Continuation route that should change the penultimate VL (VL01)
// command to the block's default speed for that direction, and adds records and updates the pointers as expected. *****************************************************

      // First, get the incoming Route message which we know is waiting for us...
      pMessage->getMAStoALLRoute(&locoNum, &extOrCont, &routeRecNum, &countdown);

      // Make a note of stopPtr value, as adding the new route will destroy it; we'll use it as a starting point below.
      byte tempTPPointer = pTrainProgress->stopPtr(locoNum);  // Element number, not a sensor number

      // Debug code to dump Train Progress for locoNum, before adding the Extension or Continuation route
      sprintf(lcdString, "T.P. loco %i BEFORE", locoNum); Serial.println(lcdString);
      pTrainProgress->display(locoNum);

      //   Add elements of the new Route to Train Progress.  Also updates pointers, isParked, timeToStart (but not isParked)
      if (extOrCont == ROUTE_TYPE_EXTENSION) {
        // Add this extension route to Train Progress (our train is currently stopped.)
        pTrainProgress->addExtensionRoute(locoNum, routeRecNum, countdown);
      } else if (extOrCont == ROUTE_TYPE_CONTINUATION) {
        // Add this continuation route to Train Progress (our train is currently moving.)
        pTrainProgress->addContinuationRoute(locoNum, routeRecNum);
      }

      // Debug code to dump Train Progress for locoNum, after adding the Extension or Continuation route
      sprintf(lcdString, "T.P. loco %i AFTER", locoNum); Serial.println(lcdString);
      pTrainProgress->display(locoNum);

      // Point to first element beyond the OLD stopPtr (glad we saved that at the top of this block of code!)
      tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);

      // Move pointer from (old Stop pointer + 1) through (new Stop pointer - 1), and process each Route record as needed...
      do {
        routeElement tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);  // Working/scratch Train Progress element

        if ((tempTPElement.routeRecType == BE) || (tempTPElement.routeRecType == BW)) {  // BLOCK element
          // MAS, OCC, and LEG: If we have a BLOCK element, reserve it.
          pBlockReservation->reserveBlock(tempTPElement.routeRecVal, tempTPElement.routeRecType, locoNum);  // Block, Dir, locoNum
          // OCC: For purposes of OCC knowing which blocks are Reserved vs Occupied (for Control Panel LED display RED vs BLUE,)
          // that is re-calculated for the entire route each time we re-paint the Block LEDs (that is, each time a route changes
          // (when we start Auto/Park, when we add a new Route, and when a sensor changes.)  So OCC only cares about if Block Res'n
          // record for this block is reserved (including by STATIC) or not reserved, and figures out occupancy itself.

        } else if ((tempTPElement.routeRecType == TN) || (tempTPElement.routeRecType == TR)) {  // TURNOUT element
          // MAS: Reserve all, but DO NOT THROW new Turnouts in the Route.
          // We don't need to reserve Turnout reservations in OCC or LEG, but this is where we'll do it in MAS:
          // pTurnoutReservation->reserveTurnout(tempTPElement.routeRecVal, locoNum);  // Turnout number, locoNum

        } else if ((tempTPElement.routeRecType == FD) || (tempTPElement.routeRecType == RD)) {  // Loco DIRECTION element
          // Nothing to see here, keep moving along...

        } else if (tempTPElement.routeRecType == VL) {  // Loco SPEED change element
          // Nothing to do.

        } else if (tempTPElement.routeRecType == SN) {  // SENSOR element
          // Nothing to do.

        }

        // Advance pointer to the next element in this loco's Train Progress table...
        tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);

        // Stop scanning forward when we reach stopPtr, since there will only ever be VL00 beyond that.
      } while (tempTPPointer != pTrainProgress->stopPtr(locoNum));  // We've reached the Stop pointer.

      // OCC: Re-paint Control Panel RED/BLUE BLOCK OCCUPANCY LEDs to include the new Blocks as RED/Reserved (unless Occupied.)
      //   RED = RESERVED, BLUE = OCCUPIED.
      //   No need to re-paint the white occupancy sensor LEDs when a new Route is received as they won't have changed.
      pOccupancyLEDs->paintAllBlockOccupancyLEDs();  // Note that this re-paints all Block LEDs, not just for this loco.

    }  // End of "we received a new Route" message

    // ***** SENSOR MESSAGE *****

    else if (msgType == 'S') {  // Rev: 09-08-24.  We got a Sensor-change message in Auto/Park mode

      // Get the Sensor Trip/Clear sensorNum and Tripped/Cleared.
      pMessage->getSNStoALLSensorStatus(&sensorNum, &trippedOrCleared);

      if (trippedOrCleared == SENSOR_STATUS_TRIPPED) {

        // Which loco tripped the sensor?
        locoNum = pTrainProgress->locoThatTrippedSensor(sensorNum);  // Will also update lastTrippedPtr to this sensor
        // Quick error check: nextToTripPtr(locoNum) should still point at the sensor we just tripped. And since we called
        // locoThatTrippedSensor(sensorNum), that function will have updated lastTrippedPtr(locoNum) to point at the same sensor.
        // So just for fun, let's make sure that the two pointers are equal, otherwise this is a bug!
        if (pTrainProgress->nextToTripPtr(locoNum) != pTrainProgress->lastTrippedPtr(locoNum)) {
          sprintf(lcdString, "NEXT/LAST SNS ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
        }

        // First some code for special cases where we've just tripped the CONT, STATION, CRAWL, or STOP sensor.

        // *** IF WE JUST TRIPPED THE ROUTE'S CONTINUATION SENSOR ***
        if (pTrainProgress->lastTrippedPtr(locoNum) == pTrainProgress->contPtr(locoNum)) {

          // MAS: Decide if we want to assign a Continuation (not stopping) route for this train; do so if desired.
          // OCC & LEG: Ignore.
          // *** MAS: DECIDE IF WE WANT TO ADD A CONTINUATION ROUTE, AND IF SO, SEND THAT ROUTE MESSAGE. *****************************************************

        }

        // *** IF WE JUST TRIPPED THE ROUTE'S STATION SENSOR ***
        if (pTrainProgress->lastTrippedPtr(locoNum) == pTrainProgress->stationPtr(locoNum)) {

          // OCC: Nothing to do.  If this is a passenger train and we're approaching a passenger platform or station that has the
          //      ability to make a P.A. announcement, but we'll wait and do the OCC station "now arriving" announcement WHEN WE
          //      TRIP CRAWL, so the OCC and LEG annoucements don't overlap.
          // LEG: If we're coming to a major station (i.e. that would have a control tower,) regardless if this is a freight or
          // passenger train, do the LEG loco "now arriving" announcement here, so we have a bit of time before we turn on the bell
          // and blow the whistle.
          // *** LEG: IF MAJOR STATION (freight or passenger,) CODE TO MAKE LOCO "NOW ARRIVING" ANNOUNCEMENT (Engineer to Tower) ***************************************************************************************

        }

        // *** IF WE JUST TRIPPED THE ROUTE'S CRAWL SENSOR ***
        if (pTrainProgress->lastTrippedPtr(locoNum) == pTrainProgress->crawlPtr(locoNum)) {

          // OCC: If this is a passenger train, and we're approaching a passenger station or platform that has the ability to make
          //      a P.A. announcement, then make the "now arriving" announcement now (after LEG has potentially done the loco-to-
          //      tower "now arriving" announcement, so they don't talk over each other.)
          // *** OCC: CODE TO MAKE STATION P.A. ARRIVING ANNOUNCEMENT (if passenger train and passenger station) *************************************************************************************
          // *** LEG: TURN ON LOCO'S BELL. *****************************************************************************************************************
          // *** LEG: BLOW HORN LEGACY_PATTERN_APPROACHING. ************************************************************************************************

        }

        // *** IF WE JUST TRIPPED THE ROUTE'S STOP SENSOR ***
        if (pTrainProgress->lastTrippedPtr(locoNum) == pTrainProgress->stopPtr(locoNum)) {
          // A little error checking, just confirm that the next element is VL00.  We'll assume it's followed by headPtr.
          byte tempTPPointer = pTrainProgress->lastTrippedPtr(locoNum);  // Element number, not a sensor number, just tripped
          tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);  // Should now be pointing at VL00
          routeElement tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);  // Should contain VL00
          if ((tempTPElement.routeRecType != VL) || (tempTPElement.routeRecVal != 0)) {  // Whoopsie!  Big bug.
            sprintf(lcdString, "NOT SN00 AT EOR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
          }
          // Now just to triple check, advance our pointer and be sure we're pointing at head.
          tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);  // Should now be == headPtr
          if (tempTPPointer != pTrainProgress->headPtr(locoNum)) {  // Whoopsie!  Big bug!
            sprintf(lcdString, "NOT HEAD AT EOR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
          }
          // Now we're certain we' ha've just tripped the route's STOP sensor.
          // MAS/OCC/LEG: Check if this is a parking siding and update Train Progress isParked appropriately.  We're guaranteed,
          // based on our Route Rules, that the element before the Stop sensor element will be a Block Number, so use this.
          tempTPPointer = pTrainProgress->stopPtr(locoNum);  // Re-establish our position
          tempTPPointer = pTrainProgress->decrementTrainProgressPtr(tempTPPointer);  // Now points at BE or BW
          tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);  // Get the block element
          if ((tempTPElement.routeRecType != BE) && (tempTPElement.routeRecType != BW)) {  // Whoopsie!  Big bug.
            sprintf(lcdString, "NOT BE/BW AT EOR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
          }
          pTrainProgress->setParked(locoNum, pBlockReservation->isParkingSiding(tempTPElement.routeRecVal));
          pTrainProgress->setStopped(locoNum, millis() + 3000);

          // OCC: If this is a passenger train, and we're stopping at a passenger station or platform that has the ability to make
          //      a P.A. "has arrived" announcement, then wait a few seconds then make the announcement (after LEG has potentially
          //      made the loco-to-tower "now arriving" announcement.)
          // LEG: Send the "slow to stop" sequence to Delayed Action
          // pDelayedAction->populateLocoSlowToStop(locoNum);  // Will stop us from current speed to stopped in 3 sec.
          // LEG: Wait 3 sec, then turn turn off bell
          // pDelayedAction->populateLocoCommand(millis() + 3000, locoNum, LEGACY_SOUND_BELL_OFF, 0, 0);
          // LEG: Wait .5 sec, then toot whistle pattern "stopped."
          // pDelayedAction->populateLocoWhistleHorn(millis() + 3500, locoNum, LEGACY_PATTERN_STOPPED);
          // LEG: If this is a passenger train with a Stationsounds Diner car, and we're stopping at a passenger station or
          //      platform, wait 3 seconds, then have the Stationsounds Diner make a "watch your step" announcement.
          // pDelayedAction->populateLocoCommand(millis() + 6500, locoNum, LEGACY_DIALOGUE, LEGACY_DIALOGUE_E2T_HAVE_ARRIVED, 0);

        }

        // Now run code that applies to *any* sensor trip *except* the Stop sensor (even including Cont, Station, and Crawl.)
        if (pTrainProgress->lastTrippedPtr(locoNum) != pTrainProgress->stopPtr(locoNum)) {
          // *** FOR ALL SENSOR TRIPS EXCEPT THE ROUTE'S STOP SENSOR ***
          // Since we know we didn't just trip the STOP sensor, we are guaranteed to have another sensor ahead in the route.
          // Process each T.P. route element from first element following the just-tripped sensor to next SN sensor record.
          byte tempTPPointer = pTrainProgress->lastTrippedPtr(locoNum);  // Element number (not a sensor number) just tripped.
          routeElement tempTPElement;
          // Advance Next-To-Trip pointer until we find the next sensor, which will become the new Next-To-Trip sensor.
          // As we move our pointer forward, handle each route element as necessary, depending on if MAS, OCC, or LEG.
          // Since tempTPPointer still points to the sensor record that we just tripped, the first thing we'll do in the following
          // do..loop is increment the pointer to the first element following the sensor we just tripped (or are sitting on.)
          do {
            tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);  // Move pointer to next element of T.P.
            tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);              // Retrieve that element for analysis

            // Tricky code when we encounter VL and FD/RD elements, as LEG has to do things different depending on if we are
            // slowing to a stop to reverse direction, or slowing to a crawl, or accelerating to a crawl, or slowing or
            // accelerating to some other speed.  Since speed and direction elements must occur together, we'll handle as a group.
            // Except at EOR (which we're not because we just checked), VL00 will *always* be followed by FD/RD which will *always*
            // be followed by VL01..VL04, but we may not have the leading VL00 or FD/RD.
            // So there are only three possibilities:
            //   VL00 + FD/RD + VL01..VL04 (immediately following SN element) means we want to stop (in 3 seconds), set direction,
            //                             and then accelerate from stopped using generic acceleration with default step/delay.
            //          FD/RD + VL01..VL04 (immediately following SN element) means we must be at the beginning of the route and want
            //                             to accelerate from stopped.  Use generic acceleration using default step/delay momentum.
            //                  VL01..VL04 (immediately following SN element) simply means adjust speed to this new speed.
            //                             We will always be already moving when we encounter a lone VL command, and thus:
            //                             1. A VL01 command will always be "decelerate to Crawl" and consider block length; or
            //                             2. A VL02..VL04 simply requires a change of speed using default step/delay momentum.
            // So let's take advantage of those facts to work through these scenarios...
            if ((tempTPElement.routeRecType == VL) || (tempTPElement.routeRecType == FD) || (tempTPElement.routeRecType == RD)) {
              // We have a chunk of 1, 2 or 3 Speed/Direction elements to deal with.
              unsigned long locoTime = millis();  // To keep track of time to execute Delayed Action commands
              bool weWillBeStopped = false;  // Assume we're moving; will set true if we stop.

              // If we need to stop first, then do that...
              if ((tempTPElement.routeRecType == VL) && (tempTPElement.routeRecVal == 0)) {
                // A VL00 can only occur here if it's the first of a 3-element sequence: VL00+FD/RD+VL## accelerate at default mom.
                // We will almost certainly be going at VL01 Crawl, or very close to it, else something is wrong.
                // I can't think of a good way to insert a test to confirm we're going slow (may not yet have reached Crawl.)
                weWillBeStopped = true;  // We know we're stopping now.
                // Regardless of our current speed, bring the train to a stop in 3 seconds...
                pDelayedAction->populateLocoSlowToStop(locoNum);  // That was easy!
                locoTime = locoTime + 3500;  // Our next operation will take place after the loco has stopped
                tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);  // Move pointer to next element of T.P.
                tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);              // Retrieve that element for analysis
              }

              // If it was necessary for us to stop in order to change direction, then we've stopped (or will in 3 seconds.)
              // Now see if we need to change direction...could be at the beginning or mid-route, but also not required such as
              // the case where we simply have a lone VL01..VL04 speed-change command.
              if (tempTPElement.routeRecType == FD) {
                // We can be certain that if we encounter an FD or RD, our train must be stopped first.
                weWillBeStopped = true;
                pDelayedAction->populateLocoCommand(locoTime, locoNum, LEGACY_ACTION_FORWARD, 0, 0);
                // We will absolutely start moving in this circumstance, so toot the whistle.
                pDelayedAction->populateLocoWhistleHorn(locoTime + 100, locoNum, LEGACY_PATTERN_DEPARTING);
                locoTime = locoTime + 3000;  // Give the whistle time to finish before we start moving
                tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);  // Move pointer to next element of T.P.
                tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);              // Retrieve that element for analysis
                weWillBeStopped = true;  // Of course we're stopped to change direction
              } else if (tempTPElement.routeRecType == RD) {
                // We can be certain that if we encounter an FD or RD, our train must be stopped first.
                weWillBeStopped = true;
                pDelayedAction->populateLocoCommand(locoTime, locoNum, LEGACY_ACTION_REVERSE, 0, 0);
                // We will absolutely start moving in this circumstance, so toot the whistle.
                pDelayedAction->populateLocoWhistleHorn(locoTime + 100, locoNum, LEGACY_PATTERN_BACKING);
                locoTime = locoTime + 3000;  // Give the whistle time to finish before we start moving
                tempTPPointer = pTrainProgress->incrementTrainProgressPtr(tempTPPointer);  // Move pointer to next element of T.P.
                tempTPElement = pTrainProgress->peek(locoNum, tempTPPointer);              // Retrieve that element for analysis
                weWillBeStopped = true;  // Of course we're stopped to change direction
              }

              // Now, whether we had to stop and change directions, or were already stopped and changed directions, or if we were
              // already moving -- NOW we'd better be looking at a VL01..VL04 speed command.
              // If we have a VL01 and we were not stopped, this can only mean we want to slow down to Crawl speed from some higher
              // speed.  Calculate slow-to-Crawl parms based on block length etc. and slow down.
              // For any other VL01..VL04, it just means change from the current speed to this new speed using generic step/delay.
              if (tempTPElement.routeRecType != VL) {  // This would be a bug
                sprintf(lcdString, "BAD VL COMBO!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
              } else {  // We've got a VL of some sort
                if ((tempTPElement.routeRecVal == 1) && (weWillBeStopped == false)) {
                  // We are *moving* and need to slow to Crawl, so do our magic using block length and loco decel parameters.
                  // Look up block length so we know how much distance we have to slow from our current speed to Crawl...

                  // Look up the loco deceleration parameters to determine our rate of deceleration and stopping distance.

                  // Based on our current (incoming) speed (which will NOT be zero!), calculate how much longer we should continue
                  // at this speed before we begin to slow down.

                  pDelayedAction->populateLocoSpeedChange((locoTime + delayTime), locoNum, speedStep, stepDelay, targetSpeed);

                } else {  // Any speed change other than "slow to Crawl"
                  // Just for fun, if we're starting from stopped, let's accelerate slower than other speed changes
                  if (weWillBeStopped == true) {

                    // LEG: Command to slowly accelerate from stopped, but not too slow -- depends on what our target speed is

                  } else {  // We were already moving, so just make a generic speed change using default step and delay

                    // LEG: Generic speed change command when moving, using default step and delay values
                    // To change speed (other than when slowing to Stop or Crawl,) either use a global default Legacy speed step
                    // and delay, or have one for each loco...  For now, we'll just pick a global and see how that works out.
                    // Globals are LEGACY_DEFAULT_SPEED_STEP = 3, and LEGACY_DEFAULT_STEP_DELAY = 300.
                    // populateLocoSpeedChange expects a Legacy speed value 1..199 (or 1..31 for TMCC) so we need to look that up
                    // based on our VL value of 01..04.
                    byte targetLegacySpeed = 0;
                    if (tempTPElement.routeRecVal == 1) {
                      targetLegacySpeed = pLoco->crawlSpeed(locoNum);
                    } else if (tempTPElement.routeRecVal == 2) {
                      targetLegacySpeed = pLoco->lowSpeed(locoNum);
                    } else if (tempTPElement.routeRecVal == 3) {
                      targetLegacySpeed = pLoco->medSpeed(locoNum);
                    } else if (tempTPElement.routeRecVal == 4) {
                      targetLegacySpeed = pLoco->highSpeed(locoNum);
                    } else {
                      sprintf(lcdString, "VL Speed ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
                    }
                    // And we're off and running!
                    pDelayedAction->populateLocoSpeedChange(locoTime, locoNum, LEGACY_DEFAULT_SPEED_STEP, LEGACY_DEFAULT_STEP_DELAY,
                                    targetLegacySpeed);

                  }
                }
              }
            // Okay, we've handled the initial VL and FD/RD element(s) that immediately followed our SN sensor, if any.

            } else if ((tempTPElement.routeRecType == BE) || (tempTPElement.routeRecType == BW)) {  // Block element
              // Block is now considered occupied
              // MAS and LEG: Can't think of anything they need to do.
              // OCC: Any BLOCKS that we traverse here should change status from RESERVED to OCCUPIED, but this is handled
              //      automatically via paintAllBlockOccupancyLEDs().

            } else if ((tempTPElement.routeRecType == TN) || (tempTPElement.routeRecType == TR)) {  // Turnout element
              // OCC: Can't think of anything OCC needs to do.
              // MAS: Throw turnout.  Turnouts are thrown by MAS via an RS-485 message to O_SWT.
              //        turnoutDir must be TURNOUT_DIR_NORMAL or TURNOUT_DIR_REVERSE
              //        byte turnoutNum = tempTPElement.routeRecVal;
              //        char turnoutDir;
              //        if (tempTPElement.routeRecType == TN) {
              //          turnoutDir = TURNOUT_DIR_NORMAL;
              //        } else {
              //          turnoutDir = TURNOUT_DIR_REVERSE;
              //        }
              //        pMessage->sendMAStoALLTurnout(turnoutNum, turnoutDir);


            } else if (tempTPElement.routeRecType == SN) {  // Sensor means we're almost done!
              // ALL: Set Next-To-Trip sensor to this sensor element number.
              pTrainProgress->setNextToTripPtr(locoNum, tempTPPointer);
              // The only way we can be parked is at the end of a route *and* in a parking siding.  We know we're not at the end
              // of our route because we already confirmed we didn't just trip the Stop sensor.  So we know we're moving now.
              pTrainProgress->setParked(locoNum, false);
              pTrainProgress->setStopped(locoNum, false);

            } else {  // UNEXPECTED ELEMENT TYPE!
              sprintf(lcdString, "UNEXPECTED ELEMENT!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
            }

          } while (tempTPElement.routeRecType != SN);  // Until we reach the next sensor along the route
        }

        // Now the next-to-trip pointer is up to date; whether it was already at the end of the route, or if we advanced it.

// 9/16/24: Left off here.  Make sure I've covered all of the record types above.
// Compare all of the above to what I've already written for LEG Sensor Trip (and LEG Route)
// Update Train Progress addRoute() function to account for Continuation route that starts in Reverse.
// Add code above for when we receive FD/RD and VL commands.  Could be VL00, RD00, VL03 and the timing needs to work.

// 9/16/24: Take all of the above and cut and paste it into LEG.
// Then I can get started writing MAS Auto/Park ***********************************************************************************************************

// 9/16/24: Also, delete the redundant comments at the top of this section which say everything that's already said in the commented
// code above (same for LEG.)  Don't want duplicate comments above and in the code. ********************************************************************************

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

      } else {  // SENSOR_STATUS_CLEARED
        //   *** SENSOR CLEAR ***  Rev: 09-06-24.

        //     Get the locoNum that cleared the sensor.
        //       Note there will always be a subsequent sensor ahead of any sensor just cleared, even at the end of a route.

        //     Process each T.P. route element from "old" Tail through "old" (just cleared) Next-To-Clear sensor:
        //       FD00/RD00: ALL: Ignore.
        //       VL##     : ALL: Ignore.
        //       BE##/BW##: MAS, OCC: IFF block does not recur ahead in route, release block reservation.
        //       TN##/TR##: MAS: IFF turnout does not recur ahead in route, release turnout reservation.
        //       SN##     : ALL: Update T.P. Tail = old Next-To-Clear.
        //                       Update T.P. Next-To-Clear = sensorNum just encountered.

        // * Release Block Reservations behind just-cleared sensor (unless block occurs again ahead in route.)
        //   OCC tracks block reservations so that it will have an idea of what locos to use for occupied blocks during Reg'n.
        // * Release Turnout Reservations behind just-cleared sensor (unless turnout occurs again ahead in route.)  MAS ONLY.
        // * Update the Tail to be equal to the old Next-To-Clear.
        // * Advance Next-To-Clear to be next Sensor record ahead in the route.
        //   Note that there will always be a sensor record ahead of any sensor that is cleared, even at the end of a route.

        // Which loco cleared the sensor?
        locoNum = pTrainProgress->locoThatClearedSensor(sensorNum);
        byte tempTPPointer = pTrainProgress->tailPtr(locoNum);  // Element number, not a sensor number
        routeElement tempTPElement;  // Working/scratch Train Progress element
        do {  // Starting at the tail and working forward towards the sensor that was just cleared...
          // Advance a temporary pointer to the next element in this loco's Train Progress table...
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
              // We don't need to release Turnout reservations in OCC or LEG, but we will in MAS
            }
          }
          // Other modules, MAS and LEG, may consider actions on other element types here...
        } while (tempTPElement.routeRecType != SN);  // Watching for the new nextToTrip sensor.
        // Now our tempTPPointer is pointing at the new tail / old next-to-clear.  Let's be sure!
        if (tempTPPointer != pTrainProgress->nextToClearPtr(locoNum)) {  // Fatal error
          sprintf(lcdString, "NTC POINTER ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
        }
        // Assign the new Tail pointer value as the old next-to-clear...
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

      //   *** EITHER TRIP OR CLEAR ***  Rev: 09-06-24.
      // OCC: Update our internal Occupancy LED array that keeps track of sensor status (OCC ONLY.)
      pOccupancyLEDs->updateSensorStatus(sensorNum, trippedOrCleared);  // Does not illuminate any LEDs, just tracks status.
      // Regardless of Tripped or Cleared, re-paint the Control Panel WHITE OCCUPANCY SENSOR LEDs
      pOccupancyLEDs->paintAllOccupancySensorLEDs(modeCurrent, stateCurrent);
      // Regardless of Tripped or Cleared, re-paint the Control Panel RED/BLUE BLOCK OCCUPANCY LEDs.
      pOccupancyLEDs->paintAllBlockOccupancyLEDs();

    }  // End of "we received a Senor tripped or cleared" message

    // ***** MODE MESSAGE *****

    else if (msgType == 'M') {  // Rev: 09-08-24.  We got a Mode message; can only be Auto/Park Stopped and we're done here.

      // Mode message at this point can only be changing from Auto or Park, Running or Stopping to Auto or Park, Stopped.
      // Message class will have filtered out Mode Auto/Park, State STOPPING for OCC and LEG because it's irrelevant.
      // For MAS, changing from Running to Stopping is important.
      // OCC and LEG: Just check to be sure...
      if (((modeCurrent != MODE_AUTO) && (modeCurrent != MODE_PARK)) || (stateCurrent != STATE_STOPPED)) {
        sprintf(lcdString, "AUTO MODE UPDT ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
      }
      // Okay they want to STOP Auto/Park mode.  It must mean all locos are stopped (or will be in a few seconds.)
      // We will just fall out of the loop below since stateCurrent is now STATE_STOPPED
    }

    else if (msgType != ' ') {  // AT this point, the only other valid response from pMessage->available() is BLANK (no message.)
      // Any message type other than S, R, M, or Blank means a serious bug.
      sprintf(lcdString, "AUTO MODE MSG ERR!"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(1);
    }

  } while (stateCurrent != STATE_STOPPED);

  // Perform any tasks that must be done ONCE, *after* our main Auto/Park loop...

  // OCC: Turn off all WHITE, RED, and BLUE control panel LEDs.
  pOccupancyLEDs->darkenAllOccupancySensorLEDs();  // Turn off all WHITE LEDs
  pOccupancyLEDs->paintOneBlockOccupancyLED(0);    // Sending 0 will turn off all RED/BLUE LEDs

  return;
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
//      const byte SC = 13;  // Sensor Clear (not implemented yet, as of 8/24)
