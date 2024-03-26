#include "pch.h"
#include "Watch.h"

///////////////////////////////////////////////////////////////////////////////
// Watch
Watch::Watch(void)
{
	mTimeSet = 0;
	SetMode(MODE_TIMER);
	mExpired = FALSE;
	mPrev = NULL;
	mNext = NULL;
}

void Watch::Stop(void)
{
	if(GetMode() != MODE_STOPWATCH)
	{
		mTitle = L"";
		mTimeSet = 0;
		mTimeStr = L"";
		mHM = { 0, };
		mExpired = FALSE;
	}
	else
	{
		mTitle = L"";
		mTimeStr = L"";
		mTimeSet = (GetTickCount64() - mTimeSet);
		mHM = { 0, };
		mExpired = TRUE;
	}
}

void Watch::SetMode(int mode)
{
	Stop();
	if(mode == MODE_TIMER)
	{
		mTime360 = TIMER_TIME360;
		mMode = MODE_TIMER;
	}
	else if(mode == MODE_ALARM)
	{
		mTime360 = MAX_TIME360;
		mMode = MODE_ALARM;
	}
	else //if(mode == MODE_STOPWATCH)
	{
		mTime360 = 1;
		mMode = MODE_STOPWATCH;
		mTimeSet = 0;
		mExpired = FALSE;
	}
}

inline BOOL Watch::IsTimeSet(void)
{
	return (mTimeSet > 0);
}

inline BOOL Watch::GetMode(void)
{
	return mMode;
}

int Watch::GetStatus(void)
{
	if(IsTimeSet() == FALSE)
	{
		/* not started yet */
		return STATUS_STOPPED;
	}
	else if(mExpired)
	{
		return STATUS_PAUSED;
	}
	else
	{
		return STATUS_RUNNING;
	}
}

LONGLONG Watch::GetRemainingTime(void)
{
	if(GetMode() == MODE_STOPWATCH)
	{
		return INT_MAX;
	}
	else
	{
		return (IsTimeSet() ? mTimeSet - GetTickCount64() : 0);
	}
}

BOOL Watch::SetTime(int h, int m, int s)
{
	ULONGLONG tcur = (GetTickCount64() / 1000) * 1000;
	if(GetMode() == MODE_ALARM)
	{
		ULONGLONG t = GetTickCount64();
		SYSTEMTIME c;
		GetLocalTime(&c);
		int dh = (h - c.wHour) * 3600000;
		int dm = (m - c.wMinute) * 60000;
		int ds = (s - c.wSecond) * 1000;
		int dt = dh + dm + ds;
		if(dt >= 0)
		{
			mTimeSet = t + dt - c.wMilliseconds;
			mHM.cx = h;
			mHM.cy = m;
			return TRUE;
		}
	}
	else
	{
		ULONGLONG t = (h * 3600 + m * 60 + s) * 1000;
		if(t <= TIMER_TIME360)
		{
			mTimeSet = tcur + (h * 3600 + m * 60 + s) * 1000;
			mHM.cx = m;
			mHM.cy = s;
			return TRUE;
		}
	}
	return FALSE;
}

void Watch::SetText(wchar_t * fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	mTimeStr.FormatV(fmt, list);
	va_end(list);
}

void Watch::GetDescription(CString & str)
{
	if(GetMode() == MODE_ALARM)
	{
		int h = (mHM.cx < 24 ? mHM.cx : mHM.cx - 24);
		str.Format(L"Alarm: %d:%02d", h, mHM.cy);
	}
	else if(GetMode() == MODE_TIMER)
	{
		int m = mHM.cx;
		int s = mHM.cy;
		if(m == 0)
		{
			str.Format(L"Timer: %d sec", mHM.cy);
		}
		else if(s == 0)
		{
			str.Format(L"Timer: %d min", mHM.cx);
		}
		else
		{
			str.Format(L"Timer: %d:%02d", mHM.cx, mHM.cy);
		}
	}
	else
	{
		str = L"Stopwatch";
	}
}

///////////////////////////////////////////////////////////////////////////////
// WatchList
WatchList::WatchList(void)
{
	mHead = NULL;
	mSize = 0;
	mItemHeight = 0;
	mItemHighlighted = 0;
	mLastMode = MODE_ALARM;
}

WatchList::~WatchList(void)
{
	while(mHead)
	{
		Watch * next = mHead->mNext;
		delete mHead;
		mHead = next;
		mSize--;
	}
}

Watch * WatchList::GetHead(void)
{
	if(mHead == NULL)
	{
		Add();
	}
	return mHead;
}

Watch * WatchList::GetNew(void)
{
	if(mHead == NULL || mHead->IsTimeSet())
	{
		Add();
	}
	return mHead;
}

Watch * WatchList::GetWatchSet(void)
{
	return (mHead->IsTimeSet() ? mHead : mHead->mNext);
}

Watch * WatchList::Get(int idx)
{
	Watch * cur = mHead;
	if(cur->IsTimeSet() == FALSE)
	{
		cur = cur->mNext;
	}
	while(cur)
	{
		if(idx == 0)
		{
			return cur;
		}
		idx--;
		cur = cur->mNext;
	}
	return NULL;
}

int WatchList::GetSize(BOOL count_unset)
{
	if(count_unset == FALSE && mHead && mHead->IsTimeSet() == FALSE)
	{
		return mSize - 1;
	}
	return mSize;
}

Watch * WatchList::Add(void)
{
	if(mHead && !mHead->IsTimeSet())
	{
		/* we already have an unset watch */
		return mHead;
	}
	Watch * watch = new Watch();
	if(watch)
	{
		if(mHead)
		{
			watch->mNext = mHead;
			mHead->mPrev = watch;
			watch->mMode = mHead->mMode;
			watch->mTime360 = mHead->mTime360;
		}
		else
		{
			watch->SetMode(mLastMode);
		}
		mHead = watch;
		mSize++;
	}
	return watch;
}

Watch * WatchList::Remove(Watch * watch)
{
	if(watch == NULL) return NULL;
 	Watch * cur = mHead;
	while(cur)
	{
		Watch * next = cur->mNext;
		if(cur == watch)
		{
			if(cur->mPrev)
			{
				cur->mPrev->mNext = cur->mNext;
			}
			if(cur->mNext)
			{
				cur->mNext->mPrev = cur->mPrev;
			}
			if(watch == mHead)
			{
				mLastMode = mHead->mMode;
				mHead = cur->mNext;
			}
			delete cur;
			mSize--;
			return next;
		}
		cur = next;
	}
}

void WatchList::RemoveHead(void)
{
	if(mHead)
	{
		mLastMode = mHead->mMode;
		Remove(mHead);
	}
}

void WatchList::RemoveStopped(void)
{
	mLastMode = mHead->mMode;
	Watch * cur = mHead;
	while(cur)
	{
		if(cur->IsTimeSet() == FALSE)
		{
			cur = Remove(cur);
		}
		else
		{
			cur = cur->mNext;
		}
	}
}

void WatchList::RemoveAll(void)
{
	mLastMode = mHead->mMode;
	while(mHead)
	{
		RemoveHead();
	}
	mHead = NULL;
	mSize = 0;
}

void WatchList::Activate(Watch * watch)
{
	if(watch == NULL || watch == mHead) return;
	if(mHead->IsTimeSet() == FALSE)
	{
		RemoveHead();
	}
	else
	{
		Sort(mHead);
	}
	Watch * cur = mHead;
	while(cur)
	{
		if(cur == watch && watch != mHead)
		{
			if(watch->mPrev) watch->mPrev->mNext = watch->mNext;
			if(watch->mNext) watch->mNext->mPrev = watch->mPrev;
			watch->mPrev = NULL;
			watch->mNext = mHead;
			mHead = watch;
			break;
		}
		cur = cur->mNext;
	}
	if(mHead)
	{
		mLastMode = mHead->mMode;
	}
}

void WatchList::Sort(Watch * watch)
{
	Watch * cur = GetHead();
	while(cur)
	{
		Watch * next = cur->mNext;
		if(next && cur->IsTimeSet() && cur->mTimeSet > next->mTimeSet)
		{
			Watch * beg = cur->mPrev;
			Watch * end = next->mNext;
			if(beg)
			{
				beg->mNext = next;
			}
			else
			{
				mHead = next;
			}
			next->mPrev = beg;
			next->mNext = cur;
			cur->mPrev = next;
			cur->mNext = end;
			if(end)
			{
				end->mPrev = cur;
			}
		}
		cur = next;
	}
}

void WatchList::CleanUp(void)
{
	Watch * watch = GetHead();
	while(watch)
	{
		Watch * next = watch->mNext;
		if(watch->mTimeSet && watch->GetRemainingTime() < 0)
		{
			Remove(watch);
		}
		watch = next;
	}
}


