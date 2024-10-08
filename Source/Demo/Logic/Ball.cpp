#include "Ball.h"
#include "ArxWorld.h"
#include "ArxTimerSystem.h"
#include "ArxRenderableSystem.h"
#include "ArxPhysicsSystem.h"

#include "Rp3dRigidBody.h"
#include "Rp3dCollisionShape.h"
#include "Rp3dUtils.h"

Ball::Ball(ArxWorld& World, ArxEntityId Id)
	:ArxEntity(World, Id)
{

}

void Ball::Initialize(bool bIsReplicated)
{
	FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
	auto UnrealWorld = WorldContext->World();

	Wrapper.CollisionShape = Rp3dCollisionShape::CreateSphereShape(55);


	auto& PhysicsSys = GetWorld().GetSystem<ArxPhysicsSystem>();
	Wrapper.RigidBody = PhysicsSys.CreateRigidBody();
	auto Collider = Wrapper.RigidBody->AddCollisionShape(Wrapper.CollisionShape);
	Collider.SetBounciness(0.5);
	Collider.SetFriction(1);

	Wrapper.RigidBody->UpdateMassPropertiesFromColliders();
	//Wrapper.RigidBody->SetIsDebugEnabled(true);


	auto Pos = FVector(-290.000000, -180.000000, 454.645386 + GetId()  - 300);
	auto Rot = FRotator(0, 0, 90);
	auto Trans = FTransform(Rot, Pos);
	Wrapper.RigidBody->SetTransform(UE_TO_RP3D(Trans));
	//Wrapper.RigidBody->SetAngularLockAxisFactor({0,0,0});
	//Wrapper.RigidBody->EnableGravity(false);

	if (bIsReplicated)
	{

	}
	else
	{
		auto& TimerSys = GetWorld().GetSystem<ArxTimerSystem>();
		Timer = TimerSys.AddTimer(GetId(), 0, ArxConstants::TimeStep);

	}
}

void Ball::Uninitialize(bool bIsReplicated)
{
	if (!bIsReplicated)
	{
		auto& TimerSys = GetWorld().GetSystem<ArxTimerSystem>();
		TimerSys.RemoveTimer(Timer);

	}

	Wrapper.RigidBody.Reset();
	Wrapper.CollisionShape.Reset();

	GetWorld().GetSystem<ArxRenderableSystem>().Unlink(GetId());
}


void Ball::Spawn()
{
	GetWorld().GetSystem<ArxRenderableSystem>().Link(GetId(), CharacterBlueprint);
}

void Ball::Serialize(ArxSerializer& Serializer)
{
	Rp3dTransform Trans;
	Rp3dVector3 LVel, AVel;

	if (Serializer.IsSaving())
	{
		Trans = Wrapper.RigidBody->GetTransform();
		LVel = Wrapper.RigidBody->GetLinearVelocity();
		AVel = Wrapper.RigidBody->GetAngularVelocity();

	}


	ARX_SERIALIZE_MEMBER_FAST(Trans);
	ARX_SERIALIZE_MEMBER_FAST(LVel);
	ARX_SERIALIZE_MEMBER_FAST(AVel);

	if (!Serializer.IsSaving())
	{
		Wrapper.RigidBody->SetTransform(Trans);
		Wrapper.RigidBody->SetLinearVelocity(LVel);
		Wrapper.RigidBody->SetAngularVelocity(AVel);
	}

	ARX_ENTITY_SERIALIZE_IMPL(Serializer);
}


void Ball::OnEvent(ArxEntityId Sender, uint64 Type, uint64 Param)
{
	if (GetWorld().GetSystem<ArxTimerSystem>().GetId() == Sender && Type == ArxTimerSystem::EVENT_ON_TIMER)
	{
		Update();
	}
}

void Ball::Update()
{
	if (!bDamping)
		return;
	auto Vel = Wrapper.RigidBody->GetLinearVelocity();
	Vel.z = 0;

	const reactphysics3d::decimal DampingRate = 1.0f;
	Vel *= -DampingRate;
	auto Damping = Vel * Wrapper.RigidBody->GetMass() / FPToRp3d(ArxConstants::TimeStep) ;
		
	const reactphysics3d::decimal TargetVel = 1000;
	const auto ForcePower = TargetVel * Wrapper.RigidBody->GetMass() / FPToRp3d(ArxConstants::TimeStep);

	Wrapper.RigidBody->ResetForce();
	Wrapper.RigidBody->ApplyForceAtCenterOfMass(Force + Damping);
}

const Rp3dTransform& Ball::GetTransform()const
{
	return Wrapper.RigidBody->GetTransform();
}

void Ball::MoveDirectly(const Rp3dVector3& Val)
{
	Force = Val;
}

void Ball::Move_Internal(ArxPlayerId PId, const Rp3dVector3& Val)
{
	MoveDirectly(Val);
}

