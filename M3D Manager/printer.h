// Header gaurd
#ifndef PRINTER_H
#define PRINTER_H


// Header files
#include <string>
#ifdef WINDOWS
	#include <windows.h>
#endif
#include "gcode.h"

using namespace std;


// Class
class Printer {

	// Public
	public:
	
		/*
		Name: Constructor
		Purpose: Initializes the variables
		*/
		Printer();
		
		/*
		Name: Destructor
		Purpose: Uninitializes the variables
		*/
		~Printer();
	
		/*
		Name: Connect
		Purpose: Connects or reconnects to the printer
		*/
		bool connect(const string &serialPort = "");
		
		/*
		Name: Is Bootloader Mode
		Purpose: Return is the printer is in bootloader mode
		*/
		bool isBootloaderMode();
		
		/*
		Name: Update Firmware
		Purpose: Updates the printer's firmware to the provided file
		*/
		bool updateFirmware(const string &file);
		
		/*
		Name: Send Request
		Purpose: Sends data to the printer
		*/
		bool sendRequestAscii(char data);
		bool sendRequestAscii(const char *data);
		bool sendRequestAscii(const Gcode &data);
		bool sendRequestBinary(const char *data);
		bool sendRequestBinary(const Gcode &data);
		
		/*
		Name: Receive Response
		Purpose: Receives data to the printer
		*/
		string receiveResponseAscii();
		string receiveResponseBinary();
	
	// Private
	private:
	
		// Write to eeprom
		bool writeToEeprom(uint16_t address, const uint8_t *data, uint16_t length);
		bool writeToEeprom(uint16_t address, uint8_t data);
		
		// CRC32
		uint32_t crc32(int32_t offset, const uint8_t *data, int32_t count);
		
		// Save serial ports
		void saveSerialPorts();
		
		// Get serial port
		string getSerialPort();
		
		// File descriptor
		#ifdef WINDOWS
			HANDLE fd;
		#else
			int fd;
		#endif
		
		// Serial ports
		vector<string> serialPorts;
		string currentSerialPort;
};


#endif
