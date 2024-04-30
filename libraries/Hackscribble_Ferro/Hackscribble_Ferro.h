// 6/29/22: Removed hard-coded "SS" from constructor and pass whatever chipSelect value we get passed from FRAM constructor.
// 6/27/22: Added support for MB85RS4MT 512KB (524288-bit) Adafruit FRAM breakout board.
// 6/21/21: Eliminated MAX_BUF_SIZE from solution; made _maxBufferSize a const (255.)  Re-wrote format().
// Connects Fujitsu Ferroelectric RAM (MB85RS range) to Arduino to add up to 256KB of fast, non-volatile storage.
// _maxBufferSize is only used as a test to confirm we are not sending some crazy-long buffer, but it can be any size and can be
// less than the const defined below, no problem.  Though max size should be 255 bytes since the size is held in a byte const.
//   This class does not reserve the buffer; it only receives a pointer to the buffer.  So if we want to reduce the buffer size to
//   save SRAM i.e. fetch a Route Reference record in two steps (since it's > 128 bytes long), that must be done by the CALLING
//   function and has nothing to do with this class.
// 11/2/20: Pretty much totally re-wrote Hackscribble_Ferro Library.
// 11/25/20: Changed max buffer size from 128 to 255 so we could load an entire Route Reference record.
//   We can make this buffer smaller and load Route Reference in two "read" operations if SRAM gets tight, or move this buffer into
//   QuadRAM heap memory, along with our pointers to all of the rest of the Class objects such as Deadlocks etc.

#ifndef HACKSCRIBBLE_FERRO_H
#define HACKSCRIBBLE_FERRO_H

#include "Arduino.h"
#include <SPI.h>

#define HS_SPI_DEFAULT_MODE			SPI_MODE0
#define HS_SPI_DEFAULT_CLOCK		SPI_CLOCK_DIV2

// MB85RS part numbers
enum ferroPartNumber
{
	MB85RS16 = 0,		// 2KB
	MB85RS64,		  	// 8KB
	MB85RS128A,			// 16KB older model
	MB85RS128B,			// 16KB newer model
	MB85RS256A,			// 32KB older model
	MB85RS256B,			// 32KB newer model
	MB85RS1MT,			// 128KB
	MB85RS2MT,			// 256KB
  MB85RS4MT,      // 512KB added 6/27/22 by RDP to support new Adafruit 512KB (524288-bit) FRAM
	numberOfPartNumbers
};
		
// MB85RS address lengths
enum ferroAddressLength
{
	ADDRESS16BIT = 0,
	ADDRESS24BIT,
	numberOfAddressLengths
};

// Result codes
enum ferroResult
{
	ferroOK = 0,
	ferroBadStartAddress,
	ferroBadNumberOfBytes,
	ferroBadFinishAddress,
	ferroArrayElementTooBig,
	ferroBadArrayIndex,
	ferroBadArrayStartAddress,
	ferroBadResponse,
	ferroPartNumberMismatch,
	ferroUnknownError = 99
};

class Hackscribble_Ferro {

	public:

		Hackscribble_Ferro(ferroPartNumber partNumber, byte chipSelect);  // Constructor
		ferroResult ferroBegin();
		ferroPartNumber getPartNumber();
		byte readProductID();
		unsigned long getBottomAddress();  // This will be zero
		unsigned long getTopAddress();     // For 2Mb/256KB this will be 262143 (256K - 1); for 4Mb/512KB this will be 524287 (512K - 1)
		ferroResult checkForFRAM();
		ferroResult format();
		ferroResult readFerro(unsigned long startAddress, byte numberOfBytes, byte* buffer);
		ferroResult writeFerro(unsigned long startAddress, byte numberOfBytes, byte* buffer);

	private:

		ferroPartNumber _partNumber;
		byte _chipSelect;
		volatile byte *_out;
		byte _bit;
		static boolean _spiIsRunning;
		// FRAM opcodes
		static const byte _WREN = 0x06;
		static const byte _WRDI = 0x04;
		static const byte _WRITE = 0x02;
		static const byte _READ = 0x03;
		static const byte _RDSR = 0x05;
		static const byte _WRSR = 0x01;
		static const byte _RDID = 0x9F;
		// Dummy write value for SPI read
		static const byte _dummy = 0x00;
		// ***** Set maximum size of buffer used to write to and read from FRAM *****
		// This is ONLY used as a test of the max number of bytes we can read/write i.e. the size of the buffer 
		// we pass from the calling program.  This does NOT reserve any memory here.
		// We use 255 rather than 256 because the buffer size is always expressed as a byte value (max value 255.)
		static const byte _maxBufferSize = 0xFF;  // Just a check that we aren't reading/writing > 255 bytes.
		unsigned long _topAddressForPartNumber[numberOfPartNumbers];
		ferroAddressLength _addressLengthForPartNumber[numberOfPartNumbers];
		ferroAddressLength _addressLength;
		byte _densityCode[numberOfPartNumbers];
		unsigned long _bottomAddress;
		unsigned long _topAddress;
		byte _readStatusRegister(void);
		void _writeStatusRegister(byte value);
		void _readMemory(unsigned long address, byte numberOfBytes, byte *buffer);
		void _writeMemory(unsigned long address, byte numberOfBytes, byte *buffer);
		void _initialiseSPI(void);
		void _initialiseCS(void);
		void _select();
		void _deselect();

};

#endif
