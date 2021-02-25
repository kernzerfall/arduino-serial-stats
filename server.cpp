#include<iostream>
#include<ctime>
#include<vector>
#include<string>
#include<signal.h>

// Shared headers
#include<datatypes.hpp>
#include<serialconstant.hpp>

// External libraries
#include<resources.hpp>
#include<rs232.h>

#ifdef __WIN32 
#define SLEEP(x) Sleep(x)
#else
#define SLEEP(x) usleep(x/1e+3)
#endif

#define DPRINT(x) if(dbgprint) printf(x)
#define DDUMP(x) if(dbgprint) printData(x)
#define FERR(x) { fprintf(stderr, x); exit(127); }

namespace Except {
	class BufferOverflow 	: public std::exception {};
	class MalformedPacket	: public std::exception {};
};

bool dbgprint = true;
int arduinoPort = -1;

auto buildTimePacket() -> std::vector<byte> {
	auto t = std::time(0);
	std::tm* now = std::localtime(&t);
	return std::vector<byte> {
		SerialConstant::Flag::DATA_START,
		SerialConstant::Data::TYPE_DATETIME,
		(byte)now->tm_hour,
		(byte)now->tm_min,
		(byte)now->tm_mday,
		(byte)now->tm_mon,
		SerialConstant::Flag::DATA_END
	};
}

auto printData(std::vector<byte> data, std::string prepend = ""){
	std::cout << prepend << "[\x20";
	for(byte c: data)
		printf("0x%02x, ", c);
	std::cout<<"\b\b\x20]\n";
}

// Note: buildCPUtilPacket blocks the thread for 1 second
auto buildCPUtilPacket() -> std::vector<byte> {
	// PROCURE CPUINFO
	byte cputil = Resources::getCPUtil();
	return std::vector<byte> {
		SerialConstant::Flag::DATA_START,
		SerialConstant::Data::TYPE_CPUTIL,
		cputil,
		SerialConstant::Flag::DATA_END
	};
}

auto buildRAMUtilPacket() -> std::vector<byte> {
	byte ramutil = Resources::getRAMUtil();
	return std::vector<byte> {
		SerialConstant::Flag::DATA_START,
		SerialConstant::Data::TYPE_RAMUTIL,
		ramutil,
		SerialConstant::Flag::DATA_END
	};
}

auto sendData(std::vector<byte> data) -> int {
	if(arduinoPort == -1) return 1;
	const u64 s = data.size();
	if(s < 4 || s > SerialConstant::Data::SIZE_SERIALBUF + 3)
		throw new Except::BufferOverflow();
	if( 
		data.at(0) != SerialConstant::Flag::DATA_START ||
		!(data.at(1) & SerialConstant::Flag::NAMESPACE)||
		data.back() != SerialConstant::Flag::DATA_END 
	) throw new Except::MalformedPacket();
	DDUMP(data);
	int err = 0;
	for(byte b: data)
		err += RS232_SendByte(arduinoPort, b);
	return err;
}

void haltArduino(){
	if(arduinoPort != -1){ 
		RS232_SendByte(arduinoPort, SerialConstant::State::HALT);
		RS232_CloseComport(arduinoPort);
	}
}

void signalHandler(int signum){
	haltArduino();
	std::cerr << "Received interrupt signal " << signum << std::endl;
	exit(signum);
}



auto main(int argc, char** argv) -> int {
	signal(SIGINT, signalHandler);
	if(argc != 2) FERR("Incorrect argc\n")
	arduinoPort = RS232_GetPortnr(argv[1]);
	if(arduinoPort == -1) FERR("Port not found\n")
	if(RS232_OpenComport(arduinoPort, 9600, "8N1", 0)) 
		FERR("Couldn't open serial port\n");
	SLEEP(7000);
	
	int errorCycles = 0;
	int errCC = 0;
	while(1){
		errCC = 0;
		std::vector<byte> sBuf;
		try {
			for(byte b: buildTimePacket()) sBuf.push_back(b);
			for(byte b: buildCPUtilPacket()) sBuf.push_back(b);
			for(byte b: buildRAMUtilPacket()) sBuf.push_back(b);
		} catch (std::exception e) {
			std::cerr << "Failed to build data packets\n";
			errorCycles++;
			continue;
		}

		errCC += sendData(sBuf);
		
		if(errCC != 0) errorCycles++;
		else errorCycles = 0;

		if(errorCycles > 2){
			haltArduino();
			FERR("Communication with Arduino failed 3 times in a row");
		}
	}
}