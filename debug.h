#pragma once
#include "base.h"
#include "math.h"

//general debug stuff

extern uint32 g_lShowDebug;


void DebugDrawLine(v4 start, v4 end, uint32_t nColor);
void DebugDrawLine(v3 start, v3 end, uint32_t nColor);
void DebugDrawBounds(v3 vmin, v3 vmax, uint32_t nColor);


#define ZDEBUG_DRAWLINE(v0, v1, color, unused) DebugDrawLine(v0, v1, color)



void DebugDrawInit();
void DebugDrawFlush();

void uplotfnxt(const char* pfmt, ...);
void uplotf(uint32_t nX, uint32_t nY, const char* pfmt, ...);
void uprintf(const char* fmt, ...);
