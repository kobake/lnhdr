//----------------------------------------------------------------------------
#define WINVER		0x0500
#define _WIN32_WINNT	0x0500
#define _WIN32_IE	0x0500

#include <windows.h>
#include <msiquery.h>
#include <tchar.h>
#include "atlbase.h"
#include "atlstr.h"
#include "resource.h"

CString getProperty(MSIHANDLE hInstall, LPCTSTR name);

//----------------------------------------------------------------------------
BOOL APIENTRY DllMain(
	HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

//----------------------------------------------------------------------------
extern "C" UINT __stdcall ShowWarning(MSIHANDLE hInstall)
{
	//CString path = getProperty(hInstall, _T("CustomActionData"));
	CString cap, message;
	cap.LoadString(IDS_STRING101);
	message.LoadString(IDS_STRING102);
	::MessageBox(NULL, message, cap, MB_ICONWARNING | MB_OK | MB_SYSTEMMODAL);
	return ERROR_SUCCESS;
}

//----------------------------------------------------------------------------
CString getProperty(MSIHANDLE hInstall, LPCTSTR name)
{
	CString str;
	DWORD dwSize = 0;
	UINT ret = ::MsiGetProperty(hInstall, name, _T(""), &dwSize);
	if(ret != ERROR_MORE_DATA && ret != ERROR_SUCCESS)
		return _T("");
	
	dwSize++;
	TCHAR *buf = str.GetBufferSetLength(dwSize);
	ret = ::MsiGetProperty(hInstall, name, buf, &dwSize);
	if(ret != ERROR_SUCCESS)
		return _T("");

	str.ReleaseBuffer();
	return str;
}

