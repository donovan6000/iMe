// Header gaurd
#ifndef GUI_H
#define GUI_H


// Header files
#include <string>
#include <functional>
#include <queue>
#include <wx/sysopt.h>
#include "common.h"
#include "printer.h"

using namespace std;


// Thread task response struct
struct ThreadTaskResponse {
	string message;
	int style;
};

// My app class
class MyApp: public wxApp {

	// Public
	public:

		// On init
		virtual bool OnInit();
};

// My frame class
class MyFrame: public wxFrame, public wxThreadHelper {

	// Public
	public:
	
		/*
		Name: Constructor
		Purpose: Initializes frame
		*/
		MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style);
		
		/*
		Name: Entry
		Purpose: Thread entry point
		*/
		wxThread::ExitCode Entry();
		
		/*
		Name: Thread task start
		Purpose: Event that's called when background thread starts a task
		*/
		void threadTaskStart(wxThreadEvent& event);
		
		/*
		Name: Thread task complete
		Purpose: Event that's called when background thread completes a task
		*/
		void threadTaskComplete(wxThreadEvent& event);
		
		/*
		Name: Close
		Purpose: Event that's called when frame closes
		*/
		void close(wxCloseEvent& event);
		
		/*
		Name: Show
		Purpose: Event that's called when frame is shown
		*/
		void show(wxShowEvent &event);
		
		/*
		Name: Connect to printer
		Purpose: Connects to the printer
		*/
		void connectToPrinter(wxCommandEvent& event);
		
		/*
		Name: Switch to firware mode
		Purpose: Switches printer into firmware mode
		*/
		void switchToFirmwareMode(wxCommandEvent& event);
		
		/*
		Name: Install iMe
		Purpose: Installs iMe as the printer's firmware
		*/
		void installIMe(wxCommandEvent& event);
		
		/*
		Name: Install drivers
		Purpose: Installs device drivers for the printer
		*/
		void installDrivers(wxCommandEvent& event);
		
		/*
		Name: Update status
		Purpose: Updates status text
		*/
		void updateStatus(wxTimerEvent& event);
		
		/*
		Name: Install firmware from file
		Purpose: Installs a file as the printer's firmware
		*/
		void installFirmwareFromFile(wxCommandEvent& event);
		
		/*
		Name: Get available serial ports
		Purpose: Returns all available serial ports
		*/
		wxArrayString getAvailableSerialPorts();
		
		/*
		Name: Refresh serial ports
		Purpose: Refreshes list of serial ports
		*/
		void refreshSerialPorts(wxCommandEvent& event);
		
		/*
		Name: Send command
		Purpose: Sends a command to the printer
		*/
		void sendCommand(wxCommandEvent& event);
		
		// Check if using Windows
		#ifdef WINDOWS
		
			/*
			Name: MSWWindowProc
			Purpose: Windows event manager
			*/
			WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
		#endif
	
	// Private
	private:
	
		// Install firmware
		ThreadTaskResponse installFirmware(const string &firmwareLocation);
		
		// Log to console
		void logToConsole(const string &text);
	
		// Controls
		wxChoice *serialPortChoice;
		wxButton *refreshSerialPortsButton;
		wxButton *connectButton;
		wxStaticText *versionText;
		wxStaticText *statusText;
		wxButton *installFirmwareFromFileButton;
		wxButton *installImeFirmwareButton;
		wxButton *switchToFirmwareModeButton;
		wxTimer *statusTimer;
		wxTextCtrl* commandInput;
		wxTextCtrl* consoleOutput;
		wxButton *sendCommandButton;
		wxButton *installDriversButton;
		
		// Critical lock
		wxCriticalSection criticalLock;
		
		// Thread start, task, and complete queues
		queue<function<void()>> threadStartCallbackQueue;
		queue<function<ThreadTaskResponse()>> threadTaskQueue;
		queue<function<void(ThreadTaskResponse response)>> threadCompleteCallbackQueue;
		
		// Printer
		Printer printer;
};


#endif
