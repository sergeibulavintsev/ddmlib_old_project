/*
 * StackTraceElement.cpp
 *
 *  Created on: Feb 8, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "StackTraceElement.hpp"

namespace ddmlib {

std::wstring StackTraceElement::getClassName() const {
	return cls;
}

std::wstring StackTraceElement::getFileName() const {
	return file;
}

int StackTraceElement::getLineNumber() const {
	return line;
}

std::wstring StackTraceElement::getMethodName() const {
	return method;
}

std::wstring StackTraceElement::toString() const {
	return (getClassName() + std::wstring(L"@"));
}

StackTraceElement::~StackTraceElement() {
}

} /* namespace ddmlib */
