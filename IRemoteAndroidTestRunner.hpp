/*
 * IRemoteAndroidTestRunner.hpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef IREMOTEANDROIDTESTRUNNER_HPP_
#define IREMOTEANDROIDTESTRUNNER_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class ITestRunListener;

class DDMLIB_API TestSize {

	std::string mRunnerValue;
	static std::vector<TestSize*> values;

public:
	/**
	 * Create a {@link TestSize}.
	 *
	 * @param runnerValue the {@link String} value that represents the size that is passed to
	 * device. Defined on device in android.test.InstrumentationTestRunner.
	 */
	TestSize(const std::string& runnerValue) {
		mRunnerValue = runnerValue;
		values.push_back(this);
	}

	std::string getRunnerValue() {
		return mRunnerValue;
	}

	/**
	 * Return the {@link TestSize} corresponding to the given Android platform defined value.
	 *
	 * @throws IllegalArgumentException if {@link TestSize} cannot be found.
	 */
	static TestSize* getTestSize(const std::string& value) {
		// build the error message in the success case too, to avoid two for loops
		std::string msgBuilder("Unknown TestSize ");
		msgBuilder.append(value);
		msgBuilder.append(", Must be one of ");
		for (std::vector<TestSize*>::iterator size = values.begin(); size != values.end(); ++size) {
			if ((*size)->getRunnerValue() == value) {
				return *size;
			}
			msgBuilder.append((*size)->getRunnerValue());
			msgBuilder.append(", ");
		}
		throw Poco::InvalidArgumentException(msgBuilder);
	}
};

class DDMLIB_API IRemoteAndroidTestRunner {
public:
	/** Run tests annotated with SmallTest */
	static TestSize SMALL;
	/** Run tests annotated with MediumTest */
	static TestSize MEDIUM;
	/** Run tests annotated with LargeTest */
	static TestSize LARGE;
	/**
	 * Returns the application package name.
	 */
	virtual std::string getPackageName() = 0;

	/**
	 * Returns the runnerName.
	 */
	virtual std::string getRunnerName() = 0;

	/**
	 * Sets to run only tests in this class
	 * Must be called before 'run'.
	 *
	 * @param className fully qualified class name (eg x.y.z)
	 */
	virtual void setClassName(const std::string& className) = 0;

	/**
	 * Sets to run only tests in the provided classes
	 * Must be called before 'run'.
	 * <p>
	 * If providing more than one class, requires a InstrumentationTestRunner that supports
	 * the multiple class argument syntax.
	 *
	 * @param classNames array of fully qualified class names (eg x.y.z)
	 */
	virtual void setClassNames(std::vector<std::string>& classNames) = 0;

	/**
	 * Sets to run only specified test method
	 * Must be called before 'run'.
	 *
	 * @param className fully qualified class name (eg x.y.z)
	 * @param testName method name
	 */
	virtual void setMethodName(const std::string& className, const std::string& testName) = 0;

	/**
	 * Sets to run all tests in specified package
	 * Must be called before 'run'.
	 *
	 * @param packageName fully qualified package name (eg x.y.z)
	 */
	virtual void setTestPackageName(const std::string& packageName) = 0;

	/**
	 * Sets to run only tests of given size.
	 * Must be called before 'run'.
	 *
	 * @param size the {@link TestSize} to run.
	 */
	virtual void setTestSize(TestSize& size) = 0;

	/**
	 * Adds a argument to include in instrumentation command.
	 * <p/>
	 * Must be called before 'run'. If an argument with given name has already been provided, it's
	 * value will be overridden.
	 *
	 * @param name the name of the instrumentation bundle argument
	 * @param value the value of the argument
	 */
	virtual void addInstrumentationArg(const std::string& name, const std::string& value) = 0;

	/**
	 * Removes a previously added argument.
	 *
	 * @param name the name of the instrumentation bundle argument to remove
	 */
	virtual void removeInstrumentationArg(const std::string& name) = 0;

	/**
	 * Adds a boolean argument to include in instrumentation command.
	 * <p/>
	 * @see RemoteAndroidTestRunner#addInstrumentationArg
	 *
	 * @param name the name of the instrumentation bundle argument
	 * @param value the value of the argument
	 */
	virtual void addBooleanArg(const std::string& name, bool value) = 0;

	/**
	 * Sets this test run to log only mode - skips test execution.
	 */
	virtual void setLogOnly(bool logOnly) = 0;

	/**
	 * Sets this debug mode of this test run. If true, the Android test runner will wait for a
	 * debugger to attach before proceeding with test execution.
	 */
	virtual void setDebug(bool debug) = 0;

	/**
	 * Sets this code coverage mode of this test run.
	 */
	virtual void setCoverage(bool coverage) = 0;

	/**
	 * Sets the maximum time allowed between output of the shell command running the tests on
	 * the devices.
	 * <p/>
	 * This allows setting a timeout in case the tests can become stuck and never finish. This is
	 * different from the normal timeout on the connection.
	 * <p/>
	 * By default no timeout will be specified.
	 *
	 * @see {@link Device#executeShellCommand(String, com.android.ddmlib.IShellOutputReceiver, int)}
	 */
	virtual void setMaxtimeToOutputResponse(int maxTimeToOutputResponse) = 0;

	/**
	 * Set a custom run name to be reported to the {@link ITestRunListener} on {@link #run}
	 * <p/>
	 * If unspecified, will use package name
	 *
	 * @param runName
	 */
	virtual void setRunName(const std::string& runName) = 0;

	/**
	 * Execute this test run.
	 * <p/>
	 * Convenience method for {@link #run(Collection)}.
	 *
	 * @param listeners listens for test results
	 * @throws TimeoutException in case of a timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws ShellCommandUnresponsiveException if the device did not output any test result for
	 * a period longer than the max time to output.
	 * @throws IOException if connection to device was lost.
	 *
	 * @see #setMaxtimeToOutputResponse(int)
	 */
	virtual void run(std::vector<std::tr1::shared_ptr<ITestRunListener> > listeners) = 0;

	/**
	 * Requests cancellation of this test run.
	 */
	virtual void cancel() = 0;
	IRemoteAndroidTestRunner();
	virtual ~IRemoteAndroidTestRunner();
};

} /* namespace ddmlib */
#endif /* IREMOTEANDROIDTESTRUNNER_HPP_ */
