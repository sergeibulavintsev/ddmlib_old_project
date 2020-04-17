/*
 * ITestRunListener.hpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef ITESTRUNLISTENER_HPP_
#define ITESTRUNLISTENER_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class TestIdentifier;

/**
 * Receives event notifications during instrumentation test runs.
 * <p/>
 * Patterned after {@link junit.runner.TestRunListener}.
 * <p/>
 * The sequence of calls will be:
 * <ul>
 * <li> testRunStarted
 * <li> testStarted
 * <li> [testFailed]
 * <li> testEnded
 * <li> ....
 * <li> [testRunFailed]
 * <li> testRunEnded
 * </ul>
 */
class DDMLIB_API ITestRunListener {
public:

    /**
     *  Types of test failures.
     */
    enum TestFailure {
        /** Test failed due to unanticipated uncaught exception. */
        TEST_ERROR,
        /** Test failed due to a false assertion. */
        TEST_FAILURE
    };

    /**
     * Reports the start of a test run.
     *
     * @param runName the test run name
     * @param testCount total number of tests in test run
     */
    virtual void testRunStarted(const std::string& runName, int testCount) = 0;

    /**
     * Reports the start of an individual test case.
     *
     * @param test identifies the test
     */
    virtual void testStarted(std::tr1::shared_ptr< TestIdentifier > test) = 0;

    /**
     * Reports the failure of a individual test case.
     * <p/>
     * Will be called between testStarted and testEnded.
     *
     * @param status failure type
     * @param test identifies the test
     * @param trace stack trace of failure
     */
    virtual void testFailed(TestFailure status, std::tr1::shared_ptr< TestIdentifier > test, const std::string& trace) = 0;

    /**
     * Reports the execution end of an individual test case.
     * <p/>
     * If {@link #testFailed} was not invoked, this test passed.  Also returns any key/value
     * metrics which may have been emitted during the test case's execution.
     *
     * @param test identifies the test
     * @param testMetrics a {@link Map} of the metrics emitted
     */
    virtual void testEnded(std::tr1::shared_ptr< TestIdentifier > test, const std::map<std::string, std::string>& testMetrics) = 0;

    /**
     * Reports test run failed to complete due to a fatal error.
     *
     * @param errorMessage {@link String} describing reason for run failure.
     */
    virtual void testRunFailed(const std::string& errorMessage) = 0;

    /**
     * Reports test run stopped before completion due to a user request.
     * <p/>
     * TODO: currently unused, consider removing
     *
     * @param elapsedTime device reported elapsed time, in milliseconds
     */
    virtual void testRunStopped(long long elapsedTime) = 0;

    /**
     * Reports end of test run.
     *
     * @param elapsedTime device reported elapsed time, in milliseconds
     * @param runMetrics key-value pairs reported at the end of a test run
     */
    virtual void testRunEnded(long long elapsedTime, std::map<std::string, std::string>& runMetrics) = 0;
	ITestRunListener() {}
	virtual ~ITestRunListener() {};
};

} /* namespace ddmlib */
#endif /* ITESTRUNLISTENER_HPP_ */
