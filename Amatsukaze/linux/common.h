/**
* Amatsukaze common header
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/
#pragma once

#include <cstdint>
#include <stdio.h>
#include <stdarg.h>

inline void PRINTF(const char* fmt,...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fflush(stderr);
}

inline void assertion_failed(const char* line, const char* file, int lineNum) {
    char buf[500];
    sprintf(buf, "Assertion failed!! %s (%s:%d)", line, file, lineNum);
    PRINTF("%s\n", buf);
    //MessageBox(NULL, "Error", "Amatsukaze", MB_OK);
    throw buf;
}

unsigned char inline _BitScanReverse(unsigned int *Index, unsigned int Mask)
{
    unsigned char rv;
    __asm(
          "bsrl %2, %0;"
          "setne %1;"
          : "=r"(*Index),"=r"(rv)
          : "g"(Mask)
          : "cc");
    return rv;
}

#ifndef _DEBUG
#define ASSERT(exp)
#else
#define ASSERT(exp) do { if(!(exp)) assertion_failed(#exp, __FILE__, __LINE__); } while(0)
#endif

