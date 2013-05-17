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
};


void BspDestroy(SOccluderBsp* pBsp);
void BspBuild(SOccluderBsp* pBsp, SOccluder* pOccluders, uint32 nNumOccluders, const SOccluderBspViewDesc& Desc);
bool BspCullObject(SOccluderBsp* pBsp, SWorldObject* pObject);


