#pragma once

#ifdef _MAC
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



struct MicroProfileInfo
{
	MicroProfileInfo(const char* sGroup, const char* sName, uint32_t nColor);
};
struct MicroProfileScopeHandler
{
	MicroProfileInfo& Info;
	MicroProfilerScopeHandler(MicroProfileInfo& Info):Info(Info)
	{
		MicroProfileEnter(Info);
	}
	~MicroProfileScopeHandler()
	{
		MicroProfileLeave(Info);
	}
};



#define ZMICROPROFILE_DECLARE(var) extern MicroProfileInfo g_mp_##var
#define ZMICROPROFILE_DEFINE(var, group, name, color) MicroProfileInfo g_mp_##var(group, name, color)


#define ZMICROPROFILE_SCOPE(var) MicroProfileScopeHandler foo ## __LINE__(g_mp_##var)
#define ZMICROPROFILE_SCOPE_INLINE(group, name, color) 









#ifdef MICRO_PROFILE_IMPL



#endif