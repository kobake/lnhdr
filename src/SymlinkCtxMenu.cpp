// SymlinkCtxMenu.cpp : Implementation of CSymlinkCtxMenu

#include "stdafx.h"
#include "SymlinkCtxMenu.h"

#include "utils.h"
#include "link.h"
#include "rsprintf.h"
#include "AboutDlg.h"
#include <process.h>

// CSymlinkCtxMenu

//---------------------------------------------------------------------------
// IShellExtInit::Initialize
//
HRESULT CSymlinkCtxMenu::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hProgID)
{
	if(m_dataObj)
	{
		m_dataObj->Release();
		m_dataObj = NULL;
	}

	if(pDataObj)
	{
		m_dataObj = pDataObj;
		m_dataObj->AddRef();
	}

	return NOERROR;
}

//---------------------------------------------------------------------------
// IContextMenu::QueryContextMenu
//
HRESULT CSymlinkCtxMenu::QueryContextMenu(
	HMENU hMenu,
	UINT  uMenuIndex,
	UINT  uidFirstCmd,
	UINT  uidLastCmd,
	UINT  uFlags)
{
	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything
	if(uFlags & CMF_DEFAULTONLY)
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

	CEnumFiles files(m_dataObj);
	
	// if no "real" files, do nothing
	if(files.getNumOfFiles() == 0)
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);


	m_idBase = uidFirstCmd;
	UINT uidCmd = uidFirstCmd;
	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	
	// Create submenu
	HMENU hSubMenu = CreateMenu();

	// Add separator
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, uMenuIndex++, TRUE, &mii);

	// "&Tools"
	CString str;
	str.LoadString(IDS_ESPRESSOTOOLS);

	// Add Menu container
	mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
	mii.wID = uidCmd;
	mii.fType = MFT_STRING;
	mii.hSubMenu = hSubMenu;
	mii.dwTypeData = (LPTSTR)str.GetBuffer();
	InsertMenuItem(hMenu, uMenuIndex++, TRUE, &mii);

	CommandInfo ci;
	ci.caption.LoadString(IDS_ESPRESSOTOOLS);
	ci.help.LoadString(IDS_ESPRESSOTOOLS_HELP);
	ci.verb = _T("espresso_tools");
	ci.menuHandler = NULL;

	m_commands.clear();
	m_commands[0] = ci;

	// Add separator
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, uMenuIndex++, TRUE, &mii);

	static const MenuItem menu[] = {
		{
			IDS_OPENLINKTARGET, IDS_OPENLINKTARGET_HELP, IDS_OPENLINKTARGET_VERB,
			FolderOnly, RejectMultipleFiles, &CSymlinkCtxMenu::checkLinkTarget,
			&CSymlinkCtxMenu::openLinkTarget
		},
		{
			IDS_CMDEXE, IDS_CMDEXE_HELP, IDS_CMDEXE_VERB,
			FolderOnly, RejectMultipleFiles, NULL,
			&CSymlinkCtxMenu::cmdHere
		},
		{
			IDS_COPYPATH, IDS_COPYPATH_HELP, IDS_COPYPATH_VERB,
			AcceptAny, AcceptMultipleFiles, NULL,
			&CSymlinkCtxMenu::copyPath
		},
		{
			IDS_COPYMD5, IDS_COPYMD5_HELP, IDS_COPYMD5_VERB,
			FileOnly, AcceptMultipleFiles, NULL,
			&CSymlinkCtxMenu::copyMD5
		},
		{
			IDS_COPYBASE64, IDS_COPYBASE64_HELP, IDS_COPYBASE64_VERB,
			FileOnly, RejectMultipleFiles, &CSymlinkCtxMenu::skipLargeFiles,
			&CSymlinkCtxMenu::copyBASE64
		},
		{
			IDS_SHOWRUNDLG, IDS_SHOWRUNDLG_HELP, IDS_SHOWRUNDLG_VERB,
			AcceptAny, RejectMultipleFiles, NULL,
			&CSymlinkCtxMenu::runDialog
		},
		{
			IDS_SEPARATOR, 0, 0,
			AcceptAny, AcceptMultipleFiles, NULL, NULL
		},
		{
			IDS_SHOWABOUT, IDS_SHOWABOUT_HELP, IDS_SHOWABOUT_VERB,
			AcceptAny, AcceptMultipleFiles, NULL,
			&CSymlinkCtxMenu::about
		},
		{0, 0, 0, AcceptAny, AcceptMultipleFiles, NULL, NULL}
	};

	UINT offset = 1;
	UINT pos = 0;

	UINT co = ConstructMenu(hSubMenu, menu, files, pos, uidCmd, offset);

	pos += co;
	offset += co;

	return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, (USHORT)offset);
}

//---------------------------------------------------------------------------
// IContextMenu::InvokeCommand
//
HRESULT CSymlinkCtxMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pInfo)
{
	// If selecting several files, do nothing
	CEnumFiles enumfile(m_dataObj);
	if(!HIWORD(pInfo->lpVerb))
	{
		UINT idCmd = LOWORD(pInfo->lpVerb);
		if(pInfo->fMask & CMIC_MASK_FLAG_NO_UI)
			return E_FAIL; // we suppose GUI interaction.
		
		CommandMap::const_iterator it = m_commands.find(idCmd);
		if(it == m_commands.end())
			return E_INVALIDARG; // No such command
		
		if(!it->second.multipleFiles && enumfile.getNumOfFiles() != 1)
			return E_FAIL; // Cannot accept multiple files

		if((this->*m_commands[idCmd].menuHandler)(pInfo, enumfile))
			return NOERROR;
	}
	return E_FAIL;
}

//---------------------------------------------------------------------------
// IContextMenu::GetCommandString
//
HRESULT CSymlinkCtxMenu::GetCommandString(
	UINT_PTR idCmd,
	UINT uFlags,
	UINT *pwReserved,
	LPSTR pszName,
	UINT cchMax)
{
	if(m_commands.find(idCmd) == m_commands.end())
		return E_FAIL; // No such command

	const CommandInfo& ci = m_commands[idCmd];
	if(uFlags == GCS_HELPTEXTA)
	{
		// Sets pszName to an ANSI string containing the help text for the
		// command.
		copyStrA(pszName, cchMax, ci.help);
		return NOERROR;
	}
	else if(uFlags == GCS_HELPTEXTW)
	{
		// Sets pszName to a Unicode string containing the help text for the
		// command.
		copyStrW(pszName, cchMax, ci.help);
		return NOERROR;
	}
	else if(uFlags ==  GCS_VERBA)
	{
		// Sets pszName to an ANSI string containing the language-independent
		// command name for the menu item.
		copyStrA(pszName, cchMax, ci.verb);
		return NOERROR;
	}
	else if(uFlags ==  GCS_VERBW)
	{
		// Sets pszName to a Unicode string containing the language-
		// independent command name for the menu item.
		copyStrW(pszName, cchMax, ci.verb);
		return NOERROR;
	}
	else if(uFlags == GCS_VALIDATEA || uFlags == GCS_VALIDATEW)
	{
		// Returns S_OK if the menu item exists, or S_FALSE otherwise.
		return NOERROR;
	}
	return E_FAIL; // Unknown flag
}

//---------------------------------------------------------------------------
// This method creates the actual menu from the MenuItem structure.
//
UINT CSymlinkCtxMenu::ConstructMenu(HMENU hMenu, const MenuItem* items, CEnumFiles& files, UINT pos, UINT id, UINT offset)
{
	enum {Folder = 1, File = 2};
	DWORD types = 0;
	UINT nFiles = files.getNumOfFiles();
	if(nFiles == 0)
		return 0;

	for(UINT i = 0; i < nFiles; i++)
	{
		TCHAR filename[MAX_PATH];
		files.getNthFileName(i, filename);
		if(PathIsDirectory(filename))
			types |= Folder;
		else
			types |= File;
	}
	
	// construct menu
	int co = 0;
	for(; items->captionId != 0; items++)
	{
		if(nFiles != 1 && items->multipleFiles == RejectMultipleFiles)
			continue; // Cannot deal with multiple files
		
		if(types & ~(DWORD)items->acceptableType)
			continue; // Some of the files are unacceptable type
		
		if(items->menuValidator &&
			!((this->*items->menuValidator)(files)))
			continue; // It should not be shown
		
		MENUITEMINFO mii;
		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(mii);

		if(items->captionId == IDS_SEPARATOR)
		{
			mii.fMask = MIIM_FTYPE;
			mii.fType = MFT_SEPARATOR;
			InsertMenuItem(hMenu, pos + co, TRUE, &mii);
			co++;
		}
		else
		{
			CommandInfo ci;
			ci.caption.LoadString(items->captionId);
			ci.help.LoadString(items->helpId);
			ci.verb.LoadString(items->verbId);
			ci.menuHandler = items->menuHandler;
			ci.menuValidator = items->menuValidator;
			ci.acceptableType = items->acceptableType;
			ci.multipleFiles = items->multipleFiles;

			mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
			mii.fType = MFT_STRING;
			mii.wID = id + offset + co;
			mii.dwTypeData = (LPTSTR)ci.caption.GetBuffer();
			InsertMenuItem(hMenu, pos + co, TRUE, &mii);
			m_commands[offset + co] = ci;
			co++;
		}
	}
	return co;
}

//---------------------------------------------------------------------------
bool CSymlinkCtxMenu::checkLinkTarget(CEnumFiles& files)
{
	if(files.getNumOfFiles() != 1)
		return false;
	
	TCHAR filename[MAX_PATH], targetPath[MAX_PATH];
	files.getNthFileName(0, filename);
	if(GetLinkTargetPath(targetPath, filename))
		return true;
	return false;
}

//---------------------------------------------------------------------------
bool CSymlinkCtxMenu::skipLargeFiles(CEnumFiles& files)
{
	UINT n = files.getNumOfFiles();
	for(UINT i = 0; i < n; i++)
	{
		TCHAR filename[MAX_PATH];
		files.getNthFileName(i, filename);
		HANDLE hFile = CreateFile(
			filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			continue;

		DWORD high = 0;
		DWORD dwSize = GetFileSize(hFile, &high);
		CloseHandle(hFile);
		if(high || dwSize >= 1024 * 1024 * 10)
			return false;
	}
	return true;
}

//---------------------------------------------------------------------------
bool CSymlinkCtxMenu::openLinkTarget(LPCMINVOKECOMMANDINFO pici, CEnumFiles& files)
{
	TCHAR filename[MAX_PATH], targetPath[MAX_PATH];
	files.getNthFileName(0, filename);
	if(GetLinkTargetPath(targetPath, filename))
		ShellExecute(pici->hwnd, NULL, targetPath, NULL, NULL, SW_SHOW);
	return true;
}

//---------------------------------------------------------------------------
// Open cmd.exe here
bool CSymlinkCtxMenu::cmdHere(LPCMINVOKECOMMANDINFO pici, CEnumFiles& files)
{
	if(files.getNumOfFiles() != 1)
		return false;

	TCHAR filename[MAX_PATH];
	files.getNthFileName(0, filename);

	// "%SystemRoot%\System32\cmd.exe"
	TCHAR cmdpath[MAX_PATH];
	ExpandEnvironmentStrings(
		_T("%SystemRoot%\\system32\\cmd.exe"), cmdpath, MAX_PATH);
	
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if(CreateProcess(
		NULL, cmdpath, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, NULL, filename, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	return true;
}

//---------------------------------------------------------------------------
// Copy the full path of the selected file
bool CSymlinkCtxMenu::copyPath(LPCMINVOKECOMMANDINFO pici, CEnumFiles& files)
{
	UINT n = files.getNumOfFiles();
	CString list;
	for(UINT i = 0; i < n; i++)
	{
		TCHAR filename[MAX_PATH];
		files.getNthFileName(i, filename);
		list += filename;
		if(n > 1)
			list += _T("\r\n");
	}
	
	return CopyToClipboard(pici->hwnd, list);
}

//---------------------------------------------------------------------------
// Copy MD5 hashes of the selected files
bool CSymlinkCtxMenu::copyMD5(LPCMINVOKECOMMANDINFO pici, CEnumFiles& files)
{
	UINT n = files.getNumOfFiles();
	CString list;
	for(UINT i = 0; i < n; i++)
	{
		TCHAR filename[MAX_PATH];
		files.getNthFileName(i, filename);
		list += getMD5Hash(filename) + _T(" *") + PathFindFileName(filename);
		if(n > 1)
			list += _T("\r\n");
	}
	return CopyToClipboard(pici->hwnd, list);
}

//---------------------------------------------------------------------------
// Copy BASE64 encoded form of the selected files
bool CSymlinkCtxMenu::copyBASE64(LPCMINVOKECOMMANDINFO pici, CEnumFiles& files)
{
	TCHAR filename[MAX_PATH];
	files.getNthFileName(0, filename);
	return CopyToClipboard(pici->hwnd, base64Encode(filename));
}

//---------------------------------------------------------------------------
struct RUNDIALOG_DATA
{
	CString caption;
	CString args;
	HWND hwnd;

	RUNDIALOG_DATA(LPCTSTR cap, LPCTSTR arg, HWND hWnd)
		: caption(cap), args(arg), hwnd(hWnd)
	{
	}
};

//---------------------------------------------------------------------------
static unsigned int __stdcall dialogFinder(void* param)
{
	RUNDIALOG_DATA* rd = (RUNDIALOG_DATA*)param;
	Sleep(10);
	HWND hwnd = FindWindow(NULL, rd->caption);
	if(!hwnd)
	{
		delete rd;
		return 0;
	}
	
	HWND hwndCb = FindWindowEx(hwnd, NULL, _T("ComboBox"), NULL);
	if(!hwndCb)
	{
		delete rd;
		return 0;
	}

	HWND hwndEdit = FindWindowEx(hwndCb, NULL, _T("Edit"), NULL);
	if(hwndEdit)
	{
		SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)rd->args);
		SendMessage(hwndEdit, EM_SETSEL, 0, (LPARAM)-1);
	}
	delete rd;
	return 0;
}

//---------------------------------------------------------------------------
// Show "Run" dialog
bool CSymlinkCtxMenu::runDialog(LPCMINVOKECOMMANDINFO pici, CEnumFiles& files)
{
	UINT n = files.getNumOfFiles();
	CString list = _T("\"");
	for(UINT i = 0; i < n; i++)
	{
		TCHAR filename[MAX_PATH];
		files.getNthFileName(i, filename);
		list += filename;
		if(i + 1 < n)
			list += _T("\" ");
		else
			list += _T("\"");
	}

	enum {SHRD_NOBROWSE = 0x01, SHRD_NOMRU = 0x02};

	TCHAR dir[MAX_PATH];
	files.getNthFileName(0, dir);
	*PathFindFileName(dir) = 0;

	typedef void (WINAPI *SHRUNDIALOG)
		(HWND hwnd, HICON hIcon, LPCTSTR lpszPath,
		LPCTSTR lpszTitle, LPCTSTR lpszPrompt, int uFlags);
	HMODULE hShell32 = LoadLibrary(_T("shell32.dll"));
	if(!hShell32) return false;

	// This code is supposed to run on Windows 2000/XP/2003.
	SHRUNDIALOG SHRunDialog
		= (SHRUNDIALOG)GetProcAddress(hShell32, (LPCSTR)61);

	CString caption, message;
	caption.LoadString(IDS_STRING150);
	message.LoadString(IDS_STRING151);

	RUNDIALOG_DATA* data = new RUNDIALOG_DATA(caption, list, pici->hwnd);
	HANDLE hThread = (HANDLE)_beginthreadex(
		NULL, 0, dialogFinder, (void*)data, 0, NULL);
	CloseHandle(hThread);

	SHRunDialog(pici->hwnd, NULL, dir, caption, message, 0);
	FreeLibrary(hShell32);
	return true;
}

//---------------------------------------------------------------------------
// Show About...
bool CSymlinkCtxMenu::about(LPCMINVOKECOMMANDINFO pici, CEnumFiles& files)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return true;
}
