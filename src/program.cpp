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

extern uint32_t g_Width;
extern uint32_t g_Height;
//render todo
//tile calc
//debug render fixing + color

uint32 g_nUseOrtho = 0;
float g_fOrthoScale = 10;
SOccluderBsp* g_Bsp = 0;
uint32 g_nUseDebugCameraPos = 2;
v3 vLockedCamPos = v3init(0,0,0);
v3 vLockedCamRight = v3init(0,-1,0);
v3 vLockedCamDir = v3init(1,0,0);


#define TILE_SIZE 16
//#define TILE_HEIGHT 8
#define MAX_NUM_LIGHTS 1024
GLuint g_LightTexture;
GLuint g_LightTileTexture;
uint32_t g_LightTileWidth;
uint32_t g_LightTileHeight;

struct LightDesc
{
	float Pos[4];
	float Color[4];
};


LightDesc g_LightBuffer[MAX_NUM_LIGHTS];

SWorldState g_WorldState;
SEditorState g_EditorState;

void DebugRender()
{
	if(g_EditorState.pSelected)
	{
		ZDEBUG_DRAWBOUNDS(g_EditorState.pSelected->mObjectToWorld, g_EditorState.pSelected->vSize, g_EditorState.bLockSelection ? (0xffffff00) : -1);

	}
	glBegin(GL_LINES);
	glColor3f(1,0,0);
	glVertex3f(0, 0, 0.f);
	glVertex3f(1.f, 0, 0.f);
	glColor3f(0,1,0);
	glVertex3f(0, 0, 0.f);
	glVertex3f(0.f, 1.f, 0.f);
	glColor3f(0,0,1);
	glVertex3f(0, 0, 0.f);
	glVertex3f(0.f, 0, 1.f);
	glEnd();


	DebugDrawFlush();
}

void EditorStateInit()
{
	memset(&g_EditorState, 0, sizeof(g_EditorState));
	g_EditorState.Manipulators[0] = new ManipulatorTranslate();
	g_EditorState.Manipulators[1] = new ManipulatorRotate();
}

void WorldInit()
{
	g_WorldState.Occluders[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[0].mObjectToWorld.trans = v4init(2.f,0.f,0.1f, 1.f);
	g_WorldState.Occluders[0].vSize = v3init(0.5f, 0.5f, 0.f);

	g_WorldState.Occluders[1].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(2.5f,0.f,-0.5f, 1.f);
	//g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(1.5f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[1].vSize = v3init(0.25f, 0.25f, 0);

	g_WorldState.Occluders[2].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[2].mObjectToWorld.trans = v4init(2.6f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[2].vSize = v3init(0.25f, 0.25f, 0);
	g_WorldState.nNumOccluders = 3;


	g_WorldState.WorldObjects[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f,0.f,-0.5f, 1.f);
	g_WorldState.WorldObjects[0].vSize = v3init(0.25f, 0.2f, 0.2f); 

	g_WorldState.WorldObjects[1].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.WorldObjects[1].mObjectToWorld.trans = v4init(3.f,0.f,-1.5f, 1.f);
	g_WorldState.WorldObjects[1].vSize = v3init(0.25f, 0.2f, 0.2f)* 0.5f; 
	g_WorldState.nNumWorldObjects = 2;

	if(1)
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
	}

	for(int i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		PhysicsAddObjectBox(&g_WorldState.WorldObjects[i]);
	}

	g_WorldState.nNumLights = 15;
	for(int i = 0; i < g_WorldState.nNumLights; ++i)
	{
		m mat = mid();
		mat.trans.x = frandrange(-5.f, 5.f);
		mat.trans.y = frandrange(-2.7f, -3.f);
		mat.trans.z = frandrange(-5.f, 5.f);
//		uprintf("LIGHT AT %f %f %f\n", mat.trans.x, mat.trans.y, mat.trans.z);
		g_WorldState.Lights[i].mObjectToWorld = mat;
		g_WorldState.Lights[i].nColor = randcolor() | 0xff000000;
	}



	g_EditorState.pSelected = 0;


	CheckGLError();
	glGenTextures(1, &g_LightTexture);
	glGenTextures(1, &g_LightTileTexture);
	memset(&g_LightBuffer, 0, sizeof(g_LightBuffer));
	glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MAX_NUM_LIGHTS*2, 1, 0, GL_RGBA, GL_FLOAT, &g_LightBuffer);


	glBindTexture(GL_TEXTURE_2D, g_LightTileTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
    }
    g_LightTileWidth = g_Width / TILE_SIZE;
    g_LightTileHeight = g_Height / TILE_SIZE;
    ZASSERT(g_LightTileWidth * TILE_SIZE == g_Width);
    ZASSERT(g_LightTileHeight * TILE_SIZE == g_Height);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_LightTileWidth, g_LightTileHeight, 0, GL_RGBA, GL_SHORT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGLError();

}




int g_nSimulate = 0;
void WorldRender()
{
	MICROPROFILE_SCOPEI("MAIN", "WorldRender", 0xff44dddd);
	if(g_KeyboardState.keys[SDLK_F1] & BUTTON_RELEASED)
	{
		g_nSimulate = !g_nSimulate;
	}
	if(g_nSimulate)
	{
		g_WorldState.WorldObjects[0].mObjectToWorld = mmult(g_WorldState.WorldObjects[0].mObjectToWorld, mrotatey(0.5f*TORAD));
		g_WorldState.Occluders[0].mObjectToWorld = mmult(g_WorldState.Occluders[0].mObjectToWorld, mrotatez(0.3f*TORAD));
		static float xx = 0;
		xx += 0.005f;
		g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f, 0.f, sin(xx)*2, 1.f);
	}

	uint32 nNumOccluders = g_WorldState.nNumOccluders;
	static float foo = 0;
	foo += 0.01f;
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
		
	
	ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.02f), -1,1);
	SOccluderBspViewDesc ViewDesc;
	ViewDesc.vOrigin = vPos;
	ViewDesc.vDirection = vDir;
	ViewDesc.vRight = vRight;
	ViewDesc.fFovY = g_WorldState.Camera.fFovY;
	ViewDesc.fAspect = (float)g_Height / (float)g_Width;
	ViewDesc.fZNear = g_WorldState.Camera.fNear;

	BspBuild(g_Bsp, &g_WorldState.Occluders[0], nNumOccluders, &g_WorldState.WorldObjects[0], 
		//g_WorldState.nNumWorldObjects, 
		1,
		ViewDesc);

	uint32 nNumObjects = g_WorldState.nNumWorldObjects;
	bool* bCulled = (bool*)alloca(nNumObjects);
	bCulled[0] = false;
	for(uint32 i = 1; i < nNumObjects; ++i)
	{
		bCulled[i] = BspCullObject(g_Bsp, &g_WorldState.WorldObjects[i]);
	}
	static v3 LightPos = v3init(0,-2.7,0);
	static v3 LightColor = v3init(0.9, 0.5, 0.9);
	static v3 LightPos0 = v3init(0,-2.7,0);
	static v3 LightColor0 = v3init(0.3, 1.0, 0.3);

	LightPos.z = (sin(foo*10)) * 5;
	LightPos0.x = (sin(foo*10)) * 5;

	g_WorldState.Lights[0].mObjectToWorld.trans.z = (sin(foo*5.5)) * 5;
	g_WorldState.Lights[1].mObjectToWorld.trans.x = (cos((foo+2)*3)) * 5;

	#define BUFFER_SIZE (512*512)
	static v3 PointBuffer[BUFFER_SIZE];
	static int nNumPoints = 0;
	bool bDumpPoints = false;
	if(g_KeyboardState.keys['b']&BUTTON_RELEASED)
	{
		bDumpPoints = true;
		nNumPoints = 0;
	}

	m mprj = (g_WorldState.Camera.mprj);
	m mprjinv = minverse(g_WorldState.Camera.mprj);
	//#define PP uprintf
	#define PP(...)
	for(int i=0; i < 200; ++i)
	{
		float foo = i - 100.f;
		v4 val = v4init(0.f,0.f,foo* g_WorldState.Camera.fNear, 1.f);
		PP("DIST %f  :: ", val.z);
		val = mtransform(mprj, val);
		val = val / val.w;
		PP(" %f %f %f\n", val.x, val.y, val.z);
	}

	for(int i=0; i < 20; ++i)
	{
		float z = (i / 19.f) * 2.f - 1.f;
		PP("inv z %f :: ", z);
		v4 val = v4init(0,0,z,1.f);
		val = mtransform(mprjinv, val);
		PP(" UNPRJ %f %f %f %f ::", val.x, val.y, val.z, val.w);
		val = val / val.w;
		PP(" PRJ %f %f %f %f\n", val.x, val.y, val.z, val.w);
	}


	v4 z = v4init(0,0,-g_WorldState.Camera.fNear,1);
	v4 z1 = v4init(0,0,g_WorldState.Camera.fNear,1);

	v4 l = mtransform(mprj, z);
	v4 l1 = mtransform(mprj, z1);
	l = l / l.w;
	l1 = l1 / l1.w;
//	ZBREAK();


	m mcliptoworld = mmult(g_WorldState.Camera.mviewinv, g_WorldState.Camera.mprjinv);
	for(int i = 0; i < g_LightTileWidth; ++i)
	{
		float x0 = 2*(float)i / (g_LightTileWidth)-1;
		float x1 = 2*(float)(i+1) / (g_LightTileWidth)-1;
		for(int j = 0; j < g_LightTileHeight; ++j)
		{
			float y0 = 2*(float)j / (g_LightTileHeight)-1;
			float y1 = 2*(float)(j+1) / (g_LightTileHeight)-1;
			v4 v0 = mtransform(mcliptoworld, v4init(x0,y0,-1.f,1));
			v4 v1 = mtransform(mcliptoworld, v4init(x1,y0,-1.f,1));
			v4 v2 = mtransform(mcliptoworld, v4init(x1,y1,-1.f,1));
			v4 v3 = mtransform(mcliptoworld, v4init(x0,y1,-1.f,1));
			v0 = v0 / v0.w;
			v1 = v1 / v1.w;
			v2 = v2 / v2.w;
			v3 = v3 / v3.w;

			if(bDumpPoints)
			{
				uprintf("DUMP %f %f %f\n", v0.x, v0.y, v0.z);
				PointBuffer[nNumPoints++] = v0.tov3();
				PointBuffer[nNumPoints++] = v1.tov3();
				PointBuffer[nNumPoints++] = v2.tov3();
				PointBuffer[nNumPoints++] = v3.tov3();
				ZASSERT(nNumPoints < BUFFER_SIZE);
			}
		}
	}
	if(nNumPoints)
	{
		ZASSERT(nNumPoints == 4 * g_LightTileWidth * g_LightTileHeight);

		///uprintf("NUM POINTS %d\n", nNumPoints);
		for(int i = 0; i < nNumPoints;i+=4)
		{
			ZDEBUG_DRAWLINE(PointBuffer[i], PointBuffer[i+1], -1, 0);
//			ZDEBUG_DRAWLINE(PointBuffer[i+1], PointBuffer[i+2], -1, 0);
//			ZDEBUG_DRAWLINE(PointBuffer[i+2], PointBuffer[i+3], -1, 0);
			ZDEBUG_DRAWLINE(PointBuffer[i+3], PointBuffer[i], -1, 0);



//			ZDEBUG_DRAWBOX(mid(), PointBuffer[i], v3rep(0.1f), (uint32_t)-1, 0);
//			ZDEBUG_DRAWBOX(ObjectToWorld, g_WorldStatew.tov3(), g_WorldState.WorldObjects[i].vSize, 0xffff0000, 0);

		}
	}

	{
		MICROPROFILE_SCOPEI("MAIN", "UpdateTexture", 0xff44dd44);
		CheckGLError();
		glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MAX_NUM_LIGHTS*2, 1, GL_RGBA, GL_FLOAT, &g_LightBuffer);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    CheckGLError();
	}
	#if 1
	for(int i = 0; i < g_WorldState.nNumLights; ++ i)
	{

//		v3 col = ;

		ZDEBUG_DRAWSPHERE(g_WorldState.Lights[i].mObjectToWorld.trans.tov3(), 2.f, g_WorldState.Lights[i].nColor);
	
		g_LightBuffer[i].Pos[0] = g_WorldState.Lights[i].mObjectToWorld.trans.x;
		g_LightBuffer[i].Pos[1] = g_WorldState.Lights[i].mObjectToWorld.trans.y;
		g_LightBuffer[i].Pos[2] = g_WorldState.Lights[i].mObjectToWorld.trans.z;
		v3 col = v3fromcolor(g_WorldState.Lights[i].nColor);
		g_LightBuffer[i].Color[0] = col.x;
		g_LightBuffer[i].Color[1] = col.y;
		g_LightBuffer[i].Color[2] = col.z;
	}
	int nNumLights = g_WorldState.nNumLights;
	#else
	g_LightBuffer[0].Pos[0] = LightPos.x;
	g_LightBuffer[0].Pos[1] = LightPos.y;
	g_LightBuffer[0].Pos[2] = LightPos.z;
	g_LightBuffer[0].Color[0] = LightColor.x;
	g_LightBuffer[0].Color[1] = LightColor.y;
	g_LightBuffer[0].Color[2] = LightColor.z;

	g_LightBuffer[1].Pos[0] = LightPos0.x;
	g_LightBuffer[1].Pos[1] = LightPos0.y;
	g_LightBuffer[1].Pos[2] = LightPos0.z;
	g_LightBuffer[1].Color[0] = LightColor0.x;
	g_LightBuffer[1].Color[1] = LightColor0.y;
	g_LightBuffer[1].Color[2] = LightColor0.z;
		int nNumLights = g_WorldState.nNumLights;
	#endif


	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	glActiveTexture(GL_TEXTURE0);

	ShaderUse(VS_LIGHTING, PS_LIGHTING);
	SHADER_SET("texLight", 1);
	SHADER_SET("NumLights", nNumLights);
	SHADER_SET("lightDelta", 1.f / (2*MAX_NUM_LIGHTS));
	SHADER_SET("LightPos[0]", LightPos);
	SHADER_SET("LightColor[0]", LightColor);

	SHADER_SET("LightPos[1]", LightPos0);
	SHADER_SET("LightColor[1]", LightColor0);

	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		glPushMatrix();
		glMultMatrixf(&g_WorldState.WorldObjects[i].mObjectToWorld.x.x);
		if(bCulled[i])
		{
//			uplotfnxt("CULLED %d", i);
			ZDEBUG_DRAWBOX(g_WorldState.WorldObjects[i].mObjectToWorld, g_WorldState.WorldObjects[i].mObjectToWorld.w.tov3(), g_WorldState.WorldObjects[i].vSize, 0xffff0000, 0);
		}
		else
		{
//			uplotfnxt("NOT CULLED %d", i);
			
			//ShaderUse(VS_DEFAULT, PS_FLAT_LIT);
			SHADER_SET("Size", g_WorldState.WorldObjects[i].vSize);
			glEnable(GL_CULL_FACE);
			MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
			glDisable(GL_CULL_FACE);
			
			CheckGLError();

		}

		glPopMatrix();
	}
	ShaderDisable();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	

}
void UpdateEditorState()
{
	for(int i = 0; i < 10; ++i)
	{
		if(g_KeyboardState.keys['0' + i] & BUTTON_RELEASED)
		{
			g_EditorState.Mode = i;
			if(g_EditorState.Dragging != DRAGSTATE_NONE && g_EditorState.DragTarget == DRAGTARGET_TOOL)
			{
				if(g_EditorState.Manipulators[g_EditorState.Mode])
					g_EditorState.Manipulators[g_EditorState.Mode]->DragEnd(g_EditorState.vDragStart, g_EditorState.vDragEnd);
			}
			g_EditorState.Dragging = DRAGSTATE_NONE;

		}
	}
	uplotfnxt("mode is %d", g_EditorState.Mode);
	if(g_KeyboardState.keys[SDLK_ESCAPE] & BUTTON_RELEASED)
	{
		g_EditorState.pSelected = 0;
	}
	if(g_KeyboardState.keys[SDLK_SPACE] & BUTTON_RELEASED)
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

			// ((g_KeyboardState.keys[SDLK_LCTRL]|g_KeyboardState.keys[SDLK_RCTRL]) & BUTTON_DOWN)|| g_EditorState.pSelected == 0)
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
	if(g_MouseState.button[SDL_BUTTON_WHEELUP] & BUTTON_RELEASED)
		g_fOrthoScale *= 0.96;
	if(g_MouseState.button[SDL_BUTTON_WHEELDOWN] & BUTTON_RELEASED)
		g_fOrthoScale /= 0.96;
	//uplotfnxt("ORTHO SCALE %f\n", g_fOrthoScale);



	if(g_KeyboardState.keys['a'] & BUTTON_DOWN)
	{
		vDir.x = -1.f;
	}
	else if(g_KeyboardState.keys['d'] & BUTTON_DOWN)
	{
		vDir.x = 1.f;
	}

	if(g_KeyboardState.keys['w'] & BUTTON_DOWN)
	{
		vDir.z = 1.f;
	}
	else if(g_KeyboardState.keys['s'] & BUTTON_DOWN)
	{
		vDir.z = -1.f;
	}
	float fSpeed = 0.1f;
	if((g_KeyboardState.keys[SDLK_RSHIFT]|g_KeyboardState.keys[SDLK_LSHIFT]) & BUTTON_DOWN)
		fSpeed *= 0.2f;
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
	if(g_KeyboardState.keys[SDLK_F2] & BUTTON_RELEASED)
		g_nUseOrtho = !g_nUseOrtho;

	float fAspect = (float)g_Width / (float)g_Height;
	
	if(g_nUseOrtho)
	{
		g_WorldState.Camera.mprj = mortho(g_fOrthoScale, g_fOrthoScale / fAspect, 1000);
	}
	else
	{
		g_WorldState.Camera.mprj = mperspective(g_WorldState.Camera.fFovY, ((float)g_Height / (float)g_Width), g_WorldState.Camera.fNear, 100.f);
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
	uprintf("lala\n");
	ZBREAK();
}


void ProgramInit()
{
	m mroty = mrotatey(45.f * TORAD);
	g_WorldState.Camera.vDir = mtransform(mroty, v3init(0,0,-1));
	g_WorldState.Camera.vRight = mtransform(mroty, v3init(1,0,0));
	g_WorldState.Camera.vPosition = g_WorldState.Camera.vDir * -5.f;
	g_WorldState.Camera.fFovY = 45.f;
	g_WorldState.Camera.fNear = 0.1f;
	g_Bsp = BspCreate();
	WorldInit();
	EditorStateInit();

}

int ProgramMain()
{

	if(g_KeyboardState.keys[SDLK_ESCAPE] & BUTTON_RELEASED)
		return 1;

	if(g_KeyboardState.keys[SDLK_SPACE] & BUTTON_RELEASED)
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

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&g_WorldState.Camera.mprj.x.x);
	glMultMatrixf(&g_WorldState.Camera.mview.x.x);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	//glLoadMatrixf(&g_WorldState.Camera.mview.x.x);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CheckGLError();
	{
		MICROPROFILE_SCOPEGPUI("GPU", "Render World", 0x88dd44);
		WorldRender();
	}
	{
		DebugRender();
	}


	glPopMatrix();

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



