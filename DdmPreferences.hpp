/*
 * DdmPreferences.h
 *
 *  Created on: Jan 27, 2012
 *      Author: Ilya Polenov
 */

#ifndef DDMPREFERENCES_HPP_
#define DDMPREFERENCES_HPP_

#include "ddmlib.hpp"

namespace ddmlib {
/**
 * Preferences for the ddm library.
 * <p/>This class does not handle storing the preferences. It is merely a central point for
 * applications using the ddmlib to override the default values.
 * <p/>Various components of the ddmlib query this class to get their values.
 * <p/>Calls to some <code>set##()</code> methods will update the components using the values
 * right away, while other methods will have no effect once {@link AndroidDebugBridge#init(boolean)}
 * has been called.
 * <p/>Check the documentation of each method.
 */
class DDMLIB_API DdmPreferences {
	static bool sThreadUpdate; //DEFAULT_INITIAL_THREAD_UPDATE
	static bool sInitialHeapUpdate; //DEFAULT_INITIAL_HEAP_UPDATE

	static int sSelectedDebugPort; //DEFAULT_SELECTED_DEBUG_PORT
	static int sDebugPortBase; //DEFAULT_DEBUG_PORT_BASE
	static int sLogLevel; //DEFAULT_LOG_LEVEL
	static int sTimeOut; //DEFAULT_TIMEOUT
	static int sProfilerBufferSizeMb; //DEFAULT_PROFILER_BUFFER_SIZE_MB

	static bool sUseAdbHost; //DEFAULT_USE_ADBHOST
	static std::string sAdbHostValue; //DEFAULT_ADBHOST_VALUE

public:
	/** Default value for thread update flag upon client connection. */
	static const bool DEFAULT_INITIAL_THREAD_UPDATE = false;
	/** Default value for heap update flag upon client connection. */
	static const bool DEFAULT_INITIAL_HEAP_UPDATE = false;
	/** Default value for the selected client debug port */
	static const int DEFAULT_SELECTED_DEBUG_PORT = 8700;
	/** Default value for the debug port base */
	static const int DEFAULT_DEBUG_PORT_BASE = 8600;
	/** Default value for the logcat {@link LogLevel} */
	static const int DEFAULT_LOG_LEVEL = 6; // ERROR
	/** Default timeout values for adb connection (milliseconds) */
	static const int DEFAULT_TIMEOUT = 5000; // standard delay, in ms
	/** Default profiler buffer size (megabytes) */
	static const int DEFAULT_PROFILER_BUFFER_SIZE_MB = 8;
	/** Default values for the use of the ADBHOST environment variable. */
	static const bool DEFAULT_USE_ADBHOST = false;
	static int getDebugPortBase();
	static int getDefaultDebugPortBase();
	static bool getDefaultInitialHeapUpdate();
	static bool getDefaultInitialThreadUpdate();
	static int getDefaultLogLevel();
	static int getDefaultProfilerBufferSizeMb();
	static int getDefaultSelectedDebugPort();
	static int getDefaultTimeout();
	static bool getDefaultUseAdbhost();
	static int getSelectedDebugPort();
	static bool getThreadUpdate();

	static int getTimeOut() {
		return sTimeOut;
	}

	static bool getUseAdbHost();

	static void setUseAdbHost(bool useAdbHost);

	static std::string getAdbHostValue();

	static int getLogLevel();

	static bool getInitialThreadUpdate();

	static bool getInitialHeapUpdate();

	static int getProfilerBufferSizeMb();

	//static std::string const DEFAULT_ADBHOST_VALUE("127.0.0.1");

	DdmPreferences();
	virtual ~DdmPreferences();
};

} /* namespace ddmlib */
#endif /* DDMPREFERENCES_HPP_ */
