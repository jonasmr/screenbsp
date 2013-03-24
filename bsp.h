#pragma once
#include "base.h"
#include "program.h"


struct SOccluderBsp;

SOccluderBsp* BspCreate();
void BspDestroy(SOccluderBsp* pBsp);
void BspBuild(SOccluderBsp* pBsp, SOccluder* pOccluders, uint32 nNumOccluders, m mWorldToView);
bool BspCullObject(SOccluderBsp* pBsp, SWorldObject* pObject);


