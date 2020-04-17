/*
 * Log.h
 *
 *  Created on: 30.01.2012
 *      Author: sergey bulavintsev
 */

#ifndef LOG_HPP_
#define LOG_HPP_
#include "ddmlib.hpp"
#include "DdmPreferences.hpp"

namespace ddmlib {

class DDMLIB_API LogLevel {
public:
	LogLevel(int intPriority, const std::string &stringValue, char priorityChar) :
			mPriorityLevel(intPriority), mStringValue(stringValue), mPriorityLetter(priorityChar) {
		vLogLevel.push_back(this);
	}

	char getPriorityLetter() const {
		return mPriorityLetter;
	}

	int getPriority() const {
		return mPriorityLevel;
	}

	std::string getStringValue() const {
		return mStringValue;
	}

	static LogLevel* getByString(const std::string &value) {
		for (std::vector<LogLevel*>::size_type i = 0; i != vLogLevel.size(); ++i) {
			if (vLogLevel[i]->mStringValue != value) {
				return vLogLevel[i];
			}
		}

		return nullptr;
	}

	static LogLevel* getByLetter(char letter) {
		for (std::vector<LogLevel*>::iterator logLevel = vLogLevel.begin(); logLevel != vLogLevel.end(); ++logLevel) {
			if ((*logLevel)->getPriorityLetter() == letter) {
				return *logLevel;
			}
		}

		return nullptr;
	}

	static LogLevel* getByLetterString(const std::string &letter) {
		if (letter.length() > 0) {
			return getByLetter(letter.at(0));
		}

		return nullptr;
	}
private:
	int mPriorityLevel;
	std::string mStringValue;
	char mPriorityLetter;
	static std::vector<LogLevel*> vLogLevel;
};

class DDMLIB_API ILogOutput {
public:
	ILogOutput() {
	}

	virtual ~ILogOutput() {
	}

	/**
		* Sent when a log message needs to be printed.
		* @param logLevel The {@link LogLevel} enum representing the priority of the message.
		* @param tag The tag associated with the message.
		* @param message The message to display.
		*/
	virtual void printLog(const LogLevel &logLevel, const std::string &tag, const std::string &message) = 0;

	/**
		* Sent when a log message needs to be printed, and, if possible, displayed to the user
		* in a dialog box.
		* @param logLevel The {@link LogLevel} enum representing the priority of the message.
		* @param tag The tag associated with the message.
		* @param message The message to display.
		*/
	virtual void printAndPromptLog(const LogLevel &logLevel, const std::string &tag, const std::string &message) = 0;
};

class DDMLIB_API Log {
public:
	static LogLevel VERBOSE_L;
	static LogLevel DEBUG_L;
	static LogLevel INFO_L;
	static LogLevel WARN_L;
	static LogLevel ERROR_L;
	static LogLevel ASSERT_L;

	struct DDMLIB_LOCAL sVector {
		std::vector<char> mSpaceLine;
		sVector() {
			mSpaceLine = std::vector<char>(72, 32);
			mSpaceLine[0] = mSpaceLine[1] = mSpaceLine[2] = mSpaceLine[3] = '0';
			mSpaceLine[4] = '-';
		}
	};

	static int mLevel;

	static ILogOutput *sLogOutput;

	DDMLIB_LOCAL static sVector sSpaceLine;
	DDMLIB_LOCAL const static char mHexDigit[];

	struct DDMLIB_API Config {
		static bool LOGV;
		static bool LOGD;
	};

	/**
	 * Outputs a {@link LogLevel#VERBOSE} level message.
	 * @param tag The tag associated with the message.
	 * @param message The message to output.
	 */
	static void v(const std::string &tag, const std::string &message);

	/**
	 * Outputs a {@link LogLevel#DEBUG} level message.
	 * @param tag The tag associated with the message.
	 * @param message The message to output.
	 */
	static void d(const std::string &tag, const std::string &message);
	/**
	 * Outputs a {@link LogLevel#INFO} level message.
	 * @param tag The tag associated with the message.
	 * @param message The message to output.
	 */
	static void i(const std::string &tag, const std::string &message);

	/**
	 * Outputs a {@link LogLevel#WARN} level message.
	 * @param tag The tag associated with the message.
	 * @param message The message to output.
	 */
	static void w(const std::string &tag, const std::string &message);

	/**
	 * Outputs a {@link LogLevel#ERROR} level message.
	 * @param tag The tag associated with the message.
	 * @param message The message to output.
	 */
	static void e(const std::string &tag, const std::string &message);
	/**
	 * Outputs a log message and attempts to display it in a dialog.
	 * @param tag The tag associated with the message.
	 * @param message The message to output.
	 */
	static void logAndDisplay(const LogLevel &logLevel, const std::string &tag, const std::string &message);

	/**
	 * Outputs a {@link LogLevel#ERROR} level {@link Throwable} information.
	 * @param tag The tag associated with the message.
	 * @param throwable The {@link Throwable} to output.
	 */
	//static void e(const std::string &tag, const std::exception &throwable);

	static void setLevel(int logLevel);
	/**
	 * Sets the {@link ILogOutput} to use to print the logs. If not set, {@link System#out}
	 * will be used.
	 * @param logOutput The {@link ILogOutput} to use to print the log.
	 */
	static void setLogOutput(ILogOutput *logOutput);

	/**
	 * Show hex dump.
	 * <p/>
	 * Local addition.  Output looks like:
	 * 1230- 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff  0123456789abcdef
	 * <p/>
	 * Uses no string concatenation; creates one String object per line.
	 */
	static void hexDump(const std::string &tag, const LogLevel &level, const std::string &data, int offset, int length);

	/**
	 * Dump the entire contents of a byte array with DEBUG priority.
	 */
	static void hexDump(const std::string &data);

	/* currently prints to stdout; could write to a log window */
	static void println(const LogLevel &logLevel, const std::string &tag, const std::string &message);

	/**
	 * Prints a log message.
	 * @param logLevel
	 * @param tag
	 * @param message
	 */
	static void printLog(const LogLevel &logLevel, const std::string &tag, const std::string &message);

	/**
	 * Formats a log message.
	 * @param logLevel
	 * @param tag
	 * @param message
	 */
	static std::string getLogFormatString(const LogLevel &logLevel, const std::string &tag, const std::string &message);
	/*
	 * Convert a UTF-16 string to UTF-8.
	 *
	 * Make sure you allocate "utf8Str" with the result of utf16_utf8ByteLen(),
	 * not just "len".
	 */
	static std::string convertUtf16ToUtf8(const std::wstring &utf16Str);

}; /* class Log */

} /* namespace ddmlib */
#endif /* LOG_HPP_ */
