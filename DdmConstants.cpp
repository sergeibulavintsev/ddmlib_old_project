/*
 * DdmConstants.cpp
 *
 *  Created on: Feb 10, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "DdmConstants.hpp"

namespace ddmlib {
const int DdmConstants::PLATFORM_UNKNOWN = 0;
const int DdmConstants::PLATFORM_LINUX = 1;
const int DdmConstants::PLATFORM_WINDOWS = 2;
const int DdmConstants::PLATFORM_DARWIN = 3;

/**
 * Returns current platform, one of {@link #PLATFORM_WINDOWS}, {@link #PLATFORM_DARWIN},
 * {@link #PLATFORM_LINUX} or {@link #PLATFORM_UNKNOWN}.
 */
const int DdmConstants::CURRENT_PLATFORM = currentPlatform();

/**
 * Extension for Traceview files.
 */
std::wstring DdmConstants::DOT_TRACE(L".trace");

/** hprof-conv executable (with extension for the current OS)  */
std::string DdmConstants::FN_HPROF_CONVERTER((CURRENT_PLATFORM == PLATFORM_WINDOWS) ? "hprof-conv.exe" : "hprof-conv");

/** traceview executable (with extension for the current OS)  */
std::string DdmConstants::FN_TRACEVIEW((CURRENT_PLATFORM == PLATFORM_WINDOWS) ? "traceview.bat" : "traceview");

DdmConstants::DdmConstants() {
}

DdmConstants::~DdmConstants() {
}

} /* namespace ddmlib */
