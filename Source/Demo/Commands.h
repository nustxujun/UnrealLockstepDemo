#pragma once 

#include "CoreMinimal.h"
#include "ArxCommandSystem.h"


template<class T>
struct BasicCommand : public ArxCommand<T>
{
};

struct CreateRoleCommand:public BasicCommand<CreateRoleCommand>
{
    virtual void Serialize(ArxSerializer& Serializer) override;
    virtual void Execute(ArxEntity& Ent, ArxPlayerId) override;
};