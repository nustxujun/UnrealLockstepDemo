#include "Connection.h"
#include "Sockets.h"
#include "SocketSubsystem.h"


FSocket* FConnection::CreateListenSocket(int32 Port,const TCHAR* SocketDebugName)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
	TSharedPtr<FInternetAddr> OutAddr = SocketSubsystem->GetLocalBindAddr(*GLog);

	OutAddr->SetPort(Port);
	FSocket* LocalSocket = SocketSubsystem->CreateSocket(NAME_Stream, SocketDebugName, OutAddr->GetProtocolType());

	checkf(LocalSocket, TEXT("Could not create listen socket"));

	LocalSocket->SetReuseAddr(false);
	LocalSocket->SetNoDelay(true);
	LocalSocket->SetNonBlocking(true);

	auto Ret = LocalSocket->Bind(*OutAddr);
	checkf(Ret, TEXT("%s"), *FString::Printf(TEXT("Failed to bind listen socket %s"), *OutAddr->ToString(true /* bAppendPort */)));
	
	int32 MaxBacklog = 16;
	Ret = LocalSocket->Listen(MaxBacklog);
	checkf(Ret, TEXT("%s"), *FString::Printf(TEXT("Failed to listen to socket %s"), *OutAddr->ToString(true /* bAppendPort */)));

	OutAddr->SetPort(Port);

	return LocalSocket;
}

TSharedPtr<FConnection> FConnection::ConnectToHost(const TCHAR* Ip, int Port, float Timeout, const TCHAR* SocketDebugName)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
	TSharedPtr<FInternetAddr> HostAddr = SocketSubsystem->GetAddressFromString(Ip);
	HostAddr->SetPort(Port);

	auto Begin = FPlatformTime::Seconds();
	bool bReady = false;
	FSocket* Socket ;
	while(true)
	{
		Socket = SocketSubsystem->CreateSocket(NAME_Stream, SocketDebugName, HostAddr->GetProtocolType());
		check(Socket);
		Socket->SetNonBlocking(true);

		auto Ret = Socket->Connect(*HostAddr);
		check(Ret);

		bReady = Socket->Wait(ESocketWaitConditions::WaitForReadOrWrite, FTimespan::FromSeconds(3.0f));
		if (bReady || (FPlatformTime::Seconds() - Begin) > Timeout)
			break;

		CloseSocket(Socket);
		Socket = nullptr;
	}


	return MakeShared<FConnection>(Socket);
}

TSharedPtr<FConnection> FConnection::AcceptConnection(FSocket* Socket, float Timeout)
{
	bool bReady;
	if (!Socket->WaitForPendingConnection(bReady, FTimespan::FromSeconds(Timeout)) || !bReady)
		return nullptr;

	auto Client = Socket->Accept(TEXT("AcceptClient"));
	if (!Client)
		return nullptr;

	return MakeShared<FConnection>(Client);
}


void FConnection::CloseSocket(FSocket* Socket)
{
	Socket->Close();
	ISocketSubsystem::Get()->DestroySocket(Socket);
}



FConnection::FConnection(FSocket* InSocket):
	Socket(InSocket)
{

}

FConnection::~FConnection()
{
	Close();
}

void FConnection::Close()
{
	if (Socket)
	{
		CloseSocket(Socket);
		Socket = nullptr;
	}
}


struct LimitCounter
{
	float End;
	LimitCounter(float Sec)
	{
		End = FPlatformTime::Seconds() + Sec;
	}

	bool Tick()
	{
		return FPlatformTime::Seconds() < End;
	}
};

bool FConnection::Send(const void* Buffer, int Size)
{
	LimitCounter Counter(300.0f);
	int32 TotalSent = 0;
	while (TotalSent < Size)
	{
		int BytesWrite;
		auto Ret = Socket->Send((const uint8*)Buffer + TotalSent, Size - TotalSent, BytesWrite);
		if (Ret)
		{ 
			TotalSent += BytesWrite;
		}
		else
		{
			if (Socket->GetConnectionState() == SCS_ConnectionError)
			{
				return false;
			}
			FPlatformProcess::Sleep(1);
		}

		FPlatformProcess::Sleep(0);
		
		checkf(Counter.Tick(), TEXT("sending msg is timeout"));
	}
	return true;
}



int FConnection::TryReceive(void* Buffer, int Size)
{
	LimitCounter Counter(300.0f);
	int TotalRead = 0;
	while(true)
	{
		int BytesRead = 0;
		bool Ret = Socket->Recv((uint8*)Buffer + TotalRead, Size - TotalRead, BytesRead);
		TotalRead += BytesRead;

		if (!Ret)
		{
			return RESULT_FAIL;
		}

		if (TotalRead == 0)
		{
			return RESULT_NODATA;
		}

		if (TotalRead == Size)
			return RESULT_OK;

		FPlatformProcess::Sleep(0);
		checkf(Counter.Tick(), TEXT("receiving msg is timeout"));
	}

	return RESULT_OK;
}

int FConnection::Receive(void* Buffer, int Size)
{
	while (true)
	{
		auto Ret = TryReceive(Buffer, Size);
		if (Ret != RESULT_NODATA)
			return Ret;
	}
}




