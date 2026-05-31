#include "CPacket.h"
#include "SerialBuffer.h"
#include <list>


CPacket::CPacket()
{
	SBuf = SBuffer::BufPool.Alloc();
	SBuf->AddRefcnt(1);
	SBuf->Clear();
}

CPacket::CPacket(SBuffer* buf)
{
	SBuf = buf;
	SBuf->AddRefcnt(1);
}

CPacket::CPacket(CPacket& buf)
{
	SBuf = buf.SBuf;
	SBuf->AddRefcnt(1);
}

CPacket::CPacket(CPacket* buf)
{
	SBuf = (SBuffer*)buf;
}


CPacket::~CPacket()
{
	SBuf->DecRefcnt();
}

CPacket& CPacket::operator=(const CPacket& buf)
{
	SBuf = buf.SBuf;
	SBuf->AddRefcnt(1);
	return *this;
}

CPacket* CPacket::MakePacketPtr()
{
	this->SBuf->AddRefcnt(1);
	return (CPacket*)this->SBuf;
}

CPacket* CPacket::GetPacketPtr(CPacket* packet)
{
	SBuffer* temp = (SBuffer*)packet;
	return packet;
}


CPacket& CPacket::operator<<(unsigned char value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		unsigned char* temp = (unsigned char*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(signed char value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		unsigned char* temp = (unsigned char*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(char value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		char* temp = (char*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(short value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		short* temp = (short*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(unsigned short value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		unsigned short* temp = (unsigned short*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(unsigned int value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		int* temp = (int*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(int value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		int* temp = (int*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(long value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		long* temp = (long*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(float value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		float* temp = (float*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

//CPacket& CPacket::operator<<(DWORD value)
//{
//	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
//	{
//		DWORD* temp = (DWORD*)SBuf->GetWritePtr();
//		*temp = value;
//		SBuf->MoveWritePos(sizeof(value));
//	}
//	return *this;
//}

CPacket& CPacket::operator<<(__int64 value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		__int64* temp = (__int64*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator<<(double value)
{
	if (SBuf->GetBufferSize() - SBuf->GetDataSize() >= sizeof(value))
	{
		double* temp = (double*)SBuf->GetWritePtr();
		*temp = value;
		SBuf->MoveWritePos(sizeof(value));
	}
	return *this;
}




CPacket& CPacket::operator>>(unsigned char& value)
{
	if (SBuf->GetDataSize()>= sizeof(value))
	{
		unsigned char* temp = (unsigned char*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(signed char& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		char* temp = (char*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(char& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		char* temp = (char*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(short& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		short* temp = (short*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(unsigned short& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		unsigned short* temp = (unsigned short*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(unsigned int value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		int* temp = (int*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(int& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		int* temp = (int*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(long& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		long* temp = (long*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(float& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		float* temp = (float*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

//CPacket& CPacket::operator>>(DWORD& value)
//{
//	if (SBuf->GetDataSize() >= sizeof(value))
//	{
//		DWORD* temp = (DWORD*)SBuf->GetReadPtr();
//		value = *temp;
//		SBuf->MoveReadPos(sizeof(value));
//	}
//	return *this;
//}

CPacket& CPacket::operator>>(__int64& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		__int64* temp = (__int64*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

CPacket& CPacket::operator>>(double& value)
{
	if (SBuf->GetDataSize() >= sizeof(value))
	{
		double* temp = (double*)SBuf->GetReadPtr();
		value = *temp;
		SBuf->MoveReadPos(sizeof(value));
	}
	return *this;
}

int CPacket::GetData(char* dest, int size)
{
	return SBuf->GetData(dest,size);
}

int CPacket::PutData(char* src, int srcsize)
{
	return SBuf->PutData(src,srcsize);
}

void CPacket::Clear(void)
{
	SBuf->Clear();
}

int CPacket::GetBufferSize(void)
{
	return SBuf->GetBufferSize();
}

int CPacket::GetDataSize(void)
{
	return SBuf->GetDataSize();
}

char* CPacket::GetBufferPtr(void)
{
	return SBuf->GetBufferPtr();
}
char* CPacket::GetReadPtr(void)
{
	return SBuf->GetReadPtr();
}
char* CPacket::GetWritePtr(void)
{
	return SBuf->GetWritePtr();
}

int CPacket::MoveWritePos(int iSize)
{
	return SBuf->MoveWritePos(iSize);
}

int CPacket::MoveReadPos(int iSize)
{
	return SBuf->MoveReadPos(iSize);
}
