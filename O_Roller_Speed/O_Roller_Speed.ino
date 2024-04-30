// Roller_Speed Speedometer.  Rev: 11/13/22
// 11/15/22: All Legacy and TMCC functions written and tested working.

// This tests various loco functions and performs time and distance recording on the roller bearing setup.

// Among other things, we can watch how fast the roller bearing is turning, and since we know its circumference, we can thus
// calculate and display the estimated speed and distance travelled of a loco (mm, mm/sec, and SMPH.)
// 11/15/22: I compared mm/sec speeds on the roller bearings versus a 5-meter section of straight track, and they were spot on.
// So it's reasonable to assume that the deceleration distances recorded on the roller bearing setup are likely to be accurate.
// VERY IMPORTANT that the white tape on the roller bearing is CLEAN, else we get false triggers which make the loco look as if
// it's moving faster than it actually is.

// We can also use the roller-bearings to determine the stopping distance from Low/Med/High speeds to Crawl speed using various
// momentum delays and speed steps.

// Finally, we've included every Legacy and TMCC function that we'll want to use, and can test those here.  Speed, effects, etc.

// Note that if the Delayed Action table introduces additional delays due to latency, we may find that continuous horn sounds
//   may have gaps.  Seems unlikely if our loop is fast enough, but might explain potential "continuous horn" audio gaps.

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

// 3. ON-TRACK DECELERATION PERCEPTION: Simple Legacy speed controls to send to locos on the layout, to determine reasonable rates
//    of deceleration.  We can try a number of different "delay between speed command" values (i.e. 160ms, 360ms, etc.) and "Legacy
//    speed steps to skip each time" such as -1, -2, -3, -4, etc. to find the largest skip that still produced non-jerky
//    deceleration.  These might be unique to each loco, since i.e. the Shay behaves very differently than the Big Boy.
//    WE CAN ALSO USE THIS TEST TO DETERMINE A "SLOW TO A STOP" PATTERN that looks good on the layout, starting at Crawl speed.
//    For example, set speed to 50% of Crawl speed for 500ms, and then Legacy speed 1 for 500ms, and then stop.
//    This might work globally for any loco, or I may need to have something different for each loco; not sure yet.
//    However if we use a formula, it's more likely to work for any loco (since each has its own Crawl speed.)  Also, we don't
//    care about distance because we'll already have tripped the "stop" sensor and must stop within a couple of inches.

// 4. ROLLER-BEARING DECELERATION CALCULATIONS for populating the Loco Reference table's stopping-distance data.  Prompt operator
//    for locoNum, Legacy Crawl/Low/Med/High speed settings, and momentum time-delay and skip-step values, and run through them
//    automatically and report the slow-down DISTANCES (and TIMES, because why not) from each speed to slow to Crawl, using each of
//    the nine delay/speed-step combinations.  NOTE: It might be that we use the SAME set of 3 delays (i.e. 500ms/250ms/125ms)
//    and/or the SAME set of 3 speed step values (i.e. -2/-4/-6) for multiple locos and/or for each of the three incoming speeds
//    for a given loco.  Won't know until we start testing.
//    Note that we can calculate the TIME required to decelerate from one speed to another (i.e. Medium to Crawl) without even
//    needing a loco on the roller bearings: just multiply the number of speed steps needed by the delay between each step.
//    So it will be interesting to compare the theoretical time with the actual time we record, for decelerating to Crawl speed.
//    We'll be using TimerOne 1ms interrupts to count the number of quarter revs of the roller bearing during the time we are
//    slowing down (and using it's circumference to determine distance.)
//    ALSO TEST from Crawl: set speed to 50% of Crawl speed for 500ms, and then Legacy speed 1 for 500ms, and then stop.

// 5. TESTING LEGACY AND TMCC COMMANDS including 9-byte / 3-"word" command functions.  Every message that we'll want to send to the
//    Legacy base has been implemented here, such as absolute speed, smoke on/off, dialogue, sound effects, etc.

// IMPORTANT PROCEDURE ON HOW TO ESTABLISH EACH LOCO'S SPEED AT VARIOUS SETTINGS (5/23/22):
// Each loco travels a slightly different distance for each rotation of the roller bearing.  Perhaps this is because of the
// geometry of large wheels versus small wheels, and/or because the distance between the roller bearings is slightly different than
// the distance between MTH Scaletrax rails...I don't know.  But even though the circumference of the roller bearing is a constant,
// each loco does have slightly different characteristics which causes it to travel slightly more or less mm/revolution.
// 1. To establish the circumference of the bearing for a given loco.  FIRST CLEAN THE ROLLER BEARING TAPE!
//    a) Put the loco on the roller bearing and use this program to determine the TIME in milliseconds for each rotation at a given
//       Legacy speed setting.  Watch the time value for many rotations and note how consistent it is.  Slow speeds result in
//       longer times per revolution, but the times tend to vary; they are not very consistent.  At high speeds, the time will be
//       more consistent, but it will be a short time and thus less accurate on that basis.  Find an intermediate speed where the
//       rotational speed is around 300 to 400 ms/revolution and the time is relatively consistent over many revolutions.
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

// CONVERTING TO/FROM SMPH and mm/Sec using 1:48 scale: 1 scale mile =    33,528mm (exactly 110 ft.)
//                                                      1  real mile = 1,609,344mm (exactly 5280 ft.)
//   1 SMPH = 33,528mm/hour = 558.8mm/min = 9.3133333mm/sec
//     i.e. 558.798mm/sec = 60 SMPH
// TARGET CRAWL = 2.5smph = 23.3mm/sec

// SPEED AND MOMENTUM NOTES.  Legacy's built-in deceleration (momentum) rates are:
//   Note: Although we must delay 30ms between successive 3-byte Legacy commands, the Legacy hardware itself is apparently not
//   limited by this; hence it is able to issue ever-decreasing speed commands up to every 16ms as seen with Momentum 1.
// Momentum 1: Issues 1 step about every 16ms.  Stops from speed 62 in one second; nearly instant stopping.
// Momentum 2: Issues 1 step every  20ms.  Way too fast to appear realistic.  Also exceeds 1-command-per-30ms max serial rate.
//                                         A train moving at 30smph would take 1.6 sec to stop (3.3 sec from 60smph.)
// Momentum 3: Issues 1 step every  40ms.  Still too fast to appear realistic.
//                                         A train moving at 30smph would take 3.3 sec to stop (6.6 sec from 60smph.)
// Momentum 4: Issues 1 step every  80ms.  Pretty fast deceleration, but realistic for short/lightweight trains.
//                                         A train moving at 30smph would take 6.6 sec to stop (13 sec from 60smph.)
// Momentum 5: Issues 1 step every 160ms.  This is a good rate of deceleration, LOOKS GOOD for most locos such as an F3.
//                                         A train moving at 30smph would take 13 sec to stop (26 sec from 60smph.)
// Momentum 6: Issues 1 step every 320ms.  This is too-slow deceleration for most, but LOOKS GOOD for Shay since it moves so slow.
//                                         A train moving at 30smph would take 26 sec to stop (52 sec from 60smph.)
//                                         Since Shay top speed is about 15smph, it will take 13 sec to stop from high speed.
// Since all of the built-in Legacy momentums use Legacy speed step 1 (actally, -1,) we can simulate similar rates of deceleration
//   by doubling the delay and the speed steps, and so on.  So for Momentum 5, instead of 1 step every 160ms, we could try 2 steps
//   every 320ms or 3 steps every 480ms or 4 steps every 640ms.  This program will allow us to try these on the layout to determine
//   the largest reasonable Legacy steps we can take while still appearing to slow down smoothly.
// See notes from Nov 14, 2022 in Loco Ref tab of Trains OOP Tables Excel spreadsheet.
// In short, we sometimes have to do 3 Legacy speed steps down every x milliseconds in order to keep the steps >= 350ms apart.
// i.e. When WP F3 comes into block #1 at high speed, we must step down 3 speeds each 350ms in order to stop in less than 100".
// The "slowest" we should stop will be 1 step each 500ms, because longer step-delays give us less accurate stopping locations.

// IMPORTANT: Regarding how many trains we can send Legacy commands to at about the same time (i.e. slow multiple trains at once.)
// Since we can only send serial commands to Legacy every 30ms (any faster and Legacy may ignore them,) we'll need to consider how
// many trains we might want to be accellerating/decelerating simultaneously.
// i.e. If we allow for possibly decelerating 10 trains at the same time, we'll only be able to send a command to each train every
// 300ms, which is about 3 commands per second.  Although that will realistically never happen, let's plan on sending speed
// commands to any given loco not more often than every 320ms.  This will be fine for high momentum, but if we want to slow down at
// low momentum, we'll need to slow down multiple Legacy speed steps at a time.
// NOTE: Once we trip a STOP sensor, we'll send commands to stop very quickly, but not instantly from Crawl.

// HORN NOTES 5/1/22: Regular vs Quilling.
// The regular horn toot is shorter than the quilling horn, which means we can make patters that are faster.  This is especially
//   desirable for short sequences such as S-S and S-S-S.
// The regular horn toot, when used in a sequence, can have an unpredictable pitch.  I.e. (at least on SP 1440) the pitch for each
//   toot can vary, which sounds weird when used in any sequence.  On steam 4-4-2 Atlantic E6, it's very very weak, terrible.
// The quilling horn toot lasts a lot longer than the Horn 1 toot, so all patters take more time to complete.  So the quilling horn
//   is less desirable for sequences, especially short ones like S-S and S-S-S.  Sounds terrible because they take so long.
// The quilling horn has the advantage that each toot will always be the same pitch, and there are (at least) three different
//   pitches.  The highest pitch is the same as the regular horn usually sounds, and the other pitches get lower (and cooler.)
// The quilling horn has an advantage in sequences in that we can delay much longer between successive toot commands, so we aren't
//   using as much of the Legacy serial port's bandwidth.

// The quilling horn doesn't sound as good especially on Big Boy discrete toots; okay for RR Xing pattern.
// Once we have multiple trains doing multiple things and we get a lot of serial commands going to the Legacy base, there is a risk
// that succesive commands to keep a long toot going might be held up and a long toot could have gaps and thus sound wrong.  So for
// continuous toots, either Horn 1 or Quilling, we might consider REDUCING the delay between successive toot commands to keep it
// going (to allow for possible delays in getting successive commands to the Legacy base.)  On the other hand, when delaying
// between successive toots, we have a MINIMUM delay, so any additional delay introduced by traffic won't be a problem.
// If we detect gaps in what should be a long toot, we can thus REDUCE the "continuous" delay to allow for some real-time delays.
// FOR DISCRETE HORN 1 TOOTS, 450ms between commands is the minimum delay; 400ms and they occasionally run together.
// FOR CONTINUOUS HORN 1 TOOTS, 175ms between commands is the maximum delay; 200ms and there are occasional gaps.
// FOR DISCRETE QUILLING TOOTS, 950ms between commands is the minimum delay; 900ms unreliable, at least on the Shay.
// FOR CONTINUOUS QUILLING TOOTS, 650ms between commands is the maximum delay; 700ms introduces occasional gaps

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_NUL;  // Global just needs to be defined for use by Train_Functions.cpp and Message.cpp.
char lcdString[LCD_WIDTH + 1] = "RSP 02/18/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// *** FRAM MEMORY STORAGE CLASS ***
#include <FRAM.h>
FRAM* pStorage;

// *** CENTIPEDE SHIFT REGISTER CLASS ***  (Used for accessory relays on LEG)
// THERE IS AN EXTERN REFERENCE FOR THIS VIA TRAIN_FUNCTIONS.H: extern Centipede* pShiftRegister;
// Thus, we do NOT need to pass pShiftRegister to any classes as long as they #include <Train_Functions.h>
// <Centipede.h> is already #included in Train_Functions.h but need to create pointer and instantiate class in calling program.
Centipede* pShiftRegister;  // We only create ONE object for one or two Centipede boards.

// *** BLOCK RESERVATION AND LOOKUP TABLE CLASS (IN FRAM) ***
#include <Block_Reservation.h>
Block_Reservation* pBlockReservation = nullptr;

// *** LOCOMOTIVE REFERENCE LOOKUP TABLE CLASS (IN FRAM) ***
#include <Loco_Reference.h>
Loco_Reference* pLoco;

// *** ROUTE REFERENCE TABLE CLASS (IN FRAM) ***
#include <Route_Reference.h>
Route_Reference* pRoute = nullptr;

// *** TRAIN PROGRESS TABLE CLASS (ON HEAP) ***
#include <Train_Progress.h>
Train_Progress* pTrainProgress = nullptr;

// *** DELAYED ACTION TABLE CLASS (ON HEAP) ***
#include <Delayed_Action.h>
Delayed_Action* pDelayedAction;

// *** ENGINEER CLASS ***
#include <Engineer.h>
Engineer* pEngineer;

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
// stored in cache is the current value.  Also we must temporarily disable interrupts whenever these volatile variables are
// read or updated from outside the ISR -- so do so only when you know that it won't be important if a call to the ISR is skipped.
volatile unsigned int bearingQuarterRevs = 0;  // This will count the number of state changes seen by the IR sensor on the bearing
// Any time we access a (volatile) variable that's used in an ISR from outside the ISR, we must temporarily disable
// (noInterrupts()) and then re-enable (interrupts()) interrupts.  Since we don't want to disable interrupts if it's possible the
// interrupt might be called, we'll create a working variable that will store the value of the volatile variable.  So we can do
// math on it, or use it however we want.
unsigned int tempBearingQuarterRevs = 0;  // We'll grab the interrupt's value and drop in here as a working variable


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
byte devNum                =  0;   // Train, Engine, Acc'y number etc.
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

unsigned long t_startTime = millis();


// Every loco will have its own set of speed steps, but we'll start with a generic set applied to whatever loco we're talking to.
// Possibly, all locos will share the same delay (i.e. 320ms) and speed steps for three different speeds, but for sure every loco
// will travel a different distance between its Low/Medium/High speeds and its Crawl speed at those deceleration rates.
unsigned int  low1delay = 640;  // ms between successively lower speed settings i.e. 320 = 320ms
byte          low1steps = 4;  // Number of Legacy steps to step down after each delay time i.e. 4 speed steps
unsigned int  low1dist = 0;  // Distance to reach Crawl speed from Low speed using above delay and speed steps i.e. 1422mm.
unsigned int  low2delay = 320;
byte          low2steps = 4;
unsigned int  low2dist = 0;
unsigned int  low3delay = 160;
byte          low3steps = 4;
unsigned int  low3dist = 0;

unsigned int  med1delay = 640;
byte          med1steps = 4;
unsigned int  med1dist = 0;
unsigned int  med2delay = 320;
byte          med2steps = 4;
unsigned int  med2dist = 0;
unsigned int  med3delay = 160;
byte          med3steps = 4;
unsigned int  med3dist = 0;

unsigned int high1delay = 640;
byte         high1steps = 4;
unsigned int high1dist = 0;
unsigned int high2delay = 320;
byte         high2steps = 4;
unsigned int high2dist = 0;
unsigned int high3delay = 160;
byte         high3steps = 4;
unsigned int high3dist = 0;

char locoDesc[8 + 1] = "        ";   // Holds 8-char loco description.  8 chars plus \0 newline.
char locoRestrict[5 + 1] = "     ";  // Holds 5-chars of restrictions from Loco Ref.  5 chars plus \0 newline.
byte numBytes = 0;  // Global used to receive from functions via pointer

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
  // FOR THIS TEST PROGRAM, we are going to connect BOTH the PC's Serial port *and* the Mini Thermal Printer to Serial 0.
  // The test bench mini printer requires 19200 baud, but the white one on the layout is 9600 baud.
  // If using the Serial monitor, be sure to set the baud rate to match that of the mini thermal printer being used.
  // 11/10/22: I can't seem to get the thermal printer to *not* print a few inches of garbage as new programs are uploaded.  Once a
  // program is uploaded, pressing the Reset button on the Mega to re-start the program does *not* produce garbage.  It happens
  // during the upload process, so there does not seem to be a way to stop it.
  // THIS ONLY HAPPENS WITH SERIAL 0; i.e. not with Serial 1/2/3.  It must be that Visual Studio is sending "Opening Port, Port
  // Open" data to the serial monitor at some default baud rate?  Even though it's set correctly and I can read it in the window.

  Serial.begin(9600);  // SERIAL0_SPEED.  9600 for White thermal printer; 19200 for Black thermal printer
  // Serial1 instantiated via Display_2004/LCD2004.
  // Serial2 will be connected to the thermal printer.
  Serial3.begin(9600);  // Legacy via MAX-232

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);        // Serial monitor
  Serial2.println(lcdString);       // Mini thermal printer

  // *** INITIALIZE FRAM CLASS AND OBJECT ***
  // We must pass a parm to the constructor (vs begin) because this object has a parent (Hackscribble_Ferro) that needs it.
  pStorage = new FRAM(MB85RS4MT, PIN_IO_FRAM_CS);  // Instantiate the object and assign the global pointer
  pStorage->begin();  // Will crash on its own if there is any problem with the FRAM
  //pStorage->setFRAMRevDate(6, 18, 60);  // Available function for testing only.
  pStorage->checkFRAMRevDate();  // Terminate with error if FRAM rev date does not match date in Train_Consts_Global.h
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
  //pLoco->populate();  // Uses the heap when populating.
  //Serial.println(pLoco->legacyID(1));

  // *** INITIALIZE DELAYED-ACTION TABLE CLASS AND OBJECT ***
  // This uses about 10K of heap memory
  pDelayedAction = new Delayed_Action;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pDelayedAction->begin(pLoco, pTrainProgress); //pLCD2004);

  // *** INITIALIZE TRAIN PROGRESS CLASS AND OBJECT ***  (Heap uses 14,552 bytes)
  // WARNING: TRAIN PROGRESS MUST BE INSTANTIATED *AFTER* BLOCK RESERVATION AND ROUTE REFERENCE.
  pTrainProgress = new Train_Progress;  // C++ quirk: no parens in ctor call if no parms; else thinks it's fn decl'n.
  pTrainProgress->begin(pBlockReservation, pRoute);

  // *** INITIALIZE ENGINEER CLASS AND OBJECT ***
  pEngineer = new Engineer;  // Create the instance of this class.  NO PARENS SINCE NO PARMS!
  pEngineer->begin(pLoco, pTrainProgress, pDelayedAction);

  // *** SET THE TIMER INTERRUPT TO FIRE EVERY 1ms ***
  Timer1.initialize(1000);  // 1000 microseconds = 1 millisecond
  // The timer interrupt will call ISR_Read_IR_Sensor() each interrupt.
  Timer1.attachInterrupt(ISR_Read_IR_Sensor);

  // *** IDENTIFY THE DEVICE AND OTHER PARMS FOR THIS TEST ***  <<<<<<<<--------<<<<<<<<--------<<<<<<<<--------<<<<<<<<--------
  // devNum =  2;  // Shay
  devNum =  4;  // ATSF NW2
  // devNum =  5;  // ATSF 4-4-2 Atlantic E6
  // devNum =  8;  // WP 803-A  (Legacy Train 8, programmed as Engine 80, 83)
  // devNum = 14;  // Big Boy
  // devNum = 40;  // SP 1440

  // devNum = 3;  // SP StationSounds Diner

  testType = 'S';    // 'S' = Speedometer on roller bearings; 'D' = Deceleration time & distance; 'F' = FX; 'P' = Populate FRAM

  // ********** THINK I NEED A STRUCT FOR ALL THIS ****************** <<<<<<<<<<<<<<<<<--------------<<<<<<<<<<<<<<<<<<<---------------

  switch (devNum) {

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
      sprintf(lcdString, "Shay");  pLCD2004->println(lcdString);
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

    case  4:  // ATSF NW2
      devType = 'E';
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
      sprintf(lcdString, "Atlantic");  pLCD2004->println(lcdString);  // Random values here...
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
      sprintf(lcdString, "WP 803-A");  pLCD2004->println(lcdString);
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
      sprintf(lcdString, "Big Boy");  pLCD2004->println(lcdString);
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
      sprintf(lcdString, "SP 1440");  pLCD2004->println(lcdString);
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
      sprintf(lcdString, "devNum Error!"); pLCD2004->println(lcdString);
    }
  }

  // NOTE: The following startup commands can all be given the same t_startTime because the Engineer will ensure they are separated
  // by the minimum required delay between successive Legacy commands (25ms.)  However we can add a slight delay if we want them to
  // be executed in a certain order...
  // Start up the loco
/*
  t_startTime = millis();
  pDelayedAction->populateCommand(t_startTime, devType, devNum, LEGACY_ACTION_STARTUP_FAST, 0, 0);
  // Set loco smoke off
  t_startTime = t_startTime + 50;
  pDelayedAction->populateCommand(t_startTime, devType, devNum, LEGACY_ACTION_SET_SMOKE, 0, 0);  // 0 = Off, 1 = Low, 2 = Med, 3 = High
  // Set loco momentum to zero
  t_startTime = t_startTime + 50;
  pDelayedAction->populateCommand(t_startTime, devType, devNum, LEGACY_ACTION_MOMENTUM_OFF, 0, 0);
  // Set direction forward
  t_startTime = t_startTime + 50;
  pDelayedAction->populateCommand(t_startTime, devType, devNum, LEGACY_ACTION_FORWARD, 0, 0);
  // Set loco speed stopped
  t_startTime = t_startTime + 50;
  pDelayedAction->populateCommand(t_startTime, devType, devNum, LEGACY_ACTION_ABS_SPEED, 0, 0);
  // Give a short toot to say Hello
  t_startTime = t_startTime + 50;
  pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_SHORT_TOOT);
  t_startTime = t_startTime + 2000;  // Dialogue can't begin until engine has finished starting up
  // Say "Hello World!"  IMPORTANT: Horn can't overlap startup
//  pDelayedAction->populateCommand(t_startTime, devType, devNum, LEGACY_DIALOGUE, LEGACY_DIALOGUE_T2E_STARTUP, 0);
//  t_startTime = t_startTime + 1000;  // We'll give everything an additional second
  while (millis() < t_startTime) {
    //pEngineer->executeDelayedActionCommand();  // And send it to the Legacy base as quickly as possible
    //pEngineer->getDelayedActionCommand();
    //pEngineer->sendCommandToTrain();
  }
*/
}

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

  switch (testType) {  // T = Time & Distance Decel, F = FX, S = Speedometer, D = Deceleration, P = Populate

    case 'P':  // Loco ref populate and display data from the Loco Reference table (tested and working 6/6/22)
    {
      // pStorage->setFRAMRevDate(4, 12, 62);  // Tested and works
      pLoco->populate();  // Uses the heap when populating.
      Serial.println(pLoco->devType(1));

      // Display everything for any loco 1..50
      // To receive data from functions with more than one return value (which will be void functions), I need to use the & address
      // prefix on the fields I want to get filled (returned.)
      // HOWEVER if it's a string, like locoDesc (8 char + \0) I just name the string (no & needed.)
      // It's very important to specify the number of chars to return NOT INCLUDING THE TERMINATING \0 NULL CHARACTER.

      byte devNum = 50;

      Serial.println(pLoco->locoNum(devNum));
      pLoco->alphaDesc(devNum, locoDesc, 8);
      Serial.println(locoDesc);
      Serial.println(pLoco->devType(devNum));
      Serial.println(pLoco->steamOrDiesel(devNum));
      Serial.println(pLoco->passOrFreight(devNum));
      pLoco->restrictions(devNum, locoRestrict, 5);
      Serial.println(locoRestrict);
      Serial.println(pLoco->length(devNum));
      Serial.println(pLoco->opCarLocoNum(devNum));
      Serial.println(pLoco->crawlSpeed(devNum));
      Serial.println(pLoco->crawlMmPerSec(devNum));
      Serial.println(pLoco->lowSpeed(devNum));
      Serial.println(pLoco->lowMmPerSec(devNum));
      Serial.println(pLoco->lowSpeedSteps(devNum));
      Serial.println(pLoco->lowMsStepDelay(devNum));
      Serial.println(pLoco->lowMmToCrawl(devNum));
      Serial.println(pLoco->medSpeed(devNum));
      Serial.println(pLoco->medMmPerSec(devNum));
      Serial.println(pLoco->medSpeedSteps(devNum));
      Serial.println(pLoco->medMsStepDelay(devNum));
      Serial.println(pLoco->medMmToCrawl(devNum));
      Serial.println(pLoco->highSpeed(devNum));
      Serial.println(pLoco->highMmPerSec(devNum));
      Serial.println(pLoco->highSpeedSteps(devNum));
      Serial.println(pLoco->highMsStepDelay(devNum));
      Serial.println(pLoco->highMmToCrawl(devNum));
      while (true) {}
    }
    case 'F':  // FX 
    {

//      blowHornReverseDiesel(devType, devNum);  // Bad on steam.  GOOD TIMING, BUT PITCH CAN CHANGE FOR EACH TOOT.  STILL, USE THIS VERSION for Diesel.
//      delay(3000);
//      blowHornReverseSteam(devType, devNum);  // Terrible on steam, no way to get it not weak even making the toots longer.
//      delay(3000);
//      blowHornReverseQuilling(devType, devNum);  // TOOTS ARE TOO LONG FOR SHORT SEQUENCES on diesel, but sounds okay on steam.
//      delay(3000);
//      blowHornCrossing(devType, devNum);  // DON'T USE.  GOOD TIMING, BUT PITCH CAN CHANGE FOR EACH TOOT
//      delay(3000);
//      blowHornCrossingQuilling(devType, devNum);  // Good on steam & diesel.  LONGER SEQUENCE THAN REGULAR TOOT VERSION, BUT SOUNDS GREAT!
//      delay(3000);
      /*

      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);
      sprintf(lcdString, "Smoke ON"); pLCD2004->println(lcdString);
      legacySetSmoke(devType, devNum, 3);  // 0 = Off, 1 = Low, 2 = Med, 3 = High
      delay(5000);
      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);
      sprintf(lcdString, "Smoke OFF"); pLCD2004->println(lcdString);
      legacySetSmoke(devType, devNum, 0);  // 0 = Off, 1 = Low, 2 = Med, 3 = High
      delay(3000);
    

      sprintf(lcdString, "Bell ON"); pLCD2004->println(lcdString);
      legacyBell(devType, devNum, true);
      delay(5000);
      sprintf(lcdString, "Bell OFF"); pLCD2004->println(lcdString);
      legacyBell(devType, devNum, false);
      delay(3000);

      sprintf(lcdString, "Front coupler"); pLCD2004->println(lcdString);
      legacyOpenCoupler(devType, devNum, 'F');
      delay(3000);
      sprintf(lcdString, "Rear coupler"); pLCD2004->println(lcdString);
      legacyOpenCoupler(devType, devNum, 'R');
      delay(3000);

      sprintf(lcdString, "Water injector FX"); pLCD2004->println(lcdString);  // Steam locos only ****** NEED TO TEST ON STEAM LOCO *******
      legacyWaterInjector(devType, devNum);
      delay(3000);
      legacyWaterInjector(devType, devNum);
      delay(3000);
      legacyStartUpSequence(devType, devNum, 'F');  // Fast or Slow startup
      delay(3000);

      // Test some Legacy dialogue.
      // Valid values are 0x06..0x14, 0x22..0x23, 0x30, 0x3D..0x41, 0x68..0x76
      // Includes Tower/Engineer chat and Legacy Stationsounds announcements.
      // As of 6/22 I don't have any Legacy Stationsounds diners (they're TMCC) so 0x68..0x76 are unused.

      for (byte i = 0x06; i < 0x15; i++) {
        sprintf(lcdString, "Legacy %i", i); pLCD2004->println(lcdString);
        legacyDialogue(devType, devNum, i);
        delay(5000);
      }

      //  legacyShutDownSequence(devType, devNum, 'S');  // Fast or Slow shutdown
      t_startTime = millis();
      devType = 'E';
      devNum = 5;
      pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_SHORT_TOOT);
      pDelayedAction->populateCommand      (t_startTime, devType, devNum, LEGACY_DIALOGUE, LEGACY_DIALOGUE_E2T_DEPARTURE_YES, 0);
      devType = DEV_TYPE_TMCC_ENGINE;  // 'N' = TMCC eNgine
      devNum = 3;  // SP StationSounds Diner
      pDelayedAction->populateCommand(t_startTime, devType, devNum, TMCC_DIALOGUE, TMCC_DIALOGUE_CONDUCTOR_TICKETS_DINER, 0);
      delay(500);
      pEngineer->getDelayedActionCommand();
      pEngineer->sendCommandToTrain();
      delay(2000);


      // TEST TMCC STATIONSOUNDS DINER both while STOPPED and while MOVING
      // TMCC_DIALOGUE_STATION_ARRIVAL         = 1;  // When STOPPED: PA announcement i.e. "The Daylight is now arriving."
      // TMCC_DIALOGUE_STATION_DEPARTURE       = 2;  // When STOPPED: PA announcement i.e. "The Daylight is now departing."
      // TMCC_DIALOGUE_CONDUCTOR_ARRIVAL       = 3;  // When STOPPED: Conductor says i.e. "Watch your step."
      // TMCC_DIALOGUE_CONDUCTOR_DEPARTURE     = 4;  // When STOPPED: Conductor says i.e. "Watch your step." then "All aboard!"
      // TMCC_DIALOGUE_CONDUCTOR_TICKETS_DINER = 5;  // When MOVING: Conductor says "Welcome aboard/Tickets please/1st seating."
      // TMCC_DIALOGUE_CONDUCTOR_STOPPING      = 6;  // When MOVING: Conductor says "Next stop coming up."
      for (byte i = 1; i < 7; i++) {
        t_startTime = millis();
        sprintf(lcdString, "TMCC %i", i); pLCD2004->println(lcdString);
        pDelayedAction->populateCommand(t_startTime, devType, devNum, TMCC_DIALOGUE, i, 0);
        pEngineer->getDelayedActionCommand();
        pEngineer->sendCommandToTrain();
        delay(2000);
      }
      while (true) {}

      for (byte i = 0; i <= 7; i++) {
        sprintf(lcdString, "Run level %i", i); pLCD2004->println(lcdString);
        legacyDieselRunLevel(devType, devNum, i);
        delay(1000);
      }
      delay(3000);
      sprintf(lcdString, "Run level 0"); pLCD2004->println(lcdString);
      legacyDieselRunLevel(devType, devNum, 0);

      delay(5000);
      for (int i = 0; i <=31; i++) {  // Engine labor can be 0..31
        sprintf(lcdString, "Engine labor %i", i); pLCD2004->println(lcdString);
        legacyEngineLabor(devType, devNum, i);
        delay(250);
      }
      delay(2000);
      sprintf(lcdString, "Engine labor 0"); pLCD2004->println(lcdString);
      legacyEngineLabor(devType, devNum, 0);
      delay(5000);

      // Can't test the following Cylinder Cock Clear on/of, Long let-off, or Auger sounds until I get a steam loco *********************************************
      sprintf(lcdString, "Cock Clear ON"); pLCD2004->println(lcdString);
      legacyCylinderCockClear(devType, devNum, true);
      delay(5000);
      sprintf(lcdString, "Cock Clear OFF"); pLCD2004->println(lcdString);
      legacyCylinderCockClear(devType, devNum, false);
      delay(3000);
      sprintf(lcdString, "Long let-off"); pLCD2004->println(lcdString);
      legacyLongLetOff(devType, devNum);
      delay(3000);
//      sprintf(lcdString, "Auger sound"); pLCD2004->println(lcdString);
//      legacyAugerSound(devType, devNum);                            // LEGACY interprets as "shut down".  F7 and "Shut Down 3" ????????????????????????
//      delay(3000);
      sprintf(lcdString, "Brake release"); pLCD2004->println(lcdString);
      legacyBrakeRelease(devType, devNum);
      delay(3000);
      sprintf(lcdString, "Refuel sound"); pLCD2004->println(lcdString);
      legacyRefuelSound(devType, devNum);
      delay(3000);

      for (int i = 1; i <= 9; i++) {
        legacyBrakeSqueal(devType, devNum);  // Only works if moving
        delay(1000);
      }
      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);


      sprintf(lcdString, "Master volume..."); pLCD2004->println(lcdString);
      // There are 10 possible volumes (9 excluding "off") i.e. 0..9
      // There is no way to go directly to a volume level so you need to just repeat up/down.
      // Assuming we start at volume 0 (off)...we'll go through the 9 steps you can hear...
      for (int i = 1; i <= 9; i++) {
        legacyMasterVolume(devType, devNum, 'U'); delay(LEGACY_CMD_DELAY);
        delay(1000);
      }
      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);
      // Since we're already at level 8, the first down will put us at level 8, etc.
      for (int i = 8; i >= 0; i--) {
        legacyMasterVolume(devType, devNum, 'D'); delay(LEGACY_CMD_DELAY);
        delay(1000);
      }
      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);

      sprintf(lcdString, "Quilling horn"); pLCD2004->println(lcdString);
      for (int i = 0; i <= 15; i++) {
        sprintf(lcdString, "Intensity %i", i); pLCD2004->println(lcdString);
        legacyQuillingHorn(devType, devNum, i);  // Intensity can be 0..15
        delay(1000);
      }
      sprintf(lcdString, "Normal horn"); pLCD2004->println(lcdString);
      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);
      delay(LEGACY_CMD_DELAY);

      //Quilling horn is LONG.  Horn toot is very short!
      //0-3 same as regular but longer
      //4-12 same deeper/louder
      //13-15 same very deep

      sprintf(lcdString, "Forward speed 20"); pLCD2004->println(lcdString);
      legacyDirection(devType, devNum, 'F');
      legacyAbsoluteSpeed(devType, devNum, 20);
      delay(3000);
      sprintf(lcdString, "Stop"); pLCD2004->println(lcdString);
      legacyAbsoluteSpeed(devType, devNum, 0);
      delay(3000);
      sprintf(lcdString, "Reverse speed 20"); pLCD2004->println(lcdString);
      legacyDirection(devType, devNum, 'R');
      legacyAbsoluteSpeed(devType, devNum, 20);
      delay(3000);
      sprintf(lcdString, "Stop"); pLCD2004->println(lcdString);
      legacyAbsoluteSpeed(devType, devNum, 0);
      delay(1000);
      sprintf(lcdString, "Forward speed 20"); pLCD2004->println(lcdString);
      legacyDirection(devType, devNum, 'F');
      legacyAbsoluteSpeed(devType, devNum, 20);
      delay(3000);
      sprintf(lcdString, "EMERGENCY STOP!"); pLCD2004->println(lcdString);
      legacyEmergencyStop();
*/
      while (true) {}
    }
    case 'S':  // Speedometer on roller bearings.  INCLUDE ALL CASE BLOCKS IN CURLY BRACES TO AVOID HANGING.
    {
      float revTime = timePerRev();  // milliseconds per 40.7332mm (bearingCircumference).  Can take several seconds.
      // First, display the current processor time so we can easily see when display changes...
      sprintf(lcdString, "Refreshed: %8ld", millis()); pLCD2004->println(lcdString);
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
    pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_CROSSING);
    t_startTime = t_startTime + 12000;
    pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_STOPPED);  // S
    t_startTime = t_startTime + 4000;
    pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_APPROACHING);  // L
    t_startTime = t_startTime + 6000;
    pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_DEPARTING);  // L-L
    t_startTime = t_startTime + 8000;
    pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_BACKING);  // S-S-S
    t_startTime = millis() + 1000;
    pDelayedAction->populateCommand(t_startTime, devType, devNum,LEGACY_ACTION_STARTUP_SLOW, 0, 0);
    t_startTime = t_startTime + 8000;
    pDelayedAction->populateWhistleHorn(t_startTime, devType, devNum, LEGACY_PATTERN_CROSSING);
    t_startTime = t_startTime + 2000;
    pDelayedAction->populateCommand(t_startTime, devType, devNum,LEGACY_ACTION_FORWARD, 0, 0);
    t_startTime = t_startTime + 100;
    pDelayedAction->populateSpeedChange(t_startTime, devType, devNum, t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed, true);
    t_startTime = t_startTime + 9000;
    t_startSpeed = t_targetSpeed;
    t_targetSpeed = 0;
*/

    // Wait for operator to press the green button...
    sprintf(lcdString, "Press to Start..."); pLCD2004->println(lcdString); Serial.println(lcdString);
    while (digitalRead(PIN_PUSHBUTTON) == HIGH) {}   // Just wait...
    // Toot the horn to ack button push was seen
    t_startTime = millis();
    pDelayedAction->populateLocoWhistleHorn(t_startTime, devNum, LEGACY_PATTERN_SHORT_TOOT);

    // ACCELERATE FROM A STOP
    t_startTime = millis();
    unsigned int t_startSpeed = 0;
    byte t_speedStep = 2;              // Seems like 2 steps at a time is great.  1 is perfect, 4 is a big jerky.
    unsigned long t_stepDelay = 500;  // 600;   // For the Big Boy, 2 steps each 400ms is very smooth, seems right unless very short siding.
    unsigned int t_targetSpeed = 63;  // Legacy speed 0..199
    pDelayedAction->populateLocoSpeedChange(t_startTime, devNum, t_speedStep, t_stepDelay, t_targetSpeed);

    // How long will it be until all of these speed commands have been flushed from D.A. into the Legacy Command Buffer?
    unsigned long speedChangeTime = pDelayedAction->speedChangeTime(t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed);
    unsigned long endTime = t_startTime + speedChangeTime;
//    sprintf(lcdString, "SCT %12lu", speedChangeTime); pLCD2004->println(lcdString);
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
    pDelayedAction->populateLocoSpeedChange(t_startTime, devNum, t_speedStep, t_stepDelay, t_targetSpeed);
    // If we don't see a "DA 1 not empty!" message on the LCD, it means we've waited long enough, as hoped.
    // Otherwise, we'd better find the error in the speedChangeTime) calculation above.

    // Toot the horn to ack target speed reached
    t_startTime = millis();
    pDelayedAction->populateLocoWhistleHorn(t_startTime, devNum, LEGACY_PATTERN_SHORT_TOOT);
    while (millis() < (t_startTime + 1000)) {
      pEngineer->executeConductorCommand();
    }

    sprintf(lcdString, "Press to slow."); pLCD2004->println(lcdString);
    while (digitalRead(PIN_PUSHBUTTON) == HIGH) {}  // Wait for operator to say "start slowing down"
    t_startTime = millis();
    t_startSpeed = t_targetSpeed;  // From above
    t_speedStep = 2;
    t_stepDelay = 500;  // ms
    t_targetSpeed = 20;
    pDelayedAction->populateLocoSpeedChange(t_startTime, devNum, t_speedStep, t_stepDelay, t_targetSpeed);
    speedChangeTime = pDelayedAction->speedChangeTime(t_startSpeed, t_speedStep, t_stepDelay, t_targetSpeed);
    t_startTime = t_startTime + speedChangeTime;
    // We should be at Crawl exactly now.
    // We'll stop immediately so that we can measure distance from t_startSpeed to Crawl/Stop.
    pDelayedAction->populateLocoCommand(t_startTime, devNum, LEGACY_ACTION_STOP_IMMED, 0, 0);
/*
    while (millis() <= t_startTime + 50) {  // Should be essentially instant.
      pEngineer->getDelayedActionCommand();
      pEngineer->sendCommandToTrain();
    }

    t_startSpeed = t_targetSpeed;
    pDelayedAction->populateSlowToStop(t_startTime, devType, devNum, t_startSpeed, true);  // Should ALWAYS take 2000ms
*/

    while (millis() <= t_startTime + 3000) {
      pEngineer->executeConductorCommand();
    }

    /*
    sprintf(lcdString, "Press to stop."); pLCD2004->println(lcdString);
    while (digitalRead(PIN_PUSHBUTTON) == HIGH) {}  // Wait for operator to say "stop now."
    t_startTime = millis();
    // Assume we are moving at previous t_targetSpeed, although we should look it up via a pEngineer-> function...
    t_startSpeed = t_targetSpeed;
    pDelayedAction->populateSlowToStop(t_startTime, devType, devNum, t_startSpeed, true);  // Should ALWAYS take 2000ms
    while (millis() <= t_startTime + 3000) {
      pEngineer->getDelayedActionCommand();
      pEngineer->sendCommandToTrain();
    }
    // We should be at crawl, or close to it.  Normally wait until we trip sensor but for testing, slow to stop.
    t_startTime = millis();
    // Assume we are moving at previous t_targetSpeed, although we should look it up via a pEngineer-> function...
    t_startSpeed = t_targetSpeed;
    pDelayedAction->populateSlowToStop(t_startTime, devType, devNum, t_startSpeed, true);  // Should ALWAYS take 2000ms
    while (millis() <= t_startTime + 3000) {
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
    endTime = t_startTime + speedChangeTime;

//sprintf(lcdString, "Time %12lu", millis()); pLCD2004->println(lcdString);
//sprintf(lcdString, "SCT %12lu", speedChangeTime); pLCD2004->println(lcdString);
//sprintf(lcdString, "ET %12lu", endTime); pLCD2004->println(lcdString);

//    while (millis() < endTime) {  // This *should* run exactly as long as it takes to send all commands
//      pEngineer->getDelayedActionCommand();
//      pEngineer->sendCommandToTrain();
//    }
    // Should be stopped now; toot horn!
    t_startTime = millis();
    pDelayedAction->populateLocoWhistleHorn(t_startTime, devNum, LEGACY_PATTERN_SHORT_TOOT);
    t_startTime = t_startTime + 1000;  // We'll wait a second for the horn to toot
    while (millis() < t_startTime) {
      pEngineer->executeConductorCommand();
    }

    while (true) {}


    //pDelayedAction->populateSlowToStop(t_startTime, devType, devNum, t_targetSpeed, false);
    //sprintf(lcdString, "Dump 0:"); pLCD2004->println(lcdString); Serial.println(lcdString);
    //pDelayedAction->dumpTable();

      // pDelayedAction->populateCommand(millis() + 3000, 'E', 40, LEGACY_ACTION_STARTUP_FAST, 0, 0);
      // pDelayedAction->populateCommand(millis() + 5000, 'E', 40, LEGACY_ACTION_ABS_SPEED, 10, 0);
 
      //populateSpeedChange(const unsigned long t_startTime, const char t_devType, const byte t_devNum, 
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

//      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);
      
      // **********************************
      // ***** Bring loco up to speed *****
      // **********************************
      byte          entrySpeed =  75;  // legacyHigh;
      byte      legacyStepDown =   5;  // How many Legacy speed steps to reduce each time we lower the speed
                   legacyCrawl =  15;  // Already been set above, but here we can monkey with it
      unsigned int legacyDelay = 1250;  // How many ms to pause between each successive reduced speed command

      byte currentSpeed = 0;
      for (byte i = 1; i <= entrySpeed; i++) {
        sprintf(lcdString, "Speed: %i", (int)i); pLCD2004->println(lcdString);
//        legacyAbsoluteSpeed(devType, devNum, i);
        currentSpeed = i;
        delay(LEGACY_CMD_DELAY);  // Really going to come up to speed quickly!
      }
      // Wait for loco to come up to speed...Meanwhile send data to printer...
      sprintf(lcdString, "Loco : %i",      devNum); pLCD2004->println(lcdString); Serial2.println(lcdString);
      sprintf(lcdString, "Speed: %i",     entrySpeed); pLCD2004->println(lcdString); Serial2.println(lcdString);
      sprintf(lcdString, "Step : %i", legacyStepDown); pLCD2004->println(lcdString); Serial2.println(lcdString);
      sprintf(lcdString, "Crawl: %i",    legacyCrawl); pLCD2004->println(lcdString); Serial2.println(lcdString);
      sprintf(lcdString, "Delay: %i",    legacyDelay); pLCD2004->println(lcdString); Serial2.println(lcdString);
      delay(3000);

      // Toot horn to signal we're starting to slow down...
//      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);

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
        sprintf(lcdString, "Speed: %i", (int)i); pLCD2004->println(lcdString);
//        legacyAbsoluteSpeed(devType, devNum, i);
        currentSpeed = i;
        delay(legacyDelay);
      }
      // We may not quite yet be at crawl, but if not, we will be less than legacyStepDown steps from it, so go to crawl now.
//      legacyAbsoluteSpeed(devType, devNum, legacyCrawl);  // Go ahead and set/confirm crawl speed

      // Okay we've reached Crawl speed!  Snag the value of how many quarter revs we've counted...
      noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
      tempBearingQuarterRevs = bearingQuarterRevs;  // Grab our ISR "volatile" value
      interrupts();
      endTime = millis();

      // Toot horn to signal we've reached Crawl speed...
//      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);

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
//      crawlToStop(devType, devNum, legacyCrawl);
      delay(LEGACY_CMD_DELAY);  // Wait for last speed command to complete
//      legacyBlowHornToot(devType, devNum); delay(LEGACY_CMD_DELAY);

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
      sprintf(lcdString, "QRevs: %i", tempBearingQuarterRevs); pLCD2004->println(lcdString); Serial2.println(lcdString);
      sprintf(lcdString, "Dist : %imm", distanceToCrawl); pLCD2004->println(lcdString); Serial2.println(lcdString);
      sprintf(lcdString, "Time : %lums", timeToCrawl); pLCD2004->println(lcdString); Serial2.println(lcdString);
      Serial2.println(); Serial2.println();
// 1573mm in 12100ms
// 1604mm in 12100ms
// 1471mm in 12100ms
// Speed 75 to speed 15 step 5 @ 1 second
      // Just be done for now...
      sprintf(lcdString, "Done."); pLCD2004->println(lcdString);
      while (true);
      break;
    }
    default:
    {
      sprintf(lcdString, "testType Error!"); pLCD2004->println(lcdString);
      while (true);
    }
  }
}

// *****************************************************************************************
// ************************ F U N C T I O N   D E F I N I T I O N S ************************
// *****************************************************************************************

void ISR_Read_IR_Sensor() {  // Rev: 6/7/22.  Started with version from Roller_Distance.ino 5/22
  // Count up the total number of bearing quarter revs.  Must be called every 1ms by TimerOne interrupt.
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

  // Wait until our bounce delay has transpired after reading a state change, before looking for the next one.
  // There is a potential problem with the first time you enter this block - it depends on the initial state of the sensor, versus
  // out variable oldSensorReading arbitrary initial value - which could result in an error of one 1/4 rev count.  However, as long
  // as we allow the interrupt to start counting and then reset bearingQuarterRevs when we are ready to pay attention to the count,
  // this will not be a problem.
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
  // This function just sits and watches the roller bearing run (at a steady speed, presumably) for a specified number of
  // revolutions, and then returns the average time per rev.
  // Watch the roller bearing IR sensor and return the time for one rev, in ms (averaged based on 4 revs.)
  // Before we begin, wait for two stable reads (separated by DEBOUNCE_DELAY ms) and *then* wait for a state change
  unsigned long const DEBOUNCE_DELAY = 4;  // Debounce time in ms when used to compare with millis()
  //byte bounceCountdown = 0;    // Wait DEBOUNCE_DELAY ms before trying to read sensor again

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
  for (int quarterTurn = 1; quarterTurn < 65; quarterTurn++) {
    // Wait for bounce from last read to stabilize
    delay(DEBOUNCE_DELAY);
    // Wait for the sensor to change again
    while (digitalRead(PIN_RPM_SENSOR) == newSensorReading) {}
    // Okay we just saw a transition
    newSensorReading = !newSensorReading;
  }
  // Completed our revs!  Stop the timer!
  float subTime = float(float((millis() - startTime)) / 16.0);
  return subTime;
}
