
// NaraTimerDlg.h: 헤더 파일
//

#pragma once

#define BUTTON_CLOSE		(0)
#define BUTTON_PIN			(1)
#define NUM_BUTTONS			(2)

// CNaraTimerDlg 대화 상자
class CNaraTimerDlg : public CDialogEx
{
// 생성입니다.
public:
	CNaraTimerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	void Stop(void);
	void SetTitle();
	void SetTopmost(BOOL topmost=TRUE);

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NARATIMER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;
	CBitmap mBmp;
	CBitmap mBuf;
	ULONGLONG mTimeSet;
	int mRadius;
	int mRadiusCenterLock;
	BOOL mSetting;
	ULONGLONG mTso;
	float mOldDeg;
	BOOL mIsTimer;
	float mTime360;
	CString mTimeStr;
	int mGridSize;
	LARGE_INTEGER mTimestamp;
	SIZE mHM;
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

	void reposition(void);
	int HitTest(CPoint pt);
	void SetArrowCursor(int hittest);
	POINT deg2pt(float deg, int r);
	float pt2deg(CPoint pt);
	ULONGLONG deg2time(float deg, BOOL stick = FALSE);
	void DrawTimer(CDC * dc, RECT * rt, float scale=1.f, RECT * crop_rect=NULL);
	void DrawPie(CDC* cd, int r, float deg, RECT* rect = NULL, COLORREF c=-1);
	void DrawBorder(CDC * dc, RECT * rt, float scale);
	ULONGLONG GetTimestamp(void);
	void GetFont(CFont& font, int height, BOOL bold = FALSE);
	int GetTitleHeight(void);
	BOOL IsTitleArea(CPoint pt);

	// 생성된 메시지 맵 함수
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
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnWindowPosChanged(WINDOWPOS* pos);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnPinToggle(WPARAM wParam, LPARAM lparam);
	DECLARE_MESSAGE_MAP()
};


