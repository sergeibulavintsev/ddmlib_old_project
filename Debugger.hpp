/*
 * Debugger.hpp
 *
 *  Created on: Jan 26, 2012
 *      Author: Ilya Polenov
 */

#ifndef DEBUGGER_HPP_
#define DEBUGGER_HPP_
#include "ddmlib.hpp"

class ByteBuffer;

namespace ddmlib {

class JdwpPacket;
class Client;
class ClientData;

/**
 * This represents a pending or established connection with a JDWP debugger.
 */
class DDMLIB_API Debugger {

	/*
	 * Messages from the debugger should be pretty small; may not even
	 * need an expanding-buffer implementation for this.
	 */
	const static int INITIAL_BUF_SIZE;
	const static int MAX_BUF_SIZE;
	std::tr1::shared_ptr<ByteBuffer> mReadBuffer;

	const static int PRE_DATA_BUF_SIZE;
	std::tr1::shared_ptr<ByteBuffer> mPreDataBuffer;

	/* connection state */
	int mConnState;
	const static int ST_NOT_CONNECTED;
	const static int ST_AWAIT_SHAKE;
	const static int ST_READY;

	/* peer */
	std::tr1::weak_ptr<Client> mClient; // client we're forwarding to/from
	int mListenPort; // listen to me
	std::tr1::shared_ptr<Poco::Net::ServerSocket> mListenChannel;

	/* this goes up and down; synchronize methods that access the field */
	std::tr1::shared_ptr<Poco::Net::StreamSocket> mChannel;
	Poco::Mutex mLock;

	void Init() {
	}

	/**
	 * Send the handshake to the debugger.  We also send along any packets
	 * we already received from the client (usually just a VM_START event,
	 * if anything at all).
	 */
	void sendHandshake();

public:
	Debugger(const std::tr1::shared_ptr<Client> &client, int listenPort);
	virtual ~Debugger();

	/**
	 * Returns "true" if a debugger is currently attached to us.
	 */
	bool isDebuggerAttached();

	/**
	 * Represent the Debugger as a string.
	 */
	std::string toString();

	/**
	 * Register the debugger's listen socket in the reactor.
	 */
	void registerInReactor();

	/**
	 * Return the Client being debugged.
	 */
	std::tr1::shared_ptr<Client> getClient();

	/**
	 * Accept a new connection, but only if we don't already have one.
	 *
	 * Must be synchronized with other uses of mChannel and mPreBuffer.
	 *
	 * Returns "null" if we're already talking to somebody.
	 */
	std::tr1::shared_ptr<Poco::Net::StreamSocket> accept();

	/**
	 * Accept a new connection from the specified listen channel.  This
	 * is so we can listen on a dedicated port for the "current" client,
	 * where "current" is constantly in flux.
	 *
	 * Must be synchronized with other uses of mChannel and mPreBuffer.
	 *
	 * Returns "null" if we're already talking to somebody.
	 */
	std::tr1::shared_ptr<Poco::Net::StreamSocket> accept(std::tr1::shared_ptr<Poco::Net::ServerSocket> listenChan);

	/**
	 * Close the data connection only.
	 */
	void closeData();

	/**
	 * Close the socket that's listening for new connections and (if
	 * we're connected) the debugger data socket.
	 */
	void close();

	/**
	 * Read data from our channel.
	 *
	 * This is called when data is known to be available, and we don't yet
	 * have a full packet in the buffer.  If the buffer is at capacity,
	 * expand it.
	 */
	void read();

	/**
	 * Return information for the first full JDWP packet in the buffer.
	 *
	 * If we don't yet have a full packet, return null.
	 *
	 * If we haven't yet received the JDWP handshake, we watch for it here
	 * and consume it without admitting to have done so.  We also send
	 * the handshake response to the debugger, along with any pending
	 * pre-connection data, which is why this can throw an IOException.
	 */
	std::tr1::shared_ptr<JdwpPacket> getJdwpPacket();

	/**
	 * Forward a packet to the client.
	 *
	 * "mClient" will never be null, though it's possible that the channel
	 * in the client has closed and our send attempt will fail.
	 *
	 * Consumes the packet.
	 */
	void forwardPacketToClient(std::tr1::shared_ptr<JdwpPacket> packet);

	/**
	 * Send a packet to the debugger.
	 *
	 * Ideally, we can do this with a single channel write.  If that doesn't
	 * happen, we have to prevent anybody else from writing to the channel
	 * until this packet completes, so we synchronize on the channel.
	 *
	 * Another goal is to avoid unnecessary buffer copies, so we write
	 * directly out of the JdwpPacket's ByteBuffer.
	 *
	 * We must synchronize on "mChannel" before writing to it.  We want to
	 * coordinate the buffered data with mChannel creation, so this whole
	 * method is synchronized.
	 */
	void sendAndConsume(std::tr1::shared_ptr<JdwpPacket> packet);

	/*
	 * Process activity from one of the debugger sockets. This could be a new
	 * connection or a data packet.
	 */
	void processDebuggerActivity(const Poco::AutoPtr<Poco::Net::ReadableNotification> &event);

	/*
	 * We have incoming data from the debugger. Forward it to the client.
	 */
	void processDebuggerData(const Poco::AutoPtr<Poco::Net::ReadableNotification> & event);
};

} /* namespace ddmlib */
#endif /* DEBUGGER_HPP_ */
