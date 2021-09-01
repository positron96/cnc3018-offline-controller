#include "printfloat.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
//#include <ctype.h>


static const unsigned int pows[] = {1, 10, 100, 1000, 10000, 100000};
static const uint8_t n_pows = 6;
    

static unsigned int pow10(uint8_t p) {
    if(p<n_pows) return pows[p];
    unsigned int r = 1;
    for(uint8_t t=0; t<p; t++) r*=10;
    return r;
}

size_t snprintfloat(char* dst, size_t sz, float f, uint8_t fraq, uint8_t len, bool padzero) {
    int ip = f;
    unsigned int fp = round(abs(f)*pow10(fraq));
    char fmt[10];
    int t = len-1-fraq;
    if(t<=1) t=1;
    snprintf(fmt, 10, padzero ? "%%0%dd.%%0%dd" : "%%%dd.%%0%dd", t, fraq);
    return snprintf(dst, sz, fmt, ip, fp);
}


static inline bool isspace(const char t) {
    if( (t>=9 && t<=13) || t==32 ) return true;
    return false;
}

static inline bool isdigit(const char t) {
    return t>='0' && t<='9';
}

double _atod(const char* arr) {
    bool neg = false;

    /* Initial whitespace */
    for (; isspace(*arr); arr++);

    /* Optional sign */
    if (*arr == '-') { neg = true; arr++; }
    else if (*arr == '+') {arr++;}

    /* Handle infinities and NaNs. */
    if(arr[0]!=0 && arr[1]!=0 && arr[2]!=0) {
        if ( (arr[0]|32)=='i' && (arr[1]|32)=='n' && (arr[2]|32)=='f') {
            return neg ? -1.0/0.0 : 1.0/0.0;
        } else if ((arr[0]|32)=='n' && (arr[1]|32)=='a' && (arr[2]|32)=='n') {
            return 0.0/0.0;
        }
    }
    
    double val = 0;    
    bool afterdot=false;
    double scale=1;

    for (; isdigit(*arr) || *arr=='.'; arr++) {
        const char &c = *arr;
        if (afterdot) {
            scale = scale/10;
            val = val + (c-'0')*scale;
        } else {
            if (c == '.') { afterdot=true; }
            else {  val = val*10.0 + (c-'0'); }
        }
    }
    if(neg) return -val;
    else    return  val;
}