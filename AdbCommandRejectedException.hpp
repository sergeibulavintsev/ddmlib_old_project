/*
 * AdbCommandRejectedException.h
 *
 *  Created on: 06.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef ADBCOMMANDREJECTEDEXCEPTION_HPP_
#define ADBCOMMANDREJECTEDEXCEPTION_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_API AdbCommandRejectedException: public std::runtime_error {
public:
	AdbCommandRejectedException(const std::string& message);
	AdbCommandRejectedException(const std::string& message, bool errorDuringDeviceSelection);
	/**
	 * Returns true if the error is due to the device being offline.
	 */
	bool isDeviceOffline() {
		return mIsDeviceOffline;
	}

	/**
	 * Returns whether adb refused to target a given device for the command.
	 * <p/>If false, adb refused the command itself, if true, it refused to target the given
	 * device.
	 */
	bool wasErrorDuringDeviceSelection() {
		return mErrorDuringDeviceSelection;
	}
private:
	static long long serialVersionUID;
	bool mIsDeviceOffline;
	bool mErrorDuringDeviceSelection;
};

} /* namespace ddmlib */
#endif /* ADBCOMMANDREJECTEDEXCEPTION_HPP_ */
