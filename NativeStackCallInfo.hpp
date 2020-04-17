/*
 * NativeStackCallInfo.hpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Ilya Polenov
 */

#ifndef NATIVESTACKCALLINFO_HPP_
#define NATIVESTACKCALLINFO_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class DDMLIB_API NativeStackCallInfo {
	bool invalid; // true if was made using default constructor

	static Poco::RegularExpression SOURCE_NAME_PATTERN;

	int mLineNumber;

	/** address of this stack frame */
	long mAddress;

	/** name of the library */
	std::string mLibrary;

	/** name of the method */
	std::string mMethod;

	/**
	 * name of the source file + line number in the format<br>
	 * &lt;sourcefile&gt;:&lt;linenumber&gt;
	 */
	std::string mSourceFile;

public:
	bool isInvalid() {
		return invalid;
	}

	/**
	 * Basic constructor with library, method, and sourcefile information
	 *
	 * @param address address of this stack frame
	 * @param lib The name of the library
	 * @param method the name of the method
	 * @param sourceFile the name of the source file and the line number
	 * as "[sourcefile]:[fileNumber]"
	 */
	NativeStackCallInfo(long address, std::string lib, std::string method, std::string sourceFile);

	// Constructs invalid
	NativeStackCallInfo() :
			invalid(true), mLineNumber(0), mAddress(0) {
	}

	virtual ~NativeStackCallInfo();

	long getAddress() const;
	std::string getLibraryName() const;
	int getLineNumber() const;
	std::string getMethodName() const;
	std::string getSourceFile() const;
};

} /* namespace ddmlib */
#endif /* NATIVESTACKCALLINFO_HPP_ */
