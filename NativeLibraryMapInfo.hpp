/*
 * NativeLibraryMapInfo.hpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Ilya Polenov
 */

#ifndef NATIVELIBRARYMAPINFO_HPP_
#define NATIVELIBRARYMAPINFO_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

/**
 * Memory address to library mapping for native libraries.
 * <p/>
 * Each instance represents a single native library and its start and end memory addresses.
 */
class DDMLIB_API NativeLibraryMapInfo {
	unsigned long long mStartAddr;
	unsigned long long mEndAddr;

	std::string mLibrary;
public:
	NativeLibraryMapInfo(unsigned long long startAddr, unsigned long long endAddr, const std::string &library) :
			mStartAddr(startAddr), mEndAddr(endAddr), mLibrary(library) {
	}

	virtual ~NativeLibraryMapInfo();
	unsigned long long getEndAddress() const;
	std::string getLibraryName() const;
	unsigned long long getStartAddress() const;

	/**
	 * Returns whether the specified address is inside the library.
	 * @param address The address to test.
	 * @return <code>true</code> if the address is between the start and end address of the library.
	 * @see #getStartAddress()
	 * @see #getEndAddress()
	 */
	bool isWithinLibrary(unsigned long long address) const;
};

} /* namespace ddmlib */
#endif /* NATIVELIBRARYMAPINFO_HPP_ */
