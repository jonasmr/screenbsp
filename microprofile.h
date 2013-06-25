#pragma once
// TODO: 
// blinking
// line over
// scroll legends
// active vs full vs current
// buffer overflow signal + handling
// multithread
// graph in detailed
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
#define MP_TICK_TO_MS(a) (TickToNs(a) / 1000000.f)



#define MICROPROFILE_DECLARE(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE(var, group, name, color) MicroProfileToken g_mp_##var(group, name, color)
#define MICROPROFILE_TOKEN_PASTE0(a, b) a ## b
#define MICROPROFILE_TOKEN_PASTE(a, b)  MICROPROFILE_TOKEN_PASTE0(a,b)
#define MICROPROFILE_SCOPE(var) MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPEI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color); MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_TEXT_WIDTH 5
#define MICROPROFILE_TEXT_HEIGHT 8
#define MICROPROFILE_DETAILED_BAR_HEIGHT 12
#define MICROPROFILE_FRAME_TIME (1000.f / 30.f)
#define MICROPROFILE_FRAME_TIME_TO_PRC (1.f / MICROPROFILE_FRAME_TIME)
#define MICROPROFILE_GRAPH_WIDTH 256
#define MICROPROFILE_GRAPH_HEIGHT 256
#define MICROPROFILE_BORDER_SIZE 1
#define MICROPROFILE_MAX_GRAPH_TIME 100.f


typedef uint32_t MicroProfileToken;
typedef uint16_t MicroProfileGroupId;


#define MICROPROFILE_INVALID_TOKEN (uint32_t)-1

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
void MicroProfileToggleDisplayMode(); //switch between off, bars, detailed
void MicroProfileToggleTimers();//Toggle timer view
void MicroProfileToggleAverageTimers(); //Toggle average view
void MicroProfileToggleMaxTimers(); //Toggle max view
void MicroProfileToggleCallCount(); // Toggle call count view
void MicroProfileClearGraph();
void MicroProfileToggleFlipDetailed();
void MicroProfileMouseMove(uint32_t nX, uint32_t nY);
void MicroProfileMouseClick(uint32_t nLeft, uint32_t nRight);
void MicroProfileMoveGraph(int nDir, int nPan);

//UNDEFINED: MUST BE IMPLEMENTED ELSEWHERE
void MicroProfileDrawText(uint32_t nX, uint32_t nY, uint32_t nColor, const char* pText);
void MicroProfileDrawBox(uint32_t nX, uint32_t nY, uint32_t nWidth, uint32_t nHeight, uint32_t nColor);
void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor);



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
#include <stdlib.h>
#include <stdio.h>

#define S g_MicroProfile
#define MICROPROFILE_MAX_TIMERS 1024
#define MICROPROFILE_MAX_GROUPS 128
#define MICROPROFILE_MAX_GRAPHS 5
#define MICROPROFILE_GRAPH_HISTORY 128
#define MICROPROFIL_LOG_BUFFER_SIZE ((4*2048)<<10)/sizeof(MicroProfileLogEntry)

#include "debug.h"

enum MicroProfileDrawMask
{
	MP_DRAW_OFF		= 0x0,
	MP_DRAW_BARS		= 0x1,
	MP_DRAW_DETAILED	= 0x2,
};
enum MicroProfileDrawBarsMask
{
	MP_DRAW_TIMERS 	= 0x1,
	MP_DRAW_AVERAGE	= 0x2,
	MP_DRAW_MAX		= 0x4,
	MP_DRAW_CALL_COUNT	= 0x8,
	MP_DRAW_ALL = 0xf,

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

struct MicroProfileGraphState
{
	float fHistory[MICROPROFILE_GRAPH_HISTORY];
	MicroProfileToken nToken;
	int32_t nKey;
};

struct MicroProfileLogEntry
{
	enum EType
	{
		EEnter, ELeave,
	};
	EType eType;
	MicroProfileToken nToken;
	uint64_t nTick;
};


struct 
{
	uint32_t nTotalTimers;
	uint32_t nGroupCount;
	uint32_t nAggregateFlip;
	uint32_t nAggregateFlipCount;
	uint32_t nAggregateFrames;
	
	uint32_t nDisplay;
	uint32_t nBars;
	uint32_t nActiveGroup;
	uint32_t nDirty;
	uint32_t nStoredGroup;
	uint32_t nFlipLog;
	float fGraphBaseTime;
	float fGraphBaseTimePos;

	uint32_t nWidth;
	uint32_t nHeight;

	uint32_t nBarWidth;
	uint32_t nBarHeight;

	uint64_t nFrameStartPrev;
	uint64_t nFrameStart;

	uint64_t nFrameStartLog;
	uint64_t nFrameEndLog;

	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	
	MicroProfileTimer 		FrameTimers[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		AggregateTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				MaxTimers[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMax[MICROPROFILE_MAX_TIMERS];
	uint64_t 				nStart[MICROPROFILE_MAX_TIMERS];



	MicroProfileGraphState	Graph[MICROPROFILE_MAX_GRAPHS];
	uint32_t				nGraphPut;

	uint32_t 				nMouseX;
	uint32_t 				nMouseY;

	MicroProfileLogEntry	Log[MICROPROFIL_LOG_BUFFER_SIZE];
	uint32_t 				nLogPos;

	MicroProfileLogEntry	LogDisplay[MICROPROFIL_LOG_BUFFER_SIZE];
	uint32_t 				nLogPosDisplay;



} g_MicroProfile;

static uint32_t 				g_nMicroProfileBackColors[2] = {  0x474747, 0x313131 };

template<typename T>
T MicroProfileMin(T a, T b)
{ return a < b ? a : b; }

template<typename T>
T MicroProfileMax(T a, T b)
{ return a > b ? a : b; }



void MicroProfileInit()
{
	static bool bOnce = true;
	if(bOnce)
	{
		bOnce = false;
		memset(&S, 0, sizeof(S));
		S.nGroupCount = 1;//0 is for inactive
		S.nBarWidth = 100;
		S.nBarHeight = MICROPROFILE_TEXT_HEIGHT;
		S.nActiveGroup = 1;
		S.GroupInfo[0].pName = "___ROOT";
		S.nAggregateFlip = 30;
		S.TimerInfo[0].pName = "Frame Time";
		S.TimerInfo[0].nColor = (uint32_t)-1;
		S.TimerInfo[0].nToken = 0;
		S.nTotalTimers = 1;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
		S.Graph[0].nToken = 0;
		S.nBars = MP_DRAW_ALL;
		S.nFlipLog = 1;
		S.fGraphBaseTime = 40.f;
		S.nWidth = 100;
		S.nHeight = 100;
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
		S.GroupInfo[S.nGroupCount].nNumTimers = 1;
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

void MicroProfileEnter(MicroProfileToken nToken_)
{
	if(S.nActiveGroup == 0xffff || MicroProfileGetGroupIndex(nToken_) == S.nActiveGroup)
	{
		MicroProfileToken nToken = nToken_ & 0xffff;
		MP_ASSERT(0 == S.nStart[nToken]);
		uint64_t nTick = TICK();
		S.nStart[nToken] = nTick;
		ZASSERT(S.nLogPos != MICROPROFIL_LOG_BUFFER_SIZE);
		if(S.nActiveGroup == 0xffff && S.nLogPos != MICROPROFIL_LOG_BUFFER_SIZE)
		{
			S.Log[S.nLogPos].nToken = nToken_;
			S.Log[S.nLogPos].nTick = nTick;
			S.Log[S.nLogPos].eType = MicroProfileLogEntry::EEnter;
			S.nLogPos++;
		}
	}
}

void MicroProfileLeave(MicroProfileToken nToken_)
{
	if(S.nActiveGroup == 0xffff || MicroProfileGetGroupIndex(nToken_) == S.nActiveGroup)
	{
		MicroProfileToken nToken = nToken_ & 0xffff;
		//nToken &= 0xffff;
		uint64_t nTick = TICK();
		S.FrameTimers[nToken].nTicks += nTick - S.nStart[nToken];
		S.FrameTimers[nToken].nCount++;
		S.nStart[nToken] = 0;

		if(S.nActiveGroup == 0xffff && S.nLogPos != MICROPROFIL_LOG_BUFFER_SIZE)
		{
			S.Log[S.nLogPos].nToken = nToken_;
			S.Log[S.nLogPos].nTick = nTick;
			S.Log[S.nLogPos].eType = MicroProfileLogEntry::ELeave;
			S.nLogPos++;
		}		
	}
}


void MicroProfileFlip()
{
	S.FrameTimers[0].nTicks += TICK() - S.nStart[0];
	S.FrameTimers[0].nCount++;
	S.nStart[0] = TICK();


	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileFlip", 0x3355ee);
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		S.AggregateTimers[i].nTicks += S.FrameTimers[i].nTicks;
		S.AggregateTimers[i].nCount += S.FrameTimers[i].nCount;
		S.Frame[i].nTicks = S.FrameTimers[i].nTicks;
		S.Frame[i].nCount = S.FrameTimers[i].nCount;
		S.MaxTimers[i] = MicroProfileMax(S.MaxTimers[i], S.FrameTimers[i].nTicks);
		S.FrameTimers[i].nTicks = 0;
		S.FrameTimers[i].nCount = 0;
	}

	bool bFlipAggregate = false;
	uint32_t nFlipFrequency = S.nAggregateFlip ? S.nAggregateFlip : 30;
	if(S.nAggregateFlip <= ++S.nAggregateFlipCount)
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
		if(S.nFlipLog || 0 == S.nLogPosDisplay)
		{			
			memcpy(&S.LogDisplay[0], &S.Log[0], sizeof(S.Log[0]) * S.nLogPos);
			S.nLogPosDisplay = S.nLogPos;
			S.nFrameStartLog = S.nFrameStart;
			S.nFrameEndLog = TICK();
		}
	}
	memset(&S.FrameTimers[0], 0, sizeof(S.FrameTimers[0]) * S.nTotalTimers);
	S.nLogPos = 0;
	S.nFrameStartPrev = S.nFrameStart;
	S.nFrameStart = TICK();


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
	S.nDirty = 1;
	uint32_t nCurrent = S.nAggregateFlip;
	uint32_t nBest = g_AggregatePresets[(sizeof(g_AggregatePresets)/sizeof(g_AggregatePresets[0]))-1];

	for(uint32_t nPreset : g_AggregatePresets)
	{
		if(nPreset < nCurrent)
		{
			nBest = nPreset;
		}
	}
	S.nAggregateFlip = nBest;
}

void MicroProfileNextGroup()
{
	if(0xffff == S.nActiveGroup) return;
	S.nActiveGroup = (S.nActiveGroup+1)%S.nGroupCount;
	MicroProfileClearGraph();
}
void MicroProfilePrevGroup()
{
	if(0xffff == S.nActiveGroup) return;
	S.nActiveGroup = (S.nActiveGroup+S.nGroupCount-1)%S.nGroupCount;
	MicroProfileClearGraph();
}

void MicroProfileToggleDisplayMode()
{
	switch(S.nDisplay)
	{
		case 2:
		{
			S.nDisplay = 0;
			S.nActiveGroup = S.nStoredGroup;
		}
		break;
		case 1:
		{
			S.nDisplay = 2;
			S.nStoredGroup = S.nActiveGroup;
			S.nActiveGroup = 0xffff;
		}
		break;		
		case 0:
		{
			S.nDisplay = 1;
		}
		break;		
	}
}
void MicroProfileToggleTimers()
{
	S.nBars ^= MP_DRAW_TIMERS;
}
void MicroProfileToggleAverageTimers()
{
	S.nBars ^= MP_DRAW_AVERAGE;
}
void MicroProfileToggleMaxTimers()
{
	S.nBars ^= MP_DRAW_MAX;
}

void MicroProfileToggleCallCount()
{
	S.nBars ^= MP_DRAW_CALL_COUNT;
}


void MicroProfileDrawFloatInfo(uint32_t nX, uint32_t nY, uint32_t nToken, uint64_t nTime)
{
#define MAX_STRINGS 32
	uint32_t nIndex = MicroProfileGetTimerIndex(nToken);
	uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
	uint32_t nAggregateCount = S.Aggregate[nIndex].nCount ? S.Aggregate[nIndex].nCount : 1;

	float fMs = MP_TICK_TO_MS(nTime);
	float fFrameMs = MP_TICK_TO_MS(S.Frame[nIndex].nTicks);
	float fAverage = MP_TICK_TO_MS(S.Aggregate[nIndex].nTicks/nAggregateFrames);
	float fCallAverage = MP_TICK_TO_MS(S.Aggregate[nIndex].nTicks / nAggregateCount);
	float fMax = MP_TICK_TO_MS(S.AggregateMax[nIndex]);

	{
		const char* ppStrings[MAX_STRINGS];
		uint32_t nColors[MAX_STRINGS];
		memset(&nColors[0], 0xff, sizeof(nColors));
		char buffer[1024];
		const char* pBufferStart = &buffer[0];
		char* pBuffer = &buffer[0];

		uint32 nGroupId = MicroProfileGetGroupIndex(nToken);
		uint32 nTimerId = MicroProfileGetTimerIndex(nToken);
		const char* pGroupName = S.GroupInfo[nGroupId].pName;
		const char* pTimerName = S.TimerInfo[nTimerId].pName;
#define SZ (intptr)(sizeof(buffer)-1-(pBufferStart - pBuffer))
		uint32_t nTextCount = 0;


		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer,SZ, "%s", pGroupName);
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer,SZ, "%s", pTimerName);
		
		if(nTime != (uint64_t)-1)
		{
			ppStrings[nTextCount++] = "Time:";
			ppStrings[nTextCount++] = pBuffer;
			pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fMs);
			ppStrings[nTextCount++] = "";
			ppStrings[nTextCount++] = "";
		}

		ppStrings[nTextCount++] = "Frame Time:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fFrameMs);

		ppStrings[nTextCount++] = "Frame Call Average:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fCallAverage);

		ppStrings[nTextCount++] = "Frame Call Count:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ, "%6d",  nAggregateCount / nAggregateFrames);
		
		ppStrings[nTextCount++] = "Average:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fAverage);

		ppStrings[nTextCount++] = "Max:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fMax);
		
		ZASSERT(nTextCount < MAX_STRINGS);
		nTextCount /= 2;
#undef SZ
		uint32_t nWidth = 0;
		uint32_t nStringLengths[MAX_STRINGS];
		for(uint32_t i = 0; i < nTextCount; ++i)
		{
			uint32_t i0 = i * 2;
			uint32_t s0, s1;
			nStringLengths[i0] = s0 = strlen(ppStrings[i0]);
			nStringLengths[i0+1] = s1 = strlen(ppStrings[i0+1]);
			nWidth = MicroProfileMax(s0+s1, nWidth);
		}
		nWidth = (MICROPROFILE_TEXT_WIDTH+1) * (2+nWidth) + 2 * MICROPROFILE_BORDER_SIZE;
		uint32_t nHeight = (MICROPROFILE_TEXT_HEIGHT+1) * nTextCount + 2 * MICROPROFILE_BORDER_SIZE;
		nY += 20;
		if(nX + nWidth > S.nWidth)
			nX = S.nWidth - nWidth;
		if(nY + nHeight > S.nHeight)
			nY = S.nHeight - nHeight;
		MicroProfileDrawBox(nX, nY, nWidth, nHeight, 0);
		for(uint32_t i = 0; i < nTextCount; ++i)
		{
			int i0 = i * 2;
			MicroProfileDrawText(nX + 1, nY + 1 + (MICROPROFILE_TEXT_HEIGHT+1) * i, (uint32_t)-1, ppStrings[i0]);
			MicroProfileDrawText(nX + nWidth - nStringLengths[i0+1] * (MICROPROFILE_TEXT_WIDTH+1), nY + 1 + (MICROPROFILE_TEXT_HEIGHT+1) * i, (uint32_t)-1, ppStrings[i0+1]);
		}

	}
#undef MAX_STRINGS
}
void MicroProfileDrawDetailedView(uint32_t nWidth, uint32_t nHeight)
{
	uint64_t nFrameEnd = S.nFrameEndLog;
	uint64_t nFrameStart = S.nFrameStartLog;
#define DETAILED_STACK_MAX 64
	uint32_t nStack[DETAILED_STACK_MAX];
	uint32_t nStackPos = 0;

	uint32_t nX = 0;
	uint32_t nY = S.nBarHeight + 1;

	float fMsBase = S.fGraphBaseTimePos;
	float fMs = S.fGraphBaseTime;
	float fMsEnd = fMs + fMsBase;
	float fWidth = nWidth;
	float fMsToScreen = fWidth / fMs;

	int nColorIndex = floor(fMsBase);
	int i = 0;
	int nSkip = (fMsEnd-fMsBase)> 50 ? 2 : 1;
	for(float f = fMsBase; f < fMsEnd; ++i)
	{
		float fStart = f;
		float fNext = MicroProfileMin<float>(floor(f)+1.f, fMsEnd);
		uint32_t nXPos = nX + ((fStart-fMsBase) * fMsToScreen);
		MicroProfileDrawBox(nXPos, nY, (fNext-fMsBase) * fMsToScreen+1, nHeight, g_nMicroProfileBackColors[nColorIndex++ & 1]);
		if(0 == (i%nSkip))
		{
			char buf[10];
			snprintf(buf, 9, "%d", (int)f);
			MicroProfileDrawText(nXPos, nY, (uint32_t)-1, buf);
		}
		f = fNext;
	}

	float fMouseX = S.nMouseX;
	float fMouseY = S.nMouseY;
	uint32_t nHoverToken = -1;
	uint64_t nHoverTime = 0;
	float fHoverDist = 0.5f;
	float fBestDist = 100000.f;

	for(uint32_t i = 0; i < S.nLogPosDisplay; ++i)
	{
		MicroProfileLogEntry& LE = S.LogDisplay[i];
		switch(LE.eType)
		{
			case MicroProfileLogEntry::EEnter:
			{
				MP_ASSERT(nStackPos < DETAILED_STACK_MAX);
				nStack[nStackPos++] = i;
			}
			break;
			case MicroProfileLogEntry::ELeave:
			{
				if(0 == nStackPos)
					continue;
				MP_ASSERT(S.LogDisplay[nStack[nStackPos-1]].nToken == LE.nToken);
				uint64_t nTickStart = S.LogDisplay[nStack[nStackPos-1]].nTick;
				uint64_t nTickEnd = LE.nTick;
				uint32_t nColor = S.TimerInfo[ LE.nToken & 0xffff].nColor;

				float fMsStart = MP_TICK_TO_MS(nTickStart - nFrameStart) - fMsBase;
				float fMsEnd = MP_TICK_TO_MS(nTickEnd - nFrameStart) - fMsBase;
				ZASSERT(fMsStart <= fMsEnd);
				float fXStart = nX + fMsStart * fMsToScreen;
				float fXEnd = nX + fMsEnd * fMsToScreen;

				float fYStart = nY + nStackPos * (MICROPROFILE_DETAILED_BAR_HEIGHT+1);
				float fYEnd = fYStart + (MICROPROFILE_DETAILED_BAR_HEIGHT+1);
				uint32_t nIntegerWidth = fXEnd - fXStart;
				float fXDist = MicroProfileMax(fXStart - fMouseX, fMouseX - fXEnd);

				if(fXDist < fHoverDist)
				{
					if(fYStart <= fMouseY && fMouseY <= fYEnd)
					{
						fHoverDist = fXDist;
						nHoverToken = LE.nToken;
						nHoverTime = nTickEnd - nTickStart;
					}
				}

				if(nIntegerWidth)
				{
					MicroProfileDrawBox(fXStart, fYStart, fXEnd - fXStart, MICROPROFILE_DETAILED_BAR_HEIGHT, nColor);
				}
				else
				{
					float fXAvg = 0.5f * (fXStart + fXEnd);
					float fLine[] = {
						fXAvg, fYStart + 0.5f,
						fXAvg, fYEnd - 0.5f

					};
					MicroProfileDrawLine2D(2, &fLine[0], nColor);
				}
				nStackPos--;
			}
			break;
		}

	}
	#undef DETAILED_STACK_MAX
	if(nHoverToken != (uint32)-1 && nHoverTime)
		MicroProfileDrawFloatInfo(S.nMouseX, S.nMouseY, nHoverToken, nHoverTime);
}




void MicroProfileCalcTimers(float* pTimers, float* pAverage, float* pMax, float* pCallAverage, uint16_t nGroup)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileCalcTimers", 0x773300);

	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(0 == i || MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == nGroup || nGroup == 0xffff)
		{
			uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
			uint32_t nAggregateCount = S.Aggregate[i].nCount ? S.Aggregate[i].nCount : 1;

			float fMs = TickToNs(S.Frame[i].nTicks) / 1000000.f;
			float fPrc = MicroProfileMin(fMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
			float fAverageMs = TickToNs(S.Aggregate[i].nTicks / nAggregateFrames) / 1000000.f;
			float fAveragePrc = MicroProfileMin(fAverageMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
			float fMaxMs = TickToNs(S.AggregateMax[i]) / 1000000.f;
			float fMaxPrc = MicroProfileMin(fMaxMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
			float fCallAverageMs = TickToNs(S.Aggregate[i].nTicks / nAggregateCount) / 1000000.f;
			float fCallAveragePrc = MicroProfileMin(fCallAverageMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
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

uint32_t MicroProfileDrawBarArray(uint32_t nX, uint32_t nY, uint32_t nGroup, float* pTimers, const char* pName, uint32_t nNumTimers)
{
	uint32_t nHeight = S.nBarHeight;
	uint32_t nWidth = S.nBarWidth;
	uint32_t tidx = 0;
	uint32_t nTextWidth = 6 * MICROPROFILE_TEXT_WIDTH;
	float fWidth = S.nBarWidth;
	#define SBUF_MAX 32
	char sBuffer[SBUF_MAX];
	MicroProfileDrawText(nX, nY, (uint32_t)-1, pName);
	nY += nHeight + 2;
	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(0 == i || MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == nGroup || nGroup == 0xffff)
		{
			snprintf(sBuffer, SBUF_MAX-1, "%5.2f", pTimers[tidx]);
			MicroProfileDrawBox(nX + nTextWidth, nY, fWidth * pTimers[tidx+1], nHeight, S.TimerInfo[i].nColor);
			MicroProfileDrawText(nX, nY, (uint32_t)-1, sBuffer);
			nY += nHeight + 1;
			tidx += 2;
		}
	}
	return nWidth + 5 + nTextWidth;
}

uint32_t MicroProfileDrawBarCallCount(uint32_t nX, uint32_t nY, uint32_t nGroup, const char* pName, uint32_t nNumTimers)
{
	uint32_t nHeight = S.nBarHeight;
	uint32_t nWidth = S.nBarWidth;
	uint32_t tidx = 0;
	uint32_t nTextWidth = 6 * MICROPROFILE_TEXT_WIDTH;
	float fWidth = S.nBarWidth;
	#define SBUF_MAX 32
	char sBuffer[SBUF_MAX];
	MicroProfileDrawText(nX, nY, (uint32_t)-1, pName);
	nY += nHeight + 2;
	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(0 == i || MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == nGroup || nGroup == 0xffff)
		{
			snprintf(sBuffer, SBUF_MAX-1, "%5d", S.Frame[i].nCount);
			MicroProfileDrawText(nX, nY, (uint32_t)-1, sBuffer);
			nY += nHeight + 1;
			tidx += 2;
		}
	}
	return 5 + nTextWidth;
}



uint32_t MicroProfileDrawBarLegend(uint32_t nX, uint32_t nY, uint32_t nGroup, uint32_t nScreenWidth, uint32_t nScreenHeight, uint32_t nNumTimers)
{
	uint32_t nHeight = S.nBarHeight;
	uint32_t nWidth = S.nBarWidth;
	uint32_t tidx = 0;
	char sBuffer[SBUF_MAX];
	nY += nHeight + 2;
	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(0 == i || MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == nGroup || nGroup == 0xffff)
		{
			MicroProfileDrawText(nX, nY, S.TimerInfo[i].nColor, S.TimerInfo[i].pName);
			nY += nHeight + 1;
			tidx += 2;
		}
	}
	return nX;
}

void MicroProfileDrawGraph(uint32_t nScreenWidth, uint32_t nScreenHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "DrawGraph", 0xff0033);
	uint32_t nX = nScreenWidth - MICROPROFILE_GRAPH_WIDTH;
	uint32_t nY = nScreenHeight - MICROPROFILE_GRAPH_HEIGHT;
	MicroProfileDrawBox(nX, nY, MICROPROFILE_GRAPH_WIDTH, MICROPROFILE_GRAPH_HEIGHT, 0x0);

	
	float fY = nScreenHeight;
	float fDX = MICROPROFILE_GRAPH_WIDTH * 1.f / MICROPROFILE_GRAPH_HISTORY;  
	float fDY = MICROPROFILE_GRAPH_HEIGHT;

	uint32_t nPut = S.nGraphPut;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		if(S.Graph[i].nToken != MICROPROFILE_INVALID_TOKEN)
		{
			float fX = nX;
			float* pGraphData = (float*)alloca(sizeof(float)* MICROPROFILE_GRAPH_HISTORY*2);
			for(uint32_t j = 0; j < MICROPROFILE_GRAPH_HISTORY; ++j)
			{
				float fWeigth = S.Graph[i].fHistory[(j+nPut)%MICROPROFILE_GRAPH_HISTORY];
				pGraphData[(j*2)] = fX;
				pGraphData[(j*2)+1] = fY - fDY * fWeigth;
				fX += fDX;
			}
			MicroProfileDrawLine2D(MICROPROFILE_GRAPH_HISTORY, pGraphData, S.TimerInfo[MicroProfileGetTimerIndex(S.Graph[i].nToken)].nColor);
		}
	}

}

void MicroProfileDrawBarView(uint32_t nScreenWidth, uint32_t nScreenHeight)
{
	if(!S.nActiveGroup)
		return;

	MICROPROFILE_SCOPEI("MicroProfile", "DrawBarView", 0x00dd77);
	uint32_t nNumTimers = S.GroupInfo[S.nActiveGroup].nNumTimers;
	uint32_t nBlockSize = 2 * nNumTimers;
	float* pTimers = (float*)alloca(nBlockSize * 4 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pCallAverage = pTimers + 3 * nBlockSize;
	MicroProfileCalcTimers(pTimers, pAverage, pMax, pCallAverage, S.nActiveGroup);

	uint32_t nHeight = S.nBarHeight;
	uint32_t nWidth = S.nBarWidth;
	// uint32_t nX = 10;
	// uint32_t nY = 10 + nHeight + 1;

	uint32_t nX = 0;
	uint32_t nY = nHeight + 1;
	int nColorIndex = 0;

	for(uint32_t i = 0; i < nNumTimers+1; ++i)
	{
		MicroProfileDrawBox(nX, nY + i * (nHeight + 1), nScreenWidth, (nHeight+1)+1, g_nMicroProfileBackColors[nColorIndex++ & 1]);
	}

	if(S.nBars & MP_DRAW_TIMERS)		
		nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pTimers, "Time", nNumTimers) + 1;
	if(S.nBars & MP_DRAW_AVERAGE)		
		nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pAverage, "Average", nNumTimers) + 1;
	if(S.nBars & MP_DRAW_MAX)		
		nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pMax, "Max Time", nNumTimers) + 1;
	if(S.nBars & MP_DRAW_CALL_COUNT)		
	{
		nX += MicroProfileDrawBarArray(nX, nY, S.nActiveGroup, pCallAverage, "Call Average", nNumTimers) + 1;
		nX += MicroProfileDrawBarCallCount(nX, nY, S.nActiveGroup, "Count", nNumTimers) + 1; 
	}
	nX += MicroProfileDrawBarLegend(nX, nY, S.nActiveGroup, nScreenWidth, nScreenHeight, nNumTimers) + 1;


	uint32_t tidx = 0;
	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		if(0 == i || MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == S.nActiveGroup || S.nActiveGroup == 0xffff)
		{
			for(uint32_t j = 0; j < MICROPROFILE_MAX_GRAPHS; ++j)
			{
				if(S.Graph[j].nToken != MICROPROFILE_INVALID_TOKEN && S.TimerInfo[i].nToken == S.Graph[j].nToken)
				{
					S.Graph[j].fHistory[S.nGraphPut] = pTimers[1+tidx];
				}
			}
			tidx += 2;
		}
	}
	S.nGraphPut = (S.nGraphPut+1) % MICROPROFILE_GRAPH_HISTORY;
	MicroProfileDrawGraph(nScreenWidth, nScreenHeight);
}

void MicroProfileDraw(uint32_t nWidth, uint32_t nHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "Draw", 0x737373);
	if(!S.nDisplay)
		return;
	S.nWidth = nWidth;
	S.nHeight = nHeight;

	uint32_t nX = 0;
	uint32_t nY = 0;
	char buffer[128];
	MicroProfileDrawBox(nX, nY, nWidth, (S.nBarHeight+1)+1, g_nMicroProfileBackColors[1]);

	if(S.nDisplay & MP_DRAW_DETAILED)
	{
		snprintf(buffer, 127, "MicroProfile Detailed Aggregate:%d", S.nAggregateFlip);
		MicroProfileDrawText(nX, nY, -1, buffer);
		MicroProfileDrawDetailedView(nWidth, nHeight);
	}
	else if(0 != (S.nDisplay & MP_DRAW_BARS) && S.nBars)
	{
		snprintf(buffer, 127, "MicroProfile Group:'%s' Aggregate:%d", S.GroupInfo[S.nActiveGroup].pName, S.nAggregateFlip);
		MicroProfileDrawText(nX, nY, -1, buffer);
		MicroProfileDrawBarView(nWidth, nHeight);
	}
}
void MicroProfileMouseMove(uint32_t nX, uint32_t nY)
{
	S.nMouseX = nX;
	S.nMouseY = nY;
}
void MicroProfileClearGraph()
{
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		if(S.Graph[i].nToken != 0)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
	}
}
void MicroProfileToggleFlipDetailed()
{
	S.nFlipLog = !S.nFlipLog;
	uprintf("FLIP LOG IS %d\n", S.nFlipLog);
}

void MicroProfileToggleGraph(MicroProfileToken nToken)
{

	int32_t nMinSort = 0x7fffffff;
	int32_t nFreeIndex = -1;
	int32_t nMinIndex = 0;
	int32_t nMaxSort = 0x80000000;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		if(S.Graph[i].nToken == MICROPROFILE_INVALID_TOKEN)
			nFreeIndex = i;
		if(S.Graph[i].nToken == nToken)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
			return;
		}
		if(S.Graph[i].nKey < nMinSort)
		{
			nMinSort = S.Graph[i].nKey;
			nMinIndex = i;
		}
		if(S.Graph[i].nKey > nMaxSort)
		{
			nMaxSort = S.Graph[i].nKey;
		}
	}
	int nIndex = nFreeIndex > -1 ? nFreeIndex : nMinIndex;
	S.Graph[nIndex].nToken = nToken;
	S.Graph[nIndex].nKey = nMaxSort+1;
	memset(&S.Graph[nIndex].fHistory[0], 0, sizeof(S.Graph[nIndex].fHistory));
}
void MicroProfileMouseClick(uint32_t nLeft, uint32_t nRight)
{
	if(nLeft)
	{
		uint32_t nBaseHeight = 2*(S.nBarHeight + 1);
		int nIndex = (S.nMouseY - nBaseHeight) / (S.nBarHeight+1);
		if(nIndex < S.GroupInfo[S.nActiveGroup].nNumTimers)
		{
			MicroProfileToken nSelected = MICROPROFILE_INVALID_TOKEN;
			uint32_t nCount = 0;
			for(uint32_t i = 0; i < S.nTotalTimers;++i)
			{
				if(0 == i || MicroProfileGetGroupIndex(S.TimerInfo[i].nToken) == S.nActiveGroup || S.nActiveGroup == 0xffff)
				{
					if(nCount == nIndex)
					{
						nSelected = S.TimerInfo[i].nToken;
					}
					nCount++;
				}
			}
			if(nSelected != MICROPROFILE_INVALID_TOKEN)
				MicroProfileToggleGraph(nSelected);
		}
	}
}


void MicroProfileMoveGraph(int nZoom, int nPan)
{
	if(nZoom)
	{
		float fBasePos = S.fGraphBaseTimePos;
		float fBaseTime = S.fGraphBaseTime;
		float fMousePrc = MicroProfileMax((S.nMouseX - 10.f) / S.nWidth ,0.f);
		float fMouseTimeCenter = fMousePrc * fBaseTime + fBasePos;
		if(nZoom > 0)
		{
			S.fGraphBaseTime *= 1.05f;
			S.fGraphBaseTime = MicroProfileMin(S.fGraphBaseTime, (float)MICROPROFILE_MAX_GRAPH_TIME);
		}
		else
			S.fGraphBaseTime /= 1.05f;

		S.fGraphBaseTimePos = fMouseTimeCenter - fMousePrc * S.fGraphBaseTime;
		S.fGraphBaseTimePos = MicroProfileMax(0.f, S.fGraphBaseTimePos);
	}
	if(nPan)
	{
		S.fGraphBaseTimePos += nPan * 2 * S.fGraphBaseTime / S.nWidth;
		S.fGraphBaseTimePos = MicroProfileMax(S.fGraphBaseTimePos, 0.f);
		S.fGraphBaseTimePos = MicroProfileMin(S.fGraphBaseTimePos, MICROPROFILE_MAX_GRAPH_TIME - S.fGraphBaseTime);
	}
}


#endif