#include<iostream>
#include<ctime>
#include<vector>
#include<string>
#include<signal.h>

// Shared headers
#include"datatypes.hpp"
#include"serialconstant.hpp"

// External libraries
#include"resources.hpp"
#include"rs232.h"

#ifdef __WIN32 
#define SLEEP(x) Sleep(x)
#else
#define SLEEP(x) usleep(x/1e+3)
#endif

// Dump a vector<byte>'s contents if dbgprint is set
#define DDUMP(x) if(dbgprint) printData(x)
// Called when an error & exit must happen
#define FATAL(x) { std::cout<<x<<std::endl; exit(EXIT_FAILURE); }

// WIP: Define some own exceptions
namespace Except {
	class BufferOverflow 	: public std::exception {};
	class MalformedPacket	: public std::exception {};
};


// Global var dbgprint, set to true if ARD_DBG found in env
// More info in main
bool dbgprint = false;

// Global var to store the Arduino's port number
int arduinoPort = -1;


// Does what it says
auto buildTimePacket() -> std::vector<byte> {
	auto t = std::time(0);
	std::tm* now = std::localtime(&t);
	return std::vector<byte> {
		// Set Header flags
		// These tell the Arduino where the packet starts and what it contains
		SerialConstant::Flag::DATA_START,
		SerialConstant::Data::TYPE_DATETIME,
		// TimePackets' body should always be in this form
		//		tp[2] = HOUR
		//		  [3] = MIN
		//		  [4] = DAY
		//		  [5] = MONTH
		// Arduino can only parse this exact body format (for now?)
		(byte)now->tm_hour,
		(byte)now->tm_min,
		(byte)now->tm_mday,
		(byte)now->tm_mon,
		// Set packet end flag
		SerialConstant::Flag::DATA_END
	};
}

// Does what it says
auto buildCPUtilPacket() -> std::vector<byte> {
	// Note: this blocks the thread for 1 second
	byte cputil = Resources::getCPUtil();
	return std::vector<byte> {
		// Set Header flags
		// These tell the Arduino where the packet starts and what it contains
		SerialConstant::Flag::DATA_START,
		SerialConstant::Data::TYPE_CPUTIL,
		// This should only contain the CPU Utilization in (0-100)%
		cputil,
		// Set packet end flag
		SerialConstant::Flag::DATA_END
	};
}

auto buildRAMUtilPacket() -> std::vector<byte> {
	byte ramutil = Resources::getRAMUtil();
	return std::vector<byte> {
		// Set Header flags
		// These tell the Arduino where the packet starts and what it contains
		SerialConstant::Flag::DATA_START,
		SerialConstant::Data::TYPE_RAMUTIL,
		// This should only contain the RAM Utilization in (0-100)%
		ramutil,
		// Set packet end flag
		SerialConstant::Flag::DATA_END
	};
}

auto printData(std::vector<byte> data, std::string prepend = ""){
	std::cout << prepend << ((prepend.size())!=0?"\x20":"") << "[\x20";
	for(byte c: data)
		printf("0x%02x, ", c);
	std::cout<<"\b\b\x20]\n";
}

auto sendData(std::vector<byte> data) -> int {
	if(arduinoPort == -1) return 1;
	const u64 s = data.size();

	// Check if the packet is bigger than the buffer size
	if(s < 4 || s > SerialConstant::Data::SIZE_SERIALBUF + 3)
		throw new Except::BufferOverflow();

	// Check if packet contains start and end flags
	// and if it has something that looks like a datatype
	if( 
		data.at(0) != SerialConstant::Flag::DATA_START ||
		!(data.at(1) & SerialConstant::Flag::NAMESPACE)|| // Probably room for improvement here
		data.back() != SerialConstant::Flag::DATA_END 
	) throw new Except::MalformedPacket();

	// Debug print
	DDUMP(data);

	// Keep track of errors here
	int err = 0;
	for(byte b: data)
		// For each sendByte that fails, increment the err counter
		err += RS232_SendByte(arduinoPort, b);
	// Return the number of errors that occured
	return err;
}

// haltArduino resets the Board
void haltArduino(){
	if(arduinoPort != -1){ 
		RS232_SendByte(arduinoPort, SerialConstant::State::HALT);
		RS232_CloseComport(arduinoPort);
	}
}

// This handles SIGINT, SIGTERM, etc.
void signalHandler(int signum){
	// Reset the board
	haltArduino();
	// CERR the sig received
	std::cerr << "Received interrupt signal " << signum << std::endl;
	// Exit with the sig
	exit(signum);
}



auto main(int argc, char** argv) -> int {
	// Handle some SIGs
 	// SIGINT  = 2
	// SIGQUIT = 3 (Not produced under Windows NT)
	// SIGKILL = 9 (Not produced under Windows NT)
	// SIGTERM = 15
	// SIGABRT = 22
	for(int i: std::vector<int>{ 2, 3, 9, 15, 22 })
		signal(i, signalHandler);

	// Program must have only 1 argument
	// The name of the COMPort to connect to	
	if(argc != 2) FATAL("Incorrect argc")

	// Try to find the port from argv[1]
	arduinoPort = RS232_GetPortnr(argv[1]);
	if(arduinoPort == -1) FATAL("Port not found");

	// Try to open the COMPort
	if(RS232_OpenComport(arduinoPort, 9600, "8N1", 0)) 
		FATAL("Couldn't open serial port");

	// If ARD_DBG is set in the environment,
	// enable debug output (buffer printing)
	dbgprint = !(std::getenv("ARD_DBG") == NULL);

	// Sleep to let the Arduino fully boot up
	SLEEP(7000);
	
	// Keep track of errors here
	// If this gets to 3, the program will exit
	u64 errorCycles = 0;

	// Infinite loop to send data to Arduino
	while(true){
		// sBuf is created on the start of each cycle;
		// No need to empty it in the end
		std::vector<byte> sBuf;

		// Try to build the packets
		try{
			for(byte b: buildTimePacket()) sBuf.push_back(b);
			// buildCPUtilPacket hangs the thread for 1 second
			// Taken advantage of in order to time the loop
			for(byte b: buildCPUtilPacket()) sBuf.push_back(b);
			for(byte b: buildRAMUtilPacket()) sBuf.push_back(b);
		} catch(std::exception e){
			std::cerr << "Failed to build data packets\n";
			errorCycles++;
			continue;
		}
		
		// If sBuf can't be sent, increment errorCycles
		if(sendData(sBuf)) errorCycles++;
		// Else reset it
		else errorCycles = 0;

		// If something fails 3 times, exit
		if(errorCycles > 2){
			haltArduino();
			FATAL("Communication with Arduino failed 3 times in a row");
		}
	}

	// We should never get here if everything functions correctly
	return EXIT_FAILURE;

}