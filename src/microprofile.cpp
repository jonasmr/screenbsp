#define MICRO_PROFILE_IMPL
#include "microprofile.h"
#include "glinc.h"
#include "text.h"
#include "shader.h"

#if MICROPROFILE_ENABLED

uint32_t g_nUseFastDraw = 1;

struct MicroProfileVertex
{
	float nX;
	float nY;
	uint32_t nColor;
	float fU;
	float fV;
};

#define MICROPROFILE_MAX_VERTICES (64<<10)

namespace
{
	uint32_t nVertexPos = 0;
	MicroProfileVertex nDrawBuffer[MICROPROFILE_MAX_VERTICES];
	
	MicroProfileVertex* PushVertices(int nCount)
	{
		if(nVertexPos + nCount > MICROPROFILE_MAX_VERTICES)
		{
			ZBREAK();
		}

		uint32_t nOut = nVertexPos;
		nVertexPos += nCount;
		return &nDrawBuffer[nOut];
	}
	uint32_t g_nW;
	uint32_t g_nH;

	uint32_t g_VBO;
}
void MicroProfileDrawInit()
{
	glGenBuffers(1, &g_VBO);
}

void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight)
{
	g_nW = nWidth;
	g_nH = nHeight;
	nVertexPos = 0;
}
extern GLuint g_FontTexture;
void MicroProfileEndDraw()
{
	if(0 == g_nUseFastDraw)
		return;
	if(0 == nVertexPos)
		return;

	ShaderUse(VS_MICROPROFILE, PS_MICROPROFILE);
	uint32_t loc = ShaderGetLocation(PS_MICROPROFILE, "tex");
	MP_ASSERT(-1 != loc);
	ShaderSetUniform(loc, g_FontTexture);

	glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(nDrawBuffer), &nDrawBuffer[0], GL_STREAM_DRAW);
	int nStride = sizeof(MicroProfileVertex);
	glVertexPointer(2, GL_FLOAT, nStride, 0);
	glColorPointer(4, GL_UNSIGNED_BYTE, nStride, (void*)(offsetof(MicroProfileVertex, nColor)));
	glTexCoordPointer(4, GL_FLOAT, nStride, (void*)(offsetof(MicroProfileVertex, fU)));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glDrawArrays(GL_QUADS, 0, nVertexPos);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ShaderDisable();

}



void MicroProfileDrawText(uint32_t nX, uint32_t nY, uint32_t nColor, const char* pText)
{
	MICROPROFILE_SCOPEI("MicroProfile", "TextDraw", 0xff88ee);
	if(g_nUseFastDraw)
	{
		const float fEndV = 9.f / 16.f;
		const float fOffsetU = 5.f / 1024.f;
		int nLen = strlen(pText);
		float fX = nX;
		float fY = nY;
		float fY2 = fY + (TEXT_CHAR_HEIGHT+1);

		MicroProfileVertex* pVertex = PushVertices(4 * nLen);
		const char* pStr = pText;
		nColor = -1;

		for(uint32_t j = 0; j < nLen; ++j)
		{
			int16_t nOffset = g_FontDescription.nCharOffsets[*pStr++];
			float fOffset = nOffset / 1024.f;
			pVertex[0].nX = fX;
			pVertex[0].nY = fY;
			pVertex[0].nColor = nColor;
			pVertex[0].fU = fOffset;
			pVertex[0].fV = 0.f;
			
			pVertex[1].nX = fX+TEXT_CHAR_WIDTH;
			pVertex[1].nY = fY;
			pVertex[1].nColor = nColor;
			pVertex[1].fU = fOffset+fOffsetU;
			pVertex[1].fV = 0.f;

			pVertex[2].nX = fX+TEXT_CHAR_WIDTH;
			pVertex[2].nY = fY2;
			pVertex[2].nColor = nColor;
			pVertex[2].fU = fOffset+fOffsetU;
			pVertex[2].fV = 1.f;


			pVertex[3].nX = fX;
			pVertex[3].nY = fY2;
			pVertex[3].nColor = nColor;
			pVertex[3].fU = fOffset;
			pVertex[3].fV = 1.f;

			fX += TEXT_CHAR_WIDTH+1;
			pVertex += 4;
		}





	}
	else
	{
		TextBegin();
		{
			MICROPROFILE_SCOPEI("MicroProfile", "TextPut", 0xff88ee);
			TextPut(nX, nY, pText, -1);
		}
		TextEnd();
	}
}
void MicroProfileDrawBox(uint32_t nX, uint32_t nY, uint32_t nWidth, uint32_t nHeight, uint32_t nColor)
{
	if(g_nUseFastDraw)
	{
		nColor |= 0xff000000;
		MicroProfileVertex* pVertex = PushVertices(4);
		pVertex[0].nX = nX;
		pVertex[0].nY = nY;
		pVertex[0].nColor = nColor;
		pVertex[0].fU = 2.f;
		pVertex[0].fV = 2.f;
		pVertex[1].nX = nX  + nWidth;
		pVertex[1].nY = nY;
		pVertex[1].nColor = nColor;
		pVertex[1].fU = 2.f;
		pVertex[1].fV = 2.f;
		pVertex[2].nX = nX + nWidth;
		pVertex[2].nY = nY + nHeight;
		pVertex[2].nColor = nColor;
		pVertex[2].fU = 2.f;
		pVertex[2].fV = 2.f;
		pVertex[3].nX = nX;
		pVertex[3].nY = nY + nHeight;
		pVertex[3].nColor = nColor;
		pVertex[3].fU = 2.f;
		pVertex[3].fV = 2.f;
	}
	else
	{
		glBegin(GL_QUADS);
		glColor4ub(0xff&(nColor>>16), 0xff&(nColor>>8), 0xff&nColor, 0xff);
		glVertex2i(nX, nY);
		glVertex2i(nX + nWidth, nY);
		glVertex2i(nX + nWidth, nY + nHeight);
		glVertex2i(nX, nY + nHeight);
		glEnd();
	}

}

void MicroProfileDrawBoxFade(uint32_t nX0, uint32_t nY0, uint32_t nX1, uint32_t nY1, uint32_t nColor)
{
	
	uint32_t r = 0xff & (nColor>>16);
	uint32_t g = 0xff & (nColor>>8);
	uint32_t b = 0xff & nColor;
	uint32_t nMax = MicroProfileMax(MicroProfileMax(MicroProfileMax(r, g), b), 30u);
	uint32_t nMin = MicroProfileMin(MicroProfileMin(MicroProfileMin(r, g), b), 180u);
	// uint32_t r0 = 0xff & (r | nMax);
	// uint32_t g0 = 0xff & (g | nMax);
	// uint32_t b0 = 0xff & (b | nMax);

	uint32_t r0 = 0xff & ((r + nMax)/2);
	uint32_t g0 = 0xff & ((g + nMax)/2);
	uint32_t b0 = 0xff & ((b + nMax)/2);

	uint32_t r1 = 0xff & ((r+nMin)/2);// >> 0);
	uint32_t g1 = 0xff & ((g+nMin)/2);// >> 0);
	uint32_t b1 = 0xff & ((b+nMin)/2);// >> 0);

	// uint32_t r1 = 0xff & ((r+r)/2);// >> 0);
	// uint32_t g1 = 0xff & ((g+g)/2);// >> 0);
	// uint32_t b1 = 0xff & ((b+b)/2);// >> 0);

	if(g_nUseFastDraw)
	{
		uint32_t nColor0 = (r0<<0)|(g0<<8)|(b0<<16)|0xff000000;
		uint32_t nColor1 = (r1<<0)|(g1<<8)|(b1<<16)|0xff000000;
		MicroProfileVertex* pVertex = PushVertices(4);
		pVertex[0].nX = nX0;
		pVertex[0].nY = nY0;
		pVertex[0].nColor = nColor0;
		pVertex[0].fU = 2.f;
		pVertex[0].fV = 2.f;
		pVertex[1].nX = nX1;
		pVertex[1].nY = nY0;
		pVertex[1].nColor = nColor0;
		pVertex[1].fU = 2.f;
		pVertex[1].fV = 2.f;
		pVertex[2].nX = nX1;
		pVertex[2].nY = nY1;
		pVertex[2].nColor = nColor1;
		pVertex[2].fU = 2.f;
		pVertex[2].fV = 2.f;
		pVertex[3].nX = nX0;
		pVertex[3].nY = nY1;
		pVertex[3].nColor = nColor1;
		pVertex[3].fU = 2.f;
		pVertex[3].fV = 2.f;
	}
	else
	{
		glBegin(GL_QUADS);
		glColor4ub(r0, g0, b0, 0xff);
		glVertex2i(nX0, nY0);
		glVertex2i(nX1, nY0);
		glColor4ub(r1, g1, b1, 0xff);
		glVertex2i(nX1, nY1);
		glVertex2i(nX0, nY1);
		glEnd();
	}
}


void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
	if(!nVertices) return;
	//exploit the fact that lines are vertical.. and draw as quads.
	if(g_nUseFastDraw)
	{
		MicroProfileVertex* pVertex = PushVertices(4);

	}
	else
	{
		glBegin(GL_LINES);
		glColor4ub(0xff&(nColor>>16), 0xff&(nColor>>8), 0xff&nColor, 0xff);
		for(uint32 i = 0; i < nVertices-1; ++i)
		{
			glVertex2f(pVertices[i*2], pVertices[(i*2)+1]);
			glVertex2f(pVertices[(i+1)*2], pVertices[((i+1)*2)+1]);
		}
		glEnd();
	}
}

#define NUM_QUERIES (8<<10)


GLuint g_GlTimers[NUM_QUERIES];
GLuint g_GlTimerPos = (GLuint)-1;


void MicroProfileQueryInitGL()
{
	g_GlTimerPos = 0;
	glGenQueries(NUM_QUERIES, &g_GlTimers[0]);		
	CheckGLError();
}

uint32_t MicroProfileGpuInsertTimeStamp()
{
	uint32_t nIndex = (g_GlTimerPos+1)%NUM_QUERIES;
	CheckGLError();
#ifndef __APPLE__
	glQueryCounter(g_GlTimers[nIndex], GL_TIMESTAMP);
#endif
	g_GlTimerPos = nIndex;
	CheckGLError();
	return nIndex;
}
uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey)
{
#ifndef __APPLE__
	uint64_t result;
	glGetQueryObjectui64v(g_GlTimers[nKey], GL_QUERY_RESULT, &result);
	CheckGLError();
	return result;
#else
	return 1;
#endif
}

uint64_t MicroProfileTicksPerSecondGpu()
{
	return 1000000000ll;
}
#endif
