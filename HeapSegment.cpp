/*
 * HeapSegment.cpp
 *
 *  Created on: Feb 8, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "HeapSegment.hpp"

namespace ddmlib {

HeapSegment::HeapSegment(std::tr1::shared_ptr<ByteBuffer> hpsgData) {
	/* Read the HPSG chunk header.
	 * These get*() calls may throw a BufferUnderflowException
	 * if the underlying data isn't big enough.
	 */

	hpsgData->setSwapEndianness(true);
	//hpsgData.order(ByteOrder.BIG_ENDIAN);
	mHeapId = hpsgData->getInt();
	mAllocationUnitSize = (int) hpsgData->get();
	mStartAddress = (long long) hpsgData->getInt() & 0x00000000ffffffffLL;
	mOffset = hpsgData->getInt();
	mAllocationUnitCount = hpsgData->getInt();

	// Hold onto the remainder of the data.
	ByteBuffer *tmp = hpsgData->slice();
	mUsageData = std::tr1::shared_ptr<ByteBuffer>(tmp->clone());
	delete tmp;
	mUsageData->setSwapEndianness(true);
	//mUsageData.order(ByteOrder.BIG_ENDIAN);   // doesn't actually matter

	// Validate the data.
//xxx do it
//xxx make sure the number of elements matches mAllocationUnitCount.
//xxx make sure the last element doesn't have P set
}

HeapSegment::~HeapSegment() {
}

HeapSegmentElement::HeapSegmentElement(const HeapSegment &hs) {
	set(hs);
}

HeapSegmentElement::HeapSegmentElement() :
		mSolidity(SOLIDITY_INVALID), mKind(KIND_INVALID), mLength(-1) {

}

int HeapSegmentElement::getKind() const {
	return mKind;
}

int HeapSegmentElement::getLength() const {
	return mLength;
}

int HeapSegmentElement::getSolidity() const {
	return mSolidity;
}

void HeapSegmentElement::setKind(int kind) {
	mKind = kind;
}

void HeapSegmentElement::setLength(int length) {
	mLength = length;
}

void HeapSegmentElement::setSolidity(int solidity) {
	mSolidity = solidity;
}

void HeapSegmentElement::set(const HeapSegment &hs) {
	/* TODO: Maybe keep track of the virtual address of each element
	 *       so that they can be examined independently.
	 */
	std::tr1::shared_ptr<ByteBuffer> data = hs.mUsageData;
	int eState = (int) data->get() & 0x000000ff;
	int eLen = ((int) data->get() & 0x000000ff) + 1;

	while ((eState & PARTIAL_MASK) != 0) {

		/* If the partial bit was set, the next byte should describe
		 * the same object as the current one.
		 */
		int nextState = (int) data->get() & 0x000000ff;
		if ((nextState & ~PARTIAL_MASK) != (eState & ~PARTIAL_MASK)) {
			throw std::runtime_error("State mismatch");
		}
		eState = nextState;
		eLen += ((int) data->get() & 0x000000ff) + 1;
	}

	setSolidity(eState & 0x7);
	setKind((eState >> 3) & 0x7);
	setLength(eLen * hs.mAllocationUnitSize);
	;
}

bool HeapSegment::canAppend(const HeapSegment &other) {
	return isValid() && other.isValid() && mHeapId == other.mHeapId && mAllocationUnitSize == other.mAllocationUnitSize
			&& getEndAddress() == other.getStartAddress();
}

bool HeapSegment::append(HeapSegment &other) {
	if (canAppend(other)) {
		/* Preserve the position.  The mark is not preserved,
		 * but we don't use it anyway.
		 */
		int pos = mUsageData->getPosition();

		// Guarantee that we have enough room for the new data.
		if (mUsageData->getCapacity() - mUsageData->getLimit() < other.mUsageData->getLimit()) {
			/* Grow more than necessary in case another append()
			 * is about to happen.
			 */
			int newSize = mUsageData->getLimit() + other.mUsageData->getLimit();
			std::tr1::shared_ptr<ByteBuffer> newData(new ByteBuffer(newSize * 2));

			mUsageData->rewind();
			newData->put(mUsageData.get());
			mUsageData.swap(newData);
		}

		// Copy the data from the other segment and restore the position.
		other.mUsageData->rewind();
		mUsageData->put(other.mUsageData.get());
		mUsageData->setPosition(pos);

		// Fix this segment's header to cover the new data.
		mAllocationUnitCount += other.mAllocationUnitCount;

		// Mark the other segment as invalid.
		other.mStartAddress = INVALID_START_ADDRESS;
		other.mUsageData.reset();

		return true;
	} else {
		return false;
	}
}

std::tr1::shared_ptr<HeapSegmentElement> HeapSegment::getNextElement(std::tr1::shared_ptr<HeapSegmentElement> reuse) {
	try {
		if (reuse != nullptr) {
			reuse->set(*this);
			return reuse;
		} else {
			return std::tr1::shared_ptr<HeapSegmentElement>(new HeapSegmentElement(*this));
		}
	} catch (std::underflow_error &ex) {
		/* Normal "end of buffer" situation.
		 */
	} catch (std::exception &ex) {
		/* Malformed data.
		 */
		//TODO: we should catch this in the constructor
	}
	return std::tr1::shared_ptr<HeapSegmentElement>();
}

} /* namespace ddmlib */
