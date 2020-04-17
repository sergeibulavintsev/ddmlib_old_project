/*
 * AndroidDebugBridge.cpp
 *
 *  Created on: Jan 27, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "AndroidDebugBridge.hpp"
#ifdef CLIENT_SUPPORT
#include "Client.hpp"
#include "HandleAppName.hpp"
#include "HandleExit.hpp"
#include "HandleHeap.hpp"
#include "HandleHello.hpp"
#include "HandleNativeHeap.hpp"
#include "HandleProfiling.hpp"
#include "HandleTest.hpp"
#include "HandleThread.hpp"
#include "HandleWait.hpp"
#include "Debugger.hpp"
#endif
#include "Device.hpp"
#include "DdmPreferences.hpp"
#include "DdmSocketReactor.hpp"
#include "Log.hpp"
#include "DeviceMonitor.hpp"
#include "AdbHelper.hpp"
#include "ProcessLauncher.hpp"

namespace ddmlib {

std::tr1::shared_ptr<AndroidDebugBridge> AndroidDebugBridge::sThis;
Poco::RegularExpression AndroidDebugBridge::sAdbVersion("^.*version ([0-9]*)\\.([0-9]*)\\.([0-9]*)$",
		Poco::RegularExpression::RE_NEWLINE_ANYCRLF | Poco::RegularExpression::RE_MULTILINE);
bool AndroidDebugBridge::sInitialized = false;
#ifdef CLIENT_SUPPORT
bool AndroidDebugBridge::sClientSupport = false;
std::set< std::tr1::shared_ptr<IClientChangeListener> > AndroidDebugBridge::sClientListeners;
#endif
char AndroidDebugBridge::ADB[] = "adb";
char AndroidDebugBridge::DDMS[] = "ddms";
char AndroidDebugBridge::SERVER_PORT_ENV_VAR[] = "ANDROID_ADB_SERVER_PORT";
char AndroidDebugBridge::ADB_HOST[] = "127.0.0.1";
unsigned int AndroidDebugBridge::ADB_PORT = 5037;
std::string AndroidDebugBridge::sHostAddr;
Poco::Net::SocketAddress AndroidDebugBridge::sSocketAddr;
Poco::Mutex AndroidDebugBridge::sLock;
std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> > AndroidDebugBridge::sBridgeListeners;
std::set< std::tr1::shared_ptr<IDeviceChangeListener> > AndroidDebugBridge::sDeviceListeners;
DdmSocketReactor AndroidDebugBridge::sReactor;
std::tr1::shared_ptr<Poco::Thread> AndroidDebugBridge::sReactorThread(new Poco::Thread("Socket Reactor"));
std::string AndroidDebugBridge::sAdbOsLocation;

AndroidDebugBridge::AndroidDebugBridge() {
}

AndroidDebugBridge::AndroidDebugBridge(const std::string &osLocation) {
	if (osLocation.empty()) {
		throw std::invalid_argument("Adb location is not set");
	}

	sAdbOsLocation = osLocation;

	//checkAdbVersion();
}

AndroidDebugBridge::~AndroidDebugBridge() {
}

void AndroidDebugBridge::checkAdbVersion() {
	// default is bad check
	mVersionCheck = false;

	if (sAdbOsLocation.empty()) {
		return;
	}

	int status = -1;

	try {
		Poco::Process::Args args;
		args.push_back("version");
		Log::d(DDMS, std::string("Launching ") + sAdbOsLocation + " " + args[0] + " to ensure ADB is running.");
		Poco::Pipe adbStdOut;
		Poco::Pipe adbStdErr;
		std::vector<std::string> errorOutput;
		std::vector<std::string> stdOutput;
#ifdef _WIN32
		DetachedProcessHandle adbHandle(DetachedProcessWin32::launch(sAdbOsLocation, args, nullptr, &adbStdOut, &adbStdErr));
#else
		Poco::ProcessHandle adbHandle(Poco::Process::launch(sAdbOsLocation, args, nullptr, &adbStdOut, &adbStdErr));
#endif
		status = grabProcessOutput(adbHandle, &adbStdErr, &adbStdOut, errorOutput, stdOutput, false);

		if (status != 0) {
			std::string msg("'adb version' failed!");
			for (std::vector<std::string>::iterator error = errorOutput.begin(); error != errorOutput.end(); ++error) {
				msg += '\n';
				msg += *error;
			}
			Log::logAndDisplay(Log::ERROR_L, "adb", msg);
		}

		// check both stdout and stderr
		bool versionFound = false;
		for (std::vector<std::string>::iterator line = stdOutput.begin(); line != stdOutput.end(); ++line) {
			versionFound = scanVersionLine(*line);
			if (versionFound)
				break;
		}
		if (!versionFound) {
			for (std::vector<std::string>::iterator line = errorOutput.begin(); line != errorOutput.end(); ++line) {
				versionFound = scanVersionLine(*line);
				if (versionFound)
					break;
			}
		}

		if (!versionFound) {
			// if we get here, we failed to parse the output.
			Log::logAndDisplay(Log::ERROR_L, ADB, "Failed to parse the output of 'adb version'"); 
		}

	} catch (Poco::IOException &e) {
		Log::logAndDisplay(Log::ERROR_L, ADB, std::string("Failed to get the adb version: ") + e.what()); 
	} /* catch (InterruptedException e) {
	 }*/
}

bool AndroidDebugBridge::scanVersionLine(const std::string &line) {
	if (!line.empty()) {
		std::vector<std::string> matches;
		int numMatches = sAdbVersion.split(line, matches);
		if (numMatches >= 3) {
			//int majorVersion = Poco::NumberParser::parse(matches[1]);
			//int minorVersion = Poco::NumberParser::parse(matches[2]);
			int microVersion = Poco::NumberParser::parse(matches[3]);

			// check only the micro version for now.
			if (microVersion < ADB_VERSION_MICRO_MIN) {
				std::string message = std::string("Adb is too old.");
				Log::logAndDisplay(Log::ERROR_L, ADB, message);
			} else if (ADB_VERSION_MICRO_MAX != -1 && microVersion > ADB_VERSION_MICRO_MAX) {
				std::string message("Unsupported adb.");
				Log::logAndDisplay(Log::ERROR_L, ADB, message);
			} else {
				mVersionCheck = true;
			}

			return true;
		}
	}
	return false;
}

void AndroidDebugBridge::init(bool clientSupport) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	if (sInitialized) {
		throw Poco::IllegalStateException("AndroidDebugBridge::init() has already been called.");
	}

	sInitialized = true;
#ifdef CLIENT_SUPPORT
	sClientSupport = clientSupport;
#endif
	sReactorThread->setName("DDMLib socket reactor thread");
	sReactorThread->start(sReactor);

	// Determine port and instantiate socket address.
	initAdbSocketAddr();

#ifdef CLIENT_SUPPORT
	HandleHello::registerInReactor();
	HandleAppName::registerInReactor();
	HandleTest::registerInReactor();
	HandleThread::registerInReactor();
	HandleHeap::registerInReactor();
	HandleWait::registerInReactor();
	HandleProfiling::registerInReactor();
	HandleNativeHeap::registerInReactor();
#endif
}

void AndroidDebugBridge::terminate() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> lLock(localLock);
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	try {
#ifdef CLIENT_SUPPORT
		{
			std::vector< std::tr1::shared_ptr< Device > > devices = sThis->getDevices();
			for (std::vector< std::tr1::shared_ptr< Device > >::iterator device = devices.begin(); device != devices.end(); ++device) {
				std::vector< std::tr1::shared_ptr< Client > > clients = (*device)->getClientList();
				for (std::vector< std::tr1::shared_ptr< Client> >::iterator client = clients.begin(); client != clients.end(); ++client) {
					try {
						std::tr1::shared_ptr<Debugger> debugger = (*client)->getDebugger();
						debugger->close();
						(*client)->dropClient(false /* notify */);
					} catch (...) {
						// TODO: do something?
					}
				}
			}
		}
#endif
		// kill the monitoring services
		if (sThis != nullptr && sThis->mDeviceMonitor != nullptr) {
			sThis->mDeviceMonitor->stop();
			sThis->mDeviceMonitor.reset();
		}
	} catch (...) {
		Log::e("ddms", "Exception during device monitor termination");
		// TODO: do something
	}
	sInitialized = false;

	Log::v("ddms", "Waiting for reactor to stop");
	sReactor.stop();
	sReactorThread->join();
	Log::v("ddms", "Reactor stopped");

	sThis.reset();
}

#ifdef CLIENT_SUPPORT
bool AndroidDebugBridge::getClientSupport() {
	return sClientSupport;
}
#endif

Poco::Net::SocketAddress AndroidDebugBridge::getSocketAddress() {
	return sSocketAddr;
}

AndroidDebugBridge *AndroidDebugBridge::createBridge() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> lLock(localLock);
	Poco::ScopedLockWithUnlock<Poco::Mutex> lock(sLock);
	if (sThis != nullptr)
		return sThis.get();
	try {
		sThis = std::tr1::shared_ptr<AndroidDebugBridge>(new AndroidDebugBridge);
		if (!sThis->start()) {
			sThis->stop();
			sThis.reset();
		}
	} catch (std::bad_alloc &e) {
		sThis.reset();
		// TODO
	}

	// because the listeners could remove themselves from the list while processing
	// their event callback, we make a copy of the list and iterate on it instead of
	// the main list.
	// This mostly happens when the application quits.
	std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> > listenersCopy(sThis->sBridgeListeners);

	lock.unlock();
	// notify the listeners of the change
	for (std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> >::iterator listener = listenersCopy.begin(); listener != listenersCopy.end(); ++listener) {
		// we attempt to catch any exception so that a bad listener doesn't kill our
		// thread
		try {
			(*listener)->bridgeChanged(sThis.get());
		} catch (std::exception &e) {
			//Log.e(DDMS, e);
		}
	}

	return sThis.get();
}

AndroidDebugBridge *AndroidDebugBridge::createBridge(const std::string &osLocation, bool forceNewBridge) {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> lLock(localLock);
	Poco::ScopedLockWithUnlock<Poco::Mutex> lock(sLock);
	if (sThis != nullptr) {
		if ((sThis->sAdbOsLocation == osLocation) && !forceNewBridge) {
			return sThis.get();
		} else {
			// stop the current server
			sThis->stop();
			sThis.reset();
		}
	}

	try {
		sThis = std::tr1::shared_ptr<AndroidDebugBridge>(new AndroidDebugBridge(osLocation));
		if (!sThis->start()) {
			sThis->stop();
			sThis.reset();
		}
	} catch (std::exception &e) {
		sThis.reset();
		// TODO
	}

	// because the listeners could remove themselves from the list while processing
	// their event callback, we make a copy of the list and iterate on it instead of
	// the main list.
	// This mostly happens when the application quits.
	std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> > listenersCopy(sThis->sBridgeListeners);

	lock.unlock();
	// notify the listeners of the change
	for (std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> >::iterator listener = listenersCopy.begin(); listener != listenersCopy.end(); ++listener) {
		// we attempt to catch any exception so that a bad listener doesn't kill our
		// thread
		try {
			(*listener)->bridgeChanged(sThis.get());
		} catch (std::exception &e) {
			//Log.e(DDMS, e);
		}
	}

	return sThis.get();
}

void AndroidDebugBridge::disconnectBridge() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> llock(localLock);
	Poco::ScopedLockWithUnlock<Poco::Mutex> lock(sLock);
	if (sThis != nullptr) {
		sThis->stop();
		sThis.reset();

		// because the listeners could remove themselves from the list while processing
		// their event callback, we make a copy of the list and iterate on it instead of
		// the main list.
		// This mostly happens when the application quits.
		std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> > listenersCopy(sThis->sBridgeListeners);

		lock.unlock();
		// notify the listeners.
		for (std::set< std::tr1::shared_ptr<IDebugBridgeChangeListener> >::iterator listener = listenersCopy.begin(); listener != listenersCopy.end(); ++listener) {
			// we attempt to catch any exception so that a bad listener doesn't kill our
			// thread
			try {
				(*listener)->bridgeChanged(sThis.get());
			} catch (std::exception &e) {
				Log::e(DDMS, e.what());
			}
		}
	}
}

bool AndroidDebugBridge::start() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> llock(localLock);
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	if (!sAdbOsLocation.empty() && (/*!mVersionCheck ||*/ !startAdb())) {
		return false;
	}

	mStarted = true;

	// now that the bridge is connected, we start the underlying services.
	mDeviceMonitor = std::tr1::shared_ptr<DeviceMonitor>(new DeviceMonitor(sThis));
	mDeviceMonitor->start();

	return true;
}

bool AndroidDebugBridge::startAdb() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> llock(localLock);
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	if (sAdbOsLocation.empty()) {
		Log::e(ADB, "Cannot start adb when AndroidDebugBridge is created without the location of adb."); 
		return false;
	}

	int status = -1;

	try {
		Poco::Process::Args args;
		args.push_back("start-server");
		Log::d(DDMS, std::string("Launching '") + sAdbOsLocation + " " + args[0] + "' to ensure ADB is running.");
		if (DdmPreferences::getUseAdbHost()) {
			std::string adbHostValue = DdmPreferences::getAdbHostValue();
			if (!adbHostValue.empty()) {
				//TODO : check that the String is a valid IP address
				Poco::Environment::set("ADBHOST", adbHostValue);
			}
		}

		Poco::Pipe adbStdOut;
		Poco::Pipe adbStdErr;
		std::vector<std::string> errorOutput;
		std::vector<std::string> stdOutput;
#ifdef _WIN32
		DetachedProcessHandle adbHandle(DetachedProcessWin32::launch(sAdbOsLocation, args, nullptr, nullptr, nullptr));
#else
		Poco::ProcessHandle adbHandle(Poco::Process::launch(sAdbOsLocation, args, nullptr, nullptr, nullptr));
#endif
		status = grabProcessOutput(adbHandle, nullptr, nullptr, errorOutput, stdOutput, false);

	} catch (Poco::IOException &ioe) {
		Log::d(DDMS, std::string("Unable to run 'adb': ") + ioe.what());
		// we'll return false;
	}/* catch (InterruptedException &ie) {
	 // TODO: Log.d(DDMS, "Unable to run 'adb': " + ie.what());
	 // we'll return false;
	 }*/

	if (status != 0) {
		Log::w(DDMS, "'adb start-server' failed -- run manually if necessary");
		return false;
	}

	Log::d(DDMS, "'adb start-server' succeeded");

	return true;
}

bool AndroidDebugBridge::stop() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> llock(localLock);
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	// if we haven't started we return false;
	if (mStarted == false) {
		return false;
	}

	// kill the monitoring services
	mDeviceMonitor->stop();
	mDeviceMonitor.reset();

	/*if (!stopAdb()) {
		return false;
	}*/

	mStarted = false;
	return true;
}

bool AndroidDebugBridge::stopAdb() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> llock(localLock);
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	if (sAdbOsLocation.empty()) {
		Log::e(ADB, "Cannot stop adb when AndroidDebugBridge is created without the location of adb."); 
		return false;
	}

	int status = -1;

	try {
		Poco::Process::Args args;
		args.push_back("kill-server");
		Log::d(DDMS, std::string("Launching '") + sAdbOsLocation + " " + args[0] + "' to stop ADB server.");
#ifdef _WIN32
		DetachedProcessHandle adbHandle(DetachedProcessWin32::launch(sAdbOsLocation, args));
#else
		Poco::ProcessHandle adbHandle(Poco::Process::launch(sAdbOsLocation, args));
#endif
		status = adbHandle.wait();
	} catch (Poco::IOException &ioe) {
		// we'll return false;
	}/*
	 catch (InterruptedException ie) {
	 // we'll return false;
	 }*/

	if (status != 0) {
		Log::w(DDMS, "'adb kill-server' failed -- run manually if necessary");
		return false;
	}

	Log::d(DDMS, "'adb kill-server' succeeded"); 
	return true;
}

bool AndroidDebugBridge::restart() {
	static Poco::Mutex localLock;
	Poco::ScopedLock<Poco::Mutex> lock(localLock);
	if (sAdbOsLocation.empty()) {
		Log::e(ADB, "Cannot restart adb when AndroidDebugBridge is created without the location of adb.");
		return false;
	}

	/*if (!mVersionCheck) {
		Log::logAndDisplay(Log::ERROR_L, ADB, "Attempting to restart adb, but version check failed!");
		return false;
	}*/
	{
		Poco::ScopedLock<Poco::Mutex> lock(sLock);
		//stopAdb();

		bool restart = startAdb();

		if (restart && mDeviceMonitor == nullptr) {
			mDeviceMonitor = std::tr1::shared_ptr<DeviceMonitor>(new DeviceMonitor(sThis));
			mDeviceMonitor->start();
		}

		return restart;
	}
}

#ifdef CLIENT_SUPPORT
int AndroidDebugBridge::grabProcessOutput(const Poco::ProcessHandle &adbHandle, Poco::Pipe *adbStdErr, Poco::Pipe *adbStdOut,
		std::vector<std::string> &errorOutput, std::vector<std::string> &stdOutput, bool waitforReaders = true) {
#else
int AndroidDebugBridge::grabProcessOutput(const DetachedProcessHandle &adbHandle, Poco::Pipe *adbStdErr, Poco::Pipe *adbStdOut,
		std::vector<std::string> &errorOutput, std::vector<std::string> &stdOutput, bool waitforReaders = true) {
#endif

	// Objects of this class are used as Runnables to run in their own threads.
	class AdbStreamReader: public Poco::Runnable {
	private:
		std::vector<std::string> *pStrVec;
		Poco::Pipe *out;
		LogLevel *logLevel;
	public:
		AdbStreamReader(Poco::Pipe *output, std::vector<std::string> &strVec, char errorLevel) {
			pStrVec = &strVec;
			out = output;
			logLevel = LogLevel::getByLetter(errorLevel);
		}

		~AdbStreamReader() {
		}

		void run() {
			// create a buffer to read the stderr output
			Poco::PipeInputStream reader(*out);
			reader.exceptions((std::ios::iostate) (std::istream::eofbit | std::istream::failbit | std::istream::badbit));
			try {
				while (true) {
					char line[1024];
					memset(line, '\0', 1024);
					if (reader.good()) {
						reader.getline(line, 1023);
						std::string strline(line);
						Poco::trim(strline);
//						std::remove(strline.begin(), strline.end(), '\n');
//						std::remove(strline.begin(), strline.end(), '\r');
						if (!strline.empty()) {
							pStrVec->push_back(strline);
							Log::logAndDisplay(*logLevel, ADB, strline);
						}
					} else {
						reader.close();
						break;
					}
				}
			} catch (Poco::IOException &e) {
				reader.close();
				Log::d(ADB, "Poco IOException");
				// do nothing.
			} catch (std::ios_base::failure &e) {
				reader.close();
				//Log::d(ADB, "ios failure");
			}
		}
	};


	Poco::Thread t1;
	AdbStreamReader readerStd = AdbStreamReader(adbStdOut, stdOutput, 'D');
	if (adbStdOut != nullptr)
		t1.start(readerStd);

	Poco::Thread t2;
	AdbStreamReader readerErr = AdbStreamReader(adbStdErr, errorOutput, 'E');
	if (adbStdErr != nullptr)
		t2.start(readerErr);

	// Original comment from java ddmlib: it looks like on windows process#waitFor() can return
	// before the thread have filled the arrays, so we wait for both threads and the
	// process itself.
	//
	// Looks like with Poco on C++ waiting for threads to finish is not necessary.
	// Waiting could take forever as reading from pipes is blocking them.
	if (waitforReaders) {
		try {
			t1.join();
		} catch (Poco::TimeoutException &e) {
			Log::w(ADB, "StdOut reading thread is taking too long.");
		}
		try {
			t2.join();
		} catch (Poco::TimeoutException &e) {
			Log::w(ADB, "StdErr reading thread is taking too long.");
		}
	}

	// get the return code from the process
	int result = adbHandle.wait();

	//t1.setPriority(Poco::Thread::PRIO_LOWEST);
	//t2.setPriority(Poco::Thread::PRIO_LOWEST);

	return result;
}

void AndroidDebugBridge::initAdbSocketAddr() {
	unsigned int adb_port = determineAndValidateAdbPort();
	sHostAddr = std::string(ADB_HOST);
	sSocketAddr = Poco::Net::SocketAddress(sHostAddr, adb_port);
}

unsigned int AndroidDebugBridge::determineAndValidateAdbPort() {
	unsigned int result = ADB_PORT;
	try {
		std::string adb_env_var(Poco::Environment::get(SERVER_PORT_ENV_VAR, ""));
		adb_env_var.erase(0, adb_env_var.find_first_not_of(" \t\f\v\n\r")); //trim prefixing spaces
		adb_env_var.erase(adb_env_var.find_last_not_of(" \t\f\v\n\r") + 1); //trim suffixing spaces

		if (!adb_env_var.empty()) {
			// C tools (adb, emulator) accept hex and octal port numbers, so need to accept
			// them too.
			if (!Poco::NumberParser::tryParseHex(adb_env_var, result))
				result = Poco::NumberParser::parseUnsigned(adb_env_var); // Octal port number is not parsed

			if (result <= 0) {
				std::string errMsg = std::string("env var ") + SERVER_PORT_ENV_VAR + ": must be >0, got "
						+ Poco::Environment::get(SERVER_PORT_ENV_VAR, "");
				throw std::invalid_argument(errMsg);
			}
		}
	} catch (Poco::SyntaxException &nfEx) {
		std::string errMsg = std::string("env var ") + SERVER_PORT_ENV_VAR + ": illegal value '"
				+ Poco::Environment::get(SERVER_PORT_ENV_VAR, "") + "'";
		throw std::invalid_argument(errMsg);
	}/* catch (Poco::NotFoundException secEx) {
	 // A security manager has been installed that doesn't allow access to env vars.
	 // So an environment variable might have been set, but we can't tell.
	 // Let's log a warning and continue with ADB's default port.
	 // The issue is that adb would be started (by the forked process having access
	 // to the env vars) on the desired port, but within this process, we can't figure out
	 // what that port is. However, a security manager not granting access to env vars
	 // but allowing to fork is a rare and interesting configuration, so the right
	 // thing seems to be to continue using the default port, as forking is likely to
	 // fail later on in the scenario of the security manager.
	 Log.w(DDMS,
	 "No access to env variables allowed by current security manager. " 
	 + "If you've set ANDROID_ADB_SERVER_PORT: it's being ignored."); 
	 }*/
	return result;
}

void AndroidDebugBridge::deviceConnected(std::tr1::shared_ptr<Device> device) {
	// because the listeners could remove themselves from the list while processing
	// their event callback, we make a copy of the list and iterate on it instead of
	// the main list.
	// This mostly happens when the application quits.
	sLock.lock();
	std::set< std::tr1::shared_ptr<IDeviceChangeListener> > listenersCopy(sThis->sDeviceListeners);
	sLock.unlock();

	// Notify the listeners
	for (std::set< std::tr1::shared_ptr<IDeviceChangeListener> >::iterator listener = listenersCopy.begin(); listener != listenersCopy.end(); ++listener) {
		// we attempt to catch any exception so that a bad listener doesn't kill our
		// thread
		try {
			(*listener)->deviceConnected(device.get());
		} catch (std::exception &e) {
			Log::e(DDMS, e.what());
		}
	}
}

void AndroidDebugBridge::deviceDisconnected(std::tr1::shared_ptr<Device> device) {
	// because the listeners could remove themselves from the list while processing
	// their event callback, we make a copy of the list and iterate on it instead of
	// the main list.
	// This mostly happens when the application quits.
	sLock.lock();
	std::set< std::tr1::shared_ptr<IDeviceChangeListener> > listenersCopy(sThis->sDeviceListeners);
	sLock.unlock();

	// Notify the listeners
	for (std::set< std::tr1::shared_ptr<IDeviceChangeListener> >::iterator listener = listenersCopy.begin(); listener != listenersCopy.end(); ++listener) {
		// we attempt to catch any exception so that a bad listener doesn't kill our
		// thread
		try {
			(*listener)->deviceDisconnected(device.get());
		} catch (std::exception &e) {
			Log::e(DDMS, e.what());
		}
	}
}

void AndroidDebugBridge::deviceChanged(std::tr1::shared_ptr<Device> device, int changeMask) {
	// because the listeners could remove themselves from the list while processing
	// their event callback, we make a copy of the list and iterate on it instead of
	// the main list.
	// This mostly happens when the application quits.
	sLock.lock();
	std::set< std::tr1::shared_ptr<IDeviceChangeListener> > listenersCopy(sThis->sDeviceListeners);
	sLock.unlock();

	// Notify the listeners
	for (std::set< std::tr1::shared_ptr<IDeviceChangeListener> >::iterator listener = listenersCopy.begin(); listener != listenersCopy.end(); ++listener) {
		// we attempt to catch any exception so that a bad listener doesn't kill our
		// thread
		try {
			(*listener)->deviceChanged(device.get(), changeMask);
		} catch (std::exception &e) {
			Log::e(DDMS, e.what());
		}
	}
}

#ifdef CLIENT_SUPPORT
void AndroidDebugBridge::clientChanged(std::tr1::shared_ptr<Client> client, int changeMask) {
	// because the listeners could remove themselves from the list while processing
	// their event callback, we make a copy of the list and iterate on it instead of
	// the main list.
	// This mostly happens when the application quits.
	sLock.lock();
	std::set< std::tr1::shared_ptr<IClientChangeListener> > listenersCopy(sThis->sClientListeners);
	sLock.unlock();

	// Notify the listeners
	for (std::set< std::tr1::shared_ptr<IClientChangeListener> >::iterator listener = listenersCopy.begin(); listener != listenersCopy.end(); ++listener) {
		// we attempt to catch any exception so that a bad listener doesn't kill our
		// thread
		try {
			(*listener)->clientChanged(client.get(), changeMask);
		} catch (std::exception &e) {
			Log::e(DDMS, e.what());
		}
	}
}
#endif

void AndroidDebugBridge::addDebugBridgeChangeListener(std::tr1::shared_ptr<IDebugBridgeChangeListener> listener) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	sBridgeListeners.insert(listener);
	if (sThis != nullptr) {
		// we attempt to catch any exception so that a bad listener doesn't kill our
		// thread
		try {
			listener->bridgeChanged(sThis.get());
		} catch (std::exception &e) {
			Log::e(DDMS, e.what());
		}
	}
}

void AndroidDebugBridge::removeDebugBridgeChangeListener(std::tr1::shared_ptr<IDebugBridgeChangeListener> listener) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	sBridgeListeners.erase(listener);
}

void AndroidDebugBridge::addDeviceChangeListener(std::tr1::shared_ptr<IDeviceChangeListener> listener) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	sDeviceListeners.insert(listener);
}

void AndroidDebugBridge::removeDeviceChangeListener(std::tr1::shared_ptr<IDeviceChangeListener> listener) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	sDeviceListeners.erase(listener);
}

#ifdef CLIENT_SUPPORT
void AndroidDebugBridge::addClientChangeListener(std::tr1::shared_ptr<IClientChangeListener> listener) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	sClientListeners.insert(listener);
}

void AndroidDebugBridge::removeClientChangeListener(std::tr1::shared_ptr<IClientChangeListener> listener) {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	sClientListeners.erase(listener);
}

void AndroidDebugBridge::setSelectedClient(std::tr1::shared_ptr<Client> selectedClient) {
	selectedClient->setAsSelectedClient();
}
#endif

std::vector<std::tr1::shared_ptr<Device> > AndroidDebugBridge::getDevices() {
	Poco::ScopedLock<Poco::Mutex> lock(sLock);
	if (mDeviceMonitor != nullptr) {
		std::vector<std::tr1::shared_ptr<Device> > devices = mDeviceMonitor->getDevices();
		return devices;
	} else return std::vector<std::tr1::shared_ptr<Device> >();

}

bool AndroidDebugBridge::hasInitialDeviceList() {
	if (mDeviceMonitor != nullptr) {
		return mDeviceMonitor->hasInitialDeviceList();
	}

	return false;
}

bool AndroidDebugBridge::isConnected() {
	if (mDeviceMonitor != nullptr) {
		return (mDeviceMonitor->isMonitoring());
	}
	return false;
}

int AndroidDebugBridge::getConnectionAttemptCount() {
	if (mDeviceMonitor != nullptr) {
		return mDeviceMonitor->getConnectionAttemptCount();
	}
	return -1;
}

int AndroidDebugBridge::getRestartAttemptCount() {
	if (mDeviceMonitor != nullptr) {
		return mDeviceMonitor->getRestartAttemptCount();
	}
	return -1;
}

std::tr1::shared_ptr<DeviceMonitor> AndroidDebugBridge::getDeviceMonitor() {
	return mDeviceMonitor;
}

Poco::Mutex &AndroidDebugBridge::getLock() {
	return sLock;
}

AndroidDebugBridge *AndroidDebugBridge::getBridge() {
	return sThis.get();
}

Poco::Net::SocketReactor &AndroidDebugBridge::getReactor() {
	return sReactor;
}

std::tr1::shared_ptr<Device> AndroidDebugBridge::findDeviceBySerial(const std::string &serial) {
	if (mDeviceMonitor != nullptr) {
		return mDeviceMonitor->findDeviceBySerial(serial);
	}
	return std::tr1::shared_ptr<Device>();
}


std::string AndroidDebugBridge::connectToNetworkDevice(const std::string &address) {
	return AdbHelper::connectToNetworkDevice(address);
}

std::string AndroidDebugBridge::disconnectFromNetworkDevice(const std::string &address) {
	return AdbHelper::disconnectFromNetworkDevice(address);
}

void AndroidDebugBridge::setADBLocation(const std::string &location) {
	sAdbOsLocation = location;
}

} /* namespace ddmlib */

