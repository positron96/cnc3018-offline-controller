#pragma once
#include <stddef.h>
#include <stdint.h>

int snprintfloat(char* dst, size_t sz, float f, uint8_t fraq=3, uint8_t len=0);

double _atod(const char* std);