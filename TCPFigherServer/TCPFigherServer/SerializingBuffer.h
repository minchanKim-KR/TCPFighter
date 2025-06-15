#pragma once
#include <Windows.h>
#include <List>
#include <type_traits>

#define DEFAULT_SIZE 1460
#define SB_MAX_SIZE 5000
using namespace std;

class SerializingBuffer
{
public:
	SerializingBuffer(int size = 0) : _buffer_size(size), _is_failed(false)
	{
		_buffer = new BYTE[size];
	}
	~SerializingBuffer()
	{
		delete _buffer;
	}

	///////////////////////////////////////////////
	// Push : 여유 공간이 있으면 넣는다.
	///////////////////////////////////////////////

	template <typename T>
		requires std::is_fundamental_v<T>
	SerializingBuffer& operator<< (const T& data)
	{
		if (GetCapacity() < sizeof(T))
		{
			if (Resize(sizeof(T) + _buffer_size) == false)
			{
				_is_failed = true;
				return *this;
			}
		}

		*((T*)(_buffer + _tail)) = data;
		_tail += sizeof(T);

		return *this;
	}

	template <typename T>
		requires std::is_fundamental_v<T>
	int PutData(list<T>& items, int num)
	{
		int count;
		typename list<T>::iterator i;
		for (i = items.begin(), count = 0; i != items.end() && count < num;i++,count++)
		{
			*this << *i;
		}

		return count;
	}

	int	PutData(const char* data, int size)
	{
		if (size <= 0 || GetCapacity() < sizeof(size))
		{
			if (Resize(size + _buffer_size) == false)
			{
				_is_failed = true;
				return 0;
			}
		}

		memcpy(_buffer + _tail, data, size);
		_tail += size;

		return size;
	}

	///////////////////////////////////////////////
	// Pop : 보유 데이터가 더 적으면 빼지 않는다.
	// GetData는 최대한 데이터를 버퍼에 넣어준다. (데이터가 없으면 0)
	///////////////////////////////////////////////

	template <typename T>
		requires std::is_fundamental_v<T>
	SerializingBuffer& operator>> (T& data)
	{
		if (_head + sizeof(T) <= _tail)
		{
			data = *((T*)(_buffer + _head));
			_head += sizeof(T);
		}
		else
		{
			_is_failed = true;
		}

		return *this;
	}

	template <typename T>
		requires std::is_fundamental_v<T>
	int GetData(list<T>& items, int num)
	{
		int count;
		for (count = 0;count < num;count++)
		{
			T item;
			if (GetData((char*)&item, sizeof(item)) != sizeof(item))
			{
				_is_failed = true;
				break;
			}
			items.emplace_back(item);
		}

		return count;
	}

	int	GetData(char* data, int size)
	{
		if (size == 0 || GetDataSize() <= 0)
		{
			_is_failed = true;
			return 0;
		}

		int pop_size;
		if (GetDataSize() > size)
		{
			pop_size = size;
		}
		else
		{
			pop_size = GetDataSize();
		}

		memcpy(data, _buffer + _head, pop_size);
		_head += pop_size;

		return size;
	}

	///////////////////////////////////////////////
	// 보조 함수
	///////////////////////////////////////////////

	int	GetDataSize(void)
	{
		return _tail - _head;
	}
	int GetCapacity()
	{
		return _buffer_size - _tail;
	}
	int GetBufferSize()
	{
		return _buffer_size;
	}
	void Clear()
	{
		_head = 0;
		_tail = 0;
	}

	bool Failed()
	{
		return _is_failed;
	}
	
	// 축소를 허용하지 않음.
	bool Resize(int size)
	{
		if (size > SB_MAX_SIZE || size < _buffer_size)
			return false;

		BYTE* temp = new BYTE[size];
		memcpy(temp, _buffer, _buffer_size);
		delete _buffer;

		_buffer = temp;
		_buffer_size = size;

		return true;
	}

	///////////////////////////////////////////////
	// Direct Access
	///////////////////////////////////////////////
	BYTE* GetBufferPtr()
	{
		return _buffer;
	}

	int	MoveWritePos(int size)
	{
		if (GetCapacity() >= sizeof(size))
		{
			_tail += size;
			return size;
		}
		else
		{
			return 0;
		}
	}
	int	MoveReadPos(int size)
	{
		if (_head + size <= _buffer_size)
		{
			_head += size;
			return size;
		}
		else
		{
			return 0;
		}
	}

private:
	BYTE* _buffer;
	int _buffer_size;
	int _head = 0;
	int _tail = 0;
	bool _is_failed;
};