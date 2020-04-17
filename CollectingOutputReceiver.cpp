/*
 * CollectingOutputReceiver.cpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "CollectingOutputReceiver.hpp"

namespace ddmlib {

CollectingOutputReceiver::CollectingOutputReceiver() {
	mIsCanceled = false;
}

CollectingOutputReceiver::~CollectingOutputReceiver() {
}

void CollectingOutputReceiver::addOutput(unsigned char* data, unsigned int offset, unsigned int length) {
	if (!isCancelled()) {
		std::string s;
		s.resize(length);
		std::copy(data + offset, data + offset + length, s.begin());
		mOutputBuffer.append(s);
	}
}

} /* namespace ddmlib */
