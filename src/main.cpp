#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#ifdef main
#undef main
#endif

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
#include "stb_image.h"

#include "imgui/imgui.h"

#ifdef _WIN32
#undef near
#undef far
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

int g_UIEnabled = 1;
//SDL_Surface* g_Surface;
#ifdef _WIN32
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
	SMouseState& MouseState = g_UIEnabled ? g_MouseStateUI : g_MouseState;
	SKeyboardState& KeyboardState = g_KeyboardState;
	switch(pEvt->type)
	{
	case SDL_QUIT:
		g_nQuit = true;
		break;
	case SDL_KEYDOWN:
		KeyboardState.keys[(int)pEvt->key.keysym.scancode] = BUTTON_DOWN|BUTTON_PUSHED;
		break;
	case SDL_KEYUP:
		KeyboardState.keys[(int)pEvt->key.keysym.scancode] = BUTTON_UP|BUTTON_RELEASED;
		break;
	case SDL_MOUSEMOTION:
		g_MouseStateUI.position[0] = g_MouseState.position[0] = pEvt->motion.x;
		g_MouseStateUI.position[1] = g_MouseState.position[1] = g_Height-pEvt->motion.y; // flip to match opengl
		g_MicroProfileMouseX = MouseState.position[0];
		g_MicroProfileMouseY = pEvt->motion.y;
		//MicroProfileMouseMove(MouseState.position[0], pEvt->motion.y);
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
				MouseState.button[pEvt->button.button] = BUTTON_UP|BUTTON_RELEASED;
			}
			else
			{
				MouseState.button[pEvt->button.button] = BUTTON_DOWN|BUTTON_PUSHED;
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

GLuint g_ImgFontTex = 0;
GLuint g_ImguiProgram = 0;
GLuint g_ImguiVertexBuffer;
GLuint g_ImguiVAO;
GLuint g_ImguiProgramLocVertex = 1;
GLuint g_ImguiProgramLocColor = 2;
GLuint g_ImguiProgramLocTC0 = 3;
GLuint g_ImguiProgramLocMatrix = 0;
GLuint g_ImguiProgramLocTex = 0;

void ImguiInit();
void ImguiRender(float fDeltaTime);
void ImguiRenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count);

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

	ImguiInit();

	uprintf("GL VERSION '%s'\n", glGetString(GL_VERSION));

#if MICROPROFILE_ENABLED
	MicroProfileQueryInitGL();
	MicroProfileDrawInit();
#endif

	InputInit();
	TextInit();
	MeshInit();
	ShaderInit();

	PhysicsInit();
	ProgramInit();
	

	

	
	while(!g_nQuit)
	{
		ImGui::NewFrame();
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
		TextFlush();





		MicroProfileMouseButton(g_MouseState.button[1] & BUTTON_DOWN ? 1 : 0, g_MouseState.button[3] & BUTTON_DOWN ? 1 : 0);
		MicroProfileMousePosition(g_MicroProfileMouseX, g_MicroProfileMouseY, g_MicroProfileMouseDelta);
		g_MicroProfileMouseDelta = 0;
		CheckGLError();

		ImguiRender(1/60.f);
		MicroProfileFlip();		
		{
			MICROPROFILE_SCOPEGPUI("GPU", "MicroProfileDraw", 0x88dd44);
			float projection[16];
			float left = 0.f;
			float right = g_Width;
			float bottom = g_Height;
			float top = 0.f;
			float near = -1.f;
			float far = 1.f;
			memset(&projection[0], 0, sizeof(projection));

			projection[0] = 2.0f / (right - left);
			projection[5] = 2.0f / (top - bottom);
			projection[10] = -2.0f / (far - near);
			projection[12] = - (right + left) / (right - left);
			projection[13] = - (top + bottom) / (top - bottom);
			projection[14] = - (far + near) / (far - near);
			projection[15] = 1.f; 
 
 			glEnable(GL_BLEND);
 			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			MicroProfileBeginDraw(g_Width, g_Height, &projection[0]);
			MicroProfileDraw(g_Width, g_Height);
			MicroProfileEndDraw();
			glDisable(GL_BLEND);
		}


		if(g_KeyboardState.keys[SDL_SCANCODE_Z]&BUTTON_RELEASED)
		{
			MicroProfileToggleDisplayMode();
		}
		if(g_KeyboardState.keys[SDL_SCANCODE_RSHIFT] & BUTTON_RELEASED)
		{
			MicroProfileTogglePause();
		}

		MICROPROFILE_SCOPEI("MAIN", "Flip", 0xffee00);

		SDL_GL_SwapWindow(pWindow); 


	}

  	SDL_GL_DeleteContext(glcontext);  
 	SDL_DestroyWindow(pWindow);
 	SDL_Quit();

	return 0;
}



void ImguiInit()
{

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(g_Width, g_Height);
    io.DeltaTime = 1.0f/60.0f;
    io.PixelCenterOffset = 0.0f;
    io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    // io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    // io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    // io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    // io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    // io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    // io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    // io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    // io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    // io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    // io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    // io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    // io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    // io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    // io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    // io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    // io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.RenderDrawListsFn = ImguiRenderDrawLists;
    io.SetClipboardTextFn = 0;
    io.GetClipboardTextFn = 0;
// #ifdef _MSC_VER
// 	io.ImeSetInputScreenPosFn = ImImpl_ImeSetInputScreenPosFn;
// #endif
    // Load font texture
    glGenTextures(1, &g_ImgFontTex);
    glBindTexture(GL_TEXTURE_2D, g_ImgFontTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Default font (embedded in code)
    const void* png_data;
    unsigned int png_size;
    ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
    int tex_x, tex_y, tex_comp;
    void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
	IM_ASSERT(tex_data != NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
	stbi_image_free(tex_data);


	

	const char* g_PixelShaderCode = "\
#version 150 \n \
uniform sampler2D tex; \
in vec2 TC0; \
in vec4 Color; \
out vec4 Out0; \
 \
void main(void)   \
{   \
	Out0 = texture(tex, TC0.xy) * Color; \
} \
";

	const char* g_VertexShaderCode = " \
#version 150 \n \
uniform mat4 Matrix; \
in vec3 VertexIn; \
in vec4 ColorIn; \
in vec2 TC0In; \
out vec2 TC0; \
out vec4 Color; \
 \
void main(void)   \
{ \
	Color = ColorIn; \
	TC0 = TC0In; \
	gl_Position = Matrix * vec4(VertexIn, 1.0); \
} \
";

	GLuint hFragment = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	GLuint hVertex = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glShaderSource(hFragment, 1, (const char**)&g_PixelShaderCode, 0);
	glCompileShader(hFragment);
	CheckGLError();
	glShaderSource(hVertex, 1, (const char**)&g_VertexShaderCode, 0);
	glCompileShader(hVertex);
	CheckGLError();
	g_ImguiProgram = glCreateProgramObjectARB();
	glAttachObjectARB(g_ImguiProgram, hFragment);
	glAttachObjectARB(g_ImguiProgram, hVertex);

	g_ImguiProgramLocVertex = 1;
	g_ImguiProgramLocColor = 2;
	g_ImguiProgramLocTC0 = 3;
	glBindAttribLocation(g_ImguiProgram, g_ImguiProgramLocVertex, "VertexIn");
	glBindAttribLocation(g_ImguiProgram, g_ImguiProgramLocColor, "ColorIn");
	glBindAttribLocation(g_ImguiProgram, g_ImguiProgramLocTC0, "TC0In");

	glLinkProgramARB(g_ImguiProgram);
	CheckGLError();
	g_ImguiProgramLocMatrix = glGetUniformLocation(g_ImguiProgram, "Matrix");
	g_ImguiProgramLocTex = glGetUniformLocation(g_ImguiProgram, "tex");

	glGenBuffers(1, &g_ImguiVertexBuffer);
	glGenVertexArrays(1, &g_ImguiVAO);


}


void ImguiRenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
	if (cmd_lists_count == 0)
        return;
    CheckGLError();

#if 1
    glBindVertexArray(g_ImguiVAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_ImguiVertexBuffer);

    glUseProgramObjectARB(g_ImguiProgram);

    CheckGLError();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    CheckGLError();
    glEnable(GL_SCISSOR_TEST);
    CheckGLError();

   	glEnableVertexAttribArray(g_ImguiProgramLocVertex);
	glEnableVertexAttribArray(g_ImguiProgramLocColor);
	glEnableVertexAttribArray(g_ImguiProgramLocTC0);
 
	CheckGLError();
	glUniform1i(g_ImguiProgramLocTex, 0);
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_ImgFontTex);

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    {
		float projection[16];
		float left = 0.f;
		float right = g_Width;
		float bottom = g_Height;
		float top = 0.f;
		float near = -1.f;
		float far = 1.f;
		memset(&projection[0], 0, sizeof(projection));

		projection[0] = 2.0f / (right - left);
		projection[5] = 2.0f / (top - bottom);
		projection[10] = -2.0f / (far - near);
		projection[12] = - (right + left) / (right - left);
		projection[13] = - (top + bottom) / (top - bottom);
		projection[14] = - (far + near) / (far - near);
		projection[15] = 1.f; 
		CheckGLError();
		glUniformMatrix4fv(g_ImguiProgramLocMatrix, 1, 0, projection);
 	}
CheckGLError();
	glBindVertexArray(g_ImguiVAO);

    // Render command lists
    for (int n = 0; n < cmd_lists_count; n++)
    {CheckGLError();
        const ImDrawList* cmd_list = cmd_lists[n];
        const unsigned char* vtx_buffer = (const unsigned char*)cmd_list->vtx_buffer.begin();
        const unsigned char* vtx_buffer_end = (const unsigned char*)cmd_list->vtx_buffer.end();
		glBindBuffer(GL_ARRAY_BUFFER, g_ImguiVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, vtx_buffer_end - vtx_buffer, vtx_buffer, GL_STREAM_DRAW);
		int nStride = sizeof(ImDrawVert);

		glVertexAttribPointer(g_ImguiProgramLocVertex, 2, GL_FLOAT, 0, nStride, 0);
		glVertexAttribPointer(g_ImguiProgramLocColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, nStride, (void*)16);
		glVertexAttribPointer(g_ImguiProgramLocTC0, 2, GL_FLOAT, 0, nStride, (void*)8);
		// glEnableVertexAttribArray(g_ImguiProgramLocVertex);
		// glEnableVertexAttribArray(g_ImguiProgramLocColor);
		// glEnableVertexAttribArray(g_ImguiProgramLocTC0);
CheckGLError();

        // glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer));
        // glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer+8));
        // glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer+16));

        int vtx_offset = 0;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
            glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            vtx_offset += pcmd->vtx_count;
        }

CheckGLError();

    }
	glDisableVertexAttribArray(g_ImguiProgramLocVertex);
	glDisableVertexAttribArray(g_ImguiProgramLocColor);
	glDisableVertexAttribArray(g_ImguiProgramLocTC0);

    glDisable(GL_SCISSOR_TEST);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgramObjectARB(0);
	glBindVertexArray(0);

    CheckGLError();


#else
    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // Setup texture
    glBindTexture(GL_TEXTURE_2D, g_ImgFontTex);
    glEnable(GL_TEXTURE_2D);

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Render command lists
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        const unsigned char* vtx_buffer = (const unsigned char*)cmd_list->vtx_buffer.begin();
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer+8));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer+16));

        int vtx_offset = 0;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
            glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
            glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            vtx_offset += pcmd->vtx_count;
        }
    }
    glDisable(GL_SCISSOR_TEST);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    CheckGLError();
#endif
}

void ImguiRender(float fDeltaTime)
{
	ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();

    io.DeltaTime = fDeltaTime;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    float mouse_x = g_MouseState.position[0], mouse_y = g_Height - g_MouseState.position[1];
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
    io.MouseDown[0] = g_MouseStateUI.button[1] & BUTTON_DOWN ? 1 : 0;
    io.MouseDown[1] = g_MouseStateUI.button[3] & BUTTON_DOWN ? 1 : 0;
    // uprintf("imgui mouse %d %d pos  %f %f\n", io.MouseDown[0],io.MouseDown[1], mouse_x, mouse_y);

    // Start the frame
    //ImGui::NewFrame();


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
