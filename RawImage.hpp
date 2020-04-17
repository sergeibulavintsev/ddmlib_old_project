/*
 * RawImage.hpp
 *
 *  Created on: 24.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef RAWIMAGE_HPP_
#define RAWIMAGE_HPP_
#include "ddmlib.hpp"

class ByteBuffer;

namespace ddmlib {

class DDMLIB_API RawImage {
public:
	int version;
	int bpp;
	int size;
	int width;
	int height;
	int red_offset;
	int red_length;
	int blue_offset;
	int blue_length;
	int green_offset;
	int green_length;
	int alpha_offset;
	int alpha_length;

	std::vector<unsigned char> data;

public:
	/**
	 * Reads the header of a RawImage from a {@link ByteBuffer}.
	 * <p/>The way the data is sent over adb is defined in system/core/adb/framebuffer_service.c
	 * @param version the version of the protocol.
	 * @param buf the buffer to read from.
	 * @return true if success
	 */
	bool readHeader(int version, std::tr1::shared_ptr<ByteBuffer> buf);
	/**
	 * Returns the mask value for the red color.
	 * <p/>This value is compatible with org.eclipse.swt.graphics.PaletteData
	 */
	int getRedMask();
	/**
	 * Returns the mask value for the green color.
	 * <p/>This value is compatible with org.eclipse.swt.graphics.PaletteData
	 */
	int getGreenMask();
	/**
	 * Returns the mask value for the blue color.
	 * <p/>This value is compatible with org.eclipse.swt.graphics.PaletteData
	 */
	int getBlueMask();
	/**
	 * Returns the size of the header for a specific version of the framebuffer adb protocol.
	 * @param version the version of the protocol
	 * @return the number of int that makes up the header.
	 */
	static int getHeaderSize(int version);
	/**
	 * Returns a rotated version of the image
	 * The image is rotated counter-clockwise.
	 */
	std::tr1::shared_ptr<RawImage> getRotated();
	/**
	 * Returns an ARGB integer value for the pixel at <var>index</var> in {@link #data}.
	 */
	unsigned int getARGB(int index);
	RawImage();
	virtual ~RawImage();

private:
	/**
	 * creates a mask value based on a length and offset.
	 * <p/>This value is compatible with org.eclipse.swt.graphics.PaletteData
	 */
	int getMask(int length, int offset);
	/**
	 * Creates a mask value based on a length.
	 * @param length
	 * @return
	 */
	static int getMask(int length);

};

} /* namespace ddmlib */
#endif /* RAWIMAGE_HPP_ */
