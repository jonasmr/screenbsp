#pragma once

#include "bsp.h"

extern uint32 g_nRunTest;
extern FILE* g_TestOut;
extern FILE* g_TestFailOut;
extern int32 g_nTestFail;
extern int32 g_nTestFalsePositives;

#define QUICK_PERF 1


#if 1
#define OCCLUSION_NUM_LARGE 10
#define OCCLUSION_NUM_SMALL 100
#define OCCLUSION_NUM_LONG 20
#define OCCLUSION_NUM_OBJECTS 2000
#define OCCLUSION_USE_GROUND 1
#else

#define OCCLUSION_NUM_LARGE 10
#define OCCLUSION_NUM_SMALL 100
#define OCCLUSION_NUM_LONG 20
#define OCCLUSION_NUM_OBJECTS 500
#define OCCLUSION_USE_GROUND 1

#endif

void WorldInitOcclusionTest();
void StopTest();
void StartTest();
void RunTest(v3& vPos_, v3& vDir_, v3& vRight_);


struct SWorldSector
{
	v3 vMin;
	v3 vMax;
	uint32_t nStart;
	uint32_t nCount;

	SOccluderDesc Desc;
};

struct SWorldGrid
{
	enum {
		MAX_SECTORS = 20*20,
		MAX_OBJECTS = OCCLUSION_NUM_OBJECTS * 4,
	};
	SWorldSector Sector[MAX_SECTORS];
	uint16_t nIndices[MAX_OBJECTS];
	uint32_t nNumSectors;
	uint32_t nNumNodes;

};

extern SWorldGrid g_Grid3;
extern SWorldGrid g_Grid4;
extern SWorldGrid g_Grid5;
extern SWorldGrid g_Grid8;
extern SWorldGrid g_Grid10;
extern SWorldGrid g_Grid12;
extern SWorldGrid g_Grid15;
extern SWorldGrid g_Grid20;


