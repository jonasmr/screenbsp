#pragma once

enum
{
	AXIS_X0 = 0,
	AXIS_Y0,
	AXIS_X1,
	AXIS_Y1,
	AXIS_MAX,
};
#define MAX_PADS 4
enum
{
	BUTTON_A,
	BUTTON_B,
	BUTTON_X,
	BUTTON_Y,
	BUTTON_RT,
	BUTTON_RB,
	BUTTON_LT,
	BUTTON_LB,
	BUTTON_MAX,
};
enum 
{
	BUTTON_DOWN		= 0x01,
	BUTTON_UP		= 0x02,
	BUTTON_RELEASED	= 0x04,
	BUTTON_PUSHED	= 0x08,
	BUTTON_FRAME = BUTTON_PUSHED|BUTTON_RELEASED
};
struct SPadAxis
{
	float x, y;
};
struct SPadState
{
	int16_t axis[AXIS_MAX];
	uint8_t button[BUTTON_MAX];
	SPadAxis vaxis[2];
};

enum
{
	MOUSE_AXIS_X,
	MOUSE_AXIS_Y,
	MOUSE_AXIS_MAX
};

enum
{
	MOUSE_BUTTON_MAX = 6,
};

struct SMouseState
{
	int32_t position[MOUSE_AXIS_MAX];
	uint8_t button[MOUSE_BUTTON_MAX];
};


enum
{
	KEYBOARD_MAX_KEYS = SDLK_LAST,
};

struct SKeyboardState
{
	uint8_t keys[KEYBOARD_MAX_KEYS];
};

extern uint32_t g_nNumJoysticks;
extern SPadState g_PadState[MAX_PADS];
extern SMouseState g_MouseState;
extern SKeyboardState g_KeyboardState;

extern bool g_bRetrieveString;
extern const char* g_pRetrieveStringResult;

inline const char* GetStringInput()
{
	return g_pRetrieveStringResult;
}
inline bool StringInputActive()
{
	return g_bRetrieveString;
}

inline void StartStringInput(bool bEnabled)
{
	g_bRetrieveString = bEnabled;
}
