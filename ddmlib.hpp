/*
 * ddmlib.hpp
 *
 *  Created on: 19.03.2012
 *      Author: Ilya Polenov
 */

#pragma once

#ifndef DDMLIB_HPP_
#define DDMLIB_HPP_

// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
  #define DDMLIB_HELPER_DLL_IMPORT __declspec(dllimport)
  #define DDMLIB_HELPER_DLL_EXPORT __declspec(dllexport)
  #define DDMLIB_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DDMLIB_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define DDMLIB_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define DDMLIB_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DDMLIB_HELPER_DLL_IMPORT
    #define DDMLIB_HELPER_DLL_EXPORT
    #define DDMLIB_HELPER_DLL_LOCAL
  #endif
#endif

// Now we use the generic helper definitions above to define DDMLIB_API and DDMLIB_LOCAL.
// DDMLIB_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// DDMLIB_LOCAL is used for non-api symbols.

#ifdef DDMLIB_DLL // defined if DDM is compiled as a DLL
  #ifdef DDMLIB_DLL_EXPORTS // defined if we are building the DDM DLL (instead of using it)
    #define DDMLIB_API DDMLIB_HELPER_DLL_EXPORT
  #else
    #define DDMLIB_API DDMLIB_HELPER_DLL_IMPORT
  #endif // DDMLIB_DLL_EXPORTS
  #define DDMLIB_LOCAL DDMLIB_HELPER_DLL_LOCAL
#else // DDMLIB_DLL is not defined: this means DDM is a static lib.
  #define DDMLIB_API
  #define DDMLIB_LOCAL
#endif // DDMLIB_DLL

#include <string>
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <assert.h>

#include <Poco\Format.h>
#include <Poco\String.h>
#include <Poco\Thread.h>
#include <Poco\RunnableAdapter.h>
#include <Poco\Process.h>
#include <Poco\Mutex.h>
#include <Poco\NumberFormatter.h>
#include <Poco\NumberParser.h>
#include <Poco\RegularExpression.h>
#include <Poco\Net\SocketStream.h>
#include <Poco\Net\StreamSocket.h>
#include <Poco\Net\ServerSocket.h>
#include <Poco\Net\Socket.h>
#include <Poco\Net\SocketNotification.h>
#include <Poco\Net\SocketReactor.h>
#include <Poco\NObserver.h>
#include <Poco\AutoPtr.h>
#include <Poco\File.h>
#include <Poco\Path.h>
#include <Poco\FileStream.h>
#include <Poco\LocalDateTime.h>
#include <Poco\DateTimeFormatter.h>
#include <Poco\Timestamp.h>
#include <Poco\Environment.h>
#include <Poco\PipeStream.h>
#include <Poco\Exception.h>
#include <Poco\Net\NetException.h>
#include <Poco\StringTokenizer.h>
#include <Poco\UnicodeConverter.h>
#include <Poco\ThreadTarget.h>
#include <Poco\ThreadPool.h>
#include <Poco\TaskManager.h>
#include <Poco\Timestamp.h>
#include <Poco\Environment.h>

#define nullptr NULL

#endif /* DDMLIB_HPP_ */
