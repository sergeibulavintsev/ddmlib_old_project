/*
 * HandleProfiling.h
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#ifndef HANDLEPROFILING_HPP_
#define HANDLEPROFILING_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

class DDMLIB_LOCAL HandleProfiling: public ChunkHandler {
public:
	static int CHUNK_MPRS;
	static int CHUNK_MPRE;
	static int CHUNK_MPSS;
	static int CHUNK_MPSE;
	static int CHUNK_MPRQ;
	static int CHUNK_FAIL;

	HandleProfiling();

	/**
	 * Register for the packets we expect to get from the client.
	 */
	static void registerInReactor();
	/**
	 * Client is ready.
	 */
	void clientReady(std::tr1::shared_ptr<Client> client) {
	}

	/**
	 * Client went away.
	 */
	void clientDisconnected(std::tr1::shared_ptr<Client> client) {
	}

	/**
	 * Chunk handler entry point.
	 */
	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);
	/**
	 * Send a MPRS (Method PRofiling Start) request to the client.
	 *
	 * The arguments to this method will eventually be passed to
	 * android.os.Debug.startMethodTracing() on the device.
	 *
	 * @param fileName is the name of the file to which profiling data
	 *          will be written (on the device); it will have {@link DdmConstants#DOT_TRACE}
	 *          appended if necessary
	 * @param bufferSize is the desired buffer size in bytes (8MB is good)
	 * @param flags see startMethodTracing() docs; use 0 for default behavior
	 */
	static void sendMPRS(std::tr1::shared_ptr<Client> client, std::wstring fileName, int bufferSize, int flags);
	/**
	 * Send a MPRE (Method PRofiling End) request to the client.
	 */
	static void sendMPRE(std::tr1::shared_ptr<Client> client);
	/**
	 * Send a MPSS (Method Profiling Streaming Start) request to the client.
	 *
	 * The arguments to this method will eventually be passed to
	 * android.os.Debug.startMethodTracing() on the device.
	 *
	 * @param bufferSize is the desired buffer size in bytes (8MB is good)
	 * @param flags see startMethodTracing() docs; use 0 for default behavior
	 */
	static void sendMPSS(std::tr1::shared_ptr<Client>, int bufferSize, int flags);
	/**
	 * Send a MPSE (Method Profiling Streaming End) request to the client.
	 */
	static void sendMPSE(std::tr1::shared_ptr<Client> client);
	/**
	 * Send a MPRQ (Method PRofiling Query) request to the client.
	 */
	static void sendMPRQ(std::tr1::shared_ptr<Client> client);

	virtual ~HandleProfiling();
private:
	/**
	 * Handle notification that method profiling has finished writing
	 * data to disk.
	 */
	void handleMPRE(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	/**
	 * Handle incoming profiling data.  The MPSE packet includes the
	 * complete .trace file.
	 */
	void handleMPSE(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	/**
	 * Receive response to query.
	 */
	void handleMPRQ(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	void handleFAIL(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	static std::tr1::shared_ptr<HandleProfiling> mInst;
};

} /* namespace ddmlib */
#endif /* HANDLEPROFILING_HPP_ */
