#pragma once
#include "fwd.h"
//stuff that doesnt belong anywhere (debug/temp stuff)


enum
{
	MAX_OCCLUDERS = 100,
	MAX_WORLD_OBJECTS = 2048,
};

struct __attribute__((aligned(16))) SObject
{
	m mObjectToWorld;
	v3 vSize;
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


	SCameraState Camera;


	v2 vCameraTranslate;
	v2 vCameraRotate;

};

extern SWorldState g_WorldState;
extern SEditorState g_EditorState;