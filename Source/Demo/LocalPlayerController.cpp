#include "LocalPlayerController.h"

#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"


#include "ArxWorld.h"
#include "ArxTimerSystem.h"
#include "ArxPlayer.h"
#include "ArxServerSubsystem.h"
#include "ArxPhysicsSystem.h"
#include "ArxRenderableSystem.h"

#include "Rp3dWorld.h"

#include "Logic/Ball.h"



#include <fstream>
const static FString Dir = TEXT("G:\\snapshot");


enum EMsgType
{
	Register = 1,
	Step ,
	Command,
	Snapshot,
	VerifiedFrame,
	Start
};

template<class ... Args>
void SerializeMsg(FMessage& Msg, const Args& ... args)
{
	int a[] = { (Msg << args, 0) ... };
}

void SerializeMsg(FMessage& Msg)
{
}

#define PREPARE_MSG(Id, ...) FMessage Msg; Msg.SetId(EMsgType::Id); SerializeMsg(Msg, ##__VA_ARGS__); Channel.SendMessage(MoveTemp(Msg));


class PlayerMgr : public ArxSystem, public ArxEntityRegister<PlayerMgr>, public ArxEventReceiver
{
	GENERATED_ARX_ENTITY_BODY()
public:
	TFunction<void(ArxPlayerId, APawn*)> OnCreatePlayer;

	PlayerMgr(ArxWorld& InWorld, ArxEntityId Id):
		ArxSystem(InWorld, Id)
	{

	}

	void Initialize(bool bIsReplicated) override
	{
		GetWorld().RegisterServerEvent(ArxServerEvent::PLAYER_ENTER, GetId());
		AddCallback(GetWorld(), ArxServerEvent::PLAYER_ENTER, [this](uint64 Event, uint64 Param) {
			ArxPlayerId PId = (ArxPlayerId)Param;

			CreateCharacter(PId,  FString(TEXT("/Game/Blueprints/RenderCharacter.RenderCharacter_C")));

			GetWorld().RequestAccessInGameThread([PId,this](const ArxWorld& World){
				auto Actor = GetLinkedActor(PId);
				auto Pawn = Cast<APawn>(Actor);
				if (OnCreatePlayer)
					OnCreatePlayer(PId, Pawn);
			});

		});

		if (!bIsReplicated)
		{
			const int Count =100;
			for (int i = 0; i < Count; ++i)
			{ 
				auto B = GetWorld().CreateEntity<Ball>(NON_PLAYER_CONTROL);
				B->CharacterBlueprint = TEXT("/Game/Blueprints/RenderCharacter.RenderCharacter_C");
				B->Spawn();

			}
		}
	}

	void Serialize(ArxSerializer& Serializer) override
	{
		ARX_SERIALIZE_MEMBER_FAST(EntityIds);
	}

	void OnEvent(ArxEntityId From, uint64 Event, uint64 Param) override
	{
		OnReceiveEvent(From, Event, Param);
	}

	void CreateCharacter(ArxPlayerId PId, FString ClassPath)
	{
		auto Ent = GetWorld().CreateEntity<Ball>(PId);
		auto Chara = static_cast<Ball*>(Ent);
		Chara->bDamping = true;
		Chara->CharacterBlueprint = ClassPath;

		Chara->Spawn();
		auto EId = EntityIds.FindRef(PId);
		if (EId != 0)
		{
			GetWorld().DestroyEntity(EId);
		}

		EntityIds.Add(PId, Ent->GetId());
	}

	TWeakObjectPtr<AActor> GetLinkedActor(ArxPlayerId PId)
	{
		auto EId = EntityIds.FindRef(PId);
		if (EId == 0)
			return nullptr;
		auto Actor = GetWorld().GetSystem<ArxRenderableSystem>().GetActor(EId);
		return Actor;
	}


private:
	TSortedMap<ArxPlayerId, ArxEntityId> EntityIds;
};

struct ClientPlayer : public ArxClientPlayer
{
	ALocalPlayerController* Controller;
	FNetChannel& Channel;
	TBitArray<> RequestBits;
	ClientPlayer(ALocalPlayerController* Ctrl, FNetChannel& InChannel) :
		ArxClientPlayer(Ctrl->GetWorld(), ArxConstants::VerificationCycle), Controller(Ctrl), Channel(InChannel)
	{

	}

	void Update() override
	{
		{
			ArxClientPlayer::Update();
		}
	}

	virtual void OnRegister(ArxWorld& InWorld)override
	{
		InWorld.AddSystem<ArxPhysicsSystem>();
		InWorld.AddSystem<ArxRenderableSystem>();
		InWorld.AddSystem<PlayerMgr>();

		InWorld.GetSystem< PlayerMgr>().OnCreatePlayer = [this](auto PId, auto Pawn){
			if (PId != Controller->Player->GetPlayerId())
				return;
			Controller->Possess(Pawn);
		};


	}

	virtual void SendCommand(int FrameId, const TArray<uint8>& Command)override
	{
		PREPARE_MSG(Command, FrameId, Command);

	}

	virtual void SendSnapshot(int FrameId, const TArray<uint8>& Snapshot)
	{
		PREPARE_MSG(Snapshot, FrameId, Snapshot);
	}

	virtual void RequestCommand(int FrameId)override
	{
		//if (FrameId >= RequestBits.Num() )
		//{
		//	auto Begin = RequestBits.Num();
		//	RequestBits.SetNumUninitialized(FrameId + 1);
		//	RequestBits.SetRange(Begin, FrameId + 1 - Begin, false);

		//}
		//if (RequestBits[FrameId])
		//	return;
		//Component->RequestCommand(FrameId);
		//RequestBits[FrameId] = true;
	}
	virtual void RequestRegister()override
	{
		PREPARE_MSG(Register);
	}
	virtual void RequestUnregister()override
	{

	}

	virtual void RequestSnapshot(int FrameId)
	{
	}

};


struct ServerPlayer : public ArxServerPlayer
{
	ALocalPlayerController* Controller;
	FNetChannel& Channel;
	ServerPlayer(ALocalPlayerController* Ctrl, FNetChannel& InChannel)
		:ArxServerPlayer(Ctrl->GetWorld()->GetSubsystem<UArxServerSubsystem>()), Controller(Ctrl),Channel(InChannel)
	{}

	virtual void SyncStep(int FrameId)override
	{
		PREPARE_MSG(Step, FrameId);
	}


	virtual void SyncStart()
	{
		PREPARE_MSG(Start);
	}


	virtual void ResponseCommand(int FrameId,  const TArray<uint8>& Command)
	{
		PREPARE_MSG(Command, FrameId, Command);
	}

	virtual void ResponseRegister(ArxPlayerId Id)
	{
		PREPARE_MSG(Register, Id);
	}

	virtual void ResponseUnregister()
	{

	}

	virtual void ResponseVerifiedFrame(int FrameId)
	{
		PREPARE_MSG(VerifiedFrame, FrameId);

	}

	virtual void ResponseSnapshot(int FrameId, const TArray<uint8>& Snapshot)
	{
		PREPARE_MSG(Snapshot, FrameId, Snapshot);

	}

};

FSocket* ALocalPlayerController::ServerSocket = nullptr;
const int Port = 12343;

ALocalPlayerController::ALocalPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanPossessWithoutAuthority = true;
	PrimaryActorTick.bCanEverTick = true;

	//Connector = CreateDefaultSubobject<UArxConnectorComponent>(TEXT("Connector"));

}

void ALocalPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// Disable killing actors outside of the world
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings(true);
	WorldSettings->bEnableWorldBoundsChecks = false;

#if WITH_SERVER_CODE
	if (IsServer())
	{
		if (!ServerSocket)
		{
			ServerSocket = FConnection::CreateListenSocket(Port, TEXT("Server"));
		}

		Player = MakeShared<ServerPlayer>(this, Channel);
		Player->Initalize();
		auto Subsystem = GetWorld()->GetSubsystem<UArxServerSubsystem>();
		Subsystem->Start((float)ArxConstants::TimeStep);


		AsyncTask(ENamedThreads::AnyThread, [Self = TWeakObjectPtr<ALocalPlayerController>(this)]() {
			FPlatformProcess::Sleep(3.0f);
			auto Conn = FConnection::AcceptConnection(ServerSocket, 10.0f);
			check(Conn);
			AsyncTask(ENamedThreads::GameThread, [Self, Conn]() {
					if (Self.IsValid() )
					{
						Self->OnConnectServerSide(Conn);
					}
				});
			});

	}
	else
#endif
	if (IsStandAlone())
	{

	}
	else
	{

		bRunning.store(true);
		Player = MakeShared<ClientPlayer>(this,Channel);
		Player->Initalize();


		bEndThread = FGenericPlatformProcess::GetSynchEventFromPool(false);
		AsyncTask(ENamedThreads::AnyThread, [Self = TWeakObjectPtr<>(this), this]() {
			auto Conn = FConnection::ConnectToHost(*NetConnection->URL.Host, Port, 10.0f, TEXT("Client"));
			if (!Conn)
			{
				bEndThread->Trigger();
				return;
			}
			OnConnectClientSide(Conn);
			Player->RequestRegister();

			while(bRunning.load())
			{
				Player->Update();
				Channel.Tick();
				FPlatformProcess::SleepNoStats(0);
			}

			bEndThread->Trigger();

		});

		//AsyncTask(ENamedThreads::AnyThread, [Self = TWeakObjectPtr<>(this), this]() {
		//	auto Conn = FConnection::ConnectToHost(*NetConnection->URL.Host, Port, 10.0f, TEXT("Client"));
		//	if (!Conn)
		//	{
		//		return;
		//	}

		//	AsyncTask(ENamedThreads::GameThread, [this, Conn](){
		//		OnConnectClientSide(Conn);
		//		Player->RequestRegister();


		//	});

		//});

	}
}

void ALocalPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bEndThread)
	{
		bRunning.store(false);
		bEndThread->Wait();
		FPlatformProcess::ReturnSynchEventToPool(bEndThread);
		bEndThread = nullptr;
	}


	Super::EndPlay(EndPlayReason);
#if WITH_SERVER_CODE
	if (IsServer())
	{
		auto Subsystem = GetWorld()->GetSubsystem<UArxServerSubsystem>();
		Subsystem->UnregisterPlayer(Player->GetPlayerId());
	}
#endif
	Player.Reset();

}

bool ALocalPlayerController::IsServer()
{
	auto Driver = GetNetDriver();
	if (!Driver)
		return false;
	auto Mode = Driver->GetNetMode();

	return Mode < NM_Client;
}

bool ALocalPlayerController::IsStandAlone()
{
	auto Driver = GetNetDriver();
	if (!Driver)
		return true;
	auto Mode = Driver->GetNetMode();

	return Mode == NM_Standalone;
}

void ALocalPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);



	if (IsStandAlone())
	{
		
		return;
	}

	if (!Player)
		return;

	if (IsServer())
	{
		Channel.Tick();
	}
	else if ( !IsServer())
	{
		//Player->Update();
		//Channel.Tick();
	}

	//if (Channel.IsConnected())
	//{
	//	for (auto& Msg : MsgQueue)
	//	{
	//		Channel.SendMessage(MoveTemp(Msg));
	//	}

	//	MsgQueue.Reset();
	//}


}




void ALocalPlayerController::OnConnectClientSide(TSharedPtr< FConnection> Conn)
{
	Channel.Connect(Conn);
	Channel.RegisterRequestEvent(EMsgType::Register, [this](auto Msg)
		{
			ArxPlayerId PId;
			Msg >> PId;
			Player->ResponseRegister(PId);
		});
	Channel.RegisterRequestEvent(EMsgType::Step, [this](auto Msg)
		{
			int FrameId;
			Msg >> FrameId;
			Player->SyncStep(FrameId);
		});

	Channel.RegisterRequestEvent(EMsgType::Command, [this](auto Msg)
		{
			int FrameId;
			Msg >> FrameId ;
			TArray<uint8> Cmds;
			Msg >> Cmds;

			Player->ResponseCommand(FrameId,  Cmds);
		});

	Channel.RegisterRequestEvent(EMsgType::Snapshot, [this](auto Msg)
		{
			int FrameId;
			Msg >> FrameId;
			TArray<uint8> Snapshot;
			Msg >> Snapshot;

			Player->ResponseSnapshot(FrameId, Snapshot);
		});

	Channel.RegisterRequestEvent(EMsgType::VerifiedFrame, [this](auto Msg)
		{
			int FrameId;
			Msg >> FrameId;
			Player->ResponseVerifiedFrame(FrameId);
		});

	Channel.RegisterRequestEvent(EMsgType::Start, [this](auto Msg)
		{
			Player->SyncStart();
		});
}


void ALocalPlayerController::OnConnectServerSide(TSharedPtr< FConnection> Conn)
{
	Channel.Accept(Conn);
	Channel.RegisterRequestEvent(EMsgType::Register, [this](auto Msg)
		{
			Player->RequestRegister();
		});
	Channel.RegisterRequestEvent(EMsgType::Command, [this](auto Msg)
		{
			int FrameId;
			Msg >> FrameId;
			TArray<uint8> Cmds;
			Msg >> Cmds;

			Player->SendCommand(FrameId, Cmds);
		});

	Channel.RegisterRequestEvent(EMsgType::Snapshot, [this](auto Msg)
		{
			int FrameId;
			Msg >> FrameId;
			TArray<uint8> Snapshot;
			Msg >> Snapshot;

			Player->SendSnapshot(FrameId, Snapshot);
		});


}

