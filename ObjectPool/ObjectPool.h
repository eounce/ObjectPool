#ifndef _OBJECT_POOL_H_
#define _OBJECT_POOL_H_

#include <rpc.h>
#include <new.h>
#pragma comment(lib, "Rpcrt4.lib")

template <typename DATA> 
class ObjectPool
{
private:
	struct BLOCK_NODE
	{
		unsigned __int64 fCode;
		DATA data;
		unsigned __int64 bCode;
		BLOCK_NODE* next;
	};

	struct LOG
	{
		DWORD threadID;
		char action;	// Alloc 0 | Free 1
		BLOCK_NODE* preTop;
		BLOCK_NODE* curTop;
	};

	__int64 _capacity;			// 할당된 오브젝트 개수
	__int64 _useCount;		// 사용중인 오브젝트 개수
	bool _placementNew;		// 생성자 호출 여부
	bool _freeList;			// freeList 사용 여부
	__int64 _id;	// 락 프리 ABA 문제용 변수
	BLOCK_NODE* _pTop;
	BLOCK_NODE* _pRoot;
	UUID _uuid;

	LONG _index;
	LOG* _logs;

public:
	ObjectPool(bool freeList, int blockNum, bool placementNew = false) : _freeList(freeList), _capacity(blockNum), _placementNew(placementNew)
	{
		_id = 0;
		_useCount = 0;
		_pTop = nullptr;
		_logs = new LOG[100000000];
		UuidCreate(&_uuid);

		for (int i = 0; i < blockNum; i++)
		{
			BLOCK_NODE* pNode = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
			pNode->fCode = (unsigned __int64) _uuid.Data4;
			pNode->bCode = (unsigned __int64) _uuid.Data4;
			pNode->next = _pTop;

			if (!placementNew)
				new (&pNode->data) DATA;

			// 13 비트만 사용
			pNode = (BLOCK_NODE*)((__int64)pNode | _id << 47);
			_pTop = pNode;
			++_id;
		}

		_pRoot = _pTop;
	}

	virtual ~ObjectPool()
	{
		while (_pRoot)
		{
			BLOCK_NODE* pNode = (BLOCK_NODE*)((__int64)_pRoot & 0x7fffffffffff);
			_pRoot = pNode->next;

			if (!_placementNew)
				pNode->data.~DATA();
			free(pNode);
		}
	}

	DATA* Alloc(void)
	{
		BLOCK_NODE* pNode;

		// Free List 경우만 추가 할당
		if (_useCount >= _capacity)
		{
			if (!_freeList)
				return nullptr;

			pNode = new BLOCK_NODE;
			pNode->fCode = (unsigned __int64)_uuid.Data4;
			pNode->bCode = (unsigned __int64)_uuid.Data4;

			while (1)
			{
				BLOCK_NODE* pNewRoot = (BLOCK_NODE*)((__int64)pNode | _id << 47);
				BLOCK_NODE* root = _pRoot;
				pNode->next = root;

				if ((LONG64)root == InterlockedCompareExchange64((LONG64*)&_pRoot, (LONG64)pNewRoot, (LONG64)root))
				{
					InterlockedIncrement64(&_capacity);
					InterlockedIncrement64(&_useCount);
					InterlockedIncrement64(&_id);
					break;
				}
			}

			return &pNode->data;
		}

		while (1)
		{
			BLOCK_NODE* pTop = _pTop;
			BLOCK_NODE* pTopNode = (BLOCK_NODE*)((__int64)pTop & 0x7fffffffffff);

			if ((LONG64)pTop == InterlockedCompareExchange64((LONG64*)&_pTop, (LONG64)pTopNode->next, (LONG64)pTop))
			{
				pNode = pTopNode;
				InterlockedIncrement64(&_useCount);
				break;
			}
		}

		if (_placementNew)
			return new (&pNode->data) DATA;
		return &pNode->data;
	}

	bool Free(DATA* pData)
	{
		BLOCK_NODE* pNode = (BLOCK_NODE*)((char*)pData - 8);

		// 사용중인 노드가 있는지 체크
		if (_useCount <= 0)
			return FALSE;

		// 내가 할당한 노드가 맞는지 체크
		if (pNode->fCode != (unsigned __int64)_uuid.Data4 || pNode->bCode != (unsigned __int64)_uuid.Data4)
			return FALSE;

		if (_placementNew)
			pNode->data.~DATA();

		while (1)
		{
			BLOCK_NODE* pNewTop = (BLOCK_NODE*)((__int64)pNode | _id << 47);
			BLOCK_NODE* pTop = _pTop;
			pNode->next = pTop;

			if ((LONG64)pTop == InterlockedCompareExchange64((LONG64*)&_pTop, (LONG64)pNewTop, (LONG64)pTop))
			{
				InterlockedDecrement64(&_useCount);
				InterlockedIncrement64(&_id);
				break;
			}
		}
		
		return TRUE;
	}

	int	GetCapacityCount(void) { return _capacity; }
	int	GetUseCount(void) { return _useCount; }

private:
	void log(DWORD threadID, char action, BLOCK_NODE* preTop, BLOCK_NODE* curTop)
	{
		LONG index = InterlockedIncrement(&_index);
		if (index >= sizeof(_logs) / sizeof(LOG))
			return;

		_logs[index].threadID = threadID;
		_logs[index].action = action;
		_logs[index].preTop = preTop;
		_logs[index].curTop = curTop;
	}
};

#endif