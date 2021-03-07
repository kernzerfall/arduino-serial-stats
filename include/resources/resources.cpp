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
	// Used to keep track of what the program is supposed to be parsing
	enum class State { Total, Available, None };
    State state = State::None;

	// ifstream of /proc/meminfo
    std::ifstream meminfo;
    meminfo.open("/proc/meminfo");

	// Important values will be stored here
    s64 available = -1 , total = -1;

	// Go through the file line by line
    std::string line;
    while(std::getline(meminfo, line)){
		// If both needed values are set, exit the loop
        if(available != -1 && total != -1) break;

		// Try to find the semicolon
        size_t start = line.find(":");
        if(start != std::string::npos){
			// If relevant info is reached, set the state
            if		(line.find("MemTotal") != std::string::npos)
				state = State::Total;
            else if (line.find("MemAvailable") != std::string::npos) 	
				state = State::Available;
			// If the line is irrelevant, there's no need to go through
			// the commands that follow
            else continue;
		// If semicolon isn't found, silently go on
        } else continue;

		// try to find the k from "kB"
        size_t end = line.find("k");
        if(end != std::string::npos){
			// Calculate the length of the relevant part in the string
            size_t span = end-start;

			// Set up a stringstream and set to ignore whitespaces
            std::istringstream num(line.substr(start, span));
            num.ignore(1);

			// Try to read the integer that *should* be on the string
            s64 readVal;
            if(num>>readVal){
                switch (state){
					// Set the appropriate variable according to the state
                    case State::Available: available = readVal; break;;
                    case State::Total: total = readVal; break;;
                    default: continue; break;;
                }
            }
        }
    
    }
	// Calculate used ram (0.0f-100.0f)
    f32 temp = static_cast<f32>(total-available)/static_cast<f32>(total) * 100.0f;
	// Round to nearest integer
	temp = floor(temp + 0.5f);
	// Return value as byte; temp should satisfy: temp ∈ [0.0f - 100.f]
	// Its value should fit in a byte, where ∀byte c: c ∈ [0-255]
	return static_cast<byte>(temp);
}
#endif
