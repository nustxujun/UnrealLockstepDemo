#include "Ball.h"
#include "ArxWorld.h"
#include "ArxTimerSystem.h"
#include "ArxRenderableSubsystem.h"
#include "ArxPhysicsSystem.h"

#include "Rp3dRigidBody.h"
#include "Rp3dCollisionShape.h"
#include "Rp3dUtils.h"

Ball::Ball(ArxWorld& World, ArxEntityId Id)
	:ArxEntity(World, Id)
{

}

void Ball::PhysicsWrapper::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (RigidBody)
		Collector.AddReferencedObject(RigidBody);
}

void Ball::Initialize(bool bIsReplicated)
{
	FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
	auto UnrealWorld = WorldContext->World();

	//Wrapper.CollisionShape = URp3dCollisionShape::CreateCapsuleShape(50,100);
	Wrapper.CollisionShape = Rp3dCollisionShape::CreateSphereShape(55);


	auto& PhysicsSys = GetWorld().GetSystem<ArxPhysicsSystem>();
	Wrapper.RigidBody = PhysicsSys.CreateRigidBody();
	auto Collider = Wrapper.RigidBody->AddCollisionShape(Wrapper.CollisionShape);
	Collider.SetBounciness(0.5);
	Collider.SetFriction(1);

	Wrapper.RigidBody->UpdateMassPropertiesFromColliders();
	Wrapper.RigidBody->SetIsDebugEnabled(true);


	auto Pos = FVector(-290.000000, -180.000000, 454.645386 + GetId() * 200 - 300);
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

	if (Wrapper.RigidBody)
		Wrapper.RigidBody->RemoveFromWorld();

	Wrapper.CollisionShape.Reset();

	UArxRenderableSubsystem::Get(GetWorld().GetUnrealWorld()).Unlink(this);

}


void Ball::Spawn()
{
	auto Class = LoadObject<UClass>(nullptr, *CharacterBlueprint);
	//auto Blueprint = Cast<UBlueprint>(StaticLoadObject(UObject::StaticClass(), NULL, *CharacterBlueprint));
	check(Class);
	if (Class)
		UArxRenderableSubsystem::Get(GetWorld().GetUnrealWorld()).Link(this, Class);
}

void Ball::Serialize(ArxSerializer& Serializer)
{
	if (Serializer.IsSaving())
	{
		Rp3dTransform Trans = Wrapper.RigidBody->GetTransform();
		auto LinearVel = Wrapper.RigidBody->GetLinearVelocity();
		auto AngularVel = Wrapper.RigidBody->GetAngularVelocity();
		ARX_SERIALIZE_MEMBER_FAST(Trans);
		ARX_SERIALIZE_MEMBER_FAST(LinearVel);
		ARX_SERIALIZE_MEMBER_FAST(AngularVel);
	}
	else
	{
		Rp3dTransform Trans;
		Rp3dVector3 LVel, AVel;
		Serializer << Trans << LVel << AVel;
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
	auto Vel = Wrapper.RigidBody->GetLinearVelocity();
	Vel.z = 0;

	const reactphysics3d::decimal DampingRate = 0.5f;
	Vel *= -DampingRate;
	auto Damping = Vel * Wrapper.RigidBody->GetMass() / ArxConstants::TimeStep ;
		
	const reactphysics3d::decimal ForcePower = 1000;

	Wrapper.RigidBody->ResetForce();
	Wrapper.RigidBody->ApplyForceAtCenterOfMass(Direction * ForcePower + Damping);
}

const Rp3dTransform& Ball::GetTransform()const
{
	return Wrapper.RigidBody->GetTransform();
}

void Ball::MoveDirectly(const Rp3dVector3& Dir)
{
	Direction = Dir;
}

void Ball::Move_Internal(ArxPlayerId PId, const Rp3dVector3& Dir)
{
	MoveDirectly(Dir);
}

