#ifndef __MAXLOCKFREEQUEUECAS__
#define __MAXLOCKFREEQUEUECAS__

#include "TlsMemoryPool.h"



template <class DATA>
class MAXLFQueue
{
	const unsigned long long ADRMASK = 0x0000ffffffffffff;
	const unsigned long long TAGMASK = 0xffff000000000000;
	const unsigned long long MAKETAG = 0x0001000000000000;
private:
	struct Node
	{
		DATA _data;
		Node* _next;
	};

public:
	__forceinline MAXLFQueue(unsigned long maxsize=50000)
	{
		Node* dummy = _nodepool.Alloc();
		dummy->_next = nullptr;
		_head = dummy;
		_tail = dummy;

		_maxSize = maxsize;
	}

	__forceinline ~MAXLFQueue()
	{
		DATA temp;
		//노드풀로 모두 반환
		while (Dequeue(&temp))
		{
		}
	}
	MAXLFQueue(const MAXLFQueue&) = delete;
	MAXLFQueue& operator=(const MAXLFQueue&) = delete;
	MAXLFQueue(MAXLFQueue&&) = delete;
	MAXLFQueue& operator=(MAXLFQueue&&) = delete;

	__forceinline bool Enqueue(DATA data)
	{

		long size = InterlockedOr(&_size,0);
		if (size >= _maxSize)
		{
			return false;
		}

		Node* newnode = _nodepool.Alloc();
		newnode->_data = data;
		newnode->_next = nullptr;
		unsigned long long countnode = (unsigned long long)newnode;

		Node* oldtail;
		Node* realadr;

		unsigned long qsize;
		while (1)
		{
			oldtail = _tail;
			unsigned long long tag = (unsigned long long) oldtail;

			tag &= TAGMASK;
			tag += MAKETAG;
			countnode |= tag;

			unsigned long long tempadr = (unsigned long long)oldtail;
			tempadr &= ADRMASK;
			realadr = (Node*)tempadr;
			if (InterlockedCompareExchange64((__int64*)&_tail, (_int64)countnode, (__int64)oldtail) == (__int64)oldtail)
			{
				if (InterlockedCompareExchange64((__int64*)&realadr->_next, (__int64)countnode, (__int64)nullptr) == (__int64)nullptr)
				{
					qsize = InterlockedIncrement(&_size);
					break;
				}
			}

		}

		return true;
		//__int64 curtail = InterlockedCompareExchange64((__int64*)&_tail, (_int64)countnode, (__int64)oldtail);



	}

	__forceinline bool Dequeue(DATA* data)
	{
		Node* oldhead = nullptr;
		Node* newhead = nullptr;
		Node* realadr = nullptr;
		unsigned long long tempadr;
		unsigned long long tempadr2;

		long qsize = InterlockedDecrement(&_size);
		if (qsize < 0)
		{
			InterlockedIncrement(&_size);
			return false;
		}

		do
		{

			while (1)
			{
				Node* oldtail;
				Node* tailadr;

				oldtail = _tail;
				unsigned long long tempadr = (unsigned long long)oldtail;
				tempadr &= ADRMASK;
				tailadr = (Node*)tempadr;
				if (tailadr->_next == nullptr)
				{
					break;
				}
				InterlockedCompareExchange64((__int64*)&_tail, (__int64)tailadr->_next, (__int64)oldtail);
			}

			do
			{
				oldhead = _head;
				tempadr = (unsigned long long)oldhead;
				tempadr &= ADRMASK;
				realadr = (Node*)tempadr;
				newhead = realadr->_next;
			} while (newhead == nullptr);

			tempadr2 = (unsigned long long)newhead;
			tempadr2 &= ADRMASK;
			Node* datanode = (Node*)tempadr2;
			*data = datanode->_data;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)newhead, (__int64)oldhead) != (__int64)oldhead);




		_nodepool.Free(realadr);

		return true;
	}


	__forceinline long GetUsedSize()
	{
		return _size;
	}
private:
	Node* _head;
	Node* _tail;
	static TlsMemoryPool<Node> _nodepool;

	long _size = 0;

	long _maxSize = 50000;
};

template <class DATA>
TlsMemoryPool<typename MAXLFQueue<DATA>::Node> MAXLFQueue<DATA>::_nodepool(100, 500);




#endif
#pragma once
