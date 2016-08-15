#include "Dali.h"
#include "Interpreter.h"

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

int Dali::parse_execute(const char* string)
{
        int result = NACK;
        uint16_t frame  = INVALID_FRAME;
        int ret = decode_command_to_frame(string, &frame); //frame is 2 bytes long. decode_command_to_frame is returning type of frame (SIMPLE, REPEAT_TWICE, QUERY)
        if(ret > 0)
        {
                if(_MODE_SIMPLE_ == ret)
                {
                        if(_ERR_OK_ == send(frame))
                                result = ACK;
                        else
                        {
                                result = _ERR_PARSE_ERROR_;
                                Serial.println("Parser error");
                        }
                }
                else if(_MODE_REPEAT_TWICE_ == ret)
                {
                        if(_ERR_OK_ == send_with_repeat(frame))
                                result = ACK;
                        else
                        {
                                result = _ERR_PARSE_ERROR_;
                                Serial.println("Parser error");
                        }
                }
                else if(_MODE_QUERY_ == ret)
                {
                        byte ans;
                        if(_ERR_OK_ == query(frame, &ans))
                        {
                                result = RESULT | ans;
                        }
                        else
                                result = _ERR_PARSE_ERROR_;
                        Serial.println("Parser error");
                }
                else if(ret != _ERR_NACK && ret != _ERR_ACK)
                        result = _ERR_UNIMPLEMENTED_;
        }
        else
        {
                result = _ERR_NACK;
        }

        return result;
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
