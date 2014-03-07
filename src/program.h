#pragma once
#include "fwd.h"
//stuff that doesnt belong anywhere (debug/temp stuff)


enum
{
	MAX_OCCLUDERS = 100,
	MAX_WORLD_OBJECTS = 10 << 10,
	MAX_LIGHTS = 1024,
};

#ifdef __APPLE__
#define __ALIGN16 __attribute__((aligned(16))) 
#elif defined(_WIN32)
#define __ALIGN16 __declspec(align(16))
#endif

struct __ALIGN16 SObject
{
	m mObjectToWorld;//!!must follow each other to remain compatible with SOccluderDesc
	v3 vSize;		 //!!must follow each other to remain compatible with SOccluderDesc
	uint32_t nColor;
	uint32_t nFlags;

	enum
	{
		OCCLUDER_PLANE = 0x1,
		OCCLUDER_BOX = 0x2,
		OCCLUSION_TEST = 0x4,
		OCCLUSION_BOX_SKIP_X = 0x8,
		OCCLUSION_BOX_SKIP_Y = 0x10,
		OCCLUSION_BOX_SKIP_Z = 0x20,
	};
};

struct SOccluder : SObject
{
};

struct SWorldObject : SObject
{


};

struct SCameraState : SObject
{
	v3 vDir;
	v3 vRight;
	v3 vPosition;
	m mview;
	m mviewinv;
	m mprj;
	m mprjinv;
	m mviewport;
	m mviewportinv;	
	float fNear;
	float fFar;
	float fFovY;
};

struct SLight : SObject
{
	uint32_t nColor;
	float fRadius;
};

v3 DirectionFromScreen(v2 vScreen, SCameraState& Camera);


enum DragState
{
	DRAGSTATE_NONE,
	DRAGSTATE_BEGIN,
	DRAGSTATE_UPDATE,
	DRAGSTATE_END,
};
enum DragTarget
{
	DRAGTARGET_NONE,
	DRAGTARGET_SELECT,
	DRAGTARGET_TOOL,
	DRAGTARGET_CAMERA,
};
enum ManipulatorMode
{
	MANIPULATOR_TRANSLATE,
	MANIPULATOR_ROTATE,
	MANIPULATOR_SCALE,
	MANIPULATOR_SIZE,
};
struct SEditorState
{
	bool bLockSelection;
	int Mode;

	int DragTarget;
	int Dragging;
	v2 vDragStart;
	v2 vDragEnd;
	v2 vDragDelta;


	Manipulator* Manipulators[MANIPULATOR_SIZE];

	SObject* pSelected;
	m mSelected;

};

struct SWorldState
{
	SOccluder Occluders[MAX_OCCLUDERS];
	uint32 nNumOccluders;

	SWorldObject WorldObjects[MAX_WORLD_OBJECTS];
	uint32 nNumWorldObjects;

	SLight Lights[MAX_LIGHTS];
	uint32 nNumLights;


	SCameraState Camera;


	v2 vCameraTranslate;
	v2 vCameraRotate;

};

extern SWorldState g_WorldState;
extern SEditorState g_EditorState;