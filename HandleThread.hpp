/*
 * HandleThread.hpp
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#ifndef HANDLETHREAD_HPP_
#define HANDLETHREAD_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

class DDMLIB_LOCAL HandleThread: public ChunkHandler {
public:
	static int CHUNK_THEN;
	static int CHUNK_THCR;
	static int CHUNK_THDE;
	static int CHUNK_THST;
	static int CHUNK_THNM;
	static int CHUNK_STKL;
	HandleThread();

	/**
	 * Register for the packets we expect to get from the client.
	 */
	static void registerInReactor();
	/**
	 * Client is ready.
	 */
	void clientReady(std::tr1::shared_ptr<Client> client);
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
	 * Send a THEN (THread notification ENable) request to the client.
	 */
	static void sendTHEN(std::tr1::shared_ptr<Client> client, bool enable);
	/**
	 * Send a STKL (STacK List) request to the client.  The VM will suspend
	 * the target thread, obtain its stack, and return it.  If the thread
	 * is no longer running, a failure result will be returned.
	 */
	static void sendSTKL(std::tr1::shared_ptr<Client> client, int threadId);
	/**
	 * This is called periodically from the UI thread.  To avoid locking
	 * the UI while we request the updates, we create a new thread.
	 *
	 */
	static void requestThreadUpdate(std::tr1::shared_ptr<Client> client);
	static void requestThreadStackCallRefresh(std::tr1::shared_ptr<Client> client, int threadId);

	virtual ~HandleThread();

	class RequestThread: public Poco::Runnable {

	public:
		RequestThread(volatile bool* mThreadStatus);

		void setClient(std::tr1::shared_ptr<Client> client);

		void run();

	private:
		std::tr1::shared_ptr<Client> mClient;
		volatile bool *m_pThreadStatusReqRunning;
	};

	class RequestThreadStack: public Poco::Runnable {
	public:
		RequestThreadStack(volatile bool *mThreadStackTrace);
		void setClientAndTID(std::tr1::shared_ptr<Client> client, int tid);
		void run();
	private:
		std::tr1::shared_ptr<Client> mClient;
		volatile bool *m_pThreadStackTraceReqRunning;
		int mThreadId;
	};

private:
	/*
	 * Handle a thread creation message.
	 *
	 * We should be tolerant of receiving a duplicate create message.  (It
	 * shouldn't happen with the current implementation.)
	 */
	void handleTHCR(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	/*
	 * Handle a thread death message.
	 */
	void handleTHDE(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) /*
	 * Handle a thread status update message.
	 *
	 * Response has:
	 *  (1b) header len
	 *  (1b) bytes per entry
	 *  (2b) thread count
	 * Then, for each thread:
	 *  (4b) threadId (matches value from THCR)
	 *  (1b) thread status
	 *  (4b) tid
	 *  (4b) utime
	 *  (4b) stime
	 */;
	void handleTHST(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	/*
	 * Handle a THNM (THread NaMe) message.  We get one of these after
	 * somebody calls Thread.setName() on a running thread.
	 */
	void handleTHNM(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	/**
	 * Parse an incoming STKL.
	 */
	void handleSTKL(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
	/*
	 * Send a THST request to the specified client.
	 */
	static void sendTHST(std::tr1::shared_ptr<Client> client);

	static std::tr1::shared_ptr<HandleThread> mInst;

	// only read/written by requestThreadUpdates()
	static volatile bool mThreadStatusReqRunning;
	static volatile bool mThreadStackTraceReqRunning;
	static Poco::Thread ThreadUpdate;
	static Poco::Thread ThreadCallRefresh;
	static RequestThread rThread;
	static RequestThreadStack rThreadStackCall;

};

} /* namespace ddmlib */
#endif /* HANDLETHREAD_HPP_ */
