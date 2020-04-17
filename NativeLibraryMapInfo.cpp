/*
 * NativeLibraryMapInfo.cpp
 *
 *  Created on: Feb 9, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "NativeLibraryMapInfo.hpp"

namespace ddmlib {
unsigned long long NativeLibraryMapInfo::getEndAddress() const {
	return mEndAddr;
}

std::string NativeLibraryMapInfo::getLibraryName() const {
	return mLibrary;
}

unsigned long long NativeLibraryMapInfo::getStartAddress() const {
	return mStartAddr;
}

bool NativeLibraryMapInfo::isWithinLibrary(unsigned long long address) const {
	return address >= mStartAddr && address <= mEndAddr;
}

NativeLibraryMapInfo::~NativeLibraryMapInfo() {
}

} /* namespace ddmlib */
