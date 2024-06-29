#include "NetChannel.h"
#include "Connection.h"

const static int MSG_HEADER_MAGIC_NUM = 0xc00c;

void FNetChannel::Accept(TSharedPtr < FConnection> Conn)
{
	check(!Connection);
	Connection = Conn;
}

bool FNetChannel::Connect(const TCHAR* Ip, int Port, float Timeout)
{
	check(!Connection);
	Connection = FConnection::ConnectToHost(Ip,Port, Timeout, TEXT("Client"));
	return Connection.IsValid();
}

void FNetChannel::Connect(TSharedPtr<FConnection> Conn)
{
	check(!Connection);
	Connection = Conn;
}

bool FNetChannel::SendMessage(FMessage&& Msg)
{
	auto& Header = Msg.GetHeader();
	if (!Connection->Send(&Header, sizeof(Header)))
		return false;
	if (!Connection->Send(Msg.GetData(), Msg.Num()))
		return false;

	return true;
}

void FNetChannel::RegisterRequestEvent(int Id, FRequestEvent&& OnRequest)
{
	check(!Router.Contains(Id));
	Router.Add(Id, MoveTemp(OnRequest));
}

bool FNetChannel::ReceiveMessages(TArray<FMessage>& Messages)
{
	while (true)
	{
		FMessage Msg;
		auto& Header = Msg.GetHeader();
		auto Ret = Connection->TryReceive(&Header, sizeof(Header));
		if (Ret == FConnection::RESULT_FAIL)
		{
			return false ;
		}

		if (Ret == FConnection::RESULT_NODATA)
		{
			return true;
		}

		check(Header.Magic == MSG_HEADER_MAGIC_NUM);

		if (Header.Len != 0)
		{
			TArray<uint8> Data;
			Data.SetNumUninitialized(Header.Len);

			while (true)
			{
				Ret = Connection->TryReceive(Data.GetData(), Header.Len);
				if (Ret == FConnection::RESULT_FAIL)
					return false;

				if (Ret == FConnection::RESULT_OK)
					break;
			}

			Msg.SetData(MoveTemp(Data));
		}
		Messages.Emplace(MoveTemp(Msg));
	}
	return true;
}

void FNetChannel::Tick()
{
	if (!Connection)
		return;
	TArray<FMessage> Msgs;
	ReceiveMessages(Msgs);

	for (auto& Msg : Msgs)
	{
		check(Router.Contains(Msg.GetId()))
		auto& Receiver = Router[Msg.GetId()];
		Receiver(MoveTemp(Msg));
	}
}

bool FNetChannel::ReceiveMessage(FMessage& Msg)
{
	auto& Header = Msg.GetHeader();
	auto Ret = Connection->Receive(&Header, sizeof(Header));
	if (Ret == FConnection::RESULT_FAIL)
	{
		return false;
	}

	check(Header.Magic == MSG_HEADER_MAGIC_NUM);
	TArray<uint8> Data;
	Data.SetNumUninitialized(Header.Len);
	if (Header.Len == 0)
		return true;

	Ret = Connection->Receive(Data.GetData(), Header.Len);
	if (Ret == FConnection::RESULT_FAIL)
		return false;

	Msg.SetData(MoveTemp(Data));
	return true;
}

bool FNetChannel::IsConnected()const
{
	return Connection.IsValid();
}