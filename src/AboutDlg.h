// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CAboutDlg : public CDialogImpl<CAboutDlg>
{
public:
	enum { IDD = IDD_SPLASH };

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
	END_MSG_MAP()
	
	CAboutDlg() : m_hBitmap(NULL)
	{
		m_hBitmap = (HBITMAP)LoadImage(
			_pModule->GetModuleInstance(),
			(LPCTSTR)IDB_SPLASH,
			IMAGE_BITMAP,
			0, 0, 0);
		GetObject(m_hBitmap, sizeof(m_info), &m_info);
	}
	
	~CAboutDlg()
	{
		if(m_hBitmap)
			DeleteObject((HGDIOBJ)m_hBitmap);
	}

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		/*m_image.LoadFromResource(
			_pModule->GetModuleInstance(), IDB_SPLASH);
		*/
		
		SetWindowPos(NULL, 0, 0, m_info.bmWidth, m_info.bmHeight, SWP_NOMOVE);
		CenterWindow(GetParent());
		return TRUE;
	}
	
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		PAINTSTRUCT ps;
		BeginPaint(&ps);
		::SetICMMode(ps.hdc, ICM_ON);
		//m_image.Draw(ps.hdc, 0, 0);
		
		HDC hdc = CreateCompatibleDC(ps.hdc);
		HGDIOBJ hPrevBmp = SelectObject(hdc, m_hBitmap);
		
		BitBlt(
			ps.hdc, 0, 0, m_info.bmWidth, m_info.bmHeight,
			hdc, 0, 0, SRCCOPY);
		
		SelectObject(hdc, hPrevBmp);
		DeleteDC(hdc);
		
		EndPaint(&ps);
		return 0;
	}
	
	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		EndDialog(0);
		return 0;
	}
	
	LRESULT OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// When deactivated, automatically closes the dialog
		if((wParam & 0xffff) == WA_INACTIVE)
			EndDialog(0);
		return 0;
	}

private:
	HBITMAP m_hBitmap;
	BITMAP m_info;
	//CImage m_image;
};
