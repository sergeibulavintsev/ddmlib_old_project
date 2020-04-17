/*
 * HandleAppname.hpp
 *
 *  Created on: Feb 10, 2012
 *      Author: Sergey Bulavintsev
 */

#ifndef HANDLEAPPNAME_HPP_
#define HANDLEAPPNAME_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

class DDMLIB_LOCAL HandleAppName: public ChunkHandler {
public:
	HandleAppName();
	virtual ~HandleAppName();

	static int CHUNK_APNM; // = ChunkHandler.type("APNM");

	/**
	 * Register for the packets we expect to get from the client.
	 */
	static void registerInReactor() {
		registerChunkHandler(CHUNK_APNM, mInst);
	}

	void clientReady(std::tr1::shared_ptr<Client> client) {
	}

	void clientDisconnected(std::tr1::shared_ptr<Client> client) {
	}

	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);

private:
	static Poco::FastMutex sLock;
	static std::tr1::shared_ptr<HandleAppName> mInst; //= new HandleAppName();
	static void handleAPNM(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
};

} /* namespace ddmlib */
#endif /* HANDLEAPPNAME_HPP_ */
