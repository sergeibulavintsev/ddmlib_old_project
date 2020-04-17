/*
 * TestIdentifier.hpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef TESTIDENTIFIER_HPP_
#define TESTIDENTIFIER_HPP_
#include "ddmlib.hpp"

#include <typeinfo>

namespace ddmlib {

class DDMLIB_API TestIdentifier {
public:
	/**
	     * Creates a test identifier.
	     *
	     * @param className fully qualified class name of the test. Cannot be null.
	     * @param testName name of the test. Cannot be null.
	     */
	TestIdentifier(const std::string& className,const std::string& testName);

	/**
	 * Returns the fully qualified class name of the test.
	 */
	std::string getClassName() {
		return mClassName;
	}

	/**
	 * Returns the name of the test.
	 */
	std::string getTestName() {
		return mTestName;
	}

	std::string toString() {
		return (std::string(typeid(*this).name()) + "#" + getTestName());
	}
	virtual ~TestIdentifier();
private:
	std::string mClassName;
	std::string mTestName;
};

} /* namespace ddmlib */
#endif /* TESTIDENTIFIER_HPP_ */
