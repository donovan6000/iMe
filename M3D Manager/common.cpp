// Header files
#include "common.h"


// Supporting function implementation
string getTemporaryLocation() {

	// Return temp location
	char* tempPath = getenv("TEMP");
	if(!tempPath)
		 tempPath = getenv("TMP");
	if(!tempPath)
		 tempPath = getenv("TMPDIR");
	
	return tempPath ? tempPath : P_tmpdir;
}
