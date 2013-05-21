#include "mesh.h"


struct MeshState
{
	Mesh* Base[MESH_SIZE];
} g_MeshState;
static uint16 QuadListToTriList(const uint16* pQuads, uint16* pTris, uint32 nNumQuads)
{
	uint16 n = 0;

	for(uint32 i = 0; i < nNumQuads; ++i)
	{
		const uint16* q = pQuads + i * 4;
		pTris[n++] = q[0];
		pTris[n++] = q[1];
		pTris[n++] = q[2];
		pTris[n++] = q[2];
		pTris[n++] = q[3];
		pTris[n++] = q[0];
	}

	return n;
}
static
void InitBoxQuadList(uint16* pQuads)
{
	int idx = 0;
	//front
	pQuads[idx++] = 0;
	pQuads[idx++] = 1;
	pQuads[idx++] = 2;
	pQuads[idx++] = 3;
	//back
	pQuads[idx++] = 7;
	pQuads[idx++] = 6;
	pQuads[idx++] = 5;
	pQuads[idx++] = 4;
	//left
	pQuads[idx++] = 1;
	pQuads[idx++] = 2;
	pQuads[idx++] = 6;
	pQuads[idx++] = 5;		
	//right
	pQuads[idx++] = 0;
	pQuads[idx++] = 3;
	pQuads[idx++] = 7;
	pQuads[idx++] = 4;		
	//top
	pQuads[idx++] = 0;
	pQuads[idx++] = 1;
	pQuads[idx++] = 5;
	pQuads[idx++] = 4;		
	//bottom
	pQuads[idx++] = 3;
	pQuads[idx++] = 2;
	pQuads[idx++] = 6;
	pQuads[idx++] = 7;
	ZASSERT(idx == 6 * 4);		
}
Mesh* CreateBoxMesh()
{
	Mesh* pMesh = new Mesh;
	const uint32 nVertices = 8;
	const uint32 nIndices = 6 * 2 * 3;
	Vertex* pVertices = new Vertex[nVertices];
	uint16* pIndices = new uint16[nIndices];
	float f = 0.5f;

	pVertices[0].Position = v3init(f,f,f);
	pVertices[0].Normal = v3init(f,f,f);
	pVertices[0].Color = (uint32)-1;
	pVertices[1].Position = v3init(f,-f,f);
	pVertices[1].Normal = v3init(f,-f,f);
	pVertices[1].Color = (uint32)-1;
	pVertices[2].Position = v3init(f,-f,-f);
	pVertices[2].Normal = v3init(f,-f,-f);
	pVertices[2].Color = (uint32)-1;
	pVertices[3].Position = v3init(f,f,-f);
	pVertices[3].Normal = v3init(f,f,-f);
	pVertices[3].Color = (uint32)-1;
	pVertices[4].Position = v3init(-f,f,f);
	pVertices[4].Normal = v3init(-f,f,f);
	pVertices[4].Color = (uint32)-1;
	pVertices[5].Position = v3init(-f,-f,f);
	pVertices[5].Normal = v3init(-f,-f,f);
	pVertices[5].Color = (uint32)-1;
	pVertices[6].Position = v3init(-f,-f,-f);
	pVertices[6].Normal = v3init(-f,-f,-f);
	pVertices[6].Color = (uint32)-1;
	pVertices[7].Position = v3init(-f,f,-f);
	pVertices[7].Normal = v3init(-f,f,-f);
	pVertices[7].Color = (uint32)-1;
	for(uint32 i = 0; i < 8; ++i)
	{
		pVertices[i].Position = v3normalize(pVertices[i].Position)*0.5f;
		pVertices[i].Normal = v3normalize(pVertices[i].Normal);
	}

	uint16 Quad[6*4];
	InitBoxQuadList(&Quad[0]);

	QuadListToTriList(&Quad[0], pIndices, 6);



	pMesh->pVertices = pVertices;
	pMesh->pIndices = pIndices;
	pMesh->nIndices = nIndices;
	pMesh->nVertices = nVertices;
	return pMesh;
}

Vertex SphereAvg(Vertex a, Vertex b)
{
	Vertex r;
	r.Position = 0.5f * v3normalize(a.Position + b.Position);
	r.Normal = v3normalize(a.Normal + b.Normal);
	r.Color = a.Color;
	return r;
}


void SubDivSphere(const Vertex* pVertices_, const uint16* pQuads_, uint32 nQuads_, uint32 nVertices_, uint32 nSubDivs_, const uint16** ppQuadOut, const Vertex** ppVertexOut, uint32* nNumQuadOut, uint32* nNumVertexOut)
{
	ZASSERT(nSubDivs_>0);
	const Vertex* pSrcVertex = pVertices_;
	const uint16* pSrcQuads = pQuads_;
	uint32 nSrcVertices = nVertices_;
	uint32 nSrcQuads = nQuads_;
	for(int i = 0; i < nSubDivs_; ++i)
	{
		uint32 nNumQuads = 4 * nSrcQuads;
		uint32 nNumVertices = nSrcVertices + nSrcQuads * 5;
		Vertex* pVertex = new Vertex[nNumVertices];
		uint16* pQuads = new uint16[nNumQuads * 4];
		memcpy(pVertex, pSrcVertex, nSrcVertices * sizeof(Vertex));
		int qidx = 0;
		int vidx = nSrcVertices;
		for(uint32 i = 0; i < nSrcQuads; ++i)
		{
			const uint16* q = pSrcQuads + i * 4;

			const int vidxbase = vidx;
			pVertex[vidx++] = SphereAvg(pVertex[q[0]], pVertex[q[1]]);
			pVertex[vidx++] = SphereAvg(pVertex[q[1]], pVertex[q[2]]);
			pVertex[vidx++] = SphereAvg(pVertex[q[2]], pVertex[q[3]]);
			pVertex[vidx++] = SphereAvg(pVertex[q[3]], pVertex[q[0]]);
			pVertex[vidx++] = SphereAvg(pVertex[vidxbase], pVertex[vidxbase+2]);

			pQuads[qidx++] = q[0];
			pQuads[qidx++] = vidxbase;
			pQuads[qidx++] = vidxbase+4;
			pQuads[qidx++] = vidxbase+3;


			pQuads[qidx++] = vidxbase+4;
			pQuads[qidx++] = vidxbase;
			pQuads[qidx++] = q[1];
			pQuads[qidx++] = vidxbase+1;

			pQuads[qidx++] = vidxbase+4;
			pQuads[qidx++] = vidxbase+1;
			pQuads[qidx++] = q[2];
			pQuads[qidx++] = vidxbase+2;

			pQuads[qidx++] = vidxbase+3;
			pQuads[qidx++] = vidxbase+4;
			pQuads[qidx++] = vidxbase+2;
			pQuads[qidx++] = q[3];
			ZASSERT(qidx <= nNumQuads * 4);
		}
		ZASSERT(qidx == nNumQuads * 4);
		ZASSERT(vidx == nNumVertices);

		if(i != 0)
		{
			delete pSrcVertex;
			delete pSrcQuads;
		}

		pSrcVertex = pVertex;
		pSrcQuads = pQuads;
		nSrcVertices = nNumVertices;
		nSrcQuads = nNumQuads;
	
	}

	ppQuadOut[0] = pSrcQuads;
	ppVertexOut[0] = pSrcVertex;
	nNumQuadOut[0] = nSrcQuads;
	nNumVertexOut[0] = nSrcVertices;
}

Mesh* CreateSphereMesh(int nSubDivs)
{
	Mesh* pBox = CreateBoxMesh();

	uint16 Quad[6*4];
	InitBoxQuadList(&Quad[0]);
	const uint16* pQuadOut = 0;
	const Vertex* pVertexOut = 0;
	uint32 nNumVertexOut = 0;
	uint32 nNumQuadsOut = 0;
	SubDivSphere(pBox->pVertices, &Quad[0], 6, pBox->nVertices, nSubDivs, &pQuadOut, &pVertexOut, &nNumQuadsOut, &nNumVertexOut);


	Mesh* pSphere = new Mesh;
	uint32 nNumIndices = nNumQuadsOut * 6;
	uint16* pIndices = new uint16[ nNumIndices];
	QuadListToTriList(pQuadOut, pIndices, nNumQuadsOut);



	pSphere->pVertices = pVertexOut;
	pSphere->pIndices = pIndices;
	pSphere->nVertices = nNumVertexOut;
	pSphere->nIndices = nNumIndices;

	delete pQuadOut;
	delete pBox;

	return pSphere;
}



void MeshInit()
{
	memset(&g_MeshState.Base[0], 0, sizeof(g_MeshState.Base));
	g_MeshState.Base[MESH_BOX] = CreateBoxMesh();
	for(int i = 0; i < 8; ++i)
	{
		g_MeshState.Base[i+MESH_SPHERE_1] = CreateSphereMesh(i+1);
	}
}


void MeshDestroy()
{
	for(int i = 0; i < MESH_SIZE; ++i)
	{
		delete g_MeshState.Base[i];
	}
}
const Mesh* GetBaseMesh(EBaseMesh type)
{
	return g_MeshState.Base[type];
}



void MeshDraw(const Mesh* pMesh, m mObjectToWorld, v3 vSize)
{

}
