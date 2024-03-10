#include "pch.h"
#include "framework.h"
#include "NaraTimer.h"
#include "NaraTimerDlg.h"
#include "afxdialogex.h"
#include "NaraUtil.h"

#define RED					RGB(249, 99, 101)
#define WHITE				RGB(255, 255, 255)
#define CHK_INTERVAL		(300)
#define LIST_GAP			(10)
#define HUD_FONT_SIZE		(mViewMode == VIEW_WATCH? ROUND(mRadius * 0.4f) : ROUND(mRoundCorner * 0.4f))
COLORREF BK_COLOR = WHITE;
COLORREF GRID_COLOR = WHITE;
COLORREF BORDER_COLOR = RED;

float TIMES_UP = -100.f;
BOOL TITLE_CHANGING = FALSE;

#define VIEW_WATCH			(0)
#define VIEW_LIST			(1)

#define TID_TICK			(0)
#define TID_REFRESH			(1)
#define TID_TIMESUP			(2)
#define TID_HELP			(3)

#pragma comment(lib, "winmm")
#include <mmsystem.h>

#define LBUTTON_DOWN		(GetKeyState(VK_LBUTTON) & 0x8000)
#define CTRL_DOWN			(GetKeyState(VK_CONTROL) & 0x8000)
#define SHIFT_DOWN			(GetKeyState(VK_LSHIFT) & 0x8000)
#define SET_DOCKED_STYLE	ModifyStyle(WS_CAPTION|WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX, WS_POPUP)
#define SET_WINDOWED_STYLE	ModifyStyle(WS_CAPTION|WS_POPUP, WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define SET_BUTTON(id, idi, idi_hover) { mButtonIcon[id] = idi; mButtonIconHover[id] = idi_hover; }
#define DEFINE_PEN(name, color, opaque, width)	Pen name(Color(opaque, GetRValue(color), GetGValue(color), GetBValue(color)), width)

#define WM_PIN				(WM_USER+1)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static COLORREF blend_color(COLORREF c0, COLORREF c1)
{
	int r = (GetRValue(c0) + GetRValue(c1) + 1) >> 1;
	int g = (GetGValue(c0) + GetGValue(c1) + 1) >> 1;
	int b = (GetBValue(c0) + GetBValue(c1) + 1) >> 1;
	return RGB(r, g, b);
}

///////////////////////////////////////////////////////////////////////////////
// help dialog 
#define TYPE_HEADING		(0)
#define TYPE_TEXT			(1)
#define TYPE_BOLDTEXT		(2)
#define TYPE_ICON			(3)
#define MAX_NUM_ITEM		(20)
class NaraMessageBox : public NaraDialog
{
public:
	NaraMessageBox(CWnd * parent, BOOL OK_Cancel=FALSE);
	int SetY(int y);
	void Clear(void);
	int AddHeading(CString str);
	int AddString(CString str);
	int AddBoldString(CString str);
	int AddIcon(int id);
	void EnableTimer(int id) { mTimerID = id; mTimerIdx = 0; }
	void DisableTimer(void) { mTimerID = -1; }
protected:
	BOOL mOKCancel;
	int mType[MAX_NUM_ITEM];
	CString mText[MAX_NUM_ITEM];
	CFont mFont;
	CFont mFontHeading;
	CFont mFontBold;
	int mFontHeight;
	int mCnt;
	int mDlgHeight;
	BOOL mOnOK;
	RECT mOKrt;
	int mY;
	int mTimerID;
	UINT mTimerIdx;
	int AddText(int type, CString str, int h);

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC * pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave(void);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(NaraMessageBox, NaraDialog)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

NaraMessageBox::NaraMessageBox(CWnd * parent, BOOL OK_Cancel) : NaraDialog(IDD_NARATIMER_DIALOG, parent)
{
	mOKCancel = OK_Cancel;
	mFontHeight = 0;
	Clear();
	mOnOK = FALSE;
	mY = -1;
	mTimerID = -1;

	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
	mFont.CreateFontIndirect(&metrics.lfMessageFont);
	mFontHeight = metrics.lfMessageFont.lfHeight;

	metrics.lfMessageFont.lfHeight = ROUND(mFontHeight * 1.2);
	metrics.lfMessageFont.lfWeight = FW_BOLD;
	mFontBold.CreateFontIndirect(&metrics.lfMessageFont);

	metrics.lfMessageFont.lfHeight = mFontHeight * 2;
	mFontHeading.CreateFontIndirect(&metrics.lfMessageFont);

	mFontHeight = abs(mFontHeight);
	mDlgHeight += mFontHeight * 2; // space between texts and the OK button
	mDlgHeight += mFontHeight * 3; // OK button
}

BOOL NaraMessageBox::OnInitDialog()
{
	NaraDialog::OnInitDialog();
	ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, 0);
	RECT wrt;
	mParent->GetWindowRect(&wrt);
	wrt.left += 10;
	wrt.right -= 10;
	if(wrt.right - wrt.left < 300)
	{
		int m = (300 - (wrt.right - wrt.left)) >> 1;
		wrt.left -= m;
		wrt.right += m;
	}

	CClientDC dc(this);
	CFont * fonto = dc.SelectObject(&mFont);
	RECT rt;
	memcpy(&rt, &wrt, sizeof(RECT));
	rt.left += mResizeMargin;
	for(int n = 0; n < mCnt; n++)
	{
		if(mType[n] == TYPE_TEXT || mType[n] == TYPE_BOLDTEXT)
		{
			dc.DrawText(mText[n], &rt, DT_SINGLELINE | DT_CALCRECT);
			if(rt.right > wrt.right - mResizeMargin)
			{
				wchar_t * buf = mText[n].GetBuffer();
				for(int i = mText[n].GetLength() - 1; i >= 0; i--)
				{
					if(buf[i] != L' ' && buf[i + 1] == L' ')
					{
						CString left = mText[n].Left(i + 1);
						dc.DrawText(left, &rt, DT_SINGLELINE | DT_CALCRECT);
						if(rt.right <= wrt.right - mResizeMargin)
						{
							CString right = mText[n].Right(mText[n].GetLength() - 1 - i);
							mText[n] = left;
							mCnt++;
							mDlgHeight += mFontHeight * 2;
							for(int m=mCnt-1; m>=n+2; m--)
							{
								mType[m] = mType[m - 1];
								mText[m] = mText[m - 1];
							}
							mType[n + 1] = mType[n];
							mText[n + 1] = right.TrimLeft();
							break;
						}
					}
				}
			}
		}
	}
	dc.SelectObject(fonto);

	wrt.top = (mY == -1 ? max(((wrt.bottom + wrt.top) >> 1) - (mDlgHeight >> 1), wrt.top) : mY);
	wrt.bottom = wrt.top + mDlgHeight;
	MoveWindow(&wrt, FALSE);

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	GetClientRect(&rt);
	rt.top = rt.bottom - mFontHeight * 3;
	mOnOK = PT_IN_RECT(pt, rt);

	if(mTimerID >= 0)
	{
		SetTimer(mTimerID, 300, NULL);
	}

	return TRUE;
}

BOOL NaraMessageBox::PreTranslateMessage(MSG * pMsg)
{
	switch(pMsg->message)
	{
	case WM_KEYDOWN:
		switch(pMsg->wParam)
		{
		case VK_RETURN:
		case VK_SPACE:
			pMsg->wParam = VK_RETURN;
		}
	}
	return NaraDialog::PreTranslateMessage(pMsg);
}

int NaraMessageBox::SetY(int y)
{
	POINT pt = { 0, y };
	mParent->ClientToScreen(&pt);
	mY = pt.y;
	return 0;
}

void NaraMessageBox::Clear(void)
{
	mCnt = 0;
	mDlgHeight = 20 + mFontHeight * 2 + mFontHeight * 3;
	mY = -1;
	mOnOK = FALSE;
}

int NaraMessageBox::AddText(int type, CString str, int h)
{
	if(mCnt >= MAX_NUM_ITEM) return -1;

	mType[mCnt] = type;
	mText[mCnt] = str;
	mCnt++;
	mDlgHeight += mFontHeight + h;
	return 0;
}

int NaraMessageBox::AddHeading(CString str)
{
	return AddText(TYPE_HEADING, str, mFontHeight * 2);
}

int NaraMessageBox::AddString(CString str)
{
	return AddText(TYPE_TEXT, str, mFontHeight);
}

int NaraMessageBox::AddBoldString(CString str)
{
	return AddText(TYPE_BOLDTEXT, str, mFontHeight);
}

int NaraMessageBox::AddIcon(int id)
{
	CString str;
	str.Format(L"%d", id);
	return AddText(TYPE_ICON, str, 64);
}

BOOL NaraMessageBox::OnEraseBkgnd(CDC* pDC)
{
	return 0;
}

void NaraMessageBox::OnPaint()
{
	NaraDialog::OnPaint();
	CClientDC dc(this);
	CDC mdc;
	mdc.CreateCompatibleDC(&dc);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, mCrt.right, mCrt.bottom);
	CBitmap * bmpo = mdc.SelectObject(&bmp);
	CFont * fonto = mdc.SelectObject(&mFont);

	mdc.FillSolidRect(&mCrt, RGB(64, 64, 64));
	mdc.SetTextColor(WHITE);
	int x = 10;
	int y = 10;
	for(int i = 0; i < mCnt; i++)
	{
		RECT rt = { x, y, mCrt.right - x, y + mFontHeight * 3 };
		if(mType[i] == TYPE_HEADING)
		{
			CFont * fonto = mdc.SelectObject(&mFontHeading);
			mdc.DrawText(mText[i], &rt, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			mdc.SelectObject(fonto);
			y += mFontHeight * 3;
		}
		else if(mType[i] == TYPE_TEXT)
		{
			mdc.DrawText(mText[i], &rt, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			y += mFontHeight * 2;
		}
		else if(mType[i] == TYPE_BOLDTEXT)
		{
			fonto = mdc.SelectObject(&mFontBold);
			mdc.DrawText(mText[i], &rt, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			mdc.SelectObject(fonto);
			y += mFontHeight * 2;
		}
		else if(mType[i] == TYPE_ICON)
		{
			int sz = 64;
			int id = _ttoi(mText[i]);
			HICON icon = static_cast<HICON>(::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(id), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR));
			DrawIconEx(mdc.m_hDC, (mCrt.right >> 1) - (sz >> 1), y, icon, sz, sz, 0, NULL, DI_NORMAL);
			DestroyIcon(icon);
			y += 64 + mFontHeight;
		}
	}
	y += mFontHeight * 2;
	RECT rt0, rt1;
	SetRect(&mOKrt, 0, y, mCrt.right, mCrt.bottom);
	SetRect(&rt0, 0, y, mCrt.right>>1, mCrt.bottom);
	SetRect(&rt1, mCrt.right>>1, y, mCrt.right, mCrt.bottom);
	if(mOnOK)
	{
		if(mOKCancel)
		{
			CPoint pt;
			GetCursorPos(&pt);
			ScreenToClient(&pt);
			mdc.FillSolidRect(pt.x < rt0.right ? &rt0 : &rt1, RGB(30, 144, 255));
		}
		else
		{
			mdc.FillSolidRect(&mOKrt, RGB(30, 144, 255));
		}
	}
	mdc.MoveTo(0, y);
	mdc.LineTo(mCrt.right, y);
	mdc.SetBkMode(TRANSPARENT);
	if(mOKCancel)
	{
		mdc.MoveTo(mCrt.right >> 1, y);
		mdc.LineTo(mCrt.right >> 1, mCrt.bottom);
		mdc.DrawText(L"OK", 2, &rt0, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		mdc.DrawText(L"Cancel", 6, &rt1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		mdc.DrawText(L"OK", 2, &mOKrt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	dc.BitBlt(0, 0, mCrt.right, mCrt.bottom, &mdc, 0, 0, SRCCOPY);
	mdc.SelectObject(bmpo);
	mdc.SelectObject(fonto);
}

void NaraMessageBox::OnLButtonDown(UINT nFlags, CPoint pt)
{
	NaraDialog::OnLButtonDown(nFlags, pt);
	if(PT_IN_RECT(pt, mOKrt))
	{
		if(mOKCancel)
		{
			if(pt.x < mCrt.right >> 1)
			{
				OnOK();
			}
			else
			{
				OnCancel();
			}
		}
		else
		{
			OnOK();
		}
	}
	else
	{
		SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
	}
}

void NaraMessageBox::OnMouseMove(UINT nFlags, CPoint pt)
{
	NaraDialog::OnMouseMove(nFlags, pt);
	BOOL onok = PT_IN_RECT(pt, mOKrt);
	static BOOL onleft = FALSE;
	if(mOnOK != onok)
	{
		Invalidate(FALSE);
		mOnOK = onok;
	}
	else if(mOKCancel)
	{
		BOOL left = (pt.x < (mCrt.right >> 1));
		if(left != onleft)
		{
			Invalidate(FALSE);
			onleft = left;
		}
	}
}

void NaraMessageBox::OnMouseLeave(void)
{
	CDialogEx::OnMouseLeave();
	mOnOK = FALSE;
	Invalidate(FALSE);
}

void NaraMessageBox::OnTimer(UINT_PTR nIDEvent)
{
	CNaraTimerDlg * timer = (CNaraTimerDlg *)GetParent();
	if(mTimerID == -1)
	{
		KillTimer(0);
		KillTimer(1);
		KillTimer(2);
		mTimerIdx = 0;
	}
	else if(mTimerID == 0)
	{
		int i = (mTimerIdx % 6);
		if(i == 0)
		{
			timer->mWatches.GetUnset()->SetMode(TRUE);
		}
		else if(i == 3)
		{
			timer->mWatches.GetUnset()->SetMode(FALSE);
		}
	}
	else if(mTimerID == 1)
	{
		if(mTimerIdx == 1)
		{
			timer->SetTitle(L"T");
		}
		else if(mTimerIdx == 2)
		{
			timer->SetTitle(L"Ti");
		}
		else if(mTimerIdx == 3)
		{
			timer->SetTitle(L"Tit");
		}
		else if(mTimerIdx == 4)
		{
			timer->SetTitle(L"Titl");
		}
		else if(mTimerIdx == 5)
		{
			timer->SetTitle(L"Tit");
		}
		else if(mTimerIdx == 6)
		{
			timer->SetTitle(L"Ti");
		}
		else if(mTimerIdx == 7)
		{
			timer->SetTitle(L"T");
		}
		else if(mTimerIdx == 8)
		{
			timer->SetTitle(L" ");
		}
		else if(mTimerIdx == 9)
		{
			timer->SetTitle(L"5", TRUE);
		}
		else if(mTimerIdx == 10)
		{
			timer->SetTitle(L"5:");
		}
		else if(mTimerIdx == 11)
		{
			timer->SetTitle(L"5:0", TRUE);
		}
		else if(mTimerIdx == 12)
		{
			timer->SetTitle(L"5:00", TRUE);
		}
		else if(mTimerIdx == 14)
		{
			timer->SetTitle(L"5:00", FALSE);
		}
	}
	else if(mTimerID == 2)
	{
		timer->SetTheme(mTimerIdx % NUM_THEMES);
	}
	mTimerIdx++;
}

///////////////////////////////////////////////////////////////////////////////
// main dialog
CNaraTimerDlg::CNaraTimerDlg(CWnd* pParent /*=nullptr*/)
	: NaraDialog(IDD_NARATIMER_DIALOG, pParent)
{
	// set class name
	WNDCLASS c = {};
	::GetClassInfo(AfxGetInstanceHandle(), L"#32770", &c);
	c.lpszClassName = L"NaraTimer_designed-by-naranicca";
	AfxRegisterClass(&c);

	mWatches.Add();

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	mViewMode = VIEW_WATCH;
	mTheme = THEME_DEFAULT;
	mSetting = NULL;
	mLastWatch = NULL;
	mOldDeg = 0;
	mRadius = 0;
	mRadiusHandsHead = 0;
	memset((void*)&mTimestamp, 0, sizeof(mTimestamp));
	memset((void*)mButtonIcon, 0, sizeof(mButtonIcon));
	memset((void*)mButtonIconHover, 0, sizeof(mButtonIconHover));
	mButtonHover = -1;
	mTopmost = FALSE;
	mDegOffset = 0;
	mMuteTick = 0;
	mInstructionIdx = 0;
	mFontScale = 100;
}

void CNaraTimerDlg::Stop(void)
{
	if(mViewMode == VIEW_WATCH)
	{
		mWatches.GetHead()->Stop();
	}
	else
	{
		mWatches.RemoveAll();
		mWatches.Add();
	}
	KillTimer(TID_TICK);
	SetWindowText(L"NaraTimer");
	Invalidate(FALSE);
}

void CNaraTimerDlg::SetTopmost(BOOL topmost)
{
	CMenu* menu = GetSystemMenu(FALSE);
	if (topmost)
	{
		::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		menu->ModifyMenu(IDM_TOPMOST, MF_BYCOMMAND | MF_STRING, IDM_TOPMOST, L"Unpin the window");
		SET_BUTTON(BUTTON_PIN, IDI_PIN, IDI_PIN);
	}
	else
	{
		::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		menu->ModifyMenu(IDM_TOPMOST, MF_BYCOMMAND | MF_STRING, IDM_TOPMOST, L"Pin the window");
		SET_BUTTON(BUTTON_PIN, NULL, IDI_UNPIN);
	}
	mTopmost = topmost;
}

void CNaraTimerDlg::SetTheme(int theme)
{
	mTheme = theme;
	AfxGetApp()->WriteProfileInt(L"Theme", L"CurrentTheme", mTheme);
	Invalidate(FALSE);
}

void CNaraTimerDlg::PlayTickSound(void)
{
	if(mTickSound)
	{
		PlaySound((LPCWSTR)MAKEINTRESOURCE(IDR_WAVE2), GetModuleHandle(NULL), SND_ASYNC | SND_RESOURCE);
	}
}

void CNaraTimerDlg::DoDataExchange(CDataExchange* pDX)
{
	NaraDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNaraTimerDlg, NaraDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_NCPAINT()
	ON_WM_NCACTIVATE()
	ON_WM_NCCALCSIZE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_ACTIVATE()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT, OnTitleChanging)
	ON_MESSAGE(WM_PIN, OnPinToggle)
	ON_COMMAND(IDM_NEW, OnNew)
	ON_COMMAND(IDM_STOP, OnStop)
	ON_COMMAND(IDM_TIMERMODE, OnTimerMode)
	ON_COMMAND(IDM_ALARMMODE, OnAlarmMode)
	ON_COMMAND(IDM_TOPMOST, OnMenuPin)
	ON_COMMAND(IDM_FONT, OnMenuFont)
	ON_COMMAND(ID_HELP, OnMenuHelp)
	ON_COMMAND(IDM_ABOUT, OnMenuAbout)
	ON_COMMAND(IDM_THEMEDEFAULT, OnThemeDefault)
	ON_COMMAND(IDM_THEMEBLACK, OnThemeDark)
	ON_COMMAND(IDM_THEMEBLUE, OnThemeBlue)
	ON_COMMAND(IDM_THEMEGREEN, OnThemeGreen)
	ON_COMMAND(IDM_THEMEORANGE, OnThemeOrange)
	ON_COMMAND(IDM_THEMEMINT, OnThemeMint)
	ON_COMMAND(IDM_THEMEPINK, OnThemePink)
	ON_COMMAND(IDM_TOGGLEDIGITALWATCH, OnToggleDigitalWatch)
	ON_COMMAND(IDM_TOGGLEDATE, OnToggleDate)
	ON_COMMAND(IDM_TOGGLETICKSOUND, OnToggleTickSound)
END_MESSAGE_MAP()

UINT FnThreadTicking(LPVOID param)
{
	CNaraTimerDlg * dlg = (CNaraTimerDlg*)param;
	HWND hwnd = dlg->GetSafeHwnd();
	while(dlg->mRunning)
	{
		int mute = dlg->mMuteTick;
		if(mute > 0)
		{
			dlg->mMuteTick = 0;
			Sleep(mute * 1000);
		}
		else
		{
			dlg->mMuteTick = 0;
			if(::FindWindow(L"NaraTimer_designed-by-naranicca", NULL) == hwnd)
			{
				dlg->PlayTickSound();
			}
			ULONGLONG t = GetTickCount64();
			Sleep(1000 - (t % 1000));
		}
	}
	return 0;
}

BOOL CNaraTimerDlg::OnInitDialog()
{
	NaraDialog::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	SET_WINDOWED_STYLE;

	CString ver = AfxGetApp()->GetProfileStringW(L"Setting", L"Version", L"");
	if(ver.GetLength() == 0)
	{
		mInstructionIdx = 1;
	}
	{
		HRSRC hResInfo;
		DWORD size;
		HGLOBAL hResData;
		LPVOID pRes, pResCopy;
		UINT uLen;
		VS_FIXEDFILEINFO * lpFfi;
		HINSTANCE hInst = AfxGetInstanceHandle();

		hResInfo = FindResource(hInst, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
		size = SizeofResource(hInst, hResInfo);
		hResData = LoadResource(hInst, hResInfo);
		pRes = LockResource(hResData);
		pResCopy = LocalAlloc(LMEM_FIXED, size);
		CopyMemory(pResCopy, pRes, size);
		FreeResource(hResData);

		VerQueryValueW(pResCopy, TEXT("\\"), (LPVOID *)&lpFfi, &uLen);
		LocalFree(pResCopy);

		DWORD ms = lpFfi->dwProductVersionMS;
		DWORD ls = lpFfi->dwProductVersionLS;

		mVersion.Format(L"%d.%d.%d.%d", HIWORD(ms), LOWORD(ms), HIWORD(ls), LOWORD(ls));
		if(mVersion != ver)
		{
			AfxGetApp()->WriteProfileStringW(L"Setting", L"Version", mVersion);
		}
	}
	mFontScale = AfxGetApp()->GetProfileInt(L"Setting", L"FontScale", 100);
	mTheme = AfxGetApp()->GetProfileInt(L"Theme", L"CurrentTheme", THEME_DEFAULT);
	mDigitalWatch = AfxGetApp()->GetProfileInt(L"Theme", L"DigitalWatch", 1);
	mHasDate = AfxGetApp()->GetProfileInt(L"Theme", L"HasDate", 1);
	mTickSound = AfxGetApp()->GetProfileInt(L"Theme", L"TickSound", 0);
	CString font = AfxGetApp()->GetProfileStringW(L"Setting", L"Font", L"");
	int len = font.GetLength();
	if(len > 0 && len < LF_FACESIZE)
	{
		memcpy(mFontFace, font.GetBuffer(), sizeof(WCHAR) * font.GetLength());
	}
	else
	{
		NONCLIENTMETRICS metrics;
		metrics.cbSize = sizeof(NONCLIENTMETRICS);
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
		memcpy(&mFontFace, &metrics.lfMessageFont.lfFaceName, sizeof(metrics.lfMessageFont.lfFaceName));
	}

	int nargs;
	LPWSTR * args = CommandLineToArgvW(GetCommandLine(), &nargs);
	CString str;
	for(int i = 1; i < nargs; i++)
	{
		if(CString(L"--position") == args[i])
		{
			int l = _wtoi(args[i + 1]);
			int t = _wtoi(args[i + 2]);
			int r = _wtoi(args[i + 3]);
			int b = _wtoi(args[i + 4]);
			MoveWindow(l, t, r - l, b - t, FALSE);
			i += 4;
		}
		else if(CString(L"--mode") == args[i])
		{
			mWatches.GetUnset()->SetMode(_wtoi(args[i + 1]));
			i++;
		}
	}
	LocalFree(args);

	reposition();
	mTitleEdit.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER, CRect(0, 0, 10, 10), this, IDC_EDIT);
	mTitleEdit.ShowWindow(SW_HIDE);

	SetTimer(TID_REFRESH, 500, NULL);
	SetTopmost(FALSE);

	SetWindowText(L"NaraTimer");

	mRunning = TRUE;
	mThread = AfxBeginThread(FnThreadTicking, this);

	SetForegroundWindow();

	return TRUE;
}

BOOL CNaraTimerDlg::PreTranslateMessage(MSG* pMsg)
{
	Watch * watch = mWatches.GetHead();
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
			TIMES_UP = -100.f;
			if(!TITLE_CHANGING)
			{
				if(mViewMode == VIEW_WATCH)
				{
					Watch * watch = mWatches.GetHead();
					if(watch->IsTimeSet())
					{
						NaraMessageBox dlg(this, TRUE);
						dlg.AddHeading(watch->IsTimerMode() ? L"Stop the timer?" : L"Stop the alarm?");
						if(dlg.DoModal() == IDOK)
						{
							Stop();
							return TRUE;
						}
					}
				}
				else if(mWatches.GetSize() > 0)
				{
					NaraMessageBox dlg(this, TRUE);
					dlg.AddHeading(L"Stop all?");
					if(dlg.DoModal() == IDOK)
					{
						Stop();
						return TRUE;
					}
					break;
				}
			}
		case VK_RETURN:
			if(TITLE_CHANGING)
			{
				CString title;
				mTitleEdit.ShowWindow(SW_HIDE);
				mTitleEdit.GetWindowText(title);
				SetWindowText(title);
				if(!mSetting && !title.IsEmpty())
				{
					WCHAR * str = title.GetBuffer();
					int len = title.GetLength();
					int time = 0;
					int num = 0;
					BOOL has_colon = FALSE;
					for(int i = 0; i < len; i++)
					{
						if(str[i] >= L'0' && str[i] <= L'9')
						{
							num = (num * 10) + (int)(str[i] - L'0');
						}
						else if(str[i] == L':')
						{
							time = time * 100 + num;
							num = 0;
							if(i == len - 1) time = -1;
							has_colon = TRUE;
						}
						else
						{
							time = -1;
							break;
						}
					}
					if(time >= 0)
					{
						int scale = (has_colon ? 1 : 100);
						time = ((time * 100) + num) * scale;
						CTime c = CTime::GetCurrentTime();
						Watch * watch = mWatches.GetUnset();
						watch->mTitle = title;
						if(watch->IsAlarmMode())
						{
							int s = 0;
							if(time > 9999)
							{
								s = (time % 100);
								time /= 100;
							}
							int m = (time % 100);
							int h = (time / 100);
							if(h < 24 && num < 60)
							{
								int h_cur = c.GetHour();
								if((h == h_cur || abs(h - h_cur) == 12) && m > c.GetMinute())
								{
									h = h_cur;
								}
								else if(h <= h_cur)
								{
									h += ((h + 24 - h_cur <= 12) ? 24 : 12);
								}
								if(watch->SetTime(h, m, s))
								{
									SetTimer(TID_TICK, CHK_INTERVAL, NULL);
									if(mWatches.GetSize() > 1)
									{
										mWatches.Add();
										SetViewMode(VIEW_LIST);
									}
								}
							}
						}
						else
						{
							int h = (time / 10000);
							int m = (time / 100) % 100;
							int s = (time % 100);
							if(watch->SetTime(h, m, s))
							{
								SetTimer(TID_TICK, CHK_INTERVAL, NULL);
							}
						}
					}
					else if(mViewMode == VIEW_WATCH)
					{
						watch->mTitle = title;
					}
				}
				else
				{
					watch->mTitle = title;
				}
				this->SetFocus();
				Invalidate(FALSE);
				TITLE_CHANGING = FALSE;
			}
			else
			{
				mWatches.Add();
				SetViewMode(VIEW_WATCH);
			}
			return TRUE;
		case '0':
			if(CTRL_DOWN)
			{
				mFontScale = 100;
				AfxGetApp()->WriteProfileInt(L"Setting", L"FontScale", mFontScale);
				Invalidate(FALSE);
				return TRUE;
			}
			break;
		case VK_OEM_PLUS:
			if(CTRL_DOWN)
			{
				mFontScale += 10;
				AfxGetApp()->WriteProfileInt(L"Setting", L"FontScale", mFontScale);
				Invalidate(FALSE);
				return TRUE;
			}
			break;
		case VK_OEM_MINUS:
			if(CTRL_DOWN && mFontScale >= 10)
			{
				mFontScale -= 10;
				AfxGetApp()->WriteProfileInt(L"Setting", L"FontScale", mFontScale);
				Invalidate(FALSE);
				return TRUE;
			}
			break;
		case VK_DOWN:
			if(mViewMode == VIEW_WATCH)
			{
				Watch * watch = mWatches.GetHead();
				if(!TITLE_CHANGING && watch->mTimeSet > 60000)
				{
					if(watch->IsAlarmMode())
					{
						if(watch->mHM.cy > 0)
						{
							watch->mHM.cy--;
						}
						else
						{
							watch->mHM.cx = (watch->mHM.cx > 0 ? watch->mHM.cx - 1 : 11);
							watch->mHM.cy = 59;
						}
						watch->SetTime(watch->mHM.cx, watch->mHM.cy, 0);
					}
					else
					{
						watch->mTimeSet -= 60000;
					}
					return TRUE;
				}
			}
			break;
		case VK_UP:
			if(mViewMode == VIEW_WATCH)
			{
				Watch * watch = mWatches.GetHead();
				if(!TITLE_CHANGING && watch->IsTimeSet())
				{
					if(watch->IsAlarmMode())
					{
						watch->mHM.cy++;
						watch->mHM.cx += (watch->mHM.cx / 60);
						watch->mHM.cy = (watch->mHM.cy % 60);
						watch->SetTime(watch->mHM.cx, watch->mHM.cy, 0);
					}
					else
					{
						watch->mTimeSet += 60000;
					}
					return TRUE;
				}
			}
			break;
		case VK_TAB:
			if(!TITLE_CHANGING)
			{
				if(CTRL_DOWN)
				{
					SetViewMode(mViewMode == VIEW_WATCH ? VIEW_LIST : VIEW_WATCH);
				}
				else
				{
					if(mWatches.GetHead()->IsTimerMode())
					{
						OnAlarmMode();
					}
					else
					{
						OnTimerMode();
					}
				}
			}
			return TRUE;
		case VK_F2:
			if(!TITLE_CHANGING)
			{
				SetTitle();
			}
			break;
		case VK_F12:
			SetTopmost(!mTopmost);
			break;
		}
	case WM_POINTERDOWN:
		if (IS_POINTER_SECONDBUTTON_WPARAM(pMsg->wParam))
		{
			PostMessage(WM_CONTEXTMENU, 0, pMsg->lParam);
		}
		break;
	case WM_CLOSE:
		mRunning = FALSE;
		WaitForSingleObject(mThread->m_hThread, INFINITE);
		break;
	}
	return NaraDialog::PreTranslateMessage(pMsg);
}

void CNaraTimerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		mInstructionIdx = 999;
	}
	else if (nID == IDM_TOPMOST)
	{
		SetTopmost(!mTopmost);
	}
	else
	{
		NaraDialog::OnSysCommand(nID, lParam);
	}
}

POINT CNaraTimerDlg::deg2pt(float deg, int r)
{
	POINT pt;
	double rad = (deg + mDegOffset) * 3.141592 / 180;
	pt.x = (LONG)(-r * sin(rad) + 0.5f);
	pt.y = (LONG)(-r * cos(rad) + 0.5f);
	return pt;
}

float CNaraTimerDlg::pt2deg(CPoint pt)
{
	int cx = (mTimerRect.right + mTimerRect.left) >> 1;
	int cy = (mTimerRect.bottom + mTimerRect.top) >> 1;
	int dx = (pt.x - cx);
	int dy = (pt.y - cy);
	float rad = (float)atan((float)dx / dy);
	if (dy >= 0) rad += 3.141592f;
	if (rad < 0) rad += 2 * 3.141592f;
	return rad * 180 / 3.141592f - mDegOffset;
}

Watch * CNaraTimerDlg::SettingTime(float deg, BOOL stick)
{
	mOldDeg = deg;
	Watch * watch = (mSetting == NULL ? mWatches.GetUnset() : mSetting);
	if(watch->IsTimerMode())
	{
		ULONGLONG t = (ULONGLONG)(deg * watch->mTime360 / 360);
		if (stick)
		{
			t = ((t + 30000) / 60000) * 60000;
		}
		watch->SetText(L"%d:%02d", (t / 60000), ((t % 60000) + 500) / 1000);
		watch->SetTime(0, (t / 60000), ((t % 60000) + 500) / 1000);
	}
	else
	{
		ULONGLONG t = (ULONGLONG)(deg * watch->mTime360 / 360);
		t = ((t + 30000) / 60000) * 60000;
		if (stick)
		{
			t = ((t + 300000) / 600000) * 600000;
		}
		CTime c = CTime::GetCurrentTime();
		t /= 1000;
		int h = c.GetHour() + (int)(t / 3600);
		int m = (int)((t % 3600) / 60);
		watch->SetTime(h, m, 0);
		h = (h < 24 ? h : h - 24);
		watch->SetText(L"%d:%02d %s", (h > 12 ? h - 12 : h), m, h < 12? L"am": L"pm");
	}
	return watch;
}

ULONGLONG CNaraTimerDlg::GetTimestamp(void)
{
	LONGLONG q = mTimestamp.QuadPart;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&mTimestamp);

	LONGLONG d = mTimestamp.QuadPart - q;
	return (LONGLONG)(d * 1000.f / freq.QuadPart);
}

static void DrawRoundRect(Gdiplus::Graphics * g, Pen * p, Gdiplus::Rect & rt, int r)
{
	GraphicsPath path;
	path.AddLine(rt.X + r, rt.Y, rt.X + rt.Width - (r * 2), rt.Y);
	path.AddArc(rt.X + rt.Width - (r * 2), rt.Y, r * 2, r * 2, 270, 90);
	path.AddLine(rt.X + rt.Width, rt.Y + r, rt.X + rt.Width, rt.Y + rt.Height - (r * 2));
	path.AddArc(rt.X + rt.Width - (r * 2), rt.Y + rt.Height - (r * 2), r * 2, r * 2, 0, 90);
	path.AddLine(rt.X + rt.Width - (r * 2), rt.Y + rt.Height, rt.X + r, rt.Y + rt.Height);
	path.AddArc(rt.X, rt.Y + rt.Height - (r * 2), r * 2, r * 2, 90, 90);
	path.AddLine(rt.X, rt.Y + rt.Height - (r * 2), rt.X, rt.Y + r);
	path.AddArc(rt.X, rt.Y, r * 2, r * 2, 180, 90);
	path.CloseFigure();
	g->DrawPath(p, &path);
}

static void FillRoundRect(Gdiplus::Graphics * g, Brush * br, Gdiplus::Rect & rt, int r)
{
	GraphicsPath path;
	path.AddLine(rt.X + r, rt.Y, rt.X + rt.Width - (r * 2), rt.Y);
	path.AddArc(rt.X + rt.Width - (r * 2), rt.Y, r * 2, r * 2, 270, 90);
	path.AddLine(rt.X + rt.Width, rt.Y + r, rt.X + rt.Width, rt.Y + rt.Height - (r * 2));
	path.AddArc(rt.X + rt.Width - (r * 2), rt.Y + rt.Height - (r * 2), r * 2, r * 2, 0, 90);
	path.AddLine(rt.X + rt.Width - (r * 2), rt.Y + rt.Height, rt.X + r, rt.Y + rt.Height);
	path.AddArc(rt.X, rt.Y + rt.Height - (r * 2), r * 2, r * 2, 90, 90);
	path.AddLine(rt.X, rt.Y + rt.Height - (r * 2), rt.X, rt.Y + r);
	path.AddArc(rt.X, rt.Y, r * 2, r * 2, 180, 90);
	path.CloseFigure();
	g->FillPath(br, &path);
}

void CNaraTimerDlg::DrawTimer(CDC * dc, Watch * watch, RECT * dst, BOOL list_mode)
{
	CFont font, * fonto;
	int font_size, tw, th, tsize;
	int x, y, w, h, r;
	RECT trt;

	Graphics g(*dc);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetTextRenderingHint(TextRenderingHintAntiAlias);
	POINT pt0, pt1;
	int clock;
	int start = 0;
	COLORREF pie_color, hand_color, handshead_color, timestr_color;
	float hand_size;
	float handshead_size;

	BK_COLOR = WHITE;
	GRID_COLOR = RGB(0, 0, 0);
	pie_color = RED;
	hand_color = RGB(220, 220, 220);
	handshead_color = RGB(77, 88, 94);
	timestr_color = RGB(20, 20, 20);
	BORDER_COLOR = RED;
	switch(mTheme)
	{
	case THEME_DARK:
		BK_COLOR = RGB(8, 9, 10);
		GRID_COLOR = WHITE;
		pie_color = RED;
		hand_color = WHITE;
		handshead_color = RGB(50, 50, 50);
		timestr_color = RGB(220, 220, 220);
		BORDER_COLOR = RGB(22, 23, 24);
		break;
	case THEME_BLUE:
		BORDER_COLOR = RGB(89, 161, 245);
		break;
	case THEME_GREEN:
		BORDER_COLOR = RGB(30, 108, 78);
		break;
	case THEME_ORANGE:
		BORDER_COLOR = RGB(229, 119, 33);
		break;
	case THEME_MINT:
		BORDER_COLOR = RGB(64, 224, 208);
		break;
	case THEME_PINK:
		BORDER_COLOR = RGB(251, 162, 139);
		break;
	}
	if(watch->IsTimerMode())
	{
		hand_size = 0.27f;
		handshead_size = 0.14f;
		hand_color = handshead_color;
	}
	else
	{
		hand_size = 1.f;
		handshead_size = 0.1f;
	}

	RECT tmp_rect, * rt = &tmp_rect;
	CopyRect(rt, dst);
	dc->FillSolidRect(rt, BK_COLOR);
	dc->SetBkMode(TRANSPARENT);
	GetClientRect(&mTimerRect);
	w = (rt->right - rt->left);
	h = (rt->bottom - rt->top);

	// font
	font_size = max(ROUND(min(w, h) * mFontScale * 0.0008), 1);
	GetFont(font, font_size);
	fonto = (CFont*)dc->SelectObject(&font);
	dc->DrawText(L"88", 3, &trt, DT_CALCRECT | DT_SINGLELINE);
	tw = (trt.right - trt.left + 10);
	th = (trt.bottom - trt.top);
	tsize = ROUND(max(tw, th) * 1.1f);
	if(list_mode)
	{
		mGridSize = 0;
		r = (MIN(w, h) >> 1) - LIST_GAP;
		x = rt->left + LIST_GAP;
		y = (rt->top + rt->bottom - (r << 1)) >> 1;
		tsize = 0;
	}
	else
	{
		mGridSize = ROUND(min(w, h) * 0.023f);
		r = (MIN(w, h) >> 1) - (mGridSize + (mGridSize >> 1) + tsize);
		x = (rt->left + rt->right - (r << 1)) >> 1;
		y = (rt->top + rt->bottom - (r << 1)) >> 1;
	}
	mRadius = r;

	if(watch->IsTimerMode())
	{
		mDegOffset = 0;
	}
	else
	{
		int m = CTime::GetCurrentTime().GetMinute();
		int s = CTime::GetCurrentTime().GetSecond();
		mDegOffset = -360000.f * (m * 60 + s) / watch->mTime360;
	}
	// draw numbers
	if(mFontScale > 0 && !list_mode)
	{
		dc->SetTextColor(GRID_COLOR);
		clock = CTime::GetCurrentTime().GetHour();
		for(int i = 0; i < 360; i += 30)
		{
			pt0 = deg2pt((float)i, r + mGridSize + (tsize >> 1));
			RECT rt = { x + r + pt0.x - (tw >> 1), y + r + pt0.y - (th >> 1), x + r + pt0.x + (tw >> 1), y + r + pt0.y + (th >> 1) };
			CString str;
			if(watch->IsTimerMode())
			{
				str.Format(L"%d", (int)(i / 30) * 5);
			}
			else
			{
				int t = (int)(i / 30) + clock;
				if(t >= 24) t -= 24;
				if(i == 0)
				{
					rt.left -= tw;
					rt.right += tw;
					if(t < 12)
					{
						str.Format(L"%d am", (t == 0 ? 12 : (t > 12 ? t - 12 : t)));
					}
					else
					{
						str.Format(L"%d pm", (t == 0 ? 12 : (t > 12 ? t - 12 : t)));
					}
				}
				else
				{
					str.Format(L"%d", (t == 0 ? 12 : (t > 12 ? t - 12 : t)));
				}
			}
			dc->DrawText(str, &rt, DT_CENTER | DT_VCENTER);
		}
	}

	// draw grids
	int head_size = ROUND(r * handshead_size);
	if(list_mode)
	{
		int th = ROUND(r / 10);
		DEFINE_PEN(penm, GRID_COLOR, 64, th);
		g.DrawEllipse(&penm, x + th/2, y + th/2, r * 2 - th, r * 2 - th);
	}
	else
	{
		DEFINE_PEN(penm, GRID_COLOR, 255, r / 100);
		clock = (watch->IsTimerMode() ? 6 : 5);
		for(int i = 0; i < 360; i += clock)
		{
			pt0 = deg2pt((float)i, r - (mGridSize >> 1));
			pt1 = deg2pt((float)i, r + (mGridSize >> 1));
			g.DrawLine(&penm, x + r + pt0.x, y + r + pt0.y, x + r + pt1.x, y + r + pt1.y);
		}
		DEFINE_PEN(penh, GRID_COLOR, 255, r / 33);
		for(int i = 0; i < 360; i += 30)
		{
			pt0 = deg2pt((float)i, r - mGridSize);
			pt1 = deg2pt((float)i, r + mGridSize);
			g.DrawLine(&penh, x + r + pt0.x, y + r + pt0.y, x + r + pt1.x, y + r + pt1.y);
		}

		// hands head shadow
		int off = ROUND(head_size * 0.15);
		mRadiusHandsHead = head_size;
		SolidBrush brgrey2(Color(64, 0, 0, 0));
		g.FillEllipse(&brgrey2, x + r + off - head_size, y + r + off - head_size, 2 * head_size, 2 * head_size);
		mButtonRect[BUTTON_CENTER].SetRect(x + r + off - head_size, y + r + off - head_size, x + r + off + head_size, y + r + off + head_size);
	}

	// draw red pie
	LONGLONG t_remain = watch->GetRemainingTime();
	float deg = 0;
	if(watch->IsTimerMode() || !LBUTTON_DOWN)
	{
		deg = 360.f * t_remain / watch->mTime360;
		deg -= mDegOffset;
	}
	else if (t_remain > 0)
	{
		CTime c = CTime::GetCurrentTime();
		int o = c.GetMinute() * 60 + c.GetSecond();
		deg = 360.f * (t_remain + o * 1000) / MAX_TIME360;
	}
	DrawPie(&g, watch, x, y, r, deg, pie_color);
	CString str = L"";
	if (deg > -mDegOffset)
	{
		ULONGLONG ts = (t_remain + 500) / 1000;
		if (ts < 3600)
		{
			str.Format(L"-%d:%02d", (int)(ts / 60), (int)(ts % 60));
		}
		else
		{
			int m = (int)(ts / 60);
			str.Format(L"-%d:%02d:%02d", (int)(m / 60), (int)(m % 60), (int)(ts % 60));
		}
		if (ts != mTso && !mSetting)
		{
			if (watch->mTitle.IsEmpty())
			{
				SetWindowText(str);
			}
			else
			{
				SetWindowText(watch->mTitle + L" " + str);
			}
			mTso = ts;
		}

		if(watch->mHM.cy > 0 && mFontScale > 0 && !list_mode)
		{
			CFont font;
			GetFont(font, font_size, TRUE);
			CFont* fonto = dc->SelectObject(&font);
			POINT pt = deg2pt(deg, r + mGridSize + (th >> 1));
			RECT rt0 = { x + r + pt.x - (tw >> 1),y + r + pt.y - (th >> 1),x + r + pt.x + (tw >> 1),y + r + pt.y + (th >> 1) };
			RECT rt1 = { rt0.left - 1, rt0.top, rt0.right - 1, rt0.bottom };
			RECT rt2 = { rt0.left + 1, rt0.top, rt0.right + 1, rt0.bottom };
			RECT rt3 = { rt0.left, rt0.top-1, rt0.right, rt0.bottom-1 };
			RECT rt4 = { rt0.left, rt0.top+1, rt0.right, rt0.bottom+1 };
			CString min;
			min.Format(L"%02d", watch->mHM.cy);
			dc->SetTextColor(BK_COLOR);
			dc->DrawText(min, &rt1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			dc->DrawText(min, &rt2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			dc->DrawText(min, &rt3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			dc->DrawText(min, &rt4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			dc->SetTextColor(pie_color);
			dc->DrawText(min, &rt0, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			dc->SelectObject(fonto);
		}
	}

	// draw digital watch
	if(mDigitalWatch || list_mode)
	{
		Gdiplus::Font font(mFontFace, (list_mode? (r << 1) : ROUND(r / 4)), FontStyleBold, UnitPixel);
		Gdiplus::StringFormat sf;
		int opaque = 255;
		if(mSetting)
		{
			str = watch->mTimeStr;
		}
		else if(t_remain == 0)
		{
			if(list_mode)
			{
				str = L"TIME'S UP";
			}
			else
			{
				CTime t = CTime::GetCurrentTime();
				int h = t.GetHour();
				h = (h > 12 ? h - 12 : h);
				str.Format(L"%d:%02d:%02d", (h == 0 ? 12 : h), t.GetMinute(), t.GetSecond());
				SetWindowText(str);
			}
		}
		if(list_mode)
		{
			SetRect(&mTimeRect, x + r + r + LIST_GAP, rt->top, rt->right, rt->bottom);
			sf.SetAlignment(StringAlignmentNear);
			sf.SetLineAlignment(StringAlignmentCenter);
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(&pt);
			opaque = (PT_IN_RECT(pt, *rt) ? 255 : 170);
		}
		else
		{
			SetRect(&mTimeRect, x, y + r + head_size, x + r + r, y + r + r - r / 3);
			sf.SetAlignment(StringAlignmentCenter);
			sf.SetLineAlignment(StringAlignmentCenter);
		}
		sf.SetTrimming(StringTrimmingNone);
		sf.SetFormatFlags(StringFormatFlagsNoWrap);
		SolidBrush br(Color(opaque, GetRValue(timestr_color), GetGValue(timestr_color), GetBValue(timestr_color)));
		RectF rtf(mTimeRect.left, mTimeRect.top, mTimeRect.right - mTimeRect.left, mTimeRect.bottom - mTimeRect.top);
		g.DrawString(str.GetBuffer(), -1, &font, rtf, &sf, &br);
	}

	// draw date complications
	if(mHasDate && !list_mode)
	{
		CFont font;
		GetFont(font, r / 5, TRUE);
		CFont* fonto = dc->SelectObject(&font);
		RECT trt = { 0, };
		CTime t = CTime::GetCurrentTime();
		CString d;
		d.Format(L"%d", t.GetDay());
		dc->DrawText(L"88", 2, &trt, DT_SINGLELINE | DT_CALCRECT);
		int w = ROUND((trt.right - trt.left) * 1.05f);
		int h = trt.bottom - trt.top;
		trt.left = (x + r - (w >> 1));
		trt.top = (y + r + r - ROUND(r / 2.5f));
		trt.right = trt.left + w ;
		trt.bottom = trt.top + h;
		CBrush br(BK_COLOR);
		CBrush* bro = dc->SelectObject(&br);
		COLORREF cl = blend_color(blend_color(blend_color(GRID_COLOR, BK_COLOR), BK_COLOR), BK_COLOR);
		CPen pen(PS_SOLID, 1, cl);
		CPen* peno = dc->SelectObject(&pen);
		dc->Rectangle(&trt);
		dc->FillSolidRect(trt.left, trt.bottom, w, ROUND(h * 0.1f) , pie_color);
		dc->SetTextColor(timestr_color);
		dc->DrawText(d, &trt, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		dc->SelectObject(fonto);
		dc->SelectObject(bro);
		dc->SelectObject(peno);
	}

	// Hands
	if(!list_mode)
	{
		if(deg < -mDegOffset) deg = -mDegOffset;
		if(hand_color == handshead_color)
		{
			float pt = (r * 0.08f);
			DEFINE_PEN(pencl, handshead_color, 255, pt);
			pencl.SetStartCap(LineCapRound);
			pencl.SetEndCap(LineCapRound);
			pt0 = deg2pt(deg, ROUND(r * hand_size - pt / 2 + 1));
			g.DrawLine(&pencl, x + r, y + r, x + r + pt0.x, y + r + pt0.y);
		}
		else
		{
			float pt = (r * 0.1f);
			DEFINE_PEN(pencl, handshead_color, 255, pt);
			pencl.SetEndCap(LineCapTriangle);
			pt1 = deg2pt(deg, ROUND(r * hand_size - pt / 2 - 1));
			g.DrawLine(&pencl, x + r, y + r, x + r + pt1.x, y + r + pt1.y);

			DEFINE_PEN(pen, hand_color, 255, pt * 0.5f);
			pen.SetStartCap(LineCapSquare);
			pen.SetEndCap(LineCapTriangle);
			pt0 = deg2pt(deg, head_size + head_size / 2);
			pt1 = deg2pt(deg, ROUND(r * hand_size - pt / 2 - 1));
			g.DrawLine(&pen, x + r + pt0.x, y + r + pt0.y, x + r + pt1.x, y + r + pt1.y);
		}
		// Hands head
		SolidBrush brgrey(Color(255, GetRValue(handshead_color), GetGValue(handshead_color), GetBValue(handshead_color)));
		g.FillEllipse(&brgrey, x + r - head_size, y + r - head_size, 2 * head_size, 2 * head_size);
		if(hand_color != handshead_color)
		{
			SolidBrush br(Color(255, GetRValue(hand_color), GetGValue(hand_color), GetBValue(hand_color)));
			int s = ROUND(head_size * 0.3f);
			g.FillEllipse(&br, x + r - s, y + r - s, 2 * s, 2 * s);
		}
	}

	/* draw title */
	if(mViewMode == VIEW_WATCH)
	{
		if(TITLE_CHANGING)
		{
			int cx = ((rt->left + rt->right) >> 1);
			int cy = ((rt->top + rt->bottom) >> 1) - (mRadius >> 1);
			int w = rt->right - rt->left - 40;
			int h = HUD_FONT_SIZE;
			int x = cx - (w >> 1);
			int y = cy - (h >> 1);
#if 0
			SolidBrush br(Color(255, 128, 128, 128));
			FillRoundRect(&g, &br, Rect(x - 10, y - 10, w + 20, h + 20), 10);
#endif
			mTitleEdit.MoveWindow(x, y, w, h, FALSE);
		}
		else if(watch->mTitle.IsEmpty() == FALSE)
		{
			DrawHUD(dc, watch->mTitle);
		}
	}

	// TIME'S UP message
	if(TIMES_UP >= 0 && t_remain <= 0 && !list_mode)
	{
		CFont font;
		GetFont(font, rt->bottom - rt->top, TRUE);
		fonto = dc->SelectObject(&font);
		RECT trt = { 0, };
		dc->DrawText(L"TIME'S UP!", &trt, DT_SINGLELINE | DT_CALCRECT);
		int w = trt.right - trt.left;
		float t = GetTickCount64() - TIMES_UP;
		trt.left = t * (rt->left - w - rt->right) / 5000 + rt->right;
		trt.top = rt->top;
		trt.right = trt.left + w;
		trt.bottom = rt->bottom;
		dc->SetTextColor(timestr_color);
		dc->DrawText(L"TIME'S UP!", &trt, DT_SINGLELINE | DT_VCENTER);
		dc->SelectObject(fonto);
		if(trt.right < 0)
		{
			TIMES_UP = GetTickCount64();
		}
	}

	static float dash_offset = 0;
	if(mInstructionIdx < 0)
	{
		Pen pen(Color(255, 255, 0, 0), 6);
		pen.SetDashOffset(dash_offset);
		dash_offset += 0.1;
		if(mInstructionIdx == -2)
		{
			pen.SetDashStyle(DashStyleDot);
			g.DrawEllipse(&pen, x + r - mRadiusHandsHead - 5, y + r - mRadiusHandsHead - 5, mRadiusHandsHead*2+10, mRadiusHandsHead*2+10);
			DrawHUD(dc, watch->IsTimerMode() ? L"Timer" : L"Alarm");
		}
		else if(mInstructionIdx == -3)
		{
			pen.SetDashStyle(DashStyleDash);
			g.DrawEllipse(&pen, x - mGridSize - 5, y - mGridSize - 5, (r + mGridSize) * 2 + 10, (r + mGridSize) * 2 + 10);
		}
		else if(mInstructionIdx == -4)
		{
		}
		else if(mInstructionIdx == -5)
		{
			pen.SetDashStyle(DashStyleDot);
			int x = mButtonRect[BUTTON_PIN].left;
			int y = mButtonRect[BUTTON_PIN].top;
			int w = mButtonRect[BUTTON_PIN].Width();
			int h = mButtonRect[BUTTON_PIN].Height();
			g.DrawEllipse(&pen, x - 5, y - 5, w + 10, h + 10);
		}
	}
}

void CNaraTimerDlg::DrawList(CDC * dc, RECT * rt)
{
	Watch * watch = mWatches.GetWatchSet();
	int num_watches = mWatches.GetSize();
	int w_crt = rt->right - rt->left;
	int h_crt = rt->bottom - rt->top;
	float h_watch = (float)h_crt / 3;

	/* estimating font size */
	int font_size = h_watch - LIST_GAP;
	CFont font;
	GetFont(font, font_size, TRUE);
	CFont * fonto = dc->SelectObject(&font);
	RECT trt = { 0, };
	dc->DrawText(L"-88:88:88", &trt, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
	int w = font_size + LIST_GAP*3 + (trt.right - trt.left);
	if(w >= w_crt)
	{
		font_size = font_size * w_crt / w;
		h_watch = font_size + LIST_GAP;
	}
	mWatches.mItemHeight = h_watch;
	dc->SelectObject(fonto);

	if(num_watches > 0)
	{
		CFont font_small;
		GetFont(font_small, ROUND(h_watch * 0.3f), FALSE);
		int top = rt->top + mRoundCorner/2;
		dc->FillSolidRect(rt->left, rt->top, w_crt, top - rt->top, BK_COLOR);
		RECT wrt;
		wrt.left = rt->left;
		wrt.right = rt->right;
		for(int i = 0; i < num_watches; i++)
		{
			wrt.top = top + ROUND(h_watch * i);
			wrt.bottom = top + ROUND(h_watch * (i + 1));
			DrawTimer(dc, watch, &wrt, TRUE);
#if 1
			if(watch->IsAlarmMode())
			{
				CFont * fonto = dc->SelectObject(&font_small);
				CString str;
				int h = (watch->mHM.cx < 24 ? watch->mHM.cx : watch->mHM.cx - 24);
				str.Format(L"%d:%02d", h, watch->mHM.cy);
				dc->SetTextColor(GRID_COLOR);
				dc->DrawText(str, CRect(wrt.left, wrt.top, wrt.left + h_watch, wrt.bottom), DT_SINGLELINE | DT_VCENTER | DT_CENTER);
				dc->SelectObject(fonto);
			}
#endif
			watch = watch->mNext;
		}
		if(wrt.bottom < rt->bottom)
		{
			dc->FillSolidRect(rt->left, wrt.bottom, rt->right, rt->bottom, BK_COLOR);
		}
	}
	else
	{
		CFont font;
		GetFont(font, font_size, TRUE);
		fonto = dc->SelectObject(&font);
		dc->FillSolidRect(rt, BK_COLOR);
		dc->SetTextColor(GRID_COLOR);
		dc->DrawText(L"Empty", rt, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
		dc->SelectObject(fonto);
	}
}

void CNaraTimerDlg::DrawBorder(CDC * dc)
{
	WINDOWPLACEMENT pl;
	GetWindowPlacement(&pl);
	BOOL maximized = (pl.showCmd == SW_MAXIMIZE);

	Graphics g(*dc);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);
	g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	int corner = (maximized ? 0 : mRoundCorner);
	// border
	{
		Pen pen(Color(255, GetRValue(BORDER_COLOR), GetGValue(BORDER_COLOR), GetBValue(BORDER_COLOR)), mResizeMargin * 2);
		DrawRoundRect(&g, &pen, Rect(mCrt.left, mCrt.top, mCrt.right - mCrt.left, mCrt.bottom - mCrt.top), corner);
	}

	// draw icon
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		int id = (i == mButtonHover ? mButtonIconHover[i] : mButtonIcon[i]);
		if (id)
		{
			RECT* brt = &mButtonRect[i];
			HICON icon = static_cast<HICON>(::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(id), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR));
			DrawIconEx(dc->m_hDC, mCrt.left + brt->left, mCrt.top + brt->top, icon,
				(brt->right - brt->left), (brt->bottom - brt->top), 0, NULL, DI_NORMAL);
			DestroyIcon(icon);
		}
	}
}

void CNaraTimerDlg::DrawHUD(CDC * dc, CString str)
{
	Graphics g(*dc);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetTextRenderingHint(TextRenderingHintAntiAlias);

	CFont font;
	GetFont(font, HUD_FONT_SIZE, TRUE);
	CFont * fonto = dc->SelectObject(&font);
	RECT trt = { 0, };
	dc->DrawText(str, &trt, DT_SINGLELINE | DT_CALCRECT);
	int w = trt.right - trt.left;
	int h = trt.bottom - trt.top;
	int max_w = (mCrt.right - mCrt.left) - (mResizeMargin << 1) - 30;
	UINT fmt = DT_SINGLELINE;
	if(w > max_w)
	{
		w = max_w;
		fmt = DT_SINGLELINE | DT_END_ELLIPSIS;
	}
	trt.left = ((mCrt.left + mCrt.right) >> 1) - (w >> 1);
	trt.right = trt.left + w;
	trt.bottom = ((mCrt.top + mCrt.bottom) >> 1) - (mRadius >> 1) + (h >> 1);
	trt.top = trt.bottom - h;
	SolidBrush br(Color(220, 128, 128, 128));
	FillRoundRect(&g, &br, Rect(trt.left - 10, trt.top - 10, w + 20, h + 20), 10);
	dc->SetTextColor(RGB(255, 255, 255));
	dc->DrawText(str, &trt, fmt);
	dc->SelectObject(fonto);
}

void CNaraTimerDlg::DrawPie(Graphics * g, Watch * watch, int x, int y, int r, float deg, COLORREF c)
{
	if(c == -1) c = RED;
	SolidBrush brred(Color(255, GetRValue(c), GetGValue(c), GetBValue(c)));
	if (deg > 0)
	{
		POINT t = deg2pt(deg, r);
		if (t.x == 0 && deg > 200)
		{
			g->FillEllipse(&brred, Rect(x, y, r << 1, r << 1));
		}
		else
		{
			if (t.x != 0 || deg > 90)
			{
				g->FillPie(&brred, Rect(x, y, (r << 1), r << 1), -90, -deg - mDegOffset);
			}
		}
	}
	Pen pg(watch->IsTimerMode()? Color(255, 128, 128, 128): Color(255, GetRValue(c), GetGValue(c), GetBValue(c)), 1);
	g->DrawLine(&pg, x + r, y + 1, x + r, y + r);
}

BOOL CNaraTimerDlg::OnEraseBkgnd(CDC* pDC)
{
	return 0;
}

static void bmp_init(CBitmap * bmp, CDC * dc, int w, int h)
{
	BITMAP bm;
	if (bmp->m_hObject && bmp->GetBitmap(&bm) && (bm.bmWidth != w || bm.bmHeight != h))
	{
		bmp->DeleteObject();
	}
	if (bmp->m_hObject == NULL)
	{
		bmp->CreateCompatibleBitmap(dc, w, h);
	}
	dc->SetStretchBltMode(HALFTONE);
}

void CNaraTimerDlg::Draw(RECT * rt)
{
	CClientDC dc(this);
	CDC mdc;
	mdc.CreateCompatibleDC(&dc);
	CBitmap * bmpo;
	RECT trt;

	int w_crt = rt->right - rt->left;
	int h_crt = rt->bottom - rt->top;

	bmp_init(&mBmp, &dc, w_crt, h_crt);
	bmpo = mdc.SelectObject(&mBmp);

	Watch * watch = mWatches.GetHead();
	trt.left = rt->left + mResizeMargin;
	trt.top = rt->top + mResizeMargin;
	trt.right = rt->right - mResizeMargin;
	trt.bottom = rt->bottom - mResizeMargin;
	if(mViewMode == VIEW_WATCH)
	{
		DrawTimer(&mdc, watch, &trt);
	}
	else
	{
		DrawList(&mdc, &trt);
	}
	DrawBorder(&mdc);
	dc.BitBlt(0, 0, w_crt, h_crt, &mdc, 0, 0, SRCCOPY);

	mdc.SelectObject(bmpo);
}

void CNaraTimerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		NaraDialog::OnPaint();

		CClientDC dc(this);
		CDC mdc;
		mdc.CreateCompatibleDC(&dc);
		CBitmap * bmpo;
		RECT trt;

		int w_crt = mCrt.right - mCrt.left;
		int h_crt = mCrt.bottom - mCrt.top;

		bmp_init(&mBmp, &dc, w_crt, h_crt);
		bmpo = mdc.SelectObject(&mBmp);

		Watch * watch = mWatches.GetHead();
		trt.left = mCrt.left + mResizeMargin;
		trt.top = mCrt.top + mResizeMargin;
		trt.right = mCrt.right - mResizeMargin;
		trt.bottom = mCrt.bottom - mResizeMargin;
		if(mViewMode == VIEW_WATCH)
		{
			DrawTimer(&mdc, watch, &trt);
			mLastWatch = watch;
		}
		else
		{
			DrawList(&mdc, &trt);
		}
		DrawBorder(&mdc);
		if(TITLE_CHANGING)
		{
			RECT rt;
			mTitleEdit.GetWindowRect(&rt);
			ScreenToClient(&rt);
			Graphics g(mdc);
			SolidBrush br(Color(200, 128, 128, 128));
			FillRoundRect(&g, &br, Rect(rt.left - 10, rt.top - 10, rt.right - rt.left + 20, rt.bottom - rt.top + 20), 10);
		}
		dc.BitBlt(0, 0, w_crt, h_crt, &mdc, 0, 0, SRCCOPY);

		mdc.SelectObject(bmpo);
	}
}

void CNaraTimerDlg::OnNcPaint()
{
	Invalidate(FALSE);
}

BOOL CNaraTimerDlg::OnNcActivate(BOOL bActivate)
{
	OnNcPaint();
	return TRUE;
}

void CNaraTimerDlg::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* params)
{
	if (bCalcValidRects)
	{
		return;
	}
	NaraDialog::OnNcCalcSize(bCalcValidRects, params);
}

void CNaraTimerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(mInstructionIdx > 0)
	{
		mInstructionIdx = -mInstructionIdx;
		{
			SetTimer(TID_HELP, 33, NULL);
			NaraMessageBox dlg(this);

			if(mInstructionIdx == -1 || mInstructionIdx <= -999)
			{
				dlg.AddIcon(IDR_MAINFRAME);
				dlg.AddHeading(L"NaraTimer");
				dlg.AddString(L"version " + mVersion);
				dlg.AddString(L"designed by naranicca");
				dlg.DoModal();
				mInstructionIdx--;
			}
			if(mInstructionIdx == -2)
			{
				dlg.Clear();
				dlg.AddHeading(L"Two Modes");
				dlg.AddString(L"NaraTimer provides two mdoes:");
				dlg.AddBoldString(L"Timer and Alarm");
				dlg.AddString(L"You can change the mode by clicking here");
				dlg.SetY(((mTimerRect.bottom + mTimerRect.top) >> 1) + mRadiusHandsHead + 10);
				dlg.EnableTimer(0);
				dlg.DoModal();
				dlg.DisableTimer();
				mInstructionIdx--;

				dlg.Clear();
				dlg.AddHeading(L"How to Set Time");
				dlg.AddString(L"You can set time by clicking and drggaging here");
				dlg.AddString(L"While the cursor is in the dial, time and grids are lined up");
				dlg.AddString(L"To set time more precisely, move the cursor outside of the dial");
				dlg.AddString(L"Or use Up/Down keys to increase/decrease by 1 minute");
				dlg.SetY(((mTimerRect.bottom + mTimerRect.top) >> 1) + mRadius + 5);
				dlg.DoModal();
				mInstructionIdx--;

				dlg.Clear();
				dlg.AddHeading(L"Set Title");
				dlg.AddBoldString(L"Just start typing to set the title!");
				dlg.AddString(L"and,");
				dlg.AddBoldString(L"You can set time with title");
				dlg.AddString(L"Setting title to \'5:00\' will set 5-minute timer (timer mode)");
				dlg.AddString(L"or set alarm for 5 (alaram mode)");
				dlg.SetY(((mTimerRect.bottom + mTimerRect.top) >> 1) + mRadius + 5);
				dlg.EnableTimer(1);
				dlg.DoModal();
				dlg.DisableTimer();
				mInstructionIdx--;
				Stop();
				SetTitle(L"");

				dlg.Clear();
				dlg.AddHeading(L"Use Pin");
				dlg.AddString(L"If you want your timer always visible,");
				dlg.AddString(L"Pin is what you're looking for");
				dlg.AddString(L"Move your cursor here, a pin will show up");
				dlg.AddBoldString(L"Keep your timer always on top");
				dlg.SetY(mButtonRect[BUTTON_PIN].bottom + 10);
				dlg.DoModal();
				mInstructionIdx--;

				int theme = mTheme;
				dlg.Clear();
				dlg.AddHeading(L"One More Thing");
				dlg.AddString(L"Want to change options and preferences?");
				dlg.AddString(L"then, ");
				dlg.AddBoldString(L"Right click!");
				dlg.AddString(L"You can set various options on context menu");
				dlg.AddString(L"and can even change the theme");
				dlg.EnableTimer(2);
				dlg.DoModal();
				dlg.DisableTimer();
				SetTheme(theme);
				mInstructionIdx--;

				dlg.Clear();
				dlg.AddHeading(L"Okay");
				dlg.AddBoldString(L"Let's get on with it!");
				dlg.AddString(L"Press F1 if you want to see this help again");
				dlg.DoModal();
			}
		}
		mInstructionIdx = 0;
		KillTimer(TID_HELP);
	}

	if (nIDEvent == TID_TICK)
	{
		if(!mSetting)
		{
			Watch * watch = mWatches.GetWatchSet();
			while(watch)
			{
				if(watch->IsTimeSet())
				{
					if(GetTickCount64() + CHK_INTERVAL < watch->mTimeSet)
					{
						Invalidate(FALSE);
					}
					else
					{
						FLASHWINFO fi;
						fi.cbSize = sizeof(FLASHWINFO);
						fi.hwnd = this->m_hWnd;
						fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
						fi.uCount = 0;
						fi.dwTimeout = 0;
						::FlashWindowEx(&fi);
						mMuteTick = 5;
						PlaySound((LPCWSTR)MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL), SND_ASYNC | SND_RESOURCE);
						mWatches.Activate(watch);
						watch->Stop();

						TIMES_UP = GetTickCount64();
						SetTimer(TID_TIMESUP, 100, NULL);
					}
				}
				watch = watch->mNext;
			}
		}
	}
	else if (nIDEvent == TID_REFRESH)
	{
		Watch * watch = mWatches.GetHead();
		if(!watch->IsTimerMode() || mDigitalWatch)
		{
			static int s = 0;
			int cm = CTime::GetCurrentTime().GetMinute();
			int cs = CTime::GetCurrentTime().GetSecond();
			if(cs != s)
			{
				s = cs;
				Invalidate(FALSE);
			}
		}
	}
	else if(nIDEvent == TID_TIMESUP)
	{
		if(TIMES_UP < 0)
		{
			KillTimer(TID_TIMESUP);
		}
		Invalidate(FALSE);
	}
	else if(nIDEvent == TID_HELP)
	{
		Invalidate(FALSE);
	}
	NaraDialog::OnTimer(nIDEvent);
}

void CNaraTimerDlg::SetTitle(CString str, BOOL still_editing)
{
	OnTitleChanging();
	mWatches.GetHead()->mTitle = str;
	mTitleEdit.SetWindowText(str);
	TITLE_CHANGING = FALSE;
	Invalidate(FALSE);
	mTitleEdit.ShowWindow(SW_HIDE);
	if(!still_editing)
	{
		TITLE_CHANGING = TRUE;
		PostMessage(WM_KEYDOWN, VK_RETURN, 0);
	}
}

void CNaraTimerDlg::SetTitle()
{
	OnTitleChanging();
	mTitleEdit.SetSel(0, -1);
	TITLE_CHANGING = TRUE;
}

void CNaraTimerDlg::OnLButtonDown(UINT nFlags, CPoint pt)
{
	this->SetFocus();
	mTitleEdit.ShowWindow(SW_HIDE);
	TITLE_CHANGING = FALSE;

	if(TIMES_UP >= 0)
	{
		TIMES_UP = -100.f;
		return;
	}

	int ht = HitTest(pt);
	if(ht != HTCLIENT)
	{
		SetArrowCursor(ht);
		SendMessage(WM_NCLBUTTONDOWN, ht, MAKELPARAM(pt.x, pt.y));
		return;
	}
	mButtonHover = -1;
	for(int i = 0; i < NUM_BUTTONS; i++)
	{
		if(PT_IN_RECT(pt, mButtonRect[i]))
		{
			mButtonHover = i;
			Invalidate(FALSE);
			SetCapture();
			NaraDialog::OnLButtonDown(nFlags, pt);
			return;
		}
	}

	if(mViewMode == VIEW_WATCH)
	{
		if(mTitleEdit.GetWindowTextLength() == 0)
		{
			mWatches.GetHead()->mTitle = L"";
			Invalidate(FALSE);
		}

		int cx = (mTimerRect.right + mTimerRect.left) >> 1;
		int cy = (mTimerRect.bottom + mTimerRect.top) >> 1;
		int d = SQ(pt.x - cx) + SQ(pt.y - cy);
		if(d < SQ(mRadius + mGridSize))
		{
			KillTimer(TID_TICK);
			float deg = pt2deg(pt);
			mSetting = mWatches.GetHead();
			SettingTime(deg, TRUE);
			if(mSetting->IsAlarmMode())
			{
				mSetting->mTime360 = MAX_TIME360;
				mSetting->mIsTimer = FALSE;
			}
			Invalidate(FALSE);
			SetCapture();
		}
		else
		{
			SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
		}
	}
	else
	{
		int top = (mResizeMargin + (mRoundCorner >> 1));
		int idx = (pt.y - top) / mWatches.mItemHeight;
		if(pt.y >= top && idx < mWatches.GetSize())
		{
			mWatches.mItemHighlighted = idx;
		}
		else
		{
			mWatches.mItemHighlighted = -1;
			SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
		}
	}
	NaraDialog::OnLButtonDown(nFlags, pt);
}

void CNaraTimerDlg::OnMouseMove(UINT nFlags, CPoint pt)
{
	NaraDialog::OnMouseMove(nFlags, pt);

	if(!(nFlags & MK_LBUTTON))
	{
		if(mButtonHover >= 0 && !PT_IN_RECT(pt, mButtonRect[mButtonHover]))
		{
			mButtonHover = -1;
			Invalidate(FALSE);
		}
		for(int i = 0; i < NUM_BUTTONS; i++)
		{
			if(PT_IN_RECT(pt, mButtonRect[i]))
			{
				mButtonHover = i;
				Invalidate(FALSE);
				break;
			}
		}
	}

	if(mViewMode == VIEW_WATCH)
	{
		static BOOL hovering_title = FALSE;
		BOOL hovering_title_now = FALSE;
		int cx = (mTimerRect.right + mTimerRect.left) >> 1;
		int cy = (mTimerRect.bottom + mTimerRect.top) >> 1;
		int d = SQ(pt.x - cx) + SQ(pt.y - cy);
		if(mSetting && (nFlags & MK_LBUTTON))
		{
			float deg = pt2deg(pt);
			if(deg + mDegOffset > 360) deg = 360 - mDegOffset;
			int quarter0 = (int)(mOldDeg / 90);
			int quarter1 = (int)(deg / 90);
			if(quarter0 >= 3 && quarter1 <= 1)
			{
				deg = 360 - mDegOffset;
			}
			else if(quarter0 == 0 && quarter1 >= 3)
			{
				deg = -mDegOffset;
			}
			SettingTime(deg, d <= (mRadius + (mGridSize >> 1)) * (mRadius + (mGridSize >> 1)));
			Invalidate(FALSE);
		}
		else
		{
			if(d < (mRadiusHandsHead * mRadiusHandsHead))
			{
				::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
			}
		}
		if(hovering_title != hovering_title_now)
		{
			Invalidate(FALSE);
			hovering_title = hovering_title_now;
		}
	}
	else
	{
		int top = (mResizeMargin + (mRoundCorner >> 1));
		int idx = (pt.y - top) / mWatches.mItemHeight;
		if(idx != mWatches.mItemHighlighted)
		{
			mWatches.mItemHighlighted = idx;
			Invalidate(FALSE);
		}
	}
}

void CNaraTimerDlg::OnLButtonUp(UINT nFlags, CPoint pt)
{
	NaraDialog::OnLButtonUp(nFlags, pt);

	if(mButtonHover >= 0)
	{
		if(PT_IN_RECT(pt, mButtonRect[mButtonHover]))
		{
			if(mButtonHover == BUTTON_CLOSE)
			{
				PostMessage(WM_CLOSE, 0, 0);
			}
			else if(mButtonHover == BUTTON_PIN)
			{
				PostMessage(WM_PIN, 0, 0);
			}
			else if(mButtonHover == BUTTON_CENTER)
			{
				Watch * watch = mWatches.GetHead();
				if(!watch->IsTimeSet())
				{
					KillTimer(TID_TICK);
					watch->SetMode(watch->IsAlarmMode());
				}
				CClientDC dc(this);
				int m = mRadiusHandsHead >> 1;
				RECT rt = { mCrt.left + m, mCrt.top + m, mCrt.right - m, mCrt.bottom - m };
				DrawTimer(&dc, mWatches.GetUnset(), &rt);
				DrawBorder(&dc);
			}
		}
		mButtonHover = -1;
		Invalidate(FALSE);
		ReleaseCapture();
	}
	else if(mViewMode == VIEW_WATCH)
	{
		if(mSetting)
		{
			ReleaseCapture();
			mSetting = NULL;
			Invalidate(FALSE);
			KillTimer(TID_TICK);
			if(mOldDeg > -mDegOffset)
			{
				SetTimer(TID_TICK, CHK_INTERVAL, NULL);
				if(mWatches.GetSize() > 1)
				{
					mWatches.Sort(mWatches.GetHead());
					mWatches.Add();
					SetViewMode(VIEW_LIST);
				}
			}
			else
			{
				Stop();
			}
		}
	}
	else
	{
		if(mWatches.mItemHighlighted >= 0 && mWatches.mItemHighlighted < mWatches.GetSize())
		{
			mLastWatch = mWatches.Get(mWatches.mItemHighlighted);
			SetViewMode(VIEW_WATCH);
		}
	}
}

void CNaraTimerDlg::SetViewMode(int mode)
{
	BOOL animation = FALSE;
	RECT rt;
	CopyRect(&rt, &mCrt);
	if(mode == VIEW_WATCH)
	{
		if(mViewMode != VIEW_WATCH)
		{
			mViewMode = VIEW_WATCH;
			if(mLastWatch)
			{
				mWatches.Activate(mLastWatch);
			}
			animation = TRUE;
		}
	}
	else
	{
		if(mViewMode != VIEW_LIST)
		{
			mViewMode = VIEW_LIST;
			animation = TRUE;
		}
		mWatches.Sort(mWatches.GetHead());
	}
	if(animation)
	{
		DWORD tbeg = GetTickCount64();
		BOOL cond = TRUE;
		while(cond)
		{
			int dt = GetTickCount64() - tbeg;
			if(dt > 300)
			{
				dt = 300;
				cond = FALSE;
			}
			float r = tanh(dt / 80.f);
			if(mViewMode == VIEW_WATCH)
			{
				rt.top = r * mCrt.top + (1 - r) * (mCrt.top + mCrt.top - mCrt.bottom);
			}
			else
			{
				rt.top = r * mCrt.top + (1 - r) * mCrt.bottom;
			}
			rt.bottom = rt.top + (mCrt.bottom - mCrt.top);
			Draw(&rt);
		}
	}
}

BOOL CNaraTimerDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if(zDelta < 0)
	{
		SetViewMode(VIEW_LIST);
	}
	else
	{
		SetViewMode(VIEW_WATCH);
	}
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void CNaraTimerDlg::OnContextMenu(CWnd * pWnd, CPoint pt)
{
	CMenu menu, theme;
	BOOL is_set = (mWatches.GetHead()->IsTimeSet());
	BOOL is_timer = (mViewMode == VIEW_WATCH && mWatches.GetHead()->IsTimerMode());
	BOOL is_alarm = (mViewMode == VIEW_WATCH && mWatches.GetHead()->IsAlarmMode());
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING | (is_set ? MF_ENABLED : MF_DISABLED), IDM_NEW, L"New");
	menu.AppendMenu(MF_STRING | (is_set ? MF_ENABLED: MF_DISABLED), IDM_STOP, L"Stop\tESC");
	menu.AppendMenu(MF_SEPARATOR, 0, L"");
	menu.AppendMenu(MF_STRING | (is_timer ? MF_CHECKED : 0), IDM_TIMERMODE, L"Timer Mode");
	menu.AppendMenu(MF_STRING | (is_alarm ? MF_CHECKED : 0), IDM_ALARMMODE, L"Alarm Mode");
	menu.AppendMenu(MF_SEPARATOR, 0, L"");
	menu.AppendMenu(MF_STRING|(mTopmost?MF_CHECKED:0), IDM_TOPMOST, L"Always On Top");
	menu.AppendMenu(MF_SEPARATOR, 0, L"");
	theme.CreatePopupMenu();
	theme.AppendMenu(MF_STRING | (mTheme == THEME_DEFAULT? MF_CHECKED: 0), IDM_THEMEDEFAULT, L"Default");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_DARK? MF_CHECKED: 0), IDM_THEMEBLACK, L"Dark");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_BLUE? MF_CHECKED: 0), IDM_THEMEBLUE, L"Blue");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_GREEN? MF_CHECKED: 0), IDM_THEMEGREEN, L"Green");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_ORANGE? MF_CHECKED: 0), IDM_THEMEORANGE, L"Orange");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_MINT? MF_CHECKED: 0), IDM_THEMEMINT, L"Mint");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_PINK? MF_CHECKED: 0), IDM_THEMEPINK, L"Pink");
	menu.AppendMenuW(MF_POPUP, (UINT_PTR)theme.Detach(), L"Themes");
	menu.AppendMenuW(MF_STRING, IDM_FONT, L"Font...");
	menu.AppendMenuW(MF_STRING | (mDigitalWatch ? MF_CHECKED : 0), IDM_TOGGLEDIGITALWATCH, L"Digital Watch");
	menu.AppendMenuW(MF_STRING | (mHasDate ? MF_CHECKED : 0), IDM_TOGGLEDATE, L"Date");
	menu.AppendMenuW(MF_STRING | (mTickSound ? MF_CHECKED : 0), IDM_TOGGLETICKSOUND, L"Ticking Sound");
	menu.AppendMenu(MF_SEPARATOR, 0, L"");
	menu.AppendMenuW(MF_STRING, ID_HELP, L"Show Help...\tF1");
	menu.AppendMenuW(MF_STRING, IDM_ABOUT, L"About NaraTimer...");
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
}

void CNaraTimerDlg::OnNew(void)
{
#if 0
	CString param;
	WINDOWPLACEMENT pl;
	GetWindowPlacement(&pl);
	if(pl.showCmd != SW_MAXIMIZE)
	{
		int off = (mResizeMargin >> 1);
		int is_timer = mWatches.GetHead()->IsTimerMode();
		param.Format(L"--position %d %d %d %d --mode %d", pl.rcNormalPosition.left, pl.rcNormalPosition.top + off, pl.rcNormalPosition.right, pl.rcNormalPosition.bottom + off, is_timer);
	}
	wchar_t path[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), path, MAX_PATH);
	ShellExecute(GetSafeHwnd(), L"open", path, param, NULL, 1);
#else
	mWatches.Add();
#endif
}

void CNaraTimerDlg::OnStop(void)
{
	Stop();
}

void CNaraTimerDlg::OnTimerMode(void)
{
	mWatches.GetUnset()->SetMode(TRUE);
}

void CNaraTimerDlg::OnAlarmMode(void)
{
	mWatches.GetUnset()->SetMode(FALSE);
}

void CNaraTimerDlg::OnMenuPin(void)
{
	SetTopmost(!mTopmost);
}

void CNaraTimerDlg::OnMenuFont(void)
{
	LOGFONT lf;
	GetLogfont(&lf, 10, FALSE);
	CFontDialog dlg(&lf, CF_ANSIONLY|CF_NOSIZESEL, NULL, NULL);
	if(dlg.DoModal() == IDOK)
	{
		LOGFONT lf;
		dlg.GetCurrentFont(&lf);
		memcpy(mFontFace, lf.lfFaceName, sizeof(mFontFace));
		mFontScale = 100;
		AfxGetApp()->WriteProfileString(L"Setting", L"Font", CString(mFontFace));
		AfxGetApp()->WriteProfileInt(L"Setting", L"FontScale", mFontScale);
	}
}

void CNaraTimerDlg::OnMenuHelp(void)
{
	mInstructionIdx = 2;
}

void CNaraTimerDlg::OnMenuAbout(void)
{
	mInstructionIdx = 999;
}

void CNaraTimerDlg::OnThemeDefault(void)
{
	SetTheme(THEME_DEFAULT);
}

void CNaraTimerDlg::OnThemeDark(void)
{
	SetTheme(THEME_DARK);
}

void CNaraTimerDlg::OnThemeBlue(void)
{
	SetTheme(THEME_BLUE);
}

void CNaraTimerDlg::OnThemeGreen(void)
{
	SetTheme(THEME_GREEN);
}

void CNaraTimerDlg::OnThemeOrange(void)
{
	SetTheme(THEME_ORANGE);
}

void CNaraTimerDlg::OnThemeMint(void)
{
	SetTheme(THEME_MINT);
}

void CNaraTimerDlg::OnThemePink(void)
{
	SetTheme(THEME_PINK);
}

void CNaraTimerDlg::OnToggleDigitalWatch(void)
{
	mDigitalWatch = !mDigitalWatch;
	AfxGetApp()->WriteProfileInt(L"Theme", L"DigitalWatch", mDigitalWatch);
	Invalidate(FALSE);
}

void CNaraTimerDlg::OnToggleDate(void)
{
	mHasDate = !mHasDate;
	AfxGetApp()->WriteProfileInt(L"Theme", L"HasDate", mHasDate);
	Invalidate(FALSE);
}

void CNaraTimerDlg::OnToggleTickSound(void)
{
	mTickSound = !mTickSound;
	AfxGetApp()->WriteProfileInt(L"Theme", L"TickSound", mTickSound);
	Invalidate(FALSE);
}

void CNaraTimerDlg::reposition(void)
{
	int x, y, r, sz, bsz;

	bsz = ROUND(MIN(mCrt.right - mCrt.left, mCrt.bottom - mCrt.top) * 0.1f);
	r = (int)((mRoundCorner - mResizeMargin) / 1.414f);
	y = (mCrt.top + mRoundCorner - r);

	// pin button
	sz = max(ROUND(bsz * 1.5f), 30);
	x = (mCrt.left + mRoundCorner - r);
	mButtonRect[BUTTON_PIN].SetRect(x, y, x + sz, y + sz);
	// close button
	sz = max(ROUND(bsz * 0.8f), 16);
	x = (mCrt.right - mRoundCorner + r);
	mButtonRect[BUTTON_CLOSE].SetRect(x - sz, y, x, y + sz);
	SET_BUTTON(BUTTON_CLOSE, IDI_CLOSE, IDI_CLOSE_HOVER);
	// hand's head - rect is updated in DrawTimer()
	SET_BUTTON(BUTTON_CENTER, NULL, NULL);
}

void CNaraTimerDlg::OnSize(UINT nType, int cx, int cy)
{
	int sz = MIN(cx, cy);
	SetWindowBorder(ROUND(sz / 3.5f), max(ROUND(sz / 14.f), 5));

	NaraDialog::OnSize(nType, cx, cy);

	reposition();
	Invalidate(FALSE);
}

void CNaraTimerDlg::OnGetMinMaxInfo(MINMAXINFO * lpMMI)
{
	int wh = 50;
	lpMMI->ptMinTrackSize.x = wh;
	lpMMI->ptMinTrackSize.y = wh;
	NaraDialog::OnGetMinMaxInfo(lpMMI);
}

void CNaraTimerDlg::OnWindowPosChanged(WINDOWPOS * pos)
{
	NaraDialog::OnWindowPosChanged(pos);
	Invalidate(FALSE);
}

void CNaraTimerDlg::OnActivate(UINT nState, CWnd * pWndOther, BOOL bMinimized)
{
	if(nState != WA_INACTIVE)
	{
		TIMES_UP = -100.f;
		mWatches.RemoveStopped();
		Invalidate(FALSE);
	}
	NaraDialog::OnActivate(nState, pWndOther, bMinimized);
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CNaraTimerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CNaraTimerDlg::OnPinToggle(WPARAM wParam, LPARAM lParam)
{
	SetTopmost(!mTopmost);
	return S_OK;
}

void CNaraTimerDlg::OnTitleChanging(void)
{
	if(TITLE_CHANGING == FALSE)
	{
		CFont font;
		int font_size = HUD_FONT_SIZE;
		GetFont(font, font_size, TRUE);
		mTitleEdit.SetFont(&font, TRUE);
		mTitleEdit.ShowWindow(SW_SHOW);
		TITLE_CHANGING = TRUE;
		if(mViewMode == VIEW_LIST)
		{
			int h = font_size + 10;
			int y = ((mCrt.top + mCrt.bottom) >> 1) - ((mCrt.bottom - mCrt.top) >> 2);
			mTitleEdit.MoveWindow(mRoundCorner, y, mCrt.right - mRoundCorner*2, h, TRUE);
		}
		Invalidate(FALSE);
	}
	mTitleEdit.SetFocus();
	TIMES_UP = -100.f;
}


