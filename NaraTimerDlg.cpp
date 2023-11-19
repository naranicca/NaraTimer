#include "pch.h"
#include "framework.h"
#include "NaraTimer.h"
#include "NaraTimerDlg.h"
#include "afxdialogex.h"

#define RED					RGB(249, 99, 101)
#define WHITE				RGB(255, 255, 255)
#define CHK_INTERVAL		(300)
#define TIMER_TIME360		(3600000) // 1h in ms
#define MAX_TIME360			(12*3600000) // 12h
#define IS_TIMER_MODE		(mTime360 == TIMER_TIME360)
#define IS_ALARM_MODE		(!IS_TIMER_MODE)
#define TITLE_OFFSET		ROUND(mTitleHeight * 1.3f)
int RESIZE_MARGIN = 15;
int ROUND_CORNER = 50;
COLORREF BORDER_COLOR = RED;

#define TID_TICK			(0)
#define TID_REFRESH			(1)

#pragma comment(lib, "winmm")
#include <mmsystem.h>

#define MIN(a, b)			((a) < (b) ? (a) : (b))
#define MAX(a, b)			((a) > (b) ? (a) : (b))
#define ROUND(a)			(int)((a) + 0.5f)
#define SQ(x)				((x) * (x))
#define LBUTTON_DOWN		(GetKeyState(VK_LBUTTON) & 0x8000)
#define CTRL_DOWN			(GetKeyState(VK_CONTROL) & 0x8000)
#define SET_DOCKED_STYLE	ModifyStyle(WS_CAPTION|WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX, WS_POPUP)
#define SET_WINDOWED_STYLE	ModifyStyle(WS_CAPTION|WS_POPUP, WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define SET_BUTTON(id, idi, idi_hover) { mButtonIcon[id] = idi; mButtonIconHover[id] = idi_hover; }
#define PT_IN_RECT(pt, rt)	((pt).x >= (rt).left && (pt).x < (rt).right && (pt).y >= (rt).top && (pt).y < (rt).bottom)
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

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CNaraTimerDlg::CNaraTimerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NARATIMER_DIALOG, pParent)
{
	// set class name
	WNDCLASS c = {};
	::GetClassInfo(AfxGetInstanceHandle(), L"#32770", &c);
	c.lpszClassName = L"NaraTimer_designed-by-naranicca";
	AfxRegisterClass(&c);

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	mTheme = THEME_LIGHT;
	mTimeSet = 0;
	mSetting = FALSE;
	mOldDeg = 0;
#if 0
	mIsTimer = TRUE;
	mTime360 = TIMER_TIME360;
#else
	mIsTimer = FALSE;
	mTime360 = MAX_TIME360;
#endif
	mRadius = 0;
	mRadiusHandsHead = 0;
	memset((void*)&mTimestamp, 0, sizeof(mTimestamp));
	memset((void*)mButtonIcon, 0, sizeof(mButtonIcon));
	memset((void*)mButtonIconHover, 0, sizeof(mButtonIconHover));
	mButtonHover = -1;
	mTopmost = FALSE;
	mTitleHeight = 0;
	mDegOffset = 0;
	mIsMiniMode = FALSE;
	mResizing = FALSE;
	mMuteTick = 0;
}

void CNaraTimerDlg::Stop(void)
{
	if (!mIsMiniMode)
	{
		KillTimer(TID_TICK);
	}
	mTimeSet = 0;
	if (IS_ALARM_MODE)
	{
		mIsTimer = FALSE;
		mTime360 = MAX_TIME360;
	}
	mTimeStr = L"";
	mHM = { 0, };
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
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNaraTimerDlg, CDialogEx)
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
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT, OnTitleChanging)
	ON_MESSAGE(WM_PIN, OnPinToggle)
	ON_COMMAND(IDM_NEW, OnNew)
	ON_COMMAND(IDM_TIMERMODE, OnTimerMode)
	ON_COMMAND(IDM_ALARMMODE, OnAlarmMode)
	ON_COMMAND(IDM_TOPMOST, OnMenuPin)
	ON_COMMAND(IDM_THEMEDEFAULT, OnThemeLight)
	ON_COMMAND(IDM_THEMEBLACK, OnThemeDark)
	ON_COMMAND(IDM_THEMEBLUE, OnThemeBlue)
	ON_COMMAND(IDM_THEMEGREEN, OnThemeGreen)
	ON_COMMAND(IDM_THEMEORANGE, OnThemeOrange)
	ON_COMMAND(IDM_THEMEMINT, OnThemeMint)
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
	CDialogEx::OnInitDialog();

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

	mTheme = AfxGetApp()->GetProfileInt(L"Theme", L"CurrentTheme", THEME_LIGHT);
	mDigitalWatch = AfxGetApp()->GetProfileInt(L"Theme", L"DigitalWatch", 1);
	mHasDate = AfxGetApp()->GetProfileInt(L"Theme", L"HasDate", 1);
	mTickSound = AfxGetApp()->GetProfileInt(L"Theme", L"TickSound", 0);

	// get font name
	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
	memcpy(&mFontFace, &metrics.lfMessageFont.lfFaceName, sizeof(metrics.lfMessageFont.lfFaceName));

	reposition();
	mTitleEdit.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER, CRect(0, 0, 10, 10), this, IDC_EDIT);
	mTitleEdit.ShowWindow(SW_HIDE);

	SetTimer(TID_REFRESH, 1000, NULL);
	SetTopmost(FALSE);

	SetWindowText(L"NaraTimer");

	mRunning = TRUE;
	mThread = AfxBeginThread(FnThreadTicking, this);

	return TRUE;
}

void CNaraTimerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
}

BOOL CNaraTimerDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
			if (mTimeSet && GetFocus()->GetSafeHwnd() != mTitleEdit.GetSafeHwnd())
			{
				CString str = (IS_TIMER_MODE ? L"Stop the timer?" : L"Stop the alarm?");
				if (mTimeSet && AfxMessageBox(str, MB_OKCANCEL) == IDOK)
				{
					Stop();
				}
			}
		case VK_RETURN:
			mTitleEdit.ShowWindow(SW_HIDE);
			mTitleEdit.GetWindowText(mTitle);
			if (mTitle.IsEmpty())
			{
				mTitleHeight = 0;
				SetWindowText(L"NaraTimer");
			}
			else
			{
				SetWindowText(mTitle);
			}
			this->SetFocus();
			Invalidate(FALSE);
			return TRUE;
		case VK_DOWN:
		case VK_OEM_MINUS:
			if (mTimeSet > 60000)
			{
				mTimeSet -= 60000;
				if (IS_ALARM_MODE)
				{
					if (mHM.cy > 0)
					{
						mHM.cy--;
					}
					else
					{
						mHM.cx = (mHM.cx > 0 ? mHM.cx - 1 : 11);
						mHM.cy = 59;
					}
					mTimeStr.Format(L"%d:%02d", (mHM.cx > 12 ? mHM.cx - 12 : mHM.cx), mHM.cy);
				}
				return TRUE;
			}
			break;
		case VK_UP:
		case VK_OEM_PLUS:
			if (mTimeSet > 0)
			{
				mTimeSet += 60000;
				if (IS_ALARM_MODE)
				{
					mTime360 += 60000;
					mHM.cy++;
					if (mHM.cy >= 60)
					{
						mHM.cx++;
						mHM.cy -= 60;
					}
					mTimeStr.Format(L"%d:%02d", (mHM.cx > 12 ? mHM.cx - 12 : mHM.cx), mHM.cy);
				}
				return TRUE;
			}
			break;
		case VK_F2:
			SetTitle();
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
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CNaraTimerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if (nID == IDM_TOPMOST)
	{
		SetTopmost(!mTopmost);
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

int CNaraTimerDlg::HitTest(CPoint pt)
{
	RECT crt;
	GetClientRect(&crt);
	int ht = HTCLIENT;
	if (GetStyle() & WS_MAXIMIZE)
	{
		ht = (pt.y < RESIZE_MARGIN ? HTCAPTION : HTCLIENT);
	}
	else
	{
		if (pt.x < ROUND_CORNER && pt.y < ROUND_CORNER)
		{
			int d = SQ(pt.x - ROUND_CORNER) + SQ(pt.y - ROUND_CORNER);
			ht = (d >= SQ(ROUND_CORNER - RESIZE_MARGIN) ? HTTOPLEFT : HTCLIENT);
		}
		else if (pt.x > crt.right - ROUND_CORNER && pt.y < ROUND_CORNER)
		{
			int d = SQ(pt.x - crt.right + ROUND_CORNER) + SQ(pt.y - ROUND_CORNER);
			ht = (d >= SQ(ROUND_CORNER - RESIZE_MARGIN) ? HTTOPRIGHT : HTCLIENT);
		}
		else if (pt.x < ROUND_CORNER && pt.y > crt.bottom - ROUND_CORNER)
		{
			int d = SQ(pt.x - ROUND_CORNER) + SQ(pt.y - crt.bottom + ROUND_CORNER);
			ht = (d >= SQ(ROUND_CORNER - RESIZE_MARGIN) ? HTBOTTOMLEFT : HTCLIENT);
		}
		else if (pt.x > crt.right - ROUND_CORNER && pt.y > crt.bottom - ROUND_CORNER)
		{
			int d = SQ(pt.x - crt.right + ROUND_CORNER) + SQ(pt.y - crt.bottom + ROUND_CORNER);
			ht = (d >= SQ(ROUND_CORNER - RESIZE_MARGIN) ? HTBOTTOMRIGHT : HTCLIENT);
		}
		else if (pt.y < RESIZE_MARGIN)
		{
			ht = (pt.x < RESIZE_MARGIN ? HTTOPLEFT : pt.x > crt.right - RESIZE_MARGIN ? HTTOPRIGHT : HTTOP);
		}
		else if (pt.y > crt.bottom - RESIZE_MARGIN)
		{
			ht = (pt.x < RESIZE_MARGIN ? HTBOTTOMLEFT : pt.x > crt.right - RESIZE_MARGIN ? HTBOTTOMRIGHT : HTBOTTOM);
		}
		else
		{
			ht = (pt.x < RESIZE_MARGIN ? HTLEFT : pt.x > crt.right - RESIZE_MARGIN ? HTRIGHT : HTCLIENT);
		}
	}
	return ht;
}

void CNaraTimerDlg::SetArrowCursor(int hittest)
{
	switch (hittest)
	{
	case HTTOPLEFT:
	case HTBOTTOMRIGHT:
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENWSE));
		break;
	case HTTOPRIGHT:
	case HTBOTTOMLEFT:
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENESW));
		break;
	case HTLEFT:
	case HTRIGHT:
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
		break;
	case HTTOP:
	case HTBOTTOM:
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
		break;
	case HTCLIENT:
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
		break;
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

ULONGLONG CNaraTimerDlg::deg2time(float deg, BOOL stick)
{
	if (IS_TIMER_MODE)
	{
		ULONGLONG t = (ULONGLONG)(deg * mTime360 / 360);
		if (stick)
		{
			t = ((t + 30000) / 60000) * 60000;
		}
		mTimeStr = L"";
		return GetTickCount64() + t;
	}
	else
	{
		ULONGLONG t = (ULONGLONG)(deg * mTime360 / 360);
		t = ((t + 30000) / 60000) * 60000;
		if (stick)
		{
			t = ((t + 300000) / 600000) * 600000;
		}
		CTime c = CTime::GetCurrentTime();
		int h = c.GetHour() + (int)(t / 3600000.f);
		int m = ((int)((t / 1000)) % 3600) / 60;
		mHM.cx = (h < 24 ? h : h - 24);
		mHM.cy = m;
		mTimeStr.Format(L"%d:%02d %s", (mHM.cx > 12 ? mHM.cx - 12 : mHM.cx), mHM.cy, mHM.cx < 12? L"am": L"pm");
		ULONGLONG o = (c.GetMinute() * 60 + c.GetSecond()) * 1000;
		return GetTickCount64() + (t > o ? t - o : 0);
	}
}

static void GetLogfont(LOGFONTW * lf, int height, BOOL bold)
{
	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
	metrics.lfMessageFont.lfHeight = height;
	metrics.lfMessageFont.lfWeight = (bold ? FW_BOLD : FW_NORMAL);
	memcpy(lf, &metrics.lfMessageFont, sizeof(LOGFONTW));
}

void CNaraTimerDlg::GetFont(CFont &font, int height, BOOL bold)
{
	LOGFONTW lf;
	GetLogfont(&lf, height, bold);
	font.CreateFontIndirect(&lf);
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

int CNaraTimerDlg::GetTitleHeight(void)
{
	RECT crt;
	GetClientRect(&crt);
	int w = crt.right - crt.left;
	int h = crt.bottom - crt.top;
	return (int)(min(w, h) * 0.1f);
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

void CNaraTimerDlg::DrawTimer(CDC * dc, RECT * rt, float scale, BOOL draw_border)
{
	Graphics g(*dc);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetTextRenderingHint(TextRenderingHintAntiAlias);
	RECT border_rect;
	POINT pt0, pt1;
	int clock;
	int start = 0;
	COLORREF bk_color, grid_color, pie_color, hand_color, handshead_color, timestr_color;
	float hand_size;
	float handshead_size;

	switch(mTheme)
	{
	case THEME_DARK:
		bk_color = RGB(8, 9, 10);
		grid_color = WHITE;
		pie_color = RED;
		hand_color = WHITE;
		handshead_color = RGB(150, 150, 150);
		timestr_color = RGB(220, 220, 220);
		BORDER_COLOR = RGB(22, 23, 24);
		break;
	case THEME_BLUE:
		bk_color = RGB(16, 26, 59);
		grid_color = WHITE;
		pie_color = RED;
		hand_color = WHITE;
		handshead_color = RGB(69, 70, 71);
		timestr_color = RGB(220, 220, 220);
		BORDER_COLOR = RGB(16, 41, 145);
		break;
	case THEME_GREEN:
		bk_color = RGB(20, 46, 29);
		grid_color = WHITE;
		pie_color = RED;
		hand_color = WHITE;
		handshead_color = RGB(69, 70, 71);
		timestr_color = RGB(220, 220, 220);
		BORDER_COLOR = RGB(30, 108, 78);
		break;
	case THEME_ORANGE:
		bk_color = RGB(229, 119, 33);
		grid_color = WHITE;
		pie_color = RGB(24, 57, 186);
		hand_color = RGB(108, 185, 66);
		handshead_color = RGB(19, 20, 21);
		timestr_color = RGB(220, 220, 220);
		BORDER_COLOR = RGB(244, 76, 11);
		break;
	case THEME_MINT:
		bk_color = RGB(64, 224, 208);
		grid_color = RGB(0, 0, 0);
		pie_color = RGB(63, 67, 84);
		hand_color = WHITE;
		handshead_color = RGB(19, 20, 21);
		timestr_color = RGB(20, 20, 20);
		BORDER_COLOR = RGB(204, 202, 205);
		break;
	default:
		bk_color = WHITE;
		grid_color = RGB(0, 0, 0);
		pie_color = RED;
		hand_color = RGB(220, 220, 220);
		handshead_color = RGB(77, 88, 94);
		timestr_color = RGB(20, 20, 20);
		BORDER_COLOR = RED;
		break;
	}
	if (IS_TIMER_MODE)
	{
		hand_size = 0.22f;
		handshead_size = 0.14f;
		hand_color = handshead_color;
	}
	else
	{
		hand_size = 1.f;
		handshead_size = 0.07f;
	}

	if(mIsMiniMode)
	{
		timestr_color = grid_color;
		for(int i = 0; i < 2; i++)
		{
			grid_color = blend_color(grid_color, bk_color);
			pie_color = blend_color(pie_color, bk_color);
			hand_color = blend_color(hand_color, bk_color);
			handshead_color = blend_color(handshead_color, bk_color);
		}
	}

	dc->FillSolidRect(rt, bk_color);
	dc->SetBkMode(TRANSPARENT);
	GetClientRect(&mTimerRect);
	CopyRect(&border_rect, rt);
	if (mTitleHeight > 0)
	{
		int off = TITLE_OFFSET;
		rt->top += ROUND(off * scale);
		mTimerRect.top += off;
	}

	// font
	CFont font;
	int font_size = ROUND(min(rt->right - rt->left, rt->bottom - rt->top) * 0.065);
	GetFont(font, font_size);
	CFont* fonto = (CFont*)dc->SelectObject(&font);
	RECT trt;
	dc->DrawText(L" 88 ", 5, &trt, DT_CALCRECT | DT_SINGLELINE);
	int tw = (trt.right - trt.left);
	int th = (trt.bottom - trt.top);
	int tsize = ROUND(max(tw, th) * 1.1f);

	int w = (rt->right - rt->left - RESIZE_MARGIN * 2);
	int h = (rt->bottom - rt->top - RESIZE_MARGIN * 2);
	mGridSize = min(ROUND(min(w, h) * 0.023f), 20);
	int r = (MIN(w, h) >> 1) - (mGridSize + (mGridSize >> 1) + tsize);
	int x = (rt->left + rt->right - (r << 1)) >> 1;
	int y = (rt->top + rt->bottom - (r << 1)) >> 1;
	mRadius = ROUND(r / scale);

	// draw title
	if (mTitleHeight > 0)
	{
		CFont font;
		int fh = ROUND(mTitleHeight * scale);
		LOGFONTW lf;
		GetLogfont(&lf, fh, TRUE);
		font.CreateFontIndirectW(&lf);
		mTitleEdit.SetFont(&font);
		Gdiplus::Font gfont(dc->m_hDC, &lf);

		mTitleRect.left = ROUND(ROUND_CORNER * scale);
		mTitleRect.top = y - mGridSize - tsize - fh;
		mTitleRect.right = rt->right - ROUND(ROUND_CORNER * scale);
		mTitleRect.bottom = mTitleRect.top + fh;

		CString t;
		mTitleEdit.GetWindowText(t);
		if(t == mTitle)
		{
			StringFormat sf;
			sf.SetAlignment(StringAlignmentCenter);
			sf.SetLineAlignment(StringAlignmentFar);
			sf.SetTrimming(StringTrimmingEllipsisWord);
			SolidBrush br(Color(255, GetRValue(grid_color), GetGValue(grid_color), GetBValue(grid_color)));
			int top = ROUND(RESIZE_MARGIN * 1.2f * scale);
			int h = (int)((mTitleRect.bottom - top) / fh) * fh;
			top = mTitleRect.bottom - h;
			g.DrawString(mTitle.GetBuffer(), -1, &gfont, RectF(mTitleRect.left, top, \
				mTitleRect.right - mTitleRect.left, mTitleRect.bottom - top), &sf, &br);
		}
		mTitleRect.left = ROUND(mTitleRect.left / scale);
		mTitleRect.top = ROUND(mTitleRect.top / scale);
		mTitleRect.right = ROUND(mTitleRect.right / scale);
		mTitleRect.bottom = ROUND(mTitleRect.bottom / scale);
	}
	else
	{
		int cx = (int)(((rt->right + rt->left) >> 1) / scale);
		SetRect(&mTitleRect, cx - (tw >> 1), RESIZE_MARGIN, cx + (tw >> 1), y - mGridSize - tsize);
	}

	if (font_size > 5 * scale)
	{
		if (IS_TIMER_MODE)
		{
			mDegOffset = 0;
		}
		else
		{
			int m = CTime::GetCurrentTime().GetMinute();
			int s = CTime::GetCurrentTime().GetSecond();
			mDegOffset = -360000.f * (m * 60 + s) / mTime360;
		}
		// draw numbers
		dc->SetTextColor(grid_color);
		clock = CTime::GetCurrentTime().GetHour();
		for (int i = 0; i < 360; i += 30)
		{
			pt0 = deg2pt((float)i, r + mGridSize + (tsize >> 1));
			RECT rt = { x + r + pt0.x - (tw >> 1), y + r + pt0.y - (th >> 1), x + r + pt0.x + (tw >> 1), y + r + pt0.y + (th >> 1) };
			CString str;
			if (IS_TIMER_MODE)
			{
				str.Format(L"%d", (int)(i / 30) * 5);
			}
			else
			{
				int t = (int)(i / 30) + clock;
				str.Format(L"%d", (t == 0 ? 12 : (t <= 24 ? (t > 12 ? t - 12 : t) : t - 24)));
			}
			dc->DrawText(str, &rt, DT_CENTER | DT_VCENTER);
		}

		// ddraw grids
		DEFINE_PEN(penm, grid_color, 255, 1 * scale);
		clock = (IS_TIMER_MODE ? 6 : 5);
		for (int i = 0; i < 360; i += clock)
		{
			pt0 = deg2pt((float)i, r - (mGridSize >> 1));
			pt1 = deg2pt((float)i, r + (mGridSize >> 1));
			g.DrawLine(&penm, x + r + pt0.x, y + r + pt0.y, x + r + pt1.x, y + r + pt1.y);
		}
		DEFINE_PEN(penh, grid_color, 255, 3 * scale);
		for (int i = 0; i < 360; i += 30)
		{
			pt0 = deg2pt((float)i, r - mGridSize);
			pt1 = deg2pt((float)i, r + mGridSize);
			g.DrawLine(&penh, x + r + pt0.x, y + r + pt0.y, x + r + pt1.x, y + r + pt1.y);
		}
	}
	else
	{
		DEFINE_PEN(pen, grid_color, 255, 3 * scale);
		g.DrawEllipse(&pen, x, y, r << 1, r << 1);
	}

	// hands head shadow
	int sz = ROUND(r * handshead_size);
	int off = ROUND(sz * 0.15);
	mRadiusHandsHead = sz;
	SolidBrush brgrey2(Color(64, 0, 0, 0));
	g.FillEllipse(&brgrey2, x + r + off - sz, y + r + off - sz, 2 * sz, 2 * sz);

	// draw red pie
	LONGLONG t_remain = (mTimeSet > 0 ? mTimeSet - GetTickCount64() : 0);
	float deg = 0;
	if (IS_TIMER_MODE || !LBUTTON_DOWN)
	{
		deg = 360.f * t_remain / mTime360;
		deg -= mDegOffset;
	}
	else if (t_remain > 0)
	{
		CTime c = CTime::GetCurrentTime();
		int o = c.GetMinute() * 60 + c.GetSecond();
		deg = 360.f * (t_remain + o * 1000) / MAX_TIME360;
	}
	DrawPie(&g, r, deg, rt, pie_color);
	CString str = L"";
	if (deg > -mDegOffset)
	{
		ULONGLONG ts = t_remain / 1000;
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
			if (mTitle.IsEmpty())
			{
				SetWindowText(str);
			}
			else
			{
				SetWindowText(mTitle + L" " + str);
			}
			mTso = ts;
		}

		if (mHM.cy > 0)
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
			min.Format(L"%02d", mHM.cy);
			dc->SetTextColor(bk_color);
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
	if ((mDigitalWatch && IS_ALARM_MODE) || mIsMiniMode)
	{
		Gdiplus::Font font(mFontFace, ROUND(r / 4), FontStyleBold, UnitPixel);
		if (mSetting)
		{
			str = mTimeStr;
		}
		else if (t_remain == 0)
		{
			CTime t = CTime::GetCurrentTime();
			int h = t.GetHour();
			h = (h > 12 ? h - 12 : h);
			str.Format(L"%d:%02d:%02d", (h == 0 ? 12 : h) , t.GetMinute(), t.GetSecond());
			SetWindowText(str);
		}
		SetRect(&mTimeRect, x, y + r + sz, x + r + r, y + r + r - r/3);
		SolidBrush br(Color(255, GetRValue(timestr_color), GetGValue(timestr_color), GetBValue(timestr_color)));
		RectF rtf(mTimeRect.left, mTimeRect.top, mTimeRect.right - mTimeRect.left, mTimeRect.bottom - mTimeRect.top);
		Gdiplus::StringFormat sf;
		sf.SetAlignment(StringAlignmentCenter);
		sf.SetLineAlignment(StringAlignmentCenter);
		g.DrawString(str.GetBuffer(), -1, &font, rtf, &sf, &br);
	}

	// draw date complications
	if(mHasDate && IS_ALARM_MODE)
	{
		CFont font;
		GetFont(font, r / 6, TRUE);
		CFont* fonto = dc->SelectObject(&font);
		RECT trt = { 0, };
		CTime t = CTime::GetCurrentTime();
		CString d;
		d.Format(L"%d", t.GetDay());
		dc->DrawText(L"88", 2, &trt, DT_SINGLELINE | DT_CALCRECT);
		int w = ROUND((trt.right - trt.left) * 1.05f);
		int h = trt.bottom - trt.top;
		trt.left = (x + r - (w >> 1));
		trt.top = (y + r + r - ROUND(r / 3.f));
		trt.right = trt.left + w ;
		trt.bottom = trt.top + h;
		CBrush br(bk_color);
		CBrush* bro = dc->SelectObject(&br);
		COLORREF cl = blend_color(blend_color(blend_color(grid_color, bk_color), bk_color), bk_color);
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
	if (deg < -mDegOffset) deg = -mDegOffset;
	float pt = (r * 0.08f);
	DEFINE_PEN(pencl, handshead_color, 255, pt);
	pencl.SetStartCap(LineCapRound);
	pencl.SetEndCap(LineCapRound);
	if (hand_color == handshead_color)
	{
		pt0 = deg2pt(deg, ROUND(r * hand_size - pt/2 + scale));
		g.DrawLine(&pencl, x + r, y + r, x + r + pt0.x, y + r + pt0.y);
	}
	else
	{
		pt0 = deg2pt(deg, ROUND(sz * 1.8f));
		pt1 = deg2pt(deg, ROUND(r * hand_size - pt / 2 - scale));
		g.DrawLine(&pencl, x + r + pt0.x, y + r + pt0.y, x + r + pt1.x, y + r + pt1.y);

		DEFINE_PEN(p, handshead_color, 255, pt * 0.5f);
		pt1 = deg2pt(deg, ROUND(r * hand_size - pt / 2 - scale));
		g.DrawLine(&p, x + r, y + r, x + r + pt1.x, y + r + pt1.y);

		DEFINE_PEN(pen, hand_color, 255, pt * 0.5f);
		pen.SetStartCap(LineCapRound);
		pen.SetEndCap(LineCapRound);
		pt0 = deg2pt(deg, sz + sz);
		pt1 = deg2pt(deg, ROUND(r * hand_size - pt / 2 - scale));
		g.DrawLine(&pen, x + r + pt0.x, y + r + pt0.y, x + r + pt1.x, y + r + pt1.y);
	}
	// Hands head
	SolidBrush brgrey(Color(255, GetRValue(handshead_color), GetGValue(handshead_color), GetBValue(handshead_color)));
	g.FillEllipse(&brgrey, x + r - sz, y + r - sz, 2 * sz, 2 * sz);
	if(hand_color != handshead_color)
	{
		SolidBrush br(Color(255, GetRValue(hand_color), GetGValue(hand_color), GetBValue(hand_color)));
		int s = ROUND(sz * 0.3f);
		g.FillEllipse(&br, x + r - s, y + r - s, 2 * s, 2 * s);
	}

	if (draw_border)
	{
		DrawBorder(dc, &border_rect, scale);
	}
}

void CNaraTimerDlg::DrawBorder(CDC * dc, RECT * rt, float scale)
{
	WINDOWPLACEMENT pl;
	GetWindowPlacement(&pl);
	BOOL maximized = (pl.showCmd == SW_MAXIMIZE);

	Graphics g(*dc);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	int corner = (maximized ? 0 : ROUND(ROUND_CORNER * scale));
	// border
	{
		Pen pen(Color(255, GetRValue(BORDER_COLOR), GetGValue(BORDER_COLOR), GetBValue(BORDER_COLOR)), ROUND(RESIZE_MARGIN * 2 * scale));
		DrawRoundRect(&g, &pen, Rect(rt->left, rt->top, rt->right - rt->left, rt->bottom - rt->top), corner);
	}
	if(!maximized)
	{
		// border highlight
		{
			Pen pen(Color(64, 255, 255, 255), 3 * scale);
			DrawRoundRect(&g, &pen, Rect(rt->left, rt->top, (rt->right << 1) - rt->left, (rt->bottom << 1) - rt->top), corner);
		}
		// border shadow
		{
			Pen pen(Color(64, 0, 0, 0), 3 * scale);
			DrawRoundRect(&g, &pen, Rect(rt->left - 100, rt->top - 100, rt->right - rt->left + 100, rt->bottom - rt->top + 100), corner);
			Pen pen2(Color(64, 0, 0, 0), (1 * scale));
			int off = (RESIZE_MARGIN * scale);
			DrawRoundRect(&g, &pen2, Rect(rt->left + off, rt->top + off, rt->right - rt->left - 2 * off, rt->bottom - rt->top - 2 * off), corner - off);
		}
	}

	// draw icon
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		int id = (i == mButtonHover ? mButtonIconHover[i] : mButtonIcon[i]);
		if (id)
		{
			RECT* brt = &mButtonRect[i];
			HICON icon = static_cast<HICON>(::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(id), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR));
			DrawIconEx(dc->m_hDC, rt->left + ROUND(brt->left* scale), rt->top + ROUND(brt->top* scale), icon,
				ROUND((brt->right - brt->left) * scale), ROUND((brt->bottom - brt->top) * scale), 0, NULL, DI_NORMAL);
			DestroyIcon(icon);
		}
	}
}

void CNaraTimerDlg::DrawPie(Graphics * g, int r, float deg, RECT* rect, COLORREF c)
{
	if(c == -1) c = RED;
	SolidBrush brred(Color(255, GetRValue(c), GetGValue(c), GetBValue(c)));
	RECT rt;
	if (rect == NULL)
	{
		rect = &rt;
		GetClientRect(&rt);
	}
	int x = ((rect->right + rect->left) >> 1) - r;
	int y = ((rect->bottom + rect->top) >> 1) - r;
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
	Pen pg(IS_TIMER_MODE? Color(255, 128, 128, 128): Color(255, GetRValue(c), GetGValue(c), GetBValue(c)), 1);
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
		CDialogEx::OnPaint();

		CClientDC dc(this);
		dc.SetStretchBltMode(HALFTONE);
		CDC mdc;
		mdc.CreateCompatibleDC(&dc);
		CBitmap *bmpo;

		RECT crt, crt2;
		GetClientRect(&crt);
		int w_crt = crt.right - crt.left;
		int h_crt = crt.bottom - crt.top;
		int sz = MAX(w_crt, h_crt);
		float scale = 1;// (sz < 720 ? 2.f : 1.f);
		static RECT trt, trt_target;

		mIsMiniMode = (MIN(w_crt, h_crt) < 200 || w_crt > (h_crt << 1));
		BOOL resize_end = (mResizing && !LBUTTON_DOWN);
		if (resize_end) mResizing = FALSE;

		if (!mIsMiniMode || mResizing)
		{
			SetRect(&crt2, 0, 0, ROUND(w_crt * scale), ROUND(h_crt * scale));
			bmp_init(&mBmp, &dc, crt2.right, crt2.bottom);
			bmpo = mdc.SelectObject(&mBmp);

			DrawTimer(&mdc, &crt2, scale);
			dc.StretchBlt(crt.left, crt.top, w_crt, h_crt, &mdc, 0, 0, crt2.right, crt2.bottom, SRCCOPY);
		}
		else
		{
			scale = (h_crt < 256? 8.f : 4.f);
			SetRect(&crt2, 0, 0, ROUND(w_crt * scale), ROUND(h_crt * scale));
			bmp_init(&mBmp, &dc, crt2.right, crt2.bottom);
			bmpo = mdc.SelectObject(&mBmp);

			DrawTimer(&mdc, &crt2, scale, FALSE);

			if (resize_end)
			{
				CopyRect(&trt, &mTimeRect);
				CFont font;
				GetFont(font, ROUND(mRadius * scale / 3 * 1.5f), TRUE);
				CString str;
				GetWindowText(str);
				CFont* fonto = dc.SelectObject(&font);
				dc.DrawText(str, &trt, DT_SINGLELINE | DT_CALCRECT);
				dc.SelectObject(fonto);
				int dx = (mTimeRect.right - trt.right + 1) >> 1;
				int dy = (mTimeRect.bottom - trt.bottom + 1) >> 1;
				trt.left += dx;
				trt.top += dy;
				trt.right += dx;
				trt.bottom += dy;

				int wrt = trt.right - trt.left;
				int hrt = trt.bottom - trt.top;
				if (wrt * h_crt < hrt * w_crt)
				{
					int c = (trt.left + trt.right + 1) >> 1;
					int w = ROUND((float)w_crt * hrt / h_crt);
					trt.left = max(c - (w >> 1), 0);
					trt.right = min(c + (w >> 1), crt2.right);
				}
				else
				{
					int c = (trt.bottom + trt.top + 1) >> 1;
					int h = ROUND((float)h_crt * wrt / w_crt);
					trt.top = max(c - (h >> 1), 0);
					trt.bottom = min(c + (h >> 1), crt2.bottom);
				}
				CopyRect(&trt_target, &trt);
				CopyRect(&trt, &crt2);
				Invalidate(FALSE);
			}
			trt.left = (trt.left + trt_target.left + 1) >> 1;
			trt.top = (trt.top + trt_target.top + 1) >> 1;
			trt.right = (trt.right + trt_target.right + 1) >> 1;
			trt.bottom = (trt.bottom + trt_target.bottom + 1) >> 1;
			if (abs(trt.left - trt_target.left) <= 1)
			{
				CopyRect(&trt, &trt_target);
			}
			else
			{
				Invalidate(FALSE);
			}

			CDC bdc;
			bdc.CreateCompatibleDC(&dc);
			bmp_init(&mBuf, &dc, crt.right, crt.bottom);
			CBitmap * bo = bdc.SelectObject(&mBuf);
			bdc.SetStretchBltMode(HALFTONE);
			bdc.StretchBlt(crt.left, crt.top, w_crt, h_crt, &mdc, trt.left, trt.top, trt.right-trt.left, trt.bottom-trt.top, SRCCOPY);

			DrawBorder(&bdc, &crt, 1);
			dc.BitBlt(0, 0, crt.right, crt.bottom, &bdc, 0, 0, SRCCOPY);
			bdc.SelectObject(bo);
		}

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
	CDialogEx::OnNcCalcSize(bCalcValidRects, params);
}

void CNaraTimerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TID_TICK)
	{
		if (!mSetting && mTimeSet != 0)
		{
			if (GetTickCount64() + CHK_INTERVAL < mTimeSet)
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
				Stop();
			}
		}
		else if (mTimeSet == 0 && mIsMiniMode)
		{
			Invalidate(FALSE);
		}
	}
	else if (nIDEvent == TID_REFRESH)
	{
		if (!IS_TIMER_MODE)
		{
			static int s = 0;
			int cm = CTime::GetCurrentTime().GetMinute();
			int cs = CTime::GetCurrentTime().GetSecond();
			if (cs != s)
			{
				s = cs;
				Invalidate(FALSE);
			}
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}

BOOL CNaraTimerDlg::IsTitleArea(CPoint pt)
{
	if(mIsMiniMode) return FALSE;
	return (pt.x >= mTitleRect.left && pt.x < mTitleRect.right && pt.y >= mTitleRect.top && pt.y < mTitleRect.bottom);
}

void CNaraTimerDlg::SetMode(BOOL is_timer)
{
	if(is_timer)
	{
		if(IS_TIMER_MODE) return;
		mTime360 = TIMER_TIME360;
		mIsTimer = TRUE;
	}
	else
	{
		if(IS_ALARM_MODE) return;
		mTime360 = MAX_TIME360;
		mIsTimer = FALSE;
	}
	mTimeSet = 0;
	SetWindowText(L"NaraTimer");
}

void CNaraTimerDlg::SetTitle()
{
	if(!mIsMiniMode)
	{
		OnTitleChanging();
		mTitleEdit.SetSel(0, -1);
	}
}

void CNaraTimerDlg::OnLButtonDown(UINT nFlags, CPoint pt)
{
	this->SetFocus();
	mTitleEdit.ShowWindow(SW_HIDE);

	if (IsTitleArea(pt))
	{
		SetTitle();
		return;
	}

	int ht = HitTest(pt);
	if (ht != HTCLIENT)
	{
		mResizing = TRUE;
		SetArrowCursor(ht);
		SendMessage(WM_NCLBUTTONDOWN, ht, MAKELPARAM(pt.x, pt.y));
		return;
	}

	if (mTitleEdit.GetWindowTextLength() == 0)
	{
		mTitle = L"";
		mTitleHeight = 0;
		Invalidate(FALSE);
	}

	mButtonHover = -1;
	for (int i = 0; i < NUM_BUTTONS; i++)
	{
		if (PT_IN_RECT(pt, mButtonRect[i]))
		{
			mButtonHover = i;
			Invalidate(FALSE);
			SetCapture();
			CDialogEx::OnLButtonDown(nFlags, pt);
			return;
		}
	}

	RECT crt;
	GetClientRect(&crt);
	int cx = (mTimerRect.right + mTimerRect.left) >> 1;
	int cy = (mTimerRect.bottom + mTimerRect.top) >> 1;
	int d = SQ(pt.x - cx) + SQ(pt.y - cy);
	if(!mIsMiniMode && d < (mRadiusHandsHead * mRadiusHandsHead))
	{
		GetTimestamp();
		CClientDC dc(this);
		int m = mRadiusHandsHead >> 1;
		RECT rt = { crt.left + m, crt.top + m, crt.right - m, crt.bottom - m };
		DrawTimer(&dc, &rt, 1.f);
		if (mTimeSet == 0 && !CTRL_DOWN)
		{
			KillTimer(TID_TICK);
			SetMode(IS_ALARM_MODE);
		}
		else
		{
			OnNew();
		}
		Invalidate(FALSE);
	}
	else if(!mIsMiniMode && d < SQ(mRadius + mGridSize))
	{
		KillTimer(TID_TICK);
		float deg = pt2deg(pt);
		mOldDeg = deg;
		mTimeSet = deg2time(deg, TRUE);
		if (IS_ALARM_MODE)
		{
			mTime360 = MAX_TIME360;
			mIsTimer = FALSE;
		}
		mSetting = TRUE;
		Invalidate(FALSE);
		SetCapture();
	}
	else
	{
		SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
	}
	CDialogEx::OnLButtonDown(nFlags, pt);
}

void CNaraTimerDlg::OnMouseMove(UINT nFlags, CPoint pt)
{
	int cx = (mTimerRect.right + mTimerRect.left) >> 1;
	int cy = (mTimerRect.bottom + mTimerRect.top) >> 1;
	int d = SQ(pt.x - cx) + SQ(pt.y - cy);
	if(mSetting && (nFlags & MK_LBUTTON))
	{
		float deg = pt2deg(pt);
		if (deg + mDegOffset > 360) deg = 360 - mDegOffset;
		int quarter0 = (int)(mOldDeg / 90);
		int quarter1 = (int)(deg / 90);
		if (quarter0 >= 3 && quarter1 <= 1)
		{
			deg = 360 - mDegOffset;
		}
		else if (quarter0 == 0 && quarter1 >= 3)
		{
			deg = -mDegOffset;
		}
		mOldDeg = deg;
		mTimeSet = deg2time(deg, d > (mRadius + (mGridSize >> 1)) * (mRadius + (mGridSize >> 1)));
		Invalidate(FALSE);
	}
	else
	{
		if (!(nFlags & MK_LBUTTON))
		{
			if (mButtonHover >= 0 && !PT_IN_RECT(pt, mButtonRect[mButtonHover]))
			{
				mButtonHover = -1;
				Invalidate(FALSE);
			}
			for (int i = 0; i < NUM_BUTTONS; i++)
			{
				if (PT_IN_RECT(pt, mButtonRect[i]))
				{
					mButtonHover = i;
					Invalidate(FALSE);
					break;
				}
			}
		}

		if(IsTitleArea(pt))
		{
			::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_IBEAM));
		}
		else if(!mIsMiniMode && d < (mRadiusHandsHead * mRadiusHandsHead))
		{
			::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
		}
		else
		{
			int ht = HitTest(pt);
			SetArrowCursor(ht);
		}
	}
	CDialogEx::OnMouseMove(nFlags, pt);
}

void CNaraTimerDlg::OnLButtonUp(UINT nFlags, CPoint pt)
{
	if (mSetting)
	{
		ReleaseCapture();
		mSetting = FALSE;
		Invalidate(FALSE);
		KillTimer(TID_TICK);
		if (mOldDeg > -mDegOffset)
		{
			SetTimer(TID_TICK, CHK_INTERVAL, NULL);
		}
		else
		{
			Stop();
		}
		mIsTimer = TRUE;
	}
	else if (mButtonHover >= 0)
	{
		if (PT_IN_RECT(pt, mButtonRect[mButtonHover]))
		{
			if (mButtonHover == BUTTON_CLOSE)
			{
				PostMessage(WM_CLOSE, 0, 0);
			}
			else if (mButtonHover == BUTTON_PIN)
			{
				PostMessage(WM_PIN, 0, 0);
			}
		}
		mButtonHover = -1;
		Invalidate(FALSE);
		ReleaseCapture();
	}
	CDialogEx::OnLButtonUp(nFlags, pt);
}

void CNaraTimerDlg::OnContextMenu(CWnd * pWnd, CPoint pt)
{
	CMenu menu, theme;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, IDM_NEW, L"New");
	menu.AppendMenu(MF_SEPARATOR, 0, L"");
	menu.AppendMenu(MF_STRING | (IS_TIMER_MODE? MF_CHECKED: 0), IDM_TIMERMODE, L"Timer Mode");
	menu.AppendMenu(MF_STRING | (IS_ALARM_MODE? MF_CHECKED: 0), IDM_ALARMMODE, L"Alarm Mode");
	menu.AppendMenu(MF_SEPARATOR, 0, L"");
	menu.AppendMenu(MF_STRING|(mTopmost?MF_CHECKED:0), IDM_TOPMOST, L"Always On Top");
	menu.AppendMenu(MF_SEPARATOR, 0, L"");
	theme.CreatePopupMenu();
	theme.AppendMenu(MF_STRING | (mTheme == THEME_LIGHT? MF_CHECKED: 0), IDM_THEMEDEFAULT, L"Light");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_DARK? MF_CHECKED: 0), IDM_THEMEBLACK, L"Dark");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_BLUE? MF_CHECKED: 0), IDM_THEMEBLUE, L"Blue");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_GREEN? MF_CHECKED: 0), IDM_THEMEGREEN, L"Green");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_ORANGE? MF_CHECKED: 0), IDM_THEMEORANGE, L"Orange");
	theme.AppendMenu(MF_STRING | (mTheme == THEME_MINT? MF_CHECKED: 0), IDM_THEMEMINT, L"Mint");
	menu.AppendMenuW(MF_POPUP, (UINT_PTR)theme.Detach(), L"Themes");
	menu.AppendMenuW(MF_STRING | (mDigitalWatch ? MF_CHECKED : 0), IDM_TOGGLEDIGITALWATCH, L"Digital Watch");
	menu.AppendMenuW(MF_STRING | (mHasDate ? MF_CHECKED : 0), IDM_TOGGLEDATE, L"Date");
	menu.AppendMenuW(MF_STRING | (mTickSound ? MF_CHECKED : 0), IDM_TOGGLETICKSOUND, L"Ticking Sound");
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
}

void CNaraTimerDlg::OnNew(void)
{
	wchar_t path[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL), path, MAX_PATH);
	ShellExecute(GetSafeHwnd(), L"open", path, NULL, NULL, 1);
}

void CNaraTimerDlg::OnTimerMode(void)
{
	SetMode(TRUE);
}

void CNaraTimerDlg::OnAlarmMode(void)
{
	SetMode(FALSE);
}

void CNaraTimerDlg::OnMenuPin(void)
{
	SetTopmost(!mTopmost);
}

void CNaraTimerDlg::OnThemeLight(void)
{
	SetTheme(THEME_LIGHT);
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
	RECT rt;
	GetClientRect(&rt);

	bsz = ROUND(MIN(rt.right - rt.left, rt.bottom - rt.top) * 0.1f);
	r = (int)((ROUND_CORNER - RESIZE_MARGIN) / 1.414f);
	y = (rt.top + ROUND_CORNER - r);

	// pin button
	sz = max(ROUND(bsz * 1.5f), 30);
	x = (rt.left + ROUND_CORNER - r);
	mButtonRect[BUTTON_PIN].SetRect(x, y, x + sz, y + sz);
	// close button
	sz = max(ROUND(bsz * 0.8f), 16);
	x = (rt.right - ROUND_CORNER + r);
	mButtonRect[BUTTON_CLOSE].SetRect(x - sz, y, x, y + sz);
	SET_BUTTON(BUTTON_CLOSE, IDI_CLOSE, IDI_CLOSE_HOVER);
}

void CNaraTimerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	RECT wrt;
	GetWindowRect(&wrt);
	int w_wrt = wrt.right - wrt.left;
	int h_wrt = wrt.bottom - wrt.top;
	int sz = MIN(w_wrt, h_wrt);

	ROUND_CORNER = ROUND(sz / 3.5f);
	RESIZE_MARGIN = max(ROUND(sz / 14.f), 5);

	CRgn rgn;
	if (nType == SIZE_MAXIMIZED)
	{
		rgn.CreateRectRgn(0, 0, w_wrt, h_wrt);
	}
	else
	{
		rgn.CreateRoundRectRgn(0, 0, w_wrt + 1, h_wrt + 1, ROUND_CORNER * 2, ROUND_CORNER * 2);
	}
	SetWindowRgn((HRGN)rgn, FALSE);

	reposition();
	Invalidate(FALSE);
	if (mTitleHeight > 0)
	{
		mTitleHeight = GetTitleHeight();
	}

	if (mTimeSet == 0 && IS_TIMER_MODE)
	{
		if (mIsMiniMode)
		{
			SetTimer(TID_TICK, CHK_INTERVAL, NULL);
		}
		else
		{
			KillTimer(TID_TICK);
		}
	}
}

void CNaraTimerDlg::OnGetMinMaxInfo(MINMAXINFO * lpMMI)
{
	int wh = 50;
	lpMMI->ptMinTrackSize.x = wh;
	lpMMI->ptMinTrackSize.y = wh;
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CNaraTimerDlg::OnWindowPosChanged(WINDOWPOS * pos)
{
	CDialogEx::OnWindowPosChanged(pos);
	Invalidate(FALSE);
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
	if(mTitleHeight == 0)
	{
		mTitleHeight = GetTitleHeight();
		OnPaint();
		CFont font;
		GetFont(font, mTitleHeight, TRUE);
		mTitleEdit.SetFont(&font, FALSE);
	}
	mTitleEdit.MoveWindow(&mTitleRect);
	mTitleEdit.SetFocus();
	mTitleEdit.ShowWindow(SW_SHOW);
}


