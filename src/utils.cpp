#include "stdafx.h"
#include "utils.h"
#include "md5.h"
#include "rsprintf.h"
#include "resource.h"
#include <vector>

//---------------------------------------------------------------------------
bool CopyToClipboard(HWND hwnd, LPCTSTR str)
{
	if(!OpenClipboard(hwnd))
		return false;
	EmptyClipboard();
	
	DWORD size = (lstrlen(str) + 1) * sizeof(TCHAR);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if(!hMem)
	{
		CloseClipboard();
		return false;
	}
	
	void* buffer = GlobalLock(hMem);
	CopyMemory(buffer, str, size);
	GlobalUnlock(buffer);
#ifdef UNICODE
	SetClipboardData(CF_UNICODETEXT, hMem);
#else
	SetClipboardData(CF_TEXT, hMem);
#endif
	CloseClipboard();
	
	return true;
}

//---------------------------------------------------------------------------
void showError(HWND hwnd, LPCTSTR fn, DWORD dwErr)
{
	LPVOID mes;
	if(dwErr == (DWORD)-1)
		dwErr = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&mes,
		0, NULL);

	if(fn)
	{
		MessageBox(hwnd,
			rsprintf(IDS_STRING104, fn, mes),
			rsprintf(IDS_STRING103),
			MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);
	}
	else
		MessageBox(hwnd,
			(LPCTSTR)mes,
			rsprintf(IDS_STRING103),
			MB_ICONSTOP | MB_OK | MB_SYSTEMMODAL);

	LocalFree(mes);
}

//---------------------------------------------------------------------------
void copyStrW(void* buffer, size_t size, const CString& str)
{
	wchar_t* buf = (wchar_t*)buffer;
	size--; // keep a room for null-terminator
	wcsncpy(buf, CT2W(str), size);
	buf[size] = 0;
}

//---------------------------------------------------------------------------
void copyStrA(void* buffer, size_t size, const CString& str)
{
	char* buf = (char*)buffer;
	size--; // keep a room for null-terminator
	strncpy(buf, CT2A(str), size);
	buf[size] = 0;
}

//---------------------------------------------------------------------------
static int base64_enc(WORD w)
{
	if(w <= 25) return (int)w + 'A';
	if(w <= 51) return (int)w - 26 + 'a';
	if(w <= 61) return (int)w - 52 + '0';
	if(w == 62) return '+';
	return '/';
}

//---------------------------------------------------------------------------
static void encode_3bytes(TCHAR* buffer, LPCBYTE src, size_t size)
{
	buffer[0] = (TCHAR)base64_enc(src[0] >> 2);
	buffer[1] = (TCHAR)base64_enc(((src[0] & 3) << 4) | (src[1] >> 4));
	buffer[2] = (TCHAR)base64_enc(((src[1] & 15) << 2) | (src[2] >> 6));
	buffer[3] = (TCHAR)base64_enc(src[2] & 63);

	if(size == 1) buffer[2] = '=';
	if(size <= 2) buffer[3] = '=';
}

//---------------------------------------------------------------------------
CString base64Encode(LPCBYTE src, size_t size)
{
	if(size == 0)
		return _T("");

	CString str;
	// Note: 18 blocks per line (72 chars)
	int blocks = (int)(size + 2) / 3;
	int lines = (blocks + 17) / 18;
	int chrs = lines * (72 + 2); // Add CRLF

	TCHAR* p = str.GetBufferSetLength(chrs);
	for(int co = 0;;)
	{
		encode_3bytes(p, src, (size < 3) ? size : 3);
		p += 4;
		src += 3;
		if(size <= 3)
		{
			lstrcpy(p, _T("\r\n"));
			str.ReleaseBuffer();
			break;
		}
		size -= 3;
		if(co == 17)
		{
			lstrcpy(p, _T("\r\n"));
			p += 2;
			co = 0;
		}
		else
			co++;
	}
	return str;
}

//---------------------------------------------------------------------------
CString base64Encode(LPCTSTR filename)
{
	HANDLE hFile = CreateFile(
		filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return _T("");

	DWORD dwSize = GetFileSize(hFile, NULL);
	std::vector<BYTE> buffer(dwSize);

	DWORD dwRead = 0;
	ReadFile(hFile, &buffer[0], dwSize, &dwRead, NULL);
	CloseHandle(hFile);

	CString content;
	content.FormatMessage(_T("MIME-Version: 1.0\r\nContent-Type: application/octet-stream; name=\"%1\"\r\nContent-Transfer-Encoding: base64\r\nContent-Disposition: attachment; filename=\"%1\"\r\n\r\n"),
		PathFindFileName(filename));

	content += base64Encode(&buffer[0], dwRead);
	return content;
}

//---------------------------------------------------------------------------
CString getMD5Hash(LPCTSTR filename)
{
	HANDLE hFile = CreateFile(
		filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return _T("");

	DWORD high = 0;
	DWORD dwSize = GetFileSize(hFile, &high);
	
	const DWORD BUFSIZE = 1024 * 1024 * 4;
	std::vector<BYTE> buffer(BUFSIZE);
	
	MD5Digest md5;
	for(;;)
	{
		DWORD dwRead = 0;
		ReadFile(hFile, &buffer[0], BUFSIZE, &dwRead, NULL);
		if(dwRead == 0)
			break;
		md5.update(&buffer[0], dwRead);
	}
	CloseHandle(hFile);
	
	BYTE sum[16];
	md5.final(sum);
	
	CString sumstr;
	sumstr.Format(
		_T("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"),
		sum[0], sum[1], sum[2], sum[3], sum[4], sum[5], sum[6], sum[7],
		sum[8], sum[9], sum[10], sum[11], sum[12], sum[13], sum[14],
		sum[15]);
	return sumstr;
}
