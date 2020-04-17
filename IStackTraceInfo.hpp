/*
 * IStackTraceInfo.hpp
 *
 *  Created on: Feb 17, 2012
 *      Author: Ilya Polenov
 */

#ifndef ISTACKTRACEINFO_HPP_
#define ISTACKTRACEINFO_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

/**
 * Classes which implement this interface provide a method that returns a stack trace.
 */
class DDMLIB_API IStackTraceInfo {
public:
	virtual std::vector<std::tr1::shared_ptr<StackTraceElement> > getStackTrace() = 0;
	virtual ~IStackTraceInfo() {};
};

} /* namespace ddmlib */
#endif /* ISTACKTRACEINFO_HPP_ */
