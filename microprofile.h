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
#define MP_BREAK __builtin_trap()
#elif defined(_WIN32)
#define MP_BREAK __debugbreak()
#endif
#define MP_ASSERT(a) do{if(!(a)){MP_BREAK();}}while(0)



#define ZMICROPROFILE_DECLARE(var) extern MicroProfileToken g_mp_##var
#define ZMICROPROFILE_DEFINE(var, group, name, color) MicroProfileToken g_mp_##var(group, name, color)
#define ZMICROPROFILE_SCOPE(var) MicroProfileScopeHandler foo ## __LINE__(g_mp_##var)
#define ZMICROPROFILE_SCOPEI(group, name, color) static MicroProfileToken g_mp##__LINE__ = MicroProfileGetToken(group, name, color); MicroProfileScopeHandler foo ## __LINE__(g_mp##__LINE__)
#define ZMICROPROFILE_TEXT_WIDTH 5
#define ZMICROPROFILE_TEXT_HEIGHT 8
#define ZMICROPROFILE_FRAME_TIME (1000.f / 30.f)
#define ZMICROPROFILE_FRAME_TIME_TO_PRC (100.f / ZMICROPROFILE_FRAME_TIME)




typedef uint32_t MicroProfileToken;
typedef uint16_t MicroProfileGroupId;


void MicroProfileInit();
MicroProfileToken MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor);
void MicroProfileEnter(MicroProfileToken nToken);
void MicroProfileLeave(MicroProfileToken nToken);
inline uint16_t MicroProfileGetTimerIndex(MicroProfileToken t){ return (t&0xffff); }
inline uint16_t MicroProfileGetGroupIndex(MicroProfileToken t){ return ((t>>16)&0xffff);}
inline MicroProfileToken MicroProfileMakeToken(uint16_t nGroup, uint16_t nTimer){ return ((uint32_t)nGroup<<16) | nTimer;}


void MicroProfileFlip(); //! called once per frame.
void MicroProfileDraw(); //! call if drawing microprofilers
void MicroProfileSetAggregateCount(uint32_t nCount); //!Set no. of frames to aggregate over. 0 for infinite
void MicroProfileNextAggregatePreset(); //! Flip over some aggregate presets
void MicroProfilePrevAggregatePreset(); //! Flip over some aggregate presets
void MicroProfileNextGroup(); //! Toggle Group of bars displayed
void MicroProfilePrevGroup(); //! Toggle Group of bars displayed
void MicroProfileToggleDetailed(); //Toggle detailed view
void MicroProfileToggleTimers();//Toggle timer view
void MicroProfileToggleAverageTimers(); //Toggle average view
void MicroProfileToggleMaxTimers(); //Toggle max view


//UNDEFINED: MUST BE IMPLEMENTED ELSEWHERE
void MicroProfileDrawText(uint32_t nX, uint32_t nY, uint32_t nColor, const char* pText);
void MicroProfileDrawBox(uint32_t nX, uint32_t nY, uint32_t nWidth, uint32_t nHeight);
void MicroProfileMouseMove(uint32_t nX, uint32_t nY);
void MicroProfileMouseClick(uint32_t nLeft, uint32_t nRight);



struct MicroProfileScopeHandler
{
	MicroProfileToken nToken;
	MicroProfileScopeHandler(MicroProfileToken Token):nToken(Token)
	{
		MicroProfileEnter(nToken);
	}
	~MicroProfileScopeHandler()
	{
		MicroProfileLeave(nToken);
	}
};




#ifdef MICRO_PROFILE_IMPL
#define S g_MicroProfile
#define MICROPROFILE_MAX_TIMERS 1024
#define MICROPROFILE_MAX_GROUPS 128
enum MicroProfileDrawMask
{
	DRAW_TIMERS 	= 0x1,
	DRAW_AVERAGE	= 0x2,
	DRAW_MAX		= 0x4,
	DRAW_DETAILED	= 0x8,
}

struct MicroProfileTimer
{
	uint64_t nTicks;
	uint32_t nCount;
};

struct MicroProfileGroupInfo
{
	const char* pName;
	uint32_t nGroupIndex;
	uint32_t nNumTimers;
};

struct MicroProfileTimerInfo
{
	MicroProfileToken nToken;
	uint32_t nTimerIndex;
	const char* pName;
	uint32_t nColor;
};

struct 
{
	uint32_t nTotalTimers;
	uint32_t nGroupCount;
	uint32_t nAggregateFlip;
	uint32_t nAggregateFlipCount;
	uint32_t nAggregateFrames;
	

	uint32_t nDisplay;
	uint32_t nActiveGroup;
	uint32_t nDirty;

	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	
	MicroProfileTimer 		FrameTimers[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		AggregateTimers[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];


	uint64_t 				nStart[MICROPROFILE_MAX_TIMERS];



} g_MicroProfile;

void MicroProfileInit()
{
	static bool bOnce = true;
	if(bOnce)
	{
		bOnce = false;
		memset(&S, 0, sizeof(S));
		S.nGroupCount = 1;//0 is for inactive
	}
}


MicroProfileToken MicroProfileGetToken(const char* pGroup, const char* pName, uint32_t nColor)
{
	uint16_t nGroupIndex = 0xffff;
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		if(!strcmp(pGroup, S.GroupInfo[i].pName))
		{
			nGroupIndex = i;
			break;
		}
	}
	if(0xffff == nGroupIndex)
	{
		S.GroupInfo[S.nGroupCount].pName = pGroup;
		S.GroupInfo[S.nGroupCount].nGroupIndex = S.nGroupCount;
		S.GroupInfo[S.nGroupCount].nNumTimers = 0;
		nGroupIndex = S.nGroupCount++;
	}
	uint16_t nTimerIndex = S.nTotalTimers++;
	MicroProfileToken nToken = MicroProfileMakeToken(nGroupIndex, nTimerIndex);
	S.TimerInfo[nTimerIndex].nToken = nToken;
	S.TimerInfo[nTimerIndex].pName = pName;
	S.TimerInfo[nTimerIndex].nColor = nColor;
	S.nDirty = 1;
	return nToken;
}

void MicroProfileEnter(MicroProfileToken nToken)
{
	nToken &= 0xffff;
	MP_ASSERT(0 == S.nStart[nToken]);
	S.nStart[nToken] = TICK();
}

void MicroProfileLeave(MicroProfileToken nToken)
{
	nToken &= 0xffff;
	S.FrameTimers[nToken].nTicks += TICK() - S.nStart[nToken];
	S.FrameTimers[nToken].nCount++;
	S.nStart[nToken] = 0;
}


#include "debug.h"

void MicroProfileDraw();

void MicroProfileFlip()
{
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		S.AggregateTimers[i].nTicks += S.FrameTimers[i].nTicks;
		S.AggregateTimers[i].nCount += S.FrameTimers[i].nCount;
		S.Frame[i].nTicks = S.Frametimers[i].nTicks;
		S.Frame[i].nCount = S.FrameTimers[i].nCount;
		S.FrameTimers[i].nTicks = 0;
		S.FrameTimers[i].nCount = 0;
	}

	bool bFlipAggregate = false;
	uint32 nFlipFrequency = S.nAggregateFlip ? S.nAggregateFlip : 30;
	if(S.nAggregateFlip == ++S.nAggregateFlipCount)
	{
		memcpy(&S.Aggregate[0], &S.AggregateTimers[0], sizeof(S.Aggregate[0]) * S.nTotalTimers);
		S.nAggregateFrames = S.nAggregateFlipCount;
		if(S.nAggregateFlip)// if 0 accumulate indefinitely
		{
			memset(&S.AggregateTimers[0], 0, sizeof(S.Aggregate[0]) * S.nTotalTimers);
			S.nAggregateFlipCount = 0;
		}
	}
	memset(&S.FrameTimers[0], 0, sizeof(S.FrameTimers[0]) * S.nTotalTimers);
}


static uint32_t g_AggregatePresets[] = {0, 10, 20, 30, 60, 120};
void MicroProfileSetAggregateCount(uint32_t nCount)
{
	nAggregateFlip = nCount;
}
void MicroProfileNextAggregatePreset()
{
	uint32_t nNext = g_AggregatePresets[0];
	uint32_t nCurrent = S.nAggregateFlip;
	for(uint32_t nPreset : g_AggregatePresets)
	{
		if(nPreset > nCurrent)
		{
			nCurrent = nPreset;
			break;
		}
	}
	S.nAggregateFlip = nCurrent;
	S.nDirty = 1;
}
void MicroProfilePrevAggregatePreset()
{
	uint32_t nNext = g_AggregatePresets[(sizeof(g_AggregatePresets)/sizeof(g_AggregatePresets[0]))-1];
	uint32_t nCurrent = S.nAggregateFlip;

	for(uint32_t nPreset : g_AggregatePresets)
	{
		if(nPreset < nCurrent)
		{
			nCurrent = nPreset;
		}
	}
	S.nAggregateFlip = nPreset;
	S.nDirty = 1;
}
void MicroProfileNextGroup()
{
	S.nActiveGroup = (S.nActiveGroup+1)%S.nGroupCount;

}
void MicroProfilePrevGroup()
{
	S.nActiveGroup = (S.nActiveGroup-1)%S.nGroupCount;
}
void MicroProfileToggleDetailed()
{
	s.nDisplay ^= DRAW_DETAILED;
}
void MicroProfileToggleTimers()
{
	s.nDisplay ^= DRAW_TIMERS;
}
void MicroProfileToggleAverageTimers()
{
	s.nDisplay ^= DRAW_AVERAGE;
}
void MicroProfileToggleMaxTimers()
{
	s.nDisplay ^= DRAW_MAX;
}



void MicroProfilePlot()
{
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uplotfnxt("Timer %s(%s) : %6.2fdms Count %d", S.GroupInfo[MicroProfileGetGroupIndex(S.TimerInfo[i].nToken)].pName,
			S.TimerInfo[i].pName,
			TickToNs(S.FrameTimers[i].nTicks)/1000000.f, S.FrameTimers[i].nCount);
	}
}


void MicroProfileDrawDetailedView()
{
}

void MicroProfileCalcTimers(float* pTimers, float* pAverage, float* pMax, float* pCallAverage, float* pMaxAverage, uint16_t nGroup)
{
	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(S.TimerInfo[i].nGroup == nGroup || nGroup == 0xffff)
		{
			float fMs = TickToNs(S.Frame.nTicks) * 1000000.f;
			float fPrc = fMs * ZMICROPROFILE_FRAME_TIME_TO_PRC;
			*pTimers++ = fMs;
			*pTimers++ = fPrc;

			float fAverageMs = TickToNs(S.Aggregate.nTicks / S.nAggregateFrames) * 1000000.f;
			float fAveragePrc = fMs * ZMICROPROFILE_FRAME_TIME_TO_PRC;
			*pAverage++ = fAverageMs;
			*pAverage++ = fAveragePrc;


		}
	}

}

void MicroProfileDrawBarView()
{
	if(!S.nActiveGroup)
		return
	uint32 nNumTimers = S.GroupInfo[S.nActiveGroup];
	float* pTimers = alloca(2*sizeof(float) * nNumTimers);
	MicroProfileCalcTimers(pTimers)
}

void MicroProfileDraw()
{
	if(s.nDisplay&DRAW_DETAILED)
	{
		MicroProfileDrawDetailedView();
	}
	else
	{
		MicroProfileDrawBarView();
	}
}


#endif