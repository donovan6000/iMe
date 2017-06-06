// Header files
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;


// Function prototypes

// Set Unicode locale
static bool setUnicodeLocale() noexcept;

// Converts a string to a wstring
static wstring stringToWstring(const char *text) noexcept;
static wstring stringToWstring(const string &text) noexcept;

// Converts a wstring to a string
static string wstringToString(const wchar_t *text) noexcept;
static string wstringToString(const wstring &text) noexcept;


// Main function
int main(int argc, char *argv[]) noexcept {

	// Check if setting a Unicode locale failed
	if(!setUnicodeLocale()) {
	
		// Display message
		wcout << L"System does not meet the minimum localization requirements" << endl;
	
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if parameters are invalid
	if(argc < 2) {
	
		// Display error
		wcout << L"File name not provided" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if help argument was specified
	if(stringToWstring(argv[1]) == L"-h" || stringToWstring(argv[1]) == L"--help") {
	
		// Display message
		wcout << L"Usage: ./bin2h \"fileToConvert\"" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Set output file name
	wstring outputFileName = stringToWstring(argv[1]);
	
	// Convert output file name to proper format
	wstring::size_type offset = outputFileName.find_last_of(L'/');
	if(offset != wstring::npos)
		outputFileName.erase(0, offset + 1);
	
	for(wchar_t &character : outputFileName)
		if(character == L'.' || character == L'-')
			character = L'_';
	
	if(iswdigit(outputFileName.front()))
		outputFileName.insert(outputFileName.begin(), L'_');

	// Set header guard name
	wstring headerGaurdName = outputFileName;
	
	// Convert header guard name to proper format
	for(wchar_t &character : headerGaurdName) {
		if(character == L' ')
			character = L'_';
		character = towupper(character);
	}
	
	// Set variable name
	wstring variableName = outputFileName;
	
	// Convert variable name to proper format
	for(wstring::size_type i = 0; i < variableName.length(); ++i)
		if(variableName[i] == L' ' || variableName[i] == L'_') {
			variableName.erase(i, 1);
			variableName[i] = towupper(variableName[i]);
		}
	
	variableName.front() = towlower(variableName.front());
	
	if(iswdigit(variableName.front()))
		variableName = L"_" + variableName;
	
	// Check if resources are packed
	#ifdef PACKED
		bool packed = true;
	#else
		bool packed = false;
	#endif
	
	// Check if not packed and resource isn't icon
	if(!packed && outputFileName != L"icon_png") {
	
		// Print message
		wcout << outputFileName << L" not used" << endl;
	
		// Return success
		return EXIT_SUCCESS;
	}
	
	// Check if opening file failed
	ifstream input(argv[1], ifstream::binary);
	if(!input) {
	
		// Display error
		wcout << L"Opening input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if getting start of the input failed
	ifstream::pos_type begin = input.tellg();
	if(begin == -1) {
	
		// Display error
		wcout << L"Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if going to the end of the input failed
	if(!input.seekg(0, input.end)) {
	
		// Display error
		wcout << L"Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if getting end of the input failed
	ifstream::pos_type end = input.tellg();
	if(end == -1) {
	
		// Display error
		wcout << L"Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if going back to the beginning of the input failed
	if(!input.seekg(0, input.beg)) {
	
		// Display error
		wcout << L"Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if opening output file failed
	wofstream output(wstringToString(outputFileName + L".h"), wofstream::binary);
	if(!output) {
	
		// Display error
		wcout << L"Opening output file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if opening resources file failed
	wofstream resources(wstringToString(L"resources.h"), wofstream::binary | wofstream::app);
	if(!resources) {
	
		// Display error
		wcout << L"Opening resources file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}

	// Write header guard to output
	output << L"// Header guard" << endl << L"#ifndef " << headerGaurdName << L"_H" << endl << L"#define " << headerGaurdName << L"_H" << endl << endl << endl;
	
	// Write size to output
	output << L"// Constants" << endl << endl << L"// Size" << endl << L"const uintmax_t " << variableName << L"Size = " << to_wstring(end - begin) << L';' << endl << endl;
	
	// Write start of data to output
	output << L"// Data" << endl << L"const uint8_t " << variableName << L"Data[] = {";
	
	// Go through all characters in the input
	for(char character; input.get(character);) {
		
		// Write character's hexadecimal representation to output
		output << L"0x" << hex << uppercase << setw(2) << setfill(L'0') << static_cast<uint8_t>(character);
		
		// Check if at least one more character exists in the file
		if(input.peek() != ifstream::traits_type::eof())
		
			// Write a comma to output
			output << L", ";
	}
	
	// Write end of data to output
	output << L"};" << endl << endl << endl;
	
	// Write end of header guard to output
	output << L"#endif";
	
	// Append output to resources
	resources << L"#include \"" << outputFileName << L".h\"" << endl;
	
	// Check if reading input failed
	if(!input.eof()) {
	
		// Delete output file
		remove(wstringToString(outputFileName + L".h").c_str());
		
		// Delete resources file
		remove(wstringToString(L"resources.h").c_str());
	
		// Display error
		wcout << L"Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if writing to output failed
	if(!output) {
	
		// Delete output file
		remove(wstringToString(outputFileName + L".h").c_str());
		
		// Delete resources file
		remove(wstringToString(L"resources.h").c_str());
	
		// Display error
		wcout << L"Writing output file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Close input
	input.clear();
	input.close();
	
	// Check if closing input failed
	if(!input) {
	
		// Delete output file
		remove(wstringToString(outputFileName + L".h").c_str());
		
		// Delete resources file
		remove(wstringToString(L"resources.h").c_str());
	
		// Display error
		wcout << L"Closing input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Close output
	output.close();
	
	// Check if closing output failed
	if(!output) {
	
		// Delete output file
		remove(wstringToString(outputFileName + L".h").c_str());
		
		// Delete resources file
		remove(wstringToString(L"resources.h").c_str());
	
		// Display error
		wcout << L"Closing output file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Close resources
	resources.close();
	
	// Check if closing resources failed
	if(!resources) {
	
		// Delete resources file
		remove(wstringToString(L"resources.h").c_str());
	
		// Display error
		wcout << L"Closing resources file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Print message
	wcout << outputFileName << L" generated successfully" << endl;
	
	// Return success
	return EXIT_SUCCESS;
}


// Supporting function implementation
bool setUnicodeLocale() noexcept {

	// Check if setting locale to en_US.UTF-8 failed
	try {
		locale::global(static_cast<locale>("en_US.UTF-8"));
	}
	
	catch(const runtime_error &error) {
	
		// Check if setting locale to C.UTF-8 failed
		try {
			locale::global(static_cast<locale>("C.UTF-8"));
		}
	
		// Set locale to system's locale
		catch(const runtime_error &error) {
			locale::global(static_cast<locale>(""));
		}
	}
	
	// Set locale of existing streams
	cout.imbue(locale());
	cerr.imbue(locale());
	clog.imbue(locale());
	cin.imbue(locale());
	wcout.imbue(locale());
	wcerr.imbue(locale());
	wclog.imbue(locale());
	wcin.imbue(locale());
	
	// Return if locale is UTF-8
	return cout.getloc().name().length() >= sizeof("UTF-8") - 1 && cout.getloc().name().substr(cout.getloc().name().length() - (sizeof("UTF-8") - 1)) == "UTF-8";
}

wstring stringToWstring(const char *text) noexcept {

	// Check if converting will fail
	mbstate_t state = mbstate_t();
	size_t length = mbsrtowcs(nullptr, &text, 0, &state);
	if(length == static_cast<size_t>(-1))
	
		// Return an empty string
		return L"";
	
	// Convert string to a wide string
	wchar_t returnValue[length + 1];
	mbsrtowcs(returnValue, &text, length + 1, &state);
	
	// Return wide string
	return returnValue;
}

wstring stringToWstring(const string &text) noexcept {

	// Return wide string
	return stringToWstring(text.c_str());
}

string wstringToString(const wchar_t *text) noexcept {

	// Check if converting will fail
	mbstate_t state = mbstate_t();
	size_t length = wcsrtombs(nullptr, &text, 0, &state);
	if(length == static_cast<size_t>(-1))
	
		// Return an empty string
		return "";
	
	// Convert wide string to string
	char returnValue[length + 1];
	wcsrtombs(returnValue, &text, length + 1, &state);
	
	// Return wide string
	return returnValue;
}

string wstringToString(const wstring &text) noexcept {

	// Return string
	return wstringToString(text.c_str());
}
