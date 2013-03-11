#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
///must be first
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glew.h>
#include "SDL.h"


typedef int64_t GameTime;
typedef uint8_t byte;
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef intptr_t intptr;
typedef uintptr_t uintptr;
typedef ptrdiff_t ptrdiff;

void uprintf(const char* fmt, ...);

#ifdef _WIN32
#define ZBREAK() __debugbreak()
#else
#define ZBREAK() __builtin_trap()
#endif

#define ZASSERT(a) do{if(!(a)){ uprintf("ASSERTION FAILED %s:%d\n",__FILE__, __LINE__);  ZBREAK();}}while(0)
#define ZASSERT_FEQ(a,b,eps) \
do{\
	if(fabs(a-b)>eps)\
	{\
		uprintf("FEQ was %f, %f --> %f expected %f", a, b, fabs(a-b), eps); \
		ZASSERT(0);\
	}\
}while(0)

#ifdef _WIN32
#define ZPUSH_OPTIMIZE_OFF __pragma(optimize("", off))
#define ZPOP_OPTIMIZE_OFF __pragma(optimize("", on))
#else
#define ZPUSH_OPTIMIZE_OFF 
#define ZPOP_OPTIMIZE_OFF 
#endif




#ifdef _WIN32
#define snprintf _snprintf
#endif

