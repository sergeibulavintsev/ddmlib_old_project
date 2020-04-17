/*
 * ThreadInfo.hpp
 *
 *  Created on: Feb 8, 2012
 *      Author: Ilya Polenov
 */

#ifndef THREADINFO_HPP_
#define THREADINFO_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class StackTraceElement;

class DDMLIB_API ThreadInfo {
	int mThreadId;
	std::wstring mThreadName;
	int mStatus;
	int mTid;
	int mUtime;
	int mStime;
	bool mIsDaemon;
	std::vector<std::tr1::shared_ptr<StackTraceElement> > mTrace;
	long long mTraceTime;

	// priority?
	// total CPU used?
	// method at top of stack?

public:
	ThreadInfo(int threadId = -1, const std::wstring &threadName = L"");

	ThreadInfo(const ThreadInfo &other);

	virtual ~ThreadInfo();

	/**
	 * Set with the values we get from a THST chunk.
	 */
	void updateThread(int status, int tid, int utime, int stime, bool isDaemon);

	/**
	 * Sets the stack call of the thread.
	 * @param trace stackcall information.
	 */
	void setStackCall(const std::vector<std::tr1::shared_ptr<StackTraceElement> > &trace);

	/**
	 * Returns the thread's ID.
	 */
	int getThreadId() const;

	/**
	 * Returns the thread's name.
	 */
	std::wstring getThreadName() const;

	void setThreadName(const std::wstring &name);

	/**
	 * Returns the system tid.
	 */
	int getTid() const;

	/**
	 * Returns the VM thread status.
	 */
	int getStatus() const;

	/**
	 * Returns the cumulative user time.
	 */
	int getUtime() const;

	/**
	 * Returns the cumulative system time.
	 */
	int getStime() const;

	/**
	 * Returns whether this is a daemon thread.
	 */
	bool isDaemon() const;

	/*
	 * (non-Javadoc)
	 * @see com.android.ddmlib.IStackTraceInfo#getStackTrace()
	 */
	std::vector<std::tr1::shared_ptr<StackTraceElement> > getStackTrace() const;

	/**
	 * Returns the approximate time of the stacktrace data.
	 * @see #getStackTrace()
	 */
	long long getStackCallTime() const;
};

} /* namespace ddmlib */
#endif /* THREADINFO_HPP_ */
