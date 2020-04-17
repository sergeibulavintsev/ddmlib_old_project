/*
 * JdwpPacket.cpp
 *
 *  Created on: Feb 6, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "JdwpPacket.hpp"
#include "Log.hpp"
#include "BadPacketException.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {
// header len
const unsigned int JdwpPacket::JDWP_HEADER_LEN = 11;

// results from findHandshake
const unsigned int JdwpPacket::HANDSHAKE_GOOD = 1;
const unsigned int JdwpPacket::HANDSHAKE_NOTYET = 2;
const unsigned int JdwpPacket::HANDSHAKE_BAD = 3;

// our cmdSet/cmd
const int JdwpPacket::DDMS_CMD_SET = 0xc7; // 'G' + 128
const int JdwpPacket::DDMS_CMD = 0x01;

// "flags" field
const unsigned int JdwpPacket::REPLY_PACKET = 0x80;

// this is sent and expected at the start of a JDWP connection
unsigned char JdwpPacket::mHandshake[sHandshakeSize] = { 'J', 'D', 'W', 'P', '-', 'H', 'a', 'n', 'd', 's', 'h', 'a', 'k', 'e' };
const unsigned int JdwpPacket::HANDSHAKE_LEN = JdwpPacket::sHandshakeSize;

unsigned int JdwpPacket::mSerialId = 0x40000000;

JdwpPacket::JdwpPacket(std::tr1::shared_ptr<ByteBuffer> buf) {
	mBuffer = buf;
	mIsNew = true;
	mErrCode = 0;
	mCmd = 0;
	mCmdSet = 0;
	mId = 0;
	mFlags = 0;
	mLength = 0;
}

JdwpPacket::~JdwpPacket() {
}

void JdwpPacket::finishPacket(int payloadLength) {
	bool oldSwap = mBuffer->getSwapEndianness();
	mBuffer->setSwapEndianness(true); // TODO: ChunkHandler.CHUNK_ORDER

	mLength = JDWP_HEADER_LEN + payloadLength;
	mId = getNextSerial();
	mFlags = 0;
	mCmdSet = DDMS_CMD_SET;
	mCmd = DDMS_CMD;

	mBuffer->putInt(0x00, mLength);
	mBuffer->putInt(0x04, mId);
	mBuffer->put(0x08, mFlags);
	mBuffer->put(0x09, mCmdSet);
	mBuffer->put(0x0a, mCmd);

	mBuffer->setSwapEndianness(oldSwap);
	mBuffer->setPosition(mLength);
}

int JdwpPacket::getNextSerial() {
	static Poco::FastMutex sync;
	Poco::ScopedLock<Poco::FastMutex> lock(sync);
	return mSerialId++;
}

std::tr1::shared_ptr<ByteBuffer> JdwpPacket::getPayload() {
	int oldPosn = mBuffer->getPosition();

	mBuffer->setPosition(JDWP_HEADER_LEN);
	std::tr1::shared_ptr<ByteBuffer> buf(mBuffer->slice()); // goes from position to limit
	mBuffer->setPosition(oldPosn);

	if (mLength > 0)
		buf->setLimit(mLength - JDWP_HEADER_LEN);
	buf->setSwapEndianness(true); // TODO:
	return buf;
}

bool JdwpPacket::isDdmPacket() const {
	return ((mFlags & REPLY_PACKET) == 0) &&
			(mCmdSet == DDMS_CMD_SET) &&
			(mCmd == DDMS_CMD);
}

bool JdwpPacket::isReply() const {
	return (mFlags & REPLY_PACKET) != 0;
}

bool JdwpPacket::isError() const {
	return isReply() && mErrCode != 0;
}

bool JdwpPacket::isEmpty() const {
	return (mLength == JDWP_HEADER_LEN);
}

int JdwpPacket::getId() const {
	return mId;
}

int JdwpPacket::getLength() const {
	return mLength;
}

void JdwpPacket::writeAndConsume(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan) {
	int oldLimit;

	//Log.i("ddms", "writeAndConsume: pos=" + mBuffer.position()
	//    + ", limit=" + mBuffer.limit());

	mBuffer->flip(); // limit<-posn, posn<-0
	oldLimit = mBuffer->getLimit();
	mBuffer->setLimit(mLength);
	while (mBuffer->getPosition() != mBuffer->getLimit()) {
		unsigned char buf = mBuffer->get();
		chan->sendBytes(&buf, 1);
	}
	// position should now be at end of packet
	assert(mBuffer->getPosition() == mLength);

	mBuffer->setLimit(oldLimit);
	mBuffer->compact(); // shift posn...limit, posn<-pending data

	//Log.i("ddms", "               : pos=" + mBuffer.position()
	//    + ", limit=" + mBuffer.limit());
}

void JdwpPacket::movePacket(std::tr1::shared_ptr<ByteBuffer> buf) {
	//Log::v("ddms", std::string("moving ") + std::string(mLength) + " bytes");
	int oldPosn = mBuffer->getPosition();

	mBuffer->setPosition(0);
	mBuffer->setLimit(mLength);
	buf->put(mBuffer.get());
	mBuffer->setPosition(mLength);
	mBuffer->setLimit(oldPosn);
	mBuffer->compact(); // shift posn...limit, posn<-pending data
}

void JdwpPacket::consume() {
	/*
	 * The "flip" call sets "limit" equal to the position (usually the
	 * end of data) and "position" equal to zero.
	 *
	 * compact() copies everything from "position" and "limit" to the
	 * start of the buffer, sets "position" to the end of data, and
	 * sets "limit" to the capacity.
	 *
	 * On entry, "position" is set to the amount of data in the buffer
	 * and "limit" is set to the capacity.  We want to call flip()
	 * so that position..limit spans our data, advance "position" past
	 * the current packet, then compact.
	 */
	mBuffer->flip(); // limit<-posn, posn<-0
	mBuffer->setPosition(mLength);
	mBuffer->compact(); // shift posn...limit, posn<-pending data
	mLength = 0;
	//Log.d("ddms", "  after compact, posn=" + mBuffer.position()
	//    + ", limit=" + mBuffer.limit());
}

std::tr1::shared_ptr<JdwpPacket> JdwpPacket::findPacket(std::tr1::shared_ptr<ByteBuffer> buf) {
	unsigned int count = buf->getPosition();
	unsigned int length, id, flags;
	int cmdSet, cmd;

	if (count < JDWP_HEADER_LEN)
		return std::tr1::shared_ptr<JdwpPacket>();

	bool oldSwapEndianness = buf->getSwapEndianness();
	buf->setSwapEndianness(true); // TODO: more proper

	length = buf->getInt(0x00);
	id = buf->getInt(0x04);
	flags = buf->get(0x08) & 0xff;
	cmdSet = buf->get(0x09) & 0xff;
	cmd = buf->get(0x0a) & 0xff;

	buf->setSwapEndianness(oldSwapEndianness);

	if (length < JDWP_HEADER_LEN)
		throw BadPacketException();
	if (count < length)
		return std::tr1::shared_ptr<JdwpPacket>();

	std::tr1::shared_ptr<JdwpPacket> pkt(new JdwpPacket(buf));
	pkt->mBuffer = buf;
	pkt->mLength = length;
	pkt->mId = id;
	pkt->mFlags = flags;

	if ((flags & REPLY_PACKET) == 0) {
		pkt->mCmdSet = cmdSet;
		pkt->mCmd = cmd;
		pkt->mErrCode = -1;
	} else {
		pkt->mCmdSet = -1;
		pkt->mCmd = -1;
		pkt->mErrCode = cmdSet | (cmd << 8);
	}

	return pkt;
}

int JdwpPacket::findHandshake(std::tr1::shared_ptr<ByteBuffer> buf) {
	unsigned int count = buf->getPosition();
	int i;

	if (count < sHandshakeSize)
		return HANDSHAKE_NOTYET;

	for (i = sHandshakeSize - 1; i >= 0; --i) {
		if (buf->get(i) != mHandshake[i])
			return HANDSHAKE_BAD;
	}

	return HANDSHAKE_GOOD;
}

void JdwpPacket::consumeHandshake(std::tr1::shared_ptr<ByteBuffer> buf) {
	// in theory, nothing else can have arrived, so this is overkill
	buf->flip(); // limit<-posn, posn<-0
	buf->setPosition(sHandshakeSize);
	buf->compact(); // shift posn...limit, posn<-pending data
}

void JdwpPacket::putHandshake(std::tr1::shared_ptr<ByteBuffer> buf) {
	buf->putBytes(mHandshake, sHandshakeSize);
}

} /* namespace ddmlib */
