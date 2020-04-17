/*
 * HandleExit.hpp
 *
 *  Created on: Feb 10, 2012
 *      Author: Sergey Bulavintsev
 */

#ifndef HANDLEEXIT_HPP_
#define HANDLEEXIT_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

class DDMLIB_LOCAL HandleExit: public ChunkHandler {

public:

	static int CHUNK_EXIT;
	HandleExit();
	virtual ~HandleExit();

	static void registerInReactor() {
	}

	void clientReady(std::tr1::shared_ptr<Client> client) {
	}

	void clientDisconnected(std::tr1::shared_ptr<Client> client) {
	}
	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);
	static void sendEXIT(std::tr1::shared_ptr<Client> client, int status);

private:
	static std::tr1::shared_ptr<HandleExit> mInst;
};

} /* namespace ddmlib */
#endif /* HANDLEEXIT_HPP_ */
