/*
 * InstallException.cpp
 *
 *  Created on: 03.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "InstallException.hpp"

namespace ddmlib {

InstallException::InstallException(std::runtime_error& e) :
		CanceledException(e.what(), e) {

}

InstallException::InstallException(const std::string& msg) :
		CanceledException(msg) {

}

} /* namespace ddmlib */
