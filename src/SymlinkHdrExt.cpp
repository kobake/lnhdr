// SymlinkHdrExt.cpp : Implementation of CSymlinkHdrExt

#include "stdafx.h"
#include "SymlinkHdrExt.h"

#include "utils.h"
#include "link.h"
#include "rsprintf.h"
// CSymlinkHdrExt

/////////////////////////////////////////////////////////////////////////////
// CSymlinkHdrExt
//
// �t�H���_�ւ̃V���{���b�N�����N�𑀍삷��ꍇ�ɌĂ΂��n���h��
//
UINT CSymlinkHdrExt::CopyCallback(
	HWND hwnd, UINT wFunc, UINT wFlags, LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs)
{
	// IDYES    : Allows the operation
	// IDNO     : Prevents the operation on this folder but continues with
	//            any other operations that have been approved
	//            (for example, a batch copy operation).
	// IDCANCEL : Prevents the current operation and cancels any pending
	//            operations.

	// �V���{���b�N�����N�łȂ���Ώ�����e�F
	if(!IsSymlink(pszSrcFile))
		return IDYES;

	if(wFunc == FO_RENAME) // ���O�ύX�͊Q���y�ڂ��Ȃ�
		return IDYES;

	bool f;
	if(wFunc == FO_DELETE)
	{
		f = (Unlink(pszSrcFile) != 0);
	}
	else if(wFunc == FO_COPY || wFunc == FO_MOVE) // �R�s�[/�ړ�
	{
		// �R�s�[/�ړ���̃f�B���N�g�����擾
		TCHAR dest_dir[MAX_PATH], src_dir[MAX_PATH], dest_fn[MAX_PATH];
		GetNormalizedPath(dest_dir, pszDestFile);
		GetNormalizedPath(src_dir, pszSrcFile);

		lstrcpy(dest_fn, pszDestFile);

		f = false;
		int plan = IDYES;
		if(lstrcmp(src_dir, dest_dir) != 0)
		{
			// ���݂̃f�B���N�g���ȊO�ɃR�s�[/�ړ�
			if(PathFileExists(pszDestFile))
			{
				plan = MessageBoxW(
					hwnd,
					rsprintf(IDS_STRING101, pszDestFile), // can overwrite ?
					rsprintf(IDS_STRING102),
					MB_YESNOCANCEL | MB_ICONQUESTION);
				if(plan == IDYES)
				{
					// ���ɑ��݂���t�@�C�����폜
					Unlink(pszDestFile);
				}
				else return plan; // ������/�L�����Z��
			}
		}
		else
		{
			// ���݂̃f�B���N�g���ɃR�s�[/�ړ�
			// -> ���O��ς��đ��s����
			CreateUniqueName(dest_fn, dest_dir, pszSrcFile, IDS_STRING121);
		}

		if(plan == IDYES) // �u�͂��v/���Ȃ��̂Ƃ��̂ݎ��s
			f = SymlinkCopyMove(pszSrcFile, dest_fn, wFunc == FO_MOVE);
	}

	if(f) return IDNO;

	showError(hwnd, PathFindFileNameW(pszSrcFile));
	return IDCANCEL;
}

/////////////////////////////////////////////////////////////////////////////
// CSymlinkHdrExt
//
// The shell passes us a SHCOLUMNINIT struct, which currently only contains
// one tidbit of info, the full path of the folder being viewed in Explorer.
// For our purposes, we don't need that info, so our extension just returns
// S_OK.
STDMETHODIMP CSymlinkHdrExt::Initialize(LPCSHCOLUMNINIT psci)
{
	return S_OK;
}


static HRESULT symlink(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);
static HRESULT filesize(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);
static HRESULT fileudate(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);
static HRESULT filecdate(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);
static HRESULT fileid(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);
static HRESULT numoflink(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);

//typedef HRESULT (*disp_func_type)(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);

typedef struct
{
	VARTYPE vt;
	DWORD   fmt;
	DWORD   csFlags;
	UINT    cChars;
	UINT	titleId;
	UINT	descId;
	HRESULT (*disp_func)(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);
} COLUMN_FORMAT;

COLUMN_FORMAT fmt[] = {
	{VT_LPSTR, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, 17, 105, 106, symlink},
	{VT_LPSTR, LVCFMT_RIGHT, SHCOLSTATE_TYPE_INT, 14, 107, 108, filesize},
	{VT_LPSTR, LVCFMT_LEFT, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_SLOW, 17, 109, 110, fileudate},
	{VT_LPSTR, LVCFMT_LEFT, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_SLOW, 17, 111, 112, filecdate},
	{VT_LPSTR, LVCFMT_LEFT, SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW, 17, 113, 114, fileid},
	{VT_I4, LVCFMT_RIGHT, SHCOLSTATE_TYPE_INT | SHCOLSTATE_SLOW, 2, 115, 116, numoflink},
};

// �e�J�����Ɋւ������Ԃ�
STDMETHODIMP CSymlinkHdrExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO* psci)
{
	if(dwIndex >= sizeof(fmt) / sizeof(COLUMN_FORMAT))
		return S_FALSE;

	psci->scid.fmtid = *_pModule->pguidVer; // �����̎��ʎq
	psci->scid.pid   = dwIndex; // �J�����ԍ�

	psci->vt      = fmt[dwIndex].vt;      // �f�[�^�𕶎���Ƃ��ĕԂ�
	psci->fmt     = fmt[dwIndex].fmt;     // ��������
	psci->csFlags = fmt[dwIndex].csFlags; // �f�[�^��INT�Ƃ��Đ���
	psci->cChars  = fmt[dwIndex].cChars;  // �f�t�H���g��(������)
	lstrcpyW(psci->wszTitle,
		rsprintfW(fmt[dwIndex].titleId).c_str());
	lstrcpyW(psci->wszDescription,
		rsprintfW(fmt[dwIndex].descId).c_str());

    return S_OK;
}

STDMETHODIMP CSymlinkHdrExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	// ���ʎq�������̂��̂��`�F�b�N����
	if(pscid->fmtid != *_pModule->pguidVer)
		return S_FALSE;

	return fmt[pscid->pid].disp_func(pscid, pscd, pvarData);
}

static HRESULT filedate(LPCSHCOLUMNDATA pscd, VARIANT* pvarData, int n)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile(pscd->wszFile, &wfd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);

		FILETIME *pft = &wfd.ftCreationTime;
		if(n == 1) pft = &wfd.ftLastAccessTime;
		else if(n == 2) pft = &wfd.ftLastWriteTime;

		FILETIME ftLocal;
		SYSTEMTIME st;
		FileTimeToLocalFileTime(pft, &ftLocal);
		FileTimeToSystemTime(&ftLocal, &st);

		TCHAR date[17];
		wsprintf(date, L"%04u/%02u/%02u %02u:%02u",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
		CComVariant vData(date);
		vData.Detach(pvarData);
		return S_OK;
	}
	return S_FALSE;
}

// �ŏI�X�V����
static HRESULT fileudate(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	return filedate(pscd, pvarData, 2);
}

// �쐬����
static HRESULT filecdate(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	return filedate(pscd, pvarData, 0);
}

// �t�@�C���T�C�Y
static HRESULT filesize(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	if(pscd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		/*
		CComVariant vData(rsprintf(IDS_STRING117).c_str()); // "<DIR>"
		vData.Detach(pvarData);
		return S_OK;
		*/
		return S_FALSE;
	}

	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile(pscd->wszFile, &wfd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		LARGE_INTEGER li;
		li.HighPart = wfd.nFileSizeHigh;
		li.LowPart  = wfd.nFileSizeLow;

		TCHAR num[27]; // 20��+�J���}6��+'\0'
		enum {num_len = sizeof(num) / sizeof(TCHAR)};
		LONGLONG n = li.QuadPart;
		num[26] = 0;
		//for(int i = 0; i < num_len - 2; i++)
		//	num[i] = ' ';
		int i;
		for(i = 0; i < num_len - 2; i++)
		{
			if((i & 3) == 3)
			{
				num[num_len - 2 - i] = ',';
				continue;
			}
			num[num_len - 2 - i] = '0' + (int)(n % 10);
			n /= 10;
			if(n == 0) break;
		}

		CComVariant vData(num + num_len - 2 - i);
		vData.Detach(pvarData);
		return S_OK;
	}
	return S_FALSE;
}

static HRESULT symlink(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	wchar_t fullpath[MAX_PATH], buffer[MAX_PATH];
	if(GetLinkTargetPath(fullpath, pscd->wszFile))
	{
		// �h���C�u���܂����ł��Ȃ������瑊�΃p�X�\���ɂ���
		if(PathIsSameRoot(fullpath, pscd->wszFile))
			PathRelativePathTo(buffer, pscd->wszFile, 0, fullpath, 0);
		else
			lstrcpy(buffer, fullpath);
	}
	else
		buffer[0] = 0;

	CComVariant vData(buffer);
	vData.Detach(pvarData);
	return S_OK;
}

static HRESULT fileid(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	HANDLE hFile = CreateFileW(
		pscd->wszFile,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		CComVariant vData(L"-");
		vData.Detach(pvarData);
		return S_OK;
	}

	BY_HANDLE_FILE_INFORMATION info;
	if(!GetFileInformationByHandle(hFile, &info))
	{
		CloseHandle(hFile);
		CComVariant vData(L"-");
		vData.Detach(pvarData);
		return S_OK;
	}
	CloseHandle(hFile);

	wchar_t buf[40];
	wsprintfW(buf, L"%08X-%08X", info.nFileIndexHigh, info.nFileIndexLow);
	CComVariant vData(buf);
	vData.Detach(pvarData);
	return S_OK;
}

static HRESULT numoflink(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	HANDLE hFile = CreateFileW(
		pscd->wszFile,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		CComVariant vData(L"-");
		vData.Detach(pvarData);
		return S_OK;
	}

	BY_HANDLE_FILE_INFORMATION info;
	if(!GetFileInformationByHandle(hFile, &info))
	{
		CloseHandle(hFile);
		CComVariant vData(L"-");
		vData.Detach(pvarData);
		return S_OK;
	}
	CloseHandle(hFile);

	CComVariant vData((int)info.nNumberOfLinks);
	vData.Detach(pvarData);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IShellExtInit
//
HRESULT CSymlinkHdrExt::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hProgID)
{
	// �ȑO�ɌĂ΂�Ă�����A�ȑO�̃I�u�W�F�N�g�����
	if(m_dataObj)
	{
		m_dataObj->Release();
		m_dataObj = NULL;
	}

	// ���t�@�����X�J�E���g�𑝂₷
	if(pDataObj)
	{
		m_dataObj = pDataObj;
		m_dataObj->AddRef();
	}

	// �^�[�Q�b�g�p�X���擾
	if(pidlFolder)
	{
		if(!SHGetPathFromIDList(pidlFolder, m_targetFolder))
			return E_FAIL;
	}
	else
		m_targetFolder[0] = 0;

	return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// IContextMenu
//
HRESULT CSymlinkHdrExt::QueryContextMenu(
	HMENU hmenu,
	UINT  uMenuIndex,
	UINT  uidFirstCmd,
	UINT  uidLastCmd,
	UINT  uFlags)
{
	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
	if(uFlags & CMF_DEFAULTONLY)
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

	// �h���b�O���̃I�u�W�F�N�g���m�F
	CEnumFiles enumfile(m_dataObj);
	if(enumfile.getNumOfFiles() == 0)
	{
		// �����N�ΏۂɂȂ�Ȃ��I�u�W�F�N�g
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	if(m_targetFolder[0] == 0)
	{
		// �^�[�Q�b�g�t�H���_�Ƀt�@�C�����쐬�ł��Ȃ��̂Œf�O
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	// ���j���[���ڂ̍쐬
	UINT uidCmd = uidFirstCmd;
	InsertMenu(hmenu, uMenuIndex++, MF_STRING | MF_BYPOSITION, uidCmd++,
		rsprintf(IDS_STRING118)); // "�����N�����(&L)"

	return MAKE_HRESULT(
		SEVERITY_SUCCESS,
		FACILITY_NULL,
		(USHORT)(uidCmd - uidFirstCmd) // ��������j���[�̐�
	);
}

// �R�}���h���s
HRESULT CSymlinkHdrExt::InvokeCommand(LPCMINVOKECOMMANDINFO pInfo)
{
	if(pInfo->lpVerb != 0)
		return E_INVALIDARG;

	CEnumFiles enumfile(m_dataObj);
	UINT uFiles = enumfile.getNumOfFiles();
	if(uFiles < 0)
		return E_INVALIDARG;

	for(UINT u = 0; u < uFiles; u++)
	{
		wchar_t filename[MAX_PATH];
		if(enumfile.getNthFileName(u, filename))
		{
			wchar_t linkname[MAX_PATH];
			CreateUniqueName(linkname, m_targetFolder, filename,
				IDS_STRING119 /* �ւ̃����N */);
			if(!CreateLink(linkname, filename))
			{
				showError(NULL, PathFindFileNameW(filename));
				break;
			}
		}
	}
	return S_OK;
}

// No need to implement GetCommandString()
HRESULT CSymlinkHdrExt::GetCommandString(UINT_PTR, UINT, UINT*, LPSTR, UINT)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// ISHellPropSheetExt
//
HRESULT CSymlinkHdrExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
	CEnumFiles enumfile(m_dataObj);
	if(enumfile.getNumOfFiles() > 1) // �t�@�C����������������y�[�W��ǉ����Ȃ�
		return E_FAIL;

	// �t�@�C�����̎擾
	wchar_t targetPath[MAX_PATH], filename[MAX_PATH];
	if(!enumfile.getNthFileName(0, filename))
		return E_FAIL; // �t�@�C�������擾�ł��Ȃ���΃y�[�W��ǉ����Ȃ�

	// �����N��̒��o
	if(!GetLinkTargetPath(targetPath, filename))
		return E_FAIL; // �����N�łȂ���΃y�[�W��ǉ����Ȃ�

	rsprintf title(IDS_STRING123);
	PROPSHEETPAGE  psp;
	psp.dwSize        = sizeof(psp);
	psp.dwFlags       = PSP_USETITLE | PSP_USECALLBACK;
	psp.hInstance     = _pModule->GetModuleInstance();
	psp.pszTemplate   = MAKEINTRESOURCE(IDD_PROPDLG1);
	psp.hIcon         = 0;
	psp.pszTitle      = title.c_str();
	psp.pfnDlgProc    = (DLGPROC)dlgProc;
	psp.pfnCallback   = psDlgCallback;
	psp.lParam        = (LPARAM)this;

	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
	if(hPage) 
	{
		if(lpfnAddPage(hPage, lParam))
		{
			this->AddRef();
			return S_OK;
		}
		else
		{
			DestroyPropertySheetPage(hPage);
		}
	}
	else
	{
		return E_OUTOFMEMORY;
	}
	return E_FAIL;
}

// �v���p�e�B�V�[�g�쐬�E�폜���ɌĂ΂��n���h��
UINT CSymlinkHdrExt::dlgCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	if(uMsg == PSPCB_ADDREF)
	{
	}
	else if(uMsg == PSPCB_CREATE)
	{
	}
	else if(uMsg == PSPCB_RELEASE)
	{
	}
	return 1;
}

// �v���p�e�B�V�[�g�̃E�B���h�E�v���V�[�W��(�C���X�^���X�ւ̃f�B�X�p�b�`)
INT_PTR CALLBACK CSymlinkHdrExt::dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const PROPSHEETPAGE *ppsp;
	if(uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
		ppsp = (const PROPSHEETPAGE *)lParam;
	}
	else
		ppsp = (const PROPSHEETPAGE *)GetWindowLongPtr(hwndDlg, DWLP_USER);

	if(ppsp)
	{
		CSymlinkHdrExt *hdrExt = (CSymlinkHdrExt *)ppsp->lParam;
		switch(uMsg)
		{
		case WM_INITDIALOG:
			return hdrExt->InitDialog(hwndDlg, uMsg, wParam, lParam);

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case  IDC_GOBUTTON:
				return hdrExt->ProcessGoButton(hwndDlg, uMsg, wParam, lParam);
			}
		}
	}
	// �C���X�^���X�������O
	return FALSE;
}

// �_�C�A���O�̏�����(WM_INITDIALOG)
INT_PTR CSymlinkHdrExt::InitDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEnumFiles enumfile(m_dataObj);
	if(enumfile.getNumOfFiles() > 1) // �t�@�C����������������y�[�W��ǉ����Ȃ�
		return TRUE;

	// �t�@�C�����̎擾
	wchar_t targetPath[MAX_PATH], filename[MAX_PATH];
	enumfile.getNthFileName(0, filename);
	
	/*
	// �����̎擾
	WIN32_FIND_DATA wfd;
	memset(&wfd, 0, sizeof(wfd));
	HANDLE hFind = FindFirstFile(filename, &wfd);
	BOOL fFileAttr = FALSE;
	if(hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		fFileAttr = TRUE;
	}

	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_ARCHIVE), fFileAttr);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_HIDDEN), fFileAttr);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_READONLY), fFileAttr);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_SYSTEM), fFileAttr);

	SendMessage(
		GetDlgItem(hwndDlg, IDC_CHK_ARCHIVE), BM_SETCHECK,
		wfd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(
		GetDlgItem(hwndDlg, IDC_CHK_HIDDEN), BM_SETCHECK,
		wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(
		GetDlgItem(hwndDlg, IDC_CHK_READONLY), BM_SETCHECK,
		wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(
		GetDlgItem(hwndDlg, IDC_CHK_SYSTEM), BM_SETCHECK,
		wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? BST_CHECKED : BST_UNCHECKED, 0);
	*/

	// �����N��̒��o
	BOOL fLinkExist = GetLinkTargetPath(targetPath, filename);
	if(!fLinkExist) targetPath[0] = 0;

	if(wcsncmp(targetPath, L"Volume{", 7) == 0)
	{
		// "\\?\Volume{...}\"�̏���
		wchar_t volumeName[MAX_PATH];
		wsprintfW(volumeName, L"\\\\?\\%s\\", targetPath);
		
		targetPath[0] = 0;
		DWORD drives = GetLogicalDrives();
		for(int i = 'A'; i <= 'Z'; i++)
		{
			if(drives & 1)
			{
				wchar_t volNm[MAX_PATH];
				wchar_t drv[] = L"?:\\";
				drv[0] = i;
				if(GetVolumeNameForVolumeMountPoint(drv, volNm, MAX_PATH))
				{
					if(wcscmp(volNm, volumeName) == 0)
					{
						wcscpy(targetPath, drv);
						break;
					}
				}
			}
			drives >>= 1;
		}
		
		if(!targetPath[0])
		{
			// �ǂ��ɂ��}�E���g����Ă��Ȃ��{�����[��
			lstrcpyW(targetPath, rsprintf(IDS_STRING124).c_str());
			fLinkExist = FALSE;
		}
		
		/*
		DWORD size;
		if(!GetVolumePathNamesForVolumeName(volumeName, targetPath, MAX_PATH, &size))
		{
			// �ǂ��ɂ��}�E���g����Ă��Ȃ��{�����[��
			lstrcpyW(targetPath, rsprintf(IDS_STRING124).c_str());
			fLinkExist = FALSE;
		}
		*/

		// �}�E���g�|�C���g
		SetDlgItemText(hwndDlg, IDC_SYMLINK_TYPE, rsprintf(IDS_STRING126));
	}
	else
	{
		// �W�����N�V����
		SetDlgItemText(hwndDlg, IDC_SYMLINK_TYPE, rsprintf(IDS_STRING125));
	}

	SetDlgItemText(hwndDlg, IDC_PATHEDIT, targetPath);
	EnableWindow(GetDlgItem(hwndDlg, IDC_PATHEDIT), fLinkExist);
	EnableWindow(GetDlgItem(hwndDlg, IDC_GOBUTTON), fLinkExist);
	return TRUE;
}

// [�ړ�(G)]�{�^���̏���
INT_PTR CSymlinkHdrExt::ProcessGoButton(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEnumFiles enumfile(m_dataObj);
	if(enumfile.getNumOfFiles() > 1) // �t�@�C����������������y�[�W��ǉ����Ȃ�
		return TRUE;

	// �����N��̒��o
	wchar_t targetPath[MAX_PATH], filename[MAX_PATH];
	enumfile.getNthFileName(0, filename);
	if(GetLinkTargetPath(targetPath, filename))
		ShellExecute(hwndDlg, NULL, targetPath, NULL, NULL, SW_SHOW); // �J��

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// IShellIconOverlayIdentifier
//
// 
HRESULT CSymlinkHdrExt::GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags)
{
	GetModuleFileName(_pModule->GetModuleInstance(), pwszIconFile, cchMax);
	*pIndex = 0;
	*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
	return S_OK;
}

HRESULT CSymlinkHdrExt::GetPriority(int* pPriority)
{
	*pPriority = 0;
	return S_OK;
}

HRESULT CSymlinkHdrExt::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	wchar_t targetPath[MAX_PATH];
	if(GetLinkTargetPath(targetPath, pwszPath))
		return S_OK;
	return S_FALSE;
}
