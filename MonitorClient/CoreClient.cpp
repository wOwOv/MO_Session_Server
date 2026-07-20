#define _CRT_SECURE_NO_WARNINGS
#include "CoreClient.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <process.h>
#include "CPacket.h"
#include "Parser.h"
#include "Logger.h"

CoreClient::CoreClient(unsigned char type)
{
	_type = type;
}

CoreClient::~CoreClient()
{
}

bool CoreClient::Start(const char* txtname, char code, char key)
{
	//config parsing하여 필요한 정보들 얻어오기
	Parsing(txtname);
	Setting(code, key);
	Network();

	return 1;
}
void CoreClient::Stop()
{
	int scretval;//cleanup retval
	int csretval;//closesocket retval

	csretval = closesocket(Sock);
	if (csretval == SOCKET_ERROR)
	{
		csretval = WSAGetLastError();
	}

	//워커스레드 리턴을 위한 postqueue
	ULONG_PTR p = 0;
	LPOVERLAPPED po = nullptr;
	for (int i = 0; i < 2; i++)
	{
		PostQueuedCompletionStatus(hcp, 0, p, po);
	}

	//monitorThread종료되길 대기->monitorthread는 accept스레드,워커스레드들 다 종료된 후 리턴하기에
	if (_heartbeatVal == 0)
	{
		WaitForSingleObject(Thread[2], INFINITE);
		printf("All thread ended!\n");
	}
	else
	{
		WaitForMultipleObjects(2, &Thread[2], TRUE, INFINITE);
		printf("All thread ended!\n");
	}

	//윈속종료
	scretval = WSACleanup();
	if (scretval == SOCKET_ERROR)
	{
		scretval = WSAGetLastError();
		LOG(L"System", LVSYSTEM, L"cleanup error: %d", scretval);
	}
}

bool CoreClient::Disconnect()
{
	closesocket(Sock);
	return true;
}

bool CoreClient::SendPacket(CPacket packet)
{

	SBuffer* msgbuf = packet.SBuf;
	msgbuf->AddRefcnt(1);	//내가 msgbuf쓰고 있음
	if (msgbuf->eFlag == 0)
	{
		AcquireSRWLockExclusive(&msgbuf->eKey);
		if (msgbuf->eFlag == 0)
		{
			SetBufferHeader(msgbuf);
			if (_type == NETCLIENT)
			{
				Encode(msgbuf);
			}
			InterlockedExchange8(&msgbuf->eFlag, 1);
		}
		ReleaseSRWLockExclusive(&msgbuf->eKey);
	}
	_Session.SendQ.Enqueue(msgbuf->GetReadPtr(), msgbuf->GetDataSize());

	msgbuf->DecRefcnt();	//내가 msgbuf 다 씀

	DoSend();

	return true;
}

void CoreClient::Parsing(const char* txtname)
{
	//CoreClient_Config.txt 파싱해오기

	char ip[16];
	int port;
	Parser* parse = new Parser;

	parse->LoadFile(txtname);
	parse->GetString("CoreClient_Config", "IP", ip, 16);
	parse->GetValue("CoreClient_Config", "Port", &port);

	ZeroMemory(&_serveraddr, sizeof(_serveraddr));
	_serveraddr.sin_family = AF_INET;
	int ineterror = inet_pton(AF_INET, ip, &_serveraddr.sin_addr);
	if (ineterror != 1)
	{
		printf("inet_pton error\n");
	}
	_serveraddr.sin_port = htons(port);

	parse->GetValue("CoreClient_Config", "Heartbeat", &_heartbeatVal);
	parse->GetValue("CoreClient_Config", "HeartbeatCycle", &_heartbeatTime);


	delete parse;

}
int CoreClient::Setting(char code, char key)
{
	//code, key setting
	_code = code;
	_fixedKey = key;

	int cpretval;//CreateIoCompletionProt retval

	//iocp 생성
	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//입출력 완료 포트 생성
	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, si.dwNumberOfProcessors);
	if (hcp == NULL)
	{
		cpretval = GetLastError();
		printf("CreateIoCompletionPort create error: %d\n", cpretval);
		return -1;
	}



	//스레드 생성
	if (_heartbeatVal == 0)
	{
		Thread = new HANDLE[3];
		for (int i = 0; i < 2; i++)
		{
			Thread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread, this, 0, NULL);
			if (Thread[i] == NULL)
			{
				return 1;
			}
		}

		//MonitorThread생성
		Thread[2] = (HANDLE)_beginthreadex(NULL, 0, &MonitorThread, this, 0, NULL);
		if (Thread[2] == NULL)
		{
			return 1;
		}
	}
	else
	{
		Thread = new HANDLE[4];
		for (int i = 0; i < 2; i++)
		{
			Thread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread, this, 0, NULL);
			if (Thread[i] == NULL)
			{
				return 1;
			}
		}

		//MonitorThread생성
		Thread[2] = (HANDLE)_beginthreadex(NULL, 0, &MonitorThread, this, 0, NULL);
		if (Thread[2] == NULL)
		{
			return 1;
		}

		//TimeOutThread생성
		Thread[3] = (HANDLE)_beginthreadex(NULL, 0, &HeartbeatThread, this, 0, NULL);
		if (Thread[3] == NULL)
		{
			return 1;
		}
	}

	return 0;

}
int CoreClient::Network()
{
	//retval
	int scretval;//startup
	int lscretval;//sock retval
	int sbretval;//setsockopt sndbuf0 retval
	int lgretval;//linger retval
	int ndretval;//nodelay retval
	int cnretval;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		scretval = WSAGetLastError();
		printf("WSAStartup error: %d\n", scretval);
		return -1;
	}

	//socket()
	Sock = socket(AF_INET, SOCK_STREAM, 0);
	if (Sock == INVALID_SOCKET)
	{
		lscretval = WSAGetLastError();
		printf("socket() error: %d\n", lscretval);
		return -1;
	}

	//SO_SNDBUF
	int bfoptval = 0;
	sbretval = setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, (char*)&bfoptval, sizeof(bfoptval));
	if (sbretval == SOCKET_ERROR)
	{
		sbretval = WSAGetLastError();
		printf("nonblocking error: %d\n", sbretval);
		return -1;
	}

	//SO_LINGER
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	lgretval = setsockopt(Sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (lgretval == SOCKET_ERROR)
	{
		lgretval = WSAGetLastError();
		printf("linger error: %d\n", lgretval);
		return -1;
	}

	//nodelay
	BOOL ndoptval = TRUE;
	ndretval = setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY, (char*)&ndoptval, sizeof(ndoptval));
	if (ndretval == SOCKET_ERROR)
	{
		ndretval = WSAGetLastError();
		printf("nodelay error: %d\n", ndretval);
		return -1;
	}

	//connect()
	cnretval = connect(Sock, (SOCKADDR*)&_serveraddr, sizeof(_serveraddr));
	if (cnretval == SOCKET_ERROR) {
		cnretval = WSAGetLastError();
		wprintf(L"errorcode : %d\n", cnretval);
		exit(-1);
	}

	HANDLE result = CreateIoCompletionPort(reinterpret_cast<HANDLE>(Sock),hcp,reinterpret_cast<ULONG_PTR>(&_Session),0);
	_Session.recvio.type = 0;
	_Session.sendio.type = 1;

	OnConnect();
	DoRecv();


	return 0;
}


unsigned __stdcall CoreClient::WorkerThread(LPVOID arg)
{
	CoreClient* coreclient = (CoreClient*)arg;
	int gqcsretval;//GetQueuedCompletionStatus retval;
	HANDLE iocp = coreclient->hcp;

	while (1)
	{
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred = 0;
		myOverlapped* myoverlapped = 0;
		IOBOX* tgt = 0;
		gqcsretval = GetQueuedCompletionStatus(iocp, &cbTransferred, (PULONG_PTR)&tgt, (LPOVERLAPPED*)&myoverlapped, INFINITE);

		//비동기 입출력 결과 확인
		//스레드 종료
		if (cbTransferred == 0 && tgt == 0 && myoverlapped == 0)
		{
			printf("workerThread ended\n");
			return 0;

		}


		//비동기 입츌력 실패
		if (gqcsretval == 0 || cbTransferred == 0)
		{
			if (gqcsretval == 0)
			{
				gqcsretval = WSAGetLastError();
				DWORD temp1, temp2;
				WSAGetOverlappedResult(coreclient->Sock, &myoverlapped->overlapped, &temp1, FALSE, &temp2);
				//printf("GetQueuedCompletionStatus error : %d\n", gqcsretval);
				coreclient->Stop();
			}
		}
		//비동기 입출력 성공
		else if (myoverlapped != 0)
		{

			//recvio overlapped임
			if (myoverlapped->type == 0)
			{
				coreclient->RecvCompletion(cbTransferred);
				coreclient->DoRecv();
			}
			if (myoverlapped->type == 1)
			{
				coreclient->SendCompletion(cbTransferred);
				InterlockedExchange(&tgt->sendflag, 0);
				coreclient->DoSend();
			}

		}



	}


}
unsigned __stdcall CoreClient::MonitorThread(LPVOID arg)
{
	CoreClient* coreclient = (CoreClient*)arg;

	while (1)
	{
		//워커스레드들 종료되면 모니터스레드도 리턴하기
		int waitret = WaitForMultipleObjects(2, coreclient->Thread, TRUE, 0);
		if (waitret == WAIT_OBJECT_0)
		{
			return 0;
		}

		//1초 되면 초기화
		Sleep(1000);

		coreclient->RecvMessageTPS = coreclient->recvcount;
		InterlockedExchange(&coreclient->recvcount, 0);
		coreclient->SendMessageTPS = coreclient->sendcount;
		InterlockedExchange(&coreclient->sendcount, 0);

		coreclient->OnSecond();
	}
}
unsigned __stdcall CoreClient::HeartbeatThread(LPVOID arg)
{
	CoreClient* coreclient = (CoreClient*)arg;
	DWORD timecount = 0;

	while (1)
	{
		//accpet스레드와 워커스레드들 종료되면 Timeout스레드도 리턴하기
		int waitret = WaitForMultipleObjects(2, coreclient->Thread, TRUE, 0);
		if (waitret == WAIT_OBJECT_0)
		{
			return 0;
		}

		//_initailTime
		Sleep(coreclient->_heartbeatTime);
		ULONGLONG now = GetTickCount64();
		if (now - coreclient->_Session.sendtime > coreclient->_heartbeatTime)
		{
			//하트비트 메시지 보내기
			//SendPacket();
		}
	}
}


bool CoreClient::Release()
{
	closesocket(Sock);
	return true;
}

void CoreClient::RecvCompletion(DWORD cbTransferred)
{
	_Session.RecvQ.MoveRear(cbTransferred);


	while (1)
	{
		//recv 후 처리
		//header만큼 들어왔는지 확인
		if (_type == LANCLIENT)
		{
			if (_Session.RecvQ.GetUsedSize() < sizeof(LanHEADER))
			{
				break;
			}

			LanHEADER header;
			int pkret = _Session.RecvQ.Peek((char*)&header, sizeof(LanHEADER));
			if (pkret != sizeof(LanHEADER))
			{
				DebugBreak();
			}

			//헤더+데이터 만큼 들어있는지 확인
			if (_Session.RecvQ.GetUsedSize() < header.len + sizeof(LanHEADER))
			{
				break;
			}
			SBuffer* msgbuf = SBuffer::BufPool.Alloc();
			msgbuf->AddRefcnt(1);
			msgbuf->ClearAtLServer();
			int deqret = _Session.RecvQ.Dequeue(msgbuf->GetWritePtr(), header.len + sizeof(LanHEADER));
			if (deqret != header.len + sizeof(LanHEADER))
			{
				DebugBreak();
			}
			int movret = msgbuf->MoveWritePos(deqret);
			if (movret != deqret)
			{
				DebugBreak();
			}
			int movrret = msgbuf->MoveReadPos(sizeof(LanHEADER));
			if (movrret != sizeof(LanHEADER))
			{
				DebugBreak();
			}
			InterlockedIncrement(&recvcount);
			CPacket packet(msgbuf);
			OnMessage(packet);
			msgbuf->DecRefcnt();
		}

		if (_type == NETCLIENT)
		{
			if (_Session.RecvQ.GetUsedSize() < sizeof(NetHEADER))
			{
				break;
			}

			NetHEADER header;
			int pkret = _Session.RecvQ.Peek((char*)&header, sizeof(NetHEADER));
			if (pkret != sizeof(NetHEADER))
			{
				DebugBreak();
			}

			//조작된 메시지임
			if (header.code != _code)
			{
				Disconnect();
				return;
			}

			//헤더+데이터 만큼 들어있는지 확인
			if (_Session.RecvQ.GetUsedSize() < header.len + sizeof(NetHEADER))
			{
				break;
			}
			SBuffer* msgbuf = SBuffer::BufPool.Alloc();
			msgbuf->AddRefcnt(1);
			msgbuf->ClearAtNServer();
			int deqret = _Session.RecvQ.Dequeue(msgbuf->GetWritePtr(), header.len + sizeof(NetHEADER));
			if (deqret != header.len + sizeof(NetHEADER))
			{
				DebugBreak();
			}
			int movret = msgbuf->MoveWritePos(deqret);
			if (movret != deqret)
			{
				DebugBreak();
			}
			InterlockedIncrement(&recvcount);

			//디코딩
			bool check = Decode(msgbuf);

			//잘못된 메시지임
			if (check == false)
			{
				msgbuf->DecRefcnt();
				Disconnect();
				break;
			}
			else
			{
				int movrret = msgbuf->MoveReadPos(sizeof(NetHEADER));
				if (movrret != sizeof(NetHEADER))
				{
					DebugBreak();
				}

				CPacket packet(msgbuf);
				OnMessage(packet);
			}
			msgbuf->DecRefcnt();
		}

	}

}

bool CoreClient::DoRecv()
{
	int rvretval;

	//WSARecv 걸기
	if (_Session.RecvQ.GetUsedSize() == _Session.RecvQ.DirectEnqueueSize())
	{
		WSABUF wsabuf;
		wsabuf.buf = _Session.RecvQ.GetRearBufferPtr();
		wsabuf.len = _Session.RecvQ.DirectEnqueueSize();
		DWORD recvbytes, flags = 0;
		ZeroMemory(&_Session.recvio.overlapped, sizeof(_Session.recvio.overlapped));

		rvretval = WSARecv(Sock, &wsabuf, 1, &recvbytes, &flags, &_Session.recvio.overlapped, NULL);
		if (rvretval == SOCKET_ERROR)
		{
			rvretval = WSAGetLastError();
			if (rvretval != ERROR_IO_PENDING)
			{
				Disconnect();
				return false;
			}
		}
		return true;
	}
	else
	{
		WSABUF wsabuf[2];
		DWORD recvbytes = 0;
		DWORD recvflags = 0;
		wsabuf[0].buf = _Session.RecvQ.GetRearBufferPtr();
		wsabuf[0].len = _Session.RecvQ.DirectEnqueueSize();
		wsabuf[1].buf = _Session.RecvQ.GetStartBufferPtr();
		wsabuf[1].len = _Session.RecvQ.GetFreeSize() - wsabuf[0].len;
		ZeroMemory(&_Session.recvio.overlapped, sizeof(_Session.recvio.overlapped));

		rvretval = WSARecv(Sock, wsabuf, 2, &recvbytes, &recvflags, &_Session.recvio.overlapped, NULL);
		if (rvretval == SOCKET_ERROR)
		{
			rvretval = WSAGetLastError();
			if (rvretval != ERROR_IO_PENDING)
			{

				Disconnect();
				return false;
			}
		}
		return true;
	}
}

void CoreClient::SendCompletion(DWORD cbTransferred)
{
	_Session.SendQ.MoveFront(cbTransferred);
}

bool CoreClient::DoSend()
{


	int sdretval;
	if (_Session.SendQ.GetUsedSize() > 0)
	{
		if (InterlockedExchange(&_Session.sendflag, 1) == 0)
		{
			WSABUF wsabuf;
			DWORD sendbytes = 0;
			DWORD sendflags = 0;

			if (_Session.SendQ.GetUsedSize() == 0)
			{
				InterlockedExchange(&_Session.sendflag, 0);
				//혹시 못 봤을 상황 대비
				if (_Session.SendQ.GetUsedSize() > 0)
				{
					DoSend();
					return true;
				}
				return false;
			}
			wsabuf.buf = _Session.SendQ.GetFrontBufferPtr();
			wsabuf.len = _Session.SendQ.DirectDequeueSize();
			ZeroMemory(&_Session.sendio.overlapped, sizeof(_Session.sendio.overlapped));
			sdretval = WSASend(Sock, &wsabuf, 1, &sendbytes, sendflags, &_Session.sendio.overlapped, NULL);
			if (sdretval == SOCKET_ERROR)
			{
				sdretval = WSAGetLastError();
				if (sdretval != ERROR_IO_PENDING)
				{
					Disconnect();
					return false;
				}
			}


			InterlockedAdd(&sendcount, 1);
			return true;

		}
		return false;
	}
	return false;
}




void CoreClient::SetBufferHeader(SBuffer* msgbuf)
{
	if (_type == LANCLIENT)
	{
		LanHEADER header;
		header.len = msgbuf->GetDataSize();
		msgbuf->read = 3;
		msgbuf->DataSize += 2;
		LanHEADER* temp = (LanHEADER*)msgbuf->GetReadPtr();
		*temp = header;
	}

	if (_type == NETCLIENT)
	{
		srand((unsigned int)GetTickCount64());

		NetHEADER header;
		header.code = _code;
		header.len = msgbuf->GetDataSize();
		header.randkey = rand();
		header.checksum = 0;
		for (int i = 0; i < header.len; i++)
		{
			header.checksum += msgbuf->buffer[5 + i];
		}
		header.checksum %= 256;

		msgbuf->read = 0;
		msgbuf->DataSize += 5;
		NetHEADER* temp = (NetHEADER*)msgbuf->GetReadPtr();
		*temp = header;
	}
}

void CoreClient::Encode(SBuffer* msg)
{
	unsigned short len = *(short*)&msg->buffer[1];
	unsigned char randkey = *(char*)&msg->buffer[3];

	char D;
	char P1 = 0;
	char E1 = 0;
	char P2 = 0;
	char E2 = 0;
	for (int i = 0; i < len + 1; i++)
	{
		D = msg->buffer[4 + i];
		P2 = D ^ (P1 + randkey + i + 1);
		E2 = P2 ^ (E1 + _fixedKey + i + 1);
		msg->buffer[4 + i] = E2;
		P1 = P2;
		E1 = E2;
	}
}

bool CoreClient::Decode(SBuffer* msg)
{
	unsigned short len = *(short*)&msg->buffer[1];
	unsigned char randkey = *(char*)&msg->buffer[3];

	char P1 = 0;
	char E1 = 0;
	char P2 = 0;
	char E2 = 0;
	for (int i = 0; i < len + 1; i++)
	{
		E2 = msg->buffer[4 + i];
		P2 = E2 ^ (E1 + _fixedKey + i + 1);
		msg->buffer[4 + i] = P2 ^ (P1 + randkey + i + 1);
		P1 = P2;
		E1 = E2;
	}

	//checksum 확인
	unsigned char checksum = *(char*)&msg->buffer[4];
	unsigned char msgchecksum = 0;
	for (int i = 0; i < len; i++)
	{
		msgchecksum += msg->buffer[5 + i];
	}
	msgchecksum %= 256;

	if (checksum != msgchecksum)
	{
		return false;
	}

	return true;
}
