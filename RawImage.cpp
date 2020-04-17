/*
 * RawImage.cpp
 *
 *  Created on: 24.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "RawImage.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

RawImage::RawImage() {
	version = 0;
	bpp = 0;
	size = 0;
	width = 0;
	height = 0;
	red_offset = 0;
	red_length = 0;
	green_offset = 0;
	green_length = 0;
	blue_offset = 0;
	blue_length = 0;
	alpha_offset = 0;
	alpha_length = 0;
}

RawImage::~RawImage() {
}

bool RawImage::readHeader(int hVersion, std::tr1::shared_ptr<ByteBuffer> buf) {
	version = hVersion;

	if (version == 16) {
		// compatibility mode with original protocol
		bpp = 16;

		// read actual values.
		size = buf->getInt();
		width = buf->getInt();
		height = buf->getInt();

		// create default values for the rest. Format is 565
		red_offset = 11;
		red_length = 5;
		green_offset = 5;
		green_length = 6;
		blue_offset = 0;
		blue_length = 5;
		alpha_offset = 0;
		alpha_length = 0;
	} else if (version == 1) {
		bpp = buf->getInt();
		size = buf->getInt();
		width = buf->getInt();
		height = buf->getInt();
		red_offset = buf->getInt();
		red_length = buf->getInt();
		blue_offset = buf->getInt();
		blue_length = buf->getInt();
		green_offset = buf->getInt();
		green_length = buf->getInt();
		alpha_offset = buf->getInt();
		alpha_length = buf->getInt();
	} else {
		// unsupported protocol!
		return false;
	}

	return true;
}

int RawImage::getRedMask() {
	return getMask(red_length, red_offset);
}

int RawImage::getGreenMask() {
	return getMask(green_length, green_offset);
}

int RawImage::getBlueMask() {
	return getMask(blue_length, blue_offset);
}

int RawImage::getHeaderSize(int version) {
	switch (version) {
	case 16: // compatibility mode
		return 3; // size, width, height
	case 1:
		return 12; // bpp, size, width, height, 4*(length, offset)
	}

	return 0;
}

std::tr1::shared_ptr<RawImage> RawImage::getRotated() {

	std::tr1::shared_ptr<RawImage> rotated(new RawImage());
	rotated->version = version;
	rotated->bpp = bpp;
	rotated->size = size;
	rotated->red_offset = red_offset;
	rotated->red_length = red_length;
	rotated->blue_offset = blue_offset;
	rotated->blue_length = blue_length;
	rotated->green_offset = green_offset;
	rotated->green_length = green_length;
	rotated->alpha_offset = alpha_offset;
	rotated->alpha_length = alpha_length;

	rotated->width = height;
	rotated->height = width;

	int count = data.size();
	rotated->data.resize(count);

	int byteCount = bpp >> 3; // bpp is in bits, we want bytes to match our array
	int w = width;
	int h = height;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			std::copy(this->data.begin() + (y * w + x) * byteCount, this->data.begin() + (y * w + x) * byteCount + byteCount,
					rotated->data.begin() + ((w - x - 1) * h + y) * byteCount);
		}
	}

	return rotated;
}

unsigned int RawImage::getARGB(int index) {
	unsigned int value;
	if (bpp == 16) {
		value = data[index] & 0x00FF;
		value |= (data[index + 1] << 8) & 0x0FF00;
	} else if (bpp == 32) {
		value = data[index] & 0x00FF;
		value |= (data[index + 1] & 0x00FF) << 8;
		value |= (data[index + 2] & 0x00FF) << 16;
		value |= (data[index + 3] & 0x00FF) << 24;
	} else {
		throw std::runtime_error("RawImage.getARGB(int) only works in 16 and 32 bit mode.");
	}

	unsigned int r = ((value >> red_offset) & getMask(red_length)) << (8 - red_length);
	unsigned int g = ((value >> green_offset) & getMask(green_length)) << (8 - green_length);
	unsigned int b = ((value >> blue_offset) & getMask(blue_length)) << (8 - blue_length);
	unsigned int a;
	if (alpha_length == 0) {
		a = 0xFF; // force alpha to opaque if there's no alpha value in the framebuffer.
	} else {
		a = ((value >> alpha_offset) & getMask(alpha_length)) << (8 - alpha_length);
	}

	return a << 24 | r << 16 | g << 8 | b;
}

int RawImage::getMask(int length, int offset) {
	int res = getMask(length) << offset;

	// if the bpp is 32 bits then we need to invert it because the buffer is in little endian
	if (bpp == 32) {

		union {
			int i;
			unsigned char c[4];
		} splitInt, reverseInt;
		splitInt.i = res;

		reverseInt.c[0] = splitInt.c[3];
		reverseInt.c[1] = splitInt.c[2];
		reverseInt.c[2] = splitInt.c[1];
		reverseInt.c[3] = splitInt.c[0];

		return reverseInt.i;
	}

	return res;
}

int RawImage::getMask(int length) {
	return (1 << length) - 1;
}

} /* namespace ddmlib */
