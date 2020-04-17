/*
 * BadPacketException.h
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef BADPACKETEXCEPTION_HPP_
#define BADPACKETEXCEPTION_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_API BadPacketException: public std::runtime_error {
public:
	BadPacketException();
	BadPacketException(const std::string &msg);
};

} /* namespace ddmlib */
#endif /* BADPACKETEXCEPTION_H_ */
