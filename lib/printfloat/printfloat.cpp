#include "printfloat.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>


static const unsigned int pows[] = {1, 10, 100, 1000, 10000, 100000};
static const uint8_t n_pows = 6;


static unsigned int pow10(uint8_t input) {
    if (input < n_pows)
        return pows[input];
    unsigned int r = 1;
    for (uint8_t t = 0; t < input; t++)
        r *= 10;
    return r;
}


#define RET  { *destination=0; return ln; } while(0)
#define ADDCH(dst, sz, ch)   { *(dst)=(ch); (dst)++; (sz)--; if((sz)==1) RET; } while(0)


int snprintfloat(char *destination, size_t sz, float f, uint8_t fractures, uint8_t len) {
    if (fractures == 0) {
        return snprintf(destination, sz, "%*d", len, int32_t(round(f)));
    }
    f = f * pow10(fractures);
    char tmp[12];
    snprintf(tmp, 12, "%0*d", fractures + 1, uint32_t(abs(round(f))));
    size_t ln = strlen(tmp), iln = ln - fractures;
    int topad = len - ln - 1;
    if (topad < 0)
        topad = 0;
    ln += topad + 1;
    if (f < 0) {
        topad--;
        ln++;
    }

    if (topad > 0)
        for (size_t i = 0; i < topad; i++) ADDCH(destination, sz, ' ');
    if (f < 0) ADDCH(destination, sz, '-');
    for (size_t i = 0; i < iln; i++) ADDCH(destination, sz, tmp[i]);

    ADDCH(destination, sz, '.');
    for (size_t i = iln; i < ln; i++) ADDCH(destination, sz, tmp[i]);

    RET;
}


static inline bool isspace(const char t) {
    if ((t >= 9 && t <= 13) || t == 32) return true;
    return false;
}

static inline bool isdigit(const char t) {
    return t >= '0' && t <= '9';
}

double _atod(const char *arr) {
    bool neg = false;

    /* Initial whitespace */
    for (; isspace(*arr); arr++);

    /* Optional sign */
    if (*arr == '-') {
        neg = true;
        arr++;
    } else if (*arr == '+') { arr++; }

    /* Handle infinities and NaNs. */
    if (arr[0] != 0 && arr[1] != 0 && arr[2] != 0) {
        if ((arr[0] | 32) == 'i' && (arr[1] | 32) == 'n' && (arr[2] | 32) == 'f') {
            return neg ? -1.0 / 0.0 : 1.0 / 0.0;
        } else if ((arr[0] | 32) == 'n' && (arr[1] | 32) == 'a' && (arr[2] | 32) == 'n') {
            return 0.0 / 0.0;
        }
    }

    double val = 0;
    bool afterdot = false;
    double scale = 1;

    for (; isdigit(*arr) || *arr == '.'; arr++) {
        const char &c = *arr;
        if (afterdot) {
            scale = scale / 10;
            val = val + (c - '0') * scale;
        } else {
            if (c == '.') { afterdot = true; }
            else { val = val * 10.0 + (c - '0'); }
        }
    }
    if (neg)
        return -val;
    else
        return val;
}