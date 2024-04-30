// Rev: 10/06/20.
// Stripped-down version of the standard library. No changes, just cut out unused code.
//Digole Digital Solutions: www.digole.com
#include "DigoleSerial.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"

/*
// Communication set up command
 * "SB":Baud (ascII bytes end with 0x00/0x0A/0x0D) -- set UART Baud Rate
 * "DC":1/0(1byte) -- set config display on/off, if set to 1, displayer will display current commucation setting when power on
// Text Function command
 * "CL": -- Clear screen--OK
 * "CS":1/0 (1 byte)-- Cursor on/off
 * "TP":x(1 byte) y(1 byte) -- set text position
 * "TT":string(bytes) end with 0x00/0x0A/0x0D -- display string under regular mode
 */

// Note that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

//UART function
void DigoleSerialDisp::write2B(unsigned int v) {  // Used by setPrintPos()
    if (v < 255)
        write(v);
    else {
        write(255);
        write(v - 255);
    }
}

void DigoleSerialDisp::preprint(void) {  // Used by print() and println() (see DigoleSerial.h)
    //write((uint8_t)0);
    writeStr("TT");
}

void DigoleSerialDisp::setPrintPos(unsigned int x, unsigned int y, uint8_t graph) {
    if (graph == 0) {
        writeStr("TP");
        write2B(x);
        write2B(y);
    } else {
        writeStr("GP");
        write2B(x);
        write2B(y);
    }
}

void DigoleSerialDisp::setLCDColRow(uint8_t col, uint8_t row) {
    writeStr("STCR");
    write(col);
    write(row);
    writeStr("\x80\xC0\x94\xD4");
}

void DigoleSerialDisp::backLightOn(void) {
    writeStr("BL");
    write((uint8_t) 99);
}

void DigoleSerialDisp::backLightOff(void) {
    writeStr("BL");
    write((uint8_t) 0);
}
