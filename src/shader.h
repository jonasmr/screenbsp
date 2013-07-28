#pragma once
#include "base.h"
#include "math.h"

enum EShaderVS
{
	VS_DEFAULT,
	VS_FOO,
	VS_MICROPROFILE,
	VS_SIZE,
};
enum EShaderPS
{
	PS_FLAT_LIT,
	PS_MICROPROFILE,
	PS_FOO,
	PS_SIZE,
};

void ShaderInit();
void ShaderUse(EShaderVS vs, EShaderPS ps);
void ShaderSetSize(v3 vSize);
int ShaderGetLocation(EShaderPS Shader, const char* pVar);
void ShaderSetUniform(int location, int value);
#define ShaderDisable() ShaderUse(VS_SIZE,PS_SIZE)

