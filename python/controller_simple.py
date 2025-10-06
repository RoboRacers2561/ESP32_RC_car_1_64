#! /usr/bin/env python3

import serial
import time

MAX_BUFF_LEN = 255
SERIAL_PORT = "/dev/ttyUSB0"  # Change this to your serial port
SERIAL_BAUD = 115200

connected = False
port = None


def read_serial(num_char = 1):
    return port.read(num_char).decode()


def write_serial(cmd):
    port.write(cmd)


prev = time.time()
while (not connected):
    try:
        port = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)
    except:
        if (time.time() - prev > 2):
            print("Can't connect to serial port: {}".format(SERIAL_PORT))
            prev = time.time()
    if (port is not None):
        connected = True


while (True):
    user_input = input("Enter throttle,steering values (ex: 50,-10): ")

    if "," not in user_input:
        print("Invalid input: {}".format(user_input))
        continue

    command = user_input.encode()
    write_serial(command + b"\n")
    print("Sent command: {}".format(command))
    
    reply = read_serial(MAX_BUFF_LEN)
    if (len(reply) > 0):
        print(reply)