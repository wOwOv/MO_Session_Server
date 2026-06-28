#ifndef  __MEMORYPOOL__
#define  __MEMORYPOOL__
#include <new>
#include <windows.h>
#include <vector>
#include <memory>
#include <mutex>

static int key = 0xaaaa;

template <class DATA>
class MemoryPool
{

private:
	static constexpr unsigned long long ADRMASK=0x0000ffffffffffff;
	static constexpr unsigned long long TAGMASK=0xffff000000000000;
	static constexpr unsigned long long MAKETAG=0x0001000000000000;

private:
	struct Node
	{
		DATA _data;
		Node* _next;
	};
	
public:
	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	MemoryPool(int BlockNum=500, bool PlacementNew = false, bool maxflag = false)
	{
		_head = nullptr;
		_pnFlag = PlacementNew;

		_maxFlag = maxflag;

		_key = 0;

		_capacity = BlockNum;
		_usingCount = 0;

		if (BlockNum == 0)
		{
			return;
		}

		//생성자 호출한 상태로 들어가야함
		Node* chunk = nullptr;
		if (_pnFlag)
		{
			chunk = static_cast<Node*>(::operator new(sizeof(Node) * 100));
		}
		else
		{
			chunk = new Node[100];
		}
		_chunks.emplace_back(chunk, ChunkDeleter{ _pnFlag });
		for (int i = 0; i < 100; ++i)
		{
			Node* node = &chunk[i];
			node->_next = _head;
			_head = node;
		}
	

	}
	virtual	~MemoryPool()
	{

	}
	MemoryPool(const MemoryPool&) = delete;
	MemoryPool& operator=(const MemoryPool&) = delete;
	MemoryPool(MemoryPool&&) = delete;
	MemoryPool& operator=(MemoryPool&&) = delete;
	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	__forceinline DATA* Alloc(void)
	{
		Node* allocated=nullptr;
		Node* oldhead=nullptr;
		Node* newhead=nullptr;
		Node* realadr=nullptr;
		unsigned long long tempadr;
		do
		{
			oldhead = _head;
			if (oldhead == nullptr)
			{
				if (_maxFlag == true&& _maxcount >= _capacity)
				{
						return nullptr;
				}
				ChunkAlloc();
				continue;
			}
			tempadr = (unsigned long long)oldhead;
			tempadr &= ADRMASK;
			realadr = (Node*)tempadr;
			newhead = realadr->_next;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)newhead, (__int64)oldhead) != (__int64)oldhead);
		allocated = realadr;
		//_pnFlag가 1이면 생성자 호출해서 나가야함
		if (_pnFlag != 0)
		{
			new(&(allocated->_data)) DATA;
		}

		InterlockedIncrement(&_usingCount);

		return &(allocated->_data);
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////반환된 Data*근처의 Node할당 주소를 찾아서 그걸 node들에 끼워넣어줘야함.
	__forceinline bool Free(DATA* data)
	{
		//노드할당 주소 찾기
		Node* retnode = (Node*)data;


		//_pnFlag가 1이라면 소멸자 호출해서 보관
		if (_pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		unsigned long long countnode = (unsigned long long)retnode;
		countnode &= ADRMASK;
		Node* oldhead;
		do
		{
			oldhead = _head;
			unsigned long long tag = (unsigned long long)oldhead;
			tag &= TAGMASK;
			tag += MAKETAG;
			countnode |= tag;
			retnode->_next = oldhead;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)countnode, (__int64)oldhead) != (__int64)oldhead);

		InterlockedDecrement(&_usingCount);

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	__forceinline int	GetCapacityCount(void)
	{
		return _capacity;
	}

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	__forceinline int	GetUseCount(void)
	{
		return _usingCount;
	}


	__forceinline void SetMaxCount(int maxcount)
	{
		_maxcount = maxcount;
	}

	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.


private:
	Node* _head;
	alignas(64) unsigned long _capacity;
	unsigned long _usingCount;
	bool _pnFlag;

	long long _position;

	int _maxcount;
	bool _maxFlag;

	short _key;

private:
	struct ChunkDeleter
	{
		bool _placementNew = false;

		void operator()(Node* chunk) const noexcept
		{
			if (chunk == nullptr)
			{
				return;
			}

			if (_placementNew)
			{
				::operator delete(chunk);
			}
			else
			{
				delete[] chunk;
			}
		}
	};
	using ChunkOwner = std::unique_ptr<Node, ChunkDeleter>;

	void ChunkAlloc()
	{
		Node* chunk = nullptr;
		if (_pnFlag)
		{
			chunk = static_cast<Node*>(::operator new(sizeof(Node) * 100));
		}
		else
		{
			chunk = new Node[100];
		}
		{
			std::lock_guard<std::mutex > lock(_mutex);
			_chunks.emplace_back(chunk, ChunkDeleter{ _pnFlag });
		}
		
		//100번->0번 순서, 0번노드는 과거 헤드를 next로 가져야함
		for (int i = 1; i < 100; ++i)
		{
			Node* node = &chunk[i];
			node->_next = &chunk[i-1];
		}
		Node* newhead = &chunk[99];
		Node* lastnode = &chunk[0];

		Node* oldhead;
		unsigned long long countnode;
		do
		{
			countnode = (unsigned long long)newhead;
			oldhead = _head;
			unsigned long long tag = (unsigned long long)oldhead;
			tag &= TAGMASK;
			tag += MAKETAG;
			countnode |= tag;
			lastnode->_next = oldhead;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)countnode, (__int64)oldhead) != (__int64)oldhead);
		InterlockedAdd((LONG*) & _capacity, 100);


	}

	std::mutex _mutex;
	std::vector<ChunkOwner> _chunks;
};









#endif
#pragma once
