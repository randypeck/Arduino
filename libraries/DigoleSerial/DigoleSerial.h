// Rev: 10/06/20.
// Stripped-down version of the standard library. No changes, just cut out unused code.
//Digole Digital Solutions: www.digole.com
#ifndef DIGOLESERIALDISP_H
#define DIGOLESERIALDISP_H

#include <inttypes.h>
#include "Print.h"
#include <Wire.h>
#include "Arduino.h"
//#include <avr/wdt.h>
// Communication set up command
// Text function command
// Graph function command
#define Serial_UART 0
#define Serial_I2C 1
#define Serial_SPI 2
#define _TEXT_ 0
#define _GRAPH_ 1
#ifdef FLASH_CHIP   //if writing to flash chip
#define _WRITE_DELAY 40
#else   //if writing to internal flash
#define _WRITE_DELAY 100
#endif
#ifndef Ver
#define Ver 32
#endif

class Digole {
  public:

    //virtual size_t read1(void);        // 11/15/22 Why are these two functions here?  They're not in the .cpp.
    //virtual void cleanBuffer(void);    // Just tried commenting them out to see if I get any errors.

};

class DigoleSerialDisp : public Print, public Digole {
  public:
#if defined(_Digole_Serial_UART_)

    DigoleSerialDisp(HardwareSerial *s, unsigned long baud) //UART Constructor set up  
    {
        _mySerial = s;
        _Baud = baud;
    }

    size_t write(uint8_t value) {
        _mySerial->write((uint8_t) value);
        return 1; // assume sucess
    }

    void digoleBegin(void) {
        _mySerial->begin(9600);
        _mySerial->print("SB");
        _mySerial->println(_Baud);
        delay(100);
        _mySerial->begin(_Baud);
    }

    size_t read1(void) {
        int t;
//        wdt_disable();
        do {
            t = _mySerial->read();
        } while (t == -1);
        return t;
    }
    void cleanBuffer(void)  //clean UART receiving buffer
    {
        while(_mySerial->read()!=-1);
    }
#endif

    /*---------fucntions for Text and Graphic LCD adapters---------*/
    void writeStr(const char *s) {
        int i = 0;
        while (s[i] != 0)
            write(s[i++]);
    }

    void disableCursor(void) {
        writeStr("CS0");
    }

    void enableCursor(void) {
        writeStr("CS1");
    }

    void clearScreen(void) {
        writeStr("CL");
    }

  //print function

    void println(const __FlashStringHelper *v) {
        preprint();
        Print::println(v);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(const String &v) {
        preprint();
        Print::println(v);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(const char v[]) {
        preprint();
        Print::println(v);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(char v) {
        preprint();
        Print::println(v);
        Print::println("\x0dTRT");
    }

    void println(unsigned char v, int base = DEC) {
        preprint();
        Print::println(v, base);
        Print::println("\x0dTRT");
    }

    void println(int v, int base = DEC) {
        preprint();
        Print::println(v, base);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(unsigned int v, int base = DEC) {
        preprint();
        Print::println(v, base);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(long v, int base = DEC) {
        preprint();
        Print::println(v, base);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(unsigned long v, int base = DEC) {
        preprint();
        Print::println(v, base);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(double v, int base = 2) {
        preprint();
        Print::println(v, base);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(const Printable& v) {
        preprint();
        Print::println(v);
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void println(void) {
        write((uint8_t) 0);
        writeStr("TRT");
    }

    void print(const __FlashStringHelper *v) {
        preprint();
        Print::print(v);
        write((uint8_t) 0);
    }

    void print(const String &v) {
        preprint();
        Print::print(v);
        write((uint8_t) 0);
    }

    void print(const char v[]) {
        preprint();
        Print::print(v);
        write((uint8_t) 0);
    }

    void print(char v) {
        preprint();
        Print::print(v);
        write((uint8_t) 0);
    }

    void print(unsigned char v, int base = DEC) {
        preprint();
        Print::print(v, base);
        write((uint8_t) 0);
    }

    void print(int v, int base = DEC) {
        preprint();
        Print::print(v, base);
        write((uint8_t) 0);
    }

    void print(unsigned int v, int base = DEC) {
        preprint();
        Print::print(v, base);
        write((uint8_t) 0);
    }

    void print(long v, int base = DEC) {
        preprint();
        Print::print(v, base);
        write((uint8_t) 0);
    }

    void print(unsigned long v, int base = DEC) {
        preprint();
        Print::print(v, base);
        write((uint8_t) 0);
    }

    void print(double v, int base = 2) {
        preprint();
        Print::print(v, base);
        write((uint8_t) 0);
    }

    void print(const Printable& v) {
        preprint();
        Print::print(v);
        write((uint8_t) 0);
    }
    void preprint(void);
    void backLightOn(void); //Turn on back light
    void backLightOff(void); //Turn off back light
    void setPrintPos(unsigned int x, unsigned int y, uint8_t = 0);
    void setLCDColRow(uint8_t col, uint8_t row);

    void write2B(unsigned int v);

private:
    unsigned long _Baud;
    HardwareSerial *_mySerial;
};

#endif
