/*
 * CanceledException.h
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef CANCELEDEXCEPTION_HPP_
#define CANCELEDEXCEPTION_HPP_

#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_API CanceledException: public std::runtime_error {
public:
	CanceledException(const std::string &msg);
	CanceledException(const std::string &msg, std::runtime_error& error);
	std::runtime_error *getCause() const;
	//virtual ~CanceledException();
	virtual bool wasCanceled() const { return false; };
private:
	const static long long serialVersionUID;
	std::runtime_error *mError;
};

} /* namespace ddmlib */
#endif /* CANCELEDEXCEPTION_H_ */
