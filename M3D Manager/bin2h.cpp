// Header files
#include <iostream>
#include <fstream>
#include <string>

using namespace std;


// Main function
int main(int argc, char *argv[]) {

	// Check if correct number of arguments
	if(argc != 2) {
	
		// Print error
		cout << "Invalid number of parameters" << endl;
		
		// Return 1
		return 1;
	}
	
	// Check if help argument was specified
	if(static_cast<string>(argv[1]) == "-h" || static_cast<string>(argv[1]) == "--help") {
	
		// Print message
		cout << "Correct usage: ./bin2h fileToConvert" << endl;
		
		// Return 1
		return 1;
	}
	
	// Open file
	ifstream fin(argv[1], ios::binary);
	
	// Check if file doesn't exists
	if(!fin.good()) {
	
		// Print error
		cout << "File not found" << endl;
		
		// Return 1
		return 1;
	}
	
	// Get size of file
	unsigned long long begin = fin.tellg();
	fin.seekg(0, ios::end);
	unsigned long long end = fin.tellg();
	fin.seekg(0, ios::beg);
	
	// Set output
	string output = argv[1];
	for(int i = 0; i < output.length(); i++)
		if(output[i] == '.' || output[i] == '-')
			output[i] = '_';
	for(int i = output.length() - 1; i >= 0; i--)
		if(output[i] == '/') {
			output.erase(0, i + 1);
			break;
		}
	if(isdigit(output[0]))
		output= '_' + output;
	
	string name = output;
	output += ".h";

	// Set upper
	string upper = name;
	for(unsigned int i = 0; i < upper.length(); i++)
		if(upper[i] == ' ')
			upper[i] = '_';
	for(unsigned int i = 0; i < upper.length(); i++)
		if(upper[i] >= 'a' && upper[i] <= 'z')
			upper[i] -= 32;
	
	// Set name
	for(unsigned int i = 0; i < name.length(); i++)
		if(name[i] == ' ') {
			name.erase(i,1);
			if(name[i] >= 'a' && name[i] <= 'z')
				name[i] -= 32;
		}
	if(name[0] >= 'A' && name[0] <= 'Z')
		name[0] += 32;
	
	// Open output file
	ofstream fout(output.c_str(), ios::binary);

	// Write header gaurd
	fout << "// Header gaurd" << endl << "#ifndef " << upper << "_H" << endl << "#define " << upper << "_H" << endl << endl << endl;
	
	// Write file size
	fout << "// Size and data" << endl << "const unsigned long long " << name << "Size = " << end - begin << ';' << endl << endl;
	
	// Write char array
	fout << "const unsigned char " << name << "Data[] = {";
	
	for(unsigned long long i = 0; i < end - begin; i++) {
		if(i % 12 == 0)
			fout << endl << '\t';
		unsigned char character = fin.get();
		if((character & 0xf0) == 0x00)
			fout << hex << "0x0" << static_cast<unsigned int>(character);
		else
			fout << hex << "0x" << static_cast<unsigned int>(character);	
		if(i != end - begin - 1)
			fout << ", ";
	}
	
	// Write end of array
	fout << endl << "};" << endl << endl << endl;;
	
	// Write end of header gaurd
	fout << "#endif";
	
	// Print message
	cout << output << " generated successfully" << endl;
	
	// Close files
	fin.close();
	fout.close();
	
	// Return 0
	return 0;
}
