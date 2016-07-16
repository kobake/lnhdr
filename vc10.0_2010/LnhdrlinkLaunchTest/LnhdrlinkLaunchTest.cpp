#include "stdafx.h"
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <vector>

int _tmain(int argc, _TCHAR* argv[])
{
	const wchar_t* link = L"C:\\_tmp\\left2.json";
	const wchar_t* target = L"C:\\_tmp\\left.json";

	// lnhdrlink.exeÉpÉX.
	wchar_t path[_MAX_PATH];
	wcscpy(path, L"C:\\lnhdr\\vc10.0\\x64\\Release\\lnhdrlink.exe");

	// à¯êî.
	std::vector<wchar_t> vargs(wcslen(link) + wcslen(target) + 10);
	wchar_t* args = &vargs[0];
	swprintf(args, L"\"%ls\" \"%ls\"", link, target);

	// lnhdrlink.exeé¿çs.
	::SHELLEXECUTEINFOW info = {0};
	info.cbSize = sizeof(info);
	info.lpVerb = L"open";
	info.lpFile = path;
	info.lpParameters = args;
	BOOL f = ::ShellExecuteExW(&info);
	printf("f = %d\n", f);
	return 0;
}
