/*
 * NullOutputReceiver.h
 *
 *  Created on: 09.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef NULLOUTPUTRECEIVER_HPP_
#define NULLOUTPUTRECEIVER_HPP_
#include "ddmlib.hpp"

#include "IShellOutputReceiver.hpp"

namespace ddmlib {

class DDMLIB_API NullOutputReceiver: public IShellOutputReceiver {
public:
	static NullOutputReceiver *getReceiver() {
		return sReceiver.get();
	}

	void addOutput(unsigned char* data, unsigned int offset, unsigned int length) {
	}

	void flush() {
	}

	bool isCancelled() {
		return false;
	}
private:
	static std::tr1::shared_ptr<NullOutputReceiver> sReceiver;
};

} /* namespace ddmlib */
#endif /* NULLOUTPUTRECEIVER_H_ */
