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

#ifdef __WIN32
static f32 CalculateCPULoad(u64 idleTicks, u64 totalTicks);
static u64 FileTimeToInt64(const FILETIME &ft);
f32 GetCPULoad();
#endif

namespace Resources {
	byte getCPUtil();
	byte getRAMUtil();
	f32 floor(f32);
};
