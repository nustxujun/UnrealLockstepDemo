#pragma once 

#include "CoreMinimal.h"

class FSocket;
class FConnection;

class FMessage
{
public:
	struct FHeader
	{
		int Magic = 0xc00c;
		int Len = 0;
		int Id = 0;
		bool bIsNtf = true;
	};
public:
	FMessage(TArray<uint8> InData) :Data(MoveTemp(InData))
	{
	};

	FMessage()
	{
		Data.Reserve(256);
	};

	void Read(void* Buffer, int Size)
	{
		check(Data.Num() - ReadPos >= Size);
		FMemory::Memcpy(Buffer, Data.GetData() + ReadPos, Size);
		ReadPos += Size;
	}

	void Write(const void* Buffer, int Size)
	{
		int Begin = Data.Num();
		Data.SetNumUninitialized(Begin + Size);
		FMemory::Memcpy(Data.GetData() + Begin, Buffer, Size);
	}

	template<class T>
	FMessage& operator <<(const T& Value)
	{
		Write(&Value, sizeof(Value));
		return *this;
	}

	template<class T>
	FMessage& operator <<(const TArray<T>& Value)
	{
		int Num = Value.Num();
		*this << Num;
		Write(Value.GetData(), Num * sizeof(T));
		return *this;
	}

	FMessage& operator <<(const FString& Value)
	{
		int Len = Value.Len() + 1;
		*this << Len;
		Write(::GetData(Value), Len * sizeof(TCHAR));
		return *this;
	}

	FMessage& operator <<(const TCHAR* Value)
	{
		*this << FString(Value);
		return *this;
	}

	FMessage& operator <<(const ANSICHAR* Value)
	{
		*this << FString(Value);
		return *this;
	}

	template<class T>
	FMessage& operator >>(T& Value)
	{
		Read(&Value, sizeof(Value));
		return *this;
	}

	template<class T>
	FMessage& operator >>(TArray<T>& Value)
	{
		int Num ;
		*this >> Num;
		Value.SetNum(Num);
		Read(Value.GetData(), Num * sizeof(T));
		return *this;
	}

	FMessage& operator >>(FString& Value)
	{
		int Len;
		*this >> Len;
		Value.GetCharArray().SetNum(Len);
		Read(::GetData(Value), Len * sizeof(TCHAR));
		return *this;
	}


	const uint8* GetData()const { return Data.GetData(); }
	const TArray<uint8>& GetDataArray()const {return Data;}
	void SetData(TArray<uint8>&& InData){Data = MoveTemp(InData); }
	int Num()const { return Data.Num(); }
	int TellReadPos() const{return ReadPos;}

	void SetId(int Id){Header.Id = Id;}
	int GetId(){return Header.Id;}
	void MarkAsRsp(){Header.bIsNtf = false;}
	bool bIsRsp(){return !Header.bIsNtf;}
	FHeader& GetHeader(){Header.Len = Num(); return Header; }
	
private:
	FHeader Header;

	TArray<uint8> Data;
	int ReadPos = 0;

};


class FNetChannel
{
public:
	using FResponseEvent = TFunction<void(FMessage&& )>;
	using FRequestEvent = TFunction<void(FMessage&&)>;
public:
	void Accept(TSharedPtr < FConnection> Conn);
	void Connect(TSharedPtr<FConnection> Conn);
	bool Connect(const TCHAR* Ip, int Port, float Timeout);

	bool SendMessage(FMessage&& Msg);
	void RegisterRequestEvent(int Id, FRequestEvent&& OnRequest);
	bool ReceiveMessage(FMessage& Msg);
	void Tick();

	bool IsConnected()const;
private:
	bool ReceiveMessages(TArray<FMessage>& Messages);
private:
	TSharedPtr<FConnection> Connection;
	TMap<int, FRequestEvent> Router;
};