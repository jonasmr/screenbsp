#include "input.h"

SPadState g_PadState[MAX_PADS];
SDL_Joystick* g_pJoyStick[MAX_PADS];
SMouseState g_MouseState;
SKeyboardState g_KeyboardState;
uint32_t g_nNumJoysticks = 0;


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