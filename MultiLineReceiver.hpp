/*
 * MultiLineReceiver.hpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Sergey Bulavintsev
 */

#ifndef MULTILINERECEIVER_HPP_
#define MULTILINERECEIVER_HPP_
#include "ddmlib.hpp"
#include "IShellOutputReceiver.hpp"

namespace ddmlib {

class DDMLIB_API MultiLineReceiver: public IShellOutputReceiver {
public:
	MultiLineReceiver();
	/**
	 * Set the trim lines flag.
	 * @param trim hether the lines are trimmed, or not.
	 */
	void setTrimLine(bool trim) {
		mTrimLines = trim;
	}
	virtual void addOutput(unsigned char* data, unsigned int offset, unsigned int length);
	virtual void flush();
	/**
	 * Terminates the process. This is called after the last lines have been through
	 * {@link #processNewLines(String[])}.
	 */
	virtual void done() {
		// do nothing.
	}
	virtual void processNewLines(const std::vector<std::string>& lines) = 0;
	virtual ~MultiLineReceiver();
private:
	bool mTrimLines;
	/** unfinished message line, stored for next packet */
	std::string mUnfinishedLine;
	std::vector<std::string> mArray;
};

} /* namespace ddmlib */
#endif /* MULTILINERECEIVER_HPP_ */
