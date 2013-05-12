#include "base.h"
#include "math.h"
#include "fixedarray.h"
#include "text.h"
#include "program.h"
#include "input.h"


#define OCCLUDER_EMPTY (0xc000)
#define OCCLUDER_LEAF (0x8000)
#define OCCLUDER_CLIP_MAX 0x100

//TODO: 
// object bounds.
//


uint32 g_nOccluderDebug=1; //0:nothing, 1:show non clipped, 2: show all non rejected steps
uint32 g_nOccluderClipLevels = -1;
uint32 g_nUseNew = 1;


struct SOccluderPlane
{
	v4 p[5];
	v3 corners[4];
	v4 normal;
};

struct SOccluderEdgeIndex
{
	uint16 	nOccluderIndex;
	uint8 	nEdge : 4;	//[0-3] edge idx // 5 is normal
	uint8 	nFlip : 1;
	uint8 	nSkip : 1;
//	uint8 	nNormal : 1;
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
	uint32 nDepth;
	TFixedArray<SOccluderPlane, 1024> Occluders;
	TFixedArray<SOccluderBspNode, 1024> Nodes;
};
void BspOccluderDebugDraw(v4* pVertexNew, v4* pVertexIn, uint32 nColor, uint32 nFill = 0) ;
void BspAddOccluder2(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex);


v3 BspPlaneIntersection(v4 p0, v4 p1)
{
	if(fabs(p0.z) < 1e-5)
		Swap(p0, p1);
	float denominator = p1.y * p0.z - p0.y * p1.z;
	if(fabs(denominator) < 1e-6)
		return v3zero();
	float rden = 1.f / denominator;
	ZASSERT(fabs(p0.z) >= 1e-5);
	float numeratory = -p1.w * p0.z + p0.w * p1.z + p0.x * p1.z - p1.x * p0.z;
	float numeratorz = p1.w * p0.y - p0.w * p1.y - p0.x * p1.y + p1.x * p0.y;
	return v3init(1,numeratory * rden, numeratorz * rden);

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

int BspAddInternal(SOccluderBsp* pBsp, uint32 nOccluderIndex, uint32 nMask, uint32 nLevel)
{
	int r = (int)pBsp->Nodes.Size();
	int nPrev = -1;
	if(nLevel + 4 > pBsp->nDepth)
		pBsp->nDepth = nLevel + 4;
	for(uint32 i = 0;nMask && i < 4; ++i)
	{
		uint32 nNextMask = nMask >> 1;
		if(nMask&1)
		{
			int nIndex = (int)pBsp->Nodes.Size();
			SOccluderBspNode* pNode = pBsp->Nodes.PushBack();
			pNode->nOutside = OCCLUDER_EMPTY;
			pNode->nInside = OCCLUDER_LEAF;
			pNode->nEdge = (uint8)i;
			pNode->nOccluderIndex = (uint16)nOccluderIndex;
			pNode->bLeaf = nNextMask == 0;
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

// void BspAddOccluderRecursive(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex, uint32 nBspIndex, uint32 nEdgeMask, int nLevel)
// {
	/*
		OccluderNode n = pBsp[nBspIndex];
		plane a = n.plane
		OccluderPoly Inside = ClipInside(a, pOccluder)
		OccluderPoly Outside = ClipRight(a, pOccluder)
		if(!Empty(Inside))
		{
			bool bOccluded = true;
			if(!n.bIsLeaf || !OccluderBehindZ(Inside))
			{
				if(n.inside == LEAF)
				{
					BspAddInternal(Inside);
				}
				else
				{
					BspAddOccluderRecursive(Inside);
				}
			}
		}
		if(!Empty(Outside))
		{
			if(n.outside == EMPTY)
				BspAddInternal(Outside);
			else
				BspAddInternal(Outside);


		}

	*/

// }

void BspAddOccluderRecursive(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex, uint32 nBspIndex, uint32 nEdgeMask, int nLevel)
{
	SOccluderBspNode Node = pBsp->Nodes[nBspIndex];
	ZASSERT(Node.nOccluderIndex != nOccluderIndex);
	SOccluderPlane* pPlane = pBsp->Occluders.Ptr() + Node.nOccluderIndex;
	uint8 nEdge = Node.nEdge;
	v4 vPlane = pPlane->p[nEdge];
	int inside[4];
	if(nLevel > pBsp->nDepth)
		pBsp->nDepth = nLevel;

	for(int i = 0; i < 4; ++i)
		inside[i] = v4dot(vPlane, pOccluder->corners[i].tov4(1.f)) < -0.01f;
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
			pBsp->Nodes[nBspIndex].nInside = (uint16)BspAddInternal(pBsp, nOccluderIndex, 0xf, nLevel +1);
		}
		else
		{
			BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nInside, 0xf, nLevel + 1);
		}
	}

	if(nNewMaskOut)
	{
		ZASSERT(Node.nOutside != OCCLUDER_LEAF);
		if(Node.nOutside==OCCLUDER_EMPTY)
		{
			pBsp->Nodes[nBspIndex].nOutside = (uint16)BspAddInternal(pBsp, nOccluderIndex, 0xf, nLevel + 1);
		}
		else
		{
			BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nOutside, 0xf, nLevel + 1);
		}
	}
}


void BspAddOccluder(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex)
{
	uint32 nNumNodes = (uint32)pBsp->Nodes.Size();
	if(pBsp->Nodes.Empty())
	{
		uint16 nIndex = (uint16)BspAddInternal(pBsp, nOccluderIndex, 0xf, 1);
		ZASSERT(nIndex==0);
	}
	else
	{
		BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, 0, 0xf, 1);
	}
	//uprintf("BSP NODE %d -> %d\n", nNumNodes, pBsp->Nodes.Size());

}



uint32 g_nBspFlipMask = 0;
uint32 g_nBspDrawMask = -1;

typedef TFixedArray<v4, 1024> DebugArray;
void DrawBspRecursive(SOccluderBsp* pBsp, uint32 nOccluderIndex, uint32 nFlipMask, uint32 nDrawMask, float fOffset, DebugArray& DEBUG)
{
	SOccluderBspNode Node = pBsp->Nodes[nOccluderIndex];
	SOccluderPlane* pPlane = pBsp->Occluders.Ptr() + Node.nOccluderIndex;
	v4 vOffset = v4init(fOffset, 0.f, 0.f, 0.f);
	v4 v0 = v4init(pPlane->corners[(Node.nEdge+3) %4],1.f);
	v4 v1 = v4init(pPlane->corners[(Node.nEdge) %4],1.f);
	DEBUG.PushBack(v0);
	DEBUG.PushBack(v1);
	if(1)
	{
		for(uint32 i = 0; i < DEBUG.Size(); i += 2)
		{
			v4 v0 = DEBUG[i] + vOffset;
			v4 v1 = DEBUG[i+1] + vOffset;
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

void DrawPlane(SOccluderPlane* pOccluder, int nEdge)
{
	v3 p0 = pOccluder->corners[(nEdge+1)%4]*1.05f;
	v3 p1 = pOccluder->corners[nEdge]*1.05f;
	ZDEBUG_DRAWLINE(p0, p1, 0, true);
}


bool BspClipQuadR(SOccluderBsp* pBsp, uint32 nNodeIndex, v4* pVertices, uint32 nNumVertices, uint32 nClipLevel, uint32 nDepth)
{
	if(nNodeIndex&0x8000)
	{
		return true;
	}
	uint32 nMaxVertex = 2*(nNumVertices+1);
	uint8* nDot = (uint8*)alloca(nNumVertices+1);
	v4* pVertexNew = (v4*)alloca(nMaxVertex*sizeof(v4));


	if((uint64)pVertexNew& 0x3)
		ZBREAK();
	if((uint64)nDot& 0x3)
		ZBREAK();

	SOccluderBspNode* pNode = &pBsp->Nodes[nNodeIndex];
	SOccluderPlane* pOccluder = pBsp->Occluders.Ptr() + pNode->nOccluderIndex;
	uint32 nEdge = pNode->nEdge;
	v4 vPlane = pOccluder->p[nEdge];
	v4 vNormalPlane = pOccluder->normal;
	//uplotfnxt("CLIP QUAD %d v:%d", nNodeIndex, nNumVertices);
	
#define INSIDE 1
#define OUTSIDE 2
	//test mod planet
	int nMask = 0;
	int n = 1;
	float fArea = 0.f;
	//uprintf("I%d O:%d %p", INSIDE, OUTSIDE, &nDot[0]);
	for(uint32 i = 0; i < nNumVertices; ++i)
	{
		v4 pVertex = pVertices[i];
		int d = v4dot(pVertex, vPlane) > 0.0f ? INSIDE : OUTSIDE;
		n = n && v4dot(pVertex, vNormalPlane) > 0.f ? 1 : 0;
		nDot[i] = d;
		//uprintf("%d ", d);
		nMask |= d;
		if(i < nNumVertices-2)
		{
			v3 vLast = pVertices[nNumVertices-1].tov3();
			v3 v0 = pVertices[i].tov3();
			v3 v1 = pVertices[i+1].tov3();
			fArea += v3length(v3cross(v0 - vLast, v1 - vLast)) * 0.5f;

		}
	}

//	uplotfnxt("N %d idx %d\n", n, nNodeIndex);
	ZASSERT(nNodeIndex < pBsp->Nodes.Size());

	if(0 == nClipLevel)
	{
		DrawPlane(pOccluder, nEdge);
	}

	//uprintf("\n");
	if(nMask == 3)
	{
		v4* pVertexNew = (v4*)alloca(nMaxVertex);
		v4* pVertexIn = pVertexNew;
		v4* pVertexOut = pVertexNew + nMaxVertex;		
		int nIndexIn = 0;
		int nIndexOut = 0;
		for(uint32 i = 0; i < nNumVertices; ++i)
		{
			int idxn = (i + 1) % nNumVertices;
			int i0 = nDot[i];
			int i1 = nDot[idxn];

			ZASSERT(i0 == INSIDE || i0 == OUTSIDE);
			ZASSERT(i1 == INSIDE || i1 == OUTSIDE);
			if(INSIDE == i0)
			{
				//PUSH i0				
				
				if(OUTSIDE == i1)
				{

					//PUSH INTERSECTION
					v4 vVertex0 = v4init(pVertices[i],1.f);
					v4 vVertex1 = v4init(pVertices[idxn],1.f);
					float Len0 = fabsf(v4dot(vVertex0, vPlane));
					float Len1 = fabsf(v4dot(vVertex1, vPlane));
					float fLenScale = 1 / (Len0 + Len1);
					Len0 *= fLenScale;
					Len1 *= fLenScale;
					v4 vIntersection = Len1 * vVertex0 + Len0 * vVertex1;
					ZASSERT(pVertexIn+1 < pVertexOut);
					
					*pVertexIn++ = pVertices[i];
					*pVertexIn++ = vIntersection;					
					*--pVertexOut = vIntersection;
				}
				else
				{
					*pVertexIn++ = pVertices[i];
				}
			}
			else
			{
				if(i1 == INSIDE)
				{
					//PUSH INTERSECTION
					v4 vVertex0 = v4init(pVertices[i],1.f);
					v4 vVertex1 = v4init(pVertices[idxn],1.f);
					float Len0 = fabsf(v4dot(vVertex0, vPlane));
					float Len1 = fabsf(v4dot(vVertex1, vPlane));
					float fLenScale = 1 / (Len0 + Len1);
					Len0 *= fLenScale;
					Len1 *= fLenScale;
					v4 vIntersection = Len1 * vVertex0 + Len0 * vVertex1;
					ZASSERT(pVertexIn < pVertexOut-1);
					*pVertexIn++ = vIntersection;
					*--pVertexOut = pVertices[i];
					*--pVertexOut = vIntersection;

				}
				else
				{
					*--pVertexOut = pVertices[i];
					//nIndicesOut[nIndexOut++] = i + nPolyIndex;
				}
			}
		}
		//remove degenerate triangles
		v4* pLast = pVertexIn-1;
		v4* pPut = pVertexNew;
		uint32 nNumVertexInbefore = pLast - pVertexNew;
		for(v4* v = pVertexNew; v < pVertexIn-1; ++v)
		{
			float len = v3length(v3cross( (*pLast-v[0]).tov3(), (v[1]-v[0]).tov3()));
			if(fabs(len)>0.001f)
			{
				*pPut++ = v[0];
				pLast = v;
			}
		}
		*pPut++ = pVertexIn[-1];
		uint32 nNumVertexIn = pPut - pVertexNew;

		pLast = pVertexNew + nMaxVertex - 1;
		pPut = pVertexOut;
		uint32 nNumVertexOutbefore = pVertexNew + nMaxVertex - pVertexOut;
		for(v4* v = pVertexOut; v < pVertexNew + nMaxVertex - 1; ++v)
		{
			float len = v3length(v3cross( (*pLast-v[0]).tov3(), (v[1]-v[0]).tov3()));
			if(fabs(len)>0.001f)
			{
				*pPut++ = v[0];
				pLast = v;
			}
		}
		*pPut++ = pVertexNew[nMaxVertex-1];
		uint32 nNumVertexOut = pPut - pVertexOut;



		if(nNumVertexIn < 3) nNumVertexIn = 0;
		if(nNumVertexOut < 3) nNumVertexOut = 0;

		if(0 == nClipLevel)
		{
//			DrawPlane(pOccluder, nEdge);
			BspOccluderDebugDraw(pVertexNew, pVertexNew + nNumVertexIn, 0x88ff0000, 1);
			BspOccluderDebugDraw(pVertexOut, pVertexOut + nNumVertexOut, 0x8800ff00, 1);
		}


		uplotfnxt("OC:%s %04x:%2d:%d [CLIP] A:%6.3f", Spaces(nDepth), nNodeIndex, pNode->nOccluderIndex, pNode->nEdge, nClipLevel == 0 ? '*' : ' ', fArea); 


		bool bTestIn = true, bTestOut = true;
		g_Offset += g_DepthOffset;
		g_Offset += g_XOffset;
		if(nNumVertexIn)
		{
			ZASSERT(pNode->nInside != OCCLUDER_EMPTY);
			// if(g_nOccluderDebug==2)
			// 	BspOccluderDebugDraw(pVertexNew, pVertexIn, 0xffffffff);
			if(OCCLUDER_LEAF == pNode->nInside || pNode->bLeaf)
			{
				for(v4* v = pVertexNew; v < pVertexIn; ++v)
				{
					bTestIn = bTestIn && (v4dot(*v, vNormalPlane) > 0.f ? 1 : 0);
				}
				
				// if(bTestIn==false && g_nOccluderDebug)
				// {
				// 	BspOccluderDebugDraw(pVertexNew, pVertexIn, 0xffff0000);//not clipped
				// }
			}
			else
			{
				bTestIn = BspClipQuadR(pBsp, pNode->nInside, pVertexNew, nNumVertexIn, nClipLevel-1, nDepth+2);
			}
		}
		g_Offset -= g_XOffset;
		g_Offset -= g_XOffset;
		if(nNumVertexOut)
		{
			ZASSERT(pNode->nOutside != OCCLUDER_LEAF);
			if(pNode->nOutside == OCCLUDER_EMPTY)
			{
				// if(g_nOccluderDebug)
				// {
				// 	BspOccluderDebugDraw(pVertexOut, pVertexOut + nNumVertexOut, 0xffff6600); // non clipped
				// 	BspOccluderDebugDraw(pVertices, pVertices + nNumVertices, 0xffff6600); // non clipped
				// }
				uplotfnxt("OC:%s %04x:>>[CLIP] 0", Spaces(nDepth), nNodeIndex);
				return false;
			}
			else
			{
				// if(g_nOccluderDebug == 1)
				// 	BspOccluderDebugDraw(pVertexOut, pVertexOut + nNumVertexOut, 0xffffffff); // maybe clipped
				bTestOut = BspClipQuadR(pBsp, pNode->nOutside, pVertexOut, nNumVertexOut, nClipLevel-1, nDepth+2);
			}
		}
		g_Offset += g_XOffset;
		g_Offset -= g_DepthOffset;
		uplotfnxt("OC:%s %04x:>>[CLIP] %d", Spaces(nDepth),nNodeIndex,  bTestIn && bTestOut);
		return bTestIn && bTestOut;




	}
	else
	{
		if(nMask == INSIDE)
		{
			if(pNode->bLeaf)
			{
				uplotfnxt("OC:%s %04x:%2d:%d [INSIDE] R:1 A:%6.3f", Spaces(nDepth), nNodeIndex,pNode->nOccluderIndex, pNode->nEdge, fArea);
				return true;
			}
			// all in inside
			ZASSERT(pNode->nInside != OCCLUDER_EMPTY);
			if(pNode->nInside == OCCLUDER_LEAF)
			{

				uplotfnxt("OC:%s %04x:%2d:%d [INSIDE] R :%d (LEAF, n==%d)A:%6.3f", Spaces(nDepth), nNodeIndex, pNode->nOccluderIndex, pNode->nEdge, n!=0, n, fArea);
				return n != 0;
			}
			else
			{
				uplotfnxt("OC:%s %04x:%2d:%d [INSIDE] A:%6.3f", Spaces(nDepth), nNodeIndex, pNode->nOccluderIndex, pNode->nEdge, fArea);
				return BspClipQuadR(pBsp, pNode->nInside, pVertices, nNumVertices, nClipLevel-1, nDepth+2);

			}

		}
		else
		{
//			uplotfnxt("OC: %2d:%d [OUTSIDE]", pNode->Index.nOccluderIndex, pNode->Index.nEdge);

			ZASSERT(nMask == OUTSIDE);
			// all in outside
			ZASSERT(pNode->nOutside != OCCLUDER_LEAF);
			if(pNode->nOutside == OCCLUDER_EMPTY)
			{
				ZASSERT(nNumVertices); 
				uplotfnxt("OC:%s %04x:%2d:%d [OUTSIDE] R:%d [GEOMETRY OUTSIDE] A:%6.3f",Spaces(nDepth), nNodeIndex, pNode->nOccluderIndex, pNode->nEdge, false, fArea);
				return false; // all is out, none is clipped
			}
			else
			{
				uplotfnxt("OC:%s %04x:%2d:%d [OUTSIDE] A:%6.3f", Spaces(nDepth), nNodeIndex, pNode->nOccluderIndex, pNode->nEdge, fArea);
				return BspClipQuadR(pBsp, pNode->nOutside, pVertices, nNumVertices, nClipLevel-1, nDepth+2);
			}
		}
	}
}
//returns true if 100% clipped
bool BspClipQuad(SOccluderBsp* pBsp, v4 v0, v4 v1, v4 v2, v4 v3)
{
	v4 vClipBuffer[OCCLUDER_CLIP_MAX];
	uint8 nIndices[OCCLUDER_CLIP_MAX];
	vClipBuffer[0] = v0;
	vClipBuffer[1] = v1;
	vClipBuffer[2] = v2;
	vClipBuffer[3] = v3;
	nIndices[0] = 0;
	nIndices[1] = 1;
	nIndices[2] = 2;
	nIndices[3] = 3;
	g_OFFSET = 0;
	return BspClipQuadR(pBsp, 0, &vClipBuffer[0], 4, g_nOccluderClipLevels, 0);
}


bool BspCullObject(SOccluderBsp* pBsp, SWorldObject* pObject)
{
	m mObjectToWorld = pObject->mObjectToWorld;
	v3 vHalfSize = pObject->vSize;
	v4 vCenterWorld = pObject->mObjectToWorld.trans;
	v3 AABB = obbtoaabb(mObjectToWorld, vHalfSize);

	v4 vCenterPoly = vCenterWorld - v4init(AABB.x, 0.f, 0.f, 0.f);
	v4 vDir0 = v4init(0.f,  AABB.y,  AABB.z, 0.f);
	v4 vDir1 = v4init(0.f, -AABB.y,  AABB.z, 0.f);
	v4 vDir2 = v4init(0.f, -AABB.y, -AABB.z, 0.f);
	v4 vDir3 = v4init(0.f,  AABB.y, -AABB.z, 0.f);


	v4 v0 = vCenterPoly + vDir0;
	v4 v1 = vCenterPoly + vDir1;
	v4 v2 = vCenterPoly + vDir2;
	v4 v3 = vCenterPoly + vDir3;
	// v4 vDir4 = v4init(2*AABB.x,  AABB.y,  AABB.z, 0.f);
	// v4 vDir5 = v4init(2*AABB.x, -AABB.y,  AABB.z, 0.f);
	// v4 vDir6 = v4init(2*AABB.x, -AABB.y, -AABB.z, 0.f);
	// v4 vDir7 = v4init(2*AABB.x,  AABB.y, -AABB.z, 0.f);
	// v4 v4_ = vCenterPoly + vDir4;
	// v4 v5 = vCenterPoly + vDir5;
	// v4 v6 = vCenterPoly + vDir6;
	// v4 v7 = vCenterPoly + vDir7;
	// ZDEBUG_DRAWLINE(v4_, v5, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v5, v6, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v6, v7, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v7, v4_, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v0, v4_, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v1, v5, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v2, v6, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v3, v7, 0xff0000ff, true);

	// ZDEBUG_DRAWLINE(v0, v1, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v1, v2, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v2, v3, 0xff0000ff, true);
	// ZDEBUG_DRAWLINE(v3, v0, 0xff0000ff, true);



	return BspClipQuad(pBsp, v0, v1, v2, v3);
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
void BspBuild(SOccluderBsp* pBsp, SOccluder* pOccluders, uint32 nNumOccluders, m mWorldToView)
{
	if(g_KeyboardState.keys[SDLK_F3]&BUTTON_RELEASED)
	{
		g_nOccluderDebug = (g_nOccluderDebug+1)%3;
	}

	if(g_KeyboardState.keys[SDLK_F7]&BUTTON_RELEASED)
	{
		g_nOccluderClipLevels = (g_nOccluderClipLevels-1);
	}
	if(g_KeyboardState.keys[SDLK_F8]&BUTTON_RELEASED)
	{
		g_nOccluderClipLevels = (g_nOccluderClipLevels+1);
	}
	uplotfnxt("OCC VIS %d", g_nOccluderClipLevels);

	long seed = rand();
	srand(42);

	pBsp->nDepth = 0;
	pBsp->Nodes.Clear();
	pBsp->Occluders.Clear();
	// 	SOccluderPlane* pPlanes = new SOccluderPlane[nNumOccluders];
	pBsp->Occluders.Resize(nNumOccluders);
	SOccluderPlane* pPlanes = pBsp->Occluders.Ptr();
	v3 vCameraPosition = v3init(0,0,0);

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
			v3 v2 = vCameraPosition;
			v3 vCenter = (v0 + v1 + v2) / v3init(3.f);
			v3 vNormal = v3normalize(v3cross(v3normalize(v1 - v0), v3normalize(v2 - v0)));
			v3 vEnd = vCenter + vNormal;
			Plane.p[i] = MakePlane(vCorners[i], vNormal);
			Plane.corners[i] = vCorners[i];
			ZDEBUG_DRAWLINE(v0, v1, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v1, v2, (uint32)-1, true);
			ZDEBUG_DRAWLINE(v2, v0, (uint32)-1, true);
		}
		Plane.normal = MakePlane(vCorners[0], vNormal);
		// Plane.normal.x = -Plane.normal.x;
		// Plane.normal.y = -Plane.normal.y;
		// Plane.normal.z = -Plane.normal.z;
		// Plane.normal.w = -Plane.normal.w;
		Plane.p[4] = Plane.normal;
	}

	pBsp->Nodes.Clear();
	if(!pBsp->Occluders.Size())
		return;





	for(uint32 i = 0; i < pBsp->Occluders.Size(); ++i)
	{
		if(g_nUseNew)
			BspAddOccluder2(pBsp, &pBsp->Occluders[i], i);
		else
			BspAddOccluder(pBsp, &pBsp->Occluders[i], i);


		for(uint j = 0; j < 4; ++j)
		{
			v4 p0 = pBsp->Occluders[i].p[j];
			v4 p1 = pBsp->Occluders[i].p[(j+1)%4];
			v4 vNormal = pBsp->Occluders[i].normal;
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
int BspAddInternal2(SOccluderBsp* pBsp, SOccluderEdgeIndex* Poly, uint32 nVertices, uint32 nLevel);
void BspAddOccluderRecursive2(SOccluderBsp* pBsp, uint32 nBspIndex, SOccluderEdgeIndex* Poly, uint32 nEdges, uint32 nLevel);



void BspAddOccluder2(SOccluderBsp* pBsp, SOccluderPlane* pOccluder, uint32 nOccluderIndex)
{
	//base poly
	SOccluderEdgeIndex Poly[ 5 ];
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
		uint16 nIndex = (uint16)BspAddInternal2(pBsp, &Poly[0], 5, 1);
		ZASSERT(nIndex==0);
	}
	else
	{
		BspAddOccluderRecursive2(pBsp, 0, &Poly[0], 5, 1);
	}
}

void BspDrawPoly(SOccluderBsp* pBsp, SOccluderEdgeIndex* Poly, uint32 nVertices, uint32 nColorPoly, uint32 nColorEdges)
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


int BspAddInternal2(SOccluderBsp* pBsp, SOccluderEdgeIndex* Poly, uint32 nVertices, uint32 nLevel)
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


uint32 BspClipPoly(SOccluderBsp* pBsp, SOccluderEdgeIndex PlaneIndex, SOccluderEdgeIndex* Poly, uint32 nEdges, SOccluderEdgeIndex* pPolyOut)
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
	uplotfnxt("bBehind %02d..%02d   :: %d %d %d %d", PlaneIndex.nOccluderIndex, PlaneIndex.nEdge,
		bBehindCorner[0]?1:0,
		bBehindCorner[1]?1:0, 
		bBehindCorner[2]?1:0, 
		bBehindCorner[3]?1:0);
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
	return nOutIndex;
}


ClipPolyResult BspClipPoly(SOccluderBsp* pBsp, SOccluderEdgeIndex ClipPlane, SOccluderEdgeIndex* Poly, uint32 nEdges, SOccluderEdgeIndex* pPolyOut, uint32& nNumVertexIn, uint32& nNumVertexOut)
{
	uint32 nIn = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, pPolyOut);
	if(nIn == 1) 
		nIn = 0;
	ClipPlane.nFlip ^= 1;
	uint32 nOut = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, pPolyOut + nIn);
	if(nOut == 1) 
		nOut = 0;

	nNumVertexIn = nIn;
	nNumVertexOut = nOut;


	return ClipPolyResult((nIn?0x1:0)| (nOut?0x2:0));
}

void BspAddOccluderRecursive2(SOccluderBsp* pBsp, uint32 nBspIndex, SOccluderEdgeIndex* Poly, uint32 nEdges, uint32 nLevel)
{
	SOccluderBspNode Node = pBsp->Nodes[nBspIndex];
	

	SOccluderPlane* pPlane = pBsp->Occluders.Ptr() + Node.nOccluderIndex;
	v4 vPlane = pPlane->p[Node.nEdge];
	SOccluderEdgeIndex ClippedPoly[OCCLUDER_POLY_MAX*2];
	uint32 nNumEdgeIn = 0, nNumEdgeOut = 0;

	SOccluderEdgeIndex ClipPlane;
	ClipPlane.nOccluderIndex = Node.nOccluderIndex;
	ClipPlane.nEdge = Node.nEdge;
	ClipPlane.nFlip = Node.nFlip;

	ClipPolyResult CR = BspClipPoly(pBsp, ClipPlane, Poly, nEdges, &ClippedPoly[0], nNumEdgeIn, nNumEdgeOut);
	ZASSERT(Node.nInside != OCCLUDER_EMPTY);
	ZASSERT(Node.nOutside != OCCLUDER_LEAF);
	SOccluderEdgeIndex* pPolyInside = ECPR_BOTH == CR ? &ClippedPoly[0] : Poly;
	uint32 nPolyEdgeIn = ECPR_BOTH == CR ? nNumEdgeIn : nEdges;
	SOccluderEdgeIndex* pPolyOutside= ECPR_BOTH == CR ? &ClippedPoly[nNumEdgeIn] : Poly;
	uint32 nPolyEdgeOut = ECPR_BOTH == CR ? nNumEdgeOut : nEdges;

	if((ECPR_INSIDE & CR) != 0)
	{
		if(Node.nInside == OCCLUDER_LEAF)
		{
		}
		else
		{
			BspAddOccluderRecursive2(pBsp, Node.nInside, pPolyInside, nPolyEdgeIn, nLevel+1);
		}
	}
	
	if((ECPR_OUTSIDE & CR) != 0)
	{
		if(Node.nOutside == OCCLUDER_EMPTY)
		{
			int nIndex = BspAddInternal2(pBsp, pPolyOutside, nPolyEdgeOut, nLevel + 1);
			pBsp->Nodes[nBspIndex].nOutside = nIndex;
		}
		else
		{
			BspAddOccluderRecursive2(pBsp, Node.nOutside, pPolyOutside, nPolyEdgeOut, nLevel+1);
		}
	}
}









	// OccluderPoly Inside = ClipInside(a, pOccluder)
	// OccluderPoly Outside = ClipRight(a, pOccluder)
	// if(!Empty(Inside))
	// {
	// 	bool bOccluded = true;
	// 	if(!n.bIsLeaf || !OccluderBehindZ(Inside))
	// 	{
	// 		if(n.inside == LEAF)
	// 		{
	// 			BspAddInternal(Inside);
	// 		}
	// 		else
	// 		{
	// 			BspAddOccluderRecursive(Inside);
	// 		}
	// 	}
	// }
	// if(!Empty(Outside))
	// {
	// 	if(n.outside == EMPTY)
	// 		BspAddInternal(Outside);
	// 	else
	// 		BspAddInternal(Outside);


	// }
	// SOccluderBspNode Node = pBsp->Nodes[nBspIndex];
	// ZASSERT(Node.nOccluderIndex != nOccluderIndex);
	// SOccluderPlane* pPlane = pBsp->Occluders.Ptr() + Node.nOccluderIndex;
	// uint8 nEdge = Node.nEdge;
	// v4 vPlane = pPlane->p[nEdge];
	// int inside[4];
	// if(nLevel > pBsp->nDepth)
	// 	pBsp->nDepth = nLevel;

	// for(int i = 0; i < 4; ++i)
	// 	inside[i] = v4dot(vPlane, pOccluder->corners[i].tov4(1.f)) < -0.01f;
	// uint32 nNewMaskIn = 0, nNewMaskOut = 0;
	// int x = 1;
	// for(int i = 0; i < 4; ++i)
	// {
	// 	if(x&nEdgeMask)
	// 	{
	// 		if(inside[(i-1)%4] || inside[i])
	// 			nNewMaskIn |= x;
	// 		if((!inside[(i-1)%4]) || (!inside[i]))
	// 			nNewMaskOut |= x;
	// 	}
	// 	x <<= 1;
	// }
	// if(nNewMaskIn)
	// {
	// 	ZASSERT(Node.nInside != OCCLUDER_EMPTY);
	// 	if(Node.nInside==OCCLUDER_LEAF)
	// 	{
	// 		pBsp->Nodes[nBspIndex].nInside = (uint16)BspAddInternal(pBsp, nOccluderIndex, 0xf, nLevel +1);
	// 	}
	// 	else
	// 	{
	// 		BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nInside, 0xf, nLevel + 1);
	// 	}
	// }

	// if(nNewMaskOut)
	// {
	// 	ZASSERT(Node.nOutside != OCCLUDER_LEAF);
	// 	if(Node.nOutside==OCCLUDER_EMPTY)
	// 	{
	// 		pBsp->Nodes[nBspIndex].nOutside = (uint16)BspAddInternal(pBsp, nOccluderIndex, 0xf, nLevel + 1);
	// 	}
	// 	else
	// 	{
	// 		BspAddOccluderRecursive(pBsp, pOccluder, nOccluderIndex, Node.nOutside, 0xf, nLevel + 1);
	// 	}
	// }
