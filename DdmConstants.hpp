/*
 * DdmConstants.hpp
 *
 *  Created on: Feb 10, 2012
 *      Author: Ilya Polenov
 */

#ifndef DDMCONSTANTS_HPP_
#define DDMCONSTANTS_HPP_

#include "ddmlib.hpp"
namespace ddmlib {

class DDMLIB_API DdmConstants {
public:
	DdmConstants();
	virtual ~DdmConstants();

	static const int PLATFORM_UNKNOWN;
	static const int PLATFORM_LINUX;
	static const int PLATFORM_WINDOWS;
	static const int PLATFORM_DARWIN;

	/**
	 * Returns current platform, one of {@link #PLATFORM_WINDOWS}, {@link #PLATFORM_DARWIN},
	 * {@link #PLATFORM_LINUX} or {@link #PLATFORM_UNKNOWN}.
	 */
	static const int CURRENT_PLATFORM;

	/**
	 * Extension for Traceview files.
	 */
	static std::wstring DOT_TRACE;

	/** hprof-conv executable (with extension for the current OS)  */
	static std::string FN_HPROF_CONVERTER;

	/** traceview executable (with extension for the current OS)  */
	static std::string FN_TRACEVIEW;

	/**
	 * Returns current platform
	 *
	 * @return one of {@link #PLATFORM_WINDOWS}, {@link #PLATFORM_DARWIN},
	 * {@link #PLATFORM_LINUX} or {@link #PLATFORM_UNKNOWN}.
	 */
	static int currentPlatform() {
		std::string os = Poco::Environment::osName();
		if (os.find("Mac OS") != os.npos) {
			return PLATFORM_DARWIN;
		} else if (os.find("Windows") != os.npos) {
			return PLATFORM_WINDOWS;
		} else if (os.find("Linux") != os.npos) {
			return PLATFORM_LINUX;
		}

		return PLATFORM_UNKNOWN;
	}
};

} /* namespace ddmlib */
#endif /* DDMCONSTANTS_HPP_ */
