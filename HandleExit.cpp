/*
 * HandleExit.hpp
 *
 *  Created on: Feb 10, 2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleExit.hpp"
#include "Client.hpp"
#include "JdwpPacket.hpp"
#include "ByteBuffer.hpp"
#include "Log.hpp"

namespace ddmlib {

int HandleExit::CHUNK_EXIT = ChunkHandler::type("EXIT");
std::tr1::shared_ptr<HandleExit> HandleExit::mInst(new HandleExit);
HandleExit::HandleExit() {
}

HandleExit::~HandleExit() {
}

void HandleExit::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {
	handleUnknownChunk(client, type, data, isReply, msgId);
}

void HandleExit::sendEXIT(std::tr1::shared_ptr<Client> client, int status) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(4);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);
	buf->setSwapEndianness(true);
	buf->putInt(status);

	finishChunkPacket(packet, CHUNK_EXIT, buf->getPosition());
	Log::d("ddm-exit", "Sending " + name(CHUNK_EXIT) + ": " + Poco::NumberFormatter::format(status));
	client->sendAndConsume(packet, mInst);
}

} /* namespace ddmlib */
