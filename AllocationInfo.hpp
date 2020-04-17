/*
 * AllocationInfo.hpp
 *
 *  Created on: Feb 17, 2012
 *      Author: Ilya Polenov
 */

#ifndef ALLOCATIONINFO_HPP_
#define ALLOCATIONINFO_HPP_

#include "ddmlib.hpp"
#include "IStackTraceInfo.hpp"

namespace ddmlib {

class DDMLIB_API AllocationInfo: public IStackTraceInfo {
	int mAllocNumber;
	std::wstring mAllocatedClass;
	int mAllocationSize;
	short mThreadId;
	std::vector<std::tr1::shared_ptr<StackTraceElement> > mStackTrace;

public:

	/*
	 * Simple constructor.
	 */
	AllocationInfo(int allocNumber, std::wstring allocatedClass, int allocationSize, short threadId,
			const std::vector<std::tr1::shared_ptr<StackTraceElement> > &stackTrace) :
			mAllocNumber(allocNumber), mAllocatedClass(allocatedClass), mAllocationSize(allocationSize), mThreadId(threadId), mStackTrace(
					stackTrace) {
	}

	AllocationInfo(const AllocationInfo &other) :
		mAllocNumber(other.mAllocNumber), mAllocatedClass(other.mAllocatedClass), mAllocationSize(other.mAllocationSize), mThreadId(other.mThreadId)  {
		for (std::vector<std::tr1::shared_ptr<StackTraceElement> >::const_iterator stackTraceElement = other.mStackTrace.begin(); stackTraceElement != other.mStackTrace.end();
				++stackTraceElement) {
			mStackTrace.push_back(*stackTraceElement);
		}
	}

	virtual ~AllocationInfo() {
	}

	/**
	 * Returns the allocation number. Allocations are numbered as they happen with the most
	 * recent one having the highest number
	 */
	int getAllocNumber() {
		return mAllocNumber;
	}

	/**
	 * Returns the name of the allocated class.
	 */
	std::wstring getAllocatedClass() {
		return mAllocatedClass;
	}

	/**
	 * Returns the size of the allocation.
	 */
	int getSize() {
		return mAllocationSize;
	}

	/**
	 * Returns the id of the thread that performed the allocation.
	 */
	short getThreadId() {
		return mThreadId;
	}

	/*
	 * (non-Javadoc)
	 * @see com.android.ddmlib.IStackTraceInfo#getStackTrace()
	 */
	std::vector<std::tr1::shared_ptr<StackTraceElement> > getStackTrace() {
		return mStackTrace;
	}

	std::wstring getFirstTraceClassName() {
		if (mStackTrace.size() > 0) {
			return mStackTrace[0]->getClassName();
		}
		return L"";
	}

	std::wstring getFirstTraceMethodName() {
		if (mStackTrace.size() > 0) {
			return mStackTrace[0]->getMethodName();
		}

		return L"";
	}

	bool filter(std::wstring filter, bool fullTrace) {
		if (Poco::toLower(mAllocatedClass).find(filter) != std::wstring::npos) {
			return true;
		}

		if (mStackTrace.size() > 0) {
			// check the top of the stack trace always
			int length = fullTrace ? mStackTrace.size() : 1;

			for (int i = 0; i < length; i++) {
				if (Poco::toLower(mStackTrace[i]->getClassName()).find(filter) != std::string::npos) {
					return true;
				}

				if (Poco::toLower(mStackTrace[i]->getMethodName()).find(filter) != std::string::npos) {
					return true;
				}
			}
		}

		return false;
	}
};

} /* namespace ddmlib */
#endif /* ALLOCATIONINFO_HPP_ */
