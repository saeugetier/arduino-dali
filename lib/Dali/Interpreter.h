#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "dali_encode.h"
#include <stdint.h>


#define MAX_BUFFER_LENGTH 128
#define _ERR_PARSE_ERROR_ -253
#define _ERR_BUFFER_FULL_ -254
#define _ERR_PARAMETER_MISSING_ -255
#define _ERR_UNIMPLEMENTED_ -256

#define _ERR_ACK 9000
#define _ERR_NACK 9001

#define _ERR_NO_ANSWER_ -100
#define _ERR_INVALID_FRAME_ -101

#define INVALID_FRAME 0x8080

char nibble_to_ascii(uint8_t nibble);

int decode_command_to_frame(const char* token, uint16_t* output);

int parse_int(const char* string, int16_t* integer);

#endif
