#pragma once 

#include "ArxGamePlayCommon.h"
#include "ArxEntity.h"
#include "ArxCommandSystem.h"
#include "CoreMinimal.h"

class Ball : public ArxEntity, public ArxEntityRegister<Ball>
{
	GENERATED_ARX_ENTITY_BODY()

public:
	Ball(ArxWorld& World, ArxEntityId Id);
	void Initialize(bool bIsReplicated = false) override;
	void Uninitialize(bool bIsReplicated) override;
	void Serialize(ArxSerializer& Serializer) override;
	void Spawn()override;

	void OnEvent(ArxEntityId Sender, uint64 Type, uint64 Param) override;
	void Update();

	void MoveDirectly(const Rp3dVector3& Val);
	EXPOSED_ENTITY_METHOD(Move, const Rp3dVector3& Val)


	const Rp3dTransform& GetTransform()const;
public:

	REFLECT_BEGIN()
	REFLECT_FIELD(int, Timer)
	REFLECT_FIELD(FString, CharacterBlueprint)
	REFLECT_FIELD(Rp3dVector3, Force)
	REFLECT_FIELD(bool, bDamping)

	REFLECT_END()

public:
    struct PhysicsWrapper: public FGCObject
    {
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
		URp3dRigidBody* RigidBody = nullptr;
		TSharedPtr<Rp3dCollisionShape> CollisionShape ;
		FString GetReferencerName()const{return TEXT("PhysicsBall"); }
    };

    PhysicsWrapper Wrapper;
};