/*
 * RemoteAndroidTestRunner.hpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef REMOTEANDROIDTESTRUNNER_HPP_
#define REMOTEANDROIDTESTRUNNER_HPP_
#include "ddmlib.hpp"

#include "IRemoteAndroidTestRunner.hpp"

namespace ddmlib {

class Device;
class InstrumentationResultParser;

class DDMLIB_API RemoteAndroidTestRunner : public IRemoteAndroidTestRunner{
public:
	/**
	 * Creates a remote Android test runner.
	 *
	 * @param packageName the Android application package that contains the tests to run
	 * @param runnerName the instrumentation test runner to execute. If null, will use default
	 *   runner
	 * @param remoteDevice the Android device to execute tests on
	 */
	RemoteAndroidTestRunner(const std::string& packageName, const std::string& runnerName,
			std::tr1::shared_ptr<Device> remoteDevice);
	/**
	 * Alternate constructor. Uses default instrumentation runner.
	 *
	 * @param packageName the Android application package that contains the tests to run
	 * @param remoteDevice the Android device to execute tests on
	 */
    RemoteAndroidTestRunner(const std::string& packageName,
			std::tr1::shared_ptr< Device > remoteDevice);

    std::string getPackageName() {
		return mPackageName;
	}

    std::string getRunnerName() {
		if (mRunnerName.empty()) {
			return DEFAULT_RUNNER_NAME;
		}
		return mRunnerName;
	}

    /**
	 * Returns the complete instrumentation component path.
	 */
	std::string getRunnerPath() {
		return getPackageName() + RUNNER_SEPARATOR + getRunnerName();
	}
	void setClassName(const std::string& className);
	void setClassNames(std::vector<std::string>& classNames);
	void setMethodName(const std::string& className, const std::string& testName);
	void setTestPackageName(const std::string& packageName);
	void addInstrumentationArg(const std::string& name, const std::string& value);
	void removeInstrumentationArg(const std::string& name);
	void addBooleanArg(const std::string& name, bool value);
	void setLogOnly(bool logOnly);
	void setDebug(bool debug);
	void setCoverage(bool coverage);
	void setTestSize(TestSize& size);
	void setMaxtimeToOutputResponse(int maxTimeToOutputResponse);
	void setRunName(const std::string& runName);
	void run(std::vector<std::tr1::shared_ptr< ITestRunListener > >& listeners);
	void cancel();
	virtual ~RemoteAndroidTestRunner();
private:
	void Init();
	std::string mPackageName;
	std::string mRunnerName;
	std::tr1::shared_ptr<Device> mRemoteDevice;
	// default to no timeout
	int mMaxTimeToOutputResponse;
	std::string mRunName;

	/** map of name-value instrumentation argument pairs */
	std::map<std::string, std::string> mArgMap;
	std::tr1::shared_ptr<InstrumentationResultParser> mParser;

	static std::string LOG_TAG;
	static std::string DEFAULT_RUNNER_NAME;

	static char CLASS_SEPARATOR;
	static char METHOD_SEPARATOR;
	static char RUNNER_SEPARATOR;

	// defined instrumentation argument names
	static std::string CLASS_ARG_NAME;
	static std::string LOG_ARG_NAME;
	static std::string DEBUG_ARG_NAME;
	static std::string COVERAGE_ARG_NAME;
	static std::string PACKAGE_ARG_NAME;
	static std::string SIZE_ARG_NAME;

	/**
	 * Returns the full instrumentation command line syntax for the provided instrumentation
	 * arguments.
	 * Returns an empty string if no arguments were specified.
	 */
	std::string getArgsCommand();
};

} /* namespace ddmlib */
#endif /* REMOTEANDROIDTESTRUNNER_HPP_ */
