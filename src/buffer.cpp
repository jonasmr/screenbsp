#include "buffer.h"

struct SVertexBufferDynamic
{
	uint32_t nVAO;
	uint32_t nBufferObject;
	uint32_t nBackBufferSize;
	uint32_t nElementCount;
	uint32_t nStride;
	uint32_t nNumCommands;
	uint32_t nNumVertices;
	EVertexFormat eFormat;
	enum
	{
		MAX_COMMANDS=128,
	};
	uint32_t nCommandList[MAX_COMMANDS];
	uint32_t nCommandVertexCount[MAX_COMMANDS];
	void* GetBackBuffer()
	{
		return (void*)(this+1);
	}
};


uint32 VertexBufferElementSize(EVertexFormat eFormat)
{
	switch(eFormat)
	{
		case EVF_FORMAT0: return sizeof(Vertex0);
		default:
			ZBREAK();
	}
	return -1;
}


SVertexBufferDynamic* VertexBufferCreatePush(EVertexFormat eFormat, uint32_t nElementCount)
{
	uint32_t nStride = VertexBufferElementSize(eFormat);
	uint32_t nDataSize = nElementCount * nStride;
	size_t size = sizeof(SVertexBufferDynamic) + nDataSize;
	SVertexBufferDynamic* pBuffer = (SVertexBufferDynamic*)malloc(size);
	//new (pBuffer) SVertexBufferDynamic();
	glGenBuffers(1, &pBuffer->nBufferObject);
	glGenVertexArrays(1, &pBuffer->nVAO);
	pBuffer->eFormat = eFormat;
	pBuffer->nBackBufferSize = nDataSize;
	pBuffer->nElementCount = nElementCount;
	pBuffer->nStride = nStride;
	pBuffer->nNumCommands = 0;
	pBuffer->nNumVertices = 0;
	memset(pBuffer->GetBackBuffer(), 0, VertexBufferElementSize(eFormat) * nElementCount);

	glBindVertexArray(pBuffer->nVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pBuffer->nBufferObject);
	glBufferData(GL_ARRAY_BUFFER, nDataSize, pBuffer->GetBackBuffer(), GL_STREAM_DRAW);
	glVertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, 0, nStride, 0);
	glVertexAttribPointer(LOC_COLOR, GL_BGRA, GL_UNSIGNED_BYTE, 1, nStride, (void*)(offsetof(Vertex0,nColor)));
	glVertexAttribPointer(LOC_TC0, 2, GL_FLOAT, 1, nStride, (void*)(offsetof(Vertex0,fU)));
//	glVertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, 0, nStride, 0);
	glEnableVertexAttribArray(LOC_POSITION);
	glEnableVertexAttribArray(LOC_COLOR);
	glEnableVertexAttribArray(LOC_TC0);
	CheckGLError();




	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	return pBuffer;
}
void* VertexBufferPushVertices(SVertexBufferDynamic* pPushBuffer, uint32_t nNumVertices, EDrawMode eMode)
{
	if(nNumVertices + pPushBuffer->nNumVertices > pPushBuffer->nElementCount||
		pPushBuffer->nNumCommands == SVertexBufferDynamic::MAX_COMMANDS-1)
	{
		VertexBufferPushFlush(pPushBuffer);
	}
	ZASSERT(nNumVertices + pPushBuffer->nNumVertices < pPushBuffer->nElementCount);

	void* pData = (char*)pPushBuffer->GetBackBuffer() + pPushBuffer->nStride * pPushBuffer->nNumVertices;
	pPushBuffer->nNumVertices += nNumVertices;
	pPushBuffer->nCommandList[pPushBuffer->nNumCommands] = eMode;
	pPushBuffer->nCommandVertexCount[pPushBuffer->nNumCommands] = nNumVertices;
	pPushBuffer->nNumCommands++;
	return pData;
}
void VertexBufferPushFlush(SVertexBufferDynamic* pPushBuffer)
{
	CheckGLError();
	glBindBuffer(GL_ARRAY_BUFFER, pPushBuffer->nBufferObject);
	glBufferData(GL_ARRAY_BUFFER, pPushBuffer->nNumVertices * pPushBuffer->nStride, pPushBuffer->GetBackBuffer(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(pPushBuffer->nVAO);
	uint32_t nOffset = 0;
	CheckGLError();
	for(int i = 0; i < pPushBuffer->nNumCommands; ++i)
	{
		CheckGLError();
		int nCount = pPushBuffer->nCommandVertexCount[i];
		glDrawArrays(pPushBuffer->nCommandList[i], nOffset, nCount);
		nOffset += nCount;
		CheckGLError();
	}
	pPushBuffer->nNumCommands = 0;
	pPushBuffer->nNumVertices = 0;
	glBindVertexArray(0);
	CheckGLError();
}


SVertexBufferStatic* VertexBufferCreateStatic(EVertexFormat eFormat, uint32_t nBufferSize)
{
	return 0;
}
SIndexBuffer* IndexBufferCreate(uint32_t nNumIndices)
{
	return 0;
}

void VertexBufferUpdate(SVertexBufferStatic* pVertexBuffer, void* pData, uint32_t nNumVertices);
