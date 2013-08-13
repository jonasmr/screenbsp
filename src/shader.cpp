#include "shader.h"

#include <stdio.h>
#include <stdlib.h>
#include "glinc.h"

struct
{
	GLuint VS[VS_SIZE];
	GLuint PS[PS_SIZE];
	GLuint LinkedPrograms[VS_SIZE * PS_SIZE];
	int nCurrentIndex;

} g_ShaderState;


char* ReadTextFile(const char *file_) 
{

    FILE * pFile;
    long lSize;
    
    pFile = fopen ( file_, "r" );
    if (pFile==NULL) return 0;

    // obtain file size.
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile) + 1;
    rewind (pFile);
    
    // allocate memory to contain the whole file.
    char* buffer = (char*) malloc (lSize);
    if (buffer == NULL) return 0;
    
    // copy the file into the buffer.
    fread (buffer,1,lSize,pFile);
    buffer[lSize - 1] = 0;
    
    //printf("%s\n", buffer);
    fclose(pFile);

    return buffer;
}


static void DumpGlLog(GLuint handle)
{
	int nLogLen = 0;
	glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &nLogLen);
	if(nLogLen > 0)
	{
		char* pChars = (char*)malloc(nLogLen);
		int nLen = 0;
		glGetInfoLogARB(handle, nLogLen, &nLen, pChars);

		uprintf("COMPILE MESSAGE\n%s\n\n", pChars);

		free(pChars);
		ZBREAK();
	}


}

GLuint CreateProgram(int nType, const char* pFile)
{
	uprintf("COMPILING %s as %s\n", pFile, nType == GL_FRAGMENT_SHADER_ARB ? "FRAGMENT SHADER" : "VERTEX SHADER");
	char* pBuffer = ReadTextFile(pFile);
	GLuint handle = glCreateShaderObjectARB(nType);
	CheckGLError();


	glShaderSource(handle, 1, (const char**)&pBuffer, 0);
	CheckGLError();

	

	glCompileShader(handle);
	CheckGLError();

	DumpGlLog(handle);


	CheckGLError();
	//static void DumpGlLog(GLuint handle)

	free(pBuffer);
	ZASSERT(handle);	

	return handle;
}

void ShaderInit()
{
	memset(&g_ShaderState, 0, sizeof(g_ShaderState));
	g_ShaderState.PS[PS_FLAT_LIT] = CreateProgram(GL_FRAGMENT_SHADER_ARB, "flat.ps");
	g_ShaderState.VS[VS_DEFAULT] = CreateProgram(GL_VERTEX_SHADER_ARB, "default.vs");

	g_ShaderState.PS[PS_MICROPROFILE] = CreateProgram(GL_FRAGMENT_SHADER_ARB, "microprofile.ps");
	g_ShaderState.VS[VS_MICROPROFILE] = CreateProgram(GL_VERTEX_SHADER_ARB, "microprofile.vs");

	g_ShaderState.PS[PS_LIGHTING] = CreateProgram(GL_FRAGMENT_SHADER_ARB, "lighting.ps");
	g_ShaderState.VS[VS_LIGHTING] = CreateProgram(GL_VERTEX_SHADER_ARB, "lighting.vs");

	g_ShaderState.PS[PS_DEBUG] = CreateProgram(GL_FRAGMENT_SHADER_ARB, "debug.ps");
	g_ShaderState.VS[VS_DEBUG] = CreateProgram(GL_VERTEX_SHADER_ARB, "debug.vs");
}
void ShaderUse(EShaderVS vs, EShaderPS ps)
{
	if(vs == VS_SIZE || ps == PS_SIZE)
	{
		g_ShaderState.nCurrentIndex = -1;
		glUseProgramObjectARB(0);
		//uprintf("USING NULL\n");
		return;
	}
	CheckGLError();

	uint32 nIndex = ps * VS_SIZE + vs;
	if(!g_ShaderState.LinkedPrograms[nIndex])
	{
		uprintf("LINKING %d\n", nIndex);
		GLuint prg = glCreateProgramObjectARB();
		CheckGLError();
		glCompileShader(g_ShaderState.PS[ps]);
		CheckGLError();
		glCompileShader(g_ShaderState.VS[vs]);

		glAttachObjectARB(prg, g_ShaderState.PS[ps]);
		glAttachObjectARB(prg, g_ShaderState.VS[vs]);
		glLinkProgramARB(prg);
		g_ShaderState.LinkedPrograms[nIndex] = prg;
		
		CheckGLError();
		DumpGlLog(prg);
		DumpGlLog(g_ShaderState.PS[ps]);
		DumpGlLog(g_ShaderState.VS[vs]);
	}
	//use
	CheckGLError();
	glUseProgramObjectARB(g_ShaderState.LinkedPrograms[nIndex]);
	g_ShaderState.nCurrentIndex = nIndex;
//	uprintf("USING %d.. prg %d\n", nIndex, g_ShaderState.LinkedPrograms[nIndex]);
	CheckGLError();

}

void ShaderSetUniform(int loc, v2 v)
{
	glUniform2fv(loc, 1, &v.x);
}



void ShaderSetUniform(int loc, v3 v)
{
	glUniform3fv(loc, 1, &v.x);
}

void ShaderSetUniform(int loc, v4 v)
{
	glUniform4fv(loc, 1, &v.x);
}

int ShaderGetLocation(const char* pVar)
{
	int r = glGetUniformLocation(g_ShaderState.LinkedPrograms[g_ShaderState.nCurrentIndex], pVar);
	CheckGLError();
//	ZASSERT(r>=0);
	return r;
}
void ShaderSetUniform(int location, int value)
{
	glUniform1i(location, value);
	CheckGLError();
}

void ShaderSetUniform(int location, float value)
{
	glUniform1f(location, value);
	CheckGLError();
}
