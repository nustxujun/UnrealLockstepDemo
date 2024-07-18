// Copyright Epic Games, Inc. All Rights Reserved.

#include "Demo.h"
#include "Modules/ModuleManager.h"
#include "Rp3dSystem.h"
#include "ArxServerSubsystem.h"
#include "ArxGamePlayCommon.h"

class FDemoModule : public FDefaultGameModuleImpl
{
    virtual void StartupModule() override
    {
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FDemoModule, Demo, "Demo" );
 
