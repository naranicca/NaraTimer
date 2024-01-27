#pragma once

/* simple math ***************************************************************/
#define abs(v)                  (((v)>0)? (v) : -(v))
#define abs32(v)                (((v)^((v)>>31)) - ((v)>>31))
#define abs16(v)                (((v)^((v)>>15)) - ((v)>>15))
#define ALIGN2(v)               (((v)+1) & 0xFFFFFFFE)
#define ALIGN4(v)               (((v)+3) & 0xFFFFFFFC)
#define ALIGN8(v)               (((v)+7) & 0xFFFFFFF8)
#define SWAP(a, b)              {(a)^=(b); (b)^=(a); (a)^=(b);}
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#define CLIP(x, min, max)       ((x)<(min)?(min):(x)>(max)?(max):(x))
#define ROUND(a)                (int)((a) + 0.5f)
#define SQ(x)                   ((x) * (x))

/* flags *********************************************************************/
#define FLAG_SET(flags, v)		(flags) = (flags) | (v)
#define FLAG_CLR(flags, v)		(flags) = (flags) & (~(v))
#define FLAG_IS_SET(flags, v)	(((flags) & (v)) != 0)
#define FLAG_IS_CLR(flags, v)	(((flags) & (v)) == 0)
#define FLAG_SET_IF(c, f, v)    { if(c) FLAG_SET(f, v); else FLAG_CLR(f, v); }

/* range *********************************************************************/
#if 0
#define IN_RANGE(x, min, max)       (x >= min && x <= max)
#define NOT_IN_RANGE(x, min, max)   !IN_RANGE(x, min, max)
#else
#define NOT_IN_RANGE(x, min, max)   ((((x) - (min)) | (max) - (x)) >> 31)
#define IN_RANGE(x, min, max)       ~NOT_IN_RANGE(x, min, max)
#define NOT_IN_RANGE4(x, y, left, top, right, bottom) \
	((((x)-(left)) | ((right)-(x)) | ((y)-(top)) | ((bottom)-(y))) >> 31)
#define IN_RANGE4(x, y, left, top, right, bottom) \
	~NOT_IN_RANGE4(x, y, left, top, right, bottom)
#endif
#if 0
#define IN_RECT(pt, rt)         ((pt).x>(rt).left && (pt).x<(rt).right && (pt).y>(rt).top && (pt).y<(rt).bottom)
#define NOT_IN_REC(pt, rt)      !IN_RECT(pt, rt)
#else
#define PT_IN_RECT(pt, rt) \
	IN_RANGE4((pt).x, (pt).y, (rt).left, (rt).top, (rt).right, (rt).bottom)
#define PT_NOT_IN_RECT(pt, rt) \
	NOT_IN_RANGE4((pt).x, (pt).y, (rt).left, (rt).top, (rt).right, (rt).bottom)
#endif

/* min/max *******************************************************************/
#define MAX_INT16       ((int16)0x7FFF)
#define MIN_INT16       ((int16)0x8000)
#define MAX_UINT16      ((uint16)0xFFFF)
#define MIN_UINT16      ((uint16)0)
#define MAX_INT32       ((int32)0x7FFFFFFF)
#define MIN_INT32       ((int32)0x80000000)
#define MAX_UINT32      ((uint32)0xFFFFFFFF)
#define MIN_UINT32      ((uint32)0)

/* min/max *******************************************************************/
inline void PrepareMouseLeave(HWND hwnd)
{
	TRACKMOUSEEVENT te;
	memset(&te, 0, sizeof(TRACKMOUSEEVENT));
	te.cbSize = sizeof(TRACKMOUSEEVENT);
	te.dwFlags = TME_LEAVE;
	te.hwndTrack = hwnd;
	te.dwHoverTime = 0;
	::_TrackMouseEvent(&te);
}

inline void SetArrowCursor(int hittest)
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


