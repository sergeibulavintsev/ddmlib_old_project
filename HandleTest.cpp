/*
 * HandleTest.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleTest.hpp"
#include "Log.hpp"
#include "JdwpPacket.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

int HandleTest::CHUNK_TEST = HandleTest::type("TEST");

std::tr1::shared_ptr<HandleTest> HandleTest::mInst(new HandleTest);

HandleTest::HandleTest() {
}

HandleTest::~HandleTest() {
}

void HandleTest::registerInReactor() {
	registerChunkHandler(CHUNK_TEST, mInst);
}

void HandleTest::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {

	Log::d("ddm-test", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_TEST) {
		handleTEST(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}
}

void HandleTest::handleTEST(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	Log::d("ddm-test", "Received:");
	Log::hexDump("ddm-test", Log::DEBUG_L, std::string(data->getArray(), data->getArray() + data->getLimit()), 0, data->getLimit());
}

} /* namespace ddmlib */
