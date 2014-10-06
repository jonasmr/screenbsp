#include "fixedarray.h"
#include "bsp.h"
#include "microprofileinc.h"
#include "mathsse.h"
#include "debug.h"

// todo:
//  prioritize after clipping vs frustum
//  make prioritization use proper area
//  rename cull to isvisible

// clean math shit
// proper debug code.

#include <algorithm>
#define OCCLUDER_EMPTY (0xc000)
#define OCCLUDER_INVALID (0xffff)
#define OCCLUDER_LEAF (0x8000)
#define OCCLUDER_CLIP_MAX 0x100
#define USE_FRUSTUM 1
#define BSP_ADD_FRONT 1
#define PLANE_TEST_EPSILON 0.0000001f
#define USE_EXCLUDE_MASK 1
#define USE_EXCLUDE_MASK_OVERLAP 1

#define BSP_DUMP_PRINTF(pBsp, ...) do{if(pBsp->Debug.DumpFrame){ uprintf(__VA_ARGS__); } }while(0)


#pragma warning( disable : 4018 )
#pragma warning( disable : 4996 )

struct SOccluderPlane;
struct SBspEdgeIndex;
struct SOccluderBspNode;


enum ClipPolyResult : int
{
	ECPR_CLIPPED = 0,
	ECPR_INSIDE = 0x1,
	ECPR_OUTSIDE = 0x2,
	ECPR_BOTH = 0x3,
};


bool BspCullObject(SOccluderBsp *pBsp, SOccluderBsp* pObject);

TFixedArray<uint32, 512> g_BspSkip;

void SkipAdd(uint32 v)
{
	g_BspSkip.PushBack(v);
}

void SkipAddRange(uint32 b, uint32 e)
{
	for(int i = b; i < e; ++i)
		g_BspSkip.PushBack(i);
}

void SkipRemove(uint32 v)
{
	for(int i = 0; i < g_BspSkip.Size();)
	{
		if(g_BspSkip[i] == v)
		{
			g_BspSkip.EraseSwapLast(i);
		}
		else
		{
			++i;
		}
	}
}
void SkipRemoveRange(uint32 b, uint32 e)
{
	for(int i = 0; i < g_BspSkip.Size();)
	{
		if(g_BspSkip[i] >= b && g_BspSkip[i] < e)
		{
			g_BspSkip.EraseSwapLast(i);
		}
		else
		{
			++i;
		}
	}
}
void SkipClear()
{
	g_BspSkip.Clear();
}
bool SkipIndex(uint32 nIndex)
{
	for(uint32 v : g_BspSkip)
		if(v == nIndex)
			return true;
	return false;
}



struct SOccluderBsp
{
	static const uint16 PLANE_FLIP_BIT = 0x8000;

	SOccluderBspViewDesc DescOrg;
	m mtobsp;
	m mfrombsp;
	uint32 nDepth;

	v3 vFrustumCorners[4];
	v4 vFrustumPlanes[4];
	TFixedArray<v4, OCCLUDER_BSP_MAX_PLANES> Planes;
	TFixedArray<v3, OCCLUDER_BSP_MAX_PLANES> Corners;
	SOccluderBspNodes Nodes;
	SOccluderBspStats Stats;

	//debugging helpers
	struct
	{
		uint32 OccluderDebugDrawClipResult;
		uint32 OccluderDrawEdges;
		uint32 OccluderDrawOccluders;
		uint32 BreakOnClipIndex;
		uint32 DebugPlane;
		uint32 BspDebugPlaneCounter;
		uint32 nInsideOutSide;
		uint32 Check;
		uint32 DumpFrame;
		uint32 nDumpAdd;
		uint32 nDrawPoly;
		uint32 nShowClipLevelSubCounter;
		uint32 nShowClipLevelSub;
		int nShowClipLevel;
		uint32 nFirst;
		uint32 nDrawCount;
	} Debug;

	SOccluderBsp()
	{
		Debug.OccluderDebugDrawClipResult = 0;
		Debug.OccluderDrawEdges = 2;
		Debug.OccluderDrawOccluders = 1;
		Debug.BreakOnClipIndex = -1;
		Debug.DebugPlane = -1;
		Debug.BspDebugPlaneCounter = 0;
		Debug.nInsideOutSide = 0x3;
		Debug.Check = 0;
		Debug.DumpFrame = 0;
		Debug.nDumpAdd = -1;
		Debug.nShowClipLevelSubCounter = 0;
		Debug.nShowClipLevelSub = 0;
		Debug.nShowClipLevel = -1;
		Debug.nFirst = 0;
		Debug.nDrawPoly = 0;

	}


};
float BspPlaneTestNew(v4 plane0, v4 plane1, v4 ptest);

inline bool BspIsLeafNode(const SOccluderBspNode &Node)
{
	return Node.nInside == OCCLUDER_LEAF;
}

inline v4 BspGetPlane(SOccluderBsp *pBsp, uint16 nIndex)
{
	v4 vPlane = pBsp->Planes[nIndex & ~SOccluderBsp::PLANE_FLIP_BIT];
	if(SOccluderBsp::PLANE_FLIP_BIT & nIndex)
		return v4neg(vPlane);
	else
		return vPlane;
}

inline v3 BspGetCorner(SOccluderBsp *pBsp, uint16 nIndex)
{
	v3 vCorner = pBsp->Corners[nIndex & ~SOccluderBsp::PLANE_FLIP_BIT];
	return vCorner;
}

inline v4 BspGetPlaneFromNode(SOccluderBsp* pBsp, SOccluderBspNodes* pNodes, uint16 nNodeIndex)
{
	uint16 nIndex = pNodes->Nodes[nNodeIndex].nPlaneIndex;
	v4 vPlane = pBsp->Planes[nIndex & ~SOccluderBsp::PLANE_FLIP_BIT];
	if(SOccluderBsp::PLANE_FLIP_BIT & nIndex)
		return v4neg(vPlane);
	else
		return vPlane;
}

inline v3 BspGetCornerFromNode(SOccluderBsp *pBsp, SOccluderBspNodes* pNodes, uint16 nNodeIndex)
{
	uint16 nIndex = pNodes->Nodes[nNodeIndex].nPlaneIndex;
	v3 vCorner = pBsp->Corners[nIndex & ~SOccluderBsp::PLANE_FLIP_BIT];
	return vCorner;
}


inline uint16 BspMakePlaneIndex(uint16 nIndex, bool bFlip)
{
	ZASSERT((nIndex & SOccluderBsp::PLANE_FLIP_BIT) == 0);
	if(bFlip)
		return SOccluderBsp::PLANE_FLIP_BIT | nIndex;
	else
		return nIndex;
}

SOccluderBsp *g_pBsp = 0;
void BspOccluderDebugDraw(v4 *pVertexNew, v4 *pVertexIn, uint32 nColor, uint32 nFill = 0);
void BspAddOccluderInternal(SOccluderBsp *pBsp, v4 *pPlanes, v3* vCorners, uint32 nNumPlanes);
bool BspAddOccluder(SOccluderBsp *pBsp, v3 vCenter, v3 vNormal, v3 vUp, float fUpSize, v3 vLeft,
					float fLeftSize, bool bFlip = false);
bool BspAddOccluder(SOccluderBsp *pBsp, v3 *vCorners, uint32 nNumCorners);

int BspAddInternal(SOccluderBsp *pBsp, uint16 nParent, bool bOutsideParent, uint16 *nPlaneIndices,
				   uint16 nNumPlanes, uint32 nExcludeMask, uint32 nLevel);
void BspAddRecursive(SOccluderBsp *pBsp, uint32 nBspIndex, uint16 *nPlaneIndices, uint16 nNumPlanes,
					 uint32 nExcludeMask, uint32 nLevel);

void BspDebugDrawHalfspace(v3 vNormal, v3 vPosition);
v3 BspGetCorner(SOccluderBsp *pBsp, uint16 *Poly, uint32 nEdges, uint32 i);
int BspFindParent(SOccluderBspNodes* pNodes, int nChildIndex);
int BspFindParent(SOccluderBspNodes* pNodes, int nChildIndex, bool &bInside);
v3 BspToPlane2(v4 p);
float BspAreaEstimate(v4* vPlanes, uint32 nNumPlanes);
float BspAreaEstimate(SOccluderBsp* pBsp, uint16* pIndices, uint32 nNumPlanes);
ClipPolyResult BspClipLeaf(SOccluderBsp* pBsp, v4 vPlane, uint16* Poly, uint32 nNumEdges);



// uint32_t g_nDebugDrawPoly = 1;

v3 BspPlaneIntersection(v4 p0, v4 p1, v4 p2)
{
//	MICROPROFILE_SCOPEIC("BSP", "BspPlaneIntersect");
	float x = -(-p0.w * p1.y * p2.z + p0.w * p2.y * p1.z + p1.w * p0.y * p2.z - p1.w * p2.y * p0.z -
				p2.w * p0.y * p1.z + p2.w * p1.y * p0.z) /
			  (-p0.x * p1.y * p2.z + p0.x * p2.y * p1.z + p1.x * p0.y * p2.z - p1.x * p2.y * p0.z -
			   p2.x * p0.y * p1.z + p2.x * p1.y * p0.z);
	float y = -(p0.w * p1.x * p2.z - p0.w * p2.x * p1.z - p1.w * p0.x * p2.z + p1.w * p2.x * p0.z +
				p2.w * p0.x * p1.z - p2.w * p1.x * p0.z) /
			  (-p0.x * p1.y * p2.z + p0.x * p2.y * p1.z + p1.x * p0.y * p2.z - p1.x * p2.y * p0.z -
			   p2.x * p0.y * p1.z + p2.x * p1.y * p0.z);
	float z = -(p0.w * p1.x * p2.y - p0.w * p2.x * p1.y - p1.w * p0.x * p2.y + p1.w * p2.x * p0.y +
				p2.w * p0.x * p1.y - p2.w * p1.x * p0.y) /
			  (p0.x * p1.y * p2.z - p0.x * p2.y * p1.z - p1.x * p0.y * p2.z + p1.x * p2.y * p0.z +
			   p2.x * p0.y * p1.z - p2.x * p1.y * p0.z);

	return v3init(x, y, z);
}

static const char *Spaces(int n)
{
	static char buf[256];
	ZASSERT(n < sizeof(buf) - 1);
	for(int i = 0; i < n; ++i)
		buf[i] = ' ';
	buf[n] = 0;
	return &buf[0];
}

void BspDump(SOccluderBsp *pBsp, int nNode, int nLevel)
{
	SOccluderBspNode *pNode = pBsp->Nodes.Nodes.Ptr() + nNode;
	v4 vPlane = BspGetPlane(pBsp, pNode->nPlaneIndex);
	int iParent = BspFindParent(&pBsp->Nodes, nNode);
	float dot3 = 0.f;
	if(iParent >= 0)
	{
		v4 vParentPlane = BspGetPlane(pBsp, pBsp->Nodes[iParent].nPlaneIndex);
		dot3 = v3dot(vParentPlane.tov3(), vPlane.tov3());

	}
	uprintf("D%s0x%04x: plane[%04x][i:%04x,o:%04x] p %04x %f %f %f %f:: d3 %f\n", Spaces(3 * nLevel), nNode,
			pNode->nPlaneIndex, pNode->nInside, pNode->nOutside, iParent, vPlane.x, vPlane.y, vPlane.z, vPlane.w, dot3);
	if(0 == (0x8000 & pNode->nInside))
		BspDump(pBsp, pNode->nInside, nLevel + 1);
	else
		uprintf("D%s0x%04x\n", Spaces(3 * (nLevel + 1)), pNode->nInside);

	if(0 == (0x8000 & pNode->nOutside))
		BspDump(pBsp, pNode->nOutside, nLevel + 1);
	else
		uprintf("D%s0x%04x\n", Spaces(3 * (nLevel + 1)), pNode->nOutside);
}



void BspDumpNodes(SOccluderBspNodes *ppNodes, int nNode, int nLevel)
{
	if(!ppNodes->Size())
		return;
	SOccluderBspNode *pNode = &ppNodes->Nodes[nNode];
	uprintf("D%s0x%04x: plane[%04x]   [i:%04x,o:%04x] parent %04x\n", Spaces(3 * nLevel), nNode,
			pNode->nPlaneIndex, pNode->nInside, pNode->nOutside, BspFindParent(ppNodes, nNode));
	if(0 == (0x8000 & pNode->nInside))
		BspDumpNodes(ppNodes, pNode->nInside, nLevel + 1);
	else
		uprintf("D%s0x%04x\n", Spaces(3 * (nLevel + 1)), pNode->nInside);

	if(0 == (0x8000 & pNode->nOutside))
		BspDumpNodes(ppNodes, pNode->nOutside, nLevel + 1);
	else
		uprintf("D%s0x%04x\n", Spaces(3 * (nLevel + 1)), pNode->nOutside);
}



SOccluderBsp *BspCreate()
{
	return new SOccluderBsp;
}

void BspDestroy(SOccluderBsp *pBsp)
{
	delete pBsp;
}
v2 BspRotate2(v2 v, float fAngle)
{
	float fSin = sinf(fAngle);
	float fCos = cosf(fAngle);
	v2 r;
	r.x = fCos* v.x + fSin * v.y;
	r.y = -fSin * v.x + fCos * v.y;
	return r;
}

#if 0
//debugging, intentionallly not removed
void BspDebugTest(v3 vEye)
{
	static float r0 = 0.f, r1 = 0.f, r2 = 0.f;
	static bool inc0 = false;
	static bool inc1 = false;
	static bool inc2 = false;
	static bool neg = false;
	if(inc0)
		r0 += 0.02f;
	if(inc1)
		r1 += 0.02f;
	if(inc2)
		r2 += 0.02f;
	if(g_KeyboardState.keys[SDL_SCANCODE_F] & BUTTON_RELEASED)
		inc0 = !inc0;
	if(g_KeyboardState.keys[SDL_SCANCODE_G] & BUTTON_RELEASED)
		inc1 = !inc1;
	if(g_KeyboardState.keys[SDL_SCANCODE_H] & BUTTON_RELEASED)
		inc2 = !inc2;
	if(g_KeyboardState.keys[SDL_SCANCODE_B] & BUTTON_RELEASED)
		neg = !neg;

	v2 n0 = v2normalize(v2init(0.5f, 0.5f));
	v2 n1 = v2normalize(v2init(0.5f, -0.5f));
	v2 n2 = v2normalize(v2init(1.f, 0.f));
	n0 = BspRotate2(n0, r0);
	n1 = BspRotate2(n1, r1);
	
	v3 vStart0 = v3init(2.f, 1.f, 0.f);
	v3 vStart1 = v3init(2.f, 0.f, 0.f);
	v3 vStart2 = v3init(2.f, 0.5f, 0.f + sin(r2));
	v3 vNormal0 = v3init(0.f, n0.y, n0.x);
	v3 vNormal1 = v3init(0.f, n1.y, n1.x);
	v3 vNormal2 = v3init(0.f, n2.y, n2.x);
	if(neg)
		vNormal2 = -vNormal2;
	v3 vDir0 = vStart0 - vEye;
	v3 vDir1 = vStart1 - vEye;
	v3 vDir2 = vStart2 - vEye;

	vNormal0 = v3cross(vDir0, vNormal0);
	vNormal0 = v3normalize(v3cross(vNormal0, vDir0));

	vNormal1 = v3cross(vDir1, vNormal1);
	vNormal1 = v3normalize(v3cross(vNormal1, vDir1));

	vNormal2 = v3cross(vDir2, vNormal2);
	vNormal2 = v3normalize(v3cross(vNormal2, vDir2));

	ZDEBUG_DRAWPLANE(vNormal0, vStart0,0xff00ff00);
	ZDEBUG_DRAWPLANE(vNormal1, vStart1,0xff44aa44);

	float fTest = BspPlaneTestNew(v4init(vNormal0, 0.0f), v4init(vNormal1, 0.f), v4init(vNormal2,0.f));

	uplotfnxt("***************************************************DEBUG PLANE TEST");
	uplotfnxt(" VALUE %9.6f", fTest);
	uplotfnxt("***************************************************DEBUG PLANE TEST");
	if(fTest < 0.f)
		ZDEBUG_DRAWPLANE(vNormal2, vStart2,0xff0000ff);
	else
		ZDEBUG_DRAWPLANE(vNormal2, vStart2,0xffff0000);
}
#endif

bool BspCanAddNodes(SOccluderBsp* pBsp, SOccluderBspNodes* pNodes, uint32 nNumNodes)
{
	if(pNodes->Nodes.Capacity() - pNodes->Nodes.Size() < nNumNodes || pNodes->Nodes.Size() > pBsp->DescOrg.nNodeCap)
		return false;
	return true;
}


struct SBspPotentialPoly
{
	uint16 nIndex;
	uint16 nCount;
	float fArea;
};

struct SBspPotentialOccluders
{
	TFixedArray<v4, OCCLUDER_BSP_MAX_PLANES> Planes;
	TFixedArray<SBspPotentialPoly, OCCLUDER_BSP_MAX_PLANES> PotentialPolys;

	//debug only stuff
	TFixedArray<v3, OCCLUDER_BSP_MAX_PLANES> Corners;

};

void BspAddPotentialOccluder(SBspPotentialOccluders& PotentialOccluders, v4* vPlanes, v3* vCorners, uint32 nNumPlanes)
{
	uint32 nIndex = PotentialOccluders.Planes.Size();
	PotentialOccluders.Planes.PushBack(vPlanes, nNumPlanes);
	PotentialOccluders.Corners.PushBack(vCorners, nNumPlanes);
	SBspPotentialPoly P = { (uint16)nIndex, 5, 0.f };
	PotentialOccluders.PotentialPolys.PushBack(P);
}

bool BspFrustumOutside(v4 vPlane, v4* vPlanes, uint32 nNumPlanes)
{
	bool R = true;
	for(uint32 i = 0; i < nNumPlanes; ++i)
	{
		v4 p0 = vPlanes[i];
		v4 p1 = vPlanes[(i+1)%nNumPlanes];
		R = R && BspPlaneTestNew(p0, p1, vPlane) < 0.f;
	}
	return R;

}

bool BspFrustumDisjoint(v4* pPlanes0, uint32 nNumPlanes0, v4* pPlanes1, uint32 nNumPlanes1)
{
	bool bSum = false;
	for(uint32 i = 0; i < nNumPlanes0; ++i)
	{
		v4 vPlane = pPlanes0[i];
		bool bTest = BspFrustumOutside(vPlane, pPlanes1, nNumPlanes1);
		bSum = bSum || bTest;
	}
	for(uint32 i = 0; i < nNumPlanes1; ++i)
	{
		v4 vPlane = pPlanes0[i];
		bool bTest = BspFrustumOutside(vPlane, pPlanes1, nNumPlanes1);
		bSum = bSum || bTest;
	}
	return bSum;
}


bool BspAddPotentialQuad(SOccluderBsp* pBsp, SBspPotentialOccluders& PotentialOccluders, v3 *vCorners,
						 uint32 nNumCorners)
{
	ZASSERT(nNumCorners == 4);
	const v3 vOrigin = v3zero();
	const v3 vDirection = v3init(0, 0, -1); // vLocalDirection

	//uint32 nIndex = PotentialOccluders.Planes.Size();
	v4 *pPlanes = (v4*)alloca(5*sizeof(v4));//PotentialOccluders.Planes.PushBackN(5);
	v3 *pCorners = (v3*)alloca(5*sizeof(v3));//PotentialOccluders.Corners.PushBackN(5);
	v3 vInnerNormal = v3normalize(v3cross(vCorners[1] - vCorners[0], vCorners[3] - vCorners[0]));
	bool bFlip = false;
	v4 vNormalPlane = v4makeplane(vCorners[0], vInnerNormal);
	{
		float fDot0 = v3dot(vCorners[0] - vOrigin, vDirection);
		float fDot1 = v3dot(vCorners[1] - vOrigin, vDirection);
		float fDot2 = v3dot(vCorners[2] - vOrigin, vDirection);
		float fDot3 = v3dot(vCorners[3] - vOrigin, vDirection);
		float fMinDot = Min(fDot0, fDot1);
		fMinDot = Min(fDot2, fMinDot);
		fMinDot = Min(fDot3, fMinDot);
		float fMaxDot = Max(fDot0, fDot1);
		fMaxDot = Max(fDot2, fMaxDot);
		fMaxDot = Max(fDot3, fMaxDot);
		if(fMaxDot < 0.f) //all behind eye
		{
			return false;
		}
		// project origin onto plane
		float fDist = v4dot(vNormalPlane, v4init(vOrigin, 1.f));
		if(fabs(fDist) < pBsp->DescOrg.fZNear) // if plane is very close to camera origin,
											   // discard to avoid intersection with near plane
		{
			if(fMinDot < 1.5 * pBsp->DescOrg.fZNear) // _unless_ all 4 points clearly in front
			{
				return false;
			}
		}
		v3 vPointOnPlane = vOrigin - fDist * vNormalPlane.tov3();
		float fFoo = v4dot(vNormalPlane, v4init(vPointOnPlane, 1.f));
		ZASSERT(fabs(fFoo) < 1e-2f);

		if(fDist > 0)
		{
			bFlip = true;
			vInnerNormal = -vInnerNormal;
			vNormalPlane = -vNormalPlane;
		}
	}
	pPlanes[4] = vNormalPlane;
	pCorners[4] = vCorners[0];
	for(uint32 i = 0; i < 4; ++i)
	{
		v3 v0 = vCorners[i];
		v3 v1 = vCorners[(i + 1) % 4];
		pCorners[i] = vCorners[i];
		v3 v2 = vOrigin;
		v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
		v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
		v3 vEnd = vCenter + vNormal;
		pPlanes[i] = v4init(vNormal, 0.f);
		if(bFlip) 
			pPlanes[i] = -pPlanes[i];
		if(pBsp->Debug.OccluderDrawEdges & 1)
		{
			v0 = mtransform(pBsp->mfrombsp, v0);
			v1 = mtransform(pBsp->mfrombsp, v1);
			v2 = mtransform(pBsp->mfrombsp, v2);
			ZDEBUG_DRAWLINE(v0, v1, (uint32) - 1, true);
			ZDEBUG_DRAWLINE(v1, v2, (uint32) - 1, true);
			ZDEBUG_DRAWLINE(v2, v0, (uint32) - 1, true);
		}
	}
	if(BspFrustumDisjoint(pPlanes, 4, &pBsp->vFrustumPlanes[0], 4))
	{
		return false;
	}
	BspAddPotentialOccluder(PotentialOccluders, pPlanes, pCorners, 5);
	return true;
}

bool BspAddPotentialBoxQuad(SOccluderBsp* pBsp, SBspPotentialOccluders& PotentialOccluders,
	v3 vCenter, v3 vNormal, v3 vUp, float fUpSize, v3 vLeft, float fLeftSize, bool bFlip)
{
	v3 vCorners[4];
	if(bFlip)
	{
		vCorners[3] = vCenter + vUp * fUpSize - vLeft * fLeftSize;
		vCorners[2] = vCenter - vUp * fUpSize - vLeft * fLeftSize;
		vCorners[1] = vCenter - vUp * fUpSize + vLeft * fLeftSize;
		vCorners[0] = vCenter + vUp * fUpSize + vLeft * fLeftSize;
	}
	else
	{
		vCorners[0] = vCenter + vUp * fUpSize + vLeft * fLeftSize;
		vCorners[1] = vCenter - vUp * fUpSize + vLeft * fLeftSize;
		vCorners[2] = vCenter - vUp * fUpSize - vLeft * fLeftSize;
		vCorners[3] = vCenter + vUp * fUpSize - vLeft * fLeftSize;
	}
	// v3 pos = mtransform(pBsp->mfrombsp, vCorners[0]);
	// ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);
	// pos = mtransform(pBsp->mfrombsp, vCorners[1]);
	// ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);
	// pos = mtransform(pBsp->mfrombsp, vCorners[2]);
	// ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);
	// pos = mtransform(pBsp->mfrombsp, vCorners[3]);
	// ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);

	const v3 vToCenter = vCenter; //- pBsp->vLocalOrigin;
	const float fDot = v3dot(vToCenter, vNormal);
//	uplotfnxt("DOT IS %f", fDot);
	if(fDot < 0.03f) // reject backfacing and gracing polygons
	{
		return BspAddPotentialQuad(pBsp, PotentialOccluders, &vCorners[0], 4);
	}
	else
	{
	//	uplotfnxt("REJECT BECAUSE DOT IS %f", fDot);
		return false;
	}
}


void BspAddPotentialBoxes(SOccluderBsp* pBsp, SBspPotentialOccluders& PotentialOccluders, SOccluderDesc** pBoxOccluders, uint32 nNumBoxOccluders)
{
	v3 vLocalOrigin = v3zero();
	m mtobsp = pBsp->mtobsp;
	for(uint32 i = 0; i < nNumBoxOccluders; ++i)
	{
		SOccluderDesc* pObject = pBoxOccluders[i];
		{
			m mat;
			memcpy(&mat, pObject->ObjectToWorld, sizeof(mat));
			mat = mmult(mtobsp, mat);

			v3 vSize;
			memcpy(&vSize, pObject->Size, sizeof(vSize));
			v3 vCenter = mat.trans.tov3();
			v3 vX = mat.x.tov3();
			v3 vY = mat.y.tov3();
			v3 vZ = mat.z.tov3();
			vSize *= BSP_BOX_SCALE;

			v3 vCenterX = vCenter + vX * vSize.x;
			v3 vCenterY = vCenter + vY * vSize.y;
			v3 vCenterZ = vCenter + vZ * vSize.z;
			v3 vCenterXNeg = vCenter - vX * vSize.x;
			v3 vCenterYNeg = vCenter - vY * vSize.y;
			v3 vCenterZNeg = vCenter - vZ * vSize.z;

			const v3 vToCenterX = vCenterX - vLocalOrigin;
			const v3 vToCenterY = vCenterY - vLocalOrigin;
			const v3 vToCenterZ = vCenterZ - vLocalOrigin;
			bool bFrontX = v3dot(vX, vToCenterX) < 0.f;
			bool bFrontY = v3dot(vY, vToCenterY) < 0.f;
			bool bFrontZ = v3dot(vZ, vToCenterZ) < 0.f;

			vX = bFrontX ? vX : -vX;
			vY = bFrontY ? vY : -vY;
			vZ = bFrontZ ? vZ : -vZ;
			vCenterX = bFrontX ? vCenterX : vCenterXNeg;
			vCenterY = bFrontY ? vCenterY : vCenterYNeg;
			vCenterZ = bFrontZ ? vCenterZ : vCenterZNeg;

			//if(0 == (pObject->nFlags & SObject::OCCLUSION_BOX_SKIP_X))
			{
				BspAddPotentialBoxQuad(pBsp, PotentialOccluders, vCenterY, vY, vZ, vSize.z, vX, vSize.x, true);
			}
			//if(0 == (pObject->nFlags & SObject::OCCLUSION_BOX_SKIP_Y))
			{
				BspAddPotentialBoxQuad(pBsp, PotentialOccluders, vCenterX, vX, vY, vSize.y, vZ, vSize.z, true);
			}
			//if(0 == (pObject->nFlags & SObject::OCCLUSION_BOX_SKIP_Z))
			{
				BspAddPotentialBoxQuad(pBsp, PotentialOccluders, vCenterZ, vZ, vY, vSize.y, vX, vSize.x, true);
			}
		}
	}
}

void BspAddPotentialOccluders(SOccluderBsp *pBsp, SBspPotentialOccluders &P, SOccluderDesc** pPlaneOccluders,
							  uint32 nNumPlaneOccluders)
{
	m mtobsp = pBsp->mtobsp;
	for(uint32 k = 0; k < nNumPlaneOccluders; ++k)
	{
		v3 vCorners[4];
		SOccluderDesc Occ = *pPlaneOccluders[k];
		m temp;
		memcpy(&temp, &Occ.ObjectToWorld, sizeof(temp));
		m mat = mmult(mtobsp, temp);
		v3 vNormal = mat.z.tov3();
		v3 vUp = mat.y.tov3();
		v3 vLeft = v3cross(vNormal, vUp);
		v3 vCenter = mat.trans.tov3();
		vCorners[0] = vCenter + vUp * Occ.Size[1] + vLeft * Occ.Size[0];
		vCorners[1] = vCenter - vUp * Occ.Size[1] + vLeft * Occ.Size[0];
		vCorners[2] = vCenter - vUp * Occ.Size[1] - vLeft * Occ.Size[0];
		vCorners[3] = vCenter + vUp * Occ.Size[1] - vLeft * Occ.Size[0];
		BspAddPotentialQuad(pBsp, P, &vCorners[0], 4);
		// BspAddOccluder(pBsp, &vCorners[0], 4);
		vCorners[0] = mtransform(pBsp->mfrombsp, vCorners[0]);
		vCorners[1] = mtransform(pBsp->mfrombsp, vCorners[1]);
		vCorners[2] = mtransform(pBsp->mfrombsp, vCorners[2]);
		vCorners[3] = mtransform(pBsp->mfrombsp, vCorners[3]);
		ZDEBUG_DRAWLINE(vCorners[0], vCorners[1], -1, 0);
		ZDEBUG_DRAWLINE(vCorners[0], vCorners[3], -1, 0);
		ZDEBUG_DRAWLINE(vCorners[2], vCorners[1], -1, 0);
		ZDEBUG_DRAWLINE(vCorners[2], vCorners[3], -1, 0);
	}
}

void BspAddFrustum(SOccluderBsp *pBsp, SBspPotentialOccluders& P, v3 vLocalDirection, v3 vLocalRight, v3 vLocalUp, v3 vLocalOrigin)
{
	// FRUSTUM
	if(USE_FRUSTUM)
	{
		v3 vFrustumCorners[4];
		SOccluderBspViewDesc Desc = pBsp->DescOrg;

		float fAngle = (Desc.fFovY * PI / 180.f) / 2.f;
		float fCA = cosf(fAngle);
		float fSA = sinf(fAngle);
		float fY = fSA / fCA;
		float fX = fY / Desc.fAspect;
		const float fDist = 3;
		const v3 vUp = vLocalUp;

		vFrustumCorners[0] = fDist * (vLocalDirection + fY * vUp + fX * vLocalRight) + vLocalOrigin;
		vFrustumCorners[1] = fDist * (vLocalDirection - fY * vUp + fX * vLocalRight) + vLocalOrigin;
		vFrustumCorners[2] = fDist * (vLocalDirection - fY * vUp - fX * vLocalRight) + vLocalOrigin;
		vFrustumCorners[3] = fDist * (vLocalDirection + fY * vUp - fX * vLocalRight) + vLocalOrigin;

		ZASSERTNORMALIZED3(vLocalDirection);
		ZASSERTNORMALIZED3(vLocalRight);
		ZASSERTNORMALIZED3(vUp);

		ZASSERT(P.Planes.Size() == 0);
		//for(uint32 i = 0; i < 4; ++i)
		for(uint32 i = 0; i < 4; ++i)
		{
			v3 v0 = vFrustumCorners[i];
			v3 v1 = vFrustumCorners[(i + 1) % 4];
			v3 v2 = vLocalOrigin;
			v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
			v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
			v3 vEnd = vCenter + vNormal;
			v4 vPlane = v4init(-vNormal, 0.f);
			pBsp->vFrustumPlanes[i] = vPlane;
			pBsp->vFrustumCorners[i] = vFrustumCorners[i];

			P.Planes.PushBack(vPlane);
			pBsp->Planes.PushBack(vPlane);
			P.Corners.PushBack(vFrustumCorners[i]);
			pBsp->Corners.PushBack(vFrustumCorners[i]);

			int nIndex = (int)pBsp->Nodes.Nodes.Size();
			SOccluderBspNode* pNode = pBsp->Nodes.Nodes.PushBack();
			SOccluderBspNodeExtra* pNodeExtra = pBsp->Nodes.NodesExtra.PushBack();
			pNode->nPlaneIndex = i;
			pNode->nInside = OCCLUDER_LEAF;
			pNode->nOutside = i == 3 ? OCCLUDER_EMPTY : (i+1);
			pNodeExtra->bLeaf = false;
			pNodeExtra->bSpecial = true;
			pNodeExtra->vDebugCorner = pBsp->Corners[i];
		}
		if(pBsp->Debug.OccluderDrawEdges & 2)
		{
			v3 vCornersWorldSpace[4];
			v3 vWorldOrigin = pBsp->DescOrg.vOrigin;
			vCornersWorldSpace[0] = mtransform(pBsp->mfrombsp, vFrustumCorners[0]);
			vCornersWorldSpace[1] = mtransform(pBsp->mfrombsp, vFrustumCorners[1]);
			vCornersWorldSpace[2] = mtransform(pBsp->mfrombsp, vFrustumCorners[2]);
			vCornersWorldSpace[3] = mtransform(pBsp->mfrombsp, vFrustumCorners[3]);
			ZDEBUG_DRAWLINE(vWorldOrigin, vCornersWorldSpace[0], 0, true);
			ZDEBUG_DRAWLINE(vWorldOrigin, vCornersWorldSpace[1], 0, true);
			ZDEBUG_DRAWLINE(vWorldOrigin, vCornersWorldSpace[2], 0, true);
			ZDEBUG_DRAWLINE(vWorldOrigin, vCornersWorldSpace[3], 0, true);
			ZDEBUG_DRAWLINE(vCornersWorldSpace[0], vCornersWorldSpace[1], 0, true);
			ZDEBUG_DRAWLINE(vCornersWorldSpace[1], vCornersWorldSpace[2], 0, true);
			ZDEBUG_DRAWLINE(vCornersWorldSpace[2], vCornersWorldSpace[3], 0, true);
			ZDEBUG_DRAWLINE(vCornersWorldSpace[3], vCornersWorldSpace[0], 0, true);
		}
	}
}

void BspDebugPlane(SOccluderBsp* pBsp, int nPlane)
{
	pBsp->Debug.DebugPlane = nPlane;
}

void BspDebugNextDrawMode(SOccluderBsp* pBsp)
{
	pBsp->Debug.OccluderDrawOccluders = (pBsp->Debug.OccluderDrawOccluders + 1) % 4;

}
void BspDebugNextDrawClipResult(SOccluderBsp* pBsp)
{
	pBsp->Debug.OccluderDebugDrawClipResult = (pBsp->Debug.OccluderDebugDrawClipResult + 1) % 2;
}

void BspDebugNextPoly(SOccluderBsp* pBsp)
{
	pBsp->Debug.nDrawPoly++;
}

void BspDebugPreviousPoly(SOccluderBsp* pBsp)
{
	pBsp->Debug.nDrawPoly--;
}
void BspDebugShowClipLevelNext(SOccluderBsp* pBsp)
{
	pBsp->Debug.nShowClipLevel++;
}
void BspDebugShowClipLevelPrevious(SOccluderBsp* pBsp)
{
	pBsp->Debug.nShowClipLevel++;
}

void BspDebugToggleInsideOutside(SOccluderBsp* pBsp)
{
	pBsp->Debug.nInsideOutSide = (pBsp->Debug.nInsideOutSide%3)+1;
}

void BspDebugClipLevelSubNext(SOccluderBsp* pBsp)
{
	pBsp->Debug.nShowClipLevelSub++;
}
void BspDebugClipLevelSubPrev(SOccluderBsp* pBsp)
{
	pBsp->Debug.nShowClipLevelSub--;
}

int BspDebugDumpFrame(SOccluderBsp* pBsp, int val)
{
	int v = pBsp->Debug.DumpFrame;
	pBsp->Debug.DumpFrame = val;
	uprintf("****************************FULL TREE DUMP\n");
	BspDump(pBsp, 0, 0);
	uprintf("****************************FULL TREE DUMP\n");
	return v;

}

uint32_t BspNumPotentialPolys(uint32_t nNumPlaneOccluders, uint32_t nNumBoxOccluders)
{
	return 4 + nNumPlaneOccluders + nNumBoxOccluders * 4;
}

uint32_t BspBuildDebugSearch(SOccluderBsp* pBsp, SOccluderDesc** pPlaneOccluders, uint32 nNumPlaneOccluders,
			  SOccluderDesc** pBoxOccluders, uint32 nNumBoxOccluders,
			  const SOccluderBspViewDesc& Desc,
			  SOccluderDesc* pObject,
			  bool* pDebugMask,
			  uint32_t nDebugMaskSize)
{
	uint32_t nNumObjects = 0;
	uint32_t nNumAdded = 1;

	BspBuild(pBsp, pPlaneOccluders, nNumPlaneOccluders, pBoxOccluders, nNumBoxOccluders, Desc);
	bool bCulled = BspCullObject(pBsp, pObject);
	ZASSERT(bCulled);
	const uint32_t nNumPotential = BspNumPotentialPolys(nNumPlaneOccluders, nNumBoxOccluders);
	ZASSERT(nDebugMaskSize >= nNumPotential);
	memset(pDebugMask, 1, nNumPotential);
	int xx = 0;
	while(nNumAdded)
	{
		uprintf("itr %5d, added %5d", xx++, nNumAdded);
		nNumAdded = 0;
		for(int i = 0; i < nNumPotential; ++i)
		{
			if(pDebugMask[i])
			{
				//try and remove..
				pDebugMask[i] = false;
				BspBuild(pBsp, pPlaneOccluders, nNumPlaneOccluders, pBoxOccluders, nNumBoxOccluders, Desc, pDebugMask);
				bool bCulled = BspCullObject(pBsp, pObject);
				if(bCulled)
				{
					//keep removed
					nNumAdded++;
					uprintf("%5d,");
				}
				else
				{
					pDebugMask[i] = true;
				}

			}
		}
		nNumObjects += nNumAdded;
		uprintf("\n");
	}
	return nNumObjects;
}


void BspBuild(SOccluderBsp* pBsp, 
			  SOccluderDesc** pPlaneOccluders, uint32 nNumPlaneOccluders,
			  SOccluderDesc** pBoxOccluders, uint32 nNumBoxOccluders,
			  const SOccluderBspViewDesc& Desc,
			  const bool* pDebugMask)
{
	g_pBsp = pBsp;
	MICROPROFILE_SCOPEIC("BSP", "Build");


	pBsp->Debug.Check = 0;
	pBsp->Debug.BspDebugPlaneCounter = 0;
	

	uplotfnxt("OCCLUDER Occluders:(f3)%d Clipresult:(f4)%d", pBsp->Debug.OccluderDrawOccluders,
			  pBsp->Debug.OccluderDebugDrawClipResult);
	if((int)pBsp->Debug.DebugPlane >= 0)
		uplotfnxt("BSP DEBUG PLANE %d", pBsp->Debug.DebugPlane);

	// uplotfnxt("INSIDEOUTSIDE %x", pBsp->Debug.nInsideOutSide);
	


	long seed = rand();
	srand(42);
	randseed(0xed32babe, 0xdeadf39c);
	pBsp->Debug.DumpFrame = 0;
	pBsp->nDepth = 0;
	pBsp->Nodes.Nodes.Clear();
	pBsp->Nodes.NodesExtra.Clear();
	pBsp->Planes.Clear();
	pBsp->Corners.Clear();
	memset(&pBsp->Stats, 0, sizeof(pBsp->Stats));
	pBsp->Debug.nFirst = 0;
	pBsp->DescOrg = Desc;
	m mtobsp = mcreate(Desc.vDirection, Desc.vRight, Desc.vOrigin);

	const v3 vLocalDirection = mrotate(mtobsp, Desc.vDirection);
	const v3 vLocalOrigin = mtransform(mtobsp, Desc.vOrigin);
	const v3 vLocalRight = mrotate(mtobsp, Desc.vRight);
	const v3 vLocalUp = v3normalize(v3cross(vLocalRight, vLocalDirection));
	const v3 vWorldOrigin = Desc.vOrigin;
	const v3 vWorldDirection = Desc.vDirection;
	pBsp->mtobsp = mtobsp;
	pBsp->mfrombsp = maffineinverse(mtobsp);

	v3 test = v3init(3, 4, 5);
	v3 lo = mtransform(pBsp->mtobsp, test);
	v3 en = mtransform(pBsp->mfrombsp, lo);
	float fDist1 = v3distance(test, en);
	float fDist = v3distance(v3init(0, 0, -1), vLocalDirection);
	if(abs(fDist) > 1e-3f || fDist1 > 1e-3f)
	{
		ZBREAK();
	}

	SBspPotentialOccluders P;
	{
		MICROPROFILE_SCOPEIC("BSP", "Build_AddPotential");
		BspAddFrustum(pBsp, P, vLocalDirection, vLocalRight, vLocalUp, vLocalOrigin);
		BspAddPotentialBoxes(pBsp, P, pBoxOccluders, nNumBoxOccluders);
		BspAddPotentialOccluders(pBsp, P, pPlaneOccluders, nNumPlaneOccluders);
	}


	pBsp->Stats.nNumPlanesConsidered = 0;
	{
		MICROPROFILE_SCOPEIC("BSP", "Build_Estimate");

		for(uint32 i = 0; i < P.PotentialPolys.Size(); ++i)
		{
			uint16 nIndex = P.PotentialPolys[i].nIndex;
			uint16 nCount = P.PotentialPolys[i].nCount;
			pBsp->Stats.nNumPlanesConsidered += nCount;
			P.PotentialPolys[i].fArea = BspAreaEstimate(P.Planes.Ptr() + nIndex, nCount);
		}
	}
	if(P.PotentialPolys.Size())
	{
		MICROPROFILE_SCOPEIC("BSP", "Build_Sort");
		std::sort(&P.PotentialPolys[0], P.PotentialPolys.Size() + &P.PotentialPolys[0],
			[](const SBspPotentialPoly& l, const SBspPotentialPoly& r) -> bool
			{
				return l.fArea > r.fArea;
			}
		);
	}



	{
		MICROPROFILE_SCOPEIC("BSP", "Build_Add");
		v4* vPlanes = P.Planes.Ptr();
		v3* vCorners = P.Corners.Ptr();
		if(pDebugMask)
		{
			for(uint32 i = 0; i < P.PotentialPolys.Size(); ++i)
			{
				if(pDebugMask[i])
				{
					uint16 nIndex = P.PotentialPolys[i].nIndex;
					uint16 nCount = P.PotentialPolys[i].nCount;
					if(BspCanAddNodes(pBsp, &pBsp->Nodes, nCount))
					{
						BspAddOccluderInternal(pBsp, nIndex + vPlanes, nIndex + vCorners, nCount);
					}
				}
			}
		}
		else
		{
			for(uint32 i = 0; i < P.PotentialPolys.Size(); ++i)
			{
				uint16 nIndex = P.PotentialPolys[i].nIndex;
				uint16 nCount = P.PotentialPolys[i].nCount;
				if(BspCanAddNodes(pBsp, &pBsp->Nodes, nCount))
				{
					BspAddOccluderInternal(pBsp, nIndex + vPlanes, nIndex + vCorners, nCount);
				}
			}		
		}
	}
	pBsp->Stats.nNumNodes = pBsp->Nodes.Nodes.Size();
	ZASSERT(pBsp->Nodes.Nodes.Size() == pBsp->Nodes.NodesExtra.Size());
	pBsp->Stats.nNumPlanes = pBsp->Planes.Size();




	pBsp->Debug.Check = 1;
	if(pBsp->Debug.DumpFrame)
		BspDump(pBsp, 0, 0);

	uplotfnxt("BSP: NODES[%03d] PLANES[%03d] DEPTH[%03d]", pBsp->Nodes.Nodes.Size(), pBsp->Planes.Size(),
			  pBsp->nDepth);
	srand(seed);
}

void BSPDUMP()
{
	BspDump(g_pBsp, 0, 0);
}
float BspAreaEstimate(v4* vPlanes, uint32 nNumPlanes)
{
	float fSum = 0;
	for(uint32 i = 0; i < nNumPlanes-1; ++i)
	{
		v4 p0 = vPlanes[i];
		v4 p1 = vPlanes[(i+1)%(nNumPlanes-1)];
		v4 p2 = vPlanes[(i+2)%(nNumPlanes-1)];
		fSum += BspPlaneTestNew(p0,p1,p2);
	}
	return fabs(fSum);
}
float BspAreaEstimate(SOccluderBsp* pBsp, uint16* pIndices, uint32 nNumPlanes)
{
	if(0 == pIndices || nNumPlanes < 3)
		return 0;
	float fSum = 0;
	for(uint32 i = 0; i < nNumPlanes-1; ++i)
	{
		v4 p0 = BspGetPlane(pBsp, pIndices[i]);
		v4 p1 = BspGetPlane(pBsp, pIndices[(i+1)%(nNumPlanes-1)]);
		v4 p2 = BspGetPlane(pBsp, pIndices[(i+2)%(nNumPlanes-1)]);
		fSum += BspPlaneTestNew(p0,p1,p2);
	}
	return fabs(fSum);
}

float BspPolygonArea(SOccluderBsp* pBsp, uint16 * pIndices, uint32 nNumPlanes)
{
	if(0 == pIndices || nNumPlanes < 3)
		return 0.f;
	uint32 nNumCorners = nNumPlanes - 1;
	v3* vCorners = (v3*)alloca(nNumCorners*sizeof(v3));
	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumCorners]);
	for(int i = 0; i < nNumCorners; ++i)
	{
		v4 p0 = BspGetPlane(pBsp, pIndices[i]);
		v4 p1 = BspGetPlane(pBsp, pIndices[(i+1)%nNumCorners]);
		vCorners[i] = BspPlaneIntersection(p0, p1, vNormalPlane);
	}

	v3 vBase = vCorners[0];
	float fSum = 0.f;
	for(int i = 1; i < nNumCorners-1; ++i)
	{
		v3 p0 = vCorners[i];
		v3 p1 = vCorners[i+1];
		v3 d0 = p0 - vBase;
		v3 d1 = p1 - vBase;
		fSum += v3length(v3cross(d0,d1)) * 0.5f;
	} 
	return fSum;
}

void BspAddOccluderInternal(SOccluderBsp *pBsp, v4 *pPlanes, v3* pCorners, uint32 nNumPlanes)
{
	//todo.. remove this to the potential phase
	v4 vNormalPlane = pPlanes[nNumPlanes-1];
	float fLen2 = vNormalPlane.x * vNormalPlane.x + vNormalPlane.y * vNormalPlane.y;
//	uplotfnxt("NORMAL W %f", vNormalPlane.w);
	// uplotfnxt("ADD WITH NORMAL %f %f %f %f .. 2dlen %f", 
	// 	vNormalPlane.x,
	// 	vNormalPlane.y,
	// 	vNormalPlane.z,
	// 	vNormalPlane.w, fLen2);
	#define LIM 0.9999
	float fMinTest = 1.f;

	for(int i = 0; i < nNumPlanes-1; ++i)
	{
		v4 p0 = pPlanes[i];
		v4 p1 = pPlanes[(i+1)%(nNumPlanes-1)];
		v4 p2 = pPlanes[(i+2)%(nNumPlanes-1)];
		float fTest = fabs(BspPlaneTestNew(p0, p1, p2));
		//uplotfnxt("T %f", fTest);
		fMinTest = Min(fTest, fMinTest);
	}
	if(fMinTest < 0.0001f)
	{
		// uplotfnxt("REJECT %f", fMinTest);
		return;
	}

	//if(fLen2 > (LIM*LIM))
	//reject if too close to origin.
	if(fabs(vNormalPlane.w) < 0.1f)
	{
		// uplotfnxt("REJECT %f", vNormalPlane.w);
		return;
	}
	uint32 nPlaneIndex = pBsp->Planes.Size();
	pBsp->Planes.PushBack(pPlanes, nNumPlanes);
	pBsp->Corners.PushBack(pCorners, nNumPlanes);
	uint32 nNodesSize = pBsp->Nodes.Nodes.Size();
	uint16 *nPlaneIndices = (uint16 *)alloca(sizeof(uint16) * nNumPlanes);
	ZASSERT(nNumPlanes > 3);
	v4 p0 = pPlanes[0];
	v4 p1 = pPlanes[1];
	v4 p2 = pPlanes[2];
	float fTest = BspPlaneTestNew(p0, p1, p2);

	if(fTest < 0.f)
	{
		for(uint32 i = 0; i < nNumPlanes - 1; ++i)
			nPlaneIndices[i] = nPlaneIndex + nNumPlanes - 2 - i;
		nPlaneIndices[nNumPlanes - 1] = nPlaneIndex + nNumPlanes - 1;
	}
	else
	{
		for(uint32 i = 0; i < nNumPlanes; ++i)
			nPlaneIndices[i] = nPlaneIndex + i;
	}

	uint32 nNumNodes = (uint32)pBsp->Nodes.Nodes.Size();
	if(pBsp->Nodes.Nodes.Empty())
	{
		uint16 nIndex = (uint16)BspAddInternal(pBsp, -1, true, nPlaneIndices, nNumPlanes, 0, 0);
		ZASSERT(nIndex == 0);
	}
	else
	{
		//MICROPROFILE_SCOPEIC("BSP", "Build_AddRecursive");
		BspAddRecursive(pBsp, 0, nPlaneIndices, nNumPlanes, 0, 0);
	}
	if(pBsp->Nodes.Nodes.Size() == nNodesSize)
	{
		pBsp->Planes.PopBackN(5);
		pBsp->Corners.PopBackN(5);
	}
}

void BspDrawPoly(SOccluderBsp *pBsp, uint16 *Poly, uint32 nVertices, uint32 nColorPoly,
				 uint32 nColorEdges, uint32 nBoxes = 0)
{
	if(nVertices < 3) return;
	uint32 nPolyEdges = nVertices - 1;
	v3 *vCorners = (v3 *)alloca(sizeof(v3) * nPolyEdges);
	v4 vNormalPlane = BspGetPlane(pBsp, Poly[nPolyEdges]);
	for(uint32 i = 0; i < nPolyEdges; ++i)
	{
		uint32 i0 = i;
		uint32 i1 = (i + 1) % nPolyEdges;
		v4 v0 = BspGetPlane(pBsp, Poly[i0]);
		v4 v1 = BspGetPlane(pBsp, Poly[i1]);
		// float fDot = v4dot(v0,v1);
		// uprintf("dp is %f", fDot);
		vCorners[i] = mtransform(pBsp->mfrombsp, BspPlaneIntersection(v0, v1, vNormalPlane));
		if(1)
		{
			//ZDEBUG_DRAWBOX(mid(), vCorners[i], v3rep(0.01f), 0xffff0000, 1);
		}
	}
	if(nPolyEdges)
	{
		v3 foo = v3init(0.01f, 0.01f, 0.01f);
		for(uint32 i = 0; i < nPolyEdges; ++i)
		{
			// {
			// 	ZDEBUG_DRAWLINE(vCorners[i], vCorners[(i + 1) % nPolyEdges], nColorEdges, true);
			// }
		}
		ZDEBUG_DRAWPOLY(&vCorners[0], nPolyEdges, nColorPoly);
	}
}

// only used for debug, so its okay to be slow
int BspFindParent(SOccluderBspNodes* ppNodes, int nChildIndex)
{
	SOccluderBspNode *pNodes = ppNodes->Nodes.Ptr();
	uint32_t nNumNodes = ppNodes->Nodes.Size();
	for(uint32 i = 0; i < nNumNodes; ++i)
	{
		if(pNodes[i].nInside == nChildIndex || pNodes[i].nOutside == nChildIndex)
			return i;
	}
	return -1;
}

int BspFindParent(SOccluderBspNodes* ppNodes, int nChildIndex, bool &bInside)
{
	SOccluderBspNode *pNodes = ppNodes->Nodes.Ptr();
	uint32_t nNumNodes = ppNodes->Nodes.Size();

	for(uint32 i = 0; i < nNumNodes; ++i)
	{
		if(pNodes[i].nInside == nChildIndex)
		{
			bInside = true;
			return i;
		}

		if(pNodes[i].nOutside == nChildIndex)
		{
			bInside = false;
			return i;
		}
	}
	return -1;
}

#define MAX_POLY_PLANES 32

v3 BspToPlane2(v4 p)
{
	return v3init(v2normalize(v2init(p.x, p.y)), p.w + p.z);
}

float BspKryds2d(v3 p0, v3 p1)
{
	return p0.x * p1.y - p1.x * p0.y;
}

float BspPlaneTestNew(v4 plane0, v4 plane1, v4 ptest)
{
	v3 xp = v3cross(plane0.tov3(), plane1.tov3());
	float fDot = v3dot(ptest.tov3(), xp);
	return fDot;
}

int BspAddInternal(SOccluderBsp *pBsp, uint16 nParent, bool bOutsideParent, uint16 *pIndices,
				   uint16 nNumPlanes, uint32 nExcludeMask, uint32 nLevel)
{
	if(!BspCanAddNodes(pBsp, &pBsp->Nodes, nNumPlanes))
		return -1;
	//MICROPROFILE_SCOPEIC("BSP", "Build_AddInternal");
	ZASSERT(nNumPlanes > 1);
	int nCount = 0;
	uint32 nBaseExcludeMask = nExcludeMask;
	int nNewIndex = (int)pBsp->Nodes.Nodes.Size();
	int nPrev = -1;
	if(nLevel + nNumPlanes > pBsp->nDepth)
		pBsp->nDepth = nLevel + nNumPlanes;

	for(uint32 i = 0; i < nNumPlanes; ++i)
	{
		if(0 == USE_EXCLUDE_MASK || 0 == (nExcludeMask & 1))
		{
			nCount++;
			int nIndex = (int)pBsp->Nodes.Nodes.Size();
			SOccluderBspNode *pNode = pBsp->Nodes.Nodes.PushBack();
			SOccluderBspNodeExtra *pNodeExtra = pBsp->Nodes.NodesExtra.PushBack();
			pNode->nOutside = OCCLUDER_EMPTY;
			pNode->nInside = OCCLUDER_LEAF;
			pNode->nPlaneIndex = pIndices[i];
			pNodeExtra->bLeaf = i == nNumPlanes - 1;
			pNodeExtra->bSpecial = false;
			if(nPrev >= 0)
				pBsp->Nodes.Nodes[nPrev].nInside = nIndex;
			nPrev = nIndex;
			pNodeExtra->vDebugCorner = pBsp->Corners[pIndices[i]&(~0x8000)];

			if(0)
			{
				v4 vPlane = BspGetPlane(pBsp, pIndices[i]);
				v3 vCorner = pNodeExtra->vDebugCorner;
				vCorner = mtransform(pBsp->mfrombsp, vCorner);
				vPlane = v4init(mrotate(pBsp->mfrombsp, vPlane.tov3()), 1.f);
				ZDEBUG_DRAWPLANE(vPlane.tov3(), vCorner, 0xff00ff00);
			}
		}
		else
		{
			//uplotfnxt("EXCLUDE %x", pIndices[i]&(~0x8000));
		}
		nExcludeMask >>= 1;
	}

	if(nParent < pBsp->Nodes.Nodes.Size())
	{
		if(bOutsideParent)
			pBsp->Nodes.Nodes[nParent].nOutside = nNewIndex;
		else
			pBsp->Nodes.Nodes[nParent].nInside = nNewIndex;
	}
	if(pBsp->Debug.OccluderDrawOccluders&1)
		BspDrawPoly(pBsp, pIndices, nNumPlanes, randcolor(), randcolor(), 1);
	return nNewIndex;
}
#define OCCLUDER_POLY_MAX 12

bool PlaneToleranceTest(v4 vPlane, v4 vClipPlane, float fPlaneTolerance)
{
	float f3dot = fabs(v3dot(vPlane.tov3(), vClipPlane.tov3()));
	float f3dot1 = fabs(v3dot((-vPlane).tov3(), vClipPlane.tov3()));
	float fMinDist = fabs(v4length(vPlane - vClipPlane));
	float fMinDist1 = fabs(v4length(-vPlane - vClipPlane));
	if(Min<float>(fMinDist, fMinDist1) < fPlaneTolerance || Max<float>(f3dot, f3dot1) > 0.9)
	{
		// uplotfnxt("MIN %f %f .. MAX %f %f", fMinDist, fMinDist1, f3dot, f3dot1);
	}

	if(Min<float>(fMinDist, fMinDist1) < fPlaneTolerance && Max<float>(f3dot, f3dot1) > 0.9)
	{
		// uprintf("FAIL %f %f  v3d %f", fMinDist, fMinDist1, f3dot);
		return true;
	}
	else
	{
		return false;
	}
}

void plotlist(const char* pPrefix, uint16* pIndices, uint32 nNumIndices)
{
	char buffer[300];
	strcpy(buffer, pPrefix);
	for(int i = 0; i < nNumIndices; ++i)
	{
		char buf[32];
		snprintf(buf, 31, "%d, ", pIndices[i]);
		strcat(buffer, buf);
	}
	uplotfnxt(buffer);
}


void BspDumpPlanes(const char* prefix, SOccluderBsp *pBsp, uint16 *pIndices, uint16 nNumPlanes)
{
	if(!nNumPlanes)
		return;
	for(int i = 0; i < (nNumPlanes-1); ++i)
	{
		v4 p = BspGetPlane(pBsp, pIndices[i]);
		v4 p1 = BspGetPlane(pBsp, pIndices[(i+1)%(nNumPlanes-1)]);
		v4 t = v4init(0, 0, 1, 0);
		float fTest = BspPlaneTestNew(p, p1, t);
		uprintf("%s %5.2f, %5.2f, %5.2f, %5.2f .. %5.2f\n", prefix, p.x, p.y, p.z, p.w, fTest);
	}
}



// #define BspClipPoly BspClipPolySafe



uint32 g_nPolyExtraDump = 0;


ClipPolyResult BspClipPoly(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
						   uint16 nNumPlanes, uint32 nExcludeMask, uint16 *pPolyIn,
						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_,
						   uint32 &nMaskIn_, uint32 &nMaskOut_)
{
	//MICROPROFILE_SCOPEIC("BSP", "ClipPoly");

	ZASSERT(nNumPlanes < 32);
	uint32 nNumEdges = nNumPlanes - 1;
	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumEdges]);

	ZASSERT(nNumEdges > 2);
	v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);



	bool *bCorners = (bool *)alloca(nNumPlanes<<1);
	bool *bCorners0 = bCorners+nNumPlanes;//(bool *)alloca(nNumPlanes);
	//v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);

	v4 vLastPlane = BspGetPlane(pBsp, pIndices[ 0 ]);
	v4 vFirstPlane = vLastPlane;
	//bool bDebug = 0 == pBsp->Debug.nFirst++;
	for(int i = 1; i < nNumEdges; ++i)
	{
		v4 p1 = BspGetPlane(pBsp, pIndices[i]);
		//v4 p1 = vPlanes[(i + 1) % nNumEdges];
		float fTest = BspPlaneTestNew(vLastPlane, p1, vClipPlane);
		vLastPlane = p1;
		bCorners[i] = fTest < PLANE_TEST_EPSILON;
		bCorners0[i] = fTest < -PLANE_TEST_EPSILON;
	}

	float fTest = BspPlaneTestNew(vLastPlane, vFirstPlane, vClipPlane);
	bCorners[0] = bCorners[nNumEdges] = fTest < PLANE_TEST_EPSILON;
	bCorners0[0] = bCorners0[nNumEdges] = fTest < -PLANE_TEST_EPSILON;
	// bCorners[nNumEdges] = bCorners[0];
	// bCorners0[nNumEdges] = bCorners0[0];

	bCorners++;
	bCorners0++;


	uint32 nEdgesIn = 0;
	uint32 nEdgesOut = 0;
	uint32 nMaskIn = 0;
	uint32 nMaskOut = 0;
	uint32 nMaskRollIn = 1;
	uint32 nMaskRollOut = 1;
	uint32 nMask = nExcludeMask;
	int nClipAddedIn = 0;
	int nClipAddedOut = 0;

	// TRUE --> OUTSIDE

	for(int i = 0; i < nNumEdges; ++i)
	{
		bool bCorner0 = bCorners0[(i - 1)];

		bool bCorner1 = bCorners0[i];
		if(bCorner0 != bCorner1)
		{
			// add to both
			if(!nClipAddedIn)
			{
				nClipAddedIn = true;
				if(bCorner0) // outside --> inside
				{
					pPolyOut[nEdgesOut++] = pIndices[i];
					nMaskOut |= nMaskRollOut & nMask;
					nMaskRollOut <<= 1;

					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
					nMaskOut |= nMaskRollOut;
					nMaskRollOut <<= 1;
					nMask <<= 1;
					if(g_nPolyExtraDump)
					{
						uprintf("Add extra out planexx %d %04x\n",  nEdgesOut-1, nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT);
					}
				}
				else // inside --> outside
				{
					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
					nMaskOut |= nMaskRollOut;
					nMaskRollOut <<= 1;
					nMask <<= 1;

					if(g_nPolyExtraDump)
					{
						uprintf("Add extra out plane %d %04x\n",  nEdgesOut-1, nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT);
					}


					pPolyOut[nEdgesOut++] = pIndices[i];
					nMaskOut |= nMaskRollOut & nMask;
					//if(nMask & 1)
					//	nMaskOut |= nMaskRollOut;
					nMaskRollOut <<= 1;
				}
			}
			else
			{
				pPolyOut[nEdgesOut++] = pIndices[i];
				nMaskOut |= nMaskRollOut & nMask;
				//if(nMask & 1)
				//	nMaskOut |= nMaskRollOut;
				nMaskRollOut <<= 1;
			}
		}
		else if(bCorner0) // both outside
		{
			pPolyOut[nEdgesOut++] = pIndices[i];
			nMaskOut |= nMaskRollOut & nMask;
			nMaskRollOut <<= 1;
		}
		else // both insides
		{
			nMask >>= 1;
		}
		//nMask >>= 1;
	}

	nMask = nExcludeMask;
	for(int i = 0; i < nNumEdges; ++i)
	{
		bool bCorner0 = bCorners[(i - 1)];
		bool bCorner1 = bCorners[i];
		if(bCorner0 != bCorner1)
		{
			// add to both
			if(!nClipAddedOut)
			{
				nClipAddedOut = 1;
				if(bCorner0) // outside --> inside
				{
					pPolyIn[nEdgesIn++] = nClipPlane ;
					nMaskIn |= nMaskRollIn;
					nMaskRollIn <<= 1;
					nMask <<= 1;

					if(g_nPolyExtraDump)
					{
						uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
					}


					pPolyIn[nEdgesIn++] = pIndices[i];
					nMaskIn |= nMaskRollIn & nMask;
					nMaskRollIn <<= 1;
				}
				else // inside --> outside
				{
					pPolyIn[nEdgesIn++] = pIndices[i] ;
					nMaskIn |= nMaskRollIn & nMask;
					nMaskRollIn <<= 1;

					pPolyIn[nEdgesIn++] = nClipPlane;
					nMaskIn |= nMaskRollIn;
					nMaskRollIn <<= 1;
					nMask <<= 1;

					if(g_nPolyExtraDump)
					{
						uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
					}

				}
			}
			else
			{
				pPolyIn[nEdgesIn++] = pIndices[i];
				nMaskIn |= nMaskRollIn & nMask;
				nMaskRollIn <<= 1;
			}
		}
		else if(bCorner0) // both outside
		{
			ZASSERT(bCorner1);
			nMask >>= 1;
		}
		else // both insides
		{
			ZASSERT(!bCorner1);
			ZASSERT(!bCorner0);
			pPolyIn[nEdgesIn++] = pIndices[i];
			nMaskIn |= nMaskRollIn & nMask;
			nMaskRollIn <<= 1;
		}
	}
	ZASSERT(nEdgesIn == 0 || nEdgesIn > 2);
	ZASSERT(nEdgesOut == 0 || nEdgesOut > 2);

	// add normal plane
	if(nEdgesIn)
		pPolyIn[nEdgesIn++] = pIndices[nNumEdges];
	if(nEdgesOut)
		pPolyOut[nEdgesOut++] = pIndices[nNumEdges];

	nEdgesIn_ = nEdgesIn;
	nEdgesOut_ = nEdgesOut;
	nMaskIn_ = nMaskIn;
	nMaskOut_ = nMaskOut;

	if(g_nPolyExtraDump)
	{
		uprintf("DumpIN\n");
		BspDumpPlanes("IN ",pBsp, pPolyIn, nEdgesIn);
		uprintf("DumpOUT\n");
		BspDumpPlanes("OUT", pBsp, pPolyOut, nEdgesOut);


	}
	return ClipPolyResult((nEdgesIn ? 0x1 : 0) | (nEdgesOut ? 0x2 : 0));
}

// ClipPolyResult BspClipPolySafe2(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
// 						   uint16 nNumPlanes, uint32 nExcludeMask, uint16 *pPolyIn,
// 						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_,
// 						   uint32 &nMaskIn_, uint32 &nMaskOut_)
// {
// 	//MICROPROFILE_SCOPEIC("BSP", "ClipPoly");

// 	ZASSERT(nNumPlanes < 32);
// 	uint32 nNumEdges = nNumPlanes - 1;
// 	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumEdges]);

// 	ZASSERT(nNumEdges > 2);
// 	v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);



// 	bool *bCorners = (bool *)alloca(nNumPlanes<<1);
// 	bool *bCorners0 = bCorners+nNumPlanes;//(bool *)alloca(nNumPlanes);
// 	//v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);

// 	v4 vLastPlane = BspGetPlane(pBsp, pIndices[ 0 ]);
// 	v4 vFirstPlane = vLastPlane;
// 	//bool bDebug = 0 == pBsp->Debug.nFirst++;
// 	for(int i = 1; i < nNumEdges; ++i)
// 	{
// 		v4 p1 = BspGetPlane(pBsp, pIndices[i]);
// 		//v4 p1 = vPlanes[(i + 1) % nNumEdges];
// 		float fTest = BspPlaneTestNew(vLastPlane, p1, vClipPlane);
// 		vLastPlane = p1;
// 		bCorners[i] = fTest < 0.f;
// 		bCorners0[i] = fTest <  0.f;
// 	}

// 	float fTest = BspPlaneTestNew(vLastPlane, vFirstPlane, vClipPlane);
// 	bCorners[0] = bCorners[nNumEdges] = fTest < 0.f;
// 	bCorners0[0] = bCorners0[nNumEdges] = fTest <  0.f;
// 	// bCorners[nNumEdges] = bCorners[0];
// 	// bCorners0[nNumEdges] = bCorners0[0];

// 	bCorners++;
// 	bCorners0++;


// 	uint32 nEdgesIn = 0;
// 	uint32 nEdgesOut = 0;
// 	uint32 nMaskIn = 0;
// 	uint32 nMaskOut = 0;
// 	uint32 nMaskRollIn = 1;
// 	uint32 nMaskRollOut = 1;
// 	uint32 nMask = nExcludeMask;
// 	int nClipAddedIn = 0;
// 	int nClipAddedOut = 0;

// 	// TRUE --> OUTSIDE

// 	for(int i = 0; i < nNumEdges; ++i)
// 	{
// 		bool bCorner0 = bCorners0[(i - 1)];

// 		bool bCorner1 = bCorners0[i];
// 		if(bCorner0 != bCorner1)
// 		{
// 			// add to both
// 			if(!nClipAddedIn)
// 			{
// 				nClipAddedIn = true;
// 				if(bCorner0) // outside --> inside
// 				{
// 					pPolyOut[nEdgesOut++] = pIndices[i];
// 					nMaskOut |= nMaskRollOut & nMask;
// 					nMaskRollOut <<= 1;

// 					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
// 					nMaskOut |= nMaskRollOut;
// 					nMaskRollOut <<= 1;
// 					nMask <<= 1;
// 					if(g_nPolyExtraDump)
// 					{
// 						uprintf("Add extra out planexx %d %04x\n",  nEdgesOut-1, nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT);
// 					}
// 				}
// 				else // inside --> outside
// 				{
// 					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
// 					nMaskOut |= nMaskRollOut;
// 					nMaskRollOut <<= 1;
// 					nMask <<= 1;

// 					if(g_nPolyExtraDump)
// 					{
// 						uprintf("Add extra out plane %d %04x\n",  nEdgesOut-1, nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT);
// 					}


// 					pPolyOut[nEdgesOut++] = pIndices[i];
// 					nMaskOut |= nMaskRollOut & nMask;
// 					//if(nMask & 1)
// 					//	nMaskOut |= nMaskRollOut;
// 					nMaskRollOut <<= 1;
// 				}
// 			}
// 			else
// 			{
// 				pPolyOut[nEdgesOut++] = pIndices[i];
// 				nMaskOut |= nMaskRollOut & nMask;
// 				//if(nMask & 1)
// 				//	nMaskOut |= nMaskRollOut;
// 				nMaskRollOut <<= 1;
// 			}
// 		}
// 		else if(bCorner0) // both outside
// 		{
// 			pPolyOut[nEdgesOut++] = pIndices[i];
// 			nMaskOut |= nMaskRollOut & nMask;
// 			nMaskRollOut <<= 1;
// 		}
// 		else // both insides
// 		{
// 			nMask >>= 1;
// 		}
// 		//nMask >>= 1;
// 	}

// 	nMask = nExcludeMask;
// 	for(int i = 0; i < nNumEdges; ++i)
// 	{
// 		bool bCorner0 = bCorners[(i - 1)];
// 		bool bCorner1 = bCorners[i];
// 		if(bCorner0 != bCorner1)
// 		{
// 			// add to both
// 			if(!nClipAddedOut)
// 			{
// 				nClipAddedOut = 1;
// 				if(bCorner0) // outside --> inside
// 				{
// 					pPolyIn[nEdgesIn++] = nClipPlane ;
// 					nMaskIn |= nMaskRollIn;
// 					nMaskRollIn <<= 1;
// 					nMask <<= 1;

// 					if(g_nPolyExtraDump)
// 					{
// 						uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 					}


// 					pPolyIn[nEdgesIn++] = pIndices[i];
// 					nMaskIn |= nMaskRollIn & nMask;
// 					nMaskRollIn <<= 1;
// 				}
// 				else // inside --> outside
// 				{
// 					pPolyIn[nEdgesIn++] = pIndices[i] ;
// 					nMaskIn |= nMaskRollIn & nMask;
// 					nMaskRollIn <<= 1;

// 					pPolyIn[nEdgesIn++] = nClipPlane;
// 					nMaskIn |= nMaskRollIn;
// 					nMaskRollIn <<= 1;
// 					nMask <<= 1;

// 					if(g_nPolyExtraDump)
// 					{
// 						uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 					}

// 				}
// 			}
// 			else
// 			{
// 				pPolyIn[nEdgesIn++] = pIndices[i];
// 				nMaskIn |= nMaskRollIn & nMask;
// 				nMaskRollIn <<= 1;
// 			}
// 		}
// 		else if(bCorner0) // both outside
// 		{
// 			ZASSERT(bCorner1);
// 			nMask >>= 1;
// 		}
// 		else // both insides
// 		{
// 			ZASSERT(!bCorner1);
// 			ZASSERT(!bCorner0);
// 			pPolyIn[nEdgesIn++] = pIndices[i];
// 			nMaskIn |= nMaskRollIn & nMask;
// 			nMaskRollIn <<= 1;
// 		}
// 	}
// 	ZASSERT(nEdgesIn == 0 || nEdgesIn > 2);
// 	ZASSERT(nEdgesOut == 0 || nEdgesOut > 2);

// 	// add normal plane
// 	if(nEdgesIn)
// 		pPolyIn[nEdgesIn++] = pIndices[nNumEdges];
// 	if(nEdgesOut)
// 		pPolyOut[nEdgesOut++] = pIndices[nNumEdges];

// 	nEdgesIn_ = nEdgesIn;
// 	nEdgesOut_ = nEdgesOut;
// 	nMaskIn_ = nMaskIn;
// 	nMaskOut_ = nMaskOut;

// 	if(g_nPolyExtraDump)
// 	{
// 		uprintf("DumpIN\n");
// 		BspDumpPlanes("IN ",pBsp, pPolyIn, nEdgesIn);
// 		uprintf("DumpOUT\n");
// 		BspDumpPlanes("OUT", pBsp, pPolyOut, nEdgesOut);


// 	}
// 	return ClipPolyResult((nEdgesIn ? 0x1 : 0) | (nEdgesOut ? 0x2 : 0));
// }

// ClipPolyResult BspClipPolySingleTest(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
// 						   uint16 nNumPlanes, uint32 nExcludeMask, uint16 *pPolyIn,
// 						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_,
// 						   uint32 &nMaskIn_, uint32 &nMaskOut_)
// {
// 	//MICROPROFILE_SCOPEIC("BSP", "ClipPoly");

// 	ZASSERT(nNumPlanes < 32);
// 	uint32 nNumEdges = nNumPlanes - 1;
// 	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumEdges]);

// 	ZASSERT(nNumEdges > 2);
// 	v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);



// 	bool *bCorners = (bool *)alloca(nNumPlanes+1);
// 	//bool *bCorners0 = bCorners+nNumPlanes;//(bool *)alloca(nNumPlanes);
// 	//v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);

// 	v4 vLastPlane = BspGetPlane(pBsp, pIndices[ 0 ]);
// 	v4 vFirstPlane = vLastPlane;
// 	//bool bDebug = 0 == pBsp->Debug.nFirst++;
// 	for(int i = 1; i < nNumEdges; ++i)
// 	{
// 		v4 p1 = BspGetPlane(pBsp, pIndices[i]);
// 		//v4 p1 = vPlanes[(i + 1) % nNumEdges];
// 		float fTest = BspPlaneTestNew(vLastPlane, p1, vClipPlane);
// 		vLastPlane = p1;
// 		bCorners[i] = fTest < 0.f;
// //		bCorners0[i] = fTest <  0.f;
// 	}

// 	float fTest = BspPlaneTestNew(vLastPlane, vFirstPlane, vClipPlane);
// 	bCorners[0] = bCorners[nNumEdges] = fTest < 0.f;
// 	//bCorners0[0] = bCorners0[nNumEdges] = fTest <  0.f;
// 	// bCorners[nNumEdges] = bCorners[0];
// 	// bCorners0[nNumEdges] = bCorners0[0];

// 	bCorners++;
// 	//bCorners0++;


// 	uint32 nEdgesIn = 0;
// 	uint32 nEdgesOut = 0;
// 	uint32 nMaskIn = 0;
// 	uint32 nMaskOut = 0;
// 	uint32 nMaskRollIn = 1;
// 	uint32 nMaskRollOut = 1;
// 	uint32 nMaskWorkIn = nExcludeMask, nMaskWorkOut = nExcludeMask;
// 	int nClipAddedIn = 0;
// 	int nClipAddedOut = 0;

// 	// TRUE --> OUTSIDE

// 	for(int i = 0; i < nNumEdges; ++i)
// 	{
// 		bool bCorner0 = bCorners[(i - 1)];
// 		bool bCorner1 = bCorners[i];
// 		if(bCorner0 != bCorner1)
// 		{
// 			// add to both
// 			if(!nClipAddedIn)
// 			{
// 				nClipAddedIn = true;
// 				if(bCorner0) // outside --> inside
// 				{
// 					pPolyOut[nEdgesOut++] = pIndices[i];
// 					nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 					nMaskRollOut <<= 1;

// 					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
// 					nMaskOut |= nMaskRollOut;
// 					nMaskRollOut <<= 1;
// 					nMaskWorkOut <<= 1;


// 					pPolyIn[nEdgesIn++] = nClipPlane ;
// 					nMaskIn |= nMaskRollIn;
// 					nMaskRollIn <<= 1;
// 					nMaskWorkIn <<= 1;

// 					if(g_nPolyExtraDump)
// 					{
// 						uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 					}


// 					pPolyIn[nEdgesIn++] = pIndices[i];
// 					nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 					nMaskRollIn <<= 1;


// 				}
// 				else // inside --> outside
// 				{
// 					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
// 					nMaskOut |= nMaskRollOut;
// 					nMaskRollOut <<= 1;
// 					nMaskWorkOut <<= 1;



// 					pPolyOut[nEdgesOut++] = pIndices[i];
// 					nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 					nMaskRollOut <<= 1;


// 					pPolyIn[nEdgesIn++] = pIndices[i] ;
// 					nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 					nMaskRollIn <<= 1;

// 					pPolyIn[nEdgesIn++] = nClipPlane;
// 					nMaskIn |= nMaskRollIn;
// 					nMaskRollIn <<= 1;
// 					nMaskWorkIn <<= 1;

// 				}
// 			}
// 			else
// 			{
// 				pPolyOut[nEdgesOut++] = pIndices[i];
// 				nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 				nMaskRollOut <<= 1;

// 				pPolyIn[nEdgesIn++] = pIndices[i];
// 				nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 				nMaskRollIn <<= 1;




// 			}
// 		}
// 		else if(bCorner0) // both outside
// 		{
// 			pPolyOut[nEdgesOut++] = pIndices[i];
// 			nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 			nMaskRollOut <<= 1;

// 			ZASSERT(bCorner1);
// 			nMaskWorkIn >>= 1;


// 		}
// 		else // both insides
// 		{
// 			nMaskWorkOut >>= 1;

// 			ZASSERT(!bCorner1);
// 			ZASSERT(!bCorner0);
// 			pPolyIn[nEdgesIn++] = pIndices[i];
// 			nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 			nMaskRollIn <<= 1;


// 		}
// 	}

// 	// for(int i = 0; i < nNumEdges; ++i)
// 	// {
// 	// 	bool bCorner0 = bCorners[(i - 1)];
// 	// 	bool bCorner1 = bCorners[i];
// 	// 	if(bCorner0 != bCorner1)
// 	// 	{
// 	// 		// add to both
// 	// 		if(!nClipAddedOut)
// 	// 		{
// 	// 			nClipAddedOut = 1;
// 	// 			if(bCorner0) // outside --> inside
// 	// 			{
// 	// 				pPolyIn[nEdgesIn++] = nClipPlane ;
// 	// 				nMaskIn |= nMaskRollIn;
// 	// 				nMaskRollIn <<= 1;
// 	// 				nMaskWorkIn <<= 1;

// 	// 				if(g_nPolyExtraDump)
// 	// 				{
// 	// 					uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 	// 				}


// 	// 				pPolyIn[nEdgesIn++] = pIndices[i];
// 	// 				nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 				nMaskRollIn <<= 1;
// 	// 			}
// 	// 			else // inside --> outside
// 	// 			{
// 	// 				pPolyIn[nEdgesIn++] = pIndices[i] ;
// 	// 				nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 				nMaskRollIn <<= 1;

// 	// 				pPolyIn[nEdgesIn++] = nClipPlane;
// 	// 				nMaskIn |= nMaskRollIn;
// 	// 				nMaskRollIn <<= 1;
// 	// 				nMaskWorkIn <<= 1;

// 	// 				if(g_nPolyExtraDump)
// 	// 				{
// 	// 					uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 	// 				}

// 	// 			}
// 	// 		}
// 	// 		else
// 	// 		{
// 	// 			pPolyIn[nEdgesIn++] = pIndices[i];
// 	// 			nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 			nMaskRollIn <<= 1;
// 	// 		}
// 	// 	}
// 	// 	else if(bCorner0) // both outside
// 	// 	{
// 	// 		// ZASSERT(bCorner1);
// 	// 		// nMaskWorkIn >>= 1;
// 	// 	}
// 	// 	else // both insides
// 	// 	{
// 	// 		// ZASSERT(!bCorner1);
// 	// 		// ZASSERT(!bCorner0);
// 	// 		// pPolyIn[nEdgesIn++] = pIndices[i];
// 	// 		// nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 		// nMaskRollIn <<= 1;
// 	// 	}
// 	// }
// 	ZASSERT(nEdgesIn == 0 || nEdgesIn > 2);
// 	ZASSERT(nEdgesOut == 0 || nEdgesOut > 2);

// 	// add normal plane
// 	if(nEdgesIn)
// 		pPolyIn[nEdgesIn++] = pIndices[nNumEdges];
// 	if(nEdgesOut)
// 		pPolyOut[nEdgesOut++] = pIndices[nNumEdges];

// 	nEdgesIn_ = nEdgesIn;
// 	nEdgesOut_ = nEdgesOut;
// 	nMaskIn_ = nMaskIn;
// 	nMaskOut_ = nMaskOut;

// 	if(g_nPolyExtraDump)
// 	{
// 		uprintf("DumpIN\n");
// 		BspDumpPlanes("IN ",pBsp, pPolyIn, nEdgesIn);
// 		uprintf("DumpOUT\n");
// 		BspDumpPlanes("OUT", pBsp, pPolyOut, nEdgesOut);


// 	}
// 	return ClipPolyResult((nEdgesIn ? 0x1 : 0) | (nEdgesOut ? 0x2 : 0));
// }

// ClipPolyResult BspClipPolySingleTest0(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
// 						   uint16 nNumPlanes, uint32 nExcludeMask, uint16 *pPolyIn,
// 						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_,
// 						   uint32 &nMaskIn_, uint32 &nMaskOut_)
// {
// 	//MICROPROFILE_SCOPEIC("BSP", "ClipPoly");

// 	ZASSERT(nNumPlanes < 32);
// 	uint32 nNumEdges = nNumPlanes - 1;
// 	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumEdges]);

// 	ZASSERT(nNumEdges > 2);
// 	v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);



// 	bool *bCorners = (bool *)alloca(nNumPlanes+1);
// 	//bool *bCorners0 = bCorners+nNumPlanes;//(bool *)alloca(nNumPlanes);
// 	//v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);

// 	v4 vLastPlane = BspGetPlane(pBsp, pIndices[ 0 ]);
// 	v4 vFirstPlane = vLastPlane;
// 	//bool bDebug = 0 == pBsp->Debug.nFirst++;
// 	for(int i = 1; i < nNumEdges; ++i)
// 	{
// 		v4 p1 = BspGetPlane(pBsp, pIndices[i]);
// 		//v4 p1 = vPlanes[(i + 1) % nNumEdges];
// 		float fTest = BspPlaneTestNew(vLastPlane, p1, vClipPlane);
// 		vLastPlane = p1;
// 		bCorners[i] = fTest < 0.f;
// //		bCorners0[i] = fTest <  0.f;
// 	}

// 	float fTest = BspPlaneTestNew(vLastPlane, vFirstPlane, vClipPlane);
// 	bCorners[0] = bCorners[nNumEdges] = fTest < 0.f;
// 	//bCorners0[0] = bCorners0[nNumEdges] = fTest <  0.f;
// 	// bCorners[nNumEdges] = bCorners[0];
// 	// bCorners0[nNumEdges] = bCorners0[0];

// 	bCorners++;
// 	//bCorners0++;


// 	uint32 nEdgesIn = 0;
// 	uint32 nEdgesOut = 0;
// 	uint32 nMaskIn = 0;
// 	uint32 nMaskOut = 0;
// 	uint32 nMaskRollIn = 1;
// 	uint32 nMaskRollOut = 1;
// 	uint32 nMaskWorkIn = nExcludeMask, nMaskWorkOut = nExcludeMask;
// 	int nClipAddedIn = 0;
// 	int nClipAddedOut = 0;

// 	// TRUE --> OUTSIDE

// 	for(int i = 0; i < nNumEdges; ++i)
// 	{
// 		bool bCorner0 = bCorners[(i - 1)];
// 		bool bCorner1 = bCorners[i];
// 		if(bCorner0 == bCorner1)
// 		{
// 			if(bCorner0) // both outside
// 			{
// 				pPolyOut[nEdgesOut++] = pIndices[i];
// 				nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 				nMaskRollOut <<= 1;

// 				ZASSERT(bCorner1);
// 				nMaskWorkIn >>= 1;


// 			}
// 			else // both insides
// 			{
// 				nMaskWorkOut >>= 1;
// 				ZASSERT(!bCorner1);
// 				ZASSERT(!bCorner0);
// 				pPolyIn[nEdgesIn++] = pIndices[i];
// 				nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 				nMaskRollIn <<= 1;


// 			}

// 		}
// 		else
// 		{

// 			// add to both
// 			if(!nClipAddedIn)
// 			{
// 				nClipAddedIn = true;
// 				if(bCorner0) // outside --> inside
// 				{
// 					pPolyOut[nEdgesOut++] = pIndices[i];
// 					nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 					nMaskRollOut <<= 1;

// 					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
// 					nMaskOut |= nMaskRollOut;
// 					nMaskRollOut <<= 1;
// 					nMaskWorkOut <<= 1;


// 					pPolyIn[nEdgesIn++] = nClipPlane ;
// 					nMaskIn |= nMaskRollIn;
// 					nMaskRollIn <<= 1;
// 					nMaskWorkIn <<= 1;

// 					if(g_nPolyExtraDump)
// 					{
// 						uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 					}


// 					pPolyIn[nEdgesIn++] = pIndices[i];
// 					nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 					nMaskRollIn <<= 1;


// 				}
// 				else // inside --> outside
// 				{
// 					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
// 					nMaskOut |= nMaskRollOut;
// 					nMaskRollOut <<= 1;
// 					nMaskWorkOut <<= 1;



// 					pPolyOut[nEdgesOut++] = pIndices[i];
// 					nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 					nMaskRollOut <<= 1;


// 					pPolyIn[nEdgesIn++] = pIndices[i] ;
// 					nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 					nMaskRollIn <<= 1;

// 					pPolyIn[nEdgesIn++] = nClipPlane;
// 					nMaskIn |= nMaskRollIn;
// 					nMaskRollIn <<= 1;
// 					nMaskWorkIn <<= 1;

// 				}
// 			}
// 			else
// 			{
// 				pPolyOut[nEdgesOut++] = pIndices[i];
// 				nMaskOut |= nMaskRollOut & nMaskWorkOut;
// 				nMaskRollOut <<= 1;

// 				pPolyIn[nEdgesIn++] = pIndices[i];
// 				nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 				nMaskRollIn <<= 1;




// 			}
// 		}
// 	}

// 	// for(int i = 0; i < nNumEdges; ++i)
// 	// {
// 	// 	bool bCorner0 = bCorners[(i - 1)];
// 	// 	bool bCorner1 = bCorners[i];
// 	// 	if(bCorner0 != bCorner1)
// 	// 	{
// 	// 		// add to both
// 	// 		if(!nClipAddedOut)
// 	// 		{
// 	// 			nClipAddedOut = 1;
// 	// 			if(bCorner0) // outside --> inside
// 	// 			{
// 	// 				pPolyIn[nEdgesIn++] = nClipPlane ;
// 	// 				nMaskIn |= nMaskRollIn;
// 	// 				nMaskRollIn <<= 1;
// 	// 				nMaskWorkIn <<= 1;

// 	// 				if(g_nPolyExtraDump)
// 	// 				{
// 	// 					uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 	// 				}


// 	// 				pPolyIn[nEdgesIn++] = pIndices[i];
// 	// 				nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 				nMaskRollIn <<= 1;
// 	// 			}
// 	// 			else // inside --> outside
// 	// 			{
// 	// 				pPolyIn[nEdgesIn++] = pIndices[i] ;
// 	// 				nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 				nMaskRollIn <<= 1;

// 	// 				pPolyIn[nEdgesIn++] = nClipPlane;
// 	// 				nMaskIn |= nMaskRollIn;
// 	// 				nMaskRollIn <<= 1;
// 	// 				nMaskWorkIn <<= 1;

// 	// 				if(g_nPolyExtraDump)
// 	// 				{
// 	// 					uprintf("Add extra in plane %d %04x\n",  nEdgesIn-1, nClipPlane);
// 	// 				}

// 	// 			}
// 	// 		}
// 	// 		else
// 	// 		{
// 	// 			pPolyIn[nEdgesIn++] = pIndices[i];
// 	// 			nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 			nMaskRollIn <<= 1;
// 	// 		}
// 	// 	}
// 	// 	else if(bCorner0) // both outside
// 	// 	{
// 	// 		// ZASSERT(bCorner1);
// 	// 		// nMaskWorkIn >>= 1;
// 	// 	}
// 	// 	else // both insides
// 	// 	{
// 	// 		// ZASSERT(!bCorner1);
// 	// 		// ZASSERT(!bCorner0);
// 	// 		// pPolyIn[nEdgesIn++] = pIndices[i];
// 	// 		// nMaskIn |= nMaskRollIn & nMaskWorkIn;
// 	// 		// nMaskRollIn <<= 1;
// 	// 	}
// 	// }
// 	ZASSERT(nEdgesIn == 0 || nEdgesIn > 2);
// 	ZASSERT(nEdgesOut == 0 || nEdgesOut > 2);

// 	// add normal plane
// 	if(nEdgesIn)
// 		pPolyIn[nEdgesIn++] = pIndices[nNumEdges];
// 	if(nEdgesOut)
// 		pPolyOut[nEdgesOut++] = pIndices[nNumEdges];

// 	nEdgesIn_ = nEdgesIn;
// 	nEdgesOut_ = nEdgesOut;
// 	nMaskIn_ = nMaskIn;
// 	nMaskOut_ = nMaskOut;

// 	if(g_nPolyExtraDump)
// 	{
// 		uprintf("DumpIN\n");
// 		BspDumpPlanes("IN ",pBsp, pPolyIn, nEdgesIn);
// 		uprintf("DumpOUT\n");
// 		BspDumpPlanes("OUT", pBsp, pPolyOut, nEdgesOut);


// 	}
// 	return ClipPolyResult((nEdgesIn ? 0x1 : 0) | (nEdgesOut ? 0x2 : 0));
// }

ClipPolyResult BspClipPolyCull(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
						   uint16 nNumPlanes, uint16 *pPolyIn,
						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_)
{
	//MICROPROFILE_SCOPEIC("BSP", "ClipPoly");

	ZASSERT(nNumPlanes < 32);
	uint32 nNumEdges = nNumPlanes - 1;
	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumEdges]);

	ZASSERT(nNumEdges > 2);
	v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);
	ZASSERT(vClipPlane.w == 0.f);



	bool *bCorners = (bool *)alloca(nNumPlanes+1);
	//bool *bCorners0 = bCorners+nNumPlanes;//(bool *)alloca(nNumPlanes);
	//v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);

	v4 vLastPlane = BspGetPlane(pBsp, pIndices[ 0 ]);
	v4 vFirstPlane = vLastPlane;
	//bool bDebug = 0 == pBsp->Debug.nFirst++;
	for(int i = 1; i < nNumEdges; ++i)
	{
		v4 p1 = BspGetPlane(pBsp, pIndices[i]);
		//v4 p1 = vPlanes[(i + 1) % nNumEdges];
		float fTest = BspPlaneTestNew(vLastPlane, p1, vClipPlane);
		vLastPlane = p1;
		bCorners[i] = fTest < 0.f;
//		bCorners0[i] = fTest <  0.f;
	}

	float fTest = BspPlaneTestNew(vLastPlane, vFirstPlane, vClipPlane);
	bCorners[0] = bCorners[nNumEdges] = fTest < 0.f;
	//bCorners0[0] = bCorners0[nNumEdges] = fTest <  0.f;
	// bCorners[nNumEdges] = bCorners[0];
	// bCorners0[nNumEdges] = bCorners0[0];

	bCorners++;
	//bCorners0++;


	uint32 nEdgesIn = 0;
	uint32 nEdgesOut = 0;
	int nClipAddedIn = 0;
	int nClipAddedOut = 0;

	// TRUE --> OUTSIDE

	for(int i = 0; i < nNumEdges; ++i)
	{
		bool bCorner0 = bCorners[(i - 1)];
		bool bCorner1 = bCorners[i];
		if(bCorner0 == bCorner1)
		{
			if(bCorner0) // both outside
			{
				pPolyOut[nEdgesOut++] = pIndices[i];

			}
			else // both insides
			{
				pPolyIn[nEdgesIn++] = pIndices[i];
			}

		}
		else
		{

			// add to both
			if(!nClipAddedIn)
			{
				nClipAddedIn = true;
				if(bCorner0) // outside --> inside
				{
					pPolyOut[nEdgesOut++] = pIndices[i];
					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
					pPolyIn[nEdgesIn++] = nClipPlane ;
					pPolyIn[nEdgesIn++] = pIndices[i];
				}
				else // inside --> outside
				{
					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
					pPolyOut[nEdgesOut++] = pIndices[i];
					pPolyIn[nEdgesIn++] = pIndices[i] ;
					pPolyIn[nEdgesIn++] = nClipPlane;

				}
			}
			else
			{
				pPolyOut[nEdgesOut++] = pIndices[i];
				pPolyIn[nEdgesIn++] = pIndices[i];

			}
		}
	}

	ZASSERT(nEdgesIn == 0 || nEdgesIn > 2);
	ZASSERT(nEdgesOut == 0 || nEdgesOut > 2);

	// add normal plane
	if(nEdgesIn)
		pPolyIn[nEdgesIn++] = pIndices[nNumEdges];
	if(nEdgesOut)
		pPolyOut[nEdgesOut++] = pIndices[nNumEdges];

	nEdgesIn_ = nEdgesIn;
	nEdgesOut_ = nEdgesOut;
	return ClipPolyResult((nEdgesIn ? 0x1 : 0) | (nEdgesOut ? 0x2 : 0));
}

ClipPolyResult BspClipPolySingle(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
						   uint16 nNumPlanes, uint32 nExcludeMask, uint16 *pPolyIn,
						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_,
						   uint32 &nMaskIn_, uint32 &nMaskOut_)
{
	//MICROPROFILE_SCOPEIC("BSP", "ClipPoly");

	ZASSERT(nNumPlanes < 32);
	uint32 nNumEdges = nNumPlanes - 1;
	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumEdges]);

	ZASSERT(nNumEdges > 2);
	v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);

	bool *bCorners = (bool *)alloca(nNumPlanes);
	v4 vLastPlane = BspGetPlane(pBsp, pIndices[ 0 ]);
	v4 vFirstPlane = vLastPlane;
	//bool bDebug = 0 == pBsp->Debug.nFirst++;
	for(int i = 1; i < nNumEdges; ++i)
	{
		v4 p1 = BspGetPlane(pBsp, pIndices[i]);
		float fTest = BspPlaneTestNew(vLastPlane, p1, vClipPlane);
		vLastPlane = p1;
		bCorners[i] = fTest < 0.f;
	}

	float fTest = BspPlaneTestNew(vLastPlane, vFirstPlane, vClipPlane);
	bCorners[0] = bCorners[nNumEdges] = fTest < 0.f;
	bCorners++;



	uint32 nEdgesIn = 0;
	uint32 nEdgesOut = 0;
	uint32 nMaskIn = 0;
	uint32 nMaskOut = 0;
	uint32 nMaskRollIn = 1;
	uint32 nMaskRollOut = 1;
	uint32 nMask = nExcludeMask;
	uint32 nMaskCopyIn = nExcludeMask;
	int nClipAddedIn = 0;
	int nClipAddedOut = 0;

	// TRUE --> OUTSIDE

	for(int i = 0; i < nNumEdges; ++i)
	{
		bool bCorner0 = bCorners[(i - 1)];
		bool bCorner1 = bCorners[i];
		if(bCorner0 == bCorner1)
		{
			if(bCorner0) // both outside
			{
				pPolyOut[nEdgesOut++] = pIndices[i];
				if(nMask & 1)
					nMaskOut |= nMaskRollOut;
				nMaskRollOut <<= 1;
			}
			else // both insides
			{
				pPolyIn[nEdgesIn++] = pIndices[i];
				if(nMaskCopyIn & 1)
					nMaskIn |= nMaskRollIn;
				nMaskRollIn <<= 1;

			}
		}
		else if(bCorner0) // outside --> inside
		{
			pPolyOut[nEdgesOut++] = pIndices[i];
			pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
			pPolyIn[nEdgesIn++] = nClipPlane ;
			pPolyIn[nEdgesIn++] = pIndices[i];
			if(nMask & 1)
				nMaskOut |= nMaskRollOut;
			nMaskRollOut <<= 1;

					
			nMaskOut |= nMaskRollOut;
			nMaskRollOut <<= 1;

					
			nMaskIn |= nMaskRollIn;
			nMaskRollIn <<= 1;



					
			if(nMaskCopyIn & 1)
				nMaskIn |= nMaskRollIn;
			nMaskRollIn <<= 1;

		}
		else
		{
			pPolyOut[nEdgesOut++] = pIndices[i];
			pPolyIn[nEdgesIn++] = pIndices[i];
			if(nMask & 1)
				nMaskOut |= nMaskRollOut;
			nMaskRollOut <<= 1;
			if(nMaskCopyIn & 1)
				nMaskIn |= nMaskRollIn;
			nMaskRollIn <<= 1;
		}
		nMask >>= 1;
		nMaskCopyIn >>= 1;

	}


	ZASSERT(nEdgesIn == 0 || nEdgesIn > 2);
	ZASSERT(nEdgesOut == 0 || nEdgesOut > 2);

	// add normal plane
	if(nEdgesIn)
		pPolyIn[nEdgesIn++] = pIndices[nNumEdges];
	if(nEdgesOut)
		pPolyOut[nEdgesOut++] = pIndices[nNumEdges];

	nEdgesIn_ = nEdgesIn;
	nEdgesOut_ = nEdgesOut;
	nMaskIn_ = nMaskIn;
	nMaskOut_ = nMaskOut;

	if(g_nPolyExtraDump)
	{
		uprintf("DumpIN\n");
		BspDumpPlanes("IN ",pBsp, pPolyIn, nEdgesIn);
		uprintf("DumpOUT\n");
		BspDumpPlanes("OUT", pBsp, pPolyOut, nEdgesOut);


	}
	return ClipPolyResult((nEdgesIn ? 0x1 : 0) | (nEdgesOut ? 0x2 : 0));
}



// ClipPolyResult BspClipPolyChecked(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
// 						   uint16 nNumPlanes, uint32 nExcludeMask, uint16 *pPolyIn,
// 						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_,
// 						   uint32 &nMaskIn_, uint32 &nMaskOut_)
// {

// 	ClipPolyResult CR = BspClipPolySafe2(pBsp, nClipPlane, pIndices, nNumPlanes, nExcludeMask,
// 						 pPolyIn, pPolyOut, nEdgesIn_, nEdgesOut_, nMaskIn_, nMaskOut_);

// 	ZASSERT(nEdgesIn_ <= nNumPlanes+1);
// 	uint32 nEdgesIn_2;
// 	uint32 nEdgesOut_2;
// 	uint32 nExcludeMaskIn2, nExcludeMaskOut2;
// 	uint16* pClippedIn = (uint16*)alloca(sizeof(uint16)*(1+nNumPlanes));
// 	uint16* pClippedOut = (uint16*)alloca(sizeof(uint16)*(1+nNumPlanes));


// 	ClipPolyResult CR2 = BspClipPolySafe2(pBsp, nClipPlane, pIndices, nNumPlanes, nExcludeMask,
// 						 pClippedIn, pClippedOut, nEdgesIn_2, nEdgesOut_2, nExcludeMaskIn2, nExcludeMaskOut2);
// 	ZASSERT(CR2 == CR);
// 	ZASSERT(nEdgesIn_2 == nEdgesIn_);
// 	ZASSERT(nEdgesOut_2 == nEdgesOut_);
// 	ZASSERT(nExcludeMaskIn2 == nMaskIn_);
// 	ZASSERT(nExcludeMaskOut2 == nMaskOut_);
// 	ZASSERT(0 == memcmp(pClippedIn, pPolyIn, sizeof(uint16) * nEdgesIn_));
// 	ZASSERT(0 == memcmp(pClippedOut, pPolyOut, sizeof(uint16) * nEdgesOut_));
// 	//uprintf("test done\n");
// 	return CR;


// }


void BspAddRecursive(SOccluderBsp *pBsp, uint32 nBspIndex, uint16 *pIndices, uint16 nNumIndices,
					 uint32 nExcludeMask, uint32 nLevel)
{
	SOccluderBspNode Node = pBsp->Nodes[nBspIndex];
	v4 vPlane = BspGetPlane(pBsp, Node.nPlaneIndex);

	bool bPlaneOverlap = false;
	#if 0
	{
		for(int i = 0; i < nNumIndices - 1; ++i)
		{
			if(0 == (nExcludeMask & (1 << i)))
			{
				v4 vInner = BspGetPlane(pBsp, pIndices[i]);
				float fDot0 = v4distance2(vInner, vPlane);
				float fDot1 = v4distance2(v4neg(vInner), vPlane);
#define TRESHOLD2 (0.000001f)
				if(fDot0 < TRESHOLD2 || fDot1 < TRESHOLD2)
				{
					#if USE_EXCLUDE_MASK_OVERLAP
					nExcludeMask |= (1 << i);
					#endif
					//bPlaneOverlap = true;
				}
			}
		}
	}
	#endif

	uint32 nCount = (nNumIndices+1)*4;
	uint16 ClippedPolyIn[OCCLUDER_POLY_MAX];
	uint16 ClippedPolyOut[OCCLUDER_POLY_MAX];

	uint16 *pPolyInside = nullptr;
	uint32 nPolyEdgeIn = 0;
	uint16 *pPolyOutside = nullptr;
	uint32 nPolyEdgeOut = 0;
	uint32 nExcludeMaskOut = 0, nExcludeMaskIn = 0;

	ClipPolyResult CR = ECPR_CLIPPED;
	if(Node.nInside == OCCLUDER_LEAF && vPlane.w != 0.f)
	{
		if(BSP_ADD_FRONT)
		{
			ZASSERT(vPlane.w != 0.f);
			bool bOk = false;
			{
				{
					ZASSERT(vPlane.w != 0.f);

					v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumIndices - 1]);
					for(int i = 0; i < nNumIndices - 1; ++i)
					{
						v4 p0 = BspGetPlane(pBsp, pIndices[i]);
						v4 p1 = BspGetPlane(pBsp, pIndices[(i + 1) % (nNumIndices - 1)]);
						v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
						float fDot = v4dot(v4init(vIntersect, 1.f), vPlane);
						bOk = bOk || fDot < 0.f;
					}
				}
			}
			if(bOk)
			{
				pPolyOutside = pIndices;
				nPolyEdgeOut = nNumIndices;
				nExcludeMaskOut = nExcludeMask;
				CR = ECPR_OUTSIDE;
			}
		}
	}
	else if(bPlaneOverlap)
	{
		uint32 nNumEdgeIn = 0, nNumEdgeOut = 0;
		int nA = 0;
		int nB = 0;
		float fDotSum = 0.f;
		float fMaxDot = 0.f;

		v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumIndices - 1]);
		BSP_DUMP_PRINTF(pBsp, "Plane Overlap %d\n", nBspIndex);
		for(int i = 0; i < nNumIndices - 1; ++i)
		{
			v4 p0 = BspGetPlane(pBsp, pIndices[i]);
			v4 p1 = BspGetPlane(pBsp, pIndices[(i + 1) % (nNumIndices - 1)]);
			v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
			float fDot = v4dot(v4init(vIntersect, 1.f), vPlane);

			BSP_DUMP_PRINTF(pBsp, "Plane Overlap %d %d:%f\n", nBspIndex, i, fDot);
			if(fDot < -0.001f)
				nA++;
			if(fDot > 0.001f)
				nB++;
			fDotSum += fDot;
			fMaxDot = Max(fMaxDot, fDot);
			float fAbsDot = fabs(fDot);
			fMaxDot = Max(fMaxDot, fAbsDot);
		}


		{
			BSP_DUMP_PRINTF(pBsp, "Plane Overlap %d maxdot %f dotsum %f\n", nBspIndex, fMaxDot, fDotSum);
			if(fMaxDot>1.f)
			{
				if(fDotSum < -0.001f)
				{
					pPolyOutside = pIndices;
					nPolyEdgeOut = nNumIndices;
					nExcludeMaskOut = nExcludeMask;
					CR = ECPR_OUTSIDE;
				}
				else if(fDotSum > 0.001f)
				{
					pPolyInside = pIndices;
					nPolyEdgeIn = nNumIndices;
					nExcludeMaskIn = nExcludeMask;
					CR = ECPR_INSIDE;
				}
			}
			else
			{
				BSP_DUMP_PRINTF(pBsp, "Plane Overlap Reject %d\n", nBspIndex);
			}
		}
	}
	else
	{
		ZASSERT(vPlane.w == 0.f);

		uint32 nNumEdgeIn = 0, nNumEdgeOut = 0;

		CR = BspClipPoly(pBsp, Node.nPlaneIndex, pIndices, nNumIndices, nExcludeMask,
						 &ClippedPolyIn[0], &ClippedPolyOut[0], nNumEdgeIn, nNumEdgeOut,
						 nExcludeMaskIn, nExcludeMaskOut);
		pPolyInside = &ClippedPolyIn[0];
		nPolyEdgeIn = nNumEdgeIn;
		pPolyOutside = &ClippedPolyOut[0];
		nPolyEdgeOut = nNumEdgeOut;
	}
	ZASSERT(Node.nInside != OCCLUDER_EMPTY);
	ZASSERT(Node.nOutside != OCCLUDER_LEAF);
	{
		if((ECPR_INSIDE & CR) != 0 && (pBsp->Debug.nInsideOutSide&1))
		{
			if(Node.nInside == OCCLUDER_LEAF)
			{
				// inside a leaf, kill it
			}
			else
			{

				BspAddRecursive(pBsp, Node.nInside, pPolyInside, nPolyEdgeIn, nExcludeMaskIn,
								nLevel + 1);
			}
		}

		if((ECPR_OUTSIDE & CR) != 0&& (pBsp->Debug.nInsideOutSide&2))
		{
			if(Node.nOutside == OCCLUDER_EMPTY)
			{
				// uplotfnxt("addSIDE %d", nPolyEdgeOut);
				int nIndex = BspAddInternal(pBsp, nBspIndex, true, pPolyOutside, nPolyEdgeOut,
											nExcludeMaskOut, nLevel);
			}
			else
			{
				/// uplotfnxt("ADD OUTSIDE_R %d", nPolyEdgeOut);
				BspAddRecursive(pBsp, Node.nOutside, pPolyOutside, nPolyEdgeOut, nExcludeMaskOut,
								nLevel + 1);
			}
		}
	}
}

bool BspCullObjectR(SOccluderBsp *pBsp, SOccluderBspNodes* pNodes, uint32 Index, uint16 *Poly, uint32 nNumEdges,
					int nClipLevel)
{	

	SOccluderBspNode Node = pNodes->Nodes[Index];
	v4 vPlane = BspGetPlane(pBsp, Node.nPlaneIndex);

	BSP_DUMP_PRINTF(pBsp, "%sCULLR %08x P[%4x] In[%08x] Out[%08x] Plane[%f,%f,%f,%f]\n", Spaces(nClipLevel*2), Index, 
		nNumEdges,
		Node.nInside,
		Node.nOutside, vPlane.x, vPlane.y, vPlane.z, vPlane.w);


	if(pBsp->Debug.DebugPlane == pBsp->Debug.BspDebugPlaneCounter)
	{
		uplotfnxt("debug testing %d", nNumEdges);
		uplotfnxt("DEBUG PLANE USING node %d", Index);
		BspDrawPoly(pBsp, Poly, nNumEdges, 0xff000000|randredcolor(), 0, 1);
		int iParent = Index;
		uint32 nColor = 0xff00ff00;
		do
		{

			v4 vPlane = BspGetPlaneFromNode(pBsp, pNodes, iParent);
			v3 vCorner = BspGetCornerFromNode(pBsp, pNodes, iParent);
			vCorner = mtransform(pBsp->mfrombsp, vCorner);
			vPlane = v4init(mrotate(pBsp->mfrombsp, vPlane.tov3()), 1.f);
			ZDEBUG_DRAWPLANE(vPlane.tov3(), vCorner, nColor);
			nColor = 0xffff0000;
			iParent = BspFindParent(pNodes, iParent);
		}while(iParent != -1);



		if(pBsp->Debug.BreakOnClipIndex && pBsp->Debug.DebugPlane == pBsp->Debug.BspDebugPlaneCounter)
		{
			//uprintf("BREAK\n");
		}
	}




	const uint32 nMaxEdges = (nNumEdges + 1);
	uint16 *ClippedPolyIn = 0;
	uint16 *ClippedPolyOut = 0;
	uint32 nIn = 0;
	uint32 nOut = 0;
	uint32 nExclusionIn = 0, nExclusionOut = 0;

	int CR = 0;





	ZASSERT(Node.nOutside != OCCLUDER_LEAF);
	if(vPlane.w != 0.f)
	{

//		TODO: lav CR her, fix loop under.
		#if 0
		ZASSERT(Node.nInside == OCCLUDER_LEAF);
		ZASSERT(vPlane.w != 0.f);
		v4 vNormalPlane = BspGetPlane(pBsp, Poly[nNumEdges - 1]);
		bool bVisible = false;
		bool bCulled = true;
		for(int i = 0; i < nNumEdges - 1; ++i)
		{
			v4 p0 = BspGetPlane(pBsp, Poly[i]);
			v4 p1 = BspGetPlane(pBsp, Poly[(i + 1) % (nNumEdges - 1)]);
			v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
			float fDot = v4dot(v4init(vIntersect, 1.f), vPlane);
			bVisible = bVisible || fDot < 0.f;
			bCulled = bCulled && fDot < 0.f;
		}
		if(bVisible && pBsp->Debug.OccluderDebugDrawClipResult)
		{
			BspDrawPoly(pBsp, Poly, nNumEdges, 0xff000000 | randredcolor(), 0, 1);
		}
		BSP_DUMP_PRINTF(pBsp, "%sLEAF_TEST %08x CULLED %d\n", Spaces(2+nClipLevel*2), Index, bVisible ? 0 : 1);
		#endif

		CR = BspClipLeaf(pBsp, vPlane, Poly, nNumEdges);
//			ClipPolyResult BspClipLeaf(SOccluderBsp* pBsp, v4 vPlane, uint16* Poly, uint32 nNumEdges)
		if(CR & ECPR_OUTSIDE)
		{
			ClippedPolyOut = &Poly[0];
			nOut = nNumEdges;
		}
		else
		{
			return true;
		}
		//always disregard inside poly.
		CR &= ~(ECPR_INSIDE);

		if(pBsp->Debug.nShowClipLevel == nClipLevel)
		{
			if(pBsp->Debug.nShowClipLevelSubCounter++ == (pBsp->Debug.nShowClipLevelSub))
			{
				uplotfnxt("**BSPShowClipLevel: LEAF TEST  %d.. child in %d out %d", Index, Node.nInside, Node.nOutside);
				// uplotfnxt("**BSPShowClipLevel: bVisible %d ", bVisible);
				int iParent = Index;
				uint32 nColor = 0xff00ff00;
				do
				{
					uplotfnxt("**BSPShowClipLevel: IDX %d, plane idx %d", iParent, pBsp->Nodes[iParent].nPlaneIndex);
					v4 vPlane = BspGetPlaneFromNode(pBsp, pNodes, iParent);
					v3 vCorner = BspGetCornerFromNode(pBsp, pNodes, iParent);
					vCorner = mtransform(pBsp->mfrombsp, vCorner);
					vPlane = v4init(mrotate(pBsp->mfrombsp, vPlane.tov3()), 1.f);
					ZDEBUG_DRAWPLANE(vPlane.tov3(), vCorner, nColor);
					nColor = 0xffff0000;
					iParent = BspFindParent(pNodes, iParent);
				}while(iParent != -1);

			}
		}


		// return !bVisible;
	}
	else
	{
		ZASSERT(vPlane.w == 0.f);
		ClippedPolyIn = (uint16 *)alloca(nMaxEdges * sizeof(uint16));
		ClippedPolyOut = (uint16 *)alloca(nMaxEdges * sizeof(uint16));
		CR = BspClipPolyCull(pBsp, Node.nPlaneIndex, Poly, nNumEdges, ClippedPolyIn, ClippedPolyOut, nIn, nOut);



	}


	// ClipPolyResult CR = BspClipPolyCull(pBsp, Node.nPlaneIndex, Poly, nNumEdges, 0, ClippedPolyIn,
	// 								ClippedPolyOut, nIn, nOut, nExclusionIn, nExclusionOut);


	if(pBsp->Debug.DebugPlane == pBsp->Debug.BspDebugPlaneCounter)
	{
		uplotfnxt("debug test in %d out %d", nIn, nOut);
	}


	pBsp->Debug.BspDebugPlaneCounter++;

	if(pBsp->Debug.nShowClipLevel == nClipLevel)
	{
		if(pBsp->Debug.nShowClipLevelSubCounter++ == (pBsp->Debug.nShowClipLevelSub))
		{
			uplotfnxt("**BSPShowClipLevel: Drawing %d.. child in %d out %d", Index, Node.nInside, Node.nOutside);
			uplotfnxt("**BSPShowClipLevel: INSIDE %d OUTSIDE %d", nIn, nOut);

			if(nOut)
				BspDrawPoly(pBsp, ClippedPolyOut, nOut, 0xffff0000, 0, 1);
			if(nIn)
				BspDrawPoly(pBsp, ClippedPolyIn, nIn, 0xffffffff, 0, 1);



			int iParent = Index;
			uint32 nColor = 0xff00ff00;
			do
			{
				uplotfnxt("**BSPShowClipLevel: IDX %d, plane idx %d", iParent, pBsp->Nodes[iParent].nPlaneIndex);
				v4 vPlane = BspGetPlaneFromNode(pBsp, pNodes, iParent);
				v3 vCorner = BspGetCornerFromNode(pBsp, pNodes, iParent);
				vCorner = mtransform(pBsp->mfrombsp, vCorner);
				vPlane = v4init(mrotate(pBsp->mfrombsp, vPlane.tov3()), 1.f);
				ZDEBUG_DRAWPLANE(vPlane.tov3(), vCorner, nColor);
				nColor = 0xffff0000;
				iParent = BspFindParent(pNodes, iParent);
			}while(iParent != -1);


			return false;
		}
	}
	bool bFail = false;

	if(CR & ECPR_INSIDE)
	{
		if(Node.nInside == OCCLUDER_EMPTY)
		{
			return false;
		}
		if(Node.nInside == OCCLUDER_LEAF)
		{
			//			ZASSERT(Node.nEdge == 4);
			// culled
		}
		else if(!BspCullObjectR(pBsp, pNodes, Node.nInside, ClippedPolyIn, nIn, nClipLevel + 1))
		{
			if(!pBsp->Debug.OccluderDebugDrawClipResult)
				return false;
			bFail = true;
			//	uplotfnxt("FAIL INSIDE");
		}
	}
	if(CR & ECPR_OUTSIDE)
	{
		if(pBsp->Debug.OccluderDebugDrawClipResult && Node.nOutside == OCCLUDER_EMPTY)
		{
			if(pBsp->Debug.nShowClipLevel < 0)
				BspDrawPoly(pBsp, ClippedPolyOut, nOut, 0xff000000 | randredcolor(), 0, 1);
		}

		ZASSERT(Node.nOutside != OCCLUDER_LEAF);
		if((Node.nOutside == OCCLUDER_EMPTY ||
			!BspCullObjectR(pBsp, pNodes, Node.nOutside, ClippedPolyOut, nOut, nClipLevel + 1)))
		{
			return false;
		}
	}
	return !bFail;
}

bool BspCullObjectSafe(SOccluderBsp *pBsp, SOccluderDesc *pObject, SOccluderBspNodes* pNodes)
{
	// if(0 == (pObject->nFlags & SObject::OCCLUSION_TEST))
	// 	return false;
	pBsp->Stats.nNumObjectsTested++;
	if(!pNodes)
	{
		pNodes = &pBsp->Nodes;
		//pNodes = &pBsp->Nodes;
	}

	MICROPROFILE_SCOPEIC("BSP", "Cull");
	if(!pNodes->Size())
	{
		return false;
	}
	pBsp->Debug.BspDebugPlaneCounter = 0;
	long seed = rand();
	srand((int)(uintptr)pObject);
	randseed(0xed32babe, 0xdeadf39c);


	pBsp->Debug.nShowClipLevelSubCounter = 0;




	// SBspEdgeIndex Poly[5];
	uint16_t Poly[5];
	uint32 nSize = pBsp->Planes.Size();
	{
		//MICROPROFILE_SCOPEIC("BSP", "CullPrepare");
		const m mObjectToWorld = mload(pObject->ObjectToWorld);
		m mObjectToBsp = mmult(pBsp->mtobsp, mObjectToWorld);
		//m mObjectToBsp = mmult_sse(&pBsp->mtobsp, &mObjectToWorld);
		v3 vHalfSize = v3load(pObject->Size);
		v4 vCenterWorld_ = mtransform(pBsp->mtobsp, mObjectToWorld.trans);
		v4 vCenterWorld_1 = mObjectToBsp.trans;
		v3 vCenterWorld = v3init(vCenterWorld_);
		v3 vCenterWorld1 = v3init(vCenterWorld_1);
		// uprintf("distance is %f\n", v3distance(vCenterWorld1, vCenterWorld));
		ZASSERT(abs(v3distance(vCenterWorld1, vCenterWorld)) < 1e-3f);
		v3 vOrigin = v3zero(); // vLocalOrigin
		v3 vToCenter = v3normalize(vCenterWorld - vOrigin);
		v3 vUp = v3init(0.f, 1.f, 0.f); // replace with camera up.
		if(v3dot(vUp, vToCenter) > 0.9f)
			vUp = v3init(1.f, 0, 0);
		v3 vRight = v3normalize(v3cross(vToCenter, vUp));
		m mbox = mcreate(vToCenter, vRight, vCenterWorld);
		m mboxinv = maffineinverse(mbox);
		m mcomposite = mmult(mbox, mObjectToBsp);
		v3 AABB = obbtoaabb(mcomposite, vHalfSize);

		vRight = mgetxaxis(mboxinv);
		vUp = mgetyaxis(mboxinv);

		v3 vCenterQuad = vCenterWorld - vToCenter * AABB.z * 1.1f;
		v3 v0 = vCenterQuad + vRight * AABB.x + vUp * AABB.y;
		v3 v1 = vCenterQuad + vRight * -AABB.x + vUp * AABB.y;
		v3 v2 = vCenterQuad + vRight * -AABB.x + vUp * -AABB.y;
		v3 p3 = vCenterQuad + vRight * AABB.x + vUp * -AABB.y;

		pBsp->Planes.Resize(nSize + 5); // TODO use thread specific blocks

		v4 *pPlanes = pBsp->Planes.Ptr() + nSize;
		v3 n0 = v3normalize(v3cross(v0, v0 - v1));
		v3 n1 = v3normalize(v3cross(v1, v1 - v2));
		v3 n2 = v3normalize(v3cross(v2, v2 - p3));
		v3 n3 = v3normalize(v3cross(p3, p3 - v0));
		pPlanes[0] = v4init(n0, 0.f);
		pPlanes[1] = v4init(n1, 0.f);
		pPlanes[2] = v4init(n2, 0.f);
		pPlanes[3] = v4init(n3, 0.f);
		pPlanes[4] = v4makeplane(p3, v3normalize(vToCenter));

		float fTest = BspPlaneTestNew(pPlanes[0], pPlanes[1], pPlanes[2]);
		if(fTest > 0.f)
		{

			for(int i = 0; i < 5; ++i)
				Poly[i] = nSize + i;
		}
		else
		{
			Poly[4] = nSize + 4;
			Poly[0] = nSize + 3;
			Poly[1] = nSize + 2;
			Poly[2] = nSize + 1;
			Poly[3] = nSize + 0;
		}
	}

	


	bool bResult = BspCullObjectR(pBsp, pNodes, 0, &Poly[0], 5, 0);
	pBsp->Planes.Resize(nSize);
	srand(seed);
	if(!bResult)
		pBsp->Stats.nNumObjectsTestedVisible++;

	return bResult;
}

enum EBPResult
{
	EBUILD_PLANES_OK,
	EBUILD_PLANES_INVALID,//object cannot be culled
};

EBPResult BspBuildPlanes(v4* pPlanes, uint16_t Poly[5], SOccluderBsp* pBsp, SOccluderDesc* pObject, uint16_t nOffset, v3* vBackCorners = 0)
{
	mat mObjectToWorld = mload44(&pObject->ObjectToWorld);
	mat mToBsp = mload44(&pBsp->mtobsp);
	mat mObjectToBsp = mmult(mToBsp, mObjectToWorld);
	vec vTrans = mObjectToWorld.w;
	vec vHalfSize = vload3(&pObject->Size[0]);
	vec vCenterWorld = mtransformaffine(mToBsp, vTrans);
	vec vToCenter = vnormalize3(vCenterWorld);
	vec vUp = vset(0.f, 1.f, 0.f, 0.f); // replace with camera up.
	if(vgetx(vdot3(vUp, vToCenter)) > 0.9f)
	{
		vUp = vset(1.f, 0.f, 0.f, 0.f);
	}
	vec vRight = vnormalize3(vcross3(vToCenter, vUp));
	vUp = vnormalize3(vcross3(vneg(vToCenter), vRight));


	mat mboxinv;
	mboxinv.x = vsetw(vRight, 0.f);
	mboxinv.y = vsetw(vUp, 0.f);
	mboxinv.z = vsetw(vneg(vToCenter), 0.f);
	mboxinv.w = vsetw(vCenterWorld, 1.f);
	mat mbox = maffineinverse(mboxinv);

	mat mcomposite = mmult(mbox, mObjectToBsp);
	vec AABB = obbtoaabb(mcomposite, vHalfSize);
	vec AABBx = vsplatx(AABB);
	vec AABBy = vsplaty(AABB);
	vec AABBz = vsplatz(AABB);
	vec vCenterQuad = vsub(vCenterWorld, vmul(vToCenter, vmul(AABBz, vrep(1.1f))));
	// if(vCenterQuad.z < 0)
	// {
	// 	//
	// }
	vec vRightx = vmul(vRight, AABBx);
	vec vUpy = vmul(vUp, AABBy);
	if(vBackCorners)
	{
		mat mBspToObject = maffineinverse(mObjectToBsp);

		vec inside = vset(0,0,0,1.f);
		inside = mtransformaffine(mBspToObject, inside);
		float hslen = vgetx(vlength3(vHalfSize));

		float fDistance = vgetx(vlength3(inside));
		if(fDistance < hslen)
		{
			return EBUILD_PLANES_INVALID;
		}



		vec vCenterBackQuad = vsub(vCenterWorld, vmul(vToCenter, vmul(vneg(AABBz), vrep(1.1f))));
		vec vx = vadd(vCenterBackQuad, vRightx);
		vec vnx = vadd(vCenterBackQuad, vneg(vRightx));
		vec vny = vneg(vUpy);
		vec c0 = vadd(vx, vUpy);
		vec c1 = vadd(vx, vny);
		vec c2 = vadd(vnx, vUpy);
		vec c3 = vadd(vnx, vny);
		vstore3(&vBackCorners[0], c0);
		vstore3(&vBackCorners[1], c1);
		vstore3(&vBackCorners[2], c2);
		vstore3(&vBackCorners[3], c3);
		mat mfrombsp = mload44(&pBsp->mfrombsp);
		__m128 v0 = mtransformaffine(mfrombsp, c0);
		__m128 v1 = mtransformaffine(mfrombsp, c1);
		__m128 v2 = mtransformaffine(mfrombsp, c2);
		__m128 v3_ = mtransformaffine(mfrombsp, c3);
		v3 x0, x1, x2, x3;
		vstore3(&x0, v0);
		vstore3(&x1, v1);
		vstore3(&x2, v2);
		vstore3(&x3, v3_);
	}

	vec v0 = vadd(vadd(vCenterQuad, vRightx), vUpy);
	vec v1 = vadd(vsub(vCenterQuad, vRightx), vUpy);
	vec v2 = vsub(vsub(vCenterQuad, vRightx), vUpy);
	vec p3 = vsub(vadd(vCenterQuad, vRightx), vUpy);
	vec n0 = vnormalize3(vcross3(v0, vsub(v0 ,v1)));
	vec n1 = vnormalize3(vcross3(v1, vsub(v1 ,v2)));
	vec n2 = vnormalize3(vcross3(v2, vsub(v2 ,p3)));
	vec n3 = vnormalize3(vcross3(p3, vsub(p3 ,v0)));
	vstore4(pPlanes + 0, vsetwzero(n0));
	vstore4(pPlanes + 1, vsetwzero(n1));
	vstore4(pPlanes + 2, vsetwzero(n2));
	vstore4(pPlanes + 3, vsetwzero(n3));
	vec plane = vmakeplane(p3, vnormalize3(vToCenter));
	vstore4(pPlanes + 4, plane);
	float fTest = tripleproduct(n0, n1, n2);
	if(fTest > 0.f)
	{
		for(int i = 0; i < 5; ++i)
			Poly[i] = nOffset + i;
	}
	else
	{
		Poly[4] = nOffset + 4;
		Poly[0] = nOffset + 3;
		Poly[1] = nOffset + 2;
		Poly[2] = nOffset + 1;
		Poly[3] = nOffset + 0;
	}

	return EBUILD_PLANES_OK;
}


bool BspCullObject(SOccluderBsp *pBsp, SOccluderDesc *pObject, SOccluderBspNodes* pNodes)
{
	BSP_DUMP_PRINTF(pBsp, "CULL %p nodes %p, size %d\n", pObject, pNodes, pNodes? pNodes->Size() : 0);

	pBsp->Stats.nNumObjectsTested++;
	if(pNodes)
	{
		pBsp->Stats.nNunSubBspTests++;
	}
	else
	{
		pNodes = &pBsp->Nodes;
	}
	if(!pNodes->Nodes.Size())
		return false;
	pBsp->Debug.BspDebugPlaneCounter = 0;
	long seed = rand();
	srand((int)(uintptr)pObject);
	randseed(0xed32babe, 0xdeadf39c);
	pBsp->Debug.nShowClipLevelSubCounter = 0;


	uint16_t Poly[5];
	uint32 nSize = pBsp->Planes.Size();
	pBsp->Planes.Resize(nSize + 5); // TODO use thread specific blocks
	v4 *pPlanes = pBsp->Planes.Ptr() + nSize;
	BspBuildPlanes(pPlanes, Poly, pBsp, pObject, nSize);
	bool bResult = BspCullObjectR(pBsp, pNodes, 0, &Poly[0], 5, 0);
	pBsp->Planes.Resize(nSize);
	srand(seed);
	if(!bResult)
		pBsp->Stats.nNumObjectsTestedVisible++;

	return bResult;
}
#define PLANE_DEBUG_TESSELATION 20
#define PLANE_DEBUG_SIZE 5

void BspDebugDrawHalfspace(v3 vNormal, v3 vPosition)
{
	v3 vLeft = v3cross(vNormal, v3init(1, 0, 0));
	if(v3length(vLeft) < 0.1f)
		vLeft = v3cross(vNormal, v3init(0, 1, 0));
	if(v3length(vLeft) < 0.51f)
		vLeft = v3cross(vNormal, v3init(0, 0, 1));

	ZASSERT(v3length(vLeft) > 0.4f);
	vLeft = v3normalize(vLeft);
	v3 vUp = v3normalize(v3cross(vNormal, vLeft));
	vLeft = v3normalize(v3cross(vUp, vNormal));
	v3 vPoints[PLANE_DEBUG_TESSELATION * PLANE_DEBUG_TESSELATION];
	for(int i = 0; i < PLANE_DEBUG_TESSELATION; ++i)
	{
		float x = PLANE_DEBUG_SIZE * (((float)i / (PLANE_DEBUG_TESSELATION - 1)) - 0.5f);
		for(int j = 0; j < PLANE_DEBUG_TESSELATION; ++j)
		{
			float y = PLANE_DEBUG_SIZE * (((float)j / (PLANE_DEBUG_TESSELATION - 1)) - 0.5f);
			vPoints[i * PLANE_DEBUG_TESSELATION + j] = vPosition + vUp * x + vLeft * y;
		}
	}

	for(int i = 0; i < PLANE_DEBUG_TESSELATION - 1; ++i)
	{
		for(int j = 0; j < PLANE_DEBUG_TESSELATION - 1; ++j)
		{
			ZDEBUG_DRAWLINE(vPoints[i * PLANE_DEBUG_TESSELATION + j],
							vPoints[i * PLANE_DEBUG_TESSELATION + j + 1], 0xffff0000, true);
			ZDEBUG_DRAWLINE(vPoints[(1 + i) * PLANE_DEBUG_TESSELATION + j],
							vPoints[i * PLANE_DEBUG_TESSELATION + j], 0xffff0000, true);
			ZDEBUG_DRAWLINE(vPoints[i * PLANE_DEBUG_TESSELATION + j],
							vPoints[i * PLANE_DEBUG_TESSELATION + j] + vNormal * 0.03, 0xff00ff00,
							true);
		}
	}
}


void BspGetStats(SOccluderBsp* pBsp, SOccluderBspStats* pStats)
{
	memcpy(pStats, &pBsp->Stats, sizeof(SOccluderBspStats));
}

void BspClearCullStats(SOccluderBsp* pBsp)
{
	memset(&pBsp->Stats, 0, sizeof(SOccluderBspStats));
	pBsp->Stats.nNumObjectsTested = 0;
	pBsp->Stats.nNumObjectsTestedVisible = 0;
	pBsp->Stats.nNumChildBspsCreated = 0;
	pBsp->Stats.nNumChildBspsVisible = 0;
	pBsp->Stats.nNunSubBspTests = 0;
}

ClipPolyResult BspClipLeaf(SOccluderBsp* pBsp, v4 vPlane, uint16* Poly, uint32 nNumEdges)
{
	ZASSERT(vPlane.w != 0.f);
	ZASSERT(vPlane.w != 0.f);
	v4 vNormalPlane = BspGetPlane(pBsp, Poly[nNumEdges - 1]);
	bool bVisible = false;
	int CR = 0;
	// bool bCulled = true;
	for(int i = 0; i < nNumEdges - 1; ++i)
	{
		v4 p0 = BspGetPlane(pBsp, Poly[i]);
		v4 p1 = BspGetPlane(pBsp, Poly[(i + 1) % (nNumEdges - 1)]);
		v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
		float fDot = v4dot(v4init(vIntersect, 1.f), vPlane);
		//verify sign lalalala
		if(fDot > 0.f)
		{
			CR |= ECPR_INSIDE;
		}
		else
		{
			CR |= ECPR_OUTSIDE;
		}
	}
	return (ClipPolyResult)CR;
}

uint16_t BspBuildSubBspR(SOccluderBspNodes& NodeBsp, SOccluderBsp *pBsp, uint32_t nIndex, uint16_t* Poly, uint32_t nNumEdges, int& Clipped, int nClipLevel)
{
	BSP_DUMP_PRINTF(pBsp, "%s BspBuildSubBspR %08x\n", Spaces(nClipLevel*2), nIndex);




	if(nIndex == OCCLUDER_LEAF || nIndex == OCCLUDER_EMPTY)
	{
		Clipped = nIndex == OCCLUDER_LEAF ? 1 : 0;
		return nIndex;//leave terminating indices.
	}
	ZASSERT(nIndex != OCCLUDER_LEAF);
	ZASSERT(nIndex != OCCLUDER_EMPTY);
	SOccluderBspNode Node = pBsp->Nodes[nIndex];
	v4 vPlane = BspGetPlane(pBsp, Node.nPlaneIndex);
	//if all inside
	ClipPolyResult CR = ECPR_CLIPPED;
	uint16_t nAdded = OCCLUDER_EMPTY;


	uint32 nMaxEdges = (nNumEdges + 1);
	uint16 *ClippedPolyIn = 0;
	uint16 *ClippedPolyOut = 0;
	uint32 nIn = 0;
	uint32 nOut = 0;

	bool bIsLeaf = Node.nInside == OCCLUDER_LEAF;
	ZASSERT(Node.nOutside != OCCLUDER_LEAF);
	if(vPlane.w != 0.f)
	{
		CR = BspClipLeaf(pBsp, vPlane, Poly, nNumEdges);
		ZASSERT(Node.nInside == OCCLUDER_LEAF);
		if(CR & ECPR_OUTSIDE)
		{
			ClippedPolyOut = Poly;
			nOut = nNumEdges;
		}
	}
	else
	{
		ClippedPolyIn = (uint16 *)alloca(nMaxEdges * sizeof(uint16));
		ClippedPolyOut = (uint16 *)alloca(nMaxEdges * sizeof(uint16));
		CR = BspClipPolyCull(pBsp, Node.nPlaneIndex, Poly, nNumEdges, ClippedPolyIn,
							ClippedPolyOut, nIn, nOut);
	}

	switch(CR)
	{
	case ECPR_CLIPPED:
		ZBREAK();
		break;
	case ECPR_INSIDE:
		if(Node.nInside == OCCLUDER_LEAF)
		{
			nAdded = NodeBsp.Nodes.Size();
			{
				SOccluderBspNode* pNode = NodeBsp.Nodes.PushBack();
				SOccluderBspNodeExtra* pNodeExtra = NodeBsp.NodesExtra.PushBack();
				*pNode = pBsp->Nodes[nIndex];
				*pNodeExtra = pBsp->Nodes.NodesExtra[nIndex];
			}
			NodeBsp.Nodes[nAdded].nInside = OCCLUDER_LEAF;
			NodeBsp.Nodes[nAdded].nOutside = OCCLUDER_EMPTY; // will never be reached, if only clipping with sub polys
			Clipped = 1;
		}
		else
		{
			nAdded = BspBuildSubBspR(NodeBsp, pBsp, Node.nInside, ClippedPolyIn, nIn, Clipped, nClipLevel + 1);
		}
		break;
	case ECPR_OUTSIDE:
		ZASSERT(Node.nOutside != OCCLUDER_LEAF);
		if(Node.nInside == OCCLUDER_LEAF)
		{
			//must add self, leaf might occlude subnodes
			nAdded = NodeBsp.Nodes.Size();
			{
				SOccluderBspNode* pNode = NodeBsp.Nodes.PushBack();
				SOccluderBspNodeExtra* pNodeExtra = NodeBsp.NodesExtra.PushBack();
				*pNode = pBsp->Nodes[nIndex];
				*pNodeExtra = pBsp->Nodes.NodesExtra[nIndex];
			}
			NodeBsp.Nodes[nAdded].nInside = OCCLUDER_LEAF;
			NodeBsp.Nodes[nAdded].nOutside = BspBuildSubBspR(NodeBsp, pBsp, Node.nOutside, ClippedPolyOut, nOut, Clipped, nClipLevel + 1);
			// Clipped = 0;
		}
		else
		{
			nAdded = BspBuildSubBspR(NodeBsp, pBsp, Node.nOutside, ClippedPolyOut, nOut, Clipped, nClipLevel + 1);
		}
		break;
	case ECPR_BOTH:
		//both sides, add node. continue with subpolys.

		nAdded = NodeBsp.Nodes.Size();
		{
			SOccluderBspNode* pNode = NodeBsp.Nodes.PushBack();
			SOccluderBspNodeExtra* pNodeExtra = NodeBsp.NodesExtra.PushBack();
			*pNode = pBsp->Nodes[nIndex];
			*pNodeExtra = pBsp->Nodes.NodesExtra[nIndex];
		}
		int ClippedIn = 2, ClippedOut = 2;
		uint16_t nAddedIn  = BspBuildSubBspR(NodeBsp, pBsp, Node.nInside, ClippedPolyIn, nIn, ClippedIn, nClipLevel + 1);
		uint16_t nAddedOut = BspBuildSubBspR(NodeBsp, pBsp, Node.nOutside, ClippedPolyOut, nOut, ClippedOut, nClipLevel + 1);
		//
		if(nAddedIn == nAddedOut)
		{
			ZASSERT(nAddedIn == OCCLUDER_EMPTY);
			ZASSERT(NodeBsp.Nodes.Size() == nAdded+1);
			//both sides end up empty, so discard.
			NodeBsp.Nodes.PopBack();
			NodeBsp.NodesExtra.PopBack();
			int a = 0;
			nAdded = OCCLUDER_EMPTY;
		}
		else
		{
			NodeBsp.Nodes[nAdded].nInside = nAddedIn;
			NodeBsp.Nodes[nAdded].nOutside = nAddedOut;
			ZASSERT(NodeBsp.Nodes[nAdded].nOutside != OCCLUDER_LEAF);
		}
		ZASSERT(ClippedIn < 2);
		ZASSERT(ClippedOut < 2);
		Clipped = (ClippedIn == 1 && ClippedOut == 1) ? 1 : 0;
		break;
	}


	return nAdded;
}

SOccluderBspNodes* BspBuildSubBsp(SOccluderBspNodes& NodeBsp, SOccluderBsp *pBsp, SOccluderDesc *pObject)
{
	MICROPROFILE_SCOPEI("CULLTest", "BuildSub", 0x4488ee);
	// uplotfnxt("************* build sub bsp");
	BSP_DUMP_PRINTF(pBsp, "******* BUILD SUB BSP %p, object %p\n", pBsp, pObject);
	pBsp->Stats.nNumChildBspsCreated++;
	NodeBsp.Nodes.Clear();
	NodeBsp.NodesExtra.Clear();
	if(!pBsp->Nodes.Size())
		return &pBsp->Nodes;
	pBsp->Debug.BspDebugPlaneCounter = 0;
	long seed = rand();
	srand((int)(uintptr)pObject);
	randseed(0xed32babe, 0xdeadf39c);

	uint16_t Poly[5];
	uint32 nSize = pBsp->Planes.Size();
	pBsp->Planes.Resize(nSize + 5); // TODO use thread specific blocks
	v4 *pPlanes = pBsp->Planes.Ptr() + nSize;
	EBPResult BPResult = BspBuildPlanes(pPlanes, Poly, pBsp, pObject, nSize, &NodeBsp.vCorners[0]);
	if(EBUILD_PLANES_INVALID == BPResult)
	{
		pBsp->Stats.nNumChildBspsVisible++;
		return &pBsp->Nodes; // if proper plane can't be built a full test of all objects is needed
	}
	if(pBsp->Debug.OccluderDrawOccluders&2)
	{
		BspDrawPoly(pBsp, Poly, 5, randcolor(), randcolor(), 1);
	}
	int Clipped = 2;
	uint16_t nAdded = BspBuildSubBspR(NodeBsp, pBsp, 0, Poly, 5, Clipped, 0);
	ZASSERT(Clipped < 2);

	pBsp->Planes.Resize(nSize);
	srand(seed);

#if 0
	uprintf("test res is %d, size is %d\n", nAdded != OCCLUDER_EMPTY ? 1 : 0, NodeBsp.Nodes.Size());
	BspDumpNodes(&NodeBsp, 0, 0);
#endif
	BSP_DUMP_PRINTF(pBsp, "/////// BUILD SUB BSP %p, object %p\n", pBsp, pObject);
	if(Clipped)
	{
		return 0;
	}
	else
	{
		pBsp->Stats.nNumChildBspsVisible++;
		return &NodeBsp;
	}
}

void BspSave(SOccluderBsp* pBsp, const char* pFile)
{
	FILE* F = fopen(pFile, "wb");
	ZASSERT(F);
	int w = fwrite(pBsp, sizeof(*pBsp), 1, F);
	ZASSERT(w == 1);
	fclose(F);
}
void BspLoad(SOccluderBsp* pBsp, const char* pFile)
{

	FILE* F = fopen(pFile, "r");
	ZASSERT(F);
	int w = fread(pBsp, sizeof(*pBsp), 1, F);
	ZASSERT(w == 1);
	fclose(F);
}



