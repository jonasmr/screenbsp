#pragma once
#include "base.h"
#include "program.h"


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


void BspDestroy(SOccluderBsp* pBsp);
void BspBuild(SOccluderBsp* pBsp, SOccluder* pOccluders, uint32 nNumOccluders, SWorldObject* pWorldObject, uint32 nNumWorldObjects, const SOccluderBspViewDesc& Desc);
bool BspCullObject(SOccluderBsp* pBsp, SWorldObject* pObject);


