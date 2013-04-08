#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <string>
#include "base.h"
#include "input.h"
#include "glinc.h"
#include "text.h"
#include "math.h"
#include "program.h"
#include "bsp.h"
#include "debug.h"


extern uint32_t g_Width;
extern uint32_t g_Height;
uint32 g_nUseOrtho = 0;
float g_fOrthoScale = 10;

SOccluderBsp* g_Bsp = 0;



SWorldState g_WorldState;


void DebugRender()
{
	if(g_WorldState.pSelected)
	{
		ZDEBUG_DRAWBOUNDS(g_WorldState.pSelected->mObjectToWorld, g_WorldState.pSelected->vSize, -1);

	}
	// for(int i = 0; i < g_WorldState.nNumOccluders; ++i)
	// {
	// }

	// for(int i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	// {
	// 	ZDEBUG_DRAWBOUNDS(g_WorldState.WorldObjects[i].mObjectToWorld, g_WorldState.WorldObjects[i].vSize, -1);
	// }
	//root
	glBegin(GL_LINES);
	glColor3f(1,0,0);
	glVertex3f(0, 0, 0.f);
	glVertex3f(1.f, 0, 0.f);
	glColor3f(0,1,0);
	glVertex3f(0, 0, 0.f);
	glVertex3f(0.f, 1.f, 0.f);
	glColor3f(0,0,1);
	glVertex3f(0, 0, 0.f);
	glVertex3f(0.f, 0, 1.f);
	glEnd();


	DebugDrawFlush();
}

void WorldInit()
{
	g_WorldState.Occluders[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[0].mObjectToWorld.trans = v4init(2.f,0.f,0.1f, 1.f);
	g_WorldState.Occluders[0].vSize = v3init(0.5f, 0.5f, 0.f);

	g_WorldState.Occluders[1].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(2.f,0.f,-1.f, 1.f);
	g_WorldState.Occluders[1].vSize = v3init(0.65f, 1.9f, 0);
	g_WorldState.nNumOccluders = 2;

	g_WorldState.WorldObjects[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f,0.f,-0.5f, 1.f);
	g_WorldState.WorldObjects[0].vSize = v3init(0.25f, 0.2f, 0.2);
	
	g_WorldState.nNumWorldObjects = 1;
	g_WorldState.pSelected = 0;
}

int g_nSimulate = 0;
void WorldRender()
{
	if(g_KeyboardState.keys[SDLK_F1] & BUTTON_RELEASED)
	{
		g_nSimulate = !g_nSimulate;
	}
	if(g_nSimulate)
	{
		g_WorldState.WorldObjects[0].mObjectToWorld = mmult(g_WorldState.WorldObjects[0].mObjectToWorld, mrotatey(0.5f*TORAD));
		g_WorldState.Occluders[0].mObjectToWorld = mmult(g_WorldState.Occluders[0].mObjectToWorld, mrotatez(0.3f*TORAD));
		static float xx = 0;
		xx += 0.005f;
		g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f, 0.f, sin(xx)*2, 1.f);
	}

	uint32 nNumOccluders = g_WorldState.nNumOccluders;
	BspBuild(g_Bsp, &g_WorldState.Occluders[0], nNumOccluders, mid());
	uint32 nNumObjects = g_WorldState.nNumWorldObjects;
	bool* bCulled = (bool*)alloca(nNumObjects);
	for(uint32 i = 0; i < nNumObjects; ++i)
	{
		bCulled[i] = BspCullObject(g_Bsp, &g_WorldState.WorldObjects[i]);

	}
	uplotfnxt("culled %d", bCulled[0] ? 1: 0);

	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		glPushMatrix();
		glMultMatrixf(&g_WorldState.WorldObjects[i].mObjectToWorld.x.x);
		float x = g_WorldState.WorldObjects[i].vSize.x;
		float y = g_WorldState.WorldObjects[i].vSize.y;
		float z = g_WorldState.WorldObjects[i].vSize.z;
		glBegin(GL_LINE_STRIP);
		if(!bCulled[i])
			glColor3f(0,1,0);
		else
			glColor3f(1,0,0);

		glVertex3f(x, y, z);
		glVertex3f(x, -y, z);
		glVertex3f(-x, -y, z);
		glVertex3f(-x, y, z);
		glVertex3f(x, y, z);
		glEnd();

		glBegin(GL_LINE_STRIP);
		//glColor3f(0,1,0);
		glVertex3f(x, y, -z);
		glVertex3f(x, -y, -z);
		glVertex3f(-x, -y, -z);
		glVertex3f(-x, y, -z);
		glVertex3f(x, y, -z);
		glEnd();

		glBegin(GL_LINES);
		//glColor3f(0,1,0);
		glVertex3f(x, y, -z);
		glVertex3f(x, y, z);
		glVertex3f(x, -y, -z);
		glVertex3f(x, -y, z);
		glVertex3f(-x, -y, -z);
		glVertex3f(-x, -y, z);
		glVertex3f(-x, y, -z);
		glVertex3f(-x, y, z);
		glVertex3f(x, y, -z);
		glVertex3f(x, y, z);
		glEnd();


		glPopMatrix();
	}

}

void UpdatePicking()
{

	v3 vMouse = v3init( g_MouseState.position[0], g_MouseState.position[1], 0);
	v3 vMouseClip = mtransform(g_WorldState.Camera.mviewportinv, vMouse);
	vMouseClip.z = -0.9f;
	v4 vMouseView_ = mtransform(g_WorldState.Camera.mprjinv, v4init(vMouseClip, 1.f));
	v3 vMouseView = vMouseView_.tov3() / vMouseView_.w;
	v3 vMouseWorld = mtransform(g_WorldState.Camera.mviewinv, vMouseView);
	uplotfnxt("vMouse %f %f %f", vMouse.x, vMouse.y, vMouse.z);
	uplotfnxt("vMouseClip %f %f %f", vMouseClip.x, vMouseClip.y, vMouseClip.z);
	uplotfnxt("vMouseView %f %f %f", vMouseView.x, vMouseView.y, vMouseView.z);
	uplotfnxt("vMouseWorld %f %f %f", vMouseWorld.x, vMouseWorld.y, vMouseWorld.z);
	v3 test = g_WorldState.Camera.vPosition + g_WorldState.Camera.vDir * 3.f;
	v3 t0 = vMouseWorld;
	ZDEBUG_DRAWLINE(test, t0, -1, 0);

	if(g_MouseState.button[1] & BUTTON_RELEASED)
	{
		v3 vPos = g_WorldState.Camera.vPosition;
		v3 vDir = vMouseWorld - vPos;
		vDir = v3normalize(vDir);
		float fNearest = 1e30;
		SObject* pNearest = 0;
		int a = []( int b ){ int r=1; while (b>0) r*=b--; return r; }(5); // 5!

		auto Intersect = [&] (SObject* pObject) 
		{ 
			float fColi = rayboxintersect(vPos, vDir, pObject->mObjectToWorld, pObject->vSize);
			if(fColi > g_WorldState.Camera.fNear && fColi < fNearest)
			{
				pNearest = pObject;
				fNearest = fColi;
			}
		};
		for(SObject& Obj : g_WorldState.WorldObjects)
			Intersect(&Obj);

		for(SObject& Obj : g_WorldState.Occluders)
			Intersect(&Obj);
		g_WorldState.pSelected = pNearest;
	}


}

void UpdateCamera()
{
	v3 vDir = v3init(0,0,0);
	if(g_MouseState.button[SDL_BUTTON_WHEELUP] & BUTTON_RELEASED)
		g_fOrthoScale *= 0.96;
	if(g_MouseState.button[SDL_BUTTON_WHEELDOWN] & BUTTON_RELEASED)
		g_fOrthoScale /= 0.96;
	uplotfnxt("ORTHO SCALE %f\n", g_fOrthoScale);



	if(g_KeyboardState.keys['a'] & BUTTON_DOWN)
	{
		vDir.x = -1.f;
	}
	else if(g_KeyboardState.keys['d'] & BUTTON_DOWN)
	{
		vDir.x = 1.f;
	}

	if(g_KeyboardState.keys['w'] & BUTTON_DOWN)
	{
		vDir.z = 1.f;
	}
	else if(g_KeyboardState.keys['s'] & BUTTON_DOWN)
	{
		vDir.z = -1.f;
	}
	float fSpeed = 0.1f;
	if((g_KeyboardState.keys[SDLK_RSHIFT]|g_KeyboardState.keys[SDLK_LSHIFT]) & BUTTON_DOWN)
		fSpeed *= 0.2f;
	g_WorldState.Camera.vPosition += g_WorldState.Camera.vDir * vDir.z * fSpeed;
	g_WorldState.Camera.vPosition += g_WorldState.Camera.vRight * vDir.x * fSpeed;


	static int mousex, mousey;
	if(g_MouseState.button[1] & BUTTON_PUSHED)
	{
		mousex = g_MouseState.position[0];
		mousey = g_MouseState.position[1];
	}

	if(g_MouseState.button[1] & BUTTON_DOWN)
	{
		int dx = g_MouseState.position[0] - mousex;
		int dy = g_MouseState.position[1] - mousey;
		mousex = g_MouseState.position[0];
		mousey = g_MouseState.position[1];

		float fRotX = dy * 0.25f;
		float fRotY = dx * -0.25f;
		m mrotx = mrotatex(fRotX*TORAD);
		m mroty = mrotatey(fRotY*TORAD);
		g_WorldState.Camera.vDir = mtransform(mroty, g_WorldState.Camera.vDir);
		g_WorldState.Camera.vRight = mtransform(mroty, g_WorldState.Camera.vRight);

		m mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, v3init(0,0,0));
		m mviewinv = minverserotation(mview);
		v3 vNewDir = mtransform(mrotx, v3init(0,0,-1));
		g_WorldState.Camera.vDir = mtransform(mviewinv, vNewDir);


	}



	g_WorldState.Camera.mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, g_WorldState.Camera.vPosition);
	if(g_KeyboardState.keys[SDLK_F2] & BUTTON_RELEASED)
		g_nUseOrtho = !g_nUseOrtho;

	float fAspect = (float)g_Width / (float)g_Height;
	
	if(g_nUseOrtho)
	{
		g_WorldState.Camera.mprj = mortho(g_fOrthoScale, g_fOrthoScale / fAspect, 1000);
	}
	else
	{
		g_WorldState.Camera.mprj = mperspective(45, ((float)g_Height / (float)g_Width), 0.001f, 100.f);
	}
	g_WorldState.Camera.mviewport = mviewport(0,0,g_Width, g_Height);

	g_WorldState.Camera.mviewportinv = minverse(g_WorldState.Camera.mviewport);
	g_WorldState.Camera.mprjinv = minverse(g_WorldState.Camera.mprj);
	g_WorldState.Camera.mviewinv = maffineinverse(g_WorldState.Camera.mview);

	uplotfnxt("FPS %4.2f Dir[%5.2f,%5.2f,%5.2f] Pos[%3.2f,%3.2f,%3.2f]" , 1.f, 
		g_WorldState.Camera.vDir.x,
		g_WorldState.Camera.vDir.y,
		g_WorldState.Camera.vDir.z,
		g_WorldState.Camera.vPosition.x,
		g_WorldState.Camera.vPosition.y,
		g_WorldState.Camera.vPosition.z);


}
void foo()
{
	uprintf("lala\n");
	ZBREAK();
}

void ProgramInit()
{
	m mroty = mrotatey(45.f * TORAD);
	g_WorldState.Camera.vDir = mtransform(mroty, v3init(0,0,-1));
	g_WorldState.Camera.vRight = mtransform(mroty, v3init(1,0,0));
	g_WorldState.Camera.vPosition = g_WorldState.Camera.vDir * -5.f;
	g_Bsp = BspCreate();
	WorldInit();
}

int ProgramMain()
{

	if(g_KeyboardState.keys[SDLK_ESCAPE] & BUTTON_RELEASED)
		return 1;

	{
		UpdateCamera();
		UpdatePicking();
	}

	glLineWidth(1.f);
	glClearColor(0.3,0.4,0.6,0);
	glViewport(0, 0, g_Width, g_Height);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&g_WorldState.Camera.mprj.x.x);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&g_WorldState.Camera.mview.x.x);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CheckGLError();
	WorldRender();
	DebugRender();


	glPopMatrix();

	return 0;
}