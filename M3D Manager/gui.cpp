// Header files
#include <fstream>
#include <iomanip>
#include <sstream>
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
wxBitmap loadImage(const unsigned char *data, unsigned long long size, int width = -1, int height = -1, int offsetX = 0, int offsetY = 0) {
	
	// Load image from data
	wxMemoryInputStream imageStream(data, size);
	wxImage image;
	image.LoadFile(imageStream, wxBITMAP_TYPE_PNG);
	
	// Resize image
	if(width == -1 || height == -1)
		image.Rescale(width == -1 ? image.GetWidth() : width, height == -1 ? image.GetHeight() : height, wxIMAGE_QUALITY_HIGH);
	
	// Offset image
	if(offsetX || offsetY)
		image.Resize(wxSize(image.GetWidth() + offsetX, image.GetHeight() + offsetY), wxPoint(offsetX, offsetY));
	
	// Return bitmap
	return wxBitmap(image);
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxFrame(nullptr, wxID_ANY, title, pos, size, style)
{

	// Set printer's log function
	printer.setLogFunction([=](const string &message) -> void {
	
		// Log message to console
		logToConsole(message);
	});
	
	// Clear establishing printer connection
	establishingPrinterConnection = false;

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
		542, 90
	#endif
	#ifdef OSX
		549, 82
	#endif
	#ifdef LINUX
		549, 97
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
		324, -1
	#endif
	#ifdef OSX
		324, -1
	#endif
	#ifdef LINUX
		328, -1
	#endif
	), getAvailableSerialPorts());
	serialPortChoice->SetSelection(serialPortChoice->FindString("Auto"));
	
	// Create refresh serial ports button
	refreshSerialPortsButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		416, 19
	#endif
	#ifdef OSX
		416, 15
	#endif
	#ifdef LINUX
		425, 22
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
	
	// Create connection button
	connectionButton = new wxButton(panel, wxID_ANY, "Connect", wxPoint(
	#ifdef WINDOWS
		450, 18
	#endif
	#ifdef OSX
		451, 18
	#endif
	#ifdef LINUX
		460, 22
	#endif
	));
	connectionButton->Bind(wxEVT_BUTTON, &MyFrame::changePrinterConnection, this);
	
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
	
	// Create log timer
	wxTimer *logTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateLog, this, logTimer->GetId());
	logTimer->Start(100);
	
	// Create status timer
	statusTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &MyFrame::updateStatus, this, statusTimer->GetId());
	statusTimer->Start(100);
	
	// Check if not using OS X
	#ifndef OSX
	
		// Create install drivers button
		installDriversButton = new wxButton(panel, wxID_ANY, "Install drivers", wxPoint(
		#ifdef WINDOWS
			448, 54
		#endif
		#ifdef LINUX
			440, 58
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
		542, 58
	#endif
	#ifdef OSX
		549, 59
	#endif
	#ifdef LINUX
		549, 60
	#endif
	));
	
	// Create switch to mode button
	switchToModeButton = new wxButton(panel, wxID_ANY, "Switch to bootloader mode", wxPoint(
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
	switchToModeButton->Enable(false);
	switchToModeButton->Bind(wxEVT_BUTTON, &MyFrame::switchToMode, this);
	
	// Create install iMe firmware button
	installImeFirmwareButton = new wxButton(panel, wxID_ANY, "Install iMe firmware", wxPoint(
	#ifdef WINDOWS
		220, 111
	#endif
	#ifdef OSX
		210, 103
	#endif
	#ifdef LINUX
		215, 118
	#endif
	));
	installImeFirmwareButton->Enable(false);
	installImeFirmwareButton->Bind(wxEVT_BUTTON, &MyFrame::installIMe, this);
	
	// Create install firmware with file button
	installFirmwareFromFileButton = new wxButton(panel, wxID_ANY, "Install firmware from file", wxPoint(
	#ifdef WINDOWS
		388, 111
	#endif
	#ifdef OSX
		363, 103
	#endif
	#ifdef LINUX
		365, 118
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
		542, 247
	#endif
	#ifdef OSX
		549, 267
	#endif
	#ifdef LINUX
		549, 269
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
		522, 182
	#endif
	#ifdef OSX
		524, 198
	#endif
	#ifdef LINUX
		529, 200
	#endif
	), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
	consoleOutput->SetFont(wxFont(
	#ifdef WINDOWS
		8
	#endif
	#ifdef OSX
		11
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
		426, -1
	#endif
	#ifdef OSX
		432, -1
	#endif
	#ifdef LINUX
		441, -1
	#endif
	), wxTE_PROCESS_ENTER);
	commandInput->SetHint("Command");
	commandInput->Bind(wxEVT_TEXT_ENTER, &MyFrame::sendCommandManually, this);
	
	// Create send command button
	sendCommandButton = new wxButton(panel, wxID_ANY, "Send", wxPoint(
	#ifdef WINDOWS
		450, 360
	#endif
	#ifdef OSX
		451, 371
	#endif
	#ifdef LINUX
		460, 389
	#endif
	));
	sendCommandButton->Bind(wxEVT_BUTTON, &MyFrame::sendCommandManually, this);
	sendCommandButton->Enable(false);
	
	// Create movement box
	new wxStaticBox(panel, wxID_ANY, "Movement", wxPoint(564, 0), wxSize(
	#ifdef WINDOWS
		542, 90
	#endif
	#ifdef OSX
		549, 82
	#endif
	#ifdef LINUX
		549, 97
	#endif
	));
	
	// Create backward movement button
	backwardMovementButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		616, 19
	#endif
	#ifdef OSX
		616, 15
	#endif
	#ifdef LINUX
		625, 22
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
	backwardMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Y" + to_string(static_cast<double>(feedRateMovementSlider->GetValue()) / 1000) + " F" TOSTRING(DEFAULT_Y_SPEED));
	});
	backwardMovementButton->Enable(false);
	
	// Create left movement button
	leftMovementButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		566, 69
	#endif
	#ifdef OSX
		566, 65
	#endif
	#ifdef LINUX
		575, 72
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
	leftMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 X-" + to_string(static_cast<double>(feedRateMovementSlider->GetValue()) / 1000) + " F" TOSTRING(DEFAULT_X_SPEED));
	});
	leftMovementButton->Enable(false);
	
	// Create home movement button
	homeMovementButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		616, 69
	#endif
	#ifdef OSX
		616, 65
	#endif
	#ifdef LINUX
		625, 72
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
	homeMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G28");
	});
	homeMovementButton->Enable(false);
	
	// Create right movement button
	rightMovementButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		666, 69
	#endif
	#ifdef OSX
		666, 65
	#endif
	#ifdef LINUX
		675, 72
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
	rightMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 X" + to_string(static_cast<double>(feedRateMovementSlider->GetValue()) / 1000) + " F" TOSTRING(DEFAULT_X_SPEED));
	});
	rightMovementButton->Enable(false);
	
	// Create forward movement button
	forwardMovementButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		616, 119
	#endif
	#ifdef OSX
		616, 115
	#endif
	#ifdef LINUX
		625, 122
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
	forwardMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Y-" + to_string(static_cast<double>(feedRateMovementSlider->GetValue()) / 1000) + " F" TOSTRING(DEFAULT_Y_SPEED));
	});
	forwardMovementButton->Enable(false);
	
	// Create up movement button
	upMovementButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		696, 44
	#endif
	#ifdef OSX
		716, 40
	#endif
	#ifdef LINUX
		725, 47
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
	upMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Z" + to_string(static_cast<double>(feedRateMovementSlider->GetValue()) / 1000) + " F" TOSTRING(DEFAULT_Z_SPEED));
	});
	upMovementButton->Enable(false);
	
	// Create down movement button
	downMovementButton = new wxBitmapButton(panel, wxID_ANY, loadImage(refresh_pngData, sizeof(refresh_pngData),
	#ifdef WINDOWS
		-1, -1, 0, 0
	#endif
	#ifdef OSX
		-1, -1, 0, 2
	#endif
	#ifdef LINUX
		-1, -1, 0, 0
	#endif
	), wxPoint(
	#ifdef WINDOWS
		696, 94
	#endif
	#ifdef OSX
		716, 90
	#endif
	#ifdef LINUX
		725, 97
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
	downMovementButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
	
		// Send commands
		sendCommand("G91");
		sendCommand("G0 Z-" + to_string(static_cast<double>(feedRateMovementSlider->GetValue()) / 1000) + " F" TOSTRING(DEFAULT_Z_SPEED));
	});
	downMovementButton->Enable(false);
	
	// Create feed rate movement slider
	feedRateMovementSlider = new wxSlider(panel, wxID_ANY, 10 * 1000, 0.001 * 1000, 50 * 1000, wxPoint(
	#ifdef WINDOWS
		616, 219
	#endif
	#ifdef OSX
		616, 219
	#endif
	#ifdef LINUX
		616, 219
	#endif
	), wxSize(
	#ifdef WINDOWS
		140, -1
	#endif
	#ifdef OSX
		140, -1
	#endif
	#ifdef LINUX
		140, -1
	#endif
	));
	feedRateMovementSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, [=](wxCommandEvent& event) {
	
		// Update feed rate movement text
		updateFeedRateMovementText();
	});
	feedRateMovementSlider->Enable(false);
	
	// Create feed rate movement text
	feedRateMovementText = new wxStaticText(panel, wxID_ANY, "", wxPoint(
	#ifdef WINDOWS
		616, 239
	#endif
	#ifdef OSX
		616, 239
	#endif
	#ifdef LINUX
		616, 239
	#endif
	), wxSize(
	#ifdef WINDOWS
		140, -1
	#endif
	#ifdef OSX
		140, -1
	#endif
	#ifdef LINUX
		140, -1
	#endif
	), wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
	updateFeedRateMovementText();
	
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
		GetChildren().GetFirst()->GetData()->SetFocus();
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
		sleepUs(10000);
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

	// Stop status timer
	statusTimer->Stop();

	// Disconnect printer
	printer.disconnect();
	
	// Check if thread is running
	if(GetThread() && GetThread()->IsRunning())
	
		// Destroy thread
		GetThread()->Delete();
	
	// Destroy self
	Destroy();
}

void MyFrame::changePrinterConnection(wxCommandEvent& event) {
	
	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);

	// Check if connecting to printer
	if(connectionButton->GetLabel() == "Connect") {
		
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
			connectionButton->Enable(false);

			// Set status text
			statusText->SetLabel("Connecting");
			statusText->SetForegroundColour(wxColour(255, 180, 0));
			
			// Set establishing printer connection
			establishingPrinterConnection = true;
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
	
			// Enable connection button
			connectionButton->Enable(true);

			// Check if connected to printer
			if(printer.isConnected()) {

				// Enable printer controls
				installFirmwareFromFileButton->Enable(true);
				installImeFirmwareButton->Enable(true);
				switchToModeButton->Enable(true);
				sendCommandButton->Enable(true);
				
				// Change connection button to disconnect	
				connectionButton->SetLabel("Disconnect");
			}
			
			// Otherwise
			else {
			
				// Enable connection controls
				serialPortChoice->Enable(true);
				refreshSerialPortsButton->Enable(true);
			}

			// Start status timer
			statusTimer->Start(100);
			
			// Clear establishing printer connection
			establishingPrinterConnection = false;
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
			switchToModeButton->Enable(false);
			sendCommandButton->Enable(false);
		
			// Change connection button to connect
			connectionButton->SetLabel("Connect");
		
			// Enable connection controls
			connectionButton->Enable(true);
			serialPortChoice->Enable(true);
			refreshSerialPortsButton->Enable(true);
		
			// Start status timer
			statusTimer->Start(100);
		});
	}
}

void MyFrame::switchToMode(wxCommandEvent& event) {

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);
	
	// Set new mode
	operatingModes newOperatingMode = switchToModeButton->GetLabel() == "Switch to firmware mode" ? FIRMWARE : BOOTLOADER;

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable connection controls
		serialPortChoice->Enable(false);
		refreshSerialPortsButton->Enable(false);
		connectionButton->Enable(false);
	
		// Disable printer controls
		installFirmwareFromFileButton->Enable(false);
		installImeFirmwareButton->Enable(false);
		switchToModeButton->Enable(false);
		sendCommandButton->Enable(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Check if switching to firmware mode
		if(newOperatingMode == FIRMWARE)
	
			// Put printer into firmware mode
			printer.switchToFirmwareMode();
		
		// Otherwise
		else
		
			// Put printer into bootloader mode
			printer.switchToBootloaderMode();

		// Check if printer isn't connected
		if(!printer.isConnected())
		
			// Return message
			return {static_cast<string>("Failed to switch printer into ") + (newOperatingMode == FIRMWARE ? "firmware" : "bootloader") + " mode", wxOK | wxICON_ERROR | wxCENTRE};
	
		// Otherwise
		else
		
			// Return message
			return {static_cast<string>("Printer has been successfully switched into ") + (newOperatingMode == FIRMWARE ? "firmware" : "bootloader") + " mode and is connected at " + printer.getCurrentSerialPort(), wxOK | wxICON_INFORMATION | wxCENTRE};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push([=](ThreadTaskResponse response) -> void {
		
		// Enable connection controls
		serialPortChoice->Enable(true);
		refreshSerialPortsButton->Enable(true);
		connectionButton->Enable(true);

		// Check if connected to printer
		if(printer.isConnected()) {

			// Enable printer controls
			installFirmwareFromFileButton->Enable(true);
			installImeFirmwareButton->Enable(true);
			switchToModeButton->Enable(true);
			sendCommandButton->Enable(true);
		}
		
		// Display message
		wxMessageBox(response.message, "M3D Manager", response.style);
	});
}

void MyFrame::installIMe(wxCommandEvent& event) {

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Stop status timer
		statusTimer->Stop();

		// Disable connection controls
		serialPortChoice->Enable(false);
		refreshSerialPortsButton->Enable(false);
		connectionButton->Enable(false);
	
		// Disable printer controls
		installFirmwareFromFileButton->Enable(false);
		installImeFirmwareButton->Enable(false);
		switchToModeButton->Enable(false);
		sendCommandButton->Enable(false);
	
		// Set status text
		statusText->SetLabel("Installing firmware");
		statusText->SetForegroundColour(wxColour(255, 180, 0));
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Set firmware location
		string firmwareLocation = getTemporaryLocation() + "iMe " TOSTRING(IME_ROM_VERSION_STRING) ".hex";

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
		connectionButton->Enable(true);
	
		// Check if connected to printer
		if(printer.isConnected()) {
	
			// Enable printer controls
			installFirmwareFromFileButton->Enable(true);
			installImeFirmwareButton->Enable(true);
			switchToModeButton->Enable(true);
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
		
		// Disable button that triggered event
		FindWindowById(event.GetId())->Enable(false);

		// Lock
		wxCriticalSectionLocker lock(criticalLock);

		// Append thread start callback to queue
		threadStartCallbackQueue.push([=]() -> void {
		
			// Stop status timer
			statusTimer->Stop();

			// Disable connection controls
			serialPortChoice->Enable(false);
			refreshSerialPortsButton->Enable(false);
			connectionButton->Enable(false);
	
			// Disable printer controls
			installFirmwareFromFileButton->Enable(false);
			installImeFirmwareButton->Enable(false);
			switchToModeButton->Enable(false);
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
			connectionButton->Enable(true);
	
			// Check if connected to printer
			if(printer.isConnected()) {
	
				// Enable printer controls
				installFirmwareFromFileButton->Enable(true);
				installImeFirmwareButton->Enable(true);
				switchToModeButton->Enable(true);
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

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);

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
			ofstream fout(getTemporaryLocation() + "M3D.cat", ios::binary);
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
				fout.open(getTemporaryLocation() + "M3D.inf", ios::binary);
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
					_tcscpy(command, (path + "\\" + executablePath + "\\pnputil.exe -i -a \"" + getTemporaryLocation() + "M3D.inf\"").c_str());

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

void MyFrame::logToConsole(const string &message) {

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append message to log queue
	logQueue.push(message);
}

void MyFrame::updateLog(wxTimerEvent& event) {

	// Loop forever
	while(true) {
	
		// Initialize message
		string message;
	
		{
			// Lock
			wxCriticalSectionLocker lock(criticalLock);
			
			// Check if no messages exists in log queue
			if(logQueue.empty())
			
				// Break
				break;
			
			// Set message to next message in log queue
			message = logQueue.front();
			logQueue.pop();
		}
		
		// Check if message exists
		if(!message.empty()) {
		
			// Check if message is to remove last line
			if(message == "Remove last line") {
		
				// Remove last line
				size_t end = consoleOutput->GetValue().Length();
				size_t start = consoleOutput->GetValue().find_last_of('\n');
				consoleOutput->Remove(start != string::npos ? start : 0, end);
			}
		
			// Otherwise
			else {
			
				// Check if printer is switching modes
				if(message == "Switching printer into bootloader mode" || message == "Switching printer into firmware mode") {
				
					// Check if not establishing printer connection
					if(!establishingPrinterConnection) {
			
						// Disable movement controls
						backwardMovementButton->Enable(false);
						forwardMovementButton->Enable(false);
						rightMovementButton->Enable(false);
						leftMovementButton->Enable(false);
						upMovementButton->Enable(false);
						downMovementButton->Enable(false);
						homeMovementButton->Enable(false);
						feedRateMovementSlider->Enable(false);
					}
				}
				
				// Otherwise check if printer is in firmware mode
				else if(message == "Printer is in firmware mode") {
			
					// Set switch mode button label
					switchToModeButton->SetLabel("Switch to bootloader mode");
					
					// Check if not establishing printer connection
					if(!establishingPrinterConnection) {
					
						// Enable movement controls
						backwardMovementButton->Enable(true);
						forwardMovementButton->Enable(true);
						rightMovementButton->Enable(true);
						leftMovementButton->Enable(true);
						upMovementButton->Enable(true);
						downMovementButton->Enable(true);
						homeMovementButton->Enable(true);
						feedRateMovementSlider->Enable(true);
					}
				}
				
				// Otherwise check if printer is in bootloader mode
				else if(message == "Printer is in bootloader mode")
				
					// Set switch mode button label
					switchToModeButton->SetLabel("Switch to firmware mode");
				
				// Otherwise check if the printer has been disconnected
				else if(message == "Printer has been disconnected") {
				
					// Check if not establishing printer connection
					if(!establishingPrinterConnection) {
				
						// Disable movement controls
						backwardMovementButton->Enable(false);
						forwardMovementButton->Enable(false);
						rightMovementButton->Enable(false);
						leftMovementButton->Enable(false);
						upMovementButton->Enable(false);
						downMovementButton->Enable(false);
						homeMovementButton->Enable(false);
						feedRateMovementSlider->Enable(false);
					}
				}
		
				// Append message to console's output
				consoleOutput->AppendText(static_cast<string>(consoleOutput->GetValue().IsEmpty() ? "" : "\n") + ">> " + message);
			}
		
			// Scroll to bottom
			consoleOutput->ShowPosition(consoleOutput->GetLastPosition());
		}
	}
}

void MyFrame::updateStatus(wxTimerEvent& event) {

	// Check if getting printer status was successful
	string status = printer.getStatus();
	if(!status.empty()) {
	
		// Check if printer is connected
		if(status == "Connected") {
	
			// Change connection button to disconnect
			if(connectionButton->GetLabel() != "Disconnect")
				connectionButton->SetLabel("Disconnect");
		
			// Disable connection controls
			serialPortChoice->Enable(false);
			refreshSerialPortsButton->Enable(false);
		
			// Set status text color
			statusText->SetForegroundColour(wxColour(0, 255, 0));
		}
	
		// Otherwise
		else {
	
			// Check if printer was just disconnected
			if(status == "Disconnected" && (statusText->GetLabel() != status || connectionButton->GetLabel() == "Disconnect")) {
				
				// Disable printer controls
				installFirmwareFromFileButton->Enable(false);
				installImeFirmwareButton->Enable(false);
				switchToModeButton->Enable(false);
				sendCommandButton->Enable(false);
		
				// Change connection button to connect
				connectionButton->SetLabel("Connect");
		
				// Enable connection controls
				connectionButton->Enable(true);
				serialPortChoice->Enable(true);
				refreshSerialPortsButton->Enable(true);
			
				// Log disconnection
				logToConsole("Printer has been disconnected");
			}
		
			// Set status text color
			statusText->SetForegroundColour(wxColour(255, 0, 0));
		}

		// Update status text
		statusText->SetLabel(status);
	}
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

	// Disable button that triggered event
	FindWindowById(event.GetId())->Enable(false);
	
	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push([=]() -> void {
	
		// Disable connection controls
		serialPortChoice->Enable(false);
		refreshSerialPortsButton->Enable(false);
		connectionButton->Enable(false);
	});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Delay
		sleepUs(300000);
	
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
		connectionButton->Enable(true);
		
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

void MyFrame::updateFeedRateMovementText() {

	// Set feed rate movement text to current feed rate
	stringstream stream;
	stream << fixed << setprecision(3) << static_cast<double>(feedRateMovementSlider->GetValue()) / 1000;
	feedRateMovementText->SetLabel(stream.str() + "mm");
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
			else

				// Return message
				return {"Firmware successfully installed", wxOK | wxICON_INFORMATION | wxCENTRE};
		}
	}
}

void MyFrame::sendCommandManually(wxCommandEvent& event) {

	// Check if commands can be sent
	if(sendCommandButton->IsEnabled()) {
	
		// Check if command exists
		string command = static_cast<string>(commandInput->GetValue());
		if(!command.empty()) {
		
			// Clear command input
			commandInput->SetValue("");
			
			// Send command
			sendCommand(command);
		}
	}
}

void MyFrame::sendCommand(const string &command, function<void()> threadStartCallback, function<void(ThreadTaskResponse response)> threadCompleteCallback) {

	// Log command
	logToConsole("Send: " + command);

	// Lock
	wxCriticalSectionLocker lock(criticalLock);

	// Append thread start callback to queue
	threadStartCallbackQueue.push(threadStartCallback ? threadStartCallback : [=]() -> void {});
	
	// Append thread task to queue
	threadTaskQueue.push([=]() -> ThreadTaskResponse {
	
		// Set changed mode
		bool changedMode = (command == "Q" && printer.getOperatingMode() == BOOTLOADER) || (command == "M115 S628" && printer.getOperatingMode() == FIRMWARE);
		
		// Check if send command failed
		if(!printer.sendRequest(command))
		
			// Log error
			logToConsole("Sending command failed");
		
		// Otherwise
		else {
		
			// Wait until command receives a response
			for(string response; !changedMode && response.substr(0, 2) != "ok" && response.substr(0, 2) != "rs" && response.substr(0, 4) != "skip" && response.substr(0, 5) != "Error";) {
				
				// Get response
				do {
					response = printer.receiveResponse();
				} while(response.empty() && printer.isConnected());
		
				// Check if printer isn't connected
				if(!printer.isConnected())
		
					// Break
					break;
				
				// Check if printer is in bootloader mode
				if(printer.getOperatingMode() == BOOTLOADER) {
				
					// Convert response to hexadecimal
					stringstream hexResponse;
					for(size_t i = 0; i < response.length(); i++)
						hexResponse << "0x" << hex << setfill('0') << setw(2) << uppercase << (static_cast<uint8_t>(response[i]) & 0xFF) << ' ';

					response = hexResponse.str();
					if(!response.empty())
						response.pop_back();
				}
				
				// Log response
				if(response != "wait")
					logToConsole("Receive: " + response);
				
				// Check if printer is in bootloader mode
				if(printer.getOperatingMode() == BOOTLOADER)
				
					// Break
					break;
			}
		}

		// Return empty response
		return {"", 0};
	});
	
	// Append thread complete callback to queue
	threadCompleteCallbackQueue.push(threadCompleteCallback ? threadCompleteCallback : [=](ThreadTaskResponse response) -> void {});
}

// Check if using Windows
#ifdef WINDOWS

	WXLRESULT MyFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) {
	
		// Check if device change
		if(message == WM_DEVICECHANGE)
		
			// Check if an interface device was removed
			if(wParam == DBT_DEVICEREMOVECOMPLETE && reinterpret_cast<PDEV_BROADCAST_HDR>(lParam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
			
				// Create device info string
				stringstream deviceInfoStringStream;
				deviceInfoStringStream << "VID_" << setfill('0') << setw(4) << hex << uppercase << PRINTER_VENDOR_ID << "&PID_" << setfill('0') << setw(4) << hex << uppercase << PRINTER_PRODUCT_ID;
				string deviceInfoString = deviceInfoStringStream.str();
				
				// Check if device has the printer's PID and VID
				PDEV_BROADCAST_DEVICEINTERFACE deviceInterface = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(lParam);
				
				if(!_tcsnicmp(deviceInterface->dbcc_name, _T("\\\\?\\USB#" + deviceInfoString), ("\\\\?\\USB#" + deviceInfoString).length())) {
					
					// Get device ID
					wstring deviceId = &deviceInterface->dbcc_name[("\\\\?\\USB#" + deviceInfoString).length() + 1];
					deviceId = _T("USB\\" + deviceInfoString + "\\") + deviceId.substr(0, deviceId.find(_T("#")));
					
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
