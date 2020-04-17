/*
 * NativeAllocationInfo.cpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "NativeAllocationInfo.hpp"
#include "NativeStackCallInfo.hpp"

namespace ddmlib {

std::string NativeAllocationInfo::END_STACKTRACE_KW("EndStacktrace");
std::string NativeAllocationInfo::BEGIN_STACKTRACE_KW("BeginStacktrace:");
std::string NativeAllocationInfo::TOTAL_SIZE_KW("TotalSize:");
std::string NativeAllocationInfo::SIZE_KW("Size:");
std::string NativeAllocationInfo::ALLOCATIONS_KW("Allocations:");

std::string NativeAllocationInfo::FILTERED_LIBRARIES[NUM_OF_FILT_LIBS] =
		{ std::string("libc.so"), std::string("libc_malloc_debug_leak.so") };

std::string NativeAllocationInfo::FILTERED_METHOD_NAME_PATTERNS[NUM_OF_FILT_PATTERNS]= { std::string("malloc"), std::string("calloc"),
		std::string("realloc"), std::string("operator new"), std::string("memalign") };

NativeAllocationInfo::NativeAllocationInfo(int size, int allocations) {
	mIsStackCallResolved = false;
	mSize = size & ~FLAG_MASK;
	mIsZygoteChild = ((size & FLAG_ZYGOTE_CHILD) != 0);
	mAllocations = allocations;
}

NativeAllocationInfo::NativeAllocationInfo(const NativeAllocationInfo &other) : mLock() {
	mSize = other.mSize;
	mIsZygoteChild = other.mIsZygoteChild;
	mAllocations = other.mAllocations;
	mStackCallAddresses = other.mStackCallAddresses;
	mResolvedStackCall = other.mResolvedStackCall;
	mIsStackCallResolved = other.mIsStackCallResolved;
}

NativeAllocationInfo & NativeAllocationInfo::operator =(const NativeAllocationInfo & other) {
	mSize = other.mSize;
	mIsZygoteChild = other.mIsZygoteChild;
	mAllocations = other.mAllocations;
	mStackCallAddresses = other.mStackCallAddresses;
	mResolvedStackCall = other.mResolvedStackCall;
	mIsStackCallResolved = other.mIsStackCallResolved;
	return *this;
}

NativeAllocationInfo::~NativeAllocationInfo() {
}

int NativeAllocationInfo::getAllocationCount() const {
	return mAllocations;
}

bool NativeAllocationInfo::isZygoteChild() const {
	return mIsZygoteChild;
}

int NativeAllocationInfo::getSize() const {
	return mSize;
}

std::vector<std::tr1::shared_ptr<NativeStackCallInfo> > NativeAllocationInfo::getResolvedStackCall() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mResolvedStackCall;
}

void NativeAllocationInfo::setResolvedStackCall(const std::vector<std::tr1::shared_ptr<NativeStackCallInfo> > &resolvedStackCall) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mResolvedStackCall = resolvedStackCall;
	mIsStackCallResolved = (!mResolvedStackCall.empty());
}

std::vector<long long> NativeAllocationInfo::getStackCallAddresses() const {
	return mStackCallAddresses;
}

bool NativeAllocationInfo::isStackCallResolved() const {
	return mIsStackCallResolved;
}

void NativeAllocationInfo::addStackCallAddress(long long address) {
	mStackCallAddresses.push_back(address);
}

std::string NativeAllocationInfo::toString() {
	std::string buffer;
	buffer.append(ALLOCATIONS_KW);
	buffer.append(" ");
	buffer.append(Poco::NumberFormatter::format(mAllocations));
	buffer.append("\n");

	buffer.append(SIZE_KW);
	buffer.append(" ");
	buffer.append(Poco::NumberFormatter::format(mSize));
	buffer.append("\n");

	buffer.append(TOTAL_SIZE_KW);
	buffer.append(" ");
	buffer.append(Poco::NumberFormatter::format(mSize * mAllocations));
	buffer.append("\n");

	if (!mResolvedStackCall.empty()) {
		buffer.append(BEGIN_STACKTRACE_KW);
		buffer.append("\n");
		for (std::vector<std::tr1::shared_ptr<NativeStackCallInfo> >::iterator source = mResolvedStackCall.begin(); source != mResolvedStackCall.end(); ++source) {
			long addr = (*source)->getAddress();
			if (addr == 0) {
				continue;
			}

			if ((*source)->getLineNumber() != -1) {
				buffer.append(
						Poco::format("\t%1$08x\t%2$s --- %3$s --- %4$s:%5$d\n", addr, (*source)->getLibraryName(),
								(*source)->getMethodName(), (*source)->getSourceFile(), (*source)->getLineNumber()));
			} else {
				buffer.append(
						Poco::format("\t%1$08x\t%2$s --- %3$s --- %4$s\n", addr, (*source)->getLibraryName(),
								(*source)->getMethodName(), (*source)->getSourceFile()));
			}
		}
		buffer.append(END_STACKTRACE_KW);
		buffer.append("\n");
	}
	return buffer;
}

bool NativeAllocationInfo::isRelevantLibrary(const std::string &libPath) const {
	for (std::size_t i = 0; i != NUM_OF_FILT_LIBS; ++i) {
		if (FILTERED_LIBRARIES[i].rbegin()
				== std::search(FILTERED_LIBRARIES[i].rbegin(), FILTERED_LIBRARIES[i].rend(), libPath.rbegin(),
						libPath.rend())) {
			return false;
		}
	}
	return true;
}

bool NativeAllocationInfo::isRelevantMethod(const std::string &methodName) const {
	for (std::size_t i = 0; i != NUM_OF_FILT_PATTERNS; ++i) {
		Poco::RegularExpression p(FILTERED_METHOD_NAME_PATTERNS[i]);
		if (p.match(methodName)) {
			return false;
		}
	}
	return true;
}

std::tr1::shared_ptr<NativeStackCallInfo> NativeAllocationInfo::getRelevantStackCallInfo() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	if (mIsStackCallResolved && !mResolvedStackCall.empty()) {
		for (std::vector<std::tr1::shared_ptr<NativeStackCallInfo> >::iterator info = mResolvedStackCall.begin(); info != mResolvedStackCall.end(); ++info) {
			if (isRelevantLibrary((*info)->getLibraryName()) && isRelevantMethod((*info)->getMethodName())) {
				return *info;
			}
		}

		// couldnt find a relevant one, so we'll return the first one if it exists.
		if (!mResolvedStackCall.empty())
			return mResolvedStackCall.at(0);
	}

	return std::tr1::shared_ptr<NativeStackCallInfo>();
}

} /* namespace ddmlib */
