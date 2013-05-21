#pragma once
#include "base.h"

enum EShaderVS
{
	VS_DEFAULT,
	VS_FOO,
	VS_SIZE,
};
enum EShaderPS
{
	PS_FLAT_LIT,
	PS_FOO,
	PS_SIZE,
};

void ShaderInit();
void ShaderUse(EShaderVS vs, EShaderPS ps);
#define ShaderDisable() ShaderUse(VS_SIZE,PS_SIZE)

