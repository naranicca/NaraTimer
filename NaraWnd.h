#pragma once

class NaraWnd : public CWnd
{
public:
	NaraWnd(CWnd * pParent = NULL);

	// set non-client region
	void SetNCMargin(int left, int top, int right, int bottom);

	// overrides
	virtual CWnd * GetParent();

protected:
	CWnd  * mParent;
	RECT    mNCMargin;
	RECT    mDst;

protected: // message handlers
	virtual BOOL PreTranslateMessage(MSG * msg);
	afx_msg BOOL OnEraseBkgnd(CDC * dc);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
	DECLARE_MESSAGE_MAP()
};

// shadow size
#define NARA_SHADOW_SIZE        24
class NaraShadow : public NaraWnd
{
public:
	NaraShadow(CWnd * pParent = NULL);
	void SetCornerRadius(int r);
	void Reposition(CWnd * parent, int size = NARA_SHADOW_SIZE);
	void Reposition(CWnd * parent, RECT * wrt, int size = NARA_SHADOW_SIZE);

protected:
	BOOL                mInited;
	int                 mSize;
	DWORD               mTime;
	int                 mRadius;
	CBitmap             mBmp;

protected:
	void Draw(CDC * dc);
	void UpdateWindow(void);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR id);
	DECLARE_MESSAGE_MAP()
};

