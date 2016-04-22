// Header files
#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include "printer.h"

// Packed files
#include "iMe 1900000001_hex.h"
#ifdef WINDOWS
	#include "M3D_cat.h"
	#include "M3D_inf.h"
#endif
#ifdef LINUX
	#include "_90_m3d_local_rules.h"
#endif

using namespace std;


// Global variables
string workingFolderLocation;
string serialPort;


// Function prototypes
void breakHandler(int signal);
bool installFirmware(const string &firmwareLocation);


// Main function
int main(int argc, char *argv[]) {
	
	// Get temp directory
	char* tempPath = getenv("TEMP");
	if(!tempPath)
		 tempPath = getenv("TMP");
	if(!tempPath)
		 tempPath = getenv("TMPDIR");
	
	// Create temporary folder
	workingFolderLocation = mkdtemp(const_cast<char *>((static_cast<string>(tempPath ? tempPath : P_tmpdir) + "/m3d-XXXXXX").c_str()));
	
	// Attach break handler
	signal(SIGINT, breakHandler);
	
	// Display version
	cout << "M3D Manager V0.01" << endl << endl;
	
	// Go through all commands
	for(uint8_t i = 0; i <= argc; i++) {
		
		// Check if help is requested
		if(i == argc || !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
		
			// Display help
			cout << "Usage: m3d-manager -d -s -i -r firmware.rom serialport" << endl;
			cout << "-d | --drivers: Install device drivers" << endl;
			cout << "-s | --start: Switches printer into firmware mode" << endl;
			cout << "-i | --ime: Installs iMe firmware" << endl;
			cout << "-r | --rom: Installs the provided firmware" << endl;
			cout << "serialport: The printer's serial port or it will automatically find printer is not specified" << endl << endl;
			
			// Break
			break;
		}
		
		// Otherwise
		else if(i < argc) {
		
			// Check if installing drivers
			if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--drivers")) {
			
				// Check if using Windows
				#ifdef WINDOWS
				
					// Unpack drivers
					ofstream fout(workingFolderLocation + "/M3D.cat", ios::binary);
					for(uint64_t i = 0; i < m3D_catSize; i++)
						fout.put(m3D_catData[i]);
					fout.close();
	
					fout.open(workingFolderLocation + "/M3D.inf", ios::binary);
					for(uint64_t i = 0; i < m3D_infSize; i++)
						fout.put(m3D_infData[i]);
					fout.close();
					
					// Check if installing drivers failed
					if(system("PnPUtil -i -a \"" + workingFolderLocation + "/M3D.inf\"")) {
						
						// Display error
						cout << "Failed to install drivers" << endl;
						break;
					}
					
					// Display message
					cout << "Drivers successfully installed" << endl;
				#endif
				
				// Otherwise check if using OS X
				#ifdef OSX
				
					// Display message
					cout << "Drivers successfully installed" << endl;
				#endif
				
				// Otherwise check if using Linux
				#ifdef LINUX
			
					// Check if user is not root
					if(getuid())
				
						// Display error
						cout << "Elevated privileges required" << endl;
				
					// Otherwise
					else {
				
						// Unpack udev rule
						ofstream fout("/etc/udev/rules.d/90-m3d-local.rules", ios::binary);
						for(uint64_t i = 0; i < _90_m3d_local_rulesSize; i++)
							fout.put(_90_m3d_local_rulesData[i]);
						fout.close();
					
						// Check if applying udev rule failed
						if(system("/etc/init.d/udev restart")) {
						
							// Display error
							cout << "Failed to install drivers" << endl;
							break;
						}
						
						// Display message
						cout << "Drivers successfully installed" << endl;
					}
				#endif
				
				// Break;
				break;
			}
		
			// Check if a switching printer into firmware mode
			if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--start")) {
			
				// Set serial port
				if(i < argc - 1)
					serialPort = argv[argc - 1];
			
				// Check if connecting to printer failed
				Printer printer;
				if(!printer.connect(serialPort)) {
			
					// Display error
					cout << "Failed to connect to the printer" << endl;
					break;
				}
				
				// Check if printer is in bootloader mode
				if(printer.isBootloaderMode())
				
					// Put printer into firmware mode
					printer.sendRequestAscii("Q");
				
				// Display message
				cout << "Printer has been successfully switched into firmware mode" << endl;
				
				// Break;
				break;
			}
			
			// Otherwise check installing iMe firmware
			else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--ime")) {
			
				// Set firmware location
				string firmwareLocation = workingFolderLocation + "/iMe 1900000001.hex";
			
				// Unpack iMe ROM
				ofstream fout(firmwareLocation, ios::binary);
				for(uint64_t i = 0; i < iMe1900000001_hexSize; i++)
					fout.put(iMe1900000001_hexData[i]);
				fout.close();
				
				// Set serial port
				if(i < argc - 1)
					serialPort = argv[argc - 1];
			
				// Install firmware
				if(!installFirmware(firmwareLocation)) {
						
					// Display error
					cout << "Failed to install iMe" << endl;
					break;
				}
				
				// Display message
				cout << "iMe successfully installed" << endl;
				
				// Break
				break;
			}
			
			// Otherwise check if a firmware ROM is provided
			else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--firmwarerom")) {
		
				// Check if firmware ROM parameter doesn't exist
				if(i >= argc - 1) {
			
					// Display error
					cout << "No firmware ROM provided" << endl;
					break;
				}

				// Set firmware location
				string firmwareLocation = argv[++i];
				
				// Set serial port
				if(i < argc - 1)
					serialPort = argv[argc - 1];
			
				// Install firmware
				if(!installFirmware(firmwareLocation)) {
						
					// Display error
					cout << "Failed to install firmware" << endl;
					break;
				}
				
				// Display message
				cout << "Firmware successfully installed" << endl;
				
				// Break
				break;
			}
		}
	}
	
	// Delete temporary folder
	rmdir(workingFolderLocation.c_str());
	
	// Return 0
	return 0;
}


// Supporting function implementation
void breakHandler(int signal) {

	// Delete temporary folder
	rmdir(workingFolderLocation.c_str());

	// Exit so that destructor is called on printer
	exit(0);
}

bool installFirmware(const string &firmwareLocation) {

	// Check if firmware ROM doesn't exists
	ifstream file(firmwareLocation);
	if(!file.good()) {

		// Display error
		cout << "Firmware ROM doesn't exist" << endl;
		return false;
	}

	// Check if firmware ROM name is valid
	uint8_t endOfNumbers = 0;
	if(firmwareLocation.find_last_of('.') != string::npos)
		endOfNumbers = firmwareLocation.find_last_of('.') - 1;
	else
		endOfNumbers = firmwareLocation.length() - 1;

	uint8_t beginningOfNumbers = endOfNumbers - 10;
	for(; beginningOfNumbers && endOfNumbers > beginningOfNumbers && isdigit(firmwareLocation[endOfNumbers]); endOfNumbers--);

	if(endOfNumbers != beginningOfNumbers) {

		// Display error
		cout << "Invalid firmware ROM name" << endl;
		return false;
	}

	// Check if connecting to printer failed
	Printer printer;
	if(!printer.connect(serialPort)) {

		// Display error
		cout << "Failed to connect to the printer" << endl;
		return false;
	}

	// Check if updating printer's firmware failed
	if(!printer.updateFirmware(firmwareLocation.c_str())) {

		// Display error
		cout << "Failed to update firmware" << endl;
		return false;
	}

	// Put printer into firmware mode
	printer.sendRequestAscii("Q");
	
	// Return true
	return true;
}
