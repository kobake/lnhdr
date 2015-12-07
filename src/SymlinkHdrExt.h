// SymlinkHdrExt.h : Declaration of the CSymlinkHdrExt

#pragma once
#include "resource.h"       // main symbols

// ISymlinkHdrExt
[
	object,
	uuid("5C38B955-9B8A-4540-A867-C0375791B13E"),
	dual,	helpstring("ISymlinkHdrExt Interface"),
	pointer_default(unique)
]
__interface ISymlinkHdrExt : IDispatch
{
};

// CSymlinkHdrExt
[
	coclass,
	threading(apartment),
	vi_progid("lnhdr.SymlinkHdrExt"),
	progid("lnhdr.SymlinkHdrExt.1"),
	version(1.0),
	uuid("37DCA5D7-ADFB-4C7F-8556-CACA48F81BCF"),
	helpstring("SymlinkHdrExt Class")
]
class ATL_NO_VTABLE CSymlinkHdrExt : 
	public ISymlinkHdrExt,
	public IShellExtInit,		// シェル拡張初期化
	public ICopyHook,			// コピーフック
	public IColumnProvider,		// カラムプロバイダ
	public IShellPropSheetExt,	// プロパティシート
	public IContextMenu,		// コンテキストメニュー
	public IShellIconOverlayIdentifier
{
public:
	CSymlinkHdrExt() : m_dataObj(NULL)
	{
		m_targetFolder[0] = 0;
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	
	void FinalRelease() 
	{
	}

BEGIN_COM_MAP(CSymlinkHdrExt)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
	COM_INTERFACE_ENTRY_IID(IID_IShellCopyHook, ICopyHook)
	COM_INTERFACE_ENTRY(IShellPropSheetExt)
	COM_INTERFACE_ENTRY_IID(IID_IColumnProvider, IColumnProvider)
	COM_INTERFACE_ENTRY_IID(IID_IShellIconOverlayIdentifier, IShellIconOverlayIdentifier)
END_COM_MAP()

public:
	// ICopyHook
	STDMETHOD_(UINT, CopyCallback)(
		HWND hwnd, UINT wFunc, UINT wFlags,
		LPCTSTR pszSrcFile,  DWORD dwSrcAttribs,
		LPCTSTR pszDestFile, DWORD dwDestAttribs);

	// IColumnProvider
	STDMETHOD(Initialize)(LPCSHCOLUMNINIT psci);
	STDMETHOD(GetColumnInfo)(DWORD dwIndex, SHCOLUMNINFO* psci);
	STDMETHOD(GetItemData)(
		LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pvarData);

	// IShellExtInit
	STDMETHOD(Initialize)(LPCITEMIDLIST, LPDATAOBJECT, HKEY);

	// ISHellPropSheetExt
	STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE, LPARAM);
	STDMETHOD(ReplacePage)(UINT, LPFNADDPROPSHEETPAGE, LPARAM) {return E_NOTIMPL;}

	// IContextMenu
	STDMETHOD(GetCommandString)(UINT_PTR, UINT, UINT*, LPSTR, UINT);
	STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO);
	STDMETHOD(QueryContextMenu)(HMENU, UINT, UINT, UINT, UINT);
	
	// IShellIconOverlayIdentifier
	STDMETHOD(GetOverlayInfo)(LPWSTR, int, int*, DWORD*);
	STDMETHOD(GetPriority)(int*);
	STDMETHOD(IsMemberOf)(LPCWSTR, DWORD);

private:
	UINT dlgCallback(HWND hwnd, UINT u, LPPROPSHEETPAGE ppsp);
	static UINT CALLBACK psDlgCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		return ((CSymlinkHdrExt *)ppsp->lParam)->dlgCallback(hwnd, uMsg, ppsp);
	}
	static INT_PTR CALLBACK dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR InitDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR ProcessGoButton(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LPDATAOBJECT m_dataObj;
	TCHAR m_targetFolder[MAX_PATH + 1];

};

