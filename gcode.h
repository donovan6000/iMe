// Header guard
#ifndef GCODE_H
#define GCODE_H


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
#define VALID_CHECKSUM_OFFSET (1 << 12)


// Types
typedef uint16_t gcodeParameterOffset;


// Gcode class
class Gcode final {

	// Public
	public:
		
		// Parse command
		void parseCommand(const char *command) noexcept;
		
		// Clear command
		void clearCommand() noexcept;
		
		// Is empty
		bool isEmpty() const noexcept;
		
		// Has parameter G
		bool hasParameterG() const noexcept;
		
		// Get parameter G
		uint8_t getParameterG() const noexcept;
		
		// Has parameter M
		bool hasParameterM() const noexcept;
		
		// Get parameter M
		uint16_t getParameterM() const noexcept;
		
		// Has parameter T
		bool hasParameterT() const noexcept;
		
		// Get parameter T
		uint8_t getParameterT() const noexcept;
		
		// Has parameter S
		bool hasParameterS() const noexcept;
		
		// Get parameter S
		int32_t getParameterS() const noexcept;
		
		// Has parameter P
		bool hasParameterP() const noexcept;
		
		// Get parameter P
		int32_t getParameterP() const noexcept;
		
		// Has parameter X
		bool hasParameterX() const noexcept;
		
		// Get parameter X
		float getParameterX() const noexcept;
		
		// Has parameter Y
		bool hasParameterY() const noexcept;
		
		// Get parameter Y
		float getParameterY() const noexcept;
		
		// Has parameter Z
		bool hasParameterZ() const noexcept;
		
		// Get parameter Z
		float getParameterZ() const noexcept;
		
		// Has parameter F
		bool hasParameterF() const noexcept;
		
		// Get parameter F
		float getParameterF() const noexcept;
		
		// Has parameter E
		bool hasParameterE() const noexcept;
		
		// Get parameter E
		float getParameterE() const noexcept;
		
		// Has parameter N
		bool hasParameterN() const noexcept;
		
		// Get parameter N
		uint64_t getParameterN() const noexcept;
		
		#if ALLOW_HOST_COMMANDS == true
		
			// Has host command
			bool hasHostCommand() const noexcept;
		
			// Get host command
			const char *getHostCommand() const noexcept;
		#endif
		
		// Has valid checksum
		bool hasValidChecksum() const noexcept;
		
		// Is parsed
		bool isParsed;
		
		// Command parameters
		gcodeParameterOffset commandParameters;
		
		// Values
		uint8_t valueG;
		uint16_t valueM;
		uint8_t valueT;
		int32_t valueS;
		int32_t valueP;
		float valueX;
		float valueY;
		float valueZ;
		float valueF;
		float valueE;
		uint64_t valueN;
		
		// Host command
		#if ALLOW_HOST_COMMANDS == true
			char hostCommand[UINT8_MAX + 1];
		#endif
};


#endif
