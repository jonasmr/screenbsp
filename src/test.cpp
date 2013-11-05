#include "base.h"
#include "math.h"
#include "program.h"
#include "test.h"
#include "microprofile.h"
#include "debug.h"
#include <functional>

uint32 g_nRunTest = 0;
FILE* g_TestOut = 0;
FILE* g_TestFailOut = 0;


int32 g_nTestIndex = 0; //0:bsp, 1:software occl
int32 g_nSubTestIndex = 0; //number of sub tests [paths through scene]
int32 g_nTestSettingIndex = 0; //tweakable param of test (depth for bsp, resolution for software occl)
int32 g_nTestInnerIndex = 0;
int32 g_nTestInnerIndexEnd = 0;
int32 g_nTestFail = 0;
int32 g_nTestTotalFail = 0;
int32 g_nTestMaxFail = 0;
int32 g_nTestTotalFailFrames = 0;
int32 g_nTestFalsePositives = 0;
int32 g_nTestTotalFalsePositives = 0;
int32 g_nTestMaxFalsePositives = 0;
int32 g_nTestFrames;
float g_fTestTime;
float g_fPrepareTime;
float g_fBuildTime;
float g_fCullTime;
float g_fTestMaxFrameTime;
extern uint32 g_nBspNodeCap;


#define OCCLUSION_TEST_HALF_SIZE 300
#define OCCLUSION_TEST_MIN (-OCCLUSION_TEST_HALF_SIZE)
#define OCCLUSION_TEST_MAX OCCLUSION_TEST_HALF_SIZE
#define OCCLUSION_NUM_LARGE 10
#define OCCLUSION_NUM_SMALL 100
#define OCCLUSION_NUM_LONG 20
#define OCCLUSION_NUM_OBJECTS 2000
#define OCCLUSION_USE_GROUND 1
#define OCCLUSION_GROUND_Y -1.0f

int nSettingsBsp[] = 
{
	// 10, 
	// 20, 
	// 32,
	// 64, 
	// 128, 
	// 200, 
	// 256, 
	// 386, 
	// 512, 
	// 768,
	1024,
	1024,
	1024,
	1024,
	1024,
	//1024,
	//1024,
	//1024,
	//1024,
	//1024,
	//1024,
	//1024,
	//1024,
	//1024,	
	// 2048,
};
const uint32 nNumSettingsBsp = sizeof(nSettingsBsp)/sizeof(nSettingsBsp[0]);
const uint32 nNumSettingsSO = 0;

void WorldOcclusionCreate(v3 vSize, uint32 nFlags, v3 vColor, v3 vPos)
{
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].mObjectToWorld = mid();
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].mObjectToWorld.trans = v4init(vPos, 1.f);
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].vSize = vSize; 
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].nFlags = nFlags;
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].nColor = vColor.tocolor();
	g_WorldState.nNumWorldObjects++;
}


void WorldOcclusionCreate(v3 vSize, uint32 nFlags, v3 vColor = v3rep(frandrange(0.6f, 0.9f)))
{
	v3 vPos = v3zero();
	vPos.x = frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
	vPos.z = frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
	vPos.y = 1.0f * vSize.y;
	WorldOcclusionCreate(vSize, nFlags, vColor, vPos);
}




void WorldInitOcclusionTest()
{
	randseed(0xed32babe, 0xdeadf39c);
	g_WorldState.nNumWorldObjects = 0;
	bool bSkipInitLong = false;
	bool bSkipInitLarge = false;
	bool bSkipInitSmall = false;
	bool bSkipInitGround = false;
	float fbar = 0.f;
	int idxx_large[] = 
	{
		-1,
		//0, 1, 2, 3, 4, 
		//5, 6, 7, 
		//8, 
		//9, 10, 
		//76 + 15,
	};

	bool bSkipInit = false;

	//void WorldOcclusionCreate(v3 vSize, uint32 nFlags, v3 vColor, v3 vPos)
	if(OCCLUSION_USE_GROUND && !bSkipInitGround)
//	if(0)
	{
		WorldOcclusionCreate(v3init(OCCLUSION_TEST_HALF_SIZE*2, 1.0f, OCCLUSION_TEST_HALF_SIZE*2),
			SObject::OCCLUDER_BOX,
			v3init(1,1,1),
			v3init(0,OCCLUSION_GROUND_Y, 0));
	}
	else
	{

	}


	for(int i = 0; i < OCCLUSION_NUM_LARGE; ++i)
	{
		float fHeight = frandrange(50, 100);
		float fWidth = frandrange(7, 15);
		float fDepth = frandrange(7, 15);
		bool bSkip = bSkipInitLarge;
		for(int x : idxx_large)
			if(x == i)
				bSkip = false;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX);
		}
	}
	
	int idxx[] = 
	{
		-1,
			// 0,1,2,3,4,5,6,7,8,9,
			// 10,11,12,13,14,15,16,17,18,19,
			// 20,21,22,23,24,25,26,27,28,29,
		
		//30,31,32,33,34,35,36,37,38,39,
		

		//40,
		//42,

		41,
		44,

		//43,
		
		

		//45,46,47,48,49,
		

		//50,51,52,53,54,55,56,57,58,59,
		//60,61,62,63,64,65,66,67,68,69,
		//70,71,72,73,74,75,76,77,78,79,


		// 75 + 15 , 
		// 76 + 15 ,
	};
	//for(int i = 75; i < 100; ++i)

	for(int i = 0; i < OCCLUSION_NUM_SMALL; ++i)
	{
		float fHeight = frandrange(10, 15);
		float fWidth = frandrange(7, 15);
		float fDepth = frandrange(7, 15);
		bool bSkip = bSkipInitSmall;
		for(int x : idxx)
			if(x == i)
				bSkip = false;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX);
		}
	}
//	uprintf("fbar %f", fbar);

	int idxx_long[] = 
	{
		-1,
		// 0,
		// 1,2,3,4,5,6,7,8,9,
		// 10,11,12,13,14,
		// 15,
		// //16,
		// 17,18,19,20,21
	};

	int idxx_long2[] = 
	{
		-1,
		// 0,
		// 1,2,3,4,5,6,7,8,9,
		// //10,
		// 11,
		// 12,13,14,
		// 15,16,
		// 17,18,19,20,21
	};
	//bool bSkipInitLong = true;
	uint32 nadd = 0;

	for(int i = 0; i < OCCLUSION_NUM_LONG; ++i)
	{
		float fHeight = frandrange(10, 20);
		float fWidth = frandrange(25, 50);
		float fDepth = frandrange(7, 12);
		if(frand() < 0.5f)
		{
			Swap(fWidth, fHeight);
		}

		bool bSkip = bSkipInitLong;
		for(int x : idxx_long)
			if(x == i)
				bSkip = true;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX
				// |SObject::OCCLUSION_BOX_SKIP_X
				// |SObject::OCCLUSION_BOX_SKIP_Y
				//|SObject::OCCLUSION_BOX_SKIP_Z
				);
			nadd++;
		}
	}
	for(int i = 0; i < OCCLUSION_NUM_LONG; ++i)
	{
		float fHeight = frandrange(10, 20);
		float fWidth = frandrange(25, 50);
		float fDepth = frandrange(7, 12);
		if(frand() < 0.5f)
		{
			Swap(fWidth, fDepth);
		}
		bool bSkip = bSkipInitLong;
		for(int x : idxx_long2)
			if(x == i)
				bSkip = true;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX
				// |SObject::OCCLUSION_BOX_SKIP_X
				// |SObject::OCCLUSION_BOX_SKIP_Y
				//|SObject::OCCLUSION_BOX_SKIP_Z

				);
			nadd++;
		}
	}
	uprintf("TOTAL ADD LONG %d\n", nadd);
	uint32 nNumObjects = g_WorldState.nNumWorldObjects;
	for(int i = 0; i < OCCLUSION_NUM_OBJECTS; ++i)
	{
		float fHeight = frandrange(1, 2);
		float fWidth = frandrange(0.5f, 0.7f);
		float fDepth = frandrange(0.5, 0.7f);
		if(frand() < 0.5f)
		{
			Swap(fWidth, fHeight);
		}
		//if(1||i == 512)
		//if(i > 16 && i < 20)
		//if(0||(i >= 168 && i < 16))
		//if(i == 169)
		//if(i + nNumObjects == 189)
		if(1)
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUSION_TEST, v3fromcolor(randcolor()));
			if(frand()<0.02f)
			{
				g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects-1].mObjectToWorld.trans.y -= 10;
			}

		}
		else
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);

		}
	}
}


void TestClear()
{
	g_nTestTotalFail = 0;
	g_nTestMaxFail = 0;
	g_nTestFail = 0;
	g_nTestTotalFailFrames = 0;
	g_nTestFalsePositives = 0;
	g_nTestTotalFalsePositives = 0;
	g_nTestFrames = 0;
 	g_fTestTime = 0;
 	g_fPrepareTime = 0;
 	g_fBuildTime = 0;
 	g_fCullTime = 0;
 	g_fTestMaxFrameTime = 0;
	g_nTestInnerIndex = 0;
	g_nTestInnerIndexEnd = -1;
}
extern uint32 g_nUseDebugCameraPos;
void StartTest()
{
	g_TestOut = fopen("test.txt", "w");
	g_TestFailOut = fopen("test.fail.txt", "w");
	g_nUseDebugCameraPos = 0;

	g_nTestIndex = -1; //0:bsp, 1:software occl
	g_nSubTestIndex = 0; //number of sub tests [paths through scene]
	g_nTestSettingIndex = 0; //tweakable param of test (depth for bsp, resolution for software occl)
	g_nTestInnerIndex = -1;
	g_nTestInnerIndexEnd = -1;
	g_nTestTotalFail = 0;
	g_nTestMaxFail = 0;
	g_nTestFail = 0;
	g_nTestTotalFailFrames = 0;
	g_nTestFalsePositives = 0;
	g_nTestTotalFalsePositives = 0;
	g_nRunTest = 1;

	fprintf(g_TestOut, "%10s %6s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s\n", 
		"", 
		"Sett",
		"frames",
		"frafail",
		"maxfail",
		"totfail",
		"maxfals",
		"totfals",
		"avgfals",
		"time",
		"maxtime",
		"avgtime",
		"preptim",
		"buildti",
		"culltim"
		);

	uprintf("%10s %6s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s\n", 
		"", 
		"Sett",
		"frames",
		"frafail",
		"maxfail",
		"totfail",
		"maxfals",
		"totfals",
		"avgfals",
		"time",
		"maxtime",
		"avgtime",
		"preptim",
		"buildti",
		"culltim"
		);



	MICROPROFILE_FORCEENABLECPUGROUP("CullTest");
	MICROPROFILE_FORCEENABLECPUGROUP("Bsp");
}

void StopTest()
{
	ZASSERT(g_TestOut);
	fclose(g_TestOut);
	ZASSERT(g_TestFailOut);
	fclose(g_TestFailOut);
	g_TestOut = 0;
	g_TestFailOut = 0;
	g_nRunTest = 0;
	MICROPROFILE_FORCEDISABLECPUGROUP("CullTest");
}





void TestWrite()
{
	const char* pTestName = g_nTestIndex == 0 ? "ScreenBsp" : "INTEL_SWO";			
	int nSettingValue = 0;
	if(g_nTestIndex == 0)
		nSettingValue = nSettingsBsp[g_nTestSettingIndex];
	else
		nSettingValue = 42;

	fprintf(g_TestOut, "%10s %6d %7d %7d %7d %7d %7d %7d %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n", 
		pTestName, 
		nSettingValue,
		g_nTestFrames,
		g_nTestTotalFailFrames,
		g_nTestMaxFail,
		g_nTestTotalFail,
		g_nTestMaxFalsePositives,
		g_nTestTotalFalsePositives,
		(float)g_nTestTotalFalsePositives / g_nTestFrames,
		g_fTestTime,
		g_fTestMaxFrameTime,
		g_fTestTime / g_nTestFrames,
		g_fPrepareTime / g_nTestFrames,
		g_fBuildTime / g_nTestFrames,
 		g_fCullTime / g_nTestFrames

		);
	fprintf(g_TestFailOut, "%10s %6d %7d %7d %7d %7d %7d %7d %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n", 
		pTestName, 
		nSettingValue,
		g_nTestFrames,
		g_nTestTotalFailFrames,
		g_nTestMaxFail,
		g_nTestTotalFail,
		g_nTestMaxFalsePositives,
		g_nTestTotalFalsePositives,
		(float)g_nTestTotalFalsePositives / g_nTestFrames,
		g_fTestTime,
		g_fTestMaxFrameTime,
		g_fTestTime / g_nTestFrames,
		g_fPrepareTime / g_nTestFrames,
		g_fBuildTime / g_nTestFrames,
 		g_fCullTime / g_nTestFrames

		);



	uprintf("%10s %6d %7d %7d %7d %7d %7d %7d %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n", 
		pTestName, 
		nSettingValue,
		g_nTestFrames,
		g_nTestTotalFailFrames,
		g_nTestMaxFail,
		g_nTestTotalFail,
		g_nTestMaxFalsePositives,
		g_nTestTotalFalsePositives,
		(float)g_nTestTotalFalsePositives / g_nTestFrames,
		g_fTestTime,
		g_fTestMaxFrameTime,
		g_fTestTime / g_nTestFrames,
		g_fPrepareTime / g_nTestFrames,
		g_fBuildTime / g_nTestFrames,
 		g_fCullTime / g_nTestFrames

		);
}

void RunTest(v3& vPos_, v3& vDir_, v3& vRight_)
{
#define NUM_TESTS 1
	std::function<int (int, v3&, v3&, v3&)> TestFuncs[] = 
	{
		[] (int index, v3& vPos, v3& vDir, v3& vUp) -> int{
			vUp = v3init(0, 1, 0);
			const int CIRCLE_TOTAL_STEPS = (1<<9);
			const int CIRCLE_REVOLUTIONS = 2;
			const int CIRCLE_INNER_RADIUS = 300;
			const int CIRCLE_OUTER_RADIUS = 350;
			float fAngle = TWOPI * float(index) / (CIRCLE_TOTAL_STEPS/CIRCLE_REVOLUTIONS);
			float fDist = CIRCLE_INNER_RADIUS + (CIRCLE_OUTER_RADIUS-CIRCLE_INNER_RADIUS) * float(index) / CIRCLE_TOTAL_STEPS;
			float fX = sinf(fAngle) * fDist;
			float fZ = cosf(fAngle) * fDist;
			vPos = v3init(fX, 10, fZ);
			vDir = -v3normalize(vPos);
			uplotfnxt("test run %d... pos %f %f %f", index, vPos.x, vPos.y, vPos.z);

			return CIRCLE_TOTAL_STEPS;
		},
		[] (int index, v3& vPos, v3& vDir, v3& vUp) -> int{
			vUp = v3init(0, 1, 0);
			const int CIRCLE_TOTAL_STEPS = (1<<5);
			const int CIRCLE_REVOLUTIONS = 3;
			const int CIRCLE_INNER_RADIUS = 50;
			const int CIRCLE_OUTER_RADIUS = 800;
			float fAngle = TWOPI * float(index) / (CIRCLE_TOTAL_STEPS/CIRCLE_REVOLUTIONS);
			float fDist = CIRCLE_INNER_RADIUS + (CIRCLE_OUTER_RADIUS-CIRCLE_INNER_RADIUS) * float(index) / CIRCLE_TOTAL_STEPS;
			float fX = sinf(fAngle) * fDist;
			float fZ = cosf(fAngle) * fDist;
			vPos = v3init(fX, 10, fZ);
			vDir = -v3normalize(vPos);
			uplotfnxt("test run %d... pos %f %f %f", index, vPos.x, vPos.y, vPos.z);

			return CIRCLE_TOTAL_STEPS;
		}
	};

	if(g_nTestInnerIndex>=0)
	{
		g_nTestTotalFail += g_nTestFail;
		g_nTestMaxFail = Max(g_nTestFail, g_nTestMaxFail);
		if(g_nTestFail)
			g_nTestTotalFailFrames++;
		g_nTestTotalFalsePositives += g_nTestFalsePositives;
		g_nTestMaxFalsePositives = Max(g_nTestFalsePositives, g_nTestMaxFalsePositives);

		float fTimeCull = MicroProfileGetTime("CullTest", "Cull");
		float fTimeBuild = MicroProfileGetTime("CullTest", "Build");
		float fTimeBuildPrepare = MicroProfileGetTime("Bsp", "CullPrepare");
		float fTotalTime = fTimeCull + fTimeBuild;		
		g_fTestTime += fTotalTime;
		g_fPrepareTime += fTimeBuildPrepare;
		g_fBuildTime += fTimeBuild;
		g_fCullTime += fTimeCull;
		g_fTestMaxFrameTime = Max(g_fTestMaxFrameTime, fTotalTime);

	}

	if(g_nTestInnerIndex == g_nTestInnerIndexEnd)
	{
		if(g_nTestIndex == -1)
		{
			g_nTestIndex = 0;
			g_nSubTestIndex = 0;
			g_nTestSettingIndex = 0;
			fprintf(g_TestOut, "\n\n*** ScreenBSP\n\n");
			TestClear();
			g_nBspNodeCap = nSettingsBsp[g_nTestSettingIndex];
		}
		else
		{
			if(g_nTestIndex == 0)
			{
				TestWrite();
				TestClear();
				g_nTestSettingIndex++;
				if(g_nTestSettingIndex == nNumSettingsBsp)
				{
					fprintf(g_TestOut, "\n\n*** SOFTWARE OCCLUSION\n\n");
					g_nTestIndex = 1;
					g_nTestSettingIndex = 0;
					StopTest();
					return;
				}
				else
				{
					g_nBspNodeCap = nSettingsBsp[g_nTestSettingIndex];
				}
			}
			else if(g_nTestIndex == 1)
			{
				TestWrite();
				TestClear();
				g_nTestSettingIndex++;
				if(g_nTestSettingIndex == nNumSettingsBsp)
				{
					fprintf(g_TestOut, "\n\nAll Done\n");
					uprintf("ALL DONE\n");
					g_nTestIndex = 1;
					g_nTestSettingIndex = 0;
					StopTest();
					return;
				}
			}
		}
	}


	g_nTestFail = 0;
	g_nTestFalsePositives = 0;
	g_nTestFrames++;

	ZASSERT(g_nTestIndex>=0 && g_nTestIndex<2);

	v3 vPos, vDir, vUp;
	g_nTestInnerIndexEnd = TestFuncs[g_nTestIndex](g_nTestInnerIndex, vPos, vDir, vUp);
	g_nTestInnerIndex++;
	vPos_ = vPos;
	vDir_ = vDir;
	vRight_ = v3normalize(v3cross(vDir, vUp));
}


