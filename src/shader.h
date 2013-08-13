#pragma once
#include "base.h"
#include "math.h"

enum EShaderVS
{
	VS_DEFAULT,
	VS_MICROPROFILE,
	VS_LIGHTING,
	VS_DEBUG,
	VS_FOO,	
	VS_SIZE,
};
enum EShaderPS
{
	PS_FLAT_LIT,
	PS_MICROPROFILE,
	PS_LIGHTING,
	PS_DEBUG,
	PS_FOO,
	PS_SIZE,
};

void ShaderInit();
void ShaderUse(EShaderVS vs, EShaderPS ps);
void ShaderSetSize(v3 vSize);
int ShaderGetLocation(const char* pVar);
void ShaderSetUniform(int location, int value);
void ShaderSetUniform(int location, float value);
void ShaderSetUniform(int location, v2 v);
void ShaderSetUniform(int location, v3 v);
void ShaderSetUniform(int location, v4 v);
#define SHADER_SET(name, v)do{ int i = ShaderGetLocation(name); ShaderSetUniform(i, v);} while(0)
#define ShaderDisable() ShaderUse(VS_SIZE,PS_SIZE)

