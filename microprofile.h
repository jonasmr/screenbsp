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
#define MP_BREAK() __builtin_trap()
#elif defined(_WIN32)
#define MP_BREAK() __debugbreak()
#endif
#define MP_ASSERT(a) do{if(!(a)){MP_BREAK();} }while(0)



#define ZMICROPROFILE_DECLARE(var) extern MicroProfileToken g_mp_##var
#define ZMICROPROFILE_DEFINE(var, group, name, color) MicroProfileToken g_mp_##var(group, name, color)
#define ZMICROPROFILE_SCOPE(var) MicroProfileScopeHandler foo ## __LINE__(g_mp_##var)
#define ZMICROPROFILE_SCOPEI(group, name, color) static MicroProfileToken g_mp##__LINE__ = MicroProfileGetToken(group, name, color); MicroProfileScopeHandler foo ## __LINE__(g_mp##__LINE__)
#define ZMICROPROFILE_TEXT_WIDTH 5
#define ZMICROPROFILE_TEXT_HEIGHT 8
#define ZMICROPROFILE_FRAME_TIME (1000.f / 30.f)
//#define ZMICROPROFILE_FRAME_TIME (1000.f / 500.f)
#define ZMICROPROFILE_FRAME_TIME_TO_PRC (1.f / ZMICROPROFILE_FRAME_TIME)




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
void MicroProfileDraw(uint32_t nWidth, uint32_t nHeight); //! call if drawing microprofilers
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
void MicroProfileDrawBox(uint32_t nX, uint32_t nY, uint32_t nWidth, uint32_t nHeight, uint32_t nColor);
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
};

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

	uint32_t nBarWidth;
	uint32_t nBarHeight;

	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	
	MicroProfileTimer 		FrameTimers[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		AggregateTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				MaxTimers[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMax[MICROPROFILE_MAX_TIMERS];


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
		S.nBarWidth = 100;
		S.nBarHeight = ZMICROPROFILE_TEXT_HEIGHT;
		S.nActiveGroup = 1;
		S.GroupInfo[0].pName = "___ROOT";
		S.nAggregateFlip = 15;
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
	S.GroupInfo[nGroupIndex].nNumTimers++;
	S.TimerInfo[nTimerIndex].nToken = nToken;
	S.TimerInfo[nTimerIndex].pName = pName;
	S.TimerInfo[nTimerIndex].nColor = nColor;
	S.nDirty = 1;
	return nToken;
}

void MicroProfileEnter(MicroProfileToken nToken)
{
	if(S.nActiveGroup == 0xffff || MicroProfileGetGroupIndex(nToken) == S.nActiveGroup)
	{
		nToken &= 0xffff;
		MP_ASSERT(0 == S.nStart[nToken]);
		S.nStart[nToken] = TICK();
	}
}

void MicroProfileLeave(MicroProfileToken nToken)
{
	if(S.nActiveGroup == 0xffff || MicroProfileGetGroupIndex(nToken) == S.nActiveGroup)
	{
		nToken &= 0xffff;
		S.FrameTimers[nToken].nTicks += TICK() - S.nStart[nToken];
		S.FrameTimers[nToken].nCount++;
		S.nStart[nToken] = 0;
	}
}


#include "debug.h"

void MicroProfileFlip()
{
	ZMICROPROFILE_SCOPEI("MicroProfile", "MicroProfileFlip", randcolor());
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		S.AggregateTimers[i].nTicks += S.FrameTimers[i].nTicks;
		S.AggregateTimers[i].nCount += S.FrameTimers[i].nCount;
		S.Frame[i].nTicks = S.FrameTimers[i].nTicks;
		S.Frame[i].nCount = S.FrameTimers[i].nCount;
		S.MaxTimers[i] = Max(S.MaxTimers[i], S.FrameTimers[i].nTicks);
		S.FrameTimers[i].nTicks = 0;
		S.FrameTimers[i].nCount = 0;
	}

	bool bFlipAggregate = false;
	uint32 nFlipFrequency = S.nAggregateFlip ? S.nAggregateFlip : 30;
	if(S.nAggregateFlip == ++S.nAggregateFlipCount)
	{
		memcpy(&S.Aggregate[0], &S.AggregateTimers[0], sizeof(S.Aggregate[0]) * S.nTotalTimers);
		memcpy(&S.AggregateMax[0], &S.MaxTimers[0], sizeof(S.AggregateMax[0]) * S.nTotalTimers);
		S.nAggregateFrames = S.nAggregateFlipCount;
		if(S.nAggregateFlip)// if 0 accumulate indefinitely
		{
			memset(&S.AggregateTimers[0], 0, sizeof(S.Aggregate[0]) * S.nTotalTimers);
			memset(&S.MaxTimers[0], 0, sizeof(S.MaxTimers[0]) * S.nTotalTimers);
			S.nAggregateFlipCount = 0;
		}
	}
	memset(&S.FrameTimers[0], 0, sizeof(S.FrameTimers[0]) * S.nTotalTimers);
}


static uint32_t g_AggregatePresets[] = {0, 10, 20, 30, 60, 120};
void MicroProfileSetAggregateCount(uint32_t nCount)
{
	S.nAggregateFlip = nCount;
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
	S.nAggregateFlip = nCurrent;
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
	S.nDisplay ^= DRAW_DETAILED;
}
void MicroProfileToggleTimers()
{
	S.nDisplay ^= DRAW_TIMERS;
}
void MicroProfileToggleAverageTimers()
{
	S.nDisplay ^= DRAW_AVERAGE;
}
void MicroProfileToggleMaxTimers()
{
	S.nDisplay ^= DRAW_MAX;
}



void MicroProfilePlot()
{
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uplotfnxt("Timer %s(%s) : %6.2fdms Count %d", S.GroupInfo[MicroProfileGetGroupIndex(S.TimerInfo[i].nToken)].pName,
			S.TimerInfo[i].pName,
			TickToNs(S.Frame[i].nTicks)/1000000.f, S.Frame[i].nCount);
	}
}


void MicroProfileDrawDetailedView(uint32_t nWidth, uint32_t nHeight)
{
}

void MicroProfileCalcTimers(float* pTimers, float* pAverage, float* pMax, float* pCallAverage, uint16_t nGroup)
{
	ZMICROPROFILE_SCOPEI("MicroProfile", "MicroProfileCalcTimers", randcolor());

	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == nGroup || nGroup == 0xffff)
		{
			float fMs = TickToNs(S.Frame[i].nTicks) / 1000000.f;
			float lala = ZMICROPROFILE_FRAME_TIME_TO_PRC;
			float fPrc = fMs * lala;
			uint32 nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
			uint32 nAggregateCount = S.Aggregate[i].nCount ? S.Aggregate[i].nCount : 1;
			float fAverageMs = TickToNs(S.Aggregate[i].nTicks / nAggregateFrames) / 1000000.f;
			float fAveragePrc = fAverageMs * ZMICROPROFILE_FRAME_TIME_TO_PRC;
			float fMaxMs = TickToNs(S.AggregateMax[i]) / 1000000.f;
			float fMaxPrc = fMaxMs * ZMICROPROFILE_FRAME_TIME_TO_PRC;
			float fCallAverageMs = TickToNs(S.Aggregate[i].nTicks / nAggregateCount) / 1000000.f;
			float fCallAveragePrc = fAverageMs * ZMICROPROFILE_FRAME_TIME_TO_PRC;
			*pTimers++ = fMs;
			*pTimers++ = fPrc;
			*pAverage++ = fAverageMs;
			*pAverage++ = fAveragePrc;
			*pMax++ = fMaxMs;
			*pMax++ = fMaxPrc;
			*pCallAverage++ = fCallAverageMs;
			*pCallAverage++ = fCallAveragePrc;
		}
	}

}

uint32_t MicroProfileDrawBarArray(uint32_t nX, uint32_t nY, uint32_t nGroup, float* pTimers)
{
	uint32_t nHeight = S.nBarHeight;
	uint32_t nWidth = S.nBarWidth;
	uint32_t tidx = 0;
	uint32_t nTextWidth = 6 * ZMICROPROFILE_TEXT_WIDTH;
	float fWidth = S.nBarWidth;
	#define SBUF_MAX 32
	char sBuffer[SBUF_MAX];
	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == nGroup || nGroup == 0xffff)
		{
			if(i == 0) pTimers[tidx+1] = 1.f;
			snprintf(sBuffer, SBUF_MAX-1, "%5.2f", pTimers[tidx]);
			MicroProfileDrawBox(nX + nTextWidth, nY, fWidth * pTimers[tidx+1], nHeight, S.TimerInfo[i].nColor);
			MicroProfileDrawText(nX, nY, (uint32_t)-1, sBuffer);
			nY += nHeight + 1;
			tidx += 2;
		}
	}
	return nWidth + 5 + nTextWidth;
}

void MicroProfileDrawBarView(uint32_t nScreenWidth, uint32_t nScreenHeight)
{
	if(!S.nActiveGroup)
		return;

	//MicroProfileDrawBox(100, 100, 200,200, (uint32)-1);

	ZMICROPROFILE_SCOPEI("MicroProfile", "DrawBarView", randcolor());
	uint32_t nNumTimers = S.GroupInfo[S.nActiveGroup].nNumTimers;
	uint32_t nBlockSize = 2 * nNumTimers;
	float* pTimers = (float*)alloca(nBlockSize * 4 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pCallAverage = pTimers + 3 * nBlockSize;
	MicroProfileCalcTimers(pTimers, pAverage, pMax, pCallAverage, S.nActiveGroup);

	uint32_t nX = 10;
	uint32_t nY = 10;

	nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pTimers) + 1;
	nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pAverage) + 1;
	nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pMax) + 1;
	nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pCallAverage) + 1;

}

void MicroProfileDraw(uint32_t nWidth, uint32_t nHeight)
{
	ZMICROPROFILE_SCOPEI("MicroProfile", "Draw", randcolor());
	MicroProfilePlot();
	if(S.nDisplay&DRAW_DETAILED)
	{
		MicroProfileDrawDetailedView(nWidth, nHeight);
	}
	else
	{
		MicroProfileDrawBarView(nWidth, nHeight);
	}
}


#endif