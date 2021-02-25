#pragma once
#include "Datatypes.hpp"

namespace SerialConstant {
	enum State : byte {
		IDLE			= 0b11110000, // 0x70 | NS
		HALT			= 0b11111110, // 0x7E | NS
    };
    enum Flag : byte {
		NAMESPACE		= 0b10000000,

		DATA_START		= 0b10000001, // 0x01 | NS
		DATA_END		= 0b11111111, // 0x7F | NS
    };
    enum Data : byte {
		TYPE_DATETIME   = 0b10010000, // 0x10 | NS
		TYPE_CPUTIL     = 0b10010001, // 0x11 | NS
		TYPE_RAMUTIL    = 0b10010010, // 0x12 | NS

		SIZE_DATETIME   = 0b00000100, // 0x04
		SIZE_CPUTIL     = 0b00000001, // 0x01
		SIZE_RAMUTIL    = 0b00000001, // 0x01

		SIZE_SERIALBUF  = 0x40        // 64 bytes
	};
};