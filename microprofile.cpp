//#include "base.h"
//#include "debug.h"

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
void MicroProfileMouseMove(uint32_t nX, uint32_t nY)
{

}
void MicroProfileMouseClick(uint32_t nLeft, uint32_t nRight)
{

}
