/*
 * BadPacketException.cpp
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "BadPacketException.hpp"

namespace ddmlib {

BadPacketException::BadPacketException() :
		std::runtime_error("") {
}

BadPacketException::BadPacketException(const std::string &msg) :
		std::runtime_error(msg) {
}

} /* namespace ddmlib */
