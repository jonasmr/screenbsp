                                                                     
                                                                     
                                                                     
                                             






#define OCCLUDER_EMPTY (0xc000)
#define OCCLUDER_LEAF (0x8000)
#define OCCLUDER_CLIP_MAX 0x100

struct SOccluderPlane
{
	float4 p[4];
	float4 corners[4];
	float4 normal;
};

struct SOccluderEdgeIndex
{
	uint8 	nEdge;//[0-3] edge idx
	uint16 	nOccluderIndex;
};

struct SOccluderBspNode
{
	SOccluderEdgeIndex Index;
	uint16	nInside;
	uint16	nOutside;
};

struct SOccluderBsp
{
	SOccluderPlane* pOccluders;
	uint32 nNumOccluders;
	TArray<SOccluderBspNode> Nodes;
};

int BspAddInternal(SOccluderBsp* pBsp, uint32 nOccluderIndex, uint32 nMask)
{
	int r = (int)pBsp->Nodes.Size();
	int nPrev = -1;
	for(uint32 i = 0;nMask && i < 4; ++i)
	{
		if(nMask&1)
		{
			int nIndex = (int)pBsp->Nodes.Size();
			SOccluderBspNode* pNode = pBsp->Nodes.PushBack();
			pNode->nOutside = OCCLUDER_EMPTY;
			pNode->nInside = OCCLUDER_LEAF;
			pNode->Index.nEdge = (uint8)i;
			pNode->Index.nOccluderIndex = (uint16)nOccluderIndex;
			if(nPrev >= 0)
			{
				pBsp->Nodes[nPrev].nInside = (uint16)nIndex;
			}
			nPrev = nIndex;
		}
		nMask >>= 1;
	}
	return r;
}

void BspAddOccluderRecursive(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex, uint32 nBspIndex, uint32 nEdgeMask)
{
	SOccluderBspNode Node = pBsp->Nodes[nBspIndex];
	ZASSERT(Node.Index.nOccluderIndex != nOccluderIndex);
	SOccluderPlane* pPlane = pBsp->pOccluders + Node.Index.nOccluderIndex;
	uint8 nEdge = Node.Index.nEdge;
	float4 vPlane = pPlane->p[nEdge];
	int inside[4];

	for(int i = 0; i < 4; ++i)
		inside[i] = WVectorDot4(vPlane, pOccluder->corners[i]) < -0.01f;
	uint32 nNewMaskIn = 0, nNewMaskOut = 0;
	int x = 1;
	for(int i = 0; i < 4; ++i)
	{
		if(x&nEdgeMask)
		{
			if(inside[(i-1)%4] || inside[i])
				nNewMaskIn |= x;
			if((!inside[(i-1)%4]) || (!inside[i]))
				nNewMaskOut |= x;
		}
		x <<= 1;
	}
	if(nNewMaskIn)
	{
		ZASSERT(Node.nInside != OCCLUDER_EMPTY);
		if(Node.nInside==OCCLUDER_LEAF)
		{
			pBsp->Nodes[nBspIndex].nInside = (uint16)BspAddInternal(pBsp, nOccluderIndex, nNewMaskIn);
		}
		else
		{
			BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nInside, nNewMaskIn);
		}
	}

	if(nNewMaskOut)
	{
		ZASSERT(Node.nOutside != OCCLUDER_LEAF);
		if(Node.nOutside==OCCLUDER_EMPTY)
		{
			pBsp->Nodes[nBspIndex].nOutside = (uint16)BspAddInternal(pBsp, nOccluderIndex, nNewMaskOut);
		}
		else
		{
			BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nOutside, nNewMaskOut);
		}
	}
}


void BspAddOccluder(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex)
{
	uint32 nNumNodes = (uint32)pBsp->Nodes.Size();
	if(pBsp->Nodes.IsEmpty())
	{
		uint16 nIndex = (uint16)BspAddInternal(pBsp, nOccluderIndex, 0xf);
		ZASSERT(nIndex==0);
	}
	else
	{
		BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, 0, 0xf);
	}
	uprintf("BSP NODE %d -> %d\n", nNumNodes, pBsp->Nodes.Size());
	uplotfnxt(ZString::Format("BSP NODE {0} -> {1}", nNumNodes, pBsp->Nodes.Size()));

}


void BuildBsp(SOccluderBsp* pBsp)
 {
	pBsp->Nodes.Clear();
	if(!pBsp->nNumOccluders)
		return;

	for(uint32 i = 0; i < pBsp->nNumOccluders; ++i)
	{
		BspAddOccluder(pBsp, &pBsp->pOccluders[i], i);
	}
}
ZConfigInt g_nBspFlipMask("BspFlipMask", "", 0);
ZConfigInt g_nBspDrawMask("BspDrawMask", "", -1);
void DrawBspRecursive(SOccluderBsp* pBsp, uint32 nOccluderIndex, uint32 nFlipMask, uint32 nDrawMask, float fOffset, TArray<float4>& DEBUG)
{
	SOccluderBspNode Node = pBsp->Nodes[nOccluderIndex];
	SOccluderPlane* pPlane = pBsp->pOccluders + Node.Index.nOccluderIndex;
	float4 vOffset = WSet(fOffset, 0.f, 0.f, 0.f);
	float4 v0 = WSetWOne(pPlane->corners[(Node.Index.nEdge+3) %4]);
	float4 v1 = WSetWOne(pPlane->corners[(Node.Index.nEdge) %4]);
	DEBUG.PushBack(v0);
	DEBUG.PushBack(v1);
	if(1)
	{
		for(uint32 i = 0; i < DEBUG.Size(); i += 2)
		{
			float4 v0 = DEBUG[i] + vOffset;
			float4 v1 = DEBUG[i+1] + vOffset;
			ZDEBUG_DRAWLINE(v0, v1, 0xffff44ff, true);
		}
	}
	if(0 == (nFlipMask&1))
	{
		if((Node.nInside&0x8000) == 0)
		{
			DrawBspRecursive(pBsp, Node.nInside, nFlipMask>>1, nDrawMask>>1, fOffset - 2.0f, DEBUG);
		}
	}
	else
	{
		if((Node.nOutside&0x8000) == 0)
		{
			DrawBspRecursive(pBsp, Node.nOutside, nFlipMask>>1, nDrawMask>>1, fOffset - 2.0f, DEBUG);
		}
	}
}

bool BspClipQuadR(SOccluderBsp* pBsp, uint32 nNodeIndex, uint32 nPolyIndex, uint32 nPolyVertices, uint32 nVertexIndex, float4* pVertices, uint8* , nIndices);)
{
	uint8 nIndicesIn[256];
	uint8 nIndicesOut[256];
	uint8 nIndicesTmp[256];
	uint8* nDot = alloca(2*(nPolyVertices+2));
	SOccluderBspNode* pNode = pBsp->Nodes[nOccluderIndex];
	SOccluderPlane* pOccluder = pBsp->pOccluders + pNode->Index.nOccluderIndex;
	uint32 nEdge = pNode->Index.nEdge;
	float4 vPlane = pOccluder->p[nEdge];
	float vNormalPlane = pOccluder->normal;
	
#define INSIDE 1
#define OUTSIDE 2
	//test mod planet
	int nMask = 0;
	int n = 1;
	for(uint32 i = 0; i < nPolyVertices; ++i)
	{
		float4 pVertex = pVertices[ nIndices[i + nPolyIndex ]];
		d = WVectorDot4(pVertex, vPlane) > 0.0f ? INSIDE : OUTSIDE;
		n = n && WVectorDot4(pVertex, vNormalPlane) > 0.f ? 1 : 0;
		nDot[i] = d;
		nMask |= d;
	}
	if(d == 3)
	{
		int nIndexIn = 0;
		for(uint32 i = 0; i < nPolyVertices; ++i)
		{
			int idxn = (i + 1) % nPolyVertices;
			int i0 = nDot[i];
			int i1 = nDot[idxn];
			if(INSIDE == i0)
			{
				//PUSH i0				
				
				if(OUTSIDE == i1)
				{

					//PUSH INTERSECTION
					float4 vVertex = WSetWOne(pVertices[ nIndices[ i + nPolyIndex ]]);
					float4 vToPlane = WVectorDot4(vVertex, vPlane) * WSetWZero(vPlane);
					float4 vIntersection = vVertex + vToPlane;
					uprintf("INTERSECTION IS %f\n", WVectorDot4(vIntersection, vPlane).ToFloat());
					int nIndex = nVertexIndex++;
					pVertices[nIndex] = vIntersection;

					nIndicesIn[nIndexIn++] = i + nPolyIndex;
					nIndicesIn[nIndexIn++] = nIndex;
					nIndicesOut[nIndexOut++] = nIndex;
					ZASSERT(nIndexIn < 256);
				}
				else
				{
					nIndicesIn[nIndexIn++] = i + nPolyIndex;
				}
			}
			else
			{
				if(i1 == INSIDE)
				{
					//PUSH INTERSECTION
					float4 vVertex = WSetWOne(pVertices[ nIndices[ i + nPolyIndex ]]);
					float4 vToPlane = WVectorDot4(vVertex, vPlane) * WSetWZero(vPlane);
					float4 vIntersection = vVertex + vToPlane;
					uprintf("INTERSECTION IS %f\n", WVectorDot4(vIntersection, vPlane).ToFloat());
					int nIndex = nVertexIndex++;
					pVertices[nIndex] = vIntersection;

					nIndicesIn[nIndexIn++] = nIndex;
					nIndicesOut[nIndexOut++] = nIndex;
					nIndicesOut[nIndexOut++] = i + nPolyIndex;
					ZASSERT(nNewPolyIndex < OCCLUDER_CLIP_MAX);
				}
				else
				{
					nIndicesOut[nIndexOut++] = i + nPolyIndex;
				}
			}
		}
		//remove degenerate triangles
		bool bTestIn = true;
		bool bTestOut = true;
		uint32 nNewPolyIndex = nPolyIndex + nPolyVertices;
		ZASSERT(nNewPolyIndex < OCCLUDER_CLIP_MAX);
		if(nIndexIn > 2)
		{
			int p1 = nIndexIn-2, p0 = nIndexIn-1;
			int nIndex = 0;
			uint32 i;
			for( i = 0; i < nIndexIn-2; ++i)
			{
				float1 len = WVectorLength3(WVectorCross3(pVertices[nIndexIn[i]] - pVertices[nIndexIn[p0]], pVertices[nIndexIn[p1]] - pVertices[nIndexIn[p0]]))
				if(fabs(len)> 0.001f)
				{
					pIndices[nNewPolyIndex + nIndex++] = p1;
				}
				p1 = p0;
				p0 = i;
			}
			if(nIndex > 0)
			{
				pIndices[nNewPolyIndex + nIndex++] = p0;
				pIndices[nNewPolyIndex + nIndex++] = i;
				bTestIn = BspClipQuadR(pBsp, pNode->nInside, nNewPolyIndex, nIndex, nVertexIndex, pVertices, nIndices);
			}
			else
			{
				bTestIn = true;
			}
		}

		if(nIndexOut > 2)
		{
			int p1 = nIndexOut-2, p0 = nIndexOut-1;
			int nIndex = 0;
			uint32 i;
			for( i = 0; i < nIndexOut-2; ++i)
			{
				float1 len = WVectorLength3(WVectorCross3(pVertices[nIndexOut[i]] - pVertices[nIndexOut[p0]], pVertices[nIndexOut[p1]] - pVertices[nIndexOut[p0]]))
				if(fabs(len)> 0.001f)
				{
					pIndices[nNewPolyIndex + nIndex++] = p1;
				}
				p1 = p0;
				p0 = i;
			}
			if(nIndex > 0)
			{
				pIndices[nNewPolyIndex + nIndex++] = p0;
				pIndices[nNewPolyIndex + nIndex++] = i;
				bTestIn = BspClipQuadR(pBsp, pNode->nOutside, nNewPolyIndex, nIndex, nVertexIndex, pVertices, nIndices);
			}
			else
			{
				bTestIn = true;
			}
		}


		return bTestIn && bTestOut;

	}
	else
	{
		if(d == 1)
		{
			// all in inside
			ZASSERT(pNode->nInside != OCCLUDER_EMPTY);
			if(pNode->nInside == OCCLUDER_LEAF)
			{
				return n != 0;
			}
			else
			{
				return n != 0 && BspClipQuadR(pBsp, pBsp->nInside, nPolyIndex, nPolyVertices, nVertexIndex, pVertices, nIndices);

			}

		}
		else
		{
			// all in outside
			ZASSERT(pNode->nOutside != OCCLUDER_LEAF);
			if(pNode->nOutside == OCCLUDER_EMPTY)
			{
				ZASSERT(nPolyVertices); 
				return false; // all is out, none is clipped
			}
			else
			{
				return BspClipQuadR(pBsp, pNode->nOutside, nPolyIndex, nPolyVertices, nVertexIndex, pVertices, nIndices);
			}
		}
	}
}
//returns true if 100% clipped
bool BspClipQuad(SOccluderBsp* pBsp, float4arg v0, float4arg v1, float4arg v2, float4arg v3)
{
	float4 vClipBuffer[OCCLUDER_CLIP_MAX];
	uint8 nInidices[OCCLUDER_CLIP_MAX];
	vClipBuffer[0] = v0;
	vClipBuffer[1] = v1;
	vClipBuffer[2] = v2;
	vClipBuffer[3] = v3;
	nInidices[0] = 0;
	nInidices[1] = 1;
	nInidices[2] = 2;
	nInidices[3] = 3;
	return BspClipQuadR(pBsp, 0, 0, 4, 4, &vClipBuffer[0], &nIndices[0]);
}

void RunBspTest(SOccluderBsp* pBsp, SOccluderPlane* pTest, uint32 nNumOccluders, ZRenderGraphNode** ppNodes, uint32 nNumNodes)
{
	for(uint32 i = 0; i< nNumNodes; ++i)
	{
		if(!ppNodes[i]) 
			continue;
		SMatrix mObjectToWorld = ppNodes[i]->GetObjectToWorldTransform();
		float4 vHalfSize = ppNodes[i]->GetLocalHalfSize();
		float4 vCenter = ppNodes[i]->GetLocalCenter();
		float4 vCenterWorld = WVectorTransform(mObjectToWorld, vCenter);
		float4 AABB = TransformOBBToAABB(mObjectToWorld, vHalfSize);

		float4 vCenterPoly = vCenterWorld - WSet(AABB.x(), 0.f, 0.f, 0.f);
		float4 vDir0 = WSet(0.f,  AABB.y(),  AABB.z(), 0.f);
		float4 vDir1 = WSet(0.f, -AABB.y(),  AABB.z(), 0.f);
		float4 vDir2 = WSet(0.f, -AABB.y(), -AABB.z(), 0.f);
		float4 vDir3 = WSet(0.f,  AABB.y(), -AABB.z(), 0.f);
		float4 v0 = vCenterPoly + vDir0;
		float4 v1 = vCenterPoly + vDir1;
		float4 v2 = vCenterPoly + vDir2;
		float4 v3 = vCenterPoly + vDir3;
		ZDEBUG_DRAWLINE(v0, v1, 0xff0000ff, true);
		ZDEBUG_DRAWLINE(v1, v2, 0xff0000ff, true);
		ZDEBUG_DRAWLINE(v2, v3, 0xff0000ff, true);
		ZDEBUG_DRAWLINE(v3, v0, 0xff0000ff, true);
		BspClipQuad(pBsp, v0, v1, v2, v3);
	}
	TArray<float4> DEBUG;
	DrawBspRecursive(pBsp, 0, g_nBspFlipMask, g_nBspDrawMask, -1.f, DEBUG);
}

float4 MakePlane(float4arg p, float4arg normal)
{
	float4 r = WSetW(-(WVectorDot3(normal,p).ToFloat()), normal);
	float1 vDot = WVectorDot4(WSetW(1.f, p), r);
	if(ZMath::Abs(vDot.ToFloat()) > 0.001f)
	{
		ZBREAK();
	}

	return r;
}

void BspOccluderTest(SOccluder* pOccluders, uint32 nNumOccluders, ZRenderGraphNode** ppNodes, uint32 nNumNodes)
{
	SOccluderPlane* pPlanes = ZNEW SOccluderPlane[nNumOccluders];
	uint8* nMasks = ZNEW uint8[nNumOccluders];
	memset(nMasks, 0xff, nNumOccluders);


	float4 vCameraPosition = WLoadZero();
	for(uint32 i = 0; i < nNumOccluders; ++i)
	{
		float4 vCorners[4];
		SOccluder Occ = pOccluders[i];
		float4 vNormal = WLoad3U(&Occ.vNormal.x);
		float4 vUp = WLoad3U(&Occ.vUp.x);
		float4 vLeft = WVectorCross3(vNormal, vUp);
		float4 vCenter = WLoad3U(&Occ.vCenter.x);
		vCorners[0] = vCenter + vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[1] = vCenter - vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[2] = vCenter - vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
		vCorners[3] = vCenter + vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
		SOccluderPlane& Plane = pPlanes[i];

		for(uint32 i = 0; i < 4; ++i)
		{
			float4 v0 = vCorners[i];
			float4 v1 = vCorners[(i+1) % 4];
			float4 v2 = vCameraPosition;
			float4 vCenter = (v0 + v1 + v2) / WReplicate(3.f);
			float4 vNormal = WVectorNormalize3(WVectorCross3(WVectorNormalize3(v1 - v0), WVectorNormalize3(v2 - v0)));
			float4 vEnd = vCenter + vNormal;
			Plane.p[i] = MakePlane(vCorners[i], vNormal);
			Plane.corners[i] = vCorners[i];
			ZDEBUG_DRAWLINE(v0, v1, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v1, v2, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v2, v0, (uint32)-1, true);
			ZDEBUG_DRAWLINE(vCenter, vEnd, 0xff00ffff, true);
		}
		Plane.normal = MakePlane(vCorners[0], vNormal);
	}
	SOccluderBsp Bsp;
	Bsp.pOccluders = pPlanes;
	Bsp.nNumOccluders = nNumOccluders;

	BuildBsp(&Bsp);

	RunBspTest(&Bsp, pPlanes, nNumOccluders, ppNodes, nNumNodes);

	ZDELETE(pPlanes);
}


