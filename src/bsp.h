#pragma once
//#include "base.h"
//#include "program.h"

#define BSP_BOX_SCALE 1.02f
#define OCCLUDER_BSP_MAX_PLANES (1024 * 4)
#define OCCLUDER_BSP_MAX_NODES (1024)


struct SOccluderBsp;
SOccluderBsp* BspCreate();

struct SOccluderBspViewDesc
{
	v3 vOrigin;
	v3 vDirection;
	v3 vRight;
	float fFovY;
	float fAspect;
	float fZNear;

	uint32_t nNodeCap;
};
struct SOccluderBspStats
{
	uint32 nNumNodes;
	uint32 nNumPlanes;
	uint32 nNumPlanesConsidered;
	uint32 nNumObjectsTested;
	uint32 nNumObjectsTestedVisible;
	uint32 nNumChildBspsCreated;
};

struct SOccluderDesc
{
	float ObjectToWorld[16];
	float Size[3];
};	


void BspDestroy(SOccluderBsp* pBsp);
void BspBuild(SOccluderBsp* pBsp, 
			  SOccluderDesc** pPlaneOccluders, uint32 nNumPlaneOccluders,
			  SOccluderDesc** pBoxOccluders, uint32 nNumBoxOccluders,
			  const SOccluderBspViewDesc& Desc);
bool BspCullObject(SOccluderBsp* pBsp, SOccluderDesc* pObject);
void BspGetStats(SOccluderBsp* pBsp, SOccluderBspStats* pStats);



void BspDebugNextDrawMode(SOccluderBsp* pBsp);
void BspDebugNextDrawClipResult(SOccluderBsp* pBsp);
void BspDebugNextPoly(SOccluderBsp* pBsp);
void BspDebugPreviousPoly(SOccluderBsp* pBsp);
void BspDebugShowClipLevelNext(SOccluderBsp* pBsp);
void BspDebugShowClipLevelPrevious(SOccluderBsp* pBsp);
void BspDebugToggleInsideOutside(SOccluderBsp* pBsp);
void BspDebugClipLevelSubNext(SOccluderBsp* pBsp);
void BspDebugClipLevelSubPrev(SOccluderBsp* pBsp);
void BspDebugDumpFrame(SOccluderBsp* pBsp);
