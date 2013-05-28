#pragma once

#include <stdint.h>
#include <string.h>
#if 1
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

#define TICK() mach_absolute_time()
inline int64_t TickToNs(int64_t nTicks)
{
	static mach_timebase_info_data_t sTimebaseInfo;	
	if(sTimebaseInfo.denom == 0) 
	{
        (void) mach_timebase_info(&sTimebaseInfo);
    }

    return nTicks * sTimebaseInfo.numer / sTimebaseInfo.denom;
}
#elif defined(_WIN32)

#endif


void MicroProfileInit();
uint64_t MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor);
void MicroProfileEnter(uint64_t nToken);
void MicroProfileLeave(uint64_t nToken);


void MicroProfileFlip();
void MicroProfilePlot();


struct MicroProfileScopeHandler
{
	uint64_t nToken;
	MicroProfileScopeHandler(uint64_t lala):nToken(lala)
	{
		MicroProfileEnter(nToken);
	}
	~MicroProfileScopeHandler()
	{
		MicroProfileLeave(nToken);
	}
};



#define ZMICROPROFILE_DECLARE(var) extern uint64_t g_mp_##var
#define ZMICROPROFILE_DEFINE(var, group, name, color) uint64_t g_mp_##var(group, name, color)


#define ZMICROPROFILE_SCOPE(var) MicroProfileScopeHandler foo ## __LINE__(g_mp_##var)
#define ZMICROPROFILE_SCOPEI(group, name, color) static uint64_t g_mp##__LINE__ = MicroProfileGetToken(group, name, color); MicroProfileScopeHandler foo ## __LINE__(g_mp##__LINE__)

#ifdef MICRO_PROFILE_IMPL

#define MICROPROFILE_MAX_TIMERS 1024
struct MicroProfileTimer
{
	uint64_t nTicks;
	uint64_t nStart;
	uint32_t nCount;
};
struct MicroProfileTimerInfo
{
	const char* pGroup;
	const char* pName;
	uint32_t nColor;
};

struct 
{
	uint32_t nTimerCount;

	MicroProfileTimer Timers[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimerInfo TimerInfo[MICROPROFILE_MAX_TIMERS];

} g_MicroProfileState;

void MicroProfileInit()
{
	static bool bOnce = true;
	if(bOnce)
	{
		bOnce = false;
		memset(&g_MicroProfileState, 0, sizeof(g_MicroProfileState));
	}
}


uint64_t MicroProfileGetToken(const char* pGroup, const char* pName, uint32_t nColor)
{
	for(uint32_t i = 0; i < g_MicroProfileState.nTimerCount; ++i)
	{
		if(pGroup == g_MicroProfileState.TimerInfo[i].pGroup && pName == g_MicroProfileState.TimerInfo[i].pName)
		{
			return i;
		}
	}

	g_MicroProfileState.TimerInfo[g_MicroProfileState.nTimerCount].pGroup = pGroup;
	g_MicroProfileState.TimerInfo[g_MicroProfileState.nTimerCount].pName = pName;
	g_MicroProfileState.TimerInfo[g_MicroProfileState.nTimerCount].nColor = nColor;

	return g_MicroProfileState.nTimerCount++;
}

void MicroProfileEnter(uint64_t nToken)
{
	g_MicroProfileState.Timers[nToken].nStart = TICK();
}

void MicroProfileLeave(uint64_t nToken)
{
	g_MicroProfileState.Timers[nToken].nTicks += TICK() - g_MicroProfileState.Timers[nToken].nStart;
	g_MicroProfileState.Timers[nToken].nCount++;
}


#include "debug.h"

void MicroProfilePlot()
{
	for(uint32_t i = 0; i < g_MicroProfileState.nTimerCount; ++i)
	{
		uplotfnxt("Timer %s(%s) : %6.2fdms Count %d", g_MicroProfileState.TimerInfo[i].pGroup,
			g_MicroProfileState.TimerInfo[i].pName,
			TickToNs(g_MicroProfileState.Timers[i].nTicks)/1000000.f, g_MicroProfileState.Timers[i].nCount);
	}
}

void MicroProfileFlip()
{
	for(uint32_t i = 0; i < g_MicroProfileState.nTimerCount; ++i)
	{
		g_MicroProfileState.Timers[i].nTicks = 0;
		g_MicroProfileState.Timers[i].nCount = 0;


	}
}




#endif