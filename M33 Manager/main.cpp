// Header files
#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include "common.h"
#include "printer.h"
#ifdef USE_GUI
	#include "gui.h"
#endif

using namespace std;


// Global variables
#ifndef USE_GUI
	Printer printer;
#endif


// Function prototypes

/*
Name: Break handler
Purpose: Terminal signal event
*/
void breakHandler(int signal);

/*
Name: Install firmware
Purpose: Installs specified firmware
*/
bool installFirmware(const string &firmwareLocation, const string &serialPort);


// Check if using GUI
#ifdef USE_GUI

	// Implement application
	wxIMPLEMENT_APP(MyApp);
#endif


// Main function
#ifdef USE_GUI
	bool MyApp::OnInit() {
#else
	int main(int argc, char *argv[]) {
#endif
	
	// Attach break handler
	signal(SIGINT, breakHandler);
	
	// Check if using GUI
	#ifdef USE_GUI
	
		// Check if using macOS
		#ifdef MACOS
		
			// Enable plainer transition
			wxSystemOptions::SetOption(wxMAC_WINDOW_PLAIN_TRANSITION, 1);
		#endif

		// Create and show window
		MyFrame *frame = new MyFrame("M33 Manager", wxDefaultPosition, wxSize(1118, 482), wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));
		frame->Center();
		frame->Show(true);
		
		// Return true
		return true;
	
	// Otherwise
	#else
	
		// Display version
		cout << "M33 Manager V" TOSTRING(VERSION) << endl << endl;
		
		// Set printer's log function
		printer.setLogFunction([=](const string &message) -> void {
	
			// Log message to console
			if(message != "Remove last line")
				cout << message << endl;
		});
	
		// Check if using command line interface
		if(argc > 1) {
	
			// Go through all commands
			for(uint8_t i = 0; i < argc; i++) {
		
				// Check if help is requested
				if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
				
					string iMeVersion = static_cast<string>(TOSTRING(IME_ROM_VERSION_STRING)).substr(2);
					for(uint8_t i = 0; i < 3; i++)
						iMeVersion.insert(i * 2 + 2 + i, ".");
		
					// Display help
					cout << "Usage: \"M33 Manager\" -d -f -b -i -3 -r firmware.rom -m serialport" << endl;
					#ifndef MACOS
						cout << "-d | --drivers: Install device drivers" << endl;
					#endif
					cout << "-f | --firmware: Switches printer into firmware mode" << endl;
					cout << "-b | --bootloader: Switches printer into bootloader mode" << endl;
					cout << "-i | --ime: Installs iMe V" << iMeVersion << endl;
					cout << "-3 | --m3d: Installs M3D V" TOSTRING(M3D_ROM_VERSION_STRING) << endl;
					cout << "-r | --rom: Installs the provided firmware" << endl;
					cout << "-m | --manual: Allows manually sending commands to the printer" << endl;
					cout << "serialport: The printer's serial port or it will automatically find the printer if not specified" << endl << endl;
			
					// Return
					return EXIT_SUCCESS;
				}
			
				// Check if not using macOS
				#ifndef MACOS
		
					// Otherwise check if installing drivers
					else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--drivers")) {
			
						// Display message
						cout << "Installing drivers" << endl;
		
						// Check if using Windows
						#ifdef WINDOWS
			
							// Check if creating drivers file failed
							ofstream fout(getTemporaryLocation() + "M3D_v2.cat", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack drivers" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack drivers
							for(uint64_t i = 0; i < m3DV2CatSize; i++)
								fout.put(m3DV2CatData[i]);
							fout.close();
					
							// Check if creating drivers file failed
							fout.open(getTemporaryLocation() + "M3D_v2.inf", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack drivers" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack drivers
							for(uint64_t i = 0; i < m3DV2InfSize; i++)
								fout.put(m3DV2InfData[i]);
							fout.close();
				
							// Check if creating process failed
							TCHAR buffer[MAX_PATH];
							GetWindowsDirectory(buffer, MAX_PATH);
							string path = buffer;
				
							string executablePath;
							ifstream file(path + "\\sysnative\\pnputil.exe", ios::binary);
							executablePath = file.good() ? "sysnative" : "System32";
							file.close();
		
							STARTUPINFO startupInfo;
							SecureZeroMemory(&startupInfo, sizeof(startupInfo));
							startupInfo.cb = sizeof(startupInfo);
						
							PROCESS_INFORMATION processInfo;
							SecureZeroMemory(&processInfo, sizeof(processInfo));
						
							TCHAR command[MAX_PATH];
							_tcscpy(command, (path + "\\" + executablePath + "\\pnputil.exe -i -a \"" + getTemporaryLocation() + "M3D_v2.inf\"").c_str());
						
							if(!CreateProcess(nullptr, command, nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo)) {

								// Display error
								cout << "Failed to install drivers" << endl;
							
								// Return
								return EXIT_FAILURE;
							}
						
							// Wait until process finishes
							WaitForSingleObject(processInfo.hProcess, INFINITE);
						
							// Check if installing drivers failed
							DWORD exitCode;
							GetExitCodeProcess(processInfo.hProcess, &exitCode);
						
							// Close process and thread handles. 
							CloseHandle(processInfo.hProcess);
							CloseHandle(processInfo.hThread);
						
							if(!exitCode) {
			
								// Display error
								cout << "Failed to install drivers" << endl;
					
								// Return
								return EXIT_FAILURE;
							}
			
							// Display message
							cout << "Drivers successfully installed" << endl;
						#endif
			
						// Otherwise check if using Linux
						#ifdef LINUX
		
							// Check if user is not root
							if(getuid()) {
			
								// Display error
								cout << "Elevated privileges required" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
			
							// Check if creating udev rule file failed
							ofstream fout("/etc/udev/rules.d/90-micro-3d-local.rules", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack udev rule" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack udev rule
							for(uint64_t i = 0; i < _90Micro3dLocalRulesSize; i++)
								fout.put(_90Micro3dLocalRulesData[i]);
							fout.close();
							
							// Check if creating udev rule file failed
							fout.open("/etc/udev/rules.d/91-micro-3d-heatbed-local.rules", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack udev rule" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack udev rule
							for(uint64_t i = 0; i < _91Micro3dHeatbedLocalRulesSize; i++)
								fout.put(_91Micro3dHeatbedLocalRulesData[i]);
							fout.close();
							
							// Check if creating udev rule file failed
							fout.open("/etc/udev/rules.d/92-m3d-pro-local.rules", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack udev rule" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack udev rule
							for(uint64_t i = 0; i < _92M3dProLocalRulesSize; i++)
								fout.put(_92M3dProLocalRulesData[i]);
							fout.close();
							
							// Check if creating udev rule file failed
							fout.open("/etc/udev/rules.d/93-micro+-local.rules", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack udev rule" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack udev rule
							for(uint64_t i = 0; i < _93Micro_localRulesSize; i++)
								fout.put(_93Micro_localRulesData[i]);
							fout.close();
							
							// Check if applying udev rule failed
							if(system("udevadm control --reload-rules") || system("udevadm trigger")) {
				
								// Display error
								cout << "Failed to install drivers" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
				
							// Display message
							cout << "Drivers successfully installed" << endl;
						#endif
			
						// Return
						return EXIT_SUCCESS;
					}
				#endif
	
				// Otherwise check if a switching printer into firmware mode
				else if(!strcmp(argv[i], "-f") || !strcmp(argv[i], "--firmware")) {
		
					// Display message
					cout << "Switching printer into firmware mode" << endl;
		
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Check if connecting to printer failed
					if(!printer.connect(serialPort)) {
		
						// Display error
						cout << printer.getStatus() << endl;
					
						// Return
						return EXIT_FAILURE;
					}
				
					// Check if printer is already in firmware mode
					if(printer.inFirmwareMode())
				
						// Display message
						cout << "Printer is already in firmware mode" << endl;
				
					// Otherwise
					else {
				
						// Put printer into firmware mode
						printer.switchToFirmwareMode();
				
						// Check if printer isn't connected
						if(!printer.isConnected()) {
		
							// Display error
							cout << printer.getStatus() << endl;
						
							// Return
							return EXIT_FAILURE;
						}
				
						// Display message
						cout << "Printer has been successfully switched into firmware mode" << endl;
					}
				
					// Display current serial port
					cout << "Current serial port: " << printer.getCurrentSerialPort() << endl;
			
					// Return
					return EXIT_SUCCESS;
				}
				
				// Otherwise check if a switching printer into bootloader mode
				else if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bootloader")) {
		
					// Display message
					cout << "Switching printer into bootloader mode" << endl;
		
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Check if connecting to printer failed
					if(!printer.connect(serialPort)) {
		
						// Display error
						cout << printer.getStatus() << endl;
					
						// Return
						return EXIT_FAILURE;
					}
				
					// Check if printer is already in bootloader mode
					if(printer.inBootloaderMode())
				
						// Display message
						cout << "Printer is already in bootloader mode" << endl;
				
					// Otherwise
					else {
				
						// Put printer into bootloader mode
						printer.switchToBootloaderMode();
				
						// Check if printer isn't connected
						if(!printer.isConnected()) {
		
							// Display error
							cout << printer.getStatus() << endl;
						
							// Return
							return EXIT_FAILURE;
						}
				
						// Display message
						cout << "Printer has been successfully switched into bootloader mode" << endl;
					}
				
					// Display current serial port
					cout << "Current serial port: " << printer.getCurrentSerialPort() << endl;
			
					// Return
					return EXIT_SUCCESS;
				}
		
				// Otherwise check installing iMe firmware
				else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--ime")) {
		
					// Display message
					cout << "Installing iMe firmware" << endl;
		
					// Set firmware location
					string firmwareLocation = getTemporaryLocation() + "iMe " TOSTRING(IME_ROM_VERSION_STRING) ".hex";
		
					// Check if creating iMe firmware ROM failed
					ofstream fout(firmwareLocation, ios::binary);
					if(fout.fail()) {
					
						// Display error
						cout << "Failed to unpack iMe firmware" << endl;
					
						// Return
						return EXIT_FAILURE;
					}
				
					// Unpack iMe ROM
					for(uint64_t i = 0; i < IME_HEX_SIZE; i++)
						fout.put(IME_HEX_DATA[i]);
					fout.close();
			
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Install firmware
					if(!installFirmware(firmwareLocation, serialPort))
					
						// Return
						return EXIT_FAILURE;
			
					// Return
					return EXIT_SUCCESS;
				}
				
				// Otherwise check installing M3D firmware
				else if(!strcmp(argv[i], "-3") || !strcmp(argv[i], "--m3d")) {
		
					// Display message
					cout << "Installing M3D firmware" << endl;
		
					// Set firmware location
					string firmwareLocation = getTemporaryLocation() + "M3D " TOSTRING(M3D_ROM_VERSION_STRING) ".hex";
		
					// Check if creating M3D firmware ROM failed
					ofstream fout(firmwareLocation, ios::binary);
					if(fout.fail()) {
					
						// Display error
						cout << "Failed to unpack M3D firmware" << endl;
					
						// Return
						return EXIT_FAILURE;
					}
				
					// Unpack M3D ROM
					for(uint64_t i = 0; i < M3D_HEX_SIZE; i++)
						fout.put(M3D_HEX_DATA[i]);
					fout.close();
			
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Install firmware
					if(!installFirmware(firmwareLocation, serialPort))
					
						// Return
						return EXIT_FAILURE;
			
					// Return
					return EXIT_SUCCESS;
				}
		
				// Otherwise check if a firmware ROM is provided
				else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--firmwarerom")) {
		
					// Display message
					cout << "Installing firmware" << endl;
	
					// Check if firmware ROM parameter doesn't exist
					if(i >= argc - 1) {
		
						// Display error
						cout << "No firmware ROM provided" << endl;
					
						// Return
						return EXIT_FAILURE;
					}

					// Set firmware location
					string firmwareLocation = static_cast<string>(argv[++i]);
			
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Install firmware
					if(!installFirmware(firmwareLocation, serialPort))
					
						// Return
						return EXIT_FAILURE;
			
					// Return
					return EXIT_SUCCESS;
				}
		
				// Otherwise check if using manual mode
				else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--manual")) {
		
					// Display message
					cout << "Starting manual mode" << endl;
		
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Check if connecting to printer failed
					if(!printer.connect(serialPort)) {
		
						// Display error
						cout << printer.getStatus() << endl;
					
						// Return
						return EXIT_FAILURE;
					}
			
					// Display message
					cout << "Enter 'quit' to exit" << endl;
		
					// Loop forever
					while(1) {

						// Get command from user
						cout << "Enter command: ";
						string line;
						getline(cin, line);
					
						// Check if quit is requested
						if(line == "quit")
					
							// Break;
							break;
						
						// Set changed mode
						bool changedMode = (line == "Q" && printer.getOperatingMode() == BOOTLOADER) || (line == "M115 S628" && printer.getOperatingMode() == FIRMWARE);
				
						// Check if failed to send command to the printer
						if(!printer.sendRequest(line)) {
	
							// Display error
							cout << (printer.isConnected() ? "Sending command failed" : "Printer disconnected") << endl << endl;
						
							// Return
							return EXIT_FAILURE;
						}
				
						// Display command
						cout << "Send: " << line << endl;
				
						// Wait until command receives a response
						for(string response; !changedMode && response.substr(0, 2) != "ok" && response.substr(0, 2) != "rs" && response.substr(0, 4) != "skip" && response.substr(0, 5) != "Error";) {
		
							// Get response
							do {
								response = printer.receiveResponse();
							} while(response.empty() && printer.isConnected());
						
							// Check if printer isn't connected
							if(!printer.isConnected()) {
						
								// Display error
								cout << "Printer disconnected" << endl << endl;
							
								// Return
								return EXIT_FAILURE;
							}
				
							// Display response
							if(response != "wait")
								cout << "Receive: " << response << endl;
							
							// Check if printer is in bootloader mode
							if(printer.getOperatingMode() == BOOTLOADER)
						
								// Break
								break;
						}
						cout << endl;
					}
				
					// Return
					return EXIT_SUCCESS;
				}
			}
		
			// Display error
			cout << "Invalid parameters" << endl;
		
			// Return
			return EXIT_FAILURE;
		}
	
		// Otherwise
		else {
	
			// Display error
			cout << "Invalid parameters" << endl;
		
			// Return
			return EXIT_FAILURE;
		}
		
		// Return
		return EXIT_SUCCESS;
	#endif
}


// Supporting function implementation
void breakHandler(int signal) {

	// Terminates the process normally
	exit(EXIT_FAILURE);
}

// Check if not using GUI
#ifndef USE_GUI

	bool installFirmware(const string &firmwareLocation, const string &serialPort) {

		// Check if firmware ROM doesn't exists
		ifstream file(firmwareLocation, ios::binary);
		if(!file.good()) {

			// Display error
			cout << "Firmware ROM doesn't exist" << endl;
		
			// Return false
			return false;
		}
		
		// Check if firmware ROM name is valid
		uint8_t endOfNumbers = 0;
		if(firmwareLocation.find_last_of('.') != string::npos)
			endOfNumbers = firmwareLocation.find_last_of('.') - 1;
		else
			endOfNumbers = firmwareLocation.length() - 1;
	
		int8_t beginningOfNumbers = endOfNumbers;
		for(; beginningOfNumbers >= 0 && isdigit(firmwareLocation[beginningOfNumbers]); beginningOfNumbers--);

		if(beginningOfNumbers != endOfNumbers - 10) {

			// Display error
			cout << "Invalid firmware ROM name" << endl;
		
			// Return false
			return false;
		}

		// Check if connecting to printer failed
		if(!printer.connect(serialPort)) {

			// Display error
			cout << printer.getStatus() << endl;
		
			// Return false
			return false;
		}

		// Check if installing printer's firmware failed
		if(!printer.installFirmware(firmwareLocation.c_str())) {

			// Display error
			cout << "Failed to update firmware" << endl;
		
			// Return false
			return false;
		}
	
		// Display message
		cout << "Firmware successfully installed" << endl;
	
		// Display current serial port
		cout << "Current serial port: " << printer.getCurrentSerialPort() << endl;
	
		// Return true
		return true;
	}
#endif
