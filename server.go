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
	err = errors.New("Something went wrong")
	return
}

func getData() (data []byte, err error) {
	data = make([]byte, dataPacketLength)
	err = nil
	dt := time.Now()
	data[0] = uint8(dt.Hour())
	data[1] = uint8(dt.Minute())
	data[2] = uint8(dt.Day())
	data[3] = uint8(dt.Month())
	// IMPORTANT: This sets the interval between cycles
	cpuU, u := cpu.Percent(time.Second, false)
	if u != nil {
		err = u
	}
	data[4] = uint8(cpuU[0])
	vMem, u := mem.VirtualMemory()
	if u != nil {
		err = u
	}
	data[5] = uint8(vMem.UsedPercent)
	return
}

func getSerialPort() (port serial.Port, err error) {
	arduinoPort, err := findArduino()
	if err != nil {
		arduinoPort = nil
		return
	}
	mode := &serial.Mode{
		BaudRate: 9600,
	}
	port, err = serial.Open(arduinoPort.Name, mode)
	if err != nil {
		arduinoPort = nil
		return
	}
	return
}

func main() {
	port, err := getSerialPort()
	if err != nil {
		log.Fatal(err)
	}
	time.Sleep(5 * time.Second)
	errCounter := uint8(0)
	for {
		data, err := getData()
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
		// cpu.Percent is used as a timer in getData()
		// time.Sleep(time.Second)
	}
}
