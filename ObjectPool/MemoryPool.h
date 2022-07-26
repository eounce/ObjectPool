#ifndef _MEMORY_POOL_H_
#define _MEMORY_POOL_H_

#include <rpc.h>
#include <new.h>
#pragma comment(lib, "Rpcrt4.lib")

template <typename DATA>
class MemoryPool
{
private:
	struct BLOCK_NODE
	{
		unsigned __int64 fCode;
		DATA data;
		unsigned __int64 bCode;
		BLOCK_NODE* next;
	};
	int _capacity; // �Ҵ�� ������Ʈ ����
	int _useCount = 0; // ������� ������Ʈ ����
	bool _placementFlag; // ������ ȣ�� ����
	BLOCK_NODE* _pTop = nullptr;
	UUID _uuid;

public:
	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	MemoryPool(int blockNum = 0, bool placementNew = false) : _capacity(blockNum), _placementFlag(placementNew)
	{
		UuidCreate(&_uuid);

		for (int i = 0; i < blockNum; i++)
		{
			BLOCK_NODE* node = (BLOCK_NODE*) malloc(sizeof(BLOCK_NODE));
			node->fCode = (unsigned __int64) _uuid.Data4;
			node->bCode = (unsigned __int64) _uuid.Data4;
			node->next = _pTop;
			_pTop = node;

			if (!_placementFlag)
				new (&node->data) DATA;
		}
	}

	virtual	~MemoryPool()
	{
		while (_pTop)
		{
			BLOCK_NODE* node = _pTop;
			_pTop = _pTop->next;

			if (!_placementFlag)
				node->data.~DATA();
			free(node);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		BLOCK_NODE* node;

		if (_useCount >= _capacity)
		{
			// ���� ������ ���� ������ �ѹ��� ������ ȣ��
			node = new BLOCK_NODE;
			node->fCode = (unsigned __int64) _uuid.Data4;
			node->bCode = (unsigned __int64) _uuid.Data4;

			_capacity++;
			_useCount++;

			return &node->data;
		}

		node = _pTop;
		_pTop = _pTop->next;
		_useCount++;

		// �̹� �ѹ� �����ڰ� ȣ��� ��ü�� Flag ���ο� ���� ����
		if (_placementFlag)
			return new (&node->data) DATA;
		return &node->data;
	}

	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool Free(DATA* pData)
	{
		BLOCK_NODE* node = (BLOCK_NODE*)((char*)pData - 8);

		// ������� ��尡 �ִ��� üũ
		if (_useCount < 0)
			return false;

		// ���� �Ҵ��� ��尡 �´��� üũ
		if (node->fCode != (unsigned __int64) _uuid.Data4 || node->bCode != (unsigned __int64) _uuid.Data4)
			return false;

		if (_placementFlag)
			node->data.~DATA();

		_useCount--;
		node->next = _pTop;
		_pTop = node;
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int	GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount(void) { return _useCount; }
};

#endif