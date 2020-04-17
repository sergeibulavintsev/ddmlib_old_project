/*
 * CollectingOutputReceiver.hpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Sergey Bulavintsev
 */

#ifndef COLLECTINGOUTPUTRECEIVER_HPP_
#define COLLECTINGOUTPUTRECEIVER_HPP_
#include "ddmlib.hpp"
#include "IShellOutputReceiver.hpp"

namespace ddmlib {

class DDMLIB_API CollectingOutputReceiver: public IShellOutputReceiver {
public:
	CollectingOutputReceiver();
	bool isCancelled() {
		return mIsCanceled;
	}

	void cancel() {
		mIsCanceled = true;
	}

	std::string getOutput() const {
		return mOutputBuffer;
	}

	void addOutput(unsigned char* data, unsigned int offset, unsigned int length);
	void flush() {/* ignore */
	}

	virtual ~CollectingOutputReceiver();
private:
	std::string mOutputBuffer;
	bool mIsCanceled;
};

} /* namespace ddmlib */
#endif /* COLLECTINGOUTPUTRECEIVER_HPP_ */
