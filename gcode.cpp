// Header files
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "gcode.h"


// Definitions
#define PARAMETER_G_OFFSET 1
#define PARAMETER_M_OFFSET (1 << 1)
#define PARAMETER_T_OFFSET (1 << 2)
#define PARAMETER_S_OFFSET (1 << 3)
#define PARAMETER_P_OFFSET (1 << 4)
#define PARAMETER_X_OFFSET (1 << 5)
#define PARAMETER_Y_OFFSET (1 << 6)
#define PARAMETER_Z_OFFSET (1 << 7)
#define PARAMETER_F_OFFSET (1 << 8)
#define PARAMETER_E_OFFSET (1 << 9)
#define PARAMETER_N_OFFSET (1 << 10)
#define PARAMETER_HOST_COMMAND_OFFSET (1 << 11)
#define INVALID 0xFF


// Global constants
const uint8_t orderToOffset[] = {
	/*    0 nul */ INVALID, /*    1 soh */ INVALID, /*    2 stx */ INVALID, /*    3 etx */ INVALID, /*    4 eot */ INVALID, /*    5 enq */ INVALID, /*    6 ack */ INVALID, /*    7 bel */ INVALID,
	/*    8 bs  */ INVALID, /*    9 ht  */ INVALID, /*   10 nl  */ INVALID, /*   11 vt  */ INVALID, /*   12 np  */ INVALID, /*   13 cr  */ INVALID, /*   14 so  */ INVALID, /*   15 si  */ INVALID,
	/*   16 dle */ INVALID, /*   17 dc1 */ INVALID, /*   18 dc2 */ INVALID, /*   19 dc3 */ INVALID, /*   20 dc4 */ INVALID, /*   21 nak */ INVALID, /*   22 syn */ INVALID, /*   23 etb */ INVALID,
	/*   24 can */ INVALID, /*   25 em  */ INVALID, /*   26 sub */ INVALID, /*   27 esc */ INVALID, /*   28 fs  */ INVALID, /*   29 gs  */ INVALID, /*   30 rs  */ INVALID, /*   31 us  */ INVALID,
	/*   32 sp  */ INVALID, /*   33  !  */ INVALID, /*   34  "  */ INVALID, /*   35  #  */ INVALID, /*   36  $  */ INVALID, /*   37  %  */ INVALID, /*   38  &  */ INVALID, /*   39  '  */ INVALID,
	/*   40  (  */ INVALID, /*   41  )  */ INVALID, /*   42  *  */ INVALID, /*   43  +  */ INVALID, /*   44  ,  */ INVALID, /*   45  -  */ INVALID, /*   46  .  */ INVALID, /*   47  /  */ INVALID,
	/*   48  0  */ INVALID, /*   49  1  */ INVALID, /*   50  2  */ INVALID, /*   51  3  */ INVALID, /*   52  4  */ INVALID, /*   53  5  */ INVALID, /*   54  6  */ INVALID, /*   55  7  */ INVALID,
	/*   56  8  */ INVALID, /*   57  9  */ INVALID, /*   58  :  */ INVALID, /*   59  ;  */ INVALID, /*   60  <  */ INVALID, /*   61  =  */ INVALID, /*   62  >  */ INVALID, /*   63  ?  */ INVALID,
	/*   64  @  */ INVALID, /*   65  A  */ INVALID, /*   66  B  */ INVALID, /*   67  C  */ INVALID, /*   68  D  */ INVALID, /*   69  E  */    9,    /*   70  F  */    8,    /*   71  G  */    0,
	/*   72  H  */ INVALID, /*   73  I  */ INVALID, /*   74  J  */ INVALID, /*   75  K  */ INVALID, /*   76  L  */ INVALID, /*   77  M  */    1,    /*   78  N  */   10,    /*   79  O  */ INVALID,
	/*   80  P  */    4,    /*   81  Q  */ INVALID, /*   82  R  */ INVALID, /*   83  S  */    3,    /*   84  T  */    2,    /*   85  U  */ INVALID, /*   86  V  */ INVALID, /*   87  W  */ INVALID,
	/*   88  X  */    5,    /*   89  Y  */    6,    /*   90  Z  */    7,    /*   91  [  */ INVALID, /*   92  \  */ INVALID, /*   93  ]  */ INVALID, /*   94  ^  */ INVALID, /*   95  _  */ INVALID,
	/*   96  `  */ INVALID, /*   97  a  */ INVALID, /*   98  b  */ INVALID, /*   99  c  */ INVALID, /*  100  d  */ INVALID, /*  101  e  */    9,    /*  102  f  */    8,    /*  103  g  */    0,
	/*  104  h  */ INVALID, /*  105  i  */ INVALID, /*  106  j  */ INVALID, /*  107  k  */ INVALID, /*  108  l  */ INVALID, /*  109  m  */    1,    /*  110  n  */   10,    /*  111  o  */ INVALID,
	/*  112  p  */    4,    /*  113  q  */ INVALID, /*  114  r  */ INVALID, /*  115  s  */    3,    /*  116  t  */    2,    /*  117  u  */ INVALID, /*  118  v  */ INVALID, /*  119  w  */ INVALID,
	/*  120  x  */    5,    /*  121  y  */    6,    /*  122  z  */    7,    /*  123  {  */ INVALID, /*  124  |  */ INVALID, /*  125  }  */ INVALID, /*  126  ~  */ INVALID, /*  127 del */ INVALID,
	/*  128     */ INVALID, /*  129     */ INVALID, /*  130     */ INVALID, /*  131     */ INVALID, /*  132     */ INVALID, /*  133     */ INVALID, /*  134     */ INVALID, /*  135     */ INVALID,
	/*  136     */ INVALID, /*  137     */ INVALID, /*  138     */ INVALID, /*  139     */ INVALID, /*  140     */ INVALID, /*  141     */ INVALID, /*  142     */ INVALID, /*  143     */ INVALID,
	/*  144     */ INVALID, /*  145     */ INVALID, /*  146     */ INVALID, /*  147     */ INVALID, /*  148     */ INVALID, /*  149     */ INVALID, /*  150     */ INVALID, /*  151     */ INVALID,
	/*  152     */ INVALID, /*  153     */ INVALID, /*  154     */ INVALID, /*  155     */ INVALID, /*  156     */ INVALID, /*  157     */ INVALID, /*  158     */ INVALID, /*  159     */ INVALID,
	/*  160     */ INVALID, /*  161     */ INVALID, /*  162     */ INVALID, /*  163     */ INVALID, /*  164     */ INVALID, /*  165     */ INVALID, /*  166     */ INVALID, /*  167     */ INVALID,
	/*  168     */ INVALID, /*  169     */ INVALID, /*  170     */ INVALID, /*  171     */ INVALID, /*  172     */ INVALID, /*  173     */ INVALID, /*  174     */ INVALID, /*  175     */ INVALID,
	/*  176     */ INVALID, /*  177     */ INVALID, /*  178     */ INVALID, /*  179     */ INVALID, /*  180     */ INVALID, /*  181     */ INVALID, /*  182     */ INVALID, /*  183     */ INVALID,
	/*  184     */ INVALID, /*  185     */ INVALID, /*  186     */ INVALID, /*  187     */ INVALID, /*  188     */ INVALID, /*  189     */ INVALID, /*  190     */ INVALID, /*  191     */ INVALID,
	/*  192     */ INVALID, /*  193     */ INVALID, /*  194     */ INVALID, /*  195     */ INVALID, /*  196     */ INVALID, /*  197     */ INVALID, /*  198     */ INVALID, /*  199     */ INVALID,
	/*  200     */ INVALID, /*  201     */ INVALID, /*  202     */ INVALID, /*  203     */ INVALID, /*  204     */ INVALID, /*  205     */ INVALID, /*  206     */ INVALID, /*  207     */ INVALID,
	/*  208     */ INVALID, /*  209     */ INVALID, /*  210     */ INVALID, /*  211     */ INVALID, /*  212     */ INVALID, /*  213     */ INVALID, /*  214     */ INVALID, /*  215     */ INVALID,
	/*  216     */ INVALID, /*  217     */ INVALID, /*  218     */ INVALID, /*  219     */ INVALID, /*  220     */ INVALID, /*  221     */ INVALID, /*  222     */ INVALID, /*  223     */ INVALID,
	/*  224     */ INVALID, /*  225     */ INVALID, /*  226     */ INVALID, /*  227     */ INVALID, /*  228     */ INVALID, /*  229     */ INVALID, /*  230     */ INVALID, /*  231     */ INVALID,
	/*  232     */ INVALID, /*  233     */ INVALID, /*  234     */ INVALID, /*  235     */ INVALID, /*  236     */ INVALID, /*  237     */ INVALID, /*  238     */ INVALID, /*  239     */ INVALID,
	/*  240     */ INVALID, /*  241     */ INVALID, /*  242     */ INVALID, /*  243     */ INVALID, /*  244     */ INVALID, /*  245     */ INVALID, /*  246     */ INVALID, /*  247     */ INVALID,
	/*  248     */ INVALID, /*  249     */ INVALID, /*  250     */ INVALID, /*  251     */ INVALID, /*  252     */ INVALID, /*  253     */ INVALID, /*  254     */ INVALID, /*  255     */ INVALID
};


// Supporting function implementation
Gcode::Gcode(const char *command) {

	// Check if a command is specified
	if(command)
	
		// Parse command
		parseCommand(command);
	
	// Otherwise
	else
	
		// Clear command
		clearCommand();
}

bool Gcode::parseCommand(const char *command) {

	// Initialize variables
	const char *firstValidCharacter = command, *lastValidCharacter;

	// Clear command
	clearCommand();
	
	// Remove leading whitespace
	while(isspace(*firstValidCharacter))
		firstValidCharacter++;
	
	// Get last valid character
	for(lastValidCharacter = firstValidCharacter; *lastValidCharacter && *lastValidCharacter != ';' && *lastValidCharacter != '*'; lastValidCharacter++);
	lastValidCharacter--;
	
	// Remove trailing white space
	while((lastValidCharacter >= firstValidCharacter) && isspace(*lastValidCharacter))
		lastValidCharacter--;
	
	// Check if command is empty
	if(++lastValidCharacter == firstValidCharacter)
	
		// Return false
		return false;
	
	// Set start and stop parsing offsets
	uint8_t startParsingOffset = firstValidCharacter - command;
	uint8_t stopParsingOffset = lastValidCharacter - command;
	
	// Check if command is a host command
	if(*firstValidCharacter == '@') {
	
		// Set command length
		uint8_t commandLength = stopParsingOffset - startParsingOffset;
	
		// Check if host command is empty
		if(commandLength == 1)
		
			// Return false
			return false;
	
		// Save host command
		strncpy(hostCommand, firstValidCharacter, commandLength);
		hostCommand[commandLength] = 0;
		
		// Set command parameters
		commandParameters |= PARAMETER_HOST_COMMAND_OFFSET;
	}
	
	// Otherwise
	else
	
		// Go through each valid character in the command
		for(uint8_t i = startParsingOffset; i < stopParsingOffset; i++) {
		
			// Get parameter offset
			uint8_t parameterOffset = orderToOffset[static_cast<uint8_t>(command[i])];
	
			// Check if character is a valid parameter
			if(parameterOffset != INVALID) {
			
				// Go through parameter's value
				const char *lastParameterCharacter = &command[++i];
				for(uint8_t j = i; j < stopParsingOffset; j++)
			
					// Check if at end of parameter's value
					if(isspace(command[j]) || isalpha(command[j])) {
				
						// Save offset
						lastParameterCharacter = &command[j];
						break;
					}
				
					// Otherwise check if parameter goes to the end of the command
					else if(j == stopParsingOffset - 1) {
				
						// Set offset
						lastParameterCharacter = NULL;
						break;
					}
			
				// Check if parameter exists
				if(lastParameterCharacter != &command[i]) {
				
					// Get parameter's bit
					uint16_t parameterBit = 1 << parameterOffset;
			
					// Set command parameters
					commandParameters |= parameterBit;
				
					// Save parameter's value
					switch(parameterBit) {
				
						case PARAMETER_G_OFFSET:
							valueG = strtoul(&command[i], const_cast<char **>(&lastParameterCharacter), 10);
						break;
					
						case PARAMETER_M_OFFSET:
							valueM = strtoul(&command[i], const_cast<char **>(&lastParameterCharacter), 10);
						break;
					
						case PARAMETER_T_OFFSET:
							valueT = strtoul(&command[i], const_cast<char **>(&lastParameterCharacter), 10);
						break;
					
						case PARAMETER_S_OFFSET:
							valueS = strtol(&command[i], const_cast<char **>(&lastParameterCharacter), 10);
						break;
					
						case PARAMETER_P_OFFSET:
							valueP = strtol(&command[i], const_cast<char **>(&lastParameterCharacter), 10);
						break;
					
						case PARAMETER_X_OFFSET:
							valueX = strtod(&command[i], const_cast<char **>(&lastParameterCharacter));
						break;
					
						case PARAMETER_Y_OFFSET:
							valueY = strtod(&command[i], const_cast<char **>(&lastParameterCharacter));
						break;
					
						case PARAMETER_Z_OFFSET:
							valueZ = strtod(&command[i], const_cast<char **>(&lastParameterCharacter));
						break;
					
						case PARAMETER_F_OFFSET:
							valueF = strtod(&command[i], const_cast<char **>(&lastParameterCharacter));
						break;
					
						case PARAMETER_E_OFFSET:
							valueE = strtod(&command[i], const_cast<char **>(&lastParameterCharacter));
						break;
					
						case PARAMETER_N_OFFSET:
							valueN = strtoul(&command[i], const_cast<char **>(&lastParameterCharacter), 10);
					}
				
					// Increment index
					i += lastParameterCharacter - &command[i];
				}
			
				// Decrement index
				i--;
			}
		}
	
	// Return if command contains parameters
	return commandParameters;
}

void Gcode::clearCommand() {

	// Set values to defaults
	commandParameters = 0;
	valueG = 0;
	valueM = 0;
	valueT = 0;
	valueS = 0;
	valueP = 0;
	valueX = 0;
	valueY = 0;
	valueZ = 0;
	valueF = 0;
	valueE = 0;
	valueN = 0;
	*hostCommand = 0;
}

bool Gcode::isEmpty() const {

	// Return if command contains no parameters
	return !commandParameters;
}

bool Gcode::hasParameterG() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_G_OFFSET;
}

uint8_t Gcode::getParameterG() const {

	// Return parameter's value
	return valueG;
}

bool Gcode::hasParameterM() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_M_OFFSET;
}

uint16_t Gcode::getParameterM() const {

	// Return parameter's value
	return valueM;
}

bool Gcode::hasParameterT() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_T_OFFSET;
}

uint8_t Gcode::getParameterT() const {

	// Return parameter's value
	return valueT;
}

bool Gcode::hasParameterS() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_S_OFFSET;
}

int32_t Gcode::getParameterS() const {

	// Return parameter's value
	return valueS;
}

bool Gcode::hasParameterP() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_P_OFFSET;
}

int32_t Gcode::getParameterP() const {

	// Return parameter's value
	return valueP;
}

bool Gcode::hasParameterX() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_X_OFFSET;
}

float Gcode::getParameterX() const {

	// Return parameter's value
	return valueX;
}

bool Gcode::hasParameterY() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_Y_OFFSET;
}

float Gcode::getParameterY() const {

	// Return parameter's value
	return valueY;
}

bool Gcode::hasParameterZ() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_Z_OFFSET;
}

float Gcode::getParameterZ() const {

	// Return parameter's value
	return valueZ;
}

bool Gcode::hasParameterF() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_F_OFFSET;
}

float Gcode::getParameterF() const {

	// Return parameter's value
	return valueF;
}

bool Gcode::hasParameterE() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_E_OFFSET;
}

float Gcode::getParameterE() const {

	// Return parameter's value
	return valueE;
}

bool Gcode::hasParameterN() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_N_OFFSET;
}

uint32_t Gcode::getParameterN() const {

	// Return parameter's value
	return valueN;
}

bool Gcode::hasHostCommand() const {

	// Return is host command is set
	return *hostCommand;
}

const char *Gcode::getHostCommand() const {

	// Return host command
	return hostCommand;
}
