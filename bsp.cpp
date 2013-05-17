#include "base.h"
#include "math.h"
#include "fixedarray.h"
#include "text.h"
#include "program.h"
#include "input.h"
#include "bsp.h"


#define OCCLUDER_EMPTY (0xc000)
#define OCCLUDER_LEAF (0x8000)
#define OCCLUDER_CLIP_MAX 0x100


uint32 g_nOccluderDebug=1; //0:nothing, 1:show non clipped, 2: show all non rejected steps
uint32 g_nBspOccluderDebugDraw = 1;

struct SOccluderPlane;
struct SBspEdgeIndex;
struct SOccluderBspNode;
struct SOccluderPlane
{
	v4 p[5];
	//v4 normal;
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
void BspOccluderDebugDraw(v4* pVertexNew, v4* pVertexIn, uint32 nColor, uint32 nFill = 0) ;
void BspAddOccluder(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex);
int BspAddInternal(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nVertices, uint32 nLevel);
void BspAddOccluderRecursive(SOccluderBsp* pBsp, uint32 nBspIndex, SBspEdgeIndex* Poly, uint32 nEdges, uint32 nLevel);

SBspEdgeIndex::SBspEdgeIndex(const SOccluderBspNode& node)
:nOccluderIndex(node.nOccluderIndex)
,nEdge(node.nEdge)
,nFlip(node.nFlip)
,nSkip(0)
{
}

v3 BspPlaneIntersection(v4 p0, v4 p1, v4 p2)
{

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


/*
	dot(p0, (1, y, z, 1)) = 0
	p0.x + p0.w + y * p0.y + z * p0.z = 0
	z * p0.z = -(p0.x + p0.w + y * p0.y)
	z = -(p0.x + p0.w + y * p0.y) / p0.z

	p1.x + p1.w + y * p1.y + z * p1.z = 0
	solve p0.x + p0.w + y * p0.y + z * p0.z = 0, p1.x + p1.w + y * p1.y + z * p1.z = 0 for x,y

	solve x0 + w0 + y * y0 + z * z0 = 0, x2 + w2 + y * y2 + z * z2 = 0 for z,y
	y = (-p1.x + p1.w + (-(p0.x + p0.w + y * p0.y) / p0.z) * p1.z) / p1.y


	dot(p0, (x, y, z, 1)) = 0
	dot(p1, (x, y, z, 1)) = 0
	dot(p2, (x, y, z, 1)) = 0
	x0 * x + y0 + y + z0 * z + w0 = 0
	x1 * x + y1 + y + z1 * z + w1 = 0
	x2 * x + y2 + y + z2 * z + w2 = 0


	solve x0 * x + y0 + y + z0 * z + f0 = 0, x1 * x + y1 + y + z1 * z + f1 = 0, x2 * x + y2 + y + z2 * z + f2 = 0, for x,y,z


*/




uint32 g_nBspFlipMask = 0;
uint32 g_nBspDrawMask = -1;

#define DEBUG_OFFSET 0.4f

float g_OFFSET = DEBUG_OFFSET;
v3 g_Offset = v3init(1,0,0);
v3 g_DepthOffset = v3init(0.4f, 0.f, 0.f);
v3 g_XOffset = v3init(0.0f, 0.0f, 0.f);
const char* Spaces(int n)
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
void BspBuild(SOccluderBsp* pBsp, SOccluder* pOccluders, uint32 nNumOccluders, const SOccluderBspViewDesc& Desc)
{
	if(g_KeyboardState.keys[SDLK_F3]&BUTTON_RELEASED)
	{
		g_nOccluderDebug = (g_nOccluderDebug+1)%3;
	}

	if(g_KeyboardState.keys[SDLK_F4]&BUTTON_RELEASED)
	{
		g_nBspOccluderDebugDraw = (g_nBspOccluderDebugDraw+1)%3;
	}


	long seed = rand();
	srand(42);

	pBsp->nDepth = 0;
	pBsp->Nodes.Clear();
	pBsp->Occluders.Clear();
	pBsp->Desc = Desc;
	v3 vOrigin = pBsp->Desc.vOrigin;
	pBsp->Occluders.Resize(nNumOccluders);
	SOccluderPlane* pPlanes = pBsp->Occluders.Ptr();
	for(uint32 i = 0; i < nNumOccluders; ++i)
	{
		v3 vCorners[4];
		SOccluder Occ = pOccluders[i];
		v3 vNormal = Occ.mObjectToWorld.z.tov3();
		v3 vUp = Occ.mObjectToWorld.y.tov3();
		v3 vLeft = v3cross(vNormal, vUp);
		v3 vCenter = Occ.mObjectToWorld.w.tov3();
		vCorners[0] = vCenter + vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[1] = vCenter - vUp * Occ.vSize.y + vLeft * Occ.vSize.x;
		vCorners[2] = vCenter - vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
		vCorners[3] = vCenter + vUp * Occ.vSize.y - vLeft * Occ.vSize.x;
		SOccluderPlane& Plane = pPlanes[i];

		for(uint32 i = 0; i < 4; ++i)
		{
			v3 v0 = vCorners[i];
			v3 v1 = vCorners[(i+1) % 4];
			v3 v2 = vOrigin;
			v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
			v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
			v3 vEnd = vCenter + vNormal;
			Plane.p[i] = MakePlane(vCorners[i], vNormal);
			ZDEBUG_DRAWLINE(v0, v1, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v1, v2, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v2, v0, (uint32)-1, true);
		}
		Plane.p[4] = MakePlane(vCorners[0], vNormal);
	}

	//TOOD: add per thread block in occluder array

	pBsp->Nodes.Clear();
	if(!pBsp->Occluders.Size())
		return;


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




		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[0], 0, true);
		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[1], 0, true);
		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[2], 0, true);
		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[3], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[0], vFrustumCorners[1], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[1], vFrustumCorners[2], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[2], vFrustumCorners[3], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[3], vFrustumCorners[0], 0, true);
	}





	for(uint32 i = 0; i < pBsp->Occluders.Size(); ++i)
	{
		BspAddOccluder(pBsp, &pBsp->Occluders[i], i);

		for(uint j = 0; j < 4; ++j)
		{
			v4 p0 = pBsp->Occluders[i].p[j];
			v4 p1 = pBsp->Occluders[i].p[(j+1)%4];
			v4 vNormal = pBsp->Occluders[i].p[4];
			v3 vIntersect = BspPlaneIntersection(p0,p1, vNormal);
			ZASSERT(v3length(vIntersect) > 0.001f);
			ZDEBUG_DRAWBOX(mid(), vIntersect, v3rep(0.01f), 0xffff0000);


		}
	}



	if(g_KeyboardState.keys[SDLK_F10] & BUTTON_RELEASED)
	{
		BspDump(pBsp, 0, 0);
	}

	uplotfnxt("BSP: NODES[%03d] OCCLUDERS[%03d] DEPTH[%03d]", pBsp->Nodes.Size(), pBsp->Occluders.Size(), pBsp->nDepth);
	srand(seed);
}


void BspOccluderDebugDraw(v4* pVertexNew, v4* pVertexIn, uint32 nColor, uint32 nFill)
{
	//if(nFill)
	{
		ZDEBUG_DRAWPOLY(pVertexNew, pVertexIn - pVertexNew, nColor);
		// v4* last = pVertexIn-1;
		// for(v4* v = pVertexNew; v < pVertexIn; ++v)
		// {
		// 	v3 v0 = v[0].tov3() + v3init(-0.01f,0,0);
		// 	v3 v1 = (*last).tov3()+ v3init(-0.01f,0,0);
		// 	last = v;
		// 	ZDEBUG_DRAWLINE(v0, v1, nColor,true);
		// }
	}
	//else
	{
		v4* last = pVertexIn-1;
		for(v4* v = pVertexNew; v < pVertexIn; ++v)
		{
			v3 v0 = v[0].tov3() + v3init(-0.01f,0,0);
			v3 v1 = (*last).tov3()+ v3init(-0.01f,0,0);
			last = v;
			ZDEBUG_DRAWLINE(v0, v1, nColor,true);
		}

	}
}



void BspAddOccluder(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex)
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
///						ZDEBUG_DRAWBOX(mid(), vIntersect, v3rep(0.01f), 0xffff0000);
			ZDEBUG_DRAWBOX(mid(), vCorners[i], v3rep(0.01f), 0xffff0000);
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


int BspAddInternal(SOccluderBsp* pBsp, SBspEdgeIndex* Poly, uint32 nVertices, uint32 nLevel)
{
	int r = (int)pBsp->Nodes.Size();
	int nPrev = -1;
	if(nLevel + nVertices > pBsp->nDepth)
		pBsp->nDepth = nLevel + nVertices;
	for(uint32 i = 0; i < nVertices; ++i)
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
		}
	}

	BspDrawPoly(pBsp, Poly, nVertices, 0x88000000 | randcolor(), 0xffff0000);

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


uint32 BspClipPoly(SOccluderBsp* pBsp, SBspEdgeIndex PlaneIndex, SBspEdgeIndex* Poly, uint32 nEdges, SBspEdgeIndex* pPolyOut)
{
	v4 vPlane = GET_PLANE(pBsp, PlaneIndex);
	v3* vCorners = (v3*)alloca(nEdges * sizeof(v3));
	bool* bBehindCorner = (bool*)alloca(nEdges);
	ZASSERT(0 == (0x3&(uintptr)vCorners));
	uint32 nRealEdges = nEdges-1;
	ZASSERT(Poly[nRealEdges].nEdge == 4);
	v4 vNormalPlane = pBsp->Occluders[ Poly[nRealEdges].nOccluderIndex ].p[4];
	for(uint32 i = 0; i < nRealEdges; ++i)
	{
		uint32 i0 = i;
		uint32 i1 = (i+nRealEdges-1)%nRealEdges;
		v4 v0 = GET_PLANE(pBsp, Poly[i0]);
		v4 v1 = GET_PLANE(pBsp, Poly[i1]);
		vCorners[i] = BspPlaneIntersection(v0, v1, vNormalPlane);
		bBehindCorner[i] = v4dot(v4init(vCorners[i],1.f), vPlane) >= 0.f;
	}
	// uplotfnxt("bBehind %02d..%02d   :: %d %d %d %d", PlaneIndex.nOccluderIndex, PlaneIndex.nEdge,
	// 	bBehindCorner[0]?1:0,
	// 	bBehindCorner[1]?1:0, 
	// 	bBehindCorner[2]?1:0, 
	// 	bBehindCorner[3]?1:0);
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
	pPolyOut[nOutIndex++] = Poly[nRealEdges]; //add normal`
	return nOutIndex;
}


ClipPolyResult BspClipPoly(SOccluderBsp* pBsp, SBspEdgeIndex ClipPlane, SBspEdgeIndex* Poly, uint32 nEdges, SBspEdgeIndex* pPolyOut, uint32& nEdgesIn, uint32& nEdgesOut)
{
	uint32 nIn = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, pPolyOut);
	if(nIn == 1) 
		nIn = 0;
	ClipPlane.nFlip ^= 1;
	uint32 nOut = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, pPolyOut + nIn);
	if(nOut == 1) 
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
	SBspEdgeIndex* pPolyInside = ECPR_BOTH == CR ? &ClippedPoly[0] : Poly;
	uint32 nPolyEdgeIn = ECPR_BOTH == CR ? nNumEdgeIn : nEdges;
	SBspEdgeIndex* pPolyOutside= ECPR_BOTH == CR ? &ClippedPoly[nNumEdgeIn] : Poly;
	uint32 nPolyEdgeOut = ECPR_BOTH == CR ? nNumEdgeOut : nEdges;

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
			ZASSERT(Node.nEdge == 4);
			//culled
		}
		else if(!BspCullObjectR(pBsp, Node.nInside, pIn, nIn))
		{
			if(!g_nBspOccluderDebugDraw)
				return false;
			bFail = true;
		//	uplotfnxt("FAIL INSIDE");
		}
	}
	if(CR & ECPR_OUTSIDE)
	{
		if(g_nBspOccluderDebugDraw && Node.nOutside == OCCLUDER_EMPTY)
		{
			BspDrawPoly(pBsp, pOut, nOut, 0xff000000|randredcolor(), 0, 1);
		}

		ZASSERT(Node.nOutside != OCCLUDER_LEAF);
		if((Node.nOutside == OCCLUDER_EMPTY || !BspCullObjectR(pBsp, Node.nOutside, pOut, nOut)))
		{
			//uplotfnxt("FAIL OUTSIDE");
			return false;
		}

	}
	return !bFail;
}

bool BspCullObject(SOccluderBsp* pBsp, SWorldObject* pObject)
{
	long seed = rand();
	srand((int)(uintptr)pObject);

	m mObjectToWorld = pObject->mObjectToWorld;
	v3 vHalfSize = pObject->vSize;
	v4 vCenterWorld_ = pObject->mObjectToWorld.trans;
	v3 vCenterWorld = v3init(vCenterWorld_);
	v3 vOrigin = pBsp->Desc.vOrigin;
	v3 vToCenter = v3normalize(vCenterWorld - vOrigin);
	v3 vUp = v3init(0.f,1.f, 0.f);//replace with camera up.
	v3 vRight = v3normalize(v3cross(vToCenter, vUp));
	m mbox = mcreate(vToCenter, vRight, vCenterWorld);
	m mboxinv = maffineinverse(mbox);
	m mcomposite = mmult(mbox, mObjectToWorld);
	v3 AABB = obbtoaabb(mcomposite, vHalfSize);
	
	vRight = mgetxaxis(mboxinv);
	vUp = mgetyaxis(mboxinv);

	v3 vCenterQuad = vCenterWorld - vToCenter * AABB.z;
	v3 v0 = vCenterQuad + vRight * AABB.x + vUp * AABB.y;
	v3 v1 = vCenterQuad + vRight * -AABB.x + vUp * AABB.y;
	v3 v2 = vCenterQuad + vRight * -AABB.x + vUp * -AABB.y;
	v3 p3 = vCenterQuad + vRight * AABB.x + vUp * -AABB.y;


	ZDEBUG_DRAWLINE(v0, v1, 0xff00ff00, true);
	ZDEBUG_DRAWLINE(v2, v1, 0xff00ff00, true);
	ZDEBUG_DRAWLINE(p3, v2, 0xff00ff00, true);
	ZDEBUG_DRAWLINE(v0, p3, 0xff00ff00, true);

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
	uint32 nSize = pBsp->Occluders.Size();
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


	SBspEdgeIndex Poly[5];
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

	bool bResult = BspCullObjectR(pBsp, 0, &Poly[0], 5);
	pBsp->Occluders.Resize(nSize);
	srand(seed);

	return bResult;
}

