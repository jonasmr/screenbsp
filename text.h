#pragma once
#include "base.h"
#define UPLOTF_START 6
#define MAX_FONT_CHARS 256

#define TEXT_CHAR_WIDTH 5
#define TEXT_CHAR_HEIGHT 8


struct SFontDescription
{
	uint16_t nCharOffsets[MAX_FONT_CHARS];
	uint32_t nTextureId;
};
extern SFontDescription g_FontDescription;
void TextInit();
void TextFlush();

void uplotfnxt(const char* pfmt, ...);
void uplotf(uint32_t nX, uint32_t nY, const char* pfmt, ...);