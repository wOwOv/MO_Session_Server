#ifndef __SERIALBUFFER__
#define __SERIALBUFFER__

#include "TlsMemoryPool.h"



class SBuffer
{
protected:
	int	BufferSize;//버퍼 전체 사이즈
	char* buffer;
	int read;
	int write;

	int	DataSize;//사용중인 사이즈
public:
	int refcnt;

	char eFlag;
	SRWLOCK eKey;

	static TlsMemoryPool<SBuffer> BufPool;
private:


	friend class CPacket;
	friend class LanServer;
	friend class NetServer;
	friend class CoreServer;
	friend class CoreClient;
	friend class ContentsServer;


public:

	enum en_CBuffer
	{
		BUFFER_DEFAULT = 300		// 패킷의 기본 버퍼 사이즈.
	};

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	__forceinline SBuffer() :read(0), write(0), BufferSize(BUFFER_DEFAULT), DataSize(0), refcnt(0), eFlag(0)
	{
		//기본 버퍼 사이즈 할당
		buffer = (char*)malloc(BUFFER_DEFAULT);
		InitializeSRWLock(&eKey);
	}
	__forceinline SBuffer(int size) :read(0), write(0), BufferSize(size), DataSize(0), refcnt(0), eFlag(0)
	{
		//인자의 BufferSize만큼 할당
		buffer = (char*)malloc(BufferSize);
		InitializeSRWLock(&eKey);
	}

	__forceinline virtual	~SBuffer()
	{
		free(buffer);
	}


	//////////////////////////////////////////////////////////////////////////
	// 패킷 청소.
	//
	// Parameters: 없음.
	// Return: 없음.
	//////////////////////////////////////////////////////////////////////////
	__forceinline void	Clear(void)
	{
		read = 5;
		write = 5;
		DataSize = 0;
		eFlag = false;
	}


	//////////////////////////////////////////////////////////////////////////
	// 버퍼 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)패킷 버퍼 사이즈 얻기.
	//////////////////////////////////////////////////////////////////////////
	__forceinline int	GetBufferSize(void)
	{
		return BufferSize;
	}
	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 사이즈 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 데이타 사이즈.
	//////////////////////////////////////////////////////////////////////////
	__forceinline int		GetDataSize(void)
	{
		return DataSize;
	}



	//////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터 얻기.
	//
	// Parameters: 없음.
	// Return: (char *)버퍼 포인터.
	//////////////////////////////////////////////////////////////////////////
	__forceinline char* GetBufferPtr(void)
	{
		return &buffer[0];
	}
	__forceinline char* GetReadPtr(void)
	{
		return &buffer[read];
	}
	__forceinline char* GetWritePtr()
	{
		return &buffer[write];
	}

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	__forceinline int		MoveWritePos(int size)
	{
		write += size;
		DataSize += size;
		return size;
	}
	__forceinline int		MoveReadPos(int size)
	{
		read += size;
		DataSize -= size;
		return size;
	}


	//연산자 오버로딩
	__forceinline SBuffer& operator=(SBuffer& srcbuffer)
	{
		memcpy(GetBufferPtr(), srcbuffer.buffer, srcbuffer.GetBufferSize());
		read = srcbuffer.read;
		write = srcbuffer.write;
		DataSize = srcbuffer.DataSize;
		return *this;
	}

	__forceinline SBuffer& operator << (unsigned char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			unsigned char* temp = (unsigned char*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (signed char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	/* __forceinline* CBuffer& operator<<(BYTE value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			BYTE* temp = (BYTE*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}*/

	__forceinline SBuffer& operator << (short value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			short* temp = (short*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (unsigned short value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			unsigned short* temp = (unsigned short*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	/*__forceinline CBuffer& operator << (WORD value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			WORD* temp = (WORD*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}*/

	__forceinline SBuffer& operator << (unsigned int value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (int value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (long value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			long* temp = (long*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (float value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			float* temp = (float*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (DWORD value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			DWORD* temp = (DWORD*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}

	__forceinline SBuffer& operator << (__int64 value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			__int64* temp = (__int64*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator << (double value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			double* temp = (double*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// 빼기.	각 변수 타입마다 모두 만듬.
	//////////////////////////////////////////////////////////////////////////
	__forceinline SBuffer& operator>>(unsigned char &value)
	{
		if (DataSize >= sizeof(value))
		{
			unsigned char* temp = (unsigned char*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(signed char& value)
	{
		if (DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(char &value)
	{
		if (DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	/*__forceinline CBuffer& operator>>(BYTE& value)
	{
		if (DataSize >= sizeof(value))
		{
			BYTE* temp = (BYTE*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}*/

	__forceinline SBuffer& operator>>(short &value)
	{
		if (DataSize >= sizeof(value))
		{
			short* temp = (short*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(unsigned short &value)
	{
		if (DataSize >= sizeof(value))
		{
			unsigned short* temp = (unsigned short*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	/*__forceinline CBuffer& operator>>(WORD& value)
	{
		if (DataSize >= sizeof(value))
		{
			WORD* temp = (WORD*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}*/

	__forceinline SBuffer& operator >> (unsigned int value)
	{
		if (DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(int &value)
	{
		if (DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(long &value)
	{
		if (DataSize >= sizeof(value))
		{
			long* temp = (long*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(float &value)
	{
		if (DataSize >= sizeof(value))
		{
			float* temp = (float*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(DWORD& value)
	{
		if (DataSize >= sizeof(value))
		{
			DWORD* temp = (DWORD*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}

	__forceinline SBuffer& operator>>(__int64 &value)
	{
		if (DataSize >= sizeof(value))
		{
			__int64* temp = (__int64*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	__forceinline SBuffer& operator>>(double &value)
	{
		if (DataSize >= sizeof(value))
		{
			double* temp = (double*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}





	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	__forceinline int		GetData(char* dest, int size)
	{

		//사용중인 공간이 0일 때
		if (DataSize == 0)
		{
			return 0;
		}

		//사용중인 공간이 빼줄 데이터양보다 클 때
		if (DataSize >= size)
		{
			memcpy(dest, &buffer[read], size);
			read += size;
			DataSize -= size;
			return size;
		}
		//사용중인 공간이 빼줄 데이터양보다 작아서 그냥 있는거 다 빼줄때..?
		else
		{
			memcpy(dest, &buffer[read], DataSize);
			read += DataSize;
			DataSize = 0;
			return DataSize;
		}


	}

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	__forceinline int		PutData(char* src, int srcsize)
	{
		//넣을 수 있는 공간이 없을때
		if (DataSize == BufferSize)
		{
			return 0;
		}
		//넣을 수 있는 공간이 충분할 때
		if (BufferSize - DataSize >= srcsize)
		{
			memcpy(&buffer[write], src, srcsize);
			write += srcsize;
			DataSize += srcsize;
			return srcsize;
		}
		//넣을 수 잇는 공간이 부족할 때 최대한 넣어주기
		else
		{
			int size = BufferSize - DataSize;
			memcpy(&buffer[write], src, size);
			write += size;
			DataSize += size;
			return size;
		}
	}

	//참조카운트 n 증가
	__forceinline int AddRefcnt(int n)
	{
		int ret = InterlockedAdd((LONG*)&refcnt, n);

		return ret;
	}

	//참조카운트 n 감소
	__forceinline int SubRefcnt(int n)
	{
		int ret = InterlockedAdd((LONG*)&refcnt, -n);

		return ret;
	}

	//참조카운트 1 감소
	__forceinline long DecRefcnt()
	{
		long check = InterlockedDecrement((LONG*)&refcnt);
		if (check == 0)
		{
			BufPool.Free(this);
		}

		if (check < 0)
		{
			DebugBreak();
		}
		return check;
	}


	__forceinline unsigned long GetSBufferCapacity()
	{
		return BufPool.GetCapacity();
	}
	__forceinline unsigned long GetSBufferUsingCount()
	{
		return BufPool.GetUsingCount();
	}

private:
	__forceinline void ClearAtLServer(void)
	{
		read = 3;
		write = 3;
		DataSize = 0;
		eFlag = false;
	}
	__forceinline void ClearAtNServer(void)
	{
		read = 0;
		write = 0;
		DataSize = 0;
		eFlag = false;
	}


};




#endif