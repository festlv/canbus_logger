import serial
import datetime
import time

import os
# chose an implementation, depending on os
if os.name == 'nt': #sys.platform == 'win32':
    from serial.tools.list_ports_windows import *
elif os.name == 'posix':
    from serial.tools.list_ports_posix import *

class SerialException(serial.SerialException):
	pass


class CANMessage(object):

	def __init__(self,id, data, timestamp=None):
		self.id = id
		self.data = data
		self.timestamp = timestamp or datetime.datetime.now()



class SerialInterface(object):
	"""Serial interface to CAN bus logger. Handles communication protocol and processes incoming data frames
	and formats them to look pythonic"""

	supported_can_bitrates = [10, 20, 50, 100, 125, 250, 500, 1000]

	def scan(self):
		"""Scans for available ports. Returns a list of device names."""
		ports = []

		for i, desc, hwid in comports():
			try:
				s = serial.Serial(i)
				ports.append(s.portstr)
				s.close()
			except serial.SerialException:
				pass
		return ports

	def connect(self, serial_port, can_bitrate):
		"""Connects to MCU via serial port, sends bitrate selection packet"""
		
		self.serial = serial.Serial(
			port=str(serial_port),
			baudrate=250000,
			timeout=1
		)
		try:
			self.serial.setDTR(True)
			time.sleep(0.2)
			self.serial.setDTR(False)
			time.sleep(2)
			self.serial.write(chr(self.supported_can_bitrates.index(int(can_bitrate))))
		except ValueError:
			raise SerialException("CAN bitrate not supported!")

	def read_message(self):
		line = self.serial.readline(32)
		if len(line)>0 and line[0]!='~':
			print line
		if len(line)>0 and line[0]=='~' and line[-2]=='.':
			#we have full line, decode it
			try:
				data = ''
				for i in range(1,9):
					data += hex(ord(line[i]))[2:]
				data = data.upper()
				identifier = ''
				for i in range(9,13):
					identifier+=hex(ord(line[i]))[2:]
				identifier = identifier.upper()
				return CANMessage(identifier, data)
			except IndexError:
				print "Index error on line: %s" % line

		return None

	def disconnect(self):
		self.serial.close()
		self.serial = None

