/*
 * MultiLineReceiver.cpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "MultiLineReceiver.hpp"

namespace ddmlib {

MultiLineReceiver::MultiLineReceiver() {
	mTrimLines = true;
}

MultiLineReceiver::~MultiLineReceiver() {
}

void MultiLineReceiver::addOutput(unsigned char* data, unsigned int offset, unsigned int length) {
	if (isCancelled() == false) {
		std::string s;
		for (std::size_t ix = offset; ix < length + offset; ++ix)
			s.push_back(data[ix]);

		// ok we've got a string
		if (s.length() != 0) {
			// if we had an unfinished line we add it.
			if (!mUnfinishedLine.empty()) {
				s = mUnfinishedLine + s;
				mUnfinishedLine.clear();
			}

			// now we split the lines
			mArray.clear();
			int start = 0;
			do {
				size_t index = s.find("\r\n", start);
				size_t offset = 2;

				// if \r\n was not found, this is an unfinished line
				// and we store it to be processed for the next packet
				if (index == std::string::npos) {
					index = s.find("\n", start);
					if (index == std::string::npos) {
						mUnfinishedLine = s.substr(start);
						break;
					} else
						offset = 1;
				}

				// so we found a \r\n;
				// extract the line
				std::string line = s.substr(start, index - start);
				if (mTrimLines) {
					line.erase(0, line.find_first_not_of(" \t\f\v\n\r"));
					line.erase(line.find_last_not_of(" \t\f\v\n\r") + 1);
				}
				mArray.push_back(line);

				// move start to after the \r\n we found
				start = index + offset;
			} while (true);

			if (mArray.size() > 0) {
				// at this point we've split all the lines.
				// make the array
				std::vector<std::string> lines(mArray);

				// send it for final processing
				processNewLines(lines);
			}
		}
	}
}

void MultiLineReceiver::flush() {
	if (!mUnfinishedLine.empty()) {
		std::vector<std::string> unfinished_line(1, mUnfinishedLine);
		processNewLines(unfinished_line);
	}

	done();
}

} /* namespace ddmlib */
