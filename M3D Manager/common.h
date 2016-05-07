// Header gaurd
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

// Packed files
#include "iMe 1900000002_hex.h"
#include "M3D_cat.h"
#include "M3D_inf.h"
#include "_90_m3d_local_rules.h"

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
