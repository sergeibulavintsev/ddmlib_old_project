/*
 * HandleHello.h
 *
 *  Created on: Feb 13, 2012
 *      Author: sergey bulavintsev
 */

#ifndef HANDLEHELLO_HPP_
#define HANDLEHELLO_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

class DDMLIB_LOCAL HandleHello: public ChunkHandler {
public:
	HandleHello();
	virtual ~HandleHello();
	static int CHUNK_HELO;
	static int CHUNK_FEAT;
	static void registerInReactor();
	void clientReady(std::tr1::shared_ptr<Client> client);
	void clientDisconnected(std::tr1::shared_ptr<Client> client);
    /**
     * Sends HELLO-type commands to the VM after a good handshake.
     * @param client
     * @param serverProtocolVersion
     * @throws IOException
     */
	static void sendHelloCommands(std::tr1::shared_ptr<Client> client, int serverProtocolVersion);
	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);
    /**
     * Send a HELO request to the client.
     */
	static void sendHELO(std::tr1::shared_ptr<Client> client, int serverProtocolVersion);
    /**
     * Send a FEAT request to the client.
     */
	static void sendFEAT(std::tr1::shared_ptr<Client> client);

private:
	static std::tr1::shared_ptr<HandleHello> mInst;
	/*
	 * Handle a reply to our HELO message.
	 */
	static void handleHELO(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
    /**
     * Handle a reply to our FEAT request.
     */
	static void handleFEAT(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	static Poco::FastMutex sMutex;
};

} /* namespace ddmlib */
#endif /* HANDLEHELLO_HPP_ */
