#pragma once

#include <Arduino.h>

#define LOGF(...)   SerialUSB.printf(__VA_ARGS__)
#define LOGLN(...)  SerialUSB.println(__VA_ARGS__)