/*
 * ThreadInfo.cpp
 *
 *  Created on: Feb 8, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "ThreadInfo.hpp"
#include "StackTraceElement.hpp"

namespace ddmlib {

ThreadInfo::~ThreadInfo() {
}

void ThreadInfo::updateThread(int status, int tid, int utime, int stime, bool isDaemon) {
	mStatus = status;
	mTid = tid;
	mUtime = utime;
	mStime = stime;
	mIsDaemon = isDaemon;
}

void ThreadInfo::setStackCall(const std::vector<std::tr1::shared_ptr<StackTraceElement> > &trace) {
	mTrace = trace;
	mTraceTime = Poco::Timestamp().epochMicroseconds() / 1000; // current time in milliseconds
}

ThreadInfo::ThreadInfo(int threadId, const std::wstring &threadName) :
		mThreadId(threadId), mThreadName(threadName), mStatus(-1), mTid(0), mUtime(0), mStime(0), mIsDaemon(false), mTraceTime(0) {
}

ThreadInfo::ThreadInfo(const ThreadInfo &other) :
	mThreadId(other.mThreadId), mThreadName(other.mThreadName), mStatus(other.mStatus),
	mTid(other.mTid), mUtime(other.mUtime), mStime(other.mStime), mIsDaemon(other.mIsDaemon), mTraceTime(other.mTraceTime)  {
	for (std::vector<std::tr1::shared_ptr<StackTraceElement> >::const_iterator stackTraceElement = other.mTrace.begin(); stackTraceElement != other.mTrace.end(); ++stackTraceElement) {
		mTrace.push_back(*stackTraceElement);
	}
}

int ThreadInfo::getThreadId() const {
	return mThreadId;
}

std::wstring ThreadInfo::getThreadName() const {
	return mThreadName;
}

void ThreadInfo::setThreadName(const std::wstring &name) {
	mThreadName = name;
}

int ThreadInfo::getTid() const {
	return mTid;
}

int ThreadInfo::getStatus() const {
	return mStatus;
}

int ThreadInfo::getUtime() const {
	return mUtime;
}

int ThreadInfo::getStime() const {
	return mStime;
}

bool ThreadInfo::isDaemon() const {
	return mIsDaemon;
}

std::vector<std::tr1::shared_ptr<StackTraceElement> > ThreadInfo::getStackTrace() const {
	return mTrace;
}

long long ThreadInfo::getStackCallTime() const {
	return mTraceTime;
}

} /* namespace ddmlib */
