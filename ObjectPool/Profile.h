#pragma once
#ifndef _PROFILE_H_
#define _PROFILE_H_

#include <Windows.h>
#define MAX_PROFILE 50

#ifdef PROFILE
#define PRO_BEGIN(TagName) ProfileBegin(TagName)
#define PRO_END(TagName)   ProfileEnd(TagName)
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

struct PROFILE_SAMPLE
{
	bool flag;
	WCHAR szName[64];

	LARGE_INTEGER startTime;

	__int64 totalTime;
	__int64 min;
	__int64 max;

	__int64 call;
};
extern PROFILE_SAMPLE proSample[MAX_PROFILE];

void ProfileBegin(LPCWSTR szName);
void ProfileEnd(LPCWSTR szName);
bool ProfileDataOutText(LPCWSTR szFileName);
void ProfileReset(void);

class Profile
{
private:
	LPCWSTR _tag;
public:
	Profile(LPCWSTR tag)
	{
		_tag = tag;
		PRO_BEGIN(tag);
	}

	~Profile()
	{
		PRO_END(_tag);
	}
};

#endif