#include "shader.h"

#include <stdio.h>
#include <stdlib.h>
#include "glinc.h"

struct
{
	GLuint VS[VS_SIZE];
	GLuint PS[PS_SIZE];
	GLuint LinkedPrograms[VS_SIZE * PS_SIZE];

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

	DumpGlLog(handle);

	glCompileShader(handle);
	CheckGLError();


	CheckGLError();

	free(pBuffer);
	ZASSERT(handle);	

	return handle;
}

void ShaderInit()
{
	memset(&g_ShaderState, 0, sizeof(g_ShaderState));
	g_ShaderState.PS[PS_FLAT_LIT] = CreateProgram(GL_FRAGMENT_SHADER_ARB, "flat.ps");
	g_ShaderState.VS[VS_DEFAULT] = CreateProgram(GL_VERTEX_SHADER_ARB, "default.vs");
}
void ShaderUse(EShaderVS vs, EShaderPS ps)
{
	if(vs == VS_SIZE || ps == PS_SIZE)
	{
		glUseProgramObjectARB(0);
		return;
	}
	uint32 nIndex = ps * VS_SIZE + vs;
	if(!g_ShaderState.LinkedPrograms[nIndex])
	{
		GLuint prg = glCreateProgramObjectARB();
		CheckGLError();
		glAttachObjectARB(prg, g_ShaderState.PS[ps]);
		glAttachObjectARB(prg, g_ShaderState.VS[vs]);
		CheckGLError();
		glLinkProgramARB(prg);
		g_ShaderState.LinkedPrograms[nIndex] = prg;
		DumpGlLog(prg);
		DumpGlLog(g_ShaderState.PS[ps]);
		DumpGlLog(g_ShaderState.VS[vs]);
	}
	//use
	CheckGLError();
	glUseProgramObjectARB(g_ShaderState.LinkedPrograms[nIndex]);
	CheckGLError();

}
void ShaderSetSize(v3 vSize)
{
	GLuint loc = glGetUniformLocation(g_ShaderState.LinkedPrograms[0], "Size");
	glUniform3fv(loc, 1, &vSize.x);

}