/*
 * NativeStackCallInfo.cpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "NativeStackCallInfo.hpp"

namespace ddmlib {
Poco::RegularExpression NativeStackCallInfo::SOURCE_NAME_PATTERN("^(.+):(\\d+)$");

NativeStackCallInfo::NativeStackCallInfo(long address, std::string lib, std::string method, std::string sourceFile) :
		mAddress(address), mLibrary(lib), mMethod(method) {
	invalid = true;
	std::vector<std::string> matches;
	SOURCE_NAME_PATTERN.split(sourceFile, matches);
	if (matches.size() > 2) {
		mSourceFile = matches[1];
		try {
			mLineNumber = Poco::NumberParser::parse(matches[2]);
		} catch (Poco::SyntaxException &e) {
			mLineNumber = -1;
		}
	} else {
		mSourceFile = sourceFile;
	}
	invalid = false;
}

long NativeStackCallInfo::getAddress() const {
	return mAddress;
}

std::string NativeStackCallInfo::getLibraryName() const {
	return mLibrary;
}

int NativeStackCallInfo::getLineNumber() const {
	return mLineNumber;
}

std::string NativeStackCallInfo::getMethodName() const {
	return mMethod;
}

std::string NativeStackCallInfo::getSourceFile() const {
	return mSourceFile;
}

NativeStackCallInfo::~NativeStackCallInfo() {
}

} /* namespace ddmlib */
