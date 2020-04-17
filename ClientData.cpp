/*
 * ClientData.cpp
 *
 *  Created on: Jan 26, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "ClientData.hpp"
#include "HeapSegment.hpp"
#include "Log.hpp"
#include "ThreadInfo.hpp"
#include "NativeAllocationInfo.hpp"
#include "NativeLibraryMapInfo.hpp"

namespace ddmlib {

std::wstring ClientData::PRE_INITIALIZED(L"<pre-initialized>");
std::wstring ClientData::HEAP_MAX_SIZE_BYTES(L"maxSizeInBytes");
std::wstring ClientData::HEAP_SIZE_BYTES(L"sizeInBytes");
std::wstring ClientData::HEAP_BYTES_ALLOCATED(L"bytesAllocated");
std::wstring ClientData::HEAP_OBJECTS_ALLOCATED(L"objectsAllocated");
std::wstring ClientData::FEATURE_PROFILING(L"method-trace-profiling");
std::wstring ClientData::FEATURE_PROFILING_STREAMING(L"method-trace-profiling-streaming");
std::wstring ClientData::FEATURE_HPROF(L"hprof-heap-dump");
std::wstring ClientData::FEATURE_HPROF_STREAMING(L"hprof-heap-dump-streaming");
Poco::Mutex ClientData::sLock;
std::tr1::shared_ptr<IMethodProfilingHandler> ClientData::sMethodProfilingHandler;
std::tr1::shared_ptr<IHprofDumpHandler> ClientData::sHprofDumpHandler;

ClientData::ClientData(int pid) :
		mIsDdmAware(false),
		mPid(pid), mDebuggerInterest(DebuggerStatusDEFAULT), mNativeTotalMemory(0),
		mAllocationStatus(AllocationTrackingStatusUNKNOWN),
		mProfilingStatus(MethodProfilingStatusUNKNOWN),
		mHeapData(new HeapData) {
}

bool ClientData::isDdmAware() const {
	return mIsDdmAware;
}

int ClientData::getPid() const {
	return mPid;
}

DebuggerStatus ClientData::getDebuggerInterest() const {
	return mDebuggerInterest;
}

void ClientData::setDebuggerInterest(const DebuggerStatus &debuggerInterest) {
	mDebuggerInterest = debuggerInterest;
}

std::wstring ClientData::getClientDescription() const {
	return mClientDescription;
}

void ClientData::setClientDescription(const std::wstring &clientDescription) {
	if (mClientDescription.empty() && !clientDescription.empty()) {
		/*
		 * The application VM is first named <pre-initialized> before being assigned
		 * its real name.
		 * Depending on the timing, we can get an APNM chunk setting this name before
		 * another one setting the final actual name. So if we get a SetClientDescription
		 * with this value we ignore it.
		 */
		if (PRE_INITIALIZED != clientDescription)
			mClientDescription = clientDescription;
	}
}

std::wstring ClientData::getVmIdentifier() {
	return mVmIdentifier;
}

void ClientData::setVmIdentifier(const std::wstring &vmIdentifier) {
	mVmIdentifier = vmIdentifier;
}

ClientData::~ClientData() {
	if (Log::Config::LOGV) {
		Log::v("ddms", "Client data of " + Log::convertUtf16ToUtf8(mClientDescription) + " is destroyed");
	}
}

std::tr1::shared_ptr<HeapData> ClientData::getNativeHeapData() {
	return mNativeHeapData;
}

std::tr1::shared_ptr<HeapData> ClientData::getHeapData() {
	return mHeapData;
}

std::set<int> ClientData::getVmHeapIds() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	std::set<int> heapIds;
	for (std::map<int, std::map<std::wstring, long long> >::iterator heapInfo = mHeapInfoMap.begin(); heapInfo != mHeapInfoMap.end(); ++heapInfo)
		heapIds.insert((*heapInfo).first);
	return heapIds;
}

std::map<std::wstring, long long> ClientData::getVmHeapInfo(int heapId) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	if (mHeapInfoMap.count(heapId) > 0)
		return mHeapInfoMap[heapId];
	else
		return std::map<std::wstring, long long>();
}

void ClientData::setHeapInfo(int heapId, long long maxSizeInBytes, long long sizeInBytes, long long bytesAllocated, long long objectsAllocated) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	std::map<std::wstring, long long> heapInfo;
	heapInfo[HEAP_MAX_SIZE_BYTES] = maxSizeInBytes;
	heapInfo[HEAP_SIZE_BYTES] = sizeInBytes;
	heapInfo[HEAP_BYTES_ALLOCATED] = bytesAllocated;
	heapInfo[HEAP_OBJECTS_ALLOCATED] = objectsAllocated;
	mHeapInfoMap[heapId] = heapInfo;
}

void ClientData::addThread(int threadId, const std::wstring &threadName) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mThreadMap[threadId] = std::tr1::shared_ptr<ThreadInfo>(new ThreadInfo(threadId, threadName));
}

void ClientData::removeThread(int threadId) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mThreadMap.erase(threadId);
}

std::set<int> ClientData::getThreadIds() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	std::set<int> threadIDs;
	for (std::map<int, std::tr1::shared_ptr<ThreadInfo> >::iterator threadInfo = mThreadMap.begin(); threadInfo != mThreadMap.end(); ++threadInfo)
		threadIDs.insert((*threadInfo).first);
	return threadIDs;
}

std::map<int, std::tr1::shared_ptr<ThreadInfo> > ClientData::getThreadMap() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mThreadMap;
}

std::tr1::shared_ptr<ThreadInfo> ClientData::getThread(int threadId) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	if (mThreadMap.count(threadId) > 0)
		return mThreadMap[threadId];
	else
		return std::tr1::shared_ptr<ThreadInfo>();
}

void ClientData::clearThreads() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mThreadMap.clear();
}

void ClientData::addNativeAllocation(const NativeAllocationInfo & allocInfo) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mNativeAllocationList.push_back(allocInfo);
}

void ClientData::clearNativeAllocationInfo() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mNativeAllocationList.clear();
}

std::vector<NativeAllocationInfo> ClientData::getNativeAllocationList() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mNativeAllocationList;
}

int ClientData::getTotalNativeMemory() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mNativeTotalMemory;
}

void ClientData::setTotalNativeMemory(int totalMemory) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mNativeTotalMemory = totalMemory;
}

void ClientData::addNativeLibraryMapInfo(unsigned long long startAddr, unsigned long long endAddr, const std::string & library) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mNativeLibMapInfo.push_back(NativeLibraryMapInfo(startAddr, endAddr, library));
}

void ClientData::setAllocationStatus(AllocationTrackingStatus status) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mAllocationStatus = status;
}

std::vector<NativeLibraryMapInfo> ClientData::getMappedNativeLibraries() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mNativeLibMapInfo;
}
AllocationTrackingStatus ClientData::getAllocationStatus() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mAllocationStatus;
}

void ClientData::setAllocations(const std::vector<std::tr1::shared_ptr<AllocationInfo> > & allocs) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mAllocations = allocs;
}
std::vector<std::tr1::shared_ptr<AllocationInfo> > ClientData::getAllocations(){
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mAllocations;
}

void ClientData::addFeature(const std::wstring &feature) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mFeatures.insert(feature);
}

bool ClientData::hasFeature(const std::wstring &feature) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return static_cast<bool>(mFeatures.count(feature) > 0);
}

void ClientData::setPendingHprofDump(const std::wstring &pendingHprofDump) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mPendingHprofDump = pendingHprofDump;
}

const std::wstring ClientData::getPendingHprofDump() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mPendingHprofDump;
}

bool ClientData::hasPendingHprofDump() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return !mPendingHprofDump.empty();
}

void ClientData::setMethodProfilingStatus(MethodProfilingStatus status) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mProfilingStatus = status;
}

MethodProfilingStatus ClientData::getMethodProfilingStatus() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mProfilingStatus;
}

void ClientData::setPendingMethodProfiling(const std::wstring & pendingMethodProfiling) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mPendingMethodProfiling = pendingMethodProfiling;
}

void ClientData::setDdmAware(bool ddmAware) {
	mIsDdmAware = ddmAware;
}

std::tr1::shared_ptr<HeapData> ClientData::getVmHeapData() {
	return mHeapData;
}

std::wstring ClientData::getPendingMethodProfiling() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return mPendingMethodProfiling;
}

HeapData::HeapData() : mLock() {
	std::set<std::tr1::shared_ptr<HeapSegment> > mHeapSegments;
	mHeapDataComplete = false;
	std::vector<unsigned char> mProcessedHeapData;
	std::map<int, std::vector<std::tr1::shared_ptr<HeapSegmentElement> > > mProcessedHeapMap;
}

HeapData::HeapData(const HeapData& other) : mLock() {
	mHeapSegments = other.mHeapSegments;
	mHeapDataComplete = other.mHeapDataComplete;
	mProcessedHeapData = other.mProcessedHeapData;
	mProcessedHeapMap = other.mProcessedHeapMap;
}

void HeapData::clearHeapData() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	/* Old comment: Abandon the old segments instead of just calling .clear().
	 * This lets the user hold onto the old set if it wants to.
	 *
	 * Ilya: we're calling clear because we're using shared_ptr.
	 * The user may still use the old data if he has aquired the pointers.
	 */
	mHeapSegments.clear();
	mHeapDataComplete = false;
}

void HeapData::addHeapData(std::tr1::shared_ptr<ByteBuffer> data) {
	if (mHeapDataComplete) {
		clearHeapData();
	}
	std::tr1::shared_ptr<HeapSegment> hs;

	try {
		hs = std::tr1::shared_ptr<HeapSegment>(new HeapSegment(data));
	} catch (std::underflow_error &e) {
		//Log::e("ddms","Discarding short HPSG data (length " + data.limit() + ")");
		return;
	}

	mHeapSegments.insert(hs);
}

void HeapData::sealHeapData() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	mHeapDataComplete = true;
}

bool HeapData::isHeapDataComplete() const {
	return mHeapDataComplete;
}

std::vector<std::tr1::shared_ptr<HeapSegment> > HeapData::getHeapSegments() {
	if (isHeapDataComplete()) {
		return std::vector<std::tr1::shared_ptr<HeapSegment> >(mHeapSegments.begin(), mHeapSegments.end());
	} else {
		return std::vector<std::tr1::shared_ptr<HeapSegment> >(0);
	}
}

void HeapData::setProcessedHeapData(const std::vector<unsigned char> &heapData) {
	mProcessedHeapData = heapData;
}

std::vector<unsigned char> HeapData::getProcessedHeapData() {
	return mProcessedHeapData;
}

void HeapData::setProcessedHeapMap(std::map<int, std::vector<std::tr1::shared_ptr<HeapSegmentElement> > > heapMap) {
	mProcessedHeapMap = heapMap;
}

std::map<int, std::vector<std::tr1::shared_ptr<HeapSegmentElement> > > HeapData::getProcessedHeapMap() {
	return mProcessedHeapMap;
}

} /* namespace ddmlib */
