#include "LocalPlayerController.h"

#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"


#include "ArxWorld.h"
#include "ArxTimerSystem.h"
#include "ArxPlayer.h"
#include "ArxServerSubsystem.h"
#include "ArxPlayerController.h"
#include "ArxCharacter.h"
#include "ArxPhysicsSystem.h"

#include "Rp3dWorld.h"

DECLARE_CYCLE_STAT(TEXT("World Update"), STAT_WorldUpdate, STATGROUP_ArxGroup);
DECLARE_CYCLE_STAT(TEXT("Physics World Update"), STAT_PhysicsWorldUpdate, STATGROUP_ArxGroup);

#include <fstream>
const static FString Dir = TEXT("G:\\snapshot");


enum EMsgType
{
	Step = 1,
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

#define PREPARE_MSG(Id, ...) FMessage Msg; Msg.SetId(EMsgType::##Id); SerializeMsg(Msg, __VA_ARGS__); Controller->MsgQueue.Add(MoveTemp(Msg));

struct ClientPlayer : public ArxClientPlayer
{
	ALocalPlayerController* Controller;
	URp3dWorld* PhysicsWorld;
	TBitArray<> RequestBits;
	ClientPlayer(ALocalPlayerController* Ctrl) :
		ArxClientPlayer(Ctrl->GetWorld(), ArxConstants::VerificationCycle), Controller(Ctrl)
	{
		PhysicsWorld = URp3dWorld::Get(Controller->GetWorld());

		auto& FileMgr = IFileManager::Get();


		FileMgr.MakeDirectory(*Dir, true);
	}

	void Update() override
	{
		{
			SCOPE_CYCLE_COUNTER(STAT_WorldUpdate);
			ArxClientPlayer::Update();
		}
	}

	virtual void OnReceiveCommand(int FrameId, const TArray<uint8>& Cmd)
	{
		//auto Path = FString::Printf(TEXT("%s\\%d\\cmds.txt"), *Dir, GetPlayerId());
		//std::fstream f(TCHAR_TO_ANSI(*Path), std::ios::binary | std::ios::app);
		//f << FrameId << Count;
		//f << Cmd.Num();
		//f.write((char*)Cmd.GetData(), Cmd.Num());
	}



	virtual void OnFrame() override
	{
		//SCOPE_CYCLE_COUNTER(STAT_PhysicsWorldUpdate);
		//PhysicsWorld->UpdatePhysics((reactphysics3d::decimal)ArxConstants::TimeStep);
	}

	virtual void OnRegister(ArxWorld& InWorld)override
	{
		InWorld.AddSystem<ArxTimerSystem>();
		InWorld.AddSystem<ArxPlayerController>();
		InWorld.AddSystem<ArxPhysicsSystem>();


		auto& FileMgr = IFileManager::Get();
		auto Path = FString::Printf(TEXT("%s\\%d"), *Dir, GetPlayerId());

		if (FileMgr.DirectoryExists(*Path))
		{
			FileMgr.Move(*(Path + "_backup"), *Path);
			FileMgr.DeleteDirectory(*(Path + "_backup"), false, true);
		}

		FileMgr.MakeDirectory(*Path, true);


		InWorld.GetSystem<ArxPlayerController>().CreateCharacter(ArxTypeName<ArxCharacter>(), FString(TEXT("/Game/ThirdPersonCPP/Blueprints/RenderCharacter.RenderCharacter_C")));
	}

	virtual void SendHash(int FrameId, uint32 HashValue)
	{
		//Component->Channel.SendMessage()
		//Component->SendHash(FrameId, HashValue);
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
		Controller->RequestRegister();
	}
	virtual void RequestUnregister()override
	{

	}

	virtual void RequestSnapshot(int FrameId)
	{
		//Component->RequestSnapshot(FrameId);
	}

};


struct ServerPlayer : public ArxServerPlayer
{
	ALocalPlayerController* Controller;

	ServerPlayer(ALocalPlayerController* Ctrl)
		:ArxServerPlayer(Ctrl->GetWorld()->GetSubsystem<UArxServerSubsystem>()), Controller(Ctrl)
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
		Controller->ResponseRegister(Id);
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
#if WITH_SERVER_CODE
	if (IsServer())
	{
		if (!ServerSocket)
		{
			ServerSocket = FConnection::CreateListenSocket(Port, TEXT("Server"));
		}

		Player = MakeShared<ServerPlayer>(this);
		auto Subsystem = GetWorld()->GetSubsystem<UArxServerSubsystem>();
#if WITH_EDITOR
		Subsystem->Start((float)ArxConstants::TimeStep);
#endif

	}
	else
#endif
	if (IsStandAlone())
	{

	}
	else
	{
		Player = MakeShared<ClientPlayer>(this);
		Player->RequestRegister();

	}


}

void ALocalPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
	}
	else if ( !IsServer())
	{
		Player->Update();


		if (!bSetPawn)
		{
			auto& World = ((ClientPlayer*)Player.Get())->GetWorld();
			auto& ArxController = World.GetSystem<ArxPlayerController>();
			auto Actor = ArxController.GetLinkedActor(Player->GetPlayerId());
			if (Actor)
			{
				auto P = Cast<APawn>(Actor);
				if (P)
				{
					bSetPawn = true;
					Possess(P);
				}
			}
		}

	}

	if (Channel.IsConnected())
	{
		for (auto& Msg : MsgQueue)
		{
			Channel.SendMessage(MoveTemp(Msg));
		}

		MsgQueue.Reset();
	}

	Channel.Tick();

}




void ALocalPlayerController::OnConnectClientSide(TSharedPtr< FConnection> Conn)
{
	Channel.Connect(Conn);

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
			bSetPawn = false;
			Player->SyncStart();
		});
}


void ALocalPlayerController::ResponseRegister_Implementation(uint32 Id)
{
	if (Player)
	{
		Player->ResponseRegister(Id);
	}


	AsyncTask(ENamedThreads::AnyThread, [Self = TWeakObjectPtr<>(this), this]() {
		auto Connection = FConnection::ConnectToHost(TEXT("127.0.0.7"), Port, 10.0f, TEXT("Client"));
		AsyncTask(ENamedThreads::GameThread, [Self, Connection, this]() {
			if (Self.IsValid() && !Self->IsPendingKill())
			{
				OnConnectClientSide(Connection);
			}
			});
		});


}

void ALocalPlayerController::OnConnectServerSide(TSharedPtr< FConnection> Conn)
{
	Channel.Accept(Conn);

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

void ALocalPlayerController::RequestRegister_Implementation()
{
	if (Player)
	{
		Player->RequestRegister();
	}

	AsyncTask(ENamedThreads::AnyThread, [Self = TWeakObjectPtr<ALocalPlayerController>(this)]() {
		auto Conn = FConnection::AcceptConnection(ServerSocket, 10.0f);
		check(Conn);
		AsyncTask(ENamedThreads::GameThread, [Self, Conn]() {
			if (Self.IsValid() && !Self->IsPendingKill())
			{
				Self->OnConnectServerSide(Conn);
			}
			});
		});

}

