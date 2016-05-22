// Header files
#include <fstream>
#include <wx/mstream.h>
#include "gui.h"
#ifdef WINDOWS
	#include <setupapi.h>
	#include <dbt.h>
#endif


// Custom events
wxDEFINE_EVENT(wxEVT_THREAD_TASK_START, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_THREAD_TASK_COMPLETE, wxThreadEvent);


// Supporting function implementation
wxBitmap loadImage(const unsigned char *data, unsigned long long size, int width = -1, int height = -1) {
	
	// Load image from data
	wxMemoryInputStream imageStream(data, size);
	wxImage image;
	image.LoadFile(imageStream, wxBITMAP_TYPE_PNG);
	
	// Resize image
	image.Rescale(width == -1 ? image.GetWidth() : width, height == -1 ? image.GetHeight() : height, wxIMAGE_QUALITY_HIGH);
	
	// Return bitmap
	return wxBitmap(image);
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxFrame(nullptr, wxID_ANY, title, pos, size, style)
{

	// Initialize PNG image handler
	wxImage::AddHandler(new wxPNGHandler);
	
	// Set icon
	#ifdef WINDOWS
		SetIcon(wxICON(MAINICON));
	#endif
	
	#ifdef LINUX
		wxIcon icon;
		icon.CopyFromBitmap(loadImage(icon_pngData, sizeof(icon_pngData))); 
		SetIcon(icon);
	#endif

	// Add menu bar
	#ifdef OSX
		wxMenuBar *menuBar = new wxMenuBar;
		SetMenuBar(menuBar);
	#endif
	
	// Close event
	Bind(wxEVT_CLOSE_WINDOW, &MyFrame::close, this);
	
	// Show event
	Bind(wxEVT_SHOW, &MyFrame::show, this);

	// Create panel
	wxPanel *panel = new wxPanel(this, wxID_ANY);
	
	// Create connection box
	new wxStaticBox(panel, wxID_ANY, "Connection", wxPoint(5, 0), wxSize(
	#ifdef WINDOWS
		531, 90
	#endif
	#ifdef OSX
		534, 82
	#endif
	#ifdef LINUX
		534, 97
	#endif
	));
	
	// Create serial port text
	new wxStaticText(panel, wxID_ANY, "Serial Port", wxPoint(
	#ifdef WINDOWS
		15, 24
	#endif
	#ifdef OSX
		15, 23
	#endif
	#ifdef LINUX
		15, 28
	#endif
	));
	
	// Create serial port choice
	serialPortChoice = new wxChoice(panel, wxID_ANY, wxPoint(
	#ifdef WINDOWS
		82, 20
	#endif
	#ifdef OSX
		87, 20
	#endif
	#ifdef LINUX
		92, 20
	#endif
	), wxSize(
	#ifdef WINDOWS
		313, -1
	#endif
	#ifdef OSX
		308, -1
	#endif
	#ifdef LINUX
		313, -1
	#endif
	), getAvailableSerialPorts());
	serialPortChoice->SetSelection(serialPortChoice->FindString("Auto"));
	
	// Create refresh serial ports button
	refreshSerialPortsButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData), 17, 17), wxPoint(
	#ifdef WINDOWS
		405, 19
	#endif
	#ifdef OSX
		400, 16
	#endif
	#ifdef LINUX
		410, 22
	#endif
	), wxSize(
	#ifdef WINDOWS
		-1, -1
	#endif
	#ifdef OSX
		27, -1
	#endif
	#ifdef LINUX
		-1, -1
	#endif
	));
	refreshSerialPortsButton->Bind(wxEVT_BUTTON, &MyFrame::refreshSerialPorts, this);
	
	// Create connect button
	connectButton = new wxButton(panel, wxID_ANY, "Connect", wxPoint(
	#ifdef WINDOWS
		439, 18
	#endif
	#ifdef OSX
		435, 18
	#endif
	#ifdef LINUX
		445, 22
	#endif
	));
	connectButton->Bind(wxEVT_BUTTON, &MyFrame::connectToPrinter, this);
	
	// Create status text
	new wxStaticText(panel, wxID_ANY, "Status:", wxPoint(
	#ifdef WINDOWS
		15, 59
	#endif
	#ifdef OSX
		15, 54
	#endif
	#ifdef LINUX
		15, 64
	#endif
	));
	statusText = new wxStaticText(panel, wxID_ANY, "Not connected", wxPoint(
	#ifdef WINDOWS
		53, 59
	#endif
	#ifdef OSX
		62, 54
	#endif
	#ifdef LINUX
		65, 64
	#endif
	), wxSize(300, -1));
	statusText->SetForegroundColour(wxColour(255, 0, 0));
	
	// Create status timer
	statusTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateStatus, this, statusTimer->GetId());
	statusTimer->Start(100);
	
	// Check if not using OS X
	#ifndef OSX
	
		// Create install drivers button
		installDriversButton = new wxButton(panel, wxID_ANY, "Install drivers", wxPoint(
		#ifdef WINDOWS
			437, 54
		#endif
		#ifdef LINUX
			425, 58
		#endif
		));
		installDriversButton->Bind(wxEVT_BUTTON, &MyFrame::installDrivers, this);
	#endif
	
	// Create firmware box
	new wxStaticBox(panel, wxID_ANY, "Firmware", wxPoint(
	#ifdef WINDOWS
		5, 91
	#endif
	#ifdef OSX
		5, 83
	#endif
	#ifdef LINUX
		5, 98
	#endif
	), wxSize(
	#ifdef WINDOWS
		531, 58
	#endif
	#ifdef OSX
		534, 59
	#endif
	#ifdef LINUX
		534, 60
	#endif
	));
	
	// Create switch to firmware mode button
	switchToFirmwareModeButton = new wxButton(panel, wxID_ANY, "Switch to firmware mode", wxPoint(
	#ifdef WINDOWS
		14, 111
	#endif
	#ifdef OSX
		11, 103
	#endif
	#ifdef LINUX
		14, 118
	#endif
	));
	switchToFirmwareModeButton->Enable(false);
	switchToFirmwareModeButton->Bind(wxEVT_BUTTON, &MyFrame::switchToFirmwareMode, this);
	
	// Create install iMe firmware button
	installImeFirmwareButton = new wxButton(panel, wxID_ANY, "Install iMe firmware", wxPoint(
	#ifdef WINDOWS
		209, 111
	#endif
	#ifdef OSX
		195, 103
	#endif
	#ifdef LINUX
		200, 118
	#endif
	));
	installImeFirmwareButton->Enable(false);
	installImeFirmwareButton->Bind(wxEVT_BUTTON, &MyFrame::installIMe, this);
	
	// Create install firmware with file button
	installFirmwareFromFileButton = new wxButton(panel, wxID_ANY, "Install firmware from file", wxPoint(
	#ifdef WINDOWS
		377, 111
	#endif
	#ifdef OSX
		347, 103
	#endif
	#ifdef LINUX
		350, 118
	#endif
	));
	installFirmwareFromFileButton->Enable(false);
	installFirmwareFromFileButton->Bind(wxEVT_BUTTON, &MyFrame::installFirmwareFromFile, this);
	
	// Create console box
	new wxStaticBox(panel, wxID_ANY, "Console", wxPoint(
	#ifdef WINDOWS
		5, 150
	#endif
	#ifdef OSX
		5, 143
	#endif
	#ifdef LINUX
		5, 159
	#endif
	), wxSize(
	#ifdef WINDOWS
		531, 247
	#endif
	#ifdef OSX
		534, 267
	#endif
	#ifdef LINUX
		534, 269
	#endif
	));
	
	// Create console output
	consoleOutput = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxPoint(
	#ifdef WINDOWS
		15, 170
	#endif
	#ifdef OSX
		19, 168
	#endif
	#ifdef LINUX
		15, 179
	#endif
	), wxSize(
	#ifdef WINDOWS
		511, 182
	#endif
	#ifdef OSX
		508, 198
	#endif
	#ifdef LINUX
		514, 200
	#endif
	), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
	consoleOutput->SetFont(wxFont(
	#ifdef WINDOWS
		8
	#endif
	#ifdef OSX
		10
	#endif
	#ifdef LINUX
		8
	#endif
	, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	
	// Create command input
	commandInput = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxPoint(
	#ifdef WINDOWS
		15, 362
	#endif
	#ifdef OSX
		17, 374
	#endif
	#ifdef LINUX
		14, 389
	#endif
	), wxSize(
	#ifdef WINDOWS
		415, -1
	#endif
	#ifdef OSX
		416, -1
	#endif
	#ifdef LINUX
		426, -1
	#endif
	));
	commandInput->SetHint("Command");
	commandInput->Bind(wxEVT_KEY_DOWN, [=](wxKeyEvent& event) {
	
		// Check if return was pressed
		if(event.GetKeyCode() == WXK_RETURN && sendCommandButton->IsEnabled()) {
		
			// Send command
			wxCommandEvent sendCommandEvent(wxEVT_BUTTON, sendCommandButton->GetId());
			wxPostEvent(sendCommandButton, sendCommandEvent);
		}
		
		// Allow key press to propagate
		event.Skip();
	});
	
	// Create send command button
	sendCommandButton = new wxButton(panel, wxID_ANY, "Send", wxPoint(
	#ifdef WINDOWS
		439, 360
	#endif
	#ifdef OSX
		435, 371
	#endif
	#ifdef LINUX
		445, 389
	#endif
	));
	sendCommandButton->Bind(wxEVT_BUTTON, &MyFrame::sendCommand, this);
	sendCommandButton->Enable(false);
	
	// Create version text
	string iMeVersion = static_cast<string>(TOSTRING(IME_ROM_VERSION_STRING)).substr(2);
	for(uint8_t i = 0; i < 3; i++)
		iMeVersion.insert(i * 2 + 2 + i, ".");
	versionText = new wxStaticText(panel, wxID_ANY, "M3D Manager V" TOSTRING(VERSION) " - iMe V" + iMeVersion, wxDefaultPosition, wxSize(
	#ifdef WINDOWS
		-1, 15
	#endif
	#ifdef OSX
		-1, 14
	#endif
	#ifdef LINUX
		-1, 13
	#endif
	), wxALIGN_RIGHT);
	wxFont font = versionText->GetFont();
	font.SetPointSize(
	#ifdef WINDOWS
		9
	#endif
	#ifdef OSX
		11
	#endif
	#ifdef LINUX
		9
	#endif
	);
	versionText->SetFont(font);
	
	// Align version text to the bottom right
	wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
	hbox1->Add(new wxPanel(panel, wxID_ANY));
	wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
	hbox2->Add(versionText);
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(hbox1, 1, wxEXPAND);
	vbox->Add(hbox2, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM,
	#ifdef WINDOWS
		2
	#endif
	#ifdef OSX
		2
	#endif
	#ifdef LINUX
		3
	#endif
	);
	panel->SetSizer(vbox);
	
	// Thread task events
	Bind(wxEVT_THREAD_TASK_START, &MyFrame::threadTaskStart, this);
	Bind(wxEVT_THREAD_TASK_COMPLETE, &MyFrame::threadTaskComplete, this);
	
	// Check if creating thread failed
	if(CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR || GetThread()->Run() != wxTHREAD_NO_ERROR)
	
		// Close
		Close();
	
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

void MyFrame::show(wxShowEvent &event) {

	// Set initial focus
	#ifdef OSX
		commandInput->SetFocus();
	#else
		SetFocus();
	#endif
}

wxThread::ExitCode MyFrame::Entry() {

	// Loop until destroyed
	while(!GetThread()->TestDestroy()) {
		
		// Initialize thread task
		function<ThreadTaskResponse()> threadTask;
		
		{
			// Lock
			wxCriticalSectionLocker lock(criticalLock);
			
			// Check if a thread task exists
			if(!threadTaskQueue.empty()) {
	
				// Set thread task to next thread task function
				threadTask = threadTaskQueue.front();
				threadTaskQueue.pop();
			}
		}
		
		// Check if thread task exists
		if(threadTask) {
		
			// Trigger thread start event
			wxQueueEvent(GetEventHandler(), new wxThreadEvent(wxEVT_THREAD_TASK_START));
			
			// Perform thread task
			ThreadTaskResponse response = threadTask();
		
			// Trigger thread complete event
			wxThreadEvent *event = new wxThreadEvent(wxEVT_THREAD_TASK_COMPLETE);
			event->SetPayload(response);
			wxQueueEvent(GetEventHandler(), event);
		}
		
		// Delay
		#ifdef WINDOWS
			Sleep(100);
		#else
			usleep(100000);
		#endif
	}
	
	// Return
	return EXIT_SUCCESS;
}

void MyFrame::threadTaskStart(wxThreadEvent& event) {

	// Initialize callback function
	function<void()> callbackFunction;
	
	{
		// Lock
		wxCriticalSectionLocker lock(criticalLock);
		
		// Check if a thread start callback is specified and one exists
		if(!threadStartCallbackQueue.empty()) {
	
			// Set callback function to next start callback function
			callbackFunction = threadStartCallbackQueue.front();
			threadStartCallbackQueue.pop();
		}
	}

	// Run callback function if it exists
	if(callbackFunction)
		callbackFunction();
}

void MyFrame::threadTaskComplete(wxThreadEvent& event) {

	// Initialize callback function
	function<void(ThreadTaskResponse response)> callbackFunction;
	
	{
		// Lock
		wxCriticalSectionLocker lock(criticalLock);
		
		// Check if a thread complete callback is specified and one exists
		if(!threadCompleteCallbackQueue.empty()) {
	
			// Set callback function to next complete callback function
			callbackFunction = threadCompleteCallbackQueue.front();
			threadCompleteCallbackQueue.pop();
		}
	}

	// Run callback function if it exists
	if(callbackFunction)
		callbackFunction(event.GetPayload<ThreadTaskResponse>());
}

void MyFrame::close(wxCloseEvent& event) {

	// Disconnect printer
	printer.disconnect();
	
	// Check if thread is running
	if(GetThread() && GetThread()->IsRunning())
	
		// Destroy thread
		GetThread()->Delete();
	
	// Destroy self
	Destroy();
}

void MyFrame::connectToPrinter(wxCommandEvent& event) {

	// Check if connecting to printer
	if(connectButton->GetLabel() == "Connect") {
		
		// Get current serial port choice
		string currentChoice = static_cast<string>(serialPortChoice->GetString(serialPortChoice->GetSelection()));

		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();
		
			// Disable connection controls
			serialPortChoice->Enable(false);
			refreshSerialPortsButton->Enable(false);
			connectButton->Enable(false);

			// Disable printer controls
			installFirmwareFromFileButton->Enable(false);
			installImeFirmwareButton->Enable(false);
			switchToFirmwareModeButton->Enable(false);
			sendCommandButton->Enable(false);

			// Set status text
			statusText->SetLabel("Connecting");
			statusText->SetForegroundColour(wxColour(255, 180, 0));
		});
		
		// Append thread task to queue
		threadTaskQueue.push([=]() -> ThreadTaskResponse {
		
			// Connect to printer
			printer.connect(currentChoice != "Auto" ? currentChoice : "");
			
			// Return empty response
			return {"", 0};
		});

		// Append thread complete callback to queue
		threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
			// Enable connect button
			connectButton->Enable(true);

			// Check if connected to printer
			if(printer.isConnected()) {

				// Enable printer controls
				installFirmwareFromFileButton->Enable(true);
				installImeFirmwareButton->Enable(true);
				switchToFirmwareModeButton->Enable(true);
				sendCommandButton->Enable(true);
				
				// Change connect button to disconnect	
				connectButton->SetLabel("Disconnect");
				
				// Log connection details to console
				logToConsole("Connected to " + printer.getSerialNumber() + " at " + printer.getCurrentSerialPort() + " running " + printer.getFirmwareType() + " firmware V" + printer.getFirmwareVersion());
			}
			
			// Otherwise
			else {
			
				// Enable connection controls
				serialPortChoice->Enable(true);
				refreshSerialPortsButton->Enable(true);
			}

			// Start status timer
			statusTimer->Start(100);
		});
	}
	
	// Otherwise
	else {
		
		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();
		});
		
		// Append thread task to queue
		threadTaskQueue.push([=]() -> ThreadTaskResponse {
		
			// Disconnect printer
			printer.disconnect();
			
			// Return empty response
			return {"", 0};
		});

		// Append thread complete callback to queue
		threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
			// Disable printer controls
			installFirmwareFromFileButton->Enable(false);
			installImeFirmwareButton->Enable(false);
			switchToFirmwareModeButton->Enable(false);
			sendCommandButton->Enable(false);
		
			// Change connect button to connect
			connectButton->SetLabel("Connect");
		
			// Enable connection controls
			serialPortChoice->Enable(true);
			refreshSerialPortsButton->Enable(true);
		
			// Start status timer
			statusTimer->Start(100);
		});
	}
}

void MyFrame::switchToFirmwareMode(wxCommandEvent& event) {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable connection controls
		serialPortChoice->Enable(false);
		refreshSerialPortsButton->Enable(false);
		connectButton->Enable(false);
	
		// Disable printer controls
		installFirmwareFromFileButton->Enable(false);
		installImeFirmwareButton->Enable(false);
		switchToFirmwareModeButton->Enable(false);
		sendCommandButton->Enable(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Check if printer is already in firmware mode
		if(printer.inFirmwareMode())
		
			// Return message
			return {"Printer is already in firmware mode", wxOK | wxICON_EXCLAMATION | wxCENTRE};

		// Otherwise
		else {

			// Put printer into firmware mode
			printer.switchToFirmwareMode();

			// Check if printer isn't connected
			if(!printer.isConnected())
			
				// Return message
				return {"Failed to switch printer into firmware mode", wxOK | wxICON_ERROR | wxCENTRE};
		
			// Otherwise
			else
			
				// Return message
				return {"Printer has been successfully switched into firmware mode", wxOK | wxICON_INFORMATION | wxCENTRE};
		}
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		
		// Enable connection controls
		serialPortChoice->Enable(true);
		refreshSerialPortsButton->Enable(true);
		connectButton->Enable(true);

		// Check if connected to printer
		if(printer.isConnected()) {

			// Enable printer controls
			installFirmwareFromFileButton->Enable(true);
			installImeFirmwareButton->Enable(true);
			switchToFirmwareModeButton->Enable(true);
			sendCommandButton->Enable(true);
		}
		
		// Display message
		wxMessageBox(response.message, "M3D Manager", response.style);
	});
}

void MyFrame::installIMe(wxCommandEvent& event) {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Stop status timer
		statusTimer->Stop();

		// Disable connection controls
		serialPortChoice->Enable(false);
		refreshSerialPortsButton->Enable(false);
		connectButton->Enable(false);
	
		// Disable printer controls
		installFirmwareFromFileButton->Enable(false);
		installImeFirmwareButton->Enable(false);
		switchToFirmwareModeButton->Enable(false);
		sendCommandButton->Enable(false);
	
		// Set status text
		statusText->SetLabel("Installing firmware");
		statusText->SetForegroundColour(wxColour(255, 180, 0));
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Set firmware location
		string firmwareLocation = getTemporaryLocation() + "/iMe " TOSTRING(IME_ROM_VERSION_STRING) ".hex";

		// Check if creating iMe ROM failed
		ofstream fout(firmwareLocation, ios::binary);
		if(fout.fail())
		
			// Return message
			return {"Failed to unpack iMe firmware", wxOK | wxICON_ERROR | wxCENTRE};

		// Otherwise
		else {

			// Unpack iMe ROM
			for(uint64_t i = 0; i < IME_HEX_SIZE; i++)
				fout.put(IME_HEX_DATA[i]);
			fout.close();
		
			// Return install firmware's message
			return installFirmware(firmwareLocation);
		}
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Enable connection controls
		serialPortChoice->Enable(true);
		refreshSerialPortsButton->Enable(true);
		connectButton->Enable(true);
	
		// Check if connected to printer
		if(printer.isConnected()) {
	
			// Enable printer controls
			installFirmwareFromFileButton->Enable(true);
			installImeFirmwareButton->Enable(true);
			switchToFirmwareModeButton->Enable(true);
			sendCommandButton->Enable(true);
		}
		
		// Start status timer
		statusTimer->Start(100);
		
		// Display message
		wxMessageBox(response.message, "M3D Manager", response.style);
	});
}

void MyFrame::installFirmwareFromFile(wxCommandEvent& event) {
	
	// Display file dialog
	wxFileDialog *openFileDialog = new wxFileDialog(this, "Open firmware file", wxEmptyString, wxEmptyString, "Firmware files|*.hex;*.bin;*.rom|All files|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	// Check if a file was selected
	if(openFileDialog->ShowModal() == wxID_OK) {
	
		// Set firmware location
		string firmwareLocation = static_cast<string>(openFileDialog->GetPath());

		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();

			// Disable connection controls
			serialPortChoice->Enable(false);
			refreshSerialPortsButton->Enable(false);
			connectButton->Enable(false);
	
			// Disable printer controls
			installFirmwareFromFileButton->Enable(false);
			installImeFirmwareButton->Enable(false);
			switchToFirmwareModeButton->Enable(false);
			sendCommandButton->Enable(false);
	
			// Set status text
			statusText->SetLabel("Installing firmware");
			statusText->SetForegroundColour(wxColour(255, 180, 0));
		});
		
		// Append thread task to queue
		threadTaskQueue.push([=]() -> ThreadTaskResponse {
		
			// Return install firmware's message
			return installFirmware(firmwareLocation);
		});
	
		// Append thread complete callback to queue
		threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
			// Enable connection controls
			serialPortChoice->Enable(true);
			refreshSerialPortsButton->Enable(true);
			connectButton->Enable(true);
	
			// Check if connected to printer
			if(printer.isConnected()) {
	
				// Enable printer controls
				installFirmwareFromFileButton->Enable(true);
				installImeFirmwareButton->Enable(true);
				switchToFirmwareModeButton->Enable(true);
				sendCommandButton->Enable(true);
			}
		
			// Start status timer
			statusTimer->Start(100);
		
			// Display message
			wxMessageBox(response.message, "M3D Manager", response.style);
		});
	}
}

void MyFrame::installDrivers(wxCommandEvent& event) {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable install drivers button
		installDriversButton->Enable(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Check if using Windows
		#ifdef WINDOWS

			// Check if creating drivers file failed
			ofstream fout(getTemporaryLocation() + "/M3D.cat", ios::binary);
			if(fout.fail())
			
				// Return message
				return {"Failed to unpack drivers", wxOK | wxICON_ERROR | wxCENTRE};

			// Otherwise
			else {

				// Unpack drivers
				for(uint64_t i = 0; i < m3D_catSize; i++)
					fout.put(m3D_catData[i]);
				fout.close();

				// Check if creating drivers file failed
				fout.open(getTemporaryLocation() + "/M3D.inf", ios::binary);
				if(fout.fail())
				
					// Return message
					return {"Failed to unpack drivers", wxOK | wxICON_ERROR | wxCENTRE};

				// Otherwise
				else {

					// Unpack drivers
					for(uint64_t i = 0; i < m3D_infSize; i++)
						fout.put(m3D_infData[i]);
					fout.close();

					// Check if creating process failed
					TCHAR buffer[MAX_PATH];
					GetWindowsDirectory(buffer, MAX_PATH);
					wstring path = buffer;

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
					_tcscpy(command, (path + "\\" + executablePath + "\\pnputil.exe -i -a \"" + getTemporaryLocation() + "\\M3D.inf\"").c_str());

					if(!CreateProcess(nullptr, command, nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
					
						// Return message
						return {"Failed to install drivers", wxOK | wxICON_ERROR | wxCENTRE};

					// Otherwise
					else {

						// Wait until process finishes
						WaitForSingleObject(processInfo.hProcess, INFINITE);

						// Check if installing drivers failed
						DWORD exitCode;
						GetExitCodeProcess(processInfo.hProcess, &exitCode);

						// Close process and thread handles. 
						CloseHandle(processInfo.hProcess);
						CloseHandle(processInfo.hThread);

						if(!exitCode)
						
							// Return message
							return {"Failed to install drivers", wxOK | wxICON_ERROR | wxCENTRE};

						// Otherwise
						else
						
							// Return message
							return {"Drivers successfully installed. You might need to reconnect the printer to the computer for the drivers to take effect.", wxOK | wxICON_INFORMATION | wxCENTRE};
					}
				}
			}
		#endif

		// Otherwise check if using Linux
		#ifdef LINUX

			// Check if user is not root
			if(getuid())
			
				// Return message
				return {"Elevated privileges required", wxOK | wxICON_ERROR | wxCENTRE};

			// Otherwise
			else {

				// Check if creating udev rule failed
				ofstream fout("/etc/udev/rules.d/90-m3d-local.rules", ios::binary);
				if(fout.fail())
				
					// Return message
					return {"Failed to unpack udev rule", wxOK | wxICON_ERROR | wxCENTRE};

				// Otherwise
				else {

					// Unpack udev rule
					for(uint64_t i = 0; i < _90_m3d_local_rulesSize; i++)
						fout.put(_90_m3d_local_rulesData[i]);
					fout.close();

					// Check if applying udev rule failed
					if(system("/etc/init.d/udev restart"))
					
						// Return message
						return {"Failed to apply udev rule", wxOK | wxICON_ERROR | wxCENTRE};
	
					// Otherwise
					else
					
						// Return message
						return {"Drivers successfully installed. You might need to reconnect the printer to the computer for the drivers to take effect.", wxOK | wxICON_INFORMATION | wxCENTRE};
				}
			}
		#endif
		
		// Return empty response
		return {"", 0};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Enable install drivers button
		installDriversButton->Enable(true);
	
		// Display message
		wxMessageBox(response.message, "M3D Manager", response.style);
	});
}

void MyFrame::updateStatus(wxTimerEvent& event) {

	// Get printer status
	string status = printer.getStatus();
	
	// Check if printer is connected
	if(status == "Connected") {
	
		// Change connect button to disconnect
		connectButton->SetLabel("Disconnect");
		
		// Disable connection controls
		serialPortChoice->Enable(false);
		refreshSerialPortsButton->Enable(false);
		
		// Set status text color
		statusText->SetForegroundColour(wxColour(0, 255, 0));
	}
	
	// Otherwise
	else {
	
		// Check if printer was just disconnected
		if(status == "Disconnected" && (statusText->GetLabel() != status || connectButton->GetLabel() == "Disconnect")) {
	
			// Disable printer controls
			installFirmwareFromFileButton->Enable(false);
			installImeFirmwareButton->Enable(false);
			switchToFirmwareModeButton->Enable(false);
			sendCommandButton->Enable(false);
			
			// Change connect button to connect
			connectButton->SetLabel("Connect");
			
			// Enable connection controls
			serialPortChoice->Enable(true);
			refreshSerialPortsButton->Enable(true);
		}
		
		// Set status text color
		statusText->SetForegroundColour(wxColour(255, 0, 0));
	}

	// Update status text
	statusText->SetLabel(status);
}

wxArrayString MyFrame::getAvailableSerialPorts() {

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

void MyFrame::refreshSerialPorts(wxCommandEvent& event) {
	
	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable connection controls
		serialPortChoice->Enable(false);
		refreshSerialPortsButton->Enable(false);
		connectButton->Enable(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Delay
		#ifdef WINDOWS
			Sleep(300);
		#else
			usleep(300000);
		#endif
	
		// Get available serial ports
		wxArrayString serialPorts = getAvailableSerialPorts();
		
		// Combine serial ports into a single response
		string response;
		for(size_t i = 0; i < serialPorts.GetCount(); i++) 
			response += serialPorts[i] + ' ';
		response.pop_back();
		
		// Return serial ports
		return {response, 0};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
	
		// Enable connection controls
		serialPortChoice->Enable(true);
		refreshSerialPortsButton->Enable(true);
		connectButton->Enable(true);
		
		// Get serial ports from response
		wxArrayString serialPorts;
		for(size_t i = 0; i < response.message.length(); i++) {
		
			size_t startingOffset = i;
			i = response.message.find(' ', i);
			serialPorts.Add(response.message.substr(startingOffset, i));
			
			if(i == string::npos)
				break;
		}
		
		// Save current choice
		wxString currentChoice = serialPortChoice->GetString(serialPortChoice->GetSelection());

		// Refresh choices
		serialPortChoice->Clear();
		serialPortChoice->Append(serialPorts);
	
		// Set choice to auto if previous choice doesn't exist anymore
		if(serialPortChoice->FindString(currentChoice) == wxNOT_FOUND)
			currentChoice = "Auto";
	
		// Select choice
		serialPortChoice->SetSelection(serialPortChoice->FindString(currentChoice));
	});
}

ThreadTaskResponse MyFrame::installFirmware(const string &firmwareLocation) {

	// Check if firmware ROM doesn't exists
	ifstream file(firmwareLocation, ios::binary);
	if(!file.good())
	
		// Return message
		return {"Firmware ROM doesn't exist", wxOK | wxICON_ERROR | wxCENTRE};

	// Otherwise
	else {

		// Check if firmware ROM name is valid
		uint8_t endOfNumbers = 0;
		if(firmwareLocation.find_last_of('.') != string::npos)
			endOfNumbers = firmwareLocation.find_last_of('.') - 1;
		else
			endOfNumbers = firmwareLocation.length() - 1;

		uint8_t beginningOfNumbers = endOfNumbers - 10;
		for(; beginningOfNumbers && endOfNumbers > beginningOfNumbers && isdigit(firmwareLocation[endOfNumbers]); endOfNumbers--);

		if(endOfNumbers != beginningOfNumbers)
		
			// Return message
			return {"Invalid firmware name", wxOK | wxICON_ERROR | wxCENTRE};

		// Otherwise
		else {

			// Check if installing printer's firmware failed
			if(!printer.installFirmware(firmwareLocation.c_str()))
			
				// Return message
				return {"Failed to update firmware", wxOK | wxICON_ERROR | wxCENTRE};

			// Otherwise
			else {

				// Put printer into firmware mode
				printer.switchToFirmwareMode();

				// Check if printer isn't connected
				if(!printer.isConnected())
				
					// Return message
					return {"Failed to update firmware", wxOK | wxICON_ERROR | wxCENTRE};
	
				// Otherwise
				else
				
					// Return message
					return {"Firmware successfully installed", wxOK | wxICON_INFORMATION | wxCENTRE};
			}
		}
	}
}

void MyFrame::sendCommand(wxCommandEvent& event) {
	
	// Append thread task to send command to queue
	string command = static_cast<string>(commandInput->GetValue());
	commandInput->SetValue("");
	
	// Lock
	wxCriticalSectionLocker lock(criticalLock);
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Send command
		if(printer.inFirmwareMode())
			printer.sendRequest(command);
		else
			printer.sendRequestAscii(command);
		
		// Return empty response
		return {"", 0};
	});
}

void MyFrame::logToConsole(const string &text) {

	// Append text to console
	consoleOutput->AppendText(static_cast<string>(consoleOutput->GetValue().IsEmpty() ? "" : "\n") + text);
}

// Check if using Windows
#ifdef WINDOWS

	WXLRESULT MyFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) {
	
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
											
												// Disconnect printer
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