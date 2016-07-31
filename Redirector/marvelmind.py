#!/usr/bin/env python
#
# marvelmind.py - small class for recieve and parse coordinates from Marvelmind mobile beacon by USB/serial port
# Written by Alexander Rudykh (awesomequality@gmail.com)
#
### Attributes:
#
#	tty - serial port device name (physical or USB/virtual). It should be provided as an argument: 
#   	/dev/ttyACM0 - typical for Linux / Raspberry Pi
#   	/dev/tty.usbmodem1451 - typical for Mac OS X
#
#	baud - baudrate. Should be match to baudrate of hedgehog-beacon
#		default: 9600
#
#	maxvaluescount - maximum count of measurements of coordinates stored in buffer
#		default: 3
#
#	lastValues - buffer of measurements
#
#	debug - debug flag which activate console output	
#		default: False
#
#	pause - pause flag. If True, class would not read serial data
#
#	terminationRequired - If True, thread would exit from main loop and stop
#
#
### Methods:
#
#	__init__ (self, tty="/dev/ttyACM0", baud=9600, maxvaluescount=3, debug=False) 
#		constructor
#
#	print_position(self)
#		print last measured data in default format
#
#	position(self)
#		return last measured data in array [x, y, z, timestamp]
#
#	stop(self)
#		stop infinite loop and close port
#
### Needed libraries:
#
# To prevent errors when installing crcmod module used in this script, use the following sequence of commands:
#   sudo apt-get install python-pip
#   sudo apt-get update
#   sudo apt-get install python-dev
#   sudo pip install crcmod
#
###

import crcmod
import serial
import struct
import collections
from time import sleep
from threading import Thread


class MarvelmindHedge(Thread):
    def __init__(self, tty="/dev/ttyACM0", baud=9600, maxvaluescount=3, recieveDataCallback=None):
        self.tty = tty  # serial
        self.baud = baud  # baudrate
        self._bufferSerialDeque = collections.deque(maxlen=255)  # serial buffer
        self.lastValues = collections.deque(maxlen=maxvaluescount)  # meas. buffer
        self.error = "NO_ERROR"
        self.recieveDataCallback = recieveDataCallback

        for x in range(0, 10):
            self.lastValues.append([0, 0, 0, 0])  # last measured positions and timestamps; [x, y, z, timestamp]
        self.terminationRequired = False

        self.serialPort = None

        Thread.__init__(self)

    def print_position(self):
        print "X: {:d}, Y: {:d}, Z: {:d} at time T: {:.2f}".format(self.position()[0], self.position()[1],
                                                                   self.position()[2], self.position()[3] / 64.0)

    def position(self):
        return list(self.lastValues)[-1]

    def stop(self):
        self.terminationRequired = True
        print ("stopping")

    def getError(self):
        return self.error

    def run(self):
        while (not self.terminationRequired):
            try:
                if (self.serialPort is None):
                    self.error = "NO_DEVICE"
                    self.serialPort = serial.Serial(self.tty, self.baud, timeout=3)
                readChar = self.serialPort.read(1)
                while (readChar is not None) and (readChar is not '') and (not self.terminationRequired):
                    self._bufferSerialDeque.append(readChar)
                    readChar = self.serialPort.read(1)
                    bufferList = list(self._bufferSerialDeque)
                    strbuf = ''.join(bufferList)
                    pktHdrOffset = strbuf.find('\xff\x47\x01\x00\x10')
                    if (pktHdrOffset >= 0 and len(bufferList) > pktHdrOffset + 4 and pktHdrOffset < 220):
                        # print(bufferList)
                        msgLen = ord(bufferList[pktHdrOffset + 4])

                        # offset+header+message+crc
                        # print 'len of buffer: ', len(bufferList), '; len of msg: ', msgLen, '; offset: ', pktHdrOffset
                        if (len(bufferList) > pktHdrOffset + 4 + msgLen + 2):
                            usnTimestamp, usnX, usnY, usnZ, usnCRC16 = struct.unpack_from('<LhhhxxxxxxH', strbuf,
                                                                                          pktHdrOffset + 5)
                            crc16 = crcmod.predefined.Crc('modbus')
                            crc16.update(strbuf[pktHdrOffset: pktHdrOffset + msgLen + 5])
                            CRC_calc = int(crc16.hexdigest(), 16)

                            if CRC_calc == usnCRC16:
                                value = [usnX, usnY, usnZ, usnTimestamp]
                                self.lastValues.append(value)
                                self.error = "NO_ERROR"
                                if (self.recieveDataCallback is not None):
                                    self.recieveDataCallback()
                            else:
                                self.error = "CRC ERROR"

                            if pktHdrOffset == -1:
                                self.error = "ERROR: Marvelmind USNAV beacon packet header not found (check modem board or radio link)"
                                continue
                                # elif pktHdrOffset >= 0:
                                # self.error = "Found USNAV beacon packet header at offset %d" % pktHdrOffset
                            for x in range(0, pktHdrOffset + msgLen + 7):
                                self._bufferSerialDeque.popleft()

            except OSError:
                self.error = "ERROR: OS error (possibly serial port is not available)"
                sleep(1)
            except serial.SerialException:
                self.error = "ERROR: serial port error (possibly beacon is reset, powered down or in sleep mode). Restarting reading process..."
                self.serialPort = None
                sleep(1)

        if (self.serialPort is not None):
            self.serialPort.close()
        self.error = "USB not connected"
