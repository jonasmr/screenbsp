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


struct SCameraState
{
	v3 vDir;
	v3 vRight;
	v3 vPosition;
} g_Camera;


SOccluder g_Occluders[2];
SWorldObject g_WorldObjects[1];

void DebugRender()
{
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
	g_Occluders[0].mObjectToWorld = mrotatey(90*TORAD);
	g_Occluders[0].mObjectToWorld.trans = v4init(2.f,0.f,0.1f, 1.f);
	g_Occluders[0].vSize = v3init(0.5f, 0.5f, 0.f);

	g_Occluders[1].mObjectToWorld = mrotatey(90*TORAD);
	g_Occluders[1].mObjectToWorld.trans = v4init(2.f,0.f,-1.f, 1.f);
	g_Occluders[1].vSize = v3init(0.65f, 1.9f, 0);

	g_WorldObjects[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldObjects[0].mObjectToWorld.trans = v4init(3.f,0.f,-0.5f, 1.f);
	g_WorldObjects[0].vSize = v3init(0.25f, 0.2f, 0.2);
	
	
}

int g_nSimulate = 1;
void WorldRender()
{
	if(g_KeyboardState.keys[SDLK_F1] & BUTTON_RELEASED)
	{
		g_nSimulate = !g_nSimulate;
	}
	if(g_nSimulate)
	{
		g_WorldObjects[0].mObjectToWorld = mmult(g_WorldObjects[0].mObjectToWorld, mrotatey(0.5f*TORAD));
		g_Occluders[0].mObjectToWorld = mmult(g_Occluders[0].mObjectToWorld, mrotatez(0.3f*TORAD));
	}

	// for(uint32 i = 0; i < 2; ++i)
	// {
	// 	glPushMatrix();
	// 	glMultMatrixf(&g_Occluders[i].mObjectToWorld.x.x);
	// 	float x = g_Occluders[i].vSize.x;
	// 	float y = g_Occluders[i].vSize.y;
	// 	glBegin(GL_LINE_STRIP);
	// 	glColor3f(1,1,1);
	// 	glVertex3f(x, y, 0.f);
	// 	glVertex3f(x, -y, 0.f);
	// 	glVertex3f(-x, -y, 0.f);
	// 	glVertex3f(-x, y, 0.f);
	// 	glVertex3f(x, y, 0.f);
	// 	glEnd();
	// 	glPopMatrix();
	// }

	//void BspOccluderTest(SOccluder* pOccluders, uint32 nNumOccluders)
	BspOccluderTest(&g_Occluders[0], 1, &g_WorldObjects[0], 1);
	for(uint32 i = 0; i < 1; ++i)
	{
		glPushMatrix();
		glMultMatrixf(&g_WorldObjects[i].mObjectToWorld.x.x);
		float x = g_WorldObjects[i].vSize.x;
		float y = g_WorldObjects[i].vSize.y;
		float z = g_WorldObjects[i].vSize.z;
		glBegin(GL_LINE_STRIP);
		glColor3f(0,1,0);
		glVertex3f(x, y, z);
		glVertex3f(x, -y, z);
		glVertex3f(-x, -y, z);
		glVertex3f(-x, y, z);
		glVertex3f(x, y, z);
		glEnd();

		glBegin(GL_LINE_STRIP);
		glColor3f(0,1,0);
		glVertex3f(x, y, -z);
		glVertex3f(x, -y, -z);
		glVertex3f(-x, -y, -z);
		glVertex3f(-x, y, -z);
		glVertex3f(x, y, -z);
		glEnd();

		glBegin(GL_LINES);
		glColor3f(0,1,0);
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
	g_Camera.vPosition += g_Camera.vDir * vDir.z * fSpeed;
	g_Camera.vPosition += g_Camera.vRight * vDir.x * fSpeed;


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

		float fRotX = dy * -0.25f;
		float fRotY = dx * -0.25f;
		m mrotx = mrotatex(fRotX*TORAD);
		m mroty = mrotatey(fRotY*TORAD);
		g_Camera.vDir = mtransform(mroty, g_Camera.vDir);
		g_Camera.vRight = mtransform(mroty, g_Camera.vRight);

		m mview = mcreate(g_Camera.vDir, g_Camera.vRight, v3init(0,0,0));
		m mviewinv = minverserotation(mview);
		v3 vNewDir = mtransform(mrotx, v3init(0,0,-1));
		g_Camera.vDir = mtransform(mviewinv, vNewDir);


	}


}


void ProgramInit()
{
	m mroty = mrotatey(45.f * TORAD);
	g_Camera.vDir = mtransform(mroty, v3init(0,0,-1));
	g_Camera.vRight = mtransform(mroty, v3init(1,0,0));
	g_Camera.vPosition = g_Camera.vDir * -5.f;

	WorldInit();
}

int ProgramMain()
{

	if(g_KeyboardState.keys[SDLK_ESCAPE] & BUTTON_RELEASED)
		return 1;

	m mprj; 
	m mview;
	{
		UpdateCamera();
		mview = mcreate(g_Camera.vDir, g_Camera.vRight, g_Camera.vPosition);
		if(g_KeyboardState.keys[SDLK_F2] & BUTTON_RELEASED)
			g_nUseOrtho = !g_nUseOrtho;

		float fAspect = (float)g_Width / (float)g_Height;

		if(g_nUseOrtho)
		{
			mprj = mortho(g_fOrthoScale, g_fOrthoScale / fAspect, 1000);
		}
		else
		{
			mprj = mperspective(30, (float)g_Width / (float)g_Height, 0.001f, 100.f);
		}


		uplotfnxt("FPS %4.2f Dir[%5.2f,%5.2f,%5.2f] Pos[%3.2f,%3.2f,%3.2f]" , 1.f, 
			g_Camera.vDir.x,
			g_Camera.vDir.y,
			g_Camera.vDir.z,
			g_Camera.vPosition.x,
			g_Camera.vPosition.y,
			g_Camera.vPosition.z);
	}

	glLineWidth(2.f);

	glClearColor(0.3,0.4,0.6,0);
	glViewport(0, 0, g_Width, g_Height);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&mprj.x.x);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&mview.x.x);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CheckGLError();

	WorldRender();

	DebugRender();


	glPopMatrix();

	return 0;
}