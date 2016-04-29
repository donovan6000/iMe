// Header files
#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#ifdef USE_GUI
	#include <wx/wxprec.h>
	#ifndef WX_PRECOMP
		#include <wx/wx.h>
	#endif
	#include <wx/sysopt.h>
	#include <wx/artprov.h>
#endif
#ifdef WINDOWS
	#include <windows.h>
	#include <tchar.h>
	#include <setupapi.h>
	#include <dbt.h>
#endif
#include "printer.h"

// Packed files
#include "iMe 1900000001_hex.h"
#include "M3D_cat.h"
#include "M3D_inf.h"
#include "_90_m3d_local_rules.h"

using namespace std;


// Definitions
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


// Global variables
Printer printer;


// Check if using GUI
#ifdef USE_GUI

	// My app class
	class MyApp: public wxApp {

		// Public
		public:
	
			// On init
			virtual bool OnInit();
	};

	// My frame class
	class MyFrame: public wxFrame {
	
		// Public
		public:
	
			// Constructor
			MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
				wxFrame(nullptr, wxID_ANY, title, pos, size, style)
			{
			
				// Add icon
				#ifdef WINDOWS
					SetIcon(wxICON(MAINICON));
				#endif
			
				// Add menu bar
				#ifdef OSX
					wxMenuBar *menuBar = new wxMenuBar;
					SetMenuBar(menuBar);
				#endif
			
				// Create panel
				wxPanel *panel = new wxPanel(this, wxID_ANY);
			
				// Create version text
				versionText = new wxStaticText(panel, wxID_ANY, "V" TOSTRING(VERSION));
				wxFont font = versionText->GetFont();
				font.SetPointSize(9);
				versionText->SetFont(font);
				
				// Align version text to the bottom right
				wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
				hbox1->Add(new wxPanel(panel, wxID_ANY));
				
				wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
				hbox2->Add(versionText);
				
				wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
				vbox->Add(hbox1, 1, wxEXPAND);
				vbox->Add(hbox2, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 1);
				panel->SetSizer(vbox);
				
				// Create serial port text
				new wxStaticText(panel, wxID_ANY, "Serial Port", wxPoint(10, 17));
				
				// Create serial port choice
				serialPortChoice = new wxChoice(panel, wxID_ANY, wxPoint(87, 10), wxSize(311, -1), getAvailableSerialPorts());
				serialPortChoice->SetSelection(serialPortChoice->FindString("Auto"));
				
				// Create refresh serial ports button
				refreshSerialPortsButton = new wxBitmapButton(panel, wxID_ANY, wxArtProvider::GetBitmap(wxART_FIND), wxPoint(403, 10), wxSize(30, 30));
				Connect(refreshSerialPortsButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MyFrame::onRefreshSerialPortsButtonClick));
				
				// Create connect button
				connectButton = new wxButton(panel, wxID_ANY, "Connect", wxPoint(438, 10));
				Connect(connectButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MyFrame::connectToPrinter));
				
				// Create switch to firmware mode button
				wxButton *switchToFirmwareModeButton = new wxButton(panel, wxID_ANY, "Switch to firmware mode", wxPoint(9, 50));
				switchToFirmwareModeButton->Enable(false);
				
				// Create install iMe firmware button
				wxButton *installImeFirmwareButton = new wxButton(panel, wxID_ANY, "Install iMe firmware", wxPoint(195, 50));
				installImeFirmwareButton->Enable(false);
				
				// Create install firmware with file button
				wxButton *installFirmwareFromFileButton = new wxButton(panel, wxID_ANY, "Install firmware from file", wxPoint(345, 50));
				installFirmwareFromFileButton->Enable(false);
				//Connect(connectButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MyFrame::onInstallFirmwareFromFileButtonClick));
				
				// Create connection text
				connectionText = new wxStaticText(panel, wxID_ANY, "Connection: Not connected", wxPoint(10, 94), wxSize(300, -1));
				wxTimer *connectionTimer = new wxTimer(this, wxID_ANY);
				connectionTimer->Start(100);
				Connect(connectionTimer->GetId(), wxEVT_TIMER, wxTimerEventHandler(MyFrame::onConnectionTimer));
				
				// Create install drivers button
				wxButton *installDriversButton = new wxButton(panel, wxID_ANY, "Install drivers", wxPoint(420, 89));
				
				// Check if using Windows
				#ifdef WINDOWS
				
					// Monitor device notifications
					DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
					SecureZeroMemory(&notificationFilter, sizeof(notificationFilter));
					notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
					notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
					RegisterDeviceNotification(GetHWND(), &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
				#endif
			}
			
			// Connect to printer
			void connectToPrinter(wxCommandEvent& event) {
			
				connectButton->Enable(false);
				connectionText->SetLabel(static_cast<string>("Connection: ") + (printer.getStatus() == "Connected" ? "Reconnecting" : "Connecting"));
				
				// Update window
				Refresh(); 
				Update();
			
				wxString currentChoice = serialPortChoice->GetString(serialPortChoice->GetSelection());
				printer.connect(currentChoice != "Auto" ? static_cast<string>(currentChoice) : "");
				
				connectButton->Enable(true);
			}
			
			// Update connection
			void onConnectionTimer(wxTimerEvent& event) {
			
				// Update connection text
				connectionText->SetLabel("Connection: " + printer.getStatus());
			}
			
			// On install firmware from file button click
			void onInstallFirmwareFromFileButtonClick(wxCommandEvent& event) {
				
				wxFileDialog *openFileDialog = new wxFileDialog(this, "Open firmware file", wxEmptyString, wxEmptyString, "Firmware files|*.hex;*.bin;*.rom|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

				if(openFileDialog->ShowModal() == wxID_OK){
					wxString fileName = openFileDialog->GetPath();
					versionText->SetLabel(fileName);     
				}
			}
			
			// Get available serial ports
			wxArrayString getAvailableSerialPorts() {
			
				// Initialize serial ports
				wxArrayString serialPorts;
				serialPorts.Add("Auto");
				
				// Get available serial ports
				vector<string> availableSerialPorts = printer.getAvailableSerialPorts();
				for(uint8_t i = 0; i < availableSerialPorts.size(); i++)
					serialPorts.Add(availableSerialPorts[i].c_str());
				
				// Return serial ports
				return serialPorts;
			}
			
			// On refresh serial ports button click
			void onRefreshSerialPortsButtonClick(wxCommandEvent& event) {
			
				// Disable widgets
				serialPortChoice->Enable(false);
				refreshSerialPortsButton->Enable(false);
				connectButton->Enable(false);
				
				// Update windows
				Refresh(); 
				Update();
				usleep(2000000);
				
				// Save current choice
				wxString currentChoice = serialPortChoice->GetString(serialPortChoice->GetSelection());
			
				// Refresh choices
				serialPortChoice->Clear();
				serialPortChoice->Append(getAvailableSerialPorts());
				
				// Set choice to auto if previous choice doesn't exist anymore
				if(serialPortChoice->FindString(currentChoice) == wxNOT_FOUND)
					currentChoice = "Auto";
				
				// Select choice
				serialPortChoice->SetSelection(serialPortChoice->FindString(currentChoice));
				
				// Enable widgets
				serialPortChoice->Enable(true);
				refreshSerialPortsButton->Enable(true);
				connectButton->Enable(true);
			}
			
			// Check if using Windows
			#ifdef WINDOWS
			
				WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) {
				
					// Check if device change
					if(message == WM_DEVICECHANGE)
					
						// Check if an interface device was removed
						if(wParam == DBT_DEVICEREMOVECOMPLETE && reinterpret_cast<PDEV_BROADCAST_HDR>(lParam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
							
							// Check if device has the printer's PID and VID
							PDEV_BROADCAST_DEVICEINTERFACE deviceInterface = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(lParam);
							
							if(!_tcsnicmp(deviceInterface->dbcc_name, _T("\\\\?\\USB#VID_03EB&PID_2404"), strlen("\\\\?\\USB#VID_03EB&PID_2404"))) {
								
								// Get device ID
								wstring deviceId = &deviceInterface->dbcc_name[strlen("\\\\?\\USB#VID_03EB&PID_2404") + 1];
								deviceId = _T("USB\\VID_03EB&PID_2404\\") + deviceId.substr(0, deviceId.find(_T("#")));
								
								// Check if getting all connected devices was successful
								HDEVINFO deviceInfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);
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
											if(SetupDiGetDeviceInstanceId(deviceInfo, deviceInfoData, buffer, sizeof(buffer), nullptr) && !_tcsnicmp(buffer, deviceId.c_str(), deviceId.length())) {
					
												// Check if getting device registry key was successful
												HKEY deviceRegistryKey = SetupDiOpenDevRegKey(deviceInfo, deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
												if(deviceRegistryKey) {
		
													// Check if getting port name was successful
													DWORD type = 0;
													DWORD size = sizeof(buffer);
													if(RegQueryValueEx(deviceRegistryKey, _T("PortName"), nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS && type == REG_SZ) {
				
														// Check if port is the printer's current port
														string currentPort = printer.getCurrentSerialPort();
														if(!_tcsicmp(buffer, wstring(currentPort.begin(), currentPort.end()).c_str()))
														
															// Disconnect pritner
															printer.disconnect();
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
							}
						}
					
					// Return event	
					return wxFrame::MSWWindowProc(message, wParam, lParam);
				}
			#endif
	
		// Private
		private:
		
			// Widgets
			wxChoice *serialPortChoice;
			wxButton *refreshSerialPortsButton;
			wxButton *connectButton;
			wxStaticText *versionText;
			wxStaticText *connectionText;
	};

	// Implement application
	wxIMPLEMENT_APP(MyApp);
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


// Main function
#ifdef USE_GUI
	bool MyApp::OnInit() {
#else
	int main(int argc, char *argv[]) {
#endif

	// Get temp location
	char* tempPath = getenv("TEMP");
	if(!tempPath)
		 tempPath = getenv("TMP");
	if(!tempPath)
		 tempPath = getenv("TMPDIR");
	string tempLocation = tempPath ? tempPath : P_tmpdir;
	
	// Attach break handler
	signal(SIGINT, breakHandler);
	
	// Check if not using GUI
	#ifndef USE_GUI
	
		// Display version
		cout << "M3D Manager V" TOSTRING(VERSION) << endl << endl;
	#endif
	
	// Check if using command line interface
	if(argc > 1) {
	
		// Check if using GUI
		#ifdef USE_GUI
		
			// Display version
			cout << "M3D Manager V" TOSTRING(VERSION) << endl << endl;
		#endif
	
		// Go through all commands
		for(uint8_t i = 0; i < argc; i++) {
		
			// Check if help is requested
			if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
		
				// Display help
				cout << "Usage: m3d-manager -d -s -i -m protocol -r firmware.rom serialport" << endl;
				cout << "-d | --drivers: Install device drivers" << endl;
				cout << "-s | --start: Switches printer into firmware mode" << endl;
				cout << "-i | --ime: Installs iMe firmware" << endl;
				cout << "-r | --rom: Installs the provided firmware" << endl;
				cout << "-m | --manual: Allows manually sending commands to the printer using the protocol Repetier or RepRap" << endl;
				cout << "serialport: The printer's serial port or it will automatically find the printer if not specified" << endl << endl;
			
				// Exit
				return false;
			}
		
			// Otherwise check if installing drivers
			else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--drivers")) {
			
				// Display message
				cout << "Installing drivers" << endl;
		
				// Check if using Windows
				#ifdef WINDOWS
			
					// Unpack drivers
					ofstream fout(tempLocation + "/M3D.cat", ios::binary);
					for(uint64_t i = 0; i < m3D_catSize; i++)
						fout.put(m3D_catData[i]);
					fout.close();

					fout.open(tempLocation + "/M3D.inf", ios::binary);
					for(uint64_t i = 0; i < m3D_infSize; i++)
						fout.put(m3D_infData[i]);
					fout.close();
				
					// Check if installing drivers failed
					TCHAR buffer[MAX_PATH];
					GetWindowsDirectory(buffer, MAX_PATH);
					#ifdef USE_GUI
						wstring
					#else
						string
					#endif
					path = buffer;
				
					string executablePath;
					ifstream file(path + "\\sysnative\\pnputil.exe");
					executablePath = file.good() ? "sysnative" : "System32";
					file.close();
				
					if(system((path + "\\" + executablePath + "\\pnputil.exe -i -a \"" + tempLocation + "\\M3D.inf\"").c_str())) {
					
						// Display error
						cout << "Failed to install drivers" << endl;
						return false;
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
					if(getuid()) {
			
						// Display error
						cout << "Elevated privileges required" << endl;
						return false;
					}
			
					// Unpack udev rule
					ofstream fout("/etc/udev/rules.d/90-m3d-local.rules", ios::binary);
					for(uint64_t i = 0; i < _90_m3d_local_rulesSize; i++)
						fout.put(_90_m3d_local_rulesData[i]);
					fout.close();
			
					// Check if applying udev rule failed
					if(system("/etc/init.d/udev restart")) {
				
						// Display error
						cout << "Failed to install drivers" << endl;
						return false;
					}
				
					// Display message
					cout << "Drivers successfully installed" << endl;
				#endif
			
				// Exit
				return false;
			}
	
			// Otherwise check if a switching printer into firmware mode
			else if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--start")) {
		
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
					return false;
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
						return false;
					}
				
					// Display message
					cout << "Printer has been successfully switched into firmware mode" << endl;
				}
				
				// Display current serial port
				cout << "Current serial port: " << printer.getCurrentSerialPort() << endl;
			
				// Exit
				return false;
			}
		
			// Otherwise check installing iMe firmware
			else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--ime")) {
		
				// Display message
				cout << "Installing iMe firmware" << endl;
		
				// Set firmware location
				string firmwareLocation = tempLocation + "/iMe 1900000001.hex";
		
				// Unpack iMe ROM
				ofstream fout(firmwareLocation, ios::binary);
				for(uint64_t i = 0; i < iMe1900000001_hexSize; i++)
					fout.put(iMe1900000001_hexData[i]);
				fout.close();
			
				// Set serial port
				string serialPort;
				if(i < argc - 1)
					serialPort = argv[argc - 1];
		
				// Install firmware
				if(!installFirmware(firmwareLocation, serialPort))
					
					// Exit
					return false;
			
				// Exit
				return false;
			}
		
			// Otherwise check if a firmware ROM is provided
			else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--firmwarerom")) {
		
				// Display message
				cout << "Installing firmware" << endl;
	
				// Check if firmware ROM parameter doesn't exist
				if(i >= argc - 1) {
		
					// Display error
					cout << "No firmware ROM provided" << endl;
					return false;
				}

				// Set firmware location
				string firmwareLocation = static_cast<string>(argv[++i]);
			
				// Set serial port
				string serialPort;
				if(i < argc - 1)
					serialPort = argv[argc - 1];
		
				// Install firmware
				if(!installFirmware(firmwareLocation, serialPort))
					
					// Exit
					return false;
			
				// Exit
				return false;
			}
		
			// Otherwise check if using manual mode
			else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--manual")) {
		
				// Display message
				cout << "Starting manual mode" << endl;
		
				// Check if protocol parameter doesn't exist
				if(i >= argc - 1) {
		
					// Display error
					cout << "No protocol provided" << endl;
					return false;
				}
			
				// Set protocol
				string protocol = static_cast<string>(argv[++i]);
			
				// Check if protocol is invalid
				if(protocol != "Repetier" && protocol != "RepRap") {
		
					// Display error
					cout << "Invalid protocol" << endl;
					return false;
				}
		
				// Set serial port
				string serialPort;
				if(i < argc - 1)
					serialPort = argv[argc - 1];
		
				// Check if connecting to printer failed
				if(!printer.connect(serialPort)) {
		
					// Display error
					cout << printer.getStatus() << endl;
					return false;
				}
			
				// Put printer into firmware mode
				printer.switchToFirmwareMode();
				
				// Check if printer isn't connected
				if(!printer.isConnected()) {
		
					// Display error
					cout << printer.getStatus() << endl;
					return false;
				}
			
				// Display message
				cout << "Enter 'quit' to exit" << endl;
		
				// Loop forever
				while(1) {

					// Get command from user
					cout << "Enter command: ";
					string line;
					getline(cin, line);
					
					// Exit if requested
					if(line == "quit")
						return false;
				
					// Check if failed to send command to the printer
					if(protocol == "Repetier" ? !printer.sendRequestBinary(line) : !printer.sendRequestAscii(line)) {
	
						// Display error
						cout << (printer.isConnected() ? "Sending command failed" : "Printer disconnected") << endl << endl;
						return false;
					}
				
					// Display command
					cout << "Send: " << line << endl;
				
					// Wait until command receives a response
					for(string response; line != "Q" && line != "M115 S628" && response.substr(0, 2) != "ok" && response.substr(0, 2) != "rs" && response.substr(0, 4) != "skip" && response.substr(0, 5) != "Error";) {
		
						// Get response
						do {
							response = printer.receiveResponseBinary();
						} while(response.empty() && printer.isConnected());
						
						// Check if printer isn't connected
						if(!printer.isConnected()) {
						
							// Display error
							cout << "Printer disconnected" << endl << endl;
							return false;
						}
				
						// Display response
						if(response != "wait")
							cout << "Receive: " << response << endl;
					}
					cout << endl;
				}
				
				// Exit
				return false;
			}
		}
		
		// Display error
		cout << "Invalid parameters" << endl;
		return false;
	}
	
	// Otherwise
	else {
	
		// Check if using GUI
		#ifdef USE_GUI
		
			// Check if using OS X
			#ifdef OSX
				
				// Enable plainer transition
				wxSystemOptions::SetOption(wxMAC_WINDOW_PLAIN_TRANSITION, 1);
			#endif
	
			// Create and show window
			MyFrame *frame = new MyFrame("M3D Manager", wxDefaultPosition, wxSize(531, 135), wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));
			frame->Center();
			frame->Show(true);
		
		// Otherwise
		#else
		
			// Display error
			cout << "Invalid parameters" << endl;
			return false;
		#endif
	}
	
	// Return true
	return true;
}


// Supporting function implementation
void breakHandler(int signal) {

	// Exit so that destructor is called on printer
	exit(0);
}

bool installFirmware(const string &firmwareLocation, const string &serialPort) {

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
	if(!printer.connect(serialPort)) {

		// Display error
		cout << printer.getStatus() << endl;
		return false;
	}

	// Check if installing printer's firmware failed
	if(!printer.installFirmware(firmwareLocation.c_str())) {

		// Display error
		cout << "Failed to update firmware" << endl;
		return false;
	}

	// Put printer into firmware mode
	printer.switchToFirmwareMode();
	
	// Check if printer isn't connected
	if(!printer.isConnected()) {

		// Display error
		cout << printer.getStatus() << endl;
		return false;
	}
	
	// Display message
	cout << "Firmware successfully installed" << endl;
	
	// Display current serial port
	cout << "Current serial port: " << printer.getCurrentSerialPort() << endl;
	
	// Return true
	return true;
}
