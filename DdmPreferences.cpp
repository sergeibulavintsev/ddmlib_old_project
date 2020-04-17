/*
 * DdmPreferences.cpp
 *
 *  Created on: Jan 27, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "DdmPreferences.hpp"

namespace ddmlib {

bool DdmPreferences::sThreadUpdate = DdmPreferences::DEFAULT_INITIAL_THREAD_UPDATE;
bool DdmPreferences::sInitialHeapUpdate = DdmPreferences::DEFAULT_INITIAL_HEAP_UPDATE;

int DdmPreferences::sSelectedDebugPort = DdmPreferences::DEFAULT_SELECTED_DEBUG_PORT;
int DdmPreferences::sDebugPortBase = DdmPreferences::DEFAULT_DEBUG_PORT_BASE;
int DdmPreferences::sLogLevel = DdmPreferences::DEFAULT_LOG_LEVEL;
int DdmPreferences::sTimeOut = DdmPreferences::DEFAULT_TIMEOUT;
int DdmPreferences::sProfilerBufferSizeMb = DdmPreferences::DEFAULT_PROFILER_BUFFER_SIZE_MB;

bool DdmPreferences::sUseAdbHost = DdmPreferences::DEFAULT_USE_ADBHOST;
std::string DdmPreferences::sAdbHostValue = "127.0.0.1";

DdmPreferences::DdmPreferences() {
}

bool DdmPreferences::getUseAdbHost() {
	return sUseAdbHost;
}

void DdmPreferences::setUseAdbHost(bool useAdbHost) {
	sUseAdbHost = useAdbHost;
}

std::string DdmPreferences::getAdbHostValue() {
	return sAdbHostValue;
}

int DdmPreferences::getLogLevel() {
	return sLogLevel;
}

bool DdmPreferences::getInitialThreadUpdate() {
	return sThreadUpdate;
}

bool DdmPreferences::getInitialHeapUpdate() {
	return sInitialHeapUpdate;
}

int DdmPreferences::getProfilerBufferSizeMb() {
	return sProfilerBufferSizeMb;
}

int DdmPreferences::getDebugPortBase() {
	return sDebugPortBase;
}

int DdmPreferences::getDefaultDebugPortBase() {
	return DEFAULT_DEBUG_PORT_BASE;
}

bool DdmPreferences::getDefaultInitialHeapUpdate() {
	return DEFAULT_INITIAL_HEAP_UPDATE;
}

bool DdmPreferences::getDefaultInitialThreadUpdate() {
	return DEFAULT_INITIAL_THREAD_UPDATE;
}

int DdmPreferences::getDefaultLogLevel() {
	return DEFAULT_LOG_LEVEL;
}

int DdmPreferences::getDefaultProfilerBufferSizeMb() {
	return DEFAULT_PROFILER_BUFFER_SIZE_MB;
}

int DdmPreferences::getDefaultSelectedDebugPort() {
	return DEFAULT_SELECTED_DEBUG_PORT;
}

int DdmPreferences::getDefaultTimeout() {
	return DEFAULT_TIMEOUT;
}

bool DdmPreferences::getDefaultUseAdbhost() {
	return DEFAULT_USE_ADBHOST;
}

int DdmPreferences::getSelectedDebugPort() {
	return sSelectedDebugPort;
}

bool DdmPreferences::getThreadUpdate() {
	return sThreadUpdate;
}

DdmPreferences::~DdmPreferences() {
}

} /* namespace ddmlib */
