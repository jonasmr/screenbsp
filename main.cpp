

#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <string>
#include "base.h"
#include "input.h"
#include "glinc.h"
#include "text.h"
#include "math.h"
#include "debug.h"
#include "mesh.h"
#include "shader.h"
#include "microprofile.h"
#include "physics.h"


SDL_Surface* g_Surface;
uint32_t g_BaseWidth =  800;
uint32_t g_BaseHeight =  600;
uint32_t g_Width = g_BaseWidth;
uint32_t g_Height =  g_BaseHeight;
uint32_t g_nQuit = 0;
uint32_t g_lShowDebug = 1;
uint32_t g_lShowDebugText = 1;

void CheckGLError()
{
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) 
	{
	    errString = gluErrorString(errCode);
		   uprintf ("UHOH: OpenGL Error: %s\n", errString);
		   ZBREAK();
	}
}


void HandleEvent(SDL_Event* pEvt)
{
	switch(pEvt->type)
	{
	case SDL_QUIT:
		g_nQuit = true;
		break;
	case SDL_KEYDOWN:
		g_KeyboardState.keys[(int)pEvt->key.keysym.sym] = BUTTON_DOWN|BUTTON_PUSHED;
		break;
	case SDL_KEYUP:
		g_KeyboardState.keys[(int)pEvt->key.keysym.sym] = BUTTON_UP|BUTTON_RELEASED;
		break;
	case SDL_MOUSEMOTION:
		g_MouseState.position[0] = pEvt->motion.x;
		g_MouseState.position[1] = g_Height-pEvt->motion.y; // flip to match opengl
		MicroProfileMouseMove(g_MouseState.position[0], pEvt->motion.y);
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if(pEvt->type == SDL_MOUSEBUTTONUP)
		{
			MicroProfileMouseClick(pEvt->button.button == 1, pEvt->button.button == 0);
		}
		if(pEvt->button.button < MOUSE_BUTTON_MAX)
		{
			int type = pEvt->type;
			if(SDL_MOUSEBUTTONUP == type)
			{
				g_MouseState.button[pEvt->button.button] = BUTTON_UP|BUTTON_RELEASED;
			}
			else
			{
				g_MouseState.button[pEvt->button.button] = BUTTON_DOWN|BUTTON_PUSHED;
			}
		}
	// 	}
	// 	break;
	// case SDL_JOYAXISMOTION:
	// 	{
	// 		int axis = pEvt->jaxis.axis;
	// 		int which = pEvt->jaxis.which;
	// 		int value = pEvt->jaxis.value;
	// 		//uprintf("motion %02d %03d %010d\n", which, axis, value);
	// 		//360 cont
	// 		switch(axis)
	// 		{
	// 		case 0: axis = AXIS_X0; break;
	// 		case 1: axis = AXIS_Y0; break;
	// 		case 3: axis = AXIS_Y1; break;
	// 		case 4: axis = AXIS_X1; break;
	// 		default: axis = AXIS_MAX; 
	// 		}
	// 		//remap axis
	// 		if(which < g_nNumJoysticks && axis < AXIS_MAX)
	// 		{
				
	// 			g_PadState[which].axis[axis] = value;
	// 			float fVal = (float)value / 0x7fff;
	// 			((float*)(&g_PadState[which].vaxis[0]))[axis] = fVal;
	// 			uprintf("%d %d :%f\n", which, axis, fVal);
	// 		}
	// 	}
	// 	break;
	// case SDL_JOYBUTTONDOWN:
	// case SDL_JOYBUTTONUP:
	// 	{
	// 		int which = pEvt->jbutton.which;
	// 		int button = pEvt->jbutton.button;
	// 		int type = pEvt->type;
	// 		uprintf("button %02d %03d %010d\n", which, button, type);
	// 		//remap button
	// 		if(which < g_nNumJoysticks && button < BUTTON_MAX)
	// 		{
	// 			if(type == SDL_JOYBUTTONUP)
	// 			{
	// 				g_PadState[which].button[button] = BUTTON_UP|BUTTON_RELEASED;
	// 			}
	// 			else
	// 			{
	// 				g_PadState[which].button[button] = BUTTON_DOWN|BUTTON_PUSHED;

	// 			}
	// 		}
	// 	}
	// 	break;
	}
}

int ProgramMain();
void ProgramInit();

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
#else
extern "C" 
int SDL_main(int argc, char** argv)
#endif
{

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) {
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,    	    8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,  	    8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,   	    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,  	    8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  	    24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,  	    8);	
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,		    32);	
	if((g_Surface = SDL_SetVideoMode(g_BaseWidth, g_BaseHeight, 32, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL  )) == NULL) 
	{
		return 1;
	}

	glewExperimental=1;
	GLenum err=glewInit();
	if(err!=GLEW_OK)
	{
		ZBREAK();
	}
	if(!GLEW_ARB_separate_shader_objects)
	{
		ZBREAK();
	}

	MicroProfileInit();
	InputInit();
	TextInit();
	MeshInit();
	ShaderInit();

	PhysicsInit();
	ProgramInit();
	

	
	while(!g_nQuit)
	{
		CheckGLError();
		//PhysicsStep();
		InputClear();
		SDL_Event Evt;
		while(SDL_PollEvent(&Evt))
		{
			HandleEvent(&Evt);
		}
		if(ProgramMain())
		{
			g_nQuit = 1;
		}




		DebugDrawFlush();
		TextFlush();


		MicroProfileFlip();
		{
			CheckGLError();
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			CheckGLError();
			glOrtho(0.0, g_Width, g_Height, 0, -1.0, 1.0);
			CheckGLError();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			CheckGLError();
		}
		MicroProfileDraw(g_Width, g_Height);



		if(g_KeyboardState.keys['z']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfileToggleDisplayMode\n");
			MicroProfileToggleDisplayMode();
		}
		if(g_KeyboardState.keys['x']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfileToggleTimers\n");
			MicroProfileToggleTimers();
		}
		if(g_KeyboardState.keys['c']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfileToggleAverageTimers\n");
			MicroProfileToggleAverageTimers();
		}
		if(g_KeyboardState.keys['v']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfileToggleMaxTimers\n");
			MicroProfileToggleMaxTimers();
		}
		if(g_KeyboardState.keys['b']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfileToggleCallCount\n");
			MicroProfileToggleCallCount();
		}
		if(g_KeyboardState.keys[']']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfileNextGroup\n");
			MicroProfileNextGroup();
		}
		if(g_KeyboardState.keys['[']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfilePrevGroup\n");
			MicroProfilePrevGroup();
		}
		if(g_KeyboardState.keys['}']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfileNextAggregateGroup\n");
			MicroProfileNextAggregatePreset();
		}
		if(g_KeyboardState.keys['{']&BUTTON_RELEASED)
		{
			uprintf("KEYDOWN MicroProfilePrevAggregatePreset\n");
			MicroProfilePrevAggregatePreset();
		}

		SDL_GL_SwapBuffers();
	}
	return 0;
}

#ifdef _WIN32
void uprintf(const char* fmt, ...)
{
	char buffer[32*1024];
	va_list args;
	va_start (args, fmt);
	vsprintf_s (buffer, fmt, args);
	OutputDebugString(&buffer[0]);
	va_end (args);
}
#else
void uprintf(const char* fmt, ...)
{
	char buffer[32*1024];
	va_list args;
	va_start (args, fmt);
	vsprintf(buffer, fmt, args);
	printf("%s", &buffer[0]);
	va_end (args);
}

#endif