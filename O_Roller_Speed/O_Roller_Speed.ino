// Roller_Speed Speedometer and Deceleration Distance calculator.  Rev: 06/30/24
// 
// 07/01/24: NOTES FROM ROLLER_DECELERATION:
// 
// Calculates distance to decelerate from High/Medium/Low speed to Crawl speed.
// This new version only has a single set of values for each speed Crawl, Low, Medium, and High (not 3 per speed as before.)
// This is because my shortest siding allows a realistic stop for any loco at each loco's High speed, so I can easily realistically
// stop at the two lower speeds as well, so no need for further parameters.
// For example, WP803A has a very fast High speed of 114 - 470mm/sec.  Using 3 speed steps each 350ms allows the loco to reach its
// Crawl speed (Legacy 7, 23mm/sec) in 2550mm = about 100", which is shorter than my shortest siding.
// Given a set of speed and delay values for a loco, this will calculate the distance (and time, which is trivial) to slow from a
// given speed (low, medium, or high) to the loco's legacy Crawl speed.

// BEFORE RECORDING RESULTS for any given locomotive, we must calculate actual time to run 5 meters (5000mm) at a certain
// intermediate (but random, doesn't matter) speed.  Then run on the roller bearings at that same speed and count the quarter
// revolutions, to determine the appropriate "bearing circumference" to use on that loco's speed calculations.

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
//   Crawl is around  23 mm/sec = 2.5 SMPH
//   Low   is around  93 mm/sec = 10 SMPH
//   Med   is around 186 mm/sec = 20 SMPH
//   High  is around 233 mm/sec = 25 SMPH
// For ultra-low-speed locos (such as Shay), we will target:
//   Crawl is around  23 mm/sec = 2.5 SMPH
//   Low   is around  65 mm/sec =  7 SMPH
//   Med   is around 112 mm/sec = 12 SMPH
//   High  is around 168 mm/sec = 18 SMPH
// 
//   High speed is around 220 mm/sec.
// 
// 
// 
// 06/28/24: Complete overhaul focusing on 1) Loco speedometer; 2) Deceleration distance calculator; 3) Loco_Ref FRAM populator.
// 11/15/22: All Legacy and TMCC functions written and tested working.
// 1. Acts as a locomotive speedometer, displaying both mm/sec and SMPH for any given Legacy speed setting.
// 2. Determines distance required to slow a loco from Low/Med/High speed to Crawl speed at given Legacy step skip and step delay.
//    For example, the distance required to slow a loco from Legacy 114 to Legacy 20, stepping down 3 steps each 500ms.
// Both functions require an accurate circumference of the roller bearing on the test track, which varies slightly for each loco.
//   Determine how long it takes a loco to run for 5 meters at a constant medium speed on the actual layout.  Convert to mm/sec.
//   Run the loco at the same speed on the roller bearings and adjust bearing circumference until displayed mm/sec matches actual.
// Primarily we watch how fast the roller bearing is turning, and since we know its circumference, we can calculate and display the
// estimated speed and distance travelled of a loco (mm, mm/sec, and SMPH.)
// IT'S VERY IMPORTANT that the white tape on the roller bearing is CLEAN, else we get false triggers which make the loco look as
// if it's moving faster than it actually is.

// This code can be used in several ways:
// 1. DETERMINE ROLLER-BEARING CIRCUMFERENCE for each loco, based on 5-meter layout speed tests vs. running on roller bearings.
//    This must be done using a speed that produces very consistent results on the roller bearings; not too slow or too fast.
//    Running at the same speed on the roller bearings as on the layout, and comparing time per rev versus actual mm/sec, will
//    give us the data we need to establish a reliable bearing circumference FOR EACH LOCO.  Unknown why they differ slightly.
//    Formula: At a given speed: (actual mm/sec measured on the layout) * (seconds/rev measured on the roller bearings) = mm/rev.
//    mm/rev is the roller bearing circumference.

//    AS OF 11/15/22, circumferences for ATSF 1484, Shay, and WP F-3 are VERY ACCURATE.  As we add new locos to the roster, we will
//    need to test time-at-a-constant speed on the 5-meter straight stretch on the layout, then run at that same speed for the same
//    amount of time on the roller bearings, to determine the roller bearing circumference.

// 2. ROLLER-BEARINGS DETERMINE LOCO SPEED.  Once accurate roller bearing circumference has been established for a given loco, run
//    the loco on the roller bearings at the Legacy speeds decided on for Crawl, Low, Medium, and High speeds, and record the
//    actual speeds (in mm/sec) in the Loco Reference table in FRAM (or at least make a record of them for later use.)

// 3. ROLLER-BEARING DECELERATION DISTANCE for populating the Loco Reference table's stopping-distance data.  Prompt operator for
//    locoNum, Legacy Crawl, Legacy Low/Med/High speed settings, momentum time-delay and skip-step values, and run through them
//    automatically and report the slow-down DISTANCES (and TIMES, because why not) from speed to slow to Crawl.
//    We have a function to calculate the TIME required to decelerate from one speed to another (i.e. Medium to Crawl) without even
//    needing a loco on the roller bearings: just multiplies the number of speed steps needed by the delay between each step.
//    We'll be using TimerOne 1ms interrupts to count the number of quarter revs of the roller bearing during the time we are
//    slowing down (and using it's circumference to determine distance.)

// 4. ON-TRACK DECELERATION PERCEPTION: Simple Legacy speed controls to send to locos on the layout, to determine reasonable rates
//    of deceleration.  We can try a number of different "delay between speed command" values (i.e. 160ms, 360ms, etc.) and "Legacy
//    speed steps to skip each time" such as -1, -2, -3, -4, etc. to find the largest skip that still produced non-jerky
//    deceleration.  These might be unique to each loco, since i.e. the Shay behaves very differently than the Big Boy.

// 5. LOCO_REF FRAM POPULATOR.  If running in Visual Studio, update the data statements in Loco_Reference.cpp and populate FRAM.

// IMPORTANT PROCEDURE ON HOW TO ESTABLISH EACH LOCO'S SPEED AT VARIOUS SETTINGS (5/23/22):
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

// CONVERTING TO/FROM SMPH and mm/Sec using 1:48 scale:
//   1  real mile = 1,609,344 mm (exactly 5280 ft.)
//   1 scale mile =    33,528 mm (exactly 110 ft.)
//   1 SMPH       =    33,528 mm/hour
//                =   558.798 mm/min
//                = 9.3133333 mm/sec
//   i.e. 558.798mm/sec = 60 SMPH

// SPEED AND MOMENTUM NOTES.  
// IMPORTANT: Regarding how many trains we can send Legacy commands to at about the same time (i.e. slow multiple trains at once.)
// Since we can only send serial commands to Legacy every 30ms (any faster and Legacy may ignore them,) we'll need to consider how
// many trains we might want to be accellerating/decelerating simultaneously.
// i.e. If we allow for possibly decelerating 10 trains at the same time, we'll only be able to send a command to each train every
// 300ms, which is about 3 commands per second.  Although that will realistically never happen, let's plan on sending speed
// commands to any given loco not more often than every 320ms.  This will be fine for high momentum, but if we want to slow down at
// low momentum, we'll need to slow down multiple Legacy speed steps at a time.
// NOTE: Once we trip a STOP sensor, we'll send commands to stop very quickly, but not instantly from Crawl.
// In short, we sometimes have to do 3 Legacy speed steps down every x milliseconds in order to keep the steps >= 350ms apart.
// i.e. When WP F3 comes into block #1 at high speed, we must step down 3 speeds each 350ms in order to stop in less than 100".
// The "slowest" we should stop will be 1 step each 500ms, because longer step-delays give us less accurate stopping locations.
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

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_NUL;  // Global just needs to be defined for use by Train_Functions.cpp and Message.cpp.
char lcdString[LCD_WIDTH + 1] = "RSP 06/30/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

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
#include <Keypad.h>  // Also includes Key.h
const byte KEYPAD_ROWS = 4; //four KEYPAD_ROWS
const byte KEYPAD_COLS = 3; //three columns
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// Analog pins A0, A1, etc. are digital pins 54, 55, etc.
// KEYPAD_ROWS are A3..A6 (57..60), Columns are A0..A2 (54..56)
byte keypadRowPins[KEYPAD_ROWS] = {60, 59, 58, 57}; // Connect to the row pinouts of the keypad
byte keypadColPins[KEYPAD_COLS] = {56, 55, 54};     // Connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), keypadRowPins, keypadColPins, KEYPAD_ROWS, KEYPAD_COLS );

// *** TIMER INTERRUPT USED FOR KEEPING TRACK OF ROLLER BEARING REVOLUTIONS (1/4 of a rev at a time) when decelerating.
#include <TimerOne.h>  // We will generate an interrupt every 1ms to check the status of the roller bearing IR sensor.
// CALCULATING BOUNCE TIME for the IR sensor that monitors the roller bearing black-and-white stripes and they pass by:
//   Running a loco at 60smph (highest reasonable speed) equals about 13.5 revs on the roller bearing = about 54 detections/second.
//   54 state changes per second (1000 ms) = a state change every 18ms.
//   Thus, DEBOUNCE_DELAY *must* be less than 18 milliseconds to work at the highest loco speeds.
//   At Legacy speed 1, bounce is usually about .3ms, but occasional spurious 3.5ms bounces sometimes happen.
//   Thus, DEBOUNCE_DELAY should be at least 4 milliseconds.
//   IMPORTANT: The IR sensor module has a pot adjustment, and if not perfectly adjusted then bounce can be much longer.
// We will define DEBOUNCE_DELAY inside of our ISR since it's not needed anywhere else.

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

// 5/2/22: Measured bearing diameter w/ calipers as 12.84 mm, which implies approx 40.338mm circumference ESTIMATE.

// 5/22/22: MEASURED TIMES ON 5-METER SECTION OF LAYOUT.  Avg times based on fwd direction; reverse times were always about the same.
// Measured  BIG BOY on 5m track @ Legacy 50 AVG  36.87 sec = 135.61mm/sec.
// Measured  BIG BOY on bearings @ Legacy 50 AVG .30698 sec/rev @ 135.61mm/sec = 41.6296mm/rev
// Measured  SP 1440 on 5m track @ Legacy 50 AVG  37.80 sec = 132.28mm/sec.
// Measured  SP 1440 on bearings @ Legacy 50 AVG .30897 sec/rev @ 132.28mm/sec = 40.8706mm/rev
// Measured WP 803-A on 5m track @ Legacy 50 AVG  36.63 sec = 136.50mm/sec.
// Measured WP 803-A on bearings @ Legacy 50 AVG .30562 sec/rev @ 136.50mm/sec = 41.7171mm/rev
//     Oct 24 2022                               .31300 sec/ref @ 133.30mm/sec
//          WP 803-A on bearings TRACTION TIRE  .29475 sec/rev @ 141.00mm/sec
// Measured     SHAY on 5m track @ Legacy 80 AVG  38.29 sec = 130.60mm/sec.
// Measured     SHAY on bearings @ Legacy 80 AVG .31609 sec/rev @ 130.60mm/sec = 41.2814mm/rev
// Measured SF 4-4-2 on 5m track @ Legacy 45 AVG  42.80 sec = 116.82mm/sec.
// Measured SF 4-4-2 on bearings @ Legacy 45 AVG  .3510 sec/rev @ 116.82mm/sec = 41.0024mm/rev (very unreliable at this speed: 220 - 287 on 6/18/22 - ugh.)
// Measured SF 4-4-2 on 5m track @ Legacy 70 AVG  23.03 sec = 217.11mm/sec.
// Measured SF 4-4-2 on bearings @ Legacy 70 AVG  .1910 sec/rev @ 217.11mm/sec = 41.4680mm/rev (Use this figure since closer to other values above)

// TARGET CRAWL = 2.5smph = 23.3mm/sec
// TARGET LOW   =  10smph = 93.1mm/sec FOR HIGHER-SPEED LOCOS
// TARGET LOW   = 
// The following average speeds for C/S/M/F are generally rounded up, so we're less likely to overrun destination when stopping.

// SHAY (Eng 2):
// NOTE: The fastest Shay ever recorded went 18mph.  So I should probably bump Fast to 18smph.
// 10/27/22 after cleaning roller bearing, speeds were consistent.  Previously very unpredictable.
//   Legacy   smph    speed     mm/sec
//       1    1.8       MIN       17  Ranged from 16.31, 17.21, 16.24, 15.68. 17mm/sec.  50mm in 2", 127mm in 5".  Now 44mm/sec=4.7smph.
//      20    2.5     CRAWL       23  10/27/22: 23mm/2.5smph.  10/29/22:   45mm, 4.9smph; 51mm, 5.5smph, 48mm, 5.2smph.
//      49      7       LOW       66  10/27/22: 66mm/7smph.  Ugh, 10/29/22: 130mm/14smph.  Previously varied from 45.4 to 72.9, really unreliable.
//      72     12       MED      112  10/27/22: 112mm/12smph. Ugh, 10/29/22: 198mm/21smph.  Now back to 187/20smph.
//      93    17.9     FAST      167  10/27/22: 167mm/18smph.  10/29/22: 196mm/21smph. 190-204

// ATSF 1484 E6 (Eng 5, Trn 7):  Fairly slow small steam loco
// NOTE: The E6 steam loco had a top speed of 58mph to 75mph depending on train size.
//   Legacy   smph    speed     mm/sec
//       1    1.5       MIN       14
//       7    2.5     CRAWL       23
//      25    6.3       LOW       61
//      50   14.5       MED      136
//      80   28.4      FAST      268

// BIG BOY (Eng 14): (Huge variation in mm/sec at low speeds; less as speeds increased past Legacy 40.)
// NOTE: The fastest a Big Boy could ever go is 70mph.
//   Legacy   smph    speed     mm/sec
//       1    1.5       MIN       14
//       8    2.5     CRAWL       23
//      26     10       LOW       95  // Recently 95mm/sec, previously 61-62 10/29/22, 65 = 7, 61=6.5, 
//      74     26       MED      239
//     100     41      FAST      384

// SP 1440 (Eng 40): Final data as of 5/23/22.
// NOTE: This is a switcher, so speeds were kept low.  The Alco S-2 switcher had a top speed of 60mph.
//   Legacy   smph    speed     mm/sec
//      15      8     CRAWL       73  But as low as 55mm/sec
//      44     12       LOW      112  Not very accurate, sometimes slower
//      55     16       MED      151
//      75     25      FAST      236

// WP F3 803-A (Eng 83, Trn 8): High-speed passenger train.
// NOTE: F3/F7 had a top speed of between 65mph and 102mph depending on gearing.
//   Legacy   smph    speed     mm/sec
//       1    1.6       MIN       15  14.94/1.6, 15.01/1.6
//       7    2.5     CRAWL       23
//      37     10       LOW       95  But as low as 90.  90.1, 90.5, 90.5, 90.0 / 91.0
//      93     35       MED      330  Consistent
//     114     50      FAST      463  Consistent 

// Declare a set of variables that will need to be defined for each engine/train:
byte locoNum                =  0;   // Train, Engine, Acc'y number etc.
float bearingCircumference =  0;   // Will be around 41mm but varies slightly with each loco

byte legacyCrawl           =  0;   // Legacy speed setting 0..199
int  rateCrawl             =  0;   // Resulting loco speed in mm/sec (can be > 255)
byte legacyLow             =  0;
int  rateLow               =  0;
byte legacyMed             =  0;
int  rateMed               =  0;
byte legacyHigh            =  0;
int  rateHigh              =  0;

//byte legacySpeed[4] = { 20, 38, 64, 83 };    // Crawl|S|M|F speed for this loco.  0..31 for TMCC or 0..199 for Legacy.//
//unsigned int decelDelay[3] = { 320, 160, 80 };  // H|M|L momentum settings to t/////////ry
//byte legacyStep = 1;               // Try 2, 3, 4, etc. w/ Legacy; try 2 w/ TMCC but each TMCC step = 6.25 Legacy steps.

const byte PIN_RPM_SENSOR = 2;   // Arduino MEGA pin number that the IR sensor is plugged into (not a serial pin.)
const byte PIN_PUSHBUTTON = 3;   // Pushbutton connects ground to Pin 3 when pressed, to tell software when to begin whatever.

const float SMPHmmSec = 9.3133;  // mm/sec per 1 SMPH.  UNIVERSAL CONSTANT for O scale, regardless of bearing size or anything else.

// Define global bytes for each of the 9 possible used in Legacy commands
char devType =  ' ';  // Engine or Train (even if TMCC)
char testType = ' ';  // 'S' = Speedometer on roller bearings; 'D' = Deceleration time & distance; 'F' = FX; 'P' = Populate FRAM

unsigned long startTime = millis();
unsigned long endTime   = millis();

char locoDesc[8 + 1] = "        ";   // Holds 8-char loco description.  8 chars plus \0 newline.
char locoRestrict[5 + 1] = "     ";  // Holds 5-chars of restrictions from Loco Ref.  5 chars plus \0 newline.
byte numBytes = 0;  // Global used to receive from functions via pointer
bool debugOn = false;  // In deceleration mode, this can produce a lot of output on the Serial monitor.
bool extTrip = false;  // In deceleration mode, do we begin slowing when pin is grounded (pushbutton or relay) or internally?
bool fastStop = false;  // In decleration mode, do we want to slow from Crawl to Stop, or Stop Immediate?

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();
  pinMode(PIN_RPM_SENSOR, INPUT);
  pinMode(PIN_PUSHBUTTON, INPUT_PULLUP);

  // *** QUADRAM EXTERNAL SRAM MODULE ***
  initializeQuadRAM();  // Add-on memory board provides 56,832 bytes for heap

  // *** INITIALIZE SERIAL PORTS ***
  Serial.begin(115200);    // SERIAL0_SPEED.
  // Serial1 instantiated via Display_2004/LCD2004.
  Serial2.begin(19200);  // Serial 2 for Thermal Printer, either 9600 or 19200 baud.
  Serial3.begin(9600);   // Serial 3 will be connected to Legacy via MAX-232

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);        // Serial monitor
  //Serial2.println(lcdString);       // Mini thermal printer

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
  Timer1.initialize(1000);  // 1000 microseconds = 1 millisecond
  // The timer interrupt will call ISR_Read_IR_Sensor() each interrupt.
  Timer1.attachInterrupt(ISR_Read_IR_Sensor);

  // *** IDENTIFY THE DEVICE AND OTHER PARMS FOR THIS TEST ***  <<<<<<<<--------<<<<<<<<--------<<<<<<<<--------<<<<<<<<--------
  // locoNum =  2;  // Shay
  locoNum =  4;  // ATSF NW2
  // locoNum =  5;  // ATSF 4-4-2 Atlantic E6
  // locoNum =  8;  // WP 803-A  (Legacy Train 8, programmed as Engine 80, 83)
  // locoNum = 14;  // Big Boy
  // locoNum = 40;  // SP 1440
  // locoNum = 3;  // SP StationSounds Diner

  // *** GET TEST TYPE FROM USER ON NUMERIC KEYPAD ***
  sprintf(lcdString, "1) Speedometer"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "2) Deceleration"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "3) Populate Loco Ref"); pLCD2004->println(lcdString); Serial.println(lcdString);
  sprintf(lcdString, "ENTER 1|2|3 + #: "); pLCD2004->println(lcdString); Serial.println(lcdString);
  unsigned int i = getKeypadValue();
  sprintf(lcdString, "%1i", i); pLCD2004->printRowCol(4, 18, lcdString); Serial.println(lcdString);
//  sprintf(lcdString, "%i", i); pLCD2004->println(lcdString); Serial.println(lcdString);
  if (i == 1) {
    testType = 'S';
  } else if (i == 2) {
    testType = 'D';
    sprintf(lcdString, "Deceleration"); Serial2.println(lcdString);
  } else if (i == 3) {
    testType = 'P';
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
//      sprintf(lcdString, "HelloABCDEFGHIJKLMNO"); pLCD2004->println(lcdString);
//      sprintf(lcdString, "Garbage text hereXYZ"); pLCD2004->println(lcdString);
//      sprintf(lcdString, "Hello againPDQASDFEW"); pLCD2004->println(lcdString);
//      sprintf(lcdString, "I'm still here!LKJIO"); pLCD2004->println(lcdString);
//      delay(3000);
//      for (int i = 1; i < 200; i++) {
//        sprintf(lcdString, "Count: %4i", i); pLCD2004->printRowCol(3, 10, lcdString);
//        sprintf(lcdString, "Count: %4i", i+200); pLCD2004->printRowCol(2, 1, lcdString);
//        sprintf(lcdString, "Count: %4i", i+100); pLCD2004->printRowCol(1, 5, lcdString);
//        sprintf(lcdString, "Count: %4i", i+950); pLCD2004->printRowCol(4, 9, lcdString);
//        delay(100);

      sprintf(lcdString, "Done!"); pLCD2004->println(lcdString); Serial.println(lcdString);

      while (true) {}

    }

    case 'S':  // Speedometer on roller bearings.
    {
      // Speedometer function does not require a locoNum (or any parms) since we'll let the user control the loco.
      // This will help us determine Legacy speed settings 1..199 (and SMPH) for our four standard speeds C/L/M/H.
      // I.e. if we want to know the Legacy speed setting for 10 SMPH, play with the throttle until the display shows 10 SMPH.
      // Then record that Legacy speed setting and the displayed speed in mm/sec into the Loco_Reference.cpp table and in Excel.
      float revTime = timePerRev();  // milliseconds per 40.7332mm (bearingCircumference).  Can take several seconds.
      // First, display the current processor time so we can easily see when display changes...
      sprintf(lcdString, "Refreshed: %8ld", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
      // Second, display how many milliseconds to make one revolution (about 40.7mm but different for every loco.)
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
      break;
    }

    case 'D':  // Deceleration test
    {
      // First ask if they want debug info on the Serial monitor (it's a lot)
      sprintf(lcdString, "Debug? 1=Y 2=N"); pLCD2004->println(lcdString); Serial.println(lcdString);
      byte d = getKeypadValue();
      debugOn = false;
      if (d == 1) {
        debugOn = true;
        sprintf(lcdString, "Yes     "); pLCD2004->printRowCol(4, 8, lcdString); Serial.println(lcdString);
      } else {
        sprintf(lcdString, "No      "); pLCD2004->printRowCol(4, 8, lcdString); Serial.println(lcdString);
      }

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
      sprintf(lcdString, "Ext trip? 1=Y 2=N"); pLCD2004->println(lcdString); Serial.println(lcdString);
      d = getKeypadValue();
      extTrip = false;
      if (d == 1) {
        extTrip = true;
        sprintf(lcdString, "Yes     "); pLCD2004->printRowCol(4, 11, lcdString); Serial.println(lcdString);
      } else {
        sprintf(lcdString, "No      "); pLCD2004->printRowCol(4, 11, lcdString); Serial.println(lcdString);
      }

      // So now we'll also need to ask if the user wants us to use "Slow From Crawl to Stop in 3 seconds" vs "Stop Immediate."
      // The reason we'd use the "Slow From Crawl" option is so we can see exactly what the loco will look like on the layout as it
      // stops.  The reason we'd use "Stop Immediate" is whenever we want to see on the actual layout exactly how far we travel
      // using the supplied parameters (using either a pushbutton or layout-controlled sensor relay.)
      sprintf(lcdString, "Fast stop? 1=Y 2=N"); pLCD2004->println(lcdString); Serial.println(lcdString);
      d = getKeypadValue();
      fastStop = false;
      if (d == 1) {
        fastStop = true;
        sprintf(lcdString, "Yes     "); pLCD2004->printRowCol(4, 12, lcdString); Serial.println(lcdString);
      } else {
        sprintf(lcdString, "No      "); pLCD2004->printRowCol(4, 12, lcdString); Serial.println(lcdString);
      }

      // Now we need to locoNum from the user...
      sprintf(lcdString, "ENTER locoNum: "); pLCD2004->println(lcdString); Serial.println(lcdString);
      locoNum = getKeypadValue();
      sprintf(lcdString, "%2i", locoNum); pLCD2004->printRowCol(4, 16, lcdString); Serial.println(lcdString);
      sprintf(lcdString, "Loco: %2i", locoNum); Serial2.println(lcdString);
      if ((locoNum < 1) || (locoNum > TOTAL_TRAINS)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString);  Serial.println(lcdString);endWithFlashingLED(4);
      }
      if (locoNum == 4) {
        bearingCircumference =  40.5279;  // Accurate for ATSF NW2 as of 6/27/24
      }
      // NOTE: The following startup commands can all be given the same startTime because the Engineer will ensure they are separated
      // by the minimum required delay between successive Legacy commands (25ms.)  However we can add a slight delay if we want them to
      // be executed in a certain order...
      pDelayedAction->initDelayedActionTable(debugOn);
      // Start up the loco
      pEngineer->initLegacyCommandBuf(debugOn);
      startTime = millis();
      pDelayedAction->populateLocoCommand(startTime, locoNum, LEGACY_ACTION_STARTUP_FAST, 0, 0);
      // Set loco smoke off
      startTime = startTime + 50;
      pDelayedAction->populateLocoCommand(startTime, locoNum, LEGACY_ACTION_SET_SMOKE, 0, 0);  // 0/1/2/3 = Off/Low/Med/High
      // Set loco momentum to zero
      startTime = startTime + 50;
      pDelayedAction->populateLocoCommand(startTime, locoNum, LEGACY_ACTION_MOMENTUM_OFF, 0, 0);
      // Set direction forward
      startTime = startTime + 50;
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

      sprintf(lcdString, "Start speed: "); pLCD2004->println(lcdString); Serial.println(lcdString);  // Generally this will be Low/Med/High
      byte startSpeed = getKeypadValue();
      sprintf(lcdString, "%3i", startSpeed); pLCD2004->printRowCol(4, 14, lcdString); Serial.println(lcdString);
      sprintf(lcdString, "Start speed: %3i", startSpeed); Serial2.println(lcdString);
      if ((startSpeed < 1) || (startSpeed > 199)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
      }

      sprintf(lcdString, "End speed: "); pLCD2004->println(lcdString); Serial.println(lcdString);  // Generally this will be Crawl
      byte targetSpeed = getKeypadValue();
      sprintf(lcdString, "%3i", targetSpeed); pLCD2004->printRowCol(4, 12, lcdString); Serial.println(lcdString);
      sprintf(lcdString, "End   speed: %3i", targetSpeed); Serial2.println(lcdString);
      if ((targetSpeed < 0) || (targetSpeed > 199)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
      }

      sprintf(lcdString, "Steps/speed: "); pLCD2004->println(lcdString); Serial.println(lcdString);  // Generally this will be 1, 2, or 3
      byte speedSteps = getKeypadValue();
      sprintf(lcdString, "%1i", speedSteps); pLCD2004->printRowCol(4, 14, lcdString); Serial.println(lcdString);
      sprintf(lcdString, "Speed steps: %3i", speedSteps); Serial2.println(lcdString);
      if ((speedSteps < 0) || (speedSteps > 10)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
       }

      sprintf(lcdString, "Step delay: "); pLCD2004->println(lcdString); Serial.println(lcdString);  // Generally this will be at least 330ms
      unsigned int stepDelay = getKeypadValue();
      sprintf(lcdString, "%4i", stepDelay); pLCD2004->printRowCol(4, 13, lcdString); Serial.println(lcdString);
      sprintf(lcdString, "Step delay: %4i", stepDelay); Serial2.println(lcdString);
      if ((stepDelay < 0) || (stepDelay > 5000)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(4);
      }


      while (true) {
        // Calculate estimated time to slow from startSpeed to targetSpeed
        unsigned long estimatedTimeToSlow = pDelayedAction->speedChangeTime(startSpeed, speedSteps, stepDelay, targetSpeed);
        sprintf(lcdString, "Est %lu ms.", estimatedTimeToSlow); pLCD2004->println(lcdString); Serial.println(lcdString);

        // Confirm we are starting at speed zero...
        byte TPSpeed = pTrainProgress->currentSpeed(locoNum);
        sprintf(lcdString, "TP Speed: %i", TPSpeed);  pLCD2004->println(lcdString); Serial.println(lcdString);

        // Populate Delayed Action with commands to speed up to our starting speed, 1 step at a time, every 50ms.
        startTime = millis() + 1000;
        pDelayedAction->populateLocoSpeedChange(startTime, locoNum, 1, 50, startSpeed);  // Assume starting speed is zero per Train Progress

        do {
          pEngineer->executeConductorCommand();
        } while (pTrainProgress->currentSpeed(locoNum) != startSpeed);

        // Now we are up to speed, according to Train Progress loco's current speed
        TPSpeed = pTrainProgress->currentSpeed(locoNum);
        sprintf(lcdString, "TP Speed: %i", TPSpeed); pLCD2004->println(lcdString); Serial.println(lcdString);
        delay(2000);  // Just run at our starting speed for a couple seconds to let everything normalize

        // To begin slowing, are we acting on our own or are we waiting for a pin to be grounded (button press or sensor relay)?
        if (extTrip) {  // Wait for our pin to be grounded, either by a pushbutton or by a layout sensor relay closure


        }

        // Whether we were waiting for an external signal, or can start on our own -- start slowing down now!
        startTime = millis();  // Start slowing immediately now
        // The following "slow down" sequence assume a starting speed as retrieved from Train Progress, per above confirmation.
        pDelayedAction->populateLocoSpeedChange(startTime, locoNum, speedSteps, stepDelay, targetSpeed);
        // Let's count some revs!
        noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
        bearingQuarterRevs = 0;  // This will count the number of state changes seen by the IR sensor on the bearing
        interrupts();

        do {
          pEngineer->executeConductorCommand();
        } while (pTrainProgress->currentSpeed(locoNum) != targetSpeed);
        
        // We've reached the target speed!  Snag the value of how many quarter revs we've counted...
        noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
        tempBearingQuarterRevs = bearingQuarterRevs;  // Grab our ISR "volatile" value
        interrupts();
        endTime = millis();
        // The following accounts for the average error of counting approx 1/4 rev (10mm) too far a distance travelled.
        // The error can be anywhere from 0 to 2 quarter-revs = 0mm to 20mm.  So we'll average the error and assume 10mm too far.
        // See the comments under ISR_Read_IR_Sensor() for full details.
        tempBearingQuarterRevs = tempBearingQuarterRevs - 1;

        if (fastStop) {
          pDelayedAction->populateLocoCommand(millis(), locoNum, LEGACY_ACTION_STOP_IMMED, 0, 0);
        } else {
          pDelayedAction->populateLocoSlowToStop(locoNum);
        }

        // Now stop the loco slowly or instantly...won't affect our distance calc either way...
        do {
          pEngineer->executeConductorCommand();
        } while (pTrainProgress->currentSpeed(locoNum) > 0);

        // Display/print the time and distance travelled from incoming speed to Crawl, in mm.  This will be:
        unsigned int distanceTravelled = tempBearingQuarterRevs * (bearingCircumference / 4.0);
        // INTERESTING TIMING: We start the timer the moment we give the command to slow to entry speed *minus* Legacy step-down,
        // which is what we will do the moment we trip the destination entry sensor (actually, probably beyond that, after we
        // continue at the incoming speed until it's time to start slowing down.)  But we stop the timer one Legacy-delay period
        // *after* giving the Crawl speed command, because that is the first moment we would want to give a "stop" command.  I.e.
        // we don't want to time this so that we give the Crawl speed command as we trip the exit sensor, but rather we want to give
        // the Crawl speed command one Legacy-delay period (i.e. 500ms) *before* we trip the exit sensor, so that we can give a
        // "stop" command (or slow to a stop from Crawl) just at the moment we trip the exit sensor.
        // Also, although there is some variation in the *distance* that we travel from the incoming speed until Crawl speed, the
        // *time* that we're recording is going to be very consistent and very accurate -- because the time won't change regardless
        // of the circumference of the bearings or whatever.  In fact, you can pretty much *calculate* the time required to slow to
        // Crawl speed simply by multiplying the number of speed step-downs by the delay time.
        sprintf(lcdString, "QRevs  : %5i", tempBearingQuarterRevs); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
        sprintf(lcdString, "Dist mm: %5i", distanceTravelled); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
        sprintf(lcdString, "Est  ms: %5lu", estimatedTimeToSlow); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);
        sprintf(lcdString, "Act  ms: %5lu", (endTime - startTime)); pLCD2004->println(lcdString); Serial.println(lcdString); Serial2.println(lcdString);

        sprintf(lcdString, "Repeat? 1=Y 2=N "); pLCD2004->println(lcdString); Serial.println(lcdString);
        d = getKeypadValue();
        if (d == 1) {
          debugOn = true;
          sprintf(lcdString, "Yes     "); pLCD2004->printRowCol(4, 9, lcdString); Serial.println(lcdString);
        } else {
          sprintf(lcdString, "No      "); pLCD2004->printRowCol(4, 9, lcdString); Serial.println(lcdString);
          while (true) {}
        }
      }
      while (true) {}

  // ********** THINK I NEED A STRUCT FOR ALL THIS ****************** <<<<<<<<<<<<<<<<<--------------<<<<<<<<<<<<<<<<<<<---------------

  switch (locoNum) {

    case  2:  // Shay Eng 2
    {
      // SHAY (Eng 2): Final data as of 5/23/22.
      // Measured     SHAY on 5m track @ Legacy 80 AVG  38.29 sec = 130.60mm/sec.
      // Measured     SHAY on bearings @ Legacy 80 AVG .31609 sec/rev @ 130.60mm/sec = 41.2814mm/rev
      //   Legacy   smph    speed     mm/sec
      //      15    2.5     CRAWL       23  Varies 21.3, 22.8
      //      37      7       LOW       69  Varies from 45.4 to 72.9 / 45.6, 47.0, 45.3 / 45.5, 48.5
      //      62     11       MED      102  Varies from 92.8 to 109.7 / 56.9, 59.0, 58.9, 58.9, 55.4 / 89.7, 89.5, 89.6
      //      84     15      FAST      141  Very consistent
      sprintf(lcdString, "Shay");  pLCD2004->println(lcdString); Serial.println(lcdString);
      devType = 'E';
      bearingCircumference =  41.2814;  // SHAY mm/rev as of 2/23/22
      legacyCrawl          =  15;  // Legacy speed setting 0..199
      rateCrawl            =  23;  // Resulting loco speed in mm/sec
      legacyLow            =  37;
      rateLow              =  69;
      legacyMed            =  62;
      rateMed              = 102;
      legacyHigh           =  84;
      rateHigh             = 141;
      break;
    }
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
//   Crawl is around  23 mm/sec = 2.5 SMPH
//   Low   is around  93 mm/sec = 10 SMPH
//   Med   is around 186 mm/sec = 20 SMPH
//   High  is around 233 mm/sec = 25 SMPH
// For ultra-low-speed locos (such as Shay), we will target:
//   Crawl is around  23 mm/sec = 2.5 SMPH
//   Low   is around  65 mm/sec =  7 SMPH
//   Med   is around 112 mm/sec = 12 SMPH
//   High  is around 168 mm/sec = 18 SMPH

    case  4:  // ATSF NW2
      // For a slow-speed switcher engine (such as an NW-2), we will target:
      //   Crawl target is around 2.5 SMPH =  23 mm/sec
      //   Low   target is around  10 SMPH =  93 mm/sec
      //   Med   target is around  20 SMPH = 186 mm/sec
      //   High  target is around  25 SMPH = 233 mm/sec
      // 06/27/24: ALL VALUES ARE CORRECT -- Circumference and speeds and rates
      //     ON THE LAYOUT, 5M at speed 101 = 183mm/sec
      //     ON THE LAYOUT, 5M at speed 140 = 312.5mm/sec
      sprintf(lcdString, "ATSF NW-2");  pLCD2004->println(lcdString); Serial.println(lcdString);
      devType = 'E';
      bearingCircumference =  40.5279;  // Accurate for ATSF NW2 as of 6/27/24
      legacyCrawl          =  21;  // Legacy speed 21 = 23mm/sec = 2.5 SMPH
      rateCrawl            =  23;  // mm/sec
      legacyLow            =  65;  // Legacy speed 65 = 93mm/sec = 10 SMPH
      rateLow              =  93;  // mm/sec
      legacyMed            = 102;  // Legacy speed 102 = 186mm/sec = 20 SMPH
      rateMed              = 186;  // mm/sec
      legacyHigh           = 117;  // Legacy speed 117 = 233mm/sec = 25 SMPH
      rateHigh             = 233;  // mm/sec
      break;

    case  5:  // ATSF 1484 4-4-2 E6 Atlantic Eng 5, Trn 7
    {
      // SF 1484 E6 (Eng 5, Trn 7): Final data as of 6/18/22.
      // Measured SF 4-4-2 on 5m track @ Legacy 70 AVG  23.03 sec = 217.11mm/sec.
      // Measured SF 4-4-2 on bearings @ Legacy 70 AVG  .1910 sec/rev @ 217.11mm/sec = 41.4680mm/rev (Use this figure since closer to other values above)
      //   Legacy   smph    speed     mm/sec
      //      12    3.4     CRAWL       33
      //      25    6.3      LOW        61
      //      50   14.5      MED       136
      //      80   28.4     FAST       268
      sprintf(lcdString, "Atlantic");  pLCD2004->println(lcdString); Serial.println(lcdString);  // Random values here...
      devType = 'E';
      bearingCircumference = 41.4680;
      legacyCrawl          =  12;  // Legacy speed setting 0..199
      rateCrawl            =  33;  // Resulting loco speed in mm/sec
      legacyLow            =  25;
      rateLow              =  61;
      legacyMed            =  50;
      rateMed              = 136;
      legacyHigh           =  80;
      rateHigh             = 268;
      break;
    }
    case 8:  // WP 803-A
    {
      // WP 803-A (Eng 83, Trn 8): Final data as of 5/23/22.
      // Measured WP 803-A on 5m track @ Legacy 50 AVG  36.63 sec = 136.50mm/sec.
      // Measured WP 803-A on bearings @ Legacy 50 AVG .30562 sec/rev @ 136.50mm/sec = 41.7171mm/rev
      //   Legacy   smph    speed     mm/sec
      //      11      6     CRAWL       53  But as low as 29mm/sec.  29.2, 28.9, 29.1, 29.4
      //      37     10       LOW       95  But as low as 90.  90.1, 90.5, 90.5, 90.0 / 91.0
      //      93     35       MED      330  Consistent
      //     114     50      FAST      463  Consistent 
      sprintf(lcdString, "WP 803-A");  pLCD2004->println(lcdString); Serial.println(lcdString);
      devType = 'T';
      bearingCircumference =  41.7171;  // WP 803-A mm/rev as of 2/23/22
      legacyCrawl          =  11;  // Legacy speed setting 0..199
      rateCrawl            =  53;  // Resulting loco speed in mm/sec
      legacyLow            =  37;
      rateLow              =  95;
      legacyMed            =  93;
      rateMed              = 330;
      legacyHigh           = 114;
      rateHigh             = 463;
      break;
    }
    case 14:  // Big Boy Eng 14
    {
      // BIG BOY (Eng 14): Final data as of 5/23/22.  Huge variation in mm/sec at low speeds; less as speeds increased past Legacy 40.
      // Measured  BIG BOY on 5m track @ Legacy 50 AVG  36.87 sec = 135.61mm/sec.
      // Measured  BIG BOY on bearings @ Legacy 50 AVG .30698 sec/rev @ 135.61mm/sec = 41.6296mm/rev
      //   Legacy   smph    speed     mm/sec
      //      12      6     CRAWL       53
      //      26     10       LOW       95
      //      74     26       MED      239
      //     100     41      FAST      384
      sprintf(lcdString, "Big Boy");  pLCD2004->println(lcdString); Serial.println(lcdString);
      devType = 'E';
      bearingCircumference =  41.6296;  // Big Boy mm/rev as of 2/23/22
      legacyCrawl          =  12;  // Legacy speed setting 0..199
      rateCrawl            =  53;  // Resulting loco speed in mm/sec
      legacyLow            =  26;
      rateLow              =  95;
      legacyMed            =  74;
      rateMed              = 239;
      legacyHigh           = 100;
      rateHigh             = 384;
    }
    case 40:  // SP 1440
    {
      // SP 1440 (Eng 40): Final data as of 5/23/22.
      // Measured  SP 1440 on 5m track @ Legacy 50 AVG  37.80 sec = 132.28mm/sec.
      // Measured  SP 1440 on bearings @ Legacy 50 AVG .30897 sec/rev @ 132.28mm/sec = 40.8706mm/rev
      //   Legacy   smph    speed     mm/sec
      //      15      8     CRAWL       73  But as low as 55mm/sec
      //      44     12       LOW      112  Not very accurate, sometimes slower
      //      55     16       MED      151
      //      75     25      FAST      236
      sprintf(lcdString, "SP 1440");  pLCD2004->println(lcdString); Serial.println(lcdString);
      devType = 'E';
      bearingCircumference =  40.8706;  // SP 1440 mm/rev as of 2/23/22
      legacyCrawl          =  15;  // Legacy speed setting 0..199.  15 = 41 to 48mm/sec in my 6/7/22 test.
      rateCrawl            =  73;  // Resulting loco speed in mm/sec (much faster than my 6/7/22 test.)
      legacyLow            =  44;
      rateLow              = 112;
      legacyMed            =  55;
      rateMed              = 151;
      legacyHigh           =  75;
      rateHigh             = 236;
      break;
    }
    default:
    {
      sprintf(lcdString, "locoNum Error!"); pLCD2004->println(lcdString); Serial.println(lcdString);
    }
  }


    // populateWhistleHorn needs a horn type and a pattern such as LEGACY_PATTERN_CROSSING.
    // LEGACY HORN COMMAND *PATTERNS* (specified as Parm1 in Delayed Action, by Conductor)
    // For the quilling horn, we'll also need to decide what pitch to specify -- there are about three distinct pitches.
    // const byte LEGACY_PATTERN_SHORT_TOOT   =  1;  // S    (Used informally i.e. to tell operator "I'm here" when registering)
    // const byte LEGACY_PATTERN_LONG_TOOT    =  2;  // L    (Used informally)
    // const byte LEGACY_PATTERN_STOPPED      =  3;  // S    (Applying brakes)
    // const byte LEGACY_PATTERN_APPROACHING  =  4;  // L    (Approaching PASSENGER station -- else not needed.  Some use S.)
    // const byte LEGACY_PATTERN_DEPARTING    =  5;  // L-L  (Releasing air brakes. Some railroads that use L-S, but I prefer L-L)
    // const byte LEGACY_PATTERN_BACKING      =  6;  // S-S-S
    // const byte LEGACY_PATTERN_CROSSING     =  7;  // L-L-S-L
/*
    pDelayedAction->populateWhistleHorn(startTime, devType, locoNum, LEGACY_PATTERN_CROSSING);
    startTime = startTime + 12000;
    pDelayedAction->populateWhistleHorn(startTime, devType, locoNum, LEGACY_PATTERN_STOPPED);  // S
    startTime = startTime + 4000;
    pDelayedAction->populateWhistleHorn(startTime, devType, locoNum, LEGACY_PATTERN_APPROACHING);  // L
    startTime = startTime + 6000;
    pDelayedAction->populateWhistleHorn(startTime, devType, locoNum, LEGACY_PATTERN_DEPARTING);  // L-L
    startTime = startTime + 8000;
    pDelayedAction->populateWhistleHorn(startTime, devType, locoNum, LEGACY_PATTERN_BACKING);  // S-S-S
    startTime = millis() + 1000;
    pDelayedAction->populateCommand(startTime, devType, locoNum,LEGACY_ACTION_STARTUP_SLOW, 0, 0);
    startTime = startTime + 8000;
    pDelayedAction->populateWhistleHorn(startTime, devType, locoNum, LEGACY_PATTERN_CROSSING);
    startTime = startTime + 2000;
    pDelayedAction->populateCommand(startTime, devType, locoNum,LEGACY_ACTION_FORWARD, 0, 0);
    startTime = startTime + 100;
    pDelayedAction->populateSpeedChange(startTime, devType, locoNum, t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed, true);
    startTime = startTime + 9000;
    t_startSpeed = t_targetSpeed;
    t_targetSpeed = 0;
*/

    // Wait for operator to press the green button...
    sprintf(lcdString, "Press to Start..."); pLCD2004->println(lcdString); Serial.println(lcdString);
    while (digitalRead(PIN_PUSHBUTTON) == HIGH) {}   // Just wait...
    // Toot the horn to ack button push was seen
    startTime = millis();
    pDelayedAction->populateLocoWhistleHorn(startTime, locoNum, LEGACY_PATTERN_SHORT_TOOT);

    // ACCELERATE FROM A STOP
    startTime = millis();
    unsigned int t_startSpeed = 0;
    byte t_speedStep = 2;              // Seems like 2 steps at a time is great.  1 is perfect, 4 is a big jerky.
    unsigned long t_stepDelay = 500;  // 600;   // For the Big Boy, 2 steps each 400ms is very smooth, seems right unless very short siding.
    unsigned int t_targetSpeed = 63;  // Legacy speed 0..199
    pDelayedAction->populateLocoSpeedChange(startTime, locoNum, t_speedStep, t_stepDelay, t_targetSpeed);

    // How long will it be until all of these speed commands have been flushed from D.A. into the Legacy Command Buffer?
    unsigned long speedChangeTime = pDelayedAction->speedChangeTime(t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed);
    unsigned long endTime = startTime + speedChangeTime;
//    sprintf(lcdString, "SCT %12lu", speedChangeTime); pLCD2004->println(lcdString); Serial.println(lcdString);
    while (millis() <= endTime) {  // This *should* run exactly as long as it takes to send all commands
      // Retrieve ripe DA action (for any loco or acc'y), if any, and enqueue in Legacy Cmd Buf for soonest possible execution
      // (subject to no successive commands within 30ms.)
      pEngineer->executeConductorCommand();
    }
    // Beware, if the LCD displays "DA Not Empty!", we stopped too soon and our speedChangeTime must have an error.  So to test,
    // populateSpeedChange with a final parm as true (wipe any existing records first) will display a message "DA 1 not empty!" if
    // there are any records for this loco still waiting in the Delayed Action table.  This would generally indicate a problem,
    // because we normally want to allow enough time for a loco to reach its target speed before giving new speed commands.
    // Since we're calling populateSpeedChange with t_startSpeed == t_targetSpeed, it won't actually insert any new D.A. records,
    // but it *will* give us a message on the LCD if D.A. wasn't empty (i.e. we didn't wait long enough for speed commands to all
    // be processed, sent to the Legacy Command Buffer, and sent to the Legacy Base.)
    t_startSpeed = t_targetSpeed;
    pDelayedAction->populateLocoSpeedChange(startTime, locoNum, t_speedStep, t_stepDelay, t_targetSpeed);
    // If we don't see a "DA 1 not empty!" message on the LCD, it means we've waited long enough, as hoped.
    // Otherwise, we'd better find the error in the speedChangeTime) calculation above.

    // Toot the horn to ack target speed reached
    startTime = millis();
    pDelayedAction->populateLocoWhistleHorn(startTime, locoNum, LEGACY_PATTERN_SHORT_TOOT);
    while (millis() < (startTime + 1000)) {
      pEngineer->executeConductorCommand();
    }

    sprintf(lcdString, "Press to slow."); pLCD2004->println(lcdString); Serial.println(lcdString);
    while (digitalRead(PIN_PUSHBUTTON) == HIGH) {}  // Wait for operator to say "start slowing down"
    startTime = millis();
    t_startSpeed = t_targetSpeed;  // From above
    t_speedStep = 2;
    t_stepDelay = 500;  // ms
    t_targetSpeed = 20;
    pDelayedAction->populateLocoSpeedChange(startTime, locoNum, t_speedStep, t_stepDelay, t_targetSpeed);
    speedChangeTime = pDelayedAction->speedChangeTime(t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed);
    startTime = startTime + speedChangeTime;
    // We should be at Crawl exactly now.
    // We'll stop immediately so that we can measure distance from t_startSpeed to Crawl/Stop.
    pDelayedAction->populateLocoCommand(startTime, locoNum, LEGACY_ACTION_STOP_IMMED, 0, 0);
/*
    while (millis() <= startTime + 50) {  // Should be essentially instant.
      pEngineer->getDelayedActionCommand();
      pEngineer->sendCommandToTrain();
    }

    t_startSpeed = t_targetSpeed;
    pDelayedAction->populateSlowToStop(startTime, devType, locoNum, t_startSpeed, true);  // Should ALWAYS take 2000ms
*/

    while (millis() <= startTime + 3000) {
      pEngineer->executeConductorCommand();
    }

    /*
    sprintf(lcdString, "Press to stop."); pLCD2004->println(lcdString); Serial.println(lcdString);
    while (digitalRead(PIN_PUSHBUTTON) == HIGH) {}  // Wait for operator to say "stop now."
    startTime = millis();
    // Assume we are moving at previous t_targetSpeed, although we should look it up via a pEngineer-> function...
    t_startSpeed = t_targetSpeed;
    pDelayedAction->populateSlowToStop(startTime, devType, locoNum, t_startSpeed, true);  // Should ALWAYS take 2000ms
    while (millis() <= startTime + 3000) {
      pEngineer->getDelayedActionCommand();
      pEngineer->sendCommandToTrain();
    }
    // We should be at crawl, or close to it.  Normally wait until we trip sensor but for testing, slow to stop.
    startTime = millis();
    // Assume we are moving at previous t_targetSpeed, although we should look it up via a pEngineer-> function...
    t_startSpeed = t_targetSpeed;
    pDelayedAction->populateSlowToStop(startTime, devType, locoNum, t_startSpeed, true);  // Should ALWAYS take 2000ms
    while (millis() <= startTime + 3000) {
      pEngineer->getDelayedActionCommand();
      pEngineer->sendCommandToTrain();
    }
*/
//pDelayedAction->dumpTable();
endWithFlashingLED(4);


    while (true) {}


// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************

    // How long will it be until all of these speed commands have been flushed from D.A. into the Legacy Command Buffer?
    speedChangeTime = pDelayedAction->speedChangeTime(t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed);
    endTime = startTime + speedChangeTime;

//sprintf(lcdString, "Time %12lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
//sprintf(lcdString, "SCT %12lu", speedChangeTime); pLCD2004->println(lcdString); Serial.println(lcdString);
//sprintf(lcdString, "ET %12lu", endTime); pLCD2004->println(lcdString); Serial.println(lcdString);

//    while (millis() < endTime) {  // This *should* run exactly as long as it takes to send all commands
//      pEngineer->getDelayedActionCommand();
//      pEngineer->sendCommandToTrain();
//    }
    // Should be stopped now; toot horn!
    startTime = millis();
    pDelayedAction->populateLocoWhistleHorn(startTime, locoNum, LEGACY_PATTERN_SHORT_TOOT);
    startTime = startTime + 1000;  // We'll wait a second for the horn to toot
    while (millis() < startTime) {
      pEngineer->executeConductorCommand();
    }

    while (true) {}


    //pDelayedAction->populateSlowToStop(startTime, devType, locoNum, t_targetSpeed, false);
    //sprintf(lcdString, "Dump 0:"); pLCD2004->println(lcdString); Serial.println(lcdString);
    //pDelayedAction->dumpTable();

      // pDelayedAction->populateCommand(millis() + 3000, 'E', 40, LEGACY_ACTION_STARTUP_FAST, 0, 0);
      // pDelayedAction->populateCommand(millis() + 5000, 'E', 40, LEGACY_ACTION_ABS_SPEED, 10, 0);
 
      //populateSpeedChange(const unsigned long startTime, const char t_devType, const byte t_locoNum, 
      //                    const unsigned int t_startSpeed, const byte t_speedStep, const unsigned int t_stepDelay,
      //                    const unsigned int t_targetSpeed);
      //pDelayedAction->populateSpeedChange(millis() + 3000, 'E', 40, 30, 10, 50, 10);

      pDelayedAction->populateLocoSpeedChange(millis() + 3000, 9, 1, 50, 1);

//      sprintf(lcdString, "Dump 1:"); pLCD2004->println(lcdString); Serial.println(lcdString);
//      pDelayedAction->dumpTable();

      unsigned long tTime = millis();

      char dType = 'X'; byte dNum = 0; byte dCmd = 0; byte dP1 = 0; byte dP2 = 0;
      while ((tTime + 5000) > millis()) {  // Give our test enough time to find all Active and ripe records...
        if (pDelayedAction->getAction(&dType, &dNum, &dCmd, &dP1, &dP2)) {
          sprintf(lcdString, "dCmd: %i", dCmd); pLCD2004->println(lcdString); Serial.println(lcdString);
          sprintf(lcdString, "dP1: %i", dP1); pLCD2004->println(lcdString); Serial.println(lcdString);
          if (dCmd == LEGACY_ACTION_STARTUP_FAST) {
            sprintf(lcdString, "Startup"); pLCD2004->println(lcdString); Serial.println(lcdString);
//            legacyStartUpSequence(dType, dNum, 'F');
          }
          if (dCmd == LEGACY_ACTION_ABS_SPEED)    {
            sprintf(lcdString, "Speed: %i", dP1); pLCD2004->println(lcdString); Serial.println(lcdString);
//            legacyAbsoluteSpeed(dType, dNum, dP1);
          }
          sprintf(lcdString, "Got at %lu", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
        }
      }

      sprintf(lcdString, "Dump 2:"); pLCD2004->println(lcdString); Serial.println(lcdString);
//      pDelayedAction->dumpTable();


      sprintf(lcdString, "Done."); pLCD2004->println(lcdString); Serial.println(lcdString);

      while (true) {}


      // These are just some inital roller-bearing tests done 5/26/22 to get a rough idea:
      // TEST RESULTS 5/26/22: WP 803-A, Legacy 114 (high) to 11 (crawl)
      //                      legacyStepDown 3, legacyDelay  500 = 137" (plus less than 2" to stop)
      //                      legacyStepDown 4, legacyDelay  500 = 102" still seems okay
      //                      legacyStepDown 5, legacyDelay  500 =  79.5" a bit rough stopping but okay
      //                      legacyStepdown 4, legacyDelay  400 =  81" and seems a bit smoother
      // TEST RESULTS 5/26/22: BIG BOY, Legacy 100 (high) to 12 (crawl)
      //                      legacyStepDown 3, legacyDelay  500 =  48" way too fast!
      //                      legacyStepDown 3, legacyDelay 1000 =  97" (plus < 2" to stop)
      //                      legacyStepDown 3, legacyDelay 1500 = 145" (plus < 2" to stop)
      // TEST RESULTS 5/26/22: SHAY   , Legacy  84 (high) to 15 (crawl)
      //                      legacyStepDown 3, legacyDelay  750 = 47" Really no need to stop any slower than this.
      //                      legacyStepDown 3, legacyDelay 1000 = 62" Looks great.
      // TEST RESULTS 5/26/22: SP 1440, Legacy  75 (high) to 15 (crawl)
      //                      legacyStepDown 3, legacyDelay  500 = 47" (plus < 2" to stop)
      //                      legacyStepDown 3, legacyDelay  750 = 70"

      // **************************************************************************************************************************
      // **************************************************************************************************************************
      // **************************************************************************************************************************
      // 6/7/22 IMPORTANT: UNFORTUNATELY, these test results vary by as much as 14% on these two tests.
      // CONCLUSION: We should establish Crawl speeds that are relatively fast so we don't die of boredom if we hit Crawl 12"
      // before the sensor.  Since our occupancy sensors are all pretty long (about 8" as I recall), we can lengthen the "slow
      // down" algorithm to account for faster Crawl speeds. Let's target Crawl speed at approximately the same SMPH (equivalent
      // mm/sec) for all engines, whether it's the Shay or the Big Boy.  Then our algorithm of "half crawl speed for x ms then
      // speed 1 for y ms then stop" should be fairly consistent regardless of loco.
      // **************************************************************************************************************************
      // **************************************************************************************************************************
      // **************************************************************************************************************************

      // TEST RESULTS 6/7/22 ON ROLLER BEARINGS for SP 1440 @ 75 to 15 (same as above) USING JUST A FOR() LOOP (not Delayed Action):
      // IT WILL BE INTERESTING TO RUN THE LOCO ON THE REAL TRACK AND SEE HOW CLOSE THE ACTUAL DISTANCES COMPARE TO THESE CALCULATIONS ****************************
      //                      stepdown 3, delay 500: 1389mm = 54.7" rounded 55"  (orig test was 47") in 10.168 sec.
      //                                    "        1266mm = 49.8"         50"
      //                                    "        1328mm = 52.8"         53"
      //                                    "        1287mm = 50.7"         51"
      //                                    "        1430mm = 56.3"         56"
      //                                    "        1410mm = 55.5"         56"
      //                                    "        1287mm = 50.7"         51"
      // FOR COMPARISON       stepdown 5, delay 833: 1246mm = 49.1"         49"                      in 10.096 sec.
      //                                             1185mm = 46.7"         47"
      //                                             1246mm = 49.1"         49"
      //                                             1205mm = 47.4"         47"
      // 
      //                      stepdown 3, delay 750: 1859mm = 73.2"         73"  (orig test was 70") in 15.167 sec.
      //                                    "        1880mm = 74.0"         74"
      //                                    "        2145mm = 84.4"         84"
      //                                    "        2115mm = 83.3"         83"
      //                                    "        1869mm = 73.6"         74"
      //                                    "        1839mm = 72.4"         72"
      //                                    "        2125mm = 83.7"         84"
      // FOR COMPARISON      stepdown 5, delay 1250: 1757mm = 69.2"         69"                       in 15.100 sec
      //                                             1839mm = 72.4"         72"
      //                                             1839mm = 72.4"         72"
      //                                    "        1818mm = 71.6"         72"

//      legacyBlowHornToot(devType, locoNum); delay(LEGACY_CMD_DELAY);
      
      // **********************************
      // ***** Bring loco up to speed *****
      // **********************************
      byte          entrySpeed =  75;  // legacyHigh;
      byte      legacyStepDown =   5;  // How many Legacy speed steps to reduce each time we lower the speed
                   legacyCrawl =  15;  // Already been set above, but here we can monkey with it
      unsigned int legacyDelay = 1250;  // How many ms to pause between each successive reduced speed command

      byte currentSpeed = 0;
      for (byte i = 1; i <= entrySpeed; i++) {
        sprintf(lcdString, "Speed: %i", (int)i); pLCD2004->println(lcdString); Serial.println(lcdString);
//        legacyAbsoluteSpeed(devType, locoNum, i);
        currentSpeed = i;
        delay(LEGACY_CMD_DELAY);  // Really going to come up to speed quickly!
      }
      // Wait for loco to come up to speed...Meanwhile send data to printer...
      sprintf(lcdString, "Loco : %i",      locoNum); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      sprintf(lcdString, "Speed: %i",     entrySpeed); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      sprintf(lcdString, "Step : %i", legacyStepDown); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      sprintf(lcdString, "Crawl: %i",    legacyCrawl); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      sprintf(lcdString, "Delay: %i",    legacyDelay); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      delay(3000);

      // Toot horn to signal we're starting to slow down...
//      legacyBlowHornToot(devType, locoNum); delay(LEGACY_CMD_DELAY);

      // Let's count some revs!
      noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
      bearingQuarterRevs = 0;  // This will count the number of state changes seen by the IR sensor on the bearing
      interrupts();
      unsigned long startTime = millis();

      // Slow the loco from current speed to Crawl, using first set of deceleration parameters...
      // We check i >= legacyCrawl because that's when we want to stop slowing down.  However, in the unlikely event that
      // legacyStepDown > legacyCrawl, the loop could execute with i < 0, which Legacy interprets as a large positive speed!
      // Start sending speeds one legacyStepDown below currentSpeed, because we're already moving at currentSpeed.
      // Stop sending speeds when we hit legacyCrawl *or* the last step down before that if the next step down would put us lower
      // than crawl.
      for (byte i = currentSpeed - legacyStepDown; ((i >= legacyCrawl) && (i >= legacyStepDown)); i = i - legacyStepDown) {  // Never allow i < 0!
        sprintf(lcdString, "Speed: %i", (int)i); pLCD2004->println(lcdString); Serial.println(lcdString);
//        legacyAbsoluteSpeed(devType, locoNum, i);
        currentSpeed = i;
        delay(legacyDelay);
      }
      // We may not quite yet be at crawl, but if not, we will be less than legacyStepDown steps from it, so go to crawl now.
//      legacyAbsoluteSpeed(devType, locoNum, legacyCrawl);  // Go ahead and set/confirm crawl speed

      // Okay we've reached Crawl speed!  Snag the value of how many quarter revs we've counted...
      noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
      tempBearingQuarterRevs = bearingQuarterRevs;  // Grab our ISR "volatile" value
      interrupts();
      // The following accounts for the average error of counting approx 1/4 rev (10mm) too far a distance travelled.
      // The error can be anywhere from 0 to 2 quarter-revs = 0mm to 20mm.  So we'll average the error and assume 10mm too far.
      // See the comments under ISR_Read_IR_Sensor() for full details.
      tempBearingQuarterRevs = tempBearingQuarterRevs - 1;
      endTime = millis();

      // Toot horn to signal we've reached Crawl speed...
//      legacyBlowHornToot(devType, locoNum); delay(LEGACY_CMD_DELAY);

      // Normally at Crawl we would keep going until we trip the siding exit sensor (unless we'd already tripped it.)
      // But we'll just stop.  Here we'll use our "Stop from crawl, slowly" function.
      // Unfortunately there isn't much discernible difference between Crawl (speed 15) and speed 1, at least for SP 1440:
      //  SP 1440 at Speed 15: 41-48mm/sec (Crawl)
      //  SP 1440 at Speed  7: 44-49mm/sec (half Crawl)
      //  SP 1440 at Speed  1: 34-38mm/sec (1)
      //  If we do 500ms at half crawl plus 500ms at speed 1 before stopping, we'll travel approx 40mm = about 1.5" before stopping.  OK.
      // However I previously recorded SP 1440 speed 15 at 73mm/sec -- proving that slow speeds are unreliable.  In any event:
      //  SP 1440 at Speed 15: 73mm/sec (Crawl per my original test)
      //  SP 1440 at Speed  7: 70mm/sec guessing; err on high side
      //  SP 1440 at Speed  1: 50mm/sec guessing; err on high side
      // Implies if we do 500ms at 70mm/sec + 50ms at 50mm/sec = 60mm to stop = 2.4", which is on the high side of acceptable.
      // In any event, we can modify the *time* we spend at half crawl and at speed 1 in global consts to make this work for any loco.
//      crawlToStop(devType, locoNum, legacyCrawl);
      delay(LEGACY_CMD_DELAY);  // Wait for last speed command to complete
//      legacyBlowHornToot(devType, locoNum); delay(LEGACY_CMD_DELAY);

      // Display/print the time and distance travelled from incoming speed to Crawl, in mm.  This will be:
      unsigned int distanceToCrawl = tempBearingQuarterRevs * (bearingCircumference / 4.0);
      // INTERESTING TIMING: We start the timer the moment we give the command to slow to entry speed *minus* Legacy step-down,
      // which is what we will do the moment we trip the destination entry sensor (actually, probably beyond that, after we
      // continue at the incoming speed until it's time to start slowing down.)  But we stop the timer one Legacy-delay period
      // *after* giving the Crawl speed command, because that is the first moment we would want to give a "stop" command.  I.e.
      // we don't want to time this so that we give the Crawl speed command as we trip the exit sensor, but rather we want to give
      // the Crawl speed command one Legacy-delay period (i.e. 500ms) *before* we trip the exit sensor, so that we can give a
      // "stop" command (or slow to a stop from Crawl) just at the moment we trip the exit sensor.
      // Also, although there is some variation in the *distance* that we travel from the incoming speed until Crawl speed, the
      // *time* that we're recording is going to be very consistent and very accurate -- because the time won't change regardless
      // of the circumference of the bearings or whatever.  In fact, you can pretty much *calculate* the time required to slow to
      // Crawl speed simply by multiplying the number of speed step-downs by the delay time.
      unsigned long timeToCrawl = endTime - startTime;
      sprintf(lcdString, "QRevs: %i", tempBearingQuarterRevs); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      sprintf(lcdString, "Dist : %imm", distanceToCrawl); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      sprintf(lcdString, "Time : %lums", timeToCrawl); pLCD2004->println(lcdString); Serial.println(lcdString); // Serial2.println(lcdString);
      // Serial2.println(); Serial2.println();
// 1573mm in 12100ms
// 1604mm in 12100ms
// 1471mm in 12100ms
// Speed 75 to speed 15 step 5 @ 1 second
      // Just be done for now...
      sprintf(lcdString, "Done."); pLCD2004->println(lcdString); Serial.println(lcdString);
      while (true);
      break;
    }
    default:
    {
      sprintf(lcdString, "testType Error!"); pLCD2004->println(lcdString); Serial.println(lcdString);
      while (true);
    }
  }
}

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

void ISR_Read_IR_Sensor() {  
  // Rev: 06-07-22.
  // Count up the total number of bearing quarter revs.  Must be called every 1ms by TimerOne interrupt.
  // This could have an error of up to 20mm, since each 1/4 rev stripe is about 10mm long.  We could start scanning at the very end
  // of a stripe, and consider that 10mm when it was potentially only 1mm, and we could end at the very beginning of a stripe,
  // which we'd see as 10mm when it was only i.e. 1mm.  So that would make it look as if we'd gone 18mm farther than we actually
  // travelled.  Only if we start at the very beginning of a stripe and end at the very end of a stripe will the results be most
  // accurate.  Since the errors will only ADD to the distance not actually travelled, we should perhaps deduct 10mm as an average
  // error factor from the total calculated distance travelled.  Thus just subtract 1 from the bearingQuarterRevs.
  // Gets called every 1ms to see if the roller bearing IR sensor has changed state.  There are four state changes per rev.
  // Updates the global volatile variable bearingQuarterRevs to count each quarter turn of the roller bearing.
  // When you want to start counting: Call nointerrupts(), then reset the volatile bearingQuarterRevs, then call interrupts().
  // When you want to stop counting: Call nointerrupts(), copy bearingQuarterRevs to a regular variable, then call interrupts().
  // We are allowing for a bounce of up to 4ms for the signal to stabilize after each state change, which is no problem because the
  // fastest loco (Big Boy) triggers a state change every 13ms, and it would *never* travel that fast on the layout.
  // At most speeds, bounce is much less than 1ms, but at speed 1 there is sometimes a weird bounce of 3.5ms (very rare.)

  const byte DEBOUNCE_DELAY = 4;  // Debounce time in 1ms increments (1ms interrupt countdown.)
  static byte bounceCountdown = 0;  // This will get reset to DEBOUNCE_DELAY after each new sensor change
  static int oldSensorReading = LOW;
  static int newSensorReading = LOW;

  // Wait until our bounce delay has transpired after reading a state change, before looking for the next one.  I.e., since this
  // function will be called every 1ms, we are reading the sensor status every 4th call, i.e. reading every 4ms.
  // There is a potential problem with the first time you enter this block - it depends on the initial state of the sensor, versus
  // out variable oldSensorReading arbitrary initial value - which could result in an error of one 1/4 rev count.  However, as long
  // as we allow the interrupt to start counting for a moment before we care about the results, and then reset bearingQuarterRevs
  // when we are ready to pay attention to the count, this will not be a problem.
  if (bounceCountdown > 0) {  // Keep waiting for any bounce to resolve
    bounceCountdown --;  // We do this because we know this is being called by the timer interrupt once per ms
  } else {  // See if bearing has turned 1/4 turn since our last check
    newSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH
    if (newSensorReading != oldSensorReading) {  // Sensor status has legitimately changed!
      oldSensorReading = newSensorReading;
      bearingQuarterRevs++;
      bounceCountdown = DEBOUNCE_DELAY;  // Reset bounce countdown time each time we fetch a new sensor reading
    }
  }
}

float timePerRev() {
  // Rev: 06-28-24.
  // 06-28-24: Added logic to vary the number of quarter revs we wait for (and count) depending on speed of the loco.  This
  //           tries to display the loco speed no faster than each .5 seconds, and no slower than each 1 second.
  //           Ranges from 2 full revs at slow speed, to 16 full revs at high speed.
  // This function just sits and watches the roller bearing run (at a steady speed, presumably) for a specified number of
  // revolutions, and then returns the average time per rev.
  // Watch the roller bearing IR sensor and return the time for one rev, in ms (averaged based on 4 revs.)

  // Before we begin, wait for two stable reads (separated by DEBOUNCE_DELAY ms) and *then* wait for a state change
  unsigned long const DEBOUNCE_DELAY = 4;  // Debounce time in ms when used to compare with millis()
  static byte bracket = 1;            // Brackets are speed ranges for how long to count revs, using next two variables
  static int quarterTurnsToWait = 9;  // Increase 9..17..33..65 as we speed up based on subTime
  static float timeDivisor =  2.0;    // Increase 2.0..4.0..8.0..16.0 as we speed up based on subTime
  int oldSensorReading = LOW;
  int newSensorReading = LOW;
  unsigned long startTime;

  // First wait for a transition so we'll know we're right at the edge, maybe bouncing or maybe not.
  // We could start timing at the first change of state, but if we happen to be at the end of a bounce, we could potentially see
  // the timer up to 3.5ms less than it actually is.  Of course this would ONLY happen at the lowest speeds when we see the
  // occasional 3.5ms fluke bounce time (usually it's about .5ms.)  But what the heck, this is for accuracy and only in our test-
  // bench code.
  // So here we "waste" up to 1/4 revolution waiting to be sure we'll be at the FRONT edge of a transition when we start the timer.
  oldSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH
  do {
    newSensorReading = digitalRead(PIN_RPM_SENSOR);
  } while (newSensorReading == oldSensorReading);  // Keep reading until there is a change
  // We just saw a transition so we are for sure near the edge of a transition from black/white or white/black.  Maybe at the
  // beginning, but POSSIBLY at the end of a 3.5ms bounce.  So now wait for debounce and then wait for next state change.
  delay(DEBOUNCE_DELAY);
  // Now we know we must be partway into a quarter rev (however long it takes to travel for DEBOUNCE_DELAY ms i.e. around 8ms.)
  // Always less than a quarter rev, which never takes less than 18ms if we are travelling at 60smph.
  oldSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH, but will be a stable read at this point
  do {
    newSensorReading = digitalRead(PIN_RPM_SENSOR);
  } while (newSensorReading == oldSensorReading);  // Kill time until we see the sensor status change
  // Okay, NOW we know we are at the FRONT edge of a transition, beginning a newSensorReading zone.
  // Start the timer and see how long it takes to go through the next four quarters (one full revolution of the bearing.)
  startTime = millis();  // Start the one-revolution timer!
  // Speed up the refresh rate (but potentially reduce the consistency) by adjusting the number of times we read a quarter turn,
  // and also don't forget to adjust the subTime calculation below.
  // For fast refresh (best at lower loco speeds), use "quarterTurn < 9"  and "startTime)) / 2.0"
  // For slower refresh (better at highest speed), use "quarterTurn < 65" and "startTime)) / 16.0"
  // I could automate this by automatically choosing the two values based on how long it takes TimePerRev() to return a value.
  // How many quarter-turns of the roller bearing to we want to count/wait, while recording the time it takes?
  for (int quarterTurn = 1; quarterTurn < quarterTurnsToWait; quarterTurn++) {
    // Wait for bounce from last read to stabilize
    delay(DEBOUNCE_DELAY);
    // Wait for the sensor to change again
    while (digitalRead(PIN_RPM_SENSOR) == newSensorReading) {}
    // Okay we just saw a transition
    newSensorReading = !newSensorReading;
  }
  // Completed our revs!  Stop the timer!
  float subTime = float(float((millis() - startTime)) / timeDivisor);

  // Now make any necessary adjustments to how many quarter revs we count, depending on how slow or fast our loco is moving...
  // This helps refresh the display more quickly when we're moving slowly, and refresh more slowly when we're moving very fast.
  float delayTime = float(millis() - startTime);  // Used only to calculate how many revs we wait for each speed reading
  if ((bracket == 1) && (delayTime < 500)) {  // Refreshing too fast; wait for more quarter turns
    bracket = 2;
    quarterTurnsToWait = 17; timeDivisor = 4.0;
  } else if ((bracket == 2) && (delayTime > 1000)) {  // Refreshing too slow; wait for fewer quarter turns
    bracket = 1;
    quarterTurnsToWait =  9; timeDivisor = 2.0;
  } else if ((bracket == 2) && (delayTime < 500)) {  // Wait for more quarter turns
    bracket = 3;
    quarterTurnsToWait = 33; timeDivisor = 8.0;
  } else if ((bracket == 3) && (delayTime > 1000)) {  // Wait for fewer quarter turns
    bracket = 2;
    quarterTurnsToWait = 17; timeDivisor = 4.0;
  } else if ((bracket == 3) && (delayTime < 500)) {  // Wait for more quarter turns
    bracket = 4;
    quarterTurnsToWait = 65; timeDivisor = 16.0;
  } else if ((bracket == 4) && (delayTime > 1000)) {  // Wait for fewer quarter turns
    bracket = 3;
    quarterTurnsToWait = 33; timeDivisor = 8.0;
  }
  Serial.print("Bracket   = "); Serial.println(bracket);
  Serial.print("subTime   = "); Serial.println(subTime);
  Serial.print("delayTime = "); Serial.println(delayTime);
  return subTime;
}

unsigned int getKeypadValue() {
  // Returns an unsigned int of the ASCII value entered, terminated by * or #, from a 3x4 membrane keypad
  // Rev: 06-27-24 by RDP
  String stringValue = "0";
  while (true) {
    char charValue = getKeypadPress();  // 0..9 or * or #
    if ((charValue == '#') || (charValue == '*')) {  // Pressed "Enter"
      return stringValue.toInt(); // - 31;  // Convert ASCII number to an integer 0..
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
