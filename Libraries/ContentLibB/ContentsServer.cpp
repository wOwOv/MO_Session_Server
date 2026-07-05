#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib")

#include "ContentsServer.h"
#include <ws2tcpip.h>
#include "Parser.h"
#include "Logger.h"
#include <conio.h>
#include "SRWLockGuard.h"

ContentsServer::ContentsServer(ServerType type) : _type(type)
{
	timeBeginPeriod(1);
	LOG(L"SYSTEM",LVSYSTEM,L"ContentsServer Created");

}

ContentsServer::~ContentsServer()
{
	CloseHandle(_hcp);
}

void ContentsServer::ServerControl()
{
	static bool controlMode = false;
	//L:컨트롤 Lock, U:컨트롤 Unlock, S:서버상태확인, Q:종료

	if (_kbhit())
	{
		int controlKey = _getch();

		//키보드 제어 허용
		if (controlKey == 'u' || controlKey == 'U')
		{
			controlMode = true;

			//관련 키 도움말 출력
			printf("Control Mode Unlockded\n");
			printf("Control Mode : Press 'L' to Lock Control, 'S' to Show Server State, 'Q' to Quit\n");
		}

		//키보드 제어 잠금
		if ((controlKey == 'L' || controlKey == 'l') && controlMode)
		{
			controlMode = false;

			printf("Control Mode Locked\n");
			printf("Press 'U' to Unlock Control\n");
		}

		//키보드 제어 풀림 상태에서 서버 상태 확인
		if ((controlKey == 'S' || controlKey == 's') && controlMode)
		{
			ShowServerInfo();
		}

		//키보드 제어 풀림 상태에서 서버 종료
		if ((controlKey == 'Q' || controlKey == 'q') && controlMode)
		{
			Stop();
		}

		if (controlMode)
		{
			OtherServerControl(controlKey);
		}
	}
}

void ContentsServer::ShowServerInfo()
{
	switch (_coreState)
	{
	case(CoreServerState::CORE_CREATED):
	{
		printf("Core State : CORE_CREATED\n");
		break;
	}
	case(CoreServerState::CORE_RUNNING):
	{
		printf("Core State : CORE_RUNNING\n");
		break;
	}
	case(CoreServerState::CORE_STOPPING):
	{
		printf("Core State : CORE_STOPPING\n");
		break;
	}
	case(CoreServerState:: CORE_STOPPED):
	{
		printf("Core State : CORE_STOPPED\n");
		break;
	}
	}
	printf("Session : %d\nAcceptTotal : %d\nAcceptTPS : %d\nRecvTPS : %d\nSendTPS: %d\nSBufferCapacity : %d\nSBufferUsing : %d\n"
	,GetSessionCount(),GetAcceptTotal(),GetAcceptTPS(),GetRecvMessageTPS(),GetSendMessageTPS(),GetSBufferCapacity(),GetSBufferUsingCount());
}


bool ContentsServer::Start(const char* txtname, char code, char key)
{
	//config parsing하여 필요한 정보들 얻어오기
	Parsing(txtname);
	Setting(code, key);
	Network();

	_coreState = CoreServerState::CORE_RUNNING;
	LOG(L"SYSTEM", LVSYSTEM, L"ContentsServer Running");
	return 1;
}

void ContentsServer::Stop()
{
	_coreState = CoreServerState::CORE_STOPPING;
	StopAcceptThread();
	StopWorkerThread();
	StopTimeOutThread();
	StopMonitorThread();

	//윈속종료
	int scretval = WSACleanup();
	if (scretval == SOCKET_ERROR)
	{
		scretval = WSAGetLastError();
		printf("cleanup error: %d\n", scretval);
	}
	_coreState = CoreServerState::CORE_STOPPED;
}


bool ContentsServer::Disconnect(__int64 sessionID)
{
	//세션id찾기

	int index = FindSession(sessionID);

	long rCheck = InterlockedIncrement(&_sessionArray[index]._ioCount);

	//rFlag가 true였다면
	if ((rCheck & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&_sessionArray[index]._ioCount) == 0)
		{
			InterlockedExchange(&_sessionArray[index]._dFlag, 1);
			PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)&_sessionArray[index], NULL);
		}
		return false;
	}

	//내가 찾던 세션이 맞는지 확인
	if (InterlockedOr((unsigned long long*) & _sessionArray[index]._sessionID, 0) == sessionID)
	{
		//rFlag가 false라면 dFlag flag바꾸고 release하도록 유도
		InterlockedExchange(&_sessionArray[index]._dFlag, 1);
		CancelIoEx((HANDLE)_sessionArray[index]._sock, NULL);
	}
	if (InterlockedDecrement(&_sessionArray[index]._ioCount) == 0)
	{
		InterlockedExchange(&_sessionArray[index]._dFlag, 1);
		PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)&_sessionArray[index], NULL);
	}

	return true;
}

bool ContentsServer::SendPacket(__int64 sessionID, CPacket packet) {
	int index = FindSession(sessionID);

	long RCheck = InterlockedIncrement((long*)&_sessionArray[index]._ioCount);

	//rFlag가 true였다면
	if ((RCheck & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&_sessionArray[index]._ioCount) == 0)
		{
			InterlockedExchange(&_sessionArray[index]._dFlag, 1);
			PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)&_sessionArray[index], NULL);
		}
		return false;
	}
	//내가 찾던 세션이 맞는지 확인
	if (InterlockedOr((unsigned long long*) & _sessionArray[index]._sessionID, 0) == sessionID)
	{

		if (_sessionArray[index]._dFlag == 0)
		{
			SBuffer* msgbuf = packet.SBuf;
			msgbuf->AddRefcnt(1);	//내가 msgbuf쓰고 있음
			if (msgbuf->eFlag == 0)
			{
				SRWExclusiveLockGuard guard(msgbuf->eKey);
				if (msgbuf->eFlag == 0)
				{
					SetBufferHeader(msgbuf);
					if (_type == ServerType::NETSERVER)
					{
						Encode(msgbuf);
					}
					InterlockedExchange8(&msgbuf->eFlag, 1);
				}
			}
			msgbuf->AddRefcnt(1);	//SendQ에 인큐할거니까
			bool check = _sessionArray[index]._sendQ.Enqueue(msgbuf);
			if (check == false)
			{
				msgbuf->DecRefcnt();
				msgbuf->DecRefcnt();
				Disconnect(sessionID);
			}
			else
			{
				msgbuf->DecRefcnt();	//내가 msgbuf 다 씀
				PostQueuedCompletionStatus(_hcp, 2, (ULONG_PTR)&_sessionArray[index], NULL);
			}
		}

	}
	if (InterlockedDecrement((long*)&_sessionArray[index]._ioCount) == 0)
	{
		InterlockedExchange(&_sessionArray[index]._dFlag, 1);
		PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)&_sessionArray[index], NULL);
	}

	return true;
}

void ContentsServer::PostQueueContentsShutDown(__int32 contentsnum)
{
	PostQueuedCompletionStatus(_hcp, contentsnum, NULL, (LPOVERLAPPED)104);
}


int ContentsServer::GetSessionCount()
{
	return _sessionCount;
}

int ContentsServer::GetAcceptTPS()
{
	return _acceptTPS;
}

unsigned long long ContentsServer::GetAcceptTotal()
{
	return _acceptCall;
}

int ContentsServer::GetRecvMessageTPS()
{
	return _recvMessageTPS;
}

int ContentsServer::GetSendMessageTPS()
{
	return _sendMessageTPS;
}

unsigned long ContentsServer::GetSBufferCapacity()
{
	return SBuffer::BufPool.GetCapacity();
}

unsigned long ContentsServer::GetSBufferUsingCount()
{
	return SBuffer::BufPool.GetUsingCount();
}

unsigned long ContentsServer::GetContentsFPS(__int32 contentsnum)
{
	unsigned long ret = 0;
	SRWSharedLockGuard guard(_mapKey);
	std::unordered_map<__int32, Contents*>::iterator it = _contentsMap.find(contentsnum);
	if (it != _contentsMap.end())
	{
		Contents* contents = it->second;
		ret = contents->GetFPS();
	}
	return ret;
}

unsigned long ContentsServer::GetContentsLogic(__int32 contentsnum)
{
	unsigned long ret = 0;
	SRWSharedLockGuard guard(_mapKey);
	std::unordered_map<__int32, Contents*>::iterator it = _contentsMap.find(contentsnum);
	if (it != _contentsMap.end())
	{
		Contents* contents = it->second;
		ret = contents->GetLogic();
	}
	return ret;
}



void ContentsServer::RegisterContents(__int32 contentsnum, Contents* contents) 
{
	SRWExclusiveLockGuard guard(_mapKey);
	_contentsMap.insert(std::make_pair(contentsnum, contents));
	contents->_mServer = this;
	if (contents->_proxy != nullptr)
	{
		contents->_proxy->ConnectServer(this);
	}
	if (contents->_stub != nullptr)
	{
		contents->_stub->ConnectServer(this);
	}
	if (contents->_frame != -1)
	{
		PostQueuedCompletionStatus(_hcp, contentsnum, contentsnum, (LPOVERLAPPED)103);//OnUpdatePQCS
	}

}

void ContentsServer::DeregisterContents(__int32 contentsnum)
{
	SRWExclusiveLockGuard guard(_mapKey);
	_contentsMap.erase(contentsnum);
}

void ContentsServer::SetDefaultContents(__int32 contentsnum)
{
	_defaultContents = contentsnum;
}

void ContentsServer::InsertToContents(__int64 sessionID, __int32 contentsnum)
{
	PostQueuedCompletionStatus(_hcp, contentsnum, sessionID, (LPOVERLAPPED)101);
}

void ContentsServer::DeleteFromContents(__int64 sessionID, __int32 contentsnum)
{
	PostQueuedCompletionStatus(_hcp, contentsnum, sessionID, (LPOVERLAPPED)102);
}


bool ContentsServer::SetContentsNum(__int64 sessionID, __int32 contentsnum)
{
	int index = FindSession(sessionID);

	long RCheck = InterlockedIncrement((long*)&_sessionArray[index]._ioCount);

	//rFlag가 true였다면
	if ((RCheck & RELEASEFLAG) == RELEASEFLAG)
	{
		if (InterlockedDecrement(&_sessionArray[index]._ioCount) == 0)
		{
			InterlockedExchange(&_sessionArray[index]._dFlag, 1);
			PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)&_sessionArray[index], NULL);
		}
		return false;
	}
	//내가 찾던 세션이 맞는지 확인
	if (InterlockedOr((unsigned long long*) & _sessionArray[index]._sessionID, 0) == sessionID)
	{

		if (_sessionArray[index]._dFlag == 0)
		{
			InterlockedExchange((long*)&_sessionArray[index]._contentsNum, contentsnum);
		}

	}
	if (InterlockedDecrement((long*)&_sessionArray[index]._ioCount) == 0)
	{
		InterlockedExchange(&_sessionArray[index]._dFlag, 1);
		PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)&_sessionArray[index], NULL);
	}

	return true;
}


void ContentsServer::AttachStub(IStub* stub)
{
	_stub = stub;
}

IStub* ContentsServer::DetachStub()
{
	IStub* ret = _stub;
	_stub = nullptr;
	return ret;
}

void ContentsServer::AttachProxy(IProxy* proxy)
{
	_proxy = proxy;
	_proxy->ConnectServer(this);
}

IProxy* ContentsServer::DetachProxy()
{
	IProxy* ret = _proxy;
	_proxy = nullptr;
	return ret;
}

void ContentsServer::StopAcceptThread()
{
	_coreState = CoreServerState::CORE_STOPPING;
	//listendsocket을 닫아서 Acceptthread리턴 유도
	closesocket(_listenSock);
	WaitForSingleObject(_acceptThread, INFINITE);
	CloseHandle(_acceptThread);
	_acceptThread = nullptr;
	LOG(L"SYSTEM", LVSYSTEM, L"ContentsServer Stop Accepting");
	printf("ContentsServer Stop Accepting\n");
	CheckAllCoreStopped();
}

void ContentsServer::StopTimeOutThread()
{
	//타임아웃스레드 생성시
	if (_timeoutVal == 1)
	{
		_coreState = CoreServerState::CORE_STOPPING;
		_timeOutThreadStop = TRUE;
		WaitForSingleObject(_timeOutThread, INFINITE);
		CloseHandle(_timeOutThread);
		_timeOutThread = nullptr;
		LOG(L"SYSTEM", LVSYSTEM, L"ContentsServer Stop TimeOut");
		printf("ContentsServer Stop TimeOut\n");
		CheckAllCoreStopped();
	}
}

void ContentsServer::StopMonitorThread()
{
	_coreState = CoreServerState::CORE_STOPPING;
	_monitorThreadStop = TRUE;
	WaitForSingleObject(_monitorThread, INFINITE);
	CloseHandle(_monitorThread);
	_monitorThread = nullptr;
	LOG(L"SYSTEM", LVSYSTEM, L"ContentsServer Stop Monitor");
	printf("ContentsServer Stop Monitor\n");
	CheckAllCoreStopped();
}

void ContentsServer::StopWorkerThread()
{
	_coreState = CoreServerState::CORE_STOPPING;
	//워커스레드 리턴을 위한 PQCS
	ULONG_PTR p = 0;
	LPOVERLAPPED po = nullptr;
	for (int i = 0; i < _workerThread; i++)
	{
		PostQueuedCompletionStatus(_hcp, 0, p, po);
	}
	WaitForMultipleObjects(_workerThread, _workerThreads, TRUE, INFINITE);
	for (int i = 0; i < _workerThread; i++)
	{
		CloseHandle(_workerThreads[i]);
	}
	_workerThreads = nullptr;
	LOG(L"SYSTEM", LVSYSTEM, L"ContentsServer Stop Worker");
	printf("ContentsServer Stop Worker\n");
	CheckAllCoreStopped();
}

//L,U,S,Q는 제외
void ContentsServer::OtherServerControl(int controlKey)
{

}

void ContentsServer::CheckAllCoreStopped()
{
	if (_acceptThread != nullptr)
	{
		return;
	}
	if (_workerThreads != nullptr)
	{
		return;
	}
	if (_timeOutThread != nullptr)
	{
		return;
	}
	if (_monitorThread != nullptr)
	{
		return;
	}
	_coreState = CoreServerState::CORE_STOPPED;
	LOG(L"SYSTEM", LVSYSTEM, L"ContentsServer All Core Stopped");
}

void ContentsServer::Parsing(const char* txtname)
{
	char ip[16];
	int port;
	Parser* parse = new Parser;

	parse->LoadFile(txtname);
	parse->GetString("ContentsServer", "IP", ip, 16);
	parse->GetValue("ContentsServer", "Port", &port);

	ZeroMemory(&_serverAddr, sizeof(_serverAddr));
	_serverAddr.sin_family = AF_INET;
	int ineterror = inet_pton(AF_INET, ip, &_serverAddr.sin_addr);
	if (ineterror != 1)
	{
		printf("inet_pton error\n");
	}
	_serverAddr.sin_port = htons(port);

	parse->GetValue("ContentsServer", "WorkerThreadCreate", &_workerThread);
	parse->GetValue("ContentsServer", "Concurrent", &_concurrent);
	parse->GetValue("ContentsServer", "Nagle", &_nagleVal);
	parse->GetValue("ContentsServer", "MaxUser", &_maxUser);
	parse->GetValue("ContentsServer", "SetTimeOut", &_timeoutVal);
	parse->GetValue("ContentsServer", "InitialTimeOut", &_initialTime);
	parse->GetValue("ContentsServer", "RegularTimeOut", &_regularTime);

	delete parse;
}

int ContentsServer::Setting(char code, char key)
{
	//code, key setting
	_code = code;
	_fixedKey = key;

	int cpretval;//CreateIoCompletionProt retval

	//최대접속자수로 배열 생성
	_sessionArray = std::make_unique<SESSION[]>(_maxUser);

	//인덱스스택에 index세팅
	for (int i = _maxUser - 1; i >= 0; i--)
	{
		_indexArray.Push(i);
	}
	_sessionCount = 0;

	//iocp 생성
	//CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//입출력 완료 포트 생성
	_hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, si.dwNumberOfProcessors - _concurrent);
	if (_hcp == NULL)
	{
		cpretval = GetLastError();
		printf("CreateIoCompletionPort create error: %d\n", cpretval);
		return -1;
	}



	//_workerthread개의 작업자 스레드 생성
	_workerThreads = new HANDLE[_workerThread];
	for (int i = 0; i < _workerThread; i++)
	{
		_workerThreads[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread, this, 0, NULL);
		if (_workerThreads[i] == NULL)
		{
			return 1;
		}
	}

	//MonitorThread생성
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, &MonitorThread, this, 0, NULL);
	if (_monitorThread == NULL)
	{
		return 1;
	}

	if (_timeoutVal == 1)
	{
		//TimeOutThread생성
		_timeOutThread = (HANDLE)_beginthreadex(NULL, 0, &TimeOutThread, this, 0, NULL);
		if (_timeOutThread == NULL)
		{
			return 1;
		}
	}

	return 0;

}

int ContentsServer::Network()
{
	//retval
	int scretval;//startup
	int lscretval;//listensock retval
	int sbretval;//setsockopt sndbuf0 retval
	int bdretval;//bind retval
	int lnretval;//listen retval
	int lgretval;//linger retval
	int ndretval;//nodelay retval

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		scretval = WSAGetLastError();
		printf("WSAStartup error: %d\n", scretval);
		return -1;
	}

	//socket()
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		lscretval = WSAGetLastError();
		printf("socket() error: %d\n", lscretval);
		return -1;
	}


	//bind()
	bdretval = bind(_listenSock, (SOCKADDR*)&_serverAddr, sizeof(_serverAddr));
	if (bdretval == SOCKET_ERROR)
	{
		bdretval = WSAGetLastError();
		printf("bind error: %d\n", bdretval);
		return -1;
	}

	//SO_SNDBUF
	int bfoptval = 0;
	sbretval = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&bfoptval, sizeof(bfoptval));
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
	lgretval = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (lgretval == SOCKET_ERROR)
	{
		lgretval = WSAGetLastError();
		printf("linger error: %d\n", lgretval);
		return -1;
	}

	//nodelay
	if (_nagleVal != 0)
	{
		BOOL ndoptval = TRUE;
		ndretval = setsockopt(_listenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&ndoptval, sizeof(ndoptval));
		if (ndretval == SOCKET_ERROR)
		{
			ndretval = WSAGetLastError();
			printf("nodelay error: %d\n", ndretval);
			return -1;
		}
	}

	//listen()
	lnretval = listen(_listenSock, SOMAXCONN_HINT(30000));
	if (lnretval == SOCKET_ERROR)
	{
		lnretval = WSAGetLastError();
		printf("listen error: %d\n", lnretval);
		return -1;
	}

	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread, this, 0, NULL);



	return 0;
}


//작업자 스레드 함수
unsigned __stdcall ContentsServer::WorkerThread(LPVOID arg)
{
	ContentsServer* server = (ContentsServer*)arg;
	int gqcsretval;//GetQueuedCompletionStatus retval;
	HANDLE iocp = server->_hcp;

	while (1)
	{
		//비동기 입출력 완료 기다리기
		DWORD cbTransferred = 0;
		myOverlapped* myoverlapped = 0;
		SESSION* tgt = 0;
		gqcsretval = GetQueuedCompletionStatus(iocp, &cbTransferred, (PULONG_PTR)&tgt, (LPOVERLAPPED*)&myoverlapped, INFINITE);

		//비동기 입출력 결과 확인
		//스레드 종료
		if (cbTransferred == 0 && tgt == 0 && myoverlapped == 0)
		{
			printf("workerThread ended\n");
			return 0;

		}

		//Release수행
		if (gqcsretval != 0 && cbTransferred == 1 && tgt != 0 && myoverlapped == 0)
		{
			server->Release(tgt);

		}
		//SendPost 수행
		if (gqcsretval != 0 && cbTransferred == 2 && tgt != 0 && myoverlapped == 0)
		{
			server->SendPost(tgt);
		}

		//cbTransferred는 contentsnum, tgt은 sessionId
		//OnEnter수행
		if (gqcsretval != 0 && (long long)myoverlapped == 101)
		{
			SRWSharedLockGuard mapGuard(server->_mapKey);
			std::unordered_map<__int32, Contents*>::iterator tgtc = server->_contentsMap.find(cbTransferred);
			if (tgtc != server->_contentsMap.end())
			{
				Contents* contents = tgtc->second;

				SRWExclusiveLockGuard contentsGuard(contents->_contentsKey);
				contents->OnEnter((__int64)tgt, nullptr);
				contents->_logicCount++;

			}

			continue;

		}
		//OnLeave...
		if (gqcsretval != 0 && (long long)myoverlapped == 102)
		{
			SRWSharedLockGuard mapGuard(server->_mapKey);

			std::unordered_map<__int32, Contents*>::iterator tgtc = server->_contentsMap.find(cbTransferred);
			if (tgtc != server->_contentsMap.end())
			{
				Contents* contents = tgtc->second;

				SRWExclusiveLockGuard contentsGuard(contents->_contentsKey);
				contents->OnLeave((__int64)tgt, nullptr);
				contents->_logicCount++;

			}

			

			continue;
		}
		//OnUpdate...
		//cbTransferred으로 contentsnum이 들어옴
		if (gqcsretval != 0 && (long long)myoverlapped == 103)
		{
			int frame = 0;
			{
				SRWSharedLockGuard mapGuard(server->_mapKey);

				std::unordered_map<__int32, Contents*>::iterator tgtc = server->_contentsMap.find(cbTransferred);
				if (tgtc != server->_contentsMap.end())
				{
					Contents* contents = tgtc->second;
					SRWExclusiveLockGuard contentsGuard(contents->_contentsKey);
					contents->OnUpdate();
					contents->_fpsCount++;
					frame = contents->_frame;
				}
			}

			Sleep(frame);

			{
				SRWSharedLockGuard mapGuard(server->_mapKey);
				std::unordered_map<__int32, Contents*>::iterator tgtit = server->_contentsMap.find(cbTransferred);
				if (tgtit != server->_contentsMap.end())
				{
					Contents* contents = tgtit->second;
					PostQueuedCompletionStatus(server->_hcp, contents->_contentsNum, contents->_contentsNum, (LPOVERLAPPED)103);
				}
			}
			continue;
		}

		//OnShutDown
		if (gqcsretval != 0 && (long long)myoverlapped == 104)
		{
			SRWSharedLockGuard mapGuard(server->_mapKey);

			std::unordered_map<__int32, Contents*>::iterator tgtc = server->_contentsMap.find(cbTransferred);
			if (tgtc != server->_contentsMap.end())
			{
				Contents* contents = tgtc->second;
				SRWExclusiveLockGuard contentsGuard(contents->_contentsKey);
				contents->OnShutDown();
				contents->_logicCount++;
			}


			continue;
		}

		//비동기 입츌력 실패
		if (gqcsretval == 0 || cbTransferred == 0)
		{
			if (gqcsretval == 0)
			{
				gqcsretval = WSAGetLastError();
				DWORD temp1, temp2;
				WSAGetOverlappedResult(tgt->_sock, &myoverlapped->_overlapped, &temp1, FALSE, &temp2);
			}

			//완료통지 온 것에 대한 iocount감소
			if (InterlockedDecrement((long*)&tgt->_ioCount) == 0)
			{
				InterlockedExchange(&tgt->_dFlag, 1);
				PostQueuedCompletionStatus(server->_hcp, 1, (ULONG_PTR)tgt, NULL);
			}

		}
		//비동기 입출력 성공
		else if (myoverlapped != 0)
		{

			//recvio overlapped임
			if (myoverlapped->_type == 0)
			{
				tgt->_firstRecv = 1;
				tgt->_time = GetTickCount64();
				server->RecvCompletion(tgt, cbTransferred);
				server->RecvPost(tgt);
			}
			if (myoverlapped->_type == 1)
			{
				server->SendCompletion(tgt, cbTransferred);
				InterlockedExchange(&tgt->_sendFlag, 0);
				server->SendPost(tgt);
			}

			//완료통지 온 것에 대한 iocount감소
			if (InterlockedDecrement((long*)&tgt->_ioCount) == 0)
			{
				InterlockedExchange(&tgt->_dFlag, 1);
				PostQueuedCompletionStatus(server->_hcp, 1, (ULONG_PTR)tgt, NULL);
			}

		}



	}


}

//accept 스레드 함수 Thread[_workerthread]
unsigned __stdcall ContentsServer::AcceptThread(LPVOID arg)
{
	ContentsServer* coreserver = (ContentsServer*)arg;
	int atretval;//accept retval
	int rvretval;//recv retval

	//데이터 통신에 사용할 변수
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes;
	DWORD flags;

	while (1)
	{

		//accept()
		SOCKET sock;
		addrlen = sizeof(clientaddr);
		sock = accept(coreserver->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (sock == INVALID_SOCKET)
		{
			atretval = WSAGetLastError();
			printf("accept error: %d\n", atretval);
			printf("AcceptThread ended\n");
			return -1;
		}

		else
		{
			InterlockedIncrement(&coreserver->_acceptCount);
			InterlockedIncrement(&coreserver->_acceptCall);
			int index;
			bool check = coreserver->_indexArray.Pop(&index);
			//소켓 정보 구조체 할당
			if (check == false)
			{
				closesocket(sock);
				printf("SessionArray full\n");
				continue;
			}

			SESSION* tgt = &coreserver->_sessionArray[index];
			InterlockedIncrement(&coreserver->_sessionCount);
			ZeroMemory(&tgt->_sendIO._overlapped, sizeof(tgt->_sendIO._overlapped));
			ZeroMemory(&tgt->_recvIO._overlapped, sizeof(tgt->_recvIO._overlapped));


			tgt->_sendIO._type = 1;
			tgt->_recvIO._type = 0;

			tgt->_recvQ.ClearBuffer();
			coreserver->ClearSendQ(&tgt->_sendQ);

			tgt->_sock = sock;
			tgt->_ip = clientaddr.sin_addr;
			tgt->_port = clientaddr.sin_port;
			__int64 temp = index;
			//sessionID에 index숨기기
			tgt->_sessionID = coreserver->_sessionKey | temp << 48;


			tgt->_time = GetTickCount64();
			tgt->_firstRecv = 0;

			tgt->_sendFlag = 0;
			InterlockedIncrement(&tgt->_ioCount);
			InterlockedAnd(&tgt->_ioCount, 0x7fffffff);



			if (coreserver->_defaultContents != 0)
			{
				tgt->_contentsNum = coreserver->_defaultContents;
			}
			else
			{
				tgt->_contentsNum = 0;
			}


			InterlockedExchange(&tgt->_dFlag, 0);



			coreserver->_sessionKey++;

			//소켓과 입출력 완료 포트 연결
			CreateIoCompletionPort((HANDLE)tgt->_sock, coreserver->_hcp, (ULONG_PTR)tgt, 0);

			coreserver->OnAccept(clientaddr, tgt->_sessionID);
			if (tgt->_contentsNum != 0)
			{
				SRWSharedLockGuard mapGuard(coreserver->_mapKey);
				std::unordered_map<__int32, Contents*>::iterator tgtc = coreserver->_contentsMap.find(tgt->_contentsNum);
				if (tgtc != coreserver->_contentsMap.end())
				{
					Contents* contents = tgtc->second;

					SRWExclusiveLockGuard contentsGuard(contents->_contentsKey);
					contents->OnEnter(tgt->_sessionID, nullptr);

				}

			}

			//비동기 입출력 시작
			WSABUF wsabuf;
			wsabuf.buf = tgt->_recvQ.GetRearBufferPtr();
			wsabuf.len = tgt->_recvQ.DirectEnqueueSize();
			recvbytes = 0;
			flags = 0;

			rvretval = WSARecv(tgt->_sock, &wsabuf, 1, &recvbytes, &flags, &tgt->_recvIO._overlapped, NULL);
			if (rvretval == SOCKET_ERROR)
			{
				rvretval = WSAGetLastError();
				if (rvretval != ERROR_IO_PENDING)
				{
					if (InterlockedDecrement((long*)&tgt->_ioCount) == 0)
					{
						InterlockedExchange(&tgt->_dFlag, 1);
						PostQueuedCompletionStatus(coreserver->_hcp, 1, (ULONG_PTR)tgt, NULL);
					}
				}

			}


		}
	}
}

//Monitor 스레드 함수 Thread[_workerthread+1]
unsigned __stdcall ContentsServer::MonitorThread(LPVOID arg)
{
	ContentsServer* coreserver = (ContentsServer*)arg;

	while (1)
	{
		if (coreserver->_monitorThreadStop)
		{
			return 0;
		}

		//1초 되면 초기화
		Sleep(1000);

		coreserver->_acceptTPS = coreserver->_acceptCount;
		InterlockedExchange(&coreserver->_acceptCount, 0);
		coreserver->_recvMessageTPS = coreserver->_recvCount;
		InterlockedExchange(&coreserver->_recvCount, 0);
		coreserver->_sendMessageTPS = coreserver->_sendCount;
		InterlockedExchange(&coreserver->_sendCount, 0);

		{
			SRWSharedLockGuard mapGuard(coreserver->_mapKey);
			std::unordered_map<__int32, Contents*>::iterator it = coreserver->_contentsMap.begin();
			for (; it != coreserver->_contentsMap.end(); it++)
			{
				Contents* contents = it->second;
				contents->_fps = contents->_fpsCount;
				contents->_fpsCount = 0;
				contents->_logic = contents->_logicCount;
				contents->_logicCount = 0;
			}
		}

		coreserver->OnSecond();
	}
}

unsigned __stdcall ContentsServer::TimeOutThread(LPVOID arg)
{
	ContentsServer* coreserver = (ContentsServer*)arg;
	DWORD timecount = 0;

	while (1)
	{
		if (coreserver->_timeOutThreadStop)
		{
			return 0;
		}

		//_initailTime
		Sleep(1000);
		ULONGLONG now = GetTickCount64();
		timecount++;
		if ((timecount % coreserver->_initialTime) == 0)
		{
			for (int i = 0; i < coreserver->_maxUser; i++)
			{
				__int64 session = InterlockedOr((unsigned long long*) & coreserver->_sessionArray[i]._sessionID, 0);

				if (coreserver->_sessionArray[i]._dFlag == 0 && coreserver->_sessionArray[i]._firstRecv == 0)
				{
					if ((now - coreserver->_sessionArray[i]._time) > coreserver->_initialTime * 1000)
					{
						coreserver->Disconnect(session);
					}
				}
			}
		}
		if ((timecount % coreserver->_regularTime) == 0)
		{
			for (int i = 0; i < coreserver->_maxUser; i++)
			{
				__int64 session = InterlockedOr((unsigned long long*) & coreserver->_sessionArray[i]._sessionID, 0);

				if (coreserver->_sessionArray[i]._dFlag == 0 && coreserver->_sessionArray[i]._firstRecv != 0)
				{
					if ((now - coreserver->_sessionArray[i]._time) > coreserver->_regularTime * 1000)
					{
						coreserver->Disconnect(session);
					}
				}
			}
		}

	}
}


bool ContentsServer::Release(SESSION* tgt)
{
	if (InterlockedCompareExchange((long*)&tgt->_ioCount, RELEASEFLAG, 0) != 0)
	{
		return false;
	}
	ReleaseCount.fetch_add(1);
	closesocket(tgt->_sock);
	tgt->_sock = INVALID_SOCKET;
	for (int i = 0; i < tgt->_packetBox._count; i++)
	{
		tgt->_packetBox._SBufferArray[i]->DecRefcnt();
	}
	tgt->_packetBox._count = 0;
	ClearSendQ(&tgt->_sendQ);


	//메모리풀 스택 방식
	InterlockedDecrement(&_sessionCount);
	int32_t contentsnum = tgt->_contentsNum;
	if (contentsnum == -1)
	{
		Release1Count.fetch_add(1);
	}
	if (contentsnum == 0)
	{
		Release0Count.fetch_add(1);
	}
	if (contentsnum != 0 && contentsnum != -1)
	{
		SRWSharedLockGuard mapGuard(_mapKey);
		std::unordered_map<__int32, Contents*>::iterator tgtc = _contentsMap.find(tgt->_contentsNum);
		if (tgtc != _contentsMap.end())
		{
			Contents* contents = tgtc->second;
			SRWExclusiveLockGuard contentsGuard(contents->_contentsKey);
			contents->OnLeave(tgt->_sessionID, nullptr);
		}
		else if(contentsnum!= 1000000001)
		{
			ReleaseFindFail.fetch_add(1);
		}
	}

	OnRelease(tgt->_sessionID, tgt->_contentsNum);

	_indexArray.Push(FindSession(tgt->_sessionID));								//인덱스번호 반환되기 전에 OnRelease호출되어야함. 반환 후 재사용되고 재사용된 세션ID에 대해 OnRelease가 갈 수도 있으므로


	return true;

}

void ContentsServer::RecvCompletion(SESSION* tgt, DWORD cbTransferred)
{
	tgt->_recvQ.MoveRear(cbTransferred);

	if (tgt->_contentsNum == 0)						//지정된 스레드 없음
	{
		if (_type == ServerType::LANSERVER)
		{
			while (1)
			{
				if (tgt->_recvQ.GetUsedSize() < sizeof(LanHEADER))
				{
					break;
				}

				LanHEADER header;
				int pkret = tgt->_recvQ.Peek((char*)&header, sizeof(LanHEADER));
				if (pkret != sizeof(LanHEADER))
				{
					DebugBreak();
				}

				//헤더+데이터 만큼 들어있는지 확인
				if (tgt->_recvQ.GetUsedSize() < header._len + sizeof(LanHEADER))
				{
					break;
				}
				SBuffer* msgbuf = SBuffer::BufPool.Alloc();
				msgbuf->AddRefcnt(1);
				msgbuf->ClearAtLServer();
				int deqret = tgt->_recvQ.Dequeue(msgbuf->GetWritePtr(), header._len + sizeof(LanHEADER));
				if (deqret != header._len + sizeof(LanHEADER))
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
				InterlockedIncrement(&_recvCount);
				CPacket packet(msgbuf);
				_stub->ProcMessage(tgt->_sessionID, packet);
				msgbuf->DecRefcnt();
			}
		}
		if (_type == ServerType::NETSERVER)
		{
			while (1)
			{
				if (tgt->_recvQ.GetUsedSize() < sizeof(NetHEADER))
				{
					break;
				}

				NetHEADER header;
				int pkret = tgt->_recvQ.Peek((char*)&header, sizeof(NetHEADER));
				if (pkret != sizeof(NetHEADER))
				{
					DebugBreak();
				}

				/////////////////////////////////////////////////////////////////////////////////////////////////////////
				//공격 대비
				if (header._len + sizeof(NetHEADER) >= SBuffer::BUFFER_DEFAULT)
				{
					Disconnect(tgt->_sessionID);
					return;
				}
				/////////////////////////////////////////////////////////////////////////////////////////////////////////

				//조작된 메시지임
				if (header._code != _code)
				{
					SOCKADDR_IN addr;
					addr.sin_addr = tgt->_ip;
					addr.sin_port = tgt->_port;
					OnUnusual(tgt->_sessionID, addr);
					Disconnect(tgt->_sessionID);
					return;
				}

				//헤더+데이터 만큼 들어있는지 확인
				if (tgt->_recvQ.GetUsedSize() < header._len + sizeof(NetHEADER))
				{
					break;
				}
				SBuffer* msgbuf = SBuffer::BufPool.Alloc();
				msgbuf->AddRefcnt(1);
				msgbuf->ClearAtNServer();
				int deqret = tgt->_recvQ.Dequeue(msgbuf->GetWritePtr(), header._len + sizeof(NetHEADER));
				if (deqret != header._len + sizeof(NetHEADER))
				{
					DebugBreak();
				}
				int movret = msgbuf->MoveWritePos(deqret);
				if (movret != deqret)
				{
					DebugBreak();
				}
				InterlockedIncrement(&_recvCount);

				//디코딩
				bool check = Decode(msgbuf);

				//잘못된 메시지임
				if (check == false)
				{
					SOCKADDR_IN addr;
					addr.sin_addr = tgt->_ip;
					addr.sin_port = tgt->_port;
					OnUnusual(tgt->_sessionID, addr);
					msgbuf->DecRefcnt();
					Disconnect(tgt->_sessionID);
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
					_stub->ProcMessage(tgt->_sessionID, packet);
				}
				msgbuf->DecRefcnt();
			}
		}
	}
	else if (tgt->_contentsNum != -1)					//특정 컨텐츠에 속함
	{
		SRWSharedLockGuard mapGuard(_mapKey);
		std::unordered_map<__int32, Contents*>::iterator tgtc = _contentsMap.find(tgt->_contentsNum);
		if (tgtc != _contentsMap.end())
		{
			Contents* contents = tgtc->second;
			//컨텐츠 로직 순차적
			SRWExclusiveLockGuard contentsGuard(contents->_contentsKey);

			//recv 후 처리
			//header만큼 들어왔는지 확인
			if (_type == ServerType::LANSERVER)
			{
				while (1)
				{
					if (tgt->_recvQ.GetUsedSize() < sizeof(LanHEADER))
					{
						break;
					}

					LanHEADER header;
					int pkret = tgt->_recvQ.Peek((char*)&header, sizeof(LanHEADER));
					if (pkret != sizeof(LanHEADER))
					{
						DebugBreak();
					}

					//헤더+데이터 만큼 들어있는지 확인
					if (tgt->_recvQ.GetUsedSize() < header._len + 1 + sizeof(LanHEADER))
					{
						break;
					}
					SBuffer* msgbuf = SBuffer::BufPool.Alloc();
					msgbuf->AddRefcnt(1);
					msgbuf->ClearAtLServer();
					int deqret = tgt->_recvQ.Dequeue(msgbuf->GetWritePtr(), header._len + 1 + sizeof(LanHEADER));
					if (deqret != header._len + 1 + sizeof(LanHEADER))
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
					InterlockedIncrement(&_recvCount);
					CPacket packet(msgbuf);
					contents->_stub->ProcMessage(tgt->_sessionID, packet);
					contents->_logicCount++;
					msgbuf->DecRefcnt();
				}
			}

			if (_type == ServerType::NETSERVER)
			{
				while (1)
				{
					if (tgt->_recvQ.GetUsedSize() < sizeof(NetHEADER))
					{
						break;
					}

					NetHEADER header;
					int pkret = tgt->_recvQ.Peek((char*)&header, sizeof(NetHEADER));
					if (pkret != sizeof(NetHEADER))
					{
						DebugBreak();
					}

					//조작된 메시지임
					if (header._code != _code)
					{
						SOCKADDR_IN addr;
						addr.sin_addr = tgt->_ip;
						addr.sin_port = tgt->_port;
						OnUnusual(tgt->_sessionID, addr);
						Disconnect(tgt->_sessionID);
						return;
					}

					//헤더+데이터 만큼 들어있는지 확인
					if (tgt->_recvQ.GetUsedSize() < header._len + sizeof(NetHEADER))
					{
						break;
					}
					SBuffer* msgbuf = SBuffer::BufPool.Alloc();
					msgbuf->AddRefcnt(1);
					msgbuf->ClearAtNServer();
					int deqret = tgt->_recvQ.Dequeue(msgbuf->GetWritePtr(), header._len + sizeof(NetHEADER));
					if (deqret != header._len + sizeof(NetHEADER))
					{
						DebugBreak();
					}
					int movret = msgbuf->MoveWritePos(deqret);
					if (movret != deqret)
					{
						DebugBreak();
					}
					int movrret = msgbuf->MoveReadPos(sizeof(NetHEADER));
					if (movrret != sizeof(NetHEADER))
					{
						DebugBreak();
					}
					InterlockedIncrement(&_recvCount);

					//디코딩
					bool check = Decode(msgbuf);

					//잘못된 메시지임
					if (check == false)
					{
						SOCKADDR_IN addr;
						addr.sin_addr = tgt->_ip;
						addr.sin_port = tgt->_port;
						OnUnusual(tgt->_sessionID, addr);
						msgbuf->DecRefcnt();
						Disconnect(tgt->_sessionID);
						break;
					}
					else
					{
						CPacket packet(msgbuf);
						contents->_stub->ProcMessage(tgt->_sessionID, packet);
						contents->_logicCount++;
					}
					msgbuf->DecRefcnt();
				}
			}



		}


	}
	else                           //contentsNum==-1 이동중이므로 메시지 무시
	{
		if (_type == ServerType::LANSERVER)
		{
			while (1)
			{
				if (tgt->_recvQ.GetUsedSize() < sizeof(LanHEADER))
				{
					break;
				}

				LanHEADER header;
				int pkret = tgt->_recvQ.Peek((char*)&header, sizeof(LanHEADER));
				if (pkret != sizeof(LanHEADER))
				{
					DebugBreak();
				}

				//헤더+데이터 만큼 들어있는지 확인
				if (tgt->_recvQ.GetUsedSize() < header._len + sizeof(LanHEADER))
				{
					break;
				}

				//이동중에는 메시지 무시하므로 front 이동시켜 제거
				tgt->_recvQ.MoveFront(header._len + sizeof(LanHEADER));
			}
		}
		if (_type == ServerType::NETSERVER)
		{
			while (1)
			{
				if (tgt->_recvQ.GetUsedSize() < sizeof(NetHEADER))
				{
					break;
				}

				NetHEADER header;
				int pkret = tgt->_recvQ.Peek((char*)&header, sizeof(NetHEADER));
				if (pkret != sizeof(NetHEADER))
				{
					DebugBreak();
				}

				/////////////////////////////////////////////////////////////////////////////////////////////////////////
				//공격 대비
				if (header._len + sizeof(NetHEADER) >= SBuffer::BUFFER_DEFAULT)
				{
					Disconnect(tgt->_sessionID);
					return;
				}
				/////////////////////////////////////////////////////////////////////////////////////////////////////////

				//조작된 메시지임
				if (header._code != _code)
				{
					SOCKADDR_IN addr;
					addr.sin_addr = tgt->_ip;
					addr.sin_port = tgt->_port;
					OnUnusual(tgt->_sessionID, addr);
					Disconnect(tgt->_sessionID);
					return;
				}

				//헤더+데이터 만큼 들어있는지 확인
				if (tgt->_recvQ.GetUsedSize() < header._len + sizeof(NetHEADER))
				{
					break;
				}

				//이동중에는 메시지 무시하므로 front 이동시켜 제거
				tgt->_recvQ.MoveFront(header._len + sizeof(NetHEADER));
			}
		}
	}

}

bool ContentsServer::RecvPost(SESSION* tgt)
{
	int rvretval;

	//Disconnect플래그 확인
	if (InterlockedOr(&tgt->_dFlag, 0) == 0)
	{
		//WSARecv 걸기
		InterlockedIncrement((long*)&tgt->_ioCount);
		if (tgt->_recvQ.GetUsedSize() == tgt->_recvQ.DirectEnqueueSize())
		{
			WSABUF wsabuf;
			wsabuf.buf = tgt->_recvQ.GetRearBufferPtr();
			wsabuf.len = tgt->_recvQ.DirectEnqueueSize();
			DWORD recvbytes, flags = 0;
			ZeroMemory(&tgt->_recvIO._overlapped, sizeof(tgt->_recvIO._overlapped));

			rvretval = WSARecv(tgt->_sock, &wsabuf, 1, &recvbytes, &flags, &tgt->_recvIO._overlapped, NULL);
			if (rvretval == SOCKET_ERROR)
			{
				rvretval = WSAGetLastError();
				if (rvretval != ERROR_IO_PENDING)
				{
					if (InterlockedDecrement((long*)&tgt->_ioCount) == 0)
					{
						InterlockedExchange(&tgt->_dFlag, 1);
						PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)tgt, NULL);
						//Release(tgt);
					}
					return false;
				}
				//recv는 비동기로 걸렸는데 disconnect플래그는 그 사이 켜졌음
				//다시 io취소해야함
				if (InterlockedOr(&tgt->_dFlag, 0) != 0)
				{
					CancelIoEx((HANDLE)tgt->_sock, NULL);
				}
			}
			return true;
		}
		else
		{
			WSABUF wsabuf[2];
			DWORD recvbytes = 0;
			DWORD recvflags = 0;
			wsabuf[0].buf = tgt->_recvQ.GetRearBufferPtr();
			wsabuf[0].len = tgt->_recvQ.DirectEnqueueSize();
			wsabuf[1].buf = tgt->_recvQ.GetStartBufferPtr();
			wsabuf[1].len = tgt->_recvQ.GetFreeSize() - wsabuf[0].len;
			ZeroMemory(&tgt->_recvIO._overlapped, sizeof(tgt->_recvIO._overlapped));

			rvretval = WSARecv(tgt->_sock, wsabuf, 2, &recvbytes, &recvflags, &tgt->_recvIO._overlapped, NULL);
			if (rvretval == SOCKET_ERROR)
			{
				rvretval = WSAGetLastError();
				if (rvretval != ERROR_IO_PENDING)
				{
					if (InterlockedDecrement((long*)&tgt->_ioCount) == 0)
					{
						InterlockedExchange(&tgt->_dFlag, 1);
						PostQueuedCompletionStatus(_hcp, 1, (ULONG_PTR)tgt, NULL);
						//Release(tgt);
					}
					return false;
				}
				if (InterlockedOr(&tgt->_dFlag, 0) != 0)
				{
					CancelIoEx((HANDLE)tgt->_sock, NULL);
				}
			}
			return true;
		}
	}
	return false;
}

void ContentsServer::SendCompletion(SESSION* tgt, DWORD cbTransferred)
{
	for (int i = 0; i < tgt->_packetBox._count; i++)
	{
		tgt->_packetBox._SBufferArray[i]->DecRefcnt();
	}
	tgt->_packetBox._count = 0;
}

bool ContentsServer::SendPost(SESSION* tgt)
{
	int sdretval;
	if (InterlockedOr(&tgt->_dFlag, 0) == 0)
	{
		if (tgt->_sendQ.GetUsedSize() > 0)
		{
			if (InterlockedExchange(&tgt->_sendFlag, 1) == 0)
			{
				WSABUF wsabuf[CPACKET_BOX_MAX];
				DWORD sendbytes = 0;
				DWORD sendflags = 0;

				long RCheck = InterlockedIncrement((long*)&tgt->_ioCount);

				//rFlag가 true였다면
				if ((RCheck & RELEASEFLAG) == RELEASEFLAG)
				{
					if (InterlockedDecrement(&tgt->_ioCount) == 0)
					{
						InterlockedExchange(&tgt->_dFlag, 1);
						Release(tgt);
					}
					return false;
				}

				int size = 0;
				for (size; size < CPACKET_BOX_MAX; size++)
				{
					bool check = tgt->_sendQ.Dequeue(&tgt->_packetBox._SBufferArray[size]);
					if (check == false)
					{
						break;
					}
					wsabuf[size].buf = tgt->_packetBox._SBufferArray[size]->GetReadPtr();
					wsabuf[size].len = tgt->_packetBox._SBufferArray[size]->GetDataSize();
				}
				tgt->_packetBox._count = size;

				if (size == 0)
				{
					InterlockedExchange(&tgt->_sendFlag, 0);
					//혹시 못 봤을 상황 대비
					if (tgt->_sendQ.GetUsedSize() > 0)
					{
						SendPost(tgt);
						if (InterlockedDecrement(&tgt->_ioCount) == 0)
						{
							InterlockedExchange(&tgt->_dFlag, 1);
							Release(tgt);
						}
						return true;
					}
					if (InterlockedDecrement(&tgt->_ioCount) == 0)
					{
						InterlockedExchange(&tgt->_dFlag, 1);
						Release(tgt);
					}
					return false;
				}

				ZeroMemory(&tgt->_sendIO._overlapped, sizeof(tgt->_sendIO._overlapped));
				sdretval = WSASend(tgt->_sock, wsabuf, size, &sendbytes, sendflags, &tgt->_sendIO._overlapped, NULL);
				if (sdretval == SOCKET_ERROR)
				{
					sdretval = WSAGetLastError();
					if (sdretval != ERROR_IO_PENDING)
					{
						if (InterlockedDecrement((long*)&tgt->_ioCount) == 0)
						{
							InterlockedExchange(&tgt->_dFlag, 1);
							Release(tgt);
						}
						return false;
					}
					if (InterlockedOr(&tgt->_dFlag, 0) != 0)
					{
						CancelIoEx((HANDLE)tgt->_sock, NULL);
					}
				}


				InterlockedAdd(&_sendCount, size);
				return true;

			}
		}
	}
	return false;
}



int ContentsServer::FindSession(__int64 tgtID)
{
	return tgtID >> 48;
}


void ContentsServer::SetBufferHeader(SBuffer* msgbuf)
{
	if (_type == ServerType::LANSERVER)
	{
		LanHEADER header;
		header._code = 0x89;
		header._len = msgbuf->GetDataSize() - 1;
		msgbuf->read = 3;
		msgbuf->DataSize += 2;
		LanHEADER* temp = (LanHEADER*)msgbuf->GetReadPtr();
		*temp = header;
	}

	if (_type == ServerType::NETSERVER)
	{
		srand((unsigned int)GetTickCount64());

		NetHEADER header;
		header._code = _code;
		header._len = msgbuf->GetDataSize();
		header._randKey = rand();
		header._checksum = 0;
		for (int i = 0; i < header._len; i++)
		{
			header._checksum += msgbuf->buffer[5 + i];
		}
		header._checksum %= 256;

		msgbuf->read = 0;
		msgbuf->DataSize += 5;
		NetHEADER* temp = (NetHEADER*)msgbuf->GetReadPtr();
		*temp = header;
	}
}

void ContentsServer::Encode(SBuffer* msg)
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

bool ContentsServer::Decode(SBuffer* msg)
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


void ContentsServer::ClearSendQ(MAXLFQueue<SBuffer*>* lfQ)
{
	SBuffer* tempbuf;
	bool check;
	while (1)
	{
		check = lfQ->Dequeue(&tempbuf);
		if (check == false)
		{
			break;
		}

		tempbuf->DecRefcnt();

	}
}









