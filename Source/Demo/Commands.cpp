#include "Commands.h"

#include "ArxWorld.h"
#include "ArxCharacter.h"


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


void KeyboardCommand::Execute(ArxEntity& Ent, ArxPlayerId)
{
	check(Ent.GetClassName() == ArxCharacter::GetTypeName());

	auto& Chara = static_cast<ArxCharacter&>(Ent);

	//Chara.Move({1,0,0});
}