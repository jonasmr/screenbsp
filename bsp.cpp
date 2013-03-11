

#define OCCLUDER_EMPTY (0xc000)
#define OCCLUDER_LEAF (0x8000)

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
	int r = pBsp->Nodes.Size();
	SOccluderBspNode* pPrev = 0;
	for(uint32 i = 0;nMask && i < 4; ++i)
	{
		if(nMask&1)
		{
			SOccluderBspNode* pNode = pBsp->Nodes.PushBack();
			pNode->nOutside = OCCLUDER_EMPTY;
			pNode->nInside = OCCLUDER_LEAF;
			pNode->Index.nEdge = i;
			pNode->Index.nIndex = nOccluderIndex;
			if(pPrev)
				pPrev->nInside = pBsp->Nodes.Size()-1;
			pPrev = pNode;
		}
		nMask >>= 1;
	}
	return r;
}

void BspAddOccluderRecursive(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex, uint32 nBspIndex, uint32 nEdgeMask)
{
	SOccluderBspNode Node = pBsp->Nodes[nBspIndex];
	SOccluderPlane* pPlane = pBsp->pOccluders + Node.Index.nOccluderIndex;
	uint8 nEdge = Node.Index.nEdge;
	float4 vPlane = pPlane.p[nEdge];
	int inside[4];

	for(int i = 0; i < 4; ++i)
		inside[i] = WVectorDot4(vPlane, pOccluder.p[i]) < 0.f;
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
		if(Node.nInside&0x8000)
		{
			Node.nInside = BspAddInternal(pBsp, nOccluderIndex, nNewMaskIn);
		}
		else
		{
			BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nInside, nNewMaskIn);
		}
	}

	if(nNewMaskOut)
	{
		if(Node.nOutside&0x8000)
		{
			Node.nOutside = BspAddInternal(pBsp, nOccluderIndex, nNewMaskOut);
		}
		else
		{
			BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nInside, nNewMaskOut);
		}
	}
}


void BspAddOccluder(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex)
{
	if(pBsp->Nodes.IsEmpty())
	{
		uint16 nIndex = BspAddInternal(pBsp, nOccluderIndex, 0xf);
		ZASSERT(nIndex==0);
	}
	else
	{
		BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, 0, 0xf);
	}

}


void BuildBsp(SOccluderBsp* pBsp)
 {
	pBsp->Bsp.Clear();
	if(!pBsp->nNumOccluders)
		return;

	for(uint32 i = 0; i < pBsp->nNumOccluders; ++i)
	{
		BspAddOccluder(pBsp, pBsp->pOccluders[i], i);
	}
}

void RunBspTest(SOccluderPlane* pTest, uint32 nNumOccluders, ZRenderGraphNode** ppNodes, uint32 nNumNodes)
{
	for(uint32 i = 0; i< nNumNodes; ++i)
	{
		if(!ppNodes[i]) continue;
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
	}
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

	BuildBsp(Bsp);

	RunBspTest(pPlanes, nNumOccluders, ppNodes, nNumNodes);

	ZDELETE(pPlanes);
}