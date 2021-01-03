package main

import (
	"errors"
	"fmt"
	"log"
	"strings"
	"time"

	"github.com/shirou/gopsutil/cpu"
	"github.com/shirou/gopsutil/v3/mem"
	"go.bug.st/serial"
	"go.bug.st/serial/enumerator"
)

const (
	dataPacketLength = 6
)

// findArduino enumerates the serial ports, finds the first Arduino and returns its PortDetails
func findArduino() (arduino *enumerator.PortDetails, err error) {
	ports, err := enumerator.GetDetailedPortsList()
	if err != nil {
		arduino = nil
		return
	}
	if len(ports) == 0 {
		err = errors.New("No serial ports found")
		arduino = nil
		return
	}
	for _, port := range ports {
		if port.IsUSB && strings.Contains(port.Product, "Arduino") {
			fmt.Printf("Found %v\n", port.Product)
			arduino = port
			return
		}
	}
	arduino = nil
	err = errors.New("Something went wrong / Arduino not found")
	return
}

// buildDataPacket gets the current date & time, and queries psutil for the computer's stats
func buildDataPacket() (data []byte, err error) {
	// Data format = [hour, minute, day, month, cpu%, gpu%]
	data = make([]byte, dataPacketLength)
	err = nil
	dt := time.Now()
	data[0] = uint8(dt.Hour())
	data[1] = uint8(dt.Minute())
	data[2] = uint8(dt.Day())
	data[3] = uint8(dt.Month())
	// IMPORTANT: This sets the interval between cycles
	// get CPU Utilization %
	cpuU, u := cpu.Percent(time.Second, false) // percpu is set to false; we only get a single value
	if u != nil {
		err = u
	}
	// from cpuU, get the first value
	data[4] = uint8(cpuU[0])
	// get RAM Utilization stats
	vMem, u := mem.VirtualMemory()
	if u != nil {
		err = u
	}
	// We care only about used ram %
	data[5] = uint8(vMem.UsedPercent)
	return
}

// getSerialPort opens a serial port and returs it as serialObj
func getSerialPort(port *enumerator.PortDetails) (serialObj serial.Port, err error) {
	mode := &serial.Mode{
		BaudRate: 9600,
	}
	serialObj, err = serial.Open(port.Name, mode)
	if err != nil {
		serialObj = nil
		return
	}
	return
}

func main() {
	arduino, err := findArduino()
	if err != nil {
		log.Fatal(err)
	}
	port, err := getSerialPort(arduino)
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
		}
		if errCounter > 3 {
			log.Fatal("Communication with Arduino failed 3 times in a row")
		}
		// cpu.Percent is used as a timer in buildDataPacket()
		// time.Sleep(time.Second)
	}
}
