#include "manipulator.h"
#include "program.h"
#include "debug.h"

void ManipulatorTranslate::Draw()
{

}
bool ManipulatorTranslate::DragBegin(v2 vDragStart, v2 vDragEnd, SObject* pSelection)
{
	m_pSelection = pSelection;
	mBase = pSelection->mObjectToWorld;
	v3 vDir = DirectionFromScreen(vDragStart, *g_WorldState.pActiveCamera);
	float fIntersect = rayboxintersect(g_WorldState.pActiveCamera->vPosition, vDir, pSelection->mObjectToWorld, pSelection->vSize);
	vBasePoint = g_WorldState.pActiveCamera->vPosition + vDir * fIntersect;
	v3 size = v3init(1,1,1)*0.3;
	m id = mid();
	fBaseDist = fIntersect;
	return fIntersect > 0.f;
}
void ManipulatorTranslate::DragUpdate(v2 vDragStart, v2 vDragEnd)
{
	ZASSERT(m_pSelection);

	v3 vDirStart = DirectionFromScreen(vDragStart, *g_WorldState.pActiveCamera);
	v3 vDirEnd = DirectionFromScreen(vDragEnd, *g_WorldState.pActiveCamera);
	v3 vNewPoint = g_WorldState.pActiveCamera->vPosition + vDirEnd * fBaseDist;
	v3 vDelta = vNewPoint - vBasePoint;
	m_pSelection->mObjectToWorld.trans = v4init(mBase.trans.tov3() + vDelta, 1.f);
	m id = mid();
	v3 vSize = v3init(0.5f, 0.5f, 0.5f);
}
void ManipulatorTranslate::DragEnd(v2 vDragStart, v2 vDragEnd)
{

}



void ManipulatorRotate::Draw()
{

}
bool ManipulatorRotate::DragBegin(v2 vDragStart, v2 vDragEnd, SObject* pSelection)
{
	m_pSelection = pSelection;
	mBase = pSelection->mObjectToWorld;
	v3 vDir = DirectionFromScreen(vDragStart, *g_WorldState.pActiveCamera);
	float fIntersect = rayboxintersect(g_WorldState.pActiveCamera->vPosition, vDir, pSelection->mObjectToWorld, pSelection->vSize);
	return fIntersect > 0.f;
}
void ManipulatorRotate::DragUpdate(v2 vDragStart, v2 vDragEnd)
{
	v2 vRot = vDragEnd - vDragStart;
	m mat = mBase;
	mat.trans = v4init(0,0,0,1);
	m mrot = mid();
	mrot.x = g_WorldState.pActiveCamera->mview.x;
	mrot.y = g_WorldState.pActiveCamera->mview.y;
	mrot.z = g_WorldState.pActiveCamera->mview.z;

	m mrotx = mrotatex(vRot.y/360 * PI);
	m mroty = mrotatey(vRot.x/360 * PI);

	m mrotinv = minverse(mrot);
	m mfinal = mmult(mrotinv, mmult(mrotx, mmult(mroty, mmult(mrot, mat))));
	mfinal.trans = mBase.trans;
	m_pSelection->mObjectToWorld = mfinal;

	// v3 vDirStart = DirectionFromScreen(vDragStart, g_WorldState.Camera);
	// v3 vDirEnd = DirectionFromScreen(vDragEnd, g_WorldState.Camera);
	// v3 vNewPoint = g_WorldState.pActiveCamera->vPosition + vDirEnd * fBaseDist;
	// v3 vDelta = vNewPoint - vBasePoint;
	// m_pSelection->mObjectToWorld.trans = v4init(mBase.trans.tov3() + vDelta, 1.f);
	// m id = mid();
	// v3 vSize = v3init(0.5f, 0.5f, 0.5f);
}
void ManipulatorRotate::DragEnd(v2 vDragStart, v2 vDragEnd)
{

}