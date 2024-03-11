#include "pch.h"
#include "Watch.h"

///////////////////////////////////////////////////////////////////////////////
// Watch
Watch::Watch(void)
{
	mTimeSet = 0;
	mIsTimer = FALSE;
	mTime360 = MAX_TIME360;
	mExpired = FALSE;
	mPrev = NULL;
	mNext = NULL;
}

void Watch::Stop(void)
{
	mTimeSet = 0;
	mTimeStr = L"";
	if(IsAlarmMode())
	{
		mIsTimer = FALSE;
		mTime360 = MAX_TIME360;
	}
	else
	{
		mIsTimer = TRUE;
		mTime360 = TIMER_TIME360;
	}
	mHM = { 0, };
	mExpired = FALSE;
}

void Watch::SetMode(BOOL is_timer)
{
	Stop();
	if(is_timer == IsTimerMode())
	{
		return;
	}
	if(is_timer)
	{
		mTime360 = TIMER_TIME360;
		mIsTimer = TRUE;
	}
	else
	{
		mTime360 = MAX_TIME360;
		mIsTimer = FALSE;
	}
}

inline BOOL Watch::IsTimeSet(void)
{
	return (mTimeSet > 0);
}

inline BOOL Watch::IsTimerMode(void)
{
	return (mTime360 == TIMER_TIME360);
}

inline BOOL Watch::IsAlarmMode(void)
{
	return !IsTimerMode();
}

LONGLONG Watch::GetRemainingTime(void)
{
	return (IsTimeSet() ? mTimeSet - GetTickCount64() : 0);
}

BOOL Watch::SetTime(int h, int m, int s)
{
	ULONGLONG tcur = (GetTickCount64() / 1000) * 1000;
	if(IsAlarmMode())
	{
		CTime c = CTime::GetCurrentTime();
		int dh = (h - c.GetHour()) * 3600;
		int dm = (m - c.GetMinute()) * 60;
		int ds = (s - c.GetSecond());
		if(dh + dm + ds >= 0)
		{
			mTimeSet = tcur + (dh + dm + ds) * 1000;
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

///////////////////////////////////////////////////////////////////////////////
// WatchList
WatchList::WatchList(void)
{
	mHead = NULL;
	mSize = 0;
	mItemHeight = 0;
	mItemHighlighted = 0;
	mLastIsTimer = FALSE;
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

Watch * WatchList::GetUnset(void)
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
	if(count_unset == FALSE && mHead->IsTimeSet() == FALSE)
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
			watch->mIsTimer = mHead->mIsTimer;
			watch->mTime360 = mHead->mTime360;
		}
		else
		{
			watch->SetMode(mLastIsTimer);
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
				mLastIsTimer = mHead->mIsTimer;
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
		mLastIsTimer = mHead->mIsTimer;
		Remove(mHead);
	}
}

void WatchList::RemoveStopped(void)
{
	mLastIsTimer = mHead->mIsTimer;
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
	mLastIsTimer = mHead->mIsTimer;
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
		mLastIsTimer = mHead->mIsTimer;
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


