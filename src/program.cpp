#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <string>
#include "base.h"
#include "input.h"
#include "glinc.h"
#include "text.h"
#include "math.h"
#include "program.h"
#include "bsp.h"
#include "debug.h"
#include "manipulator.h"
#include "mesh.h"
#include "shader.h"
#include "physics.h"
#include "microprofile.h"

uint32_t g_nDump = 0;
extern uint32_t g_Width;
extern uint32_t g_Height;
//render todo
//tile calc
//debug render fixing + color


void WorldDrawObjects(bool* bCulled);


uint32 g_nUseOrtho = 0;
float g_fOrthoScale = 10;
SOccluderBsp* g_Bsp = 0;
uint32_t g_nBspNodeCap = 512;
uint32 g_nUseDebugCameraPos = 2;
v3 vLockedCamPos = v3init(0,0,0);
v3 vLockedCamRight = v3init(0,-1,0);
v3 vLockedCamDir = v3init(1,0,0);


#define TILE_SIZE 8
#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

#define SHADOWMAP_SIZE 2048

#define MAX_LIGHT_INDEX (32<<10)
#define LIGHT_INDEX_SIZE 1024
//#define TILE_HEIGHT 8
#define MAX_NUM_LIGHTS 1024
GLuint g_LightTexture;
GLuint g_LightTileTexture;
GLuint g_LightIndexTexture;
uint32_t g_LightTileWidth;
uint32_t g_LightTileHeight;

struct LightDesc
{
	float Pos[4];
	float Color[4];
};

struct LightTileIndex
{
	int16_t nBase;
	int16_t nCount;
};
struct LightIndex
{
	int16_t nIndex;
	int16_t nDummy;
};

LightIndex g_LightIndex[MAX_LIGHT_INDEX];
LightTileIndex g_LightTileInfo[(MAX_HEIGHT/TILE_SIZE)*(MAX_WIDTH/TILE_SIZE)];
LightDesc g_LightBuffer[MAX_NUM_LIGHTS];

SWorldState g_WorldState;
SEditorState g_EditorState;

void DebugRender(m mprj)
{
	if(g_EditorState.pSelected)
	{
		ZDEBUG_DRAWBOUNDS(g_EditorState.pSelected->mObjectToWorld, g_EditorState.pSelected->vSize, g_EditorState.bLockSelection ? (0xffffff00) : -1);
		uplotfnxt("SELECTION %p", g_EditorState.pSelected);
	}

	DebugDrawFlush(mprj);
}

void EditorStateInit()
{
	memset(&g_EditorState, 0, sizeof(g_EditorState));
	g_EditorState.Manipulators[0] = new ManipulatorTranslate();
	g_EditorState.Manipulators[1] = new ManipulatorRotate();
}

struct ShadowMap
{
	GLuint FrameBufferId;
	GLuint TextureId;
	m mprjview;
	m mprjviewtex;
};

ShadowMap g_SM;

ShadowMap AllocateShadowMap()
{
	CheckGLError();
	ShadowMap r;
	glGenTextures(1, &r.TextureId);
	glBindTexture(GL_TEXTURE_2D, r.TextureId);
	CheckGLError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	CheckGLError();

	CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	CheckGLError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	
	CheckGLError();
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &r.FrameBufferId);

	glBindFramebuffer(GL_FRAMEBUFFER, r.FrameBufferId);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	CheckGLError();

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r.TextureId, 0);



	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		uprintf("STATUS IS %d\n", status);


		ZBREAK();
	}


	CheckGLError();
	return r;
}


void RenderShadowMap(ShadowMap& SM)
{
	MICROPROFILE_SCOPEGPUI("GPU", "Shadowmap", 0xff0099);
	v3 vDirection = v3normalize(v3init(0.7071067811, -0.7071067811, 0.3));
	v3 vRight = v3normalize(v3cross(v3init(0,1,0), vDirection));
	v3 vUp = v3normalize(v3cross(vRight, vDirection));
	m mview = mcreate(vDirection, vRight, v3zero());
	m mprj = morthogl(-500, 500, -500, 500, -500.f, 500.f);
	m moffset = mid();
	moffset.x = v4init(0.5, 0.0, 0.0, 0.0f);
	moffset.y = v4init(0.0, 0.5, 0.0, 0.0f);
	moffset.z = v4init(0.0, 0.0, 0.5, 0.0f);

	moffset.trans.x = 0.5f;
	moffset.trans.y = 0.5f;
	moffset.trans.z = 0.5f;
	SM.mprjview = mmult(mprj, mview);
	SM.mprjviewtex = mmult(moffset, mmult(mprj, mview));


	CheckGLError();
	uplotfnxt("render shadowmap");
	glBindFramebuffer(GL_FRAMEBUFFER, SM.FrameBufferId);
	glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	glColorMask(0,0,0,0);
	glClear(GL_DEPTH_BUFFER_BIT);


	ShaderUse(VS_SHADOWMAP, PS_SHADOWMAP);
	SHADER_SET("ProjectionMatrix", SM.mprjview);

	WorldDrawObjects(nullptr);

	CheckGLError();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glColorMask(1,1,1,1);
	glDepthMask(1);
	glViewport(0, 0, g_Width, g_Height);
	CheckGLError();
}

#define OCCLUSION_TEST_HALF_SIZE 300
#define OCCLUSION_TEST_MIN (-OCCLUSION_TEST_HALF_SIZE)
#define OCCLUSION_TEST_MAX OCCLUSION_TEST_HALF_SIZE
#define OCCLUSION_NUM_LARGE 10
#define OCCLUSION_NUM_SMALL 100
#define OCCLUSION_NUM_LONG 20
#define OCCLUSION_NUM_OBJECTS 500
#define OCCLUSION_USE_GROUND 1
#define OCCLUSION_GROUND_Y 0.f

void WorldOcclusionCreate(v3 vSize, uint32 nFlags, v3 vColor, v3 vPos)
{
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].mObjectToWorld = mid();
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].mObjectToWorld.trans = v4init(vPos, 1.f);
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].vSize = vSize; 
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].nFlags = nFlags;
	g_WorldState.WorldObjects[g_WorldState.nNumWorldObjects].nColor = vColor.tocolor();
	g_WorldState.nNumWorldObjects++;
}


void WorldOcclusionCreate(v3 vSize, uint32 nFlags, v3 vColor = v3zero())
{
	v3 vPos = v3zero();
	vPos.x = frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
	vPos.z = frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
	vPos.y = 1.0f * vSize.y;
	WorldOcclusionCreate(vSize, nFlags, vColor, vPos);

}
void WorldInitOcclusionTest()
{
	g_WorldState.nNumWorldObjects = 0;
	float fbar = 0.f;
	int idxx_large[] = 
	{
		-1,
		//0, 1, 2, 3, 4, 
		//5, 6, 7, 
		//8, 
		//9, 10, 
		//76 + 15,
	};

	bool bSkipInit = false;

	//void WorldOcclusionCreate(v3 vSize, uint32 nFlags, v3 vColor, v3 vPos)
	if(OCCLUSION_USE_GROUND)
	{
		WorldOcclusionCreate(v3init(OCCLUSION_TEST_HALF_SIZE*2, 1.0f, OCCLUSION_TEST_HALF_SIZE*2),
			SObject::OCCLUDER_BOX,
			v3init(1,1,1),
			v3init(0,OCCLUSION_GROUND_Y, 0));
	}


	for(int i = 0; i < OCCLUSION_NUM_LARGE; ++i)
	{
		float fHeight = frandrange(50, 100);
		float fWidth = frandrange(7, 15);
		float fDepth = frandrange(7, 15);
		bool bSkip = bSkipInit;
		for(int x : idxx_large)
			if(x == i)
				bSkip = false;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX);
		}
	}
	
	int idxx[] = 
	{
		-1,
		// 75 + 15 , 
		// 76 + 15 ,
	};
	//for(int i = 75; i < 100; ++i)
	for(int i = 0; i < OCCLUSION_NUM_SMALL; ++i)
	{
		float fHeight = frandrange(10, 15);
		float fWidth = frandrange(7, 15);
		float fDepth = frandrange(7, 15);
		bool bSkip = bSkipInit;
		for(int x : idxx)
			if(x == i)
				bSkip = false;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX);
		}
	}
//	uprintf("fbar %f", fbar);

	int idxx_long[] = 
	{
		//0, 1, 2, 3, 4, 
		//5, 6, 
		7, 
		//8, 
		//9, 10, 
		//76 + 15,
	};

	int idxx_long2[] = 
	{
		//0, 1, 2, 3, 4, 
		5, 
		//6, 
		//7, 8, 
		//9, 10, 
		//76 + 15,
	};

	for(int i = 0; i < OCCLUSION_NUM_LONG; ++i)
	{
		float fHeight = frandrange(10, 20);
		float fWidth = frandrange(25, 50);
		float fDepth = frandrange(7, 12);
		if(frand() < 0.5f)
		{
			Swap(fWidth, fHeight);
		}

		bool bSkip = bSkipInit;
		for(int x : idxx_long)
			if(x == i)
				bSkip = false;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX);
		}
	}
	for(int i = 0; i < OCCLUSION_NUM_LONG; ++i)
	{
		float fHeight = frandrange(10, 20);
		float fWidth = frandrange(25, 50);
		float fDepth = frandrange(7, 12);
		if(frand() < 0.5f)
		{
			Swap(fWidth, fDepth);
		}
		bool bSkip = bSkipInit;
		for(int x : idxx_long2)
			if(x == i)
				bSkip = false;
		if(bSkip)
		{
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
			fbar += frandrange(OCCLUSION_TEST_MIN, OCCLUSION_TEST_MAX);
		}
		else
		{
			WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUDER_BOX);
		}
	}

	for(int i = 0; i < OCCLUSION_NUM_OBJECTS; ++i)
	{
		float fHeight = frandrange(1, 2);
		float fWidth = frandrange(0.5f, 0.7f);
		float fDepth = frandrange(0.5, 0.7f);
		if(frand() < 0.5f)
		{
			Swap(fWidth, fHeight);
		}

		WorldOcclusionCreate(v3init(fWidth, fHeight, fDepth), SObject::OCCLUSION_TEST, v3init(1,0,0));
	}

}

void WorldInit()
{
	g_WorldState.Occluders[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[0].mObjectToWorld.trans = v4init(3.f,0.f,0.1f, 1.f);
	g_WorldState.Occluders[0].vSize = v3init(0.5f, 0.5f, 0.f);

	g_WorldState.Occluders[1].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(2.5f,0.f,-0.5f, 1.f);
	//g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(1.5f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[1].vSize = v3init(0.75f, 0.75f, 0);

	g_WorldState.Occluders[2].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[2].mObjectToWorld.trans = v4init(2.6f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[2].vSize = v3init(0.25f, 0.25f, 0);
	g_WorldState.nNumOccluders = 0;


	if(1)
	{
		g_WorldState.nNumWorldObjects = 0;
		WorldInitOcclusionTest();
	}
	else
	{
		g_WorldState.WorldObjects[0].mObjectToWorld = mrotatey(90*TORAD);
		g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f,0.f,-0.5f, 1.f);
		g_WorldState.WorldObjects[0].vSize = v3init(0.25f, 0.2f, 0.2f); 
		g_WorldState.WorldObjects[0].nFlags = 0;
		g_WorldState.WorldObjects[0].nFlags |= SObject::OCCLUDER_BOX; 

		g_WorldState.WorldObjects[1].mObjectToWorld = mrotatey(90*TORAD);
		g_WorldState.WorldObjects[1].mObjectToWorld.trans = v4init(3.f,0.f,-1.5f, 1.f);
		g_WorldState.WorldObjects[1].vSize = v3init(0.25f, 0.2f, 0.2f)* 0.5f; 
		g_WorldState.WorldObjects[1].nFlags = 0; 
		g_WorldState.WorldObjects[1].nFlags |= SObject::OCCLUDER_BOX; 
		g_WorldState.nNumWorldObjects = 2;
		g_WorldState.nNumWorldObjects = 0;
	}
	if(0)
	{
		int idx = 0;
		#define GRID_SIZE 10
		for(uint32 i = 0; i < GRID_SIZE; i++)
		{
		for(uint32 j = 0; j < GRID_SIZE; j++)
		{
		for(uint32 k = 0; k < GRID_SIZE; k++)
		{
			v3 vPos = v3init(i,j,k) / (GRID_SIZE-1.f);
			vPos.z += j * 0.01f;
			vPos -= 0.5f;
			vPos *= 8.f;
			vPos += 0.03f;
			vPos.x += 2.0f;
			g_WorldState.WorldObjects[idx].mObjectToWorld = mid();
			g_WorldState.WorldObjects[idx].mObjectToWorld.trans = v4init(vPos,1.f);
			g_WorldState.WorldObjects[idx].vSize = v3init(0.25f, 0.2f, 0.2); 

			idx++;
		}}}
		g_WorldState.nNumWorldObjects = idx;
		for(int i = 0; i < g_WorldState.nNumWorldObjects; ++i)
		{
			PhysicsAddObjectBox(&g_WorldState.WorldObjects[i]);
		}
	}


#if 0
	g_WorldState.nNumLights = 1;
	for(int i = 0; i < g_WorldState.nNumLights; ++i)
	{
		m mat = mid();
		mat.trans.y = -3.f;
		// mat.trans.x = frandrange(-5.f, 5.f);
		// mat.trans.y = frandrange(-2.7f, -3.f);
		// mat.trans.z = frandrange(-5.f, 5.f);
//		uprintf("LIGHT AT %f %f %f\n", mat.trans.x, mat.trans.y, mat.trans.z);
		g_WorldState.Lights[i].mObjectToWorld = mat;
		g_WorldState.Lights[i].nColor = randcolor() | 0xff000000;
		g_WorldState.Lights[i].fRadius = 2.f;
	}
#else
	g_WorldState.nNumLights = 15;
	for(int i = 0; i < g_WorldState.nNumLights; ++i)
	{
		ZDEBUG_DRAWSPHERE(g_WorldState.Lights[i].mObjectToWorld.trans.tov3(), 2.f, -1);
		m mat = mid();
		mat.trans.x = frandrange(-5.f, 5.f);
		mat.trans.y = frandrange(-2.7f, -3.f);
		mat.trans.z = frandrange(-5.f, 5.f);
//		uprintf("LIGHT AT %f %f %f\n", mat.trans.x, mat.trans.y, mat.trans.z);
		g_WorldState.Lights[i].mObjectToWorld = mat;
		g_WorldState.Lights[i].nColor = randcolor() | 0xff000000;
		g_WorldState.Lights[i].fRadius = 2.f;
	}
#endif


	g_EditorState.pSelected = 0;


	CheckGLError();
	glGenTextures(1, &g_LightTexture);
	glGenTextures(1, &g_LightTileTexture);
	glGenTextures(1, &g_LightIndexTexture);
	memset(&g_LightIndex, 0, sizeof(g_LightIndex));
	memset(&g_LightBuffer, 0, sizeof(g_LightBuffer));
	glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, MAX_NUM_LIGHTS*2, 1, 0, GL_RGBA, GL_FLOAT, &g_LightBuffer);


	glBindTexture(GL_TEXTURE_2D, g_LightTileTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    g_LightTileWidth = g_Width / TILE_SIZE;
    g_LightTileHeight = g_Height / TILE_SIZE;
    ZASSERT(g_LightTileWidth * TILE_SIZE == g_Width);
    ZASSERT(g_LightTileHeight * TILE_SIZE == g_Height);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, g_LightTileWidth, g_LightTileHeight, 0, GL_RGBA, GL_SHORT, 0);
    CheckGLError();
    

	glBindTexture(GL_TEXTURE_2D, g_LightIndexTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, LIGHT_INDEX_SIZE, LIGHT_INDEX_SIZE, 0, GL_RGBA, GL_SHORT, 0);
    CheckGLError();

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_LightTileWidth, g_LightTileHeight, 0, GL_RGBA, GL_SHORT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGLError();

}



void WorldDrawObjects(bool* bCulled)
{
	int nLocSize = ShaderGetLocation("Size");
	int nLocModelView = ShaderGetLocation("ModelViewMatrix");
	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		if(bCulled)
		{
			if(bCulled[i])
			{
				ZDEBUG_DRAWBOX(g_WorldState.WorldObjects[i].mObjectToWorld, g_WorldState.WorldObjects[i].mObjectToWorld.trans.tov3(), g_WorldState.WorldObjects[i].vSize, 0xffff0000, 0);
			}
			else
			{
				// SHADER_SET("Size", g_WorldState.WorldObjects[i].vSize);
				// SHADER_SET("ModelViewMatrix", g_WorldState.WorldObjects[i].mObjectToWorld);
				ShaderSetUniform(nLocSize, g_WorldState.WorldObjects[i].vSize);
				ShaderSetUniform(nLocModelView, g_WorldState.WorldObjects[i].mObjectToWorld);
				MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
			}
		}
		else
		{
			ShaderSetUniform(nLocSize, g_WorldState.WorldObjects[i].vSize);
			ShaderSetUniform(nLocModelView, g_WorldState.WorldObjects[i].mObjectToWorld);
			MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
		}
	}
}


int g_nSimulate = 0;
void WorldRender()
{
	MICROPROFILE_SCOPEI("MAIN", "WorldRender", 0xff44dddd);
	if(g_KeyboardState.keys[SDL_SCANCODE_F1] & BUTTON_RELEASED)
	{
		g_nSimulate = !g_nSimulate;
	}
	static int incfoo = 1;
	if(g_nSimulate)
	{
		g_WorldState.WorldObjects[0].mObjectToWorld = mmult(g_WorldState.WorldObjects[0].mObjectToWorld, mrotatey(0.5f*TORAD));
		g_WorldState.Occluders[0].mObjectToWorld = mmult(g_WorldState.Occluders[0].mObjectToWorld, mrotatez(0.3f*TORAD));
		static float xx = 0;
		if(incfoo)
			xx += 0.005f;
		g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f, 0.f, sin(xx)*2, 1.f);
	}

	uint32 nNumOccluders = g_WorldState.nNumOccluders;
	static float foo = 0;
	
	if(g_KeyboardState.keys[SDL_SCANCODE_Y] & BUTTON_RELEASED)
	{
		incfoo = !incfoo;
	}



	if(incfoo)
		foo += 0.01f;
	//foo = 10.2201;
	uplotfnxt("foo is %f", foo);
	//foo = 1.0f;

	bool bShift = 0 != ((g_KeyboardState.keys[SDL_SCANCODE_RSHIFT]|g_KeyboardState.keys[SDL_SCANCODE_LSHIFT]) & BUTTON_DOWN);
	if(g_KeyboardState.keys[SDL_SCANCODE_L] & BUTTON_RELEASED)
	{
		if(bShift)
		{
			g_WorldState.Camera.vPosition = vLockedCamPos;
			g_WorldState.Camera.vDir = vLockedCamDir;
			g_WorldState.Camera.vRight = vLockedCamRight;
			uprintf("g_WorldState.Camera.vPosition = v3init(%f,%f,%f);\n", vLockedCamPos.x, vLockedCamPos.y, vLockedCamPos.z);
			uprintf("g_WorldState.Camera.vDir = v3init(%f,%f,%f);\n", vLockedCamDir.x, vLockedCamDir.y, vLockedCamDir.z);
			uprintf("g_WorldState.Camera.vRight = v3init(%f,%f,%f)\n;", vLockedCamRight.x, vLockedCamRight.y, vLockedCamRight.z);
		}
		else
			g_WorldState.Camera.vPosition = v3init(0,sin(foo), 0);;
	}


	v3 vPos = vLockedCamPos;
	v3 vDir = vLockedCamDir;
	v3 vRight = vLockedCamRight;
	switch(g_nUseDebugCameraPos)
	{
		case 2:
			vPos = v3init(0,sin(foo), 0);
			vDir = v3normalize(v3init(3,0,0) - vPos);
			vRight = v3init(0,-1,0);
			{
				v3 vUp = v3normalize(v3cross(vRight, vDir));
				vRight = v3normalize(v3cross(vDir, vUp));
			}
			break;
		case 0:
			vPos = g_WorldState.Camera.vPosition;
			vDir = g_WorldState.Camera.vDir;
			vRight = g_WorldState.Camera.vRight;
			vLockedCamPos = vPos;
			vLockedCamDir = vDir;
			vLockedCamRight = vRight;
			break;
		case 1:
			//locked.
			break;
	}
	if(g_KeyboardState.keys[SDL_SCANCODE_Q]&BUTTON_RELEASED)
	{
		g_nBspNodeCap <<= 1;
		if(g_nBspNodeCap > 1024)
			g_nBspNodeCap = 2;
	}

		
	
	ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.02f), -1,1);
	ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.04f), -1,1);
	ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.06f), -1,1);
	SOccluderBspViewDesc ViewDesc;
	ViewDesc.vOrigin = vPos;
	ViewDesc.vDirection = vDir;
	ViewDesc.vRight = vRight;
	ViewDesc.fFovY = g_WorldState.Camera.fFovY;
	ViewDesc.fAspect = (float)g_Height / (float)g_Width;
	ViewDesc.fZNear = g_WorldState.Camera.fNear;
	ViewDesc.nNodeCap = g_nBspNodeCap;
	uplotfnxt("DEBUG POS %f %f %f", vPos.x, vPos.y, vPos.z);

	BspBuild(g_Bsp, &g_WorldState.Occluders[0], 
		nNumOccluders,
		&g_WorldState.WorldObjects[0], 
		g_WorldState.nNumWorldObjects, 
		//0,//2,
		ViewDesc);

	uint32 nNumObjects = g_WorldState.nNumWorldObjects;
	bool* bCulled = (bool*)alloca(nNumObjects);
	bool* bCulledRef = (bool*)alloca(nNumObjects);
	int* nCulledRef = (int*)alloca(nNumObjects*4);
	bCulled[0] = false;
	memset(bCulled, 0, nNumObjects);
	memset(bCulledRef, 0, nNumObjects);
	memset(nCulledRef, 0, nNumObjects*4);
	for(uint32 i = 0; i < nNumObjects; ++i)
	{
		bCulled[i] = BspCullObject(g_Bsp, &g_WorldState.WorldObjects[i]);
	}
	SOccluderBspStats Stats;
	BspGetStats(g_Bsp, &Stats);
	uplotfnxt("********************BSP STATS********************");
	uplotfnxt("** TESTED %5d VISIBLE %5d", Stats.nNumObjectsTested, Stats.nNumObjectsTestedVisible);

	static v3 LightPos = v3init(0,-2.7,0);
	static v3 LightColor = v3init(0.9, 0.5, 0.9);
	static v3 LightPos0 = v3init(0,-2.7,0);
	static v3 LightColor0 = v3init(0.3, 1.0, 0.3);

	LightPos.z = (sin(foo*10)) * 5;
	LightPos0.x = (sin(foo*10)) * 5;

#if 1
	g_WorldState.Lights[0].mObjectToWorld.trans.z = (sin(foo*5.5)) * 5;
	g_WorldState.Lights[1].mObjectToWorld.trans.x = (cos((foo+2)*3)) * 5;
#endif

	#define BUFFER_SIZE (512*512)
	static v3 PointBuffer[BUFFER_SIZE];
	static v3 DumpPlaneNormal0, DumpPlaneNormal1, DumpPlaneNormal2, DumpPlaneNormal3;
	static v3 DumpPlanePos0, DumpPlanePos1, DumpPlanePos2, DumpPlanePos3;
	static int nNumPoints = 0;
	bool bDumpPoints = false;
	if(g_KeyboardState.keys[SDL_SCANCODE_B]&BUTTON_RELEASED)
	{
		bDumpPoints = true;
		nNumPoints = 0;
	}

	m mprj = (g_WorldState.Camera.mprj);
	m mprjinv = minverse(g_WorldState.Camera.mprj);



	int nLightIndex = 0;

	m mcliptoworld = mmult(g_WorldState.Camera.mviewinv, g_WorldState.Camera.mprjinv);
	v3 veye = g_WorldState.Camera.vPosition;
	int nMaxTileLights = 0;
	if(0)
	for(int i = 0; i < g_LightTileWidth; ++i)
	{
			MICROPROFILE_SCOPEI("MAIN", "Tile Stuff", 0xff44dddd);
			//g_LightTileWidth

		float x0 = 2*(float)i / (g_LightTileWidth)-1;
		float x1 = 2*(float)(i+1) / (g_LightTileWidth)-1;
		for(int j = 0; j < g_LightTileHeight; ++j)
		{
			float y0 = 2*(float)j / (g_LightTileHeight)-1;
			float y1 = 2*(float)(j+1) / (g_LightTileHeight)-1;
			v4 p0 = mtransform(mcliptoworld, v4init(x0,y0,-1.f,1));
			v4 p1 = mtransform(mcliptoworld, v4init(x1,y0,-1.f,1));
			v4 p2 = mtransform(mcliptoworld, v4init(x1,y1,-1.f,1));
			v4 p3 = mtransform(mcliptoworld, v4init(x0,y1,-1.f,1));
			v3 _p0 = p0.tov3() / p0.w;
			v3 _p1 = p1.tov3() / p1.w;
			v3 _p2 = p2.tov3() / p2.w;
			v3 _p3 = p3.tov3() / p3.w;

			LightTileIndex& Index = g_LightTileInfo[i+j*g_LightTileWidth];
			Index.nBase = nLightIndex;
			Index.nCount = 0;
			v3 vNormal0 = -v3normalize(v3cross(_p0-_p1, _p1 - veye));
			v3 vNormal1 = -v3normalize(v3cross(_p1-_p2, _p2 - veye));
			v3 vNormal2 = -v3normalize(v3cross(_p2-_p3, _p3 - veye));
			v3 vNormal3 = -v3normalize(v3cross(_p3-_p0, _p0 - veye));
			v4 plane0 = v4makeplane(_p0, vNormal0);
			v4 plane1 = v4makeplane(_p1, vNormal1);
			v4 plane2 = v4makeplane(_p2, vNormal2);
			v4 plane3 = v4makeplane(_p3, vNormal3);

			for(int k = 0; k < g_WorldState.nNumLights && nLightIndex < MAX_LIGHT_INDEX; ++k)
			{
				v4 center = g_WorldState.Lights[k].mObjectToWorld.trans;
				float radius = g_WorldState.Lights[k].fRadius;
				center.w = 1.f;
				//ZASSERT(center.w == 1.f);
				float fDot0 = v4dot(plane0, center);
				float fDot1 = v4dot(plane1, center);
				float fDot2 = v4dot(plane2, center);
				float fDot3 = v4dot(plane3, center);

				
				if( fDot0 + radius >= 0 &&
					fDot1 + radius >= 0 &&
					fDot2 + radius >= 0 &&
					fDot3 + radius >= 0)
				{
				//	ZBREAK();
					int idx = nLightIndex++;

					g_LightIndex[idx].nIndex = k;
					g_LightIndex[idx].nDummy = k;
					Index.nCount++;
					///ZASSERT(Index.nCount == (nLightIndex - Index.nBase));
					//ZASSERT(nLightIndex < MAX_LIGHT_INDEX);
				}
			}
			// if(Index.nCount)
			// 	uprintf("INDEX AT %d, %d has %d\n", i, j, Index.nCount);
			nMaxTileLights = Max((int)Index.nCount, nMaxTileLights);
			//Index.nCount = j % 1;
//			Index.nBase = i % 10;

			if(bDumpPoints)
			{
//				uprintf("DUMP %f %f %f\n", p0.x, p0.y, p0.z);
				if(i == 50 && j == 50)
				{

					DumpPlaneNormal0 = vNormal0;
					DumpPlanePos0 = _p0;
					DumpPlaneNormal1 = vNormal1;
					DumpPlanePos1 = _p1;
					DumpPlaneNormal2 = vNormal2;
					DumpPlanePos2 = _p2;
					DumpPlaneNormal3 = vNormal3;
					DumpPlanePos3 = _p3;
					//ZBREAK();

				}
				PointBuffer[nNumPoints++] = p0.tov3();
				PointBuffer[nNumPoints++] = p1.tov3();
				PointBuffer[nNumPoints++] = p2.tov3();
				PointBuffer[nNumPoints++] = p3.tov3();
				ZASSERT(nNumPoints < BUFFER_SIZE);
			}
		}
	}
	if(nNumPoints)
	{
		ZASSERT(nNumPoints == 4 * g_LightTileWidth * g_LightTileHeight);
		for(int i = 0; i < nNumPoints;i+=4)
		{
			ZDEBUG_DRAWLINE(PointBuffer[i], PointBuffer[i+1], -1, 0);
			ZDEBUG_DRAWLINE(PointBuffer[i+3], PointBuffer[i], -1, 0);
		}

		ZDEBUG_DRAWPLANE(DumpPlaneNormal0, DumpPlanePos0, -1);
		ZDEBUG_DRAWPLANE(DumpPlaneNormal1, DumpPlanePos1, -1);
		ZDEBUG_DRAWPLANE(DumpPlaneNormal2, DumpPlanePos2, -1);
		ZDEBUG_DRAWPLANE(DumpPlaneNormal3, DumpPlanePos3, -1);
	}
	for(int i = 0; i < g_WorldState.nNumLights; ++ i)
	{
		g_LightBuffer[i].Pos[0] = g_WorldState.Lights[i].mObjectToWorld.trans.x;
		g_LightBuffer[i].Pos[1] = g_WorldState.Lights[i].mObjectToWorld.trans.y;
		g_LightBuffer[i].Pos[2] = g_WorldState.Lights[i].mObjectToWorld.trans.z;
		v3 col = v3fromcolor(g_WorldState.Lights[i].nColor);
		g_LightBuffer[i].Color[0] = col.x;
		g_LightBuffer[i].Color[1] = col.y;
		g_LightBuffer[i].Color[2] = col.z;
		g_LightBuffer[i].Color[3] = g_WorldState.Lights[i].fRadius;
	}



	RenderShadowMap(g_SM);









	int nNumLights = g_WorldState.nNumLights;
	{
		MICROPROFILE_SCOPEI("MAIN", "UpdateTexture", 0xff44dd44);
		CheckGLError();
		glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nNumLights*2, 1, GL_RGBA, GL_FLOAT, &g_LightBuffer);
	    CheckGLError();

	    glBindTexture(GL_TEXTURE_2D, g_LightTileTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_LightTileWidth, g_LightTileHeight, GL_RG, GL_SHORT, &g_LightTileInfo);
	    CheckGLError();

	    glBindTexture(GL_TEXTURE_2D, g_LightIndexTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, LIGHT_INDEX_SIZE, MAX_LIGHT_INDEX/LIGHT_INDEX_SIZE, GL_RG, GL_SHORT, &g_LightIndex);
	    CheckGLError();

	    glBindTexture(GL_TEXTURE_2D, 0);
	    CheckGLError();

	}




	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, g_LightTileTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, g_LightIndexTexture);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, g_SM.TextureId);
	glActiveTexture(GL_TEXTURE0);
	static int g_Mode = 0;
	uplotfnxt("mode is %d", g_Mode);
	if(g_KeyboardState.keys[SDL_SCANCODE_T] & BUTTON_RELEASED)
	{
		g_Mode = (g_Mode+1)%3;
	}



	ShaderUse(VS_LIGHTING, PS_LIGHTING);
	SHADER_SET("texLight", 1);
	SHADER_SET("texLightTile", 2);
	SHADER_SET("texLightIndex", 3);
	SHADER_SET("texDepth", 4);
	SHADER_SET("NumLights", nNumLights);
	//g_WorldState.nNumLights 
	SHADER_SET("lightDelta", 1.f / (2*MAX_NUM_LIGHTS));
	SHADER_SET("MaxTileLights", nMaxTileLights);
	SHADER_SET("ScreenSize", v2init(g_Width, g_Height));
	v3 vTileSize = v3init(g_LightTileWidth, g_LightTileHeight, TILE_SIZE);
	SHADER_SET("TileSize", vTileSize);
	SHADER_SET("MaxLightTileIndex", nLightIndex);
	SHADER_SET("vWorldEye", g_WorldState.Camera.vPosition);
	m mprjview = mmult(g_WorldState.Camera.mprj, g_WorldState.Camera.mview);
	SHADER_SET("ProjectionMatrix", mprjview);
	SHADER_SET("ShadowMatrix", g_SM.mprjviewtex);
	SHADER_SET("mode", g_Mode);
	#define MAX_FAIL 32
	static SWorldObject* pFailObjects[MAX_FAIL];
	static int nNumFail = 0;
	if(g_nUseDebugCameraPos == 0)
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		int nLocSize = ShaderGetLocation("Size");
		int nLocModelView = ShaderGetLocation("ModelViewMatrix");
		for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
		{
			SWorldObject* pObject = g_WorldState.WorldObjects + i;
			if(pObject->nFlags & SObject::OCCLUDER_BOX)
			{
				v3 vSizeScaled = pObject->vSize;
				vSizeScaled *= BSP_BOX_SCALE;
				ShaderSetUniform(nLocSize, vSizeScaled);
				ShaderSetUniform(nLocModelView, pObject->mObjectToWorld);
				MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
			}
		}
		glDepthMask(0);
		enum{MAX_QUERIES=10<<10,};
		static GLuint Queries[MAX_QUERIES] = {0};
		ZASSERT(10<<10 > g_WorldState.nNumWorldObjects);
		if(!Queries[0])
		{
			glGenQueries(MAX_QUERIES, &Queries[0]);
			uprintf("Generated Queries\n");
		}
		for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
		{
			SWorldObject* pObject = g_WorldState.WorldObjects + i;
			if(pObject->nFlags & SObject::OCCLUSION_TEST)
			{
				ShaderSetUniform(nLocSize, pObject->vSize);
				ShaderSetUniform(nLocModelView, pObject->mObjectToWorld);

				glBeginQuery(GL_SAMPLES_PASSED, Queries[i]);

				MeshDraw(GetBaseMesh(MESH_BOX_FLAT));

				glEndQuery(GL_SAMPLES_PASSED);
			}
		}
		for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
		{
			SWorldObject* pObject = g_WorldState.WorldObjects + i;
			if(pObject->nFlags & SObject::OCCLUSION_TEST)
			{
				int result, result0;
				glGetQueryObjectiv(Queries[i], GL_QUERY_RESULT, &result0);
				glGetQueryObjectiv(Queries[i], GL_QUERY_RESULT, &result);
				ZASSERT(result == result0);
				bCulledRef[i] = 10 > result;
				nCulledRef[i] = result;

			}
		}
		nNumFail = 0;
		uint32 nAgree = 0, nFailCull = 0, nFalsePositives = 0;
		for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
		{

			SWorldObject* pObject = g_WorldState.WorldObjects + i;
			if(pObject->nFlags & SObject::OCCLUSION_TEST)
			{
				if(bCulled[i] && !bCulledRef[i])
				{
					++nFailCull;
					uprintf("FAIL CULL %d .. count %d\n", i, nCulledRef[i]);
					if(nNumFail < MAX_FAIL)
						pFailObjects[nNumFail++] = pObject;

				}
				else if(bCulledRef[i] && !bCulled[i])
				{
					++nFalsePositives;
					uprintf("FALSE CULL %d\n", i);
				}
				else
				{
					++nAgree;
				}
			}
		}	
		uplotfnxt("FAIL %d  FALSE %d AGREE %d", nFailCull, nFalsePositives, nAgree);
		if(nNumFail)
		{
			uprintf("FAIL %d  FALSE %d AGREE %d\n", nFailCull, nFalsePositives, nAgree);
			uprintf("locking camera\n");
			g_nUseDebugCameraPos = 1;
		}
	}

	static float fub = 0;
	fub += 0.1f;
	float fMult = 2.2f + cos(fub) * 5.3f;
	if(nNumFail)
		uplotfnxt("DRAWING FAIL %d", nNumFail);
	for(int i = 0; i < nNumFail; ++i)
	{
		SWorldObject* pObject = pFailObjects[i];
		ZDEBUG_DRAWBOX(pObject->mObjectToWorld, pObject->mObjectToWorld.trans.tov3(), fMult * pObject->vSize, 0xffffff00, 0);

	}


	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	static int usezpass = 1;
	{
		MICROPROFILE_SCOPEGPUI("GPU", "OBJ PASS", 0xffff77);
		if(usezpass)
		{
			glDepthFunc(GL_LEQUAL);
			{
				//ShaderUse(VS_LIGHTING, PS_LIGHTING2);
				SHADER_SET("ProjectionMatrix", mprjview);
				MICROPROFILE_SCOPEGPUI("GPU", "zpass", 0x9900ee);
				glColorMask(0,0,0,0);
				WorldDrawObjects(&bCulled[0]);
			}
			ShaderUse(VS_LIGHTING, PS_LIGHTING);
			glDepthFunc(GL_EQUAL);
			glColorMask(1,1,1,1);
			WorldDrawObjects(&bCulled[0]);
			glDepthFunc(GL_LEQUAL);
		}
		else
		{
			glColorMask(1,1,1,1);
			WorldDrawObjects(&bCulled[0]);
		}
	}	
	ShaderDisable();
	glDisable(GL_CULL_FACE);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	CheckGLError();


	

}
void UpdateEditorState()
{
	//for(int i = 0; i < 10; ++i)
	{
		int but = 0;
		int released = 0;
		if(g_KeyboardState.keys[SDL_SCANCODE_0] & BUTTON_RELEASED)
		{
			but = 0;
			released = 1;

		}
		else if(g_KeyboardState.keys[SDL_SCANCODE_1] & BUTTON_RELEASED)
		{
			but = 1;
			released = 1;
		}
		else if(g_KeyboardState.keys[SDL_SCANCODE_2] & BUTTON_RELEASED)
		{
			but = 2;
			released = 1;
		}

		if(released)
		{
			g_EditorState.Mode = but;
			uprintf("switched mode to %d\n", g_EditorState.Mode);
			if(g_EditorState.Dragging != DRAGSTATE_NONE && g_EditorState.DragTarget == DRAGTARGET_TOOL)
			{
				if(g_EditorState.Manipulators[g_EditorState.Mode])
					g_EditorState.Manipulators[g_EditorState.Mode]->DragEnd(g_EditorState.vDragStart, g_EditorState.vDragEnd);
			}
			g_EditorState.Dragging = DRAGSTATE_NONE;

		}
	}
	uplotfnxt("mode is %d", g_EditorState.Mode);
	if(g_KeyboardState.keys[SDL_SCANCODE_ESCAPE] & BUTTON_RELEASED)
	{
		g_EditorState.pSelected = 0;
	}
	bool bShift = 0 != ((g_KeyboardState.keys[SDL_SCANCODE_RSHIFT]|g_KeyboardState.keys[SDL_SCANCODE_LSHIFT]) & BUTTON_DOWN);

	if(bShift && (g_KeyboardState.keys[SDL_SCANCODE_SPACE] & BUTTON_RELEASED))
	{
		if(g_EditorState.pSelected)
		{
			g_EditorState.bLockSelection = !g_EditorState.bLockSelection;
		}
		else
		{
			g_EditorState.bLockSelection = false;
		}
	}

	{
		v2 vPos = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		v3 vDir = DirectionFromScreen(vPos, g_WorldState.Camera);
		v3 t1 = g_WorldState.Camera.vPosition + vDir * 5.f;//vMouseWorld + v3normalize(vMouseWorld -g_WorldState.Camera.vPosition) *5.f;
	//	ZDEBUG_DRAWBOX(mid(), t1, v3init(0.2, 0.2, 0.2), -1);

	}

	if(g_MouseState.button[1]&BUTTON_PUSHED)
	{
		v2 vPos = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		if(!g_EditorState.pSelected || !g_EditorState.Manipulators[g_EditorState.Mode] || !g_EditorState.Manipulators[g_EditorState.Mode]->DragBegin(vPos, vPos, g_EditorState.pSelected))

			// ((g_KeyboardState.keys[SDL_SCANCODE_LCTRL]|g_KeyboardState.keys[SDL_SCANCODE_RCTRL]) & BUTTON_DOWN)|| g_EditorState.pSelected == 0)
		{
			g_EditorState.DragTarget = DRAGTARGET_CAMERA;
			uprintf("Drag begin CAMERA\n");
		}
		else
		{
			g_EditorState.DragTarget = DRAGTARGET_TOOL;
			uprintf("Drag begin TOOL %d\n", g_EditorState.Mode);
		}

		{

			g_EditorState.Dragging = DRAGSTATE_BEGIN;
			g_EditorState.vDragEnd = g_EditorState.vDragStart = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		}
	}
	else if(g_MouseState.button[1]&BUTTON_RELEASED)
	{
		g_EditorState.Dragging = DRAGSTATE_END;
		v2 vPrev = g_EditorState.vDragEnd;
		g_EditorState.vDragEnd = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		g_EditorState.vDragDelta = g_EditorState.vDragEnd - vPrev;
		uprintf("Drag end\n");
	}
	else if(g_MouseState.button[1]&BUTTON_DOWN)
	{
		g_EditorState.Dragging = DRAGSTATE_UPDATE;
		v2 vPrev = g_EditorState.vDragEnd;
		g_EditorState.vDragEnd = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		g_EditorState.vDragDelta = g_EditorState.vDragEnd - vPrev;
		uplotfnxt("DRAGGING %f %f ", g_EditorState.vDragEnd.x, g_EditorState.vDragEnd.y);
	}
	else
	{
		g_EditorState.vDragEnd = g_EditorState.vDragStart = g_EditorState.vDragDelta = v2init(0,0);
		g_EditorState.Dragging = DRAGSTATE_NONE;
	}

	switch(g_EditorState.DragTarget)
	{
		case DRAGTARGET_CAMERA:
		{
			g_WorldState.vCameraRotate = g_EditorState.vDragDelta;
		}
		break;
		case DRAGTARGET_TOOL:
		{
			ZASSERT(g_EditorState.pSelected);
			switch(g_EditorState.Mode)
			{
				default:
				{
					switch(g_EditorState.Dragging)
					{
						case DRAGSTATE_BEGIN:
						{
							g_EditorState.mSelected = g_EditorState.pSelected->mObjectToWorld;
						}
						break;
						case DRAGSTATE_UPDATE:
						{
							uplotfnxt("DRAG UP");
							if(g_EditorState.Manipulators[g_EditorState.Mode])
							{
								g_EditorState.Manipulators[g_EditorState.Mode]->DragUpdate(g_EditorState.vDragStart, g_EditorState.vDragEnd);
							}
						}

					}
				}
			}
		}
		break;
	}

	switch(g_EditorState.Mode)
	{
		case 0: //translate
		{

		}
	}

	if(g_EditorState.Dragging == DRAGSTATE_END)
	{
		g_EditorState.DragTarget = DRAGTARGET_NONE;
	}
}
void UpdatePicking()
{

	v3 vMouse = v3init( g_MouseState.position[0], g_MouseState.position[1], 0);
	v3 vMouseClip = mtransform(g_WorldState.Camera.mviewportinv, vMouse);
	vMouseClip.z = -0.9f;
	v4 vMouseView_ = mtransform(g_WorldState.Camera.mprjinv, v4init(vMouseClip, 1.f));
	v3 vMouseView = vMouseView_.tov3() / vMouseView_.w;
	v3 vMouseWorld = mtransform(g_WorldState.Camera.mviewinv, vMouseView);
	// uplotfnxt("vMouse %f %f %f", vMouse.x, vMouse.y, vMouse.z);
	// uplotfnxt("vMouseClip %f %f %f", vMouseClip.x, vMouseClip.y, vMouseClip.z);
	// uplotfnxt("vMouseView %f %f %f", vMouseView.x, vMouseView.y, vMouseView.z);
	// uplotfnxt("vMouseWorld %f %f %f", vMouseWorld.x, vMouseWorld.y, vMouseWorld.z);
	// v3 test = g_WorldState.Camera.vPosition + g_WorldState.Camera.vDir * 3.f;
	// ZDEBUG_DRAWBOX(mid(), test, v3init(0.3, 0.3, 0.3), -1);
	v3 t0 = vMouseWorld;
	v3 t1 = vMouseWorld + v3normalize(vMouseWorld -g_WorldState.Camera.vPosition) *5.f;
	//	ZDEBUG_DRAWBOX(mid(), t1, v3init(0.1, 0.1, 0.1), -1);
	// ZDEBUG_DRAWLINE(test, t0, -1, 0);

	if(g_MouseState.button[1] & BUTTON_RELEASED && !g_EditorState.bLockSelection)
	{
		v3 vPos = g_WorldState.Camera.vPosition;
		v3 vDir = vMouseWorld - vPos;
		vDir = v3normalize(vDir);
		float fNearest = 1e30;
		SObject* pNearest = 0;

		auto Intersect = [&] (SObject* pObject) 
		{ 
			float fColi = rayboxintersect(vPos, vDir, pObject->mObjectToWorld, pObject->vSize);
			if(fColi > g_WorldState.Camera.fNear && fColi < fNearest)
			{
				pNearest = pObject;
				fNearest = fColi;
			}
		};
		for(SObject& Obj : g_WorldState.WorldObjects)
			Intersect(&Obj);

		for(SObject& Obj : g_WorldState.Occluders)
			Intersect(&Obj);
		g_EditorState.pSelected = pNearest;
	}


}

void UpdateCamera()
{
	v3 vDir = v3init(0,0,0);

	// if(g_MouseState.button[SDL_BUTTON_WHEELUP] & BUTTON_RELEASED)
	// 	g_fOrthoScale *= 0.96;
	// if(g_MouseState.button[SDL_BUTTON_WHEELDOWN] & BUTTON_RELEASED)
	// 	g_fOrthoScale /= 0.96;



	if(g_KeyboardState.keys[SDL_SCANCODE_A] & BUTTON_DOWN)
	{
		vDir.x = -1.f;
	}
	else if(g_KeyboardState.keys[SDL_SCANCODE_D] & BUTTON_DOWN)
	{
		vDir.x = 1.f;
	}

	if(g_KeyboardState.keys[SDL_SCANCODE_W] & BUTTON_DOWN)
	{
		vDir.z = 1.f;
	}
	else if(g_KeyboardState.keys[SDL_SCANCODE_S] & BUTTON_DOWN)
	{
		vDir.z = -1.f;
	}
	float fSpeed = 0.1f;
	if((g_KeyboardState.keys[SDL_SCANCODE_RSHIFT]|g_KeyboardState.keys[SDL_SCANCODE_LSHIFT]) & BUTTON_DOWN)
		fSpeed *= 0.2f;
	if((g_KeyboardState.keys[SDL_SCANCODE_RCTRL]|g_KeyboardState.keys[SDL_SCANCODE_LCTRL]) & BUTTON_DOWN)
		fSpeed *= 12.0f;
	g_WorldState.Camera.vPosition += g_WorldState.Camera.vDir * vDir.z * fSpeed;
	g_WorldState.Camera.vPosition += g_WorldState.Camera.vRight * vDir.x * fSpeed;


	// static int mousex, mousey;
	// if(g_MouseState.button[1] & BUTTON_PUSHED)
	// {
	// 	mousex = g_MouseState.position[0];
	// 	mousey = g_MouseState.position[1];
	// }

	//if(g_MouseState.button[1] & BUTTON_DOWN)
	{
		int dx = g_WorldState.vCameraRotate.x;//g_MouseState.position[0] - mousex;
		int dy = g_WorldState.vCameraRotate.y;//g_MouseState.position[1] - mousey;
		// mousex = g_MouseState.position[0];
		// mousey = g_MouseState.position[1];

		float fRotX = dy * 0.25f;
		float fRotY = dx * -0.25f;
		m mrotx = mrotatex(fRotX*TORAD);
		m mroty = mrotatey(fRotY*TORAD);
		g_WorldState.Camera.vDir = mtransform(mroty, g_WorldState.Camera.vDir);
		g_WorldState.Camera.vRight = mtransform(mroty, g_WorldState.Camera.vRight);

		m mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, v3init(0,0,0));
		m mviewinv = minverserotation(mview);
		v3 vNewDir = mtransform(mrotx, v3init(0,0,-1));
		g_WorldState.Camera.vDir = mtransform(mviewinv, vNewDir);

	}
	ZASSERTNORMALIZED3(g_WorldState.Camera.vDir);
	ZASSERTNORMALIZED3(g_WorldState.Camera.vRight);



	g_WorldState.Camera.mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, g_WorldState.Camera.vPosition);
	if(g_KeyboardState.keys[SDL_SCANCODE_F2] & BUTTON_RELEASED)
		g_nUseOrtho = !g_nUseOrtho;

	float fAspect = (float)g_Width / (float)g_Height;
	
	if(g_nUseOrtho)
	{
		g_WorldState.Camera.mprj = mortho(g_fOrthoScale, g_fOrthoScale / fAspect, 1000);
	}
	else
	{
		g_WorldState.Camera.mprj = mperspective(g_WorldState.Camera.fFovY, ((float)g_Height / (float)g_Width), g_WorldState.Camera.fNear, 2000.f);
	}
	
	g_WorldState.Camera.mviewport = mviewport(0,0,g_Width, g_Height);
	g_WorldState.Camera.mviewportinv = minverse(g_WorldState.Camera.mviewport);
	g_WorldState.Camera.mprjinv = minverse(g_WorldState.Camera.mprj);
	g_WorldState.Camera.mviewinv = maffineinverse(g_WorldState.Camera.mview);

	uplotfnxt("FPS %4.2f Dir[%5.2f,%5.2f,%5.2f] Pos[%3.2f,%3.2f,%3.2f]" , 1.f, 
		g_WorldState.Camera.vDir.x,
		g_WorldState.Camera.vDir.y,
		g_WorldState.Camera.vDir.z,
		g_WorldState.Camera.vPosition.x,
		g_WorldState.Camera.vPosition.y,
		g_WorldState.Camera.vPosition.z);


}
void foo()
{
	//uprintf("lala\n");
	ZBREAK();
}


void ProgramInit()
{
	m mroty = mrotatey(45.f * TORAD);
#if 0
	g_WorldState.Camera.vDir = mtransform(mroty, v3init(0,0,-1));
	g_WorldState.Camera.vRight = mtransform(mroty, v3init(1,0,0));
	g_WorldState.Camera.vPosition = g_WorldState.Camera.vDir * -5.f;
#else
	g_WorldState.Camera.vPosition = v3init(133.101120,116.293892,111.730286);
	g_WorldState.Camera.vDir = v3init(-0.690784,-0.394744,-0.605801);
	g_WorldState.Camera.vRight = v3init(0.659345,0.000000,-0.751839);
#endif
	g_WorldState.Camera.fFovY = 45.f;
	g_WorldState.Camera.fNear = 0.1f;
	g_Bsp = BspCreate();
	WorldInit();
	EditorStateInit();


	g_SM = AllocateShadowMap();

}

int ProgramMain()
{

	if(g_KeyboardState.keys[SDL_SCANCODE_ESCAPE] & BUTTON_RELEASED)
		return 1;

	if(g_KeyboardState.keys[SDL_SCANCODE_SPACE] & BUTTON_RELEASED)
	{
		g_nUseDebugCameraPos = (g_nUseDebugCameraPos+1)%3;
	}
	MICROPROFILE_SCOPEGPUI("GPU", "Full Frame", 0x88dd44);
	UpdateEditorState();
	{
		UpdateCamera();
		UpdatePicking();
	}

	glLineWidth(1.f);
	glClearColor(0.3,0.4,0.6,0);
	glViewport(0, 0, g_Width, g_Height);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	CheckGLError();

	m mprj = g_WorldState.Camera.mprj;
	m mview = g_WorldState.Camera.mview;
	m mprjview = mmult(mprj, mview);

	// glMatrixMode(GL_PROJECTION);
	// glLoadMatrixf(&g_WorldState.Camera.mprj.x.x);
	// glMultMatrixf(&g_WorldState.Camera.mview.x.x);

	// glMatrixMode(GL_MODELVIEW);
	// glPushMatrix();
	// glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_DEPTH_TEST);
	CheckGLError();
	{
		MICROPROFILE_SCOPEGPUI("GPU", "Render World", 0x88dd44);
		WorldRender();
	}
	CheckGLError();
	{
		DebugRender(mprjview);
	}
	CheckGLError();

	//glPopMatrix();

	return 0;
}




v3 DirectionFromScreen(v2 vScreen, SCameraState& Camera)
{
	v3 vMouse = v3init(vScreen.x, vScreen.y, 0);
	v3 vMouseClip = mtransform(Camera.mviewportinv, vMouse);
	vMouseClip.z = -0.9f;
	v4 vMouseView_ = mtransform(Camera.mprjinv, v4init(vMouseClip, 1.f));
	v3 vMouseView = vMouseView_.tov3() / vMouseView_.w;
	v3 vMouseWorld = mtransform(Camera.mviewinv, vMouseView);
	return v3normalize(vMouseWorld - Camera.vPosition);

}



