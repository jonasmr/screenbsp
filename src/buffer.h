#pragma once
#include "base.h"
#include "glinc.h"

enum EVertexFormat
{
	EVF_FORMAT0,
	EVF_SIZE,
};
enum EDrawMode
{
	EDM_LINES = GL_LINES,
	EDM_TRIANGLES = GL_TRIANGLES,
	EDM_POLYGON = GL_POLYGON,
	EDM_SIZE,
};

enum
{
	LOC_POSITION = 0,
	LOC_COLOR,
	LOC_NORMAL,
	LOC_TC0,
};

struct Vertex0
{
	float nX;
	float nY;
	float nZ;
	uint32_t nColor;
	float fU;
	float fV;
};

#define Q0(d, v) d[0] = v
#define Q1(d, v) d[1] = v; d[3] = v
#define Q2(d, v) d[4] = v
#define Q3(d, v) d[2] = v; d[5] = v


struct SVertexBufferDynamic;
struct SVertexBufferStatic;
struct SIndexBuffer;

SVertexBufferDynamic* VertexBufferCreatePush(EVertexFormat eFormat, uint32_t nElementCount);
SVertexBufferStatic* VertexBufferCreateStatic(EVertexFormat eFormat, uint32_t nBufferSize);
SIndexBuffer* IndexBufferCreate(uint32_t nNumIndices);

void VertexBufferUpdate(SVertexBufferStatic* pVertexBuffer, void* pData, uint32_t nNumVertices);
void* VertexBufferPushVertices(SVertexBufferDynamic* pPushBuffer, uint32_t nNumVertices, EDrawMode eMode);
void VertexBufferPushFlush(SVertexBufferDynamic* pPushBuffer);