#pragma once

#define CPU_LOAD_INTERVAL 1000 // millis

#include <datatypes.hpp>
#include <exception>
#include <cmath>

#ifdef __WIN32
#include <windows.h>
#include <ctime>
#else
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

namespace Resources {
	byte getCPUtil();
	byte getRAMUtil();
};
