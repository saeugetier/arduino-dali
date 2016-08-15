#include <Arduino.h>
#include <ExtSoftwareSerial.h>
#include <dali_codes.h>
#include <dali_encode.h>
#include <Interpreter.h>

#define ACK 0x3FFF
#define NACK -1
#define RESULT 0x4000

class Dali
{
public:
        Dali(uint8_t rx_pin, uint8_t tx_pin, uint8_t tx_en_pin);
        int sendDirect(address_mode mode, uint8_t address, uint8_t brightness);
        int parse_execute(const char* string);
        ~Dali();
protected:
        ExtSoftwareSerial extSerial;
        int send(uint16_t);
        int send_with_repeat(uint16_t frame);
        int query(uint16_t frame, uint8_t* result);
};
