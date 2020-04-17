/*
 * HandleWait.h
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#ifndef HANDLEWAIT_HPP_
#define HANDLEWAIT_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

/**
 * Handle the "wait" chunk (WAIT).  These are sent up when the client is
 * waiting for something, e.g. for a debugger to attach.
 */
class DDMLIB_LOCAL HandleWait: public ChunkHandler {
public:
	static int CHUNK_WAIT;

	HandleWait();

	static void registerInReactor();
	void clientReady(std::tr1::shared_ptr<Client> client) {
	}
	void clientDisconnected(std::tr1::shared_ptr<Client> client) {
	}
	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);

	virtual ~HandleWait();

private:
	static void handleWAIT(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);

	static std::tr1::shared_ptr<HandleWait> mInst;
	static Poco::FastMutex sLock;

};

} /* namespace ddmlib */
#endif /* HANDLEWAIT_HPP_ */
