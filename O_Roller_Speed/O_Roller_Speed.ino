// Roller_Speed Populator, Speedometer, and Deceleration Distance calculator.  Rev: 07/25/24.
// 
// IMPORTANT: Calibration of roller-bearing circumference is required FOR EACH LOCO using on-layout 5-meter timing before reliable
// results can be determined for that loco on the roller bearings with this utility.
// BEFORE RECORDING SPEED OR DECELERATION RESULTS for any given locomotive, we must calculate actual time to run 5 meters (5000mm)
// at a certain intermediate (but random, doesn't matter) speed.  Then run on the roller bearings at that same speed and count the
// quarter revolutions, to determine the appropriate "bearing circumference" to use on that loco's speed calculations.
// It isn't clear why bearing circumference varies by locomotive, but it's almost always between 40.5mm to 41.7mm, so the variance
// is +/- 0.6mm of 41.1mm, which is less than 1.5%.  Repeated tests by the same loco at the same speed generally vary by more than
// that, so it's not a major factor.
// Formula: At a given speed: (actual mm/sec measured on the layout) * (seconds/rev measured on the roller bearings) = mm/rev,
// which is the roller bearing circumference.
// IT'S VERY IMPORTANT that the white tape on the roller bearing is CLEAN, else we get false triggers which make the loco look as
// if it's moving faster than it actually is.

// FRAM POPULATOR UTILITY.  This is a handy way to write newly-recorded loco parameters in the Loco Reference populate utility to
// the local FRAM.  Does the same thing as our O_FRAM_Populator.ino utility, for Loco Reference only.

// SPEEDOMETER UTILITY.  Once accurate roller bearing circumference has been established for a given loco, run the loco on the
// roller bearings at various Legacy speeds to determine the SMPH and mm/sec, and decide on candidate values for Crawl, Low,
// Medium, and High.  Then put the loco on the layout to confirm these speeds are appropriate.  Finally, plug the values displayed
// by this utility into the Loco_Reference->populate() data table and transfer to FRAM using the Populator utility.

// DECELERATION UTILITY.  Calculates the distance to decelerate from High/Medium/Low speed to Crawl speed.  The user is prompted
// for parameters including Legacy (or TMCC) start speed (typ. High, Med, or Low), Legacy end speed (typ. Crawl), Legacy steps to
// skip each iteration (typ. 2, 3, or 4), and delay between successive speed commands (typ. 330ms or longer.)
// The shortest siding, 104", allows a realistic stop for any loco at each loco's High speed, so there is no need for more than one
// rate of deceleration per each of the three standard speeds.  Since the distance required to stop will always be shorter than any
// siding, the plan is to calculate a delay to keep moving at the start speed (based on known mm/sec) before beginning to slow.
// For example, WP803A has a very fast High speed of Legacy 114 = 470mm/sec.  Using 3 speed steps each 350ms allows the loco to
// reach its Crawl speed (Legacy 7 = 23mm/sec) in 2550mm = about 100", which is shorter than my shortest siding.
// We can try a number of different "delay between speed command" values (i.e. 160ms, 360ms, etc.) and "Legacy steps to skip"
// values (such as -1, -2, -3, -4, etc.) to find the largest delay and skip values that still produced non-jerky deceleration.
// These might be unique to each loco, since i.e. the Shay behaves very differently than the Big Boy.
// Both time to travel a given distance at a constant speed, and time to slow from one speed to another, are trivial calculations
// for which our Train Progress class has functions.

// ON-TRACK DECELERATION.  If we want to see how a particular set of deceleration parameters looks when the loco is actually
// down on the layout, we can use an external trigger (such as a push button or occupancy sensor relay on the layout) to tell the
// utility when to begin decelerating from the beginning constant speed.  If we select the "stop immediate" option (rather than the
// slow-to-stop-in-3-seconds function that will normally be used,) we can also physically measure the distance to confirm that it
// matches the stopping distance calculated on roller bearings (immediate stop only adds about 1/4" to distance when Crawling.)

// For a high-speed passenger train (such as WP 803A), we will target:
//   Crawl is around  28 mm/sec =  3 SMPH
//   Low   is around  93 mm/sec = 10 SMPH
//   Med   is around 326 mm/sec = 35 SMPH
//   High  is around 465 mm/sec = 50 SMPH
// For a medium-speed freight train (such as ATSF 4-4-2), we will target:
//   Crawl is around  28 mm/sec =  3 SMPH
//   Low   is around  93 mm/sec = 10 SMPH
//   Med   is around 186 mm/sec = 20 SMPH
//   High  is around 326 mm/sec = 35 SMPH
// For a slow-speed switcher engine (such as an NW-2), we will target:
//   Crawl is around  23 mm/sec = 2.5 SMPH (Though perhaps we want to target 3 SMPH for consistency with other locos, and because we don't have to slow down so much before stopping with a small train, and due to boredom waiting if we hit Crawl much ahead of the stop sensor.)
//   Low   is around  93 mm/sec = 10 SMPH
//   Med   is around 186 mm/sec = 20 SMPH
//   High  is around 233 mm/sec = 25 SMPH
// For ultra-low-speed locos (such as Shay), we will target:
//   Crawl is around  23 mm/sec = 2.5 SMPH (Same comments as with slow-speed switcher - maybe Crawl should be 3 SMPH...)
//   Low   is around  65 mm/sec =  7 SMPH
//   Med   is around 112 mm/sec = 12 SMPH
//   High  is around 168 mm/sec = 18 SMPH

// DETAILS ON ESTABLISHING ROLLER BEARING CIRCUMFERENCE FOR A GIVEN LOCO:
// Each loco travels a slightly different distance for each rotation of the roller bearing.  Perhaps this is because of the
// geometry of large wheels versus small wheels...I don't know.  But even though the circumference of the roller bearing is a
// constant, each loco does have slightly different characteristics which causes it to travel slightly more or less mm/revolution.
// 1. To establish the circumference of the bearing for a given loco.  FIRST CLEAN THE ROLLER BEARING TAPE!
//    a) Put the loco on the roller bearing and use this program to determine the TIME in milliseconds for each rotation at a given
//       Legacy speed setting.  Watch the time value for many rotations and note how consistent it is.  Slow speeds result in
//       longer times per revolution, but the times tend to vary; they are not very consistent.  At high speeds, the time will be
//       more consistent, but it will be a short time and thus less accurate on that basis.  Find an intermediate speed where the
//       speed is relatively consistent over many revolutions.
//    b) Note the time per revolution at the Legacy speed that works best, and move the loco to the layout.
//    c) Run the locomotive through the 5-meter straight section and use the iPhone stopwatch to time the 5-meter run.  Repeat this
//       several times, and determine the average time per 5000mm.
//    d) Do the math to determine the circumference of the bearing based on the roller-bearing time per revolution noted in the
//       first step.  EXAMPLE:
//       For Shay loco at Legacy speed 80, the average time per revolution was 315.3ms = 0.3153 seconds/revolution.
//       The time to traverse 5000mm on the layout at speed 80 was average 38.285sec = 5000/38.285 = 130.59945 mm/sec.
//       How far does the loco travel at speed 80 (130.59945 mm/sec) in 0.3153 seconds?
//       130.59945mm/sec for 315.3ms (.3153sec) = 130.59945 * .3153 = 41.008mm.
//       Thus we will use 41.008mm as the circumference of the roller bearing WHEN TESTING THE SHAY LOCO.
//       Now plug this new value into bearingCircumference, below, and re-run the roller bearing test at the same Legacy speed,
//       and confirm that the LCD now displays a mm/sec value that is very close to what you calculated on the 5-meter test track.

// HOW TO CONVERT TO/FROM SMPH and mm/Sec using 1:48 scale:
//   1  real mile = 1,609,344 mm (exactly 5280 ft.)
//   1 scale mile =    33,528 mm (exactly 110 ft.)
//   1 SMPH       =    33,528 mm/hour
//                =   558.798 mm/min
//                = 9.3133333 mm/sec
//   i.e. 558.798mm/sec = 60 SMPH

// IMPORTANT: Regarding how many trains we can send Legacy commands to at about the same time (i.e. slow multiple trains at once.)
// Since we can only send serial commands to Legacy every 30ms (any faster and Legacy may ignore them,) we'll need to consider how
// many trains we might want to be accellerating/decelerating (and blowing the horn, etc.) simultaneously.
// i.e. If we allow for possibly decelerating 10 trains at the same time, we'll only be able to send a command to each train every
// 300ms, which is about 3 commands per second.  Although that will realistically never happen, let's plan on sending speed
// commands to any given loco not more often than every 320ms.  This will often be fine for high momentum (slowing down slowly)
// even using single speed steps, but if we want to slow down at lower momentum (slow down more quickly), we'll simply need to slow
// down multiple Legacy speed steps at a time.
// NOTE: Once we trip a STOP sensor, we'll call a function to slow the loco from Crawl to Stop in 3 seconds, in less than an inch.
// NOTE: We should not exceed 500ms delay between speed steps, because longer delays seem to give less accurate stopping distances.
// Legacy's built-in deceleration (momentum) rates are:
//   Note: Although we must delay 30ms between successive 3-byte Legacy commands, the Legacy hardware itself is apparently not
//   limited by this; hence it is able to issue ever-decreasing speed commands up to every 16ms as seen with Momentum 1.
//   Momentum 1: Issues 1 step about every 16ms.  Stops from speed 62 in one second; nearly instant stopping.
//   Momentum 2: Issues 1 step every  20ms.  Way too fast to appear realistic.  Also exceeds 1-command-per-30ms max serial rate.
//                                           A train moving at 30smph would take 1.6 sec to stop (3.3 sec from 60smph.)
//   Momentum 3: Issues 1 step every  40ms.  Still too fast to appear realistic.
//                                           A train moving at 30smph would take 3.3 sec to stop (6.6 sec from 60smph.)
//   Momentum 4: Issues 1 step every  80ms.  Pretty fast deceleration, but realistic for short/lightweight trains.
//                                           A train moving at 30smph would take 6.6 sec to stop (13 sec from 60smph.)
//                                           To achieve not faster than 320ms/step, we can try 4 steps each 320ms.
//   Momentum 5: Issues 1 step every 160ms.  This is a good rate of deceleration, LOOKS GOOD for most locos such as an F3.
//                                           A train moving at 30smph would take 13 sec to stop (26 sec from 60smph.)
//                                           To achieve not faster than 320ms/step, we can try 2 steps each 320ms.
//   Momentum 6: Issues 1 step every 320ms.  This is too-slow deceleration for most, but LOOKS GOOD for Shay since it goes so slow.
//                                           A train moving at 30smph would take 26 sec to stop (52 sec from 60smph.)
//                                           Since Shay top speed is about 15smph, it will take 13 sec to stop from high speed.
//                                           To achieve not faster than 320ms/step, we can use 1 step each 320ms no problem.
// Since all of the built-in Legacy momentums use Legacy speed step 1 (actally, -1,) we can simulate similar rates of deceleration
//   by doubling the delay and the speed steps, and so on.  So for Momentum 5, instead of 1 step every 160ms, we could try 2 steps
//   every 320ms or 3 steps every 480ms or 4 steps every 640ms.  This program will allow us to try these on the layout to determine
//   the largest reasonable Legacy steps we can take while still appearing to slow down smoothly.
// Since my shortest stopping siding is 104" = 2641mm, be sure that I choose speed steps and delay such that I can always slow from
// a loco's High speed to Crawl using parameters that stop in less than 104".

// CALCULATING BOUNCE TIME for the IR sensor that monitors the roller bearing black-and-white stripes and they pass by:
// Applies to "manual" rev counts done by the Speedometer utility as well as the interrupt counts done by the Deceleration utility.
// OSCILLOSCOPE ROLLER BEARING BOUNCE TEST with four sections(two black, two white) :
//   Slowest loco (Shay), slowest speed (1), bounce is usually .3ms but can glitch up to 3.5ms.  Never happens above min. speed.
//   Fastest loco (Big Boy), fastest speed (199), transitions every 13ms - the fastest I could ever see.
//   Thus a debounce of 4ms (and up to 10ms) would work fine.
// Running a loco at 60smph (highest reasonable speed) equals about 13.5 revs on the roller bearing = about 54 detections/second.
//   54 state changes per second (1000 ms) = a state change every 18ms.
//   Thus, DEBOUNCE_DELAY *must* be less than 18 milliseconds to work at the highest loco speeds.
//   At Legacy speed 1, bounce is usually about .3ms, but occasional spurious 3.5ms bounces sometimes happen.
//   Thus, DEBOUNCE_DELAY should be at least 4 milliseconds.
//   IMPORTANT: The IR sensor module has a pot adjustment, and if not perfectly adjusted then bounce can be much longer.
// CAUTION: If the loco stops when the sensor is between black/white stripes, it may think it's still moving.

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_NUL;  // Global just needs to be defined for use by Train_Functions.cpp and Message.cpp.
char lcdString[LCD_WIDTH + 1] = "RSP 07/25/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorage;

// *** CENTIPEDE SHIFT REGISTER CLASS ***  (Used for accessory relays on LEG)
// NOTE: This Roller Speed utility won't use a shift register, but the pointer is needed by some classes even though not used here.
// THERE IS AN EXTERN REFERENCE FOR THIS VIA TRAIN_FUNCTIONS.H: extern Centipede* pShiftRegister;
// Thus, we do NOT need to pass pShiftRegister to any classes as long as they #include <Train_Functions.h>
// <Centipede.h> is already #included in Train_Functions.h but need to create pointer and instantiate class in calling program.
Centipede* pShiftRegister;  // We only create ONE object for one or two Centipede boards.

// *** BLOCK RESERVATION AND LOOKUP TABLE CLASS (IN FRAM) ***
// This class is instantiated used because it must be passed to classes that we use.
#include <Block_Reservation.h>
Block_Reservation* pBlockReservation = nullptr;

// *** LOCOMOTIVE REFERENCE LOOKUP TABLE CLASS (IN FRAM) ***
#include <Loco_Reference.h>
Loco_Reference* pLoco;

// *** ROUTE REFERENCE TABLE CLASS (IN FRAM) ***
// This class is instantiated used because it must be passed to classes that we use.
#include <Route_Reference.h>
Route_Reference* pRoute = nullptr;

// *** TRAIN PROGRESS TABLE CLASS (ON HEAP) ***
// This class is instantiated used because it must be passed to classes that we use.
#include <Train_Progress.h>
Train_Progress* pTrainProgress = nullptr;

// *** DELAYED ACTION TABLE CLASS (ON HEAP) ***
#include <Delayed_Action.h>
Delayed_Action* pDelayedAction;

// *** ENGINEER CLASS ***
#include <Engineer.h>
Engineer* pEngineer;

// *** NUMERIC KEYPAD ***
#include <Keypad.h>  // Also includes Key.h.
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
// Note: It is NOT necessary to initialize these pins via pinMode(), as the class does it automatically.
byte keypadRowPins[KEYPAD_ROWS] = {60, 59, 58, 57}; // Connect to the row pinouts of the keypad
byte keypadColPins[KEYPAD_COLS] = {56, 55, 54};     // Connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), keypadRowPins, keypadColPins, KEYPAD_ROWS, KEYPAD_COLS );

// *** TIMER INTERRUPT USED FOR KEEPING TRACK OF ROLLER BEARING REVOLUTIONS (1/4 of a rev at a time) when decelerating.
#include <TimerOne.h>  // We will generate an interrupt every 1ms to check the status of the roller bearing IR sensor.

// Any variables that are used in an interrupt *and* which might be accessed from outside the interrupt *must* be declared as
// volatile, which tells the compiler to retrieve a copy from memory each time it is accessed, and not assume that a value
// stored in cache is the current value (because it's value could be changed inside the interrupt, which the compiler can't "see".)
// Also we must temporarily disable interrupts whenever these volatile variables are read or updated from outside the ISR -- so do
// so only when you know that it won't be important if a call to the ISR is skipped.
volatile unsigned int bearingQuarterRevs = 0;  // This will count the number of state changes seen by the IR sensor on the bearing.
// Any time we access a (volatile) variable that's used in an ISR from outside the ISR, we must temporarily disable
// (noInterrupts()) and then re-enable (interrupts()) interrupts.  Since we don't want to disable interrupts if it's possible the
// interrupt might be called, we'll create a working variable that will store the value of the volatile variable.  So we can do
// math on it, or use it however we want.
unsigned int tempBearingQuarterRevs = 0;  // We'll grab the interrupt's value and drop in here as a working variable as needed.

const byte DEBOUNCE_DELAY   = 4;  // Roller bearing IR state change debounce time in 1ms increments (1ms interrupt countdown.)

// Declare a set of variables that will need to be defined for each engine/train:
byte locoNum               =  0;   // Train, Engine, Acc'y number etc.

// 5/2/22: Measured bearing diameter w/ calipers as 12.84mm, which implies approx 40.338mm circumference ESTIMATE.
// But based on actual test results on the roller bearings, the average circumference is 41.1mm, so that will be our default.
// To see our more accurate circumference of 41.1mm, the diameter would have to be 13.08 mm, a difference of 0.24mm.
float bearingCircumference =  41.1;  // Will be around 41.1mm but varies slightly (+/- 0.6mm) with each loco

// Pins 0 and 1 are Serial 0, Pins 14-19 are Serial 1, 2, and 3
// Pins 2..12 are good for general purpose digital I/O
const byte PIN_RPM_SENSOR        = 2;  // Arduino MEGA pin number that the IR sensor is plugged into (not a serial pin.)
const byte PIN_FAST_SLOW_STOP    = 3;  // Toggle switch for Decel to determine if Crawl should include 3-second slow-to-stop
const byte PIN_EXTERNAL_TRIP_1   = 4;  // Connects to int. trip pushbutton.
const byte PIN_EXTERNAL_TRIP_2   = 5;  // Connects to ext. pushbutton or port occupancy sensor relay.
const byte PIN_EXTERNAL_TRIP_LED = 6;  // Illuminates built-in pushbutton LED
// Pins 54..60 (A0..A6) are used for the membrane 3x4 keypad

const float SMPHmmSec = 9.3133;  // mm/sec per 1 SMPH.  UNIVERSAL CONSTANT for O scale.

char testType = ' ';  // 'S' = Speedometer on roller bearings; 'D' = Deceleration time & distance; 'P' = Populate FRAM

unsigned long startTime = millis();
unsigned long endTime   = millis();

bool debugOn  = false;  // In deceleration mode, this can produce a lot of output on the Serial monitor.
bool extTrip  = false;  // In deceleration mode, do we begin slowing when pin is grounded (pushbutton or relay) or internally?

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  digitalWrite(PIN_OUT_LED, LOW);       // Built-in LED LOW=off
  pinMode(PIN_OUT_LED, OUTPUT);
  pinMode(PIN_RPM_SENSOR, INPUT);
  pinMode(PIN_FAST_SLOW_STOP, INPUT_PULLUP);
  pinMode(PIN_EXTERNAL_TRIP_1, INPUT_PULLUP);
  pinMode(PIN_EXTERNAL_TRIP_2, INPUT_PULLUP);
  pinMode(PIN_EXTERNAL_TRIP_LED, OUTPUT);
  digitalWrite(PIN_EXTERNAL_TRIP_LED, HIGH);  // Pushbutton internal LED off

  // *** QUADRAM EXTERNAL SRAM MODULE ***
  initializeQuadRAM();  // Add-on memory board provides 56,832 bytes for heap

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(115200);  // SERIAL0_SPEED.
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(9600);   // Serial 2 for Thermal Printer, either 9600 or 19200 baud.
  Serial3.begin(9600);   // Serial 3 will be connected to Legacy via MAX-232

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();             // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);     // Serial monitor
  Serial2.println(lcdString);    // Mini thermal printer

  // *** INITIALIZE FRAM CLASS AND OBJECT ***
  // We must pass a parm to the constructor (vs begin) because this object has a parent (Hackscribble_Ferro) that needs it.
  pStorage = new FRAM(MB85RS4MT, PIN_IO_FRAM_CS);  // Instantiate the object and assign the global pointer
  pStorage->begin();  // Will crash on its own if there is any problem with the FRAM
  //pStorage->setFRAMRevDate(6, 18, 60);  // Available function for testing only.
  //pStorage->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
  //pStorage->testFRAM(); sprintf(lcdString, "Test complete."); pLCD2004->println(lcdString);
  //pStorage->testSRAM(); sprintf(lcdString, "Test complete."); pLCD2004->println(lcdString);
  //pStorage->testQuadRAM(56647); sprintf(lcdString, "Test complete."); pLCD2004->println(lcdString);

  // *** INITIALIZE CENTIPEDE SHIFT REGISTER ***
  Wire.begin();                               // Start I2C for Centipede shift register
  pShiftRegister = new Centipede;
  pShiftRegister->begin();                    // Set all registers to default
  pShiftRegister->initializePinsForOutput();  // Set all Centipede shift register pins to OUTPUT for Turnout solenoids

  // *** INITIALIZE BLOCK RESERVATION CLASS AND OBJECT *** (Heap uses 26 bytes)
  pBlockReservation = new Block_Reservation;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pBlockReservation->begin(pStorage);

  // *** INITIALIZE LOCOMOTIVE REFERENCE TABLE CLASS AND OBJECT *** (Heap version saves 73 bytes SRAM)
  // Quirk about C++: NO PARENTHESES in the constructor call if there are no params; else thinks it's a function declaration!
  pLoco = new Loco_Reference;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pLoco->begin(pStorage);

  // *** INITIALIZE ROUTE REFERENCE CLASS AND OBJECT ***  (Heap uses 177 bytes)
  pRoute = new Route_Reference;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pRoute->begin(pStorage);  // Assuming we'll need a pointer to the Block Reservation class

  // *** INITIALIZE TRAIN PROGRESS CLASS AND OBJECT ***  (Heap uses 14,552 bytes)
  // WARNING: TRAIN PROGRESS MUST BE INSTANTIATED *AFTER* BLOCK RESERVATION AND ROUTE REFERENCE.
  pTrainProgress = new Train_Progress;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pTrainProgress->begin(pBlockReservation, pRoute);

  // *** INITIALIZE DELAYED-ACTION TABLE CLASS AND OBJECT ***
  // This uses about 10K of heap memory
  pDelayedAction = new Delayed_Action;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pDelayedAction->begin(pLoco, pTrainProgress); //pLCD2004);

  // *** INITIALIZE ENGINEER CLASS AND OBJECT ***
  pEngineer = new Engineer;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pEngineer->begin(pLoco, pTrainProgress, pDelayedAction);

  // *** SET THE TIMER INTERRUPT TO FIRE EVERY 1ms ***
  // We'll be using TimerOne 1ms interrupts to count the number of quarter revs of the roller bearing during our decel tests.
  Timer1.initialize(1000);  // 1000 microseconds = 1 millisecond
  // The timer interrupt will call ISR_Read_IR_Sensor() each interrupt.
  Timer1.attachInterrupt(ISR_Read_IR_Sensor);

  // *** GET TEST TYPE FROM USER ON NUMERIC KEYPAD ***
  sprintf(lcdString, "1) Speedometer"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "2) Deceleration"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "3) Populate Loco Ref"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "ENTER 1|2|3 + #: ");
  unsigned int i = getKeypadResponse(lcdString, 4, 18);
  sprintf(lcdString, " "); pLCD2004->println(lcdString); Serial.println(lcdString);
  if (i == 1) {
    testType = 'S';
    sprintf(lcdString, "*** SPEEDOMETER ***"); pLCD2004->println(lcdString); Serial.println(lcdString);
  } else if (i == 2) {
    testType = 'D';
    sprintf(lcdString, "** DECELEROMETER **"); pLCD2004->println(lcdString); Serial.println(lcdString);
  } else if (i == 3) {
    testType = 'P';
    sprintf(lcdString, "*** POPULATOR ***"); pLCD2004->println(lcdString); Serial.println(lcdString);
  } else {
    sprintf(lcdString, "Invalid entry."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
  }

}

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  switch (testType) {  // S = Speedometer, D = Decelerometer, P = Populate Loco_Ref FRAM
    // INCLUDE ALL CASE BLOCKS IN CURLY BRACES TO AVOID HANGING.

    // *****************************************************************************************
    // ***************************  P O P U L A T E   U T I L I T Y  ***************************
    // *****************************************************************************************
    case 'P':  // Loco ref populate and display data from the Loco Reference table (tested and working 6/6/22)
    {
      sprintf(lcdString, "Populating!"); pLCD2004->println(lcdString); Serial.println(lcdString);
      // pStorage->setFRAMRevDate(4, 12, 62);  // Tested and works
      pLoco->populate();  // Uses the heap when populating.
      for (byte loco = 1; loco <= TOTAL_TRAINS; loco++) {
        if (pLoco->active(loco)) {
          pLoco->display(loco);
        }
      }
      sprintf(lcdString, "Done!"); pLCD2004->println(lcdString); Serial.println(lcdString);
      while (true) {}
    }

    // *****************************************************************************************
    // ************************  S P E E D O M E T E R   U T I L I T Y  ************************
    // *****************************************************************************************
    case 'S':  // Speedometer on roller bearings.
    {
      // This function helps us determine Legacy speed settings 1..199 (and SMPH) for our four standard speeds C/L/M/H.
      // I.e. if we want to know the Legacy speed setting for 10 SMPH, play with the throttle until the display shows 10 SMPH.
      // Then record that Legacy speed setting and the displayed speed in mm/sec into the Loco_Reference.cpp table and in Excel.
      // In order for the Speedometer function (and the Deceleration function) to provide the most accurate results, we need to
      // know the circumference of the roller bearing when used with each specific loco.  This varies from around 40.5mm to 41.7mm.
      // So the user should calculate the roller bearing diameter for each loco based on running the loco on the layout for 5
      // meters at an intermediate speed, and repeat using this speedometer function until a good roller bearing diameter value is
      // established.
      // So each time we run this function, we'll prompt the user to enter the locoNum and then look up the pre-determined roller
      // bearing circumference that's been previously calculated for this loco.  If the user selects loco 0, we'll default to the
      // average value of 41.1 which will likely be within an error margin of +/- 1.5% of actual.
      sprintf(lcdString, "Enter locoNum: ");
      locoNum = getKeypadResponse(lcdString, 4, 16);
      if ((locoNum < 1) || (locoNum > TOTAL_TRAINS)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); endWithFlashingLED(4);
      }
      bearingCircumference = getBearingCircumference(locoNum);  // Will also display/print loco name
      startUpLoco(locoNum);  // Will start up, set smoke off, momentum off, forward, abs speed zero, toot the horn

      do {
        float revTime = timePerRev();  // milliseconds/bearingCircumference.  Can take several seconds at very low speeds.
        // First, display the current processor time so we can easily see when display changes...
        sprintf(lcdString, "Refreshed: %8ld", millis() / 1000); pLCD2004->println(lcdString);
        // Second, display how many milliseconds to make one revolution (about 41.1mm but different for every loco.)
        dtostrf(revTime, 7, 2, lcdString);
        lcdString[7] = ' '; lcdString[8] = 'm'; lcdString[9] = 's'; lcdString[10] = 0;
        pLCD2004->println(lcdString);  // ms per revolution of the roller bearing
        // Now convert that to mm/Sec => bearingCircumference / revTime (ms)
        // i.e. 40.7332 / (3129/1000) (slowest speed) =  13.02 mm/Sec
        // i.e. 40.7332 / (1000/1000) (1 Rev/Sec)     =  40.73 mm/Sec
        // i.e. 40.7332 / (  97/1000) (maximum speed) = 419.93 mm/Sec
        float mmPerSec = bearingCircumference * 1000.0 / revTime;  // Confirmed correct rounding
        dtostrf(mmPerSec, 7, 2, lcdString);
        lcdString[7] = ' '; lcdString[8] = 'm'; lcdString[9] = 'm'; lcdString[10] = '/'; lcdString[11] = 'S'; lcdString[12] = 'e'; lcdString[13] = 'c'; lcdString[14] = 0;
        pLCD2004->println(lcdString);  // mm/sec at current speed using bearingCircumference
        // Now convert mm/sec to SMPH
        // SMPHmmSec = 9.3133mm/sec per 1 SMPH.  UNIVERSAL CONSTANT for O scale, regardless of bearing size or anything else.
        float sMPH = mmPerSec / SMPHmmSec;
        dtostrf(sMPH, 7, 2, lcdString);
        lcdString[7] = ' '; lcdString[8] = 'S'; lcdString[9] = 'M'; lcdString[10] = 'P'; lcdString[11] = 'H'; lcdString[12] = 0;
        pLCD2004->println(lcdString);  // SMPH
      } while (true);
      break;
    }

    // *****************************************************************************************
    // ***********************  D E C E L E R A T I O N   U T I L I T Y  ***********************
    // *****************************************************************************************
    case 'D':  // Deceleration test
    {
      // We'll be using TimerOne 1ms interrupts to count the number of quarter revs of the roller bearing during our decel tests.

      // A couple of variables that will control the inner and outer loops of this utility...
      bool decelRestart = false;  // Set true if we want to re-enter new speed parms for this loco.
      bool decelRepeat  = false;  // Set true if we want to repeat using the same speed parms for this loco (default.)

      // *** GET DEBUG MODE? ***
      // First ask if they want debug info on the Serial monitor (it's a lot.)  The potential problem is that it takes a lot of
      // time to serial print/display the debug info, which may throw off the timing of the test.  So generally don't use debug.
      sprintf(lcdString, "Debug? 1=Y [2]=N");
      byte d = getKeypadResponse(lcdString, 4, 18);
      debugOn = false;
      if (d == 1) debugOn = true;

      // *** GET EXTERNAL TRIP? ***
      // Are we going to trigger the start of deceleration independently, or by grounding a pin?  This would be for when we want to
      // slow the loco on the actual layout (vs the roller bearings) so we can:
      // 1. Watch how the slowing parameters look on the layout i.e. are we skipping too many speed steps; and
      // 2. Measure the actual distance travelled by a loco so we can confirm the estimates made on the roller bearing.
      // The pin that is grounded could be:
      // 1. A pushbutton pressed by the operator to precisely time when the loco passes a certain point on the layout, and they
      //    can then measure manually from there to the stopping point (and use the option that stops immediate, rather than the
      //    option that slows from Crawl to Stop in 3 seconds.)
      // 2. A time-delay relay connected to a layout sensor, so we could more easily determine the precise distance required to
      //    slow to Crawl (and use the option that stops immediate.)
      sprintf(lcdString, "Ext trip 1=Y [2]=N");
      d = getKeypadResponse(lcdString, 4, 20);
      extTrip = false;
      if (d == 1) extTrip = true;

      // *** GET LOCO NUMBER and DATA ***
      sprintf(lcdString, "Enter locoNum : ");
      locoNum = getKeypadResponse(lcdString, 4, 17);
      if ((locoNum < 1) || (locoNum > TOTAL_TRAINS)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString);endWithFlashingLED(4);
      }
      sprintf(lcdString, "Loco %2i", locoNum); Serial.println(lcdString); Serial2.println(lcdString);
      bearingCircumference = getBearingCircumference(locoNum);  // Will also display/print loco name

      startUpLoco(locoNum);  // Will start up, set smoke off, momentum off, forward, abs speed zero, toot the horn

      // In order to change any of the above (Debug mode, External Trip, and/or locoNum), user must power off and re-start.
      // Starting here, the user can repeat tests with the above info, but can (re)specify new speed parms if desired.
      do {  // Allow user to (re)input start speed, end speed, steps/speed, and step delay...

        // *** GET START SPEED ***
        sprintf(lcdString, "Start speed   : ");  // Generally this will be Low/Med/High
        byte startSpeed = getKeypadResponse(lcdString, 4, 17);
        sprintf(lcdString, "Start speed   :  %3i", startSpeed); Serial.println(lcdString); Serial2.println(lcdString);
        if ((startSpeed < 1) || (startSpeed > 199)) {
          sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
        }

        // *** GET END SPEED ***
        sprintf(lcdString, "End   speed   : ");  // Generally this will be Crawl
        byte targetSpeed = getKeypadResponse(lcdString, 4, 17);
        sprintf(lcdString, "End   speed   :  %3i", targetSpeed); Serial.println(lcdString); Serial2.println(lcdString);
        if ((targetSpeed < 0) || (targetSpeed > 199)) {
          sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
        }

        // *** GET STEPS/SPEED TO SKIP ***
        sprintf(lcdString, "Steps/speed   : ");  // Generally this will be 1, 2, or 3
        byte speedSteps = getKeypadResponse(lcdString, 4, 17);
        sprintf(lcdString, "Steps/speed   :  %3i", speedSteps); Serial.println(lcdString); Serial2.println(lcdString);
        if ((speedSteps < 0) || (speedSteps > 9)) {
          sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
         }

        // *** GET DELAY MS BETWEEN SPEED STEPS ***
        sprintf(lcdString, "Step delay ms : ");  // Generally this will be at least 330ms
        unsigned int stepDelay = getKeypadResponse(lcdString, 4, 17);
        sprintf(lcdString, "Step delay ms : %4i", stepDelay); Serial.println(lcdString); Serial2.println(lcdString);
        if ((stepDelay < 0) || (stepDelay > 5000)) {
          sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
        }

        // Starting here, the user can repeat tests with the same loco *and* same speed parms as many times as desired.
        do {  // Repeat deceleration using above parms as many times as user wants...

          // Repeat direction/stopped/toot each time through loop in case operator backed up and left in reverse for next try...
          // Set direction forward
          startTime = millis();
          pDelayedAction->populateLocoCommand(startTime, locoNum, LEGACY_ACTION_FORWARD, 0, 0);
          // Set loco speed stopped
          startTime = startTime + 50;
          pDelayedAction->populateLocoCommand(startTime, locoNum, LEGACY_ACTION_ABS_SPEED, 0, 0);
          // Give a short toot to say Hello
          startTime = startTime + 50;
          pDelayedAction->populateLocoWhistleHorn(startTime, locoNum, LEGACY_PATTERN_SHORT_TOOT);
          while (millis() < (startTime + 1000)) {
            pEngineer->executeConductorCommand();  // Get a Delayed Action command and send to the Legacy base as quickly as possible
          }

          // Calculate estimated time to slow from startSpeed to targetSpeed, just for fun to compare with actual time to slow.
          unsigned long estimatedTimeToSlow = pDelayedAction->speedChangeTime(startSpeed, speedSteps, stepDelay, targetSpeed);
          sprintf(lcdString, "Est time ms   :%5lu", estimatedTimeToSlow); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);

          // Confirm we are starting at speed zero...if not, just wait until we are at zero (operator's responsibility.)
          // Note that we are relying on the Train Progress speed field, which will not reflect any adjustments made on the CAB-2
          // remote.  So the user better be dang sure the loco is stopped before he gets to this point.
          while (pTrainProgress->currentSpeed(locoNum) > 0) { }  // Almost pointless test, but no harm done.

          // Populate Delayed Action with commands to speed up to our starting speed, 1 step at a time, every 50ms.
          startTime = millis() + 1000;
          pDelayedAction->populateLocoSpeedChange(startTime, locoNum, 1, 50, startSpeed);  // Assumes starting speed is zero

          // Let the Engineer execute ripe commands until we have accelerated to our start speed.
          do {
            pEngineer->executeConductorCommand();
          } while (pTrainProgress->currentSpeed(locoNum) < startSpeed);

          // Now we are up to speed, according to Train Progress loco's current speed.  Confirm to user.
          sprintf(lcdString, "Reached speed :  %3i", pTrainProgress->currentSpeed(locoNum)); pLCD2004->println(lcdString); Serial.println(lcdString);
          delay(2000);  // Just run at our starting speed for a couple seconds to let everything normalize

          // To begin slowing, are we acting on our own or are we waiting for a button press or sensor relay?
          if (extTrip) {  // Wait for our pin to be grounded, either by a pushbutton or by a layout sensor relay closure
            digitalWrite(PIN_EXTERNAL_TRIP_LED, LOW);  // Turn on push button built-in LED so user knows it's okay to press button
            while ((digitalRead(PIN_EXTERNAL_TRIP_1) != LOW) && (digitalRead(PIN_EXTERNAL_TRIP_2) != LOW)) { }  // Wait for pushbutton or relay contacts to ground this pin.
            digitalWrite(PIN_EXTERNAL_TRIP_LED, HIGH);  // Pushbutton internal LED off once it's been pressed
          }

          // Whether we were waiting for an external signal or can start on our own -- start slowing down now!
          startTime = millis();  // Start slowing immediately now
          // The following "slow down" sequence assume a starting speed as retrieved from Train Progress, per above confirmation.
          // This command takes 2ms per my tests, fyi, which will have no effect on time-distance slow-down calculation.
          pDelayedAction->populateLocoSpeedChange(startTime, locoNum, speedSteps, stepDelay, targetSpeed);

          // Let's count some revs!
          noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
          bearingQuarterRevs = 0;  // This will count the number of state changes seen by the IR sensor on the bearing
          interrupts();
          do {
            pEngineer->executeConductorCommand();
          } while (pTrainProgress->currentSpeed(locoNum) != targetSpeed);

          // We've reached the Crawl speed!  Snag the value of how many quarter revs we've counted...
          noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
          tempBearingQuarterRevs = bearingQuarterRevs;  // Grab our ISR "volatile" value
          interrupts();
          endTime = millis();
          // tempBearingQuarterRevs can have an error of +/- 1 (+/- 10mm) but we can't know which way, so no adjustment needed.
          // See the comments under ISR_Read_IR_Sensor() for full details.

          // *** FAST OR SLOW STOP FROM CRAWL? ***
          // Do we "Stop Immediate" or "Slow From Crawl to Stop in 3 seconds" when we reach Crawl speed?
          // Rather than prompt the user, we'll read the status of a toggle switch on the control box to determine if the user
          // wants to do an immediate stop, or a Crawl-to-Stop-in-3-Seconds.  The toggle switch allows the user to re-run a test
          // multiple times and try it both ways on the layout -- to measure accurate stopping distance (immediate stop) vs. to
          // visualize what a 3-second stop-from-Crawl will look like (delayed stop.)
          // It's a moot point for Auto mode (not ext. trip) as it won't affect any numbers.
          if (digitalRead(PIN_FAST_SLOW_STOP) == LOW) {
            pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_STOP_IMMED, 0, 0);
          } else {
            pDelayedAction->populateLocoSlowToStop(locoNum);
          }
          // Now stop the loco slowly or instantly...won't affect our distance calc either way...
          do {
            pEngineer->executeConductorCommand();
          } while (pTrainProgress->currentSpeed(locoNum) > 0);

          // We have stopped!
          // Display/print the time and distance travelled from incoming speed to Crawl, in mm.  This will be:
          unsigned int distanceTravelled = tempBearingQuarterRevs * (bearingCircumference / 4.0);
          // INTERESTING TIMING: We start the timer the moment we give the command to slow to entry speed *minus* Legacy step-down,
          // which is what we will do the moment we trip the destination entry sensor (actually, usually beyond that, after we
          // continue at the incoming speed until it's time to start slowing down.)  But we stop the timer one Legacy-delay period
          // *after* giving the Crawl speed command, because that is the first moment we would want to give a "stop" command.  I.e.
          // we don't want to time this so that we give the Crawl speed command as we trip the exit sensor, but rather we want to
          // give the Crawl speed command one Legacy-delay period (i.e. 500ms) *before* we trip the exit sensor, so that we can
          // give a "stop" command (or slow to a stop from Crawl) just at the moment we trip the exit sensor.
          // This is kind of silly because our error margins far exceed this level of accuracy, but at least we can do what we can
          // to try!
          // Also, although there is some variation in the *distance* that we travel from the incoming speed until Crawl speed, the
          // *time* that we're recording is going to be very consistent and very accurate -- because the time won't change
          // regardless of the circumference of the bearings or whatever.  In fact, we can always accurately *calculate* the time
          // required to slow to Crawl speed simply by multiplying the number of speed step-downs by the delay time.
          // The Train Progress class provides a function for this.
          sprintf(lcdString, "Quarter Revs  :%5i", tempBearingQuarterRevs); pLCD2004->println(lcdString); Serial.println(lcdString);
          //sprintf(lcdString, "Est  ms: %5lu", estimatedTimeToSlow); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
          sprintf(lcdString, "Act time ms   :%5lu", (endTime - startTime)); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
          sprintf(lcdString, "Distance mm   :%5i", distanceTravelled); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);

          sprintf(lcdString, "Restart=1 Repeat=2"); //pLCD2004->println(lcdString);
          decelRestart = false;  // Set true if we want to re-enter new speed parms for this loco.
          decelRepeat  = false;  // Set true if we want to repeat using the same speed parms for this loco (default.)
          d = getKeypadResponse(lcdString, 4, 20);
          if (d == 1) {  // Restart at the top loop
            decelRestart = true;
            sprintf(lcdString, "Restarting..."); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
          } else {
            decelRepeat = true;
            sprintf(lcdString, "Repeating..."); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
          }
        } while (decelRepeat == true);  // Innder loop - repeat the test using same speed parms for this loco
      } while (decelRestart == true);   // Outer loop - re-prompt for new speed parms for this loco
    }
  }
}

// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

float getBearingCircumference(byte t_locoNum) {
  // Rev: 07-25-24
  // Returns the precise value of the circumference of the roller bearing as it applies to use with a specific loco.
  // The value is pre-determined via empirical tests comparing time-distance of the loco at a fixed speed on the layout versus on
  // the roller bearings.
  // We will display/print the Loco Reference's description as well as the roller-bearing circumference that will be used.
  // This is also where we will initially record our desired values for Legacy C/L/M/H and deceleration parameters.
  // SIDING LENGTH: The SHORTEST siding is 104" = 8' 8" = 2641mm.  Thus, longest HIGH to CRAWL must be LESS THAN 2641mm.

  char alphaDesc[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};  // Init to 8 chars + null
  pLoco->alphaDesc(t_locoNum, alphaDesc, 8);       // Returns description of loco from Loco Ref in alphaDesc
  sprintf(lcdString, alphaDesc); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
  float locoBearingCircumference = 0;
  switch (t_locoNum) {

    case  2:  // Shay = Engine 2.  ALL CALCS COMPLETE.  SPEED & DECEL DATA ENTERED INTO LOCO_REF as of 7/24/24.
    {
      // SHAY (Eng 2): Final data as of 7/24/24.  Confirmed C/L/M/H speeds on layout seem good.
      // NOTE: The fastest Shay ever recorded went 18mph.  So I could bump Fast to 18smph.
      // Measured     SHAY on 5m track @ Legacy 80 AVG  38.29 sec = 130.60mm/sec.
      // Measured     SHAY on bearings @ Legacy 80 AVG            = 130.8 mm/sec using 41.2814 on 7/24/24.  Perfect.
      // SHAY REAL WORLD TEST RESULTS on 5 METER TRACK (5/2/22):
      //   Speed  20: 209.20 sec. =  23.90 mm/sec = almost exactly what the rollers register on 7/24/24
      //   Speed  40:  99.04 sec. =  50.48 mm/sec = almost exactly what the rollers register on 7/24/24
      //   Speed  60:  58.07 sec. =  86.10 mm/sec = almost exactly what the rollers register on 7/24/24
      //   Speed  80   38.35 sec. = 130.38 mm/sec = almost exactly what the rollers register on 7/24/24
      //   Speed 100:  27.05 sec. = 184.84 mm/sec = almost exactly what the rollers register on 7/24/24
      //   Legacy   smph    speed     mm/sec
      //      19    2.5     CRAWL       23  (Reading avg  23 on rollers 7/24/24)
      //      37      5      LOW        46  (Reading avg  46 on rollers 7/24/24)
      //      49      7                 65  (Reading avg  65 on rollers 7/24/24)
      //      63     10      MED        93  (Reading avg  93 on rollers 7/24/24)
      //      72     12                112  (Reading avg 112 on rollers 7/24/24)
      //      84     15      HIGH      140  (Reading avg 140 on rollers 7/24/24)
      // Step-down results:
      // LOW to CRAWL : 37 to 19.  Step 2 per 1000ms =  286mm (11.25".)  LOOKS GOOD - USE THIS.
      //                                2 per  750ms =  216mm ( 8.50".)  Stops in too little distance.
      // MED to CRAWL : 63 to 19.  Step 2 per  750ms =  864mm (34.00".)  LOOKS GOOD - USE THIS.
      //                                2 per 1000ms = 1143mm (45.00".)  Takes too long to stop.
      // HIGH to CRAWL: 84 to 19.  Step 2 per  750ms = 1797mm (70.75".)  LOOKS GOOD - USE THIS.  Max reasonable rate of slowing.
      //                                3 per  750ms = 1181mm (46.50".)  A bit jerky, slowing down too quickly.
      //                                1 per 1000ms = Way overshot the siding
      //                                2 per 1000ms = 2388mm (94.00")  Slowing down too slowly.
      locoBearingCircumference =  41.2814;  // SHAY mm/rev re-confirmed perfect on 7/24/24
      break;
    }

    case 4:  // ATSF NW2 = Engine 4
    {
      // Speed limits for switchers on yard tracks can range from 5mph to 20mph, with 10mph being typical.
      // ATSF NW2: Correct for layout and rollers as of 7/24/24:
      //   Legacy   smph    speed     mm/sec
      //      21    2.5                 24  (Reading avg  22   on rollers 7/24/24; confirmed  24 mm/sec on layout 7/22/24)
      //      25      3     CRAWL       28  (Reading avg  27.5 on rollers 7/24/24; confirmed  28 mm/sec on layout 7/22/24)
      //      39      5                 46  (Reading avg  46.8 on rollers 7/24/24; confirmed  46 mm/sec on layout 7/22/24)
      // around 49 is a good low, 65 is too fast
      //      65     10       LOW       93  (Reading avg  94   on rollers 7/24/24; confirmed  93 mm/sec on layout 7/22/24)
      //     102     20       MED      186  (Reading avg 190.5 on rollers 7/24/24; confirmed 186 mm/sec on layout 7/22/24)
      //     117     25      HIGH      234  (Reading avg 239   on rollers 7/24/24; confirmed 234 mm/sec on layout 7/22/24)
      locoBearingCircumference =  41.6517;  // Accurate for ATSF NW2 as of 7/24/24 (was 42.5)
      break;
    }

    case  5:  // ATSF 1484 4-4-2 E6 Atlantic = Engine 5, Train 7.  Fairly slow and small. (NOTE: WHY IS IT A TRAIN???)
    {
      // NOTE: The E6 steam loco had a top speed of 58mph to 75mph depending on train size.
      // ATSF 1484 E6: Final data as of 6/18/22.
      // Measured SF 4-4-2 on 5m track @ Legacy 70 AVG  23.03 sec = 217.11mm/sec.
      // Measured SF 4-4-2 on bearings @ Legacy 70 AVG  .1910 sec/rev @ 217.11mm/sec = 41.4680mm/rev (use this value)
      // Measured SF 4-4-2 on 5m track @ Legacy 45 AVG  42.80 sec = 116.82mm/sec.
      // Measured SF 4-4-2 on bearings @ Legacy 45 AVG  .3510 sec/rev @ 116.82mm/sec = 41.0024mm/rev (unreliable at this speed: 220 - 287 on 6/18/22 - ugh.)
      //   Legacy   smph    speed     mm/sec
      //       1    1.5                 14
      //       9    2.5     CRAWL       23  (Fairly variable from 21 to 23 on rollers)
      //      25    6.3                 61
      //      31    7.5       LOW       70  (Varies from 6.8 to 7.6 on rollers)
      //      51     14       MED      131  (Pretty consistent on rollers)
      //      80     28      FAST      261
      locoBearingCircumference = 41.4680;
      break;
    }

    case 8:  // WP 803-A = Engine 83, Train 8.  Lash-up includes other Engines with (hopefully) identical characteristics.
    {
      // NOTE: F3/F7 had a top speed of between 65mph and 102mph depending on gearing.
      // 803A has a very fast High speed of Legacy 114 = 463mm/sec.  Using 3 speed steps each 350ms allows the loco to reach its
      // Crawl speed in about 100", which is shorter than my shortest siding, so perfect for high speed, high momentum stop.
      // WP 803-A: Final data as of 5/23/22.
      // Measured WP 803-A on 5m track @ Legacy 50 AVG  36.63 sec = 136.50mm/sec.
      // Measured WP 803-A on bearings @ Legacy 50 AVG            = 136.50mm/sec using 41.7171.  Perfect.
      //   Legacy   smph    speed     mm/sec
      //       1    1.6       MIN       15  14.94/1.6, 15.01/1.6
      //       7    2.5     CRAWL       23
      //      37     10       LOW       95  But as low as 90.  90.1, 90.5, 90.5, 90.0 / 91.0
      //      93     35       MED      330  Consistent
      //     114     50      FAST      463  Consistent 
      //   WP 803-A REAL WORLD TEST RESULTS: WP 803-A on 5 METER TRACK (5/2/22):
      //   Speed  20: 102.70 sec.
      //   Speed  40:  48.96 sec.
      //   Speed  50:  36.76 sec.  Also 36.70, 36.52, 36.48, 36.63, 36.66, 37.02, 36.90 (reverse)
      //   Speed  60:  28.92 sec.
      //   Speed  80:  18.96 sec.
      //   Speed 100:  13.31 sec.
      //   Legacy   smph    speed     mm/sec
      //      11      6     CRAWL       53  But as low as 29mm/sec.  29.2, 28.9, 29.1, 29.4
      //      37     10       LOW       95  But as low as 90.  90.1, 90.5, 90.5, 90.0 / 91.0
      //      93     35       MED      330  Consistent
      //     114     50      FAST      463  Consistent 
      // These are just some inital roller-bearing tests done 5/26/22 to get a rough idea:
      // TEST RESULTS 5/26/22: WP 803-A, Legacy 114 (high) to 11 (crawl)
      //                      legacyStepDown 3, legacyDelay  500 = 137" (plus less than 2" to stop)
      //                      legacyStepDown 4, legacyDelay  500 = 102" still seems okay
      //                      legacyStepDown 5, legacyDelay  500 =  79.5" a bit rough stopping but okay
      //                      legacyStepdown 4, legacyDelay  400 =  81" and seems a bit smoother
      locoBearingCircumference =  41.7171;  // WP 803-A mm/rev as of 2/23/22
      break;
    }

    case 14:  // Big Boy = Engine 14
    {
      // BIG BOY: Final data as of 5/23/22.
      // NOTE: The fastest a Big Boy could ever go is 70mph.
      // Huge variation in mm/sec at low speeds; less as speeds increased past Legacy 40.
      // Measured  BIG BOY on 5m track @ Legacy 50 AVG  36.87 sec = 135.61mm/sec.
      // Measured  BIG BOY on bearings @ Legacy 50 AVG .30698 sec/rev @ 135.61mm/sec = 41.6296mm/rev
      //   Legacy   smph    speed     mm/sec
      //       1    1.5       MIN       14
      //       8    2.5     CRAWL       23
      //      26     10       LOW       95  // Recently 95mm/sec, previously 61-62 10/29/22, 65 = 7, 61=6.5, 
      //      74     26       MED      239
      //     100     41      FAST      384
      //   BIG BOY REAL WORLD TEST RESULTS on 5 METER TRACK (5/2/22):
      //   Speed  20: 105.64 sec.
      //   Speed  40:  48.85 sec.  Also: 49.08, 49.09, 49.03, 49.00 (fwd), 48.46, 48.65 (rev)
      //   Speed  60:  28.84 sec.
      //   Speed  80:  18.96 sec.
      //   Legacy   smph    speed     mm/sec
      //      12      6     CRAWL       53
      //      26     10       LOW       95
      //      74     26       MED      239
      //     100     41      FAST      384
      // These are just some inital roller-bearing tests done 5/26/22 to get a rough idea:
      // TEST RESULTS 5/26/22: BIG BOY, Legacy 100 (high) to 12 (crawl)
      //                      legacyStepDown 3, legacyDelay  500 =  48" way too fast!
      //                      legacyStepDown 3, legacyDelay 1000 =  97" (plus < 2" to stop)
      //                      legacyStepDown 3, legacyDelay 1500 = 145" (plus < 2" to stop)
      locoBearingCircumference =  41.6296;  // Big Boy mm/rev as of 2/23/22
      break;
    }

    case 40:  // SP 1440 = Engine 40
    {
      // SP 1440: Final data as of 5/23/22.
      // NOTE: This is a switcher, so speeds were kept low.  The Alco S-2 switcher had a top speed of 60mph.
      // Measured  SP 1440 on 5m track @ Legacy 50 AVG  37.80 sec = 132.28mm/sec.
      // Measured  SP 1440 on bearings @ Legacy 50 AVG .30897 sec/rev @ 132.28mm/sec = 40.8706mm/rev
      //   SP 1440 REAL WORLD TEST RESULTS: SP 1440 on 5 METER TRACK (5/2/22):
      //   Speed  20: 111.33 sec.
      //   Speed  30:  70.74 sec.
      //   Speed  40:  50.28 sec.
      //   Speed  60:  29.26 sec.
      //   Speed  80:  19.05 sec.
      //   Speed  84:  17.70 sec.
      //   Speed 100:  13.48 sec.
      //   Legacy   smph    speed     mm/sec
      //      15      8     CRAWL       73  But as low as 55mm/sec
      //      44     12       LOW      112  Not very accurate, sometimes slower
      //      55     16       MED      151
      //      75     25      FAST      236
      // These are just some inital roller-bearing tests done 5/26/22 to get a rough idea:
      // TEST RESULTS 5/26/22: SP 1440, Legacy  75 (high) to 15 (crawl)
      //                      legacyStepDown 3, legacyDelay  500 = 47" (plus < 2" to stop)
      //                      legacyStepDown 3, legacyDelay  750 = 70"
      locoBearingCircumference =  40.8706;  // SP 1440 mm/rev as of 2/23/22
      break;
    }

    default:
    {
      locoBearingCircumference =  41.1;  // This is a good average value to use in the absence of actual data
      sprintf(lcdString, "DEFAULT LOCO"); pLCD2004->println(lcdString); Serial.println(lcdString);
      break;
    }

    dtostrf(locoBearingCircumference, 7, 4, lcdString);  // i.e. "41.1234"
    lcdString[8] = 'm'; lcdString[9] = 'm'; lcdString[10] = 0;
    pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);

  }
  return locoBearingCircumference;
}

void ISR_Read_IR_Sensor() {
  // Rev: 07-07-24.
  // Used for deceleration utility distance loco travels when slowing; not used with speedometer utility.
  // 07-07-24: Made DEBOUNCE_DELAY a byte const at top of code, even when used with delay()
  // Increment global bearingQuarterRevs each time we see a state transition (white-to-black or black-to-white.)
  // Called every 1ms by TimerOne interrupt, uses a bounce delay of 4ms.
  // Gets called every 1ms to see if the roller bearing IR sensor has changed state.  There are four state changes per rev.
  // Updates the global volatile variable bearingQuarterRevs to count each quarter turn of the roller bearing.
  // When you want to start counting: Call nointerrupts(), then reset the volatile bearingQuarterRevs, then call interrupts().
  // When you want to stop counting: Call nointerrupts(), copy bearingQuarterRevs to a regular variable, then call interrupts().
  // We are allowing for a bounce of up to 4ms for the signal to stabilize after each state change, which is no problem because the
  // fastest loco (Big Boy) triggers a state change every 13ms, and it would *never* travel that fast on the layout.
  // At most speeds, bounce is much less than 1ms, but at speed 1 there is sometimes a weird bounce of 3.5ms (very rare.)
  // Bounce can take up to 3.5ms (per oscilloscope testing) at very low speeds.
  // Quarter rev never takes less than 16ms (at highest speed.)
  // Total quarter rev (state-change) count could have an error of +/- 10mm, since each 1/4 rev stripe is about 10mm long:
  //   1. We could start at the very beginning of a stripe and end at the very beginning of a stripe, which would be accurate.
  //   2. We could start at the very end of a stripe and end at the very end of a stripe, which would be accurate.
  //   3. We could start at the very beginning of a stripe and end at the very end of a stripe, which would make 20mm look like 10mm.
  //   4. We could start at the very end of a stripe and end at the very beginning of a stripe, which would make 0mm look like 10mm.

  // The following three STATIC variables get set ONCE and then retain their states between calls to this function.
  static byte bounceCountdown = 0;  // This will get reset to DEBOUNCE_DELAY (4) after each new sensor change
  static byte oldSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH
  static byte newSensorReading = oldSensorReading;

  // Wait until BOUNCE_DELAY (4) ms has transpired after reading a state change, before looking for the next one.
  if (bounceCountdown > 0) {  // Keep waiting for any bounce to resolve
    bounceCountdown --;  // We do this because we know this is being called by the timer interrupt once per ms
  } else {  // See if bearing has turned 1/4 turn (i.e. changed state) since our last check
    newSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH
    if (newSensorReading != oldSensorReading) {  // Sensor status has legitimately changed!
      oldSensorReading = newSensorReading;
      bearingQuarterRevs++;
      bounceCountdown = DEBOUNCE_DELAY;  // Reset bounce countdown time each time we fetch a new sensor reading
    }
  }
}

float timePerRev() {
  // Rev: 07-24-24.
  // 07-23-24: Re-wrote to precisely time based on number of state changes rather than a timer and how many revs, since we could be
  //           in the mid-point between transitions which introduces unnecessary error.
  //           We'll use the toggle switch built-in to the box to choose a low-speed versus high-speed version (how long we wait.)
  // 07-22-24: Changing to just count revs for 4 seconds regardless of RPM.  Should work fine for lowest practical speeds + high speed.
  // 07-07-24: Made DEBOUNCE_DELAY a byte const at top of code, even when used with delay()
  // 06-28-24: Added logic to vary the number of quarter revs we wait for (and count) depending on speed of the loco.  This
  //           tries to display the loco speed no faster than each .5 seconds, and no slower than each 1 second.
  //           Ranges from 2 full revs at slow speed, to 16 full revs at high speed.
  // This function just sits and watches the roller bearing run (at a steady speed, presumably) for a specified number of
  // revolutions, and then returns the average time per rev. (as a float value so we can more accurately divide etc.)
  // Watch the roller bearing IR sensor and return the time for one rev, in ms (averaged based on 4 revs.)

  // We will time how long it takes the loco to traverse a specific number of state-transitions (about 10mm = 1cm each.)
  // We'll somewhat arbitrarily decide how many transitions to count (and time) based on if the operator tells us the loco is
  // moving fairly slowly or farly fast.  With a fast loco, we can count a bunch of revs to get an accurate timer per rev, but for
  // a slow-moving loco we don't have the patience to wait that long (it can take a couple of minutes to go 5 meters at low speed.)
  // We'll set the number of transitions to count as a multiple of 4, just to keep the math as simple as possible.
  unsigned int transitionsToCount = 0;  // How many quarter turns of the roller bearing is enough to determine accurate speed?
  if (digitalRead(PIN_FAST_SLOW_STOP) == LOW) {
    // Switch is set to Fast, so do a faster refresh.  Assume loco is moving slow; count fewer revs to report more frequently.
    transitionsToCount = 12;  // 3 full revs of the roller bearing = approx. 3 * 40cm = 120cm (1.2 meters) = 4 feet
  } else {
    // Switch is set to Slow, so count more revs before refreshing display.  Assume loco is moving at higher speed.
    transitionsToCount = 48;  // 12 full revs of the roller bearing = approx. 12 * 40cm = 480cm (4.8 meters) = 16 feet
  }

  unsigned int currentTransitionCount = 0;  // How many quarter revs per LOCAL_DELAY_TIMER ms
  unsigned long localStartTime;
  unsigned long localEndTime;

  // Before we begin, wait for two stable reads (separated by DEBOUNCE_DELAY ms) and *then* wait for a state change
  byte oldSensorReading = LOW;
  byte newSensorReading = LOW;
  // First wait for a transition so we'll know we're right at the edge, maybe bouncing or maybe not.
  // We could start timing at the first change of state, but if we happen to be at the end of a bounce, we could potentially see
  // the timer up to 3.5ms less than it actually is.  Of course this would ONLY happen at the lowest speeds when we see the
  // occasional 3.5ms fluke bounce time (usually it's about .5ms.)  But what the heck, this is for accuracy and only in our test-
  // bench code. So here we'll "waste" up to 1/4 rev waiting to be sure we're at the FRONT edge of a transition.
  oldSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH
  do {
    newSensorReading = digitalRead(PIN_RPM_SENSOR);
  } while (newSensorReading == oldSensorReading);  // Keep reading until there is a change
  // We just saw a transition so we are for sure near the edge of a transition from black/white or white/black.  Maybe at the
  // beginning, but POSSIBLY at the end of a 3.5ms bounce.  So now wait for debounce and then wait for next state change.
  delay(DEBOUNCE_DELAY);
  // Now we know we must be partway into a quarter rev (however long it takes to travel for DEBOUNCE_DELAY ms.)
  // Always less than a quarter rev, which never takes less than 18ms even if we are travelling at 60smph.
  oldSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH, but will be a stable read at this point
  do {
    newSensorReading = digitalRead(PIN_RPM_SENSOR);
  } while (newSensorReading == oldSensorReading);  // Kill time until we see the sensor status change
  // Okay, NOW we know we are at the FRONT edge of a transition, beginning a newSensorReading zone!  Ready to roll!

  // Start the timer and count "transitionsToCount" transition state changes.
  localStartTime = millis();  // Start the timer!
  while (currentTransitionCount < transitionsToCount) {
    currentTransitionCount++;  // First time in will set this to 1.
    // Wait for bounce from last read to stabilize.  Do this *before* we wait for the next transition in case it's the last, we
    // don't want to add DEBOUNCE_DELAY to the total time.
    delay(DEBOUNCE_DELAY);
    // Wait for the sensor to change again
    while (digitalRead(PIN_RPM_SENSOR) == newSensorReading) {}
    // Okay we just saw a transition
    newSensorReading = !(newSensorReading);
  }
  localEndTime = millis();
  // Completed counting transitions - that was easy!  And the time should be very accurate.

  // Number of full revs will be 1/4 of currentTransitionCount.  We want to return ms per full rev.
  unsigned int fullRevs = currentTransitionCount / 4;
  // Total time will be end time - start time
  unsigned int totalMS = localEndTime - localStartTime;
  float MSPerFullRev = float(totalMS) / float(fullRevs);

  return MSPerFullRev;  // Time in ms per full revolution
}

void startUpLoco(byte t_locoNum) {
  // Rev: 07-25-24
  // NOTE: The following startup commands can all be given the same startTime because the Engineer will ensure they are separated
  // by the minimum required delay between successive Legacy commands (25ms.)  However we can add a slight delay if we want them to
  // be executed in a certain order...
  pDelayedAction->initDelayedActionTable(debugOn);
  // Start up the loco
  pEngineer->initLegacyCommandBuf(debugOn);
  startTime = millis();
  pDelayedAction->populateLocoCommand(startTime, t_locoNum, LEGACY_ACTION_STARTUP_FAST, 0, 0);
  // Set loco smoke off
  startTime = startTime + 50;
  pDelayedAction->populateLocoCommand(startTime, t_locoNum, LEGACY_ACTION_SET_SMOKE, 0, 0);  // 0/1/2/3 = Off/Low/Med/High
  // Set loco momentum to zero
  startTime = startTime + 50;
  pDelayedAction->populateLocoCommand(startTime, t_locoNum, LEGACY_ACTION_MOMENTUM_OFF, 0, 0);
  // Set direction forward
  startTime = startTime + 50;
  pDelayedAction->populateLocoCommand(startTime, t_locoNum, LEGACY_ACTION_FORWARD, 0, 0);
  // Set loco speed stopped
  startTime = startTime + 50;
  pDelayedAction->populateLocoCommand(startTime, t_locoNum, LEGACY_ACTION_ABS_SPEED, 0, 0);
  // Give a short toot to say Hello
  startTime = startTime + 50;
  pDelayedAction->populateLocoWhistleHorn(startTime, t_locoNum, LEGACY_PATTERN_SHORT_TOOT);
  while (millis() < (startTime + 1000)) {
    pEngineer->executeConductorCommand();  // Get a Delayed Action command and send to the Legacy base as quickly as possible
  }
  return;
}

unsigned int getKeypadResponse(char* lcdString, byte localRow, byte localCol) {
  // Rev: 07-25-24
  // Returns an unsigned int of the ASCII value entered, when user terminates input with * or #, from a 3x4 membrane keypad
  // Displays each numeric value as it is entered at the row and column passed as parm.
  // NOTE: If user simply presses # to default to a value, it gets returned as zero.  Thus we can't have a prompt which accepts
  // zero as a valid return value unless we disallow # as "choose default."
 pLCD2004->println(lcdString);
  String stringValue = "0";
  while (true) {
    char charValue = getKeypadPress();  // 0..9 or * or #
    if ((charValue == '#') || (charValue == '*')) {  // Pressed "Enter"
      return stringValue.toInt();  // Converts an ASCII string with numberic value to an integer
    } else {  // Otherwise they hit a digit
      sprintf(lcdString, "%1c", charValue); pLCD2004->printRowCol(localRow, localCol, lcdString);
      stringValue += charValue;
      localCol++;
    }
  }

}

unsigned int getKeypadValue() {
  // Rev: 06-27-24
  // Simpler version of getKeypadResponse, doesn't include the prompt or display the keypress.
  // Returns an unsigned int of the ASCII value entered, terminated by * or #, from a 3x4 membrane keypad.
  // Not used in O_Roller_Speed.ino, but retained here for future reference/use.
  // NOTE: If user simply presses # to default to a value, it gets returned as zero.  Thus we can't have a prompt which accepts
  // zero as a valid return value unless we disallow # as "choose default."

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
  // Rev: 06-27-24
  // Returns a single key press as a char from a 3x4 membrane keypad.
  char key = 0;
  while (key == 0) {
    key = keypad.getKey();
  }
  return key;
}
