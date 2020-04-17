/*
 * InstrumentationResultParser.hpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef INSTRUMENTATIONRESULTPARSER_HPP_
#define INSTRUMENTATIONRESULTPARSER_HPP_
#include "ddmlib.hpp"

/**
 * Parses the 'raw output mode' results of an instrumentation test run from shell and informs a
 * ITestRunListener of the results.
 *
 * <p>Expects the following output:
 *
 * <p>If fatal error occurred when attempted to run the tests:
 * <pre>
 * INSTRUMENTATION_STATUS: Error=error Message
 * INSTRUMENTATION_FAILED:
 * </pre>
 * <p>or
 * <pre>
 * INSTRUMENTATION_RESULT: shortMsg=error Message
 * </pre>
 *
 * <p>Otherwise, expect a series of test results, each one containing a set of status key/value
 * pairs, delimited by a start(1)/pass(0)/fail(-2)/error(-1) status code result. At end of test
 * run, expects that the elapsed test time in seconds will be displayed
 *
 * <p>For example:
 * <pre>
 * INSTRUMENTATION_STATUS_CODE: 1
 * INSTRUMENTATION_STATUS: class=com.foo.FooTest
 * INSTRUMENTATION_STATUS: test=testFoo
 * INSTRUMENTATION_STATUS: numtests=2
 * INSTRUMENTATION_STATUS: stack=com.foo.FooTest#testFoo:312
 *    com.foo.X
 * INSTRUMENTATION_STATUS_CODE: -2
 * ...
 *
 * Time: X
 * </pre>
 * <p>Note that the "value" portion of the key-value pair may wrap over several text lines
 */

#include "MultiLineReceiver.hpp"

namespace ddmlib {

class TestIdentifier;
class ITestRunListener;

/**
 * Test result data
 */

class DDMLIB_API InstrumentationResultParser: public MultiLineReceiver {
	struct StatusKeys {
		static std::string TEST;
		static std::string CLASS;
		static std::string STACK;
		static std::string NUMTESTS;
		static std::string ERRORS;
		static std::string SHORTMSG;
	};

	/** Test result status codes. */
	struct StatusCodes {
		static const int FAILURE;
		static const int START;
		static const int ERRORS;
		static const int OK;
		static const int IN_PROGRESS;
	};

	/** Prefixes used to identify output. */
	struct Prefixes {
		static std::string STATUS;
		static std::string STATUS_CODE;
		static std::string STATUS_FAILED;
		static std::string CODE;
		static std::string RESULT;
		static std::string TIME_REPORT;
	};

	class TestResult {
	public:
		int mCode;
		std::string mTestName;
		std::string mTestClass;
		std::string mStackTrace;
		int mNumTests;

		/** Returns true if all expected values have been parsed */
		bool isComplete() {
			return mCode != 0 && !mTestName.empty() && !mTestClass.empty();
		}

		/** Provides a more user readable string for TestResult, if possible */

	public:
		TestResult() :
				mCode(0), mTestName(""), mTestClass(""), mStackTrace(""), mNumTests(0) {};

		std::string toString() {
			std::string output;
			if (!mTestClass.empty()) {
				output.append(mTestClass);
				output.append("#");
			}
			if (!mTestName.empty()) {
				output.append(mTestName);
			}
			if (output.length() > 0) {
				return output;
			}
			return "unknown result";
		}
	};
public:

	 /** Error message supplied when no parseable test results are received from test run. */
	static std::string NO_TEST_RESULTS_MSG;

	/** Error message supplied when a test start bundle is parsed, but not the test end bundle. */
	static std::string INCOMPLETE_TEST_ERR_MSG_PREFIX;
	static std::string INCOMPLETE_TEST_ERR_MSG_POSTFIX;

	/** Error message supplied when the test run is incomplete. */
	static std::string INCOMPLETE_RUN_ERR_MSG_PREFIX;

	void Init();
	 /**
	     * Creates the InstrumentationResultParser.
	     *
	     * @param runName the test run name to provide to
	     *            {@link ITestRunListener#testRunStarted(String, int)}
	     * @param listeners informed of test results as the tests are executing
	     */
	InstrumentationResultParser(const std::string& runName, const std::vector<std::tr1::shared_ptr< ITestRunListener> >& listeners);
	InstrumentationResultParser(const std::string& runName, std::tr1::shared_ptr< ITestRunListener > listener);
	/**
	     * Processes the instrumentation test output from shell.
	     *
	     * @see MultiLineReceiver#processNewLines
	     */

	void processNewLines(const std::vector<std::string>& lines);
	/**
	 * Returns true if test run canceled.
	 *
	 * @see IShellOutputReceiver#isCancelled()
	 */
	bool isCancelled() {
		return mIsCancelled;
	}

	/**
	 * Requests cancellation of test run.
	 */
	void cancel() {
		mIsCancelled = true;
	}

	/**
	 * Process a instrumentation run failure
	 */
	void handleTestRunFailed(const std::string& errorMsg);

    /**
	 * Called by parent when adb session is complete.
	 */

	void done() {
		MultiLineReceiver::done();
		if (!mTestRunFailReported) {
			handleOutputDone();
		}
	}

	virtual ~InstrumentationResultParser();
private:
	/**
	 * Parse an individual output line. Expects a line that is one of:
	 * <ul>
	 * <li>
	 * The start of a new status line (starts with Prefixes.STATUS or Prefixes.STATUS_CODE),
	 * and thus there is a new key=value pair to parse, and the previous key-value pair is
	 * finished.
	 * </li>
	 * <li>
	 * A continuation of the previous status (the "value" portion of the key has wrapped
	 * to the next line).
	 * </li>
	 * <li> A line reporting a fatal error in the test run (Prefixes.STATUS_FAILED) </li>
	 * <li> A line reporting the total elapsed time of the test run. (Prefixes.TIME_REPORT) </li>
	 * </ul>
	 *
	 * @param line  Text output line
	 */
	void parse(const std::string& line);
	/**
	 * Stores the currently parsed key-value pair in the appropriate place.
	 */
	void submitCurrentKeyValue();
	/**
	     * A utility method to return the test metrics from the current test case execution and get
	     * ready for the next one.
	     */
	std::map<std::string, std::string> getAndResetTestMetrics();
	std::tr1::shared_ptr<TestResult> getCurrentTestInfo();
	void clearCurrentTestInfo();
	/**
	 * Parses the key from the current line.
	 * Expects format of "key=value".
	 *
	 * @param line full line of text to parse
	 * @param keyStartPos the starting position of the key in the given line
	 */
	void parseKey(const std::string& line, int keyStartPos);
	/**
	 * Parses the start of a key=value pair.
	 *
	 * @param line - full line of text to parse
	 * @param valueStartPos - the starting position of the value in the given line
	 */
	void parseValue(const std::string& line, int valueStartPos);
	/**
	 * Parses out a status code result.
	 */
	void parseStatusCode(const std::string& line);
	/**
	 * Reports a test result to the test run listener. Must be called when a individual test
	 * result has been fully parsed.
	 *
	 * @param statusMap key-value status pairs of test result
	 */
	void reportResult(std::tr1::shared_ptr<TestResult> testInfo);
	/**
	 * Reports the start of a test run, and the total test count, if it has not been previously
	 * reported.
	 *
	 * @param testInfo current test status values
	 */
	void reportTestRunStarted(std::tr1::shared_ptr<TestResult> testInfo);
	/**
	 * Returns the stack trace of the current failed test, from the provided testInfo.
	 */
	std::string getTrace(std::tr1::shared_ptr<TestResult> testInfo);
	/**
	     * Parses out and store the elapsed time.
	     */
	void parseTime(const std::string& line);

    /**
     * Handles the end of the adb session when a test run failure has not been reported yet
     */
    void handleOutputDone();

private:

	std::vector<std::tr1::shared_ptr<ITestRunListener> > mTestListeners;

	/** The set of expected status keys. Used to filter which keys should be stored as metrics */
	static std::set<std::string> KNOWN_KEYS;

	/** the name to provide to {@link ITestRunListener#testRunStarted(String, int)} */
	std::string mTestRunName;

	/** Stores the status values for the test result currently being parsed */
	std::tr1::shared_ptr<TestResult> mCurrentTestResult;

	/** Stores the status values for the test result last parsed */
	std::tr1::shared_ptr<TestResult> mLastTestResult;

	/** Stores the current "key" portion of the status key-value being parsed. */
	std::string mCurrentKey;

	/** Stores the current "value" portion of the status key-value being parsed. */
	std::string mCurrentValue;

	/** True if start of test has already been reported to listener. */
	bool mTestStartReported;

	/** True if the completion of the test run has been detected. */
	bool mTestRunFinished;

	/** True if test run failure has already been reported to listener. */
	bool mTestRunFailReported;

	/** The elapsed time of the test run, in milliseconds. */
	long long mTestTime;

	/** True if current test run has been canceled by user. */
	bool mIsCancelled;

	/** The number of tests currently run  */
	int mNumTestsRun;

	/** The number of tests expected to run  */
	int mNumTestsExpected;

	/** True if the parser is parsing a line beginning with "INSTRUMENTATION_RESULT" */
	bool mInInstrumentationResultKey;

	/**
	 * Stores key-value pairs under INSTRUMENTATION_RESULT header, these are printed at the
	 * end of a test run, if applicable
	 */
	std::map<std::string, std::string> mInstrumentationResultBundle;

	/**
	 * Stores key-value pairs of metrics emitted during the execution of each test case.  Note that
	 * standard keys that are stored in the TestResults class are filtered out of this Map.
	 */
	std::map<std::string, std::string> mTestMetrics;

	static std::string LOG_TAG;

};

} /* namespace ddmlib */
#endif /* INSTRUMENTATIONRESULTPARSER_HPP_ */
