#pragma once
//stuff that doesnt belong anywhere (debug/temp stuff)


enum
{
	MAX_OCCLUDERS = 100,
	MAX_WORLD_OBJECTS = 1024,
};

struct SObject
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
	m mprj;
	m mprjinv;
	m mviewinv;

	v3 vMouseWorld;
};



struct SWorldState
{
	SOccluder Occluders[MAX_OCCLUDERS];
	uint32 nNumOccluders;

	SWorldObject WorldObjects[MAX_WORLD_OBJECTS];
	uint32 nNumWorldObjects;


	SCameraState Camera;
};

extern SWorldState g_WorldState;