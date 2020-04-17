/*
 * DdmSocketReactor.hpp
 *
 *  Created on: 12.03.2012
 *      Author: Ilya Polenov
 */

#ifndef DDMSOCKETREACTOR_HPP_
#define DDMSOCKETREACTOR_HPP_

#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_LOCAL DdmSocketReactor: public Poco::Net::SocketReactor {
public:
	DdmSocketReactor() {};
	virtual ~DdmSocketReactor() {};
	void onIdle() {
		Poco::Net::SocketReactor::onIdle();
		Poco::Thread::sleep(1);
	}
};

} /* namespace ddmlib */
#endif /* DDMSOCKETREACTOR_HPP_ */
