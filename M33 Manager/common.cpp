// Header files
#include <algorithm>
#include "common.h"


// Supporting function implementation
string getTemporaryLocation() {

	// Return temp location
	char* path = getenv("TEMP");
	if(!path)
		 path = getenv("TMP");
	if(!path)
		path = getenv("TMPDIR");
	
	string tempPath = path ? path : P_tmpdir;
	
	// Make sure path ends with a path seperator
	char pathSeperator =
	#ifdef WINDOWS
		'\\'
	#else
		'/'
	#endif
	;
	
	if(tempPath.back() != pathSeperator)
		tempPath.push_back(pathSeperator);
	
	// Return temp path
	return tempPath;
}

string toLowerCase(const string &text) {

	// Initialize return value
	string returnValue = text;
	
	// Convert return value to lower case
	transform(returnValue.begin(), returnValue.end(), returnValue.begin(), ::tolower);
	
	// Return return value
	return returnValue;
}
