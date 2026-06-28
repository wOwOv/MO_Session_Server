#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__

#include <new>
#include <windows.h>
#include <vector>
#include <memory>
#include <mutex>

static unsigned long Cookie = 0x01010100;

template <class DATA>
class TlsMemoryPool
{
private:
	static constexpr unsigned long long ADRMASK = 0x0000ffffffffffff;

	struct Node
	{
		DATA _data;
		Node* _next;
		Node* _chunknext;
	}typedef Chunk;

	struct TlsPool
	{
		Node* _nodelist;
		unsigned int _nodeCount;

		Node* _freelist;
		unsigned int _freeCount;

		int _storedCount;
	};

public:

	TlsMemoryPool(int BlockNum=500, int chunksize=500,bool PlacementNew = false, bool maxflag = false)
	{
		_tlsIndex = TlsAlloc();
		if (_tlsIndex == TLS_OUT_OF_INDEXES)
		{
			DebugBreak();
		}

		_pnFlag = PlacementNew;
		_maxCount = BlockNum;
		_maxFlag = maxflag;

		_chunkSize = chunksize;

		_capacity = 0;
		_usingCount = 0;

		_top = nullptr;
		_chunkCount=0;
		_key = 0;
	}
	~TlsMemoryPool()
	{
		
		TlsFree(_tlsIndex);
		
	}

	__forceinline DATA* Alloc(void)
	{
		Node* allocated;

		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			tpool = new TlsPool;
			tpool->_nodelist = nullptr;
			tpool->_nodeCount = 0;
			tpool->_freelist = nullptr;
			tpool->_freeCount = 0;
			tpool->_storedCount = 0;
			{
				std::lock_guard<std::mutex > lock(_tlsmutex);
				_tlsPools.emplace_back(tpool);
			}

			//생성자 호출한 상태로 들어가야함
			Node* chunk = nullptr;
			if (_pnFlag)
			{
				chunk = static_cast<Node*>(::operator new(sizeof(Node) * _maxCount));
			}
			else
			{
				chunk = new Node[_maxCount];
			}
			{
				std::lock_guard<std::mutex > lock(_mutex);
				_chunks.emplace_back(chunk, ChunkDeleter{ _pnFlag });
			}
			//100번->0번 순서, 0번노드는 과거 헤드를 next로 가져야함
			for (int i = 1; i < _maxCount; ++i)
			{
				Node* node = &chunk[i];
				node->_next = &chunk[i - 1];
			}
			Node* newhead = &chunk[_maxCount - 1];
			Node* lastnode = &chunk[0];
			lastnode->_next = nullptr;

			tpool->_nodelist = newhead;
			tpool->_nodeCount = _maxCount;
			tpool->_storedCount = tpool->_nodeCount+tpool->_freeCount;
			InterlockedAdd((long*) & _capacity, _maxCount);
			TlsSetValue(_tlsIndex, (LPVOID)tpool);
		}

		//사용하려고 보관하고 있는 내 노드가 있다면 
		if (tpool->_nodeCount > 0)
		{
			allocated = tpool->_nodelist;
			tpool->_nodelist = allocated->_next;
			//_pnFlag가 1이면 생성자 호출해서 나가야함
			if (_pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_nodeCount--;
		}
		//내 노드는 없고 반환하려고 보관해둔 노드가 있다면
		else if (tpool->_freeCount > 0)
		{
			allocated = tpool->_freelist;
			tpool->_freelist = allocated->_next;
			//_pnFlag가 1이면 생성자 호출해서 나가야함
			if ( _pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_freeCount--;
		}
		//공용풀에서 받아와야한다면
		else
		{
			Node* nodechunk = GetBucket();
			//공용풀에서 노드묶음 받아옴
			if (nodechunk != nullptr)
			{
				//내 노드리스트에 연결
				tpool->_nodelist = nodechunk;

				//노드 떼서 주기
				allocated = tpool->_nodelist;
				tpool->_nodelist = allocated->_next;
				//_pnFlag가 1이면 생성자 호출해서 나가야함
				if ( _pnFlag != 0)
				{
					new(&(allocated->_data)) DATA;
				}
				tpool->_nodeCount =  _chunkSize - 1;
			}
			//공용풀에서도 못 받아왔음
			else
			{
				//진짜 할당해서 줘야함
				ChunkAlloc();
				allocated = tpool->_nodelist;
				tpool->_nodelist = allocated->_next;
				//_pnFlag가 1이면 생성자 호출해서 나가야함
				if (_pnFlag != 0)
				{
					new(&(allocated->_data)) DATA;
				}
				tpool->_nodeCount--;
			}
			
		}
		tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

		InterlockedIncrement(&_usingCount);

		return &(allocated->_data);
	}




//반환된 Data*근처의 Node할당 주소를 찾아서 그걸 node들에 끼워넣어줘야함.
	__forceinline bool Free(DATA* data)
	{
		//노드할당 주소 찾기
		Node* retnode = (Node*)data;


		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			tpool = new TlsPool;
			tpool->_nodelist = nullptr;
			tpool->_nodeCount = 0;
			tpool->_freelist = nullptr;
			tpool->_freeCount = 0;
			tpool->_storedCount = 0;

			//생성자 호출한 상태로 들어가야함
			if (_pnFlag == 0)
			{
				for (int i = 0; i < _maxCount; i++)
				{
					Node* node = new Node;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}
			else//생성자 호출 없이 들어가야함
			{
				for (int i = 0; i < _maxCount; i++)
				{
					Node* node = (Node*)malloc(sizeof(Node));
					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}

			tpool->_nodeCount = _maxCount;
			tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

			TlsSetValue(_tlsIndex, (LPVOID)tpool);
		}

		//_pnFlag가 1이라면 소멸자 호출해서 보관
		if ( _pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		//nodelist 자리가 있다면
		if (tpool->_nodeCount <  _chunkSize)
		{
			retnode->_next = tpool->_nodelist;
			tpool->_nodelist = retnode;
			tpool->_nodeCount++;
		}
		//nodelist에 자리가 없다면 freelist로
		else
		{
			retnode->_next = tpool->_freelist;
			tpool->_freelist = retnode;
			tpool->_freeCount++;

			//freelist가 다 찼다면
			if (tpool->_freeCount ==  _chunkSize)
			{
				ReturnBucket(tpool->_freelist);
				tpool->_freelist = nullptr;
				tpool->_freeCount = 0;
			}
		}
		tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

		InterlockedDecrement(&_usingCount);

		return true;
	}


	__forceinline int	GetStoredNodeCount(void)
	{
		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			DebugBreak();
		}
		return tpool->_storedCount;
	}

	__forceinline unsigned long long GetChunkCount()
	{
		return _chunkCount;
	}

	__forceinline unsigned long GetCapacity()
	{
		return _capacity;
	}

	__forceinline unsigned long GetUsingCount()
	{
		return _usingCount;
	}
	
//Bucket 스택에서 얻어보고 반환할 때 쓰는 함수
private:
	__forceinline void ReturnBucket(Node* nodechunk)
	{
		Chunk* chunk = (Chunk*)nodechunk;
		unsigned long long tagadr = (unsigned long long)chunk;
		unsigned long long tag = InterlockedIncrement16(&_key);
		tagadr |= (tag << 48);
		Chunk* tagbucket = (Chunk*)tagadr;
		Chunk* oldtop;
		do
		{
			oldtop = _top;
			chunk->_chunknext = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)tagbucket, (__int64)oldtop) != (__int64)oldtop);
		InterlockedIncrement(&_chunkCount);
	}
	__forceinline Node* GetBucket()
	{
		Chunk* oldtop;
		Chunk* newtop;
		Chunk* realadr;
		unsigned long long tempadr;
		Node* retptr;
		long size = InterlockedDecrement(&_chunkCount);
		if (size < 0)
		{
			InterlockedIncrement(&_chunkCount);
			return nullptr;
		}

		do
		{
			oldtop = _top;
			tempadr = (unsigned long long)oldtop;
			tempadr &= ADRMASK;
			realadr = (Chunk*)tempadr;
			newtop = realadr->_chunknext;
			retptr = realadr;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);

		return retptr;
	}

private:
	long long _position;

	bool _pnFlag;
	int _maxCount;
	bool _maxFlag;

	unsigned int _chunkSize;
	
	unsigned long _capacity;
	unsigned long _usingCount;

	DWORD _tlsIndex = 0;
	
	Chunk* _top;							//Chunk Top
	unsigned long _chunkCount;				//보관 중인 chunk개수
	short _key;								//tag

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
			chunk = static_cast<Node*>(::operator new(sizeof(Node) * _chunkSize));
		}
		else
		{
			chunk = new Node[_chunkSize];
		}
		{
			std::lock_guard<std::mutex > lock(_mutex);
			_chunks.emplace_back(chunk, ChunkDeleter{ _pnFlag });
		}
		//100번->0번 순서, 0번노드는 과거 헤드를 next로 가져야함
		for (int i = 1; i < _chunkSize; ++i)
		{
			Node* node = &chunk[i];
			node->_next = &chunk[i - 1];
		}
		Node* newhead = &chunk[_chunkSize-1];
		Node* lastnode = &chunk[0];
		lastnode->_next = nullptr;

		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		tpool->_nodelist = newhead;
		tpool->_nodeCount = _chunkSize;
		InterlockedAdd((long*)&_capacity, _chunkSize);



	}

	std::mutex _mutex;
	std::vector<ChunkOwner> _chunks;

	std::mutex _tlsmutex;
	std::vector<std::unique_ptr<TlsPool>> _tlsPools;

};




#endif

