// Roller_Distance Distance Calculator.  Rev: 05/02/22
// This is my new code for blowing horn patterns, and starting and stopping the train using various steps/delays to Legacy.
// Also allows you to specify locoNum, target speed, and time-at-speed, and displays/prints the number of mm it thinks were travelled.
// Compare real-world times recorded on the 5-meter stretch on the layout, at a certain speed, and duplicate that speed and time
// here, and see how close the estimated distance travelled comes to the 5 meters we think it should be.  Data in comments below.

// 5/1/22: Cleaned up code to test regular and quilling horn for grade crossing, and forward/backing toots.  Also has code to test
//         TMCC, but there is no advantage as horn is exactly the same as Legacy horn.
// IMPORTANT NOTES: We will use this utility to calculate speed and distance-to-decelerate for a given loco, using various starting
// speeds and deceleration (momentum) rates.  Legacy's built-in deceleration (momentum) rates are:
//   Note: Although we must delay 30ms between successive 3-byte Legacy commands, the Legacy hardware itself is apparently not
//   limited by this; hence it is able to issue ever-decreasing speed commands up to every 16ms as seen with Momentum 1.
// Momentum 1: Issues 1 step about every 16ms.  Stops from speed 62 in one second; nearly instant stopping.
// Momentum 2: Issues 1 step every  20ms.  Similar to 2 steps every 40ms, 3 steps every 60ms, 4 steps every 80ms, 5 steps every 100ms.
//                                         Way too fast to appear realistic.
//                                         A train moving at 30smph would take 1.6 sec to stop (3.3 sec from 60smph.)
// Momentum 3: Issues 1 step every  40ms.  Still too fast to appear realistic.
//                                         A train moving at 30smph would take 3.3 sec to stop (6.6 sec from 60smph.)
// Momentum 4: Issues 1 step every  80ms.  Pretty fast deceleration, but realistic for short/lightweight trains.
//                                         A train moving at 30smph would take 6.6 sec to stop (13 sec from 60smph.)
// Momentum 5: Issues 1 step every 160ms.  This is a good rate of deceleration, LOOKS GOOD for most locos such as an F3.
//                                         A train moving at 30smph would take 13 sec to stop (26 sec from 60smph.)
// Momentum 6: Issues 1 step every 320ms.  This is too-slow deceleration for most, but LOOKS GOOD for Shay since it's moving so slow.
//                                         A train moving at 30smph would take 26 sec to stop (52 sec from 60smph.)
//                                         Since Shay top speed is about 15smph, it will take 13 sec to stop from fast speed.

// Since we can only send serial commands to Legacy every 30ms (any faster and Legacy may ignore them,) we'll need to consider how
// many trains we might want to be accellerating/decelerating simultaneously.
// i.e. If we allow for possibly decelerating 10 trains at the same time, we'll only be able to send a command to each train every
// 300ms, which is about 3 commands per second.  Although that will realistically never happen, let's plan on sending speed
// commands to any given loco not more than every 320ms.  This will be fine for Momentum 6, but if we want to slow down at momentum
// 5, then we'll need to slow down TWO Legacy speed steps at a time; and if we want to slow down at momentum 4, then we'll need to
// slow down FOUR Legacy speed steps at a time.  (NOTE: Once we trip a STOP sensor, we'll send Speed 0 to stop immediately.)

// HORN NOTES 5/1/22: The Quilling horn toot lasts a lot longer than the Horn 1 toot.  The good thing about the Quilling horn is
// that we can delay much longer between successive horn commands i.e. less serial commands to be sent = less traffic.
// Unfortunately, the quilling horn doesn't sound as good especially on Big Boy discrete toots; okay for RR Xing pattern.
// Once we have multiple trains doing multiple things and we get a lot of serial commands going to the Legacy base, there is a risk
// that succesive commands to keep a long toot going might be held up and a long toot could have gaps and thus sound wrong.  So for
// continuous toots, either Horn 1 or Quilling, we might consider REDUCING the delay between successive toot commands to keep it
// going (to allow for possible delays in getting successive commands to the Legacy base.)  On the other hand, when delaying
// between successive toots, we have a MINIMUM delay, so any additional delay introduced by traffic won't be a problem.
// If we detect gaps in what should be a long toot, we can thus REDUCE the "continuous" delay to allow for some real-time delays.
// FOR DISCRETE HORN 1 TOOTS, 450ms between commands is the minimum delay; 400ms and they occasionally run together.
// FOR CONTINUOUS HORN 1 TOOTS, 175ms between commands is the maximum delay; 200ms and there are occasional gaps.
// FOR DISCRETE QUILLING TOOTS, 950ms between commands is the minimum delay; 900ms unreliable on Shay.
// FOR CONTINUOUS QUILLING TOOTS, 650ms between commands is the maximum delay; 700ms introduces occasional gaps

// We might also try throwing in some HORN commands, as they must be repeated every 250ms or so (Rudy says every 100ms so need to test.)

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
const byte THIS_MODULE = ARDUINO_NUL;  // Need some value so global functions will work
char lcdString[LCD_WIDTH + 1] = "RDS 02/18/24";  // Global array holds 20-char string + null, sent to Digole 2004 LCD.

// *** SERIAL LCD DISPLAY CLASS ***
// #include <Display_2004.h> is already in <Train_Functions.h> so not needed here.
Display_2004* pLCD2004 = nullptr;  // pLCD2004 is #included in Train_Functions.h, so it's effectively a global variable.

// NOTES REGARDING EMPIRICAL TESTING OF BOUNCE AND REV SPEED OF ROLLER BEARING.  5/2/22.
//   OSCILLOSCOPE ROLLER BEARING BOUNCE TEST with four sections(two black, two white) :
//     Slowest loco (Shay), slowest speed (1), bounce is usually .3ms but can glitch up to 3.5ms.  Never happens above min. speed.
//     Fastest loco (Big Boy), fastest speed (199), transitions every 13ms - the fastest I could ever see.
//     Thus a bounce of 4ms (and up to 10ms) would work fine.

#include <TimerOne.h>  // We will generate an interrupt every 1ms to help us count roller bearing revs
const byte DEBOUNCE_DELAY = 4;  // Number of ms to allow IR sensor signal to stabilize after changing.
// Any variables that are used in an interrupt *and* which might be access from outside the interrupt *must* be declared as
// volatile, which tells the compiler to retrieve a copy from memory each time it is accessed, and not assume that a value
// stored in cache is the current value.  Also we must temporarily disable interrupts whenever these volatile variables are
// read or updated from outside the ISR -- so do so only when you know that it won't be important if a call to the ISR is skipped.
volatile unsigned long bearingQuarterRevs = 0;  // This will count the number of state changes seen by the IR sensor on the bearing
// Any time we access a (volatile) variable that's used in an ISR from outside the ISR, we must temporarily disable
// (noInterrupts()) and then re-enable (interrupts()) interrupts.  Since we don't want to disable interrupts if it's possible the
// interrupt might be called, we'll create a working variable that will store the value of the volatile variable.  So we can do
// math on it, or use it however we want, even when interrupts are enabled.
unsigned long tempBearingQuarterRevs = 0;  // We'll grab the interrupt's value and drop in here as a working variable

// *** MISC CONSTANTS AND GLOBALS:

// 5/2/22: **** IMPORTANT **** The following roller-bearing CIRCUMFERENCE data was out-dated once Roller_Speed.ino was used to
// calculate unique roller-bearing diameters for each loco.  However, the REAL WORLD times collected from the 5-meter section on
// the layout itself are still valid.
// 
// 5/2/22: Measured bearing diameter w/ calipers as 12.84 mm, so that's a good starting point.  Implies approx 40.338mm circ.
// Real-world testing shows that at slow speeds, times vary quite a lot but maybe not as a percentage of total (longer) time.
// Real-world testing shows that at high speeds, times vary less but are a higher percentage of the total due to short times.
// The most accurate real-world measurements for calculating bearing circumference seem to be at middle-ish speeds.
const float bearingCircumference = 40.7332;  // In mm, with one layer of tape on it.
const float bearingQuarterCircumference = 10.1833;  // Since we count quarter revs and not whole revs

// 5/2/22 REAL WORLD TEST RESULTS: SHAY on 5 METER TRACK:
//   Speed  20: 209.20 sec. ROLLER BEARINGS running for 209.20s => 4989mm = short by  11mm/5000mm = 0.22% short
//                                           "                     4969mm = short by  31mm/5000mm = 0.62% short
//   Speed  40:  99.04 sec. ROLLER BEARINGS running for  99.04s => 4979mm = short by  21mm/5000mm = 0.42% short
//                                           "                     4979mm = short by  21mm/5000mm = 0.42% short
//   Speed  60:  58.07 sec. ROLLER BEARINGS running for  58.07s => 5030mm = long  by  30mm/5000mm = 0.59% LONG
//                                           "                     5010mm = long  by  10mm/5000mm = 0.20% LONG

//   Speed  80 (5/19/22): 38.35, 38.35, 38.20, 28.22 (fwd), 38.27, 38.31 (rev)

//   Speed 100:  27.05 sec. ROLLER BEARINGS running for  27.05s => 4959mm = short by  41mm/5000mm = 0.82% short
//                                           "                     4969mm = short by  31mm/5000mm = 0.62% short
//   Reverse Speed 60: 57.70s
//   Reverse Speed 100: 27.11s

// 5/2/22 REAL WORLD TEST RESULTS: BIG BOY on 5 METER TRACK:
//   Speed  20: 105.64 sec. ROLLER BEARINGS running for 105.52s => 4887mm = short by 113mm/5000mm = 2.26% short
//                                           "                     4877mm = short by 123mm/5000mm = 2.45% short
//                                           "                     4887mm = short by 113mm/5000mm = 2.26% short
//   Speed  40:  48.85 sec. ROLLER BEARINGS running for  48.85s => 4867mm = short by 133mm/5000mm = 2.67% short
//                                           "                     4877mm = short by 123mm/5000mm = 2.46% short

//   Speed  40 (5/19/22): 49.08, 49.09, 49.03, 49.00 (fwd), 48.46, 48.65 (rev)

//   Speed  60:  28.84 sec. ROLLER BEARINGS running for  28.84s => 4918mm = short by  82mm/5000mm = 1.64% short
//                                           "                     4918mm = short by  82mm/5000mm = 1.64% short
//                                           "                     4918mm = short by  82mm/5000mm = 1.64% short
//   Speed  80:  18.96 sec. ROLLER BEARINGS running for  18.96s => 4959mm = short by  41mm/5000mm = 0.82% short
//                                           "                     4959mm = short by  41mm/5000mm = 0.82% short

// 5/2/22 REAL WORLD TEST RESULTS: SP 1440 on 5 METER TRACK:
//   Speed  20: 111.33 sec. ROLLER BEARINGS running for 111.33s => 5213mm = long  by 213mm/5000mm = 4.25% LONG -- a fluke?
//                                           "                     5213mm = long  by 213mm/5000mm = 4.25% LONG -- a fluke?
//                          (in reverse)                           5213mm = long  by 213mm/5000mm = 4.25% LONG
//             Try again 111.19 sec.         "                     5010mm = long  by  10mm/5000mm = 0.02% LONG
//                                           "                     4989mm = short by  11mm/5000mm = 0.02% short
//             Took off track then back on   "                     4979mm = short by  21mm/5000mm = 0.42% short
//                          (in reverse)                           5234mm = long  by 234mm/5000mm = 4.68% LONG -- way off.
//   Speed  30:   70.74 sec.
//   Speed  40:  50.28 sec. ROLLER BEARINGS running for  50.28s => 4969mm = short by  31mm/5000mm = 0.62% short
//                                           "                     4979mm = short by  21mm/5000mm = 0.42% short
//   Speed  60:  29.26 sec. ROLLER BEARINGS running for  29.26s => 4959mm = short by  41mm/5000mm = 0.82% short
//                                           "                     4969mm = short by  31mm/5000mm = 0.62% short
//   Speed  80:  19.05 sec. ROLLER BEARINGS running for  19.05s => 4959mm = short by  41mm/5000mm = 0.82% short
//                                           "                     4949mm = short by  51mm/5000mm = 1.02% short
//   Speed  84:   17.7  sec.
//   Speed 100:  13.48 sec. ROLLER BEARINGS running for  13.48s => 4979mm = short by  21mm/5000mm = 0.42% short
//                                           "                     4949mm = short by  51mm/5000mm = 1.02% short
//                                           "                     4969mm = short by  31mm/5000mm = 0.62% short

// 5/2/22 REAL WORLD TEST RESULTS: WP 803-A on 5 METER TRACK:
//   Speed  20: 102.60 sec. ROLLER BEARINGS running for 102.60s => 4704mm = short by 296mm/5000mm = 5.92% short
//                                           "                     4725mm = short by 275mm/5000mm = 5.50% short
//                          (in reverse)                           4714mm = short by 286mm/5000mm = 5.72% short
//              102.80 sec.                             102.80s    4796mm = short by 204mm/5000mm = 4.08% short
//                          (in reverse)                           4765mm = short by 235mm/5000mm = 4.70% short
//   Speed  40:  48.96 sec. ROLLER BEARINGS running for  48.96s => 4837mm = short by 163mm/5000mm = 3.26% short
//                                           "                     4806mm = short by 194mm/5000mm = 3.88% short
//                          (in reverse)                           4847mm = short by 153mm/5000mm = 3.06% short

//   Speed  50 (5/19/22): 36.76, 36.70, 36.52, 36.48, 36.63, 36.66 (fwd), 37.02, 36.90 (rev)

//   Speed  60:  28.92 sec. ROLLER BEARINGS running for  28.92s => 4857mm = short by 143mm/5000mm = 2.86% short
//                                           "                     4857mm = short by 143mm/5000mm = 2.86% short
//                          (in reverse)                           4847mm = short by 153mm/5000mm = 3.06% short
//                                           "                     4857mm = short by 143mm/5000mm = 2.86% short
//   Speed  80:  18.96 sec. ROLLER BEARINGS running for  18.96s => 4877mm = short by 123mm/5000mm = 2.46% short
//                                           "                     4877mm = short by 123mm/5000mm = 2.46% short
//                          (in reverse)                           4877mm = short by 123mm/5000mm = 2.46% short
//   Speed 100:  13.31 sec. ROLLER BEARINGS running for  13.31s => 4867mm = short by 133mm/5000mm = 2.66% short
//                                           "                     4877mm = short by 123mm/5000mm = 2.46% short
//                          (in reverse)                           4867mm = short by 133mm/5000mm = 2.66% short


// CAUTION: If the loco stops when the sensor is between black/white stripes, it may think it's still moving.

// NOTE updated 5/2/22: To calculate between smph and mm/Sec using 1:48 scale: 1 scale mile = 33528mm (exactly 110 ft.)
//   9.3133 mm/Sec = 1 smph == 558.798 mm/Sec = 60smph.
//   ROUND NUMBERS: Max speed of about 60smph would be about 559mm/sec.
// Ran SP 1440 on layout 4/23/22 for 5 meters at Legacy speed 84 in 17.68 seconds.
//   = 1 meter per 3.536 sec = .2828 meters/sec = 282.8 mm/sec.  This is 30.37smph.
// Since the bearing is about 41mm circumference, that's about 6.8 revolutions per second at 30smph.
// That's about 27 DETECTIONS/SECOND at 30smph (for any train,) or 54 detections/second at 60smph.
// 54 state changes per second (1000 ms) = a state change every 18ms.
// Thus, DEBOUNCE_DELAY *must* be less than 18 milliseconds.
// At loco speed 1, bounce is usually about .3ms, but occasional spurious 3.5ms bounces sometimes happen.  So use 4ms bounce delay.
// At higher speeds, bounce is reliably about .3ms or less.
// At loco speed 60mph, each 1/4 turn transition lasts 18ms, so a 4ms bounce delay is absolutely no problem.
// 5/2/22: OSCILLOSCOPE ROLLER BEARING BOUNCE TEST with four sections(two black, two white).  It is IMPORTANT to note that the IR
//   sensor module has a potentiometer adjustment on it, and if it's not perfectly adjusted then the bounce can be much longer.
// UP Big Boy:
//   MIN SPEED 1, bounce is typically .3ms or less, but rarely can be up to 3.5ms.  Confirmed w/ Shay running at Speed 1.
//   MAX SAFE SPEED 120, transitions every 19ms.  This is likely the fastest we *would* ever transition in actual operation.
//   TOP SPEED 199, transitions every 13ms.  This is the absolute fastest we *could* ever transition.

const byte PIN_LED_ON_BOARD = 13;  // OUPTUT.  Arduino on-board LED.  Pull LOW to turn on.
const byte PIN_RPM_SENSOR = 2;

unsigned long currentTime;
unsigned long endTime;
unsigned long startTime;
unsigned long loopTime;  // How long did it take to count the number of revs we were waiting for
unsigned long totalTime;
unsigned long totalDistance;
unsigned long RPM = 0;
unsigned long SMPH = 0;
float calcRPM = 0;  // Use this for floating point calcs, but can't print float values with sprintf
float calcSMPH = 0;  // This will store the scale mph based on RPM

// NOT SURE ABOUT THESE NUMBERS:
// RPM / mm/sec = 1.5 (was 3.0)
// Since 1.5 (was 3) RPM = 1 mm/sec, 9.313333 mm/sec * 1.5 = 13.97
// 1 RPM = 27.94 smph
const float RPMtoSMPH = 27.94; // smph = RPM / 27.94 NOT USING THIS YET BUT WILL WANT TO SO MAY NEED TO TWEAK CONSTANT

byte deviceNumber = 0;  // 14 = big boy, 40 = SP 1440, 80 = WP 803-A, 2 = Shay
byte legacy11 = 0;
byte legacy12 = 0;
byte legacy13 = 0;

// *****************************************************************************************
// **************************************  S E T U P  **************************************
// *****************************************************************************************

void setup() {

  // *** INITIALIZE ARDUINO I/O PINS ***
  initializePinIO();
  pinMode(PIN_RPM_SENSOR, INPUT);
  pinMode(PIN_LED_ON_BOARD, OUTPUT);
  digitalWrite(PIN_LED_ON_BOARD, LOW);

  // *** INITIALIZE SERIAL PORTS ***
  // FOR THIS TEST PROGRAM, we are going to connect BOTH the PC's Serial port *and* the Mini Thermal Printer to Serial 0.
  // Since the mini printer requires 19200 baud, we'll set the serial monitor to 19200 as well.
  Serial.begin(19200);  // SERIAL0_SPEED.  9600 for White thermal printer; 19200 for Black thermal printer
  // Serial1 instantiated via Display_2004/LCD2004.
  // Serial2 will be connected to the thermal printer.
  Serial3.begin(9600);  // Legacy via MAX-232

  // *** INITIALIZE LCD CLASS AND OBJECT *** (Heap uses 98 bytes)
  // We must pass parms to the constructor (vs begin) because needed by parent DigoleSerialDisp.
  pLCD2004 = new Display_2004(&Serial1, SERIAL1_SPEED);  // Instantiate the object and assign the global pointer.
  pLCD2004->begin();  // 20-char x 4-line LCD display via Serial 1.
  pLCD2004->println(lcdString);  // Display app version, defined above.
  Serial.println(lcdString);
  Serial2.println(lcdString);       // Mini thermal printer

// TRY TO RESET THERMAL PRINTER @ 9600 BAUD ON SERIAL 3
/*
  ESC 7 n1 n2 n3
    [Name] Setting Control Parameter Command
    [Format] ASCII: ESC 7 n1 n2 n3
    Decimal       : 27 55 n1 n2 n3
    Hexadecimal   : 1B 37 n1 n2 n3
    * Set “max heating dots”, ” heating time”, “heating interval”
      n1 = 0 - 255 Max printing dots, Unit(8dots), Default : 7(64 dots)
      n2 = 3 - 255 Heating time, Unit(10us), Default : 80(800us)
      n3 = 0 - 255 Heating interval, Unit(10us), Default : 2(20us)
    * The more max heating dots, the more peak current will cost when printing, the faster printing speed.The max heating dots is
      8 * (n1 + 1)
    * The more heating time, the more density, but the slower printing speed.If heating time is too short, blank page may occur.
    * The more heating interval, the clearer, but the slower printing speed.
*/

  //  Serial.print(27); Serial.print(55); Serial.print(7); Serial.print(80); Serial.print(2);  // DEFAULT Heating dots/time/interval
// Serial.print(27); Serial.print(55); Serial.print(11); Serial.print(120); Serial.print(40);  // ADAFRUIT Heating dots/time/interval

  //  Serial.print(27); Serial.print(55); Serial.print(250); Serial.print(250); Serial.print(250);  // Heating dots/time/interval
  //  Serial.print(27); Serial.print(55); Serial.print(7); Serial.print(80); Serial.print(2);  // Heating dots/time/interval

/* TRY THIS FOR DARKER PRINT(DENSITY) :
  DC2 # n
    [Name] Set printing density
    [Format] ASCII: DC2 # n
    Decimal : 18 35 n
    Hexadecimal : 12 23 n
    [Description] D4..D0 of n is used to set the printing density
    Density is 50 % +5 % *n(D4 - D0)
    D7..D5 of n is used to set the printing break time
    Break time is n(D7 - D5) * 250us

  Serial.write(0x12); //FOR DARKER PRINT
  Serial.write(0x23);
  Serial.write(0x00); // 05);  // This is the value to modify.  Tried 01, 0F, 00, FF and nothing makes a difference
*/

/*!
     * @brief Sets print head heating configuration
     * @param dots max printing dots, 8 dots per increment
     * @param time heating time, 10us per increment
     * @param interval heating interval, 10 us per increment
     */
// setHeatConfig(uint8_t dots = 11, uint8_t time = 120, uint8_t interval = 40),
//Serial.write(27); Serial.write('7'); // Esc 7 
//Serial.write(11); Serial.write(240); Serial.write(240);

/*!
 * @brief Sets print density
 * @param density printing density
 * @param breakTime printing break time
 */
//  setPrintDensity(uint8_t density = 10, uint8_t breakTime = 2),
//Serial.write(18);  // Ctrl-2 = ASCII DC2
//Serial.write('#');
//Serial.write(10 << 5 | 2);  // density | breaktime


  // Set the timer interrupt to fire every 1ms.
  Timer1.initialize(1000);  // 1000 microseconds = 1 millisecond
  // The timer interrupt will call ISR_Read_IR_Sensor() each interrupt.
  Timer1.attachInterrupt(ISR_Read_IR_Sensor);

  deviceNumber = 2;  // 14 = big boy, 40 = SP 1440, 80 = WP 803-A, 2 = Shay
  
  // Always set loco momentum to zero
  legacy11 = 0xF8;  // Legacy Engine
  legacy12 = (deviceNumber * 2);  // Shift loco num left one bit, fill with zero
  legacy13 = 0xC8;  // Momentum 0 (Momentum 7 would b 0xCF)
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(30);  // 30ms delay before next command, just being sure here
  // Don't forget we'll need at least 30ms delay before sending another command

/*
// TMCC HORN COMMAND:
  const byte TMCCHorn = B11100;  // TMCC horn command
  legacy11 = 0xFE;  // For TMCC, 0xFE is *always* the first byte
  byte typeEngine = B00000000;  // TMCC device type for Engine (1..99)
  byte typeTrain = B11001000;   // TMCC device type for Train (1..9)
  byte deviceType = typeEngine;
  byte deviceID = 14;  // UP Big Boy 4014
  byte commandField = 0;  // 00 for many commands including horn
  byte dataField = B00011100;  // Horn 1 (Horn 2 does not work on Big Boy for either TMCC or Legacy)
  legacy12 = deviceType | (deviceID >> 1); // Byte 2 is combination of type field and first 7 bit of TMCC address
  legacy13 = (deviceID << 7) | (commandField << 5) | dataField; // Byte 3 is last bit of TMCC address, command field (2 bits), plus data field
  blowHornCrossing();
  delay(4000);
  blowHornReverse();
  delay(4000);
*/

// LEGACY HORN COMMAND:
  legacy11 = 0xF8;  // Legacy Engine command (Legacy Train command = 0xF9)
  legacy12 = (deviceNumber * 2) + 1;  // Shift loco num left one bit, fill with zero; add 1 since this is a horn command
  legacy13 = 0x1C;  // Horn byte 1C
//  blowHornCrossing();
//  delay(4000);
//  blowHornReverse();
//  delay(4000);

// LEGACY QUILLING HORN
  legacy11 = 0xF8;  // Legacy Engine command (Legacy Train command = 0xF9)
  legacy12 = (deviceNumber * 2) + 1;  // Shift loco num left one bit, fill with zero; add 1 since this is a quilling horn command
  legacy13 = 239;  // Quilling horn 1 (other values are 224 thru 239)
//  blowHornCrossingQuilling();
//  delay(4000);
//  blowHornReverseQuilling();
//  delay(4000);


  sprintf(lcdString, "Begin test."); Serial.println(lcdString);

// ACCELERATING AND DECELERATING 5/1/22
// Accelerating/decelerating using multiple steps up to about 5 you really can't even tell, EXCEPT at the very lowest speeds.
// So maybe write the code so that acceleration starts from stopped in single steps until you hit about 20, and then can jump
// to 4 steps at 4x the delay.
// Similarly, when slowing down, begin slowing in steps of 4 at 4x delay, and then slow in single steps below about speed 20 or 10.

  blowHornToot(deviceNumber); delay(30);
  unsigned long momentumDelay = 80; // 160;  // ms delay between successive speed commands
  byte speedStep = 1; // 1;
  byte maxSpeed = 80;
  long int runningDelay = 18960;  // How long do we want to run the train for at this speed

  // Legacy speed can be 0..199
  for (byte i = 0; i < maxSpeed + 1; i=i + speedStep) {
    sprintf(lcdString, "Speed %3i", i); pLCD2004->println(lcdString);  // THIS WILL CERTAINLY THROW OFF MOMENTUM DOING IT THIS WAY ********************************
    legacy12 = (deviceNumber * 2) + 0;  // Shift loco num left one bit, fill with zero; add 0 for abs speed command
    legacy13 = i;  // Absolute speed
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(momentumDelay);
  }
  blowHornToot(deviceNumber); delay(30);
delay(2000);  // Let the train reach maximum speed in case of minor momentum

sprintf(lcdString, "Mid test."); Serial.println(lcdString);

  // Let's count some revs!
  noInterrupts();  // Disable timer IRQ so we can (re)set variables that it uses (plus our own)
  bearingQuarterRevs = 0;  // This will count the number of state changes seen by the IR sensor on the bearing
  interrupts();
  startTime = millis();
  delay(runningDelay);
  endTime = millis();
  noInterrupts();
  tempBearingQuarterRevs = bearingQuarterRevs;
  interrupts();

  for (byte i = maxSpeed; i > 0; i = i - speedStep) {  // Caution don't let i < 0 or will crash Legacy
    legacy12 = (deviceNumber * 2) + 0;  // Shift loco num left one bit, fill with zero; add 0 for abs speed command
    legacy13 = i;  // Absolute speed
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(momentumDelay);
  }
  legacy13 = 0;  // Full stop
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  blowHornToot(deviceNumber); delay(30);

  sprintf(lcdString, "Qtrs: %10ld", tempBearingQuarterRevs); pLCD2004->println(lcdString); Serial2.println(lcdString);
  totalDistance = (bearingQuarterCircumference * tempBearingQuarterRevs);
  sprintf(lcdString, "Dist: %10ld", totalDistance); pLCD2004->println(lcdString); Serial2.println(lcdString);
  totalTime = endTime - startTime;
  sprintf(lcdString, "Time: %10ld", totalTime); pLCD2004->println(lcdString); Serial2.println(lcdString);

sprintf(lcdString, "Test complete."); Serial.println(lcdString);

  while (true) { };
}

// *****************************************************************************************
// ***************************************  L O O P  ***************************************
// *****************************************************************************************

void loop() {

}

void ISR_Read_IR_Sensor() {
  // 5/2/22: Gets called every 1ms to see if the roller bearing IR sensor has changed state.  There are four state changes per rev.
  // Updates the global volatile variable bearingQuarterRevs to count each quarter turn of the roller bearing.
  // We are allowing for a bounce of up to 4ms for the signal to stabilize after each state change, which is no problem because the
  // fastest loco (Big Boy) triggers a state change every 13ms, and it would *never* travel that fast on the layout.
  // At most speeds, bounce is much less than 1ms, but at speed 1 there is sometimes a weird bounce of 3.5ms (very rare.)
  static byte bounceCountdown = 0;  // This will get reset to DEBOUNCE_DELAY after each new sensor change
  static int oldSensorReading = LOW;
  static int newSensorReading = LOW;

  // Wait until our bounce delay has transpired after reading a state change, before looking for the next one.
  if (bounceCountdown > 0) {  // Keep waiting for any bounce to resolve
    bounceCountdown --;
  } else {  // No risk of bounce; see if bearing has turned 1/4 turn since our last check
    newSensorReading = digitalRead(PIN_RPM_SENSOR);  // Will be LOW or HIGH
    if (newSensorReading != oldSensorReading) {  // Sensor status has legitimately changed!
      oldSensorReading = newSensorReading;
      bearingQuarterRevs++;
      bounceCountdown = DEBOUNCE_DELAY;  // Reset bounce countdown time each time we fetch a new sensor reading
    }
  }
}

//byte legacy12(char engineOrTrain, byte deviceNum, byte commandNum, byte commandData) {
  // Rev: 04/29/22.  Assemble a TMCC command, byte 1 of 2.
  // engineOrTrain = [E|T].  I don't think we will ever use Train but we'll allow for it.
  // deviceNum = up to 99 Engines, up to (only) 9 Trains in TMCC
  // commandNum = [0|1|2|3].
  // commandData = Speed 0..31, or other specific to each command
  // TMCCCommand1 = deviceType | (deviceID >> 1); // Byte 2 is combination of type field and first 7 bit of TMCC address
  // TMCCCommand2 = (deviceID << 7) | (commandField << 5) | dataField; // Byte 3 is last bit of TMCC address, command field (2 bits), plus data field
  //   where these are the deviceTypes :
  //   typeEngine = B00000000
  //   typeTrain  = B11001000

//}

void blowHornCrossing() {
  // L-L-S-L, works for both Legacy and TMCC
  // This is a shorter sequence than when using the Quilling horn; both sound good.
  // FOR DISCRETE HORN 1 TOOTS, 450ms between commands is the minimum delay; 400ms and they occasionally run together.
  // FOR CONTINUOUS HORN 1 TOOTS, 175ms between commands is the maximum delay; 200ms and there are occasional gaps.
  unsigned long hornDelay = 175;  // 200 is too long for WP 803-A!
  for (int x = 0; x < 8; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
  delay(700);
  for (int x = 0; x < 8; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
  delay(900);
  for (int x = 0; x < 2; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
  delay(700);
  for (int x = 0; x < 16; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
}

void blowHornCrossingQuilling() {
  // 5/1/22: L-L-S-L, works for both Legacy and TMCC
  // This is a longer sequence than when using the Horn 1 (non-quilling) horn; both sound good.
  // FOR DISCRETE QUILLING TOOTS, 950ms between commands is the minimum delay; 900ms unreliable on Shay.
  // FOR CONTINUOUS QUILLING TOOTS, 650ms between commands is the maximum delay; 700ms introduces occasional gaps
  unsigned long hornDelay = 650;  // 650ms is reliable maximum on Big Boy, 700 has gaps
  for (int x = 0; x < 3; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
  delay(1500);
  for (int x = 0; x < 3; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
  delay(1200);
  for (int x = 0; x < 1; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
  delay(600);
  for (int x = 0; x < 5; x++) {
    Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
    delay(hornDelay);
  }
}

void blowHornReverse() {
  // 5/1/22: Toots using Horn 1 are shorter than when using the Quilling Horn, so better for short discrete toots.
  // FOR DISCRETE HORN 1 TOOTS, 450ms between commands is the minimum delay; 400ms and they occasionally run together.
  // FOR CONTINUOUS HORN 1 TOOTS, 175ms between commands is the maximum delay; 200ms and there are occasional gaps.
  unsigned long hornDelay = 700;  // Gap between discrete toots must be AT LEAST 450ms, though 700ms sounds better
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(hornDelay);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(hornDelay);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(2000);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(hornDelay);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
}

void blowHornReverseQuilling() {  // DO NOT USE
  // 5/1/22: Toots are too long; doesn't sound as good as Horn 1 (non-quilling) on Big Boy or WP 803-A or Shay
  // Quilled toots are longer than Horn 1 toots, so we need different delays between toot commands.
  // FOR DISCRETE QUILLING TOOTS, 950ms between commands is the minimum delay; 900ms unreliable on Shay.
  // FOR CONTINUOUS QUILLING TOOTS, 650ms between commands is the maximum delay; 700ms introduces occasional gaps
  unsigned long hornDelay = 950;  // 900 works for Big Boy Minimum time between one toot and the next, if a gap is desired
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(hornDelay);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(hornDelay);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(2000);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
  delay(hornDelay);
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
}

void blowHornToot(byte locoNum) {
  legacy12 = (locoNum * 2) + 1;  // Shift loco num left one bit, fill with zero; add 1 since this is a horn command
  legacy13 = 0x1C;  // Horn byte 1C
  Serial3.write(legacy11); Serial3.write(legacy12); Serial3.write(legacy13);  // Send all 3 bytes to Legacy base
}