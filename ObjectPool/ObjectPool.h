#ifndef _OBJECT_POOL_H_
#define _OBJECT_POOL_H_

#define LOGGING

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

	__int64 _capacity;		// �Ҵ�� ������Ʈ ����
	__int64 _useCount;		// ������� ������Ʈ ����
	bool _placementNew;		// ������ ȣ�� ����
	bool _freeList;			// freeList ��� ����
	unsigned __int64 _id;	// �� ���� ABA ������ ����
	BLOCK_NODE* _pTop;
	BLOCK_NODE* _pRoot;
	UUID _uuid;

#ifdef LOGGING
	enum Action { ALLOC, FREE };
	struct LOG
	{
		DWORD threadID;		// ������ ID
		Action action;		// Alloc 0 | Free 1
		UINT64 preTop;		// ���� top
		UINT64 curTop;		// ���� top
		UINT64 curTopNext;	// ���� top�� next
	};

	ULONG _index = ULONG_MAX;
	ULONG _logBufferSize = 0xfffff;
	LOG* _logBuffer = new LOG[_logBufferSize];

	__forceinline void log(DWORD threadID, Action action, UINT64 preTop, UINT64 curTop)
	{
		ULONG index = InterlockedIncrement(&_index);
		index = _logBufferSize & index;

		_logBuffer[index].threadID = threadID;
		_logBuffer[index].action = action;
		_logBuffer[index].preTop = preTop;
		_logBuffer[index].curTop = curTop;
	}
#endif

public:
	ObjectPool(bool freeList, int blockNum, bool placementNew = false) : _freeList(freeList), _capacity(blockNum), _placementNew(placementNew)
	{
		_id = 0;
		_useCount = 0;
		_pTop = nullptr;
		UuidCreate(&_uuid);

		for (int i = 0; i < blockNum; i++)
		{
			BLOCK_NODE* pNode = (BLOCK_NODE*)malloc(sizeof(BLOCK_NODE));
			pNode->fCode = (unsigned __int64) _uuid.Data4;
			pNode->bCode = (unsigned __int64) _uuid.Data4;
			pNode->next = _pTop;

			if (!placementNew)
				new (&pNode->data) DATA;

			// 17 ��Ʈ�� ���
			pNode = (BLOCK_NODE*)((__int64)pNode | _id << 47);
			_pTop = pNode;
			InterlockedIncrement(&_id);
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

		// Free List ��츸 �߰� �Ҵ�
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
					InterlockedIncrement(&_id);
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
				#ifdef LOGGING
				log(GetCurrentThreadId(), Action::ALLOC, (UINT64)pTop, (UINT64)pTopNode->next);
				#endif

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

		// ���� �Ҵ��� ��尡 �´��� üũ
		if (pNode->fCode != (unsigned __int64)_uuid.Data4 || pNode->bCode != (unsigned __int64)_uuid.Data4)
			return FALSE;

		// ������� ��尡 �ִ��� üũ
		InterlockedDecrement64(&_useCount);
		if (_useCount < 0)
		{
			InterlockedIncrement64(&_useCount);
			return FALSE;
		}
		
		if (_placementNew)
			pNode->data.~DATA();
		
		while (1)
		{
			BLOCK_NODE* pNewTop = (BLOCK_NODE*)((__int64)pNode | _id << 47);
			BLOCK_NODE* pTop = _pTop;
			pNode->next = pTop;

			if ((LONG64)pTop == InterlockedCompareExchange64((LONG64*)&_pTop, (LONG64)pNewTop, (LONG64)pTop))
			{
				#ifdef LOGGING
				log(GetCurrentThreadId(), Action::FREE, (UINT64)pTop, (UINT64)pNewTop);
				#endif

				InterlockedIncrement(&_id);
				break;
			}
		}
		
		return TRUE;
	}

	int	GetCapacityCount(void) { return _capacity; }
	int	GetUseCount(void) { return _useCount; }
};

#endif