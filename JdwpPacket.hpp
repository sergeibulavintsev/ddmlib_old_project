/*
 * JdwpPacket.hpp
 *
 *  Created on: Feb 6, 2012
 *      Author: Ilya Polenov
 */

#ifndef JDWPPACKET_HPP_
#define JDWPPACKET_HPP_
#include "ddmlib.hpp"

class ByteBuffer;

namespace ddmlib {

class DDMLIB_LOCAL JdwpPacket {
private:
	// our cmdSet/cmd
	static const int DDMS_CMD_SET; // 'G' + 128
	static const int DDMS_CMD;

	// "flags" field
	static const unsigned int REPLY_PACKET;

	// this is sent and expected at the start of a JDWP connection
	static const std::size_t sHandshakeSize = 14;
	static unsigned char mHandshake[sHandshakeSize];

	std::tr1::shared_ptr<ByteBuffer> mBuffer;
	int mId, mCmdSet, mCmd, mErrCode;
	unsigned int mLength, mFlags;
	bool mIsNew;

	static unsigned int mSerialId;

public:
	// header len
	static const unsigned int JDWP_HEADER_LEN;

	// results from findHandshake
	static const unsigned int HANDSHAKE_GOOD;
	static const unsigned int HANDSHAKE_NOTYET;
	static const unsigned int HANDSHAKE_BAD;

	static const unsigned int HANDSHAKE_LEN;

	~JdwpPacket();

	/**
	 * Create a new, empty packet, in "buf".
	 */
	JdwpPacket(std::tr1::shared_ptr<ByteBuffer> buf);

	/**
	 * Get the next serial number.  This creates a unique serial number
	 * across all connections, not just for the current connection.  This
	 * is a useful property when debugging, but isn't necessary.
	 *
	 * We can't synchronize on an int, so we use a sync method.
	 */
	static int getNextSerial();

	/**
	 * Return a slice of the byte buffer, positioned past the JDWP header
	 * to the start of the chunk header.  The buffer's limit will be set
	 * to the size of the payload if the size is known; if this is a
	 * packet under construction the limit will be set to the end of the
	 * buffer.
	 *
	 * Doesn't examine the packet at all -- works on empty buffers.
	 */
	std::tr1::shared_ptr<ByteBuffer> getPayload();

	/**
	 * Returns "true" if this JDWP packet has a JDWP command type.
	 *
	 * This never returns "true" for reply packets.
	 */
	bool isDdmPacket() const;

	/**
	 * Returns "true" if this JDWP packet is tagged as a reply.
	 */
	bool isReply() const;

	/**
	 * Returns "true" if this JDWP packet is a reply with a nonzero
	 * error code.
	 */
	bool isError() const;

	/**
	 * Returns "true" if this JDWP packet has no data.
	 */
	bool isEmpty() const;

	/**
	 * Return the packet's ID.  For a reply packet, this allows us to
	 * match the reply with the original request.
	 */
	int getId() const;

	/**
	 * Return the length of a packet.  This includes the header, so an
	 * empty packet is 11 bytes long.
	 */
	int getLength() const;

	/**
	 * Write our packet to "chan".  Consumes the packet as part of the
	 * write.
	 *
	 * The JDWP packet starts at offset 0 and ends at mBuffer.position().
	 */
	void writeAndConsume(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan);

	/**
	 * Finish a packet created with newPacket().
	 *
	 * This always creates a command packet, with the next serial number
	 * in sequence.
	 *
	 * We have to take "payloadLength" as an argument because we can't
	 * see the position in the "slice" returned by getPayload().  We could
	 * fish it out of the chunk header, but it's legal for there to be
	 * more than one chunk in a JDWP packet.
	 *
	 * On exit, "position" points to the end of the data.
	 */
	void finishPacket(int payloadLength);

	/**
	 * "Move" the packet data out of the buffer we're sitting on and into
	 * buf at the current position.
	 */
	void movePacket(std::tr1::shared_ptr<ByteBuffer> buf);

	/**
	 * Consume the JDWP packet.
	 *
	 * On entry and exit, "position" is the #of bytes in the buffer.
	 */
	void consume();

	/**
	 * Find the JDWP packet at the start of "buf".  The start is known,
	 * but the length has to be parsed out.
	 *
	 * On entry, the packet data in "buf" must start at offset 0 and end
	 * at "position".  "limit" should be set to the buffer capacity.  This
	 * method does not alter "buf"s attributes.
	 *
	 * Returns a new JdwpPacket if a full one is found in the buffer.  If
	 * not, returns null.  Throws an exception if the data doesn't look like
	 * a valid JDWP packet.
	 */
	static std::tr1::shared_ptr<JdwpPacket> findPacket(std::tr1::shared_ptr<ByteBuffer> buf);

	/**
	 * Like findPacket(), but when we're expecting the JDWP handshake.
	 *
	 * Returns one of:
	 *   HANDSHAKE_GOOD   - found handshake, looks good
	 *   HANDSHAKE_BAD    - found enough data, but it's wrong
	 *   HANDSHAKE_NOTYET - not enough data has been read yet
	 */
	static int findHandshake(std::tr1::shared_ptr<ByteBuffer> buf);

	/**
	 * Remove the handshake string from the buffer.
	 *
	 * On entry and exit, "position" is the #of bytes in the buffer.
	 */
	static void consumeHandshake(std::tr1::shared_ptr<ByteBuffer> buf);

	/**
	 * Copy the handshake string into the output buffer.
	 *
	 * On exit, "buf"s position will be advanced.
	 */
	static void putHandshake(std::tr1::shared_ptr<ByteBuffer> buf);
};

} /* namespace ddmlib */
#endif /* JDWPPACKET_HPP_ */
