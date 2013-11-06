#pragma once
#include "base.h"

struct SWorldObject;


void PhysicsInit();
void PhysicsStep();

void PhysicsAddObjectBox(SWorldObject* pObject);