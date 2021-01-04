// build +unix

package serialport

import (
	"errors"
	"os"

	"go.bug.st/serial"
)

// TODO: Find a way to read the serial port's name under unix

// GetSerialPort opens a serial port and returs it as serialObj
func GetSerialPort() (serialObj serial.Port, err error) {
	_, er := os.Stat("/dev/ttyACM0")
	if !os.IsNotExist(er) {
		mode := &serial.Mode{
			BaudRate: 9600,
		}
		serialObj, err = serial.Open("/dev/ttyACM0", mode)
		if err != nil {
			serialObj = nil
			return
		}
	} else {
		err = errors.New("ttyACM0 doesn't exist")
	}
	return
}
