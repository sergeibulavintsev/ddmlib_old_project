/*
 * DeviceMonitor.hpp
 *
 *  Created on: Jan 30, 2012
 *      Author: Ilya Polenov
 */

#ifndef DEVICEMONITOR_HPP_
#define DEVICEMONITOR_HPP_

#include "ddmlib.hpp"

namespace ddmlib {

class AndroidDebugBridge;
class Device;
#ifdef CLIENT_SUPPORT
class Debugger;
class Client;
#endif

/**
 * A Device monitor. This connects to the Android Debug Bridge and get device and
 * debuggable process information from it.
 */
class DDMLIB_LOCAL DeviceMonitor: public std::tr1::enable_shared_from_this<DeviceMonitor> {

	static const size_t mLengthBufferSize = 4;

	unsigned char mLengthBuffer[mLengthBufferSize];
	unsigned char mLengthBuffer2[mLengthBufferSize];

	bool mQuit;

	std::tr1::weak_ptr<AndroidDebugBridge> mServer;

	std::tr1::shared_ptr<Poco::Net::StreamSocket> mMainAdbConnection;
	bool mMonitoring;
	int mConnectionAttempt;
	int mRestartAttemptCount;
	bool mInitialDeviceListDone;

	std::vector<std::tr1::shared_ptr<Device> > mDevices;
	Poco::Mutex mDevicesLock;
#ifdef CLIENT_SUPPORT
	std::vector<int> mDebuggerPorts;
	Poco::Mutex mDebuggerPortsLock;
	std::map<std::tr1::shared_ptr<Client>, int> mClientsToReopen;
	Poco::Mutex mClientsLock;

	std::tr1::shared_ptr<Poco::Thread> mDeviceClientThread;
	std::tr1::shared_ptr<Poco::RunnableAdapter<DeviceMonitor> > raDeviceClientMonitorThread;
#endif

	Poco::Mutex mLock;

	std::tr1::shared_ptr<Poco::Thread> mMonitorThread;
	std::tr1::shared_ptr<Poco::RunnableAdapter<DeviceMonitor> > raMonitorThread;

	Poco::TaskManager notifier;

	/**
	 * Sleeps for a little bit.
	 */
	void waitABit();

	/**
	 * Attempts to connect to the debug bridge server.
	 * @return a connect socket if success, null otherwise
	 */
	std::tr1::shared_ptr<Poco::Net::StreamSocket> openAdbConnection();

	/**
	 *
	 * @return
	 * @throws IOException
	 */
	bool sendDeviceListMonitoringRequest();

	/**
	 * Monitors the devices. This connects to the Debug Bridge
	 */
	void deviceMonitorLoop();

	void handleExceptionInMonitorLoop(std::exception &e); // was handleExpectioninMonitorLoop in java. Srsly?

	/**
	 * Processes an incoming device message from the socket
	 * @param socket
	 * @param length
	 * @throws IOException
	 */
	void processIncomingDeviceData(int length);

	/**
	 *  Updates the device list with the new items received from the monitoring service.
	 */
	void updateDevices(std::vector<std::tr1::shared_ptr<Device> > &newList);

	void removeDevice(std::tr1::shared_ptr<Device> device);

	/**
	 * Queries a device for its build info.
	 * @param device the device to query.
	 */
	void queryNewDeviceForInfo(std::tr1::shared_ptr<Device> device);

	void queryNewDeviceForMountingPoint(std::tr1::shared_ptr<Device> device, const std::string &name);

	void startDeviceMonitorThread();

#ifdef CLIENT_SUPPORT
	void deviceClientMonitorLoop();

	bool sendDeviceMonitoringRequest(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, std::tr1::shared_ptr<Device> device);

	/**
	 * Opens and creates a new client.
	 * @return
	 */
	void openClient(std::tr1::shared_ptr<Device> device, int pid, int port);

	/**
	 * Creates a client and register it to the monitor thread
	 * @param device
	 * @param pid
	 * @param socket
	 * @param debuggerPort the debugger port.
	 * @param monitorThread the {@link MonitorThread} object.
	 */
	void createClient(std::tr1::shared_ptr<Device> device, int pid, std::tr1::shared_ptr<Poco::Net::StreamSocket> socket,
			int debuggerPort);

	int getNextDebuggerPort();
#endif
	/**
	 * Reads the length of the next message from a socket.
	 * @param socket The {@link SocketChannel} to read from.
	 * @return the length, or 0 (zero) if no data is available from the socket.
	 * @throws IOException if the connection failed.
	 */
	int readLength(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size);

	/**
	 * Fills a buffer from a socket.
	 * @param socket
	 * @param buffer
	 * @return the content of the buffer as a string, or null if it failed to convert the buffer.
	 * @throws IOException
	 */
	std::string read(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size);

public:
	virtual ~DeviceMonitor() {
	}

	/**
	 * Creates a new {@link DeviceMonitor} object and links it to the running
	 * {@link AndroidDebugBridge} object.
	 * @param server the running {@link AndroidDebugBridge}.
	 */
	DeviceMonitor(std::tr1::shared_ptr<AndroidDebugBridge> androidDebugBridge);

	/**
	 * Starts the monitoring.
	 */
	void start();

	/**
	 * Stops the monitoring.
	 */
	void stop();

	/**
	 * Returns if the monitor is currently connected to the debug bridge server.
	 * @return
	 */
	bool isMonitoring();

	/**
	 * Returns the devices.
	 */
	std::vector<std::tr1::shared_ptr<Device> > getDevices();

	bool hasInitialDeviceList();

	std::tr1::shared_ptr<AndroidDebugBridge> getServer();

#ifdef CLIENT_SUPPORT
	void addClientToDropAndReopen(std::tr1::shared_ptr<Client> client, int port);

	/*
	 * Accept a new connection from a debugger. If successful, register it with
	 * the Selector.
	 */
	void acceptNewDebugger(std::tr1::shared_ptr<Debugger> dbg, std::tr1::shared_ptr<Poco::Net::ServerSocket> acceptChan);

	void addPortToAvailableList(int port);
#endif

	int getConnectionAttemptCount() {
		return mConnectionAttempt;
	}

	int getRestartAttemptCount() {
		return mRestartAttemptCount;
	}

	void processIncomingJdwpData(std::tr1::shared_ptr<Device> device, std::tr1::shared_ptr<Poco::Net::StreamSocket> monitorSocket,
			int length);

	/**
	 * Starts a monitoring service for a device.
	 * @param device the device to monitor.
	 * @return true if success.
	 */
	bool startMonitoringDevice(std::tr1::shared_ptr<Device> device);

	std::tr1::shared_ptr<Device> findDeviceBySerial(const std::string &serial);
};

} /* namespace ddmlib */
#endif /* DEVICEMONITOR_HPP_ */
