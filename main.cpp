

#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <string>
#include "base.h"
#include "input.h"
#include "glinc.h"
#include "text.h"


SDL_Surface* g_Surface;
uint32_t g_BaseWidth =  1280;
uint32_t g_BaseHeight =  720;
uint32_t g_Width = g_BaseWidth;
uint32_t g_Height =  g_BaseHeight;
uint32_t g_nQuit = 0;

SPadState g_PadState[MAX_PADS];
SDL_Joystick* g_pJoyStick[MAX_PADS];
SMouseState g_MouseState;
SKeyboardState g_KeyboardState;
uint32_t g_nNumJoysticks = 0;

uint32_t g_lShowDebug = 1;
uint32_t g_lShowDebugText = 1;
void InputInit();
void InputClear();

void CheckGLError()
{
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) 
	{
	    errString = gluErrorString(errCode);
		   uprintf ("UHOH: OpenGL Error: %s\n", errString);
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
		g_MouseState.position[1] = pEvt->motion.y;
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
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
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  	    0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,  	    0);	
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,		    32);	
	if((g_Surface = SDL_SetVideoMode(g_BaseWidth, g_BaseHeight, 32, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL  )) == NULL) 
	{
		return 1;
	}

	InputInit();
	TextInit();

	while(!g_nQuit)
	{
		CheckGLError();

		InputClear();
		SDL_Event Evt;
		while(SDL_PollEvent(&Evt))
		{
			HandleEvent(&Evt);
		}
		if(g_KeyboardState.keys['q'] & BUTTON_RELEASED)
			g_nQuit = 1;


		glClearColor(0.2,0.2,0.2,0);
		glViewport(0, 0, g_Width, g_Height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, g_Width, g_Height, 0, 1, -1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		uplotfnxt("FPS %f", 1000.f);
		uplotfnxt("FPS %f", 1000.f);
		uplotfnxt("FPS %f", 1000.f);


		TextFlush();
		SDL_GL_SwapBuffers();
	}
	return 0;
}


void InputInit()
{
	g_nNumJoysticks = SDL_NumJoysticks();
	if(g_nNumJoysticks > AXIS_MAX)
		g_nNumJoysticks = AXIS_MAX;
	memset(g_pJoyStick, 0, sizeof(g_pJoyStick));
	uprintf("opening %d joysticks\n", g_nNumJoysticks);
	for(uint32_t i = 0; i < g_nNumJoysticks; ++i)
	{
		g_pJoyStick[i] = SDL_JoystickOpen(i);
	}
	memset(&g_PadState, 0, sizeof(g_PadState));
	memset(&g_MouseState, 0, sizeof(g_MouseState));
	memset(&g_KeyboardState, 0, sizeof(g_MouseState));
}

void InputClose()
{
	for(uint32_t i = 0; i < g_nNumJoysticks; ++i)
	{
		SDL_JoystickClose(g_pJoyStick[i]);
	}
}

void InputClear()
{
	for(uint32_t i = 0; i < g_nNumJoysticks; ++i)
		for(uint32_t j = 0; j < BUTTON_MAX; ++j)
			g_PadState[i].button[j] &= ~BUTTON_FRAME;

	for(uint32_t i = 0; i < MOUSE_BUTTON_MAX; ++i)
		g_MouseState.button[i] &= ~BUTTON_FRAME;
	for(uint32_t i = 0; i < KEYBOARD_MAX_KEYS; ++i)
		g_KeyboardState.keys[i] &= ~BUTTON_FRAME;
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