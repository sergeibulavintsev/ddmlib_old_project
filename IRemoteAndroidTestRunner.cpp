/*
 * IRemoteAndroidTestRunner.cpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "IRemoteAndroidTestRunner.hpp"

namespace ddmlib {

std::vector<TestSize*> TestSize::values;

TestSize IRemoteAndroidTestRunner::SMALL("small"), /** Run tests annotated with SmallTest */
/** Run tests annotated with MediumTest */
		IRemoteAndroidTestRunner::MEDIUM("medium"),
/** Run tests annotated with LargeTest */
		IRemoteAndroidTestRunner::LARGE("large");

IRemoteAndroidTestRunner::IRemoteAndroidTestRunner() {
	// TODO Auto-generated constructor stub

}

IRemoteAndroidTestRunner::~IRemoteAndroidTestRunner() {
	// TODO Auto-generated destructor stub
}

} /* namespace ddmlib */
