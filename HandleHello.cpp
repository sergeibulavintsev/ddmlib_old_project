/*
 * HandleHello.cpp
 *
 *  Created on: Feb 13, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleHello.hpp"
#include "HandleProfiling.hpp"
#include "Log.hpp"
#include "JdwpPacket.hpp"
#include "Client.hpp"
#include "ClientData.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

int HandleHello::CHUNK_HELO = ChunkHandler::type("HELO");
int HandleHello::CHUNK_FEAT = ChunkHandler::type("FEAT");
std::tr1::shared_ptr<HandleHello> HandleHello::mInst(new HandleHello);

Poco::FastMutex HandleHello::sMutex;

HandleHello::HandleHello() {
}

HandleHello::~HandleHello() {
}

void HandleHello::registerInReactor() {
	registerChunkHandler(CHUNK_HELO, mInst);
}

void HandleHello::clientReady(std::tr1::shared_ptr<Client> client) {
	Log::d("ddm-hello", "Now ready: " + client->toString());
}

void HandleHello::clientDisconnected(std::tr1::shared_ptr<Client> client) {
	Log::d("ddm-hello", "Now disconnected: " + client->toString());
}

void HandleHello::sendHelloCommands(std::tr1::shared_ptr<Client> client, int serverProtocolVersion) {
	sendHELO(client, serverProtocolVersion);
	sendFEAT(client);
	HandleProfiling::sendMPRQ(client);
}

void HandleHello::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {

	Log::d("ddm-hello", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_HELO) {
		assert(isReply);
		handleHELO(client, data);
	} else if (type == CHUNK_FEAT) {
		handleFEAT(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}
}

void HandleHello::handleHELO(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int version, pid, vmIdentLen, appNameLen;
	std::wstring vmIdent, appName;

	version = data->getInt();
	pid = data->getInt();
	vmIdentLen = data->getInt();
	appNameLen = data->getInt();

	vmIdent = getString(data, vmIdentLen);
	appName = getString(data, appNameLen);

	Log::d("ddm-hello",
			"HELO: v=" + Poco::NumberFormatter::format(version) + ", pid=" + Poco::NumberFormatter::format(pid) + ", vm='"
					+ Log::convertUtf16ToUtf8(vmIdent) + "', app='" + Log::convertUtf16ToUtf8(appName) + "'");

	std::tr1::shared_ptr<ClientData> cd = client->getClientData();

	Poco::ScopedLock<Poco::FastMutex> lock(sMutex);
	if (cd->getPid() == pid) {
		cd->setVmIdentifier(vmIdent);
		cd->setClientDescription(appName);
		cd->setDdmAware(true);
	} else {
		Log::e("ddm-hello",
				"Received pid (" + Poco::NumberFormatter::format(pid) + ") does not match client pid ("
						+ Poco::NumberFormatter::format(cd->getPid()) + ")");
	}

	client = checkDebuggerPortForAppName(client, appName);

	if (client != nullptr) {
		client->update(Client::CHANGE_NAME);
	}
}

void HandleHello::sendHELO(std::tr1::shared_ptr<Client> client, int serverProtocolVersion) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(4);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);
	buf->setSwapEndianness(true);
	buf->putInt(serverProtocolVersion);

	finishChunkPacket(packet, CHUNK_HELO, buf->getPosition());
	Log::d("ddm-hello", "Sending " + name(CHUNK_HELO) + " ID=0x" + Poco::NumberFormatter::formatHex(packet->getId()));
	client->sendAndConsume(packet, mInst);
}

void HandleHello::handleFEAT(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int featureCount;
	int i;

	featureCount = data->getInt();
	for (i = 0; i < featureCount; i++) {
		int len = data->getInt();
		std::wstring feature = getString(data, len);
		client->getClientData()->addFeature(feature);

		Log::d("ddm-hello", "Feature: " + Log::convertUtf16ToUtf8(feature));
	}
}

void HandleHello::sendFEAT(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(4);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data

	finishChunkPacket(packet, CHUNK_FEAT, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_FEAT));
	client->sendAndConsume(packet, mInst);
}

} /* namespace ddmlib */
