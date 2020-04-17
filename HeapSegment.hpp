/*
 * HeapSegmentElement.hpp
 *
 *  Created on: Feb 8, 2012
 *      Author: Ilya Polenov
 */

#ifndef HEAPSEGMENTELEMENT_HPP_
#define HEAPSEGMENTELEMENT_HPP_
#include "ddmlib.hpp"

#include "ByteBuffer.hpp"

namespace ddmlib {

class HeapSegment;

/**
 * Describes an object/region encoded in the HPSG data.
 */
class DDMLIB_API HeapSegmentElement {
	/**
	 * A bit in the HPSG data that indicates that an element should
	 * be combined with the element that follows, typically because
	 * an element is too large to be described by a single element.
	 */
	static const int PARTIAL_MASK = 1 << 7;

	/**
	 * Describes the reachability/solidity of the element.  Must
	 * be set to one of the SOLIDITY_* values.
	 */
	int mSolidity;

	/**
	 * Describes the type/kind of the element.  Must be set to one
	 * of the KIND_* values.
	 */
	int mKind;

	/**
	 * Describes the length of the element, in bytes.
	 */
	int mLength;

public:

	/*
	 * Solidity values, which must match the values in
	 * the HPSG data.
	 */

	/** The element describes a free block. */
	static const int SOLIDITY_FREE = 0;

	/** The element is strongly-reachable. */
	static const int SOLIDITY_HARD = 1;

	/** The element is softly-reachable. */
	static const int SOLIDITY_SOFT = 2;

	/** The element is weakly-reachable. */
	static const int SOLIDITY_WEAK = 3;

	/** The element is phantom-reachable. */
	static const int SOLIDITY_PHANTOM = 4;

	/** The element is pending finalization. */
	static const int SOLIDITY_FINALIZABLE = 5;

	/** The element is not reachable, and is about to be swept/freed. */
	static const int SOLIDITY_SWEEP = 6;

	/** The reachability of the object is unknown. */
	static const int SOLIDITY_INVALID = -1;

	/*
	 * Kind values, which must match the values in
	 * the HPSG data.
	 */

	/** The element describes a data object. */
	static const int KIND_OBJECT = 0;

	/** The element describes a class object. */
	static const int KIND_CLASS_OBJECT = 1;

	/** The element describes an array of 1-byte elements. */
	static const int KIND_ARRAY_1 = 2;

	/** The element describes an array of 2-byte elements. */
	static const int KIND_ARRAY_2 = 3;

	/** The element describes an array of 4-byte elements. */
	static const int KIND_ARRAY_4 = 4;

	/** The element describes an array of 8-byte elements. */
	static const int KIND_ARRAY_8 = 5;

	/** The element describes an unknown type of object. */
	static const int KIND_UNKNOWN = 6;

	/** The element describes a native object. */
	static const int KIND_NATIVE = 7;

	/** The object kind is unknown or unspecified. */
	static const int KIND_INVALID = -1;
	int getKind() const;
	int getLength() const;
	int getSolidity() const;
	void setKind(int kind);
	void setLength(int length);
	void setSolidity(int solidity);

	/**
	 * Create an element describing the entry at the current
	 * position of hpsgData.
	 *
	 * @param hs The heap segment to pull the entry from.
	 * @throws BufferUnderflowException if there is not a whole entry
	 *                                  following the current position
	 *                                  of hpsgData.
	 * @throws ParseException           if the provided data is malformed.
	 */
	HeapSegmentElement(const HeapSegment &hs);
	HeapSegmentElement();

	/**
	 * Replace the element with the entry at the current position of
	 * hpsgData.
	 *
	 * @param hs The heap segment to pull the entry from.
	 * @return this object.
	 * @throws BufferUnderflowException if there is not a whole entry
	 *                                  following the current position of
	 *                                  hpsgData.
	 * @throws ParseException           if the provided data is malformed.
	 */
	void set(const HeapSegment &hs);

	virtual ~HeapSegmentElement() {
	}
};

/**
 * Describes the types and locations of objects in a segment of a heap.
 */
class DDMLIB_API HeapSegment {
	friend class HeapSegmentElement;
protected:
	//* The ID of the heap that this segment belongs to.
	int mHeapId;

	//* The size of an allocation unit, in bytes. (e.g., 8 bytes)
	int mAllocationUnitSize;

	//* The virtual address of the start of this segment.
	long mStartAddress;

	//* The offset of this pices from mStartAddress, in bytes.
	int mOffset;

	//* The number of allocation units described in this segment.
	int mAllocationUnitCount;

	//* The raw data that describes the contents of this segment.
	std::tr1::shared_ptr<ByteBuffer> mUsageData;

	//* mStartAddress is set to this value when the segment becomes invalid.
	static const long INVALID_START_ADDRESS = -1L;
public:
	/**
	 * Create a new HeapSegment based on the raw contents
	 * of an HPSG chunk.
	 *
	 * @param hpsgData The raw data from an HPSG chunk.
	 * @throws BufferUnderflowException if hpsgData is too small
	 *                                  to hold the HPSG chunk header data.
	 */
	HeapSegment(std::tr1::shared_ptr<ByteBuffer> hpsgData);

	/**
	 * See if this segment still contains data, and has not been
	 * appended to another segment.
	 *
	 * @return true if this segment has not been appended to
	 *         another segment.
	 */
	bool isValid() const {
		return mStartAddress != INVALID_START_ADDRESS;
	}

	/**
	 * See if <code>other</code> comes immediately after this segment.
	 *
	 * @param other The HeapSegment to check.
	 * @return true if <code>other</code> comes immediately after this
	 *         segment.
	 */
	bool canAppend(const HeapSegment &other);

	long getStartAddress() const {
		return mStartAddress + mOffset;
	}

	int getLength() const {
		return mAllocationUnitSize * mAllocationUnitCount;
	}

	long getEndAddress() const {
		return getStartAddress() + getLength();
	}

	void rewindElements() {
		if (mUsageData != nullptr) {
			mUsageData->rewind();
		}
	}

	/**
	 * Append the contents of <code>other</code> to this segment
	 * if it describes the segment immediately after this one.
	 *
	 * @param other The segment to append to this segment, if possible.
	 *              If appended, <code>other</code> will be invalid
	 *              when this method returns.
	 * @return true if <code>other</code> was successfully appended to
	 *         this segment.
	 */
	bool append(HeapSegment &other);

	std::tr1::shared_ptr<HeapSegmentElement> getNextElement(std::tr1::shared_ptr<HeapSegmentElement> reuse);

	virtual ~HeapSegment();
};

} /* namespace ddmlib */
#endif /* HEAPSEGMENTELEMENT_HPP_ */
