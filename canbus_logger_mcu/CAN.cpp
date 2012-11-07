#include <SPI.h>
#include <SoftwareSerial.h>
#include "CAN.h"

void CANClass::begin()//constructor for initializing can module.
{
	// set the slaveSelectPin as an output 
	pinMode (SCK_PIN,OUTPUT);
	pinMode (MISO_PIN,INPUT);
	pinMode (MOSI_PIN, OUTPUT);
	pinMode (SS_PIN, OUTPUT);
	pinMode(RESET_PIN,OUTPUT);
	pinMode(INT_PIN,INPUT);
	digitalWrite(INT_PIN,HIGH);

	// initialize SPI:
	SPI.begin(); 
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);
	SPI.setBitOrder(MSBFIRST);

	digitalWrite(RESET_PIN,LOW); /* RESET CAN CONTROLLER*/
	delay(10);
	digitalWrite(RESET_PIN,HIGH);
	delay(100);
	
	resetFiltersAndMasks();
}

void CANClass::baudConfig(int bitRate)//sets bitrate for CAN node
{
	byte config0 = 0x00,config1 = 0xB8,config2 = 0x05;

	switch (bitRate)
	{
case 10:
		config0 = 0x31;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 20:
		config0 = 0x18;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 50:
		config0 = 0x09;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 100:
		config0 = 0x04;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 125:
		config0 = 0x03;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 250:
		config0 = 0x01;
		config1 = 0xB8;
		config2 = 0x05;
		break;

case 500:
		config0 = 0x00;
		config1 = 0xB8;
		config2 = 0x05;
		break;
case 1000:
		//1 megabit mode added by Patrick Cruce(pcruce_at_igpp.ucla.edu)
		//Faster communications enabled by shortening bit timing phases(3 Tq. PS1 & 3 Tq. PS2) Note that this may exacerbate errors due to synchronization or arbitration.
		config0 = 0x80;
		config1 = 0x90;
		config2 = 0x02;
	}
	digitalWrite(SS, LOW);
	delay(10);
	SPI.transfer(WRITE);
	SPI.transfer(CNF0);
	SPI.transfer(config0);
	delay(10);
	digitalWrite(SS, HIGH);
	delay(10);

	digitalWrite(SS, LOW);
	delay(10);
	SPI.transfer(WRITE);
	SPI.transfer(CNF1);
	SPI.transfer(config1);
	delay(10);
	digitalWrite(SS, HIGH);
	delay(10);

	digitalWrite(SS, LOW);
	delay(10);
	SPI.transfer(WRITE);
	SPI.transfer(CNF2);
	SPI.transfer(config2);
	delay(10);
	digitalWrite(SS, HIGH);
	delay(10);

	//set output LEDs
	// see data sheet for explanation
	digitalWrite(SS,LOW);
	delay(10);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(BFPCTRL);
	SPI.transfer(0b1111);
	SPI.transfer(0b1111);
	delay(10);
	digitalWrite(SS,HIGH);
	delay(10);
}

void CANClass::setMaskOrFilter(byte mask, byte b0, byte b1, byte b2, byte b3) {
	setMode(CONFIGURATION);  
	setRegister(mask, b0);
	setRegister(mask+1, b1);
	setRegister(mask+2, b2);
	setRegister(mask+3, b3);
	setMode(NORMAL);
}

void CANClass::setRegister(byte reg, byte value) {
 	digitalWrite(SS_PIN, LOW);
	delay(10);
	SPI.transfer(WRITE);
	SPI.transfer(reg);
	SPI.transfer(value);
	delay(10);
	digitalWrite(SS_PIN, HIGH);
	delay(10);
}

//Method added to enable testing in loopback mode.(pcruce_at_igpp.ucla.edu)
void CANClass::setMode(CANMode mode) { //put CAN controller in one of five modes

	byte writeVal = 0x00,mask = 0xE0;

	switch(mode) {
  	case CONFIGURATION:
			writeVal = 0x80;
			break;
  	case NORMAL:
		  writeVal = 0x00;
			break;
  	case SLEEP:
			writeVal = 0x20;
	  	break;
    case LISTEN:
			writeVal = 0x60;
	  	break;
  	case LOOPBACK:
			writeVal = 0x40;
	  	break;
   }

	digitalWrite(SS_PIN, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(CANCTRL);
	SPI.transfer(mask);
	SPI.transfer(writeVal);
	digitalWrite(SS_PIN, HIGH);

}

//extending CAN data read to full frames(pcruce_at_igpp.ucla.edu)
//It is the responsibility of the user to allocate memory for output.
//If you don't know what length the bus frames will be, data_out should be 8-bytes
void CANClass::readDATA_ff_0(byte* length_out, byte *data_out, uint32_t *id_out, byte *ext, byte *filter){
	readRXBuffer(READ_RX_BUF_0_ID, length_out, data_out, id_out, ext, filter);
}

void CANClass::readDATA_ff_1(byte* length_out, byte *data_out, uint32_t *id_out, byte *ext, byte *filter){
	readRXBuffer(READ_RX_BUF_1_ID, length_out, data_out, id_out, ext, filter);
}

void CANClass::readRXBuffer(byte buffer, byte* length_out, byte *data_out, uint32_t *id_out, byte *ext, byte *filter) {
	byte id_h,id_l,len,i,ed_8,ed_0;

	digitalWrite(SS_PIN, LOW);
	SPI.transfer(buffer);
	id_h = (unsigned short) SPI.transfer(0xFF); //id high
	id_l = (unsigned short) SPI.transfer(0xFF); //id low
    ed_8 = (unsigned short) SPI.transfer(0xFF); //extended id high(unused)
	ed_0 = (unsigned short) SPI.transfer(0xFF); //extended id low(unused)
	len = (SPI.transfer(0xFF) & 0x0F); //data length code
	for (i = 0;i<len;i++) {
		data_out[i] = SPI.transfer(0xFF);
	}
	digitalWrite(SS_PIN, HIGH);

	(*length_out) = len;
	if ((id_l & 0x08) == 0x08)  {
		*((uint16_t *) id_out + 1)  = (uint16_t) id_h << 5;
		*((uint8_t *)  id_out + 1)  = ed_8;
		*((uint8_t *)  id_out + 2) |= (id_l >> 3) & 0x1C;
		*((uint8_t *)  id_out + 2) |=  id_l & 0x03;
		*((uint8_t *)  id_out)      = ed_0;
	(*ext) = 1;
		//(*id_out) = (id_h << 21);// + ((id_l & 0xe0) << 13) + ((id_l & 0x03) << 16) + (ed_8 << 8) + ed_0;
	} else {
		(*id_out) = ((id_h << 3) + ((id_l & 0xE0) >> 5)); //repack identifier
		(*ext) = 0;
	}
	(*filter) = getFilterHit(buffer);
}

	//Adding method to read status register
	//can be used to determine whether a frame was received.
	//(readStatus() & 0x80) == 0x80 means frame in buffer 0
	//(readStatus() & 0x40) == 0x40 means frame in buffer 1
byte CANClass::readStatus() 
{
	byte retVal;
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(RX_STATUS);
	retVal = SPI.transfer(0xFF);
	SPI.transfer(0xFF);
	digitalWrite(SS_PIN, HIGH);
	return retVal;
}

bool CANClass::isInitialized() {
	byte retVal;
	retVal=0;
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(READ);
	SPI.transfer(CANSTAT);
	retVal = SPI.transfer(0xff);
	Serial.println(retVal, HEX);
	digitalWrite(SS_PIN, HIGH);
}
bool CANClass::interruptStatus() {
	byte retVal;
	retVal=0;
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(READ_STATUS);
	retVal = SPI.transfer(0xff);
	SPI.transfer(0xff);
	Serial.println(retVal, HEX);
	digitalWrite(SS_PIN, HIGH);
}

void CANClass::load_ff(byte txbuf, byte length, uint32_t *id, byte *data, bool ext, byte send_cmd)
{
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(txbuf);
	if (ext) {
		SPI.transfer(*((uint16_t *) id + 1) >> 5);
 		//uint8_t tmp  = ;
		SPI.transfer(((*((uint8_t *) id + 2) << 3) & 0xe0) | (1 << 3) | ((*((uint8_t *) id + 2)) & 0x03));
		SPI.transfer(*((uint8_t *) id + 1));
		SPI.transfer(*((uint8_t *) id));
	} else {
		SPI.transfer((byte) (*id >> 3)); //identifier high bits
		SPI.transfer((byte) ((*id << 5) & 0x00E0)); //identifier low bits
		SPI.transfer(0x00); //extended identifier registers(unused)
		SPI.transfer(0x00);
	}
	SPI.transfer(length);
	for (byte i=0;i<length;i++) { //load data buffer
		SPI.transfer(data[i]);
	}
	digitalWrite(SS_PIN, HIGH);
	delay(50);
	/*digitalWrite(SS_PIN, LOW);
	SPI.transfer(send_cmd);
	digitalWrite(SS_PIN, HIGH);
	*/
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(TXB0CTRL); //target
	SPI.transfer(0b1000);
	SPI.transfer(0xFF);
	digitalWrite(SS_PIN, HIGH);
}

void CANClass::load_ff_0(byte length, uint32_t *id, byte *data, bool ext) {
	load_ff(LOAD_TX_BUF_0_ID, length, id, data, ext, SEND_TX_BUF_0);
}

void CANClass::load_ff_1(byte length, uint32_t *id, byte *data, bool ext)
{
	load_ff(LOAD_TX_BUF_1_ID, length, id, data, ext, SEND_TX_BUF_1);
}

void CANClass::load_ff_2(byte length, uint32_t *id, byte *data, bool ext)
{
	load_ff(LOAD_TX_BUF_2_ID, length, id, data, ext, SEND_TX_BUF_2);
}

byte CANClass::getFilterHit(byte rxbuf) {
	byte retVal = 0;
	switch (rxbuf) { 
		case READ_RX_BUF_0_ID:
			digitalWrite(SS_PIN, LOW);
			SPI.transfer(READ);
			SPI.transfer(RXB0CTRL);
			retVal = SPI.transfer(0xff);
			digitalWrite(SS_PIN, HIGH);
			break;
		case READ_RX_BUF_1_ID:
			digitalWrite(SS_PIN, LOW);
			SPI.transfer(READ);
			SPI.transfer(RXB1CTRL);
			retVal = SPI.transfer(0xff);
			digitalWrite(SS_PIN, HIGH);
			break;
	}
	return retVal & 0x03;
}

void CANClass::resetFiltersAndMasks() {
	//disable first buffer
	setMaskOrFilter(MASK_0,   0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(FILTER_0, 0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(FILTER_1, 0b00000000, 0b00000000, 0b00000000, 0b00000000);

	//disable the second buffer
	setMaskOrFilter(MASK_1,   0b00000000, 0b00000000, 0b00000000, 0b00000000); 
	setMaskOrFilter(FILTER_2, 0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(FILTER_3, 0b00000000, 0b00000000, 0b00000000, 0b00000000); 
	setMaskOrFilter(FILTER_4, 0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(FILTER_5, 0b00000000, 0b00000000, 0b00000000, 0b00000000); 
}

void CANClass::toggleRxAcceptance(byte buffer, bool std, bool ext) {
	setMode(CONFIGURATION);
	delay(100);
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(buffer); //target
	SPI.transfer(0b01100000);
	SPI.transfer(((ext ? 1 : 0) << 6)|((std ? 1 : 0) << 5));
	digitalWrite(SS_PIN, HIGH);
	delay(100);
	setMode(LISTEN);
}

void CANClass::toggleRxBuffer0Acceptance(bool std, bool ext) {
	toggleRxAcceptance(RXB0CTRL, std, ext);
}

void CANClass::toggleRxBuffer1Acceptance(bool std, bool ext) {
	toggleRxAcceptance(RXB1CTRL, std, ext);
}

void CANClass::clearRX0Status() {
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(CANINTF); //target
	SPI.transfer(0b01);
	SPI.transfer(0x00);
	digitalWrite(SS_PIN, HIGH);
}

void CANClass::clearRX1Status() {
	digitalWrite(SS_PIN, LOW);
	SPI.transfer(BIT_MODIFY);
	SPI.transfer(CANINTF); //target
	SPI.transfer(0b10);
	SPI.transfer(0x00);
	digitalWrite(SS_PIN, HIGH);
}

bool CANClass::buffer0DataWaiting() {
	return ((CAN.readStatus() & 0x40) == 0x40);
}

bool CANClass::buffer1DataWaiting() {
	return ((CAN.readStatus() & 0x80) == 0x80);
}

