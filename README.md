CANBus logger using Arduino and MCP2515/MCP2551.

Hardware
===========
* Arduino Duemilanove
* MCP2551 CAN transceiver
* MCP2515 CAN controller

I used schematics based on SparkFun's CANBus shield: http://www.sparkfun.com/datasheets/DevTools/Arduino/canbus_shield-v12.pdf

Software
==========
* CAN library from Steven (http://modelrail.otenko.com/arduino/arduino-controller-area-network-can)
* Simple Arduino program which sends received CAN messages via serial port @ 115200 baud to PC
* GUI in Python which handles selection of CANBus baud rate and receiving messages.

GUI requirements
=================
* Python 2.7
* pyserial
* wxpython

Hardware connections
======================
  Arduino pin   MCP2515 pin (DIP18)
  2				reset, 17

  SPI interface
  10 			cs, 16
  11 			si, 14
  12 			so, 15
  13			sck, 13




Usage
=============
* Connect hardware
* Upload Arduino program. I use scons build system, so I don't know (nor care) if it compiles in brain-dead Arduino IDE.
* Launch GUI by running `python logger.py`. Select serial port, CAN bitrate and connect.



Credits
==========
* SparkFun for their CANBus shield schematics
* Steven for his thorough description of CANBus usage with Arduino (http://modelrail.otenko.com/arduino/arduino-controller-area-network-can)
