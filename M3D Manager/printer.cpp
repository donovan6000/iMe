// Header files
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#ifdef WINDOWS
	#include <windows.h>
	#include <tchar.h>
	#include <setupapi.h>
#else
	#include <termios.h>
#endif
#ifdef OSX
	#include <CoreFoundation/CoreFoundation.h>
	#include <IOKit/IOKitLib.h>
	#include <IOKit/serial/IOSerialKeys.h>
	#include <IOKit/IOBSD.h>
	#include <sys/param.h>
#endif
#include "printer.h"

using namespace std;



// Definitions

// Chip details
#define CHIP_NAME ATxmega32C4
#define CHIP_PAGE_SIZE 0x80
#define CHIP_NRWW_SIZE 0x20
#define CHIP_NUMBER_OF_PAGES 0x80
#define CHIP_TOTAL_MEMORY CHIP_NUMBER_OF_PAGES * CHIP_PAGE_SIZE * 2

// ROM decryption and encryption tables
const uint8_t romDecryptionTable[] = {0x26, 0xE2, 0x63, 0xAC, 0x27, 0xDE, 0x0D, 0x94, 0x79, 0xAB, 0x29, 0x87, 0x14, 0x95, 0x1F, 0xAE, 0x5F, 0xED, 0x47, 0xCE, 0x60, 0xBC, 0x11, 0xC3, 0x42, 0xE3, 0x03, 0x8E, 0x6D, 0x9D, 0x6E, 0xF2, 0x4D, 0x84, 0x25, 0xFF, 0x40, 0xC0, 0x44, 0xFD, 0x0F, 0x9B, 0x67, 0x90, 0x16, 0xB4, 0x07, 0x80, 0x39, 0xFB, 0x1D, 0xF9, 0x5A, 0xCA, 0x57, 0xA9, 0x5E, 0xEF, 0x6B, 0xB6, 0x2F, 0x83, 0x65, 0x8A, 0x13, 0xF5, 0x3C, 0xDC, 0x37, 0xD3, 0x0A, 0xF4, 0x77, 0xF3, 0x20, 0xE8, 0x73, 0xDB, 0x7B, 0xBB, 0x0B, 0xFA, 0x64, 0x8F, 0x08, 0xA3, 0x7D, 0xEB, 0x5C, 0x9C, 0x3E, 0x8C, 0x30, 0xB0, 0x7F, 0xBE, 0x2A, 0xD0, 0x68, 0xA2, 0x22, 0xF7, 0x1C, 0xC2, 0x17, 0xCD, 0x78, 0xC7, 0x21, 0x9E, 0x70, 0x99, 0x1A, 0xF8, 0x58, 0xEA, 0x36, 0xB1, 0x69, 0xC9, 0x04, 0xEE, 0x3B, 0xD6, 0x34, 0xFE, 0x55, 0xE7, 0x1B, 0xA6, 0x4A, 0x9A, 0x54, 0xE6, 0x51, 0xA0, 0x4E, 0xCF, 0x32, 0x88, 0x48, 0xA4, 0x33, 0xA5, 0x5B, 0xB9, 0x62, 0xD4, 0x6F, 0x98, 0x6C, 0xE1, 0x53, 0xCB, 0x46, 0xDD, 0x01, 0xE5, 0x7A, 0x86, 0x75, 0xDF, 0x31, 0xD2, 0x02, 0x97, 0x66, 0xE4, 0x38, 0xEC, 0x12, 0xB7, 0x00, 0x93, 0x15, 0x8B, 0x6A, 0xC5, 0x71, 0x92, 0x45, 0xA1, 0x59, 0xF0, 0x06, 0xA8, 0x5D, 0x82, 0x2C, 0xC4, 0x43, 0xCC, 0x2D, 0xD5, 0x35, 0xD7, 0x3D, 0xB2, 0x74, 0xB3, 0x09, 0xC6, 0x7C, 0xBF, 0x2E, 0xB8, 0x28, 0x9F, 0x41, 0xBA, 0x10, 0xAF, 0x0C, 0xFC, 0x23, 0xD9, 0x49, 0xF6, 0x7E, 0x8D, 0x18, 0x96, 0x56, 0xD1, 0x2B, 0xAD, 0x4B, 0xC1, 0x4F, 0xC8, 0x3A, 0xF1, 0x1E, 0xBD, 0x4C, 0xDA, 0x50, 0xA7, 0x52, 0xE9, 0x76, 0xD8, 0x19, 0x91, 0x72, 0x85, 0x3F, 0x81, 0x61, 0xAA, 0x05, 0x89, 0x0E, 0xB5, 0x24, 0xE0};

const uint8_t romEncryptionTable[] = {0xAC, 0x9C, 0xA4, 0x1A, 0x78, 0xFA, 0xB8, 0x2E, 0x54, 0xC8, 0x46, 0x50, 0xD4, 0x06, 0xFC, 0x28, 0xD2, 0x16, 0xAA, 0x40, 0x0C, 0xAE, 0x2C, 0x68, 0xDC, 0xF2, 0x70, 0x80, 0x66, 0x32, 0xE8, 0x0E, 0x4A, 0x6C, 0x64, 0xD6, 0xFE, 0x22, 0x00, 0x04, 0xCE, 0x0A, 0x60, 0xE0, 0xBC, 0xC0, 0xCC, 0x3C, 0x5C, 0xA2, 0x8A, 0x8E, 0x7C, 0xC2, 0x74, 0x44, 0xA8, 0x30, 0xE6, 0x7A, 0x42, 0xC4, 0x5A, 0xF6, 0x24, 0xD0, 0x18, 0xBE, 0x26, 0xB4, 0x9A, 0x12, 0x8C, 0xD8, 0x82, 0xE2, 0xEA, 0x20, 0x88, 0xE4, 0xEC, 0x86, 0xEE, 0x98, 0x84, 0x7E, 0xDE, 0x36, 0x72, 0xB6, 0x34, 0x90, 0x58, 0xBA, 0x38, 0x10, 0x14, 0xF8, 0x92, 0x02, 0x52, 0x3E, 0xA6, 0x2A, 0x62, 0x76, 0xB0, 0x3A, 0x96, 0x1C, 0x1E, 0x94, 0x6E, 0xB2, 0xF4, 0x4C, 0xC6, 0xA0, 0xF0, 0x48, 0x6A, 0x08, 0x9E, 0x4E, 0xCA, 0x56, 0xDA, 0x5E, 0x2F, 0xF7, 0xBB, 0x3D, 0x21, 0xF5, 0x9F, 0x0B, 0x8B, 0xFB, 0x3F, 0xAF, 0x5B, 0xDB, 0x1B, 0x53, 0x2B, 0xF3, 0xB3, 0xAD, 0x07, 0x0D, 0xDD, 0xA5, 0x95, 0x6F, 0x83, 0x29, 0x59, 0x1D, 0x6D, 0xCF, 0x87, 0xB5, 0x63, 0x55, 0x8D, 0x8F, 0x81, 0xED, 0xB9, 0x37, 0xF9, 0x09, 0x03, 0xE1, 0x0F, 0xD3, 0x5D, 0x75, 0xC5, 0xC7, 0x2D, 0xFD, 0x3B, 0xAB, 0xCD, 0x91, 0xD1, 0x4F, 0x15, 0xE9, 0x5F, 0xCB, 0x25, 0xE3, 0x67, 0x17, 0xBD, 0xB1, 0xC9, 0x6B, 0xE5, 0x77, 0x35, 0x99, 0xBF, 0x69, 0x13, 0x89, 0x61, 0xDF, 0xA3, 0x45, 0x93, 0xC1, 0x7B, 0xC3, 0xF1, 0xD7, 0xEB, 0x4D, 0x43, 0x9B, 0x05, 0xA1, 0xFF, 0x97, 0x01, 0x19, 0xA7, 0x9D, 0x85, 0x7F, 0x4B, 0xEF, 0x73, 0x57, 0xA9, 0x11, 0x79, 0x39, 0xB7, 0xE7, 0x1F, 0x49, 0x47, 0x41, 0xD9, 0x65, 0x71, 0x33, 0x51, 0x31, 0xD5, 0x27, 0x7D, 0x23};

// CRC seed and table
const uint32_t crc32Seed = 0xFFFFFFFF;

const uint32_t crc32Table[] = {0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D};


// Supporting function implementation
void sleepUs(uint64_t microSeconds) {

	// Check if using Windows
	#ifdef WINDOWS
	
		// Sleep
		LARGE_INTEGER totalTime;
		totalTime.QuadPart = -10 * microSeconds;
		HANDLE timer = CreateWaitableTimer(nullptr, true, nullptr);
		SetWaitableTimer(timer, &totalTime, 0, nullptr, nullptr, 0);
		WaitForSingleObject(timer, INFINITE);
		CloseHandle(timer);
	
	// Otherwise
	#else
	
		// Sleep
		usleep(microSeconds);
	#endif
}

// Check if using Windows
#ifdef WINDOWS

	bool isNumeric(LPCTSTR value) {

		// Get length of value
		size_t length = _tcslen(value);
	
		// Check if value is empty
		if(!length)
	
			// Return false
			return false;
	
		// Go through all characters
		for(size_t i = 0; i < length; i++)
	
			// Check if character isn't a digit
			if(!_istdigit(value[i]))
		
				// Return false
				return false;

		// Return true
		return true;
	}
	
	static DWORD WINAPI staticThreadStart(void *parameter) {
	
		// Update status
		return reinterpret_cast<Printer *>(parameter)->updateStatus();
	}
#endif

Printer::Printer() {

	// Clear file descriptor
	fd = 0;
	
	// Clear stop thread
	stopThread = false;
	
	// Set status
	status = "Not connected";
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Create mutex
		mutex = CreateMutex(nullptr, false, nullptr); 
	
		// Create update status thread
		updateStatusThread = CreateThread(nullptr, 0, staticThreadStart, reinterpret_cast<void *>(this), 0, nullptr);
	
	// Otherwise
	#else
	
		// Create update status thread
		updateStatusThread = thread(&Printer::updateStatus, this);
	#endif
}

Printer::~Printer() {

	// Disconnect
	disconnect();
	
	// Acquire lock
	acquireLock();
	
	// Stop thread
	stopThread = true;
	
	// Release lock
	releaseLock();
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Wait until thread is stopped
		WaitForSingleObject(updateStatusThread, INFINITE);
	
	// Otherwise
	#else
	
		// Wait until thread is stopped
		updateStatusThread.join();
	#endif
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Close mutex
		CloseHandle(mutex);
		
		// Close thread
		CloseHandle(updateStatusThread);
	#endif
}

#ifdef WINDOWS
	DWORD
#else
	void
#endif
Printer::updateStatus() {

	// Loop forever
	while(true) {
	
		// Acquire lock
		acquireLock();
		
		// Check if stopping thread
		if(stopThread) {
		
			// Release lock
			releaseLock();
			
			// Break
			break;
		}
	
		// Update status
		if(isConnected())
			status = "Connected";
		
		// Release lock
		releaseLock();
		
		// Sleep
		sleepUs(100000);
	}
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Return 0
		return 0;
	#endif
}

bool Printer::connect(const string &serialPort) {

	// Acquire lock
	acquireLock();
	
	// Update status
	if(status == "Connected")
		status = "Reconnecting";
	else
		status = "Connecting";
	
	// Disconnect if already connected
	disconnect();
        
        // Attempt to connect for 2 seconds
        for(uint8_t i = 0; i < 2000000 / 250000; i++) {
        
        	// Wait 250 milliseconds
		sleepUs(250000);
		
		// Get current serial port
		currentSerialPort = serialPort.length() ? serialPort : getNewSerialPort();

		// Check if no serial ports were found
		if(!currentSerialPort.length())
		
			// Try again
			continue;
		
		// Check if using Windows
		#ifdef WINDOWS
		
			// Check if opening device was successful
			if((fd = CreateFile(("\\\\.\\" + currentSerialPort).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr)) != INVALID_HANDLE_VALUE) {
			
				// Check if getting port settings was successful
				DCB serialPortSettings;
				SecureZeroMemory(&serialPortSettings, sizeof(serialPortSettings));
				serialPortSettings.DCBlength = sizeof(serialPortSettings);
				if(GetCommState(fd, &serialPortSettings)) {
	
					// Set port settings to 8n1 with 115200 baud rate
					serialPortSettings.BaudRate = CBR_115200;
					serialPortSettings.ByteSize = 8;
					serialPortSettings.StopBits = ONESTOPBIT;
					serialPortSettings.Parity = NOPARITY;
					
					// Check if setting port settings was successful
					if(SetCommState(fd, &serialPortSettings)) {
	
						// Configure port timeouts
						COMMTIMEOUTS serialPortTimeouts;
						SecureZeroMemory(&serialPortTimeouts, sizeof(serialPortTimeouts));
						serialPortTimeouts.ReadIntervalTimeout = 50;
						serialPortTimeouts.ReadTotalTimeoutConstant = 50;
						serialPortTimeouts.ReadTotalTimeoutMultiplier = 10;
						serialPortTimeouts.WriteTotalTimeoutConstant = 50;
						serialPortTimeouts.WriteTotalTimeoutMultiplier = 10;
						
						// Check if setting port timeouts was successful
						if(SetCommTimeouts(fd, &serialPortTimeouts)) {
						
							// Release lock
							releaseLock();
						
							// Return true
							return true;
						}
		
						// Otherwise
						else
				
							// Disconnect
							disconnect();
					}
		
					// Otherwise
					else
					
						// Disconnect
						disconnect();
				}
		
				// Otherwise
				else
			
					// Disconnect
					disconnect();
			}
		
			// Otherwise
			else {
			
				// Update status
				switch(GetLastError()) {
					case ERROR_FILE_NOT_FOUND:
						status = "Device not found";
					break;
					case ERROR_ACCESS_DENIED:
						status = "No read/write access to the device";
					break;
					case ERROR_SHARING_VIOLATION:
						status = "Device is busy";
					break;
				}
		
				// Disconnect
				disconnect();
			}
		
		// Otherwise
		#else
		
			// Check if opening device was successful
			if((fd = open(currentSerialPort.c_str(), O_RDWR | O_NOCTTY)) != -1) {
		
				// Create file lock
				struct flock fileLock;
				fileLock.l_type = F_WRLCK;
				fileLock.l_start = 0;
				fileLock.l_whence = SEEK_SET;
				fileLock.l_len = 0;
			
				// Check if file isn't already locked by another process
				if(fcntl(fd, F_SETLK, &fileLock) != -1) {
				
					// Set port settings to 8n1 with 115200 baud rate
					termios settings;
					memset(&settings, 0, sizeof(settings));
					settings.c_iflag = 0;
					settings.c_oflag = 0;
					settings.c_cflag= CS8 | CREAD | CLOCAL;
					settings.c_lflag = 0;
					settings.c_cc[VMIN] = 0;
					settings.c_cc[VTIME] = 1;
					cfsetospeed(&settings, B115200);
					cfsetispeed(&settings, B115200);

					// Check if setting port settings was successful
					if(tcsetattr(fd, TCSANOW, &settings) != -1) {
				
						// Release lock
						releaseLock();

						// Return true
						return true;
					}
				
					// Otherwise
					else
			
						// Disconnect
						disconnect();
				}
				
				// Otherwise
				else {
				
					// Update status
					status = "Device is busy";
		
					// Disconnect
					disconnect();
				}
			}
			
			// Otherwise
			else {
				
				// Update status
				switch(errno) {
					case ENOENT:
						status = "Device not found";
					break;
					case EACCES:
						status = "No read/write access to the device";
					break;
				}
		
				// Disconnect
				disconnect();
			}
		#endif
	}
	
	// Update status
	if(status == "Connecting" || status == "Reconnecting") {
	
		if(!currentSerialPort.length())
			status = "Device not found";
		else
			status = "Failed to connect to the device";
	}
	
	// Release lock
	releaseLock();
	
	// Return false
	return false;
}

void Printer::disconnect() {

	// Acquire lock
	acquireLock();
	
	// Update status
	if(status == "Connected")
		status = "Disconnected";

	// Check if file descriptor is open
        if(fd) {
        
        	// Check if using Windows
        	#ifdef WINDOWS
        	
        		// Close device
			CloseHandle(fd);
		
		// Otherwise
		#else
		
			// Close device
			close(fd);
		#endif
		
		// Clear file descriptor
		fd = 0;
	}
	
	// Clear current port
	currentSerialPort.clear();
	
	// Release lock
	releaseLock();
}

bool Printer::isConnected() {

	// Acquire lock
	acquireLock();

	// Check if device isn't open
	if(!fd) {
	
		// Release lock
		releaseLock();
	
		// Return false
		return false;
	}

	// Check if using Windows
	#ifdef WINDOWS
	
		// Return if device is connected
		bool returnValue = !currentSerialPort.empty();
	
	// Otherwise
	#else
	
		// Return if device is connected
		termios settings;
		bool returnValue = !tcgetattr(fd, &settings);
	#endif
	
	// Disconnect if not connected
	if(!returnValue)
		disconnect();
	
	// Release lock
	releaseLock();
	
	// Return value
	return returnValue;
}

bool Printer::inBootloaderMode() {

	// Check if printer is connected and sending command was successful
	if(sendRequestAscii("M115"))
	
		// Return if in bootloader mode
		return receiveResponseAscii()[0] == 'B';
	
	// Return false
	return false;
}

bool Printer::inFirmwareMode() {

	// Check if printer is connected and sending command was successful
	if(sendRequestAscii("M115"))
	
		// Return if in bootloader mode
		return receiveResponseAscii()[0] != 'B';
	
	// Return false
	return false;
}

void Printer::switchToBootloaderMode() {

	// Check if printer is in firmware mode
	if(inFirmwareMode()) {
	
		// Switch printer into bootloader mode failed
		sendRequestAscii("M115 S628");
		sendRequestBinary("M115 S628");
	}
}

void Printer::switchToFirmwareMode() {

	// Check if printer is in bootloader mode
	if(inBootloaderMode())

		// Switch printer into firmware mode
		sendRequestAscii("Q");
}

bool Printer::installFirmware(const string &file) {

	// Switch to bootloader mode
	switchToBootloaderMode();
	
	// Check if ROM doesn't exist
	ifstream romInput(file, ios::binary);
	if(!romInput.good())
	
		// Return false
		return false;
	
	// Read in the encrypted ROM
	string romBuffer;
	while(romInput.peek() != EOF)
		romBuffer.push_back(romInput.get());
	romInput.close();

	// Check if ROM isn't encrypted
	if(static_cast<uint8_t>(romBuffer[0]) == 0x0C || static_cast<uint8_t>(romBuffer[0]) == 0xFD) {

		// Encrypt the ROM
		string encryptedRomBuffer;
		for(uint16_t i = 0; i < romBuffer.length(); i++)

			// Check if padding wasn't required
			if(i % 2 != 0 || i != romBuffer.length() - 1)

				// Encrypt the ROM
				encryptedRomBuffer.push_back(romEncryptionTable[static_cast<uint8_t>(romBuffer[i + (i % 2 ? -1 : 1)])]);			
	
		// Set encrypted ROM
		romBuffer = encryptedRomBuffer;
	}

	// Check if ROM is too big
	if(romBuffer.length() > CHIP_TOTAL_MEMORY)

		// Return false
		return false;
	
	// Check if requesting that chip failed to be erased
	if(!sendRequestAscii('E'))
	
		// Return false
		return false;
	
	// Delay
	sleepUs(1000000);
	
	// Check if chip failed to be erased
	if(receiveResponseAscii() != "\r")
	
		// Return false
		return false;
	
	// Check if address wasn't acknowledged
	if(!sendRequestAscii('A') || !sendRequestAscii('\x00') || !sendRequestAscii('\x00') || receiveResponseAscii() != "\r")
	
		// Return false
		return false;

	// Set pages to write
	uint16_t pagesToWrite = romBuffer.length() / 2 / CHIP_PAGE_SIZE;
	if(romBuffer.length() / 2 % CHIP_PAGE_SIZE != 0)
		pagesToWrite++;

	// Go through all pages to write
	for(uint16_t i = 0; i < pagesToWrite; i++) {

		// Check if sending write to page request failed
		if(!sendRequestAscii('B') || !sendRequestAscii(CHIP_PAGE_SIZE * 2 >> 8, false) || !sendRequestAscii(static_cast<char>(CHIP_PAGE_SIZE * 2), false))
		
			// Return false
			return false;

		// Go through all values for the page
		for(int j = 0; j < CHIP_PAGE_SIZE * 2; j++) {

			// Check if data to be written exists
			uint32_t position = j + CHIP_PAGE_SIZE * i * 2;
			if(position < romBuffer.length()) {

				// Return false if sending value failed
				if(!sendRequestAscii(romBuffer[position + (position % 2 ? -1 : 1)], false))
					return false;
			}

			// Otherwise
			else {

				// Return false if sending padding failed
				if(!sendRequestAscii(romEncryptionTable[0xFF], false))
					return false;
			}
		}

		// Check if chip failed to be flashed
		if(receiveResponseAscii() != "\r")

			// Return false
			return false;
	}

	// Check if address wasn't acknowledged
	if(!sendRequestAscii('A') || !sendRequestAscii('\x00') || !sendRequestAscii('\x00') || receiveResponseAscii() != "\r")
	
		// Return false
		return false;

	// Check if requesting CRC from chip failed
	if(!sendRequestAscii('C') || !sendRequestAscii('A'))
	
		// Return false
		return false;

	// Get response
	string response = receiveResponseAscii();

	// Get chip CRC
	uint32_t chipCrc = 0;
	for(uint8_t i = 0; i < 4; i++) {
		chipCrc <<= 8;
		chipCrc += static_cast<uint8_t>(response[i]);
	}

	// Decrypt the ROM
	uint8_t decryptedRom[CHIP_TOTAL_MEMORY];
	for(uint16_t i = 0; i < CHIP_TOTAL_MEMORY; i++) {

		// Check if data exists in the ROM
		if (i < romBuffer.length()) {
	
			// Check if padding is required
			if(i % 2 == 0 && i == romBuffer.length() - 1)
		
				// Put padding
				decryptedRom[i] = 0xFF;
			
			// Otherwise
			else
		
				// Decrypt the ROM
				decryptedRom[i] = romDecryptionTable[static_cast<uint8_t>(romBuffer[i + (i % 2 ? -1 : 1)])];
		}
	
		// Otherwise
		else
	
			// Put padding
			decryptedRom[i] = 0xFF;
	}

	// Get ROM CRC
	uint32_t romCrc = crc32(0, decryptedRom, CHIP_TOTAL_MEMORY);

	// Check if firmware update failed
	if(chipCrc != __builtin_bswap32(romCrc))

		// Return false
		return false;

	// Set ROM version from file name
	uint8_t endOfNumbers = 0;
	if(file.find_last_of('.') != string::npos)
		endOfNumbers = file.find_last_of('.') - 1;
	else
		endOfNumbers = file.length() - 1;
	uint32_t romVersion = stoi(file.substr(endOfNumbers - 10));

	// Check if updating firmware version in EEPROM failed
	for(uint8_t i = 0; i < 4; i++)
		if(!writeToEeprom(i, romVersion >> 8 * i))

			// Return false
			return false;

	// Check if updating firmware CRC in EEPROM failed
	for(uint8_t i = 0; i < 4; i++)
		if(!writeToEeprom(i + 4, romCrc >> 8 * i))

			// Return false
			return false;

	// Return true
	return true;
}

bool Printer::sendRequestAscii(const char *data, bool checkForModeSwitching) {

	// Return false if not connected
	if(!isConnected())
		return false;

	// Update available serial ports if switching into bootloader or firmware mode
	if(checkForModeSwitching && (!strcmp(data, "M115 S628") || !strcmp(data, "Q")))
		updateAvailableSerialPorts();
	
	// Acquire lock
	acquireLock();

	// Send data to the device
	#ifdef WINDOWS
		DWORD bytesSent = 0;
		bool returnValue = WriteFile(fd, data, strlen(data), &bytesSent, nullptr) && bytesSent == strlen(data);
	#else
		bool returnValue = write(fd, data, strlen(data)) == static_cast<signed int>(strlen(data));
	#endif
	
	// Release lock
	releaseLock();
	
	// Reconnect if data was successfully sent and switching into bootloader mode
	if(returnValue && checkForModeSwitching && (!strcmp(data, "M115 S628") || !strcmp(data, "Q")))
		connect();
	
	// Disconnect if sending request failed
	if(!returnValue || !isConnected())
		disconnect();
	
	// Return if request was successfully sent
	return returnValue && isConnected();
}

bool Printer::sendRequestAscii(char data, bool checkForModeSwitching) {

	// Return false if not connected
	if(!isConnected())
		return false;

	// Update available serial ports if switching into firmware mode
	if(checkForModeSwitching && data == 'Q')
		updateAvailableSerialPorts();
	
	// Acquire lock
	acquireLock();

	// Send data to the device
	#ifdef WINDOWS
		DWORD bytesSent = 0;
		bool returnValue = WriteFile(fd, &data, 1, &bytesSent, nullptr) && bytesSent == 1;
	#else
		bool returnValue = write(fd, &data, 1) == 1;
	#endif
	
	// Release lock
	releaseLock();
	
	// Reconnect if data was successfully sent and switching into bootloader mode
	if(returnValue && checkForModeSwitching && data == 'Q')
		connect();
	
	// Disconnect if sending request failed
	if(!returnValue || !isConnected())
		disconnect();
	
	// Return if request was successfully sent
	return returnValue && isConnected();
}

bool Printer::sendRequestAscii(const string &data, bool checkForModeSwitching) {

	// Send request
	return sendRequestAscii(data.c_str(), checkForModeSwitching);
}

bool Printer::sendRequestAscii(const Gcode &data) {

	// Send request
	return sendRequestAscii(data.getAscii(), true);
}

bool Printer::sendRequestBinary(const Gcode &data) {

	// Return false if not connected
	if(!isConnected())
		return false;

	// Update available serial ports if switching into bootloader mode
	if(data.getValue('M') == "115" && data.getValue('S') == "628")
		updateAvailableSerialPorts();

	// Get binary data
	vector<uint8_t> request = data.getBinary();
	
	// Acquire lock
	acquireLock();
	
	// Send binary request to the device
	#ifdef WINDOWS
		DWORD bytesSent = 0;
		bool returnValue = WriteFile(fd, request.data(), request.size(), &bytesSent, nullptr) && bytesSent == request.size();
	#else
		bool returnValue = write(fd, request.data(), request.size()) == static_cast<signed int>(request.size());		
	#endif
	
	// Release lock
	releaseLock();
	
	// Reconnect if data was successfully sent and switching into bootloader mode
	if(returnValue && data.getValue('M') == "115" && data.getValue('S') == "628")
		connect();
	
	// Disconnect if sending request failed
	if(!returnValue || !isConnected())
		disconnect();
	
	// Return if request was successfully sent
	return returnValue && isConnected();
}

bool Printer::sendRequestBinary(const char *data) {
	
	// Check if line was successfully parsed
	Gcode gcode;
	if(gcode.parseLine(data))
	
		// Send request
		return sendRequestBinary(gcode);
	
	// Disconnect and return false
	disconnect();
	return false;
}

bool Printer::sendRequestBinary(const string &data) {

	// Send request
	return sendRequestBinary(data.c_str());
}

string Printer::receiveResponseAscii() {

	// Return an empty string if not connected
	if(!isConnected())
		return "";

	// Initialize variables
	string response;
	char character;
	uint16_t i = 0;
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Wait 1 second for a response
		DWORD bytesReceived = 0;
		for(; i < 1000 && !bytesReceived; i++) {
		
			// Acquire lock
			acquireLock();
		
			// Check if failed to receive a response
			if(!ReadFile(fd, &character, sizeof(character), &bytesReceived, nullptr)) {
			
				// Release lock
				releaseLock();
				
				// Disconnect and return an empty string
				disconnect();
				return "";
			}
			
			// Release lock
			releaseLock();
			
			if(!bytesReceived)
				sleepUs(1000);
		}
		
		// Return an empty string if no response is received
		if(i == 1000)
			return "";
		
		// Get response
		do {
			response.push_back(character);
			sleepUs(50);
			
			// Acquire lock
			acquireLock();
			
			// Check if failed to receive a response
			if(!ReadFile(fd, &character, sizeof(character), &bytesReceived, nullptr)) {
			
				// Release lock
				releaseLock();
				
				// Disconnect and return an empty string
				disconnect();
				return "";
			}
			
			// Release lock
			releaseLock();
		} while(bytesReceived);
	
	// Otherwise
	#else
	
		// Wait 1 second for a response
		int bytesReceived = 0;
		for(; i < 1000 && !bytesReceived; i++) {
		
			// Acquire lock
			acquireLock();
		
			// Check if failed to receive a response
			if((bytesReceived = read(fd, &character, sizeof(character))) == -1) {
			
				// Release lock
				releaseLock();
				
				// Disconnect and return an empty string
				disconnect();
				return "";
			}
			
			// Release lock
			releaseLock();
			
			if(!bytesReceived)
				sleepUs(1000);
		}
		
		// Return an empty string if no response is received
		if(i == 1000)
			return "";
	
		// Get response
		do {
			response.push_back(character);
			sleepUs(50);
			
			// Acquire lock
			acquireLock();
			
			// Check if failed to receive a response
			if((bytesReceived = read(fd, &character, sizeof(character))) == -1) {
			
				// Release lock
				releaseLock();
				
				// Disconnect and return an empty string
				disconnect();
				return "";
			}
			
			// Release lock
			releaseLock();
		} while(bytesReceived);
	#endif
	
	// Return response
	return response;
}

string Printer::receiveResponseBinary() {

	// Return an empty string if not connected
	if(!isConnected())
		return "";

	// Initialize variables
	string response;
	char character;
	uint16_t i = 0;
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Wait 1 second for a response
		DWORD bytesReceived = 0;
		for(; i < 1000 && !bytesReceived; i++) {
		
			// Acquire lock
			acquireLock();
		
			// Check if failed to receive a response
			if(!ReadFile(fd, &character, sizeof(character), &bytesReceived, nullptr)) {
			
				// Release lock
				releaseLock();
				
				// Disconnect and return an empty string
				disconnect();
				return "";
			}
			
			// Release lock
			releaseLock();
			
			if(!bytesReceived)
				sleepUs(1000);
		}
		
		// Return an empty string if no response is received
		if(i == 1000)
			return "";
		
		// Get response
		while(character != '\n') {
			response.push_back(character);
			do {
			
				// Acquire lock
				acquireLock();
			
				// Check if failed to receive a response
				if(!ReadFile(fd, &character, sizeof(character), &bytesReceived, nullptr)) {
				
					// Release lock
					releaseLock();
					
					// Disconnect and return an empty string
					disconnect();
					return "";
				}
				
				// Release lock
				releaseLock();
			} while(!bytesReceived);
		}
	
	// Otherwise
	#else
	
		// Wait 1 second for a response
		int bytesReceived = 0;
		for(; i < 1000 && !bytesReceived; i++) {
		
			// Acquire lock
			acquireLock();
		
			// Check if failed to receive a response
			if((bytesReceived = read(fd, &character, sizeof(character))) == -1) {
			
				// Release lock
				releaseLock();
				
				// Disconnect and return an empty string
				disconnect();
				return "";
			}
			
			// Release lock
			releaseLock();
			
			if(!bytesReceived)
				sleepUs(1000);
		}
	
		// Return an empty string if no response is received
		if(i == 1000)
			return "";
		
		// Get response
		while(character != '\n') {
			response.push_back(character);
			do {
			
				// Acquire lock
				acquireLock();
			
				// Check if failed to receive a response
				if((bytesReceived = read(fd, &character, sizeof(character))) == -1) {
				
					// Release lock
					releaseLock();
					
					// Disconnect and return an empty string
					disconnect();
					return "";
				}
				
				// Release lock
				releaseLock();
			} while(!bytesReceived);
		}
	#endif
	
	// Return response
	return response;
}

bool Printer::writeToEeprom(uint16_t address, const uint8_t *data, uint16_t length) {

	// Check if sending write to EEPROM request failed
	if(!sendRequestAscii('U') || !sendRequestAscii(address >> 8, false) || !sendRequestAscii(address, false) || !sendRequestAscii(length >> 8, false) || !sendRequestAscii(length, false))
		
		// Return false
		return false;
	
	// Go through all bytes of data
	for(uint16_t i = 0; i < length; i++)
	
		// Check if sending byte failed
		if(!sendRequestAscii(data[i], false))
		
			// Return false
			return false;
	
	// Return if write was successful
	return receiveResponseAscii() == "\r";
}

bool Printer::writeToEeprom(uint16_t address, uint8_t data) {

	// Return if write was successful
	return writeToEeprom(address, &data, 1);
}

uint32_t Printer::crc32(int32_t offset, const uint8_t *data, int32_t count) {

	// Initialize variables
	uint32_t crc = 0;
	
	// Update CRC
	crc ^= crc32Seed;

	// Go through data
	while(--count >= 0)
	
		// Update CRC
		crc = crc32Table[(crc ^ data[offset++]) & 0xFF] ^ (crc >> 8);
	
	// Return updated CRC
	return crc ^ crc32Seed;
}

void Printer::updateAvailableSerialPorts() {

	// Clear available serial ports
	availableSerialPorts.clear();
	
	// Check if using Windows
	#ifdef WINDOWS
	
		// Check if getting all connected devices was successful
		HDEVINFO deviceInfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
		if(deviceInfo != INVALID_HANDLE_VALUE) {
	
			// Check if allocating memory was successful
			PSP_DEVINFO_DATA deviceInfoData = reinterpret_cast<PSP_DEVINFO_DATA>(HeapAlloc(GetProcessHeap(), 0, sizeof(SP_DEVINFO_DATA)));
			if(deviceInfoData) {
		
				// Set device info size
				deviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
	
				// Go through all devices
				for(DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfo, i, deviceInfoData); i++) {
	
					// Check if device is has the printer's PID and VID
					TCHAR buffer[MAX_PATH];
					if(SetupDiGetDeviceInstanceId(deviceInfo, deviceInfoData, buffer, sizeof(buffer), nullptr) && !_tcsnicmp(buffer, _T("USB\\VID_03EB&PID_2404"), strlen("USB\\VID_03EB&PID_2404"))) {
					
						// Check if getting device registry key was successful
						HKEY deviceRegistryKey = SetupDiOpenDevRegKey(deviceInfo, deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
						if(deviceRegistryKey) {
		
							// Check if getting port name was successful
							DWORD type = 0;
							DWORD size = sizeof(buffer);
							if(RegQueryValueEx(deviceRegistryKey, _T("PortName"), nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS && type == REG_SZ) {
				
								// Check if port is valid
								if(_tcslen(buffer) > 3 && !_tcsnicmp(buffer, _T("COM"), strlen("COM")) && isNumeric(&buffer[3]))
								
									// Append serial port to list
									availableSerialPorts.push_back(buffer);
							}
							
							// Close registry key
							RegCloseKey(deviceRegistryKey);
						}
					}
				}
	
				// Clear device info data
				HeapFree(GetProcessHeap(), 0, deviceInfoData);
			}
			
			// Clear device info
			SetupDiDestroyDeviceInfoList(deviceInfo);
		}
	#endif
	
	// Otherwise check if using OS X
	#ifdef OSX
		
		// Check if establish connection to IOKit was successful
		mach_port_t masterPort;
		if(IOMasterPort(MACH_PORT_NULL, &masterPort) == KERN_SUCCESS) {
			
			// Check if creating matching dictionary for IOKit devices was successful
			CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
			if(classesToMatch) {
			
				// Set dictionary to match modem devices
				CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDModemType));
				
				// Check if getting list of devices was successful
				io_iterator_t serialPortIterator;
				if(IOServiceGetMatchingServices(masterPort, classesToMatch, &serialPortIterator) == KERN_SUCCESS) {
					
					// Iterate through all devices
					io_object_t modemService;
					while((modemService = IOIteratorNext(serialPortIterator))) {

						// Check if device has a VID
						CFTypeRef vidType = IORegistryEntrySearchCFProperty(modemService, kIOServicePlane, CFSTR("idVendor"), kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
						if(vidType) {

							// Check if device has a PID
							CFTypeRef pidType = IORegistryEntrySearchCFProperty(modemService, kIOServicePlane, CFSTR("idProduct"), kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
							if(pidType) {
						
								// Check if device has the printer's PID and VID
								int pid, vid;
								if(CFNumberGetValue(reinterpret_cast<CFNumberRef>(vidType), kCFNumberIntType, &vid) && CFNumberGetValue(reinterpret_cast<CFNumberRef>(pidType), kCFNumberIntType, &pid) && vid == 0x03EB && pid == 0x2404) {
							
									// Check if device has a file path
									CFTypeRef deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(modemService, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0);
									if(deviceFilePathAsCFString) {
									
										// Check if getting device's file path was successful
										char deviceFilePath[MAXPATHLEN] = {};
										if(CFStringGetCString(reinterpret_cast<CFStringRef>(deviceFilePathAsCFString), deviceFilePath, sizeof(deviceFilePath), kCFStringEncodingASCII))
									
											// Append serial port to list
											availableSerialPorts.push_back(deviceFilePath);
									
										// release device's file path
										CFRelease(deviceFilePathAsCFString);
									}
								}

								// Release PID
								CFRelease(pidType);
							}

							// Release VID
							CFRelease(vidType);
						}
						
						// Release current device
						IOObjectRelease(modemService);
					}
					
					// Release list of devices
					IOObjectRelease(serialPortIterator);
				}
			}
		}
	#endif
	
	// Otherwise check if using Linux
	#ifdef LINUX

		// Check if path exists
		DIR *path = opendir("/sys/class/tty/");
		if(path) {
		
			// Go through all serial devices
			dirent *entry;
		        while((entry = readdir(path))) {
		        
		        	// Check if current device is a serial device
		                if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") && !strncmp("ttyACM", entry->d_name, 6)) {
		                
		                	// Check if uevent file exists for the device
		                	ifstream device(static_cast<string>("/sys/class/tty/") + entry->d_name + "/device/modalias", ios::binary);
		                	if(device.good()) {
		                	
				        	// Read in file
				        	string info;
						while(device.peek() != EOF)
							info.push_back(toupper(device.get()));
					
						// Check if device has the printer's PID and VID
						if(info.find("USB:V03EBP2404") == 0)
					
							// Append serial port to list
							availableSerialPorts.push_back(static_cast<string>("/dev/") + entry->d_name);
						
						// Close device
						device.close();
					}
				}
			}
			
			// Close path
			closedir(path);
		}
	#endif
}

string Printer::getNewSerialPort() {

	// Save available serial ports
	vector<string> temp = availableSerialPorts;
	
	// Update available serial ports
	updateAvailableSerialPorts();
	
	// Check if no current serial port has been set
	if(!currentSerialPort.length() && availableSerialPorts.size())
	
		// Return first available serial port
		return availableSerialPorts[0];

	// Go through all available serial ports
	for(uint8_t i = 0; i < availableSerialPorts.size(); i++)
	
		// Check if current serial port exists
        	if(currentSerialPort == availableSerialPorts[i])
        	
        		// Return current serial port
        		return currentSerialPort;
	
	// Go through all available serial ports
	for(uint8_t i = 0; i < availableSerialPorts.size(); i++) {
	
		// Set new port found
		bool newPortFound = true;
	
		// Go through all previoud serial ports
		for(uint8_t j = 0; j < temp.size(); j++) {
		
			// Check if serial port was previously connected
			if(temp[j] == availableSerialPorts[i]) {
			
				// Clear new port found
				newPortFound = false;
				
				// Break
				break;
			}
		}
		
		// Check if a new port was found
		if(newPortFound)
		
			// Return serial port
			return availableSerialPorts[i];
	}
	
	// Return nothing
	return "";		
}

vector<string> Printer::getAvailableSerialPorts() {

	// Update available serial ports
	updateAvailableSerialPorts();
	
	// Return available serial ports
	return availableSerialPorts;
}

string Printer::getCurrentSerialPort() {

	// Acquire lock
	acquireLock();

	// Get current serial port
	string temp = currentSerialPort;
	
	// Release lock
	releaseLock();
	
	// Return current serial port
	return temp;
}

string Printer::getStatus() {

	// Acquire lock
	acquireLock();
	
	// Get status
	string temp = status;
	
	// Release lock
	releaseLock();
	
	// Return status
	return temp;
}

void Printer::acquireLock() {

	// Check if using Windows
	#ifdef WINDOWS
	
		// Acquire lock
		WaitForSingleObject(mutex, INFINITE);
	
	// Otherwise
	#else
	
		// Acquire lock
		mutex.lock();
	#endif
}

void Printer::releaseLock() {

	// Check if using Windows
	#ifdef WINDOWS
	
		// Release lock
		ReleaseMutex(mutex);
	
	// Otherwise
	#else
	
		// Release lock
		mutex.unlock();
	#endif
}
