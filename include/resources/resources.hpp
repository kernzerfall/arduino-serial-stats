#pragma once

#include <datatypes.hpp>
#include <exception>
#include <cmath>

#ifdef __WIN32
#include <windows.h>
#include <ctime>
#else
#include <fstream>
#endif

namespace Resources {
	byte getCPUtil();
	byte getRAMUtil();
};
