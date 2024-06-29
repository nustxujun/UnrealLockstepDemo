// Copyright Epic Games, Inc. All Rights Reserved.

#include "Demo.h"
#include "Modules/ModuleManager.h"
#include "Rp3dSystem.h"
#include "ArxServerSubsystem.h"

#pragma optimize("",off)
class FDemoModule : public FDefaultGameModuleImpl
{
    virtual void StartupModule() override
    {
        FWorldDelegates::OnPostWorldCreation.AddLambda([](UWorld* World){
            if (World->HasAnyFlags(RF_ClassDefaultObject))
                return;

#if WITH_SERVER_CODE
            auto NetMode = World->GetNetMode();
            if (NetMode != NM_Client)
            {
                return;
            }
            else
#endif
            {
                FRp3dWorldSettings Settings;
                Settings.bAutoUpdate = false;
                URp3dSystem::Get().CreateWorld(World, MoveTemp(Settings));
            }
        });

        FWorldDelegates::OnPostWorldCleanup.AddLambda([](auto World, auto,auto){
            URp3dSystem::Get().DestroyWorld(World);
        });

#if WITH_SERVER_CODE
        FWorldDelegates::OnPostWorldInitialization.AddLambda([](auto World, auto){
            auto NetMode = World->GetNetMode();
            if ((World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE) && NetMode != NM_Client )
            {
                auto& Server = UArxServerSubsystem::Get(World);
                Server.Start(1.0 / 30.0f);
            }
        });
#endif
    }
};
#pragma optimize("",on)

IMPLEMENT_PRIMARY_GAME_MODULE(FDemoModule, Demo, "Demo" );
 
