/*
 * ArrayHelper.cpp
 *
 *  Created on: 03.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "ArrayHelper.hpp"

namespace ddmlib {

namespace DDMLIB_LOCAL ArrayHelper {
/**
     * Swaps an unsigned value around, and puts the result in an array that can be sent to a device.
     * @param value The value to swap.
     * @param dest the destination array
     * @param offset the offset in the array where to put the swapped value.
     *      Array length must be at least offset + 4
     */
    void swap32bitsToArray(unsigned int value, unsigned char* dest, int offset) {
        dest[offset] = (unsigned char)(value & 0x000000FF);
        dest[offset + 1] = (unsigned char)((value & 0x0000FF00) >> 8);
        dest[offset + 2] = (unsigned char)((value & 0x00FF0000) >> 16);
        dest[offset + 3] = (unsigned char)((value & 0xFF000000) >> 24);
    }

    void swap32bitsToArray(unsigned int value, std::vector< unsigned char >& dest, int offset) {
        dest[offset] = (unsigned char)(value & 0x000000FF);
        dest[offset + 1] = (unsigned char)((value & 0x0000FF00) >> 8);
        dest[offset + 2] = (unsigned char)((value & 0x00FF0000) >> 16);
        dest[offset + 3] = (unsigned char)((value & 0xFF000000) >> 24);
    }

    /**
     * Reads a signed 32 bit integer from an array coming from a device.
     * @param value the array containing the int
     * @param offset the offset in the array at which the int starts
     * @return the integer read from the array
     */
    int swap32bitFromArray(const unsigned char* value, int offset) {
        int v = 0;
        v |= ((int)value[offset]) & 0x000000FF;
        v |= (((int)value[offset + 1]) & 0x000000FF) << 8;
        v |= (((int)value[offset + 2]) & 0x000000FF) << 16;
        v |= (((int)value[offset + 3]) & 0x000000FF) << 24;

        return v;
    }

    int swap32bitFromArray(const std::vector<unsigned char>& value, int offset) {
        int v = 0;
        v |= ((int)value[offset]) & 0x000000FF;
        v |= (((int)value[offset + 1]) & 0x000000FF) << 8;
        v |= (((int)value[offset + 2]) & 0x000000FF) << 16;
        v |= (((int)value[offset + 3]) & 0x000000FF) << 24;

        return v;
    }
    /**
     * Reads an unsigned 16 bit integer from an array coming from a device,
     * and returns it as an 'int'
     * @param value the array containing the 16 bit int (2 byte).
     * @param offset the offset in the array at which the int starts
     *      Array length must be at least offset + 2
     * @return the integer read from the array.
     */
    int swapU16bitFromArray(const unsigned char* value, int offset) {
        int v = 0;
        v |= ((int)value[offset]) & 0x000000FF;
        v |= (((int)value[offset + 1]) & 0x000000FF) << 8;

        return v;
    }

    /**
     * Reads a signed 64 bit integer from an array coming from a device.
     * @param value the array containing the int
     * @param offset the offset in the array at which the int starts
     *      Array length must be at least offset + 8
     * @return the integer read from the array
     */
 long long swap64bitFromArray(const unsigned char* value, int offset) {
        long long v = 0;
        v |= ((long long)value[offset]) & 0x00000000000000FFL;
        v |= (((long long)value[offset + 1]) & 0x00000000000000FFL) << 8;
        v |= (((long long)value[offset + 2]) & 0x00000000000000FFL) << 16;
        v |= (((long long)value[offset + 3]) & 0x00000000000000FFL) << 24;
        v |= (((long long)value[offset + 4]) & 0x00000000000000FFL) << 32;
        v |= (((long long)value[offset + 5]) & 0x00000000000000FFL) << 40;
        v |= (((long long)value[offset + 6]) & 0x00000000000000FFL) << 48;
        v |= (((long long)value[offset + 7]) & 0x00000000000000FFL) << 56;

        return v;
    }
}

} /* namespace ddmlib */
