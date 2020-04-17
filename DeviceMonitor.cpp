/*
 * DeviceMonitor.cpp
 *
 *  Created on: Jan 30, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "DeviceMonitor.hpp"
#include "Log.hpp"
#ifdef CLIENT_SUPPORT
#include "Debugger.hpp"
#include "Client.hpp"
#include "ClientData.hpp"
#include "DebugPortManager.hpp"
#endif
#include "AdbHelper.hpp"
#include "AndroidDebugBridge.hpp"
#include "GetPropReceiver.hpp"
#include "ShellCommandUnresponsiveException.hpp"
#include "AdbCommandRejectedException.hpp"
#include "EmulatorConsole.hpp"
#include "Log.hpp"

namespace ddmlib {

void DeviceMonitor::waitABit() {
	try {
		Poco::Thread::sleep(1000);
	} catch (...) {
	}
}

std::tr1::shared_ptr<Poco::Net::StreamSocket> DeviceMonitor::openAdbConnection() {
	Log::d("DeviceMonitor", "Connecting to adb for Device List Monitoring...");

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChannel;
	try {
		adbChannel = std::tr1::shared_ptr<Poco::Net::StreamSocket>(new Poco::Net::StreamSocket(AndroidDebugBridge::getSocketAddress()));
		adbChannel->setNoDelay(true);
	} catch (Poco::IOException &e) {
	}

	return adbChannel;
}

bool DeviceMonitor::sendDeviceListMonitoringRequest() {
	std::vector<unsigned char> request = AdbHelper::formAdbRequest("host:track-devices"); 

	try {
		AdbHelper::write(mMainAdbConnection, request);

		AdbHelper::AdbResponse resp = AdbHelper::readAdbResponse(mMainAdbConnection, false /* readDiagString */);

		if (!resp.okay) {
			// request was refused by adb!
			Log::e("DeviceMonitor", "adb refused request: " + resp.message);
		}

		return resp.okay;
	} catch (Poco::IOException &e) {
		Log::e("DeviceMonitor", "Sending Tracking request failed!");
		mMainAdbConnection->close();
		throw;
	}
	return false;
}

void DeviceMonitor::deviceMonitorLoop() {
	do {
		try {
			if (mMainAdbConnection == nullptr) {
				Log::d("DeviceMonitor", "Opening adb connection");
				mMainAdbConnection = openAdbConnection();
				if (mMainAdbConnection == nullptr) {
					mConnectionAttempt++;
					Log::e("DeviceMonitor", "Connection attempts: " + Poco::NumberFormatter::format(mConnectionAttempt));
					if (mConnectionAttempt > 3) {
						if (mServer.lock()->startAdb() == false) {
							mRestartAttemptCount++;
							Log::e("DeviceMonitor", "adb restart attempts: " + Poco::NumberFormatter::format(mRestartAttemptCount));
						} else {
							mRestartAttemptCount = 0;
						}
					}
					waitABit();
				} else {
					Log::d("DeviceMonitor", "Connected to adb for device monitoring");
					mConnectionAttempt = 0;
				}
			}

			if (mMainAdbConnection != nullptr && !mMonitoring) {
				mMonitoring = sendDeviceListMonitoringRequest();
			}

			if (mMonitoring) {
				// read the length of the incoming message
				mMainAdbConnection->setBlocking(true);
				int length = readLength(mMainAdbConnection, mLengthBufferSize);

				if (length >= 0) {
					// read the incoming message
					processIncomingDeviceData(length);

					// flag the fact that we have build the list at least once.
					mInitialDeviceListDone = true;
				}
			}
		/*        } catch (AsynchronousCloseException ace) {
		 // this happens because of a call to Quit. We do nothing, and the loop will break.*/
		} catch (Poco::TimeoutException &ioe) {
			//handleExceptionInMonitorLoop(ioe);
		} catch (Poco::IOException &ioe) {
			handleExceptionInMonitorLoop(ioe);
		} catch (std::exception &ex) {
			handleExceptionInMonitorLoop(ex);
		}
	} while (!mQuit);
}

void DeviceMonitor::handleExceptionInMonitorLoop(std::exception & e) {
	if (mQuit == false) {
		class Notifier : public Poco::Task {
		private:
			std::tr1::shared_ptr<AndroidDebugBridge> mServer;
			std::tr1::shared_ptr<Device> mDevice;
		public:
			Notifier(std::tr1::shared_ptr<AndroidDebugBridge> server,
				std::tr1::shared_ptr<Device> device) : Poco::Task("Device disconnected: " + device->getSerialNumber()), mServer(server), mDevice(device) {};

			void runTask() {
				mServer->deviceDisconnected(mDevice);
			}
		};
		std::vector< std::tr1::shared_ptr< Notifier > > notifiers;
		Log::e("DeviceMonitor", std::string("Adb connection Error:") + e.what());
		mMonitoring = false;
		if (mMainAdbConnection != nullptr) {
			try {
				mMainAdbConnection->close();
			} catch (Poco::IOException &ioe) {
				// we can safely ignore that one.
			}
			mMainAdbConnection.reset();

			// remove all devices from list
			// because we are going to call mServer.deviceDisconnected which will acquire this
			// lock we lock it first, so that the AndroidDebugBridge lock is always locked
			// first.
			{
				Poco::ScopedLock<Poco::Mutex> lock(AndroidDebugBridge::getLock());
				{
					Poco::ScopedLock<Poco::Mutex> lock2(mDevicesLock);
					int n = mDevices.size();
					while (mDevices.size() != 0) {
						std::tr1::shared_ptr<Device> device = mDevices[--n];
						removeDevice(device);
						notifier.start(new Notifier(mServer.lock(), device));
					}
				}
			}
		}
	}
}

void DeviceMonitor::processIncomingDeviceData(int length) {
	std::vector<std::tr1::shared_ptr<Device> > list;

	list.clear();
	if (length > 0) {
		std::string result = read(mMainAdbConnection, length);

		Poco::StringTokenizer devices(result, "\n");

		for (Poco::StringTokenizer::Iterator d = devices.begin(); d != devices.end(); ++d) {
			Poco::StringTokenizer param(*d, "\t");
			if (param.count() == 2) {
				// new adb uses only serial numbers to identify devices
				//add the device to the list
				list.push_back(std::tr1::shared_ptr<Device>(new Device(shared_from_this(), param[0] /*serialnumber*/, param[1])));
			}
		}
	}

	// now merge the new devices with the old ones.
	updateDevices(list);
}

void DeviceMonitor::updateDevices(std::vector<std::tr1::shared_ptr<Device> > & newList) {
	std::vector<std::tr1::shared_ptr<Device> > toSkipList;
	std::vector<std::tr1::shared_ptr<Device> > toSkipListOld;
	std::vector<std::tr1::shared_ptr<Device> > disconnectedDevices;
	std::vector<std::tr1::shared_ptr<Device> > connectedDevices;
	std::vector<std::tr1::shared_ptr<Device> > changedDevices;
	// array to store the devices that must be queried for information.
	// it's important to not do it inside the synchronized loop as this could block
	// the whole workspace (this lock is acquired during build too).
	std::vector<std::tr1::shared_ptr<Device> > devicesToQuery;

	if (mQuit)
		return;

	// For each device in the current list, we look for a matching the new list.
	// * if we find it, we update the current object with whatever new information
	//   there is
	//   (mostly state change, if the device becomes ready, we query for build info).
	//   We also remove the device from the new list to mark it as "processed"
	// * if we do not find it, we remove it from the current list.
	// Once this is done, the new list contains device we aren't monitoring yet, so we
	// add them to the list, and start monitoring them.
	{	
		// because we are going to call mServer.deviceDisconnected which will acquire this lock
		// we lock it first, so that the AndroidDebugBridge lock is always locked first.
		Poco::ScopedLock<Poco::Mutex> adbLock(AndroidDebugBridge::getLock());
		Poco::ScopedLock<Poco::Mutex> devLock(mDevicesLock);
		if (mQuit)
			return;

		Log::v("DeviceMonitor", "Updating devices");
		std::vector<std::tr1::shared_ptr<Device> >::iterator deviceIt = mDevices.begin();
		while (deviceIt != mDevices.end()) {
			std::tr1::shared_ptr<Device> device = *deviceIt;
			if (std::count(toSkipListOld.begin(), toSkipListOld.end(), device) > 0) {
				++deviceIt;
				continue;
			}

			// look for a similar device in the new list.
			//size_t count = newList.size();
			bool foundMatch = false;
			for (std::vector<std::tr1::shared_ptr<Device> >::iterator newDevice = newList.begin(); newDevice != newList.end(); ++newDevice) {
				if (std::count(toSkipList.begin(), toSkipList.end(), *newDevice) > 0)
					continue;

				if ((*newDevice)->getSerialNumber() == device->getSerialNumber()) {
					foundMatch = true;

					// update the state if needed.
					if (device->getState() != (*newDevice)->getState()) {
						device->setState((*newDevice)->getState());
						// ugly, but need to unlock to avoid deadlock with device change listeners that are going to try to create bridge
						changedDevices.push_back(device);

						// if the device just got ready/online, we need to start
						// monitoring it.
						if (device->isOnline()) {
#ifdef CLIENT_SUPPORT
							if (AndroidDebugBridge::getClientSupport()) {
								if (!startMonitoringDevice(device)) {
									Log::e("DeviceMonitor", "Failed to start monitoring " + device->getSerialNumber());
								}
							}
#endif 

							if (device->getPropertyCount() == 0) {
								devicesToQuery.push_back(device);
							}
						}
					}

					// remove the new device from the list since it's been used
					toSkipList.push_back(*newDevice);
					break;
				}
			}

			if (!foundMatch) {
				// the device is gone, we need to remove it, and keep current index
				// to process the next one.
				removeDevice(device);
				disconnectedDevices.push_back(device);
				deviceIt = mDevices.begin();
			} else {
				// process the next one
				++deviceIt;
			}
			toSkipListOld.push_back(device);
		}

		// at this point we should still have some new devices in newList, so we
		// process them.
		for (std::vector<std::tr1::shared_ptr<Device> >::iterator newDevice = newList.begin(); newDevice != newList.end(); ++newDevice) {
			if (std::count(toSkipList.begin(), toSkipList.end(), *newDevice) > 0)
				continue;
			// add them to the list
			mDevices.push_back(*newDevice);
			connectedDevices.push_back(*newDevice);

#ifdef CLIENT_SUPPORT
			// start monitoring them.
			if (AndroidDebugBridge::getClientSupport()) {
				if ((*newDevice)->isOnline()) {
					startMonitoringDevice(*newDevice);
				}
			}
#endif

			// look for their build info.
			if ((*newDevice)->isOnline()) {
				devicesToQuery.push_back(*newDevice);
			}
		}
	}
	// Notify device change listeners
	for (std::vector<std::tr1::shared_ptr<Device> >::iterator device = disconnectedDevices.begin(); device != disconnectedDevices.end(); ++device) {
		mServer.lock()->deviceDisconnected(*device);
	}

	for (std::vector<std::tr1::shared_ptr<Device> >::iterator device = changedDevices.begin(); device != changedDevices.end(); ++device) {
		(*device)->update(Device::CHANGE_STATE);
	}

	for (std::vector<std::tr1::shared_ptr<Device> >::iterator device = connectedDevices.begin(); device != connectedDevices.end(); ++device) {
		mServer.lock()->deviceConnected(*device);
	}

	// query the new devices for info.
	for (std::vector<std::tr1::shared_ptr<Device> >::iterator d = devicesToQuery.begin(); d != devicesToQuery.end(); ++d) {
		queryNewDeviceForInfo(*d);
	}

	newList.clear();
}

void DeviceMonitor::removeDevice(std::tr1::shared_ptr<Device> device) {
	Poco::ScopedLock<Poco::Mutex> lock(mDevicesLock);
#ifdef CLIENT_SUPPORT
	device->clearClientList();
#endif
	std::vector<std::tr1::shared_ptr<Device> > newDeviceList;
	for (std::vector<std::tr1::shared_ptr<Device> >::iterator oldDevice = mDevices.begin(); oldDevice != mDevices.end(); ++oldDevice) {
		if ((*oldDevice)->getSerialNumber() != device->getSerialNumber()) {
			newDeviceList.push_back(*oldDevice);
		}
	}
	mDevices.clear();
	mDevices = newDeviceList;

#ifdef CLIENT_SUPPORT
	std::tr1::shared_ptr<Poco::Net::StreamSocket> channel = device->getClientMonitoringSocket();
	if (channel != nullptr) {
		try {
			channel->close();
		} catch (Poco::IOException &e) {
			// doesn't really matter if the close fails.
		}
	}
#endif
}

void DeviceMonitor::queryNewDeviceForInfo(std::tr1::shared_ptr<Device> device) {
	// TODO: do this in a separate thread.
	std::tr1::shared_ptr<GetPropReceiver> rcvr(new GetPropReceiver(device));
	try {
		// first get the list of properties.
		device->executeShellCommand(GetPropReceiver::GETPROP_COMMAND, rcvr.get());

		queryNewDeviceForMountingPoint(device, Device::MNT_EXTERNAL_STORAGE);
		queryNewDeviceForMountingPoint(device, Device::MNT_DATA);
		queryNewDeviceForMountingPoint(device, Device::MNT_ROOT);

		// now get the emulator Virtual Device name (if applicable).

		if (device->isEmulator()) {
			 std::tr1::shared_ptr< EmulatorConsole > console = EmulatorConsole::getConsole(device);
			 if (console != nullptr) {
				 device->setAvdName(console->getAvdName());
			 }
		}
	} catch (Poco::TimeoutException &e) {
		Log::w("DeviceMonitor", "Connection timeout getting info for device " + device->getSerialNumber());
	} catch (AdbCommandRejectedException &e) {
		// This should never happen as we only do this once the device is online.
		Log::w("DeviceMonitor",
				"Adb rejected command to get  device " + device->getSerialNumber() + " info: " + std::string(e.what()));
	} catch (ShellCommandUnresponsiveException &e) {
		Log::w("DeviceMonitor", "Adb shell command took too long returning info for device" + device->getSerialNumber());
	} catch (Poco::IOException &e) {
		Log::w("DeviceMonitor", "IO Error getting info for device " + device->getSerialNumber());
	}
}

void DeviceMonitor::queryNewDeviceForMountingPoint(std::tr1::shared_ptr<Device> device, const std::string & name) {
	class MounterReceiver: public MultiLineReceiver {
		std::tr1::shared_ptr<Device> mDevice;
		std::string mName;
	public:
		MounterReceiver(std::tr1::shared_ptr<Device> device, const std::string &name) :
				mDevice(device), mName(name) {
		}
		bool isCancelled() {
			return false;
		}

		void processNewLines(const std::vector<std::string> &lines) {
			for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
				if ((*line).length() > 0) {
					// this should be the only one.
					mDevice->setMountingPoint(mName, *line);
				}
			}
		}
	};
	std::tr1::shared_ptr<MounterReceiver> mounterReceiver(new MounterReceiver(device, name));
	device->executeShellCommand("echo $" + name, mounterReceiver.get());
}

#ifdef CLIENT_SUPPORT
bool DeviceMonitor::startMonitoringDevice(std::tr1::shared_ptr<Device> device) {
	std::tr1::shared_ptr<Poco::Net::StreamSocket> socketChannel = openAdbConnection();

	if (socketChannel != nullptr) {
		try {
			bool result = sendDeviceMonitoringRequest(socketChannel, device);
			if (result) {

				if (!(mDeviceClientThread->isRunning())) {
					startDeviceMonitorThread();
				}

				device->setClientMonitoringSocket(socketChannel);

				{
					Poco::ScopedLock<Poco::Mutex> lock(mDevicesLock);
					// always wakeup before doing the register. The synchronized block
					// ensure that the selector won't select() before the end of this block.
					// @see deviceClientMonitorLoop
					// mSelector.wakeup();

					//socketChannel->configureBlocking(false);
					//socketChannel->register(mSelector, SelectionKey.OP_READ, device);
					device->registerInReactor();
				}

				return true;
			}
		} catch (Poco::TimeoutException &e) {
			try {
				// attempt to close the socket if needed.
				socketChannel->close();
			} catch (Poco::IOException &e1) {
				// we can ignore that one. It may already have been closed.
			}
			Log::d("DeviceMonitor",
					"Connection Failure when starting to monitor device '" + device->toString() + "' : timeout");
		} catch (AdbCommandRejectedException &e) {
			try {
				// attempt to close the socket if needed.
				socketChannel->close();
			} catch (Poco::IOException &e1) {
				// we can ignore that one. It may already have been closed.
			}
			Log::d("DeviceMonitor", "Adb refused to start monitoring device '" + device->toString() + "' : " + e.what());
		} catch (Poco::IOException &e) {
			try {
				// attempt to close the socket if needed.
				socketChannel->close();
			} catch (Poco::IOException &e1) {
				// we can ignore that one. It may already have been closed.
			}
			Log::d("DeviceMonitor",
					"Connection Failure when starting to monitor device '" + device->toString() + "' : " + e.what());
		}
	}

	return false;
}

void DeviceMonitor::startDeviceMonitorThread() {
	mDeviceClientThread->setName("DDMLib device client thread");
	mDeviceClientThread->start(*raDeviceClientMonitorThread);
}

void DeviceMonitor::deviceClientMonitorLoop() {
	do {
		try {
			// This synchronized block stops us from doing the select() if a new
			// Device is being added.
			// @see startMonitoringDevice()
			mDevicesLock.lock();
			mDevicesLock.unlock();

			//int count = mSelector.select();

			if (mQuit) {
				return;
			}

			{
				Poco::ScopedLock<Poco::Mutex> lock(mClientsLock);
				if (mClientsToReopen.size() > 0) {
					for (std::map<std::tr1::shared_ptr<Client>, int>::iterator clientPorts = mClientsToReopen.begin(); clientPorts != mClientsToReopen.end(); ++clientPorts) {
						std::tr1::shared_ptr<Client> client = (*clientPorts).first;
						std::tr1::shared_ptr<Device> device = client->getDevice();
						int pid = client->getClientData()->getPid();

						client->dropClient(false /* notify */);

						// This is kinda bad, but if we don't wait a bit, the client
						// will never answer the second handshake!
						waitABit();

						int port = (*clientPorts).second;
						if (port == IDebugPortProvider::NO_STATIC_PORT) {
							port = getNextDebuggerPort();
						}
						Log::d("DeviceMonitor", "Reopening " + client->toString());
						openClient(device, pid, port);
						device->update(Device::CHANGE_CLIENT_LIST);
					}

					mClientsToReopen.clear();
				} else {
					Poco::Thread::sleep(50);
				}
			}

		} catch (Poco::IOException &e) {
			if (!mQuit) {

			}
		}

	} while (!mQuit);
}

bool DeviceMonitor::sendDeviceMonitoringRequest(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket,
		std::tr1::shared_ptr<Device> device) {
	try {
		AdbHelper::setDevice(socket, device.get());

		std::vector<unsigned char> request = AdbHelper::formAdbRequest("track-jdwp"); 

		AdbHelper::write(socket, request);

		AdbHelper::AdbResponse resp = AdbHelper::readAdbResponse(socket, false /* readDiagString */);

		if (!resp.okay) {
			// request was refused by adb!
			Log::e("DeviceMonitor", "adb refused request: " + resp.message);
		}

		return resp.okay;
	} catch (Poco::TimeoutException &e) {
		Log::e("DeviceMonitor", "Sending jdwp tracking request timed out!");
		throw;
	} catch (Poco::IOException &e) {
		Log::e("DeviceMonitor", "Sending jdwp tracking request failed!");
		throw;
	}
	return false;
}

void DeviceMonitor::processIncomingJdwpData(std::tr1::shared_ptr<Device> device,
		std::tr1::shared_ptr<Poco::Net::StreamSocket> monitorSocket, int length) {
	if (length >= 0) {
		// array for the current pids.
		std::vector<int> pidList;

		// get the string data if there are any
		if (length > 0) {
			std::string result = read(monitorSocket, length);

			// split each line in its own list and create an array of integer pid
			Poco::StringTokenizer pids(result, "\n");

			for (Poco::StringTokenizer::Iterator pid = pids.begin(); pid != pids.end(); ++pid) {
				try {
					pidList.push_back(Poco::NumberParser::parse(*pid));
				} catch (Poco::SyntaxException &nfe) {
					// looks like this pid is not really a number. Lets ignore it.
					continue;
				}
			}
		}

		// Now we merge the current list with the old one.
		// this is the same mechanism as the merging of the device list.

		// For each client in the current list, we look for a matching the pid in the new list.
		// * if we find it, we do nothing, except removing the pid from its list,
		//   to mark it as "processed"
		// * if we do not find any match, we remove the client from the current list.
		// Once this is done, the new list contains pids for which we don't have clients yet,
		// so we create clients for them, add them to the list, and start monitoring them.

		std::vector<std::tr1::shared_ptr<Client> > clients = device->getClientList();

		bool changed = false;

		// because MonitorThread#dropClient acquires first the monitorThread lock and then the
		// Device client list lock (when removing the Client from the list), we have to make
		// sure we acquire the locks in the same order, since another thread (MonitorThread),
		// could call dropClient itself.
		{
			Poco::ScopedLock<Poco::Mutex> lock(mClientsLock);

			for (unsigned int c = 0; c < clients.size();) {
				std::tr1::shared_ptr<Client> client = clients[c];
				int pid = client->getClientData()->getPid();

				// look for a matching pid
				int match = -1;
				for (std::vector<int>::iterator matchingPid = pidList.begin(); matchingPid != pidList.end(); ++matchingPid) {
					if (pid == *matchingPid) {
						match = *matchingPid;
						break;
					}
				}

				if (match != -1) {
					pidList.erase(std::find(pidList.begin(), pidList.end(), match));
					c++; // move on to the next client.
				} else {
					// we need to drop the client. the client will remove itself from the
					// list of its device which is 'clients', so there's no need to
					// increment c.
					// We ask the monitor thread to not send notification, as we'll do
					// it once at the end.
					client->dropClient(false /* notify */);
					changed = true;
					++c;
				}
			}
		}

		// at this point whatever pid is left in the list needs to be converted into Clients.
		for (std::vector<int>::iterator newPid = pidList.begin(); newPid != pidList.end(); ++newPid) {
			openClient(device, *newPid, getNextDebuggerPort());
			changed = true;
		}

		if (changed) {
			mServer.lock()->deviceChanged(device, Device::CHANGE_CLIENT_LIST);
		}
	}
}

void DeviceMonitor::openClient(std::tr1::shared_ptr<Device> device, int pid, int port) {
	std::tr1::shared_ptr<Poco::Net::StreamSocket> clientSocket;
	try {
		clientSocket = AdbHelper::createPassThroughConnection(AndroidDebugBridge::getSocketAddress(), device, pid);

		// required for Selector
		// clientSocket.configureBlocking(false);
	} catch (Poco::Net::DNSException &uhe) {
		Log::d("DeviceMonitor", "Unknown Jdwp pid: " + Poco::NumberFormatter::format(pid));
		return;
	} catch (Poco::TimeoutException &e) {
		Log::w("DeviceMonitor", "Failed to connect to client '" + Poco::NumberFormatter::format(pid) + "': timeout");
		return;
	} catch (AdbCommandRejectedException &e) {
		Log::w("DeviceMonitor",
				"Adb rejected connection to client '" + Poco::NumberFormatter::format(pid) + "': " + std::string(e.what()));
		return;

	} catch (Poco::IOException &ioe) {
		Log::w("DeviceMonitor",
				"Failed to connect to client '" + Poco::NumberFormatter::format(pid) + "': " + std::string(ioe.what()));
		return;
	}

	createClient(device, pid, clientSocket, port);
}

void DeviceMonitor::createClient(std::tr1::shared_ptr<Device> device, int pid, std::tr1::shared_ptr<Poco::Net::StreamSocket> socket,
		int debuggerPort) {
	/*
	 * Successfully connected to something. Create a Client object, add
	 * it to the list, and initiate the JDWP handshake.
	 */

	std::tr1::shared_ptr<Client> client(new Client(device, socket, pid));

	if (client->sendHandshake()) {
		try {
			if (AndroidDebugBridge::getClientSupport()) {
				client->listenForDebugger(debuggerPort);
			}
		} catch (Poco::IOException &ioe) {
			client->getClientData()->setDebuggerInterest(DebuggerStatusERROR);
			Log::e("ddms", "Can't bind to local " + Poco::NumberFormatter::format(debuggerPort) + " for debugger");
			// oh well
		}

		client->requestAllocationStatus();
	} else {
		Log::e("ddms", "Handshake with " + client->toString() + " failed!");
		/*
		 * The handshake send failed. We could remove it now, but if the
		 * failure is "permanent" we'll just keep banging on it and
		 * getting the same result. Keep it in the list with its "error"
		 * state so we don't try to reopen it.
		 */
	}

	if (client->isValid()) {
		device->addClient(client);
		client->registerInReactor();
		std::tr1::shared_ptr<Debugger> debugger = client->getDebugger();
		debugger->registerInReactor();
	} else {
		client.reset();
	}
}

int DeviceMonitor::getNextDebuggerPort() {
	Poco::ScopedLock<Poco::Mutex> lock(mDebuggerPortsLock);
	if (!mDebuggerPorts.empty()) {
		int port = mDebuggerPorts[0];

		// remove it.
		mDebuggerPorts.erase(mDebuggerPorts.begin());

		// if there's nothing left, add the next port to the list
		if (mDebuggerPorts.size() == 0) {
			mDebuggerPorts.push_back(port + 1);
		}

		return port;
	}
	return -1;
}
#endif

int DeviceMonitor::readLength(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size) {
	std::string msg = read(socket, size);

	if (!msg.empty()) {
		try {
			return Poco::NumberParser::parseHex(msg);
		} catch (Poco::SyntaxException &nfe) {
			// we'll throw an exception below.
		}
	}

	// we receive something we can't read. It's better to reset the connection at this point.
	throw Poco::IOException("Unable to read length");
}

std::string DeviceMonitor::read(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size) {
	//ByteBuffer *buf = ByteBuffer::wrap(buffer, size);
	char *buffer = new char[size];
	unsigned int count = 0;

	while (count < size) {
		int bytesRead = socket->receiveBytes(buffer + count, size - count);
		count += bytesRead;
		if (bytesRead <= 0) {
			throw Poco::IOException("EOF");
		}
		DeviceMonitor::waitABit();
	}

	//try {
	return std::string(buffer, size);
	/*} catch (UnsupportedEncodingException e) {
	 // we'll return null below.
	 }*/

	//return "";
}

DeviceMonitor::DeviceMonitor(std::tr1::shared_ptr<AndroidDebugBridge> androidDebugBridge) :
		mQuit(false), mServer(androidDebugBridge), mMonitoring(false), mConnectionAttempt(0), mRestartAttemptCount(0), mInitialDeviceListDone(
				false), mMonitorThread(new Poco::Thread("Device List Monitor")), raMonitorThread(
				new Poco::RunnableAdapter<DeviceMonitor>(*this, &DeviceMonitor::deviceMonitorLoop))
#ifdef CLIENT_SUPPORT
				,
				mDeviceClientThread(
				new Poco::Thread("Device Client Monitor")),
				raDeviceClientMonitorThread(
				new Poco::RunnableAdapter<DeviceMonitor>(*this, &DeviceMonitor::deviceClientMonitorLoop)) 
#endif
		{
	for (int i = 0; i != mLengthBufferSize; ++i) {
		mLengthBuffer[i] = 0;
		mLengthBuffer2[i] = 0;
	}

#ifdef CLIENT_SUPPORT
	mDebuggerPorts.push_back(DdmPreferences::getDebugPortBase());
#endif
}

void DeviceMonitor::start() {
	mMonitorThread->start(*raMonitorThread);
}

void DeviceMonitor::stop() {
	Poco::ScopedLock<Poco::Mutex> bridgeLock(AndroidDebugBridge::getLock());
	Poco::ScopedLock<Poco::Mutex> devLock(mDevicesLock);
	if (mQuit)
		return;
	mQuit = true;

	// wakeup the main loop thread by closing the main connection to adb.
	try {
		if (mMainAdbConnection != nullptr)
			mMainAdbConnection->close();
	} catch (Poco::IOException &e1) {
	}
	Log::d("ddms", "Waiting for Monitor thread");
	mMonitorThread->join();
	Log::d("ddms", "Monitor thread stopped");
#ifdef CLIENT_SUPPORT
	mDeviceClientThread->join();
	Log::d("ddms", "Device client thread stopped");
	mClientsToReopen.clear();
	for (std::vector<std::tr1::shared_ptr<Device> >::iterator device = mDevices.begin(); device != mDevices.end(); ++device) {
		(*device)->clearClientList();
	}
#endif
	mDevices.clear();
	mMainAdbConnection.reset();
}

bool DeviceMonitor::isMonitoring() {
	return mMonitoring;
}

std::vector<std::tr1::shared_ptr<Device> > DeviceMonitor::getDevices() {
	Poco::ScopedLock<Poco::Mutex> lock(mDevicesLock);
	return mDevices;
}

bool DeviceMonitor::hasInitialDeviceList() {
	return mInitialDeviceListDone;
}

std::tr1::shared_ptr<AndroidDebugBridge> DeviceMonitor::getServer() {
	return mServer.lock();
}

#ifdef CLIENT_SUPPORT
void DeviceMonitor::addClientToDropAndReopen(std::tr1::shared_ptr<Client> client, int port) {
	Poco::ScopedLock<Poco::Mutex> lock(mClientsLock);
	Log::d("DeviceMonitor",
			"Adding " + client->toString() + " to list of client to reopen (" + Poco::NumberFormatter::format(port) + ").");
	if (mClientsToReopen[client] == 0) {
		mClientsToReopen[client] = port;
	}
}

void DeviceMonitor::acceptNewDebugger(std::tr1::shared_ptr<Debugger> dbg, std::tr1::shared_ptr<Poco::Net::ServerSocket> acceptChan) {
	// TODO: wow, i forgot this one
}

void DeviceMonitor::addPortToAvailableList(int port) {
	if (port > 0) {
		{
			Poco::ScopedLock<Poco::Mutex> lock(mDebuggerPortsLock);
			// because there could be case where clients are closed twice, we have to make
			// sure the port number is not already in the list.
			if (std::find(mDebuggerPorts.begin(), mDebuggerPorts.end(), port) == mDebuggerPorts.end()) {
				// add the port to the list while keeping it sorted. It's not like there's
				// going to be tons of objects so we do it linearly.
				int count = mDebuggerPorts.size();
				for (int i = 0; i < count; i++) {
					if (port < mDebuggerPorts[i]) {
						mDebuggerPorts.insert(mDebuggerPorts.begin() + i, port);
						break;
					}
				}
				// TODO: check if we can compact the end of the list.
			}
		}
	}
}
#endif


std::tr1::shared_ptr<Device> DeviceMonitor::findDeviceBySerial(const std::string &serial) {
	Poco::ScopedLock<Poco::Mutex> lock(mDevicesLock);
	std::tr1::shared_ptr<Device> foundDevice;
	for (std::vector<std::tr1::shared_ptr<Device> >::iterator device = mDevices.begin(); device != mDevices.end(); device++) {
		if ((*device)->getSerialNumber() == serial) {
			foundDevice = *device;
			break;
		}
	}
	return foundDevice;
}

} /* namespace ddmlib */
