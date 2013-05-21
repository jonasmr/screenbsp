#pragma once
#include "base.h"
#include "math.h"


enum EBaseMesh
{
	MESH_BOX,
	MESH_SPHERE_1,
	MESH_SPHERE_2,
	MESH_SPHERE_3,
	MESH_SPHERE_4,
	MESH_SPHERE_5,
	MESH_SPHERE_6,
	MESH_SPHERE_7,
	MESH_SPHERE_8,
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
};

void MeshInit();
void MeshDestroy();
void MeshDraw(const Mesh* pMesh, m mObjectToWorld, v3 vSize);
const Mesh* GetBaseMesh(EBaseMesh type);



