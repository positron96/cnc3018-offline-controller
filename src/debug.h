#pragma once

#include <Arduino.h>

#ifdef LOG_DEBUG

    #define LOGF(...)   do{SerialUSB.printf(__VA_ARGS__);}while(0)
    #define LOGLN(...)  do{SerialUSB.println(__VA_ARGS__);}while(0)

#else

    #define LOGF(...) do{} while(0)
    #define LOGLN(...) do{} while(0)

#endif

//#define LOGF(...)
//#define LOGLN(...)