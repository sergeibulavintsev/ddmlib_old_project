/*
 * HandleThread.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleThread.hpp"
#include "Log.hpp"
#include "Client.hpp"
#include "ThreadInfo.hpp"
#include "StackTraceElement.hpp"
#include "ByteBuffer.hpp"
#include "JdwpPacket.hpp"
#include "ClientData.hpp"

namespace ddmlib {

int HandleThread::CHUNK_THEN = HandleThread::type("THEN");
int HandleThread::CHUNK_THCR = HandleThread::type("THCR");
int HandleThread::CHUNK_THDE = HandleThread::type("THDE");
int HandleThread::CHUNK_THST = HandleThread::type("THST");
int HandleThread::CHUNK_THNM = HandleThread::type("THNM");
int HandleThread::CHUNK_STKL = HandleThread::type("STKL");

std::tr1::shared_ptr<HandleThread> HandleThread::mInst(new HandleThread);

volatile bool HandleThread::mThreadStatusReqRunning = false;
volatile bool HandleThread::mThreadStackTraceReqRunning = false;

Poco::Thread HandleThread::ThreadUpdate("Thread Status Req");
Poco::Thread HandleThread::ThreadCallRefresh("Thread Status Req");

HandleThread::RequestThread HandleThread::rThread(&HandleThread::mThreadStatusReqRunning);
HandleThread::RequestThreadStack HandleThread::rThreadStackCall(&HandleThread::mThreadStackTraceReqRunning);

HandleThread::HandleThread(){
}

HandleThread::~HandleThread() {
}

void HandleThread::registerInReactor() {
	registerChunkHandler(CHUNK_THCR, mInst);
	registerChunkHandler(CHUNK_THDE, mInst);
	registerChunkHandler(CHUNK_THST, mInst);
	registerChunkHandler(CHUNK_THNM, mInst);
	registerChunkHandler(CHUNK_STKL, mInst);
}

void HandleThread::clientReady(std::tr1::shared_ptr<Client> client) {
	Log::d("ddm-thread", "Now ready: " + client->toString());
	if (client->isThreadUpdateEnabled())
		sendTHEN(client, true);
}

void HandleThread::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {

	Log::d("ddm-thread", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_THCR) {
		handleTHCR(client, data);
	} else if (type == CHUNK_THDE) {
		handleTHDE(client, data);
	} else if (type == CHUNK_THST) {
		handleTHST(client, data);
	} else if (type == CHUNK_THNM) {
		handleTHNM(client, data);
	} else if (type == CHUNK_STKL) {
		handleSTKL(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}
}

void HandleThread::handleTHCR(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int threadId, nameLen;
	std::wstring name;
	threadId = data->getInt();
	nameLen = data->getInt();
	name = getString(data, nameLen);

	Log::v("ddm-thread", "THCR: " + Poco::NumberFormatter::format(threadId) + " '" + Log::convertUtf16ToUtf8(name) + "'");

	client->getClientData()->addThread(threadId, name);
	client->update(Client::CHANGE_THREAD_DATA);
}

void HandleThread::handleTHDE(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int threadId;

	threadId = data->getInt();
	Log::v("ddm-thread", "THDE: " + Poco::NumberFormatter::format(threadId));

	client->getClientData()->removeThread(threadId);
	client->update(Client::CHANGE_THREAD_DATA);
}

void HandleThread::handleTHST(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int headerLen, bytesPerEntry, extraPerEntry;
	int threadCount;

	headerLen = (data->get() & 0xff);
	bytesPerEntry = (data->get() & 0xff);
	threadCount = data->getShort();

	headerLen -= 4; // we've read 4 bytes
	while (headerLen-- > 0)
		data->get();

	extraPerEntry = bytesPerEntry - 18; // we want 18 bytes

	Log::v("ddm-thread", "THST: threadCount=" + Poco::NumberFormatter::format(threadCount));

	/*
	 * For each thread, extract the data, find the appropriate
	 * client, and add it to the ClientData.
	 */
	for (int i = 0; i < threadCount; i++) {
		int threadId, status, tid, utime, stime;
		bool isDaemon = false;

		threadId = data->getInt();
		status = data->get();
		tid = data->getInt();
		utime = data->getInt();
		stime = data->getInt();
		if (bytesPerEntry >= 18)
			isDaemon = (data->get() != 0);

		Log::v("ddm-thread",
				"  id=" + Poco::NumberFormatter::format(threadId) + ", status=" + Poco::NumberFormatter::format(status)
						+ ", tid=" + Poco::NumberFormatter::format(tid) + ", utime=" + Poco::NumberFormatter::format(utime)
						+ ", stime=" + Poco::NumberFormatter::format(stime));

		std::tr1::shared_ptr<ClientData> cd = client->getClientData();
		std::tr1::shared_ptr<ThreadInfo> threadInfo = cd->getThread(threadId);
		if ((threadInfo != nullptr) && (threadInfo->getThreadId() != -1))
				threadInfo->updateThread(status, tid, utime, stime, isDaemon);
		else
			Log::d("ddms", "Thread with id=" + Poco::NumberFormatter::format(threadId) + " not found");

		// slurp up any extra
		for (int slurp = extraPerEntry; slurp > 0; slurp--)
			data->get();
	}

	client->update(Client::CHANGE_THREAD_DATA);
}

void HandleThread::handleTHNM(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	int threadId, nameLen;
	std::wstring name;

	threadId = data->getInt();
	nameLen = data->getInt();
	name = getString(data, nameLen);

	Log::v("ddm-thread", "THNM: " + Poco::NumberFormatter::format(threadId) + " '" + Log::convertUtf16ToUtf8(name) + "'");

	std::tr1::shared_ptr<ThreadInfo> threadInfo = client->getClientData()->getThread(threadId);
	if ((threadInfo != nullptr) && (threadInfo->getThreadId() != -1)) {
		threadInfo->setThreadName(name);
		client->update(Client::CHANGE_THREAD_DATA);
	} else {
		Log::d("ddms", "Thread with id=" + Poco::NumberFormatter::format(threadId) + " not found");
	}
}

void HandleThread::handleSTKL(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	std::vector<std::tr1::shared_ptr<StackTraceElement> > trace;
	int future = data->getInt();
	int threadId = data->getInt();

	Log::v("ddms", "STKL: " + Poco::NumberFormatter::format(threadId));

	/* un-serialize the StackTraceElement[] */
	int stackDepth = data->getInt();
	trace.resize(stackDepth);
	for (int i = 0; i < stackDepth; i++) {
		std::wstring className, methodName, fileName;
		int len, lineNumber;

		len = data->getInt();
		className = getString(data, len);
		len = data->getInt();
		methodName = getString(data, len);
		len = data->getInt();
		if (len == 0) {
			fileName = L"";
		} else {
			fileName = getString(data, len);
		}
		lineNumber = data->getInt();

		trace[i] = std::tr1::shared_ptr<StackTraceElement>(new StackTraceElement(className, methodName, fileName, lineNumber));
	}

	std::tr1::shared_ptr<ThreadInfo> threadInfo = client->getClientData()->getThread(threadId);
	if ((threadInfo != nullptr) && (threadInfo->getThreadId() != -1)) {
		threadInfo->setStackCall(trace);
		client->update(Client::CHANGE_THREAD_STACKTRACE);
	} else {
		Log::d("STKL",
				"Got stackcall for thread" + Poco::NumberFormatter::format(threadId) + ", which does not exists (anymore?).");
	}
}

void HandleThread::sendTHEN(std::tr1::shared_ptr<Client> client, bool enable) {

	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(1);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	if (enable)
		buf->put((byte) 1);
	else
		buf->put((byte) 0);

	finishChunkPacket(packet, CHUNK_THEN, buf->getPosition());
	std::stringstream ss;
	ss << std::boolalpha << enable;
	Log::d("ddm-thread", "Sending " + name(CHUNK_THEN) + ": " + ss.str());
	client->sendAndConsume(packet, mInst);
}

void HandleThread::sendSTKL(std::tr1::shared_ptr<Client> client, int threadId) {

	if (false) {
		Log::d("ddm-thread", "would send STKL " + Poco::NumberFormatter::format(threadId));
		return;
	}

	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(4);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);
	buf->setSwapEndianness(true);
	buf->putInt(threadId);

	finishChunkPacket(packet, CHUNK_STKL, buf->getPosition());
	Log::d("ddm-thread", "Sending " + name(CHUNK_STKL) + ": " + Poco::NumberFormatter::format(threadId));
	client->sendAndConsume(packet, mInst);
}

void HandleThread::requestThreadUpdate(std::tr1::shared_ptr<Client> client) {
	if (client->isDdmAware() && client->isThreadUpdateEnabled()) {
		if (mThreadStatusReqRunning) {
			Log::w("ddms", "Waiting for previous thread update req to finish");
			return;
		}

		rThread.setClient(client);
		ThreadUpdate.start(rThread);
	}
}

void HandleThread::requestThreadStackCallRefresh(std::tr1::shared_ptr<Client> client, int threadId) {
	if (client->isDdmAware() && client->isThreadUpdateEnabled()) {
		if (mThreadStackTraceReqRunning) {
			Log::w("ddms", "Waiting for previous thread stack call req to finish");
			return;
		}
		rThreadStackCall.setClientAndTID(client, threadId);
		ThreadCallRefresh.start(rThreadStackCall);
	}

}

void HandleThread::sendTHST(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// nothing much to say

	finishChunkPacket(packet, CHUNK_THST, buf->getPosition());
	Log::d("ddm-thread", "Sending " + name(CHUNK_THST));
	client->sendAndConsume(packet, mInst);
}

HandleThread::RequestThread::RequestThread(volatile bool* mThreadStatus) :
		m_pThreadStatusReqRunning(mThreadStatus) {}

void HandleThread::RequestThread::setClient(std::tr1::shared_ptr<Client> client) {
	mClient = client;
}
void HandleThread::RequestThread::run() {
	*m_pThreadStatusReqRunning = true;
	try {

		HandleThread::sendTHST(mClient);
		*m_pThreadStatusReqRunning = false;
	} catch (Poco::IOException& ioe) {
		Log::d("ddms", "Unable to request thread updates from " + mClient->toString() + ": " + ioe.what());
		*m_pThreadStatusReqRunning = false;
	}
}

HandleThread::RequestThreadStack::RequestThreadStack(volatile bool *mThreadStackTrace) :
				m_pThreadStackTraceReqRunning(mThreadStackTrace),
				mThreadId(0) {
		}

void HandleThread::RequestThreadStack::setClientAndTID(std::tr1::shared_ptr<Client> client, int tid){
	mClient = client;
	mThreadId = tid;
}

void HandleThread::RequestThreadStack::run() {
	*m_pThreadStackTraceReqRunning = true;
	try {
		HandleThread::sendSTKL(mClient, mThreadId);
		*m_pThreadStackTraceReqRunning = false;
	} catch (Poco::IOException& ioe) {
		Log::d("ddms", "Unable to request thread stack call updates from " + mClient->toString() + ": " + ioe.what());
		*m_pThreadStackTraceReqRunning = false;
	}
}

} /* namespace ddmlib */
