#define MICRO_PROFILE_IMPL
#include "microprofile.h"
#include "glinc.h"
#include "text.h"

void MicroProfileDrawText(uint32_t nX, uint32_t nY, uint32_t nColor, const char* pText)
{
	TextBegin();
	TextPut(nX, nY, pText, -1);
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

void MicroProfileDrawBox0(uint32_t nX0, uint32_t nY0, uint32_t nX1, uint32_t nY1, uint32_t nColor)
{
	glBegin(GL_QUADS);
	glColor4ub(0xff&(nColor>>16), 0xff&(nColor>>8), 0xff&nColor, 0xff);
	glVertex2i(nX0, nY0);
	glVertex2i(nX1, nY0);
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

