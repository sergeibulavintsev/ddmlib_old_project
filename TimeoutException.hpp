/*
 * TimeoutException.hpp
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef TIMEOUTEXCEPTION_HPP_
#define TIMEOUTEXCEPTION_HPP_
#include "ddmlib.hpp"

namespace ddmlib {
class DDMLIB_API TimeoutException: public std::runtime_error {
};
}

#endif /* TIMEOUTEXCEPTION_HPP_ */
