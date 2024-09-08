// OCCUPANCY_LEDs.CPP Rev: 07/30/24.  FINISHED BUT NOT TESTED.
// Part of O_OCC.
// This class is responsible for illumination of Control Panel WHITE OCCUPANCY SENSOR LEDs and BLUE/RED BLOCK OCCUPANCY LEDs.
// Illumination depends on Mode and State.
// RED = RESERVED, BLUE = OCCUPIED.
// There is no support for blinking WHITE or RED/BLUE LEDs.
// Note: Sensor Status stores how Control Panel LEDs should be lit, NOT the current status of each sensor.
// Main modules (MAS, OCC, LEG) use the Sensor Block table to save and retrieve sensor status during Registration and Auto/Park.

#include "Occupancy_LEDs.h"

Occupancy_LEDs::Occupancy_LEDs() {
  // Rev: 03/19/23.  FINISHED BUT NOT YET TESTED.
  // No constructor tasks.
  return;
}

void Occupancy_LEDs::begin(Train_Progress* t_pTrainProgress) {
  // Rev: 03/20/23.  FINISHED BUT NOT YET TESTED.
  m_pTrainProgress = t_pTrainProgress;     // To find Reserved and Occupied blocks for each loco by scanning Train Progress
  if (m_pTrainProgress == nullptr) {
    sprintf(lcdString, "UN-INIT'd TP PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  initSensorStatus();    // (Just an array) Set all m_sensorStatus[] elements to Cleared.
  initStaticBlocks();    // (Just an array) Set all blocks to initially *not* occupied by STATIC equipment.
  initNewBlockStatus();  // (Just an array) Set all m_newBlockStatus[] and elements to LED_OFF
  initOldBlockStatus();  // (Just an array) Prevents unnecessary digitalWrite() commands in paintAllBlockOccupancyLEDs()
  return;
}

void Occupancy_LEDs::updateSensorStatus(const byte t_sensorNum, const char t_tripOrClear) {
  // Rev: 03/19/23.  FINISHED BUT NOT YET TESTED.
  // Update our private sensor status array.  This does not change the illumination of any LEDs.
  // Appropriate for ALL MODES.  We will only receive Sensor Change messages when MAS deems them appropriate.
  // Call this function EVERY TIME a Sensor Change message is received, to keep our private m_sensorStatus[] array up to date.
  // m_sensorStatus[] should always reflect the actual status of the sensors (once populated); not necessarily what is lit on the
  // control panel (which is dependent on mode and state.)
  // t_sensorNum == 1..TOTAL_SENSORS and our private m_sensorStatus[] array is 0..(TOTAL_SENSORS - 1)
  // const byte LED_OFF             = 0;  // LED off
  // const byte LED_SENSOR_OCCUPIED = 1;  // White occupancy LED lit
  if (t_tripOrClear == SENSOR_STATUS_CLEARED) {  // 'C'
    m_sensorStatus[t_sensorNum - 1] = LED_OFF;
  } else if (t_tripOrClear == SENSOR_STATUS_TRIPPED) {  // 'T'
    m_sensorStatus[t_sensorNum - 1] = LED_SENSOR_OCCUPIED;
  } else {
    sprintf(lcdString, "FATAL OCC LED ERR 1"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }
  return;
}

void Occupancy_LEDs::setBlockStatic(const byte t_blockNum) {
  // Rev: 03/20/23.  FINISHED BUT NOT YET TESTED.
  // Populated during Registration, used during Auto/Park when illuminating RED/BLUE BLOCK OCCUPANCY LEDs.
  // Each time a STATIC "loco" is Registered, must call this function to update the Block Static indicator.
  m_staticBlock[t_blockNum - 1] = true;
  return;
}

void Occupancy_LEDs::darkenAllOccupancySensorLEDs() {
  // Rev: 07/30/24: New function since "paintOne" with a parm of zero didn't work due to confusion over sensorNum 0 vs 1
  for (byte i = 0; i < TOTAL_SENSORS; i++) {
    pShiftRegister->digitalWrite(64 + i, HIGH);  // Turn off the LED
  }
  return;
}

void Occupancy_LEDs::paintAllOccupancySensorLEDs(const byte t_mode, const byte t_state) {
  // Rev: 03/23/23.  FINISHED BUT NOT YET TESTED.
  // 03/23/23: Modified to illuminate all WHITE LEDs appropriately during Registration, as this seems more helpful than one only.
  // Works for ALL MODES.  Assumes m_sensorStatus[] array reflects current state of all Occupancy Sensors.
  // Based on the current mode and state, and current sensor status, update every white Occupancy Sensor LED on the control panel.
  // We already have: m_sensorStatus[0..(TOTAL_SENSORS - 1)] = 0 (off) or 1 (on).
  // Note 11/1/20: If we get RS485 input buffer overflow, modify this code to keep track of each LED status, and only write to the
  // Centipede shift register if an LED status changes.  Writing to ALL of the LEDs every time we call this function can take a
  // relatively long time and we could miss rapidly incoming RS485 messages, as we did in O_LED until we made the change I'm
  // suggesting here.  May also need to do the same thing for the red/blue block occupancy LEDs.
  // Write a ZERO to a bit to turn on the LED, write a ONE to a bit to turn OFF the LED.  Opposite of our sensorStatus[] array.
  for (byte i = 0; i < TOTAL_SENSORS; i++) {       // For every sensor/"bit" of the Centipede shift register output
    // If STOPPED we want to darken all LEDs no matter what mode...
    if (t_state == STATE_STOPPED) {
      pShiftRegister->digitalWrite(64 + i, HIGH);  // turn off the LED
    /*
    } else  if (t_mode == MODE_REGISTER) {    // Darken all white LEDs whenever we are in Register mode, any state
      pShiftRegister->digitalWrite(64 + i, HIGH);  // turn off the LED
    // Not STOPPED and not REGISTER means illuminate according to sensor status (MANUAL/AUTO/PARK, RUNNING/STOPPING)
    */
    // Else if NOT STOPPED, any mode, we want to illuminate according to sensor status (MANUAL/AUTO/PARK, RUNNING/STOPPING)
    } else if (m_sensorStatus[i] == LED_OFF) {
      pShiftRegister->digitalWrite(64 + i, HIGH);  // turn off the LED
    } else if (m_sensorStatus[i] == LED_SENSOR_OCCUPIED) {
      pShiftRegister->digitalWrite(64 + i, LOW);   // turn on the LED
    } else {
      sprintf(lcdString, "FATAL OCC LED ERR 2"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
    }
  }
  return;
}

void Occupancy_LEDs::paintOneBlockOccupancyLED(const byte t_blockNum) {
  // Rev: 03/19/23.  FINISHED BUT NOT YET TESTED.
  // REGISTRATION mode only.  Thus we only care to illuminate a RED LED (not BLUE.)
  // Turn ONE RED BLOCK OCCUPANCY LED ON and all others OFF (t_blockNum > 0), or turn ALL RED AND BLUE LEDs OFF (t_blockNum = 0)
  // This function does not support turning a single BLUE LED on.
  // Passing t_blockNum == 0 will turn off ALL Block Occupancy LEDs (such as when initially powering up the system.)
  // Else t_blockNum = 1..TOTAL_BLOCKS (i.e. starts at 1 not 0.)
  // LEDBlueBlockPin[] and LEDRedBlockPin[] record corresponds to (block number - 1) = 0..25 for blocks 1..26.
  for (byte i = 1; i <= TOTAL_BLOCKS; i++) {  // For every block = 1..TOTAL_BLOCKS
    // The following test will always fail if t_blockNum == 0, which is what we want for turning ALL LEDs off.
    if (i == t_blockNum) {  // Paint this block LED red
      pShiftRegister->digitalWrite(LEDBlueBlockPin[i - 1], HIGH);  // Blue LED off
      pShiftRegister->digitalWrite(LEDRedBlockPin[i - 1], LOW);    // Red LED on
    } else {                // Turn LED off
      pShiftRegister->digitalWrite(LEDBlueBlockPin[i - 1], HIGH);  // Blue LED off
      pShiftRegister->digitalWrite(LEDRedBlockPin[i - 1], HIGH);   // Red LED off
    }
  }
  return;
}

void Occupancy_LEDs::paintAllBlockOccupancyLEDs() {
  // Rev: 08/05/24.  FINISHED BUT NOT YET TESTED.
  // IMPORTANT: We can always modify this to accept a locoNum parm, and only re-paint for a given Train Progress record.  But that
  //            that would not be trivial, since we re-initialize every Block LED value at the beginning of this function.  So only
  //            worry about this if performance becomes an issue (and since this is OCC's only job, that seems unlikely, but we will
  //            have to see if it results in flashing LEDs when it's refreshed or other annoying behavior.)
  // Works for OCC AUTO/PARK (Running and Stopping) modes only, as it relies on an accurate Train Progress table.
  // Scan the ENTIRE Train Progress table, for all locos, and illuminate all RED "reserved" and BLUE "occupied" LEDs.
  // We will call this function in Auto/Park mode:
  //   1. When we first start Auto/Park
  //   2. Whenever an Extension or Continuation route is added in Auto/Park
  //   3. Whenever a Sensor state-change message (Tripped or Cleared) is received in Auto/Park
  // Since we'll only want to paint the entire control panel blue/red LEDs in Auto/Park mode, don't bother checking mode/state.
  // Always be sure the Train Progress "next to trip" pointer has been updated BEFORE calling this function to paint the LEDs.

  // Although Train Progress will include all "real" locos that have Reserved or Occupied blocks, it does NOT include STATIC
  // rolling stock that will also be occupying blocks.  Thus we must also scan m_staticBlock[] array for any STATIC reservations.

  // When Stopped, all of the Red/Blue LEDs should be off.
  // When Running in Manual mode, all of the Red/Blue LEDs should be off.
  // When Running in Registration mode, all of the Red/Blue LEDs should be off EXCEPT when prompting operator for the locoNum of
  //   equipment occupying a block -- and we will not use this function to light that single Red LED.
  // When Running/Stopping in Auto/Park modes, all Red and Blue LEDs will be illuminated appropriately, per Train Progress.

  // First, we fully populate an array m_newBlockStatus[] from scratch of how each Red/Blue LED should be lit.  This data will come
  // from both Train Progress and our private m_staticBlock[] array (for STATIC equipment.)
  // Then, for each element, we compare it to the old value m_oldBlockStatus[] and *if they are different* write to the LED.
  // This is because digitalWrite() is really slow, so we keep a second parallel array so we only write when something changes.

  // Eventually we might refine this so we don't have to do the entire process every time this function is called.  We'll call here
  // to paint every Block LED each time a sensor changes, each time Auto/Park mode starts, and each time a new Route is added in
  // Auto/Park mode.  Maybe we can get away with ONLY updating blocks that are part of this particular train's Train Progress
  // route -- not every single train.
  // But if this performance is ever an issue, I *think* this will work just fine if we limit ourselves to the loco that tripped
  // or cleared the sensor or got the new Route assigned.  So we could easily also pass the locoNum that tripped or cleared the
  // sensor, and *only* process blocks that are part of that route.  Not sure what impact that would have on m_oldBlockStatus[],
  // because what happens if we clear a sensor and have already moved Tail forward and have an orphan block that we won't see in
  // the route anymore?  If we did pass a t_locoNum parm, then when we first start Auto and Park modes, we could set up a loop for
  // every loco 1..50, call this function so that the entire panel could be painted for all trains.

  // So how do we determine how each Block Occupancy LED should be lit?  We can do the whole thing by scanning the Train Progress
  // table, and also the m_staticBlock[] array just for STATIC equipment.
  // There is a potential problem if a block occurs more than once in a Route, one block might be occupied and the other block is
  // just reserved.  Thus we will traverse each loco's Train Progress table from HEAD to TAIL, so if there are multiple occurrences
  // of a given block, we'll encounter the Occupied record last, and thus overwrite any previous Reserved status that could occur
  // in the same Train Progress record.
  // We'll have to do some testing to see if this method is fast enough to keep up with a lot of trains tripping and clearing
  // sensors pretty quickly.  If it's slow, just reduce to illuminating the sensors for one train at a time should work fine.

  // *** RESET THE ARRAY THAT WILL HOLD THE DISIRED COLOR OF EVERY RED/BLUE BLOCK OCCUPANCY LED ***
  initNewBlockStatus();  // Set every element of m_newBlockStatus[] to LED_OFF

  // ** SCAN m_staticBlock[] ARRAY FOR EVERY BLOCK THAT IS RESERVED FOR STATIC EQUIPMENT AND FLAG THAT BLOCK AS OCCUPIED ***
  // Any block that is reserved for STATIC will never be part of any Train Progress route.
  for (byte blockNum = 1; blockNum <= TOTAL_BLOCKS; blockNum++) {
    if (m_staticBlock[blockNum - 1] == true) {  // true means we have tagged this block as occupied by STATIC loco
      m_newBlockStatus[blockNum - 1] = LED_BLOCK_OCCUPIED;  // Element number = block number - 1
    }
  }

  // *** SCAN TRAIN PROGRESS FOR EVERY ACTIVE LOCO, AND POPULATE OUR ARRAY BASED ON WHAT IS RESERVED AND OCCUPIED ***
  // For every possible Train Progress record locoNum 1..TOTAL_TRAINS
  for (byte locoNum = 1; locoNum <= TOTAL_TRAINS; locoNum++) {

    // If this is an Active Train Progress table
    if (m_pTrainProgress->isActive(locoNum)) {

      // 1. All blocks between TAIL and NEXT-TO-TRIP should be illuminated in BLUE = Occupied.
      //    This will work even in cases where a loco is between occupancy sensors within a block.
      // 2. All blocks between NEXT-TO-TRIP and HEAD should be illuminated in RED = Reserved (but not Occupied.)
      // FYI, TAIL, NEXT-TO-TRIP, and HEAD pointers are guaranteed never to point to Block elements.

      // IMPORTANT: To maintain encapsulation, we should have function in Train Progress that return a list of both Reserved and
      // Occupied blocks for a given train.  But for now, let's just get this working.

      byte trainProgressWorkingPointer    = m_pTrainProgress->headPtr(locoNum);        // Start at HEAD and work to TAIL
      byte trainProgressNextToTripPointer = m_pTrainProgress->nextToTripPtr(locoNum);
      byte trainProgressTailPointer       = m_pTrainProgress->tailPtr(locoNum);
      bool foundNextToTrip = false;  // We'll set this true when workingPointer encounters the nextToTripPtr.  That's when blocks
                                     // change from being Reserved to being Occupied (when moving from HEAD to TAIL.)

      // Since HEAD does not point to *any* type of element, including a BE/BW block, we'll start at the first element behind it.
      trainProgressWorkingPointer = m_pTrainProgress->decrementTrainProgressPtr(trainProgressWorkingPointer);
      // Not important here, but FYI this pointer should now be pointing at the very front element, which should always be VL00.

      // Traverse this loco's Train Progress table and flag any blocks found as either Reserved or Occupied.
      while (trainProgressWorkingPointer != trainProgressTailPointer) {
        // Working our way from (HEAD - 1) back to TAIL.
        routeElement thisElement = m_pTrainProgress->peek(locoNum, trainProgressWorkingPointer);
        // We might grab any type of Train Progress element, such as SN Sensor, FD direction, TN turnout, VL velocity, etc, but all
        // we are interested in are BE and BW Block-type elements.

        // When we reach NextToTrip, the rest of the blocks we find will be RESERVED but not yet OCCUPIED.
        // It doesn't matter if we do this test and set foundNextToTrip=true either before or after the test for Block elements,
        // because the tests are mutually exclusive -- the NextToTripPointer will *never* point to a BE/BW Block element.
        if ((trainProgressWorkingPointer == trainProgressNextToTripPointer)) {
          foundNextToTrip = true;
        }

        // If we're pointing at a BE/BW Block element, we'll want to flag it as either Reserved or Occupied.
        if ((thisElement.routeRecType == BE) || (thisElement.routeRecType == BW)) {
          if (foundNextToTrip == false) {  // Still a RESERVED element; not yet occupied.
            m_newBlockStatus[thisElement.routeRecVal - 1] = LED_BLOCK_RESERVED;  // Element 0 = Block 1, etc.
          } else {  // We are behind nextToTrip and thus this block is OCCUPIED
            m_newBlockStatus[thisElement.routeRecVal - 1] = LED_BLOCK_OCCUPIED;  // Account for 0 vs 1 recNum vs blockNum
          }
        }

        // Now move along to the next element in the Train Progress table for this loco, until we reach the tail.
        trainProgressWorkingPointer = m_pTrainProgress->decrementTrainProgressPtr(trainProgressWorkingPointer);
      }  // Finished traversing this loco's Train Progress table
    }  // End of "is this an Active loco?"
  }  // Finished processing this loco; move on to the next until all loco's Train Progress tables have been examined.

  // WOW!  Now, m_newBlockStatus[] should be populated with LED_OFF, LED_BLOCK_OCCUPIED, and/or LED_BLOCK_RESERVED.  And these
  // flags should be accurate for the entire layout -- for all locos and STATIC equipment!  So that's all we need to accurately
  // re-paint all RED and BLUE BLOCK OCCUPANCY LEDs on the Control Panel!

// If we saved locoNum in addition to OFF, RESERVED, and OCCUPIED, then we'd have all we need to fully re-populate the Block Res'n
// table as well!  Maybe we should do this...

  // *** FINALLY, UPDATE EACH PHYSICAL RED/BLUE LEDs ON THE CONTROL PANEL ***
  // We'll include some extra tests to see if the status changed, so we won't re-illuminate an LED the same way it already was.
  // This is because if/then lookups are significantly faster than digitalWrite(), which is pretty slow.
  for (byte blockNum = 1; blockNum <= TOTAL_BLOCKS; blockNum++) {
    byte elementNum = blockNum - 1;
    if (m_oldBlockStatus[elementNum] != m_newBlockStatus[elementNum]) {   // If the LED status changed, write to Control Panel.
      if (m_newBlockStatus[elementNum] == LED_OFF) {
        pShiftRegister->digitalWrite(LEDRedBlockPin[elementNum], HIGH);   // Red LED off
        pShiftRegister->digitalWrite(LEDBlueBlockPin[elementNum], HIGH);  // Blue LED off
      } else if (m_newBlockStatus[elementNum] == LED_BLOCK_RESERVED) {    // RED LED = RESERVED (not occupied)
        pShiftRegister->digitalWrite(LEDRedBlockPin[elementNum], LOW);    // Red LED on
        pShiftRegister->digitalWrite(LEDBlueBlockPin[elementNum], HIGH);  // Blue LED off
      } else if (m_newBlockStatus[elementNum] == LED_BLOCK_OCCUPIED) {    // BLUE LED = OCCUPIED
        pShiftRegister->digitalWrite(LEDRedBlockPin[elementNum], HIGH);   // Red LED off
        pShiftRegister->digitalWrite(LEDBlueBlockPin[elementNum], LOW);   // Blue LED on
      }
    }
    // Either way, go ahead and set the "old" status to the new status.
    m_oldBlockStatus[elementNum] = m_newBlockStatus[elementNum];
  }
  return;
}

// *** PRIVATE FUNCTIONS ***

void Occupancy_LEDs::initSensorStatus() {  // Private
  // Rev: 03/19/23.  FINISHED BUT NOT YET TESTED.
  // WHITE OCCUPANCY SENSOR STATUS.  Tells us how each WHITE SENSOR LED *should* be lit.
  // Record corresponds to (sensor number - 1) = 0..51 for sensors 1..52.
  // White occupancy sensor 1 = Centipede pin 64
  // White occupancy sensor 64 = Centipede pin 127
  // Since we wired sensor #n to Centipede pin #n, we don't need an actual cross-reference for Centipede Pin Num <-> Sensor Num.
  // But don't overlook that Centipede pin numbers start at 0, and sensor numbers start at 1.
  // The following zeroes should really be {LED_OFF, LED_OFF, LED_OFF, ... } but just use zeroes here for the sake of brevity.
  for (byte i = 0; i < TOTAL_SENSORS; i++) {
    m_sensorStatus[i] = LED_OFF;
  }
  return;
}

void Occupancy_LEDs::initStaticBlocks() {  // Private
  // Rev: 03/20/23.  FINISHED BUT NOT YET TESTED.
  // Set all blocks to initially *not* occupied by STATIC equipment.
  // Array element corresponds to (block number - 1) = 0..25 for blocks 1..26.
  for (byte i = 0; i < TOTAL_BLOCKS; i++) {
    m_staticBlock[i] = false;
  }
  return;
}

void Occupancy_LEDs::initNewBlockStatus() {  // Private
  // Rev: 03/20/23.  FINISHED BUT NOT YET TESTED.
  // RED/BLUE BLOCK OCCUPANCY STATUS.  Tells us how each RED/BLUE BLOCK OCCUPANCY LED *should* be lit.
  // Array element corresponds to (block number - 1) = 0..25 for blocks 1..26.
  for (byte i = 0; i < TOTAL_BLOCKS; i++) {
    m_newBlockStatus[i] = LED_OFF;
  }
  return;
}

void Occupancy_LEDs::initOldBlockStatus() {  // Private
  // Rev: 03/20/23.  FINISHED BUT NOT YET TESTED.
  // "Previous" RED/BLUE BLOCK OCCUPANCY STATUS.  Tells us how each RED/BLUE BLOCK OCCUPANCY LED is *already* lit.
  // Record corresponds to (blockNum - 1) = 0..25 for blocks 1..26.
  for (byte i = 0; i < TOTAL_BLOCKS; i++) {
    m_oldBlockStatus[i] = LED_OFF;
  }
  return;
}
