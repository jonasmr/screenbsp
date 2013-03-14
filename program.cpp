#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <string>
#include "base.h"
#include "input.h"
#include "glinc.h"
#include "text.h"
#include "math.h"
extern uint32_t g_Width;
extern uint32_t g_Height;


struct SCameraState
{
	v3 vDir;
	v3 vRight;
	v3 vPosition;
} g_Camera;


void ProgramInit()
{
	g_Camera.vDir = v3init(0,0,-1);
	g_Camera.vRight = v3init(1,0,0);
	g_Camera.vPosition = v3init(0,0,5);
}

int ProgramMain()
{

	if((g_KeyboardState.keys['q']|g_KeyboardState.keys[SDLK_ESCAPE]) & BUTTON_RELEASED)
		return 1;



	m mprj; 
	m mview;
	if(0)
	{
		v3 vdir = v3init(0,0,-1);
		v3 vright = v3init(1,0,0);
		static float f = 0;
		f += 3.8f;
		m mrotx = mrotatey(f * TORAD);
		v3 vdirx = mtransform(mrotx, vdir);
		v3 vrightx = mtransform(mrotx, vright);
		//m mrotz = mrotatez(45.f * TORAD);
		//v3 vdirzx = mtransform(mrotz, vdirx);
		//v3 vrightzx = mtransform(mrotz, vrightx);
		mview = mcreate(vdirx, vrightx, vdirx * -5.f);
		//mat = mcreate(vdir, vright, v3init(0,0,5));
		mprj = mperspective(45, (float)g_Width / (float)g_Height, 0.1f, 1000.f);
	}
	else
	{
		uplotfnxt("FPS %4.2f Dir[%5.2f,%5.2f,%5.2f] Pos[%3.2f,%3.2f,%3.2f]", 1.f, 
			g_Camera.vDir.x,
			g_Camera.vDir.y,
			g_Camera.vDir.z,
			g_Camera.vPosition.x,
			g_Camera.vPosition.y,
			g_Camera.vPosition.z);
		mview = mcreate(g_Camera.vDir, g_Camera.vRight, g_Camera.vPosition);
		mprj = mperspective(45, (float)g_Width / (float)g_Height, 0.1f, 1000.f);


	}


	glClearColor(0.3,0.4,0.6,0);
	glViewport(0, 0, g_Width, g_Height);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&mprj.x.x);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&mview.x.x);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CheckGLError();


	float x = 0.5f;
	glPointSize(5.f);
	glBegin(GL_POINTS);
	glColor3f(1,0,0);
	glVertex3f(x, x, 0.f);
	glVertex3f(x, -x, 0.f);
	glVertex3f(-x, -x, 0.f);
	glVertex3f(-x, x, 0.f);
	glEnd();
	CheckGLError();
	glPopMatrix();

	return 0;
}