/*
 * CanceledException.cpp
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "CanceledException.hpp"

namespace ddmlib {

const long long CanceledException::serialVersionUID = 1LL;

CanceledException::CanceledException(const std::string &msg) :
		std::runtime_error(msg) {
	mError = NULL;
}

CanceledException::CanceledException(const std::string &msg, std::runtime_error& error) :
		std::runtime_error(msg) {
	mError = &error;
}

std::runtime_error *CanceledException::getCause() const {
	return mError;
}

} /* namespace ddmlib */
