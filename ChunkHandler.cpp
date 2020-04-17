/*
 * ChunkHandler.cpp
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "ChunkHandler.hpp"
#include "Client.hpp"
#include "ByteBuffer.hpp"
#include "JdwpPacket.hpp"
#include "Log.hpp"
#include "DeviceMonitor.hpp"
#include "DebugPortManager.hpp"
#include "AndroidDebugBridge.hpp"

namespace ddmlib {

int ChunkHandler::CHUNK_FAIL = ChunkHandler::type("FAIL");
bool ChunkHandler::CHUNK_ORDER = false;
const int ChunkHandler::CHUNK_HEADER_LEN = 8;
const int ChunkHandler::CLIENT_READY = 2;
const int ChunkHandler::CLIENT_DISCONNECTED = 3;

Poco::FastMutex ChunkHandler::sLock;
std::map<int, std::tr1::shared_ptr<ChunkHandler> > ChunkHandler::mHandlerMap;

ChunkHandler::ChunkHandler() {
}

ChunkHandler::~ChunkHandler() {
}

/**
 * Handle chunks not recognized by handlers.  The handleChunk() method
 * in sub-classes should call this if the chunk type isn't recognized.
 */
void ChunkHandler::handleUnknownChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data,
		bool isReply, int msgId) {

	if (type == CHUNK_FAIL) {

		int errorCode, msgLen;
		std::wstring msg;

		errorCode = data->getInt();
		msgLen = data->getInt();
		msg = getString(data, msgLen);
		Log::w("ddms", "WARNING: failure code=" + Poco::NumberFormatter::format(errorCode) + " msg=" + Log::convertUtf16ToUtf8(msg));

	} else {

		std::stringstream bool_to_string;
		bool_to_string << isReply;
		Log::w("ddms",
				"WARNING: received unknown chunk " + name(type) + ": len=" + Poco::NumberFormatter::format(data->getLimit())
						+ ", reply=" + bool_to_string.str() + ", msgId=0x" + Poco::NumberFormatter::formatHex(msgId)); //Integer.toHexString(msgId));

	}
	Log::w("ddms", "client " + client->toString() + ", handler " + Poco::NumberFormatter::format(this));

}
/**
 * Utility function to copy a std::string out of a ByteBuffer.
 *
 * This is here because multiple chunk handlers can make use of it,
 * and there's nowhere better to put it.
 */
std::wstring ChunkHandler::getString(std::tr1::shared_ptr<ByteBuffer> buf, int len) {
	std::wstring data(len, ' ');
	for (int i = 0; i < len; i++)
		data[i] = buf->getWChar();
	return data;
}

/**
 * Utility function to copy a std::string into a ByteBuffer.
 */
void ChunkHandler::putString(std::tr1::shared_ptr<ByteBuffer> buf, const std::wstring& str) {
	int len = str.length();
	for (int i = 0; i < len; i++)
		buf->putWChar(str.at(i));
}

/**
 * Convert a 4-character std::string to a 32-bit type.
 */
int ChunkHandler::type(const std::string &typeName) {
	int val = 0;

	if (typeName.length() != 4) {
		Log::e("ddms", "Type name must be 4 letter long");
		throw Poco::RuntimeException("Type name must be 4 letter long");
	}

	for (int i = 0; i < 4; i++) {
		val <<= 8;
		val |= (int) typeName.at(i);
	}

	return val;
}

/**
 * Convert an integer type to a 4-character std::string.
 */
std::string ChunkHandler::name(int type) {
	char ascii[5];

	ascii[0] = (char) ((type >> 24) & 0xff);
	ascii[1] = (char) ((type >> 16) & 0xff);
	ascii[2] = (char) ((type >> 8) & 0xff);
	ascii[3] = (char) (type & 0xff);
	ascii[4] = '\0';

	return std::string(ascii);
}

/**
 * Allocate a ByteBuffer with enough space to hold the JDWP packet
 * header and one chunk header in addition to the demands of the
 * chunk being created.
 *
 * "maxChunkLen" indicates the size of the chunk contents only.
 */
std::tr1::shared_ptr<ByteBuffer> ChunkHandler::allocBuffer(int maxChunkLen) {
	std::tr1::shared_ptr<ByteBuffer> buf(new ByteBuffer(JdwpPacket::JDWP_HEADER_LEN + 8 + maxChunkLen));
	buf->setSwapEndianness(CHUNK_ORDER);
	return buf;
}

/**
 * Return the slice of the JDWP packet buffer that holds just the
 * chunk data.
 */
std::tr1::shared_ptr<ByteBuffer> ChunkHandler::getChunkDataBuf(std::tr1::shared_ptr<ByteBuffer> jdwpBuf) {

	assert(jdwpBuf->getPosition() == 0);

	jdwpBuf->setPosition(JdwpPacket::JDWP_HEADER_LEN + CHUNK_HEADER_LEN);
	std::tr1::shared_ptr<ByteBuffer> slice(jdwpBuf->slice());
	slice->setSwapEndianness(CHUNK_ORDER);
	jdwpBuf->setPosition(0);

	return slice;
}

/**
 * Write the chunk header at the start of the chunk.
 *
 * Pass in the byte buffer returned by JdwpPacket.getPayload().
 */
void ChunkHandler::finishChunkPacket(std::tr1::shared_ptr<JdwpPacket> packet, int type, int chunkLen) {
	std::tr1::shared_ptr<ByteBuffer> buf = packet->getPayload();

	buf->putInt(0x00, type);
	buf->putInt(0x04, chunkLen);

	packet->finishPacket(CHUNK_HEADER_LEN + chunkLen);
}

/**
 * Check that the client is opened with the proper debugger port for the
 * specified application name, and if not, reopen it.
 * @param client
 * @param uiThread
 * @param appName
 * @return
 */
std::tr1::shared_ptr<Client> ChunkHandler::checkDebuggerPortForAppName(std::tr1::shared_ptr<Client> client,
		const std::wstring &appName) {

	std::tr1::shared_ptr<IDebugPortProvider> provider = DebugPortManager::getProvider();

	if (provider != nullptr) {
		std::tr1::shared_ptr<Device> device = client->getDevice();

		int newPort = provider->getPort(device, appName);
		if (newPort != IDebugPortProvider::NO_STATIC_PORT && newPort != client->getDebuggerListenPort()) {

			AndroidDebugBridge *bridge = AndroidDebugBridge::getBridge();
			if (bridge != nullptr) {
				std::tr1::shared_ptr<DeviceMonitor> deviceMonitor = bridge->getDeviceMonitor();
				if (deviceMonitor != nullptr) {
					deviceMonitor->addClientToDropAndReopen(client, newPort);
					client.reset();
				}
			}
		}
	}

	return client;
}

void ChunkHandler::registerChunkHandler(int type, std::tr1::shared_ptr<ChunkHandler> handler) {
		mHandlerMap[type] = handler;
}

void ChunkHandler::broadcast(int event, std::tr1::shared_ptr<Client> client) {
	Log::d("ddms", "broadcast " + Poco::NumberFormatter::format(event) + ": " + client->toString());

	/*
	 * The handler objects appear once in mHandlerMap for each message they
	 * handle. We want to notify them once each, so we convert the HashMap
	 * to a HashSet before we iterate.
	 */
	std::set<std::tr1::shared_ptr<ChunkHandler> > setChunkHandlers;
	Poco::ScopedLock<Poco::FastMutex> lock(sLock);

	for (std::map<int, std::tr1::shared_ptr<ChunkHandler> >::iterator it = mHandlerMap.begin(); it != mHandlerMap.end(); ++it)
		setChunkHandlers.insert(it->second);

	std::set<std::tr1::shared_ptr<ChunkHandler> >::iterator iter = setChunkHandlers.begin();
	while (iter != setChunkHandlers.end()) {
		std::tr1::shared_ptr<ChunkHandler> handler = *iter;
		++iter;
		switch (event) {
		case CLIENT_READY:
			try {
				handler->clientReady(client);
			} catch (Poco::IOException& ioe) {
				// Something failed with the client. It should
				// fall out of the list the next time we try to
				// do something with it, so we discard the
				// exception here and assume cleanup will happen
				// later. May need to propagate farther. The
				// trouble is that not all values for "event" may
				// actually throw an exception.
				Log::w("ddms", "Got exception while broadcasting 'ready'");
				return;
			}
			break;
		case CLIENT_DISCONNECTED:
			handler->clientDisconnected(client);
			break;
		default:
			throw Poco::RuntimeException("Unsupported Operation Exception"); //unsupportedOperationException();
		}
	}
}

void ChunkHandler::callHandler(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<JdwpPacket> packet,
		std::tr1::shared_ptr<ChunkHandler> handler) {

	// on first DDM packet received, broadcast a "ready" message
	if (!client->ddmSeen())
		broadcast(CLIENT_READY, client);

	std::tr1::shared_ptr<ByteBuffer> buf = packet->getPayload();
	int type, length;
	bool reply = true;

	type = buf->getInt();
	length = buf->getInt();

	if (handler == nullptr) {
		// not a reply, figure out who wants it
		Poco::ScopedLock<Poco::FastMutex> lock(sLock);
		handler = mHandlerMap[type];
		reply = false;

	}

	if (handler == nullptr) {
		Log::w("ddms",
				"Received unsupported chunk type " + ChunkHandler::name(type) + " (len="
						+ Poco::NumberFormatter::format(length) + ")");
	} else {
		Log::d("ddms",
				"Calling handler for " + ChunkHandler::name(type) + " (len=" + Poco::NumberFormatter::format(length) + ")");
		std::tr1::shared_ptr<ByteBuffer> ibuf(buf->slice());
		ibuf->setSwapEndianness(true);
		// do the handling of the chunk synchronized on the client list
		// to be sure there's no concurrency issue when we look for HOME
		// in hasApp()
		handler->handleChunk(client, type, ibuf, reply, packet->getId());
	}
}

} /* namespace ddmlib */
