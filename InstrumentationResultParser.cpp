/*
 * InstrumentationResultParser.cpp
 *
 *  Created on: 05.03.2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "InstrumentationResultParser.hpp"
#include "ITestRunListener.hpp"
#include "TestIdentifier.hpp"
#include "Log.hpp"

namespace ddmlib {

std::string InstrumentationResultParser::StatusKeys::TEST = "test";
std::string InstrumentationResultParser::StatusKeys::CLASS = "class";
std::string InstrumentationResultParser::StatusKeys::STACK = "stack";
std::string InstrumentationResultParser::StatusKeys::NUMTESTS = "numtests";
std::string InstrumentationResultParser::StatusKeys::ERRORS = "Error";
std::string InstrumentationResultParser::StatusKeys::SHORTMSG = "shortMsg";

const int InstrumentationResultParser::StatusCodes::FAILURE = -2;
const int InstrumentationResultParser::StatusCodes::START = 1;
const int InstrumentationResultParser::StatusCodes::ERRORS = -1;
const int InstrumentationResultParser::StatusCodes::OK = 0;
const int InstrumentationResultParser::StatusCodes::IN_PROGRESS = 2;

std::string InstrumentationResultParser::Prefixes::STATUS = "INSTRUMENTATION_STATUS: ";
std::string InstrumentationResultParser::Prefixes::STATUS_CODE = "INSTRUMENTATION_STATUS_CODE: ";
std::string InstrumentationResultParser::Prefixes::STATUS_FAILED = "INSTRUMENTATION_FAILED: ";
std::string InstrumentationResultParser::Prefixes::CODE = "INSTRUMENTATION_CODE: ";
std::string InstrumentationResultParser::Prefixes::RESULT = "INSTRUMENTATION_RESULT: ";
std::string InstrumentationResultParser::Prefixes::TIME_REPORT = "Time: ";


std::string InstrumentationResultParser::NO_TEST_RESULTS_MSG = "No test results";
std::string InstrumentationResultParser::INCOMPLETE_TEST_ERR_MSG_PREFIX = "Test failed to run to completion";
std::string InstrumentationResultParser::INCOMPLETE_TEST_ERR_MSG_POSTFIX = "Check device logcat for details";
std::string InstrumentationResultParser::INCOMPLETE_RUN_ERR_MSG_PREFIX = "Test run failed to complete";

std::set<std::string> InstrumentationResultParser::KNOWN_KEYS;

std::string InstrumentationResultParser::LOG_TAG = "InstrumentationResultParser";

InstrumentationResultParser::InstrumentationResultParser(const std::string& runName, std::tr1::shared_ptr< ITestRunListener > listener) {
	// TODO Auto-generated constructor stub
	Init();
    mTestRunName = runName;
    mTestListeners.resize(1);
    mTestListeners.push_back(listener);

}

InstrumentationResultParser::InstrumentationResultParser(const std::string& runName, const std::vector<std::tr1::shared_ptr< ITestRunListener> >& listeners){
	Init();
	mTestRunName = runName;
	mTestListeners = listeners;
}

void InstrumentationResultParser::Init(){
    /** Stores the status values for the test result currently being parsed */
    mCurrentTestResult.reset();

    /** Stores the status values for the test result last parsed */
    mLastTestResult.reset();

    /** True if start of test has already been reported to listener. */
    mTestStartReported = false;

    /** True if the completion of the test run has been detected. */
    mTestRunFinished = false;

    /** True if test run failure has already been reported to listener. */
    mTestRunFailReported = false;

    /** The elapsed time of the test run, in milliseconds. */
    mTestTime = 0;

    /** True if current test run has been canceled by user. */
    mIsCancelled = false;

    /** The number of tests currently run  */
    mNumTestsRun = 0;

    /** The number of tests expected to run  */
    mNumTestsExpected = 0;

    /** True if the parser is parsing a line beginning with "INSTRUMENTATION_RESULT" */
    mInInstrumentationResultKey = false;
}

InstrumentationResultParser::~InstrumentationResultParser() {
	// TODO Auto-generated destructor stub
}

void InstrumentationResultParser::processNewLines(const std::vector<std::string>& lines) {
	for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
		parse(*line);
		// in verbose mode, dump all adb output to log
		Log::v(LOG_TAG, *line);
	}
}

void InstrumentationResultParser::parse(const std::string& line) {
	if (line.find(Prefixes::STATUS_CODE) == 0) {
		// Previous status key-value has been collected. Store it.
		submitCurrentKeyValue();
		mInInstrumentationResultKey = false;
		parseStatusCode(line);
	} else if (line.find(Prefixes::STATUS) == 0) {
		// Previous status key-value has been collected. Store it.
		submitCurrentKeyValue();
		mInInstrumentationResultKey = false;
		parseKey(line, Prefixes::STATUS.length());
	} else if (line.find(Prefixes::RESULT) == 0) {
		// Previous status key-value has been collected. Store it.
		submitCurrentKeyValue();
		mInInstrumentationResultKey = true;
		parseKey(line, Prefixes::RESULT.length());
	} else if (line.find(Prefixes::STATUS_FAILED) == 0 || line.find(Prefixes::CODE) == 0) {
		// Previous status key-value has been collected. Store it.
		submitCurrentKeyValue();
		mInInstrumentationResultKey = false;
		// these codes signal the end of the instrumentation run
		mTestRunFinished = true;
		// just ignore the remaining data on this line
	} else if (line.find(Prefixes::TIME_REPORT) != std::string::npos) {
		parseTime(line);
	} else {
		if (!mCurrentValue.empty()) {
			// this is a value that has wrapped to next line.
			mCurrentValue.append("\r\n");
			mCurrentValue.append(line);
		} else {
			std::string temp(line);
			temp.erase(0, temp.find_first_not_of(" \t\f\v\n\r"));
			temp.erase(temp.find_last_not_of(" \t\f\v\n\r") + 1, std::string::npos);
			if (temp.length() > 0) {
				Log::d(LOG_TAG, "unrecognized line " + temp);
			}
		}
	}
}

void InstrumentationResultParser::submitCurrentKeyValue() {
	if (!mCurrentKey.empty() && !mCurrentValue.empty()) {
		std::string statusValue = mCurrentValue;
		if (mInInstrumentationResultKey) {
			if (!KNOWN_KEYS.count(mCurrentKey)) {
				mInstrumentationResultBundle.insert(std::make_pair(mCurrentKey, statusValue));
			} else if (mCurrentKey == StatusKeys::SHORTMSG) {
				// test run must have failed
				handleTestRunFailed("Instrumentation run failed due to '" + statusValue + "'");
			}
		} else {
			std::tr1::shared_ptr<TestResult> testInfo = getCurrentTestInfo();

			if (mCurrentKey == StatusKeys::CLASS) {
				std::string temp(statusValue);
				temp.erase(0, temp.find_first_not_of(" \t\f\v\n\r"));
				temp.erase(temp.find_last_not_of(" \t\f\v\n\r") + 1, std::string::npos);
				testInfo->mTestClass = temp;
			} else if (mCurrentKey == StatusKeys::TEST) {
				std::string temp(statusValue);
				temp.erase(0, temp.find_first_not_of(" \t\f\v\n\r"));
				temp.erase(temp.find_last_not_of(" \t\f\v\n\r") + 1, std::string::npos);
				testInfo->mTestName = temp;
			} else if (mCurrentKey == StatusKeys::NUMTESTS) {
				try {
					testInfo->mNumTests = Poco::NumberParser::parse(statusValue);
				} catch (Poco::DataFormatException& e) {
					Log::w(LOG_TAG, "Unexpected integer number of tests, received " + statusValue);
				}
			} else if (mCurrentKey == StatusKeys::ERRORS) {
				// test run must have failed
				handleTestRunFailed(statusValue);
			} else if (mCurrentKey == StatusKeys::STACK) {
				testInfo->mStackTrace = statusValue;
			} else if (!KNOWN_KEYS.count(mCurrentKey)) {
				// Not one of the recognized key/value pairs, so dump it in mTestMetrics
				mTestMetrics.insert(std::make_pair(mCurrentKey, statusValue));
			}
		}

		mCurrentKey.clear();
		mCurrentValue.clear();
	}
}

std::map<std::string, std::string> InstrumentationResultParser::getAndResetTestMetrics() {
	std::map < std::string, std::string > retVal = mTestMetrics;
	mTestMetrics.clear();
	return retVal;
}

std::tr1::shared_ptr<InstrumentationResultParser::TestResult> InstrumentationResultParser::getCurrentTestInfo() {
	if (mCurrentTestResult == nullptr) {
		mCurrentTestResult = std::tr1::shared_ptr<TestResult>(new TestResult);
	}
	return mCurrentTestResult;
}

void InstrumentationResultParser::clearCurrentTestInfo() {
	mLastTestResult = mCurrentTestResult;
	mCurrentTestResult.reset();
}


void InstrumentationResultParser::parseKey(const std::string& line, int keyStartPos) {
	int endKeyPos = line.find('=', keyStartPos);
	if (endKeyPos != -1) {
		std::string temp = line.substr(keyStartPos, endKeyPos);
		temp.erase(0, temp.find_first_not_of(" \t\f\v\n\r"));
		temp.erase(temp.find_last_not_of(" \t\f\v\n\r") + 1, std::string::npos);
		mCurrentKey = temp;
		parseValue(line, endKeyPos + 1);
	}
}

void InstrumentationResultParser::parseValue(const std::string& line, int valueStartPos) {
	mCurrentValue = std::string();
	mCurrentValue.append(line.substr(valueStartPos));
}


void InstrumentationResultParser::parseStatusCode(const std::string& line) {
	std::string temp = line.substr(Prefixes::STATUS_CODE.length());
	temp.erase(0, temp.find_first_not_of(" \t\f\v\n\r"));
	temp.erase(temp.find_last_not_of(" \t\f\v\n\r") + 1, std::string::npos);
	std::string value = temp;
	std::tr1::shared_ptr<TestResult> testInfo = getCurrentTestInfo();
	testInfo->mCode = StatusCodes::ERRORS;
	try {
		testInfo->mCode = Poco::NumberParser::parse(value);
	} catch (Poco::DataFormatException& e) {
		Log::w(LOG_TAG, "Expected integer status code, received: " + value);
		testInfo->mCode = StatusCodes::ERRORS;
	}
	if (testInfo->mCode != StatusCodes::IN_PROGRESS) {
		// this means we're done with current test result bundle
		reportResult(testInfo);
		clearCurrentTestInfo();
	}
}

void InstrumentationResultParser::reportResult(std::tr1::shared_ptr<TestResult> testInfo) {
	if (!testInfo->isComplete()) {
		Log::w(LOG_TAG, "invalid instrumentation status bundle " + testInfo->toString());
		return;
	}
	reportTestRunStarted(testInfo);
	std::tr1::shared_ptr<TestIdentifier> testId(new TestIdentifier(testInfo->mTestClass, testInfo->mTestName));
	std::map<std::string, std::string> metrics;

	switch (testInfo->mCode) {
	case StatusCodes::START:
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			(*listener)->testStarted(testId);
		}
		break;
	case StatusCodes::FAILURE:
		metrics = getAndResetTestMetrics();
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			(*listener)->testFailed(ITestRunListener::TestFailure::TEST_FAILURE, testId, getTrace(testInfo));

			(*listener)->testEnded(testId, metrics);
		}
		mNumTestsRun++;
		break;
	case StatusCodes::ERRORS:
		metrics = getAndResetTestMetrics();
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			(*listener)->testFailed(ITestRunListener::TestFailure::TEST_ERROR, testId, getTrace(testInfo));
			(*listener)->testEnded(testId, metrics);
		}
		mNumTestsRun++;
		break;
	case StatusCodes::OK:
		metrics = getAndResetTestMetrics();
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			(*listener)->testEnded(testId, metrics);
		}
		mNumTestsRun++;
		break;
	default:
		metrics = getAndResetTestMetrics();
		Log::e(LOG_TAG, "Unknown status code received: " + Poco::NumberFormatter::format(testInfo->mCode));
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			(*listener)->testEnded(testId, metrics);
		}
		mNumTestsRun++;
		break;
	}

}

void InstrumentationResultParser::reportTestRunStarted(std::tr1::shared_ptr<TestResult> testInfo) {
	// if start test run not reported yet
	if (!mTestStartReported && testInfo->mNumTests != 0) {
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			(*listener)->testRunStarted(mTestRunName, testInfo->mNumTests);
		}
		mNumTestsExpected = testInfo->mNumTests;
		mTestStartReported = true;
	}
}

std::string InstrumentationResultParser::getTrace(std::tr1::shared_ptr<TestResult> testInfo) {
	if (!testInfo->mStackTrace.empty()) {
		return testInfo->mStackTrace;
	} else {
		Log::e(LOG_TAG, "Could not find stack trace for failed test ");
		return "Unknown failure";
	}
}

void InstrumentationResultParser::parseTime(const std::string& line) {
	Poco::RegularExpression timePattern(Prefixes::TIME_REPORT + "\\s*([\\d\\.]+)");
	std::vector<std::string> group;
	timePattern.split(line, group);
	if (group.size() > 1 ) {
		std::string timeString = group.at(1);
		try {
			double timeSeconds = Poco::NumberParser::parseFloat(timeString);
			mTestTime = (long long) (timeSeconds * 1000);
		} catch (Poco::DataFormatException& e) {
			Log::w(LOG_TAG, std::string("Unexpected time format " + line));
		}
	} else {
		Log::w(LOG_TAG, std::string("Unexpected time format " + line));
	}
}

void InstrumentationResultParser::handleTestRunFailed(const std::string& errorMsg) {
	std::string errorMessage = (errorMsg.empty() ? "Unknown error" : errorMsg);
	Log::i(LOG_TAG, std::string("test run failed: '%1$s'") + errorMessage);
	if (mLastTestResult != nullptr && mLastTestResult->isComplete() && StatusCodes::START == mLastTestResult->mCode) {

		// received test start msg, but not test complete
		// assume test caused this, report as test failure
		std::tr1::shared_ptr<TestIdentifier> testId(new TestIdentifier(mLastTestResult->mTestClass, mLastTestResult->mTestName));
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			(*listener)->testFailed(
					ITestRunListener::TestFailure::TEST_ERROR,
					testId,
					errorMessage + ". Reason: '" + INCOMPLETE_TEST_ERR_MSG_PREFIX + "'." + INCOMPLETE_TEST_ERR_MSG_POSTFIX);
			(*listener)->testEnded(testId, getAndResetTestMetrics());
		}
	}
	for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
		if (!mTestStartReported) {
			// test run wasn't started - must have crashed before it started
			(*listener)->testRunStarted(mTestRunName, 0);
		}
		(*listener)->testRunFailed(errorMsg);
		(*listener)->testRunEnded(mTestTime, mInstrumentationResultBundle);
	}
	mTestStartReported = true;
	mTestRunFailReported = true;
}

void InstrumentationResultParser::handleOutputDone() {
	if (!mTestStartReported && !mTestRunFinished) {
		// no results
		handleTestRunFailed(NO_TEST_RESULTS_MSG);
	} else if (mNumTestsExpected > mNumTestsRun) {
		std::string message = INCOMPLETE_RUN_ERR_MSG_PREFIX + ". Expected " + Poco::NumberFormatter::format(mNumTestsExpected)
				+ " tests, received " + Poco::NumberFormatter::format(mNumTestsRun);
		handleTestRunFailed(message);
	} else {
		for (std::vector<std::tr1::shared_ptr<ITestRunListener> >::iterator listener = mTestListeners.begin(); listener != mTestListeners.end(); ++listener) {
			if (!mTestStartReported) {
				// test run wasn't started, but it finished successfully. Must be a run with
				// no tests
				(*listener)->testRunStarted(mTestRunName, 0);
			}
			(*listener)->testRunEnded(mTestTime, mInstrumentationResultBundle);
		}
	}
}

} /* namespace ddmlib */
