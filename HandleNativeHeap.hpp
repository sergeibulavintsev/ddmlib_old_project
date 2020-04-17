/*
 * HandleNativeHeap.h
 *
 *  Created on: Feb 13, 2012
 *      Author: sergey bulavintsev
 */

#ifndef HANDLENATIVEHEAP_HPP_
#define HANDLENATIVEHEAP_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;
class ClientData;

class DDMLIB_LOCAL HandleNativeHeap: public ChunkHandler {
public:
	HandleNativeHeap();
	virtual ~HandleNativeHeap();

	static int CHUNK_NHGT; 
	static int CHUNK_NHSG; 
	static int CHUNK_NHST; 
	static int CHUNK_NHEN; 

	static void registerInReactor();
	void clientReady(std::tr1::shared_ptr<Client> client) {
	}
	;
	void clientDisconnected(std::tr1::shared_ptr<Client> client) {
	}
	;
	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);
	static void sendNHGT(std::tr1::shared_ptr<Client> client);

private:
	static std::tr1::shared_ptr<HandleNativeHeap> mInst;
	void handleNHGT(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	void handleNHSG(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	void parseMaps(std::tr1::shared_ptr<ClientData> cd, std::vector< unsigned char > maps);

};

} /* namespace ddmlib */
#endif /* HANDLENATIVEHEAP_HPP_ */
