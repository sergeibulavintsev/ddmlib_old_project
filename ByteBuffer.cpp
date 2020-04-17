/**
 ByteBuffer
 ByteBuffer.cpp
 Copyright 2011 Ramsey Kant

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "ByteBuffer.hpp"
#include <cstdlib>

/**
 * ByteBuffer constructor
 * Reserves specified size in internal vector
 *
 * @param size Size of space to preallocate internally. Default is 4096 bytes
 */
ByteBuffer::ByteBuffer(size_t size) {
	init(size);
}

/**
 * ByteBuffer constructor
 * Consume an entire byte array of length len in the ByteBuffer
 *
 * @param arr byte array of data (should be of length len)
 * @param size Size of space to allocate
 */
ByteBuffer::ByteBuffer(byte* arr, size_t size) {
	init(size);
	putBytes(arr, size);
}

void ByteBuffer::init(size_t size) {
	pos = 0;
	limit = size;
	capacity = size;
	if (size > 0)
		buf = (byte*) calloc(size, sizeof(byte));
	else
		buf = 0;
	fromArray = false;
	swapEndianness = false;
}

/**
 * ByteBuffer Deconstructor
 *
 */
ByteBuffer::~ByteBuffer() {
	if (!fromArray)
		free(buf);
}

/**
 * Bytes Remaining
 * Returns the number of bytes from the current read position till the end of the buffer
 *
 * @return Number of bytes from rpos to the end (size())
 */
unsigned int ByteBuffer::bytesRemaining() {
	return size() - pos;
}

ByteBuffer *ByteBuffer::wrap(byte *array, size_t size) {
	ByteBuffer *ret = new ByteBuffer(0);
	free(ret->buf);
	ret->pos = 0;
	ret->limit = size;
	ret->capacity = size;
	ret->buf = array;
	ret->fromArray = true;
	return ret;
}

ByteBuffer *ByteBuffer::slice() {
	return wrap(buf + pos, limit - pos);
}

/**
 * Clear
 * Clears out all data from the internal vector (original preallocated size remains), resets the positions to 0
 */
void ByteBuffer::clear() {
	pos = 0;
	limit = capacity;
	memset(buf, 0, capacity);
}

/**
 * Clone
 * Allocate an exact copy of the ByteBuffer on the heap and return a pointer
 *
 * @return A pointer to the newly cloned ByteBuffer. NULL if no more memory available
 */
ByteBuffer* ByteBuffer::clone() {
	ByteBuffer* ret = new ByteBuffer(limit);

	// Copy data
	for (unsigned int i = 0; i < limit; i++) {
		ret->put(i, (byte) get(i));
	}

	// Reset positions
	ret->setPosition(0);
	ret->fromArray = false;

	return ret;
}

void ByteBuffer::compact() {
	if (pos != limit && pos != 0)
		memmove(buf, buf + pos, limit - pos);
	setPosition(limit - pos);
	setLimit(capacity);
}

/**
 * Equals, test for data equivilancy
 * Compare this ByteBuffer to another by looking at each byte in the internal buffers and making sure they are the same
 *
 * @param other A pointer to a ByteBuffer to compare to this one
 * @return True if the internal buffers match. False if otherwise
 */
bool ByteBuffer::equals(ByteBuffer* other) {
	// If sizes aren't equal, they can't be equal
	if (limit != other->limit)
		return false;

	// Compare byte by byte
	unsigned int len = limit;
	for (unsigned int i = 0; i < len; i++) {
		if ((byte) get(i) != (byte) other->get(i))
			return false;
	}

	return true;
}

/**
 * Resize
 * Reallocates memory for the internal buffer of size newSize. Read and write positions will also be reset
 *
 * @param newSize The amount of memory to allocate
 */
void ByteBuffer::resize(unsigned int newSize) {
	pos = 0;
	setLimit(newSize);
	capacity = newSize;
	realloc(buf, newSize);
}

/**
 * Size
 * Returns the size of the internal buffer...not necessarily the length of bytes used as data!
 *
 * @return size of the internal buffer
 */
unsigned int ByteBuffer::size() {
	return limit;
}

// Replacement

/**
 * Replace
 * Replace occurance of a particular byte, key, with the byte rep
 *
 * @param key Byte to find for replacement
 * @param rep Byte to replace the found key with
 * @param start Index to start from. By default, start is 0
 * @param firstOccuranceOnly If true, only replace the first occurance of the key. If false, replace all occurances. False by default
 */
void ByteBuffer::replace(byte key, byte rep, unsigned int start, bool firstOccuranceOnly) {
	unsigned int len = limit;
	for (unsigned int i = start; i < len; i++) {
		byte data = read<byte>(i);
		// Wasn't actually found, bounds of buffer were exceeded
		if ((key != 0) && (data == 0))
			break;

		// Key was found in array, perform replacement
		if (data == key) {
			buf[i] = rep;
			if (firstOccuranceOnly)
				return;
		}
	}
}

// Read Functions

byte ByteBuffer::peek() {
	return read<byte>(pos);
}

byte ByteBuffer::get() {
	return read<byte>();
}

byte ByteBuffer::get(unsigned int index) {
	return read<byte>(index);
}

void ByteBuffer::getBytes(byte* buf, unsigned int len) {
	for (unsigned int i = 0; i < len; i++) {
		buf[i] = read<byte>();
	}
}

char ByteBuffer::getChar() {
	return read<char>();
}

char ByteBuffer::getChar(unsigned int index) {
	return read<char>(index);
}

double ByteBuffer::getDouble() {
	return read<double>();
}

double ByteBuffer::getDouble(unsigned int index) {
	return read<double>(index);
}

float ByteBuffer::getFloat() {
	return read<float>();
}

float ByteBuffer::getFloat(unsigned int index) {
	return read<float>(index);
}

int ByteBuffer::getInt() {
	return read<int>();
}

int ByteBuffer::getInt(unsigned int index) {
	return read<int>(index);
}

long ByteBuffer::getLong() {
	return read<long>();
}

long ByteBuffer::getLong(unsigned int index) {
	return read<long>(index);
}

long long ByteBuffer::getLongLong() {
	return read<long long>();
}

long long ByteBuffer::getLongLong(unsigned int index) {
	return read<long long>(index);
}

short ByteBuffer::getShort() {
	return read<short>();
}

short ByteBuffer::getShort(unsigned int index) {
	return read<short>(index);
}

wchar_t ByteBuffer::getWChar() {
	return read<wchar_t>();
}

wchar_t ByteBuffer::getWChar(unsigned int index) {
	return read<wchar_t>(index);
}

// Write Functions

void ByteBuffer::put(ByteBuffer* src) {
	int len = src->size();
	if (pos + len > limit)
		throw std::overflow_error("Position value can't be higher than buffer's limit!");
	for (int i = 0; i < len; i++)
		append<byte>(src->get(i));
}

void ByteBuffer::put(byte b) {
	append<byte>(b);
}

void ByteBuffer::put(unsigned int index, byte b) {
	insert<byte>(b, index);
}

void ByteBuffer::putBytes(byte* b, unsigned int len) {
	if (pos + len > limit)
		throw std::overflow_error("Position value can't be higher than buffer's limit!");
	// Insert the data one byte at a time into the internal buffer at position i+starting index
	for (unsigned int i = 0; i < len; i++)
		append<byte>(b[i]);
}

void ByteBuffer::putBytes(byte* b, unsigned int len, unsigned int index) {
	if (index + len > limit)
		throw std::out_of_range("Position value can't be higher than buffer's limit!");
	pos = index;

	// Insert the data one byte at a time into the internal buffer at position i+starting index
	for (unsigned int i = 0; i < len; i++)
		append<byte>(b[i]);
}

void ByteBuffer::putChar(char value) {
	append<char>(value);
}

void ByteBuffer::putChar(unsigned int index, char value) {
	insert<char>(value, index);
}

void ByteBuffer::putDouble(double value) {
	append<double>(value);
}

void ByteBuffer::putDouble(unsigned int index, double value) {
	insert<double>(value, index);
}
void ByteBuffer::putFloat(float value) {
	append<float>(value);
}

void ByteBuffer::putFloat(unsigned int index, float value) {
	insert<float>(value, index);
}

void ByteBuffer::putInt(int value) {
	append<int>(value);
}

void ByteBuffer::putInt(unsigned int index, int value) {
	insert<int>(value, index);
}

void ByteBuffer::putLong(long value) {
	append<long>(value);
}

void ByteBuffer::putLong(unsigned int index, long value) {
	insert<long>(value, index);
}

void ByteBuffer::putShort(short value) {
	append<short>(value);
}

void ByteBuffer::putShort(unsigned int index, short value) {
	insert<short>(value, index);
}

void ByteBuffer::putWChar(wchar_t value) {
	append<wchar_t>(value);
}

void ByteBuffer::putWChar(unsigned int index, wchar_t value) {
	insert<wchar_t>(value, index);
}
