#pragma once 

#include "CoreMinimal.h"



class FSocket;


class FConnection
{
public:
	enum
	{
		RESULT_FAIL,
		RESULT_NODATA,
		RESULT_OK
	};

public:
	void Close();
	//bool Send(const FMessage& Msg);
	//bool Receive(TArray<FMessage>& Messages);
	//bool Receive(FMessage& Messages);
	bool Send(const void* Buffer, int Size);
	int TryReceive(void* Buffer, int Size);
	int Receive(void* Buffer, int Size);

	FConnection(FSocket* InSocket);
	~FConnection();

	static FSocket* CreateListenSocket(int Port, const TCHAR* DebugName);
	static TSharedPtr<FConnection> ConnectToHost(const TCHAR* Ip, int Port,float Timeout, const TCHAR* SocketDebugName);
	static TSharedPtr<FConnection> AcceptConnection(FSocket* Socket, float Timeout = MAX_FLT);
	static void CloseSocket(FSocket* Socket);
private:
	TFunction<void()> Receiver;
	FSocket* Socket;
};