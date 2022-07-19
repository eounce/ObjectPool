#include <stdio.h>
#include <iostream>
#include <tchar.h>
#include <Windows.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "Profile.h"
#include "ProfileInit.h"

ProfileInit::ProfileInit()
{
	for (int cnt = 0; cnt < MAX_PROFILE; cnt++)
	{
		proSample[cnt].min = LLONG_MAX;
		proSample[cnt].max = LLONG_MIN;
	}
}

ProfileInit::~ProfileInit()
{
	WCHAR fileName[128];
	tm TM;
	time_t timer;

	time(&timer);
	localtime_s(&TM, &timer);

	swprintf_s(fileName, 128, L"Profile%04d%02d%02d_%02d%02d%02d.txt",
		TM.tm_year + 1900,
		TM.tm_mon + 1,
		TM.tm_mday,
		TM.tm_hour, TM.tm_min, TM.tm_sec);

	ProfileDataOutText(fileName);
}

ProfileInit proInit; // PROFILE_SAMPLE 구조체 초기화