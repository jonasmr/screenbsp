#include "base.h"
#include "fixedarray.h"
#include "text.h"
#include "stb_image.h"
#include "math.h"
#include "debug.h"
#include <algorithm>
#include "glinc.h"

#define MAX_PLOTS 1024
#define TEXT_SCREEN_WIDTH (1920/(TEXT_CHAR_WIDTH+1))
#define TEXT_SCREEN_HEIGHT (1200/8)

extern uint32_t g_lShowDebug, g_lShowDebugText;
extern uint32_t g_Width;
extern uint32_t g_Height;


ZPUSH_OPTIMIZE_OFF
uint32_t g_nTextX2=0;
namespace
{
	struct SLine
	{
		char c[TEXT_SCREEN_WIDTH];
	};

	struct SPlot
	{
		uint32_t nX, nY;
		uint32_t nCount;
	};

	struct STextRenderState
	{
		SLine Lines[TEXT_SCREEN_HEIGHT];
		TFixedArray<SPlot, MAX_PLOTS> Plots;
		uint32_t nFnxtPos;
	};
	STextRenderState g_TextRenderState;
}
SFontDescription g_FontDescription;

void TextInit()
{
	//load texture
	for(uint32_t i = 0; i < MAX_FONT_CHARS; ++i)
		g_FontDescription.nCharOffsets[i] = 206; //blank
	for(uint32_t i = 'A'; i <= 'Z'; ++i)
	{
		g_FontDescription.nCharOffsets[i] = (i-'A')*8+1;
	}
	for(uint32_t i = 'a'; i <= 'z'; ++i)
	{
		g_FontDescription.nCharOffsets[i] = (i-'a')*8+217;
	}
	for(uint32_t i = '0'; i <= '9'; ++i)
	{
		g_FontDescription.nCharOffsets[i] = (i-'0')*8+433;
	}
	for(uint32_t i = '!'; i <= '/'; ++i)
	{
		g_FontDescription.nCharOffsets[i] = (i-'!')*8+513;
	}

	for(uint32_t i = ':'; i <= '@'; ++i)
	{
		g_FontDescription.nCharOffsets[i] = (i-':')*8+625+8;
	}

	for(uint32_t i = '['; i <= '_'; ++i)
	{
		g_FontDescription.nCharOffsets[i] = (i-'[')*8+681+8;
	}

	for(uint32_t i = '{'; i <= '~'; ++i)
	{
		g_FontDescription.nCharOffsets[i] = (i-'{')*8+721+8;
	}
	int x,y,comp;

	const char* fname = "font2.png";
	

	const unsigned char* pImage = stbi_load(fname, &x, &y, &comp, 4);
	uint32_t* p4 = (uint32_t*)pImage;
	for(uint32_t i = 0; i < x*y; ++i)
	{
		if(0 == (0xff & p4[i]))
			p4[i] = ~p4[i] | 0xff000000;
		else
			p4[i] = ~p4[i] & 0xffffff;
	}

	glGenTextures(1, &g_FontDescription.nTextureId);
	glBindTexture(GL_TEXTURE_2D, g_FontDescription.nTextureId);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
    }

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &p4[0]);

	free((void*)pImage);

	//char array[] = "$+-*/=%\\\"'#@&_(),.;:?!|{}<>[]'^~";
	//uint32_t nLen = strlen(array);
	//std::sort(&array[0], &array[nLen]);
	//for(uint32_t i = 0; i < nLen; ++i)
	//{
	//	uprintf("%c : %d\n", array[i], array[i]);
	//}
	g_TextRenderState.nFnxtPos = UPLOTF_START;

	glBindTexture(GL_TEXTURE_2D, 0);
}

extern void SetProgramTexture(uint32_t eType, const char* name, uint32_t nTextureId, uint32_t nIndex = 0);

void TextFlush()
{
	if(g_lShowDebug && g_lShowDebugText)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if(g_nTextX2)
			glOrtho(0, g_Width/2,g_Height/2, 0, 1, -1);
		else
			glOrtho(0, g_Width,g_Height, 0, 1, -1);
				
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, g_FontDescription.nTextureId);
		
		const float fEndV = 9.f / 16.f;
		const float fOffsetU = 5.f / 1024.f;

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);

		glBegin(GL_QUADS);
		glColor3f(1.f,1.f,1.f);
		for(uint32_t i = 0; i < g_TextRenderState.Plots.Size(); ++i)
		{
			int nX = g_TextRenderState.Plots[i].nX;
			int nY = g_TextRenderState.Plots[i].nY;
			int nLen = g_TextRenderState.Plots[i].nCount;
			const unsigned char* pStr = (unsigned char*)&g_TextRenderState.Lines[nY].c[nX];
			float fX = nX*TEXT_CHAR_WIDTH;
			float fY = nY*(TEXT_CHAR_HEIGHT+1);
			float fY2 = fY + (TEXT_CHAR_HEIGHT+1);
			for(uint32_t j = 0; j < nLen; ++j)
			{
				int16_t nOffset = g_FontDescription.nCharOffsets[*pStr++];
				float fOffset = nOffset / 1024.f;
				glTexCoord2f(fOffset, 0.f);
				glVertex2f(fX, fY);
				glTexCoord2f(fOffset+fOffsetU, 0.f);
				glVertex2f(fX+TEXT_CHAR_WIDTH, fY);
				glTexCoord2f(fOffset+fOffsetU, 1.f);
				glVertex2f(fX+TEXT_CHAR_WIDTH, fY2);
				glTexCoord2f(fOffset, 1.f);
				glVertex2f(fX, fY2);
				fX += TEXT_CHAR_WIDTH+1;
			}
			
		}
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}
	g_TextRenderState.Plots.Clear();
	g_TextRenderState.nFnxtPos = UPLOTF_START;
}


void uplotfnxt_inner(const char* s, uint32_t nX, uint32_t nY)
{
	uint32_t nLen = strlen(s);
	if(g_TextRenderState.Plots.Size() != g_TextRenderState.Plots.Capacity() && nY < TEXT_SCREEN_HEIGHT && nX < TEXT_SCREEN_WIDTH)
	{
		SPlot* pPlot = g_TextRenderState.Plots.PushBack();
		pPlot->nX = nX;
		pPlot->nY = nY;
		pPlot->nCount = nLen;
		char* pDest = &g_TextRenderState.Lines[nY].c[0];
		uint32_t nEnd = Min<uint32_t>(nLen + nX, TEXT_SCREEN_WIDTH);
		for(uint32_t i = nX; i < nEnd; ++i)
			pDest[i] = *s++;
	}
}

#ifndef _WIN32
#define vsprintf_s vsprintf
#endif


void uplotfnxt(const char* fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start (args, fmt);
	vsprintf_s (buffer, fmt, args);
	uplotfnxt_inner(buffer, 0, g_TextRenderState.nFnxtPos++);
	va_end (args);

}

void uplotf(uint32_t nX, uint32_t nY, const char* fmt, ...)
{
	char buffer[1024];
	va_list args;
	va_start (args, fmt);
	vsprintf_s (buffer, fmt, args);
	uplotfnxt_inner(buffer, nX, nY);
	va_end (args);
}



void TextBegin()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_FontDescription.nTextureId);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glBegin(GL_QUADS);
	glColor3f(1.f,1.f,1.f);

}
void TextEnd()
{
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void TextPut(uint32_t nX, uint32_t nY, const char* pStr, uint32_t nNumChars)
{
	const float fEndV = 9.f / 16.f;
	const float fOffsetU = 5.f / 1024.f;
	int nLen = nNumChars == (uint32_t)-1 ? strlen(pStr) : nNumChars;
	float fX = nX;
	float fY = nY;
	float fY2 = fY + (TEXT_CHAR_HEIGHT+1);
	for(uint32_t j = 0; j < nLen; ++j)
	{
		int16_t nOffset = g_FontDescription.nCharOffsets[*pStr++];
		float fOffset = nOffset / 1024.f;
		glTexCoord2f(fOffset, 0.f);
		glVertex2f(fX, fY);
		glTexCoord2f(fOffset+fOffsetU, 0.f);
		glVertex2f(fX+TEXT_CHAR_WIDTH, fY);
		glTexCoord2f(fOffset+fOffsetU, 1.f);
		glVertex2f(fX+TEXT_CHAR_WIDTH, fY2);
		glTexCoord2f(fOffset, 1.f);
		glVertex2f(fX, fY2);
		fX += TEXT_CHAR_WIDTH+1;
	}
}


