/*
 * TestIdentifier.cpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "TestIdentifier.hpp"

namespace ddmlib {

TestIdentifier::TestIdentifier(const std::string& className,const std::string& testName) {
	// TODO Auto-generated constructor stub
	 if (className.empty() || testName.empty()) {
	            throw Poco::InvalidArgumentException(std::string("className and testName must ") +
	                    "be non-null");
	        }
	        mClassName = className;
	        mTestName = testName;
}

TestIdentifier::~TestIdentifier() {
	// TODO Auto-generated destructor stub
}

} /* namespace ddmlib */
