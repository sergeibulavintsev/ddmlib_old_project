/*
 * logcat.cpp
 *
 *  Created on: 06.03.2012
 *      Author: Ilya Polenov
 */
#include "Log.hpp"
#include "MultiLineReceiver.hpp"
#include "AndroidDebugBridge.hpp"
#include "DeviceMonitor.hpp"
#include "Device.hpp"
#include "EmulatorConsole.hpp"
#include <Poco/Thread.h>
#include <iostream>
#include <memory>
#include <vector>

using namespace ddmlib;

class LogCatReceiver : public MultiLineReceiver {
public:
	LogCatReceiver() : mCancelled(false) {}

	virtual void processNewLines(const std::vector<std::string>& lines) {
		for (auto &line: lines) {
			if (!line.empty()) {
				auto ll = LogLevel::getByLetter(line[0]);
				if (ll != nullptr)
					Log::println(*ll, "logcat", line.substr(1));
				else
					Log::v("logcat", line);
			}
		}
	}

	virtual bool isCancelled() {
		return mCancelled;
	}

	void cancel() {
		mCancelled = true;
	}
private:
	bool mCancelled;
};

class DCListener: public IDeviceChangeListener {
	std::shared_ptr<LogCatReceiver> lcr;
public:
	virtual void deviceConnected(Device *device) {
		lcr = std::make_shared<LogCatReceiver>();
		Log::v("test", "Device " + device->getSerialNumber() + " connected : " + device->getState());
		//device->executeShellCommand("logcat", lcr);
	}

	virtual void deviceDisconnected(Device *device) {
		Log::v("test", std::string("Device ") + device->getSerialNumber() + std::string(" disconnected : ") + device->getState());
	}

	virtual void deviceChanged(Device *device, int changeMask) {
		Log::v("test", std::string("Device ") + device->getSerialNumber() + std::string(" new state : ") + device->getState());
	}

	void cancel() {
		lcr->cancel();
	}
};

int main() {
	{
		AndroidDebugBridge *adb = nullptr;
		try {
			Log::setLevel(0);
			AndroidDebugBridge::init(false);
			do {
				//adb = AndroidDebugBridge::createBridge(std::string("C:\\Program Files\\Android\\android-sdk\\platform-tools\\adb.exe"), false);
				adb = AndroidDebugBridge::createBridge();
			} while (adb == nullptr);
			std::shared_ptr<DCListener> dcl = std::make_shared<DCListener>();
			adb->addDeviceChangeListener(dcl.get());
			std::shared_ptr<DeviceMonitor> dm = adb->getDeviceMonitor();
			std::cin.get();
			auto devices = adb->getDevices();
			for (auto &device : devices) {
				if (device->isEmulator()) {
					auto ec = EmulatorConsole::getConsole(device);
					ec->sendLocation(56.1, 60.1, 250.3);
				}
			}
			std::cin.get();
			dcl->cancel();
		} catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		}
		AndroidDebugBridge::disconnectBridge();
	}
	Poco::Thread::sleep(500);
	return 0;
}

