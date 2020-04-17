/*
 * ClientData.hpp
 *
 *  Created on: Jan 26, 2012
 *      Author: Ilya Polenov
 */

#ifndef CLIENTDATA_HPP_
#define CLIENTDATA_HPP_

#include "ddmlib.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;
class ThreadInfo;
class AllocationInfo;
class NativeLibraryMapInfo;
class NativeAllocationInfo;
class HeapSegment;
class HeapSegmentElement;

enum DebuggerStatus {
	/** Debugger connection status: not waiting on one, not connected to one, but accepting
	 * new connections. This is the default value. */
	DebuggerStatusDEFAULT,
	/**
	 * Debugger connection status: the application's VM is paused, waiting for a debugger to
	 * connect to it before resuming. */
	DebuggerStatusWAITING,
	/** Debugger connection status : Debugger is connected */
	DebuggerStatusATTACHED,
	/** Debugger connection status: The listening port for debugger connection failed to listen.
	 * No debugger will be able to connect. */
	DebuggerStatusERROR
};

enum AllocationTrackingStatus {
	/**
	 * Allocation tracking status: unknown.
	 * <p/>This happens right after a {@link Client} is discovered
	 * by the {@link AndroidDebugBridge}, and before the {@link Client} answered the query
	 * regarding its allocation tracking status.
	 * @see Client#requestAllocationStatus()
	 */
	AllocationTrackingStatusUNKNOWN,
	/** Allocation tracking status: the {@link Client} is not tracking allocations. */
	AllocationTrackingStatusOFF,
	/** Allocation tracking status: the {@link Client} is tracking allocations. */
	AllocationTrackingStatusON
};

enum MethodProfilingStatus {
	/**
	 * Method profiling status: unknown.
	 * <p/>This happens right after a {@link Client} is discovered
	 * by the {@link AndroidDebugBridge}, and before the {@link Client} answered the query
	 * regarding its method profiling status.
	 * @see Client#requestMethodProfilingStatus()
	 */
	MethodProfilingStatusUNKNOWN,
	/** Method profiling status: the {@link Client} is not profiling method calls. */
	MethodProfilingStatusOFF,
	/** Method profiling status: the {@link Client} is profiling method calls. */
	MethodProfilingStatusON
};

/**
 * Heap Information.
 * <p/>The heap is composed of several {@link HeapSegment} objects.
 * <p/>A call to {@link #isHeapDataComplete()} will indicate if the segments (available through
 * {@link #getHeapSegments()}) represent the full heap.
 */
class DDMLIB_API HeapData {
	std::set<std::tr1::shared_ptr<HeapSegment> > mHeapSegments;
	bool mHeapDataComplete;
	std::vector<unsigned char> mProcessedHeapData;
	std::map<int, std::vector<std::tr1::shared_ptr<HeapSegmentElement> > > mProcessedHeapMap;
	Poco::Mutex mLock;

public:
	HeapData();

	HeapData(const HeapData& other);

	/**
	 * Abandon the current list of heap segments.
	 */
	void clearHeapData();

	/**
	 * Add raw HPSG chunk data to the list of heap segments.
	 *
	 * @param data The raw data from an HPSG chunk.
	 */
	void addHeapData(std::tr1::shared_ptr<ByteBuffer> data);

	/**
	 * Called when all heap data has arrived.
	 */
	void sealHeapData();

	/**
	 * Returns whether the heap data has been sealed.
	 */
	bool isHeapDataComplete() const;

	/**
	 * Get the collected heap data, if sealed.
	 *
	 * @return The list of heap segments if the heap data has been sealed, or null if it hasn't.
	 */
	std::vector<std::tr1::shared_ptr<HeapSegment> > getHeapSegments();

	/**
	 * Sets the processed heap data.
	 *
	 * @param heapData The new heap data (can be null)
	 */
	void setProcessedHeapData(const std::vector<unsigned char> &heapData);

	/**
	 * Get the processed heap data, if present.
	 *
	 * @return the processed heap data, or null.
	 */
	std::vector<unsigned char> getProcessedHeapData();

	void setProcessedHeapMap(std::map<int, std::vector<std::tr1::shared_ptr<HeapSegmentElement> > > heapMap);

	std::map<int, std::vector<std::tr1::shared_ptr<HeapSegmentElement> > > getProcessedHeapMap();
};

/**
 * Handlers able to act on HPROF dumps.
 */
class DDMLIB_API IHprofDumpHandler {
public:
	IHprofDumpHandler() {
	}

	virtual ~IHprofDumpHandler() {
	}

	/**
	 * Called when a HPROF dump succeeded.
	 * @param remoteFilePath the device-side path of the HPROF file.
	 * @param client the client for which the HPROF file was.
	 */
	virtual void onSuccess(const std::wstring &remoteFilePath, std::tr1::shared_ptr<Client> client) = 0;

	/**
	 * Called when a HPROF dump was successful.
	 * @param data the data containing the HPROF file, streamed from the VM
	 * @param client the client that was profiled.
	 */
	virtual void onSuccess(unsigned char *data, size_t length, std::tr1::shared_ptr<Client> client) = 0;

	/**
	 * Called when a hprof dump failed to end on the VM side
	 * @param client the client that was profiled.
	 * @param message an optional (<code>null<code> ok) error message to be displayed.
	 */
	virtual void onEndFailure(std::tr1::shared_ptr<Client> client, const std::string &message) = 0;
};

/**
 * Handlers able to act on Method profiling info
 */
class DDMLIB_API IMethodProfilingHandler {
public:
	IMethodProfilingHandler() {
	}
	;
	virtual ~IMethodProfilingHandler() {
	}
	;
	/**
	 * Called when a method tracing was successful.
	 * @param remoteFilePath the device-side path of the trace file.
	 * @param client the client that was profiled.
	 */
	virtual void onSuccess(const std::wstring &remoteFilePath, std::tr1::shared_ptr<Client> client) = 0;

	/**
	 * Called when a method tracing was successful.
	 * @param data the data containing the trace file, streamed from the VM
	 * @param client the client that was profiled.
	 */
	virtual void onSuccess(unsigned char *data, size_t length, std::tr1::shared_ptr<Client> client) = 0;

	/**
	 * Called when method tracing failed to start
	 * @param client the client that was profiled.
	 * @param message an optional (<code>null<code> ok) error message to be displayed.
	 */
	virtual void onStartFailure(std::tr1::shared_ptr<Client> client, const std::string &message) = 0;

	/**
	 * Called when method tracing failed to end on the VM side
	 * @param client the client that was profiled.
	 * @param message an optional (<code>null<code> ok) error message to be displayed.
	 */
	virtual void onEndFailure(std::tr1::shared_ptr<Client> client, const std::string &message) = 0;
};

/**
 * Contains the data of a {@link Client}.
 */
class DDMLIB_API ClientData {
	/* This is a place to stash data associated with a Client, such as thread
	 * states or heap data.  ClientData maps 1:1 to Client, but it's a little
	 * cleaner if we separate the data out.
	 *
	 * Message handlers are welcome to stash arbitrary data here.
	 *
	 * IMPORTANT: The data here is written by HandleFoo methods and read by
	 * FooPanel methods, which run in different threads.  All non-trivial
	 * access should be synchronized against the ClientData object.
	 */

	/** Temporary name of VM to be ignored. */
	static std::wstring PRE_INITIALIZED;

	static std::tr1::shared_ptr<IHprofDumpHandler> sHprofDumpHandler;
	static std::tr1::shared_ptr<IMethodProfilingHandler> sMethodProfilingHandler;

	// is this a DDM-aware client?
	bool mIsDdmAware;

	// the client's process ID
	int mPid;

	// Java VM identification string
	std::wstring mVmIdentifier;

	// client's self-description
	std::wstring mClientDescription;

	// how interested are we in a debugger?
	DebuggerStatus mDebuggerInterest;

	// List of supported features by the client.
	std::set<std::wstring> mFeatures;

	// Thread tracking (THCR, THDE).
	std::map<int, std::tr1::shared_ptr<ThreadInfo> > mThreadMap;

	/** VM Heap data */
	std::tr1::shared_ptr<HeapData> mHeapData;

	/** Native Heap data */
	std::tr1::shared_ptr<HeapData> mNativeHeapData;

	std::map<int, std::map<std::wstring, long long> > mHeapInfoMap;

	/** library map info. Stored here since the backtrace data
	 * is computed on a need to display basis.
	 */
	std::vector<NativeLibraryMapInfo> mNativeLibMapInfo;

	/** Native Alloc info list */
	std::vector<NativeAllocationInfo> mNativeAllocationList;
	int mNativeTotalMemory;

	std::vector<std::tr1::shared_ptr<AllocationInfo> > mAllocations;

	AllocationTrackingStatus mAllocationStatus;

	std::wstring mPendingHprofDump;

	MethodProfilingStatus mProfilingStatus;

	std::wstring mPendingMethodProfiling;

	Poco::Mutex mLock;
	static Poco::Mutex sLock;
public:
	ClientData(int pid);
	virtual ~ClientData();

	/**
	 * Name of the value representing the max size of the heap, in the {@link Map} returned by
	 * {@link #getVmHeapInfo(int)}
	 */
	static std::wstring HEAP_MAX_SIZE_BYTES;

	/**
	 * Name of the value representing the size of the heap, in the {@link Map} returned by
	 * {@link #getVmHeapInfo(int)}
	 */
	static std::wstring HEAP_SIZE_BYTES;

	/**
	 * Name of the value representing the number of allocated bytes of the heap, in the
	 * {@link Map} returned by {@link #getVmHeapInfo(int)}
	 */
	static std::wstring HEAP_BYTES_ALLOCATED;

	/**
	 * Name of the value representing the number of objects in the heap, in the {@link Map}
	 * returned by {@link #getVmHeapInfo(int)}
	 */
	static std::wstring HEAP_OBJECTS_ALLOCATED;

	/**
	 * String for feature enabling starting/stopping method profiling
	 * @see #hasFeature(String)
	 */
	static std::wstring FEATURE_PROFILING;

	/**
	 * String for feature enabling direct streaming of method profiling data
	 * @see #hasFeature(String)
	 */
	static std::wstring FEATURE_PROFILING_STREAMING;

	/**
	 * String for feature allowing to dump hprof files
	 * @see #hasFeature(String)
	 */
	static std::wstring FEATURE_HPROF;

	/**
	 * String for feature allowing direct streaming of hprof dumps
	 * @see #hasFeature(String)
	 */
	static std::wstring FEATURE_HPROF_STREAMING;
	/**
	 * Sets the handler to receive notifications when an HPROF dump succeeded or failed.
	 */
	static void setHprofDumpHandler(std::tr1::shared_ptr<IHprofDumpHandler> handler) {
		sHprofDumpHandler.swap(handler);
	}

	static std::tr1::shared_ptr<IHprofDumpHandler> getHprofDumpHandler() {
		return sHprofDumpHandler;
	}

	/**
	 * Sets the handler to receive notifications when an HPROF dump succeeded or failed.
	 */
	static void setMethodProfilingHandler(std::tr1::shared_ptr<IMethodProfilingHandler> handler) {
		Poco::ScopedLock<Poco::Mutex> lock(sLock);
		sMethodProfilingHandler.swap(handler);
	}

	static std::tr1::shared_ptr<IMethodProfilingHandler> getMethodProfilingHandler() {
		Poco::ScopedLock<Poco::Mutex> lock(sLock);
		return sMethodProfilingHandler;
	}

	bool isDdmAware() const;
	void setDdmAware(bool ddmAware);
	int getPid() const;
	std::wstring getVmIdentifier();
	void setVmIdentifier(const std::wstring &vmIdentifier);

	/**
	 * Sets client description.
	 *
	 * There may be a race between HELO and APNM.  Rather than try
	 * to enforce ordering on the device, we just don't allow an empty
	 * name to replace a specified one.
	 */
	void setClientDescription(const std::wstring &clientDescription);

	/**
	 * Returns the client description.
	 * <p/>This is generally the name of the package defined in the
	 * <code>AndroidManifest.xml</code>.
	 *
	 * @return the client description or empty string if not the description was not yet
	 * sent by the client.
	 */
	std::wstring getClientDescription() const;

	DebuggerStatus getDebuggerInterest() const;
	void setDebuggerInterest(const DebuggerStatus &debuggerInterest);

	/**
	 * Sets the current heap info values for the specified heap.
	 *
	 * @param heapId The heap whose info to update
	 * @param sizeInBytes The size of the heap, in bytes
	 * @param bytesAllocated The number of bytes currently allocated in the heap
	 * @param objectsAllocated The number of objects currently allocated in
	 *                         the heap
	 */
	// TODO: keep track of timestamp, reason
	void setHeapInfo(int heapId, long long maxSizeInBytes, long long sizeInBytes, long long bytesAllocated, long long objectsAllocated);

	std::tr1::shared_ptr<HeapData> getHeapData();

	std::tr1::shared_ptr<HeapData> getNativeHeapData();

	/*
	 * Returns known VM heap IDs.
	 */
	std::set<int> getVmHeapIds();

	/*
	 * Returns the most-recent info values for the specified VM heap.
	 */
	std::map<std::wstring, long long> getVmHeapInfo(int heapId);


	/**
	 * Returns the {@link HeapData} object for the VM.
	 */
	std::tr1::shared_ptr<HeapData> getVmHeapData();

	/**
	 * Adds a new thread to the list.
	 */
	void addThread(int threadId, const std::wstring &threadName);

	/**
	 * Removes a thread from the list.
	 */
	void removeThread(int threadId);

	/**
	 * Returns set of TIDs.
	 */
	std::set<int> getThreadIds();

	/**
	 * Returns thread map.
	 */
	std::map<int, std::tr1::shared_ptr<ThreadInfo> > getThreadMap();

	/**
	 * Returns the {@link ThreadInfo} by thread id.
	 */
	std::tr1::shared_ptr<ThreadInfo> getThread(int threadId);

	void clearThreads();



	/**
	 * adds a new {@link NativeAllocationInfo} to the {@link Client}
	 * @param allocInfo The {@link NativeAllocationInfo} to add.
	 */
	void addNativeAllocation(const NativeAllocationInfo &allocInfo);

	/**
	 * Clear the current malloc info.
	 */
	void clearNativeAllocationInfo();

	std::vector<NativeAllocationInfo> getNativeAllocationList();

	/**
	 * Returns the total native memory.
	 * @see Client#requestNativeHeapInformation()
	 */
	int getTotalNativeMemory();

	void setTotalNativeMemory(int totalMemory);

	void addNativeLibraryMapInfo(unsigned long long startAddr, unsigned long long endAddr, const std::string &library);

	std::vector<NativeLibraryMapInfo> getMappedNativeLibraries();

	void setAllocationStatus(AllocationTrackingStatus status);

	/**
	 * Returns the allocation tracking status.
	 * @see Client#requestAllocationStatus()
	 */
	AllocationTrackingStatus getAllocationStatus();

	void setAllocations(const std::vector<std::tr1::shared_ptr<AllocationInfo> > &allocs);

	std::vector<std::tr1::shared_ptr<AllocationInfo> > getAllocations();

	void addFeature(const std::wstring &feature);

	/**
	 * Returns true if the {@link Client} supports the given <var>feature</var>
	 * @param feature The feature to test.
	 * @return true if the feature is supported
	 *
	 * @see ClientData#FEATURE_PROFILING
	 * @see ClientData#FEATURE_HPROF
	 */
	bool hasFeature(const std::wstring &feature);

	/**
	 * Sets the device-side path to the hprof file being written
	 * @param pendingHprofDump the file to the hprof file
	 */
	void setPendingHprofDump(const std::wstring &pendingHprofDump);

	/**
	 * Returns the path to the device-side hprof file being written.
	 */
	const std::wstring getPendingHprofDump();

	bool hasPendingHprofDump();

	void setMethodProfilingStatus(MethodProfilingStatus status);

	/**
	 * Returns the method profiling status.
	 * @see Client#requestMethodProfilingStatus()
	 */
	MethodProfilingStatus getMethodProfilingStatus();

	/**
	 * Sets the device-side path to the method profile file being written
	 * @param pendingMethodProfiling the file being written
	 */
	void setPendingMethodProfiling(const std::wstring &pendingMethodProfiling);

	/**
	 * Returns the path to the device-side method profiling file being written.
	 */
	std::wstring getPendingMethodProfiling();
};

} /* namespace ddmlib */
#endif /* CLIENTDATA_HPP_ */
