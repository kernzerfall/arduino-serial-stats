import time
from serial import Serial
import psutil
import datetime
import os

data = [
    0x00, # Hour
    0x00, # Minute
    0x00, # Day
    0x00, # Month
    0x00, # CPU%
    0x00  # RAM%
]

ser = None

def getSerialDevice() -> Serial:
    try:
        if os.name == 'nt':
            return Serial('COM4',9600)
        else:
            return Serial('/dev/ttyACM0', 9600)
    except:
        print("serial nf")
        return None


def loop(ser) -> None:
    while True:
        try:
            u = datetime.datetime.now()
            data[0] = u.hour
            data[1] = u.minute
            data[2] = u.day
            data[3] = u.month
            data[4] = int(psutil.cpu_percent())
            data[5] = int(psutil.virtual_memory().percent)
            ser.write(bytes(data))
            print(data)
            time.sleep(1)
        except:
            ser = getSerialDevice()
            if ser!=None:
                loop(ser)
                break
            else:
                break
    quit(0)

if __name__ == "__main__":
    ser = getSerialDevice()
    if ser!=None:
        time.sleep(5)
        loop(ser)
    else:
        quit(0)