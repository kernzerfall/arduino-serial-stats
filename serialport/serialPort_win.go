// +build windows

package serialport

import (
	"errors"
	"fmt"
	"log"
	"strings"

	"go.bug.st/serial"
	"go.bug.st/serial/enumerator"
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

// GetSerialPort opens a serial port and returs it as serialObj
func GetSerialPort() (serialObj serial.Port, err error) {
	arduino, err := findArduino()
	if err != nil {
		log.Fatal(err)
	}
	mode := &serial.Mode{
		BaudRate: 9600,
	}
	serialObj, err = serial.Open(arduino.Name, mode)
	if err != nil {
		serialObj = nil
		return
	}
	return
}
