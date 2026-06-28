#ifndef __RINGBUFFER__
#define __RINGBUFFER__



static constexpr int DEFAULT_BUFFER_SIZE = 5000;


class RingBuffer
{
public:
	char* _buffer;
	int _front;
	int _rear;
	int _size;

public:
	__forceinline RingBuffer() :_front(0), _rear(0), _size(DEFAULT_BUFFER_SIZE)
	{
		_buffer = (char*)malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
	}

	__forceinline RingBuffer(int buffersize) :_front(0), _rear(0), _size(buffersize)
	{
		_buffer = (char*)malloc(sizeof(char) * buffersize);
	}
	__forceinline ~RingBuffer()
	{
		free(_buffer);
	}

	__forceinline void Resize(int size)
	{
		int deqret;
		char* temp = new char[_size];
		int usedsize = GetUsedSize();
		deqret = Dequeue(temp, usedsize);
		if (deqret != usedsize)
		{
			DebugBreak();
		}
		free(_buffer);
		_buffer = new char[size];
		memcpy(_buffer, temp, deqret);
		_front = 0;
		_rear = deqret;
	}
	
	__forceinline int GetBufferSize()
	{
		return _size - 1;
	}
	__forceinline int GetUsedSize()
	{
		int fixedfront = _front;
		int fixedrear = _rear;

		if (fixedfront <= fixedrear)
		{
			return fixedrear - fixedfront;
		}
		else
		{
			return _size - (fixedfront - fixedrear);
		}
	}
	__forceinline int GetFreeSize()
	{
		return (_size - GetUsedSize() - 1);
	}

	__forceinline int Enqueue(const char* data, int size)

	{
		int freesize = GetFreeSize();
		if (freesize >= size)
		{
			if (_rear < _front)
			{
				memcpy(&_buffer[_rear], data, size);
				_rear = (_rear + size) % _size;
				return size;
			}
			else
			{
				if ((_size - _rear) >= size)
				{
					memcpy(&_buffer[_rear], data, size);
					_rear = (_rear + size) % _size;
					return size;
				}

				int remainbyte = size;
				int movebyte = _size - _rear;
				memcpy(&_buffer[_rear], data, movebyte);
				data += movebyte;
				remainbyte -= movebyte;
				memcpy(&_buffer[0], data, remainbyte);
				_rear = (_rear + size) % _size;
				return size;
			}
		}
		else
		{
			return 0;
		}
	}
	__forceinline int Dequeue(char* dest, int size)
	{
		if (GetUsedSize() >= size)
		{
			if (_front < _rear)
			{
				memcpy(dest, &_buffer[_front], size);
				_front = (_front + size) % _size;
				return size;
			}
			else
			{
				if ((_size - _front) >= size)
				{
					memcpy(dest, &_buffer[_front], size);
					_front = (_front + size) % _size;
					return size;
				}
				int remainbyte = size;
				int movebyte = _size - _front;
				memcpy(dest, &_buffer[_front], movebyte);
				dest += movebyte;
				remainbyte -= movebyte;
				memcpy(dest, &_buffer[0], remainbyte);
				_front = (_front + size) % _size;
				return size;
			}
		}
		else
		{
			//return 0;

			int usedsize = GetUsedSize();
			if (usedsize == 0)
			{
				return 0;
			}
			if (_front < _rear)
			{
				memcpy(dest, &_buffer[_front], usedsize);
				_front = (_front + usedsize) % _size;
				return usedsize;
			}
			else
			{
				int remainbyte = usedsize;
				int movebyte = _size - _front;
				memcpy(dest, &_buffer[_front], movebyte);
				dest += movebyte;
				remainbyte -= movebyte;
				memcpy(dest, &_buffer[0], remainbyte);
				_front = (_front + usedsize) % _size;
				return usedsize;
			}
		}
	}
	__forceinline int Peek(char* dest, int size)

	{
		if (GetUsedSize() >= size)
		{
			if (_front < _rear)
			{
				memcpy(dest, &_buffer[_front], size);
				return size;
			}
			else
			{
				if ((_size - _front) >= size)
				{
					memcpy(dest, &_buffer[_front], size);
					return size;
				}
				int remainbyte = size;
				int movebyte = _size - _front;
				memcpy(dest, &_buffer[_front], movebyte);
				dest += movebyte;
				remainbyte -= movebyte;
				memcpy(dest, &_buffer[0], remainbyte);
				return size;
			}
		}
		else
		{
			//return 0;

			int usedsize = GetUsedSize();
			if (usedsize == 0)
			{
				return 0;
			}
			if (_front < _rear)
			{
				memcpy(dest, &_buffer[_front], usedsize);
				return usedsize;
			}
			else
			{
				int remainbyte = usedsize;
				int movebyte = _size - _front;
				memcpy(dest, &_buffer[_front], movebyte);
				dest += movebyte;
				remainbyte -= movebyte;
				memcpy(dest, &_buffer[0], remainbyte);
				return usedsize;
			}
		}
	}
	
	__forceinline void ClearBuffer()
	{
		_front = _rear = 0;
	}
	
	__forceinline int DirectEnqueueSize()
	{
		int fixedfront = _front;
		int fixedrear = _rear;

		if (fixedrear < fixedfront)
		{
			return fixedfront - fixedrear - 1;
		}
		else
		{
			if (fixedfront == 0)
			{
				return _size - fixedrear - 1;
			}
			else
			{
				return _size - fixedrear;
			}

		}

	}
	__forceinline int DirectDequeueSize()
	{
		int fixedfront = _front;
		int fixedrear = _rear;

		if (fixedfront < fixedrear)
		{
			return fixedrear - fixedfront;
		}
		else
		{
			return _size - fixedfront;
		}
	}

	__forceinline int MoveRear(int size)
	{
		if (this->GetFreeSize() >= size)
		{
			_rear = (_rear + size) % _size;
			return size;
		}
		else
		{
			if (_rear <= _front)
			{
				int byte = _front - _rear - 1;
				_rear = _front - 1;
				return byte;
			}
			else
			{
				int byte = (_size - _rear) + _front - 1;
				_rear = _front - 1;
				return byte;
			}
		}
	}
	__forceinline int MoveFront(int size)

	{
		if (this->GetUsedSize() >= size)
		{
			_front = (_front + size) % _size;
			return size;
		}
		else
		{
			if (_front <= _rear)
			{
				int byte = _rear - _front;
				_front = _rear;
				return byte;
			}
			else
			{
				int byte = (_size - _front) + _rear;
				_front = _rear;
				return byte;
			}
		}
	}
	__forceinline char* GetFrontBufferPtr()
	{
		return &_buffer[_front];
	}
	__forceinline char* GetRearBufferPtr()
	{
		return &_buffer[_rear];
	}
	__forceinline char* GetStartBufferPtr()
	{
		return &_buffer[0];
	}

private:
	//int MoveRear(int size);
	//int MoveFront(int size);
	//char* GetFrontBufferPtr();
	//char* GetRearBufferPtr();
	//char* GetStartBufferPtr();


};


#endif