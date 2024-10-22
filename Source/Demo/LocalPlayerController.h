#pragma once 
#include "CoreMinimal.h"
#include "Network/Connection.h"
#include "Network/NetChannel.h"
#include <atomic>
#include "LocalPlayerController.generated.h"


UCLASS(Blueprintable, BlueprintType)
class ALocalPlayerController : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:

    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    void OnConnectServerSide(TSharedPtr<class FConnection> Conn);
    void OnConnectClientSide(TSharedPtr<class FConnection> Conn);


    bool IsStandAlone();
    bool IsServer();

    TSharedPtr<class ArxPlayerChannel> Player;
    FNetChannel Channel;
    TArray<FMessage> MsgQueue;


    static FSocket* ServerSocket;

    FEvent* bEndThread = nullptr;

    std::atomic_bool bRunning;

private:
};