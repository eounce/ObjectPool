#include <stdio.h>
#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "Profile.h"

PROFILE_SAMPLE proSample[MAX_PROFILE];
using namespace std;

void ProfileBegin(LPCWSTR szName)
{
	// tag 검색
	PROFILE_SAMPLE* pProSample = nullptr;
	for (int cnt = 0; cnt < MAX_PROFILE; cnt++)
	{
		if (_tcscmp(szName, proSample[cnt].szName) == 0)
		{
			pProSample = &proSample[cnt];
		}
	}

	// tag 생성
	if (pProSample == nullptr)
	{
		for (int cnt = 0; cnt < MAX_PROFILE; cnt++)
		{
			if (proSample[cnt].flag) continue;
			wcscpy_s(proSample[cnt].szName, sizeof(PROFILE_SAMPLE::szName), szName);
			proSample[cnt].flag = true;
			pProSample = &proSample[cnt];
			break;
		}
	}

	// 시작 시간 측정
	QueryPerformanceCounter(&pProSample->startTime);
}

void ProfileEnd(LPCWSTR szName)
{
	// 종료 시간 측정
	LARGE_INTEGER End;
	QueryPerformanceCounter(&End);

	// tag 검색
	PROFILE_SAMPLE* pProSample = nullptr;
	for (int cnt = 0; cnt < MAX_PROFILE; cnt++)
	{
		if (wcscmp(szName, proSample[cnt].szName) == 0)
		{
			pProSample = &proSample[cnt];
		}
	}

	if (pProSample == nullptr)
	{
		cout << L"not found tag error" << endl;

		// tag가 없다면 임의로 오류 발생
		int* p = nullptr;
		*p = 0;
	}

	__int64 time = End.QuadPart - pProSample->startTime.QuadPart;
	pProSample->call++;
	pProSample->totalTime += time;

	pProSample->min = min(time, pProSample->min);
	pProSample->max = max(time, pProSample->max);
}

bool ProfileDataOutText(LPCWSTR szFileName)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	FILE* pFile = nullptr;
	errno_t err = _wfopen_s(&pFile, szFileName, L"wt");
	if (err != 0)
	{
		cout << "file open error" << endl;
		return false;
	}

	fwprintf_s(pFile, L"%20s | %17s | %17s | %17s | %15s\n",
		L"Name", L"Average", L"Min", L"Max", L"Call");
	for (int cnt = 0; cnt < MAX_PROFILE; cnt++)
	{
		if (proSample[cnt].flag)
		{
			// max, min 값 제외
			proSample[cnt].totalTime -= proSample[cnt].max;
			proSample[cnt].totalTime -= proSample[cnt].min;
			proSample[cnt].call -= 2;

			// 마이크로 단위 변환
			double avgTime = (proSample[cnt].totalTime / (double)proSample[cnt].call * 1000000.0) / freq.QuadPart;
			double minTime = (proSample[cnt].min * 1000000.0) / (double)freq.QuadPart;
			double maxTime = (proSample[cnt].max * 1000000.0) / (double)freq.QuadPart;

			fwprintf_s(pFile, L"%20ls | %15.4lfus | %15.4lfus | %15.4lfus | %15lld\n",
				proSample[cnt].szName, avgTime, minTime, maxTime, proSample[cnt].call);
		}
	}
	fclose(pFile);

	return true;
}

void ProfileReset(void)
{
	for (int cnt = 0; cnt < MAX_PROFILE; cnt++)
	{
		if (proSample[cnt].flag == 0) continue;
		proSample[cnt].call = 0;
		proSample[cnt].totalTime = 0;
		proSample[cnt].max = LLONG_MIN;
		proSample[cnt].min = LLONG_MAX;
	}
}