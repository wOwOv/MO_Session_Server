#ifndef __CPACKET__
#define __CPACKET__


class CPacket
{
	friend class LanServer;
	friend class NetServer;
	friend class CoreServer;
	friend class CoreClient;
	friend class SBuffer;
	friend class ContentsServer;

public:
	CPacket();

	CPacket(SBuffer* buf);

	CPacket(CPacket& buf);

	CPacket(CPacket* buf);


	~CPacket();

	CPacket& operator=(const CPacket& buf);

	CPacket* MakePacketPtr();
	static CPacket* GetPacketPtr(CPacket* packet);

	void	Clear(void);

	// 버퍼 사이즈 얻기.
	int	GetBufferSize(void);

	// 현재 사용중인 사이즈 얻기.
	int		GetDataSize(void);



	// 버퍼 포인터 얻기.
	char* GetBufferPtr(void);
	char* GetReadPtr(void);
	char* GetWritePtr(void);

	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);


	//연산자 오버로딩
	CPacket& operator=(CPacket& srcbuffer);

	CPacket& operator << (unsigned char value);
	CPacket& operator << (signed char value);
	CPacket& operator << (char value);
	//CPacket& operator<<(BYTE value);

	CPacket& operator << (short value);
	CPacket& operator << (unsigned short value);
	//CPacket& operator << (WORD value);

	CPacket& operator << (unsigned int value);
	CPacket& operator << (int value);
	CPacket& operator << (long value);
	CPacket& operator << (float value);
	//CPacket& operator << (DWORD value);

	CPacket& operator << (__int64 value);
	CPacket& operator << (double value);

	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator>>(unsigned char& value);
	CPacket& operator>>(signed char& value);
	CPacket& operator>>(char& value);
	//CPacket& operator>>(BYTE& value);

	CPacket& operator>>(short& value);
	CPacket& operator>>(unsigned short& value);
	//CPacket& operator>>(WORD& value);

	CPacket& operator >> (unsigned int value);
	CPacket& operator>>(int& value);
	CPacket& operator>>(long& value);
	CPacket& operator>>(float& value);
	//CPacket& operator>>(DWORD& value);

	CPacket& operator>>(__int64& value);
	CPacket& operator>>(double& value);


	// 데이타 얻기.
	int		GetData(char* dest, int size);

	// 데이타 삽입.
	int		PutData(char* src, int srcsize);



private:
	SBuffer* SBuf;

	friend class ChatServer;

};

#endif
