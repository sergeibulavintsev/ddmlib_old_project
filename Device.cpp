/*
 * Device.cpp
 *
 *  Created on: 26.01.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "Device.hpp"
#include "Log.hpp"
#ifdef CLIENT_SUPPORT
#include "Client.hpp"
#include "ClientData.hpp"
#endif
#include "DeviceMonitor.hpp"
#include "CollectingOutputReceiver.hpp"
#include "AndroidDebugBridge.hpp"
#include "AdbHelper.hpp"
#include "AdbCommandRejectedException.hpp"
#include "InstallException.hpp"
#include "IShellOutputReceiver.hpp"
#include "NullOutputReceiver.hpp"
#include "CollectingOutputReceiver.hpp"
#include "ShellCommandUnresponsiveException.hpp"
#include "SyncService.hpp"
#include "MultiLineReceiver.hpp"
#include "RawImage.hpp"

namespace ddmlib {

const char Device::PROP_BUILD_VERSION[] = "ro.build.version.release";
const char Device::PROP_BUILD_API_LEVEL[] = "ro.build.version.sdk";
const char Device::PROP_BUILD_CODENAME[] = "ro.build.version.codename";
const char Device::PROP_DEBUGGABLE[] = "ro.debuggable";
const char Device::FIRST_EMULATOR_SN[] = "emulator-5554";
const char Device::PROP_BUILD_VERSION_NUMBER[] = "ro.build.version.sdk"; // = PROP_BUILD_API_LEVEL;

const char Device::MNT_EXTERNAL_STORAGE[] = "EXTERNAL_STORAGE";
const char Device::MNT_ROOT[] = "ANDROID_ROOT";
const char Device::MNT_DATA[] = "ANDROID_DATA";

std::string Device::BOOTLOADER("bootloader"), Device::OFFLINE("offline"), Device::ONLINE("device"),
		Device::RECOVERY("recovery");

const char Device::RE_EMULATOR_SN[] = "emulator-(\\d+)";
const char Device::LOG_TAG[] = "Device";
const char Device::InstallReceiver::SUCCESS_OUTPUT[] = "Success";
const Poco::RegularExpression Device::InstallReceiver::FAILURE_PATTERN(std::string("Failure\\s+\\[(.*)\\]"));
const Poco::RegularExpression Device::BatteryReceiver::BATTERY_LEVEL(std::string("\\s*level: (\\d+)"));
const Poco::RegularExpression Device::BatteryReceiver::SCALE(std::string("\\s*scale: (\\d+)"));
const Poco::RegularExpression Device::IPaddressReceiver::IP_ADDRESS(std::string("^(wlan|eth|ra|ath|en)\\d\\s+UP\\s+(\\d+.\\d+.\\d+.\\d+)"));
Poco::Mutex Device::sLock;
Poco::Timestamp Device::current_time;

Device::Device(std::tr1::shared_ptr<DeviceMonitor> monitor, const std::string &serialNumber, const std::string &deviceState) {
	mArePropertiesSet = false;
	mLastBatteryLevel = 0;
	mLastBatteryCheckTime = 0;
	mState = "";
	mMonitor = monitor;
	mSerialNumber = serialNumber;
	mState = deviceState;
	Log::v("ddms", "Created new device " + serialNumber);
}

Device::~Device() {
	Log::v("ddms", "Device " + toString() + " is destroyed");
#ifdef CLIENT_SUPPORT
	if (mSocketChannel != nullptr) {
		AndroidDebugBridge::getReactor().removeEventHandler(*(mSocketChannel.get()),
				Poco::NObserver<Device, Poco::Net::ReadableNotification>(*this, &Device::processDeviceReadActivity));
	}
#endif
}

void Device::InstallReceiver::processNewLines(const std::vector<std::string>& lines) {
	for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
		if (!(*line).empty()) {
			Log::v("Install", *line);
			if ((*line).find("SUCCESS_OUTPUT") == 0) {
				mErrorMessage = "";
			} else {
				std::vector<std::string> temp;
				if (FAILURE_PATTERN.split(*line, temp)) {
					mErrorMessage = temp.at(1);
				}
			}
		}
	}
}


Device::BatteryReceiver::BatteryReceiver() {
	mBatteryLevel = 0;
	mBatteryScale = 0;
}

void Device::BatteryReceiver::processNewLines(const std::vector<std::string>& lines) {
	std::vector<std::string> temp;
	for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
		if (BATTERY_LEVEL.split(*line, temp))
			try {
				mBatteryLevel = Poco::NumberParser::parse(temp.at(1));
			} catch (Poco::SyntaxException& e) {
				Log::w(LOG_TAG, "Failed to parse " + temp.at(1) + " as an integer");
			}

		if (SCALE.split(*line, temp)) {
			try {
				mBatteryScale = Poco::NumberParser::parse(temp.at(1));
			} catch (Poco::SyntaxException& e) {
				Log::w(LOG_TAG, "Failed to parse " + temp.at(1) + " as an integer");
			}
		}
	}
}

int Device::BatteryReceiver::getBatteryLevel() {
	if (mBatteryLevel != 0 && mBatteryScale != 0) {
		return (mBatteryLevel * 100) / mBatteryScale;
	}
	return 0;
}

bool Device::BatteryReceiver::isCancelled() {
	return false;
}


std::string Device::getSerialNumber() const {
	return mSerialNumber;
}

std::string Device::getAvdName() const {
	return mAvdName;
}


Device::InstallReceiver::InstallReceiver() {
}

bool Device::InstallReceiver::isCancelled() {
	return false;
}

std::string Device::InstallReceiver::getErrorMessage() {
	return mErrorMessage;
}
std::string Device::IPaddressReceiver::getAddress(){
	if(!ipaddress.empty())
		return ipaddress;
	else return std::string("");
}

void Device::IPaddressReceiver::processNewLines(const std::vector<std::string> &lines){
	std::vector<std::string> temp;
	
	for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
		if (IP_ADDRESS.split(*line, temp)){
			ipaddress = temp.at(2);
		}
	}
}


void Device::setAvdName(const std::string &avdName) {
	if (isEmulator() == false) {
		throw std::invalid_argument("Cannot set the AVD name of the device is not an emulator");
	}

	mAvdName = avdName;
}

std::string Device::getState() const {
	return mState;
}

void Device::setState(const std::string &state) {
	mState = state;
}

std::map<std::string, std::string> Device::getProperties() const {
	return mProperties;
}

int Device::getPropertyCount() {
	return mProperties.size();
}

std::string Device::getProperty(const std::string &name) {
	return mProperties[name];
}

bool Device::arePropertiesSet() {
	return mArePropertiesSet;
}

std::string Device::getPropertyCacheOrSync(const std::string &name) {
	if (mArePropertiesSet) {
		return getProperty(name);
	} else {
		return getPropertySync(name);
	}
}

std::string Device::getPropertySync(const std::string &name) {
	std::tr1::shared_ptr<CollectingOutputReceiver> receiver(new CollectingOutputReceiver);
	executeShellCommand(std::string("getprop ") + name, receiver.get());
	std::string value = receiver->getOutput();
	if (value.empty()) {
		return std::string("");
	}
	return value;
}

std::string Device::getMountPoint(const std::string &name) {
	if (mMountPoints.count(name) > 0)
		return mMountPoints[name];
	else
		return std::string();
}

std::string Device::toString() {
	return mSerialNumber;
}

bool Device::isOnline() {
	return (mState == Device::ONLINE);
}

bool Device::isEmulator() {
	return (mSerialNumber.find("emulator-") != std::string::npos);
}

bool Device::isOffline() {
	return (mState == Device::OFFLINE);
}

bool Device::isBootLoader() {
	return (mState == Device::BOOTLOADER);
}

#ifdef CLIENT_SUPPORT
bool Device::hasClients() {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	return !mClients.empty();
}

std::vector<std::tr1::shared_ptr<Client> > Device::getClients() {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	return mClients;
}

std::tr1::shared_ptr<Client> Device::getClient(const std::wstring &applicationName) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	for (std::vector<std::tr1::shared_ptr<Client> >::iterator c = mClients.begin(); c != mClients.end(); ++c) {
		if (applicationName == (*c)->getClientData()->getClientDescription()) {
			return *c;
		}
	}

	return std::tr1::shared_ptr<Client>();
}

std::wstring Device::getClientName(int pid) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	for (std::vector<std::tr1::shared_ptr<Client> >::iterator c = mClients.begin(); c != mClients.end(); ++c) {
		if ((*c)->getClientData()->getPid() == pid) {
			return (*c)->getClientData()->getClientDescription();
		}
	}

	return std::wstring();
}

void Device::addClient(std::tr1::shared_ptr<Client> client) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	mClients.push_back(client);
}

std::vector<std::tr1::shared_ptr<Client> > Device::getClientList() {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	return mClients;
}

bool Device::hasClient(int pid) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	for (std::vector<std::tr1::shared_ptr<Client> >::iterator client = mClients.begin(); client != mClients.end(); ++client) {
		if ((*client)->getClientData()->getPid() == pid) {
			return true;
		}
	}

	return false;
}

void Device::clearClientList() {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	mClients.clear();
}

void Device::setClientMonitoringSocket(std::tr1::shared_ptr<Poco::Net::StreamSocket> socketChannel) {
	mSocketChannel = socketChannel;
}

std::tr1::shared_ptr<Poco::Net::StreamSocket> Device::getClientMonitoringSocket() {
	return mSocketChannel;
}

void Device::removeClient(std::tr1::shared_ptr<Client> client, bool notify) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	std::vector<std::tr1::shared_ptr<Client> >::iterator clientPos = std::find(mClients.begin(), mClients.end(), client);
	if (clientPos != mClients.end())
		mClients.erase(clientPos);
	if (!mMonitor.expired()) {
		mMonitor.lock()->addPortToAvailableList(client->getDebuggerListenPort());
		if (notify) {
			mMonitor.lock()->getServer()->deviceChanged(shared_from_this(), CHANGE_CLIENT_LIST);
		}
	}
}

void Device::update(std::tr1::shared_ptr<Client> client, int changeMask) {
	mMonitor.lock()->getServer()->clientChanged(client, changeMask);
}


#endif

std::tr1::shared_ptr<SyncService> Device::getSyncService() {
	std::tr1::shared_ptr<SyncService> syncService(new SyncService(AndroidDebugBridge::getSocketAddress(), shared_from_this()));
	if (syncService->openSync()) {
		return syncService;
	}
	return std::tr1::shared_ptr<SyncService>();
}

std::tr1::shared_ptr<FileListingService> Device::getFileListingService() {
	return std::tr1::shared_ptr<FileListingService>(new FileListingService(shared_from_this()));
}

std::tr1::shared_ptr<RawImage> Device::getScreenshot() {
	return AdbHelper::getFrameBuffer(AndroidDebugBridge::getSocketAddress(), shared_from_this());
}

void Device::executeShellCommand(const std::string &command, IShellOutputReceiver *receiver) {
	AdbHelper::executeRemoteCommand(AndroidDebugBridge::getSocketAddress(), command,
			this, receiver, DdmPreferences::getTimeOut());
}

void Device::executeShellCommand(const std::string &command, IShellOutputReceiver *receiver,
		int maxTimeToOutputResponse) {
	AdbHelper::executeRemoteCommand(AndroidDebugBridge::getSocketAddress(), command,
			this, receiver, maxTimeToOutputResponse);
}

void Device::runEventLogService(LogReceiver *receiver) {
	AdbHelper::runEventLogService(AndroidDebugBridge::getSocketAddress(),
			this, receiver);
}

void Device::runLogService(const std::string &logname, LogReceiver *receiver) {
	AdbHelper::runLogService(AndroidDebugBridge::getSocketAddress(), this, logname,
			receiver);
}

void Device::createForward(int localPort, int remotePort) {
	AdbHelper::createForward(AndroidDebugBridge::getSocketAddress(), shared_from_this(), localPort,
			remotePort);
}

void Device::removeForward(int localPort, int remotePort) {
	AdbHelper::removeForward(AndroidDebugBridge::getSocketAddress(), shared_from_this(), localPort,
			remotePort);
}


std::tr1::shared_ptr<DeviceMonitor> Device::getMonitor() {
	return mMonitor.lock();
}

void Device::update(int changeMask) {
	if ((changeMask & CHANGE_BUILD_INFO) != 0) {
		mArePropertiesSet = true;
	}
	mMonitor.lock()->getServer()->deviceChanged(shared_from_this(), changeMask);
}

void Device::addProperty(const std::string& label, const std::string& value) {
	mProperties[label] = value;
}

void Device::setMountingPoint(const std::string &name, const std::string &value) {
	mMountPoints[name] = value;
}

void Device::pushFile(const std::string& local, const std::string& remote) {
	std::tr1::shared_ptr<SyncService> sync;
	try {
		std::string targetFileName = getFileName(local);

		Log::d(targetFileName, ("Uploading " + targetFileName + "s onto device '" + getSerialNumber() + "'"));

		sync = getSyncService();
		if (sync != nullptr) {
			std::string message = std::string("Uploading file onto device '" + getSerialNumber() + "'");
			Log::d(LOG_TAG, message);
			sync->pushFile(local, remote, SyncService::getNullProgressMonitor());
			sync->close();

		} else {
			throw Poco::IOException("Unable to open sync connection!");
		}
	} catch (Poco::TimeoutException& e) {
		if (sync != nullptr) {
			sync->close();
		}
		Log::e(LOG_TAG, "Error during Sync: timeout.");
		throw;

	} catch (Poco::IOException& e) {
		if (sync != nullptr) {
			sync->close();
		}
		Log::e(LOG_TAG, std::string(std::string("Error during Sync: ") + e.what()));
		throw;

	} catch (std::runtime_error& e) {
		if (sync != nullptr) {
			sync->close();
		}
		Log::e(LOG_TAG, std::string(std::string("Error during Sync: ") + e.what()));
		throw;

	}
}

void Device::pullFile(const std::string &remote, const std::string &local) {
	std::tr1::shared_ptr<SyncService> sync;
	try {
		std::string targetFileName = getFileName(remote);

		Log::d(targetFileName, "Downloading " + targetFileName + " from device '" + getSerialNumber() + "'");

		sync = getSyncService();
		if (sync != nullptr) {
			std::string message = "Downloading file from device " + getSerialNumber();
			Log::d(LOG_TAG, message);
			sync->pullFile(remote, local, SyncService::getNullProgressMonitor());
			sync->close();
		} else {
			throw Poco::IOException("Unable to open sync connection!");
		}
	} catch (Poco::TimeoutException& e) {
		Log::e(LOG_TAG, "Error during Sync: timeout.");
		if (sync != nullptr)
			sync->close();
		throw;

	} catch (SyncException& e) {
		Log::e(LOG_TAG, std::string(std::string("Error during Sync: ") + e.what()));
		if (sync != nullptr)
			sync->close();
		throw;

	} catch (Poco::IOException& e) {
		Log::e(LOG_TAG, std::string(std::string("Error during Sync: ") + e.what()));
		if (sync != nullptr)
			sync->close();
		throw;

	}
}

std::string Device::installPackage(const std::string& packageFilePath, bool reinstall,
		const std::vector<std::string>& extraArgs) {
	try {
		std::string remoteFilePath = syncPackageToDevice(packageFilePath);
		std::string result = installRemotePackage(remoteFilePath, reinstall, extraArgs);
		removeRemotePackage(remoteFilePath);
		return result;
	} catch (Poco::IOException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
		//throw std::exception();
	} catch (AdbCommandRejectedException& e) {
		throw InstallException(e);
	} catch (Poco::TimeoutException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
	} catch (SyncException& e) {
		throw InstallException(e);
	}
	return std::string();
}

std::string Device::syncPackageToDevice(const std::string& localFilePath) {
	std::tr1::shared_ptr<SyncService> sync;
	try {
		std::string packageFileName = getFileName(localFilePath);
		std::string remoteFilePath = std::string("/data/local/tmp/" + packageFileName); 

		Log::d(packageFileName, std::string("Uploading ") + packageFileName + " onto device " + getSerialNumber());

		sync = getSyncService();
		if (sync != nullptr) {
			std::string message = ("Uploading file onto device " + getSerialNumber());
			Log::d(LOG_TAG, message);
			sync->pushFile(localFilePath, remoteFilePath, SyncService::getNullProgressMonitor());
		} else {
			throw Poco::IOException("Unable to open sync connection!");
		}
		return remoteFilePath;
	} catch (Poco::TimeoutException& e) {
		Log::e(LOG_TAG, "Error during Sync: timeout.");
		if (sync != nullptr)
			sync->close();
		throw;

	} catch (SyncException& e) {
		Log::e(LOG_TAG, std::string("Error during Sync: ") + e.what());
		if (sync != nullptr)
			sync->close();
		throw;

	} catch (Poco::IOException& e) {
		Log::e(LOG_TAG, std::string("Error during Sync: ") + e.what());
		if (sync != nullptr)
			sync->close();
		throw;

	}
	return std::string();
}

/**
 * Helper method to retrieve the file name given a local file path
 * @param filePath full directory path to file
 * @return {@link String} file name
 */
std::string Device::getFileName(const std::string& filePath) {
	return Poco::Path(filePath).getFileName();
}

std::string Device::installRemotePackage(const std::string& remoteFilePath, bool reinstall,
		const std::vector<std::string>& extraArgs) {
	try {
		std::tr1::shared_ptr<InstallReceiver> receiver(new InstallReceiver);
		std::string optionString;
		if (reinstall) {
			optionString.append("-r ");
		}
		for (std::vector<std::string>::const_iterator arg = extraArgs.begin(); arg != extraArgs.end(); ++arg) {
			optionString.append(*arg);
			optionString.append(" ");
		}
		std::string cmd = std::string("pm install " + optionString + "\"" + remoteFilePath + "\"");
		executeShellCommand(cmd, receiver.get(), INSTALL_TIMEOUT);
		return receiver->getErrorMessage();
	} catch (Poco::TimeoutException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
	} catch (AdbCommandRejectedException& e) {
		throw InstallException(e);
	} catch (ShellCommandUnresponsiveException& e) {
		throw InstallException(e);
	} catch (Poco::IOException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
	}
	return std::string();
}

void Device::removeRemotePackage(const std::string &remoteFilePath) {
	try {
		executeShellCommand("rm " + remoteFilePath, NullOutputReceiver::getReceiver(), INSTALL_TIMEOUT);
	} catch (Poco::IOException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
	} catch (Poco::TimeoutException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
	} catch (AdbCommandRejectedException& e) {
		throw InstallException(e);
	} catch (ShellCommandUnresponsiveException& e) {
		throw InstallException(e);
	}
}

std::string Device::uninstallPackage(const std::string& packageName) {
	try {
		std::tr1::shared_ptr<InstallReceiver> receiver(new InstallReceiver);
		executeShellCommand("pm uninstall " + packageName, receiver.get(), INSTALL_TIMEOUT);
		return receiver->getErrorMessage();
	} catch (Poco::TimeoutException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
	} catch (AdbCommandRejectedException& e) {
		throw InstallException(e);
	} catch (ShellCommandUnresponsiveException& e) {
		throw InstallException(e);
	} catch (Poco::IOException& e) {
		throw InstallException(e.what()); //TODO e or e.what()?
	}
	return std::string();
}

/*
 * (non-Javadoc)
 * @see com.android.ddmlib.Device#reboot()
 */
void Device::reboot(const std::string& into) {
	AdbHelper::reboot(into, AndroidDebugBridge::getSocketAddress(), shared_from_this());
}

int Device::getBatteryLevel() {
	// use default of 5 minutes
	return getBatteryLevel(5 * 60 * 1000);
}

int Device::getBatteryLevel(long long freshnessMs) {
	if (mLastBatteryLevel != 0 && mLastBatteryCheckTime > (current_time.epochMicroseconds() / 1000 - freshnessMs)) {
		return mLastBatteryLevel;
	}
	std::tr1::shared_ptr<BatteryReceiver> receiver(new BatteryReceiver());
	executeShellCommand("dumpsys battery", receiver.get());
	mLastBatteryLevel = receiver->getBatteryLevel();
	mLastBatteryCheckTime = current_time.epochMicroseconds() / 1000;
	return mLastBatteryLevel;
}

std::string Device::getIPaddress(){
	std::tr1::shared_ptr<IPaddressReceiver> receiver(new IPaddressReceiver);
	executeShellCommand("netcfg", receiver.get());
	mIPaddress = receiver->getAddress();
	return mIPaddress;
}

void Device::registerInReactor() {
#ifdef CLIENT_SUPPORT
	if (mSocketChannel != nullptr) {
		AndroidDebugBridge::getReactor().addEventHandler(*(mSocketChannel.get()),
				Poco::NObserver<Device, Poco::Net::ReadableNotification>(*this, &Device::processDeviceReadActivity));
	}
#endif
}

#ifdef CLIENT_SUPPORT
void Device::processDeviceReadActivity(const Poco::AutoPtr<Poco::Net::ReadableNotification> & notification) {

	std::tr1::shared_ptr<Poco::Net::StreamSocket> socket = getClientMonitoringSocket();

	if (socket != nullptr) {
		try {
			Log::v("ddms", "processDeviceReadActivity in " + toString());
			int length = readLength(socket, 4);

			mMonitor.lock()->processIncomingJdwpData(shared_from_this(), socket, length);
		} catch (Poco::IOException& ioe) {
			Log::d("DeviceMonitor", std::string("Error reading jdwp list: ") + ioe.what());
			socket->close();

			// restart the monitoring of that device


			//if (!mMonitor.expired())
			//	mMonitor.lock()->startMonitoringDevice(shared_from_this());
		}
	}
}
#endif

int Device::readLength(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size) {
	std::string msg = read(socket, size);

	if (!msg.empty()) {
		try {
			return Poco::NumberParser::parseHex(msg);
		} catch (Poco::SyntaxException &nfe) {
			Log::e("ddms", "Can't parse " + msg);
			// we'll throw an exception below.
		}
	}

	// we receive something we can't read. It's better to reset the connection at this point.
	throw Poco::IOException("Unable to read length");
}

std::string Device::read(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size) {
	//ByteBuffer *buf = ByteBuffer::wrap(buffer, size);
	unsigned int count = 0;
	char *buffer = new char[size];

	memset(buffer, '\0', size);

	try {
		while (count < size) {
			int bytesRead = socket->receiveBytes(buffer + count, size - count);
			if (bytesRead <= 0) {
				throw Poco::IOException("EOF");
			}
			count += bytesRead;
			Device::waitABit();
		}
	} catch (Poco::Exception &e) {
		Log::e("ddms", std::string("Exception in socket read ") + e.what());
	} catch (...) {
		Log::e("ddms", std::string("Unknown exception in socket read"));
		delete []buffer;
		throw;
	}
	std::string str(buffer, size);
	delete []buffer;

	//try {
	return str;
	/*} catch (UnsupportedEncodingException e) {
	 // we'll return null below.
	 }*/

	//return "";
}

void Device::waitABit() {
	try {
		Poco::Thread::sleep(5);
	} catch (...) {
	}
}

void Device::restartInTcpip(int port) {
	AdbHelper::restartInTcpip(shared_from_this(), port);
}

void Device::restartInUSB() {
	AdbHelper::restartInUSB(shared_from_this());
}

} /* namespace ddmlib */

