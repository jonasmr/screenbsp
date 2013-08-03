#include "mesh.h"
#include "glinc.h"
#include "shader.h"


void MeshCreateBuffers(Mesh* pMesh);
void MeshDestroyBuffers(Mesh* pMesh);

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
	//front (X)
	pQuads[idx++] = 0;
	pQuads[idx++] = 1;
	pQuads[idx++] = 2;
	pQuads[idx++] = 3;
	//back
	pQuads[idx++] = 7;
	pQuads[idx++] = 6;
	pQuads[idx++] = 5;
	pQuads[idx++] = 4;
	//left (Y)
	pQuads[idx++] = 5;
	pQuads[idx++] = 6;
	pQuads[idx++] = 2;
	pQuads[idx++] = 1;		
	//right
	pQuads[idx++] = 0;
	pQuads[idx++] = 3;
	pQuads[idx++] = 7;
	pQuads[idx++] = 4;		
	//top (Z)
	pQuads[idx++] = 4;
	pQuads[idx++] = 5;
	pQuads[idx++] = 1;
	pQuads[idx++] = 0;		
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
	float f = 1.0f;

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
//		pVertices[i].Position = v3normalize(pVertices[i].Position);
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
	r.Position = v3normalize(a.Position + b.Position);
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
		for(uint32 j = 0; j < nSrcVertices; ++j)
		{
			pVertex[j].Position = v3normalize(pVertex[j].Position);
		}
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


Mesh* CreateFlatMesh(Mesh* pMesh)
{
	//explode new mesh
	uint32 nNewVertices = pMesh->nIndices;
	const Vertex* pSrcVertices = pMesh->pVertices;
	const uint16* pSrcIndices = pMesh->pIndices;
	Vertex* pVertices = new Vertex[nNewVertices];
	uint16* pIndices = new uint16[nNewVertices];
	int idx = 0;

	for(uint32 i = 0; i < pMesh->nIndices; i += 3)
	{
		//1-0 > up y  [0]
		//1-2 > down z [1]
		Vertex v0 = pSrcVertices[ pSrcIndices[i] ];
		Vertex v1 = pSrcVertices[ pSrcIndices[i+1] ];
		Vertex v2 = pSrcVertices[ pSrcIndices[i+2] ];

		v3 d0 = v0.Position - v1.Position;
		v3 d1 = v2.Position - v1.Position;
		v3 normal = v3normalize(v3cross(d1,d0));
//		uprintf("NORMAL IS %f %f %f ::: %f\n", normal.x, normal.y, normal.z, v3dot(normal, v0.Normal));
		pVertices[idx] = v0;
		pVertices[idx].Normal = normal;
		pVertices[idx+1] = v1;
		pVertices[idx+1].Normal = normal;
		pVertices[idx+2] = v2;
		pVertices[idx+2].Normal = normal;
		pIndices[idx] = idx;
		idx++;
		pIndices[idx] = idx;
		idx++;
		pIndices[idx] = idx;
		idx++;
	}
	Mesh* pMeshOut = new Mesh;
	pMeshOut->pVertices = pVertices;
	pMeshOut->pIndices = pIndices;
	pMeshOut->nIndices = nNewVertices;
	pMeshOut->nVertices = nNewVertices;
	return pMeshOut;
}



void MeshInit()
{
	memset(&g_MeshState.Base[0], 0, sizeof(g_MeshState.Base));
	g_MeshState.Base[MESH_BOX] = CreateBoxMesh();

	for(int i = 0; i < 5; ++i)
	{
		g_MeshState.Base[i+MESH_SPHERE_1] = CreateSphereMesh(i+1);
	}

	for(int i = 0; i < MESH_SPHERE_5; ++i)
	{
		g_MeshState.Base[i + MESH_BOX_FLAT] = CreateFlatMesh(g_MeshState.Base[i]);
	}


	for(int i = 0; i < MESH_SIZE; ++i)
	{
		if(g_MeshState.Base[i])
			MeshCreateBuffers(g_MeshState.Base[i]);
	}
}


void MeshDestroy()
{
	for(int i = 0; i < MESH_SIZE; ++i)
	{
		if(g_MeshState.Base[i])
		{
			MeshDestroyBuffers(g_MeshState.Base[i]);
			delete g_MeshState.Base[i]->pVertices;
			delete g_MeshState.Base[i]->pIndices;
			delete g_MeshState.Base[i];
		}
	}
}
const Mesh* GetBaseMesh(EBaseMesh type)
{
	return g_MeshState.Base[type];
}

//GL specific shit below here
void MeshCreateBuffers(Mesh* pMesh)
{
	ZASSERT(pMesh);
	glGenBuffers(1, &pMesh->VertexBuffer);
	glGenBuffers(1, &pMesh->IndexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, pMesh->VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, pMesh->nVertices * sizeof(Vertex), pMesh->pVertices, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMesh->nIndices * sizeof(uint16), pMesh->pIndices, GL_STATIC_DRAW);
 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MeshDestroyBuffers(Mesh* pMesh)
{
	ZASSERT(pMesh);
}



void MeshDraw(const Mesh* pMesh)
{

	glBindBuffer(GL_ARRAY_BUFFER, pMesh->VertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->IndexBuffer);

	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
	glNormalPointer(GL_FLOAT, sizeof(Vertex), (const void*)offsetof(Vertex, Normal));
	glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(Vertex), (const void*)offsetof(Vertex,Color));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	CheckGLError();


//	SHADER_SET("Size", vSize);
	// int loc = ShaderGetLocation("Size");
	// if(loc > -1)
	// 	ShaderSetUniform(loc, vSize);
	CheckGLError();



	glDrawElements(GL_TRIANGLES, pMesh->nIndices, GL_UNSIGNED_SHORT, (const void*)0);


	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	
	CheckGLError();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CheckGLError();
}
