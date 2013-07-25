#include "base.h"
#include "math.h"
#include "fixedarray.h"
#include "text.h"
#include "program.h"
#include "input.h"
#include "bsp.h"
#include "microprofileinc.h"

#include <algorithm>
#define OCCLUDER_EMPTY (0xc000)
#define OCCLUDER_LEAF (0x8000)
#define OCCLUDER_CLIP_MAX 0x100
#define USE_FRUSTUM 0


uint32 g_nBspOccluderDebugDrawClipResult = 1;
uint32 g_nBspOccluderDrawEdges = 2;
uint32 g_nBspOccluderDrawOccluders = 1;
uint32 g_nBreakOnClipIndex = -1;

uint32 g_nBspDebugPlane = -1;
uint32 g_nBspDebugPlaneCounter = 0;
uint32 g_nCheck = 0;

struct SOccluderPlane;
struct SBspEdgeIndex;
struct SOccluderBspNode;
struct SOccluderPlane
{
	v4 p[5];
};



struct SBspEdgeIndex
{
	SBspEdgeIndex(){}
	SBspEdgeIndex(const SOccluderBspNode& node);
	uint16 	nOccluderIndex;
	uint8 	nEdge : 4;	//[0-3] edge idx // 5 is normal
	uint8 	nFlip : 1;
	uint8 	nSkip : 1;
};

struct SOccluderBspNode
{
	uint16 nOccluderIndex;
	uint8 nEdge : 4;//[0-3] edge idx
	uint8 nFlip : 1;
	uint8 bLeaf : 1;
	uint16	nInside;
	uint16	nOutside;
};

struct SOccluderBsp
{
	SOccluderBspViewDesc Desc;
	uint32 nDepth;
	TFixedArray<SOccluderPlane, 1024> Occluders;
	TFixedArray<SOccluderBspNode, 1024> Nodes;
};

SOccluderBsp* g_pBsp = 0;
void BspOccluderDebugDraw(v4* pVertexNew, v4* pVertexIn, uint32 nColor, uint32 nFill = 0) ;
void BspAddOccluderInternal(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex);
bool BspAddOccluder(SOccluderBsp* pBsp, v3 vCenter, v3 vNormal, v3 vUp, float fUpSize, v3 vLeft, float fLeftSize, bool bFlip = false);
bool BspAddOccluder(SOccluderBsp* pBsp, v3* vCorners, uint32 nNumCorners);
int BspAddInternal(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nVertices, uint32 nLevel);
void BspAddOccluderRecursive(SOccluderBsp* pBsp, uint32 nBspIndex, SBspEdgeIndex* Poly, uint32 nEdges, uint32 nLevel);
void BspDebugDrawHalfspace(v3 vNormal, v3 vPosition);
v3 BspGetCorner(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nEdges, uint32 i);


SBspEdgeIndex::SBspEdgeIndex(const SOccluderBspNode& node)
:nOccluderIndex(node.nOccluderIndex)
,nEdge(node.nEdge)
,nFlip(node.nFlip)
,nSkip(0)
{
}

v3 BspPlaneIntersection(v4 p0, v4 p1, v4 p2)
{
MICROPROFILE_SCOPEIC("BSP", "BspPlaneIntersect");

	float x = -(-p0.w*p1.y*p2.z+p0.w*p2.y*p1.z+p1.w*p0.y*p2.z-p1.w*p2.y*p0.z-p2.w*p0.y*p1.z+p2.w*p1.y*p0.z)
	/(-p0.x*p1.y*p2.z+p0.x*p2.y*p1.z+p1.x*p0.y*p2.z-p1.x*p2.y*p0.z-p2.x*p0.y*p1.z+p2.x*p1.y*p0.z);
	float y= -(p0.w*p1.x*p2.z-p0.w*p2.x*p1.z-p1.w*p0.x*p2.z+p1.w*p2.x*p0.z+p2.w*p0.x*p1.z-p2.w*p1.x*p0.z)
	/(-p0.x*p1.y*p2.z+p0.x*p2.y*p1.z+p1.x*p0.y*p2.z-p1.x*p2.y*p0.z-p2.x*p0.y*p1.z+p2.x*p1.y*p0.z);
	float z = -(p0.w*p1.x*p2.y-p0.w*p2.x*p1.y-p1.w*p0.x*p2.y+p1.w*p2.x*p0.y+p2.w*p0.x*p1.y-p2.w*p1.x*p0.y)
	/(p0.x*p1.y*p2.z-p0.x*p2.y*p1.z-p1.x*p0.y*p2.z+p1.x*p2.y*p0.z+p2.x*p0.y*p1.z-p2.x*p1.y*p0.z);

	return v3init(x,y,z);
}


#define GET_PLANE(pBsp, idx) BspGetPlane(pBsp, idx.nOccluderIndex, idx.nEdge, idx.nFlip)
inline
v4 BspGetPlane(SOccluderBsp* pBsp, uint32 nOccluderIndex, uint32 nEdge, uint32 nFlip)
{
	v4 vPlane = pBsp->Occluders[ nOccluderIndex ].p[ nEdge ];
	if(nFlip)
		return -vPlane;
	else
		return vPlane;
}

static const char* Spaces(int n)
{
	static char buf[256];
	ZASSERT(n<sizeof(buf)-1);
	for(int i = 0; i < n; ++i)
		buf[i] = ' ';
	buf[n] = 0;
	return &buf[0];
}


v4 MakePlane(v3 p, v3 normal)
{
	v4 r = v4init(normal, -(v3dot(normal,p)));
	float vDot = v4dot(v4init(p, 1.f), r);
	if(fabsf(vDot) > 0.001f)
	{
		ZBREAK();
	}

	return r;
}
void BspDump(SOccluderBsp* pBsp, int nNode, int nLevel)
{
	SOccluderBspNode* pNode = pBsp->Nodes.Ptr() + nNode;
	uprintf("D%s0x%04x: plane %2d:%2d   [i:%04x,o:%04x]\n", Spaces(3*nLevel), nNode, pNode->nOccluderIndex, pNode->nEdge, pNode->nInside, pNode->nOutside);
	if(0 == (0x8000&pNode->nInside))
		BspDump(pBsp, pNode->nInside, nLevel + 1);
	else
		uprintf("D%s0x%04x\n", Spaces(3*(nLevel+1)), pNode->nInside);

	if(0 == (0x8000&pNode->nOutside))
		BspDump(pBsp, pNode->nOutside, nLevel + 1);
	else
		uprintf("D%s0x%04x\n", Spaces(3*(nLevel+1)), pNode->nOutside);



}

SOccluderBsp* BspCreate()
{
	return new SOccluderBsp;
}
void BspDestroy(SOccluderBsp* pBsp)
{
	delete pBsp;
}
void BspBuild(SOccluderBsp* pBsp, SOccluder* pOccluderDesc, uint32 nNumOccluders, SWorldObject* pWorldObjects, uint32 nNumWorldObjects, const SOccluderBspViewDesc& Desc)
{
	g_pBsp = pBsp;
MICROPROFILE_SCOPEIC("BSP", "Build");
	if(g_KeyboardState.keys[SDLK_F3]&BUTTON_RELEASED)
	{
		g_nBspOccluderDrawOccluders = (g_nBspOccluderDrawOccluders+1)%2;
	}

	if(g_KeyboardState.keys[SDLK_F4]&BUTTON_RELEASED)
	{
		g_nBspOccluderDebugDrawClipResult = (g_nBspOccluderDebugDrawClipResult+1)%2;
	}

	if(g_KeyboardState.keys[SDLK_F5]&BUTTON_RELEASED)
	{
		g_nBspOccluderDrawEdges = (g_nBspOccluderDrawEdges+1)%4;
	}

	g_nCheck = 0;
	g_nBspDebugPlaneCounter = 0;

	uplotfnxt("OCCLUDER Occluders:(f3)%d Clipresult:(f4)%d Edges:(f5)%d", g_nBspOccluderDrawOccluders, g_nBspOccluderDebugDrawClipResult, g_nBspOccluderDrawEdges);
	if(g_KeyboardState.keys[SDLK_F8] & BUTTON_RELEASED)
		g_nBspDebugPlane ++;
	if(g_KeyboardState.keys[SDLK_F7] & BUTTON_RELEASED)
		g_nBspDebugPlane--;
	uplotfnxt("BSP DEBUG PLANE %d", g_nBspDebugPlane);


	long seed = rand();
	srand(42);

	pBsp->nDepth = 0;
	pBsp->Nodes.Clear();
	pBsp->Occluders.Clear();

	pBsp->Desc = Desc;
	const v3 vOrigin = pBsp->Desc.vOrigin;
	const v3 vDirection = pBsp->Desc.vDirection;
//	pBsp->Occluders.Resize(1+nNumOccluders);
//	SOccluderPlane* pPlanes = pBsp->Occluders.Ptr();


	//FRUSTUM
	if(USE_FRUSTUM)
	{
		v3 vFrustumCorners[4];
		SOccluderBspViewDesc Desc = pBsp->Desc;

		float fAngle = (Desc.fFovY * PI / 180.f)/2.f;
		float fCA = cosf(fAngle);
		float fSA = sinf(fAngle);
		float fY = fSA / fCA;
		float fX = fY / Desc.fAspect;
		const float fDist = 3;
		const v3 vUp = v3cross(Desc.vRight, Desc.vDirection);

		vFrustumCorners[0] = fDist * (Desc.vDirection + fY * vUp + fX * Desc.vRight) + vOrigin;
		vFrustumCorners[1] = fDist * (Desc.vDirection - fY * vUp + fX * Desc.vRight) + vOrigin;
		vFrustumCorners[2] = fDist * (Desc.vDirection - fY * vUp - fX * Desc.vRight) + vOrigin;
		vFrustumCorners[3] = fDist * (Desc.vDirection + fY * vUp - fX * Desc.vRight) + vOrigin;

		ZASSERTNORMALIZED3(Desc.vDirection);
		ZASSERTNORMALIZED3(Desc.vRight);
		ZASSERTNORMALIZED3(vUp);
		SOccluderPlane& Plane = *pBsp->Occluders.PushBack();
		int nPrev = -1;
		for(uint32 i = 0; i < 4; ++i)
		{
			v3 v0 = vFrustumCorners[i];
			v3 v1 = vFrustumCorners[(i+1) % 4];
			v3 v2 = vOrigin;
			v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
			v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
			v3 vEnd = vCenter + vNormal;
			Plane.p[i] = -(MakePlane(vFrustumCorners[i], vNormal));
			{
				int nIndex = (int)pBsp->Nodes.Size();
				SOccluderBspNode* pNode = pBsp->Nodes.PushBack();
				pNode->nOutside = OCCLUDER_EMPTY;
				pNode->nInside = OCCLUDER_LEAF;
				pNode->nEdge = i;
				pNode->nOccluderIndex = 0;
				pNode->nFlip = 0;
				pNode->bLeaf = 1;
				if(nPrev >= 0)
					pBsp->Nodes[nPrev].nOutside = nIndex;
				nPrev = nIndex;
			}
		}
		Plane.p[4] = v4init(0.f);

		if(g_nBspOccluderDrawEdges&2)
		{
			ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[0], 0, true);
			ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[1], 0, true);
			ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[2], 0, true);
			ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[3], 0, true);
			ZDEBUG_DRAWLINE(vFrustumCorners[0], vFrustumCorners[1], 0, true);
			ZDEBUG_DRAWLINE(vFrustumCorners[1], vFrustumCorners[2], 0, true);
			ZDEBUG_DRAWLINE(vFrustumCorners[2], vFrustumCorners[3], 0, true);
			ZDEBUG_DRAWLINE(vFrustumCorners[3], vFrustumCorners[0], 0, true);
		}
	}
#if 1
	{
		struct SSortKey
		{
			float fDistance;
			int nIndex;
			bool operator < (const SSortKey& other) const
			{
				return fDistance < other.fDistance;
			}
		};
		SSortKey* pKeys = (SSortKey*)alloca(nNumWorldObjects*sizeof(SSortKey));
		int n = 0;
		for(uint32 i = 0; i < nNumWorldObjects; ++i)
		{
			v3 vEyeToObject = pWorldObjects[i].mObjectToWorld.trans.tov3() - Desc.vOrigin;
			float fDist = v3dot(vDirection, vEyeToObject);
			if(fDist > 0)
			{
				pKeys[n].nIndex = i;
				pKeys[n++].fDistance = fDist;
			}
		}
		std::sort(pKeys, pKeys + n);
		for(uint32 i = 0; i < n; ++i)
		{
			SWorldObject* pObject = pKeys[i].nIndex + pWorldObjects;
			m mat = pObject->mObjectToWorld;
			v3 vSize = pObject->vSize;
			v3 vCenter = mat.trans.tov3();
			v3 vX = mat.x.tov3();
			v3 vY = mat.y.tov3();
			v3 vZ = mat.z.tov3();


			v3 vCenterX = vCenter + vX * vSize.x;
			v3 vCenterY = vCenter + vY * vSize.y;
			v3 vCenterZ = vCenter + vZ * vSize.z;
			v3 vCenterXNeg = vCenter - vX * vSize.x;
			v3 vCenterYNeg = vCenter - vY * vSize.y;
			v3 vCenterZNeg = vCenter - vZ * vSize.z;

			const v3 vToCenterX = vCenterX - vOrigin;
			const v3 vToCenterY = vCenterY - vOrigin;
			const v3 vToCenterZ = vCenterZ - vOrigin;
			uplotfnxt(" dot %f", v3dot(vY, vToCenterY));
			bool bFrontX = v3dot(vX, vToCenterX) < 0.f;
			bool bFrontY = v3dot(vY, vToCenterY) < 0.f;
			bool bFrontZ = v3dot(vZ, vToCenterZ) < 0.f;

			vX = bFrontX ? vX : -vX;
			vY = bFrontY ? vY : -vY;
			vZ = bFrontZ ? vZ : -vZ;
			uplotfnxt("LALACenterY %f %f %f :: %f %f %f", vCenterY.x, vCenterY.y, vCenterY.z,
				vCenterYNeg.x, vCenterYNeg.y, vCenterYNeg.z);

			//uplotfnxt("bNEG %d", bNeg ? 1 : 0);
			vCenterX = bFrontX ? vCenterX : vCenterXNeg;
			vCenterY = bFrontY ? vCenterY : vCenterYNeg;
			vCenterZ = bFrontZ ? vCenterZ : vCenterZNeg;
			uplotfnxt("CenterY %f %f %f :: %f %f %f", vCenterY.x, vCenterY.y, vCenterY.z,
				vY.x, vY.y, vY.z);

			BspAddOccluder(pBsp, vCenterY/**1.01*/, vY, vZ, vSize.z, vX, vSize.x,true);			
//			BspAddOccluder(pBsp, vCenterX/**1.01*/, vX, vY, vSize.y, vZ, vSize.z, true);
			BspAddOccluder(pBsp, vCenterZ/**1.01*/, vZ, vY, vSize.y, vX, vSize.x, true);

			// BspAddOccluder(pBsp, vCenter + vY * vSize.y/**1.01*/, vY, vZ, vSize.z, vX, vSize.x,true);			
			// BspAddOccluder(pBsp, vCenter + vX * vSize.x/**1.01*/, vX, vY, vSize.y, vZ, vSize.z, true);
			//BspAddOccluder(pBsp, vCenter + vZ * vSize.z/**1.01*/, vZ, vY, vSize.y, vX, vSize.x, true);


			//uprintf("DOT %i %i %i\n", v3dot(vX, vDirection) < 0.f, v3dot(vY, vDirection) < 0.f, v3dot(vZ, vDirection) < 0.f);
			
			// BspAddOccluder(pBsp, vCenter + vY * vSize.y/**1.01*/, vY, vZ, vSize.z, vX, vSize.x,true);			
			// BspAddOccluder(pBsp, vCenter + vX * vSize.x/**1.01*/, vX, vY, vSize.y, vZ, vSize.z, true);
			//BspAddOccluder(pBsp, vCenter + vZ * vSize.z/**1.01*/, vZ, vY, vSize.y, vX, vSize.x, true);
			if(pBsp->Occluders.Size()>150 || pBsp->Nodes.Size() > 300)
				break;
		}
	}




//	uint32 nOccluderIndex = 1; // first is frustum

#else
	for(uint32 k = 0; k < nNumOccluders; ++k)
	{
		v3 vCorners[4];
		SOccluder Occ = pOccluderDesc[k];
		v3 vNormal = Occ.mObjectToWorld.z.tov3();
		v3 vUp = Occ.mObjectToWorld.y.tov3();
		v3 vLeft = v3cross(vNormal, vUp);
		v3 vCenter = Occ.mObjectToWorld.w.tov3();
		vCorners[0] = vCenter + vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[1] = vCenter - vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[2] = vCenter - vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
		vCorners[3] = vCenter + vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
		BspAddOccluder(pBsp, &vCorners[0], 4);
		// SOccluderPlane& Plane = pPlanes[nOccluderIndex];
		// v3 vInnerNormal = v3normalize(v3cross(vCorners[1]-vCorners[0], vCorners[3]-vCorners[0]));
		// bool bFlip = false;
		// v4 vNormalPlane = MakePlane(vCorners[0], vInnerNormal);
		// {
		// 	float fMinDot = v3dot(vCorners[0]-vOrigin, vDirection);
		// 	fMinDot = Min(fMinDot, v3dot(vCorners[1]-vOrigin, vDirection));
		// 	fMinDot = Min(fMinDot, v3dot(vCorners[2]-vOrigin, vDirection));
		// 	fMinDot = Min(fMinDot, v3dot(vCorners[3]-vOrigin, vDirection));

		// 	//project origin onto plane
		// 	float fDist = v4dot(vNormalPlane, v4init(vOrigin, 1.f));

		// 	if(fabs(fDist) < Desc.fZNear)//if plane is very close to camera origin, discard to avoid intersection with near plane
		// 	{
		// 		if(fMinDot < 1.5 * Desc.fZNear) // _unless_ all 4 points clearly in front
		// 		{
		// 			continue;
		// 		}
				
		// 	}
		// 	v3 vPointOnPlane = vOrigin - fDist * vNormalPlane.tov3();
		// 	float fFoo = v4dot(vNormalPlane, v4init(vPointOnPlane, 1.f));
		// 	ZASSERT(fabs(fFoo) < 1e-4f);

		// 	if(fDist>0)
		// 	{
		// 		bFlip = true;
		// 		vInnerNormal = -vInnerNormal;
		// 		vNormalPlane = -vNormalPlane;
		// 	}
		// }
		// Plane.p[4] = vNormalPlane;


		// for(uint32 i = 0; i < 4; ++i)
		// {
		// 	v3 v0 = vCorners[i];
		// 	v3 v1 = vCorners[(i+1) % 4];
		// 	v3 v2 = vOrigin;
		// 	v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
		// 	v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
		// 	v3 vEnd = vCenter + vNormal;
		// 	Plane.p[i] = MakePlane(vCorners[i], vNormal);
		// 	if(bFlip)
		// 		Plane.p[i] = -Plane.p[i];
		// 	if(g_nBspOccluderDrawEdges&1)
		// 	{
		// 		ZDEBUG_DRAWLINE(v0, v1, (uint32)-1, true);
		// 		ZDEBUG_DRAWLINE(v1, v2, (uint32)-1, true);
		// 		ZDEBUG_DRAWLINE(v2, v0, (uint32)-1, true);
		// 	}
		// }
		// BspAddOccluder(pBsp, &pBsp->Occluders[nOccluderIndex], nOccluderIndex);
		// nOccluderIndex++;
	}
#endif
//	ZASSERT(nOccluderIndex <= pBsp->Occluders.Size());

	g_nCheck = 1;
	if(g_KeyboardState.keys[SDLK_F10] & BUTTON_RELEASED)
	{
		BspDump(pBsp, 0, 0);
	}

	uplotfnxt("BSP: NODES[%03d] OCCLUDERS[%03d] DEPTH[%03d]", pBsp->Nodes.Size(), pBsp->Occluders.Size(), pBsp->nDepth);
	srand(seed);
}
void BSPDUMP()
{
	BspDump(g_pBsp, 0, 0);
}

void BspAddOccluderInternal(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex)
{
	//base poly
	SBspEdgeIndex Poly[ 5 ];
	memset(&Poly[0], 0, sizeof(Poly));
	for(uint32 i = 0; i < 4; ++i)
	{
		Poly[i].nOccluderIndex = nOccluderIndex;
		Poly[i].nEdge = i;
	}
	Poly[4].nOccluderIndex = nOccluderIndex;
	Poly[4].nEdge = 4;

	uint32 nNumNodes = (uint32)pBsp->Nodes.Size();
	if(pBsp->Nodes.Empty())
	{
		uint16 nIndex = (uint16)BspAddInternal(pBsp, &Poly[0], 5, 1);
		ZASSERT(nIndex==0);
	}
	else
	{
		BspAddOccluderRecursive(pBsp, 0, &Poly[0], 5, 1);
	}
}


bool BspAddOccluder(SOccluderBsp* pBsp, v3 vCenter, v3 vNormal, v3 vUp, float fUpSize, v3 vLeft, float fLeftSize, bool bFlip)
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
//	const v3 vNormal = v3cross(vCorners[0] - vCorners[1], vCorners[2] - vCorners[0]);
//	const v3 vCenter = (vCorners[0] + vCorners[1] + vCorners[2] + vCorners[3]) * 0.25f;
	const v3 vToCenter = vCenter - pBsp->Desc.vOrigin;
	const float fDot = v3dot(vToCenter,vNormal);
	uplotfnxt("DOT IS %f", fDot);
	if(fDot < 0.03f)//reject backfacing and gracing polygons
	{
		return BspAddOccluder(pBsp, &vCorners[0], 4);
	}
	else
	{
		uplotfnxt("REJECT BECAUSE DOT IS %f", fDot);
		return false;
	}
}

bool BspAddOccluder(SOccluderBsp* pBsp, v3* vCorners, uint32 nNumCorners)
{
	ZASSERT(nNumCorners == 4);
	const v3 vOrigin = pBsp->Desc.vOrigin;
	const v3 vDirection = pBsp->Desc.vDirection;
	uint32 nIndex = pBsp->Occluders.Size();
	SOccluderPlane* pPlane = pBsp->Occluders.PushBack();
	v3 vInnerNormal = v3normalize(v3cross(vCorners[1]-vCorners[0], vCorners[3]-vCorners[0]));
	bool bFlip = false;
	v4 vNormalPlane = MakePlane(vCorners[0], vInnerNormal);
	{
		float fMinDot = v3dot(vCorners[0]-vOrigin, vDirection);
		fMinDot = Min(fMinDot, v3dot(vCorners[1]-vOrigin, vDirection));
		fMinDot = Min(fMinDot, v3dot(vCorners[2]-vOrigin, vDirection));
		fMinDot = Min(fMinDot, v3dot(vCorners[3]-vOrigin, vDirection));
		//project origin onto plane
		float fDist = v4dot(vNormalPlane, v4init(vOrigin, 1.f));
		if(fabs(fDist) < pBsp->Desc.fZNear)//if plane is very close to camera origin, discard to avoid intersection with near plane
		{
			if(fMinDot < 1.5 * pBsp->Desc.fZNear) // _unless_ all 4 points clearly in front
			{
				pBsp->Occluders.PopBack();
				return false;
			}
			
		}
		v3 vPointOnPlane = vOrigin - fDist * vNormalPlane.tov3();
		float fFoo = v4dot(vNormalPlane, v4init(vPointOnPlane, 1.f));
		ZASSERT(fabs(fFoo) < 1e-4f);

		if(fDist>0)
		{
			bFlip = true;
			vInnerNormal = -vInnerNormal;
			vNormalPlane = -vNormalPlane;
		}
	}
	pPlane->p[4] = vNormalPlane;


	for(uint32 i = 0; i < 4; ++i)
	{
		v3 v0 = vCorners[i];
		v3 v1 = vCorners[(i+1) % 4];
		v3 v2 = vOrigin;
		v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
		v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
		v3 vEnd = vCenter + vNormal;
		pPlane->p[i] = MakePlane(vCorners[i], vNormal);
		if(bFlip)
			pPlane->p[i] = -pPlane->p[i];
		if(g_nBspOccluderDrawEdges&1)
		{
			ZDEBUG_DRAWLINE(v0, v1, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v1, v2, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v2, v0, (uint32)-1, true);
		}
	}
	BspAddOccluderInternal(pBsp, pPlane, nIndex);
	return true;

}


void BspDrawPoly(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nVertices, uint32 nColorPoly, uint32 nColorEdges, uint32 nBoxes = 0)
{
	uint32 nPolyEdges = nVertices-1;
	v3* vCorners = (v3*)alloca(sizeof(v3)*nPolyEdges);
	v4 vNormalPlane = pBsp->Occluders[ Poly[nPolyEdges].nOccluderIndex ].p[4];
	for(int i = 0; i < nPolyEdges; ++i)
	{
		uint32 i0 = i;
		uint32 i1 = (i+1)%nPolyEdges;
		v4 v0 = pBsp->Occluders[ Poly[i0].nOccluderIndex ].p[ Poly[i0].nEdge ];
		v4 v1 = pBsp->Occluders[ Poly[i1].nOccluderIndex ].p[ Poly[i1].nEdge ];
		vCorners[i] = BspPlaneIntersection(v0, v1, vNormalPlane);
		if(nBoxes)
		{
			ZDEBUG_DRAWBOX(mid(), vCorners[i], v3rep(0.01f), 0xffff0000, 1);
		}
	}
	if(nPolyEdges)
	{
		v3 foo = v3init(0.01f, 0.01f, 0.01f);
		for(int i = 0; i < nPolyEdges; ++i)
		{
			if(0 == Poly[(i+nPolyEdges+1)%nPolyEdges].nSkip)
			{
				ZDEBUG_DRAWLINE(vCorners[i], vCorners[(i+1)%nPolyEdges], nColorEdges, true);
			}
		}
		ZDEBUG_DRAWPOLY(&vCorners[0], nPolyEdges, nColorPoly);

	}
}



int BspAddInternal(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nEdges, uint32 nLevel)
{
	int r = (int)pBsp->Nodes.Size();
	int nPrev = -1;
	if(nLevel + nEdges > pBsp->nDepth)
		pBsp->nDepth = nLevel + nEdges;
	for(uint32 i = 0; i < nEdges; ++i)
	{
		if(0 == Poly[i].nSkip)
		{
			int nIndex = (int)pBsp->Nodes.Size();
			SOccluderBspNode* pNode = pBsp->Nodes.PushBack();
			pNode->nOutside = OCCLUDER_EMPTY;
			pNode->nInside = OCCLUDER_LEAF;
			pNode->nEdge = (uint8)Poly[i].nEdge;
			pNode->nOccluderIndex = (uint16)Poly[i].nOccluderIndex;
			pNode->nFlip = Poly[i].nFlip;
			pNode->bLeaf = Poly[i].nEdge == 4;
			if(nPrev >= 0)
				pBsp->Nodes[nPrev].nInside = nIndex;
			nPrev = nIndex;
			//if(i < nEdges - 1)
			if(g_nBspDebugPlane == g_nBspDebugPlaneCounter++)
			{
				v4 vPlane = GET_PLANE(pBsp, Poly[i]);
				uplotfnxt("DEBUG PLANE %d :: %d, flip %d", Poly[i].nOccluderIndex, Poly[i].nEdge, Poly[i].nFlip);
				v3 vCorner = BspGetCorner(pBsp, Poly, nEdges, i < nEdges - 1 ? i : i-1);
				BspDebugDrawHalfspace(vPlane.tov3(), vCorner);
			}
		}
	}

	if(g_nBspOccluderDrawOccluders)
	{
		BspDrawPoly(pBsp, Poly, nEdges, 0x88000000 | randcolor(), 0xffff0000);
	}

	return r;
}
#define OCCLUDER_POLY_MAX 12
enum ClipPolyResult
{
	ECPR_CLIPPED = 0,
	ECPR_INSIDE = 0x1,
	ECPR_OUTSIDE = 0x2,
	ECPR_BOTH = 0x3,
};


v3 BspGetCorner(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nEdges, uint32 i)
{
	uint32 nRealEdges = nEdges-1;
	ZASSERT(Poly[nRealEdges].nEdge == 4);
	ZASSERT(i < nRealEdges);
	ZASSERT(nRealEdges >= 2);
	v4 vNormalPlane = pBsp->Occluders[ Poly[nRealEdges].nOccluderIndex ].p[4];
	uint32 i0 = i;
	uint32 i1 = (i+nRealEdges-1)%nRealEdges;
	v4 v0 = GET_PLANE(pBsp, Poly[i0]);
	v4 v1 = GET_PLANE(pBsp, Poly[i1]);
	return BspPlaneIntersection(v0, v1, vNormalPlane); 
}

bool PlaneToleranceTest(v4 vPlane, v4 vClipPlane, float fPlaneTolerance)
{
	float f3dot = fabs(v3dot(vPlane.tov3(), vClipPlane.tov3()));
	float f3dot1 = fabs(v3dot((-vPlane).tov3(), vClipPlane.tov3()));
	float fMinDist = fabs(v4length(vPlane -  vClipPlane));
	float fMinDist1 = fabs(v4length(-vPlane - vClipPlane));
	if(Min<float>(fMinDist, fMinDist1) < fPlaneTolerance || Max<float>(f3dot, f3dot1) > 0.9)
	{
		//uplotfnxt("MIN %f %f .. MAX %f %f", fMinDist, fMinDist1, f3dot, f3dot1);

	}

	if(Min<float>(fMinDist, fMinDist1) < fPlaneTolerance && Max<float>(f3dot, f3dot1) > 0.9)
	{
		//uprintf("FAIL %f %f  v3d %f", fMinDist, fMinDist1, f3dot);
		return true;
	}
	else
	{
		return false;
	}


}

uint32 BspClipPoly(SOccluderBsp* pBsp, SBspEdgeIndex PlaneIndex, SBspEdgeIndex* Poly, uint32 nEdges, SBspEdgeIndex* pPolyOut)
{
	v4 vPlane = GET_PLANE(pBsp, PlaneIndex);
	v3* vCorners = (v3*)alloca(nEdges * sizeof(v3));
	bool* bBehindCorner = (bool*)alloca(nEdges);
	ZASSERT(0 == (0x3&(uintptr)vCorners));
	uint32 nRealEdges = nEdges-1;
	ZASSERT(Poly[nRealEdges].nEdge == 4);
	const float fPlaneTolerance = 1e-4f;
//	ZASSERT(4 == Poly[nRealEdges].nEdge);
	v4 vNormalPlane = pBsp->Occluders[ Poly[nRealEdges].nOccluderIndex ].p[Poly[nRealEdges].nEdge];
	bool bHasRejectPlane = false; //dont clip if too close
	int nBehindCount = 0; // no. of corners behind
	int nFrontCount = 0; // no. of corners in front
	float fDistLort = 0;
	int nRejectIndex = -1;
	for(uint32 i = 0; i < nRealEdges; ++i)
	{
		uint32 i0 = i;
		uint32 i1 = (i+nRealEdges-1)%nRealEdges;
		v4 v0 = GET_PLANE(pBsp, Poly[i0]);
		v4 v1 = GET_PLANE(pBsp, Poly[i1]);
		bool bPlaneTol = PlaneToleranceTest(v0, vPlane, fPlaneTolerance);
		if(bPlaneTol)
		{
			ZASSERT(nRejectIndex == -1);
			nRejectIndex = i;

		}
		//bHasRejectPlane = bHasRejectPlane ||  
		// if(v4length(v0 - vPlane) < fPlaneTolerance)
		// {
		// 	uplotfnxt("TOL FAIL %16.10f", v4length(v0 - vPlane));
		//  }
		// else
		{
			vCorners[i] = BspPlaneIntersection(v0, v1, vNormalPlane);
			float fDist = v4dot(v4init(vCorners[i],1.f), vPlane);
			fDistLort += fDist;
			bool bBehind = fDist >= 0.f;
			bBehindCorner[i] = bBehind;
			nBehindCount += v4dot(v4init(vCorners[i],1.f), vPlane) >= 0.1f ? 1 : 0;
			nFrontCount += v4dot(v4init(vCorners[i],1.f), vPlane) <= -0.1f ? 0 : 1;
			if(bPlaneTol)
			{
				//void BspDebugDrawHalfspace(v3 vNormal, v3 vPosition);
				// BspDebugDrawHalfspace(vPlane.tov3(), vCorners[i]);
				// BspDebugDrawHalfspace(v0.tov3(), vCorners[i]);
			}
		}
	}
	if(nRejectIndex> -1)
	{
		// if(nBehindCount == 0 && nFrontCount == 0)
		// 	return 0;
		// ZASSERT(nBehindCount == 0 || nFrontCount == 0);
		// ZASSERT(nBehindCount != nFrontCount);
		uplotfnxt("REJECT %f", fDistLort);
		if(fDistLort > 0.1f)
		{
			ZASSERT(nRejectIndex != nEdges-1);
			int nOut=0;
			for(uint32 i = 0; i < nEdges; ++i)
			{
				if(i != nRejectIndex)
				{
					pPolyOut[nOut++] = Poly[i];
				}
			}
			//memcpy(pPolyOut, Poly, nEdges * sizeof(*Poly));
			return nOut;
		}
		else
		{
			return 0;
		}
	}
	PlaneIndex.nSkip = 1;
	uint32 nOutIndex = 0;
	for(uint32 i = 0; i < nRealEdges; ++i)
	{
		uint32 i0 = i;
		uint32 i1 = (i+1)%nRealEdges;
		if(bBehindCorner[i0])
		{
			//output edge
			pPolyOut[nOutIndex++] = Poly[i];
			if(bBehindCorner[i1])
			{
				;//output by next itera
			}
			else
			{
				//output Plane
				pPolyOut[nOutIndex++] = PlaneIndex;
			}
		}
		else
		{
			if(bBehindCorner[i1])
			{
				pPolyOut[nOutIndex++] = Poly[i];
			}
			else
			{
				//both outside, cut
			}
		}
	}
	pPolyOut[nOutIndex++] = Poly[nRealEdges]; //add normal
	if(g_nCheck)
	{
		v4 vNormalPlane = GET_PLANE(pBsp, Poly[nRealEdges]);
		for(uint32 i = 0; i < nOutIndex-1; ++i)
		{
			uint32 i0 = i;
			uint32 i1 = (i+1)%(nOutIndex-1);
			v4 v0 = GET_PLANE(pBsp, pPolyOut[i0]);
			v4 v1 = GET_PLANE(pBsp, pPolyOut[i1]);
			float fCrossLength = v3length( v3cross(v0.tov3(), v1.tov3()));
			float fCrossLength0 = v3length( v3cross(v0.tov3(), vNormalPlane.tov3()));
			if(fCrossLength0 < 0.3f || fCrossLength < 0.3f)
				uplotfnxt("LEN IS %f %f", fCrossLength0, fCrossLength);



		}
	}
	return nOutIndex;
}


ClipPolyResult BspClipPoly(SOccluderBsp* pBsp, SBspEdgeIndex ClipPlane, SBspEdgeIndex* Poly, uint32 nEdges, SBspEdgeIndex* pPolyOut, uint32& nEdgesIn, uint32& nEdgesOut)
{
	uint32 nIn = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, pPolyOut);
	if(nIn <= 2) 
		nIn = 0;
	ClipPlane.nFlip ^= 1;
	uint32 nOut = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, pPolyOut + nIn);
	if(nOut <= 2) 
		nOut = 0;

	nEdgesIn = nIn;
	nEdgesOut = nOut;


	return ClipPolyResult((nIn?0x1:0)| (nOut?0x2:0));
}

void BspAddOccluderRecursive(SOccluderBsp* pBsp, uint32 nBspIndex, SBspEdgeIndex* Poly, uint32 nEdges, uint32 nLevel)
{
	SOccluderBspNode Node = pBsp->Nodes[nBspIndex];
	

	SOccluderPlane* pPlane = pBsp->Occluders.Ptr() + Node.nOccluderIndex;
	v4 vPlane = pPlane->p[Node.nEdge];
	SBspEdgeIndex ClippedPoly[OCCLUDER_POLY_MAX*2];
	uint32 nNumEdgeIn = 0, nNumEdgeOut = 0;

	SBspEdgeIndex ClipPlane;
	ClipPlane.nOccluderIndex = Node.nOccluderIndex;
	ClipPlane.nEdge = Node.nEdge;
	ClipPlane.nFlip = Node.nFlip;

	ClipPolyResult CR = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, &ClippedPoly[0], nNumEdgeIn, nNumEdgeOut);
	ZASSERT(Node.nInside != OCCLUDER_EMPTY);
	ZASSERT(Node.nOutside != OCCLUDER_LEAF);
	SBspEdgeIndex* pPolyInside = &ClippedPoly[0];
	uint32 nPolyEdgeIn = nNumEdgeIn;
	// SBspEdgeIndex* pPolyInside = ECPR_BOTH == CR ? &ClippedPoly[0] : Poly;
	// uint32 nPolyEdgeIn = ECPR_BOTH == CR ? nNumEdgeIn : nEdges;
	

	//SBspEdgeIndex* pPolyOutside= ECPR_BOTH == CR ? &ClippedPoly[nNumEdgeIn] : Poly;
	SBspEdgeIndex* pPolyOutside = &ClippedPoly[nNumEdgeIn];
	//uint32 nPolyEdgeOut = ECPR_BOTH == CR ? nNumEdgeOut : nEdges;
	uint32 nPolyEdgeOut = nNumEdgeOut;

	if((ECPR_INSIDE & CR) != 0)
	{
		if(Node.nInside == OCCLUDER_LEAF)
		{
		}
		else
		{
			BspAddOccluderRecursive(pBsp, Node.nInside, pPolyInside, nPolyEdgeIn, nLevel+1);
		}
	}
	
	if((ECPR_OUTSIDE & CR) != 0)
	{
		if(Node.nOutside == OCCLUDER_EMPTY)
		{
			int nIndex = BspAddInternal(pBsp, pPolyOutside, nPolyEdgeOut, nLevel + 1);
			pBsp->Nodes[nBspIndex].nOutside = nIndex;
		}
		else
		{
			BspAddOccluderRecursive(pBsp, Node.nOutside, pPolyOutside, nPolyEdgeOut, nLevel+1);
		}
	}
}



bool BspCullObjectR(SOccluderBsp* pBsp, uint32 Index, SBspEdgeIndex* Poly, uint32 nNumEdges)
{
	if(g_nBspDebugPlane == g_nBspDebugPlaneCounter)
	{
		BspDrawPoly(pBsp, Poly, nNumEdges, 0xff000000|randredcolor(), 0, 1);
		if(g_nBreakOnClipIndex && g_nBspDebugPlane == g_nBspDebugPlaneCounter)
		{
			//uprintf("BREAK\n");
		}
	}
	g_nBspDebugPlaneCounter++;



	SOccluderBspNode Node = pBsp->Nodes[Index];
	SBspEdgeIndex EI = Node;
	uint32 nMaxEdges = (nNumEdges+1) * 2;
	SBspEdgeIndex* ClippedPoly = (SBspEdgeIndex*)alloca(nMaxEdges * sizeof(SBspEdgeIndex));
	uint32 nIn = 0;
	uint32 nOut = 0;

	ClipPolyResult CR = BspClipPoly(pBsp, EI, Poly, nNumEdges, ClippedPoly, nIn, nOut);
	SBspEdgeIndex* pIn = &ClippedPoly[0];
	SBspEdgeIndex* pOut = &ClippedPoly[nIn];
	bool bFail = false;
	if(CR & ECPR_INSIDE)
	{
		ZASSERT(Node.nInside != OCCLUDER_EMPTY);
		if(Node.nInside == OCCLUDER_LEAF)
		{
//			ZASSERT(Node.nEdge == 4);
			//culled
		}
		else if(!BspCullObjectR(pBsp, Node.nInside, pIn, nIn))
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
			BspDrawPoly(pBsp, pOut, nOut, 0xff000000|randredcolor(), 0, 1);
		}

		ZASSERT(Node.nOutside != OCCLUDER_LEAF);
		if((Node.nOutside == OCCLUDER_EMPTY || !BspCullObjectR(pBsp, Node.nOutside, pOut, nOut)))
		{
			uplotfnxt("FAIL OUTSIDE %d :: %d,%d", Index, Node.nOccluderIndex, Node.nEdge);

			return false;
		}

	}
	return !bFail;
}

bool BspCullObject(SOccluderBsp* pBsp, SWorldObject* pObject)
{
MICROPROFILE_SCOPEIC("BSP", "Cull");
	if(!pBsp->Nodes.Size())
		return false;
	g_nBspDebugPlaneCounter = 0;	
	long seed = rand();
	srand((int)(uintptr)pObject);

	SBspEdgeIndex Poly[5];
	uint32 nSize = pBsp->Occluders.Size();
	{
	MICROPROFILE_SCOPEIC("BSP", "CullPrepare");

		m mObjectToWorld = pObject->mObjectToWorld;
		v3 vHalfSize = pObject->vSize;
		v4 vCenterWorld_ = pObject->mObjectToWorld.trans;
		v3 vCenterWorld = v3init(vCenterWorld_);
		v3 vOrigin = pBsp->Desc.vOrigin;
		v3 vToCenter = v3normalize(vCenterWorld - vOrigin);
		v3 vUp = v3init(0.f,1.f, 0.f);//replace with camera up.
		if(v3dot(vUp, vToCenter) > 0.9f)
			vUp = v3init(1.f, 0, 0);
		v3 vRight = v3normalize(v3cross(vToCenter, vUp));
		m mbox = mcreate(vToCenter, vRight, vCenterWorld);
		m mboxinv = maffineinverse(mbox);
		m mcomposite = mmult(mbox, mObjectToWorld);
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
			ZDEBUG_DRAWLINE(v0, v1, 0xff00ff00, true);
			ZDEBUG_DRAWLINE(v2, v1, 0xff00ff00, true);
			ZDEBUG_DRAWLINE(p3, v2, 0xff00ff00, true);
			ZDEBUG_DRAWLINE(v0, p3, 0xff00ff00, true);
		}
		// vCenterQuad += vToCenter * 2*AABB.z;
		// v3 v0_ = vCenterQuad + vRight * AABB.x + vUp * AABB.y;
		// v3 v1_ = vCenterQuad + vRight * -AABB.x + vUp * AABB.y;
		// v3 v2_ = vCenterQuad + vRight * -AABB.x + vUp * -AABB.y;
		// v3 v3_ = vCenterQuad + vRight * AABB.x + vUp * -AABB.y;

		// ZDEBUG_DRAWLINE(v0_, v1_, 0xff00ff00, true);
		// ZDEBUG_DRAWLINE(v2_, v1_, 0xff00ff00, true);
		// ZDEBUG_DRAWLINE(v3_, v2_, 0xff00ff00, true);
		// ZDEBUG_DRAWLINE(v0_, v3_, 0xff00ff00, true);

		// ZDEBUG_DRAWLINE(v0, v0_, 0xff00ff00, true);
		// ZDEBUG_DRAWLINE(v1, v1_, 0xff00ff00, true);
		// ZDEBUG_DRAWLINE(v2, v2_, 0xff00ff00, true);
		// ZDEBUG_DRAWLINE(p3, v3_, 0xff00ff00, true);

		// ZDEBUG_DRAWLINE(v0*5.f, v3zero(), 0, true);
		// ZDEBUG_DRAWLINE(v1*5.f, v3zero(), 0, true);
		// ZDEBUG_DRAWLINE(v2*5.f, v3zero(), 0, true);
		// ZDEBUG_DRAWLINE(p3*5.f, v3zero(), 0, true);
		
		pBsp->Occluders.Resize(nSize+1);//TODO use thread specific blocks

		SOccluderPlane* pPlanes = pBsp->Occluders.Ptr() + nSize;
		v3 n0 = v3normalize(v3cross(v0, v0 - v1));
		v3 n1 = v3normalize(v3cross(v1, v1 - v2));
		v3 n2 = v3normalize(v3cross(v2, v2 - p3));
		v3 n3 = v3normalize(v3cross(p3, p3 - v0));
		pPlanes[0].p[0] = MakePlane(v0, n0);
		pPlanes[0].p[1] = MakePlane(v1, n1);
		pPlanes[0].p[2] = MakePlane(v2, n2);
		pPlanes[0].p[3] = MakePlane(p3, n3);
		pPlanes[0].p[4] = MakePlane(p3, v3normalize(vToCenter));



		Poly[0].nOccluderIndex = nSize;
		Poly[0].nEdge = 0;
		Poly[0].nFlip = 0;
		Poly[0].nSkip = 0;
		Poly[1].nOccluderIndex = nSize;
		Poly[1].nEdge = 1;
		Poly[1].nFlip = 0;
		Poly[1].nSkip = 0;
		Poly[2].nOccluderIndex = nSize;
		Poly[2].nEdge = 2;
		Poly[2].nFlip = 0;
		Poly[2].nSkip = 0;
		Poly[3].nOccluderIndex = nSize;
		Poly[3].nEdge = 3;
		Poly[3].nFlip = 0;
		Poly[3].nSkip = 0;
		Poly[4].nOccluderIndex = nSize;
		Poly[4].nEdge = 4;
		Poly[4].nFlip = 0;
		Poly[4].nSkip = 0;
	}
	bool bResult = BspCullObjectR(pBsp, 0, &Poly[0], 5);
	pBsp->Occluders.Resize(nSize);
	srand(seed);

	return bResult;
}


#define PLANE_DEBUG_TESSELATION 20
#define PLANE_DEBUG_SIZE 5

void BspDebugDrawHalfspace(v3 vNormal, v3 vPosition)
{
	v3 vLeft = v3cross(vNormal, v3init(1,0,0));
	if(v3length(vLeft) < 0.1f)
		vLeft = v3cross(vNormal, v3init(0,1,0));
	if(v3length(vLeft) < 0.51f)
		vLeft = v3cross(vNormal, v3init(0,0,1));

	ZASSERT(v3length(vLeft) > 0.4f);
	vLeft = v3normalize(vLeft);
	v3 vUp = v3normalize(v3cross(vNormal, vLeft));
	vLeft = v3normalize(v3cross(vUp, vNormal));
	v3 vPoints[PLANE_DEBUG_TESSELATION * PLANE_DEBUG_TESSELATION];
	for(int i = 0; i < PLANE_DEBUG_TESSELATION; ++i)
	{
		float x = PLANE_DEBUG_SIZE * (((float)i / (PLANE_DEBUG_TESSELATION-1)) - 0.5f);
		for(int j = 0; j < PLANE_DEBUG_TESSELATION; ++j)
		{
			float y = PLANE_DEBUG_SIZE * (((float)j / (PLANE_DEBUG_TESSELATION-1)) - 0.5f);
			vPoints[i*PLANE_DEBUG_TESSELATION+j] = vPosition + vUp * x + vLeft * y;
		}
	}

	for(int i = 0; i < PLANE_DEBUG_TESSELATION-1; ++i)
	{
		for(int j = 0; j < PLANE_DEBUG_TESSELATION-1; ++j)
		{
			ZDEBUG_DRAWLINE(vPoints[i*PLANE_DEBUG_TESSELATION+j],vPoints[i*PLANE_DEBUG_TESSELATION+j+1], 0xffff0000, true);
			ZDEBUG_DRAWLINE(vPoints[(1+i)*PLANE_DEBUG_TESSELATION+j],vPoints[i*PLANE_DEBUG_TESSELATION+j], 0xffff0000, true);
			ZDEBUG_DRAWLINE(vPoints[i*PLANE_DEBUG_TESSELATION+j],vPoints[i*PLANE_DEBUG_TESSELATION+j] + vNormal * 0.03, 0xff00ff00, true);
		}
	}


}







