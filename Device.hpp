/*
 * Device.h
 *
 *  Created on: 26.01.2012
 *      Author: sergey bulavintsev
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include "ddmlib.hpp"
#include "MultiLineReceiver.hpp"

namespace ddmlib {

class SyncService;
class DeviceMonitor;
class FileListingService;
class LogReceiver;
class IShellOutputReceiver;
class RawImage;

#ifdef CLIENT_SUPPORT
class Client;
#endif

class DDMLIB_API Device: public std::tr1::enable_shared_from_this<Device> {
public:
	static std::string BOOTLOADER;
	static std::string OFFLINE;
	static std::string ONLINE;
	static std::string RECOVERY;

	Device(std::tr1::shared_ptr<DeviceMonitor> monitor, const std::string &serialNumber, const std::string &deviceState);
	virtual ~Device();

	const static char PROP_BUILD_VERSION[];
	const static char PROP_BUILD_API_LEVEL[];
	const static char PROP_BUILD_CODENAME[];
	const static char PROP_DEBUGGABLE[];

	/** Serial number of the first connected emulator. */
	const static char FIRST_EMULATOR_SN[];
	/** Emulator Serial Number regexp. */
	const static char RE_EMULATOR_SN[];
	/** Device change bit mask: {@link DeviceState} change. */
	const static int CHANGE_STATE = 0x0001;
	/** Device change bit mask: {@link Client} list change. */
	const static int CHANGE_CLIENT_LIST = 0x0002;
	/** Device change bit mask: build info change. */
	const static int CHANGE_BUILD_INFO = 0x0004;

	/** @deprecated Use {@link #PROP_BUILD_API_LEVEL}. */
	const static char PROP_BUILD_VERSION_NUMBER[];
	const static char MNT_EXTERNAL_STORAGE[];
	const static char MNT_ROOT[];
	const static char MNT_DATA[];

	std::string getSerialNumber() const;
	std::string getAvdName() const;
	std::string getState() const;
	std::map<std::string, std::string> getProperties() const;
	std::string getProperty(const std::string &name);
	int getPropertyCount();
	std::string getPropertySync(const std::string &name);
	std::string getPropertyCacheOrSync(const std::string &name);
	std::string getMountPoint(const std::string &name);
	std::tr1::shared_ptr<DeviceMonitor> getMonitor();
#ifdef CLIENT_SUPPORT
	std::tr1::shared_ptr<Poco::Net::StreamSocket> getClientMonitoringSocket();
	std::vector<std::tr1::shared_ptr<Client> > getClients();
	std::tr1::shared_ptr<Client> getClient(const std::wstring &applicationName);
	std::vector<std::tr1::shared_ptr<Client> > getClientList();
	bool hasClient(int pid);
	bool hasClients();
	void setClientMonitoringSocket(std::tr1::shared_ptr<Poco::Net::StreamSocket> socketChannel);
#endif
	std::tr1::shared_ptr<SyncService> getSyncService();
	std::tr1::shared_ptr<FileListingService> getFileListingService();
	std::tr1::shared_ptr<RawImage> getScreenshot();
	std::string getFileName(const std::string& filePath);
	int getBatteryLevel();
	int getBatteryLevel(long long freshnessMs);
	std::string getIPaddress();

	void setAvdName(const std::string &avdName);
	void setState(const std::string &state);
	void setMountingPoint(const std::string &name, const std::string &value);

	bool arePropertiesSet();
	bool isOnline();
	bool isEmulator();
	bool isOffline();
	bool isBootLoader();

	std::string toString();
#ifdef CLIENT_SUPPORT
	void addClient(std::tr1::shared_ptr<Client> client);
	void clearClientList();
	void removeClient(std::tr1::shared_ptr<Client>, bool notify);
	void addClient(std::tr1::shared_ptr<Client> client);
	void clearClientList();
	void removeClient(std::tr1::shared_ptr<Client>, bool notify);
	void update(std::tr1::shared_ptr<Client>, int changeMask);
	std::wstring getClientName(int pid);
#endif
	void update(int changeMask);
	void addProperty(const std::string &label, const std::string &value);
	void executeShellCommand(const std::string &command, IShellOutputReceiver *receiver);
	void executeShellCommand(const std::string &command, IShellOutputReceiver *receiver,
			int maxTimeToOutputResponse);
	void runEventLogService(LogReceiver *receiver);
	void runLogService(const std::string &logname, LogReceiver *receiver);
	void createForward(int localPort, int remotePort);
	void removeForward(int localPort, int remotePort);
	void pushFile(const std::string& local, const std::string& remote);
	void pullFile(const std::string& remote, const std::string& local);
	std::string installPackage(const std::string& packageFilePath, bool reinstall, const std::vector<std::string>& extraArgs);
	std::string syncPackageToDevice(const std::string& localFilePath);
	std::string installRemotePackage(const std::string& remoteFilePath, bool reinstall,
			const std::vector<std::string>& extraArgs);
	void removeRemotePackage(const std::string& remoteFilePath);
	std::string uninstallPackage(const std::string& packageName);
	void reboot(const std::string& into);
	void restartInTcpip(int port);
	void restartInUSB();
	void processDeviceReadActivity(const Poco::AutoPtr<Poco::Net::ReadableNotification> & notification);
	void registerInReactor();
	void waitABit();
private:

	int readLength(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size);
	std::string read(std::tr1::shared_ptr<Poco::Net::StreamSocket> socket, size_t size);

	class InstallReceiver: public MultiLineReceiver {

	public:
		InstallReceiver();

		virtual ~InstallReceiver() {};

		void processNewLines(const std::vector<std::string>& lines);

		bool isCancelled();

		std::string getErrorMessage();
	private:
		const static char SUCCESS_OUTPUT[]; // = "Success"; 
		const static Poco::RegularExpression FAILURE_PATTERN; 

		std::string mErrorMessage;

	};

	class BatteryReceiver: public MultiLineReceiver {
	public:
		BatteryReceiver();
		virtual ~BatteryReceiver() {};

		/**
		 * Get the parsed percent battery level.
		 * @return
		 */
		int getBatteryLevel();
		void processNewLines(const std::vector<std::string>& lines);
		bool isCancelled();

	private:
		const static Poco::RegularExpression BATTERY_LEVEL;
		const static Poco::RegularExpression SCALE;
		int mBatteryLevel;
		int mBatteryScale;

	};
	
	class IPaddressReceiver : public MultiLineReceiver {
	public:
		IPaddressReceiver(){};
		virtual ~IPaddressReceiver(){};
		void processNewLines(const std::vector<std::string>& lines);
		std::string getAddress();
		bool isCancelled(){ return false;};
	private:
		const static Poco::RegularExpression IP_ADDRESS;
		std::string ipaddress;
	};
	static Poco::Timestamp current_time;
	const static int INSTALL_TIMEOUT = 2 * 60 * 1000; //2min
	static Poco::Mutex sLock;


	/** Serial number of the device */
	std::string mSerialNumber;

	/** Name of the AVD */
	std::string mAvdName;

	/** State of the device. */
	std::string mState;

	/** Device properties. */
	std::map<std::string, std::string> mProperties;
	std::map<std::string, std::string> mMountPoints;

#ifdef CLIENT_SUPPORT
	std::vector<std::tr1::shared_ptr<Client> > mClients;
	/**
	 * Socket for the connection monitoring client connection/disconnection.
	 */
	std::tr1::shared_ptr<Poco::Net::StreamSocket> mSocketChannel;
#endif
	std::tr1::weak_ptr<DeviceMonitor> mMonitor;

	const static char LOG_TAG[];

	unsigned char mLengthBuffer[4];

	bool mArePropertiesSet; // = false;
	int mLastBatteryLevel; // = 0;
	long long mLastBatteryCheckTime; // = 0;
	std::string mIPaddress;
};

} /* namespace ddmlib */
#endif /* DEVICE_H_ */
