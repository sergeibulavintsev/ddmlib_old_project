/*
 * ShellCommandUnresponsiveException.hpp
 *
 *  Created on: 07.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef SHELLCOMMANDUNRESPONSIVEEXCEPTION_HPP_
#define SHELLCOMMANDUNRESPONSIVEEXCEPTION_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_API ShellCommandUnresponsiveException: public std::runtime_error {
private:
	static long long serialVersionUID;
};

} /* namespace ddmlib */
#endif /* SHELLCOMMANDUNRESPONSIVEEXCEPTION_HPP_ */
