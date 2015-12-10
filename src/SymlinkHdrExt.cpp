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
// フォルダへのシンボリックリンクを操作する場合に呼ばれるハンドラ
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

	// シンボリックリンクでなければ処理を容認
	if(!IsSymlink(pszSrcFile))
		return IDYES;

	if(wFunc == FO_RENAME) // 名前変更は害を及ぼさない
		return IDYES;

	bool f;
	if(wFunc == FO_DELETE)
	{
		f = (Unlink(pszSrcFile) != 0);
	}
	else if(wFunc == FO_COPY || wFunc == FO_MOVE) // コピー/移動
	{
		// コピー/移動先のディレクトリを取得
		TCHAR dest_dir[MAX_PATH], src_dir[MAX_PATH], dest_fn[MAX_PATH];
		GetNormalizedPath(dest_dir, pszDestFile);
		GetNormalizedPath(src_dir, pszSrcFile);

		lstrcpy(dest_fn, pszDestFile);

		f = false;
		int plan = IDYES;
		if(lstrcmp(src_dir, dest_dir) != 0)
		{
			// 現在のディレクトリ以外にコピー/移動
			if(PathFileExists(pszDestFile))
			{
				plan = MessageBoxW(
					hwnd,
					rsprintf(IDS_STRING101, pszDestFile), // can overwrite ?
					rsprintf(IDS_STRING102),
					MB_YESNOCANCEL | MB_ICONQUESTION);
				if(plan == IDYES)
				{
					// 既に存在するファイルを削除
					Unlink(pszDestFile);
				}
				else return plan; // いいえ/キャンセル
			}
		}
		else
		{
			// 現在のディレクトリにコピー/移動
			// -> 名前を変えて続行する
			CreateUniqueName(dest_fn, dest_dir, pszSrcFile, IDS_COPYMOVE_NAME_PATTERN);
		}

		if(plan == IDYES) // 「はい」/問題なしのときのみ実行
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

// 各カラムに関する情報を返す
STDMETHODIMP CSymlinkHdrExt::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO* psci)
{
	if(dwIndex >= sizeof(fmt) / sizeof(COLUMN_FORMAT))
		return S_FALSE;

	psci->scid.fmtid = *_pModule->pguidVer; // 自分の識別子
	psci->scid.pid   = dwIndex; // カラム番号

	psci->vt      = fmt[dwIndex].vt;      // データを文字列として返す
	psci->fmt     = fmt[dwIndex].fmt;     // 中央揃え
	psci->csFlags = fmt[dwIndex].csFlags; // データはINTとして整列
	psci->cChars  = fmt[dwIndex].cChars;  // デフォルト幅(文字数)
	lstrcpyW(psci->wszTitle,
		rsprintfW(fmt[dwIndex].titleId).c_str());
	lstrcpyW(psci->wszDescription,
		rsprintfW(fmt[dwIndex].descId).c_str());

    return S_OK;
}

STDMETHODIMP CSymlinkHdrExt::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	// 識別子が自分のものかチェックする
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

// 最終更新日時
static HRESULT fileudate(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	return filedate(pscd, pvarData, 2);
}

// 作成日時
static HRESULT filecdate(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData)
{
	return filedate(pscd, pvarData, 0);
}

// ファイルサイズ
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

		TCHAR num[27]; // 20桁+カンマ6個+'\0'
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
		// ドライブをまたいでいなかったら相対パス表示にする
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
	// 以前に呼ばれていたら、以前のオブジェクトを解放
	if(m_dataObj)
	{
		m_dataObj->Release();
		m_dataObj = NULL;
	}

	// リファレンスカウントを増やす
	if(pDataObj)
	{
		m_dataObj = pDataObj;
		m_dataObj->AddRef();
	}

	// ターゲットパスを取得
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

	// ドラッグ中のオブジェクトを確認
	CEnumFiles enumfile(m_dataObj);
	if(enumfile.getNumOfFiles() == 0)
	{
		// リンク対象にならないオブジェクト
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	if(m_targetFolder[0] == 0)
	{
		// ターゲットフォルダにファイルを作成できないので断念
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
	}

	// 対象アイテムが1個の場合、表示するメニュー項目を絞る.
	// (複数個の場合は絞ってもキリが無いのであえて絞らない)
	bool enableJunction = true;
	bool enableHardlink = true;
	bool enableSymlink = true;
	wchar_t filename[MAX_PATH];
	if(enumfile.getNumOfFiles() == 1 && enumfile.getNthFileName(0, filename)){
		CheckTargetAttributes(
			filename,
			&enableJunction,
			&enableHardlink,
			&enableSymlink
		);
	}

	// メニュー項目の作成.
	wchar_t buf[1024];
	UINT uidCmd = uidFirstCmd;

	MENUITEMINFO mi = {0};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE;
	mi.fType = MFT_STRING;
	mi.fState = MFS_ENABLED;
	mi.dwTypeData = buf;
	
	wcscpy(buf, rsprintf(IDS_MAKE_JUNCTION)); // "ジャンクションを作る(&J)"
	mi.wID = uidCmd++;
	mi.fState = enableJunction ? MFS_ENABLED : MFS_GRAYED;
	InsertMenuItem(hmenu, uMenuIndex++, TRUE, &mi);

	wcscpy(buf, rsprintf(IDS_MAKE_HARDLINK)); // "ハードリンクを作る(&H)"
	mi.wID = uidCmd++;
	mi.fState = enableHardlink ? MFS_ENABLED : MFS_GRAYED;
	InsertMenuItem(hmenu, uMenuIndex++, TRUE, &mi);

	wcscpy(buf, rsprintf(IDS_MAKE_SYMLINK)); // "シンボリックリンクを作る(&L)"
	mi.wID = uidCmd++;
	mi.fState = enableSymlink ? MFS_ENABLED : MFS_GRAYED;
	InsertMenuItem(hmenu, uMenuIndex++, TRUE, &mi);
	
	// 結果.
	return MAKE_HRESULT(
		SEVERITY_SUCCESS,
		FACILITY_NULL,
		(USHORT)(uidCmd - uidFirstCmd) // 作ったメニューの数
	);
}

// コマンド実行
HRESULT CSymlinkHdrExt::InvokeCommand(LPCMINVOKECOMMANDINFO pInfo)
{
	// リンクの種類.
	int verb = (int)pInfo->lpVerb;
	if(verb < 0 || verb >= 3){
		return E_INVALIDARG;
	}
	LinkType linkType = (LinkType)verb;

	// 対象ファイル列挙.
	CEnumFiles enumfile(m_dataObj);
	UINT uFiles = enumfile.getNumOfFiles();
	if(uFiles < 0)
		return E_INVALIDARG;
	for(UINT u = 0; u < uFiles; u++)
	{
		wchar_t filename[MAX_PATH];
		if(enumfile.getNthFileName(u, filename))
		{
			// リンク名の決定.
			int namePattern = 0;
			if(linkType == LINK_JUNCTION)namePattern = IDS_JUNCTION_NAME_PATTERN;
			else if(linkType == LINK_HARDLINK)namePattern = IDS_HARDLINK_NAME_PATTERN;
			else if(linkType == LINK_SYMLINK)namePattern = IDS_SYMLINK_NAME_PATTERN;
			wchar_t linkname[MAX_PATH];
			CreateUniqueName(
				linkname,
				m_targetFolder,
				filename,
				namePattern
			);

			// リンク生成.
			if(!CreateLink(linkname, filename, linkType))
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
	if(enumfile.getNumOfFiles() > 1) // ファイルが複数だったらページを追加しない
		return E_FAIL;

	// ファイル名の取得
	wchar_t targetPath[MAX_PATH], filename[MAX_PATH];
	if(!enumfile.getNthFileName(0, filename))
		return E_FAIL; // ファイル名が取得できなければページを追加しない

	// リンク先の抽出
	if(!GetLinkTargetPath(targetPath, filename))
		return E_FAIL; // リンクでなければページを追加しない

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

// プロパティシート作成・削除時に呼ばれるハンドラ
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

// プロパティシートのウィンドウプロシージャ(インスタンスへのディスパッチ)
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
	// インスタンス初期化前
	return FALSE;
}

// ダイアログの初期化(WM_INITDIALOG)
INT_PTR CSymlinkHdrExt::InitDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEnumFiles enumfile(m_dataObj);
	if(enumfile.getNumOfFiles() > 1) // ファイルが複数だったらページを追加しない
		return TRUE;

	// ファイル名の取得
	wchar_t targetPath[MAX_PATH], filename[MAX_PATH];
	enumfile.getNthFileName(0, filename);
	
	/*
	// 属性の取得
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

	// リンク先の抽出
	BOOL fLinkExist = GetLinkTargetPath(targetPath, filename);
	if(!fLinkExist) targetPath[0] = 0;

	if(wcsncmp(targetPath, L"Volume{", 7) == 0)
	{
		// "\\?\Volume{...}\"の処理
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
			// どこにもマウントされていないボリューム
			lstrcpyW(targetPath, rsprintf(IDS_STRING124).c_str());
			fLinkExist = FALSE;
		}
		
		/*
		DWORD size;
		if(!GetVolumePathNamesForVolumeName(volumeName, targetPath, MAX_PATH, &size))
		{
			// どこにもマウントされていないボリューム
			lstrcpyW(targetPath, rsprintf(IDS_STRING124).c_str());
			fLinkExist = FALSE;
		}
		*/

		// マウントポイント
		SetDlgItemText(hwndDlg, IDC_SYMLINK_TYPE, rsprintf(IDS_STRING126));
	}
	else
	{
		// ジャンクション
		SetDlgItemText(hwndDlg, IDC_SYMLINK_TYPE, rsprintf(IDS_STRING125));
	}

	SetDlgItemText(hwndDlg, IDC_PATHEDIT, targetPath);
	EnableWindow(GetDlgItem(hwndDlg, IDC_PATHEDIT), fLinkExist);
	EnableWindow(GetDlgItem(hwndDlg, IDC_GOBUTTON), fLinkExist);
	return TRUE;
}

// [移動(G)]ボタンの処理
INT_PTR CSymlinkHdrExt::ProcessGoButton(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEnumFiles enumfile(m_dataObj);
	if(enumfile.getNumOfFiles() > 1) // ファイルが複数だったらページを追加しない
		return TRUE;

	// リンク先の抽出
	wchar_t targetPath[MAX_PATH], filename[MAX_PATH];
	enumfile.getNthFileName(0, filename);
	if(GetLinkTargetPath(targetPath, filename))
		ShellExecute(hwndDlg, NULL, targetPath, NULL, NULL, SW_SHOW); // 開く

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
