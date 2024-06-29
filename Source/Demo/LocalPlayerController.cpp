#include "LocalPlayerController.h"
#include "ArxConnectorComponent.h"


ALocalPlayerController::ALocalPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanPossessWithoutAuthority = true;


	Connector = CreateDefaultSubobject<UArxConnectorComponent>(TEXT("Connector"));

}

void ALocalPlayerController::PossessLocal(APawn* InPawn)
{
	Possess(InPawn);
}