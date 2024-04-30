// SENSOR_BLOCK.H Rev: 01/26/23.  TESTED AND WORKING.
// ONE RECORD FOR EACH SENSOR.
// 12/14/20: Added sensor status field Tripped/Cleared get/set for modules *other than* SNS.
// A set of functions to retrieve data from the Sensor Block cross reference table, which is stored in FRAM.

#ifndef SENSOR_BLOCK_H
#define SENSOR_BLOCK_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>
#include <FRAM.h>

class Sensor_Block {

  public:

    Sensor_Block();  // Constructor must be called above setup() so the object will be global to the module.
    void begin(FRAM* t_pStorage);  // Called at beginning of registration.

    // These functions expect sensorNum to start at 1!  And return block numbers starting at 1.
    byte getSensorNumber(const byte t_sensorNum);

    byte whichBlock(const byte t_sensorNum);  // 1..n
    char whichEnd(const byte t_sensorNum);    // East or West
    char getStatus(const byte t_sensorNum);   // Tripped or Cleared
    void setStatus(const byte t_sensorNum, const char t_sensorStatus);  // Set sensorNum to Tripped or Cleared

    void display(const byte t_sensorNum);  // Display a single record to Serial COM.
    void populate();  // Special utility reads hard-coded data, writes records to FRAM.

  private:

    void getSensorBlock(const byte t_sensorNum);
    void setSensorBlock(const byte t_sensorNum);

    unsigned long sensorBlockAddress(const byte t_sensorNum);  // Return the FRAM address of the *record* in the Sensor Block X-ref table for this sensor.

    // SENSOR BLOCK CROSS REFERENCE TABLE.  This struct is only known inside the class; calling routines use getters and setters, never direct struct access.
    struct sensorBlockStruct {
      byte sensorNum;             // Actual sensor number 1..52 (not 0..51)
      byte blockNum;              // Actual block number 1..26 (not 0..25)
      char whichEnd;              // [E|W] = SENSOR_END_EAST | SENSOR_END_WEST
      char status;                // [T|C] = SENSOR_STATUS_TRIPPED, SENSOR_STATUS_CLEARED

    };
    sensorBlockStruct m_sensorBlock;

    FRAM* m_pStorage;           // Pointer to the FRAM memory module

};

#endif
