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


using namespace std;


// Definitions
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


// Packed files
#include "icon_png.h"
#include "refresh_png.h"
#include TOSTRING(IME_HEADER)
#include TOSTRING(M3D_HEADER)
#include "M3D_cat.h"
#include "M3D_inf.h"
#include "_90_m3d_local_rules.h"


// Function prototypes

/*
Name: Get temporary location
Purpose: Returns the location of a temporary directory
*/
string getTemporaryLocation();


#endif
