#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <Windows.h>
#include <process.h>
#include "CrashDump.h"
#include "ObjectPool.h"

const int MAX_POOL_SIZE = 40000;
const int POOL_SIZE = 10000;
const int THREAD_COUNT = 4;

struct NODE
{
	__int64 data;
	NODE* next;
};

ObjectPool<NODE> g_nodePool(false, MAX_POOL_SIZE);
unsigned WINAPI threadProc(LPVOID lpParam);

CrashDump cd;

int wmain()
{
	HANDLE hThreads[THREAD_COUNT];
	NODE* pNode[MAX_POOL_SIZE];

	// 데이터 초기화
	for (int i = 0; i < MAX_POOL_SIZE; i++)
	{
		pNode[i] = g_nodePool.Alloc();
		pNode[i]->data = 0;
		pNode[i]->next = (NODE*)0xEFFFFFFF;
	}

	for (int i = 0; i < MAX_POOL_SIZE; i++)
		g_nodePool.Free(pNode[i]);

	for (int i = 0; i < THREAD_COUNT; i++)
		hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, threadProc, (LPVOID)i, 0, NULL);

	WaitForMultipleObjects(THREAD_COUNT, hThreads, TRUE, INFINITE);
	
	return 0;
}

unsigned WINAPI threadProc(LPVOID lpParam)
{
	NODE* pNode[POOL_SIZE];

	while (1)
	{
		for (int i = 0; i < POOL_SIZE; i++)
		{
			pNode[i] = g_nodePool.Alloc();
			if (pNode[i] == nullptr)
				CrashDump::Crash();

			if (pNode[i]->data != 0)
				CrashDump::Crash();

			if (pNode[i]->next != (NODE*)0xEFFFFFFF)
				CrashDump::Crash();
		}

		for (int i = 0; i < POOL_SIZE; i++)
		{
			__int64 data = InterlockedIncrement64(&pNode[i]->data);
			if (data != 1)
				CrashDump::Crash();

			__int64 next = InterlockedIncrement64((LONG64*)&pNode[i]->next);
			if (next != 0xF0000000)
				CrashDump::Crash();
		}

		Sleep(0);
		Sleep(0);
		Sleep(0);
		Sleep(0);

		for (int i = 0; i < POOL_SIZE; i++)
		{
			__int64 data = InterlockedDecrement64(&pNode[i]->data);
			if (data != 0)
				CrashDump::Crash();

			__int64 next = InterlockedDecrement64((LONG64*)&pNode[i]->next);
			if (next != 0xEFFFFFFF)
				CrashDump::Crash();

			if (!g_nodePool.Free(pNode[i]))
				CrashDump::Crash();
		}

		wprintf(L"OK : %d\n", (int)lpParam);
	}

	return 0;
}