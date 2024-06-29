#include "Commands.h"

#include "ArxWorld.h"
#include "ArxCharacter.h"
#include "ArxRenderActor.h"
#include "ArxRenderActorSubsystem.h"

#pragma optimize("",off)

void CreateRoleCommand::Serialize(ArxSerializer& Serializer)
{
}

void CreateRoleCommand::Execute(ArxEntity& Ent, ArxPlayerId PlayerId)
{
    auto& World = Ent.GetWorld();


	UE_LOG(LogCore, Error, TEXT("%s"),*ArxCharacter::GetTypeName().ToString());
    //for (int i = 0; i < 30; ++i)
    auto Character = World.CreateEntity<ArxCharacter>(PlayerId);
	
	
	Character->CharacterBlueprint =TEXT("/Game/ThirdPersonCPP/Blueprints/RenderCharacter.RenderCharacter_C");

	Character->Spawn();

}

#pragma optimize("",on)


#include "Rp3dSystem.h"

#pragma optimize("",off)
auto test = []() {
	FWorldDelegates::OnPreWorldInitialization.AddLambda([](auto World, auto) {
		FRp3dWorldSettings Settings;
		Settings.bAutoUpdate = false;
		URp3dSystem::Get().CreateWorld(World, Settings);
		});
	return 0;
}();
#pragma optimize("",on)
