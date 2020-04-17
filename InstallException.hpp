/*
 * InstallException.h
 *
 *  Created on: 03.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef INSTALLEXCEPTION_HPP_
#define INSTALLEXCEPTION_HPP_
#include "ddmlib.hpp"

#include "CanceledException.hpp"
#include "SyncException.hpp"

namespace ddmlib {

class DDMLIB_API InstallException: public CanceledException {
public:
	InstallException(std::runtime_error& e);
	InstallException(const std::string& msg);
	bool wasCanceled() const {
		std::runtime_error *cause = getCause();
		return static_cast<SyncException *>(cause)->wasCanceled();
	}
};

} /* namespace ddmlib */
#endif /* INSTALLEXCEPTION_H_ */
