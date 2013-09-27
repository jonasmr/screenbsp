#include "debug.h"
#include "fixedarray.h"
#include "glinc.h"
#include "mesh.h"
#include "shader.h"
#include "buffer.h"
#include "microprofile.h"


namespace
{
	SVertexBufferDynamic* pPushBuffer = 0;
}

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
struct SDebugDrawSphere
{
	v3 vCenter;
	float fRadius;
	uint32_t nColor;
};

struct SDebugDrawBounds
{
	m mat;
	v3 vSize;
	uint32 nColor;
	uint32 nUseZ;
};

struct SDebugDrawPlane
{
	v3 vNormal;
	v3 vPosition;
	uint32_t nColor;
};

struct SDebugDrawState
{
	TFixedArray<SDebugDrawLine, 16<<10> Lines;
	TFixedArray<SDebugDrawPlane, 128> Planes;
	TFixedArray<SDebugDrawPoly, 2048> Poly;

	TFixedArray<v4, 2048*4> PolyVert;

	TFixedArray<SDebugDrawSphere, 16<<10> Spheres;
	TFixedArray<SDebugDrawBounds, 2048> Bounds;
	TFixedArray<SDebugDrawBounds, 2048> Boxes;
} g_DebugDrawState;


void DebugDrawPlane(v3 vNormal, v3 vPosition, uint32_t nColor)
{
	if(g_DebugDrawState.Planes.Full()) return;
	SDebugDrawPlane* pPlane = g_DebugDrawState.Planes.PushBack();
	ZASSERT(v3length(vNormal) > 0.8f);
	pPlane->vNormal = vNormal;
	pPlane->vPosition = vPosition;
	pPlane->nColor = nColor;
}

void DebugDrawBounds(m mObjectToWorld, v3 vSize, uint32 nColor)
{
	if(g_DebugDrawState.Bounds.Full()) return;

	SDebugDrawBounds* pBounds = g_DebugDrawState.Bounds.PushBack();
	pBounds->mat = mObjectToWorld;
	pBounds->vSize = vSize;
	pBounds->nColor = nColor;
}
void DebugDrawBox(m rot, v3 pos, v3 size, uint32 nColor, uint32 nUseZ)
{
	if(g_DebugDrawState.Boxes.Full()) return;

	SDebugDrawBounds * pBox = g_DebugDrawState.Boxes.PushBack();

	pBox->mat = rot;
	pBox->mat.trans = v4init(pos, 1);
	pBox->vSize = size;
	pBox->nColor = nColor;
	pBox->nUseZ = nUseZ;


}

void DebugDrawSphere(v3 vCenter, float fRadius, uint32_t nColor)
{
	if(g_DebugDrawState.Spheres.Full()) return;
	SDebugDrawSphere* pSphere = g_DebugDrawState.Spheres.PushBack();
	pSphere->vCenter = vCenter;
	pSphere->fRadius = fRadius;
	pSphere->nColor = nColor;
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


#define PLANE_DEBUG_TESSELATION 20
#define PLANE_DEBUG_SIZE 5


void DebugDrawPlane(SDebugDrawPlane& Plane)
{
	v3 vNormal = Plane.vNormal;
	v3 vPosition = Plane.vPosition;
	v3 vLeft = v3cross(vNormal, v3init(1,0,0));
	if(v3length(vLeft) < 0.1f)
		vLeft = v3cross(vNormal, v3init(0,1,0));
	if(v3length(vLeft) < 0.51f)
		vLeft = v3cross(vNormal, v3init(0,0,1));

	ZASSERT(v3length(vLeft) > 0.4f);
	vLeft = v3normalize(vLeft);
	v3 vUp = v3normalize(v3cross(vNormal, vLeft));
	vLeft = v3normalize(v3cross(vUp, vNormal));
	v3 vPoints[PLANE_DEBUG_TESSELATION * PLANE_DEBUG_TESSELATION];
	for(int i = 0; i < PLANE_DEBUG_TESSELATION; ++i)
	{
		float x = PLANE_DEBUG_SIZE * (((float)i / (PLANE_DEBUG_TESSELATION-1)) - 0.5f);
		for(int j = 0; j < PLANE_DEBUG_TESSELATION; ++j)
		{
			float y = PLANE_DEBUG_SIZE * (((float)j / (PLANE_DEBUG_TESSELATION-1)) - 0.5f);
			vPoints[i*PLANE_DEBUG_TESSELATION+j] = vPosition + vUp * x + vLeft * y;
		}
	}


	int nCount = 6 * (PLANE_DEBUG_TESSELATION-1) * (PLANE_DEBUG_TESSELATION-1);
	Vertex0* pLines = (Vertex0*)VertexBufferPushVertices(pPushBuffer, 
		nCount, EDM_LINES);
	uint32_t color = Plane.nColor;
	for(int i = 0; i < PLANE_DEBUG_TESSELATION-1; ++i)
	{
		for(int j = 0; j < PLANE_DEBUG_TESSELATION-1; ++j)
		{
			v3 p0 = vPoints[i*PLANE_DEBUG_TESSELATION+j];
			v3 p1 = vPoints[i*PLANE_DEBUG_TESSELATION+j+1];
			v3 p2 = vPoints[(1+i)*PLANE_DEBUG_TESSELATION+j];
			v3 p3 = vPoints[i*PLANE_DEBUG_TESSELATION+j];
			v3 p4 = vPoints[i*PLANE_DEBUG_TESSELATION+j];
			v3 p5 = vPoints[i*PLANE_DEBUG_TESSELATION+j] + vNormal * 0.03f;
			*pLines++ = Vertex0(p0.x, p0.y, p0.z, color, 0.f, 0.f);
			*pLines++ = Vertex0(p1.x, p1.y, p1.z, color, 0.f, 0.f);
			*pLines++ = Vertex0(p2.x, p2.y, p2.z, color, 0.f, 0.f);
			*pLines++ = Vertex0(p3.x, p3.y, p3.z, color, 0.f, 0.f);
			*pLines++ = Vertex0(p4.x, p4.y, p4.z, color, 0.f, 0.f);
			*pLines++ = Vertex0(p5.x, p5.y, p5.z, color, 0.f, 0.f);
			nCount -= 6;
		}
	}
	ZASSERT(0 == nCount);


}



namespace
{
	void DebugDrawBox(const SDebugDrawBounds& Box)
	{

		SHADER_SET("ModelViewMatrix", Box.mat);
		SHADER_SET("UseVertexColor", 1.f);
		SHADER_SET("Size", Box.vSize);
		uint32_t nColor = Box.nColor|0xff000000;
		Vertex0* pLines = (Vertex0*)VertexBufferPushVertices(pPushBuffer, 8 * 3, EDM_LINES);
		float f = 1.0f;
		*pLines++ = Vertex0(f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, f, -f, nColor, 0.f, 0.f);
		
		*pLines++ = Vertex0(-f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, -f, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(-f, -f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, -f, -f, nColor, 0.f, 0.f);
		
		*pLines++ = Vertex0(f, -f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, -f, nColor, 0.f, 0.f);


		*pLines++ = Vertex0(f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, f, nColor, 0.f, 0.f);
		
		*pLines++ = Vertex0(-f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, -f, f, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(-f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, -f, -f, nColor, 0.f, 0.f);
		
		*pLines++ = Vertex0(f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, -f, nColor, 0.f, 0.f);


		*pLines++ = Vertex0(f,  f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, f, nColor, 0.f, 0.f);
		
		*pLines++ = Vertex0(f,  -f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, -f, f, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(f,  -f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, -f, -f, nColor, 0.f, 0.f);
		
		*pLines++ = Vertex0(f,  f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, -f, nColor, 0.f, 0.f);
		//VertexBufferPushFlush(pPushBuffer);
	}
	void DebugDrawBounds(const SDebugDrawBounds& Bounds)
	{			

		v3 vScale = Bounds.vSize;
		vScale = v3max(vScale, 0.1f) * 1.07f;
		SHADER_SET("ModelViewMatrix", Bounds.mat);
		SHADER_SET("UseVertexColor", 1.f);
		SHADER_SET("Size", vScale);
		uint32_t nColor = Bounds.nColor;
		float f = 1.0f;
		float z = 0.64f;
		Vertex0* pLines = (Vertex0*)VertexBufferPushVertices(pPushBuffer, 8 * 6, EDM_LINES);
		*pLines++ = Vertex0(f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(z, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, z, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, f, z, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(-f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-z, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, z, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, z, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(f, -f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(z, -f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -z, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, z, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(-f,-f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-z,-f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-z, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-f, f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-f, z, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(z, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, z, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, f, -z, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(-f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-z, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, z, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f, f, -z, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(f, -f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(z, -f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -z, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(f, -f, -z, nColor, 0.f, 0.f);

		*pLines++ = Vertex0(-f,-f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-z,-f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-z, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-f, -f, nColor, 0.f, 0.f);
		*pLines++ = Vertex0(-f,-f, -z, nColor, 0.f, 0.f);
		//VertexBufferPushFlush(pPushBuffer);

	}
}


void DebugDrawFlush(m mprj)
{
	if(g_lShowDebug)
	{
		MICROPROFILE_SCOPEI("Debug", "DebugDrawFlush", 0xff00ff00);
		MICROPROFILE_SCOPEGPUI("GPU", "DebugDrawFlush", 0x44bb44);

		if(!pPushBuffer)
		{
			pPushBuffer = VertexBufferCreatePush(EVF_FORMAT0, 64 << 10);
			CheckGLError();
		}
		glDisable(GL_CULL_FACE);




		//ENABLE DEPTH
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		ShaderUse(VS_DEBUG, PS_DEBUG);

		SHADER_SET("Color", v4init(1,0,0,1));
		SHADER_SET("ProjectionMatrix", mprj);
		SHADER_SET("ModelViewMatrix", mid());
		SHADER_SET("UseVertexColor", 1.f);
		SHADER_SET("Size", v3init(1,1,1));


		for(SDebugDrawBounds& Box : g_DebugDrawState.Boxes)
		{
			if(Box.nUseZ)
			{
				DebugDrawBox(Box);
				VertexBufferPushFlush(pPushBuffer);
			}
		}






		//DISABLE DEPTH
		glDisable(GL_DEPTH_TEST);
		glDepthMask(0);

		for(SDebugDrawBounds& Box : g_DebugDrawState.Boxes)
		{
			if(!Box.nUseZ)
			{
				DebugDrawBox(Box);
				VertexBufferPushFlush(pPushBuffer);
			}
		}
		VertexBufferPushFlush(pPushBuffer);


		ShaderUse(VS_DEBUG, PS_DEBUG);
		SHADER_SET("Color", v4init(1,0,0,0));
		SHADER_SET("ProjectionMatrix", mprj);
		SHADER_SET("ModelViewMatrix", mid());
		SHADER_SET("UseVertexColor", 1.f);
		SHADER_SET("Size", v3init(1,1,1));

		for(SDebugDrawBounds& Bounds : g_DebugDrawState.Bounds)
		{
			DebugDrawBounds(Bounds);
			VertexBufferPushFlush(pPushBuffer);
		}
		

		{
			ShaderUse(VS_DEBUG, PS_DEBUG);
			SHADER_SET("Color", v4init(1,0,0,0));
			SHADER_SET("ProjectionMatrix", mprj);
			SHADER_SET("ModelViewMatrix", mid());
			SHADER_SET("UseVertexColor", 1.f);
			SHADER_SET("Size", v3init(1,1,1));

			Vertex0* pLines = (Vertex0*)VertexBufferPushVertices(pPushBuffer, g_DebugDrawState.Lines.Size() * 2, EDM_LINES);
			for(uint32_t i = 0; i < g_DebugDrawState.Lines.Size(); ++i)
			{
				uint32_t nColor = g_DebugDrawState.Lines[i].nColor;
				nColor = -1;
				pLines[i*2+0].nColor = nColor;
				pLines[i*2+0].nX = g_DebugDrawState.Lines[i].start.x;
				pLines[i*2+0].nY = g_DebugDrawState.Lines[i].start.y;
				pLines[i*2+0].nZ = g_DebugDrawState.Lines[i].start.z;
				pLines[i*2+1].nColor = nColor;
				pLines[i*2+1].nX = g_DebugDrawState.Lines[i].end.x;
				pLines[i*2+1].nY = g_DebugDrawState.Lines[i].end.y;
				pLines[i*2+1].nZ = g_DebugDrawState.Lines[i].end.z;
			}
			VertexBufferPushFlush(pPushBuffer);
		}

		glEnable(GL_DEPTH_TEST);
		glDepthMask(1);

		v4* pVert = g_DebugDrawState.PolyVert.Ptr();
		for(uint32 i = 0; i < g_DebugDrawState.Poly.Size(); ++i)
		{
			uint32_t nColor = g_DebugDrawState.Poly[i].nColor;
			uint32 nVertices = g_DebugDrawState.Poly[i].nVertices;
			uint32 nStart = g_DebugDrawState.Poly[i].nStart;
			if(nVertices < 3)
			{
				uprintf("empty poly!\n");
				continue;
			}
			Vertex0* pVertices = (Vertex0*)VertexBufferPushVertices(pPushBuffer, 3 *(nVertices-2), EDM_TRIANGLES);
			for(uint32 j = 1; j < nVertices-1; ++j)
			{
				v4 v0 = pVert[nStart];
				v4 v1 = pVert[nStart+j];
				v4 v2 = pVert[nStart+j+1];
				*pVertices++ = Vertex0(v0.x, v0.y, v0.z, nColor, 0.f, 0.f);
				*pVertices++ = Vertex0(v1.x, v1.y, v1.z, nColor, 0.f, 0.f);
				*pVertices++ = Vertex0(v2.x, v2.y, v2.z, nColor, 0.f, 0.f);
			}
		}
		VertexBufferPushFlush(pPushBuffer);

		CheckGLError();
		{
			glDepthMask(0);

			glDisable(GL_DEPTH_TEST);
			ShaderUse(VS_DEBUG, PS_DEBUG);
			int nSizeLoc = ShaderGetLocation("Size");
			int nColorLoc = ShaderGetLocation("Color");
			SHADER_SET("UseVertexColor", 1.f);
			SHADER_SET("Color", v4init(1,0,0,0));
			SHADER_SET("ProjectionMatrix", mprj);
			SHADER_SET("ModelViewMatrix", mid());
			SHADER_SET("Size", v3init(1,1,1));


			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			for(uint32_t i = 0; i < g_DebugDrawState.Spheres.Size(); ++i)
			{
				SDebugDrawSphere& Sphere = g_DebugDrawState.Spheres[i];
				m m0 = mid();
				v3 vPos = Sphere.vCenter;
				m0.trans = v4init(vPos, 1.f);
				ShaderSetUniform(nSizeLoc, v3rep(Sphere.fRadius));
				ShaderSetUniform(nColorLoc, v4fromcolor(Sphere.nColor));
				SHADER_SET("ModelViewMatrix", m0);
				SHADER_SET("ConstantColor", v4fromcolor(Sphere.nColor|0xff000000));
				MeshDraw(GetBaseMesh(MESH_SPHERE_1));
				uplotfnxt("SPHERE %f %f %f :: radius %f", vPos.x, vPos.y, vPos.z, Sphere.fRadius);

			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		


		SHADER_SET("Color", v4init(1,0,0,0));
		SHADER_SET("ProjectionMatrix", mprj);
		SHADER_SET("ModelViewMatrix", mid());
		SHADER_SET("UseVertexColor", 1.f);
		SHADER_SET("Size", v3init(1,1,1));


		for(SDebugDrawPlane& Plane : g_DebugDrawState.Planes)
		{
			DebugDrawPlane(Plane);

		}
		VertexBufferPushFlush(pPushBuffer);



	}
	g_DebugDrawState.Lines.Clear();
	g_DebugDrawState.Poly.Clear();
	g_DebugDrawState.PolyVert.Clear();
	g_DebugDrawState.Bounds.Clear();
	g_DebugDrawState.Boxes.Clear();
	g_DebugDrawState.Spheres.Clear();
	g_DebugDrawState.Planes.Clear();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	CheckGLError();
}