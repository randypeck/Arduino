// 06/30/22: FRAM duplication utility will require us to use digitalWrite(_chipSelect, LOW/HIGH) to select/de-select chips.
//           If we were only using one FRAM, we could keep it selected all the time and run about 14% faster, but in order to
//           support "Duplicate a FRAM" util with two FRAMs, we need to select/deselect each chip as we read and write.
// 06/28/22: Added support for MB85RS4MT 512KB (524288-bit) Adafruit FRAM breakout board.
// 06/28/22: Un-commented digitalWrite(_chipSelect, LOW/HIGH) before/after _select and _deselect
// 11/2/20: Pretty much totally re-wrote.
// Deleted "control block" logic, invented by author of the original Hackscribble driver; not a feature of FRAM.  Wasn't needed,
//   and dictated the size of the read/write buffer (which was originally 128 bytes.)  Now the read/write buffer size is limited
//   only by the maximum value of a byte, which is 255 (because buffer size is passed as a byte value, not an int value, and that
//   *is* built-in to the FRAM.)
// Note that the buffer is passed here as a pointer; nothing here dictates buffer size except that max length is 255 bytes.

// Hackscribble_Ferro Library
// ==========================
// Connects Fujitsu Ferroelectric RAM (MB85RS range) to your Arduino to add up to 512KB of fast, non-volatile storage.

// For information on how to install and use the library, read "Hackscribble_Ferro user guide.md".

// Created on 18 April 2014 By Ray Benitez.  Last modified on 29 September 2014 By Ray Benitez.
// Change history in "README.md"
// git@hackscribble.com | http://www.hackscribble.com | http://www.twitter.com/hackscribble
 
#include "wiring_digital.c"
#include "Arduino.h"
#include "Hackscribble_Ferro.h"

boolean Hackscribble_Ferro::_spiIsRunning = false;

Hackscribble_Ferro::Hackscribble_Ferro(ferroPartNumber partNumber, byte chipSelect) {  // Constructor
  _partNumber                             = partNumber;
  _chipSelect                             = chipSelect;
	_topAddressForPartNumber[MB85RS16]		  = 0x0007FFUL;
	_topAddressForPartNumber[MB85RS64]		  = 0x001FFFUL;
	_topAddressForPartNumber[MB85RS128A]	  = 0x003FFFUL;
	_topAddressForPartNumber[MB85RS128B]	  = 0x003FFFUL;
	_topAddressForPartNumber[MB85RS256A]	  = 0x007FFFUL;
	_topAddressForPartNumber[MB85RS256B]	  = 0x007FFFUL;
	_topAddressForPartNumber[MB85RS1MT]		  = 0x01FFFFUL;
	_topAddressForPartNumber[MB85RS2MT]		  = 0x03FFFFUL;
  _topAddressForPartNumber[MB85RS4MT]     = 0x07FFFFUL;    // Added to support RS4MT 512KB / 524288-bit FRAM
	_addressLengthForPartNumber[MB85RS16]	  = ADDRESS16BIT;
	_addressLengthForPartNumber[MB85RS64]	  = ADDRESS16BIT;
	_addressLengthForPartNumber[MB85RS128A]	= ADDRESS16BIT;
	_addressLengthForPartNumber[MB85RS128B]	= ADDRESS16BIT;
	_addressLengthForPartNumber[MB85RS256A]	= ADDRESS16BIT;
	_addressLengthForPartNumber[MB85RS256B]	= ADDRESS16BIT;
	_addressLengthForPartNumber[MB85RS1MT]	= ADDRESS24BIT;
	_addressLengthForPartNumber[MB85RS2MT]	= ADDRESS24BIT;
  _addressLengthForPartNumber[MB85RS4MT]	= ADDRESS24BIT;  // Added to support RS4MT 512KB / 524288-bit FRAM
	_densityCode[MB85RS16]					        = 0x00;          // RDID not supported
	_densityCode[MB85RS64]					        = 0x00;          // RDID not supported
	_densityCode[MB85RS128A]				        = 0x00;          // RDID not supported
	_densityCode[MB85RS128B]				        = 0x04;          // Density code in bits 4..0
	_densityCode[MB85RS256A]			        	= 0x00;          // RDID not supported
	_densityCode[MB85RS256B]		        		= 0x05;          // Density code in bits 4..0
	_densityCode[MB85RS1MT]		        			= 0x07;          // Density code in bits 4..0
	_densityCode[MB85RS2MT]					        = 0x08;          // Density code in bits 4..0
	_densityCode[MB85RS4MT]	        				= 0x09;          // Density code in bits 4..0 Per data sheet page 10
	_bottomAddress                          = 0x000000;
	_topAddress                             = _topAddressForPartNumber[_partNumber];
	_addressLength                          = _addressLengthForPartNumber[_partNumber];
}

//
// PLATFORM SPECIFIC, LOW LEVEL METHODS
//

void Hackscribble_Ferro::_initialiseSPI(void)
{
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode (HS_SPI_DEFAULT_MODE);
	SPI.setClockDivider(HS_SPI_DEFAULT_CLOCK);
}
	
void Hackscribble_Ferro::_initialiseCS()
// Rev: 06/28/22 by RDP: Added pinMode(_chipSelect, OUTPUT) and removed from Train_Functions.cpp
{	
	pinMode (SS, OUTPUT); 	// Set the standard SS pin as an output to keep Arduino SPI happy
	byte timer = digitalPinToTimer(_chipSelect);
	_bit = digitalPinToBitMask(_chipSelect);
	byte port = digitalPinToPort(_chipSelect);
	if (port == NOT_A_PIN) return;
	// If the pin that support PWM output, we need to turn it off before doing a digital write.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);
	_out = portOutputRegister(port);
	pinMode (_chipSelect, OUTPUT);
	_deselect();
}

void Hackscribble_Ferro::_select()
{
  digitalWrite(_chipSelect, LOW);
	byte oldSREG = SREG;
	cli();
	*_out &= ~_bit;
	SREG = oldSREG;
}

void Hackscribble_Ferro::_deselect()
{
	digitalWrite(_chipSelect, HIGH);
	byte oldSREG = SREG;
	cli();
	*_out |= _bit;
	SREG = oldSREG;
}

byte Hackscribble_Ferro::_readStatusRegister()
{
	byte rxByte = 0;
	_select();
	SPI.transfer(_RDSR);
	rxByte = SPI.transfer(_dummy);
	_deselect();
	return rxByte;
}


void Hackscribble_Ferro::_writeStatusRegister(byte value)
{
	_select();
	SPI.transfer(_WREN);
	_deselect();
	_select();
	SPI.transfer(_WRSR);
	SPI.transfer(value);
	_deselect();
}


void Hackscribble_Ferro::_readMemory(unsigned long address, byte numberOfBytes, byte *buffer)
{
	_select();
	SPI.transfer(_READ);
	if (_addressLength == ADDRESS24BIT)
	{
		SPI.transfer(address / 65536);
	}
	SPI.transfer(address / 256);
	SPI.transfer(address % 256);
	for (byte i = 0; i < numberOfBytes; i++)
	{
		buffer[i] = SPI.transfer(_dummy);
	}
	_deselect();
}


void Hackscribble_Ferro::_writeMemory(unsigned long address, byte numberOfBytes, byte *buffer)
{
	_select();
	SPI.transfer(_WREN);
	_deselect();
	_select();
	SPI.transfer(_WRITE);
	if (_addressLength == ADDRESS24BIT)
	{
		SPI.transfer(address / 65536);
	}
	SPI.transfer(address / 256);
	SPI.transfer(address % 256);
	for (byte i = 0; i < numberOfBytes; i++)
	{
		SPI.transfer(buffer[i]);
	}
	_deselect();
}


byte Hackscribble_Ferro::readProductID()
{
	// If the device supports RDID, returns density code (bits 4..0 of third byte of RDID response)
	// Otherwise, returns 0
	const byte densityMask = 0x1F; // Density code in bits 4..0
	byte densityCodeByte = 0x00;
	if (_densityCode[_partNumber] != 0x00)
	{
		_select();
		SPI.transfer(_RDID);
		SPI.transfer(_dummy); // Discard first two bytes
		SPI.transfer(_dummy); //
		densityCodeByte = SPI.transfer(_dummy) & densityMask;
		_deselect();
	}
	return densityCodeByte;
}

//
// PLATFORM INDEPENDENT, HIGH LEVEL METHODS
//

ferroResult Hackscribble_Ferro::ferroBegin()
{
	_initialiseCS();  // Added pinMode(_chipSelect, OUTPUT) to this function on 6/28/22
	if (!_spiIsRunning)
	{
		_initialiseSPI();
		_spiIsRunning = true;
	}	
	return checkForFRAM();
}
	
ferroPartNumber Hackscribble_Ferro::getPartNumber()
{
	return _partNumber;
}

unsigned long Hackscribble_Ferro::getBottomAddress()
{
	return _bottomAddress;
}


unsigned long Hackscribble_Ferro::getTopAddress()
{
	return _topAddress;
}

ferroResult Hackscribble_Ferro::checkForFRAM()
{
	// Tests that the unused status register bits can be read, inverted, written back and read again
	// If RDID is supported, tests that Product ID reported by device matches part number
	const byte srMask = 0x70; // Unused bits are bits 6..4
	byte registerValue = 0;
	byte newValue = 0;
	// Read current value
	registerValue = _readStatusRegister();
	// Invert current value
	newValue = registerValue ^ srMask;
	// Write new value
	_writeStatusRegister(newValue);
	// Read again
	registerValue = _readStatusRegister();
	if (readProductID() != _densityCode[_partNumber])
	{
		return ferroPartNumberMismatch;
	}
	else if (((registerValue & srMask) != (newValue & srMask)))
	{
		return ferroBadResponse;
	}
	else
	{
		return ferroOK;
	}
}

ferroResult Hackscribble_Ferro::readFerro(unsigned long startAddress, byte numberOfBytes, byte *buffer)
{
	// Copies numberOfBytes bytes from FRAM (starting at startAddress) into buffer (starting at 0)
	// Returns result code
	// Validations:
	//		_bottomAddress <= startAddress <= _topAddress
	//		0 < numberOfBytes <= maxBuffer
	//		startAddress + numberOfBytes <= _topAddress
	if ((startAddress < _bottomAddress) || (startAddress > _topAddress))
	{
		return ferroBadStartAddress;
	}
	// Since maxBufferSize is 255, it's impossible for this test to fail (since numberOfBytes is a byte value; must be < 256)
	if ((numberOfBytes > _maxBufferSize) || (numberOfBytes == 0))
	{
		return ferroBadNumberOfBytes;
	}
	if ((startAddress + numberOfBytes - 1) > _topAddress)
	{
		return ferroBadFinishAddress;
	}
	_readMemory(startAddress, numberOfBytes, buffer);
	return ferroOK;
}

ferroResult Hackscribble_Ferro::writeFerro(unsigned long startAddress, byte numberOfBytes, byte *buffer)
{
	// Copies numberOfBytes bytes from buffer (starting at 0) into FRAM (starting at startAddress)
	// Returns result code
	// Validations:
	//		_bottomAddress <= startAddress <= _topAddress
	//		0 < numberOfBytes <= maxBuffer
	//		startAddress + numberOfBytes - 1 <= _topAddress
	if ((startAddress < _bottomAddress) || (startAddress > _topAddress))
	{
		return ferroBadStartAddress;
	}
	if ((numberOfBytes > _maxBufferSize) || (numberOfBytes == 0))
	{
		return ferroBadNumberOfBytes;
	}
	if ((startAddress + numberOfBytes - 1) > _topAddress)
	{
		return ferroBadFinishAddress;
	}
	_writeMemory(startAddress, numberOfBytes, buffer);
	return ferroOK;
}

ferroResult Hackscribble_Ferro::format()
{
	// 6/21/21: Fills FRAM with 0s.
	// Returns result code from ferroWrite function, or ferroOK if format is successful
	byte dataToWrite = 0;  // Fill FRAM with this value
	ferroResult result = ferroOK;
	for (unsigned long i = _bottomAddress; i < _topAddress; i++)	{
		result = writeFerro(i, 1, &dataToWrite);
		if (result != ferroOK) {
			return result;
		}
	}
	return result;
}

