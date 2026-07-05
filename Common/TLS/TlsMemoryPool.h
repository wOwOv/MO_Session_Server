#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__
#include <new.h>
#include <windows.h>

static unsigned long Cookie = 0x01010100;

template <class DATA>
class TlsMemoryPool
{
private:
	const unsigned long long ADRMASK = 0x0000ffffffffffff;

	struct Node
	{
		DATA _data;
		Node* _next;
		Node* _bunchnext;
	}typedef Bunch;

	struct TlsPool
	{
		Node* _nodelist;
		unsigned int _nodeCount;

		Node* _freelist;
		unsigned int _freeCount;

		int _storedCount;
	};

public:

	TlsMemoryPool(int BlockNum=500, int bunchsize=500,bool PlacementNew = false, bool maxflag = false)
	{
		_tlsIndex = TlsAlloc();
		if (_tlsIndex == TLS_OUT_OF_INDEXES)
		{
			DebugBreak();
		}

		_pnFlag = PlacementNew;
		_maxCount = BlockNum;
		_maxFlag = maxflag;

		_bunchSize = bunchsize;

		_capacity = 0;
		_usingCount = 0;

		_top = nullptr;
		_bunchCount=0;
		_key = 0;
	}
	~TlsMemoryPool()
	{
		
		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool != nullptr)
		{

		Node* node = tpool->_nodelist;
		Node* temp = node->_next;
			while (1)
			{
				if (node != nullptr)
				{
					temp = node->_next;
					delete node;
					node = temp;
				}
				else
				{
					break;
				}
			}
		

		node = tpool->_freelist;
		temp = node->_next;
		while (1)
		{
			if (node != nullptr)
			{
				temp = node->_next;
				delete node;
				node = temp;
			}
			else
			{
				break;
			}
		}

		TlsFree(_tlsIndex);
		}
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
			Node* nodebunch = GetBucket();
			//공용풀에서 노드묶음 받아옴
			if (nodebunch != nullptr)
			{
				//내 노드리스트에 연결
				tpool->_nodelist = nodebunch;

				//노드 떼서 주기
				allocated = tpool->_nodelist;
				tpool->_nodelist = allocated->_next;
				//_pnFlag가 1이면 생성자 호출해서 나가야함
				if ( _pnFlag != 0)
				{
					new(&(allocated->_data)) DATA;
				}
				tpool->_nodeCount =  _bunchSize - 1;
			}
			//공용풀에서도 못 받아왔음
			else
			{
				//진짜 할당해서 줘야함
				allocated = new Node;
				InterlockedIncrement(&_capacity);
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
		if (tpool->_nodeCount <  _bunchSize)
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
			if (tpool->_freeCount ==  _bunchSize)
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

	__forceinline unsigned long long GetBunchCount()
	{
		return _bunchCount;
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
	__forceinline void ReturnBucket(Node* nodebunch)
	{
		Bunch* bunch = (Bunch*)nodebunch;
		unsigned long long tagadr = (unsigned long long)bunch;
		unsigned long long tag = InterlockedIncrement16(&_key);
		tagadr |= (tag << 48);
		Bunch* tagbucket = (Bunch*)tagadr;
		Bunch* oldtop;
		do
		{
			oldtop = _top;
			bunch->_bunchnext = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)tagbucket, (__int64)oldtop) != (__int64)oldtop);
		InterlockedIncrement(&_bunchCount);
	}
	__forceinline Node* GetBucket()
	{
		Bunch* oldtop;
		Bunch* newtop;
		Bunch* realadr;
		unsigned long long tempadr;
		Node* retptr;
		long size = InterlockedDecrement(&_bunchCount);
		if (size < 0)
		{
			InterlockedIncrement(&_bunchCount);
			return nullptr;
		}

		do
		{
			oldtop = _top;
			tempadr = (unsigned long long)oldtop;
			tempadr &= ADRMASK;
			realadr = (Bunch*)tempadr;
			newtop = realadr->_bunchnext;
			retptr = realadr;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);

		return retptr;
	}

private:
	long long _position;

	bool _pnFlag;
	int _maxCount;
	bool _maxFlag;

	unsigned int _bunchSize;
	
	unsigned long _capacity;
	unsigned long _usingCount;

	DWORD _tlsIndex = 0;
	
	Bunch* _top;							//Bunch Top
	unsigned long _bunchCount;				//보관 중인 Bunch개수
	short _key;								//tag
};




#endif

