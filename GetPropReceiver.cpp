/*
 * GetPropReceiver.cpp
 *
 *  Created on: 09.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "GetPropReceiver.hpp"

namespace ddmlib {

std::string GetPropReceiver::GETPROP_COMMAND("getprop");
Poco::RegularExpression GetPropReceiver::GETPROP_PATTERN(std::string("^\\[([^]]+)\\]\\:\\s*\\[(.*)\\]$"),
		Poco::RegularExpression::RE_NEWLINE_ANYCRLF | Poco::RegularExpression::RE_MULTILINE);

void GetPropReceiver::processNewLines(const std::vector<std::string>& lines) {
	// We receive an array of lines. We're expecting
	// to have the build info in the first line, and the build
	// date in the 2nd line. There seems to be an empty line
	// after all that.

	for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
		if (line->length() == 0 || line->find('#') == 0) {
			continue;
		}

		std::vector<std::string> temp;
		if (GETPROP_PATTERN.split(*line, temp)) {
			std::string label = temp.at(1);
			std::string value = temp.at(2);

			if (label.length() > 0) {
				mDevice->addProperty(label, value);
			}
		}
	}
}

} /* namespace ddmlib */
