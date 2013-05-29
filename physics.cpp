#include "physics.h"
#include "btBulletDynamicsCommon.h"
#include "math.h"
#include "program.h"


struct 
{
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* World;
	btStaticPlaneShape* Ground;
} g_Physics;

void PhysicsInit()
{
	memset(&g_Physics, 0, sizeof(g_Physics));
	g_Physics.collisionConfiguration = new btDefaultCollisionConfiguration();
	g_Physics.dispatcher = new	btCollisionDispatcher(g_Physics.collisionConfiguration);
	g_Physics.overlappingPairCache = new btDbvtBroadphase();
	g_Physics.solver = new btSequentialImpulseConstraintSolver;
	g_Physics.World = new btDiscreteDynamicsWorld(g_Physics.dispatcher,g_Physics.overlappingPairCache,g_Physics.solver,g_Physics.collisionConfiguration);
	g_Physics.World->setGravity(btVector3(0,-10,0));

	g_Physics.Ground = new btStaticPlaneShape(btVector3(0,1,0), -4);
	btVector3 localInertia(0,0,0);
	btScalar mass(0.);
	btTransform groundTransform;
	groundTransform.setIdentity();
	btDefaultMotionState* pMotionState = new btDefaultMotionState(groundTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,pMotionState,g_Physics.Ground,localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	g_Physics.World->addRigidBody(body);


}
void PhysicsStep()
{
	g_Physics.World->stepSimulation(1.f/60.f,10);

	const int nNumObjects = g_Physics.World->getNumCollisionObjects();
	for(int i = 0; i < nNumObjects; ++i)
	{
		btCollisionObject*	pCollisionObject = g_Physics.World->getCollisionObjectArray()[i];
		btRigidBody* pBody = btRigidBody::upcast(pCollisionObject);
		if(pBody && pBody->getMotionState())
		{
			btDefaultMotionState* pMotionState = (btDefaultMotionState*)pBody->getMotionState();
			SWorldObject* pWorldObject = (SWorldObject*)pMotionState->m_userPointer;
			if(pWorldObject)
			{
				pMotionState->m_graphicsWorldTrans.getOpenGLMatrix((float*)&pWorldObject->mObjectToWorld);
			}

		}
	}
}

void PhysicsAddObjectBox(SWorldObject* pObject)
{
	btBoxShape* pShape = new btBoxShape(btVector3(pObject->vSize.x, pObject->vSize.y, pObject->vSize.z));
	btVector3 localInertia(0,0,0);
	btScalar mass(1.f);
	pShape->calculateLocalInertia(mass,localInertia);
	btTransform startTransform;
	startTransform.setIdentity();
	m mat = pObject->mObjectToWorld;
	startTransform.setOrigin(btVector3(mat.trans.x,mat.trans.y,mat.trans.z));
	btDefaultMotionState* pMotionState = new btDefaultMotionState(startTransform);
	pMotionState->m_userPointer = pObject;
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, pMotionState, pShape, localInertia);	
	btRigidBody* body = new btRigidBody(rbInfo);
	g_Physics.World->addRigidBody(body);

}