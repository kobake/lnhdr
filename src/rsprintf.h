//--------------------------------------------------------------------------
// FILENAME    : rsprintf.h
// DESCRIPTION : resource message processing (Unicode Version)
// UPDATE      : 2000/08/15
// Programmed by T.Kawasaki
//--------------------------------------------------------------------------
#ifndef _RSPRINTF_H_
#define _RSPRINTF_H_

#include "stdafx.h"
#include "atlstr.h"

#ifndef WINNT_ONLY
#define WINNT_ONLY 0
#endif

struct ANSI_TRAITS
{
	typedef char CHAR_TYPE;
	typedef char *STR_TYPE;
	static DWORD format(STR_TYPE buffer, STR_TYPE string, va_list *args)
	{
		return FormatMessageA(
			FORMAT_MESSAGE_FROM_STRING |
			FORMAT_MESSAGE_ALLOCATE_BUFFER,
			string, 0, 0, buffer, 0, args);
	}
	static int strlen(STR_TYPE str) {return strlen(str);}
	static void strcpy(STR_TYPE dest, const STR_TYPE src)
		{strcpy(dest, src);}
	static int loadString(HINSTANCE hInst, UINT uId, STR_TYPE buffer, int max)
		{return LoadStringA(hInst, uId, buffer, max);}
};

struct WCHAR_TRAITS
{
	typedef wchar_t CHAR_TYPE;
	typedef wchar_t *STR_TYPE;
	static DWORD format(STR_TYPE buffer, STR_TYPE string, va_list *args)
	{
		if(isWinnt())
			return FormatMessageW(
				FORMAT_MESSAGE_FROM_STRING |
				FORMAT_MESSAGE_ALLOCATE_BUFFER,
				string, 0, 0, buffer, 0, args);

		char *temp;
		if(!FormatMessageA(
			FORMAT_MESSAGE_FROM_STRING |
			FORMAT_MESSAGE_ALLOCATE_BUFFER,
			CW2A(string), 0, 0, (char *)&temp, 0, args))
			return 0;

		CA2W wstr(temp);
		LocalFree(temp);
		size_t len = wcslen(wstr);
		wchar_t *strbuf = (wchar_t *)LocalAlloc(
				LMEM_FIXED, (len + 1) * sizeof(wchar_t));
		if(strbuf)
			wcscpy(strbuf, wstr);
		*(wchar_t **)buffer = strbuf;
		return (DWORD)len;
	}

	static int strlen(STR_TYPE str) {return (int)wcslen(str);}
	static void strcpy(STR_TYPE dest, const STR_TYPE src)
		{wcscpy(dest, src);}
	static int loadString(HINSTANCE hInst, UINT uId, STR_TYPE buffer, int max)
		{return LoadStringW(hInst, uId, buffer, max);}
	
	static bool isWinnt()
	{
		if(!WINNT_ONLY)
		{
			OSVERSIONINFO ovi;
			memset(&ovi, 0, sizeof(ovi));
			ovi.dwOSVersionInfoSize = sizeof(ovi);
			GetVersionEx(&ovi);
			return (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) ? true : false;
		}
		return true;
	}
	
};

template<class TRAITS> class rsprintf_T
{
public:
	typedef typename TRAITS::CHAR_TYPE CHAR_TYPE;
	typedef typename TRAITS::STR_TYPE STR_TYPE;
	operator const CHAR_TYPE *() {return (const CHAR_TYPE *)lpMsg;}
	const CHAR_TYPE * c_str() {return (const CHAR_TYPE *)lpMsg;}

	rsprintf_T() : status(0), lpMsg(NULL)
	{
	}

	explicit rsprintf_T(int id, ...) : status(0), lpMsg(NULL)
	{
		va_list vl;
		va_start(vl, id);
		as(id, vl);
		va_end(vl);
	}

	void assign(int id, ...)
	{
		va_list vl;
		va_start(vl, id);
		as(id, vl);
		va_end(vl);
	}

	rsprintf_T(const rsprintf_T& src)
	{
		duplicate(src);
	}

	rsprintf_T& operator=(const rsprintf_T& src)
	{
		duplicate(src);
		return *this;
	}

	~rsprintf_T()
	{
		if(status)
		{
			LocalFree(lpMsg);
		}
	}
private:
	LPVOID lpMsg;
	DWORD status;

	void as(int id, va_list& vl)
	{

		id = convert_id(id);

		if(status) LocalFree(lpMsg);
		CHAR_TYPE buf[256];
		if(TRAITS::loadString(_pModule->GetModuleInstance(), id, buf, 256) == 0)
		{
			lpMsg = NULL;
			status = 0;
			return;
		}
		status = TRAITS::format((STR_TYPE)&lpMsg, buf, &vl);
	}

	int convert_id(int id)
	{
		if(id > 0) return id;
		return 0;
	}

	void duplicate(const rsprintf_T& src)
	{
		if(src.status)
		{
			lpMsg = LocalAlloc(0,
				(TRAITS::strlen((STR_TYPE)src.lpMsg) + 1) * sizeof(CHAR_TYPE));
			TRAITS::strcpy((STR_TYPE)lpMsg, (const STR_TYPE)src.lpMsg);
			status = src.status;
		}
		else
		{
			lpMsg = NULL;
			status = 0;
		}
	}
};

typedef rsprintf_T<ANSI_TRAITS> rsprintfA;
typedef rsprintf_T<WCHAR_TRAITS> rsprintfW;

#if _UNICODE
typedef rsprintfW rsprintf;
#else
typedef rsprintfA rsprintf;
#endif

#endif // _RSPRINTF_H_
