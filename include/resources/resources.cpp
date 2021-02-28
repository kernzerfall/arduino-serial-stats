#include <resources.hpp>

#ifdef _WIN32
/*  CPU Load Percent START                                                                           *\
|*  Credit: Jeremy Friesner                                                                          *|
\*  https://stackoverflow.com/questions/23143693/retrieving-cpu-load-percent-total-in-windows-with-c */
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
	Sleep(CPU_LOAD_INTERVAL);
	util = jfriesner_stackoverflow::getCPULoad();
	if(util < 0 || util > 1.0f) throw new std::exception();
	return static_cast<unsigned char>((floor(util*100.0f+0.5f)));	
}

unsigned char Resources::getRAMUtil(){
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	return static_cast<unsigned char>(statex.dwMemoryLoad);
}

#else

byte Resources::getCPUtil(){
	std::ifstream stat;
	std::string line;

	s64 total = 0.0f;
	s64 work = 0.0f;
	for(s16 i = 0; i < 2; ++i){
		stat.open("/proc/stat");
		while(std::getline(stat, line)){
			if(line.substr(0,3).find("cpu") != std::string::npos){
				line = line.substr(line.find_first_of(" ")+1);
				std::istringstream iss(line);
				s64 curr;
				for(u16 j = 0; iss>>curr; j++){
					if(j<3) work += ((i==0)?(-1):(1)) * curr;
					total += ((i==0)?(-1):(1)) * curr;
				}
				// if cpu line found, no need to parse the rest of the file
				break;
			} else continue;
		}
		stat.close();
		usleep(CPU_LOAD_INTERVAL * 1e+3);
	}

	f32 res = floor((f32)(work) / (f32)(total) * 100.0f + 0.5f);
	return static_cast<byte>(res);
}
byte Resources::getRAMUtil(){
	struct sysinfo si;
	sysinfo(&si);
	f32 ramutil = floor((f32)(si.totalram-si.freeram)/(f32)si.totalram * 100.0f + 0.5f);
	return static_cast<byte>(ramutil);
}
#endif
