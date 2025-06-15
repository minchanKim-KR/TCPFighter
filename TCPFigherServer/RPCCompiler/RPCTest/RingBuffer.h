#pragma once
#include <memory>
#include <iostream>

#define MAX_RINGBUF_SIZE 5000
class RingBuffer
{
public:
	RingBuffer(int size = 1193)
	{
		_buffer = new char[size];
		_size = size;
		_dataSize = 0;

		_head = 0;
		_tail = 0;
	}

	~RingBuffer()
	{
		delete _buffer;
	}
	bool IsFull() const
	{
		return _dataSize == _size;
	}
	bool IsEmpty() const
	{
		return _dataSize == 0;
	}

	int GetDataSize()
	{
		return _dataSize;
	}

	int GetCapacity()
	{
		return _size - _dataSize;
	}
	bool Resize(int size)
	{
		if (MAX_RINGBUF_SIZE < size)
			return false;

		std::cout << "Resize : " << size << std::endl;

		char* temp = new char[size];
		int temp_dataSize = Peek(temp, size);
		delete _buffer;

		_buffer = temp;
		_dataSize = temp_dataSize;
		_size = size;

		_head = 0;
		_tail = temp_dataSize % size;

		return true;
	}

	int Enqueue(const char* data, int size)
	{
		if (size < 1)
			return 0;

		// ��� �����͸� ���� ��, ���� ������ ������ ret 0 or Resize
		if (size > _size - _dataSize)
		{
			//Resize(_size + size);
			return 0;
		}

		int direct = DirectEnqueueSize();

		// 
		if (size > direct)
		{
			// �׻� DirectEnqueueSize�� > 0
			memcpy(_buffer + _tail, data, direct);
			memcpy(_buffer, data + direct, size - direct);
		}
		else
			memcpy(_buffer + _tail, data, size);

		_dataSize += size;
		_tail = (_tail + size) % _size;

		return size;
	}

	int Peek(char* data, int size) const
	{
		if (size < 1 || _dataSize == 0)
			return 0;

		// ���� ������ ũ��
		int data_num;
		int direct = DirectDequeueSize();

		// ���� �����Ͱ� ������ ������ �ִ� ��ŭ��.
		if (_dataSize >= size)
			data_num = size;
		else
			data_num = _dataSize;

		if (data_num > direct)
		{
			// �����Ͱ� �ִٸ� �׻� direct�� > 0
			memcpy(data, _buffer + _head, direct);
			memcpy(data + direct, _buffer, data_num - direct);
		}
		else
			memcpy(data, _buffer + _head, data_num);

		return data_num;
	}

	int Dequeue(char* data, int size)
	{
		if (size < 1 || _dataSize == 0)
			return 0;

		// ���� ������ ũ��
		int data_num;
		int direct = DirectDequeueSize();

		// ���� �����Ͱ� ������ ������ �ִ� ��ŭ��.
		if (_dataSize >= size)
			data_num = size;
		else
			data_num = _dataSize;

		if (data_num > direct)
		{
			// �����Ͱ� �ִٸ� �׻� direct�� > 0
			memcpy(data, _buffer + _head, direct);
			memcpy(data + direct, _buffer, data_num - direct);
		}
		else
			memcpy(data, _buffer + _head, data_num);

		_dataSize -= data_num;
		_head = (_head + data_num) % _size;

		return data_num;
	}

	void ClearBuffer()
	{
		_head = 0;
		_tail = 0;
		_dataSize = 0;
	}

	/////////////////////////////////////////////////////////////////////////
	// Head �տ� ������ ���� �ϴ� ���
	// Head�� index 0�� ���� ���� �ƴ� ��, DirectEnqueueSize�� �޶�����.
	// �ش� �κп��� �б⸦ Ÿ�� ���� �ʾ�, ������� ������.
	/////////////////////////////////////////////////////////////////////////

	// _tail ���ķ� ä�� �� �ִ� ��
	int	DirectEnqueueSize(void) const
	{
		if (IsFull())
		{
			return 0;
		}

		if (_head <= _tail)
		{
			return _size - _tail;
		}
		else
		{
			return _head - _tail;
		}
	}

	// _head ���ķ� �������� �ʴ� ������ ũ��
	int	DirectDequeueSize(void) const
	{
		if (IsEmpty())
		{
			return 0;
		}

		if (_head < _tail)
		{
			return _tail - _head;
		}
		else
		{
			return _size - _head;
		}
	}

	int	MoveTail(int iSize)
	{
		_tail = (_tail + iSize) % _size;
		return iSize;
	}
	int	MoveHead(int iSize)
	{
		_head = (_head + iSize) % _size;
		return iSize;
	}

	char* GetHeadPtr(void)
	{
		return &_buffer[_head];
	}

	char* GetTailPtr(void)
	{
		return &_buffer[_tail];
	}

// HACK : �׽�Ʈ ������ PRIVATE
public:
	char* _buffer;
	// index
	int _head;
	int _tail;
	// ���� ũ�� (�Ҵ� �޸� ũ��)
	int _size;
public:
	// ���� ����Ʈ ũ�� 
	int _dataSize;
};