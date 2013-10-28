#pragma once
#include "base.h"
#include "math.h"

//general debug stuff
#define ZDEBUG_DRAW_ENABLED 1

extern uint32 g_lShowDebug;

#if ZDEBUG_DRAW_ENABLED
void DebugDrawLine(v4 start, v4 end, uint32_t nColor);
void DebugDrawLine(v3 start, v3 end, uint32_t nColor);
void DebugDrawBounds(v3 vmin, v3 vmax, uint32_t nColor);
void DebugDrawPoly(v3* pVertex, uint32 nNumVertex, uint32_t nColor);
void DebugDrawPoly(v4* pVertex, uint32 nNumVertex, uint32_t nColor);
void DebugDrawBounds(m mObjectToWorld, v3 vSize, uint32 nColor);
void DebugDrawBox(m rot, v3 pos, v3 size, uint32 nColor, uint32 usez);
void DebugDrawSphere(v3 vPos, float fRadius, uint32_t nColor);
void DebugDrawPlane(v3 vNormal, v3 pos, uint32_t nColor);

#define ZDEBUG_DRAWLINE(v0, v1, color, unused) DebugDrawLine(v0, v1, color)
#define ZDEBUG_DRAWPOLY(pVert, NumVert, color) DebugDrawPoly(pVert, NumVert, color)
#define ZDEBUG_DRAWBOUNDS(mObjectToWorld, vSize, color) DebugDrawBounds(mObjectToWorld, vSize, color)
#define ZDEBUG_DRAWBOX(mrot, pos, size, color, usez) DebugDrawBox(mrot, pos, size, color, usez)
#define ZDEBUG_DRAWSPHERE(pos, radius, color) DebugDrawSphere(pos, radius, color)
#define ZDEBUG_DRAWPLANE(normal, pos, color) DebugDrawPlane(normal, pos, color)


void DebugDrawInit();
void DebugDrawFlush(m mprj);

void uplotfnxt(const char* pfmt, ...);
void uplotf(uint32_t nX, uint32_t nY, const char* pfmt, ...);
#else

#define ZDEBUG_DRAWLINE(...) do{} while(0)
#define ZDEBUG_DRAWPOLY(...) do{} while(0)
#define ZDEBUG_DRAWBOUNDS(...) do{} while(0)
#define ZDEBUG_DRAWBOX(...) do{} while(0)
#define ZDEBUG_DRAWSPHERE(...) do{} while(0)
#define ZDEBUG_DRAWPLANE(...) do{} while(0)

#define DebugDrawInit() do{}while(0)
#define DebugDrawFlush(m) do{} while(0)

#define uplotfnxt(...) do{}while(0)
#define uplotf(...) do{}while(0)

#endif

void uprintf(const char* fmt, ...);