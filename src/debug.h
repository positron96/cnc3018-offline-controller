#pragma once

#include <Arduino.h>

#define LOGF(...)   do{SerialUSB.printf(__VA_ARGS__);}while(0)
#define LOGLN(...)  do{SerialUSB.println(__VA_ARGS__);}while(0)

//#define LOGF(...)
//#define LOGLN(...)
