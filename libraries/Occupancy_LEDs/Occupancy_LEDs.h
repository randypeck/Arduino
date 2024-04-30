// OCCUPANCY_LEDs.H Rev: 04/07/24.  FINISHED BUT NOT YET TESTED.
// Part of O_OCC.
// This class is responsible for lighting the Control Panel WHITE SENSOR OCCUPANCY SENSOR LEDs and BLUE/RED BLOCK OCCUPANCY LEDs.
// Illumination depends on Mode and State.
// For this class to function properly, it is critical to call updateSensorStatus() every time a sensor change occurs.

#ifndef OCCUPANCY_LEDS_H
#define OCCUPANCY_LEDS_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <Train_Progress.h>

class Occupancy_LEDs {

  public:

    Occupancy_LEDs();  // Constructor must be called above setup() so the object will be global to the module.

    void begin(Train_Progress* t_pTrainProgress);

    void updateSensorStatus(const byte t_sensorNum, const char t_tripOrClear);
    // Update our private sensor status array.  This does not change the illumination of any LEDs.
    // Appropriate for ALL MODES.  We will only receive Sensor Change messages when MAS deems them appropriate.
    // Call this function EVERY TIME a Sensor Change message is received, to keep our private m_sensorStatus[] array up to date.

    char getSensorStatus(const byte t_sensorNum);
    // Does NOT return actual sensor status; returns the status per our internal array which *should* match actual.
    // Returns SENSOR_STATUS_TRIPPED ('T') or SENSOR_STATUS_CLEARED ('C')

    void setBlockStatic(const byte t_blockNum);
    // Populated during Registration, used during Auto/Park when illuminating RED/BLUE BLOCK OCCUPANCY LEDs.

    void paintOneOccupancySensorLED(const byte t_sensorNum);
    // Originally for REGISTRATION mode only, unless turning ALL off, but now I think Reg mode will light all occupied.
    // Turn ONE WHITE OCCUPANCY SENSOR LED ON and turn all the others OFF.
    // Passing t_sensorNum == 0 will turn off ALL Occupancy Sensor LEDs (such as when initially powering up the system.)

    void paintAllOccupancySensorLEDs(const byte t_mode, const byte t_state);
    // Works for ALL MODES.  Assumes private m_sensorStatus[] array reflects current state of all Occupancy Sensors.
    // To turn ALL WHITE OCCUPANCY SENSORS OFF, simply pass t_state == STATE_STOPPED.

    void paintOneBlockOccupancyLED(const byte t_blockNum);
    // REGISTRATION mode only, unless turning ALL off.  Can only turn on RED LEDs for now, because that's all we need it for.
    // Turn ONE RED BLOCK OCCUPANCY LED ON and all others OFF.
    // Passing t_blockNum == 0 will turn off ALL Block Occupancy LEDs (such as when initially powering up the system.)

    void paintAllBlockOccupancyLEDs();
    // AUTO/PARK RUNNING/STOPPING modes only.
    // Scan Train Progress and illuminate all RED "occupied" and BLUE "reserved" LEDs.  We will ONLY call this function when we
    // first start Auto or Park mode (Park, because we can't guarantee that we will run Auto mode before starting Park mode) AND
    // whenever a Sensor state-change message is received when Auto/Park is Running/Stopping.  This is the only time that we'll
    // want to paint the entire control panel blue/red LEDs.  So we won't even bother checking mode and state here.

  private:

    void initSensorStatus();
    // Clear the array that represents the status of each WHITE OCCUPANCY SENSOR LED.  This should only be done before all sensor
    // statuses will be sent, via updateSensorStatus(), when MANUAL or REGISTRATION modes start.

    void initStaticBlocks();
    // Set all blocks to initially *not* occupied by STATIC equipment.  Populated during Registration.

    void initNewBlockStatus();
    void initOldBlockStatus();
    // These two Block Status arrays are only used by paintAllBlockOccupancyLEDs(), which is only called when a sensor status

    const byte LED_OFF              =   0;  // Used by Sensor Status and Block Status arrays.
    const byte LED_SENSOR_OCCUPIED  =   1;  // Used by Sensor Status array.
    const byte LED_BLOCK_OCCUPIED   =   1;  // Used by Block Status array.
    const byte LED_BLOCK_RESERVED   =   2;  // Used by Block Status array.

    // WHITE OCCUPANCY SENSOR STATUS.  Tells us how each WHITE SENSOR LED *should* be lit.
    // Array element corresponds to (sensor number - 1) = 0..51 for sensors 1..52.
    // White occupancy sensor  1 = Centipede pin  64
    // White occupancy sensor 64 = Centipede pin 127
    // Write a ZERO to a bit to turn ON the LED, write a ONE to a bit to turn OFF the LED.
    byte m_sensorStatus[TOTAL_SENSORS];

    // STATIC BLOCK bool array defined during Registration will be true for every block that's occupied by STATIC equipment.
    bool m_staticBlock[TOTAL_BLOCKS];

    // RED/BLUE LED BLOCK STATUS.  Tells us how each RED/BLUE BLOCK OCCUPANCY LED *should* be lit and *were* lit.
    // We'll compare new with old to see if we can avoid individual LED digitalWrite() commands to save time.
    // Array element corresponds to (block number - 1) = 0..25 for blocks 1..26.
    // Notice there are TWO Centipede pins for each two-color LED (one for RED and one for BLUE.)
    byte m_newBlockStatus[TOTAL_BLOCKS];  // This will store how LEDs should be illuminated
    byte m_oldBlockStatus[TOTAL_BLOCKS];  // This will store how they have previously been illuminated

    // RED/BLUE control panel block reservation and occupancy LEDs connect to Centipede 1 (address 0.)
    // WHITE control panel occupancy-indication LEDs connect to Centipede 2 (address 1.)

    // RED/BLUE LED CENTIPEDE PIN NUMBER <-> BLOCK NUMBER CROSS REFERENCE.
    // CENTIPEDE #1, Address 0: RED/BLUE BLOCK INDICATOR LEDS
    // Centipede pin  0 corresponds with block  1 blue
    // Centipede pin 15 corresponds with block 16 blue
    // Centipede pin 16 corresponds with block  1 red
    // Centipede pin 31 corresponds with block 16 red
    // Centipede pin 32 corresponds with block 17 blue
    // Centipede pin 47 corresponds with block 32 blue  // though there are only 26
    // Centipede pin 48 corresponds with block 17 red
    // Centipede pin 63 corresponds with block 32 red   // though there are only 26
    // 26 bytes 0..25 = Blocks 1..26 (blue)
    const byte LEDBlueBlockPin[TOTAL_BLOCKS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,32,33,34,35,36,37,38,39,40,41};
    // 26 bytes 0..25 = Blocks 1..26 (red)
    const byte LEDRedBlockPin[TOTAL_BLOCKS]  = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,48,49,50,51,52,53,54,55,56,57};

    Train_Progress*    m_pTrainProgress;     // To find Reserved and Occupied blocks for each loco.

};

#endif
