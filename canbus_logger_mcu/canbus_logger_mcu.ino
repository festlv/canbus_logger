/*
Canbus logger
 
  Arduino pin   MCP2515 pin (DIP18)
  2				reset, 17

  SPI interface
  10 			cs, 16
  11 			si, 14
  12 			so, 15
  13			sck, 13

 */

#include "CAN.h"

const int supported_bit_rates[8] = {10, 20, 50, 100, 125, 250, 500, 1000};
unsigned long next_delay = 0;


//#define LOOPBACK_MODE


/* 
* 	Waits for initial baud rate setting by PC
*/
void setup()  
{
	Serial.begin(500000);

	int baudConfig = 125;
	byte incoming = 0;

	while (true) {
		if (Serial.available()>0) {
			incoming = Serial.read();
			if (incoming >=0 && incoming<8) {
				baudConfig = supported_bit_rates[incoming];
				Serial.write("Baud rate is set: ");
				Serial.println(baudConfig);
				break;
			} else {
				Serial.println("Unsupported baud rate!");
			}
		}
	}


	CAN.begin();
	CAN.setMode(CONFIGURATION);
	CAN.baudConfig(baudConfig);
	CAN.toggleRxBuffer0Acceptance(true, true);
	CAN.toggleRxBuffer1Acceptance(true, true);
	#ifdef LOOPBACK_MODE
		CAN.setMode(LOOPBACK);
	#else
		CAN.setMode(LISTEN);
	#endif
	delay(100);
	next_delay = millis() + 6000;
}


inline void send_data(byte *data_out, uint32_t id_out) {
	Serial.write("~");
	for (int i=0;i<8;i++) {
        Serial.write(data_out[i]);
    }
    unsigned char *id = (unsigned char*)&id_out;
    for (int i=3;i>=0;i--)
    	Serial.write(id[i]);

	Serial.write(".\n");
}

void loop() // run over and over
{	
	byte length,rx_status,filter,ext;
	long unsigned int frame_id;
	byte frame_data[8];

	rx_status = CAN.readStatus();

	if ((rx_status & 0x40) == 0x40) {
		//clear receive buffers, just in case.
		frame_data[0] = 0x00;
		frame_data[1] = 0x00;
		frame_data[2] = 0x00;
		frame_data[3] = 0x00;
		frame_data[4] = 0x00;
		frame_data[5] = 0x00;
		frame_data[6] = 0x00;
		frame_data[7] = 0x00;

		frame_id = 0x0000;

		length = 0;
		CAN.readDATA_ff_0(&length,frame_data,&frame_id, &ext, &filter);
		send_data(frame_data, frame_id);
		CAN.clearRX0Status();
	}

	if ((rx_status & 0x80) == 0x80) {
		//clear receive buffers, just in case.
		frame_data[0] = 0x00;
		frame_data[1] = 0x00;
		frame_data[2] = 0x00;
		frame_data[3] = 0x00;
		frame_data[4] = 0x00;
		frame_data[5] = 0x00;
		frame_data[6] = 0x00;
		frame_data[7] = 0x00;

		frame_id = 0x0000;

		length = 0;
		CAN.readDATA_ff_1(&length,frame_data,&frame_id, &ext, &filter);
		send_data(frame_data, frame_id);      
		CAN.clearRX1Status();
	}
	#ifdef LOOPBACK_MODE
		if (next_delay<millis()) {
			frame_data[0] = 0x55;
			frame_data[1] = 0xA4;
			frame_data[2] = 0x04;
			frame_data[3] = 0xF4;
			frame_data[4] = 0x14;
			frame_data[5] = 0xC4;
			frame_data[6] = 0xFC;
			frame_data[7] = 0x1D;


			frame_id = 0x123000;
			length = 8;
			CAN.load_ff_0(length,&frame_id,frame_data, true);
	      	delay(10);    

	      
			next_delay = millis()+6000;
		}
	#endif
}

