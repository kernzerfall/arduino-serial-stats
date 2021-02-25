// CPU Load Percent (Credit to Jeremy Friesner): https://stackoverflow.com/questions/23143693/retrieving-cpu-load-percent-total-in-windows-with-c
#include <resources.hpp>

f32 Resources::floor(f32 x){
	s64 o = (s64)trunc(10.0f*x);
	o = o - (o % 10);
	f32 k = ((float)o)/10.0f;
	return k;
}

#ifdef _WIN32
#include <Windows.h>
static f32 CalculateCPULoad(u64 idleTicks, u64 totalTicks){
	static u64 _previousTotalTicks = 0;
	static u64 _previousIdleTicks = 0;

	u64 totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
	u64 idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;

	f32 ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((f32)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);

	_previousTotalTicks = totalTicks;
	_previousIdleTicks  = idleTicks;
	return ret;
}

static u64 FileTimeToInt64(const FILETIME &ft){
	return (((u64)(ft.dwHighDateTime))<<32) | ((u64)ft.dwLowDateTime);
}

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.  Returns -1.0 on error.
f32 GetCPULoad(){
	FILETIME idleTime, kernelTime, userTime;
	#ifndef __INTELLISENSE__
	return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime)+FileTimeToInt64(userTime)) : -1.0f;
	#endif
}
using namespace std;
unsigned char Resources::getCPUtil(){
	auto util = GetCPULoad();
	if(util < 0) throw new std::exception();
	Sleep(1000);
	util = GetCPULoad();
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
