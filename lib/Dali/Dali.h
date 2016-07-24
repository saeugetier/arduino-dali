#include <Arduino.h>
#include <ExtSoftwareSerial.h>
#include <dali_codes.h>
#include <dali_encode.h>

#define _ERR_NO_ANSWER_ -100
#define _ERR_INVALID_FRAME_ -101

#define INVALID_FRAME 0x8080

class Dali
{
public:
  Dali(uint8_t rx_pin, uint8_t tx_pin, uint8_t tx_en_pin);
  int sendDirect(address_mode mode, uint8_t address, uint8_t brightness);
  ~Dali();
protected:
  ExtSoftwareSerial extSerial;
  int send(uint16_t);
  int send_with_repeat(uint16_t frame);
  int query(uint16_t frame, uint8_t* result);
};
