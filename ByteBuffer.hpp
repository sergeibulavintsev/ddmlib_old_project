/**
 ByteBuffer
 ByteBuffer.h
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

/*
 * 07.02.2012 Ilya: implemented limit, added flip, compact.
 */

#ifndef _BYTEBUFFER_HPP_
#define _BYTEBUFFER_HPP_

#include <cstdlib>
#include <cstring>
#include <vector>
#include <stdexcept>

typedef unsigned char byte;

class ByteBuffer {
private:
	unsigned int pos, limit, capacity;
	byte *buf;
	bool fromArray, swapEndianness;

	template<typename T> T read() {
		T data = read<T>(pos);
		size_t s = sizeof(T);
		pos += s;
		return data;
	}

	template<typename T> T read(unsigned int index) const {
		size_t s = sizeof(T);
		if (index + s > limit)
			throw std::out_of_range("Position value can't be higher than buffer's limit!");

		T data = *((T*) (buf + index));
		if (swapEndianness)
			swap((byte*) &data, s);
		return data;
	}

	template<typename T> void append(T data) {
		size_t s = sizeof(data);

		if ((pos + s) > limit)
			throw std::overflow_error("Position value can't be higher than buffer's limit!");

		if (size() < (pos + s))
			realloc(buf, s);

		if (swapEndianness)
			swap((byte*) &data, s);
		memcpy(buf + pos, (byte*) &data, s);

		pos += s;
	}

	template<typename T> void insert(T data, unsigned int index) {
		size_t s = sizeof(data);
		if ((index + s) > limit)
			throw std::out_of_range("Position value can't be higher than buffer's limit!");

		if (swapEndianness)
			swap((byte*) &data, s);
		memcpy(buf + index, (byte*) &data, s);
		pos = index + s;
	}

	void swap(byte *data, size_t N) const {
		byte cache;
		for (size_t n = 0; n < (N / 2); ++n) {
			cache = data[n];
			data[n] = data[N - n - 1];
			data[N - n - 1] = cache;
		}
	}

public:
	ByteBuffer(size_t size = 4096);
	ByteBuffer(byte* arr, size_t size);
	~ByteBuffer();
	void init(size_t size);

	byte *getArray() {
		return buf;
	}
	;
	static ByteBuffer *wrap(byte *array, size_t size);

	unsigned int bytesRemaining(); // Number of bytes from the current read position till the end of the buffer
	void clear(); // Clear our the vector and reset read and write positions
	ByteBuffer* clone(); // Return a new instance of a bytebuffer with the exact same contents and the same state (rpos, wpos)
	void compact();
	ByteBuffer* slice();

	bool equals(ByteBuffer* other); // Compare if the contents are equivalent
	void resize(unsigned int newSize);
	unsigned int size(); // Size of internal vector

	// Basic Searching (Linear)
	template<typename T> int find(T key, unsigned int start = 0) {
		int ret = -1;
		unsigned int len = limit;
		for (unsigned int i = start; i < len; i++) {
			T data = read<T>(i);
			// Wasn't actually found, bounds of buffer were exceeded
			if ((key != 0) && (data == 0))
				break;

			// Key was found in array
			if (data == key) {
				ret = i;
				break;
			}
		}
		return ret;
	}

	// Replacement
	void replace(byte key, byte rep, unsigned int start = 0, bool firstOccuranceOnly = false);

	// Read

	byte peek(); // Relative peek. Reads and returns the next byte in the buffer from the current position but does not increment the read position
	byte get(); // Relative get method. Reads the byte at the buffers current position then increments the position
	byte get(unsigned int index); // Absolute get method. Read byte at index
	void getBytes(byte* buf, unsigned int len); // Absolute read into array buf of length len
	char getChar(); // Relative
	char getChar(unsigned int index); // Absolute
	double getDouble();
	double getDouble(unsigned int index);
	float getFloat();
	float getFloat(unsigned int index);
	int getInt();
	int getInt(unsigned int index);
	long getLong();
	long getLong(unsigned int index);
	long long getLongLong();
	long long getLongLong(unsigned int index);
	short getShort();
	short getShort(unsigned int index);
	wchar_t getWChar();
	wchar_t getWChar(unsigned int index);

	// Write

	void put(ByteBuffer* src); // Relative write of the entire contents of another ByteBuffer (src)
	void put(byte b); // Relative write
	void put(unsigned int index, byte b); // Absolute write at index
	void putBytes(byte* b, unsigned int len); // Relative write
	void putBytes(byte* b, unsigned int len, unsigned int index); // Absolute write starting at index
	void putChar(char value); // Relative
	void putChar(unsigned int index, char value); // Absolute
	void putDouble(double value);
	void putDouble(unsigned int index, double value);
	void putFloat(float value);
	void putFloat(unsigned int index, float value);
	void putInt(int value);
	void putInt(unsigned int index, int value);
	void putLong(long value);
	void putLong(unsigned int index, long value);
	void putShort(short value);
	void putShort(unsigned int index, short value);
	void putWChar(wchar_t value);
	void putWChar(unsigned int index, wchar_t value);

	// Buffer Position Accessors & Mutators

	void rewind() {
		setPosition(0);
	}

	void flip() {
		setLimit(pos);
		setPosition(0);
	}

	unsigned int getLimit() const {
		return limit;
	}

	void setLimit(unsigned int l) {
		if (l > capacity) {
			realloc(buf, l);
			capacity = l;
		}
		limit = l;
	}

	unsigned int getPosition() const {
		return pos;
	}

	void setPosition(unsigned int p) {
		if (p > limit)
			throw std::invalid_argument("Position value can't be higher than buffer's limit!");
		pos = p;
	}

	bool getSwapEndianness() const {
		return swapEndianness;
	}

	void setSwapEndianness(bool val) {
		swapEndianness = val;
	}

	unsigned int getCapacity() const {
		return capacity;
	}
};

#endif
