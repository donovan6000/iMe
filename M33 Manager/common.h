// Header guard
#ifndef COMMON_H
#define COMMON_H


// Header files
#include <string>
#ifdef USE_GUI
	#include <wx/wxprec.h>
	#ifndef WX_PRECOMP
		#include <wx/wx.h>
	#endif
#endif
#ifdef WINDOWS
	#include <windows.h>
	#include <tchar.h>
#endif
#include "working/resources.h"


using namespace std;


// Definitions
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


// Function prototypes

/*
Name: Get temporary location
Purpose: Returns the location of a temporary directory
*/
string getTemporaryLocation();


#endif
