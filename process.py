import multiprocessing
import wx
import serial_interface
import Queue

class SerialProcess(multiprocessing.Process):
	def __init__(self, serial_port, bitrate, queue):
		multiprocessing.Process.__init__(self)
		self._queue = queue
		self._abort_pending = False
		self._connected = False
		self._serial_port = serial_port
		self._bitrate = bitrate
		self.start()

	def run(self):
		self._serial = serial_interface.SerialInterface()
		self._serial.connect(self._serial_port, self._bitrate)
		while True:
			try:
				stop = self._queue.get(False)
				if stop=="stop":
					break
			except Queue.Empty:
				pass
			
			can_message = self._serial.read_message()
			if can_message:
				self._queue.put(can_message)




