/*
 * HandleAppname.hpp
 *
 *  Created on: Feb 10, 2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleAppName.hpp"
#include "Log.hpp"
#include "ClientData.hpp"
#include "JdwpPacket.hpp"
#include "Client.hpp"
#include "ByteBuffer.hpp"

class ByteBuffer;

namespace ddmlib {

int HandleAppName::CHUNK_APNM = ChunkHandler::type("APNM");
std::tr1::shared_ptr<HandleAppName> HandleAppName::mInst(new HandleAppName);
Poco::FastMutex HandleAppName::sLock;

HandleAppName::HandleAppName() {
}

HandleAppName::~HandleAppName() {
}

void HandleAppName::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {

	Log::d("ddm-appname", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_APNM) {
		assert(!isReply);
		handleAPNM(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}
}

void HandleAppName::handleAPNM(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int appNameLen;
	std::wstring appName;

	appNameLen = data->getInt();
	appName = getString(data, appNameLen);

	Log::d("ddm-appname", "APNM: app='" + Log::convertUtf16ToUtf8(appName) + "'");

	std::tr1::shared_ptr<ClientData> cd = client->getClientData();
	Poco::ScopedLock<Poco::FastMutex> lock(sLock);
	cd->setClientDescription(appName);

	client = checkDebuggerPortForAppName(client, appName);

	if (client != nullptr) {
		client->update(Client::CHANGE_NAME);
	}
}

} /* namespace ddmlib */
