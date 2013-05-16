#pragma once
#include "base.h"
#include "program.h"


struct SOccluderBsp;

SOccluderBsp* BspCreate();
void BspDestroy(SOccluderBsp* pBsp);
void BspBuild(SOccluderBsp* pBsp, SOccluder* pOccluders, uint32 nNumOccluders, v3 vOrigin, v3 vDirection);
bool BspCullObject(SOccluderBsp* pBsp, SWorldObject* pObject);


