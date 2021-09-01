#pragma once
#include <stddef.h>
#include <stdint.h>

size_t snprintfloat(char* dst, size_t sz, float f, uint8_t fraq=3, uint8_t len=0, bool padzero=false);

double _atod(const char* std);