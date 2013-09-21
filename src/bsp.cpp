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
int g_EdgeMask = 0xff;


m g_TOBSP;
m g_TOBSP__;



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
	v3 vDebugCorner;
};

struct SOccluderBsp
{
	SOccluderBspViewDesc DescOrg;
	m mtobsp;
	m mfrombsp;
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
void BspDrawPoly2(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nVertices, uint32 nColorPoly, uint32 nColorEdges, int nIndex);
int BspFindParent(SOccluderBsp* pBsp, int nChildIndex);
int BspFindParent(SOccluderBsp* pBsp, int nChildIndex, bool& bInside);
v3 BspToPlane2(v4 p);


uint32_t g_nDrawCount = 0;
uint32_t g_nDebugDrawPoly = 1;


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


void BspDump(SOccluderBsp* pBsp, int nNode, int nLevel)
{
	SOccluderBspNode* pNode = pBsp->Nodes.Ptr() + nNode;
	uprintf("D%s0x%04x: plane %2d:%2d   [i:%04x,o:%04x] parent %04x\n", Spaces(3*nLevel), nNode, pNode->nOccluderIndex, pNode->nEdge, pNode->nInside, pNode->nOutside, BspFindParent(pBsp, nNode));
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
	if(g_KeyboardState.keys[SDL_SCANCODE_F3]&BUTTON_RELEASED)
	{
		g_nBspOccluderDrawOccluders = (g_nBspOccluderDrawOccluders+1)%2;
	}

	if(g_KeyboardState.keys[SDL_SCANCODE_F4]&BUTTON_RELEASED)
	{
		g_nBspOccluderDebugDrawClipResult = (g_nBspOccluderDebugDrawClipResult+1)%2;
	}

	if(g_KeyboardState.keys[SDL_SCANCODE_F5]&BUTTON_RELEASED)
	{
		g_nBspOccluderDrawEdges = (g_nBspOccluderDrawEdges+1)%4;
	}
	if(g_KeyboardState.keys[SDL_SCANCODE_V]&BUTTON_RELEASED)
	{
		g_nDebugDrawPoly++;
	}
	if(g_KeyboardState.keys[SDL_SCANCODE_C]&BUTTON_RELEASED)
	{
		g_nDebugDrawPoly--;
	}


	g_nCheck = 0;
	g_nBspDebugPlaneCounter = 0;
	g_nDrawCount = 0;

	uplotfnxt("OCCLUDER Occluders:(f3)%d Clipresult:(f4)%d Edges:(f5)%d", g_nBspOccluderDrawOccluders, g_nBspOccluderDebugDrawClipResult, g_nBspOccluderDrawEdges);
	if(g_KeyboardState.keys[SDL_SCANCODE_F8] & BUTTON_RELEASED)
		g_nBspDebugPlane ++;
	if(g_KeyboardState.keys[SDL_SCANCODE_F7] & BUTTON_RELEASED)
		g_nBspDebugPlane--;
	if((int)g_nBspDebugPlane >= 0)
		uplotfnxt("BSP DEBUG PLANE %d", g_nBspDebugPlane);


	long seed = rand();
	srand(42);

	pBsp->nDepth = 0;
	pBsp->Nodes.Clear();
	pBsp->Occluders.Clear();

	pBsp->DescOrg = Desc;
	m mtobsp = mcreate(Desc.vDirection, Desc.vRight, Desc.vOrigin);
	const v3 vLocalDirection = mrotate(mtobsp, Desc.vDirection);
	const v3 vLocalOrigin = mtransform(mtobsp, Desc.vOrigin);
	const v3 vLocalRight = mrotate(mtobsp, Desc.vRight);
	const v3 vLocalUp = v3normalize(v3cross(vLocalRight, vLocalDirection));
	const v3 vWorldOrigin = Desc.vOrigin;
	const v3 vWorldDirection = Desc.vDirection;
	uplotfnxt("vLocalRight %4.2f %4.2f %4.2f", vLocalRight.x, vLocalRight.y, vLocalRight.z);
	uplotfnxt("vLocalOrigin %4.2f %4.2f %4.2f", vLocalOrigin.x, vLocalOrigin.y, vLocalOrigin.z);
	uplotfnxt("vLocalDirection %4.2f %4.2f %4.2f", vLocalDirection.x, vLocalDirection.y, vLocalDirection.z);
	pBsp->mtobsp = mtobsp;
	pBsp->mfrombsp = maffineinverse(mtobsp);

	v3 test = v3init(3,4,5);
	v3 lo = mtransform(pBsp->mtobsp, test);
	v3 en = mtransform(pBsp->mfrombsp, lo);
	float fDist1 = v3distance(test,en);
	float fDist = v3distance(v3init(0,0,-1), vLocalDirection);
	//uprintf("DISTANCE IS %f... %f\n", fDist, fDist1);
	if(abs(fDist)> 1e-5f || fDist1 > 1e-5f)
	{
		ZBREAK();
	}

//m mcreate(v3 vDir, v3 vRight, v3 vPoint);


//	pBsp->Occluders.Resize(1+nNumOccluders);
//	SOccluderPlane* pPlanes = pBsp->Occluders.Ptr();


	//FRUSTUM
	if(USE_FRUSTUM)
	{
		ZBREAK(); //todo retest this code
		v3 vFrustumCorners[4];
		SOccluderBspViewDesc Desc = pBsp->DescOrg;

		float fAngle = (Desc.fFovY * PI / 180.f)/2.f;
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
		SOccluderPlane& Plane = *pBsp->Occluders.PushBack();
		int nPrev = -1;
		for(uint32 i = 0; i < 4; ++i)
		{
			v3 v0 = vFrustumCorners[i];
			v3 v1 = vFrustumCorners[(i+1) % 4];
			v3 v2 = vLocalOrigin;
			v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
			v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
			v3 vEnd = vCenter + vNormal;
			Plane.p[i] = -(v4makeplane(vFrustumCorners[i], vNormal));
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
			v3 vCornersWorldSpace[4];
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
#if 1
	static float foo = 0.f;
	foo += 0.11f;
	float DBG = sin(foo);
	float DBG2 = cos(foo);
	v3 vDBG = v3rep(DBG) * 0.3f + 0.7f;
	v3 vDBG2 = v3rep(DBG2) * 0.3f + 0.7f;
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
			v3 vEyeToObject = pWorldObjects[i].mObjectToWorld.trans.tov3() - vWorldOrigin;
			float fDist = v3dot(vWorldDirection, vEyeToObject);
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
			//v3 pos = mat.trans.tov3();
//			ZDEBUG_DRAWBOX(mid(), pos, vDBG, 0xffff0000, 1);

			mat = mmult(mtobsp, mat);
//			pos = mtransform(pBsp->mfrombsp, mat.trans).tov3();
//			ZDEBUG_DRAWBOX(mid(), pos, vDBG2, 0xff00ff, 1);


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

			const v3 vToCenterX = vCenterX - vLocalOrigin;
			const v3 vToCenterY = vCenterY - vLocalOrigin;
			const v3 vToCenterZ = vCenterZ - vLocalOrigin;
			// uplotfnxt(" dot %f", v3dot(vY, vToCenterY));
			bool bFrontX = v3dot(vX, vToCenterX) < 0.f;
			bool bFrontY = v3dot(vY, vToCenterY) < 0.f;
			bool bFrontZ = v3dot(vZ, vToCenterZ) < 0.f;

			vX = bFrontX ? vX : -vX;
			vY = bFrontY ? vY : -vY;
			vZ = bFrontZ ? vZ : -vZ;
			// uplotfnxt("LALACenterY %f %f %f :: %f %f %f", vCenterY.x, vCenterY.y, vCenterY.z,
			// 	vCenterYNeg.x, vCenterYNeg.y, vCenterYNeg.z);

			//uplotfnxt("bNEG %d", bNeg ? 1 : 0);
			vCenterX = bFrontX ? vCenterX : vCenterXNeg;
			vCenterY = bFrontY ? vCenterY : vCenterYNeg;
			vCenterZ = bFrontZ ? vCenterZ : vCenterZNeg;
			// uplotfnxt("CenterY %f %f %f :: %f %f %f", vCenterY.x, vCenterY.y, vCenterY.z,
			// 	vY.x, vY.y, vY.z);

			BspAddOccluder(pBsp, vCenterY/**1.01*/, vY, vZ, vSize.z, vX, vSize.x,true);			
//			BspAddOccluder(pBsp, vCenterX/**1.01*/, vX, vY, vSize.y, vZ, vSize.z, true);
			g_EdgeMask = 7;
			BspAddOccluder(pBsp, vCenterZ/**1.01*/, vZ, vY, vSize.y, vX, vSize.x, true);
			g_EdgeMask = 0xff;

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
		// v4 vNormalPlane = v4makeplane(vCorners[0], vInnerNormal);
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
		// 	Plane.p[i] = v4makeplane(vCorners[i], vNormal);
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


	if(g_nBspDebugPlane < pBsp->Nodes.Size())
	{
		v4 vPlane = GET_PLANE(pBsp, pBsp->Nodes[g_nBspDebugPlane]);
		v3 vCorner = mtransform(pBsp->mfrombsp, pBsp->Nodes[g_nBspDebugPlane].vDebugCorner);
		v3 vPlaneDir = mrotate(pBsp->mfrombsp, vPlane.tov3());
		ZDEBUG_DRAWPLANE(vPlaneDir, vCorner, -1);
		v4 vColor0 = v4init(0.7,0.7,0.7,1);
		v4 vColor1 = v4init(1,0,0,1);
		int child = g_nBspDebugPlane;
		int nChildArray[20];

		int nCount = 0;
		//uprintf("Childs ");
		for(int i = 0; i < 20; ++i)
		{
			
			for(int j = 0;j < pBsp->Nodes.Size(); ++j)
			{
				SOccluderBspNode& N = pBsp->Nodes[j];
				if(N.nInside == child || N.nOutside == child)
				{
					nChildArray[nCount++] = j; 
					//uprintf("%d ", j);
					child = j;
					break;
				}
			}
		}
		//uprintf("\n");
		for(int i = 0; i < nCount; ++i)
		{
			float f = float(i) / (nCount-1);
			v4 vColor = v4lerp(vColor0, vColor1, f);
			int idx = nChildArray[i];
			v4 vPlane = GET_PLANE(pBsp, pBsp->Nodes[idx]);
			v3 vCornerWorld = mtransform(pBsp->mfrombsp, pBsp->Nodes[idx].vDebugCorner);
			v3 vPlaneWorld = mrotate(pBsp->mfrombsp, vPlane.tov3());
			ZDEBUG_DRAWPLANE(vPlane.tov3(), vCornerWorld, vColor.tocolor());

		}
	}


	g_nCheck = 1;
	if(g_KeyboardState.keys[SDL_SCANCODE_F10] & BUTTON_RELEASED)
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
	uint32_t nEdgeCount = 0;
	for(uint32 i = 0; i < 4; ++i)
	{
		if( (1<<i) & g_EdgeMask)
		{
			Poly[i].nOccluderIndex = nOccluderIndex;
			Poly[i].nEdge = i;
			nEdgeCount++;
		}
	}
	Poly[nEdgeCount].nOccluderIndex = nOccluderIndex;
	Poly[nEdgeCount].nEdge = 4;
	nEdgeCount++;

	uint32 nNumNodes = (uint32)pBsp->Nodes.Size();
	if(pBsp->Nodes.Empty())
	{
		uint16 nIndex = (uint16)BspAddInternal(pBsp, &Poly[0], nEdgeCount, 1);
		ZASSERT(nIndex==0);
		if(g_nBspOccluderDrawOccluders)
		{
			if(g_nDrawCount++ == g_nDebugDrawPoly)
			{
				BspDrawPoly2(pBsp, &Poly[0], nEdgeCount, 0x88000000 | randcolor(), 0xffff0000, nIndex);
			}

		}


	}
	else
	{
		BspAddOccluderRecursive(pBsp, 0, &Poly[0], nEdgeCount, 1);
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
	v3 pos = mtransform(pBsp->mfrombsp, vCorners[0]);
	ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);
	pos = mtransform(pBsp->mfrombsp, vCorners[1]);
	ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);
	pos = mtransform(pBsp->mfrombsp, vCorners[2]);
	ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);
	pos = mtransform(pBsp->mfrombsp, vCorners[3]);
	ZDEBUG_DRAWBOX(mid(), pos, v3rep(0.03f), 0xff00ff, 1);


//	const v3 vNormal = v3cross(vCorners[0] - vCorners[1], vCorners[2] - vCorners[0]);
//	const v3 vCenter = (vCorners[0] + vCorners[1] + vCorners[2] + vCorners[3]) * 0.25f;
	const v3 vToCenter = vCenter;//- pBsp->vLocalOrigin;
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
	const v3 vOrigin = v3zero();
	const v3 vDirection = v3init(0,0,-1);//vLocalDirection

	uint32 nIndex = pBsp->Occluders.Size();
	SOccluderPlane* pPlane = pBsp->Occluders.PushBack();
	v3 vInnerNormal = v3normalize(v3cross(vCorners[1]-vCorners[0], vCorners[3]-vCorners[0]));
	bool bFlip = false;
	v4 vNormalPlane = v4makeplane(vCorners[0], vInnerNormal);
	{
		float fMinDot = v3dot(vCorners[0]-vOrigin, vDirection);
		fMinDot = Min(fMinDot, v3dot(vCorners[1]-vOrigin, vDirection));
		fMinDot = Min(fMinDot, v3dot(vCorners[2]-vOrigin, vDirection));
		fMinDot = Min(fMinDot, v3dot(vCorners[3]-vOrigin, vDirection));
		//project origin onto plane
		float fDist = v4dot(vNormalPlane, v4init(vOrigin, 1.f));
		if(fabs(fDist) < pBsp->DescOrg.fZNear)//if plane is very close to camera origin, discard to avoid intersection with near plane
		{
			if(fMinDot < 1.5 * pBsp->DescOrg.fZNear) // _unless_ all 4 points clearly in front
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
		pPlane->p[i] = v4makeplane(vCorners[i], vNormal);
		if(bFlip)
			pPlane->p[i] = -pPlane->p[i];
		if(g_nBspOccluderDrawEdges&1)
		{
			v0 = mtransform(pBsp->mfrombsp, v0);
			v1 = mtransform(pBsp->mfrombsp, v1);
			v2 = mtransform(pBsp->mfrombsp, v2);
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
		vCorners[i] = mtransform(pBsp->mfrombsp, BspPlaneIntersection(v0, v1, vNormalPlane));
		if(1)
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


//only used for debug, so its okay to be slow
int BspFindParent(SOccluderBsp* pBsp, int nChildIndex)
{
	for(int i = 0; i < pBsp->Nodes.Size(); ++i)
	{
		if(pBsp->Nodes[i].nInside == nChildIndex || pBsp->Nodes[i].nOutside == nChildIndex)
			return i;
	}
	return -1;

}

int BspFindParent(SOccluderBsp* pBsp, int nChildIndex, bool& bInside)
{
	for(int i = 0; i < pBsp->Nodes.Size(); ++i)
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


	return v3init(p.x, p.y, p.w + p.z);
}

float BspKryds2d(v3 p0, v3 p1)
{
	return p0.x * p1.y - p1.x * p0.y;
}


bool BspPlaneTest(v4 plane0, v4 plane1, v4 ptest, bool bParrallel, bool bDebug = false)
{
	v3 p2d0 = BspToPlane2(plane0);
	v3 p2d1 = BspToPlane2(plane1);
	v3 p2dtest = BspToPlane2(ptest);

	v2 p1 = p2d0.tov2() * -p2d0.z;
	v2 p2 = v2hat(p2d0.tov2()) + p1;
	v2 p3 = p2d1.tov2() * -p2d1.z;
	v2 p4 = v2hat(p2d1.tov2()) + p3;

	ZASSERT(abs(v3dot(v3init(p1, 1.f), p2d0))<0.0001f);
	ZASSERT(abs(v3dot(v3init(p2, 1.f), p2d0))<0.0001f);
	ZASSERT(abs(v3dot(v3init(p3, 1.f), p2d1))<0.0001f);
	ZASSERT(abs(v3dot(v3init(p4, 1.f), p2d1))<0.0001f);

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
	float fDot = v3dot( v3init(numx, numy, denom), p2dtest);
	bool bRet1 = denom * fDot >= 0.f;
	// bool bRet2 = v3dot( v3init(numx, numy, denom), p2dtest) <= 0.f;
	// if(denom < 0.f)
	// 	bRet2 = !bRet2;
	// ZASSERT(0.f == denom || bRet1 == bRet2);
	bParrallel = fDot * denom == 0.f;
	if(bDebug)
	{
		uplotfnxt("plane test dot is %f --> %d par %d .. unsigned %f .. %f\n", denom * fDot, bRet1, bParrallel, denom, fDot);
	}


	return bRet1 || bParrallel;
}
void BspDrawPoly2(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nVertices, uint32 nColorPoly, uint32 nColorEdges, int nIndex)
{
	uint32 nPolyEdges = nVertices-1;
	v3* vCorners = (v3*)alloca(sizeof(v3)*MAX_POLY_PLANES);
	v4* vPlanes = (v4*)alloca(sizeof(v4)*MAX_POLY_PLANES);
	ZASSERT(Poly[nPolyEdges].nEdge == 4);
	v4 vNormalPlane = pBsp->Occluders[ Poly[nPolyEdges].nOccluderIndex ].p[Poly[nPolyEdges].nEdge];

	int nPlaneIndex = 0;
	for(int i = 0; i<nPolyEdges; i++)
	{
		if(!Poly[i].nSkip)//todo check if !nSkip is used anywhere
		{
			vPlanes[nPlaneIndex++] = GET_PLANE(pBsp, Poly[i]);
			if(nPlaneIndex>1)
			{
				v4 vPlane = vPlanes[nPlaneIndex-1];
				v4 vPrevPlane = vPlanes[nPlaneIndex-2];
				v3 vIntersect = BspPlaneIntersection(vPrevPlane, vPlane, vNormalPlane);
				v3 vDir = vPlane.tov3();
				vDir = mrotate(pBsp->mfrombsp, vDir);
				vIntersect = mtransform(pBsp->mfrombsp, vIntersect);


				ZDEBUG_DRAWPLANE(vDir, vIntersect, 0xffffff);


			}
		}
	}
	uplotfnxt("START PLANES %d", nPlaneIndex);
	v3 vCross; 
	bool bCrossSet = false;
	static float lala = 0;
	lala += 0.03f;
	bool bInside = false;
	for(int nParent = BspFindParent(pBsp, nIndex, bInside); nParent != -1; nParent = BspFindParent(pBsp, nParent, bInside))
	{
		if(pBsp->Nodes[nParent].nInside == OCCLUDER_LEAF )
			continue; //not part of the poly
		v4 vPlane = GET_PLANE(pBsp, pBsp->Nodes[nParent]);
		if(!bInside) vPlane = v4neg(vPlane);
	
		{
			v4 vPrevPlane = vPlanes[nPlaneIndex-1];
			v3 vIntersect = BspPlaneIntersection(vPrevPlane, vPlane, vNormalPlane);
			v3 vDir = vPlane.tov3();
			vDir = mrotate(pBsp->mfrombsp, vDir);
			vIntersect = mtransform(pBsp->mfrombsp, vIntersect);


			//ZDEBUG_DRAWPLANE(vDir, vIntersect, 0xff00ff);


		}
		#if 1
		v3 p = BspToPlane2(vPlane);
		int i = 0;
		for( i = 0; i < nPlaneIndex; ++i)
		{
			v3 pInner = BspToPlane2(vPlanes[i]);
			float fHat = p.x * pInner.y - p.y * pInner.x;
			if(fHat <= 0)
			{
				break;
			}
		}
		for(int j = nPlaneIndex; j > i; --j)
			vPlanes[j] = vPlanes[j-1];
		vPlanes[i] = vPlane;
		nPlaneIndex++;
		#endif
		// vPlanes[nPlaneIndex++ ] = vPlane;
	}
	uplotfnxt("PLANES AFTER %d", nPlaneIndex);
// static float lala = 0;
// lala += 0.03f;
	float foooo = sin(lala);



	// for(int i = 0; i < nPlaneIndex; ++i)
	// {
	// 	uint32 i0 = i;
	// 	uint32 i1 = (i+1)%nPlaneIndex;
	// 	v4 v0 = vPlanes[i0];
	// 	v4 v1 = vPlanes[i1];
	// 	vCorners[i] = BspPlaneIntersection(v0, v1, vNormalPlane);
	// 	vCorners[i] = mtransform(pBsp->mfrombsp, vCorners[i]);
	// 	v3 vPlaneDir = mrotate(pBsp->mfrombsp, v0.tov3());
	// 	//ZDEBUG_DRAWBOX(mid(), vCorners[i], v3rep(0.03f*foooo), 0xffff0000, 1);
	// 	float fColor = float(i) / (nPlaneIndex-1);
	// 	v3 vColor = v3rep(fColor);
	// 	ZDEBUG_DRAWPLANE(vPlaneDir, vCorners[i], vColor.tocolor());
	// }


//#define DEBUG_PLANE_LOOP

#ifdef DEBUG_PLANE_LOOP
	static int nDebugIndex = -1;
	static int inc = 1;
	static float fOffset = 0.f;
	if(g_KeyboardState.keys[SDL_SCANCODE_K]&BUTTON_RELEASED)
	{
		inc = !inc;
		nDebugIndex += 1;
	}
	if(g_KeyboardState.keys[SDL_SCANCODE_N]&BUTTON_RELEASED)
	{
		fOffset += 0.1f;
	}
	if(g_KeyboardState.keys[SDL_SCANCODE_M]&BUTTON_RELEASED)
	{
		fOffset -= 0.1f;
	}

	int nView = (nDebugIndex) % nPlaneIndex;

	uplotfnxt("DEBUG INDEX %d offset %f", nView, fOffset);
	if(nDebugIndex > 0){
		for(int i = 0; i < nPlaneIndex;++i)
		{
			v4 plane = vPlanes[i];
			if(fabs(fOffset) > 0.05f)
			{
				plane.w += fOffset;
			}
			v4 before = vPlanes[(i+nPlaneIndex-1)%nPlaneIndex];
			v4 after = vPlanes[(i+1)%nPlaneIndex];

			if(nView == i)
			{
				v3 plane2d = BspToPlane2(plane);
				v3 before2d = BspToPlane2(before);
				v3 after2d = BspToPlane2(after);
				float dot1 = plane2d.x * before2d.x + plane2d.y * before2d.y;
				float dot2 = plane2d.x * after2d.x + plane2d.y * after2d.y;
				float v1 = acos(dot1);
				float v2 = acos(dot2);
				float kryds1 = BspKryds2d(before2d, plane2d);
				float kryds2 = BspKryds2d(plane2d, after2d);
				float fDotSum = dot1 + dot2;

				float fVinkelSum = v1 + v2;
				uplotfnxt("VINKELSUM %f ... %f %f ", fVinkelSum * 180 / PI, v1 * 180 / PI, v2 * 180 / PI);
				uplotfnxt("KRYDS %f %f", kryds1, kryds2);
				uplotfnxt("DOT %f %f %f", dot1, dot2, fDotSum);
				
				//if(


				v3 vCorner0 = mtransform(pBsp->mfrombsp, BspPlaneIntersection(before, plane, vNormalPlane));
				v3 vCorner1 = mtransform(pBsp->mfrombsp, BspPlaneIntersection(plane, after, vNormalPlane));
				v3 plane_world = mrotate(pBsp->mfrombsp, plane.tov3());
				v3 before_world = mrotate(pBsp->mfrombsp, before.tov3());
				v3 after_world = mrotate(pBsp->mfrombsp, after.tov3());

				bool p;
				bool planeTest = BspPlaneTest(before, after, plane, p, true);
				uplotfnxt("plane test result %d", planeTest);


				ZDEBUG_DRAWPLANE(before_world, vCorner0, -1);
				ZDEBUG_DRAWPLANE(plane_world, vCorner1, 0);
				ZDEBUG_DRAWPLANE(after_world, vCorner1, -1);



			}
		}
	}
#endif


	if(nPlaneIndex > 3)
	{
		int nNumRemoved = 0;
		do
		{
			nNumRemoved = 0;
			for(int i = 0; i < nPlaneIndex;)
			{
				v4 plane = vPlanes[i];
				v4 before = vPlanes[(i+nPlaneIndex-1)%nPlaneIndex];
				v4 after = vPlanes[(i+1)%nPlaneIndex];

				bool p;
				bool bPlaneTest = BspPlaneTest(before, after, plane, p);

				v3 plane2d = BspToPlane2(plane);
				v3 before2d = BspToPlane2(before);
				v3 after2d = BspToPlane2(after);
				float dot1 = plane2d.x * before2d.x + plane2d.y * before2d.y;
				float dot2 = plane2d.x * after2d.x + plane2d.y * after2d.y;
				float fDotSum = dot1 + dot2;


				if(fDotSum > 0.f && !bPlaneTest)
				{
					memmove(&vPlanes[i], &vPlanes[i+1], (nPlaneIndex - (i+1)) * sizeof(vPlanes[0]));
					--nPlaneIndex;
					++nNumRemoved;
					uplotfnxt("remove plane .. %d", i);
				}
				else
				{
					++i;
				}
			}

		}while(nNumRemoved && nPlaneIndex > 3);
	}

	for(int i = 0; i < nPlaneIndex; ++i)
	{
		uint32 i0 = i;
		uint32 i1 = (i+1)%nPlaneIndex;
		v4 v0 = vPlanes[i0];
		v4 v1 = vPlanes[i1];
		vCorners[i] = BspPlaneIntersection(v0, v1, vNormalPlane);
		vCorners[i] = mtransform(pBsp->mfrombsp, vCorners[i]);
		v3 vPlaneDir = mrotate(pBsp->mfrombsp, v0.tov3());
		ZDEBUG_DRAWBOX(mid(), vCorners[i], v3rep(0.03f*foooo), 0xffff0000, 1);
		float fColor = float(i) / (nPlaneIndex-1);
		v3 vColor = v3rep(fColor);
		ZDEBUG_DRAWPLANE(vPlaneDir, vCorners[i], vColor.tocolor());
	}


#if 0
	// for(int i = 0; i < nPlaneIndex; ++i)
	// {
	// 	uint32 i0 = i;
	// 	uint32 i1 = (i+1)%nPlaneIndex;
	// 	v4 v0 = vPlanes[i0];
	// 	v4 v1 = vPlanes[i1];
	// 	vCorners[i] = BspPlaneIntersection(v0, v1, vNormalPlane);
	// 	vCorners[i] = mtransform(pBsp->mfrombsp, vCorners[i]);
	// 	ZDEBUG_DRAWBOX(mid(), vCorners[i], v3rep(0.01f), 0xffff0000, 1);
	// 	ZDEBUG_DRAWPLANE(v0.tov3(), vCorners[i], 0xff0000);
	// }
	ZASSERT(nPlaneIndex >= 3);
	if(nPlaneIndex)
	{
		v3 foo = v3init(0.01f, 0.01f, 0.01f);
		for(int i = 0; i < nPlaneIndex; ++i)
		{
			ZDEBUG_DRAWLINE(vCorners[i], vCorners[(i+1)%nPlaneIndex], nColorEdges, true);
		}
		ZDEBUG_DRAWPOLY(&vCorners[0], nPlaneIndex, nColorPoly);

	}
#endif
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
			pNode->vDebugCorner = BspGetCorner(pBsp, Poly, nEdges, i < nEdges - 1 ? i : i-1);
			if(pNode->vDebugCorner.y > 10000)
			{
				uprintf("vDebugCorner %f %f %f\n", 
					pNode->vDebugCorner.x,
					pNode->vDebugCorner.y,
					pNode->vDebugCorner.z);
				pNode->vDebugCorner = BspGetCorner(pBsp, Poly, nEdges, i < nEdges - 1 ? i : i-1);
	
			}




			// if(g_nBspDebugPlane == g_nBspDebugPlaneCounter++)
			// {
			// 	v4 vPlane = GET_PLANE(pBsp, Poly[i]);
			// 	uplotfnxt("DEBUG PLANE %d :: %d, flip %d", Poly[i].nOccluderIndex, Poly[i].nEdge, Poly[i].nFlip);
				
			// 	//BspDebugDrawHalfspace(vPlane.tov3(), vCorner);
			// 	ZDEBUG_DRAWPLANE(vPlane.tov3(), vCorner, 0xff00ff00);
			// }
		}
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
//		uplotfnxt("REJECT %f", fDistLort);
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
			// if(fCrossLength0 < 0.3f || fCrossLength < 0.3f)
			// 	uplotfnxt("LEN IS %f %f", fCrossLength0, fCrossLength);



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
			if(g_nBspOccluderDrawOccluders)
			{
				if(g_nDrawCount++ == g_nDebugDrawPoly)
				{
					uplotfnxt("add %d   parent %d index %d", g_nDebugDrawPoly, nBspIndex, nIndex);
					BspDrawPoly2(pBsp, pPolyOutside, nPolyEdgeOut, 0x88000000 | randcolor(), 0xffff0000, nIndex);
				}

			}

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
			//uplotfnxt("FAIL OUTSIDE %d :: %d,%d", Index, Node.nOccluderIndex, Node.nEdge);

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
		m mObjectToBsp = mmult(pBsp->mtobsp, mObjectToWorld);
		v3 vHalfSize = pObject->vSize;
		v4 vCenterWorld_ = mtransform(pBsp->mtobsp, pObject->mObjectToWorld.trans);
		v4 vCenterWorld_1 = mObjectToBsp.trans;
		v3 vCenterWorld = v3init(vCenterWorld_);
		v3 vCenterWorld1 = v3init(vCenterWorld_1);
		//uprintf("distance is %f\n", v3distance(vCenterWorld1, vCenterWorld));
		ZASSERT(abs(v3distance(vCenterWorld1, vCenterWorld)) < 1e-3f);
		v3 vOrigin = v3zero(); //vLocalOrigin
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
		pPlanes[0].p[0] = v4makeplane(v0, n0);
		pPlanes[0].p[1] = v4makeplane(v1, n1);
		pPlanes[0].p[2] = v4makeplane(v2, n2);
		pPlanes[0].p[3] = v4makeplane(p3, n3);
		pPlanes[0].p[4] = v4makeplane(p3, v3normalize(vToCenter));



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








