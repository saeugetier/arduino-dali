#include "Dali.h"

#define _delay_ms delay

Dali::Dali(uint8_t rx_pin, uint8_t tx_pin, uint8_t tx_en_pin) : extSerial(rx_pin,tx_pin)
{
  extSerial.setTransmitEnablePin(tx_en_pin);
  extSerial.begin(1200, 8, 16, true);
}

Dali::~Dali()
{

}

int Dali::sendDirect(address_mode mode, uint8_t address, uint8_t brightness)
{
  uint16_t frame;
  dali_direct_arc(&frame, mode, address, brightness);
  return this->send(frame);
}

int Dali::send(uint16_t frame)
{
	if(frame == INVALID_FRAME)
		return _ERR_INVALID_FRAME_;
	extSerial.write(frame);
	return _ERR_OK_;
}

int Dali::send_with_repeat(uint16_t frame)
{
	if(frame == INVALID_FRAME)
		return _ERR_INVALID_FRAME_;
	extSerial.write(frame);
    	_delay_ms(40);
	extSerial.write(frame);
	return _ERR_OK_;
}


int Dali::query(uint16_t frame, uint8_t* result)
{
	int i;
	if(frame == INVALID_FRAME)
		return _ERR_INVALID_FRAME_;
	extSerial.write(frame);
	_delay_ms(20);
	for(i = 0; i < 100; i++)
	{
		_delay_ms(1);
		if(extSerial.available())
			*result = extSerial.read();
	}
	return _ERR_OK_;
}
