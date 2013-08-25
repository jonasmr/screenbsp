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

//SDL_Surface* g_Surface;
#ifdef _WIN32123
#define START_WIDTH 1280
#define START_HEIGHT 1024
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

MICROPROFILE_DEFINE(MAIN, "MAIN", "Main", 0xff0000);


#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <libkern/OSAtomic.h>
#define TICK() mach_absolute_time()
inline int64_t TicksPerSecond()
{
	static int64_t nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		mach_timebase_info_data_t sTimebaseInfo;	
		mach_timebase_info(&sTimebaseInfo);
		nTicksPerSecond = 1000000000ll * sTimebaseInfo.denom / sTimebaseInfo.numer;
	}
	return nTicksPerSecond;
}
#elif defined(_WIN32)
#include <windows.h>
inline int64_t TicksPerSecond()
{
	static int64_t nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&nTicksPerSecond);
	}
	return nTicksPerSecond;
}
inline int64_t GetTick()
{
	int64_t ticks;
	QueryPerformanceCounter((LARGE_INTEGER*)&ticks);
	return ticks;
}

#define TICK() GetTick()
#endif



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
		g_KeyboardState.keys[(int)pEvt->key.keysym.scancode] = BUTTON_DOWN|BUTTON_PUSHED;
		break;
	case SDL_KEYUP:
		g_KeyboardState.keys[(int)pEvt->key.keysym.scancode] = BUTTON_UP|BUTTON_RELEASED;
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
	case SDL_MOUSEWHEEL:
		{
			g_MicroProfileMouseDelta += pEvt->wheel.y;

		}
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
		#if 0 
		if(pEvt->button.button == SDL_BUTTON_WHEELUP)
		{
			g_MicroProfileMouseDelta--;
		}
		else if(pEvt->button.button == SDL_BUTTON_WHEELDOWN)
		{
			g_MicroProfileMouseDelta++;
		}

		#endif
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
void MicroProfileDrawInit();
void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight, float* prj);
void MicroProfileEndDraw();

//extern "C" 
int main(int argc, char* argv[])
{

	MicroProfileOnThreadCreate("Main");

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		return 1;
	}
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,    	    8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,  	    8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,   	    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,  	    8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  	    24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,  	    8);	
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,		    32);	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,	    1);	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetSwapInterval(1);

	SDL_Window * pWindow = SDL_CreateWindow("ScreenBsp", 10, 10, g_BaseWidth, g_BaseHeight, SDL_WINDOW_OPENGL);
	if(!pWindow)
		return 1;

	SDL_GLContext glcontext = SDL_GL_CreateContext(pWindow);
	glewExperimental=1;
	GLenum err=glewInit();
	glGetError();
	CheckGLError();
	if(err!=GLEW_OK)
	{
		ZBREAK();
	}
	if(!GLEW_ARB_separate_shader_objects)
	{
		ZBREAK();
	}
	CheckGLError();



	uprintf("GL VERSION '%s'\n", glGetString(GL_VERSION));

#if MICROPROFILE_ENABLED
	CheckGLError();
	MicroProfileQueryInitGL();
	CheckGLError();
	MicroProfileDrawInit();
	CheckGLError();
#endif


	//Microprofile test
	std::thread t0(WorkerThread, 0);
	std::thread t1(WorkerThread, 1);
	std::thread t2(WorkerThread, 2);
	std::thread t3(WorkerThread, 3);


	std::thread t42(WorkerThread, 42);
	//std::thread t43(WorkerThread, 43);
	//std::thread t44(WorkerThread, 44);
	//std::thread t45(WorkerThread, 45);

	CheckGLError();
	InputInit();
	CheckGLError();
	TextInit();
	CheckGLError();
	MeshInit();
	CheckGLError();
	ShaderInit();

	PhysicsInit();
	ProgramInit();
	

	

	
	while(!g_nQuit)
	{
		int64_t nStart = TICK();
		MICROPROFILE_SCOPE(MAIN);
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
		CheckGLError();
		{
			srand(0);
			MICROPROFILE_SCOPEI("MAIN", "DebugRender", 0x00eeee);
			{
				MICROPROFILE_SCOPEGPUI("GPU", "Render Debug", 0x88dd44);
//				DebugDrawFlush();
				CheckGLError();
			}


			{
				MICROPROFILE_SCOPEGPUI("GPU", "Render Text", 0x88dd44);
				TextFlush();
				CheckGLError();
			}
			if(0)
			{
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
		}


		//MicroProfileMouseClick();pEvt->button.button == 1, pEvt->button.button == 3);
		MicroProfileMouseButton(g_MouseState.button[1] & BUTTON_DOWN ? 1 : 0, g_MouseState.button[3] & BUTTON_DOWN ? 1 : 0);
		MicroProfileMousePosition(g_MicroProfileMouseX, g_MicroProfileMouseY, g_MicroProfileMouseDelta);
		g_MicroProfileMouseDelta = 0;
		CheckGLError();
		MicroProfileFlip();
		{
			MICROPROFILE_SCOPEGPUI("GPU", "MicroProfileDraw", 0x88dd44);

			// CheckGLError();
			// glMatrixMode(GL_PROJECTION);
			// glLoadIdentity();
			// CheckGLError();
			// glOrtho(0.0, g_Width, g_Height, 0, -1.0, 1.0);
			// CheckGLError();
			// glMatrixMode(GL_MODELVIEW);
			// glLoadIdentity();
			m prj = morthogl(0.0, g_Width, g_Height, 0, -1.0, 1.0);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			CheckGLError();

			CheckGLError();
			MicroProfileBeginDraw(g_Width, g_Height, (float*)&prj);
			MicroProfileDraw(g_Width, g_Height);
			CheckGLError();
			MicroProfileEndDraw();
			CheckGLError();
		}
		



		if(g_KeyboardState.keys[SDL_SCANCODE_Z]&BUTTON_RELEASED)
		{
			MicroProfileToggleDisplayMode();
		}
		if(g_KeyboardState.keys[SDL_SCANCODE_RSHIFT] & BUTTON_RELEASED)
		{
			MicroProfileTogglePause();
		}



		int64_t nEnd= TICK();
		static int64_t nTotal = 0;
		static int64_t nCount = 0;
		nTotal += nEnd-nStart;
		nCount++;

		int64_t nUsec = 1000000 * (nEnd-nStart) / TicksPerSecond();
		int64_t nUsecAvg = 1000000 * (nTotal/nCount) / TicksPerSecond();
		//if(0 == nCount %128)
		{
			uplotfnxt("ms %7.4fms ... AVG %7.4f :: %d\n", nUsec / 1000.f, nUsecAvg / 1000.f, nCount);
		}


		MICROPROFILE_SCOPEI("MAIN", "Flip", 0xffee00);
		//SDL_GL_SwapBuffers();


		SDL_GL_SwapWindow(pWindow); 


	}


	t0.join();
	t1.join();
	t2.join();
	t3.join();
	t42.join();
	//t43.join();
	//t44.join();
	//t45.join();



  	SDL_GL_DeleteContext(glcontext);  
 	SDL_DestroyWindow(pWindow);
 	SDL_Quit();

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