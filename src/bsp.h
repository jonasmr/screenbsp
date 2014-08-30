#pragma once
//#include "base.h"
//#include "program.h"
#include "fixedarray.h"

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
	uint32 nNumChildBspsVisible;
	uint32 nNunSubBspTests;
};

struct SOccluderDesc
{
	float ObjectToWorld[16];
	float Size[3];
};	

struct SOccluderBspNode
{
	uint16 nPlaneIndex;
	uint16 nInside;
	uint16 nOutside;
};

struct SOccluderBspNodeExtra
{
	v3 vDebugCorner;
	bool bLeaf;
	bool bSpecial;
};

struct SOccluderBspNodes
{
	TFixedArray<SOccluderBspNode, OCCLUDER_BSP_MAX_NODES> Nodes;
	TFixedArray<SOccluderBspNodeExtra, OCCLUDER_BSP_MAX_NODES> NodesExtra;
	SOccluderBspNode& operator[](const int i)
	{
		return Nodes[i];
	}
	const SOccluderBspNode& operator[](const int i) const
	{
		return Nodes[i];
	}
	uint32_t Size(){return Nodes.Size();}
};


void BspDestroy(SOccluderBsp* pBsp);
void BspBuild(SOccluderBsp* pBsp, 
			  SOccluderDesc** pPlaneOccluders, uint32 nNumPlaneOccluders,
			  SOccluderDesc** pBoxOccluders, uint32 nNumBoxOccluders,
			  const SOccluderBspViewDesc& Desc);
bool BspCullObject(SOccluderBsp* pBsp, SOccluderDesc* pObject, SOccluderBspNodes* pSubBsp = 0);
void BspGetStats(SOccluderBsp* pBsp, SOccluderBspStats* pStats);
void BspClearCullStats(SOccluderBsp* pBsp);


void BspDebugPlane(SOccluderBsp* pBsp, int nPlane);


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

bool BspBuildSubBsp(SOccluderBspNodes& NodeBsp, SOccluderBsp *pBsp, SOccluderDesc *pObject);
