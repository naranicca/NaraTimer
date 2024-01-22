#include "pch.h"
#include "NaraWnd.h"
#include "NaraUtil.h"

BEGIN_MESSAGE_MAP(NaraWnd, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

NaraWnd::NaraWnd(CWnd * pParent)
{
	mParent = pParent;
	memset(&mNCMargin, 0, sizeof(RECT));
	memset(&mDst, 0, sizeof(RECT));
}

void NaraWnd::SetNCMargin(int left, int top, int right, int bottom)
{
	mNCMargin.left   = left;
	mNCMargin.top    = top;
	mNCMargin.right  = right;
	mNCMargin.bottom = bottom;
}

CWnd * NaraWnd::GetParent(void)
{
	return (mParent ? mParent : CWnd::GetParent());
}

BOOL NaraWnd::OnEraseBkgnd(CDC * dc)
{
	return TRUE;
}

void NaraWnd::OnSize(UINT nType, int cx, int cy)
{
	Invalidate(FALSE);
	CWnd::OnSize(nType, cx, cy);
}

BOOL NaraWnd::PreTranslateMessage(MSG * msg)
{
	static BOOL dragging = FALSE;

	if((msg->message == WM_MOUSEMOVE && (msg->wParam & MK_LBUTTON) == 0) || \
		msg->message == WM_LBUTTONDOWN || msg->message == WM_LBUTTONUP || \
		msg->message == WM_RBUTTONDOWN || msg->message == WM_RBUTTONUP || \
		msg->message == WM_LBUTTONDBLCLK || \
		msg->message == WM_MOUSEWHEEL)
	{
		CPoint pt = msg->pt;
		ScreenToClient(&pt);
		CRect crt;
		GetClientRect(&crt);
		if(PT_IN_RECT(pt, crt))
		{
			crt.left += mNCMargin.left;
			crt.top += mNCMargin.top;
			crt.right -= mNCMargin.right;
			crt.bottom -= mNCMargin.bottom;
			if(PT_IN_RECT(pt, crt))
			{
				if(msg->message == WM_LBUTTONDOWN)
				{
					dragging = TRUE;
				}
				else if(msg->message == WM_LBUTTONUP)
				{
					dragging = FALSE;
				}
			}
			else if(dragging == FALSE)
			{
				CWnd * parent = (mParent ? mParent : GetParent());
				if(parent)
				{
					pt = msg->pt;
					parent->ScreenToClient(&pt);
					parent->SendMessage(msg->message, msg->wParam, MAKELPARAM(pt.x, pt.y));
					return TRUE;
				}
			}
		}
	}
	return CWnd::PreTranslateMessage(msg);
}

void NaraWnd::OnMouseMove(UINT nFlags, CPoint pt)
{
	CWnd::OnMouseMove(nFlags, pt);
	if(GetStyle() & WS_CHILD)
	{
		if((nFlags&MK_LBUTTON) == 0 && GetActiveWindow() == AfxGetApp()->m_pMainWnd)
		{
			SetFocus();
		}
	}
	// to get mouse_leave message
	TRACKMOUSEEVENT te;
	memset(&te, 0, sizeof(TRACKMOUSEEVENT));
	te.cbSize = sizeof(TRACKMOUSEEVENT);
	te.dwFlags = TME_LEAVE;
	te.hwndTrack = m_hWnd;
	te.dwHoverTime = 0;
	::_TrackMouseEvent(&te);
}


BEGIN_MESSAGE_MAP(NaraShadow, NaraWnd)
	ON_WM_SIZE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

NaraShadow::NaraShadow(CWnd * pParent)
{
	mInited = FALSE;
	mSize = NARA_SHADOW_SIZE;
	mTime = GetTickCount();
	mRadius = 9;
}

void NaraShadow::SetCornerRadius(int r)
{
	mRadius = r;
}

void NaraShadow::Reposition(CWnd * parent, RECT * wrt, int size)
{
	if(::IsWindow(parent->m_hWnd) == FALSE) return;

	mParent = parent;
	mSize = size;

	if(mInited == FALSE)
	{
		CString str = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 0, (HBRUSH)(COLOR_WINDOW + 1));
		BOOL ret = CreateEx(WS_EX_LAYERED, str, L"Shadow", WS_POPUP,
			CRect(0, 0, 0, 0), parent, 0, NULL);
		ASSERT(ret);

		SetNCMargin(mSize, mSize, mSize, mSize);

		mInited = TRUE;
		mTime = GetTickCount();
	}
	if(::IsWindow(m_hWnd) == FALSE) return;

	memcpy(&mDst, wrt, sizeof(RECT));

	if(GetTickCount() - mTime < 30)
	{
		SetTimer(1, 30, NULL);
		return;
	}

	wrt->left -= mSize;
	wrt->top -= mSize;
	wrt->right += mSize;
	wrt->bottom += mSize;
	int cx = wrt->right - wrt->left;
	int cy = wrt->bottom - wrt->top;

	if(cx > (mSize<<1) && cy > (mSize<<1))
	{
		SetWindowPos(parent, wrt->left, wrt->top, cx, cy, SWP_SHOWWINDOW|SWP_NOACTIVATE);

		CRgn rgn, rgn0, rgn1;
		rgn0.CreateRectRgn(0, 0, cx, cy);
		rgn1.CreateRoundRectRgn(mSize + 1, mSize + 1, cx - mSize, cy - mSize, (mRadius<<1), (mRadius<<1));
		rgn.CreateRectRgn(0, 0, cx, cy);
		rgn.CombineRgn(&rgn0, &rgn1, RGN_DIFF);
		SetWindowRgn((HRGN)rgn, TRUE);
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
}

void NaraShadow::Reposition(CWnd * parent, int size)
{
	if(::IsWindow(parent->m_hWnd))
	{
		RECT wrt;
		parent->GetWindowRect(&wrt);
		Reposition(parent, &wrt, size >= 0? size: mSize);
		memset(&mDst, 0, sizeof(RECT));
	}
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

void NaraShadow::Draw(CDC * dc)
{
	Graphics g(*dc);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);
	g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	RECT crt;
	GetClientRect(&crt);
	for(int i = 0; i <= mSize; i++)
	{
		float alpha = 32.f * i * i / mSize / mSize;
		Pen pen(Color(alpha, 0, 0, 0), 3.f);
		int y = (mSize >> 1) + i;
		DrawRoundRect(&g, &pen, Rect(i, y, crt.right - i - i - 1, crt.bottom - i - y - 1), mRadius);
	}
}

void NaraShadow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	mBmp.DeleteObject();
	UpdateWindow();

	CRgn rgn, rgn0, rgn1;
	rgn0.CreateRectRgn(0, 0, cx, cy);
	rgn1.CreateRoundRectRgn(mSize + 1, mSize + 1, cx - mSize, cy - mSize, (mRadius<<1), (mRadius<<1));
	rgn.CreateRectRgn(0, 0, cx, cy);
	rgn.CombineRgn(&rgn0, &rgn1, RGN_XOR);
	SetWindowRgn((HRGN)rgn, FALSE);
}

void NaraShadow::OnTimer(UINT_PTR id)
{
	KillTimer(id);
	if(mDst.right - mDst.left == 0)
	{
		Reposition(mParent, mSize);
	}
	else
	{
		Reposition(mParent, &mDst, mSize);
	}
}

void NaraShadow::UpdateWindow(void)
{
	CDC * dc = GetDC();

	RECT wrt;
	GetWindowRect(&wrt);

	SIZE sz = { wrt.right - wrt.left, wrt.bottom - wrt.top };
	if(sz.cx > 0 && sz.cy > 0)
	{
		CDC memDC;
		memDC.CreateCompatibleDC(dc);
		memDC.SetStretchBltMode(COLORONCOLOR);

		if(mBmp.m_hObject == NULL)
		{
			mBmp.CreateCompatibleBitmap(dc, sz.cx, sz.cy);
		}
		CBitmap * bmpo = memDC.SelectObject(&mBmp);

		Draw(&memDC);

		BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
		POINT pt_dst = { wrt.left, wrt.top };
		POINT pt = { 0, 0 };
		UpdateLayeredWindow(dc, &pt_dst, &sz, &memDC, &pt, 0, &bf, ULW_ALPHA);

		memDC.SelectObject(bmpo);

		memDC.DeleteDC();
	}
	ReleaseDC(dc);
}


