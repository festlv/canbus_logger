#include <SPI.h>
#include <SoftwareSerial.h>

/*
Created by: Kyle Crockett
For canduino with 16MHz oscillator.
CNFx register values.
use preprocessor command "_XXkbps" 
"XX" is the baud rate.

10 kbps
CNF1/BRGCON1    b'00110001'     0x31
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

20 kbps
CNF1/BRGCON1    b'00011000'     0x18
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

50 kbps
CNF1/BRGCON1    b'00001001'     0x09
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

100 kbps
CNF1/BRGCON1    b'00000100'     0x04
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

125 kbps
CNF1/BRGCON1    b'00000011'     0x03
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

250 kbps
CNF1/BRGCON1    b'00000001'     0x01
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

500 kbps
CNF1/BRGCON1    b'00000000'     0x00
CNF2/BRGCON2    b'10111000'     0xB8
CNF3/BRGCON3    b'00000101'     0x05

800 kbps
Not yet supported

1000 kbps
Settings added by Patrick Cruce(pcruce_at_igpp.ucla.edu)
CNF1=b'10000000'=0x80 = SJW = 3 Tq. & BRP = 0
CNF2=b'10010000'=0x90 = BLTMode = 1 & SAM = 0 & PS1 = 3 & PR = 1
CNF3=b'00000010'=0x02 = SOF = 0  & WAKFIL = 0 & PS2 = 3

*/
#ifndef can_h
#define can_h


#if  defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
  #define SCK_PIN   13
  #define MISO_PIN  12
  #define MOSI_PIN  11
  #define SS_PIN    10
#endif

#if defined(__AVR_ATmega128__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2650__)
  #define SCK_PIN  52
  #define MISO_PIN 50
  #define MOSI_PIN 51
  #define SS_PIN   53
#endif

#define RESET_PIN 8
#define INT_PIN 3

#define RESET_REG 0xc0  
#define READ 0x03 
#define WRITE 0x02 //read and write comands for SPI

#define READ_RX_BUF_0_ID 0x90
#define READ_RX_BUF_0_DATA 0x92
#define READ_RX_BUF_1_ID 0x94
#define READ_RX_BUF_1_DATA 0x96 //SPI commands for reading CAN RX buffers

#define LOAD_TX_BUF_0_ID 0x40
#define LOAD_TX_BUF_0_DATA 0x41
#define LOAD_TX_BUF_1_ID 0x42
#define LOAD_TX_BUF_1_DATA 0x43
#define LOAD_TX_BUF_2_ID 0x44
#define LOAD_TX_BUF_2_DATA 0x45 //SPI commands for loading CAN TX buffers

#define SEND_TX_BUF_0 0x81
#define SEND_TX_BUF_1 0x82
#define SEND_TX_BUF_2 0x83 //SPI commands for transmitting CAN TX buffers

#define READ_STATUS 0xA0
#define RX_STATUS 0xB0
#define BIT_MODIFY 0x05 //Other commands


//Registers
#define CNF0 0x2A
#define CNF1 0x29
#define CNF2 0x28
#define TXB0CTRL 0x30 
#define TXB1CTRL 0x40
#define TXB2CTRL 0x50 //TRANSMIT BUFFER CONTROL REGISTER
#define RXB0CTRL 0x60
#define RXB1CTRL 0x70
#define TXB0DLC 0x35 //Data length code registers
#define TXB1DLC 0x45
#define TXB2DLC 0x55
#define CANCTRL 0x0F //Mode control register
#define CANSTAT 0x0E //Mode status register
#define CANINTF 0x2C //Mode status register
#define CANINTE 0x2B //Mode status register
#define BFPCTRL 0x0c

#define MASK_0 0x20
#define MASK_1 0x24
#define FILTER_0 0x00
#define FILTER_1 0x04
#define FILTER_2 0x08
#define FILTER_3 0x10
#define FILTER_4 0x14
#define FILTER_5 0x18

//#include "WProgram.h"

enum CANMode {CONFIGURATION,NORMAL,SLEEP,LISTEN,LOOPBACK};

class CANClass
{
private:

public:
	static void begin();//sets up MCP2515
	static void baudConfig(int bitRate);//sets up baud

	//Method added to enable testing in loopback mode.(pcruce_at_igpp.ucla.edu)
	static void setMode(CANMode mode) ;//put CAN controller in one of five modes

	//extending CAN data read to full frames(pcruce_at_igpp.ucla.edu)
	//data_out should be array of 8-bytes or frame length.
	static void readRXBuffer(byte buffer, byte* length_out, byte *data_out, uint32_t *id_out, byte *ext, byte *filter);
	static void readDATA_ff_0(byte* length_out, byte *data_out, uint32_t *id_out, byte *ext, byte *filter);
	static void readDATA_ff_1(byte* length_out, byte *data_out, uint32_t *id_out, byte *ext, byte *filter);

	//Adding can to read status register(pcruce_at_igpp.ucla.edu)
	//can be used to determine whether a frame was received.
	//(readStatus() & 0x80) == 0x80 means frame in buffer 0
	//(readStatus() & 0x40) == 0x40 means frame in buffer 1
	static byte readStatus();

	//extending CAN write to full frame(pcruce_at_igpp.ucla.edu)
	//Identifier should be a value between 0 and 2^11-1, longer identifiers will be truncated(ie does not support extended frames)
	static void load_ff(byte txbuf, byte length, uint32_t *id, byte *data, bool ext, byte send_cmd);
	static void load_ff_0(byte length, uint32_t *id, byte *data, bool ext);
	static void load_ff_1(byte length, uint32_t *id, byte *data, bool ext);
	static void load_ff_2(byte length, uint32_t *id, byte *data, bool ext);
        
	static void setMaskOrFilter(byte mask, byte b0, byte b1, byte b2, byte b3);
	static void setRegister(byte reg, byte value);
	static byte getFilterHit(byte rxbuf);
	
	static void resetFiltersAndMasks();
	
	static void toggleRxAcceptance(byte buffer, bool std, bool ext);
	static void toggleRxBuffer0Acceptance(bool std, bool ext);
	static void toggleRxBuffer1Acceptance(bool std, bool ext);
	
	static void clearRX0Status();
	static void clearRX1Status();
	
	static bool buffer0DataWaiting();
	static bool buffer1DataWaiting();

	static bool isInitialized();
	static bool interruptStatus();
};
extern CANClass CAN;
#endif
