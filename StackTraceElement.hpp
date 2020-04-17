/*
 * StackTraceElement.hpp
 *
 *  Created on: Feb 8, 2012
 *      Author: Ilya Polenov
 */

#ifndef STACKTRACEELEMENT_HPP_
#define STACKTRACEELEMENT_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

// Copies Java's StackTraceElement
class DDMLIB_API StackTraceElement {
	std::wstring cls;
	std::wstring method;
	std::wstring file;
	int line;
public:
	StackTraceElement(const std::wstring &cls, const std::wstring &method, const std::wstring &file, int line) :
			cls(cls), method(method), file(file), line(line) {
	}


	virtual ~StackTraceElement();
	std::wstring getClassName() const;
	std::wstring getFileName() const;
	std::wstring getMethodName() const;
	int getLineNumber() const;

	std::wstring toString() const;
};

} /* namespace ddmlib */
#endif /* STACKTRACEELEMENT_HPP_ */
