// Roller_Speed Speedometer and Deceleration Distance calculator.  Rev: 07/07/24
// 
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
// roller bearings at various Legacy speeds to determine the SMPH and mm/sec, and decide on values for Crawl, Low, Medium, and High
// speeds.  Record the actual speeds in mm/sec in the Loco Reference table in FRAM.

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
// matches the stopping distance calculated on roller bearings.

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
// KEYPAD PINS: KEYPAD_ROWS are A3..A6 (57..60), KEYPAD_COLS are A0..A2 (54..56)
// Note: Analog pins A0, A1, etc. are digital pins 54, 55, etc.
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
//   SHAY REAL WORLD TEST RESULTS on 5 METER TRACK (5/2/22):
//   Speed  20: 209.20 sec.
//   Speed  40:  99.04 sec.
//   Speed  60:  58.07 sec. (reverse: 57.70 sec.)
//   Speed  80   38.35 sec. (reverse: 38.31 sec.)
//   Speed 100:  27.05 sec. (reverse: 27.11 sec.)

// ATSF 1484 E6 (Eng 5, Trn 7):  Fairly slow small steam loco
//   NOTE: The E6 steam loco had a top speed of 58mph to 75mph depending on train size.
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
//   BIG BOY REAL WORLD TEST RESULTS on 5 METER TRACK (5/2/22):
//   Speed  20: 105.64 sec.
//   Speed  40:  48.85 sec.  Also: 49.08, 49.09, 49.03, 49.00 (fwd), 48.46, 48.65 (rev)
//   Speed  60:  28.84 sec.
//   Speed  80:  18.96 sec.

// SP 1440 (Eng 40): Final data as of 5/23/22.
// NOTE: This is a switcher, so speeds were kept low.  The Alco S-2 switcher had a top speed of 60mph.
//   Legacy   smph    speed     mm/sec
//      15      8     CRAWL       73  But as low as 55mm/sec
//      44     12       LOW      112  Not very accurate, sometimes slower
//      55     16       MED      151
//      75     25      FAST      236
//   SP 1440 REAL WORLD TEST RESULTS: SP 1440 on 5 METER TRACK (5/2/22):
//   Speed  20: 111.33 sec.
//   Speed  30:  70.74 sec.
//   Speed  40:  50.28 sec.
//   Speed  60:  29.26 sec.
//   Speed  80:  19.05 sec.
//   Speed  84:  17.70 sec.
//   Speed 100:  13.48 sec.

// WP F3 803-A (Eng 83, Trn 8): High-speed passenger train.
// NOTE: F3/F7 had a top speed of between 65mph and 102mph depending on gearing.
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
  // We'll be using TimerOne 1ms interrupts to count the number of quarter revs of the roller bearing during our decel tests.
  // In order to account for "bounce" on the roller bearing, we'll only look each 4ms.  See function for explanation.
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
      // This function helps us determine Legacy speed settings 1..199 (and SMPH) for our four standard speeds C/L/M/H.
      // I.e. if we want to know the Legacy speed setting for 10 SMPH, play with the throttle until the display shows 10 SMPH.
      // Then record that Legacy speed setting and the displayed speed in mm/sec into the Loco_Reference.cpp table and in Excel.
      // In order for the Speedometer function (and the Deceleration function) to provide the most accurate results, we need to
      // know the circumference of the roller bearing when used with each specific loco.  This varies from around 40.5mm to 41.7mm.
      // So the user should calculate the roller bearing diameter for each loco based on running the loco on the layout for 5
      // meters at an intermediate speed, and repeat using this speedometer function until a good roller bearing diameter value is
      // established.
      // So each time we run this function, we'll prompt the user to enter the locoNum and then look up the pre-determined value to
      // use.  If the user selects loco 0, we'll default to the average value of 41.1 which will likely be within an error margin
      // of +/- 1.5% of actual.
      sprintf(lcdString, "ENTER locoNum: "); pLCD2004->println(lcdString); Serial.println(lcdString);
      locoNum = getKeypadValue();
      sprintf(lcdString, "%2i", locoNum); pLCD2004->printRowCol(4, 16, lcdString); Serial.println(lcdString);
      sprintf(lcdString, "Loco: %2i", locoNum); Serial2.println(lcdString);
      if ((locoNum < 1) || (locoNum > TOTAL_TRAINS)) {
        sprintf(lcdString, "Out of range."); pLCD2004->println(lcdString);  Serial.println(lcdString);endWithFlashingLED(4);
      }
      bearingCircumference = getBearingCircumference(locoNum);
      if (locoNum == 4) {
        bearingCircumference =  40.5279;  // Accurate for ATSF NW2 as of 6/27/24
      }

      float revTime = timePerRev();  // milliseconds/bearingCircumference.  Can take several seconds at very low speeds.
      // First, display the current processor time so we can easily see when display changes...
      sprintf(lcdString, "Refreshed: %8ld", millis()); pLCD2004->println(lcdString); Serial.println(lcdString);
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
      break;
    }

    case 'D':  // Deceleration test
    {
      // We'll be using TimerOne 1ms interrupts to count the number of quarter revs of the roller bearing during our decel tests.
      // NOTE regarding getKeypadValue(): If user simply presses # to default to a value, it gets returned as 0.  Thus we can't
      // have a prompt which accepts 0 as a valid return value unless we don't want to allow # as "choose default."
      // First ask if they want debug info on the Serial monitor (it's a lot.)  The potential problem is that it takes a lot of
      // time to serial print/display the debug info, which may throw off the timing of the test.  So generally don't use debug.
      sprintf(lcdString, "Debug? 1=Y [2]=N"); pLCD2004->println(lcdString); Serial.println(lcdString);
      byte d = getKeypadValue();
      debugOn = false;
      if (d == 1) {
        debugOn = true;
        sprintf(lcdString, "Yes     "); pLCD2004->printRowCol(4, 10, lcdString); Serial.println(lcdString);
      } else {  // The either entered 2# or simply pressed # to accept default of "N"
        sprintf(lcdString, "No      "); pLCD2004->printRowCol(4, 10, lcdString); Serial.println(lcdString);
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



// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************
// ***************************************************************************************************************************************************


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

float getBearingCircumference(byte t_locoNum) {
  switch (t_locoNum) {
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
      bearingCircumference =  41.2814;  // SHAY mm/rev as of 2/23/22
      break;
    }

    case 4:  // ATSF NW2
    {
      // ATSF NW2 correct as of 06/27/24.
      // Measured NW2 on 5m track @ Legacy 101 = 183mm/sec
      // Measured NW2 on 5m track @ Legacy 140 = 312.5mm/sec
      //   Legacy   smph    speed     mm/sec
      //      21    2.5     CRAWL       23
      //      65     10       LOW       93
      //     102     20       MED      186
      //     117     25      FAST      233
      sprintf(lcdString, "ATSF NW-2");  pLCD2004->println(lcdString); Serial.println(lcdString);
      bearingCircumference =  40.5279;  // Accurate for ATSF NW2 as of 6/27/24
      break;
    }

    case  5:  // ATSF 1484 4-4-2 E6 Atlantic Eng 5, Trn 7
    {
      // SF 1484 E6 (Eng 5, Trn 7): Final data as of 6/18/22.
      // Measured SF 4-4-2 on 5m track @ Legacy 70 AVG  23.03 sec = 217.11mm/sec.
      // Measured SF 4-4-2 on bearings @ Legacy 70 AVG  .1910 sec/rev @ 217.11mm/sec = 41.4680mm/rev
      //   Legacy   smph    speed     mm/sec
      //      12    3.4     CRAWL       33
      //      25    6.3      LOW        61
      //      50   14.5      MED       136
      //      80   28.4     FAST       268
      sprintf(lcdString, "Atlantic");  pLCD2004->println(lcdString); Serial.println(lcdString);  // Random values here...
      bearingCircumference = 41.4680;
      break;
    }

    case 8:  // WP 803-A = TRAIN 8 (not Engine 8)
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
      bearingCircumference =  41.7171;  // WP 803-A mm/rev as of 2/23/22
      break;
    }

    case 14:  // Big Boy Eng 14
    {
      // BIG BOY (Eng 14): Final data as of 5/23/22.
      // Huge variation in mm/sec at low speeds; less as speeds increased past Legacy 40.
      // Measured  BIG BOY on 5m track @ Legacy 50 AVG  36.87 sec = 135.61mm/sec.
      // Measured  BIG BOY on bearings @ Legacy 50 AVG .30698 sec/rev @ 135.61mm/sec = 41.6296mm/rev
      //   Legacy   smph    speed     mm/sec
      //      12      6     CRAWL       53
      //      26     10       LOW       95
      //      74     26       MED      239
      //     100     41      FAST      384
      sprintf(lcdString, "Big Boy");  pLCD2004->println(lcdString); Serial.println(lcdString);
      bearingCircumference =  41.6296;  // Big Boy mm/rev as of 2/23/22
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
      bearingCircumference =  40.8706;  // SP 1440 mm/rev as of 2/23/22
      break;
    }

    default:
    {
      sprintf(lcdString, "DEFAULT LOCO"); pLCD2004->println(lcdString); Serial.println(lcdString);
      bearingCircumference =  41.1;  // This is a good average value to use in the absence of actual data
      break;
    }

  }
  return bearingCircumference;
}

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
