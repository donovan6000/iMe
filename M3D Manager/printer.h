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

// Printer details
#define PRINTER_VENDOR_ID 0x03EB
#define PRINTER_PRODUCT_ID 0x2404
#define PRINTER_BAUD_RATE 115200
#define DEFAULT_X_SPEED 3000
#define DEFAULT_Y_SPEED 3000
#define DEFAULT_Z_SPEED 90
#define DEFAULT_E_SPEED 345

// Firmware types
enum firmwareTypes {M3D, M3D_MOD, IME, UNKNOWN_FIRMWARE};

// Operating modes
enum operatingModes {BOOTLOADER, FIRMWARE};

// Fan types
enum fanTypes {HENGLIXIN = 0x01, LISTENER = 0x02, SHENZHEW = 0x03, XINYUJIE = 0x04, CUSTOM_FAN = 0xFE, NO_FAN = 0xFF};


// Function prototypes

/*
Name: Sleep microseconds
Purpose: Sleeps the specified amount of microseconds
*/
void sleepUs(uint64_t microseconds);


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
		bool connect(const string &serialPort = "", bool connectingToNewPrinter = true);
		
		/*
		Name: Disconnect
		Purpose: Disconnects from the printer
		*/
		void disconnect();
		
		/*
		Name: Is connected
		Purpose: Returns if printer is connected
		*/
		bool isConnected();
		
		/*
		Name: In mode
		Purpose: Return is the printer is in either bootloader or firmware mode
		*/
		bool inBootloaderMode();
		bool inFirmwareMode();
		
		/*
		Name: Switch to mode
		Purpose: Switches printer into either bootloader or firmware mode
		*/
		void switchToBootloaderMode();
		void switchToFirmwareMode();
		
		/*
		Name: Install firmware
		Purpose: Flashes the printer's firmware to the provided file
		*/
		bool installFirmware(const string &file);
		
		/*
		Name: Send request
		Purpose: Sends data to the printer
		*/
		bool sendRequestAscii(const char *data, bool checkForModeSwitching = true);
		bool sendRequestAscii(char data, bool checkForModeSwitching = true);
		bool sendRequestAscii(const string &data, bool checkForModeSwitching = true);
		bool sendRequestAscii(const Gcode &data);
		bool sendRequestRepetier(const Gcode &data);
		bool sendRequestRepetier(const char *data);
		bool sendRequestRepetier(const string &data);
		bool sendRequest(const Gcode &data);
		bool sendRequest(const char *data);
		bool sendRequest(const string &data);
		
		/*
		Name: Receive response
		Purpose: Receives data to the printer
		*/
		string receiveResponseAscii();
		string receiveResponseTerminated();
		string receiveResponse();
		
		/*
		Name: Get available serial ports
		Purpose: Returns all available serial ports
		*/
		vector<string> getAvailableSerialPorts();
		
		/*
		Name: Get current serial port
		Purpose: Returns printer's current serial port
		*/
		string getCurrentSerialPort();
		
		/*
		Name: Get status
		Purpose: Returns the printer's status
		*/
		string getStatus();
		
		/*
		Name: Update status
		Purpose: Thread that updates the printer's status in real time
		*/
		#ifdef WINDOWS
			DWORD
		#else
			void
		#endif
		updateStatus();
		
		/*
		Name: Get operating mode
		Purpose: Returns the printer's current operating mode
		*/
		operatingModes getOperatingMode();
		
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
		Name: Get firmware release
		Purpose: Returns the printer's firmware release number
		*/
		string getFirmwareRelease();
		
		/*
		Name: Set log function
		Purpose: Sets the function used to log the printer's actions
		*/
		void setLogFunction(function<void(const string &message)>);
		
		/*
		Name: Set fan type
		Purpose: Sets the printer to use the specified fan type
		*/
		bool setFanType(fanTypes fanType, bool logDetails = true);
		
		/*
		Name: Set extruder current
		Purpose: Sets the printer's extruder motor's current
		*/
		bool setExtruderCurrent(uint16_t current, bool logDetails = true);
	
	// Private
	private:
	
		// Try to acquire lock
		bool tryToAcquireLock();
	
		// Acquire lock
		void acquireLock();
		
		// Release lock
		void releaseLock();
		
		// Operating mode
		operatingModes operatingMode;
		
		// EEPROM
		string eeprom;
		
		// Firmware is valid
		bool firmwareValid;
		
		// Firmware version
		uint32_t firmwareVersion;
		
		// Serial number
		string serialNumber;
		
		// Firmware type
		firmwareTypes firmwareType;
		
		// Collect printer information
		bool collectPrinterInformation(bool logDetails = true);
		
		// EEPROM get int
		uint32_t eepromGetInt(uint16_t offset, uint8_t length);
		
		// EEPROM get float
		float eepromGetFloat(uint16_t offset, uint8_t length);
		
		// EEPROM get string
		string eepromGetString(uint16_t offset, uint8_t length);
		
		// EEPROM write int
		bool eepromWriteInt(uint16_t offset, uint8_t length, uint32_t value);
		
		// EEPROM write float
		bool eepromWriteFloat(uint16_t offset, uint8_t length, float value);
		
		// EEPROM write string
		bool eepromWriteString(uint16_t offset, uint8_t length, const string &value);
		
		// EEPROM keep float within range
		bool eepromKeepFloatWithinRange(uint16_t offset, uint8_t length, float min, float max, float defaultValue);
	
		// Write to EEPROM
		bool writeToEeprom(uint16_t address, const uint8_t *data, uint16_t length);
		bool writeToEeprom(uint16_t address, uint8_t data);
		
		// CRC32
		uint32_t crc32(int32_t offset, const uint8_t *data, int32_t count);
		
		// Get new serial port
		string getNewSerialPort(string lastAttemptedPort = "", string stopAtPort = "");
		
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
		
		// Log function
		function<void(const string &message)> logFunction;
		
		// Get firmware type from firmware version
		firmwareTypes getFirmwareTypeFromFirmwareVersion(uint32_t firmwareVersion);
		
		// Get firmware type as string
		string getFirmwareTypeAsString(firmwareTypes firmwareType);
		
		// Get firmware release from firmware version
		string getFirmwareReleaseFromFirmwareVersion(uint32_t firmwareVersion);
};


#endif
