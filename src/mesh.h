#pragma once
#include "base.h"
#include "math.h"

enum EVertexStreamIndex
{
	STREAM_POSITION = 0,
	STREAM_COLOR = 1,
	STREAM_NORMAL = 2,
	STREAM_TC0 = 3,

};


enum EBaseMesh
{
	MESH_BOX,
	MESH_SPHERE_1,
	MESH_SPHERE_2,
	MESH_SPHERE_3,
	MESH_SPHERE_4,
	MESH_SPHERE_5,
	// MESH_SPHERE_6,
	// MESH_SPHERE_7,
	// MESH_SPHERE_8,
	MESH_BOX_FLAT,
	MESH_SPHERE_1_FLAT,
	MESH_SPHERE_2_FLAT,
	MESH_SPHERE_3_FLAT,
	MESH_SPHERE_4_FLAT,
	MESH_SPHERE_5_FLAT,
	// MESH_SPHERE_6_FLAT,
	// MESH_SPHERE_7_FLAT,
	// MESH_SPHERE_8_FLAT,

	MESH_SIZE,
};


struct Vertex
{
	v3 Position;
	v3 Normal;
	uint32 Color;
};


struct Mesh
{
	uint32 	nVertices;
	uint32 	nIndices;
	const Vertex* 	pVertices;
	const uint16* 	pIndices;

	uint32 IndexBuffer;
	uint32 VertexBuffer;
	uint32 VAO;
};

void MeshInit();
void MeshDestroy();
void MeshDraw(const Mesh* pMesh);
const Mesh* GetBaseMesh(EBaseMesh type);



