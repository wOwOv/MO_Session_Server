#ifndef __LOCKFREESTACK__
#define __LOCKFREESTACK__
#include <windows.h>
#include <new.h>
#include "TLSMemoryPool.h"

template <class DATA>
class LFStack
{
	struct Node
	{
		DATA _data;
		Node* _next;
	};

public:
	LFStack() :_top(nullptr),_nodepool(0), _size(0)
	{
	}

	~LFStack()
	{
		DATA temp;
		//노드풀로 모두 반환
		while (Pop(&temp))
		{
		}
	}

	bool Push(DATA data)
	{
		Node* newnode = _nodepool.Alloc();
		newnode->_data = data;
		unsigned long long temp = InterlockedIncrement16(&_key);
		unsigned long long countnode = (unsigned long long)newnode;
		countnode |= (temp << 48);
		Node* oldtop;
		do
		{
			oldtop = _top;
			newnode->_next = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)countnode , (__int64)oldtop) != (__int64)oldtop);
		InterlockedIncrement(&_size);
		return true;
	}

	bool Pop(DATA* data)
	{
		Node* oldtop;
		Node* newtop;
		Node* realadr;
		unsigned long long tempadr;

		unsigned long long ssize = InterlockedDecrement(&_size);
		if (ssize < 0)
		{
			InterlockedIncrement(&_size);
			return false;
		}

		do
		{
			oldtop = _top;
			tempadr = (unsigned long long)oldtop;
			tempadr <<= 16;
			tempadr >>= 16;
			realadr = (Node*)tempadr;
			newtop = realadr->_next;
			*data =realadr->_data;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);

		_nodepool.Free(realadr);
		return true;
	}

	unsigned long long GetSize()
	{
		return _size;
	}
	int GetNodeCapacity()
	{
		return _nodepool.GetCapacityCount();
	}

private:
	Node* _top;
	unsigned long long _size;
	short _key= 0;
	TlsMemoryPool<Node> _nodepool;
};

#endif