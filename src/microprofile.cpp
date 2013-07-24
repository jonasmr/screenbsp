#define MICRO_PROFILE_IMPL
#include "microprofile.h"
#include "glinc.h"
#include "text.h"

#if MICROPROFILE_ENABLED
void MicroProfileDrawText(uint32_t nX, uint32_t nY, uint32_t nColor, const char* pText)
{
	MICROPROFILE_SCOPEI("MicroProfile", "TextDraw", 0xff88ee);
	TextBegin();
	{
		MICROPROFILE_SCOPEI("MicroProfile", "TextPut", 0xff88ee);
		TextPut(nX, nY, pText, -1);
	}
	TextEnd();
}
void MicroProfileDrawBox(uint32_t nX, uint32_t nY, uint32_t nWidth, uint32_t nHeight, uint32_t nColor)
{
	glBegin(GL_QUADS);
	glColor4ub(0xff&(nColor>>16), 0xff&(nColor>>8), 0xff&nColor, 0xff);
	glVertex2i(nX, nY);
	glVertex2i(nX + nWidth, nY);
	glVertex2i(nX + nWidth, nY + nHeight);
	glVertex2i(nX, nY + nHeight);
	glEnd();
}

void MicroProfileDrawBoxFade(uint32_t nX0, uint32_t nY0, uint32_t nX1, uint32_t nY1, uint32_t nColor)
{
	glBegin(GL_QUADS);
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


	glColor4ub(r0, g0, b0, 0xff);
	glVertex2i(nX0, nY0);
	glVertex2i(nX1, nY0);
	glColor4ub(r1, g1, b1, 0xff);
	glVertex2i(nX1, nY1);
	glVertex2i(nX0, nY1);
	glEnd();
}


void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
	if(!nVertices) return;

	glBegin(GL_LINES);
	glColor4ub(0xff&(nColor>>16), 0xff&(nColor>>8), 0xff&nColor, 0xff);
	for(uint32 i = 0; i < nVertices-1; ++i)
	{
		glVertex2f(pVertices[i*2], pVertices[(i*2)+1]);
		glVertex2f(pVertices[(i+1)*2], pVertices[((i+1)*2)+1]);
	}
	glEnd();
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
	#if 1
	glQueryCounter(g_GlTimers[nIndex], GL_TIMESTAMP);
	#endif
	CheckGLError();
	return nIndex;
}
uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey)
{
	#if 1
	uint64_t result;
	glGetQueryObjectui64v(g_GlTimers[nKey], GL_QUERY_RESULT, &result);
	CheckGLError();
	return result;
	#else
	return 1;
	#endif
}
#endif
