#include <resources.hpp>

#ifdef _WIN32
/* 	CPU Load Percent START																			 *\
|*	Credit: Jeremy Friesner																			 *|
\*	https://stackoverflow.com/questions/23143693/retrieving-cpu-load-percent-total-in-windows-with-c */
namespace jfriesner_stackoverflow {
	static f32 calculateCPULoad(u64 idleTicks, u64 totalTicks){
		static u64 _pTotalTicks = 0;
		static u64 _pIdleTicks = 0;

		u64 totalTicksSinceLast = totalTicks-_pTotalTicks;
		u64 idleTicksSinceLast  = idleTicks-_pIdleTicks;

		f32 ret = 1.0f-((totalTicksSinceLast > 0) ? ((f32)idleTicksSinceLast)/totalTicksSinceLast : 0);

		_pTotalTicks = totalTicks;
		_pIdleTicks  = idleTicks;
		return ret;
	}

	static u64 fileTimeToInt64(const FILETIME &ft){
		return (((u64)(ft.dwHighDateTime))<<32) | ((u64)ft.dwLowDateTime);
	}

	// Range 0.0f - 1.0f
	// Error -1.0f
	// Must be called at regular intervals, measures "since last time called"
	f32 getCPULoad(){
		FILETIME idleTime, kernelTime, userTime;
		#ifndef __INTELLISENSE__
		return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? calculateCPULoad(fileTimeToInt64(idleTime), fileTimeToInt64(kernelTime)+fileTimeToInt64(userTime)) : -1.0f;
		#endif
	}
}

using namespace std;
unsigned char Resources::getCPUtil(){
	auto util = jfriesner_stackoverflow::getCPULoad();
	if(util < 0) throw new std::exception();
	Sleep(1000);
	util = jfriesner_stackoverflow::getCPULoad();
	if(util < 0 || util > 1.0f) throw new std::exception();
	return (unsigned char)(floor(util*100.0f+0.5));	
}

unsigned char Resources::getRAMUtil(){
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	return (unsigned char)statex.dwMemoryLoad;
}

#else
byte Resources::getCPUtil(){
	// read 1st val /proc/loadavg
	// *100/no of cpuc
	return 0xfe;
}
byte Resources::getRAMUtil(){
	return 0xfe;
}
#endif
