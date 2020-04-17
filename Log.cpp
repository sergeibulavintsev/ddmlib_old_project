/*
 * Log.cpp
 *
 *  Created on: 30.01.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "Log.hpp"

namespace ddmlib {

bool Log::Config::LOGD = true;
bool Log::Config::LOGV = true;
std::vector<LogLevel*> LogLevel::vLogLevel;
LogLevel Log::VERBOSE_L(2, "verbose", 'V'), Log::DEBUG_L(3, "debug", 'D'), Log::INFO_L(4, "info", 'I'), Log::WARN_L(5, "warn", 'W'),
		Log::ERROR_L(6, "error", 'E'), Log::ASSERT_L(7, "assert", 'A');
int Log::mLevel = DdmPreferences::getLogLevel();

ILogOutput *Log::sLogOutput(nullptr);

const char Log::mHexDigit[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

void Log::v(const std::string &tag, const std::string &message) {
	println(VERBOSE_L, tag, message);
}

void Log::d(const std::string &tag, const std::string &message) {
	println(DEBUG_L, tag, message);
}

void Log::i(const std::string &tag, const std::string &message) {
	println(INFO_L, tag, message);
}

void Log::w(const std::string &tag, const std::string &message) {
	println(WARN_L, tag, message);
}

void Log::e(const std::string &tag, const std::string &message) {
	println(ERROR_L, tag, message);
}

void Log::logAndDisplay(const LogLevel &logLevel, const std::string &tag, const std::string &message) {
	if (sLogOutput != nullptr) {
		sLogOutput->printAndPromptLog(logLevel, tag, message);
	} else {
		println(logLevel, tag, message);
	}
}

//void Log::e(const std::string &tag, const std::exception &throwable) {
//	println(ERROR_L, tag, std::string(throwable.what()) + '\n');
//}

void Log::setLevel(int logLevel) {
	mLevel = logLevel;
}

void Log::setLogOutput(ILogOutput *logOutput) {
	sLogOutput = logOutput;
}

void Log::hexDump(const std::string &tag, const LogLevel &level, const std::string &data, int offset, int length) {

	int kHexOffset = 6;
	int kAscOffset = 55;
	//char[] line = new char[mSpaceLine.length];
	std::string line;
	int addr, baseAddr, count;
	int i, ch;
	bool needErase = true;

	//Log.w(tag, "HEX DUMP: off=" + offset + ", length=" + length);

	baseAddr = 0;
	while (length != 0) {
		if (length > 16) {
			// full line
			count = 16;
		} else {
			// partial line; re-copy blanks to clear end
			count = length;
			needErase = true;
		}

		if (needErase) {
			//System.arraycopy(mSpaceLine, 0, line, 0, mSpaceLine.length);
			line.assign(data);
			needErase = false;
		}

		// output the address (currently limited to 4 hex digits)
		addr = baseAddr;
		addr &= 0xffff;
		ch = 3;
		while (addr != 0) {
			line[ch] = mHexDigit[addr & 0x0f];
			ch--;
			addr >>= 4;
		}

		// output hex digits and ASCII chars
		ch = kHexOffset;
		for (i = 0; i < count; i++) {
			char val = data[offset + i];

			line[ch++] = mHexDigit[(val >> 4) & 0x0f];
			line[ch++] = mHexDigit[val & 0x0f];
			ch++;

			if (val >= 0x20 && val < 0x7f)
				line[kAscOffset + i] = (char) val;
			else
				line[kAscOffset + i] = '.';
		}

		println(level, tag, line);

		// advance to next chunk of data
		length -= count;
		offset += count;
		baseAddr += count;
	}

}

void Log::hexDump(const std::string &data) {
	hexDump("ddms", DEBUG_L, data, 0, data.length());
}

/* currently prints to stdout; could write to a log window */
void Log::println(const LogLevel &logLevel, const std::string &tag, const std::string &message) {
	if (logLevel.getPriority() >= mLevel) {
		if (sLogOutput != nullptr) {
			sLogOutput->printLog(logLevel, tag, message);
		} else {
			printLog(logLevel, tag, message);
		}
	}
}

void Log::printLog(const LogLevel &logLevel, const std::string &tag, const std::string &message) {
	std::cout << getLogFormatString(logLevel, tag, message) << std::endl;
}

std::string Log::getLogFormatString(const LogLevel &logLevel, const std::string &tag, const std::string &message) {
	return std::string(
			Poco::DateTimeFormatter::format(Poco::LocalDateTime(), "%H:%M:%S") + " " + logLevel.getPriorityLetter() + "/" + tag + ": "
					+ message);

}

std::string Log::convertUtf16ToUtf8(const std::wstring &src) {
	int len = src.length();
	std::string utf8Str(len, ' ');
	std::string::iterator utf8StrIt = utf8Str.begin();
	std::wstring::const_iterator utf16Str = src.begin();
    while (len--) {
        unsigned int uic = *utf16Str++;

        /*
         * The most common case is (uic > 0 && uic <= 0x7f).
         */
        if (uic == 0 || uic > 0x7f) {
            if (uic > 0x07ff) {
                *utf8StrIt++ = (uic >> 12) | 0xe0;
                *utf8StrIt++ = ((uic >> 6) & 0x3f) | 0x80;
                *utf8StrIt++ = (uic & 0x3f) | 0x80;
            } else /*(uic > 0x7f || uic == 0)*/ {
                *utf8StrIt++ = (uic >> 6) | 0xc0;
                *utf8StrIt++ = (uic & 0x3f) | 0x80;
            }
        } else {
            *utf8StrIt++ = uic;
        }
    }
    return utf8Str;
}

} /* namespace ddmlib */
