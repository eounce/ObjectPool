#ifndef _CRASH_DUMP_H_
#define _CRASH_DUMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <Windows.h>
#include <psapi.h>
#include <winnt.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

class CrashDump
{
public:
	static long _dumpCount;

	CrashDump()
	{
		_dumpCount = 0;

		_invalid_parameter_handler oldHandler, newHandler;
		newHandler = myInvalidParameterHandler;

		oldHandler = _set_invalid_parameter_handler(newHandler);
		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(_custom_Report_hook);

		//-----------------------------------------------
		// pure virtual function called 에러 핸들러를 사용자 정의 함수로 우회시킨다.
		//-----------------------------------------------
		_set_purecall_handler(myPurecallHandler);

		setHandlerDump();
	}

	static void Crash()
	{
		int* p = nullptr;
		*p = 0;
	}

	static LONG WINAPI myExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		int workingMemory = 0;
		SYSTEMTIME nowTime;

		long dumpCount = InterlockedIncrement(&_dumpCount);

		//----------------------------------------------
		// 현재 프로세스의 메모리 사용량을 얻어온다.
		//----------------------------------------------
		HANDLE hProcess = 0;
		PROCESS_MEMORY_COUNTERS pmc;

		hProcess = GetCurrentProcess();
		if (hProcess == NULL)
			return 0;

		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			workingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(hProcess);

		//-------------------------------------------------
		// 현재 날짜와 시간을 알아온다.
		//-------------------------------------------------
		WCHAR filename[MAX_PATH];

		GetLocalTime(&nowTime);
		wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp",
			nowTime.wYear, nowTime.wMonth, nowTime.wDay, nowTime.wHour, nowTime.wMinute, nowTime.wSecond, dumpCount, workingMemory);
		wprintf_s(L"Now Save dump file...\n");

		HANDLE hDumpFile = ::CreateFile(filename,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation;

			minidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			minidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			minidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpWithFullMemory,
				&minidumpExceptionInformation,
				NULL,
				NULL);

			CloseHandle(hDumpFile);

			wprintf_s(L"CrashDump Save Finish!\n");
		}
		else
		{
			wprintf(L"error : %d\n", GetLastError());
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void setHandlerDump()
	{
		SetUnhandledExceptionFilter(myExceptionFilter);
	}

	static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler()
	{
		Crash();
	}
};

long CrashDump::_dumpCount = 0;

#endif