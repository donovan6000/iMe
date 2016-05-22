// Header gaurd
#ifndef PRINTER_H
#define PRINTER_H


// Header files
#include <string>
#include <thread>
#include <mutex>
#ifdef WINDOWS
	#include <windows.h>
#endif
#include "gcode.h"

using namespace std;


// Definitions

// Firmware types
enum firmwareTypes {M3D, M3D_MOD, IME, UNKNOWN};


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
		bool connect(const string &serialPort = "", bool getEeprom = true);
		
		/*
		Name: Disconnect
		Purpose: Disconnects from the printer
		*/
		void disconnect();
		
		/*
		Name: Is Connected
		Purpose: Returns if printer is connected
		*/
		bool isConnected();
		
		/*
		Name: In Mode
		Purpose: Return is the printer is in either bootloader or firmware mode
		*/
		bool inBootloaderMode();
		bool inFirmwareMode();
		
		/*
		Name: Switch To Mode
		Purpose: Switches printer into either bootloader or firmware mode
		*/
		void switchToBootloaderMode();
		void switchToFirmwareMode();
		
		/*
		Name: Install Firmware
		Purpose: Flashes the printer's firmware to the provided file
		*/
		bool installFirmware(const string &file);
		
		/*
		Name: Send Request
		Purpose: Sends data to the printer
		*/
		bool sendRequestAscii(const char *data, bool checkForModeSwitching = true);
		bool sendRequestAscii(char data, bool checkForModeSwitching = true);
		bool sendRequestAscii(const string &data, bool checkForModeSwitching = true);
		bool sendRequestAscii(const Gcode &data);
		bool sendRequestBinary(const Gcode &data);
		bool sendRequestBinary(const char *data);
		bool sendRequestBinary(const string &data);
		bool sendRequest(const Gcode &data);
		bool sendRequest(const char *data);
		bool sendRequest(const string &data);
		
		/*
		Name: Receive Response
		Purpose: Receives data to the printer
		*/
		string receiveResponseAscii();
		string receiveResponseBinary();
		
		/*
		Name: Get Available Serial Ports
		Purpose: Returns all available serial ports
		*/
		vector<string> getAvailableSerialPorts();
		
		/*
		Name: Get Current Serial Port
		Purpose: Returns printer's current serial port
		*/
		string getCurrentSerialPort();
		
		/*
		Name: Get Status
		Purpose: Returns the printer's status
		*/
		string getStatus();
		
		/*
		Name: Update Status
		Purpose: Thread that updates the printer's status in real time
		*/
		#ifdef WINDOWS
			DWORD
		#else
			void
		#endif
		updateStatus();
		
		/*
		Name: Get serial number
		Purpose: Returns the printer's serial number
		*/
		string getSerialNumber();
		
		/*
		Name: Get firmware type
		Purpose: Returns the printer's firmware type
		*/
		string getFirmwareType();
		
		/*
		Name: Get firmware version
		Purpose: Returns the printer's firmware version
		*/
		string getFirmwareVersion();
	
	// Private
	private:
	
		// Acquire lock
		void acquireLock();
		
		// Release lock
		void releaseLock();
		
		// EEPROM
		string eeprom;
		
		// Firmware version
		uint32_t firmwareVersion;
		
		// Serial number
		string serialNumber;
		
		// Firmware type
		firmwareTypes firmwareType;
		
		// Read EEPROM
		bool readEeprom();
		
		// EEPROM get int
		uint32_t eepromGetInt(uint16_t offset, uint8_t length);
		
		// EEPROM get float
		float eepromGetFloat(uint16_t offset, uint8_t length);
		
		// EEPROM get string
		string eepromGetString(uint16_t offset, uint8_t length);
	
		// Write to EEPROM
		bool writeToEeprom(uint16_t address, const uint8_t *data, uint16_t length);
		bool writeToEeprom(uint16_t address, uint8_t data);
		
		// CRC32
		uint32_t crc32(int32_t offset, const uint8_t *data, int32_t count);
		
		// Get new serial port
		string getNewSerialPort();
		
		// update available serial ports
		void updateAvailableSerialPorts();
		
		// File descriptor
		#ifdef WINDOWS
			HANDLE
		#else
			int
		#endif
		fd;
		
		// Available serial ports
		vector<string> availableSerialPorts;
		
		// Current serial port
		string currentSerialPort;
		
		// Status
		string status;
		
		// Lock
		#ifdef WINDOWS
			HANDLE
		#else
			recursive_mutex
		#endif
		mutex;
		
		// Thread
		#ifdef WINDOWS
			HANDLE
		#else
			thread
		#endif
		updateStatusThread;
		bool stopThread;
};


#endif