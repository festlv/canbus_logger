import multiprocessing
import wx
import serial_interface
import Queue

class SerialProcess(multiprocessing.Process):
	def __init__(self, window, serial_port, bitrate, queue):
		multiprocessing.Process.__init__(self)
		self._notify_window = window
		self._queue = queue
		self._abort_pending = False
		self._serial = serial_interface.SerialInterface()
		self._serial.connect(serial_port, bitrate)
		self.start()

	def run(self):
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




