// Header files
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;


// Main function
int main(int argc, char *argv[]) noexcept {
	
	// Check if parameters are invalid
	if(argc < 2) {
	
		// Display error
		cout << "File name not provided" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if help argument was specified
	if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
	
		// Display message
		cout << "Usage: ./bin2h \"fileToConvert\"" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Set output file name
	string outputFileName = argv[1];
	
	// Convert output file name to proper format
	string::size_type offset = outputFileName.find_last_of('/');
	if(offset != string::npos)
		outputFileName.erase(0, offset + 1);
	
	for(char &character : outputFileName)
		if(character == '.' || character == '-')
			character = '_';
	
	if(isdigit(outputFileName.front()))
		outputFileName.insert(outputFileName.begin(), '_');

	// Set header guard name
	string headerGaurdName = outputFileName;
	
	// Convert header guard name to proper format
	for(char &character : headerGaurdName) {
		if(character == ' ' || character == '+')
			character = '_';
		character = toupper(character);
	}
	
	// Set variable name
	string variableName = outputFileName;
	
	// Convert variable name to proper format
	for(string::size_type i = 0; i < variableName.length(); ++i)
		if(variableName[i] == ' ' || variableName[i] == '_' || variableName[i] == '+') {
			variableName.erase(i, 1);
			variableName[i] = toupper(variableName[i]);
		}
	
	variableName.front() = tolower(variableName.front());
	
	if(isdigit(variableName.front()))
		variableName = "_" + variableName;
	
	// Check if resources are packed
	#ifdef PACKED
		bool packed = true;
	#else
		bool packed = false;
	#endif
	
	// Check if not packed and resource isn't icon
	if(!packed && outputFileName != "icon_png") {
	
		// Print message
		cout << outputFileName << " not used" << endl;
	
		// Return success
		return EXIT_SUCCESS;
	}
	
	// Check if opening file failed
	ifstream input(argv[1], ifstream::binary);
	if(!input) {
	
		// Display error
		cout << "Opening input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if getting start of the input failed
	ifstream::pos_type begin = input.tellg();
	if(begin == -1) {
	
		// Display error
		cout << "Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if going to the end of the input failed
	if(!input.seekg(0, input.end)) {
	
		// Display error
		cout << "Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if getting end of the input failed
	ifstream::pos_type end = input.tellg();
	if(end == -1) {
	
		// Display error
		cout << "Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if going back to the beginning of the input failed
	if(!input.seekg(0, input.beg)) {
	
		// Display error
		cout << "Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if opening output file failed
	ofstream output(outputFileName + ".h", ofstream::binary);
	if(!output) {
	
		// Display error
		cout << "Opening output file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if opening resources file failed
	ofstream resources("resources.h", ofstream::binary | ofstream::app);
	if(!resources) {
	
		// Display error
		cout << "Opening resources file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}

	// Write header guard to output
	output << "// Header guard" << endl << "#ifndef " << headerGaurdName << "_H" << endl << "#define " << headerGaurdName << "_H" << endl << endl << endl;
	
	// Write size to output
	output << "// Constants" << endl << endl << "// Size" << endl << "const uintmax_t " << variableName << "Size = " << to_string(end - begin) << ';' << endl << endl;
	
	// Write start of data to output
	output << "// Data" << endl << "const uint8_t " << variableName << "Data[] = {";
	
	// Go through all characters in the input
	for(char character; input.get(character);) {
		
		// Write character's hexadecimal representation to output
		output << "0x" << hex << uppercase << setw(2) << setfill('0') << (static_cast<uint16_t>(character) & 0xFF);
		
		// Check if at least one more character exists in the file
		if(input.peek() != ifstream::traits_type::eof())
		
			// Write a comma to output
			output << ", ";
	}
	
	// Write end of data to output
	output << "};" << endl << endl << endl;
	
	// Write end of header guard to output
	output << "#endif";
	
	// Append output to resources
	resources << "#include \"" << outputFileName << ".h\"" << endl;
	
	// Check if reading input failed
	if(!input.eof()) {
	
		// Delete output file
		remove((outputFileName + ".h").c_str());
		
		// Delete resources file
		remove("resources.h");
	
		// Display error
		cout << "Reading input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Check if writing to output failed
	if(!output) {
	
		// Delete output file
		remove((outputFileName + ".h").c_str());
		
		// Delete resources file
		remove("resources.h");
	
		// Display error
		cout << "Writing output file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Close input
	input.clear();
	input.close();
	
	// Check if closing input failed
	if(!input) {
	
		// Delete output file
		remove((outputFileName + ".h").c_str());
		
		// Delete resources file
		remove("resources.h");
	
		// Display error
		cout << "Closing input file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Close output
	output.close();
	
	// Check if closing output failed
	if(!output) {
	
		// Delete output file
		remove((outputFileName + ".h").c_str());
		
		// Delete resources file
		remove("resources.h");
	
		// Display error
		cout << "Closing output file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Close resources
	resources.close();
	
	// Check if closing resources failed
	if(!resources) {
	
		// Delete resources file
		remove("resources.h");
	
		// Display error
		cout << "Closing resources file failed" << endl;
		
		// Return failure
		return EXIT_FAILURE;
	}
	
	// Print message
	cout << outputFileName << " generated successfully" << endl;
	
	// Return success
	return EXIT_SUCCESS;
}
