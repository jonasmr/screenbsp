#include "debug.h"
#include "fixedarray.h"
#include "glinc.h"



struct SDebugDrawLine
{
	v3 start;
	v3 end;
	uint32_t nColor;
};
struct SDebugDrawPoly
{
	uint32 nStart;
	uint32 nVertices;
	uint32 nColor;
};

struct SDebugDrawBounds
{
	m mat;
	v3 vSize;
	uint32 nColor;
};

struct SDebugDrawState
{
	TFixedArray<SDebugDrawLine, 2048> Lines;
	TFixedArray<SDebugDrawPoly, 2048> Poly;
	TFixedArray<v4, 2048*4> PolyVert;

	TFixedArray<SDebugDrawBounds, 2048> Bounds;
	TFixedArray<SDebugDrawBounds, 2048> Boxes;
} g_DebugDrawState;


void DebugDrawBounds(m mObjectToWorld, v3 vSize, uint32 nColor)
{
	if(g_DebugDrawState.Bounds.Full()) return;

	SDebugDrawBounds* pBounds = g_DebugDrawState.Bounds.PushBack();
	pBounds->mat = mObjectToWorld;
	pBounds->vSize = vSize;
	pBounds->nColor = nColor;
}
void DebugDrawBox(m rot, v3 pos, v3 size, uint32 nColor)
{
	if(g_DebugDrawState.Boxes.Full()) return;

	SDebugDrawBounds * pBox = g_DebugDrawState.Boxes.PushBack();

	pBox->mat = rot;
	pBox->mat.trans = v4init(pos, 1);
	pBox->vSize = size;
	pBox->nColor = nColor;


}

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

void DebugDrawPoly(v3* pVertex, uint32 nNumVertex, uint32_t nColor)
{
	v4* pArray = (v4*)alloca(sizeof(v4)*nNumVertex);
	for(uint32 i = 0; i < nNumVertex; ++i)
		pArray[i] = v4init(pVertex[i],1.f);
	DebugDrawPoly(pArray, nNumVertex, nColor);
}

void DebugDrawPoly(v4* pVertex, uint32 nNumVertex, uint32_t nColor)
{
	if(g_DebugDrawState.Poly.Full() || g_DebugDrawState.PolyVert.Capacity() - g_DebugDrawState.PolyVert.Size() < nNumVertex)
		return;
	SDebugDrawPoly* pPoly = g_DebugDrawState.Poly.PushBack();
	pPoly->nVertices = nNumVertex;
	pPoly->nColor = nColor;
	pPoly->nStart = g_DebugDrawState.PolyVert.Size();
	g_DebugDrawState.PolyVert.PushBack(pVertex, nNumVertex);

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
		glDisable(GL_CULL_FACE);
		for(SDebugDrawBounds& Box : g_DebugDrawState.Boxes)
		{
			glPushMatrix();
			//glMultMatrixf(&Box.mat.x.x);
			glTranslatef(Box.mat.trans.x, Box.mat.trans.y, Box.mat.trans.z);
			v3 vScale = Box.vSize;
//			vScale = v3max(vScale, 0.1f) * 1.07f;
			glScalef(vScale.x, vScale.y, vScale.z);

			glBegin(GL_LINES);
			glColor3ub(Box.nColor>>16, Box.nColor>>8, Box.nColor);

			float f = 1.0f;
			glVertex3f(f, f, f);
			glVertex3f(f, f, -f);
			
			glVertex3f(-f, f, f);
			glVertex3f(-f, f, -f);

			glVertex3f(-f, -f, f);
			glVertex3f(-f, -f, -f);
			
			glVertex3f(f, -f, f);
			glVertex3f(f, -f, -f);





			glVertex3f(f, f, f);
			glVertex3f(f, -f, f);
			
			glVertex3f(-f, f, f);
			glVertex3f(-f, -f, f);

			glVertex3f(-f, f, -f);
			glVertex3f(-f, -f, -f);
			
			glVertex3f(f, f, -f);
			glVertex3f(f, -f, -f);


			glVertex3f(f,  f, f);
			glVertex3f(-f, f, f);
			
			glVertex3f(f,  -f, f);
			glVertex3f(-f, -f, f);

			glVertex3f(f,  -f, -f);
			glVertex3f(-f, -f, -f);
			
			glVertex3f(f,  f, -f);
			glVertex3f(-f, f, -f);


			glEnd();

			glPopMatrix();
		}
		for(SDebugDrawBounds& Bounds : g_DebugDrawState.Bounds)
		{
			glPushMatrix();
			glMultMatrixf(&Bounds.mat.x.x);
			v3 vScale = Bounds.vSize;
			vScale = v3max(vScale, 0.1f) * 1.07f;
			glScalef(vScale.x, vScale.y, vScale.z);

			glBegin(GL_LINES);
			glColor3ub(Bounds.nColor>>16, Bounds.nColor>>8, Bounds.nColor);

			float f = 1.0f;
			float z = 0.64f;

			glVertex3f(f, f, f);
			glVertex3f(z, f, f);
			glVertex3f(f, f, f);
			glVertex3f(f, z, f);
			glVertex3f(f, f, f);
			glVertex3f(f, f, z);

			glVertex3f(-f, f, f);
			glVertex3f(-z, f, f);
			glVertex3f(-f, f, f);
			glVertex3f(-f, z, f);
			glVertex3f(-f, f, f);
			glVertex3f(-f, f, z);

			glVertex3f(f, -f, f);
			glVertex3f(z, -f, f);
			glVertex3f(f, -f, f);
			glVertex3f(f, -z, f);
			glVertex3f(f, -f, f);
			glVertex3f(f, -f, z);

			glVertex3f(-f,-f, f);
			glVertex3f(-z,-f, f);
			glVertex3f(-f,-f, f);
			glVertex3f(-f,-z, f);
			glVertex3f(-f,-f, f);
			glVertex3f(-f,-f, z);

			glVertex3f(f, f, -f);
			glVertex3f(z, f, -f);
			glVertex3f(f, f, -f);
			glVertex3f(f, z, -f);
			glVertex3f(f, f, -f);
			glVertex3f(f, f, -z);

			glVertex3f(-f, f, -f);
			glVertex3f(-z, f, -f);
			glVertex3f(-f, f, -f);
			glVertex3f(-f, z, -f);
			glVertex3f(-f, f, -f);
			glVertex3f(-f, f, -z);

			glVertex3f(f, -f, -f);
			glVertex3f(z, -f, -f);
			glVertex3f(f, -f, -f);
			glVertex3f(f, -z, -f);
			glVertex3f(f, -f, -f);
			glVertex3f(f, -f, -z);

			glVertex3f(-f,-f, -f);
			glVertex3f(-z,-f, -f);
			glVertex3f(-f,-f, -f);
			glVertex3f(-f,-z, -f);
			glVertex3f(-f,-f, -f);
			glVertex3f(-f,-f, -z);




			glEnd();




			glPopMatrix();
		}



		v4* pVert = g_DebugDrawState.PolyVert.Ptr();
		// glEnable(GL_DEPTH_TEST);
		// glDepthMasth(G
		//glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		for(uint32 i = 0; i < g_DebugDrawState.Poly.Size(); ++i)
		{
			glBegin(GL_POLYGON);
			uint32_t nColor = g_DebugDrawState.Poly[i].nColor;
			uint32 nVertices = g_DebugDrawState.Poly[i].nVertices;
			uint32 nStart = g_DebugDrawState.Poly[i].nStart;
			glColor4ub(nColor >> 16, nColor >> 8, nColor, nColor>>24);
			for(uint32 j = 0; j < nVertices; ++j)
			{
				v4 v = pVert[nStart+j];// + v4init(i * 0.5f,0,0,0);
				glVertex3fv(&v.x);
			}

			glEnd();
		}
		glDisable(GL_BLEND);

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
	g_DebugDrawState.Poly.Clear();
	g_DebugDrawState.PolyVert.Clear();
	g_DebugDrawState.Bounds.Clear();
	g_DebugDrawState.Boxes.Clear();

}