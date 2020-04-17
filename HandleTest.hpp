/*
 * HandleTest.hpp
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#ifndef HANDLETEST_HPP_
#define HANDLETEST_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

class DDMLIB_LOCAL HandleTest: public ChunkHandler {
public:
	static int CHUNK_TEST;
	HandleTest();
	static void registerInReactor();
	void clientReady(std::tr1::shared_ptr<Client> client) {
	}
	;
	void clientDisconnected(std::tr1::shared_ptr<Client> client) {
	}
	;
	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);

	virtual ~HandleTest();
private:
	void handleTEST(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);

	static std::tr1::shared_ptr<HandleTest> mInst;
};

} /* namespace ddmlib */
#endif /* HANDLETEST_HPP_ */
