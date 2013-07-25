#pragma once
// TODO: 
// internal drawing
//  	border
//		unify / cleanup
//		faster
// presets
// exclusive time
// disabling of microprofiler.

#define MICROPROFILE_ENABLED 1
//#pragma optimize("", off)
#if 0 == MICROPROFILE_ENABLED

#define MICROPROFILE_DECLARE(var)
#define MICROPROFILE_DEFINE(var, group, name, color)
#define MICROPROFILE_DECLARE_GPU(var)
#define MICROPROFILE_DEFINE_GPU(var, group, name, color)
#define MICROPROFILE_SCOPE(var) do{}while(0)
#define MICROPROFILE_SCOPEI(group, name, color) do{}while(0)
#define MICROPROFILE_SCOPEGPU(var) do{}while(0)
#define MICROPROFILE_SCOPEGPUI(group, name, color) do{}while(0)
#define MicroProfileOnThreadCreate(foo) do{}while(0)

#else

#include <stdint.h>
#include <string.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <libkern/OSAtomic.h>
#define MP_TICK() mach_absolute_time()
//inline float MicroProfileTickToMs(int64_t nTicks)
//{
//	static mach_timebase_info_data_t sTimebaseInfo;	
//	if(sTimebaseInfo.denom == 0) 
//	{
//        (void) mach_timebase_info(&sTimebaseInfo);
//    }
//    return 0.000001f * (nTicks * sTimebaseInfo.numer / sTimebaseInfo.denom);
//}
//
//
//inline int64_t MicroProfileMsToTick(float fMs)
//{
//	static mach_timebase_info_data_t sTimebaseInfo;	
//	if(sTimebaseInfo.denom == 0) 
//	{
//        (void) mach_timebase_info(&sTimebaseInfo);
//    }
//    return fMs * 1000000.f * sTimebaseInfo.denom / sTimebaseInfo.numer;
//}
inline int64_t MicroProfileTicksPerSecondCpu()
{
	static int64_t nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		mach_timebase_info_data_t sTimebaseInfo;	
		mach_timebase_info(&sTimebaseInfo);
		nTicksPerSecond = 1000000000ll * sTimebaseInfo.denom / sTimebaseInfo.numer;
	}
	return nTicksPerSecond;
}



#define MP_BREAK() __builtin_trap()
#define MP_THREAD_LOCAL __thread
#elif defined(_WIN32)
#include <windows.h>
inline int64_t MicroProfileTicksPerSecondCpu()
{
	static int64_t nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&nTicksPerSecond);
	}
	return nTicksPerSecond;
}
inline int64_t MicroProfileGetTick()
{
	int64_t ticks;
	QueryPerformanceCounter((LARGE_INTEGER*)&ticks);
	return ticks;
}

//inline float MicroProfileTickToMs(int64_t nTicks)
//{
//	static int64_t nTicksPerSecond = 0;	
//	if(nTicksPerSecond == 0) 
//	{
//		QueryPerformanceFrequency((LARGE_INTEGER*)&nTicksPerSecond);
//	}
//	return (nTicks * 1000.f / nTicksPerSecond);
//}
//inline int64_t MicroProfileMsToTick(float fMs)
//{
//	static int64_t nTicksPerSecond = 0;	
//	if(nTicksPerSecond == 0) 
//	{
//		QueryPerformanceFrequency((LARGE_INTEGER*)&nTicksPerSecond);
//	}
//	return (int64_t)(fMs * 0.001f * nTicksPerSecond);
//}

#define MP_TICK() MicroProfileGetTick()
#define MP_BREAK() __debugbreak()
#define MP_THREAD_LOCAL __declspec(thread)
#endif

#define MP_ASSERT(a) do{if(!(a)){MP_BREAK();} }while(0)
#define MICROPROFILE_DECLARE(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE(var, group, name, color) MicroProfileToken g_mp_##var = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeCpu)
#define MICROPROFILE_DECLARE_GPU(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE_GPU(var, group, name, color) MicroProfileToken g_mp_##var = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeGpu)
#define MICROPROFILE_TOKEN_PASTE0(a, b) a ## b
#define MICROPROFILE_TOKEN_PASTE(a, b)  MICROPROFILE_TOKEN_PASTE0(a,b)
#define MICROPROFILE_SCOPE(var) MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPEI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeCpu); MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_SCOPEGPU(var) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPEGPUI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color,  MicroProfileTokenTypeGpu); MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_TEXT_WIDTH 5
#define MICROPROFILE_TEXT_HEIGHT 8
#define MICROPROFILE_DETAILED_BAR_HEIGHT 12
#define MICROPROFILE_GRAPH_WIDTH 256
#define MICROPROFILE_GRAPH_HEIGHT 256
#define MICROPROFILE_BORDER_SIZE 1
#define MICROPROFILE_MAX_GRAPH_TIME 100.f
#define MICROPROFILE_INVALID_TICK ((uint64_t)-1)
#define MICROPROFILE_GPU_FRAME_DELAY 3 //must be > 0
#define MICROPROFILE_PIX3_MODE 1
//#define MICROPROFILE_PIX3_HEIGHT 7
#define MICROPROFILE_PIX3_HEIGHT (MICROPROFILE_DETAILED_BAR_HEIGHT-4)

#define MICROPROFILE_GROUP_MASK_ALL 0xffffffffffff


typedef uint64_t MicroProfileToken;
typedef uint16_t MicroProfileGroupId;

#define MICROPROFILE_INVALID_TOKEN (uint32_t)-1

enum MicroProfileTokenType
{
	MicroProfileTokenTypeCpu,
	MicroProfileTokenTypeGpu,
};

void MicroProfileInit();
MicroProfileToken MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor, MicroProfileTokenType Token = MicroProfileTokenTypeCpu);
uint64_t MicroProfileEnter(MicroProfileToken nToken);
void MicroProfileLeave(MicroProfileToken nToken, uint64_t nTick);
uint64_t MicroProfileGpuEnter(MicroProfileToken nToken);
void MicroProfileGpuLeave(MicroProfileToken nToken, uint64_t nTick);
inline uint16_t MicroProfileGetTimerIndex(MicroProfileToken t){ return (t&0xffff); }
inline uint64_t MicroProfileGetGroupMask(MicroProfileToken t){ return ((t>>16)&MICROPROFILE_GROUP_MASK_ALL);}
inline MicroProfileToken MicroProfileMakeToken(uint64_t nGroupMask, uint16_t nTimer){ return (nGroupMask<<16) | nTimer;}

void MicroProfileFlip(); //! called once per frame.
void MicroProfileDraw(uint32_t nWidth, uint32_t nHeight); //! call if drawing microprofilers
void MicroProfileToggleGraph(MicroProfileToken nToken);
bool MicroProfileDrawGraph(uint32_t nScreenWidth, uint32_t nScreenHeight);
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
void MicroProfileMousePosition(uint32_t nX, uint32_t nY, int nWheelDelta);
void MicroProfileMouseButton(uint32_t nLeft, uint32_t nRight);
void MicroProfileOnThreadCreate(const char* pThreadName); //should be called from newly created threads

//UNDEFINED: MUST BE IMPLEMENTED ELSEWHERE
void MicroProfileDrawText(uint32_t nX, uint32_t nY, uint32_t nColor, const char* pText);
void MicroProfileDrawBox(uint32_t nX, uint32_t nY, uint32_t nWidth, uint32_t nHeight, uint32_t nColor);
void MicroProfileDrawBoxFade(uint32_t nX, uint32_t nY, uint32_t nX1, uint32_t nY1, uint32_t nColor);
void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor);
uint32_t MicroProfileGpuInsertTimeStamp();
uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey);
uint64_t MicroProfileTicksPerSecondGpu();

extern void* g_pFUUU;
struct MicroProfileScopeHandler
{
	MicroProfileToken nToken;
	uint64_t nTick;
	MicroProfileScopeHandler(MicroProfileToken Token):nToken(Token)
	{
		nTick = MicroProfileEnter(nToken);
	}
	~MicroProfileScopeHandler()
	{
		MicroProfileLeave(nToken, nTick);
	}
};

struct MicroProfileScopeGpuHandler
{
	MicroProfileToken nToken;
	uint64_t nTick;
	MicroProfileScopeGpuHandler(MicroProfileToken Token):nToken(Token)
	{
		nTick = MicroProfileGpuEnter(nToken);
	}
	~MicroProfileScopeGpuHandler()
	{
		MicroProfileGpuLeave(nToken, nTick);
	}
};



#ifdef MICRO_PROFILE_IMPL

#ifdef _WIN32
#define _VARIADIC_MAX 6 //hrmph
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <thread>
#include <mutex>
#include <atomic>


#define S g_MicroProfile
#define MICROPROFILE_MAX_TIMERS 1024
#define MICROPROFILE_MAX_GROUPS 48
#define MICROPROFILE_MAX_GRAPHS 5
#define MICROPROFILE_GRAPH_HISTORY 128
#define MICROPROFILE_BUFFER_SIZE (((2048)<<10)/sizeof(MicroProfileLogEntry))
#define MICROPROFILE_MAX_THREADS 32
#define MICROPROFILE_STACK_MAX 64
#define MICROPROFILE_MAX_PRESETS 5


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
	MicroProfileTokenType Type;
};

struct MicroProfileTimerInfo
{
	MicroProfileToken nToken;
	uint32_t nTimerIndex;
	uint32_t nGroupIndex;
	const char* pName;
	uint32_t nColor;
};

struct MicroProfileGraphState
{
	uint64_t nHistory[MICROPROFILE_GRAPH_HISTORY];
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
	uint8_t nStackDepth;
	MicroProfileToken nToken;
	union
	{
		uint64_t nTick;
		struct
		{
			uint32_t nStartRelative;
			uint32_t nEndRelative;
		};
	};
};

struct MicroProfileThreadLog
{
	MicroProfileThreadLog*  pNext;
	MicroProfileLogEntry	Log[MICROPROFILE_BUFFER_SIZE];

	std::atomic<uint32_t>	nPut;
	std::atomic<uint32_t>	nGet;
	uint32_t				nGpuGet[MICROPROFILE_GPU_FRAME_DELAY];
	uint32_t 				nActive;
	uint32_t 				nGpu;
	enum
	{
		THREAD_MAX_LEN = 64,
	};
	char					ThreadName[64];
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
	uint64_t nActiveGroup;

	//menu/mouse over stuff
	uint64_t nMenuActiveGroup;
	uint32_t nMenuAllGroups;
	uint32_t nMenuAllThreads;
	uint32_t nHoverToken;
	uint32_t nHoverTime;
	uint32_t nOverflow;

	uint64_t nGroupMask;
	uint32_t nDirty;
	uint32_t nStoredGroup;
	uint32_t nFlipLog;
	uint32_t nMaxGroupSize;
	float fGraphBaseTime;
	float fGraphBaseTimePos;
	float fReferenceTime;
	float fRcpReferenceTime;

	int nOffsetY;

	uint32_t nWidth;
	uint32_t nHeight;

	uint32_t nBarWidth;
	uint32_t nBarHeight;

	uint64_t nFrameStartCpuPrev;
	uint64_t nFrameStartCpu;
	uint32_t nFrameStartGpu[MICROPROFILE_GPU_FRAME_DELAY+1];


	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	
	MicroProfileTimer 		AggregateTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				MaxTimers[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMax[MICROPROFILE_MAX_TIMERS];

	MicroProfileGraphState	Graph[MICROPROFILE_MAX_GRAPHS];
	uint32_t				nGraphPut;

	uint32_t 				nMouseX;
	uint32_t 				nMouseY;
	int						nMouseWheelDelta;
	uint32_t				nMouseDownLeft;
	uint32_t				nMouseDownRight;
	uint32_t 				nMouseLeft;
	uint32_t 				nMouseRight;
	uint32_t 				nActiveMenu;

	uint32_t				nThreadActive[MICROPROFILE_MAX_THREADS];
	MicroProfileThreadLog* 	Pool[MICROPROFILE_MAX_THREADS];
	MicroProfileThreadLog* 	DisplayPool[MICROPROFILE_MAX_THREADS];
	uint32_t				nNumLogs;
	uint32_t 				nNumDisplayLogs;
	uint32_t 				nMemUsage;

	uint64_t DisplayPoolFrameStartCpu;
	uint64_t DisplayPoolFrameEndCpu;
	uint64_t DisplayPoolFrameStartGpu;
	uint64_t DisplayPoolFrameEndGpu;
	MicroProfileLogEntry* pDisplayMouseOver;
	



} g_MicroProfile;

MicroProfileThreadLog*			g_MicroProfileGpuLog = 0;
MP_THREAD_LOCAL MicroProfileThreadLog* g_MicroProfileThreadLog = 0;
static uint32_t 				g_nMicroProfileBackColors[2] = {  0x474747, 0x313131 };
static uint32_t g_MicroProfileAggregatePresets[] = {0, 10, 20, 30, 60, 120};
static float g_MicroProfileReferenceTimePresets[] = {5.f, 10.f, 15.f,20.f, 33.33f, 66.66f, 100.f};
static const char* g_MicroProfilePresetNames[] = 
{
	"Default",
	"Render",
	"GPU",
	"Lighting",
	"AI",
	"Visibility",
	"Sound",
};

inline std::recursive_mutex& MicroProfileMutex()
{
	static std::recursive_mutex Mutex;
	return Mutex;
}

template<typename T>
T MicroProfileMin(T a, T b)
{ return a < b ? a : b; }

template<typename T>
T MicroProfileMax(T a, T b)
{ return a > b ? a : b; }



MicroProfileThreadLog* MicroProfileCreateThreadLog(const char* pName);
void MicroProfileLoadPreset(const char* pSuffix);
void MicroProfileSavePreset(const char* pSuffix);


inline int64_t MicroProfileMsToTick(float fMs, int64_t nTicksPerSecond)
{
	return (int64_t)(fMs*0.001f*nTicksPerSecond);
}

inline float MicroProfileTickToMsMultiplier(int64_t nTicksPerSecond)
{
	return 1000.f / nTicksPerSecond;
}


inline uint16_t MicroProfileGetGroupIndex(MicroProfileToken t){ return S.TimerInfo[MicroProfileGetTimerIndex(t)].nGroupIndex;}


#include "debug.h"



void MicroProfileInit()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	static bool bOnce = true;
	if(bOnce)
	{
		S.nMemUsage += sizeof(S);
		uprintf("***** MEM USAGE %dkb\n", S.nMemUsage >> 10);
		bOnce = false;
		memset(&S, 0, sizeof(S));
		S.nGroupCount = 0;
		S.nBarWidth = 100;
		S.nBarHeight = MICROPROFILE_TEXT_HEIGHT;
		S.nActiveGroup = 1;
		S.nMenuAllGroups = 1;
		S.nMenuActiveGroup = 1;
		S.nMenuAllThreads = 1;
		S.nAggregateFlip = 30;
		S.nTotalTimers = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
		S.nBars = MP_DRAW_ALL;
		S.nFlipLog = 1;
		S.fGraphBaseTime = 40.f;
		S.nWidth = 100;
		S.nHeight = 100;
		S.nActiveMenu = (uint32_t)-1;
		S.fReferenceTime = 33.33f;
		S.fRcpReferenceTime = 1.f / S.fReferenceTime;

		memset(S.nFrameStartGpu, 0xff, sizeof(S.nFrameStartGpu));

		MicroProfileThreadLog* pGpu = MicroProfileCreateThreadLog("GPU");
		g_MicroProfileGpuLog = pGpu;
		S.Pool[S.nNumLogs++] = pGpu;
		pGpu->nGpu = 1;

	}
}


MicroProfileThreadLog* MicroProfileCreateThreadLog(const char* pName)
{
	MicroProfileThreadLog* pLog = new MicroProfileThreadLog;
	S.nMemUsage += sizeof(MicroProfileThreadLog);
	uprintf("***** MEM USAGE %dkb\n", S.nMemUsage >> 10);
	memset(pLog, 0, sizeof(*pLog));
	int len = strlen(pName);
	int maxlen = sizeof(pLog->ThreadName)-1;
	len = len < maxlen ? len : maxlen;
	memcpy(&pLog->ThreadName[0], pName, len);
	pLog->ThreadName[len] = '\0';
	return pLog;
}

void MicroProfileOnThreadCreate(const char* pThreadName)
{
	MicroProfileInit();
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	MP_ASSERT(g_MicroProfileThreadLog == 0);
	MicroProfileThreadLog* pLog = MicroProfileCreateThreadLog(pThreadName);
	MP_ASSERT(pLog);
	g_MicroProfileThreadLog = pLog;
	S.Pool[S.nNumLogs++] = pLog;
}



MicroProfileToken MicroProfileGetToken(const char* pGroup, const char* pName, uint32_t nColor, MicroProfileTokenType Type)
{
	MicroProfileInit();
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
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
		S.GroupInfo[S.nGroupCount].Type = Type;
		nGroupIndex = S.nGroupCount++;
		S.nGroupMask = (S.nGroupMask<<1)|1;
		uprintf("***** CREATED group %s index %d mask %08x\n", pGroup, nGroupIndex, 1ll << nGroupIndex);
		ZASSERT(nGroupIndex < 48 );//limit is 48 groups
	}
	uint16_t nTimerIndex = S.nTotalTimers++;
	uint64_t nGroupMask = 1ll << nGroupIndex;
	MicroProfileToken nToken = MicroProfileMakeToken(nGroupMask, nTimerIndex);
	S.GroupInfo[nGroupIndex].nNumTimers++;
	MP_ASSERT(S.GroupInfo[nGroupIndex].Type == Type); //dont mix cpu & gpu timers in the same group
	S.nMaxGroupSize = MicroProfileMax(S.nMaxGroupSize, S.GroupInfo[nGroupIndex].nNumTimers);
	S.TimerInfo[nTimerIndex].nToken = nToken;
	S.TimerInfo[nTimerIndex].pName = pName;
	S.TimerInfo[nTimerIndex].nColor = nColor;
	S.TimerInfo[nTimerIndex].nGroupIndex = nGroupIndex;
	S.nDirty = 1;

	uprintf("***** CREATED TIMER %08llx .. idx %d grp %d grp %s name %s\n", nToken, nTimerIndex, nGroupIndex, S.GroupInfo[nGroupIndex].pName, pName);
	return nToken;
}

inline void MicroProfileLogPut(MicroProfileToken nToken_, uint64_t nTick, MicroProfileLogEntry::EType eEntry, MicroProfileThreadLog* pLog)
{
	if(S.nDisplay)
	{
		MP_ASSERT(pLog != 0); //this assert is hit if MicroProfileOnCreateThread is not called
		uint32_t nPos = pLog->nPut.load(std::memory_order_relaxed);
		uint32_t nNextPos = (nPos+1) % MICROPROFILE_BUFFER_SIZE;
		if(nNextPos == pLog->nGet.load(std::memory_order_relaxed))
		{
			S.nOverflow = 100;
		}
		else
		{
			pLog->Log[nPos].nToken = nToken_;
			pLog->Log[nPos].nTick = nTick;
			pLog->Log[nPos].eType = eEntry;
			pLog->nPut.store(nNextPos, std::memory_order_release);
		}
	}		

}

uint64_t MicroProfileEnter(MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		uint64_t nTick = MP_TICK();
		MicroProfileLogPut(nToken_, nTick, MicroProfileLogEntry::EEnter, g_MicroProfileThreadLog);
		return nTick;
	}
	return MICROPROFILE_INVALID_TICK;
}



void MicroProfileLeave(MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(MICROPROFILE_INVALID_TICK != nTickStart)
	{
		uint64_t nTick = MP_TICK();
		MicroProfileLogPut(nToken_, nTick, MicroProfileLogEntry::ELeave, g_MicroProfileThreadLog);
	}
}


uint64_t MicroProfileGpuEnter(MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp();
		MicroProfileLogPut(nToken_, nTimer, MicroProfileLogEntry::EEnter, g_MicroProfileGpuLog);
		return 1;
	}
	return 0;
}

void MicroProfileGpuLeave(MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(nTickStart)
	{
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp();
		MicroProfileLogPut(nToken_, nTimer, MicroProfileLogEntry::ELeave, g_MicroProfileGpuLog);
	}
}
void MicroProfileFlip()
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileFlip", 0x3355ee);
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	{
		static int once = 0;
		if(0 == once)
		{
			uint32_t nDisplay = S.nDisplay;
			MicroProfileLoadPreset(g_MicroProfilePresetNames[0]);
			once++;
			S.nDisplay = nDisplay;// dont load display, just state
		}
	}


	uint64_t nFrameStartCpu = S.nFrameStartCpu;
	uint64_t nFrameEndCpu = MP_TICK();
	S.nFrameStartCpuPrev = S.nFrameStartCpu;
	S.nFrameStartCpu = nFrameEndCpu;

	uint32_t nQueryIndex = MicroProfileGpuInsertTimeStamp();
	uint64_t nFrameStartGpu = S.nFrameStartGpu[0] != (uint32_t)-1 ? MicroProfileGpuGetTimeStamp(S.nFrameStartGpu[0]) : 0;
	uint64_t nFrameEndGpu = S.nFrameStartGpu[1] != (uint32_t)-1 ? MicroProfileGpuGetTimeStamp(S.nFrameStartGpu[1]) : nFrameStartGpu+1;
	for(uint32_t i = 0; i < MICROPROFILE_GPU_FRAME_DELAY; ++i)
	{
		S.nFrameStartGpu[i] = S.nFrameStartGpu[i+1];
	}
	S.nFrameStartGpu[MICROPROFILE_GPU_FRAME_DELAY] = nQueryIndex;


	uint32_t nPutStart[MICROPROFILE_MAX_THREADS];
	uint32_t nGetStart[MICROPROFILE_MAX_THREADS];
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		MicroProfileThreadLog* pLog = S.Pool[i];
		if(!pLog)
		{
				nPutStart[i] = 0;
				nGetStart[i] = 0;
		}
		else
		{
			if(!pLog->nGpu)
			{
				nPutStart[i] = pLog->nPut.load(std::memory_order_acquire);
				nGetStart[i] = pLog->nGet.load(std::memory_order_relaxed);
			}
			else // GPU update is lagging 
			{
				nPutStart[i] = pLog->nGpuGet[0]; 
				nGetStart[i] = pLog->nGet.load(std::memory_order_relaxed);
			}
		}
	}
	if(S.nFlipLog)
	{
		{
			MICROPROFILE_SCOPEI("MicroProfile", "Clear", 0x3355ee);
			for(uint32_t i = 0; i < S.nTotalTimers; ++i)
			{
				S.Frame[i].nTicks = 0;
				S.Frame[i].nCount = 0;
			}
		}
		{
			MICROPROFILE_SCOPEI("MicroProfile", "ThreadLoop", 0x3355ee);
			for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
			{
				uint32_t nPut = nPutStart[i];
				uint32_t nGet = nGetStart[i];
				uint32_t nRange[2][2] = { {0, 0}, {0, 0}, };
				MicroProfileThreadLog* pLog = S.Pool[i];
				if(!pLog) continue;

				if(nPut > nGet)
				{
					nRange[0][0] = nGet;
					nRange[0][1] = nPut;
					nRange[1][0] = nRange[1][1] = 0;
				}
				else if(nPut != nGet)
				{
					MP_ASSERT(nGet != MICROPROFILE_BUFFER_SIZE);
					uint32_t nCountEnd = MICROPROFILE_BUFFER_SIZE - nGet;
					nRange[0][0] = nGet;
					nRange[0][1] = nGet + nCountEnd;
					nRange[1][0] = 0;
					nRange[1][1] = nPut;
				}
				uint32_t nMaxStackDepth = 0;
				if(0==S.nThreadActive[i] && 0==S.nMenuAllThreads)
					continue;

				uint64_t nFrameStart = pLog->nGpu ? nFrameStartGpu : nFrameStartCpu;
				uint64_t nFrameEnd = pLog->nGpu ? nFrameEndGpu : nFrameEndCpu;

				if(pLog->nGpu)
				{
					for(uint32_t j = 0; j < 2; ++j)
					{
						uint32_t nStart = nRange[j][0];
						uint32_t nEnd = nRange[j][1];
						for(uint32_t k = nStart; k < nEnd; ++k)
						{
							
							int64_t nRet = MicroProfileGpuGetTimeStamp((uint32_t)pLog->Log[k].nTick);
							pLog->Log[k].nTick = nRet;

						}
					}
				}
				uint32_t nStack[MICROPROFILE_STACK_MAX];
				uint32_t nStackPos = 0;

				for(uint32_t j = 0; j < 2; ++j)
				{
					uint32_t nStart = nRange[j][0];
					uint32_t nEnd = nRange[j][1];
					for(uint32_t k = nStart; k < nEnd; ++k)
					{
						MicroProfileLogEntry& LE = pLog->Log[k];
						switch(LE.eType)
						{
							case MicroProfileLogEntry::EEnter:
							{
								MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);
								nStack[nStackPos++] = k;
							}
							break;
							case MicroProfileLogEntry::ELeave:
							{
								uint64_t nTickStart = 0 != nStackPos ? pLog->Log[nStack[nStackPos-1]].nTick : nFrameStart;
								if(0 != nStackPos)
								{
									MP_ASSERT(pLog->Log[nStack[nStackPos-1]].nToken == LE.nToken);
									nStackPos--;
								}
								uint64_t nTicks = LE.nTick - nTickStart;
								uint32_t nTimerIndex = MicroProfileGetTimerIndex(LE.nToken);
								S.Frame[nTimerIndex].nTicks += nTicks;
								S.Frame[nTimerIndex].nCount += 1;
							}
						}
					}
				}
				for(uint32_t j = 0; j < nStackPos; ++j)
				{
					MicroProfileLogEntry& LE = pLog->Log[nStack[j]];
					uint64_t nTicks = nFrameEnd - LE.nTick;
					uint32_t nTimerIndex = LE.nToken&0xffff;
					S.Frame[nTimerIndex].nTicks += nTicks;
				}
			}
		}
		{
			MICROPROFILE_SCOPEI("MicroProfile", "Accumulate", 0x3355ee);
			for(uint32_t i = 0; i < S.nTotalTimers; ++i)
			{
				S.AggregateTimers[i].nTicks += S.Frame[i].nTicks;
				S.AggregateTimers[i].nCount += S.Frame[i].nCount;
				S.MaxTimers[i] = MicroProfileMax(S.MaxTimers[i], S.Frame[i].nTicks);
			}
		}
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			if(S.Graph[i].nToken != MICROPROFILE_INVALID_TOKEN)
			{
				MicroProfileToken nToken = S.Graph[i].nToken;
				S.Graph[i].nHistory[S.nGraphPut] = S.Frame[MicroProfileGetTimerIndex(nToken)].nTicks;
			}
		}
		S.nGraphPut = (S.nGraphPut+1) % MICROPROFILE_GRAPH_HISTORY;

	}


	bool bFlipAggregate = false;
	uint32_t nFlipFrequency = S.nAggregateFlip ? S.nAggregateFlip : 30;
	if(S.nFlipLog && S.nAggregateFlip <= ++S.nAggregateFlipCount)
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
		if(S.nFlipLog)
		{
			S.pDisplayMouseOver = 0;
			for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
			{
				uint32_t nPut = nPutStart[i];
				uint32_t nGet = nGetStart[i];
				MicroProfileThreadLog* pLog = S.Pool[i];
				if(!pLog)
					continue;

				uint64_t nFrameStart = pLog->nGpu ? nFrameStartGpu : nFrameStartCpu;
				uint64_t nFrameEnd = pLog->nGpu ? nFrameEndGpu : nFrameEndCpu;

				uint32_t nRanges[2][2] = 
				{{0,0},{0,0},};
				if(nPut > nGet)
				{
					nRanges[0][0] = nGet;
					nRanges[0][1] = nPut;
				}
				else if(nPut != nGet)
				{
					nRanges[0][0] = nGet;
					nRanges[0][1] = MICROPROFILE_BUFFER_SIZE;
					nRanges[1][0] = 0;
					nRanges[1][1] = nPut;
				}
				MicroProfileLogEntry StartEntries[MICROPROFILE_STACK_MAX];
				uint32_t nStartEntryPos = MICROPROFILE_STACK_MAX;
	
				uint32_t nStack[MICROPROFILE_STACK_MAX];
				uint32_t nStackPos = 0;
				for(uint32_t j = 0; j < 2; ++j)
				{
					uint32_t nStart = nRanges[j][0];
					uint32_t nEnd = nRanges[j][1];
					for(uint32_t k = nStart; k != nEnd; ++k)
					{
						MicroProfileLogEntry& LE = pLog->Log[k];
						switch(LE.eType)
						{
							case MicroProfileLogEntry::EEnter:
							{
								MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);
								nStack[nStackPos++] = k;
							}
							break;
							case MicroProfileLogEntry::ELeave:
							{
								if(0 == nStackPos)
								{
									MP_ASSERT(nStartEntryPos);
									nStartEntryPos--;
									StartEntries[nStartEntryPos].eType = MicroProfileLogEntry::EEnter;
									StartEntries[nStartEntryPos].nToken = LE.nToken;						
									StartEntries[nStartEntryPos].nStartRelative = 0;
									StartEntries[nStartEntryPos].nEndRelative = LE.nTick < nFrameStart ? 1 : LE.nTick - nFrameStart;

								}
								else
								{
									MP_ASSERT(pLog->Log[nStack[nStackPos-1]].nToken == LE.nToken);
									uint32_t nStartIndex = nStack[nStackPos-1];
									MicroProfileLogEntry& LEEnter = pLog->Log[nStartIndex];
									uint64_t nTickStart = LEEnter.nTick;
									uint64_t nTickEnd = LE.nTick;
									LEEnter.nToken = LEEnter.nToken;
									LEEnter.nStackDepth = nStackPos;
									LEEnter.nStartRelative = nTickStart - nFrameStart;
									LEEnter.nEndRelative = nTickEnd - nFrameStart;
									MP_ASSERT(nTickStart <= nTickEnd);
									nStackPos--;
								}
							}
							break;
						}
					}
				}
				for(uint32_t i = 0; i < nStackPos; ++i)
				{
					MicroProfileLogEntry& LEEnter = pLog->Log[nStack[i]];
					uint64_t nTickStart = LEEnter.nTick;
					uint64_t nTickEnd = nFrameEnd;
					if(nTickEnd < nTickStart)
						nTickEnd = nTickStart+1;
					LEEnter.nStackDepth = i;
					LEEnter.nStartRelative = nTickStart - nFrameStart;
					LEEnter.nEndRelative = nTickEnd - nFrameStart;
					MP_ASSERT(nTickStart <= nTickEnd);
				}
				uint32_t nOut = 0;
				MicroProfileThreadLog* pDest = S.DisplayPool[i];
				if(!pDest)
				{
					S.DisplayPool[i] = new MicroProfileThreadLog;
					S.nMemUsage += sizeof(MicroProfileThreadLog);
					uprintf("***** MEM USAGE %dkb\n", S.nMemUsage >> 10);
					pDest = S.DisplayPool[i];
				}
				uint32_t nStackDepth = 0;
				if(MICROPROFILE_STACK_MAX != nStartEntryPos)
				{
					uint32_t nCount = (MICROPROFILE_STACK_MAX-nStartEntryPos);
					for(uint32_t j = nStartEntryPos; j < MICROPROFILE_STACK_MAX; ++j)
					{
						MicroProfileLogEntry& Out = pDest->Log[nOut++];
						Out.eType = MicroProfileLogEntry::EEnter;
						Out.nToken = StartEntries[j].nToken;
						Out.nStackDepth = nStackDepth++;
						Out.nStartRelative = StartEntries[j].nStartRelative;
						Out.nEndRelative = StartEntries[j].nEndRelative;
					}
				}
				for(uint32_t j = 0; j < 2; ++j)
				{
					uint32_t nStart = nRanges[j][0];
					uint32_t nEnd = nRanges[j][1];
					for(uint32_t k = nStart; k != nEnd; ++k)
					{
						MicroProfileLogEntry& LE = pLog->Log[k];
						switch(LE.eType)
						{
							case MicroProfileLogEntry::EEnter:
								pDest->Log[nOut] = LE;
								pDest->Log[nOut++].nStackDepth = nStackDepth++;
								break;
							case MicroProfileLogEntry::ELeave:
								nStackDepth--;
								break;
						}
					}
				}
				pDest->nGet.store(0, std::memory_order_relaxed);
				pDest->nPut.store(nOut, std::memory_order_relaxed);
				//pDest->nOwningThread = S.Pool[i].nOwningThread;
				pDest->nGpu = pLog->nGpu;
				memcpy(&pDest->ThreadName[0], &pLog->ThreadName[0], sizeof(pLog->ThreadName));
			}
			S.DisplayPoolFrameStartCpu = nFrameStartCpu;
			S.DisplayPoolFrameEndCpu = nFrameEndCpu;
			S.DisplayPoolFrameStartGpu = nFrameStartGpu;
			S.DisplayPoolFrameEndGpu = nFrameEndGpu;
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		MicroProfileThreadLog* pLog = S.Pool[i];
		if(pLog)
		{
			if(!pLog->nGpu)
			{
				pLog->nGet.store(nPutStart[i], std::memory_order_release);
			}
			else
			{
				pLog->nGet.store(pLog->nGpuGet[0], std::memory_order_release);
				for(uint32_t j = 0; j < MICROPROFILE_GPU_FRAME_DELAY-1; ++j)
				{
					pLog->nGpuGet[j] = pLog->nGpuGet[j+1];
				}
				pLog->nGpuGet[MICROPROFILE_GPU_FRAME_DELAY-1] =  pLog->nPut.load(std::memory_order_relaxed);
			}
		}
	}
}




void MicroProfileSetAggregateCount(uint32_t nCount)
{
	S.nAggregateFlip = nCount;
}
void MicroProfileNextAggregatePreset()
{
	uint32_t nNext = g_MicroProfileAggregatePresets[0];
	uint32_t nCurrent = S.nAggregateFlip;
	for(uint32_t nPreset : g_MicroProfileAggregatePresets)
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
	uint32_t nBest = g_MicroProfileAggregatePresets[(sizeof(g_MicroProfileAggregatePresets)/sizeof(g_MicroProfileAggregatePresets[0]))-1];

	for(uint32_t nPreset : g_MicroProfileAggregatePresets)
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
	if(MICROPROFILE_GROUP_MASK_ALL == S.nActiveGroup) return;
	S.nActiveGroup = (S.nActiveGroup+1)%S.nGroupCount;
	MicroProfileClearGraph();
}
void MicroProfilePrevGroup()
{
	if(MICROPROFILE_GROUP_MASK_ALL == S.nActiveGroup) return;
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
			S.nActiveGroup = MICROPROFILE_GROUP_MASK_ALL;
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


void MicroProfileDrawFloatWindow(uint32_t nX, uint32_t nY, const char** ppStrings, uint32_t nNumStrings, uint32_t* pColors = 0)
{
	uint32_t nWidth = 0;
	uint32_t* nStringLengths = (uint32_t*)alloca(nNumStrings * sizeof(uint32_t));
	uint32_t nTextCount = nNumStrings/2;
	for(uint32_t i = 0; i < nTextCount; ++i)
	{
		uint32_t i0 = i * 2;
		uint32_t s0, s1;
		nStringLengths[i0] = s0 = strlen(ppStrings[i0]);
		nStringLengths[i0+1] = s1 = strlen(ppStrings[i0+1]);
		nWidth = MicroProfileMax(s0+s1, nWidth);
	}
	nWidth = (MICROPROFILE_TEXT_WIDTH+1) * (2+nWidth) + 2 * MICROPROFILE_BORDER_SIZE;
	if(pColors)
		nWidth += MICROPROFILE_TEXT_WIDTH + 1;
	uint32_t nHeight = (MICROPROFILE_TEXT_HEIGHT+1) * nTextCount + 2 * MICROPROFILE_BORDER_SIZE;
	if(nX + nWidth > S.nWidth)
		nX = S.nWidth - nWidth;
	if(nY + nHeight > S.nHeight)
		nY = S.nHeight - nHeight;
	MicroProfileDrawBox(nX, nY, nWidth, nHeight, 0);
	if(pColors)
	{
		nX += MICROPROFILE_TEXT_WIDTH+1;
		nWidth -= MICROPROFILE_TEXT_WIDTH+1;
	}
	for(uint32_t i = 0; i < nTextCount; ++i)
	{
		int i0 = i * 2;
		if(pColors)
		{
			MicroProfileDrawBox(nX-MICROPROFILE_TEXT_WIDTH, nY, MICROPROFILE_TEXT_WIDTH, MICROPROFILE_TEXT_WIDTH, pColors[i]);
		}
		MicroProfileDrawText(nX + 1, nY + 1, (uint32_t)-1, ppStrings[i0]);
		MicroProfileDrawText(nX + nWidth - nStringLengths[i0+1] * (MICROPROFILE_TEXT_WIDTH+1), nY + 1, (uint32_t)-1, ppStrings[i0+1]);
		nY += (MICROPROFILE_TEXT_HEIGHT+1);
	}

}


void MicroProfileDrawFloatTooltip(uint32_t nX, uint32_t nY, uint32_t nToken, uint64_t nTime)
{
	uint32_t nIndex = MicroProfileGetTimerIndex(nToken);
	uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
	uint32_t nAggregateCount = S.Aggregate[nIndex].nCount ? S.Aggregate[nIndex].nCount : 1;

	uint32_t nGroupId = MicroProfileGetGroupIndex(nToken);
	uint32_t nTimerId = MicroProfileGetTimerIndex(nToken);
	bool bGpu = S.GroupInfo[nGroupId].Type == MicroProfileTokenTypeGpu;

	float fToMs = MicroProfileTickToMsMultiplier(bGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());

	float fMs = fToMs * (nTime);
	float fFrameMs = fToMs * (S.Frame[nIndex].nTicks);
	float fAverage = fToMs * (S.Aggregate[nIndex].nTicks/nAggregateFrames);
	float fCallAverage = fToMs * (S.Aggregate[nIndex].nTicks / nAggregateCount);
	float fMax = fToMs * (S.AggregateMax[nIndex]);

	{
		#define MAX_STRINGS 16
		const char* ppStrings[MAX_STRINGS];
		char buffer[1024];
		const char* pBufferStart = &buffer[0];
		char* pBuffer = &buffer[0];

		const char* pGroupName = S.GroupInfo[nGroupId].pName;
		const char* pTimerName = S.TimerInfo[nTimerId].pName;
#define SZ (intptr_t)(sizeof(buffer)-1-(pBufferStart - pBuffer))
		uint32_t nTextCount = 0;


		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer,SZ, "%s", pGroupName);
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer,SZ, "%s", pTimerName);
		
		if(nTime != (uint64_t)0)
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
		
		MP_ASSERT(nTextCount <= MAX_STRINGS);
#undef SZ
		MicroProfileDrawFloatWindow(nX, nY+20, &ppStrings[0], nTextCount);
	}
}


inline
void MicroProfileDetailedBar(float fXStart, float fYStart, float fXEnd, float fYEnd, uint32_t nColor)
{


}

void MicroProfileDrawDetailedView(uint32_t nWidth, uint32_t nHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "Detailed View", 0x8888000);
	uint64_t nFrameEndCpu = S.DisplayPoolFrameEndCpu;
	uint64_t nFrameStartCpu = S.DisplayPoolFrameStartCpu;
	uint64_t nFrameEndGpu = S.DisplayPoolFrameEndGpu;
	uint64_t nFrameStartGpu = S.DisplayPoolFrameStartGpu;

	int nX = 0;
	int nBaseY = S.nBarHeight + 1;
	int nY = nBaseY - S.nOffsetY;

	float fMsBase = S.fGraphBaseTimePos;
	uint64_t nBaseTicksCpu = MicroProfileMsToTick(fMsBase, MicroProfileTicksPerSecondCpu());
	uint64_t nBaseTicksGpu = MicroProfileMsToTick(fMsBase, MicroProfileTicksPerSecondGpu());
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
		MicroProfileDrawBox(nXPos, nBaseY, (fNext-fMsBase) * fMsToScreen+1, nHeight, g_nMicroProfileBackColors[nColorIndex++ & 1]);
		f = fNext;
	}
	nY += MICROPROFILE_TEXT_HEIGHT+1;
	MicroProfileLogEntry* pMouseOver = S.pDisplayMouseOver;
	MicroProfileLogEntry* pMouseOverNext = 0;

	float fMouseX = S.nMouseX;
	float fMouseY = S.nMouseY;
	uint32_t nHoverToken = -1;
	uint64_t nHoverTime = 0;
	float fHoverDist = 0.5f;
	float fBestDist = 100000.f;

	static int nHoverCounter = 155;
	static int nHoverCounterDelta = 10;
	nHoverCounter += nHoverCounterDelta;
	if(nHoverCounter >= 245)
		nHoverCounterDelta = -10;
	else if(nHoverCounter < 100)
		nHoverCounterDelta = 10;
	uint32_t nHoverColor = (nHoverCounter<<24)|(nHoverCounter<<16)|(nHoverCounter<<8)|nHoverCounter;

	uint32_t nLinesDrawn[MICROPROFILE_STACK_MAX]={0};
	for(uint32_t j = 0; j < MICROPROFILE_MAX_THREADS; ++j)
	{
		MicroProfileThreadLog* pLog = S.DisplayPool[j];
		if(!pLog)
			continue;
		uint32_t nSize = pLog->nPut != pLog->nGet;
		uint32_t nMaxStackDepth = 0;
		if(0 == nSize || (0==S.nThreadActive[j] && 0==S.nMenuAllThreads))
			continue;

		uint64_t nFrameStart = pLog->nGpu ? nFrameStartGpu : nFrameStartCpu;
		uint64_t nFrameEnd = pLog->nGpu ? nFrameEndGpu : nFrameEndCpu;

		bool bGpu = pLog->nGpu != 0;
		float fToMs = MicroProfileTickToMsMultiplier(bGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());
		int64_t nBaseTicks = bGpu ? nBaseTicksGpu : nBaseTicksCpu;


		nY += 3;
		MicroProfileDrawText(nX, nY, (uint32_t)-1, &pLog->ThreadName[0]);
		nY += 3;
		nY += MICROPROFILE_TEXT_HEIGHT + 1;
		uint32_t nGet = pLog->nGet.load(std::memory_order_relaxed);
		uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
		uint32_t nYDelta = MICROPROFILE_PIX3_HEIGHT;
		for(uint32_t i = nGet; i < nPut; ++i)
		{
			MicroProfileLogEntry* pEntry = pLog->Log + i;
			MP_ASSERT(MicroProfileLogEntry::EEnter == pEntry->eType);
			{
				int32_t nTickStart = pEntry->nStartRelative;
				int32_t nTickEnd = pEntry->nEndRelative;

				uint32_t nColor = S.TimerInfo[ MicroProfileGetTimerIndex(pEntry->nToken) ].nColor;
				if(pEntry == pMouseOver)
					nColor = nHoverColor;
				uint32_t nStackPos = pEntry->nStackDepth;
				nMaxStackDepth = MicroProfileMax(nMaxStackDepth, nStackPos);
				float fMsStart = fToMs * (nTickStart - nBaseTicks);
				float fMsEnd = fToMs * (nTickEnd - nBaseTicks);
				MP_ASSERT(fMsStart <= fMsEnd);
				float fXStart = nX + fMsStart * fMsToScreen;
				float fXEnd = nX + fMsEnd * fMsToScreen;
				float fYStart = nY + nStackPos * nYDelta;
				float fYEnd = fYStart + (MICROPROFILE_DETAILED_BAR_HEIGHT);
				float fXDist = MicroProfileMax(fXStart - fMouseX, fMouseX - fXEnd);
				bool bHover = fXDist < fHoverDist && fYStart <= fMouseY && fMouseY <= fYEnd;
				uint32_t nIntegerWidth = fXEnd - fXStart;
				if(nIntegerWidth)
				{
					if(bHover)
					{
						nHoverToken = pEntry->nToken;
						nHoverTime = nTickEnd - nTickStart;
						pMouseOverNext = pEntry;
					}
					MicroProfileDrawBoxFade(fXStart, fYStart, fXEnd, fYEnd, nColor);
				}
				else
				{
					float fXAvg = 0.5f * (fXStart + fXEnd);
					int nLineX = round(fXAvg);
					if(nLineX != nLinesDrawn[nStackPos])
					{
						if(bHover)
						{
							nHoverToken = pEntry->nToken;
							nHoverTime = nTickEnd - nTickStart;
							pMouseOverNext = pEntry;
						}
						nLinesDrawn[nStackPos] = nLineX;
						float fLine[] = {
							fXAvg, fYStart + 0.5f,
							fXAvg, fYEnd + 0.5f

						};
						MicroProfileDrawLine2D(2, &fLine[0], nColor);
					}
				}
			}
		}
		nY += nMaxStackDepth * nYDelta + MICROPROFILE_DETAILED_BAR_HEIGHT+1;
	}
	S.pDisplayMouseOver = pMouseOverNext;

	for(float f = fMsBase; f < fMsEnd; ++i)
	{
		float fStart = f;
		float fNext = MicroProfileMin<float>(floor(f)+1.f, fMsEnd);
		uint32_t nXPos = nX + ((fStart-fMsBase) * fMsToScreen);
		if(0 == (i%nSkip))
		{
			char buf[10];
			snprintf(buf, 9, "%d", (int)f);
			MicroProfileDrawText(nXPos, nBaseY, (uint32_t)-1, buf);
		}
		f = fNext;
	}
	if(nHoverToken != (uint32_t)-1 && nHoverTime)
	{
		S.nHoverToken = nHoverToken;
		S.nHoverTime = nHoverTime;
	}
}



void MicroProfileLoopActiveGroups(uint32_t nX, uint32_t nY, const char* pName, std::function<void (uint32_t, uint32_t, uint64_t, uint32_t, uint32_t, float)> CB)
{
	if(pName)
		MicroProfileDrawText(nX, nY, (uint32_t)-1, pName);

	nY += S.nBarHeight + 2;
	uint32_t nGroup = S.nActiveGroup;
	uint64_t nGroupMask = (uint64_t)-1;
	uint32_t nCount = 0;
	for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		uint64_t nMask = 1ll << j;
		if(nMask & nGroup)
		{
			nY += S.nBarHeight + 1;
			float fToMs = MicroProfileTickToMsMultiplier(S.GroupInfo[j].Type == MicroProfileTokenTypeGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());

			for(uint32_t i = 0; i < S.nTotalTimers;++i)
			{
				uint64_t nTokenMask = MicroProfileGetGroupMask(S.TimerInfo[i].nToken);
				if(nTokenMask & nMask)
				{
					CB(i, nCount, nMask, nX, nY, fToMs);
					nCount += 2;
					nY += S.nBarHeight + 1;
				}
			}
		}
	}
}


void MicroProfileCalcTimers(float* pTimers, float* pAverage, float* pMax, float* pCallAverage, uint64_t nGroup, uint32_t nSize)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileCalcTimers", 0x773300);
	MicroProfileLoopActiveGroups(0, 0, 0, 
		[&](uint32_t nTimer, uint32_t nIdx, uint64_t nGroupMask, uint32_t nX, uint32_t nY, float fToMs){
			uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
			uint32_t nAggregateCount = S.Aggregate[nTimer].nCount ? S.Aggregate[nTimer].nCount : 1;
			float fToPrc = S.fRcpReferenceTime;
			float fMs = fToMs * (S.Frame[nTimer].nTicks);
			float fPrc = MicroProfileMin(fMs * fToPrc, 1.f);
			float fAverageMs = fToMs * (S.Aggregate[nTimer].nTicks / nAggregateFrames);
			float fAveragePrc = MicroProfileMin(fAverageMs * fToPrc, 1.f);
			float fMaxMs = fToMs * (S.AggregateMax[nTimer]);
			float fMaxPrc = MicroProfileMin(fMaxMs * fToPrc, 1.f);
			float fCallAverageMs = fToMs * (S.Aggregate[nTimer].nTicks / nAggregateCount);
			float fCallAveragePrc = MicroProfileMin(fCallAverageMs * fToPrc, 1.f);
			pTimers[nIdx] = fMs;
			pTimers[nIdx+1] = fPrc;
			pAverage[nIdx] = fAverageMs;
			pAverage[nIdx+1] = fAveragePrc;
			pMax[nIdx] = fMaxMs;
			pMax[nIdx+1] = fMaxPrc;
			pCallAverage[nIdx] = fCallAverageMs;
			pCallAverage[nIdx+1] = fCallAveragePrc;
		}
	);
}

#define SBUF_MAX 32

uint32_t MicroProfileDrawBarArray(uint32_t nX, uint32_t nY, float* pTimers, const char* pName)
{
	const uint32_t nHeight = S.nBarHeight;
	const uint32_t nWidth = S.nBarWidth;
	const uint32_t nTextWidth = 6 * (1+MICROPROFILE_TEXT_WIDTH);
	const float fWidth = S.nBarWidth;
	MicroProfileLoopActiveGroups(nX, nY, pName, 
		[=](uint32_t nTimer, uint32_t nIdx, uint64_t nGroupMask, uint32_t nX, uint32_t nY, float fToMs){
			char sBuffer[SBUF_MAX];
			snprintf(sBuffer, SBUF_MAX-1, "%5.2f", pTimers[nIdx]);
			MicroProfileDrawBoxFade(nX + nTextWidth, nY, nX + nTextWidth + fWidth * pTimers[nIdx+1], nY + nHeight, S.TimerInfo[nTimer].nColor);
			MicroProfileDrawText(nX, nY, (uint32_t)-1, sBuffer);
			
		});
	return nWidth + 5 + nTextWidth;

}

uint32_t MicroProfileDrawBarCallCount(uint32_t nX, uint32_t nY, const char* pName)
{
	MicroProfileLoopActiveGroups(nX, nY, pName, 
		[](uint32_t nTimer, uint32_t nIdx, uint64_t nGroupMask, uint32_t nX, uint32_t nY, float fToMs){
			char sBuffer[SBUF_MAX];
			snprintf(sBuffer, SBUF_MAX-1, "%5d", S.Frame[nTimer].nCount);//fix
			MicroProfileDrawText(nX, nY, (uint32_t)-1, sBuffer);
		});
	uint32_t nTextWidth = 6 * MICROPROFILE_TEXT_WIDTH;
	return 5 + nTextWidth;
}



uint32_t MicroProfileDrawBarLegend(uint32_t nX, uint32_t nY)
{
	MicroProfileLoopActiveGroups(nX, nY, 0, 
		[](uint32_t nTimer, uint32_t nIdx, uint64_t nGroupMask, uint32_t nX, uint32_t nY, float fToMs){
			MicroProfileDrawText(nX, nY, S.TimerInfo[nTimer].nColor, S.TimerInfo[nTimer].pName);
			if(S.nMouseY >= nY && S.nMouseY < nY + MICROPROFILE_TEXT_HEIGHT+1  && S.nMouseX < nX + 20 * (MICROPROFILE_TEXT_WIDTH+1))
			{
				S.nHoverToken = nTimer;
				S.nHoverTime = 0;
			}
		});
	return nX;
}

bool MicroProfileDrawGraph(uint32_t nScreenWidth, uint32_t nScreenHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "DrawGraph", 0xff0033);
	bool bEnabled = false;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		if(S.Graph[i].nToken != MICROPROFILE_INVALID_TOKEN)
			bEnabled = true;
	if(!bEnabled)
		return false;
	
	uint32_t nX = nScreenWidth - MICROPROFILE_GRAPH_WIDTH;
	uint32_t nY = nScreenHeight - MICROPROFILE_GRAPH_HEIGHT;
	MicroProfileDrawBox(nX, nY, MICROPROFILE_GRAPH_WIDTH, MICROPROFILE_GRAPH_HEIGHT, g_nMicroProfileBackColors[0]|g_nMicroProfileBackColors[1]);
	bool bMouseOver = S.nMouseX >= nX && S.nMouseY >= nY;
	float fMouseXPrc =(float(S.nMouseX - nX)) / MICROPROFILE_GRAPH_WIDTH;
	if(bMouseOver)
	{
		float fXAvg = fMouseXPrc * MICROPROFILE_GRAPH_WIDTH + nX;
		float fLine[] = {
			fXAvg, (float)nY,
			fXAvg, (float)nY + MICROPROFILE_GRAPH_HEIGHT,

		};
		MicroProfileDrawLine2D(2, &fLine[0], (uint32_t)-1);
	}




	
	float fY = nScreenHeight;
	float fDX = MICROPROFILE_GRAPH_WIDTH * 1.f / MICROPROFILE_GRAPH_HISTORY;  
	float fDY = MICROPROFILE_GRAPH_HEIGHT;
	uint32_t nPut = S.nGraphPut;
	float* pGraphData = (float*)alloca(sizeof(float)* MICROPROFILE_GRAPH_HISTORY*2);
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		if(S.Graph[i].nToken != MICROPROFILE_INVALID_TOKEN)
		{
			uint32_t nGroupId = MicroProfileGetGroupIndex(S.Graph[i].nToken);
			bool bGpu = S.GroupInfo[nGroupId].Type == MicroProfileTokenTypeGpu;
			float fToMs = MicroProfileTickToMsMultiplier(bGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());
			float fToPrc = fToMs * S.fRcpReferenceTime * 3 / 4;

			float fX = nX;
			for(uint32_t j = 0; j < MICROPROFILE_GRAPH_HISTORY; ++j)
			{
				float fWeigth = MicroProfileMin(fToPrc * (S.Graph[i].nHistory[(j+nPut)%MICROPROFILE_GRAPH_HISTORY]), 1.f);
				pGraphData[(j*2)] = fX;
				pGraphData[(j*2)+1] = fY - fDY * fWeigth;
				fX += fDX;
			}
			MicroProfileDrawLine2D(MICROPROFILE_GRAPH_HISTORY, pGraphData, S.TimerInfo[MicroProfileGetTimerIndex(S.Graph[i].nToken)].nColor);
		}
	}
	{
		float fY1 = 0.25f * MICROPROFILE_GRAPH_HEIGHT + nY;
		float fY2 = 0.50f * MICROPROFILE_GRAPH_HEIGHT + nY;
		float fY3 = 0.75f * MICROPROFILE_GRAPH_HEIGHT + nY;
		float fLine[] = {
			(float)nX, fY1,
			(float)nX + MICROPROFILE_GRAPH_WIDTH, fY1,
			(float)nX, fY2,
			(float)nX + MICROPROFILE_GRAPH_WIDTH, fY2,
			(float)nX, fY3,
			(float)nX + MICROPROFILE_GRAPH_WIDTH, fY3,
		};
		MicroProfileDrawLine2D(2, &fLine[0], 0xdd4444);
		MicroProfileDrawLine2D(2, &fLine[4], g_nMicroProfileBackColors[0]);
		MicroProfileDrawLine2D(2, &fLine[8], g_nMicroProfileBackColors[0]);
		char buf[32];
		snprintf(buf, sizeof(buf)-1, "%5.2fms", S.fReferenceTime);
		MicroProfileDrawText(nX+1, fY1 - (2+MICROPROFILE_TEXT_HEIGHT), (uint32_t)-1, buf);
	}



	if(bMouseOver)
	{
		const char* ppStrings[MICROPROFILE_MAX_GRAPHS*2];
		uint32_t pColors[MICROPROFILE_MAX_GRAPHS];
		char buffer[1024];
		const char* pBufferStart = &buffer[0];
		char* pBuffer = &buffer[0];
#define SZ (intptr_t)(sizeof(buffer)-1-(pBufferStart - pBuffer))
		uint32_t nTextCount = 0;
		uint32_t nGraphIndex = (S.nGraphPut + MICROPROFILE_GRAPH_HISTORY - int(MICROPROFILE_GRAPH_HISTORY*(1.f - fMouseXPrc))) % MICROPROFILE_GRAPH_HISTORY;

		const uint32_t nBoxSize = MICROPROFILE_TEXT_HEIGHT;
		uint32_t nX = S.nMouseX;
		uint32_t nY = S.nMouseY + 20;

		float fToMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			if(S.Graph[i].nToken != MICROPROFILE_INVALID_TOKEN)
			{
				uint32_t nGroupId = MicroProfileGetGroupIndex(S.Graph[i].nToken);
				bool bGpu = S.GroupInfo[nGroupId].Type == MicroProfileTokenTypeGpu;
				float fToMs = MicroProfileTickToMsMultiplier(bGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());
				uint32_t nIndex = MicroProfileGetTimerIndex(S.Graph[i].nToken);
				uint32_t nColor = S.TimerInfo[nIndex].nColor;
				const char* pName = S.TimerInfo[nIndex].pName;
				pColors[nTextCount/2] = nColor;
				ppStrings[nTextCount++] = pName;
				ppStrings[nTextCount++] = pBuffer;
				pBuffer += 1 + snprintf(pBuffer, SZ, "%5.2fms", fToMs * (S.Graph[i].nHistory[nGraphIndex]));
			}
		}
		if(nTextCount)
		{
			MicroProfileDrawFloatWindow(nX, nY, ppStrings, nTextCount, pColors);
		}
#undef SZ

		if(S.nMouseRight)
		{
			for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
			{
				S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
			}
		}
	}

	return bMouseOver;
}

void MicroProfileDrawBarView(uint32_t nScreenWidth, uint32_t nScreenHeight)
{
	if(!S.nActiveGroup)
		return;
	MICROPROFILE_SCOPEI("MicroProfile", "DrawBarView", 0x00dd77);

	const uint32_t nHeight = S.nBarHeight;
	const uint32_t nWidth = S.nBarWidth;
	int nColorIndex = 0;
	uint32_t nX = 0;
	uint32_t nY = nHeight + 1 - S.nOffsetY;
	
	uint32_t nNumTimers = 0;
	uint32_t nNumGroups = 0;
	for(uint32 j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		if(S.nActiveGroup & (1ll << j))
		{
			nNumTimers += S.GroupInfo[j].nNumTimers;
			nNumGroups += 1;
		}
	}
	uint32_t nBlockSize = 2 * nNumTimers;
	float* pTimers = (float*)alloca(nBlockSize * 4 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pCallAverage = pTimers + 3 * nBlockSize;
	MicroProfileCalcTimers(pTimers, pAverage, pMax, pCallAverage, S.nActiveGroup, nNumTimers);
	for(uint32_t i = 0; i < nNumTimers+nNumGroups+1; ++i)
	{
		MicroProfileDrawBox(nX, nY + i * (nHeight + 1), nScreenWidth, (nHeight+1)+1, g_nMicroProfileBackColors[nColorIndex++ & 1]);
	}
	uint32_t nLegendOffset = 1;
	for(uint32 j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		if(S.nActiveGroup & (1ll << j))
		{
			MicroProfileDrawText(nX, nY + (1+nHeight) * nLegendOffset, (uint32_t)-1, S.GroupInfo[j].pName);
			nLegendOffset += S.GroupInfo[j].nNumTimers+1;
		}
	}
	if(S.nBars & MP_DRAW_TIMERS)		
		nX += MicroProfileDrawBarArray(nX, nY, pTimers, "Time") + 1;
	if(S.nBars & MP_DRAW_AVERAGE)		
		nX += MicroProfileDrawBarArray(nX, nY, pAverage, "Average") + 1;
	if(S.nBars & MP_DRAW_MAX)		
		nX += MicroProfileDrawBarArray(nX, nY, pMax, "Max Time") + 1;
	if(S.nBars & MP_DRAW_CALL_COUNT)		
	{
		nX += MicroProfileDrawBarArray(nX, nY, pCallAverage, "Call Average") + 1;
		nX += MicroProfileDrawBarCallCount(nX, nY, "Count") + 1; 
	}
	nX += MicroProfileDrawBarLegend(nX, nY) + 1;
}

bool MicroProfileDrawMenu(uint32_t nWidth, uint32_t nHeight)
{
	uint32_t nX = 0;
	uint32_t nY = 0;
	bool bMouseOver = S.nMouseY < MICROPROFILE_TEXT_HEIGHT + 1;
	char buffer[128];
	MICROPROFILE_SCOPEI("MicroProfile", "Menu", 0x0373e3);
	MicroProfileDrawBox(nX, nY, nWidth, (S.nBarHeight+1)+1, g_nMicroProfileBackColors[1]);

#define MICROPROFILE_MENU_MAX 16
	const char* pMenuText[MICROPROFILE_MENU_MAX] = {0};
	uint32_t 	nMenuX[MICROPROFILE_MENU_MAX] = {0};
	uint32_t nNumMenuItems = 0;

	snprintf(buffer, 127, "MicroProfile");
	MicroProfileDrawText(nX, nY, -1, buffer);
	nX += (sizeof("MicroProfile")+2) * (MICROPROFILE_TEXT_WIDTH+1);
	//mode
	pMenuText[nNumMenuItems++] = "Mode";
	pMenuText[nNumMenuItems++] = "Groups";
	pMenuText[nNumMenuItems++] = "Threads";
	char AggregateText[64];
	snprintf(AggregateText, sizeof(AggregateText)-1, "Aggregate[%d]", S.nAggregateFlip ? S.nAggregateFlip : S.nAggregateFlipCount);
	pMenuText[nNumMenuItems++] = &AggregateText[0];
	pMenuText[nNumMenuItems++] = "Timers";
	pMenuText[nNumMenuItems++] = "Reference";
	pMenuText[nNumMenuItems++] = "Preset";
	const int nPauseIndex = nNumMenuItems;
	pMenuText[nNumMenuItems++] = S.nFlipLog ? "Pause" : "Unpause";
	if(S.nOverflow)
	{
		pMenuText[nNumMenuItems++] = "!BUFFERSFULL!";
	}


	typedef std::function<const char* (int, bool&)> SubmenuCallback; 
	typedef std::function<void(int)> ClickCallback;
	SubmenuCallback GroupCallback[] = 
	{	[] (int index, bool& bSelected) -> const char*{
			switch(index)
			{
				case 0: 
					bSelected = S.nDisplay == MP_DRAW_DETAILED;
					return "Detailed";
				case 1:
					bSelected = S.nDisplay == MP_DRAW_BARS; 
					return "Timers";
				case 2:
					bSelected = false; 
					return "Off";

				default: return 0;
			}
		},
		[] (int index, bool& bSelected) -> const char*{
			if(index == 0)
			{
				bSelected = S.nMenuAllGroups != 0;
				return "ALL";
			}
			else
			{
				index = index-1;
				bSelected = 0 != (S.nMenuActiveGroup & (1ll << index));
				if(index < MICROPROFILE_MAX_GROUPS && S.GroupInfo[index].pName)
					return S.GroupInfo[index].pName;
				else
					return 0;
			}
		},
		[] (int index, bool& bSelected) -> const char*{
			if(0 == index)
			{
				bSelected = S.nMenuAllThreads != 0;
				return "All Threads";
			}
			else if(index-1 < MICROPROFILE_MAX_THREADS)
			{
				if(S.Pool[index-1])
				{
					bSelected = S.nThreadActive[index-1];
					return S.Pool[index-1]->ThreadName[0]?&S.Pool[index-1]->ThreadName[0]:0;
				}
				else
				{
					bSelected = false;
					return 0;
				}
			}
			return 0;
		},
		[] (int index, bool& bSelected) -> const char*{
			if(index < sizeof(g_MicroProfileAggregatePresets)/sizeof(g_MicroProfileAggregatePresets[0]))
			{
				int val = g_MicroProfileAggregatePresets[index];
				bSelected = S.nAggregateFlip == val;
				if(0 == val)
					return "Infinite";
				else
				{
					static char buf[128];
					snprintf(buf, sizeof(buf)-1, "%7d", val);
					return buf;
				}
			}
			return 0;
		},
		[] (int index, bool& bSelected) -> const char*{
			bSelected = 0 != (S.nBars & (1 << index));
			switch(index)
			{
				case 0: return "Timers";
				case 1: return "Average";
				case 2: return "Max";
				case 3: return "Call Count";
			}
			return 0;
		},
		[] (int index, bool& bSelected) -> const char*{
			if(index < sizeof(g_MicroProfileReferenceTimePresets)/sizeof(g_MicroProfileReferenceTimePresets[0]))
			{
				float val = g_MicroProfileReferenceTimePresets[index];
				bSelected = S.fReferenceTime == val;
				static char buf[128];
				snprintf(buf, sizeof(buf)-1, "%5.2fms", val);
				return buf;
			}
			return 0;
		},

		[] (int index, bool& bSelected) -> const char*{
			static char buf[128];
			bSelected = false;
			int nNumPresets = sizeof(g_MicroProfilePresetNames) / sizeof(g_MicroProfilePresetNames[0]);
			int nIndexSave = index - nNumPresets - 1;
			if(index == nNumPresets)
				return "--";
			else if(nIndexSave >=0 && nIndexSave <nNumPresets)
			{
				snprintf(buf, sizeof(buf)-1, "Save '%s'", g_MicroProfilePresetNames[nIndexSave]);
				return buf;
			}
			else if(index < nNumPresets)
			{
				snprintf(buf, sizeof(buf)-1, "Load '%s'", g_MicroProfilePresetNames[index]);
				return buf;
			}
			else
			{
				return 0;
			}
		},

		[] (int index, bool& bSelected) -> const char*{
			return 0;
		},
		[] (int index, bool& bSelected) -> const char*{
			return 0;
		},



	};
	ClickCallback CBClick[] = 
	{
		[](int nIndex)
		{
			switch(nIndex)
			{
				case 0:
					S.nDisplay = MP_DRAW_DETAILED;
					break;
				case 1:
					S.nDisplay = MP_DRAW_BARS;
					break;
				case 2:
					S.nDisplay = 0;
					break;
			}
		},
		[](int nIndex)
		{
			if(nIndex == 0)
				S.nMenuAllGroups = 1-S.nMenuAllGroups;
			else
				S.nMenuActiveGroup ^= (1ll << (nIndex-1));
		},
		[](int nIndex)
		{
			if(nIndex == 0)
				S.nMenuAllThreads = 1-S.nMenuAllThreads;
			else
			{
				S.nThreadActive[nIndex-1] = 1-S.nThreadActive[nIndex-1];
			}
		},
		[](int nIndex)
		{
			S.nAggregateFlip = g_MicroProfileAggregatePresets[nIndex];
			if(0 == S.nAggregateFlip)
			{
				memset(S.AggregateTimers, 0, sizeof(S.AggregateTimers));
				memset(S.MaxTimers, 0, sizeof(S.MaxTimers));
				S.nAggregateFlipCount = 0;
			}
		},
		[](int nIndex)
		{
			S.nBars ^= (1 << nIndex);
		},
		[](int nIndex)
		{
			S.fReferenceTime = g_MicroProfileReferenceTimePresets[nIndex];
			S.fRcpReferenceTime = 1.f / S.fReferenceTime;
		},
		[](int nIndex)
		{
			int nNumPresets = sizeof(g_MicroProfilePresetNames) / sizeof(g_MicroProfilePresetNames[0]);
			int nIndexSave = nIndex - nNumPresets - 1;
			if(nIndexSave >= 0 && nIndexSave < nNumPresets)
			{
				MicroProfileSavePreset(g_MicroProfilePresetNames[nIndexSave]);
			}
			else if(nIndex >= 0 && nIndex < nNumPresets)
			{
				MicroProfileLoadPreset(g_MicroProfilePresetNames[nIndex]);
			}
		},
		[](int nIndex)
		{
		},
		[](int nIndex)
		{
		},



	};


	uint32_t nSelectMenu = (uint32_t)-1;
	for(uint32_t i = 0; i < nNumMenuItems; ++i)
	{
		nMenuX[i] = nX;
		int nLen = strlen(pMenuText[i]);
		int nEnd = nX + nLen * (MICROPROFILE_TEXT_WIDTH+1);
		if(S.nMouseY <= MICROPROFILE_TEXT_HEIGHT && S.nMouseX <= nEnd && S.nMouseX >= nX)
		{
			MicroProfileDrawBox(nX-1, nY, 1+nLen * (MICROPROFILE_TEXT_WIDTH+1), (S.nBarHeight+1)+1, 0x888888);
			nSelectMenu = i;
			if((S.nMouseLeft || S.nMouseRight) && i == nPauseIndex)
			{
				S.nFlipLog = !S.nFlipLog;
			}
		}
		MicroProfileDrawText(nX, nY, -1, pMenuText[i]);
		nX += (nLen+1) * (MICROPROFILE_TEXT_WIDTH+1);
	}
	uint32_t nMenu = nSelectMenu != (uint32_t)-1 ? nSelectMenu : S.nActiveMenu;
	S.nActiveMenu = nMenu;
	if((uint32_t)-1 != nMenu)
	{
		#define SBUF_SIZE 256
		char sBuffer[SBUF_SIZE];
		nX = nMenuX[nMenu];
		nY += MICROPROFILE_TEXT_HEIGHT+1;
		SubmenuCallback CB = GroupCallback[nMenu];
		int nNumLines = 0;
		bool bSelected = false;
		const char* pString = CB(nNumLines, bSelected);
		uint32_t nWidth = 0, nHeight = 0;
		while(pString)
		{
			nWidth = MicroProfileMax<int>(nWidth, strlen(pString));
			nNumLines++;
			pString = CB(nNumLines, bSelected);
		}
		nWidth = (2+nWidth) * (MICROPROFILE_TEXT_WIDTH+1);
		nHeight = nNumLines * (MICROPROFILE_TEXT_HEIGHT+1);
		if(S.nMouseY <= nY + nHeight+0 && S.nMouseY >= nY-0 && S.nMouseX <= nX + nWidth+0 && S.nMouseX >= nX-0)
		{
			S.nActiveMenu = nMenu;
		}
		else if(nSelectMenu == (uint32_t)-1)
		{
			S.nActiveMenu = (uint32_t)-1;
		}
		MicroProfileDrawBox(nX, nY, nWidth, nHeight, g_nMicroProfileBackColors[1]);
		for(int i = 0; i < nNumLines; ++i)
		{
			bool bSelected = false;
			const char* pString = CB(i, bSelected);
			if(S.nMouseY >= nY && S.nMouseY < nY + MICROPROFILE_TEXT_HEIGHT + 1)
			{
				bMouseOver = true;
				if(S.nMouseLeft || S.nMouseRight)
				{
					CBClick[nMenu](i);
				}
				MicroProfileDrawBox(nX, nY, nWidth, MICROPROFILE_TEXT_HEIGHT + 1, 0x888888);
			}
			snprintf(buffer, SBUF_SIZE-1, "%c %s", bSelected ? '*' : ' ' ,pString);
			MicroProfileDrawText(nX, nY, -1, buffer);
			nY += MICROPROFILE_TEXT_HEIGHT+1;
		}
	}
	return bMouseOver;
}


void MicroProfileMoveGraph()
{
	int nZoom = S.nMouseWheelDelta;
	int nPanX = 0;
	int nPanY = 0;
	static int X = 0, Y = 0;
	if(S.nMouseDownLeft)
	{
		nPanX = S.nMouseX - X;
		nPanY = S.nMouseY - Y;
	}
	X = S.nMouseX;
	Y = S.nMouseY;

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
	if(nPanX)
	{
		S.fGraphBaseTimePos -= nPanX * 2 * S.fGraphBaseTime / S.nWidth;
		S.fGraphBaseTimePos = MicroProfileMax(S.fGraphBaseTimePos, 0.f);
		S.fGraphBaseTimePos = MicroProfileMin(S.fGraphBaseTimePos, MICROPROFILE_MAX_GRAPH_TIME - S.fGraphBaseTime);
	}
	S.nOffsetY -= nPanY;
	if(S.nOffsetY<0)
		S.nOffsetY = 0;
}

void MicroProfileDraw(uint32_t nWidth, uint32_t nHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "Draw", 0x737373);
	if(S.nMouseLeft && S.nMouseX < 12 && S.nMouseY < 12)
	{
		MicroProfileToggleDisplayMode();
	}

	if(S.nDisplay)
	{
		S.nWidth = nWidth;
		S.nHeight = nHeight;
		S.nHoverToken = MICROPROFILE_INVALID_TOKEN;
		S.nHoverTime = 0;

		MicroProfileMoveGraph();


		if(S.nDisplay & MP_DRAW_DETAILED)
		{
			MicroProfileDrawDetailedView(nWidth, nHeight);
		}
		else if(0 != (S.nDisplay & MP_DRAW_BARS) && S.nBars)
		{
			MicroProfileDrawBarView(nWidth, nHeight);
		}

		bool bMouseOverMenu = MicroProfileDrawMenu(nWidth, nHeight);
		bool bMouseOverGraph = MicroProfileDrawGraph(nWidth, nHeight);

		if(!bMouseOverMenu && !bMouseOverGraph)
		{
			if(S.nHoverToken != MICROPROFILE_INVALID_TOKEN)
			{
				MicroProfileDrawFloatTooltip(S.nMouseX, S.nMouseY, S.nHoverToken, S.nHoverTime);
			}
			if(S.nMouseLeft || S.nMouseRight)
			{
				if(S.nHoverToken != MICROPROFILE_INVALID_TOKEN)
					MicroProfileToggleGraph(S.nHoverToken);
			}
		}
		S.nActiveGroup = S.nMenuAllGroups ? (S.nGroupMask & (uint64_t)-1) : S.nMenuActiveGroup;
	}
	S.nMouseLeft = S.nMouseRight = 0;
	S.nMouseWheelDelta = 0;
	if(S.nOverflow)
		S.nOverflow--;

}
void MicroProfileMousePosition(uint32_t nX, uint32_t nY, int nWheelDelta)
{
	S.nMouseX = nX;
	S.nMouseY = nY;
	S.nMouseWheelDelta = nWheelDelta;
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
}

void MicroProfileToggleGraph(MicroProfileToken nToken)
{
	nToken &= 0xffff;
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
	memset(&S.Graph[nIndex].nHistory[0], 0, sizeof(S.Graph[nIndex].nHistory));
}
void MicroProfileMouseButton(uint32_t nLeft, uint32_t nRight)
{
	if(0 == nLeft && S.nMouseDownLeft)
		S.nMouseLeft = 1;
	if(0 == nRight && S.nMouseDownRight)
		S.nMouseRight = 1;

	S.nMouseDownLeft = nLeft;
	S.nMouseDownRight = nRight;
	
}

void* g_pFUUU = 0;



#include <stdio.h>

#define MICROPROFILE_PRESET_HEADER_MAGIC 0x28586813
struct MicroProfilePresetHeader
{
	uint32_t nMagic;
	//groups, threads, aggregate, reference frame, graphs timers
	uint32_t nGroups[MICROPROFILE_MAX_GROUPS];
	uint32_t nThreads[MICROPROFILE_MAX_THREADS];
	uint32_t nGraphName[MICROPROFILE_MAX_GRAPHS];
	uint32_t nGraphGroupName[MICROPROFILE_MAX_GRAPHS];
	uint32_t nMenuAllGroups;
	uint32_t nMenuAllThreads;
	uint32_t nAggregateFlip;
	float fReferenceTime;
	uint32_t nBars;
	uint32_t nDisplay;

};

const char* MicroProfilePresetFilename(const char* pSuffix)
{
	static char filename[512];
	snprintf(filename, sizeof(filename)-1, ".microprofilepreset.%s", pSuffix);
	return filename;
}

void MicroProfileSavePreset(const char* pPresetName)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	FILE* F = fopen(MicroProfilePresetFilename(pPresetName), "w");
	if(!F) return;

	MicroProfilePresetHeader Header;
	memset(&Header, 0, sizeof(Header));
	Header.nAggregateFlip = S.nAggregateFlip;
	Header.nBars = S.nBars;
	Header.fReferenceTime = S.fReferenceTime;
	Header.nMenuAllGroups = S.nMenuAllGroups;
	Header.nMenuAllThreads = S.nMenuAllThreads;
	Header.nMagic = MICROPROFILE_PRESET_HEADER_MAGIC;
	Header.nDisplay = S.nDisplay;
	fwrite(&Header, sizeof(Header), 1, F);
	uint64_t nMask = 1;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
	{
		if(S.nMenuActiveGroup & nMask)
		{
			uint32_t offset = ftell(F);
			const char* pName = S.GroupInfo[i].pName;
			int nLen = strlen(pName)+1;
			fwrite(pName, nLen, 1, F);
			Header.nGroups[i] = offset;
		}
		nMask <<= 1;
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		MicroProfileThreadLog* pLog = S.Pool[i];
		if(pLog && S.nThreadActive[i])
		{
			uint32_t nOffset = ftell(F);
			const char* pName = &pLog->ThreadName[0];
			int nLen = strlen(pName)+1;
			fwrite(pName, nLen, 1, F);
			Header.nThreads[i] = nOffset;
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		MicroProfileToken nToken = S.Graph[i].nToken;
		if(nToken != MICROPROFILE_INVALID_TOKEN)
		{
			uint32_t nGroupIndex = MicroProfileGetGroupIndex(nToken);
			uint32_t nTimerIndex = MicroProfileGetTimerIndex(nToken);
			const char* pGroupName = S.GroupInfo[nGroupIndex].pName;
			const char* pTimerName = S.TimerInfo[nTimerIndex].pName;
			MP_ASSERT(pGroupName);
			MP_ASSERT(pTimerName);
			int nGroupLen = strlen(pGroupName)+1;
			int nTimerLen = strlen(pTimerName)+1;

			uint32_t nOffsetGroup = ftell(F);
			fwrite(pGroupName, nGroupLen, 1, F);
			uint32_t nOffsetTimer = ftell(F);
			fwrite(pTimerName, nTimerLen, 1, F);
			Header.nGraphName[i] = nOffsetTimer;
			Header.nGraphGroupName[i] = nOffsetGroup;
		}
	}
	fseek(F, 0, SEEK_SET);
	fwrite(&Header, sizeof(Header), 1, F);

	fclose(F);

}



void MicroProfileLoadPreset(const char* pSuffix)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	FILE* F = fopen(MicroProfilePresetFilename(pSuffix), "r");
	if(!F)
	{
	 	return;
	}
	fseek(F, 0, SEEK_END);
	int nSize = ftell(F);
	char* const pBuffer = (char*)alloca(nSize);
	fseek(F, 0, SEEK_SET);
	int nRead = fread(pBuffer, nSize, 1, F);
	fclose(F);
	if(1 != nRead)
		return;
	
	MicroProfilePresetHeader& Header = *(MicroProfilePresetHeader*)pBuffer;

	if(Header.nMagic != MICROPROFILE_PRESET_HEADER_MAGIC)
	{
		MP_BREAK();
		return;
	}

	S.nAggregateFlip = Header.nAggregateFlip;
	S.nBars = Header.nBars;
	S.fReferenceTime = Header.fReferenceTime;
	S.fRcpReferenceTime = 1.f / Header.fReferenceTime;
	S.nMenuAllGroups = Header.nMenuAllGroups;
	S.nMenuAllThreads = Header.nMenuAllThreads;
	S.nDisplay = Header.nDisplay;
	S.nMenuActiveGroup = 0;
	memset(&S.nThreadActive[0], 0, sizeof(S.nThreadActive));

	for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
	{
		if(Header.nGroups[i])
		{
			const char* pGroupName = pBuffer + Header.nGroups[i];	
			for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
			{
				if(S.GroupInfo[j].pName && 0 == strcmp(pGroupName, S.GroupInfo[j].pName))
				{
					S.nMenuActiveGroup |= (1 << j);
				}
			}
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		if(Header.nThreads[i])
		{
			const char* pThreadName = pBuffer + Header.nThreads[i];
			for(uint32_t j = 0; j < MICROPROFILE_MAX_THREADS; ++j)
			{
				MicroProfileThreadLog* pLog = S.Pool[j];
				if(pLog && 0 == strcmp(pThreadName, &pLog->ThreadName[0]))
				{
					S.nThreadActive[j] = 1;
				}
			}
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		MicroProfileToken nPrevToken = S.Graph[i].nToken;
		S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		if(Header.nGraphName[i] && Header.nGraphGroupName[i])
		{
			const char* pGraphName = pBuffer + Header.nGraphName[i];
			const char* pGraphGroupName = pBuffer + Header.nGraphGroupName[i];
			for(uint32_t j = 0; j < S.nTotalTimers; ++j)
			{
				uint64_t nGroupIndex = S.TimerInfo[j].nGroupIndex;
				if(0 == strcmp(pGraphName, S.TimerInfo[j].pName) && 0 == strcmp(pGraphGroupName, S.GroupInfo[nGroupIndex].pName))
				{
					MicroProfileToken nToken = MicroProfileMakeToken(1ll << nGroupIndex, j);
					S.Graph[i].nToken = nToken;
					if(nToken != nPrevToken)
					{
						memset(&S.Graph[i].nHistory, 0, sizeof(S.Graph[i].nHistory));
					}
					break;
				}
			}
		}
	}
}




#endif
#endif
