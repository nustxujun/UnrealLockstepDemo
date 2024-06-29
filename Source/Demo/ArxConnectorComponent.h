#pragma once 
#include "CoreMinimal.h"
#include "ArxCommon.h"
#include "Network/Connection.h"
#include "Network/NetChannel.h"

class ArxWorld;

#include "ArxConnectorComponent.generated.h"

UCLASS(Blueprintable, BlueprintType)
class UArxConnectorComponent : public UActorComponent
{
    GENERATED_UCLASS_BODY()
public :
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


    //UFUNCTION(Server, Unreliable)
    //void SendHash(int FrameId, uint32 Hash);

    //UFUNCTION(Server, Unreliable)
    //void SendCommands(int FrameId, const TArray<uint8>& Data);

    //UFUNCTION(Server, Unreliable)
    //void SendSnapshot(int FrameId, const TArray<uint8>& Data);

    //UFUNCTION(Client, Unreliable)
    //void ResponseCommand(int FrameId,int Count, const TArray<uint8>& Data);

    //UFUNCTION(Server, Unreliable)
    //void RequestCommand(int FrameId);

    //UFUNCTION(Client, Reliable)
    //void ResponseRegister(uint32 Id);

    //UFUNCTION(Server, Reliable)
    //void RequestRegister();

    //UFUNCTION(Server, Unreliable)
    //void RequestSnapshot(int FrameId);


    //UFUNCTION(Client, Unreliable)
    //void SyncStep(int FrameId);

    //UFUNCTION(Client, Reliable)
    //void SyncStart();

    //UFUNCTION(Client, Unreliable)
    //void ResponseVerifiedFrame(int FrameId);

    //UFUNCTION(Client, Unreliable)
    //void ResponseSnapshot(int FrameId, const TArray<uint8>& Snapshot);


    UFUNCTION(Client, Reliable)
    void ResponseRegister(uint32 Id);

    UFUNCTION(Server, Reliable)
    void RequestRegister();

    void OnConnectServerSide(TSharedPtr<class FConnection> Conn);
    void OnConnectClientSide(TSharedPtr<class FConnection> Conn);


    bool IsStandAlone();
    bool IsServer();
    
    TSharedPtr<class ArxPlayerChannel> Player;
    FNetChannel Channel;
    TArray<FMessage> MsgQueue;


    static FSocket* ServerSocket;
};