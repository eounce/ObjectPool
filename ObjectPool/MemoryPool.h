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
	int _capacity; // 할당된 오브젝트 개수
	int _useCount = 0; // 사용중인 오브젝트 개수
	bool _placementFlag; // 생성자 호출 여부
	BLOCK_NODE* _pTop = nullptr;
	UUID _uuid;

public:
	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
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
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		BLOCK_NODE* node;

		if (_useCount >= _capacity)
		{
			// 새로 생성된 경우는 무조건 한번은 생성자 호출
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

		// 이미 한번 생성자가 호출된 객체는 Flag 여부에 따라 결정
		if (_placementFlag)
			return new (&node->data) DATA;
		return &node->data;
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool Free(DATA* pData)
	{
		BLOCK_NODE* node = (BLOCK_NODE*)((char*)pData - 8);

		// 사용중인 노드가 있는지 체크
		if (_useCount < 0)
			return false;

		// 내가 할당한 노드가 맞는지 체크
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
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int	GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount(void) { return _useCount; }
};

#endif