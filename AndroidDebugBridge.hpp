/*
 * AndroidDebugBridge.h
 *
 *  Created on: Jan 27, 2012
 *      Author: Ilya Polenov
 */

#ifndef ANDROIDDEBUGBRIDGE_H_
#define ANDROIDDEBUGBRIDGE_H_

#include "ddmlib.hpp"
#include "ProcessLauncher.hpp"

namespace ddmlib {

class AndroidDebugBridge;
class DeviceMonitor;
class DdmSocketReactor;
class Device;
#ifdef CLIENT_SUPPORT
class Client;
class Debugger;
#endif

/**
 * Classes which implement this interface provide a method that deals
 * with {@link AndroidDebugBridge} changes.
 */
class DDMLIB_API IDebugBridgeChangeListener {
public:
	IDebugBridgeChangeListener() {}
	virtual ~IDebugBridgeChangeListener() {}

	/**
	 * Sent when a new {@link AndroidDebugBridge} is connected.
	 * <p/>
	 * This is sent from a non UI thread.
	 * @param bridge the new {@link AndroidDebugBridge} object.
	 */
	virtual void bridgeChanged(AndroidDebugBridge *bridge) = 0;
};

/**
 * Classes which implement this interface provide methods that deal
 * with {@link Device} addition, deletion, and changes.
 */
class DDMLIB_API IDeviceChangeListener {
public:
	IDeviceChangeListener() {}
	virtual ~IDeviceChangeListener() {}
	/**
	 * Sent when the a device is connected to the {@link AndroidDebugBridge}.
	 * <p/>
	 * This is sent from a non UI thread.
	 * @param device the new device.
	 */
	virtual void deviceConnected(Device *device) = 0;

	/**
	 * Sent when the a device is connected to the {@link AndroidDebugBridge}.
	 * <p/>
	 * This is sent from a non UI thread.
	 * @param device the new device.
	 */
	virtual void deviceDisconnected(Device *device) = 0;

	/**
	 * Sent when a device data changed, or when clients are started/terminated on the device.
	 * <p/>
	 * This is sent from a non UI thread.
	 * @param device the device that was updated.
	 * @param changeMask the mask describing what changed. It can contain any of the following
	 * values: {@link Device#CHANGE_BUILD_INFO}, {@link Device#CHANGE_STATE},
	 * {@link Device#CHANGE_CLIENT_LIST}
	 */
	virtual void deviceChanged(Device *device, int changeMask) = 0;
};

#ifdef CLIENT_SUPPORT
/**
 * Classes which implement this interface provide methods that deal
 * with {@link Client}  changes.
 */
class DDMLIB_API IClientChangeListener {
public:
	IClientChangeListener() {}
	virtual ~IClientChangeListener() {}

	/**
	 * Sent when an existing client information changed.
	 * <p/>
	 * This is sent from a non UI thread.
	 * @param client the updated client.
	 * @param changeMask the bit mask describing the changed properties. It can contain
	 * any of the following values: {@link Client#CHANGE_INFO},
	 * {@link Client#CHANGE_DEBUGGER_STATUS}, {@link Client#CHANGE_THREAD_MODE},
	 * {@link Client#CHANGE_THREAD_DATA}, {@link Client#CHANGE_HEAP_MODE},
	 * {@link Client#CHANGE_HEAP_DATA}, {@link Client#CHANGE_NATIVE_HEAP_DATA}
	 */
	virtual void clientChanged(Client *client, int changeMask) = 0;
};
#endif

class DDMLIB_API AndroidDebugBridge {

	/*
	 * Minimum and maximum version of adb supported. This correspond to
	 * ADB_SERVER_VERSION found in //device/tools/adb/adb.h
	 */
	static int const ADB_VERSION_MICRO_MIN = 20;
	static int const ADB_VERSION_MICRO_MAX = -1;

	static Poco::RegularExpression sAdbVersion;

	static char ADB[];
	static char DDMS[];
	static char SERVER_PORT_ENV_VAR[];

	// Where to find the ADB bridge.
	static char ADB_HOST[];
	static unsigned int ADB_PORT;

	static std::string sHostAddr;
	static Poco::Net::SocketAddress sSocketAddr;

	static std::tr1::shared_ptr<AndroidDebugBridge> sThis;
	static bool sInitialized;

#ifdef CLIENT_SUPPORT
	static bool sClientSupport;
	static std::set< std::tr1::shared_ptr<IClientChangeListener> > sClientListeners;
#endif

	/** Full path to adb. */
	static std::string sAdbOsLocation;

	bool mVersionCheck;

	bool mStarted;

	std::tr1::shared_ptr<DeviceMonitor> mDeviceMonitor;

	static std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> > sBridgeListeners;
	static std::set< std::tr1::shared_ptr<IDeviceChangeListener> > sDeviceListeners;

	static Poco::Mutex sLock;

	static DdmSocketReactor sReactor;
	static std::tr1::shared_ptr<Poco::Thread> sReactorThread;

	/**
	 * Creates a new bridge not linked to any particular adb executable.
	 */
	AndroidDebugBridge();

	/**
	 * Creates a new bridge.
	 * @param osLocation the location of the command line tool
	 * @throws invalid_argument exception
	 */
	AndroidDebugBridge(const std::string &osLocation);

	/**
	 * Queries adb for its version number and checks it against {@link #MIN_VERSION_NUMBER} and
	 * {@link #MAX_VERSION_NUMBER}
	 */
	void checkAdbVersion();

	/**
	 * Scans a line resulting from 'adb version' for a potential version number.
	 * <p/>
	 * If a version number is found, it checks the version number against what is expected
	 * by this version of ddms.
	 * <p/>
	 * Returns true when a version number has been found so that we can stop scanning,
	 * whether the version number is in the acceptable range or not.
	 *
	 * @param line The line to scan.
	 * @return True if a version number was found (whether it is acceptable or not).
	 */
	bool scanVersionLine(const std::string &line);

	/**
	 * Starts the debug bridge.
	 * @return true if success.
	 */
	bool start();

	/**
	 * Kills the debug bridge, and the adb host server.
	 * @return true if success
	 */
	bool stop();

	/**
	 * Restarts adb, but not the services around it.
	 * @return true if success.
	 */
	bool restart();

	/**
	 * Instantiates sSocketAddr with the address of the host's adb process.
	 */
	static void initAdbSocketAddr();

	/**
	 * Determines port where ADB is expected by looking at an env variable.
	 * <p/>
	 * The value for the environment variable ANDROID_ADB_SERVER_PORT is validated,
	 * IllegalArgumentException is thrown on illegal values.
	 * <p/>
	 * @return The port number where the host's adb should be expected or started.
	 * @throws invalid_argument if ANDROID_ADB_SERVER_PORT has a non-numeric value.
	 */
	static unsigned int determineAndValidateAdbPort();

public:
	/**
	 * Initializes the <code>ddm</code> library.
	 * <p/>This must be called once <b>before</b> any call to
	 * {@link #createBridge(String, boolean)}.
	 * <p>The library can be initialized in 2 ways:
	 * <ul>
	 * <li>Mode 1: <var>clientSupport</var> == <code>true</code>.<br>The library monitors the
	 * devices and the applications running on them. It will connect to each application, as a
	 * debugger of sort, to be able to interact with them through JDWP packets.</li>
	 * <li>Mode 2: <var>clientSupport</var> == <code>false</code>.<br>The library only monitors
	 * devices. The applications are left untouched, letting other tools built on
	 * <code>ddmlib</code> to connect a debugger to them.</li>
	 * </ul>
	 * <p/><b>Only one tool can run in mode 1 at the same time.</b>
	 * <p/>Note that mode 1 does not prevent debugging of applications running on devices. Mode 1
	 * lets debuggers connect to <code>ddmlib</code> which acts as a proxy between the debuggers and
	 * the applications to debug. See {@link Client#getDebuggerListenPort()}.
	 * <p/>The preferences of <code>ddmlib</code> should also be initialized with whatever default
	 * values were changed from the default values.
	 * <p/>When the application quits, {@link #terminate()} should be called.
	 * @param clientSupport Indicates whether the library should enable the monitoring and
	 * interaction with applications running on the devices.
	 * @see AndroidDebugBridge#createBridge(String, boolean)
	 * @see DdmPreferences
	 */
	static void init(bool clientSupport);
	/**
	 * Terminates the ddm library. This must be called upon application termination.
	 */
	static void terminate();

	/**
	 * Returns the socket address of the ADB server on the host.
	 */
	static Poco::Net::SocketAddress getSocketAddress();

	/**
	 * Creates a {@link AndroidDebugBridge} that is not linked to any particular executable.
	 * <p/>This bridge will expect adb to be running. It will not be able to start/stop/restart
	 * adb.
	 * <p/>If a bridge has already been started, it is directly returned with no changes (similar
	 * to calling {@link #getBridge()}).
	 * @return a pointer to connected bridge.
	 */
	static AndroidDebugBridge *createBridge();

	/**
	 * Creates a new debug bridge from the location of the command line tool.
	 * <p/>
	 * Any existing server will be disconnected, unless the location is the same and
	 * <code>forceNewBridge</code> is set to false.
	 * @param osLocation the location of the command line tool 'adb'
	 * @param forceNewBridge force creation of a new bridge even if one with the same location
	 * already exists.
	 * @return a pointer to connected bridge.
	 */
	static AndroidDebugBridge *createBridge(const std::string &osLocation, bool forceNewBridge);

	/**
	 * Returns the current debug bridge. Can be <code>nullptr</code> if none were created.
	 */
	static AndroidDebugBridge *getBridge();

	/**
	 * Disconnects the current debug bridge, and destroy the object.
	 * <p/>This also stops the current adb host server.
	 * <p/>
	 * A new object will have to be created with {@link #createBridge(String, boolean)}.
	 */
	static void disconnectBridge();

	/**
	 * Adds the listener to the collection of listeners who will be notified when a new
	 * {@link AndroidDebugBridge} is connected, by sending it one of the messages defined
	 * in the {@link IDebugBridgeChangeListener} interface.
	 * @param listener The listener which should be notified.
	 */
	static void addDebugBridgeChangeListener(std::tr1::shared_ptr<IDebugBridgeChangeListener> listener);

	/**
	 * Removes the listener from the collection of listeners who will be notified when a new
	 * {@link AndroidDebugBridge} is started.
	 * @param listener The listener which should no longer be notified.
	 */
	static void removeDebugBridgeChangeListener(std::tr1::shared_ptr<IDebugBridgeChangeListener> listener);

	/**
	 * Adds the listener to the collection of listeners who will be notified when a {@link Device}
	 * is connected, disconnected, or when its properties or its {@link Client} list changed,
	 * by sending it one of the messages defined in the {@link IDeviceChangeListener} interface.
	 * @param listener The listener which should be notified.
	 */
	static void addDeviceChangeListener(std::tr1::shared_ptr<IDeviceChangeListener> listener);

	/**
	 * Removes the listener from the collection of listeners who will be notified when a
	 * {@link Device} is connected, disconnected, or when its properties or its {@link Client}
	 * list changed.
	 * @param listener The listener which should no longer be notified.
	 */
	static void removeDeviceChangeListener(std::tr1::shared_ptr<IDeviceChangeListener> listener);

#ifdef CLIENT_SUPPORT
	/**
	 * Returns whether the ddmlib is setup to support monitoring and interacting with
	 * {@link Client}s running on the {@link Device}s.
	 */
	static bool getClientSupport();

	/**
	 * Adds the listener to the collection of listeners who will be notified when a {@link Client}
	 * property changed, by sending it one of the messages defined in the
	 * {@link IClientChangeListener} interface.
	 * @param listener The listener which should be notified.
	 */
	static void addClientChangeListener(std::tr1::shared_ptr<IClientChangeListener> listener);

	/**
	 * Removes the listener from the collection of listeners who will be notified when a
	 * {@link Client} property changed.
	 * @param listener The listener which should no longer be notified.
	 */
	static void removeClientChangeListener(std::tr1::shared_ptr<IClientChangeListener> listener);
	
	/**
	 * Notify the listener of a modified {@link Client}.
	 * <p/>
	 * The notification of the listeners is done in a synchronized block. It is important to
	 * expect the listeners to potentially access various methods of {@link Device} as well as
	 * {@link #getDevices()} which use internal locks.
	 * <p/>
	 * For this reason, any call to this method from a method of {@link DeviceMonitor},
	 * {@link Device} which is also inside a synchronized block, should first synchronize on
	 * the {@link AndroidDebugBridge} lock. Access to this lock is done through {@link #getLock()}.
	 * @param device the modified <code>Client</code>.
	 * @param changeMask the mask indicating what changed in the <code>Client</code>
	 * @see #getLock()
	 */
	void clientChanged(std::tr1::shared_ptr<Client> client, int changeMask);
#endif

	/**
	 * Returns set of the devices.
	 * @see #hasInitialDeviceList()
	 */
	std::vector<std::tr1::shared_ptr<Device> > getDevices();

	/**
	 * Returns whether the bridge has acquired the initial list from adb after being created.
	 * <p/>Calling {@link #getDevices()} right after {@link #createBridge(String, boolean)} will
	 * generally result in an empty list. This is due to the internal asynchronous communication
	 * mechanism with <code>adb</code> that does not guarantee that the {@link Device} list has been
	 * built before the call to {@link #getDevices()}.
	 * <p/>The recommended way to get the list of {@link Device} objects is to create a
	 * {@link IDeviceChangeListener} object.
	 */
	bool hasInitialDeviceList();

#ifdef CLIENT_SUPPORT
	/**
	 * Sets the client to accept debugger connection on the custom "Selected debug port".
	 * @param selectedClient the client. Can be null.
	 */
	void setSelectedClient(std::tr1::shared_ptr<Client> selectedClient);
#endif

	/**
	 * Returns whether the {@link AndroidDebugBridge} object is still connected to the adb daemon.
	 */
	bool isConnected();

	/**
	 * Returns the number of times the {@link AndroidDebugBridge} object attempted to connect
	 * to the adb daemon.
	 */
	int getConnectionAttemptCount();

	/**
	 * Returns the number of times the {@link AndroidDebugBridge} object attempted to restart
	 * the adb daemon.
	 */
	int getRestartAttemptCount();

	/**
	 * Returns the {@link DeviceMonitor} object.
	 */
	std::tr1::shared_ptr<DeviceMonitor> getDeviceMonitor();

	/**
	 * Starts the adb host side server.
	 * @return true if success
	 */
	bool startAdb();

	/**
	 * Stops the adb host side server.
	 * @return true if success
	 */
	bool stopAdb();

	/**
	 * Get the stderr/stdout outputs of a process and return when the process is done.
	 * Both <b>must</b> be read or the process will block on windows.
	 * @param process The process to get the ouput from
	 * @param errorOutput The array to store the stderr output. cannot be null.
	 * @param stdOutput The array to store the stdout output. cannot be null.
	 * @param displayStdOut If true this will display stdout as well
	 * @param waitforReaders if true, this will wait for the reader threads.
	 * @return the process return code.
	 * @throws InterruptedException
	 */
#ifdef CLIENT_SUPPORT
	int grabProcessOutput(const Poco::ProcessHandle &adbHandle, Poco::Pipe *adbStdErr, Poco::Pipe *adbStdOut,
			std::vector<std::string> &errorOutput, std::vector<std::string> &stdOutput, bool waitforReaders);
#else
	int grabProcessOutput(const DetachedProcessHandle &adbHandle, Poco::Pipe *adbStdErr, Poco::Pipe *adbStdOut,
			std::vector<std::string> &errorOutput, std::vector<std::string> &stdOutput, bool waitforReaders);
#endif

	/**
	 * Returns the singleton lock used by this class to protect any access to the listener.
	 * <p/>
	 * This includes adding/removing listeners, but also notifying listeners of new bridges,
	 * devices, and clients.
	 */
	static Poco::Mutex &getLock();

	virtual ~AndroidDebugBridge();

	/**
	 * Notify the listener of a new {@link Device}.
	 * <p/>
	 * The notification of the listeners is done in a synchronized block. It is important to
	 * expect the listeners to potentially access various methods of {@link Device} as well as
	 * {@link #getDevices()} which use internal locks.
	 * <p/>
	 * For this reason, any call to this method from a method of {@link DeviceMonitor},
	 * {@link Device} which is also inside a synchronized block, should first synchronize on
	 * the {@link AndroidDebugBridge} lock. Access to this lock is done through {@link #getLock()}.
	 * @param device the new <code>Device</code>.
	 * @see #getLock()
	 */
	void deviceConnected(std::tr1::shared_ptr<Device> device);

	/**
	 * Notify the listener of a disconnected {@link Device}.
	 * <p/>
	 * The notification of the listeners is done in a synchronized block. It is important to
	 * expect the listeners to potentially access various methods of {@link Device} as well as
	 * {@link #getDevices()} which use internal locks.
	 * <p/>
	 * For this reason, any call to this method from a method of {@link DeviceMonitor},
	 * {@link Device} which is also inside a synchronized block, should first synchronize on
	 * the {@link AndroidDebugBridge} lock. Access to this lock is done through {@link #getLock()}.
	 * @param device the disconnected <code>Device</code>.
	 * @see #getLock()
	 */
	void deviceDisconnected(std::tr1::shared_ptr<Device> device);

	/**
	 * Notify the listener of a modified {@link Device}.
	 * <p/>
	 * The notification of the listeners is done in a synchronized block. It is important to
	 * expect the listeners to potentially access various methods of {@link Device} as well as
	 * {@link #getDevices()} which use internal locks.
	 * <p/>
	 * For this reason, any call to this method from a method of {@link DeviceMonitor},
	 * {@link Device} which is also inside a synchronized block, should first synchronize on
	 * the {@link AndroidDebugBridge} lock. Access to this lock is done through {@link #getLock()}.
	 * @param device the modified <code>Device</code>.
	 * @see #getLock()
	 */
	void deviceChanged(std::tr1::shared_ptr<Device> device, int changeMask);

	static Poco::Net::SocketReactor &getReactor();

	std::tr1::shared_ptr<Device> findDeviceBySerial(const std::string &serial);

	std::string connectToNetworkDevice(const std::string &address);
	std::string disconnectFromNetworkDevice(const std::string &address);
	
	static void setADBLocation(const std::string &location);
};

} /* namespace ddmlib */
#endif /* ANDROIDDEBUGBRIDGE_H_ */
