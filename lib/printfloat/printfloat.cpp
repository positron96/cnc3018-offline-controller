#include "printfloat.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>


static const unsigned int pows[] = {1, 10, 100, 1000, 10000, 100000};
static const uint8_t n_pows = 6;
    

static unsigned int pow10(uint8_t p) {
    if(p<n_pows) return pows[p];
    unsigned int r = 1;
    for(uint8_t t=0; t<p; t++) r*=10;
    return r;
}


size_t snprintfloat(char* dst, size_t sz, float f, uint8_t flen, uint8_t len) {
    // char intPart_reversed[10];
    // int i, charCount = 0;
    // double fp_int, fp_frac;

    // fp_frac = modf(f,&fp_int); //Separate integer/fractional parts

    // fp_int = abs(fp_int);
    // fp_frac = abs(fp_frac);
    // while (fp_int > 0) { //Convert integer part, if any
    //     intPart_reversed[charCount++] = '0' + (int)fmod(fp_int,10);
    //     fp_int = floor(fp_int/10);
    // }

    // if(f<0) { dst[0]='-'; dst++; }
    // for (i=0; i<charCount; i++) 
    //     dst[i] = intPart_reversed[charCount-i-1];

    // dst[charCount++] = '.'; 

    // while (fp_frac > 0)  {
    //     fp_frac*=10;
    //     fp_frac = modf(fp_frac,&fp_int);
    //     dst[charCount++] = '0' + (int)fp_int;
    // }

    // conversion[charCount] = 0; //String terminator

    // float ipf;
    // float fpf = modf(f, &ipf);
    // unsigned ipi = abs(int(ipf));
    // unsigned fpi = round(abs(fpf)*pow10(flen));
    // int ilen = len-1-flen;
    // if(ilen<=1) ilen=1;
    // char fmt[10];
    // snprintf(fmt, 10, /*padzero ? "%%0%dd.%%0%dd" : */ (ipi==0 && f<0) ? " %%%dd.%%0%dd" : "%%%dd.%%0%dd", ilen, flen);
    // ilen = snprintf(dst, sz, fmt, ipi, fpi);
    // if(ipi==0 && f<0) {
    //     // replace last space with '-'
    //     size_t p=0;
    //     while(dst[p]==' ') p++;
    //     dst[p-1] = '-';
    // }
    // return ilen;

}

size_t snprintfloat(char* dst, size_t sz, float f, uint8_t flen, uint8_t len) {
    f = f*pow10(flen);
    
    char fmt[5];
    snprintf(fmt, 5, "%%0%dd" , flen+1 );
    char tmp[12];
    snprintf(tmp, 12, fmt , uint32_t(abs(round(f))) );
    size_t ln = strlen(tmp);
    cout<<"tmp:"<<tmp<<", ln:"<<ln<<endl;
    size_t dotpos = ln>len-1 ? ln-flen : len-flen-2;
    size_t pdst=0, psrc=0;
    if(len>ln)while(pdst<len-ln) dst[pdst++]=' ';
    //if(f<0) { dst[pdst++]='-'; dotpos++; }
    while(pdst<sz && psrc<ln) {
        if(pdst==dotpos) { 
            dst[pdst]='.';
        } else {
            dst[pdst] = tmp[psrc];
            psrc++;
        }
        pdst++; 
    }
    dst[pdst]=0;
    return pdst;
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