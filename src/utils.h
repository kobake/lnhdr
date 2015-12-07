#ifndef _utils_h_
#define _utils_h_

#include "stdafx.h"

void showError(HWND hwnd, LPCTSTR fn = NULL, DWORD dwErr = (DWORD)-1);
bool CopyToClipboard(HWND hwnd, LPCTSTR str);
void copyStrW(void* buffer, size_t size, const CString& str);
void copyStrA(void* buffer, size_t size, const CString& str);
CString base64Encode(LPCTSTR filename);
CString base64Encode(LPCBYTE src, size_t size);
CString getMD5Hash(LPCTSTR filename);

/////////////////////////////////////////////////////////////////////////////
// File Enumeration Object
//
class CEnumFiles
{
public:
	CEnumFiles(LPDATAOBJECT pDataObj)
		: m_dataObj(pDataObj), m_enumerated(0), m_hDrop(0)
	{
	}

	~CEnumFiles()
	{
		if(m_enumerated)
		{
			if(m_hDrop)
			{
				GlobalUnlock(m_hDrop);
				ReleaseStgMedium(&m_medium);
			}
		}
	}

	UINT getNumOfFiles()
	{
		if(load())
			return DragQueryFile(m_hDrop, -1, NULL, 0);
		return 0;
	}

	bool getNthFileName(UINT u, TCHAR *buffer)
	{
		if(load())
			return DragQueryFile(m_hDrop, u, buffer, MAX_PATH) ? true : false;
		return false;
	}

private:
	bool load()
	{
		if(!m_enumerated)
		{
			m_enumerated = true;
			FORMATETC fmte = {
				CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
			if(FAILED(m_dataObj->GetData(&fmte, &m_medium)))
				return false;
			m_hDrop = (HDROP)GlobalLock(m_medium.hGlobal);
			if(!m_hDrop)
				return false;
		}
		return true;
	}

	LPDATAOBJECT m_dataObj;
	STGMEDIUM m_medium;
	HDROP m_hDrop;
	bool m_enumerated;
};

#endif // _utils_h_
