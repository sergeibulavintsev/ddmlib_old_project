/*
 * Client.hpp
 *
 *  Created on: Jan 26, 2012
 *      Author: Ilya Polenov
 */

#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include "ddmlib.hpp"

class ByteBuffer;

namespace ddmlib {

class Device;
class ChunkHandler;
class Debugger;
class ClientData;
class HeapData;
class JdwpPacket;

class DDMLIB_API Client: public std::tr1::enable_shared_from_this<Client> {
private:
	static const int CLIENT_READY;

	static const int CLIENT_DISCONNECTED;

	static Client* selectedClient;

	std::tr1::shared_ptr<Poco::Net::StreamSocket> mChan;

	// debugger we're associated with, if any
	std::tr1::shared_ptr<Debugger> mDebugger;
	int mDebuggerListenPort;

	// list of IDs for requests we have sent to the client
	std::map<int, std::tr1::shared_ptr<ChunkHandler> > mOutstandingReqs;
	Poco::Mutex mOutstandingReqsLock;

	// chunk handlers stash state data in here
	std::tr1::shared_ptr<ClientData> mClientData;

	// User interface state.  Changing the value causes a message to be
	// sent to the client.
	bool mThreadUpdateEnabled;
	bool mHeapUpdateEnabled;

	/*
	 * Read/write buffers.  We can get large quantities of data from the
	 * client, e.g. the response to a "give me the list of all known classes"
	 * request from the debugger.  Requests from the debugger, and from us,
	 * are much smaller.
	 *
	 * Pass-through debugger traffic is sent without copying.  "mWriteBuffer"
	 * is only used for data generated within Client.
	 */
	static unsigned int const INITIAL_BUF_SIZE;
	static unsigned int const MAX_BUF_SIZE;
	std::tr1::shared_ptr<ByteBuffer> mReadBuffer;

	static unsigned int const WRITE_BUF_SIZE;
	std::tr1::shared_ptr<ByteBuffer> mWriteBuffer;

	std::tr1::weak_ptr<Device> mDevice;

	int mConnState;

	static int const ST_INIT = 1;
	static int const ST_NOT_JDWP = 2;
	static int const ST_AWAIT_SHAKE = 10;
	static int const ST_NEED_DDM_PKT = 11;
	static int const ST_NOT_DDM = 12;
	static int const ST_READY = 13;
	static int const ST_ERROR = 20;
	static int const ST_DISCONNECTED = 21;

	void init();

public:

	static int const SERVER_PROTOCOL_VERSION = 1;

	/** Client change bit mask: application name change */
	static int const CHANGE_NAME = 0x0001;
	/** Client change bit mask: debugger status change */
	static int const CHANGE_DEBUGGER_STATUS = 0x0002;
	/** Client change bit mask: debugger port change */
	static int const CHANGE_PORT = 0x0004;
	/** Client change bit mask: thread update flag change */
	static int const CHANGE_THREAD_MODE = 0x0008;
	/** Client change bit mask: thread data updated */
	static int const CHANGE_THREAD_DATA = 0x0010;
	/** Client change bit mask: heap update flag change */
	static int const CHANGE_HEAP_MODE = 0x0020;
	/** Client change bit mask: head data updated */
	static int const CHANGE_HEAP_DATA = 0x0040;
	/** Client change bit mask: native heap data updated */
	static int const CHANGE_NATIVE_HEAP_DATA = 0x0080;
	/** Client change bit mask: thread stack trace updated */
	static int const CHANGE_THREAD_STACKTRACE = 0x0100;
	/** Client change bit mask: allocation information updated */
	static int const CHANGE_HEAP_ALLOCATIONS = 0x0200;
	/** Client change bit mask: allocation information updated */
	static int const CHANGE_HEAP_ALLOCATION_STATUS = 0x0400;
	/** Client change bit mask: allocation information updated */
	static int const CHANGE_METHOD_PROFILING_STATUS = 0x0800;

	/** Client change bit mask: combination of {@link Client#CHANGE_NAME},
	 * {@link Client#CHANGE_DEBUGGER_STATUS}, and {@link Client#CHANGE_PORT}.
	 */
	static int const CHANGE_INFO = CHANGE_NAME | CHANGE_DEBUGGER_STATUS | CHANGE_PORT;

	/**
	 * Create an object for a new client connection.
	 *
	 * @param device the device this client belongs to
	 * @param chan the connected {@link SocketChannel}.
	 * @param pid the client pid.
	 */
	Client(std::tr1::shared_ptr<Device> device, std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, int pid);
	virtual ~Client();

	/**
	 * Returns a string representation of the {@link Client} object.
	 */
	std::string toString() const;

	/**
	 * Returns the {@link Device} on which this Client is running.
	 */
	std::tr1::shared_ptr<Device> getDevice() const;

	/**
	 * Returns the debugger port for this client.
	 */
	int getDebuggerListenPort();

	/**
	 * Returns <code>true</code> if the client VM is DDM-aware.
	 *
	 * Calling here is only allowed after the connection has been
	 * established.
	 */
	bool isDdmAware() const;

	/**
	 * Returns <code>true</code> if a debugger is currently attached to the client.
	 */
	bool isDebuggerAttached();

	/**
	 * Return the Debugger object associated with this client.
	 */
	std::tr1::shared_ptr<Debugger> getDebugger();

	/**
	 * Returns the {@link ClientData} object containing this client information.
	 */
	std::tr1::shared_ptr<ClientData> getClientData();

	/**
	 * Forces the client to execute its garbage collector.
	 */
	void executeGarbageCollector();

	/**
	 * Makes the VM dump an HPROF file
	 */
	void dumpHprof();

	void toggleMethodProfiling();

	/**
	 * Sends a request to the VM to send the enable status of the method profiling.
	 * This is asynchronous.
	 * <p/>The allocation status can be accessed by {@link ClientData#getAllocationStatus()}.
	 * The notification that the new status is available will be received through
	 * {@link IClientChangeListener#clientChanged(Client, int)} with a <code>changeMask</code>
	 * containing the mask {@link #CHANGE_HEAP_ALLOCATION_STATUS}.
	 */
	void requestMethodProfilingStatus();

	/**
	 * Enables or disables the thread update.
	 * <p/>If <code>true</code> the VM will be able to send thread information. Thread information
	 * must be requested with {@link #requestThreadUpdate()}.
	 * @param enabled the enable flag.
	 */
	void setThreadUpdateEnabled(bool enabled);

	/**
	 * Returns whether the thread update is enabled.
	 */
	bool isThreadUpdateEnabled();

	/**
	 * Sends a thread update request. This is asynchronous.
	 * <p/>The thread info can be accessed by {@link ClientData#getThreads()}. The notification
	 * that the new data is available will be received through
	 * {@link IClientChangeListener#clientChanged(Client, int)} with a <code>changeMask</code>
	 * containing the mask {@link #CHANGE_THREAD_DATA}.
	 */
	void requestThreadUpdate();

	/**
	 * Sends a thread stack trace update request. This is asynchronous.
	 * <p/>The thread info can be accessed by {@link ClientData#getThreads()} and
	 * {@link ThreadInfo#getStackTrace()}.
	 * <p/>The notification that the new data is available
	 * will be received through {@link IClientChangeListener#clientChanged(Client, int)}
	 * with a <code>changeMask</code> containing the mask {@link #CHANGE_THREAD_STACKTRACE}.
	 */
	void requestThreadStackTrace(int threadId);

	/**
	 * Enables or disables the heap update.
	 * <p/>If <code>true</code>, any GC will cause the client to send its heap information.
	 * <p/>The heap information can be accessed by {@link ClientData#getVmHeapData()}.
	 * <p/>The notification that the new data is available
	 * will be received through {@link IClientChangeListener#clientChanged(Client, int)}
	 * with a <code>changeMask</code> containing the value {@link #CHANGE_HEAP_DATA}.
	 * @param enabled the enable flag
	 */
	void setHeapUpdateEnabled(bool enabled);

	/**
	 * Returns whether the heap update is enabled.
	 * @see #setHeapUpdateEnabled(boolean)
	 */
	bool isHeapUpdateEnabled();

	/**
	 * Sends a native heap update request. this is asynchronous.
	 * <p/>The native heap info can be accessed by {@link ClientData#getNativeAllocationList()}.
	 * The notification that the new data is available will be received through
	 * {@link IClientChangeListener#clientChanged(Client, int)} with a <code>changeMask</code>
	 * containing the mask {@link #CHANGE_NATIVE_HEAP_DATA}.
	 */
	bool requestNativeHeapInformation();

	/**
	 * Enables or disables the Allocation tracker for this client.
	 * <p/>If enabled, the VM will start tracking allocation informations. A call to
	 * {@link #requestAllocationDetails()} will make the VM sends the information about all the
	 * allocations that happened between the enabling and the request.
	 * @param enable
	 * @see #requestAllocationDetails()
	 */
	void enableAllocationTracker(bool enable);

	/**
	 * Sends a request to the VM to send the enable status of the allocation tracking.
	 * This is asynchronous.
	 * <p/>The allocation status can be accessed by {@link ClientData#getAllocationStatus()}.
	 * The notification that the new status is available will be received through
	 * {@link IClientChangeListener#clientChanged(Client, int)} with a <code>changeMask</code>
	 * containing the mask {@link #CHANGE_HEAP_ALLOCATION_STATUS}.
	 */
	void requestAllocationStatus();

	/**
	 * Sends a request to the VM to send the information about all the allocations that have
	 * happened since the call to {@link #enableAllocationTracker(boolean)} with <var>enable</var>
	 * set to <code>null</code>. This is asynchronous.
	 * <p/>The allocation information can be accessed by {@link ClientData#getAllocations()}.
	 * The notification that the new data is available will be received through
	 * {@link IClientChangeListener#clientChanged(Client, int)} with a <code>changeMask</code>
	 * containing the mask {@link #CHANGE_HEAP_ALLOCATIONS}.
	 */
	void requestAllocationDetails();

	/**
	 * Sends a kill message to the VM.
	 */
	void kill();

	/**
	 * Registers the client with a Selector.
	 */
	void registerInReactor();

	/**
	 * Sets the client to accept debugger connection on the "selected debugger port".
	 *
	 * @see AndroidDebugBridge#setSelectedClient(Client)
	 * @see DdmPreferences#setSelectedDebugPort(int)
	 */
	void setAsSelectedClient();

	/**
	 * Returns whether this client is the current selected client, accepting debugger connection
	 * on the "selected debugger port".
	 *
	 * @see #setAsSelectedClient()
	 * @see AndroidDebugBridge#setSelectedClient(Client)
	 * @see DdmPreferences#setSelectedDebugPort(int)
	 */
	bool isSelectedClient() const;

	/**
	 * Tell the client to open a server socket channel and listen for
	 * connections on the specified port.
	 */
	void listenForDebugger(int listenPort);

	/**
	 * Initiate the JDWP handshake.
	 *
	 * On failure, closes the socket and returns false.
	 */
	bool sendHandshake();

	/**
	 * Send a non-DDM packet to the client.
	 *
	 * Equivalent to sendAndConsume(packet, null).
	 */
	void sendAndConsume(std::tr1::shared_ptr<JdwpPacket> packet);

	/**
	 * Send a DDM packet to the client.
	 *
	 * Ideally, we can do this with a single channel write.  If that doesn't
	 * happen, we have to prevent anybody else from writing to the channel
	 * until this packet completes, so we synchronize on the channel.
	 *
	 * Another goal is to avoid unnecessary buffer copies, so we write
	 * directly out of the JdwpPacket's ByteBuffer.
	 */
	void sendAndConsume(std::tr1::shared_ptr<JdwpPacket> packet, std::tr1::shared_ptr<ChunkHandler> replyHandler);

	/**
	 * Forward the packet to the debugger (if still connected to one).
	 *
	 * Consumes the packet.
	 */
	void forwardPacketToDebugger(std::tr1::shared_ptr<JdwpPacket> packet);

	/**
	 * Read data from our channel.
	 *
	 * This is called when data is known to be available, and we don't yet
	 * have a full packet in the buffer.  If the buffer is at capacity,
	 * expand it.
	 */
	void read();

	/**
	 * Return information for the first full JDWP packet in the buffer.
	 *
	 * If we don't yet have a full packet, return null.
	 *
	 * If we haven't yet received the JDWP handshake, we watch for it here
	 * and consume it without admitting to have done so.  Upon receipt
	 * we send out the "HELO" message, which is why this can throw an
	 * IOException.
	 */
	std::tr1::shared_ptr<JdwpPacket> getJdwpPacket();

	/*
	 * Add the specified ID to the list of request IDs for which we await
	 * a response.
	 */
	void addRequestId(int id, std::tr1::shared_ptr<ChunkHandler> handler);

	/*
	 * Remove the specified ID from the list, if present.
	 */
	void removeRequestId(int id);

	/**
	 * Determine whether this is a response to a request we sent earlier.
	 * If so, return the ChunkHandler responsible.
	 */
	std::tr1::shared_ptr<ChunkHandler> isResponseToUs(int id);

	/**
	 * An earlier request resulted in a failure.  This is the expected
	 * response to a HELO message when talking to a non-DDM client.
	 */
	void packetFailed(JdwpPacket &reply);

	/**
	 * The MonitorThread calls this when it sees a DDM request or reply.
	 * If we haven't seen a DDM packet before, we advance the state to
	 * ST_READY and return "false".  Otherwise, just return true.
	 *
	 * The idea is to let the MonitorThread know when we first see a DDM
	 * packet, so we can send a broadcast to the handlers when a client
	 * connection is made.  This method is synchronized so that we only
	 * send the broadcast once.
	 */
	bool ddmSeen();

	/**
	 * Close the client socket channel.  If there is a debugger associated
	 * with us, close that too.
	 *
	 * Closing a channel automatically unregisters it from the selector.
	 * However, we have to iterate through the selector loop before it
	 * actually lets them go and allows the file descriptors to close.
	 * The caller is expected to manage that.
	 * @param notify Whether or not to notify the listeners of a change.
	 */
	void close(bool notify);

	/**
	 * Returns whether this {@link Client} has a valid connection to the application VM.
	 */
	bool isValid() const;

	void update(int changeMask);

	/*
	 * Process client read notification.
	 */
	void processClientReadActivity(const Poco::AutoPtr<Poco::Net::ReadableNotification> &notification);

	/*
	 * Process client shutdown notification.
	 */
	void processClientShutdownActivity(const Poco::AutoPtr<Poco::Net::ShutdownNotification> &notification);

	/*
	 * Process client error notification.
	 */
	void processClientError(const Poco::AutoPtr<Poco::Net::ErrorNotification> &notification);

	/**
	 * Drops the client from the monitor.
	 * <p/>This will lock the {@link Client} list of the {@link Device} running <var>client</var>.
	 * @param client
	 * @param notify
	 */
	void dropClient(bool notify);

};

} /* namespace ddmlib */
#endif /* CLIENT_HPP_ */
