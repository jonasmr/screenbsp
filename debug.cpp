#include "debug.h"
#include "fixedarray.h"
#include "glinc.h"



struct SDebugDrawLine
{
	v3 start;
	v3 end;
	uint32_t nColor;
};

struct SDebugDrawState
{
	TFixedArray<SDebugDrawLine, 2048> Lines;
} g_DebugDrawState;

void DebugDrawLine(v3 start, v3 end, uint32_t nColor)
{
	if(g_DebugDrawState.Lines.Full()) return;
	SDebugDrawLine* pLine = g_DebugDrawState.Lines.PushBack();
	pLine->start = start;
	pLine->end = end;
	pLine->nColor = nColor;
}
void DebugDrawLine(v4 start, v4 end, uint32_t nColor)
{
	return DebugDrawLine(start.tov3(), end.tov3(), nColor);
}

void DebugDrawBounds(v3 vmin, v3 vmax, uint32_t nColor)
{
	ZBREAK();
	v3 p0 = vmin;
	v3 p1 = vmin;
	v3 p2 = vmin;
	v3 p3 = vmax;
	p1.x += vmax.x - vmin.x;
	p2.y += vmax.y - vmin.y;
	DebugDrawLine(p0, p1, nColor);
	DebugDrawLine(p1, p3, nColor);
	DebugDrawLine(p3, p2, nColor);
	DebugDrawLine(p2, p0, nColor);
}
void DebugDrawFlush()
{
	if(g_lShowDebug)
	{
		glBegin(GL_LINES);
		for(uint32_t i = 0; i < g_DebugDrawState.Lines.Size(); ++i)
		{
			uint32_t nColor = g_DebugDrawState.Lines[i].nColor;
			glColor3ub(nColor >> 16, nColor >> 8, nColor);
			glVertex3fv((float*)&g_DebugDrawState.Lines[i].start);
			glVertex3fv((float*)&g_DebugDrawState.Lines[i].end);
		}
		glEnd();

	}
	g_DebugDrawState.Lines.Clear();

}