#pragma once
#define MODE_TIMER			(0)
#define MODE_ALARM			(1)
#define MODE_STOPWATCH		(2)

#define STATUS_STOPPED		(0)
#define STATUS_RUNNING		(1)
#define STATUS_PAUSED		(2)

#define TIMER_TIME360		(3600000) // 1h in ms
#define MAX_TIME360			(12*3600000) // 12h

class Watch
{
public:
	Watch(void);
	~Watch(void) {};
	void Stop(void);
	void SetMode(int mode);
	LONGLONG GetRemainingTime(void);
	BOOL IsTimeSet();
	BOOL SetTime(int h, int m, int s);
	void SetText(wchar_t * fmt, ...);
	int GetMode(void);
	int GetStatus(void);
	void GetDescription(CString & str);

	int mMode;
	float mTime360;
	ULONGLONG mTimeSet;
	CString mTitle;
	CString mTimeStr;
	SIZE mHM;
	BOOL mExpired;
	BOOL mMarkerHover;

	Watch * mPrev;
	Watch * mNext;
};

class WatchList
{
public:
	WatchList(void);
	~WatchList(void);
	Watch * GetHead(void);
	Watch * GetNew(void);
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
	int mLastMode;
public:
	int mItemHeight;
	int mItemHighlighted;
};


