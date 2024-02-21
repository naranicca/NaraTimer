#pragma once
#include "NaraWnd.h"

#define BUTTON_CLOSE		(0)
#define BUTTON_PIN			(1)
#define BUTTON_CENTER		(2)
#define NUM_BUTTONS			(3)

#define THEME_DEFAULT		(0)
#define THEME_DARK			(1)
#define THEME_BLUE			(2)
#define THEME_GREEN			(3)
#define THEME_ORANGE		(4)
#define THEME_MINT			(5)
#define THEME_PINK			(6)
#define NUM_THEMES			(7)

class Watch
{
public:
	Watch();
	~Watch() {};
	void Stop();
	void SetMode(BOOL is_timer);
	LONGLONG GetRemainingTime();
	BOOL IsTimeSet();
	BOOL IsTimerMode(void);
	BOOL IsAlarmMode(void);
	void SetText(wchar_t * fmt, ...);

	BOOL mIsTimer;
	float mTime360;
	ULONGLONG mTimeSet;
	CString mTimeStr;
	SIZE mHM;
};

class CNaraTimerDlg : public NaraDialog
{
public:
	CNaraTimerDlg(CWnd* pParent = nullptr);
	void Stop(void);
	void SetMode(BOOL is_timer = TRUE);
	void SetTitle(CString str, BOOL still_editing=FALSE);
	void SetTopmost(BOOL topmost=TRUE);
	void SetTheme(int theme);
	void PlayTickSound(void);

	Watch mWatch;

	BOOL mRunning;
	CWinThread * mThread;
	int mMuteTick;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NARATIMER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;
	CBitmap mBmp;
	CBitmap mBuf;
	int mTheme;
	int mDigitalWatch;
	int mTickSound;
	int mHasDate;
	int mRadius;
	int mRadiusHandsHead;
	BOOL mSetting;
	ULONGLONG mTso;
	float mOldDeg;
	int mGridSize;
	LARGE_INTEGER mTimestamp;
	BOOL mTopmost;
	CString mTitle;
	CEdit mTitleEdit;
	int mTitleHeight;
	RECT mTitleRect;
	RECT mTimerRect;
	RECT mTimeRect;
	float mDegOffset;
	int mButtonHover;
	CRect mButtonRect[NUM_BUTTONS];
	int mButtonIcon[NUM_BUTTONS];
	int mButtonIconHover[NUM_BUTTONS];
	BOOL mIsMiniMode;
	BOOL mResizing;
	int mInstructionIdx;
	CString mVersion;
	int mFontScale;

	void reposition(void);
	POINT deg2pt(float deg, int r);
	float pt2deg(CPoint pt);
	ULONGLONG deg2time(float deg, BOOL stick = FALSE);
	void SetTitle();
	void DrawTimer(CDC* dc, RECT* rt, float scale = 1.f);
	void DrawPie(Graphics * g, int r, float deg, RECT* rect = NULL, COLORREF c=-1);
	void DrawBorder(CDC * dc);
	ULONGLONG GetTimestamp(void);
	int GetTitleHeight(void);
	BOOL IsTitleArea(CPoint pt);

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnEraseBkgnd(CDC * pDC);
	afx_msg void OnPaint();
	afx_msg void OnNcPaint();
	afx_msg BOOL OnNcActivate(BOOL bActivate);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS * params);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd * pWnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnWindowPosChanged(WINDOWPOS* pos);
	afx_msg LRESULT OnPinToggle(WPARAM wParam, LPARAM lparam);
	afx_msg void OnTitleChanging();
	afx_msg void OnNew();
	afx_msg void OnStop();
	afx_msg void OnTimerMode();
	afx_msg void OnAlarmMode();
	afx_msg void OnMenuPin();
	afx_msg void OnMenuFont();
	afx_msg void OnMenuHelp();
	afx_msg void OnMenuAbout();
	afx_msg void OnThemeDefault();
	afx_msg void OnThemeDark();
	afx_msg void OnThemeBlue();
	afx_msg void OnThemeGreen();
	afx_msg void OnThemeOrange();
	afx_msg void OnThemeMint();
	afx_msg void OnThemePink();
	afx_msg void OnToggleDigitalWatch();
	afx_msg void OnToggleDate();
	afx_msg void OnToggleTickSound();
	DECLARE_MESSAGE_MAP()
};


