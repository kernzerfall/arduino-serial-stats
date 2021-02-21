package main

import (
	"fmt"
	"log"
	"time"

	"./serialport"
	"github.com/shirou/gopsutil/v3/cpu"
	"github.com/shirou/gopsutil/v3/mem"
)

const (
	dataPacketLength = 6
	dataStart        = 0b10000001
	dataEnd          = 0b11111111
	dataDateTime     = 0b10010000
	dataRAMUtil      = 0b10010010
	dataCPUtil       = 0b10010001
)

// buildDataPacket gets the current date & time, and queries psutil for the computer's stats
func buildDataPacket() (data []byte, err error) {
	// Data format = [hour, minute, day, month, cpu%, gpu%]
	data = make([]byte, 15)
	err = nil
	dt := time.Now()
	data[0] = dataStart
	data[1] = dataDateTime
	data[2] = uint8(dt.Hour())
	data[3] = uint8(dt.Minute())
	data[4] = uint8(dt.Day())
	data[5] = uint8(dt.Month())
	data[6] = dataEnd
	data[7] = dataStart
	data[8] = dataCPUtil
	// IMPORTANT: This sets the interval between cycles
	// get CPU Utilization %
	cpuU, u := cpu.Percent(time.Second, false) // percpu is set to false; we only get a single value
	if u != nil {
		err = u
	}
	// from cpuU, get the first value
	data[9] = uint8(cpuU[0])

	data[10] = dataEnd
	data[11] = dataStart
	data[12] = dataRAMUtil

	// get RAM Utilization stats
	vMem, u := mem.VirtualMemory()
	if u != nil {
		err = u
	}
	// We care only about used ram %
	data[13] = uint8(vMem.UsedPercent)
	data[14] = dataEnd
	return
}

func main() {
	port, err := serialport.GetSerialPort()
	if err != nil {
		log.Fatal(err)
	}
	defer port.Close()
	time.Sleep(5 * time.Second)
	errCounter := uint8(0)
	for {
		data, err := buildDataPacket()
		if err != nil {
			log.Fatal("Couldn't get System Data")
		}
		fmt.Println(data)
		_, err = port.Write(data)
		if err != nil {
			fmt.Println("Couldn't send message to Arduino")
			errCounter++
		} else {
			errCounter = 0
			//res := make([]byte, 512)
			//port.Read(res)
			//fmt.Println(string(res))

		}
		if errCounter > 3 {
			log.Fatal("Communication with Arduino failed 3 times in a row")
		}
		// cpu.Percent is used as a timer in buildDataPacket()
		// time.Sleep(time.Second)
	}
}
