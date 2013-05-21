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
	uprintf("COMPILING %s as %s", pFile, nType == GL_FRAGMENT_SHADER_ARB ? "FRAGMENT SHADER" : "VERTEX SHADER");
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

// GLuint CreateProgram(const char *vertFile, const char *fragFile)
// {
//     GLhandleARB v,f,p;
//     FILE *vf,*ff;
//     char *vs,*fs;

//     v = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
//     f = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);   

//     vf = ReadTextFile(vertFile, vs);
//     ff = ReadTextFile(fragFile, fs);

//     glShaderSourceARB(v, 1, (const GLcharARB**) &vs, NULL);
//     glShaderSourceARB(f, 1, (const GLcharARB**) &fs, NULL);

//     FreeTextFileAndBuf(vf, vs);
//     FreeTextFileAndBuf(ff, fs);

//     glCompileShaderARB(v);
//     uprintf("GLSL: compiling vertex %s with fragment %s combo\n", vertFile, fragFile);
//   	PrintGLLog(v, "VERT: ");
//     glCompileShaderARB(f);
//   	PrintGLLog(f, "FRAG: ");

//     // create a program object to link the shaders together
//     p = glCreateProgramObjectARB();

//     // attach shader objects and link together
//     glAttachObjectARB(p,v);
//     glAttachObjectARB(p,f);
//     glLinkProgramARB(p);

//   	//PrintGLLog(p, "Link: ");

//     glUseProgramObjectARB(p);

// 	return p; 
// }


void ShaderInit()
{
	memset(&g_ShaderState, 0, sizeof(g_ShaderState));
	g_ShaderState.PS[PS_FLAT_LIT] = CreateProgram(GL_FRAGMENT_SHADER_ARB, "flat.ps");
	g_ShaderState.VS[VS_DEFAULT] = CreateProgram(GL_VERTEX_SHADER_ARB, "basic.vs");
	ShaderUse(VS_DEFAULT, PS_FLAT_LIT);
	ShaderDisable();
}
void ShaderUse(EShaderVS vs, EShaderPS ps)
{
	if(vs == VS_SIZE || ps == PS_SIZE)
	{
		glUseProgramObjectARB(0);
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
	}
	//use
	CheckGLError();
	glUseProgramObjectARB(g_ShaderState.LinkedPrograms[nIndex]);
	CheckGLError();
	glUseProgramObjectARB(0);

}
