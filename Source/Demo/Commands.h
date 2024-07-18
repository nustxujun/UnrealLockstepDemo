#pragma once 

#include "CoreMinimal.h"
#include "ArxCommandSystem.h"
#include "ArxReflection.h"
#include "ArxGamePlayCommon.h"


template<class T>
struct BasicCommand : public ArxCommand<T>
{
};

#define CMD_SERIALIZE_IMPL(Serializer) Arx::Visit(*this, [&Serializer](auto& Field, auto Name) {Serializer << Field;});
#define CMD_SERIALIZE_METHOD() void Serialize(ArxSerializer& Serializer) override{CMD_SERIALIZE_IMPL(Serializer)}


struct CreateRoleCommand:public BasicCommand<CreateRoleCommand>
{
    virtual void Serialize(ArxSerializer& Serializer) override;
    virtual void Execute(ArxEntity& Ent, ArxPlayerId) override;
};

struct KeyboardCommand :public BasicCommand<KeyboardCommand>
{
    virtual void Execute(ArxEntity& Ent, ArxPlayerId) override;
    CMD_SERIALIZE_METHOD()

    REFLECT_FIELD(int, Key)
    REFLECT_FIELD(bool, bPressed)

};