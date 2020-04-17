/*
 * NativeAllocationInfo.hpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Ilya Polenov
 */

#ifndef NATIVEALLOCATIONINFO_HPP_
#define NATIVEALLOCATIONINFO_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class NativeStackCallInfo;

class DDMLIB_API NativeAllocationInfo {

	/* constants for flag bits */
	static const int FLAG_ZYGOTE_CHILD = (1 << 31);
	static const int FLAG_MASK = (FLAG_ZYGOTE_CHILD);

	/** Libraries whose methods will be assumed to be not part of the user code. */
	static const unsigned int NUM_OF_FILT_LIBS = 2;
	static const unsigned int NUM_OF_FILT_PATTERNS = 5;
	static std::string FILTERED_LIBRARIES[NUM_OF_FILT_LIBS];
	static std::string FILTERED_METHOD_NAME_PATTERNS[NUM_OF_FILT_PATTERNS];

	int mSize;
	bool mIsZygoteChild;
	int mAllocations;
	std::vector<long long> mStackCallAddresses;
	std::vector<std::tr1::shared_ptr<NativeStackCallInfo> > mResolvedStackCall;
	bool mIsStackCallResolved;

	Poco::Mutex mLock;

	bool isRelevantLibrary(const std::string &libPath) const;
	bool isRelevantMethod(const std::string &methodName) const;
public:
	/* Keywords used as delimiters in the string representation of a NativeAllocationInfo */
	static std::string END_STACKTRACE_KW;
	static std::string BEGIN_STACKTRACE_KW;
	static std::string TOTAL_SIZE_KW;
	static std::string SIZE_KW;
	static std::string ALLOCATIONS_KW;
	/**
	 * Constructs a new {@link NativeAllocationInfo}.
	 * @param size The size of the allocations.
	 * @param allocations the allocation count
	 */
	NativeAllocationInfo(int size, int allocations);
	NativeAllocationInfo(const NativeAllocationInfo &other);
	NativeAllocationInfo &operator =(const NativeAllocationInfo &other);
	virtual ~NativeAllocationInfo();

	/**
	 * Adds a stack call address for this allocation.
	 * @param address The address to add.
	 */
	void addStackCallAddress(long long address);

	// Getters
	bool isStackCallResolved() const;
	int getAllocationCount() const;
	bool isZygoteChild() const;
	int getSize() const;
	std::vector<long long> getStackCallAddresses() const;

	std::vector<std::tr1::shared_ptr<NativeStackCallInfo> > getResolvedStackCall();
	void setResolvedStackCall(const std::vector<std::tr1::shared_ptr<NativeStackCallInfo> > &resolvedStackCall);

	std::string toString();

	/**
	 * Returns the first {@link NativeStackCallInfo} that is relevant.
	 * <p/>
	 * A relevant <code>NativeStackCallInfo</code> is a stack call that is not deep in the
	 * lower level of the libc, but the actual method that performed the allocation.
	 * @return a <code>NativeStackCallInfo</code> or <code>null</code> if the stack call has not
	 * been processed from the raw addresses.
	 * @see #setResolvedStackCall(ArrayList)
	 * @see #isStackCallResolved()
	 */
	std::tr1::shared_ptr<NativeStackCallInfo> getRelevantStackCallInfo();

};

} /* namespace ddmlib */
#endif /* NATIVEALLOCATIONINFO_HPP_ */
