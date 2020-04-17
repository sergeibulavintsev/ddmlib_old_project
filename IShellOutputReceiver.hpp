/*
 * IShellOutputReceiver.hpp
 *
 *  Created on: 07.02.2012
 *      Author: sergey bulavintsev
*/

#ifndef ISHELLOUTPUTRECEIVER_HPP_
#define ISHELLOUTPUTRECEIVER_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_API IShellOutputReceiver {
public:
	virtual void addOutput(unsigned char* data, unsigned int offset, unsigned int length) = 0;
	virtual void flush() = 0;
	virtual bool isCancelled() = 0;
	virtual ~IShellOutputReceiver() {
	}
};

} /* namespace ddmlib */
#endif /* ISHELLOUTPUTRECEIVER_HPP_ */
