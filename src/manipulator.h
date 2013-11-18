#pragma once
#include "base.h"
#include "fwd.h"
#include "math.h"

class Manipulator
{
public:
	virtual void Draw() = 0;
	virtual bool DragBegin(v2 vDragStart, v2 vDragEnd, SObject* pSelection) = 0;
	virtual void DragUpdate(v2 vDragStart, v2 vDragEnd) = 0;
	virtual void DragEnd(v2 vDragStart, v2 vDragEnd) = 0;
};


class ManipulatorTranslate : public Manipulator
{
	m mBase;
	v3 vBasePoint;
	float fBaseDist;
	SObject* m_pSelection;
public:
	void Draw();
	bool DragBegin(v2 vDragStart, v2 vDragEnd, SObject* pSelection);
	void DragUpdate(v2 vDragStart, v2 vDragEnd);
	void DragEnd(v2 vDragStart, v2 vDragEnd);
};


class ManipulatorRotate : public Manipulator
{
	m mBase;
	SObject* m_pSelection;
public:
	void Draw();
	bool DragBegin(v2 vDragStart, v2 vDragEnd, SObject* pSelection);
	void DragUpdate(v2 vDragStart, v2 vDragEnd);
	void DragEnd(v2 vDragStart, v2 vDragEnd);
};