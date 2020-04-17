/*
 * ArrayHelper.h
 *
 *  Created on: 03.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef ARRAYHELPER_HPP_
#define ARRAYHELPER_HPP_

#include "ddmlib.hpp"

namespace ddmlib {

namespace ArrayHelper {

/**
 * Swaps an unsigned value around, and puts the result in an array that can be sent to a device.
 * @param value The value to swap.
 * @param dest the destination array
 * @param offset the offset in the array where to put the swapped value.
 *      Array length must be at least offset + 4
 */
DDMLIB_LOCAL void swap32bitsToArray(unsigned int value, unsigned char* dest, int offset);
DDMLIB_LOCAL void swap32bitsToArray(unsigned int value, std::vector<unsigned char>& dest, int offset);

/**
 * Reads a signed 32 bit integer from an array coming from a device.
 * @param value the array containing the int
 * @param offset the offset in the array at which the int starts
 * @return the integer read from the array
 */
DDMLIB_LOCAL int swap32bitFromArray(const unsigned char* value, int offset);
DDMLIB_LOCAL int swap32bitFromArray(const std::vector<unsigned char>& value, int offset);
/**
 * Reads an unsigned 16 bit integer from an array coming from a device,
 * and returns it as an 'int'
 * @param value the array containing the 16 bit int (2 byte).
 * @param offset the offset in the array at which the int starts
 *      Array length must be at least offset + 2
 * @return the integer read from the array.
 */
DDMLIB_LOCAL int swapU16bitFromArray(const unsigned char* value, int offset);

/**
 * Reads a signed 64 bit integer from an array coming from a device.
 * @param value the array containing the int
 * @param offset the offset in the array at which the int starts
 *      Array length must be at least offset + 8
 * @return the integer read from the array
 */
DDMLIB_LOCAL long long swap64bitFromArray(const unsigned char* value, int offset);
}

} /* namespace ddmlib */
#endif /* ARRAYHELPER_H_ */
