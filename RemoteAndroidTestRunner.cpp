/*
 * RemoteAndroidTestRunner.cpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "RemoteAndroidTestRunner.hpp"
#include "ShellCommandUnresponsiveException.hpp"
#include "Log.hpp"
#include "InstrumentationResultParser.hpp"
#include "AdbCommandRejectedException.hpp"
#include "IRemoteAndroidTestRunner.hpp"
#include "Device.hpp"

namespace ddmlib {

std::string RemoteAndroidTestRunner::LOG_TAG = "RemoteAndroidTest";
std::string RemoteAndroidTestRunner::DEFAULT_RUNNER_NAME = "android.test.InstrumentationTestRunner";

char RemoteAndroidTestRunner::CLASS_SEPARATOR = ',';
char RemoteAndroidTestRunner::METHOD_SEPARATOR = '#';
char RemoteAndroidTestRunner::RUNNER_SEPARATOR = '/';

// defined instrumentation argument names
std::string RemoteAndroidTestRunner::CLASS_ARG_NAME = "class";
std::string RemoteAndroidTestRunner::LOG_ARG_NAME = "log";
std::string RemoteAndroidTestRunner::DEBUG_ARG_NAME = "debug";
std::string RemoteAndroidTestRunner::COVERAGE_ARG_NAME = "coverage";
std::string RemoteAndroidTestRunner::PACKAGE_ARG_NAME = "package";
std::string RemoteAndroidTestRunner::SIZE_ARG_NAME = "size";

void RemoteAndroidTestRunner::Init(){
	int mMaxTimeToOutputResponse = 0;
}

RemoteAndroidTestRunner::RemoteAndroidTestRunner(const std::string& packageName, const std::string& runnerName,
		std::tr1::shared_ptr<Device> remoteDevice) {
	// TODO Auto-generated constructor stub
	Init();
    mPackageName = packageName;
    mRunnerName = runnerName;
    mRemoteDevice = remoteDevice;
    mArgMap = std::map<std::string, std::string>();

}

RemoteAndroidTestRunner::RemoteAndroidTestRunner(const std::string& packageName, std::tr1::shared_ptr<Device> remoteDevice) {
	// TODO Auto-generated destructor stub
	Init();
    mPackageName = packageName;
    mRunnerName = "";
    mRemoteDevice = remoteDevice;
    mArgMap = std::map<std::string, std::string>();
}

void RemoteAndroidTestRunner::setClassName(const std::string& className) {
	addInstrumentationArg(CLASS_ARG_NAME, className);
}

void RemoteAndroidTestRunner::setClassNames(std::vector<std::string>& classNames) {
	std::string classArgBuilder;

	for (unsigned int i = 0; i < classNames.size(); i++) {
		if (i != 0) {
			classArgBuilder.push_back(CLASS_SEPARATOR);
		}
		classArgBuilder.append(classNames[i]);
	}
	setClassName(classArgBuilder);
}

void RemoteAndroidTestRunner::setMethodName(const std::string& className, const std::string& testName) {
	setClassName(className + METHOD_SEPARATOR + testName);
}

void RemoteAndroidTestRunner::setTestPackageName(const std::string& packageName) {
	addInstrumentationArg(PACKAGE_ARG_NAME, packageName);
}

 void RemoteAndroidTestRunner::addInstrumentationArg(const std::string& name, const std::string& value) {
	if (name.empty() || value.empty()) {
		throw Poco::InvalidArgumentException("name or value arguments cannot be null");
	}
	mArgMap.insert(std::make_pair(name, value));
}

void RemoteAndroidTestRunner::removeInstrumentationArg(const std::string& name) {
	if (name.empty()) {
		throw Poco::InvalidArgumentException("name argument cannot be null");
	}
	mArgMap.erase(name);
}

void RemoteAndroidTestRunner::addBooleanArg(const std::string& name, bool value) {

	addInstrumentationArg(name, Poco::NumberFormatter::format(value));
}

void RemoteAndroidTestRunner::setLogOnly(bool logOnly) {
	addBooleanArg(LOG_ARG_NAME, logOnly);
}

void RemoteAndroidTestRunner::setDebug(bool debug) {
       addBooleanArg(DEBUG_ARG_NAME, debug);
   }

void RemoteAndroidTestRunner::setCoverage(bool coverage) {
	addBooleanArg(COVERAGE_ARG_NAME, coverage);
}


void RemoteAndroidTestRunner::setTestSize(TestSize& size) {
	addInstrumentationArg(SIZE_ARG_NAME, size.getRunnerValue());
}

void RemoteAndroidTestRunner::setMaxtimeToOutputResponse(int maxTimeToOutputResponse) {
	mMaxTimeToOutputResponse = maxTimeToOutputResponse;
}

void RemoteAndroidTestRunner::setRunName(const std::string& runName) {
	mRunName = runName;
}
void RemoteAndroidTestRunner::run(std::vector<std::tr1::shared_ptr< ITestRunListener > >& listeners){
        std::string runCaseCommandStr = std::string("am instrument -w -r ") + getArgsCommand() +" " + getRunnerPath();
        Log::i(LOG_TAG, std::string("Running ") + runCaseCommandStr + " on " + mRemoteDevice->getSerialNumber());
        std::string runName = (mRunName.empty() ? mPackageName : mRunName);
        mParser = std::tr1::shared_ptr< InstrumentationResultParser >(new InstrumentationResultParser(runName, listeners));

        try {
            mRemoteDevice->executeShellCommand(runCaseCommandStr, mParser.get(), mMaxTimeToOutputResponse);
        } catch (Poco::IOException& e) {
            Log::w(
				LOG_TAG,
				std::string("IOException ") + e.what() + " when running tests " + getPackageName() + " on "
						+ mRemoteDevice->getSerialNumber());
            // rely on parser to communicate results to listeners
            mParser->handleTestRunFailed(e.what());
            throw;
        } catch (ShellCommandUnresponsiveException& e) {
            Log::w(
				LOG_TAG,
				std::string("ShellCommandUnresponsiveException ") + e.what() + "s when running tests " + getPackageName()
						+ " on " + mRemoteDevice->getSerialNumber());
            mParser->handleTestRunFailed(std::string(
                    "Failed to receive adb shell test output within %1$d ms. ") +
                    "Test may have timed out, or adb connection to device became unresponsive" +
                    Poco::NumberFormatter::format(mMaxTimeToOutputResponse));
            throw;
        } catch (Poco::TimeoutException& e) {
            Log::w(
				LOG_TAG,
				std::string("TimeoutException when running tests ") + getPackageName() + " on "
						+ mRemoteDevice->getSerialNumber());
            mParser->handleTestRunFailed(e.what());
            throw;
        } catch (AdbCommandRejectedException& e) {
            Log::w(
				LOG_TAG,
				std::string("AdbCommandRejectedException ") + e.what() + " when running tests " + getPackageName() + " on "
						+ mRemoteDevice->getSerialNumber());
            mParser->handleTestRunFailed(e.what());
            throw;
        }
    }

void RemoteAndroidTestRunner::cancel() {
	if (mParser != nullptr) {
		mParser->cancel();
	}
}


std::string RemoteAndroidTestRunner::getArgsCommand() {
	std::string commandBuilder;
	for (std::map<std::string, std::string>::iterator argPair = mArgMap.begin(); argPair != mArgMap.end(); ++argPair) {
		std::string argCmd = std::string(" -e ") + (*argPair).first+ " " + (*argPair).second;
		commandBuilder.append(argCmd);
	}
	return commandBuilder;
}

RemoteAndroidTestRunner::~RemoteAndroidTestRunner() {
	// TODO Auto-generated destructor stub

}

} /* namespace ddmlib */
