#include "base.h"
#include "math.h"
#include "fixedarray.h"
#include "text.h"
#include "program.h"
#include "input.h"
#include "bsp.h"
#include "microprofileinc.h"

// todo:
//  dont add unused/clipped planes
//  prioritize after clipping vs frustum
//  make prioritization use proper area
//  generate test scene
//  seperate objects in test scene
//  basic lighting
//  object colors
//  generate path through test scene
//  rename cull to isvisible
//  optimize!
//

// precision list:
//   normal planes with w near 0 tend to cause precision issues
//   clip with the front plane
//   

#include <algorithm>
#define OCCLUDER_EMPTY (0xc000)
#define OCCLUDER_LEAF (0x8000)
#define OCCLUDER_CLIP_MAX 0x100
#define USE_FRUSTUM 1
#define BSP_ADD_FRONT 1
#define PLANE_TEST_EPSILON 0.0001f
uint32 g_DEBUG = 0;
uint32 g_DEBUG2 = 0;
uint32 g_DEBUGLEVEL = 0;
uint32 g_DEBUGLEVELSTART = 3;
uint32 g_nBspOccluderDebugDrawClipResult = 1;
uint32 g_nBspOccluderDrawEdges = 2;
uint32 g_nBspOccluderDrawOccluders = 1;
uint32 g_nBreakOnClipIndex = -1;
uint32 g_nBspPlaneCap = -1;
uint32 g_nBspDebugPlane = -1;
uint32 g_nBspDebugPlaneCounter = 0;
uint32 g_nRecursiveClipCounter = 0;
uint32 g_nRecursiveClip = -1;
uint32 g_nInsideOutSide = 0x3;
uint32 g_nCheck = 0;
int g_nShowClipLevel = -1;
int g_EdgeMask = 0xff;

uint32 g_nFirst = 0;
struct SOccluderPlane;
struct SBspEdgeIndex;
struct SOccluderBspNode;

struct SOccluderBspNode
{
	uint16 nPlaneIndex;
	uint16 nInside;
	uint16 nOutside;
	bool bLeaf;
	bool bSpecial;
	v3 vDebugCorner;
};

struct SOccluderBsp
{
	static const int MAX_PLANES = 1024 * 4;
	static const int MAX_NODES = 1024;
	static const uint16 PLANE_FLIP_BIT = 0x8000;

	SOccluderBspViewDesc DescOrg;
	m mtobsp;
	m mfrombsp;
	uint32 nDepth;

	v3 vFrustumCorners[4];
	v4 vFrustumPlanes[4];
	TFixedArray<v4, MAX_PLANES> Planes;
	TFixedArray<v3, MAX_PLANES> Corners;
	TFixedArray<SOccluderBspNode, MAX_NODES> Nodes;

	SOccluderBspStats Stats;
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
void BspDrawPoly2(SOccluderBsp *pBsp, uint16 *Poly, uint32 nVertices, uint32 nColorPoly,
				  uint32 nColorEdges, int nIndex);
int BspFindParent(SOccluderBsp *pBsp, int nChildIndex);
int BspFindParent(SOccluderBsp *pBsp, int nChildIndex, bool &bInside);
v3 BspToPlane2(v4 p);
float BspAreaEstimate(v4* vPlanes, uint32 nNumPlanes);
float BspAreaEstimate(SOccluderBsp* pBsp, uint16* pIndices, uint32 nNumPlanes);


uint32_t g_nDrawCount = 0;
uint32_t g_nDebugDrawPoly = 1;

v3 BspPlaneIntersection(v4 p0, v4 p1, v4 p2)
{
	MICROPROFILE_SCOPEIC("BSP", "BspPlaneIntersect");
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
	SOccluderBspNode *pNode = pBsp->Nodes.Ptr() + nNode;
	uprintf("D%s0x%04x: plane[%04x]   [i:%04x,o:%04x] parent %04x\n", Spaces(3 * nLevel), nNode,
			pNode->nPlaneIndex, pNode->nInside, pNode->nOutside, BspFindParent(pBsp, nNode));
	if(0 == (0x8000 & pNode->nInside))
		BspDump(pBsp, pNode->nInside, nLevel + 1);
	else
		uprintf("D%s0x%04x\n", Spaces(3 * (nLevel + 1)), pNode->nInside);

	if(0 == (0x8000 & pNode->nOutside))
		BspDump(pBsp, pNode->nOutside, nLevel + 1);
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

bool BspCanAddNodes(SOccluderBsp* pBsp, uint32 nNumNodes)
{
	if(pBsp->Nodes.Capacity() - pBsp->Nodes.Size() < nNumNodes || pBsp->Nodes.Size() > pBsp->DescOrg.nNodeCap)
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
	TFixedArray<v4, SOccluderBsp::MAX_PLANES> Planes;
	TFixedArray<SBspPotentialPoly, SOccluderBsp::MAX_PLANES> PotentialPolys;

	//debug only stuff
	TFixedArray<v3, SOccluderBsp::MAX_PLANES> Corners;

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
	char buffer[256];
	char foo[100];
	strcpy(buffer, "***FRDIST ");
	bool bSum = false;
	for(uint32 i = 0; i < nNumPlanes0; ++i)
	{
		v4 vPlane = pPlanes0[i];
		bool bTest = BspFrustumOutside(vPlane, pPlanes1, nNumPlanes1);
		bSum = bSum || bTest;

		snprintf(foo, 100, "%d ", bTest ? 1 : 0);
		strcat(buffer, foo);
	}
	for(uint32 i = 0; i < nNumPlanes1; ++i)
	{
		v4 vPlane = pPlanes0[i];
		bool bTest = BspFrustumOutside(vPlane, pPlanes1, nNumPlanes1);
		bSum = bSum || bTest;
		snprintf(foo, 100, "%d ", bTest ? 1 : 0);
		strcat(buffer, foo);
	}
	strcat(buffer, " TOTAL ");
	strcat(buffer, bSum ? "1" : "0");
//	uplotfnxt(buffer);
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
		if(g_nBspOccluderDrawEdges & 1)
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


void BspAddPotentialBoxes(SOccluderBsp* pBsp, SBspPotentialOccluders& PotentialOccluders, SWorldObject *pWorldObjects, uint32 nNumWorldObjects)
{
	v3 vLocalOrigin = v3zero();
	m mtobsp = pBsp->mtobsp;
	for(uint32 i = 0; i < nNumWorldObjects; ++i)
	{
		SWorldObject *pObject = i + pWorldObjects;
		if(pObject->nFlags & SObject::OCCLUDER_BOX)
		{
			m mat = pObject->mObjectToWorld;
			mat = mmult(mtobsp, mat);

			v3 vSize = pObject->vSize;
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

			BspAddPotentialBoxQuad(pBsp, PotentialOccluders, vCenterY, vY, vZ, vSize.z, vX, vSize.x, true);
			BspAddPotentialBoxQuad(pBsp, PotentialOccluders, vCenterX, vX, vY, vSize.y, vZ, vSize.z, true);
			BspAddPotentialBoxQuad(pBsp, PotentialOccluders, vCenterZ, vZ, vY, vSize.y, vX, vSize.x, true);
		}
	}
}

void BspAddPotentialOccluders(SOccluderBsp *pBsp, SBspPotentialOccluders &P, SOccluder *pOccluders,
							  uint32 nNumOccluders)
{
	m mtobsp = pBsp->mtobsp;
	for(uint32 k = 0; k < nNumOccluders; ++k)
	{
		v3 vCorners[4];
		SOccluder Occ = pOccluders[k];
		m mat = mmult(mtobsp, Occ.mObjectToWorld);
		v3 vNormal = mat.z.tov3();
		v3 vUp = mat.y.tov3();
		v3 vLeft = v3cross(vNormal, vUp);
		v3 vCenter = mat.trans.tov3();
		vCorners[0] = vCenter + vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[1] = vCenter - vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[2] = vCenter - vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
		vCorners[3] = vCenter + vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
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

			int nIndex = (int)pBsp->Nodes.Size();
			SOccluderBspNode* pNode = pBsp->Nodes.PushBack();
			pNode->nInside = OCCLUDER_LEAF;
			pNode->nOutside = i == 3 ? OCCLUDER_EMPTY : (i+1);
//			pNode->nOutside = OCCLUDER_EMPTY;
			pNode->bLeaf = false;
			pNode->bSpecial = true;
			pNode->nPlaneIndex = i;
			pNode->vDebugCorner = pBsp->Corners[i];
			//ZASSERT(nIndex == 0);
		//	uplotfnxt("localdir is %f %f %f", vLocalDirection.x,vLocalDirection.y,vLocalDirection.z);
		}
		//pBsp->Planes[0] = v4init(vLocalDirection, 0.1f);


		if(g_nBspOccluderDrawEdges & 2)
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

void BspBuild(SOccluderBsp *pBsp, SOccluder *pOccluderDesc, uint32 nNumOccluders,
			  SWorldObject *pWorldObjects, uint32 nNumWorldObjects,
			  const SOccluderBspViewDesc &Desc)
{
	g_pBsp = pBsp;
	MICROPROFILE_SCOPEIC("BSP", "Build");
	if(g_KeyboardState.keys[SDL_SCANCODE_F3] & BUTTON_RELEASED)
	{
		g_nBspOccluderDrawOccluders = (g_nBspOccluderDrawOccluders + 1) % 2;
	}

	if(g_KeyboardState.keys[SDL_SCANCODE_F4] & BUTTON_RELEASED)
	{
		g_nBspOccluderDebugDrawClipResult = (g_nBspOccluderDebugDrawClipResult + 1) % 2;
	}

	if(g_KeyboardState.keys[SDL_SCANCODE_F5] & BUTTON_RELEASED)
	{
		g_nBspOccluderDrawEdges = (g_nBspOccluderDrawEdges + 1) % 4;
	}
	if(g_KeyboardState.keys[SDL_SCANCODE_V] & BUTTON_RELEASED)
	{
		g_nDebugDrawPoly++;
	}
	if(g_KeyboardState.keys[SDL_SCANCODE_C] & BUTTON_RELEASED)
	{
		g_nDebugDrawPoly--;
	}

	g_nCheck = 0;
	g_nBspDebugPlaneCounter = 0;
	g_nRecursiveClipCounter = 0;
	g_nDrawCount = 0;

	uplotfnxt("OCCLUDER Occluders:(f3)%d Clipresult:(f4)%d Edges:(f5)%d", g_nBspOccluderDrawOccluders,
			  g_nBspOccluderDebugDrawClipResult, g_nBspOccluderDrawEdges);
	if(g_KeyboardState.keys[SDL_SCANCODE_F8] & BUTTON_RELEASED)
		g_nBspDebugPlane++;
	if(g_KeyboardState.keys[SDL_SCANCODE_F7] & BUTTON_RELEASED)
		g_nBspDebugPlane--;
	if((int)g_nBspDebugPlane >= 0)
		uplotfnxt("BSP DEBUG PLANE %d", g_nBspDebugPlane);

	if(g_KeyboardState.keys[SDL_SCANCODE_M] & BUTTON_RELEASED)
		g_nRecursiveClip++;
	if(g_KeyboardState.keys[SDL_SCANCODE_N] & BUTTON_RELEASED)
		g_nRecursiveClip--;
	if((int)g_nRecursiveClip>= 0)
		uplotfnxt("BSP RECURSIVE CLIP DEBUG %d", g_nRecursiveClip);

	if(g_KeyboardState.keys[SDL_SCANCODE_END] & BUTTON_RELEASED)
		g_nInsideOutSide = (g_nInsideOutSide%3)+1;
	uplotfnxt("INSIDEOUTSIDE %x", g_nInsideOutSide);
	


	if(g_KeyboardState.keys[SDL_SCANCODE_F5] & BUTTON_RELEASED)
		g_nShowClipLevel++;
	if(g_KeyboardState.keys[SDL_SCANCODE_F6] & BUTTON_RELEASED)
		g_nShowClipLevel--;
	if((int)g_nShowClipLevel >= 0)
		uplotfnxt("BSP Debug clip level %d", g_nShowClipLevel);

	long seed = rand();
	srand(42);

	pBsp->nDepth = 0;
	pBsp->Nodes.Clear();
	pBsp->Planes.Clear();
	pBsp->Corners.Clear();
	memset(&pBsp->Stats, 0, sizeof(pBsp->Stats));
	g_nFirst = 0;
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
	// uprintf("DISTANCE IS %f... %f\n", fDist, fDist1);
	if(abs(fDist) > 1e-3f || fDist1 > 1e-3f)
	{
		ZBREAK();
	}

	SBspPotentialOccluders P;

	BspAddFrustum(pBsp, P, vLocalDirection, vLocalRight, vLocalUp, vLocalOrigin);
	BspAddPotentialBoxes(pBsp, P, pWorldObjects, nNumWorldObjects);
	BspAddPotentialOccluders(pBsp, P, pOccluderDesc, nNumOccluders);


	pBsp->Stats.nNumPlanesConsidered = 0;
	for(uint32 i = 0; i < P.PotentialPolys.Size(); ++i)
	{
		uint16 nIndex = P.PotentialPolys[i].nIndex;
		uint16 nCount = P.PotentialPolys[i].nCount;
		pBsp->Stats.nNumPlanesConsidered += nCount;
		P.PotentialPolys[i].fArea = BspAreaEstimate(P.Planes.Ptr() + nIndex, nCount);
	}
	if(P.PotentialPolys.Size())
	{
		std::sort(&P.PotentialPolys[0], P.PotentialPolys.Size() + &P.PotentialPolys[0],
			[](const SBspPotentialPoly& l, const SBspPotentialPoly& r) -> bool
			{
				return l.fArea > r.fArea;
			}
		);
	}
	{
		v4* vPlanes = P.Planes.Ptr();
		v3* vCorners = P.Corners.Ptr();
		for(uint32 i = 0; i < P.PotentialPolys.Size(); ++i)
		{
			//uplotfnxt("pp %d .. cap %d", i, g_nBspPlaneCap);
			if(i > g_nBspPlaneCap)
				break;
			if(i == g_nBspPlaneCap)
			{
				g_DEBUG2 = 1;
				g_DEBUGLEVEL = g_DEBUGLEVELSTART;
				//ZBREAK();

//uint32 g_DEBUGLEVELSTART = 1;

			}
			uint16 nIndex = P.PotentialPolys[i].nIndex;
			uint16 nCount = P.PotentialPolys[i].nCount;
			if(BspCanAddNodes(pBsp, nCount))
			{
				float fMin = 100.f;
				BspAddOccluderInternal(pBsp, nIndex + vPlanes, nIndex + vCorners, nCount);
			}
			g_DEBUG2 = 0;
			g_DEBUGLEVEL = -1;

		}
	}
	pBsp->Stats.nNumNodes = pBsp->Nodes.Size();
	pBsp->Stats.nNumPlanes = pBsp->Planes.Size();




	g_nCheck = 1;
	if(g_KeyboardState.keys[SDL_SCANCODE_F10] & BUTTON_RELEASED)
	{
		BspDump(pBsp, 0, 0);
	}

	uplotfnxt("BSP: NODES[%03d] PLANES[%03d] DEPTH[%03d]", pBsp->Nodes.Size(), pBsp->Planes.Size(),
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
		uplotfnxt("REJECT %f", fMinTest);
		return;
	}

	//if(fLen2 > (LIM*LIM))
	//reject if too close to origin.
	if(fabs(vNormalPlane.w) < 0.1f)
	{
		uplotfnxt("REJECT %f", vNormalPlane.w);
		return;
	}
	uint32 nPlaneIndex = pBsp->Planes.Size();
	pBsp->Planes.PushBack(pPlanes, nNumPlanes);
	pBsp->Corners.PushBack(pCorners, nNumPlanes);
	uint32 nNodesSize = pBsp->Nodes.Size();
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

	uint32 nNumNodes = (uint32)pBsp->Nodes.Size();
	if(pBsp->Nodes.Empty())
	{
		uint16 nIndex = (uint16)BspAddInternal(pBsp, -1, true, nPlaneIndices, nNumPlanes, 0, 0);
		ZASSERT(nIndex == 0);
	}
	else
	{
		BspAddRecursive(pBsp, 0, nPlaneIndices, nNumPlanes, 0, 0);
	}
	if(pBsp->Nodes.Size() == nNodesSize)
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
int BspFindParent(SOccluderBsp *pBsp, int nChildIndex)
{
	for(uint32 i = 0; i < pBsp->Nodes.Size(); ++i)
	{
		if(pBsp->Nodes[i].nInside == nChildIndex || pBsp->Nodes[i].nOutside == nChildIndex)
			return i;
	}
	return -1;
}

int BspFindParent(SOccluderBsp *pBsp, int nChildIndex, bool &bInside)
{
	for(uint32 i = 0; i < pBsp->Nodes.Size(); ++i)
	{
		if(pBsp->Nodes[i].nInside == nChildIndex)
		{
			bInside = true;
			return i;
		}

		if(pBsp->Nodes[i].nOutside == nChildIndex)
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

	// v4 ptest = v4init(p.tov3() * -p.w, 1.f);
	// float fDot = v4dot(ptest, p);
	// float fAbs = abs(fDot);
	// ZASSERT(fAbs < 0.001f);
	// p = mtransform(g_TOBSP__, p);

	// ptest = v4init(p.tov3() * -p.w, 1.f);
	// fDot = v4dot(ptest, p);
	// fAbs = abs(fDot);
	// ZASSERT(fAbs < 0.001f);

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

bool BspPlaneTest(v4 plane0, v4 plane1, v4 ptest, bool bDebug = false)
{
	return BspPlaneTestNew(plane0, plane1, ptest) > 0.f;
	bool bParrallel;
	v3 p2d0 = BspToPlane2(plane0);
	v3 p2d1 = BspToPlane2(plane1);
	v3 p2dtest = BspToPlane2(ptest);

	v2 p1 = p2d0.tov2() * -p2d0.z;
	v2 p2 = v2hat(p2d0.tov2()) + p1;
	v2 p3 = p2d1.tov2() * -p2d1.z;
	v2 p4 = v2hat(p2d1.tov2()) + p3;

	float fDot1 = v3dot(v3init(p1, 1.f), p2d0);
	if(fabs(fDot1) > 0.0001f)
		ZBREAK();
	float fDot2 = v3dot(v3init(p2, 1.f), p2d0);
	if(fabs(fDot2) > 0.0001f)
		ZBREAK();
	float fDot3 = v3dot(v3init(p3, 1.f), p2d1);
	if(fabs(fDot3) > 0.0001f)
		ZBREAK();
	float fDot4 = v3dot(v3init(p4, 1.f), p2d1);
	if(fabs(fDot4) > 0.0001f)
		ZBREAK();

	float t0 = p1.x * p2.y - p1.y * p2.x;
	float t1 = p3.x * p4.y - p3.y * p4.x;
	float x1x2 = p1.x - p2.x;
	float x3x4 = p3.x - p4.x;
	float y1y2 = p1.y - p2.y;
	float y3y4 = p3.y - p4.y;

	float numx = t0 * x3x4 - x1x2 * t1;
	float numy = t0 * y3y4 - y1y2 * t1;
	float denom = x1x2 * y3y4 - y1y2 * x3x4;

	if(bDebug)
	{
	}

	// dot((nx/d, ny/d, 1), (px,py,pz) < 0;
	// 1/d * dot((nx,ny,d), (px,py,pz) < 0;
	// -- since we only compare to 0
	// d * dot((nx,ny, d), (px,py,pz) < 0;
	float fDot = v3dot(v3init(numx, numy, denom), p2dtest);
	bool bRet1 = denom * fDot >= 0.f;
	// bool bRet2 = v3dot( v3init(numx, numy, denom), p2dtest) <= 0.f;
	// if(denom < 0.f)
	// 	bRet2 = !bRet2;
	// ZASSERT(0.f == denom || bRet1 == bRet2);
	bParrallel = fDot * denom == 0.f;
	if(bDebug)
	{
		uplotfnxt("plane test dot is %f --> %d par %d .. unsigned %f .. %f\n", denom * fDot, bRet1,
				  bParrallel, denom, fDot);
		uplotfnxt("Plane test Ned %f", BspPlaneTestNew(plane0, plane1, ptest));
	}

	return bRet1 || bParrallel;
}


int BspAddInternal(SOccluderBsp *pBsp, uint16 nParent, bool bOutsideParent, uint16 *pIndices,
				   uint16 nNumPlanes, uint32 nExcludeMask, uint32 nLevel)
{
	if(!BspCanAddNodes(pBsp, nNumPlanes))
		return -1;
	ZASSERT(nNumPlanes > 1);
	int nCount = 0;
	uint32 nBaseExcludeMask = nExcludeMask;
	int nNewIndex = (int)pBsp->Nodes.Size();
	int nPrev = -1;
	if(nLevel + nNumPlanes > pBsp->nDepth)
		pBsp->nDepth = nLevel + nNumPlanes;
	if(g_DEBUG2 && g_DEBUGLEVEL == 0)
	{
		uprintf("ADD INTERNAL\n");
		for(uint32 i = 0; i < nNumPlanes; ++i)
		{
			v4 p0 = BspGetPlane(pBsp, pIndices[i]);
			uprintf("p%d: %12.10f %12.10f %12.10f %12.10f\n", i, p0.x, p0.y, p0.z, p0.w);
		}
		if(nNumPlanes>3)
		{			
			uint32 nCount = nNumPlanes-1;
			for(uint32 i = 0; i < nCount; ++i)
			{
				v4 p0 = BspGetPlane(pBsp, pIndices[i]);
				v4 p1 = BspGetPlane(pBsp, pIndices[(i+1)%nCount]);
				v4 p2 = BspGetPlane(pBsp, pIndices[(i+2)%nCount]);
				uprintf("%f ", BspPlaneTestNew(p0, p1, p2));
			}
		}
		uprintf("\n");
	}

	for(uint32 i = 0; i < nNumPlanes; ++i)
	{
		if(0 == (nExcludeMask & 1))
		{
			nCount++;
			int nIndex = (int)pBsp->Nodes.Size();
			SOccluderBspNode *pNode = pBsp->Nodes.PushBack();
			pNode->nOutside = OCCLUDER_EMPTY;
			pNode->nInside = OCCLUDER_LEAF;
			pNode->bLeaf = i == nNumPlanes - 1;
			pNode->nPlaneIndex = pIndices[i];
			pNode->bSpecial = false;
			if(nPrev >= 0)
				pBsp->Nodes[nPrev].nInside = nIndex;
			nPrev = nIndex;
			pNode->vDebugCorner = pBsp->Corners[pIndices[i]];

			if(g_nBspDebugPlane == g_nBspDebugPlaneCounter++ ||
				g_DEBUGLEVEL == 0

				)
			{
				v4 vPlane = BspGetPlane(pBsp, pIndices[i]);
				v3 vCorner = pNode->vDebugCorner;
				vCorner = mtransform(pBsp->mfrombsp, vCorner);
				vPlane = v4init(mrotate(pBsp->mfrombsp, vPlane.tov3()), 1.f);
				ZDEBUG_DRAWPLANE(vPlane.tov3(), vCorner, 0xff00ff00);
			}
		}
		nExcludeMask >>= 1;
	}

	if(nParent < pBsp->Nodes.Size())
	{
		if(bOutsideParent)
			pBsp->Nodes[nParent].nOutside = nNewIndex;
		else
			pBsp->Nodes[nParent].nInside = nNewIndex;
	}
	if(g_DEBUGLEVEL-- == 0)
	{
		// if(nNumPlanes>6)
		// {
		// 	uint16 lala[32];
		// 	memcpy(lala, pIndices, nNumPlanes*sizeof(uint16));
		// 	lala[3] = lala[6];
		// 	lala[4] = lala[6];
		// 	//lala[5] = lala[6];
		// 	//BspDrawPoly(pBsp, pIndices, nNumPlanes, randcolor(), randcolor(), 1);
		// 	BspDrawPoly(pBsp, lala, 4, randcolor(), randcolor(), 1);
		// }
		BspDrawPoly(pBsp, pIndices, nNumPlanes, randcolor(), randcolor(), 1);
	}
	// else if((int)g_nRecursiveClip < 0 && 0 == g_DEBUG2)
	if(g_nBspPlaneCap == (uint32)-1)
		BspDrawPoly(pBsp, pIndices, nNumPlanes, randcolor(), randcolor(), 1);
	return nNewIndex;
}
#define OCCLUDER_POLY_MAX 12
enum ClipPolyResult
{
	ECPR_CLIPPED = 0,
	ECPR_INSIDE = 0x1,
	ECPR_OUTSIDE = 0x2,
	ECPR_BOTH = 0x3,
};

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

// int BspReducePoly(SOccluderBsp *pBsp, uint16 *pPoly, uint32 nNumIndices, uint32 &nExcludeMask)
// {
// 	if(!nNumIndices)
// 		return 0;
// 	ZBREAK();
// 	uint16 *pPolyCopy = (uint16 *)alloca(sizeof(uint16) * nNumIndices);
// 	memcpy(pPolyCopy, pPoly, nNumIndices * sizeof(uint16));
// 	uint32 nMask = nExcludeMask;
// 	uint32 nExcludeMaskOut = 0;
// 	uint32 nNumOut = 0;
// 	uint32 nNumPlanes = nNumIndices - 1;
// 	for(int i = 0; i < nNumPlanes; ++i)
// 	{
// 		int i0 = (i + nNumPlanes - 1) % nNumPlanes;
// 		int i1 = (i + 1) % nNumPlanes;
// 		v4 p0 = BspGetPlane(pBsp, pPolyCopy[i0]);
// 		v4 p = BspGetPlane(pBsp, pPolyCopy[i]);
// 		v4 p1 = BspGetPlane(pBsp, pPolyCopy[i1]);
// 		float fTest = BspPlaneTestNew(p0, p1, p);
// 		uplotfnxt("test %d::%f", i, fTest);
// 		if(fTest > 0.0001f)
// 		{
// 			pPoly[nNumOut] = i;
// 			nExcludeMaskOut |= (1 & nMask) ? (1 << nNumOut) : 0;
// 			nNumOut++;
// 		}
// 		nMask >>= 1;
// 	}
// 	nExcludeMask = nExcludeMaskOut;
// 	return nNumOut;
// }

ClipPolyResult BspClipPoly(SOccluderBsp *pBsp, uint16 nClipPlane, uint16 *pIndices,
						   uint16 nNumPlanes, uint32 nExcludeMask, uint16 *pPolyIn,
						   uint16 *pPolyOut, uint32 &nEdgesIn_, uint32 &nEdgesOut_,
						   uint32 &nMaskIn_, uint32 &nMaskOut_)
{
	ZASSERT(nNumPlanes < 32);
	uint32 nNumEdges = nNumPlanes - 1;
	v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumEdges]);

	ZASSERT(nNumEdges > 2);
	bool *bCorners = (bool *)alloca(nNumPlanes - 1);
	bool *bCorners0 = (bool *)alloca(nNumPlanes - 1);
	v4 *vPlanes = (v4 *)alloca(sizeof(v4) * (nNumPlanes));
	v4 vClipPlane = BspGetPlane(pBsp, nClipPlane);
	if(g_DEBUG)
	{
		v3 dir = mrotate(pBsp->mfrombsp, vClipPlane.tov3());
		//ZDEBUG_DRAWPLANE(dir, mtransform(pBsp->mfrombsp, pBsp->Corners[nClipPlane]), -1);
	}
	// uprintf("poly ");
	for(int i = 0; i < nNumPlanes; ++i)
	{
		// uprintf("%d ", pIndices[i]);
		vPlanes[i] = BspGetPlane(pBsp, pIndices[i]);
	}
	//	uprintf("\n");
	bool bDebug = 0 == g_nFirst++;
	for(int i = 0; i < nNumEdges; ++i)
	{
		v4 p0 = vPlanes[i];
		v4 p1 = vPlanes[(i + 1) % nNumEdges];
		bCorners[i] = BspPlaneTestNew(p0, p1, vClipPlane) < PLANE_TEST_EPSILON;
		bCorners0[i] = BspPlaneTestNew(p0, p1, vClipPlane) < -PLANE_TEST_EPSILON;
	}

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
		bool bCorner0 = bCorners0[(i + nNumEdges - 1) % nNumEdges];
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
					if(nMask & 1)
						nMaskOut |= nMaskRollOut;
					nMaskRollOut <<= 1;

					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
					nMaskOut |= nMaskRollOut;
					nMaskRollOut <<= 1;
				}
				else // inside --> outside
				{
					pPolyOut[nEdgesOut++] = nClipPlane ^ SOccluderBsp::PLANE_FLIP_BIT;
					nMaskOut |= nMaskRollOut;
					nMaskRollOut <<= 1;

					pPolyOut[nEdgesOut++] = pIndices[i];
					if(nMask & 1)
						nMaskOut |= nMaskRollOut;
					nMaskRollOut <<= 1;
				}
			}
			else
			{
				pPolyOut[nEdgesOut++] = pIndices[i];
				if(nMask & 1)
					nMaskOut |= nMaskRollOut;
				nMaskRollOut <<= 1;
			}
		}
		else if(bCorner0) // both outside
		{
			pPolyOut[nEdgesOut++] = pIndices[i];
			if(nMask & 1)
				nMaskOut |= nMaskRollOut;
			nMaskRollOut <<= 1;
		}
		else // both insides
		{
		}
		nMask >>= 1;
	}


	// for(int i = 0; i < nNumEdges; ++i)
	// {
	// 	v4 p0 = BspGetPlane(pBsp, pIndices[i]);
	// 	v4 p1 = BspGetPlane(pBsp, pIndices[(i+1)%nNumEdges]);
	// 	v4 p2 = BspGetPlane(pBsp, pIndices[(i+2)%nNumEdges]);
	// 	if(pIndices[i] < pBsp->Corners.Size() && g_DEBUG)
	// 	{
	// 		v3 dir = mrotate(pBsp->mfrombsp, p0.tov3());
	// 		v3 c0 = v3init(0.3, 0.3, 0.3);
	// 		v3 c1 = v3init(0.0, 1.0, 0.0);
	// 		float fLerp = (float)i/(float)(nEdgesOut-1);
	// 		v3 c = v3lerp(c0, c1, fLerp);
	// 		ZDEBUG_DRAWPLANE(dir, mtransform(pBsp->mfrombsp, pBsp->Corners[pIndices[i]]), c.tocolor());
	// 		float fSignTest = BspPlaneTestNew(p0,p1,p2);
	// 		uplotfnxt("SIGN TEST IS %f", fSignTest);
	// 		if(fSignTest<0)
	// 			uprintf("negative\n");//ZBREAK();
	// 	}
	// }


	// //sanity out
	// for(int i = 0; i < nEdgesOut; ++i)
	// {
	// 	v4 p0 = BspGetPlane(pBsp, pPolyOut[i]);
	// 	v4 p1 = BspGetPlane(pBsp, pPolyOut[(i+1)%nEdgesOut]);
	// 	// if(pPolyOut[i] < pBsp->Corners.Size() && g_DEBUG)
	// 	// {
	// 	// 	v3 dir = mrotate(pBsp->mfrombsp, p0.tov3());
	// 	// 	v3 c0 = v3init(0.3, 0.3, 0.3);
	// 	// 	v3 c1 = v3init(0.0, 1.0, 0.0);
	// 	// 	float fLerp = (float)i/(float)(nEdgesOut-1);
	// 	// 	v3 c = v3lerp(c0, c1, fLerp);
	// 	// 	ZDEBUG_DRAWPLANE(dir, mtransform(pBsp->mfrombsp, pBsp->Corners[pPolyOut[i]]), c.tocolor());
	// 	// }

	// 	float fTest = BspPlaneTestNew(p0, p1, vClipPlane);
	// 	//float fTest0 = BspPlaneTestNew(p1, p0, vClipPlane);
	// 	bool bTest = BspPlaneTest1(p0, p1, vClipPlane);
	// 	v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
	// 	float fDist = v4dot(vClipPlane, v4init(vIntersect, 1.f));
	// 	uplotfnxt("SAN out %6.2f %d dot %f ", fTest, bTest?1:0, fDist);
	// }

	nMask = nExcludeMask;
	for(int i = 0; i < nNumEdges; ++i)
	{
		bool bCorner0 = bCorners[(i + nNumEdges - 1) % nNumEdges];
		bool bCorner1 = bCorners[i];
		if(bCorner0 != bCorner1)
		{
			// add to both
			if(!nClipAddedOut)
			{
				nClipAddedOut = 1;
				if(bCorner0) // outside --> inside
				{
					pPolyIn[nEdgesIn++] = nClipPlane;
					nMaskIn |= nMaskRollIn;
					nMaskRollIn <<= 1;

					pPolyIn[nEdgesIn++] = pIndices[i];
					if(nMask & 1)
						nMaskIn |= nMaskRollIn;
					nMaskRollIn <<= 1;
				}
				else // inside --> outside
				{
					pPolyIn[nEdgesIn++] = pIndices[i];
					if(nMask & 1)
						nMaskIn |= nMaskRollIn;
					nMaskRollIn <<= 1;

					pPolyIn[nEdgesIn++] = nClipPlane;
					nMaskIn |= nMaskRollIn;
					nMaskRollIn <<= 1;
				}
			}
			else
			{
				pPolyIn[nEdgesIn++] = pIndices[i];
				if(nMask & 1)
					nMaskIn |= nMaskRollIn;
				nMaskRollIn <<= 1;
			}
		}
		else if(bCorner0) // both outside
		{
		}
		else // both insides
		{
			pPolyIn[nEdgesIn++] = pIndices[i];
			if(nMask & 1)
				nMaskIn |= nMaskRollIn;
			// nMaskIn |= (nMaskRollIn) & ((int)nMask)<<31)>>31);
			nMaskRollIn <<= 1;
		}
		nMask >>= 1;
	}
	// for(int i = 0; i < nEdgesIn; ++i)
	// {
	// 	v4 p0 = BspGetPlane(pBsp, pPolyIn[i]);
	// 	v4 p1 = BspGetPlane(pBsp, pPolyIn[(i+1)%nEdgesIn]);
	// 	float fTest = BspPlaneTestNew(p0, p1, vClipPlane);
	// 	float fTest0 = BspPlaneTestNew(p1, p0, vClipPlane);

	// 	v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
	// 	float fDist = v4dot(vClipPlane, v4init(vIntersect, 1.f));
	// 	uplotfnxt("SAN in %6.2f %6.2f dot %f ", fTest, fTest0, fDist);
	// }
	// {
	// 	plotlist("INPUT: ", pIndices, nNumPlanes);
	// 	plotlist("   IN: ", pPolyIn, nEdgesIn);
	// 	plotlist("  OUT: ", pPolyOut, nEdgesOut);
	// }



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

	return ClipPolyResult((nEdgesIn ? 0x1 : 0) | (nEdgesOut ? 0x2 : 0));
}

void BspAddRecursive(SOccluderBsp *pBsp, uint32 nBspIndex, uint16 *pIndices, uint16 nNumIndices,
					 uint32 nExcludeMask, uint32 nLevel)
{
	SOccluderBspNode Node = pBsp->Nodes[nBspIndex];

	for(int i = 0; i < nNumIndices-1; ++i)
	{
		v4 p0 = BspGetPlane(pBsp, pIndices[i]);
		v4 p1 = BspGetPlane(pBsp, pIndices[(i+1)%(nNumIndices-1)]);
		v4 p2 = BspGetPlane(pBsp, pIndices[(i+2)%(nNumIndices-1)]);
		if(pIndices[i] < pBsp->Corners.Size() && g_DEBUG)
		{
			float fSignTest = BspPlaneTestNew(p0,p1,p2);
			uplotfnxt("SIGN TEST IS %f", fSignTest);
			if(fSignTest<0)
				ZBREAK();
		}
	}





	if(g_nRecursiveClipCounter == g_nRecursiveClip)
	{
		//uprintf("lala\n");
		g_DEBUG = 1;
	}

	v4 vPlane = BspGetPlane(pBsp, Node.nPlaneIndex);
	char pTest[256] = "Test ";
	bool bPlaneOverlap = false;
	for(int i = 0; i < nNumIndices - 1; ++i)
	{
		if(0 == (nExcludeMask & (1 << i)))
		{
			v4 vInner = BspGetPlane(pBsp, pIndices[i]);
			float fDot0 = v4distance(vInner, vPlane);
			float fDot1 = v4distance(v4neg(vInner), vPlane);
			if(fDot0 < 0.001f || fDot1 < 0.001f)
			{
				nExcludeMask |= (1 << i);
				bPlaneOverlap = true;
				// uplotfnxt("PLANE OVERLAP [%5.2f,%5.2f,%5.2f]-[%5.2f,%5.2f,%5.2f]", vInner.x,
				// 		  vInner.y, vInner.z, vPlane.x, vPlane.y, vPlane.z);
			}
			char buff[64];
			snprintf(buff, 64, "[%f,%f] ", fDot0, fDot1);
			strcat(pTest, buff);
		}
	}
	// uplotfnxt(pTest);

	uint16 ClippedPolyIn[OCCLUDER_POLY_MAX];
	uint16 ClippedPolyOut[OCCLUDER_POLY_MAX];

	// ClipPolyResult CR = BspClipPoly(pBsp, Node.nPlaneIndex, pIndices, nNumIndices, nExcludeMask,
	// &ClippedPolyIn[0], &ClippedPolyOut[0], nNumEdgeIn, nNumEdgeOut, nExcludeMaskIn,
	// nExcludeMaskOut);

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
			// todo test
			if(bOk)
			{
				//uplotfnxt("!!!!!!!!!!!!!!!!!LEAF_ADD!!!!!!!!!!!!!!!!!");

				pPolyOutside = pIndices;
				nPolyEdgeOut = nNumIndices;
				nExcludeMaskOut = nExcludeMask;
				CR = ECPR_OUTSIDE;
			}
		}
	}
	else if(bPlaneOverlap)
	{
//		uplotfnxt("!!!!!!!!!!!!!!!!!PLANEOVERLAP!!!!!!!!!!!!!!!!!");
		uint32 nNumEdgeIn = 0, nNumEdgeOut = 0;
		// uint32 nExcludeMaskIn_ = 0;
		// uint32 nExcludeMaskOut_ = 0;
		int nA = 0;
		int nB = 0;

		// ZASSERT(vPlane.w != 0.f);
		v4 vNormalPlane = BspGetPlane(pBsp, pIndices[nNumIndices - 1]);
		for(int i = 0; i < nNumIndices - 1; ++i)
		{
			v4 p0 = BspGetPlane(pBsp, pIndices[i]);
			v4 p1 = BspGetPlane(pBsp, pIndices[(i + 1) % (nNumIndices - 1)]);
			v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
			float fDot = v4dot(v4init(vIntersect, 1.f), vPlane);
			//			uplotfnxt("VISIBLE dot %f", fDot);
			if(fDot < -0.001f)
				nA++;
			if(fDot > 0.001f)
				nB++;
			// bOk = bOk || fDot < 0.f;
		}

		if(0)
		{
			CR = BspClipPoly(pBsp, Node.nPlaneIndex, pIndices, nNumIndices, nExcludeMask,
							 &ClippedPolyIn[0], &ClippedPolyOut[0], nNumEdgeIn, nNumEdgeOut,
							 nExcludeMaskIn, nExcludeMaskOut);
			pPolyInside = &ClippedPolyIn[0];
			nPolyEdgeIn = nNumEdgeIn;
			pPolyOutside = &ClippedPolyOut[0];
			nPolyEdgeOut = nNumEdgeOut;
			// uplotfnxt("OVERLAP COUNT in %d .. out %d .. mask %08x %08x %d %d", nNumEdgeIn,
			// 		  nNumEdgeOut, nExcludeMaskIn, nExcludeMaskOut, nA, nB);
		}
		else
		{
			//uplotfnxt("A %d B %d", nA, nB);
			if(nA > nB)
			{
				pPolyOutside = pIndices;
				nPolyEdgeOut = nNumIndices;
				nExcludeMaskOut = nExcludeMask;
				CR = ECPR_OUTSIDE;
			}
			else if(nB)
			{
				pPolyInside = pIndices;
				nPolyEdgeIn = nNumIndices;
				nExcludeMaskIn = nExcludeMask;
				CR = ECPR_INSIDE;
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
	float fArea0 = BspPolygonArea(pBsp, pIndices, nNumIndices);
	float fArea1 = BspPolygonArea(pBsp, pPolyInside, nPolyEdgeIn);
	float fArea2 = BspPolygonArea(pBsp, pPolyOutside, nPolyEdgeOut);
	bool bOk = fArea0 >= fArea1 && fArea0 >= fArea2;
	if(!bOk)
	{
		uint32 nNumEdgeIn = 0, nNumEdgeOut = 0;


		CR = BspClipPoly(pBsp, Node.nPlaneIndex, pIndices, nNumIndices, nExcludeMask,
						 &ClippedPolyIn[0], &ClippedPolyOut[0], nNumEdgeIn, nNumEdgeOut,
						 nExcludeMaskIn, nExcludeMaskOut);
		pPolyInside = &ClippedPolyIn[0];
		nPolyEdgeIn = nNumEdgeIn;
		pPolyOutside = &ClippedPolyOut[0];
		nPolyEdgeOut = nNumEdgeOut;


	}
	if(!bOk);//uplotfnxt("%s AREA %7.4f %7.4f %7.4f", bOk ? "  " : "**", fArea0, fArea1, fArea2);
	ZASSERT(Node.nInside != OCCLUDER_EMPTY);
	ZASSERT(Node.nOutside != OCCLUDER_LEAF);
	//uplotfnxt("ADD p[%d] IN[%d] OUT[%d]", nBspIndex, nPolyEdgeIn, nPolyEdgeOut);

	//	int BspReducePoly(SOccluderBsp *pBsp, uint16 *pPoly, uint32 nNumIndices, uint32
	//&nExcludeMask)
	// nPolyEdgeIn = BspReducePoly(pBsp, pPolyInside, nPolyEdgeIn, nExcludeMaskIn);
	g_DEBUG = 0;
	if(g_nRecursiveClipCounter++ == g_nRecursiveClip)
	{
		uplotfnxt("DRAWING %d.. in %d out %d", g_nRecursiveClip, nPolyEdgeIn, nPolyEdgeOut);
		BspDrawPoly(pBsp, pPolyInside, nPolyEdgeIn, randredcolor(), randredcolor(), 1);
		BspDrawPoly(pBsp, pPolyOutside, nPolyEdgeOut, randcolor(), randcolor(), 1);
	}
	else
	{
		if((ECPR_INSIDE & CR) != 0 && (g_nInsideOutSide&1))
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

		if((ECPR_OUTSIDE & CR) != 0&& (g_nInsideOutSide&2))
		{
			if(Node.nOutside == OCCLUDER_EMPTY)
			{
				// uplotfnxt("ADD OUTSIDE %d", nPolyEdgeOut);
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

///// TODO TEST MED KUN 1 NIVEAU

bool BspCullObjectR(SOccluderBsp *pBsp, uint32 Index, uint16 *Poly, uint32 nNumEdges,
					int nClipLevel)
{
	// if(g_nBspDebugPlane == g_nBspDebugPlaneCounter)
	// {
	// 	BspDrawPoly(pBsp, Poly, nNumEdges, 0xff000000|randredcolor(), 0, 1);
	// 	if(g_nBreakOnClipIndex && g_nBspDebugPlane == g_nBspDebugPlaneCounter)
	// 	{
	// 		//uprintf("BREAK\n");
	// 	}
	// }
	//	g_nBspDebugPlaneCounter++;

	SOccluderBspNode Node = pBsp->Nodes[Index];
	v4 vPlane = BspGetPlane(pBsp, Node.nPlaneIndex);
	if(Node.nInside == OCCLUDER_LEAF && vPlane.w != 0.f)
	{
		ZASSERT(vPlane.w != 0.f);
		// v3 BspPlaneIntersection(v4 p0, v4 p1, v4 p2)
		v4 vNormalPlane = BspGetPlane(pBsp, Poly[nNumEdges - 1]);
		bool bVisible = false;
		bool bCulled = true;
		for(int i = 0; i < nNumEdges - 1; ++i)
		{
			v4 p0 = BspGetPlane(pBsp, Poly[i]);
			v4 p1 = BspGetPlane(pBsp, Poly[(i + 1) % (nNumEdges - 1)]);
			v3 vIntersect = BspPlaneIntersection(p0, p1, vNormalPlane);
			float fDot = v4dot(v4init(vIntersect, 1.f), vPlane);
//			uplotfnxt("VISIBLE dot %f", fDot);
			bVisible = bVisible || fDot < 0.f;
			bCulled = bCulled && fDot < 0.f;
		}
		if(bVisible)
		//if(!bCulled)
		{
			if(g_nShowClipLevel < 0)
				BspDrawPoly(pBsp, Poly, nNumEdges, 0xff000000 | randredcolor(), 0, 1);
		}
		return !bVisible;
		//return bCulled;
	}
	else
	{
		ZASSERT(vPlane.w == 0.f);
	}
	uint32 nMaxEdges = (nNumEdges + 1);
	uint16 *ClippedPolyIn = (uint16 *)alloca(nMaxEdges * sizeof(uint16));
	uint16 *ClippedPolyOut = (uint16 *)alloca(nMaxEdges * sizeof(uint16));
	uint32 nIn = 0;
	uint32 nOut = 0;
	uint32 nExclusionIn = 0, nExclusionOut = 0;

	// ClipPolyResult BspClipPoly(SOccluderBsp* pBsp, uint16 nClipPlane, uint16* pIndices,
	// 	uint16 nNumPlanes, uint32 nExcludeMask, uint16* pPolyIn, uint16* pPolyOut,
	// 	uint32& nEdgesIn_, uint32& nEdgesOut_, uint32& nMaskIn_, uint32& nMaskOut_)

	ClipPolyResult CR = BspClipPoly(pBsp, Node.nPlaneIndex, Poly, nNumEdges, 0, ClippedPolyIn,
									ClippedPolyOut, nIn, nOut, nExclusionIn, nExclusionOut);

	// ClippedPolyIn, ClippedPolyOut,
	// nIn, nOut,
	// nExclusionIn, nExclusionOut);
	bool bFail = false;
	if(g_nShowClipLevel == nClipLevel)
	{
		{
			uplotfnxt("debug drawing %d.. child in %d out %d", Index, Node.nInside, Node.nOutside);
			if(nOut)
				BspDrawPoly(pBsp, ClippedPolyOut, nOut, 0xff00ff00, 0, 1);
			if(nIn)
				BspDrawPoly(pBsp, ClippedPolyIn, nIn, 0xffffffff, 0, 1);
			return false;
		}
	}
	if(CR & ECPR_INSIDE)
	{
		ZASSERT(Node.nInside != OCCLUDER_EMPTY);
		if(Node.nInside == OCCLUDER_LEAF)
		{
			//			ZASSERT(Node.nEdge == 4);
			// culled
		}
		else if(!BspCullObjectR(pBsp, Node.nInside, ClippedPolyIn, nIn, nClipLevel + 1))
		{
			if(!g_nBspOccluderDebugDrawClipResult)
				return false;
			bFail = true;
			//	uplotfnxt("FAIL INSIDE");
		}
	}
	if(CR & ECPR_OUTSIDE)
	{
		if(g_nBspOccluderDebugDrawClipResult && Node.nOutside == OCCLUDER_EMPTY)
		{
			if(g_nShowClipLevel < 0)
				BspDrawPoly(pBsp, ClippedPolyOut, nOut, 0xff000000 | randredcolor(), 0, 1);
		}

		ZASSERT(Node.nOutside != OCCLUDER_LEAF);
		if((Node.nOutside == OCCLUDER_EMPTY ||
			!BspCullObjectR(pBsp, Node.nOutside, ClippedPolyOut, nOut, nClipLevel + 1)))
		{
			// uplotfnxt("FAIL OUTSIDE %d :: %d,%d", Index, Node.nOccluderIndex, Node.nEdge);

			return false;
		}
	}
	return !bFail;
}

bool BspCullObject(SOccluderBsp *pBsp, SWorldObject *pObject)
{
	if(0 == (pObject->nFlags & SObject::OCCLUSION_TEST))
		return false;
	pBsp->Stats.nNumObjectsTested++;

	MICROPROFILE_SCOPEIC("BSP", "Cull");
	if(!pBsp->Nodes.Size())
		return false;
	g_nBspDebugPlaneCounter = 0;
	long seed = rand();
	srand((int)(uintptr)pObject);

	// SBspEdgeIndex Poly[5];
	uint16_t Poly[5];
	uint32 nSize = pBsp->Planes.Size();
	{
		MICROPROFILE_SCOPEIC("BSP", "CullPrepare");
		m mObjectToWorld = pObject->mObjectToWorld;
		m mObjectToBsp = mmult(pBsp->mtobsp, mObjectToWorld);
		v3 vHalfSize = pObject->vSize;
		v4 vCenterWorld_ = mtransform(pBsp->mtobsp, pObject->mObjectToWorld.trans);
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

		if(g_nBspOccluderDebugDrawClipResult)
		{
			v3 v0_ = mtransform(pBsp->mfrombsp, v0);
			v3 v1_ = mtransform(pBsp->mfrombsp, v1);
			v3 v2_ = mtransform(pBsp->mfrombsp, v2);
			v3 p3_ = mtransform(pBsp->mfrombsp, p3);
			ZDEBUG_DRAWLINE(v0_, v1_, 0xff00ff00, true);
			ZDEBUG_DRAWLINE(v2_, v1_, 0xff00ff00, true);
			ZDEBUG_DRAWLINE(p3_, v2_, 0xff00ff00, true);
			ZDEBUG_DRAWLINE(v0_, p3_, 0xff00ff00, true);
		}

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

	


	bool bResult = BspCullObjectR(pBsp, 0, &Poly[0], 5, 0);
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
