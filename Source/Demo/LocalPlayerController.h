#pragma once 
#include "CoreMinimal.h"

#include "LocalPlayerController.generated.h"


UCLASS(Blueprintable, BlueprintType)
class ALocalPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable,Category = Pawn, meta = (Keywords = "set controller"))
	virtual void PossessLocal(APawn* InPawn) ; // DEPRECATED(4.22, "Possess is marked virtual final as you should now be overriding OnPossess instead")

private:
	UPROPERTY()
	class UArxConnectorComponent* Connector;
};