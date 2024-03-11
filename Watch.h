#pragma once
#define TIMER_TIME360		(3600000) // 1h in ms
#define MAX_TIME360			(12*3600000) // 12h

class Watch
{
public:
	Watch(void);
	~Watch(void) {};
	void Stop(void);
	void SetMode(BOOL is_timer);
	LONGLONG GetRemainingTime(void);
	BOOL IsTimeSet();
	BOOL IsTimerMode(void);
	BOOL IsAlarmMode(void);
	BOOL SetTime(int h, int m, int s);
	void SetText(wchar_t * fmt, ...);

	BOOL mIsTimer;
	float mTime360;
	ULONGLONG mTimeSet;
	CString mTitle;
	CString mTimeStr;
	SIZE mHM;
	BOOL mExpired;

	Watch * mPrev;
	Watch * mNext;
};

class WatchList
{
public:
	WatchList(void);
	~WatchList(void);
	Watch * GetHead(void);
	Watch * GetUnset(void);
	Watch * GetWatchSet(void);
	Watch * Get(int idx);
	int GetSize(BOOL count_unset=FALSE);
	Watch * Add(void);
	Watch * Remove(Watch * watch);
	void RemoveHead(void);
	void RemoveStopped(void);
	void RemoveAll(void);
	void Activate(Watch * watch);
	void Sort(Watch * watch);
	void CleanUp(void);
protected:
	Watch * mHead;
	int mSize;
	int mLastIsTimer;
public:
	int mItemHeight;
	int mItemHighlighted;
};


