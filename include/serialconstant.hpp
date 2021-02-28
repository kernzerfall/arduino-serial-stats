/*     This header contains some constants that are used     *\
|*       during the serial communication between server      *|
\*      and client to distinguish and parse data packets     */


#pragma once
#include "datatypes.hpp"

namespace SerialConstant {
	enum State : byte {
		IDLE			= 0b11110000, // 0x70 | NS : This is set when the parser hasn't found the start of a packets
		HALT			= 0b11111110, // 0x7E | NS : When the board receives this, it will reset immediately
	};
    enum Flag : byte {
		NAMESPACE		= 0b10000000, // 0x80 = NS : Used to distingush between a packet's body and its header/footer		

		DATA_START		= 0b10000001, // 0x01 | NS : Signifies a data packet's start
		DATA_END		= 0b11111111, // 0x7F | NS : Signifies a data packet's end
	};
    enum Data : byte {
		// Define data types
		DATETIME		= 0b10010000, // 0x10 | NS
		CPUTIL			= 0b10010001, // 0x11 | NS
		RAMUTIL    		= 0b10010010, // 0x12 | NS
	};

	// Define preset sizes
	enum Size : byte {
		SDATETIME		= 0x04,
		SCPUTIL 		= 0x01,
		SRAMUTIL		= 0x01,

		SERIALBUF		= 0x40 // 64 bytes
	};
};