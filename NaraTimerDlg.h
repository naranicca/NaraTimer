#pragma once
#include "NaraWnd.h"
#include "Watch.h"

#define BUTTON_CLOSE		(0)
#define BUTTON_PIN			(1)
#define BUTTON_CENTER		(2)
#define BUTTON_REMOVE		(3)
#define BUTTON_BAR			(4)
#define BUTTON_STOP			(5)
#define NUM_BUTTONS			(6)

#define THEME_DEFAULT		(0)
#define THEME_DARK			(1)
#define THEME_RED			(2)
#define THEME_BLUE			(3)
#define THEME_NAVY 			(4)
#define THEME_GREEN 		(5)
#define THEME_BLACK			(6)
#define NUM_THEMES			(7)

class CNaraTimerDlg : public NaraDialog
{
public:
	CNaraTimerDlg(CWnd* pParent = nullptr);
	void Stop(void);
	void SetView(int view);
	void SetTitle(CString str, BOOL still_editing=FALSE);
	void SetTopmost(BOOL topmost=TRUE);
	void SetTheme(int theme);
	void PlayTickSound(void);

	WatchList mWatches;

	BOOL mRunning;
	CWinThread * mThreadTick;
	CWinThread * mThreadCheckVersion;
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
	int mView;
	int mTheme;
	int mDigitalWatch;
	int mTickSound;
	int mHasDate;
	int mRadius;
	int mRadiusHandsHead;
	Watch * mSetting;
	Watch * mLastWatch;
	Watch * mTitleEditingWatch;
	ULONGLONG mTso;
	float mOldDeg;
	int mGridSize;
	LARGE_INTEGER mTimestamp;
	BOOL mTopmost;
	CEdit mTitleEdit;
	int mTitleHeight;
	RECT mTimerRect;
	RECT mTimeRect;
	float mDegOffset;
	int mButtonHover;
	CRect mButtonRect[NUM_BUTTONS];
	int mButtonIcon[NUM_BUTTONS];
	int mButtonIconHover[NUM_BUTTONS];
	int mInstructionIdx;
	CString mVersion;
	int mFontScale;
	int mBarAlpha;
	ULONGLONG mTimeClick;
	int mTickInterval;
	POINT mHandCoord[2];

	void reposition(void);
	POINT deg2pt(float deg, int r);
	float pt2deg(CPoint pt);
	Watch * SettingTime(float deg, BOOL stick = FALSE);
	void SetTitle();
	void Draw(RECT * rt);
	void DrawSlide(BOOL swipe_down);
	void DrawTimer(CDC * dc, Watch * watch, RECT * rt, BOOL list_mode=FALSE);
	void DrawStopwatch(CDC * dc, Watch * watch, RECT * rt);
	void DrawList(CDC * dc, RECT * rt);
	void DrawPie(Graphics * g, Watch * watch, int x, int y, int r, float deg, COLORREF c=-1, int opaque=255);
	void DrawBar(CDC * dc, RECT * rt);
	void DrawBorder(CDC * dc);
	void DrawHUD(CDC * dc, CString str);
	void DrawLoadingBar(CDC * dc);
	ULONGLONG GetTimestamp(void);
	void StopTimesUp(void);
	void StopwatchStart(Watch * watch);
	void StopwatchPause(Watch * watch);
	BOOL PtOnHand(CPoint pt);

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
	afx_msg void OnMouseLeave(void);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
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
	afx_msg void OnThemeRed();
	afx_msg void OnThemeBlue();
	afx_msg void OnThemeNavy();
	afx_msg void OnThemeGreen();
	afx_msg void OnThemeBlack();
	afx_msg void OnToggleDigitalWatch();
	afx_msg void OnToggleDate();
	afx_msg void OnToggleTickSound();
	DECLARE_MESSAGE_MAP()
};


