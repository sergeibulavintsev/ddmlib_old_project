/*
 * AdbCommandRejectedException.cpp
 *
 *  Created on: 06.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "AdbCommandRejectedException.hpp"

namespace ddmlib {

long long AdbCommandRejectedException::serialVersionUID = 1LL;

AdbCommandRejectedException::AdbCommandRejectedException(const std::string& message) :
		std::runtime_error(message) {
	mIsDeviceOffline = (message == "device offline");
	mErrorDuringDeviceSelection = false;
}

AdbCommandRejectedException::AdbCommandRejectedException(const std::string& message, bool errorDuringDeviceSelection) :
		std::runtime_error(message) {
	mErrorDuringDeviceSelection = errorDuringDeviceSelection;
	mIsDeviceOffline = (message == "device offline");
}

} /* namespace ddmlib */
