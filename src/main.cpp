#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <string>
#include <thread>
#include <atomic>

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

#ifdef _WIN32
#include <windows.h>
void usleep(__int64 usec) 
{ 


	if(usec > 20000)
	{
		Sleep(usec/1000);
	}
	else if(usec >= 1000)
	{
		timeBeginPeriod(1);
		Sleep(usec/1000);
		timeEndPeriod(1);
	}
	else
	{
		__int64 time1 = 0, time2 = 0, freq = 0;
		QueryPerformanceCounter((LARGE_INTEGER *) &time1);
		QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

		do {
			QueryPerformanceCounter((LARGE_INTEGER *) &time2);
		} while((time2-time1)*1000000ll/freq < usec);
	}
}
#endif

SDL_Surface* g_Surface;
#ifdef _WIN32
#define START_WIDTH 1600
#define START_HEIGHT 1080
#else
#define START_WIDTH 800
#define START_HEIGHT 600
#endif
uint32_t g_BaseWidth = START_WIDTH;
uint32_t g_BaseHeight = START_HEIGHT;
uint32_t g_Width = g_BaseWidth;
uint32_t g_Height =  g_BaseHeight;
uint32_t g_nQuit = 0;
uint32_t g_lShowDebug = 1;
uint32_t g_lShowDebugText = 1;

MICROPROFILE_DECLARE(ThreadSafeMain);
MICROPROFILE_DECLARE(ThreadSafeInner0);
MICROPROFILE_DECLARE(ThreadSafeInner1);
MICROPROFILE_DECLARE(ThreadSafeInner2);
MICROPROFILE_DECLARE(ThreadSafeInner3);
MICROPROFILE_DECLARE(ThreadSafeInner4);

MICROPROFILE_DEFINE(ThreadSafeInner4,"ThreadSafe", "Inner4", 0xff00ff00);
MICROPROFILE_DEFINE(ThreadSafeInner3,"ThreadSafe", "Inner3", randcolor());
MICROPROFILE_DEFINE(ThreadSafeInner2,"ThreadSafe", "Inner2", randcolor());
MICROPROFILE_DEFINE(ThreadSafeInner1,"ThreadSafe", "Inner1", randcolor());
MICROPROFILE_DEFINE(ThreadSafeInner0,"ThreadSafe", "Inner0", randcolor());
MICROPROFILE_DEFINE(ThreadSafeMain,"ThreadSafe", "Main", randcolor());

//fake work
void WorkerThread(int threadId)
{
	uprintf("ENTERING WORKER\n");
	char name[100];
	snprintf(name, 99, "Worker%d", threadId);
	MicroProfileOnThreadCreate(&name[0]);
	uint32_t c0 = randcolor();
	uint32_t c1 = randcolor();
	uint32_t c2 = randcolor();
	uint32_t c3 = randcolor();
	uint32_t c4 = randcolor();

	while(!g_nQuit)
	{
		switch(threadId)
		{
		case 0:
		{
			usleep(100);
			{MICROPROFILE_SCOPEI("Thread0", "Work Thread0", c4); usleep(200);
			{MICROPROFILE_SCOPEI("Thread0", "Work Thread1", c3); usleep(200);
			{MICROPROFILE_SCOPEI("Thread0", "Work Thread2", c2); usleep(200);
			{MICROPROFILE_SCOPEI("Thread0", "Work Thread3", c1); usleep(200);
			}}}}
		}
		break;
		
		case 1:
			{
				usleep(100);
				MICROPROFILE_SCOPEI("Thread1", "Work Thread 1", c1);
				usleep(2000);
			}
			break;

		case 2:
			{
				usleep(1000);
				{MICROPROFILE_SCOPEI("Thread2", "Worker2", c0); usleep(200);
				{MICROPROFILE_SCOPEI("Thread2", "InnerWork0", c1); usleep(100);
				{MICROPROFILE_SCOPEI("Thread2", "InnerWork1", c2); usleep(100);
				{MICROPROFILE_SCOPEI("Thread2", "InnerWork2", c3); usleep(100);
				{MICROPROFILE_SCOPEI("Thread2", "InnerWork3", c4); usleep(100);
				}}}}}
			}
			break;
		case 3:
			{
				MICROPROFILE_SCOPEI("ThreadWork 3", "MAIN", c0);
				usleep(1000);;
				for(uint32_t i = 0; i < 10; ++i)
				{
					MICROPROFILE_SCOPEI("ThreadWork", "Inner0", c1);
					usleep(100);
					for(uint32_t j = 0; j < 4; ++j)
					{
						MICROPROFILE_SCOPEI("ThreadWork", "Inner1", c4);
						usleep(50);
						MICROPROFILE_SCOPEI("ThreadWork", "Inner2", c2);
						usleep(50);
						MICROPROFILE_SCOPEI("ThreadWork", "Inner3", c3);
						usleep(50);
						MICROPROFILE_SCOPEI("ThreadWork", "Inner4", c3);
						usleep(50);
					}
				}


			}
			break;
		default:
			
			MICROPROFILE_SCOPE(ThreadSafeMain);
			usleep(1000);;
			for(uint32_t i = 0; i < 5; ++i)
			{
				MICROPROFILE_SCOPE(ThreadSafeInner0);
				usleep(1000);
				for(uint32_t j = 0; j < 4; ++j)
				{
					MICROPROFILE_SCOPE(ThreadSafeInner1);
					usleep(500);
					MICROPROFILE_SCOPE(ThreadSafeInner2);
					usleep(150);
					MICROPROFILE_SCOPE(ThreadSafeInner3);
					usleep(150);
					MICROPROFILE_SCOPE(ThreadSafeInner4);
					usleep(150);
				}
			}
			break;
		}
	}
}

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

uint32_t g_MicroProfileMouseX = 0;
uint32_t g_MicroProfileMouseY = 0;
int g_MicroProfileMouseDelta = 0;

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
		g_MicroProfileMouseX = g_MouseState.position[0];
		g_MicroProfileMouseY = pEvt->motion.y;
		//MicroProfileMouseMove(g_MouseState.position[0], pEvt->motion.y);
		//{
		//	static int nPosX = -1;
		//	static int nPosY = -1;
		//	if(g_KeyboardState.keys[SDLK_LCTRL] & BUTTON_DOWN)
		//	{
		//		if(nPosX>=0 && nPosY>=0)
		//		{
		//			MicroProfileMoveGraph(0,pEvt->motion.x-nPosX, pEvt->motion.y-nPosY);
		//		}
		//		nPosX = pEvt->motion.x;
		//		nPosY = pEvt->motion.y;
		//	}
		//	else
		//	{
		//		nPosX = nPosY = -1;
		//	}
		//}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if(pEvt->type == SDL_MOUSEBUTTONUP)
		{
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
		if(pEvt->button.button == SDL_BUTTON_WHEELUP)
		{
			g_MicroProfileMouseDelta--;
		}
		else if(pEvt->button.button == SDL_BUTTON_WHEELDOWN)
		{
			g_MicroProfileMouseDelta++;
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

void MicroProfileQueryInitGL();

extern "C" 
int SDL_main(int argc, char* argv[])
{

	MicroProfileOnThreadCreate("Main");

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


	MicroProfileQueryInitGL();


	//Microprofile test
	std::thread t0(WorkerThread, 0);
	std::thread t1(WorkerThread, 1);
	std::thread t2(WorkerThread, 2);
	std::thread t3(WorkerThread, 3);


	std::thread t42(WorkerThread, 42);
	std::thread t43(WorkerThread, 43);
	std::thread t44(WorkerThread, 44);
	std::thread t45(WorkerThread, 45);


	InputInit();
	TextInit();
	MeshInit();
	ShaderInit();

	PhysicsInit();
	ProgramInit();
	

	MicroProfileToken MainTok = MicroProfileGetToken("MAIN", "Main", 0xff0000);

	
	while(!g_nQuit)
	{
		CheckGLError();
		PhysicsStep();
		InputClear();
		SDL_Event Evt;
		while(SDL_PollEvent(&Evt))
		{
			HandleEvent(&Evt);
		}
		{
			MICROPROFILE_SCOPEI("MAIN", "Program", 0x33ee55);
			if(ProgramMain())
			{
				g_nQuit = 1;
			}

		}

		{
			srand(0);
			MICROPROFILE_SCOPEI("MAIN", "DebugRender", 0x00eeee);
			{
				MICROPROFILE_SCOPEGPUI("GPU", "Render Debug", 0x88dd44);
				DebugDrawFlush();
			}
			{
				MICROPROFILE_SCOPEGPUI("GPU", "Render Text", 0x88dd44);
				TextFlush();
			}
			MICROPROFILE_SCOPEI("MAIN", "DUMMY", randcolor());

			for(int i = 0; i < 5; ++i)
			{
				MICROPROFILE_SCOPEI("MAIN", "DUM0", randcolor());
				usleep(100);				
				{MICROPROFILE_SCOPEI("MAIN", "DUM1", randcolor());
				usleep(100);
				{MICROPROFILE_SCOPEI("MAIN", "DUM2", randcolor());
				usleep(100);
				{MICROPROFILE_SCOPEI("MAIN", "DUM3", randcolor());
				usleep(200);
				{MICROPROFILE_SCOPEI("MAIN", "DUM4", randcolor());
				usleep(200);
				{MICROPROFILE_SCOPEI("MAIN", "DUM5", randcolor());
				usleep(200);
				}}}}}
	
			}

		}


		//MicroProfileMouseClick();pEvt->button.button == 1, pEvt->button.button == 3);
		MicroProfileMouseButton(g_MouseState.button[1] & BUTTON_DOWN ? 1 : 0, g_MouseState.button[3] & BUTTON_DOWN ? 1 : 0);
		MicroProfileMousePosition(g_MicroProfileMouseX, g_MicroProfileMouseY, g_MicroProfileMouseDelta);
		g_MicroProfileMouseDelta = 0;

		static uint64_t nMain = 0;
		MicroProfileLeave(MainTok, nMain);
		MicroProfileFlip();
		nMain = MicroProfileEnter(MainTok);
		{
			MICROPROFILE_SCOPEGPUI("GPU", "MicroProfileDraw", 0x88dd44);

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
			MicroProfileDraw(g_Width, g_Height);
		}
		



		if(g_KeyboardState.keys['z']&BUTTON_RELEASED)
		{
			MicroProfileToggleDisplayMode();
		}
		if(g_KeyboardState.keys['x']&BUTTON_RELEASED)
		{
			MicroProfileToggleTimers();
		}
		if(g_KeyboardState.keys['c']&BUTTON_RELEASED)
		{
			MicroProfileToggleAverageTimers();
		}
		if(g_KeyboardState.keys['v']&BUTTON_RELEASED)
		{
			MicroProfileToggleMaxTimers();
		}
		if(g_KeyboardState.keys['b']&BUTTON_RELEASED)
		{
			MicroProfileToggleCallCount();
		}
		if(g_KeyboardState.keys[']']&BUTTON_RELEASED && 0 == ((g_KeyboardState.keys[SDLK_RSHIFT]|g_KeyboardState.keys[SDLK_LSHIFT]) & BUTTON_DOWN))
		{
			MicroProfileNextGroup();
		}
		if(g_KeyboardState.keys['[']&BUTTON_RELEASED && 0 == ((g_KeyboardState.keys[SDLK_RSHIFT]|g_KeyboardState.keys[SDLK_LSHIFT]) & BUTTON_DOWN))
		{
			MicroProfilePrevGroup();
		}
		if(g_KeyboardState.keys[']']&BUTTON_RELEASED && 0 != ((g_KeyboardState.keys[SDLK_RSHIFT]|g_KeyboardState.keys[SDLK_LSHIFT]) & BUTTON_DOWN))
		{
			MicroProfileNextAggregatePreset();
		}
		if(g_KeyboardState.keys['[']&BUTTON_RELEASED && 0 != ((g_KeyboardState.keys[SDLK_RSHIFT]|g_KeyboardState.keys[SDLK_LSHIFT]) & BUTTON_DOWN))
		{
			MicroProfilePrevAggregatePreset();
		}
		if(g_KeyboardState.keys[SDLK_RSHIFT] & BUTTON_RELEASED)
		{
			MicroProfileToggleFlipDetailed();
		}

		MICROPROFILE_SCOPEI("MAIN", "Flip", 0xffee00);
		SDL_GL_SwapBuffers();
	}


	t0.join();
	t1.join();
	t2.join();
	t3.join();
	t42.join();
	t43.join();
	t44.join();
	t45.join();
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