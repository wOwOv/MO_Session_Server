#ifndef __CORECLIENT__
#define __CORECLIENT__

#include <winsock2.h>
#include "RingBuffer.h"
#include "SerialBuffer.h"
#include "CPacket.h"

#define LANCLIENT 2
#define NETCLIENT 5

class CoreClient
{
private:
	struct LanHEADER
	{
		unsigned short len;
	};
#pragma pack(push,1)
	struct NetHEADER
	{
		unsigned char code;
		unsigned short len;
		unsigned char randkey;
		unsigned char checksum;
	};
#pragma pack(pop)
	struct myOverlapped
	{
		WSAOVERLAPPED overlapped;
		bool type;				//0이면 recv 1이면 send
	};

	struct IOBOX
	{
		myOverlapped sendio;
		myOverlapped recvio;

		RingBuffer RecvQ;
		RingBuffer SendQ;

		ULONGLONG sendtime = 0;

		long sendflag = 0;
	};

public:

	CoreClient(unsigned char type = LANCLIENT);
	~CoreClient();

	bool Start(const char* txtname, char code = 0, char key = 0);
	void Stop();
	bool Disconnect();
	bool SendPacket(CPacket packet);


	int GetRecvMessageTPS();
	int GetSendMessageTPS();
	unsigned long GetRecvBuf();
	unsigned long GetSendBuf();

private:
	virtual void OnConnect() = 0;
	virtual void OnRelease() = 0;
	virtual void OnMessage(CPacket packet) = 0;
	virtual void OnSecond() = 0;
	//virtual void OnError(int errorcode, wchar* stringbox) = 0;

private:
	//Start가 쓰는 함수들
	void Parsing(const char* txtname);
	int Setting(char code, char key);
	int Network();
	///////////////////////////////////

	static unsigned __stdcall WorkerThread(LPVOID arg);
	static unsigned __stdcall MonitorThread(LPVOID arg);
	static unsigned __stdcall HeartbeatThread(LPVOID arg); //하트비트를 위해 시간이 지나기 전에 메시지 보내기

	//WorkerThread가 쓰는 함수들
	bool Release();
	void RecvCompletion(DWORD cbTransferred);
	bool DoRecv();
	void SendCompletion(DWORD cbTransferred);
	bool DoSend();
	//////////////////////////////////////////////////////////////////////

private:
	//직렬화버퍼 헤더 세팅
	void SetBufferHeader(SBuffer* msgbuf);
	//직렬화버퍼 인코딩
	void Encode(SBuffer* msg);
	//직렬화버퍼 디코딩
	bool Decode(SBuffer* msg);

private:
	long RecvMessageTPS = 0;
	long recvcount = 0;
	long SendMessageTPS = 0;
	long sendcount = 0;


private:
	SOCKET Sock;
	HANDLE hcp;
	IOBOX _Session;
	HANDLE* Thread;


	unsigned char _code;
	unsigned char _fixedKey;

private:
	unsigned char _type;
	SOCKADDR_IN _serveraddr;
	int _heartbeatVal;
	int _heartbeatTime;
};


#endif