#include "Arduino.h"
#include "json_lib.h"

#ifndef PACKER_H
#define PACKER_H

#define REQUEST "request"
#define RESPONSE "response"
#define STRING "string"
#define NUMBER "number"
#define MODULE "module"

class MessagePacker
{

public:
    String pack(const char* sensor_data, const char* module_name, const char* type);
    String message(const char* code, const char* type);
};



#endif