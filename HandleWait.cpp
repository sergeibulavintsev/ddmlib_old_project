/*
 * HandleWait.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleWait.hpp"
#include "Log.hpp"
#include "ClientData.hpp"
#include "Client.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

int HandleWait::CHUNK_WAIT = ChunkHandler::type("WAIT");
std::tr1::shared_ptr<HandleWait> HandleWait::mInst(new HandleWait);

Poco::FastMutex HandleWait::sLock;

HandleWait::HandleWait() {
}

HandleWait::~HandleWait() {
}

void HandleWait::registerInReactor() {
	registerChunkHandler(CHUNK_WAIT, mInst);
}

void HandleWait::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {

	Log::d("ddm-wait", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_WAIT) {
		assert(!isReply);
		handleWAIT(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}
}

void HandleWait::handleWAIT(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	byte reason;

	reason = data->get();

	Log::d("ddm-wait", "WAIT: reason=" + Poco::NumberFormatter::format(reason));

	std::tr1::shared_ptr<ClientData> cd = client->getClientData();
	Poco::ScopedLock<Poco::FastMutex> lock(sLock);

	cd->setDebuggerInterest(DebuggerStatusWAITING);

	client->update(Client::CHANGE_DEBUGGER_STATUS);
}

} /* namespace ddmlib */
