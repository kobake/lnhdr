// SymlinkCtxMenu.h : Declaration of the CSymlinkCtxMenu

#pragma once
#include "utils.h"
#include "resource.h"       // main symbols
#include <map>

// ISymlinkCtxMenu
[
	object,
	uuid("C6AD2837-7569-4893-8337-74797F5728D0"),
	dual,	helpstring("ISymlinkCtxMenu Interface"),
	pointer_default(unique)
]
__interface ISymlinkCtxMenu : IDispatch
{
};

// CSymlinkCtxMenu
[
	coclass,
	threading(apartment),
	vi_progid("lnhdr.SymlinkCtxMenu"),
	progid("lnhdr.SymlinkCtxMenu.1"),
	version(1.0),
	uuid("6A055C64-6717-49df-9D23-A182BA4F5452"),
	helpstring("SymlinkCtxMenu Class")
]
class ATL_NO_VTABLE CSymlinkCtxMenu : 
	public ISymlinkCtxMenu,
	public IShellExtInit,		// シェル拡張初期化
	public IContextMenu			// コンテキストメニュー

{
public:
	CSymlinkCtxMenu() : m_dataObj(NULL)
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

BEGIN_COM_MAP(CSymlinkCtxMenu)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
END_COM_MAP()

public:
	// IShellExtInit
	STDMETHOD(Initialize)(LPCITEMIDLIST, LPDATAOBJECT, HKEY);

	// IContextMenu
	STDMETHOD(GetCommandString)(UINT_PTR, UINT, UINT*, LPSTR, UINT);
	STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO);
	STDMETHOD(QueryContextMenu)(HMENU, UINT, UINT, UINT, UINT);

private:
	LPDATAOBJECT m_dataObj;

	typedef bool (CSymlinkCtxMenu::*MenuHandler)(LPCMINVOKECOMMANDINFO, CEnumFiles&);
	typedef bool (CSymlinkCtxMenu::*MenuValidator)(CEnumFiles&);
	
	enum AcceptableType
	{
		AcceptAny = 3,
		FolderOnly = 1,
		FileOnly = 2,
	};
	
	enum MultipleFiles
	{
		RejectMultipleFiles = 0,
		AcceptMultipleFiles = 1,
	};

	struct CommandInfo
	{
		AcceptableType acceptableType;
		MultipleFiles multipleFiles;
		MenuValidator menuValidator;
		MenuHandler menuHandler;
		CString verb;
		CString caption;
		CString help;
	};

	struct MenuItem
	{
		UINT captionId;
		UINT helpId;
		UINT verbId;
		AcceptableType acceptableType;
		MultipleFiles multipleFiles;
		MenuValidator menuValidator;
		MenuHandler menuHandler;
	};

	typedef std::map<UINT_PTR, CommandInfo> CommandMap;
	CommandMap m_commands;
	UINT m_idBase;
	
	static const UINT IDS_SEPARATOR = 1;

	UINT ConstructMenu(
		HMENU hMenu, const MenuItem* items, CEnumFiles& files, UINT pos, UINT id, UINT offset);
	
	bool checkLinkTarget(CEnumFiles&);
	bool skipLargeFiles(CEnumFiles&);
	bool openLinkTarget(LPCMINVOKECOMMANDINFO, CEnumFiles&);
	bool cmdHere(LPCMINVOKECOMMANDINFO, CEnumFiles&);
	bool copyPath(LPCMINVOKECOMMANDINFO, CEnumFiles&);
	bool copyMD5(LPCMINVOKECOMMANDINFO, CEnumFiles&);
	bool copyBASE64(LPCMINVOKECOMMANDINFO, CEnumFiles&);
	bool runDialog(LPCMINVOKECOMMANDINFO, CEnumFiles&);
	bool about(LPCMINVOKECOMMANDINFO, CEnumFiles&);
};

