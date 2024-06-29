#include "ArxConnectorComponent.h"
#include "Commands.h"

#include "ArxWorld.h"
#include "ArxTimerSystem.h"
#include "ArxPlayer.h"
#include "ArxServerSubsystem.h"
#include "ArxRenderActorSubsystem.h"
#include "Rp3dWorld.h"

#include "Network/Connection.h"

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

#define PREPARE_MSG(Id, ...) FMessage Msg; Msg.SetId(EMsgType::##Id); SerializeMsg(Msg, __VA_ARGS__); Component->MsgQueue.Add(MoveTemp(Msg));

struct ClientPlayer : public ArxClientPlayer
{
	UArxConnectorComponent* Component;
	URp3dWorld* PhysicsWorld;
	TBitArray<> RequestBits;
	ClientPlayer(UArxConnectorComponent* Comp):
		ArxClientPlayer(Comp->GetWorld(),ArxConstants::VerificationCycle),Component(Comp)
	{
		PhysicsWorld = URp3dWorld::Get(Component->GetWorld());

		auto& FileMgr = IFileManager::Get();


		FileMgr.MakeDirectory(*Dir, true);
	}

	void Update() override
	{
		{
			SCOPE_CYCLE_COUNTER(STAT_WorldUpdate);
			ArxClientPlayer::Update();
		}
		PhysicsWorld->DrawDebug();
	}

	virtual void CreateSnapshot(TArray<uint8>& Data)
	{
		ArxClientPlayer::CreateSnapshot(Data);

		TArray<uint8> DebugData;
		ArxDebugSerializer Ser(DebugData);
		GetWorld().Serialize(Ser);
		auto Path = FString::Printf(TEXT("%s\\%d\\%d.txt"), *Dir, GetPlayerId(), GetCurrentFrameId());

		std::fstream f(TCHAR_TO_ANSI(*Path), std::ios::binary | std::ios::out);
		f.write((char*)DebugData.GetData(), DebugData.Num());

	}

	virtual void OnFrame() override
	{
		SCOPE_CYCLE_COUNTER(STAT_PhysicsWorldUpdate);
		PhysicsWorld->UpdatePhysics(ArxConstants::TimeStep);
	}

	virtual void OnRegister(ArxWorld& InWorld)override
	{
		InWorld.AddSystem<ArxTimerSystem>();

		auto& ComSys = InWorld.GetSystem<ArxCommandSystem>();

		CreateRoleCommand Cmd;
		ComSys.SendCommand(InWorld.GetId(), Cmd);



		auto& FileMgr = IFileManager::Get();
		auto Path = FString::Printf(TEXT("%s\\%d"), *Dir, GetPlayerId());

		if (FileMgr.DirectoryExists(*Path))
		{
			FileMgr.Move(*(Path + "_backup"),*Path);
			FileMgr.DeleteDirectory(*(Path + "_backup"), false, true);
		}

		FileMgr.MakeDirectory(*Path, true);

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
		Component->RequestRegister();
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
	UArxConnectorComponent* Component;

	ServerPlayer(UArxConnectorComponent* Comp)
	:ArxServerPlayer(Comp->GetWorld()->GetSubsystem<UArxServerSubsystem>()),Component(Comp)
	{}

	virtual void SyncStep(int FrameId)override
	{
		PREPARE_MSG(Step, FrameId);
	}


	virtual void SyncStart()
	{
		PREPARE_MSG(Start);
	}


	virtual void ResponseCommand(int FrameId, int Count, const TArray<uint8>& Command)
	{
		PREPARE_MSG(Command, FrameId,Count, Command);
	}

	virtual void ResponseRegister(ArxPlayerId Id)
	{
		Component->ResponseRegister(Id);
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

FSocket* UArxConnectorComponent::ServerSocket = nullptr;
const int Port = 12343;
UArxConnectorComponent::UArxConnectorComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{

    PrimaryComponentTick.bCanEverTick = true;
}
#pragma optimize("",off)
void UArxConnectorComponent::BeginPlay()
{
    Super::BeginPlay();
	auto Owner = GetOwner();
#if WITH_SERVER_CODE
	if (IsServer())
    {
		if (!ServerSocket)
		{
			ServerSocket = FConnection::CreateListenSocket(Port, TEXT("Server"));
		}

		auto Subsystem = GetWorld()->GetSubsystem<UArxServerSubsystem>();
		Player = MakeShared<ServerPlayer>(this);

    }
    else 
#endif
    {
		Player = MakeShared<ClientPlayer>(this);
		Player->RequestRegister();
    }

}
#pragma optimize("",on)


void UArxConnectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

bool UArxConnectorComponent::IsStandAlone()
{
	auto Owner = GetOwner();
	auto Driver = Owner->GetNetDriver();
	return Driver == nullptr;
}

bool UArxConnectorComponent::IsServer()
{
	auto Owner = GetOwner();
	auto Driver = Owner->GetNetDriver();
	if (!Driver)
		return false;
    auto Mode = Driver->GetNetMode();

    return Mode < NM_Client;
}

void UArxConnectorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) 
{

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsStandAlone() || IsServer() )
	{
	}
	else if (IsStandAlone() || !IsServer())
	//else if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		Player->Update();

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



//void UArxConnectorComponent::SendHash_Implementation(int FrameId, uint32 Hash)
//{
//	if (Player)
//	{
//		Player->SendHash(FrameId, Hash);
//	}
//}
//
//void UArxConnectorComponent::SendCommands_Implementation(int FrameId,const TArray<uint8>& Data)
//{
//	if (Player)
//	{
//		Player->SendCommand(FrameId, Data);
//	}
//}
//
//void UArxConnectorComponent::SendSnapshot_Implementation(int FrameId, const TArray<uint8>& Data)
//{
//	if (Player)
//	{
//		Player->SendSnapshot(FrameId, Data);
//	}
//}
//
//void UArxConnectorComponent::ResponseCommand_Implementation(int FrameId,int Count, const TArray<uint8>& Data)
//{
//	if (Player)
//	{
//		Player->ResponseCommand(FrameId, Count, Data);
//	}
//
//}
//
//void UArxConnectorComponent::RequestCommand_Implementation(int FrameId)
//{
//	if (Player)
//	{
//		Player->RequestCommand(FrameId);
//	}
//
//}

void UArxConnectorComponent::OnConnectClientSide(TSharedPtr< FConnection> Conn)
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
		int FrameId, Count;
		Msg >> FrameId >> Count;
		TArray<uint8> Cmds;
		Msg >> Cmds;

		Player->ResponseCommand(FrameId, Count, Cmds);
	});

	Channel.RegisterRequestEvent(EMsgType::Snapshot, [this](auto Msg)
	{
		int FrameId;
		Msg >> FrameId ;
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


void UArxConnectorComponent::ResponseRegister_Implementation(uint32 Id)
{
	if (Player)
	{
		Player->ResponseRegister(Id);
	}


	AsyncTask(ENamedThreads::AnyThread,[Self = TWeakObjectPtr<>(this), this](){
		auto Connection = FConnection::ConnectToHost(TEXT("127.0.0.7"), Port, 10.0f, TEXT("Client"));
		AsyncTask(ENamedThreads::GameThread, [Self, Connection, this](){
			if (Self.IsValid() && !Self->IsPendingKill())
			{
				OnConnectClientSide(Connection);
			}
		});
	});


}

void UArxConnectorComponent::OnConnectServerSide(TSharedPtr< FConnection> Conn)
{
	Channel.Accept(Conn);

	Channel.RegisterRequestEvent(EMsgType::Command, [this](auto Msg)
		{
			int FrameId;
			Msg >> FrameId ;
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

			Player->SendSnapshot(FrameId,Snapshot);
		});


}

void UArxConnectorComponent::RequestRegister_Implementation()
{
	if (Player)
	{
		Player->RequestRegister();
	}

	AsyncTask(ENamedThreads::AnyThread, [Self = TWeakObjectPtr<UArxConnectorComponent>(this)]() {
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

//void UArxConnectorComponent::RequestSnapshot_Implementation(int FrameId)
//{
//	if (Player)
//	{
//		Player->RequestSnapshot(FrameId);
//	}
//}
//
//void UArxConnectorComponent::SyncStep_Implementation(int FrameId)
//{
//	if (Player)
//	{
//		Player->SyncStep(FrameId);
//	}
//}
//
//void UArxConnectorComponent::SyncStart_Implementation()
//{
//	if (Player)
//	{
//		Player->SyncStart();
//	}
//}
//
//void UArxConnectorComponent::ResponseVerifiedFrame_Implementation(int FrameId)
//{
//	if (Player)
//	{
//		Player->ResponseVerifiedFrame(FrameId);
//	}
//}
//
//void UArxConnectorComponent::ResponseSnapshot_Implementation(int FrameId, const TArray<uint8>& Snapshot)
//{
//	if (Player)
//	{
//		Player->ResponseSnapshot(FrameId, Snapshot);
//	}
//}
