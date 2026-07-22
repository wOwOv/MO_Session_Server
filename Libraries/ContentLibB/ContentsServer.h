#ifndef __CONTENTSSERVER__
#define __CONTENTSSERVER__

#include <unordered_map>
#include <winsock2.h>
#include "RingBuffer.h"
#include "LockFreeQueue(CAS).h"
#include "MaxLockFreeQueue(CAS).h"
#include "CPacket.h"
#include "SerialBuffer.h"
#include "Contents.h"
#include "LockFreeStack.h"
#include <shared_mutex>
#include <cstdint>
#include <memory>

static constexpr int CPACKET_BOX_MAX = 500;

//#define LANSERVER 2
//#define NETSERVER 5
enum class ServerType : unsigned char {
	LANSERVER = 2,
	NETSERVER = 5
};

class Contents;

class ContentsServer
{
	static constexpr long RELEASEFLAG = 0x80000000;

private:
	enum class CoreServerState: std::uint8_t
	{
		CORE_CREATED = 0,  // Start 전
		CORE_RUNNING = 1,  // Start 성공 후 동작 중
		CORE_STOPPING = 2, // Stop 진행 중
		CORE_STOPPED = 3   // 완전 종료
	};


#pragma pack(push,1)
	struct LanHEADER
	{
		BYTE	_code;			// 패킷코드 0x89 고정.
		BYTE	_len;			// 패킷 사이즈.
	};
	struct NetHEADER
	{
		unsigned char _code;
		unsigned short _len;
		unsigned char _randKey;
		unsigned char _checksum;
	};
#pragma pack(pop)
	struct myOverlapped
	{
		WSAOVERLAPPED _overlapped;
		bool _type;				//0이면 recv 1이면 send
	};
	struct PacketBox
	{
		SBuffer* _SBufferArray[CPACKET_BOX_MAX];
		int _count;
	};

	struct SESSION
	{
		myOverlapped _sendIO;
		myOverlapped _recvIO;

		RingBuffer _recvQ;
		MAXLFQueue<SBuffer*> _sendQ;

		SOCKET _sock;
		IN_ADDR _ip;
		std::uint16_t _port;

		ULONGLONG _time = 0;
		bool _firstRecv = 0;

		std::int64_t _sessionID;

		long _sendFlag = 0;

		long _ioCount = 0;

		long _dFlag = 0;		//disconnect flag

		std::int32_t _msgCount = 0;

		PacketBox _packetBox;

		std::int32_t _contentsNum=0;//0: 처음들어옴,-1: 이동중
	};

public:

	ContentsServer(ServerType type = ServerType::LANSERVER);
	virtual ~ContentsServer();

	ContentsServer(const ContentsServer&) = delete;
	ContentsServer& operator=(const ContentsServer&) = delete;
	ContentsServer(ContentsServer&&) = delete;
	ContentsServer& operator=(ContentsServer&&) = delete;

	virtual void ServerControl();
	virtual void ShowServerInfo();

	bool Start(const char* txtname, char code = 0, char key = 0);
	virtual void Stop();
	bool Disconnect(std::int64_t sessionID);
	bool SendPacket(std::int64_t sessionID, CPacket packet);
	void PostQueueContentsShutDown(std::int32_t contentsnum);

	int GetSessionCount();
	unsigned long long GetAcceptTotal();
	int GetAcceptTPS();
	int GetRecvMessageTPS();
	int GetSendMessageTPS();
	unsigned long GetSBufferCapacity();
	unsigned long GetSBufferUsingCount();
	unsigned long GetContentsFPS(std::int32_t contentsnum);
	unsigned long GetContentsLogic(std::int32_t contentsnum);
	long GetFpsMin();
	long GetFpsMax();
	long GetFpsAvg();


	void RegisterContents(std::int32_t contentsnum, const std::shared_ptr<Contents>& contents);
	void DeregisterContents(std::int32_t contentsnum);

	void SetDefaultContents(std::int32_t contentsnum);
	void InsertToContents(std::int64_t sessionID, std::int32_t contentsnum);
	void DeleteFromContents(std::int64_t sessionID, std::int32_t contentsnum);
	bool TryMoveSessionToContents(std::int64_t sessionID, std::int32_t contentsnum);

	void AttachStub(IStub* stub);
	IStub* DetachStub();
	void AttachProxy(IProxy* proxy);
	IProxy* DetachProxy();

protected:
	void StopAcceptThread();
	void StopTimeOutThread();
	void StopMonitorThread();
	void StopWorkerThread();
	void StopFrameSchedulerThread();

	virtual void OtherServerControl(int controlKey);

private:
	void CheckAllCoreStopped();

protected:
	virtual bool OnConnectionRequest(const SOCKADDR_IN& clientaddr) = 0;
	virtual void OnAccept(const SOCKADDR_IN& clientaddr, std::int64_t sessionID) = 0;
	virtual void OnRelease(std::int64_t sessionID, std::int32_t contentsnum) = 0;
	virtual void OnUnusual(std::int64_t sessionID, const SOCKADDR_IN& clientaddr) = 0;
	virtual void OnSecond() = 0;
	

private:
	//Start가 쓰는 함수들
	void Parsing(const char* txtname);
	int Setting(char code, char key);
	int Network();
	///////////////////////////////////

	static unsigned __stdcall WorkerThread(LPVOID arg);
	static unsigned __stdcall AcceptThread(LPVOID arg);
	static unsigned __stdcall MonitorThread(LPVOID arg);
	static unsigned __stdcall TimeOutThread(LPVOID arg);

	static unsigned __stdcall FrameScheduler(LPVOID arg);

	//WorkerThread가 쓰는 함수들
	bool Release(SESSION* tgt);
	void RecvCompletion(SESSION* tgt, DWORD cbTransferred);
	bool RecvPost(SESSION* tgt);
	void SendCompletion(SESSION* tgt, DWORD cbTransferred);
	bool SendPost(SESSION* tgt);


	//////////////////////////////////////////////////////////////////////

	//AcceptThread가 쓰는 함수들
	int FindSession(std::int64_t tgtID);

private:
	//직렬화버퍼 헤더 세팅
	void SetBufferHeader(SBuffer* msgbuf);
	//직렬화버퍼 인코딩
	void Encode(SBuffer* msg);
	//직렬화버퍼 디코딩
	bool Decode(SBuffer* msg);
	//SendQ 락프리큐 초기화용
	void ClearSendQ(MAXLFQueue<SBuffer*>* lfQ);

private:
	unsigned long long _acceptCall = 0;
	long _acceptTPS = 0;
	long _acceptCount = 0;
	long _recvMessageTPS = 0;
	long _recvCount = 0;
	long _sendMessageTPS = 0;
	long _sendCount = 0;
	std::int64_t _sessionKey = 1;

	long _fpsMin = 0;
	long _fpsMax = 0;
	long _fpsAvg = 0;

private:
	CoreServerState _coreState = CoreServerState::CORE_CREATED;
	SOCKET _listenSock;
	HANDLE _hcp;
	std::unique_ptr<SESSION[]> _sessionArray;
	DWORD _sessionCount;

	HANDLE _acceptThread;
	HANDLE* _workerThreads;
	HANDLE _timeOutThread;
	HANDLE _monitorThread;
	HANDLE _frameSchedulerThread;
	volatile LONG _timeOutThreadStop = FALSE;
	volatile LONG _monitorThreadStop = FALSE;
	volatile LONG _frameSchedulerStop = FALSE;

	unsigned char _code;
	unsigned char _fixedKey;

private:
	LFStack<int> _indexArray;

private:
	std::unordered_map<std::int32_t, std::shared_ptr<Contents>> _contentsMap;		// shared ownership; map retains registered Contents lifetime
	SRWLOCK _mapKey;
	friend class Contents;

private:
	ServerType _type;
	SOCKADDR_IN _serverAddr;
	int _workerThread;
	int _concurrent;
	int _nagleVal;
	int _maxUser;
	int _timeoutVal;
	int _initialTime;
	int _regularTime;

	std::int32_t _defaultContents = -1;

private:
	IStub* _stub = nullptr;
	IProxy* _proxy = nullptr;
};


#endif

