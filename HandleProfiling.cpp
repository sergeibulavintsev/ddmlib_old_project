/*
 * HandleProfiling.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleProfiling.hpp"
#include "Client.hpp"
#include "ClientData.hpp"
#include "Log.hpp"
#include "JdwpPacket.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

int HandleProfiling::CHUNK_MPRS = HandleProfiling::type("MPRS");
int HandleProfiling::CHUNK_MPRE = HandleProfiling::type("MPRE");
int HandleProfiling::CHUNK_MPSS = HandleProfiling::type("MPSS");
int HandleProfiling::CHUNK_MPSE = HandleProfiling::type("MPSE");
int HandleProfiling::CHUNK_MPRQ = HandleProfiling::type("MPRQ");
int HandleProfiling::CHUNK_FAIL = HandleProfiling::type("FAIL");

std::tr1::shared_ptr<HandleProfiling> HandleProfiling::mInst(new HandleProfiling);

HandleProfiling::HandleProfiling() {
}

HandleProfiling::~HandleProfiling() {
}

void HandleProfiling::registerInReactor() {
	registerChunkHandler(CHUNK_MPRE, mInst);
	registerChunkHandler(CHUNK_MPSE, mInst);
	registerChunkHandler(CHUNK_MPRQ, mInst);
}

void HandleProfiling::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {

	Log::d("ddm-prof", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_MPRE) {
		handleMPRE(client, data);
	} else if (type == CHUNK_MPSE) {
		handleMPSE(client, data);
	} else if (type == CHUNK_MPRQ) {
		handleMPRQ(client, data);
	} else if (type == CHUNK_FAIL) {
		handleFAIL(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}
}

void HandleProfiling::sendMPRS(std::tr1::shared_ptr<Client> client, std::wstring fileName, int bufferSize, int flags) {

	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(3 * 4 + fileName.length() * 2);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);
	buf->setSwapEndianness(true);
	buf->putInt(bufferSize);
	buf->putInt(flags);
	buf->putInt(fileName.length());
	putString(buf, fileName);

	finishChunkPacket(packet, CHUNK_MPRS, buf->getPosition());
	Log::d("ddm-prof",
			"Sending " + name(CHUNK_MPRS) + " '" + Log::convertUtf16ToUtf8(fileName) + "', size=" + Poco::NumberFormatter::format(bufferSize)
					+ ", flags=" + Poco::NumberFormatter::format(flags));
	client->sendAndConsume(packet, mInst);

	// record the filename we asked for.
	client->getClientData()->setPendingMethodProfiling(fileName);

	// send a status query. this ensure that the status is properly updated if for some
	// reason starting the tracing failed.
	sendMPRQ(client);
}

void HandleProfiling::sendMPRE(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data

	finishChunkPacket(packet, CHUNK_MPRE, buf->getPosition());
	Log::d("ddm-prof", "Sending " + name(CHUNK_MPRE));
	client->sendAndConsume(packet, mInst);
}

void HandleProfiling::handleMPRE(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	unsigned char result;

	// get the filename and make the client not have pending HPROF dump anymore.
	std::wstring filename = client->getClientData()->getPendingMethodProfiling();
	client->getClientData()->setPendingMethodProfiling(L"");

	result = data->get();

	// get the app-level handler for method tracing dump
	std::tr1::shared_ptr<IMethodProfilingHandler> handler = ClientData::getMethodProfilingHandler();
	if (handler != nullptr) {
		if (result == 0) {
			handler->onSuccess(filename, client);

			Log::d("ddm-prof", "Method profiling has finished");
		} else {
			handler->onEndFailure(client, "" /*message*/);

			Log::w("ddm-prof", "Method profiling has failed (check device log)");
		}
	}

	client->getClientData()->setMethodProfilingStatus(MethodProfilingStatusOFF);
	client->update(Client::CHANGE_METHOD_PROFILING_STATUS);
}

void HandleProfiling::sendMPSS(std::tr1::shared_ptr<Client> client, int bufferSize, int flags) {

	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(2 * 4);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	buf->setSwapEndianness(true);
	buf->putInt(bufferSize);
	buf->putInt(flags);

	finishChunkPacket(packet, CHUNK_MPSS, buf->getPosition());
	Log::d("ddm-prof",
			"Sending " + name(CHUNK_MPSS) + "', size=" + Poco::NumberFormatter::format(bufferSize) + ", flags="
					+ Poco::NumberFormatter::format(flags));
	client->sendAndConsume(packet, mInst);

	// send a status query. this ensure that the status is properly updated if for some
	// reason starting the tracing failed.
	sendMPRQ(client);
}

void HandleProfiling::sendMPSE(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data

	finishChunkPacket(packet, CHUNK_MPSE, buf->getPosition());
	Log::d("ddm-prof", "Sending " + name(CHUNK_MPSE));
	client->sendAndConsume(packet, mInst);
}

void HandleProfiling::handleMPSE(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	std::tr1::shared_ptr<IMethodProfilingHandler> handler = ClientData::getMethodProfilingHandler();
	if (handler != nullptr) {
		unsigned char *stuff = new unsigned char[data->getCapacity()];
		std::memcpy(stuff, data->getArray(), data->getCapacity());
		Log::d("ddm-prof", "got trace file, size: " + Poco::NumberFormatter::format(data->getCapacity()) + " bytes");

		handler->onSuccess(stuff, data->getCapacity(), client);
		delete []stuff;
	}

	client->getClientData()->setMethodProfilingStatus(MethodProfilingStatusOFF);
	client->update(Client::CHANGE_METHOD_PROFILING_STATUS);
}

void HandleProfiling::sendMPRQ(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data

	finishChunkPacket(packet, CHUNK_MPRQ, buf->getPosition());
	Log::d("ddm-prof", "Sending " + name(CHUNK_MPRQ));
	client->sendAndConsume(packet, mInst);
}

void HandleProfiling::handleMPRQ(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	unsigned char result;

	result = data->get();

	if (result == 0) {
		client->getClientData()->setMethodProfilingStatus(MethodProfilingStatusOFF);
		Log::d("ddm-prof", "Method profiling is not running");
	} else {
		client->getClientData()->setMethodProfilingStatus(MethodProfilingStatusON);
		Log::d("ddm-prof", "Method profiling is running");
	}
	client->update(Client::CHANGE_METHOD_PROFILING_STATUS);
}

void HandleProfiling::handleFAIL(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int errorCode = data->getInt();
	int length = data->getInt();
	std::wstring message(length, L' ');

	message = getString(data, length);

	// this can be sent if
	// - MPRS failed (like wrong permission)
	// - MPSE failed for whatever reason

	std::wstring filename = client->getClientData()->getPendingMethodProfiling();
	if (!filename.empty()) {
		// reset the pending file.
		client->getClientData()->setPendingMethodProfiling(L"");

		// and notify of failure
		std::tr1::shared_ptr<IMethodProfilingHandler> handler = ClientData::getMethodProfilingHandler();
		if (handler != nullptr) {
			handler->onStartFailure(client, Log::convertUtf16ToUtf8(message));
		}
	} else {
		// this is MPRE
		// notify of failure
		std::tr1::shared_ptr<IMethodProfilingHandler> handler = ClientData::getMethodProfilingHandler();
		if (handler != nullptr) {
			handler->onEndFailure(client, Log::convertUtf16ToUtf8(message));
		}
	}
	Log::d("HandleProfiling", Log::convertUtf16ToUtf8(message));
	// send a query to know the current status
	try {
		sendMPRQ(client);
	} catch (Poco::IOException& e) {
		Log::e("HandleProfiling", e.what());
	}
}

} /* namespace ddmlib */
