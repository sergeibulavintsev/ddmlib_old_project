/*
 * Client.cpp
 *
 *  Created on: Jan 26, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#ifdef CLIENT_SUPPORT
#include "Client.hpp"
#include "ClientData.hpp"
#include "AndroidDebugBridge.hpp"
#include "Device.hpp"
#include "DeviceMonitor.hpp"
#include "DdmConstants.hpp"
#include "DdmPreferences.hpp"
#include "Debugger.hpp"
#include "Log.hpp"
#include "JdwpPacket.hpp"
#include "HandleAppName.hpp"
#include "HandleExit.hpp"
#include "HandleHeap.hpp"
#include "HandleHello.hpp"
#include "HandleNativeHeap.hpp"
#include "HandleProfiling.hpp"
#include "HandleTest.hpp"
#include "HandleThread.hpp"
#include "HandleWait.hpp"
#include "DebugPortManager.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

const int Client::CLIENT_READY = 2;
const int Client::CLIENT_DISCONNECTED = 3;
unsigned int const Client::INITIAL_BUF_SIZE = 2 * 1024;
unsigned int const Client::MAX_BUF_SIZE = 200 * 1024 * 1024;
unsigned int const Client::WRITE_BUF_SIZE = 256;
Client* Client::selectedClient;

Client::Client(std::tr1::shared_ptr<Device> device, std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, int pid) {
	init();
	mDevice = device;
	mChan = chan;
	mDebuggerListenPort = DdmPreferences::getDebugPortBase();

	mReadBuffer = std::tr1::shared_ptr<ByteBuffer>(new ByteBuffer(INITIAL_BUF_SIZE));
	mWriteBuffer = std::tr1::shared_ptr<ByteBuffer>(new ByteBuffer(INITIAL_BUF_SIZE));

	mConnState = ST_INIT;

	mClientData = std::tr1::shared_ptr<ClientData>(new ClientData(pid));

	mThreadUpdateEnabled = DdmPreferences::getInitialThreadUpdate();
	mHeapUpdateEnabled = DdmPreferences::getInitialHeapUpdate();
}

Client::~Client() {
	if (mChan != nullptr) {
	AndroidDebugBridge::getReactor().removeEventHandler(*mChan,
			Poco::NObserver<Client, Poco::Net::ReadableNotification>(*this, &Client::processClientReadActivity));
	AndroidDebugBridge::getReactor().removeEventHandler(*mChan,
			Poco::NObserver<Client, Poco::Net::ErrorNotification>(*this, &Client::processClientError));
	AndroidDebugBridge::getReactor().removeEventHandler(*mChan,
			Poco::NObserver<Client, Poco::Net::ShutdownNotification>(*this, &Client::processClientShutdownActivity));
	}
	Log::v("ddms", "Client " + toString() + " is destroyed");
}

void Client::init() {
	Log::v("ddms", "New client created");
}

std::string Client::toString() const {
	return std::string("[Client pid: ") + Poco::NumberFormatter::format(mClientData->getPid()) + "]";
}

std::tr1::shared_ptr<Device> Client::getDevice() const {
	return mDevice.lock();
}

int Client::getDebuggerListenPort() {
	return mDebuggerListenPort;
}

bool Client::isDdmAware() const {
	switch (mConnState) {
	case ST_INIT:
	case ST_NOT_JDWP:
	case ST_AWAIT_SHAKE:
	case ST_NEED_DDM_PKT:
	case ST_NOT_DDM:
	case ST_ERROR:
	case ST_DISCONNECTED:
		return false;
	case ST_READY:
		return true;
	default:
		assert(false);
		return false;
	}
}

bool Client::isDebuggerAttached() {
	return mDebugger->isDebuggerAttached();
}

std::tr1::shared_ptr<Debugger> Client::getDebugger() {
	return mDebugger;
}

std::tr1::shared_ptr<ClientData> Client::getClientData() {
	return mClientData;
}

void Client::executeGarbageCollector() {
	try {
		HandleHeap::sendHPGC(shared_from_this());
	} catch (Poco::IOException &ioe) {
		Log::w("ddms", "Send of HPGC message failed");
		// ignore
	}
}

void Client::dumpHprof() {
	bool canStream = mClientData->hasFeature(ClientData::FEATURE_HPROF_STREAMING);
	try {
		if (canStream) {
			HandleHeap::sendHPDS(shared_from_this());
		} else {
			std::wstring clientDescription = mClientData->getClientDescription();
			std::wstring toRemove(L"\\:.*");
			size_t pos = clientDescription.find(toRemove);
			while (pos != clientDescription.npos) {
				clientDescription.erase(pos, toRemove.length());
			}
			std::wstring file = std::wstring(L"/sdcard/") + clientDescription + L".hprof";
			HandleHeap::sendHPDU(shared_from_this(), file);
		}
	} catch (Poco::IOException &e) {
		Log::w("ddms", "Send of HPDU message failed");
		// ignore
	}
}

void Client::toggleMethodProfiling() {
	bool canStream = mClientData->hasFeature(ClientData::FEATURE_PROFILING_STREAMING);
	try {
		if (mClientData->getMethodProfilingStatus() == MethodProfilingStatusON) {
			if (canStream) {
				HandleProfiling::sendMPSE(shared_from_this());
			} else {
				HandleProfiling::sendMPRE(shared_from_this());
			}
		} else {
			int bufferSize = DdmPreferences::getProfilerBufferSizeMb() * 1024 * 1024;
			if (canStream) {
				HandleProfiling::sendMPSS(shared_from_this(), bufferSize, 0);
			} else {
				std::wstring clientDescription = mClientData->getClientDescription();
				std::wstring toRemove(L"\\:.*");
				size_t pos = clientDescription.find(toRemove);
				while (pos != clientDescription.npos) {
					clientDescription.erase(pos, toRemove.length());
				}

				std::wstring file = std::wstring(L"/sdcard/") + clientDescription + DdmConstants::DOT_TRACE;
				HandleProfiling::sendMPRS(shared_from_this(), file, bufferSize, 0);
			}
		}
	} catch (Poco::IOException & e) {
		Log::w("ddms", "Toggle method profiling failed");
		// ignore
	}
}

void Client::requestMethodProfilingStatus() {
	try {
		HandleProfiling::sendMPRQ(shared_from_this());
	} catch (Poco::IOException & e) {
		Log::e("ddmlib", e.what());
	}
}

void Client::setThreadUpdateEnabled(bool enabled) {
	mThreadUpdateEnabled = enabled;
	if (enabled == false) {
		mClientData->clearThreads();
	}
	try {
		HandleThread::sendTHEN(shared_from_this(), enabled);
	} catch (Poco::IOException & ioe) {
		// ignore it here; client will clean up shortly
		Log::e("ddmlib", ioe.what());
	}
	update(CHANGE_THREAD_MODE);
}

bool Client::isThreadUpdateEnabled() {
	return mThreadUpdateEnabled;
}

void Client::requestThreadUpdate() {
	HandleThread::requestThreadUpdate(shared_from_this());
}

void Client::requestThreadStackTrace(int threadId) {
	HandleThread::requestThreadStackCallRefresh(shared_from_this(), threadId);
}

void Client::setHeapUpdateEnabled(bool enabled) {
	mHeapUpdateEnabled = enabled;
	try {
		HandleHeap::sendHPIF(shared_from_this(), enabled ? HandleHeap::HPIF_WHEN_EVERY_GC : HandleHeap::HPIF_WHEN_NEVER);
		HandleHeap::sendHPSG(shared_from_this(), enabled ? HandleHeap::WHEN_GC : HandleHeap::WHEN_DISABLE,
				HandleHeap::WHAT_MERGE);
	} catch (Poco::IOException & ioe) {
		// ignore it here; client will clean up shortly
	}
	update(CHANGE_HEAP_MODE);
}

bool Client::isHeapUpdateEnabled() {
	return mHeapUpdateEnabled;
}

bool Client::requestNativeHeapInformation() {
	try {
		HandleNativeHeap::sendNHGT(shared_from_this());
		return true;
	} catch (Poco::IOException & e) {
		Log::e("ddmlib", e.what());
	}
	return false;
}

void Client::enableAllocationTracker(bool enable) {
	try {
		HandleHeap::sendREAE(shared_from_this(), enable);
	} catch (Poco::IOException & e) {
		Log::e("ddmlib", e.what());
	}
}

void Client::requestAllocationStatus() {
	try {
		HandleHeap::sendREAQ(shared_from_this());
	} catch (Poco::IOException & e) {
		Log::e("ddmlib", e.what());
	}
}

void Client::requestAllocationDetails() {
	try {
		HandleHeap::sendREAL(shared_from_this());
	} catch (Poco::IOException & e) {
		Log::e("ddmlib", e.what());
	}
}

void Client::kill() {
	try {
		HandleExit::sendEXIT(shared_from_this(), 1);
	} catch (Poco::IOException & ioe) {
		Log::w("ddms", "Send of EXIT message failed");
		// ignore
	}
}

void Client::registerInReactor() {
	if (mChan != nullptr) {
		AndroidDebugBridge::getReactor().addEventHandler(*mChan,
				Poco::NObserver<Client, Poco::Net::ReadableNotification>(*this, &Client::processClientReadActivity));
		AndroidDebugBridge::getReactor().addEventHandler(*mChan,
				Poco::NObserver<Client, Poco::Net::ErrorNotification>(*this, &Client::processClientError));
		AndroidDebugBridge::getReactor().addEventHandler(*mChan,
				Poco::NObserver<Client, Poco::Net::ShutdownNotification>(*this, &Client::processClientShutdownActivity));
	}
}

void Client::setAsSelectedClient() {
	selectedClient = this;
}

bool Client::isSelectedClient() const {
	return (selectedClient == this);
}

void Client::listenForDebugger(int listenPort) {
	mDebuggerListenPort = listenPort;
	mDebugger = std::tr1::shared_ptr<Debugger>(new Debugger(shared_from_this(), listenPort));
}

bool Client::sendHandshake() {
	assert(mWriteBuffer->getPosition() == 0);
	try {
		// assume write buffer can hold 14 bytes
		JdwpPacket::putHandshake(mWriteBuffer);
		int expectedLen = mWriteBuffer->getPosition();
		mWriteBuffer->flip();
		if (mChan->sendBytes(mWriteBuffer->getArray(), expectedLen) != expectedLen) {
			throw Poco::IOException("partial handshake write");
		}
	} catch (Poco::IOException &ioe) {
		Log::e("ddms-client", std::string("IO error during handshake: ") + ioe.what());
		mConnState = ST_ERROR;
		close(true);
		mWriteBuffer->clear();
		return false;
	} catch (...) {
		mWriteBuffer->clear();
		throw;
	}
	mWriteBuffer->clear();
	mConnState = ST_AWAIT_SHAKE;
	return true;
}

void Client::sendAndConsume(std::tr1::shared_ptr<JdwpPacket> packet) {
	sendAndConsume(packet, std::tr1::shared_ptr<ChunkHandler>());
}

void Client::sendAndConsume(std::tr1::shared_ptr<JdwpPacket> packet, std::tr1::shared_ptr<ChunkHandler> replyHandler) {
	if (mChan == nullptr) {
		// can happen for e.g. THST packets
		Log::v("ddms", "Not sending packet -- client is closed");
		return;
	}
	if (replyHandler != nullptr) {
		/*
		 * Add the ID to the list of outstanding requests.  We have to do
		 * this before sending the packet, in case the response comes back
		 * before our thread returns from the packet-send function.
		 */
		addRequestId(packet->getId(), replyHandler);
	}
	static Poco::Mutex chanLock;
	{
		Poco::ScopedLock<Poco::Mutex> lock(chanLock);
		try {
			packet->writeAndConsume(mChan);
		} catch (Poco::IOException & ioe) {
			removeRequestId(packet->getId());
			throw;
		}
	}
}

void Client::forwardPacketToDebugger(std::tr1::shared_ptr<JdwpPacket> packet) {
	if (mDebugger == nullptr) {
		Log::d("ddms", "Discarding packet");
		packet->consume();
	} else {
		mDebugger->sendAndConsume(packet);
	}
}

void Client::read() {
	unsigned int count = 0;
	int pos = -1;

	try {

		count = mChan->available();

		pos = mReadBuffer->getPosition();
		if (count < 0)
			throw Poco::IOException("read failed");

		if ((mReadBuffer->getPosition() + count) >= mReadBuffer->size()) {
			if (mReadBuffer->getLimit()*2 > MAX_BUF_SIZE) {
				Log::e("ddms", "Exceeded MAX_BUF_SIZE!");
				throw std::overflow_error("Exceeded MAX_BUF_SIZE!");
			}

			Log::d("ddms", "Expanding read buffer to " + Poco::NumberFormatter::format(mReadBuffer->getLimit()*2));

			std::tr1::shared_ptr<ByteBuffer> newBuffer(new ByteBuffer(mReadBuffer->getLimit()*2));
			// copy entire buffer to new buffer
			mReadBuffer->setPosition(0);
			newBuffer->put(mReadBuffer.get()); // leaves "position" at end of copied
			mReadBuffer.swap(newBuffer);
		}

		pos += mChan->receiveBytes(mReadBuffer->getArray() + pos, count);

	} catch (Poco::TimeoutException& e) {
		Log::e("Client", "Timeout error");
	}


	mReadBuffer->setPosition(pos);
	if (Log::Config::LOGV)
		Log::v("ddms", "Read " + Poco::NumberFormatter::format(count) + " bytes from " + toString());

	//Log.hexDump("ddms", Log.DEBUG, mReadBuffer.array(),
	//    mReadBuffer.arrayOffset(), mReadBuffer.position());
}
std::tr1::shared_ptr<JdwpPacket> Client::getJdwpPacket() {
	/*
	 * On entry, the data starts at offset 0 and ends at "position".
	 * "limit" is set to the buffer capacity.
	 */
	if (mConnState == ST_AWAIT_SHAKE) {
		/*
		 * The first thing we get from the client is a response to our
		 * handshake.  It doesn't look like a packet, so we have to
		 * handle it specially.
		 */
		unsigned int result;

		result = JdwpPacket::findHandshake(mReadBuffer);
		//Log.v("ddms", "findHand: " + result);
		if (result == JdwpPacket::HANDSHAKE_GOOD) {
			Log::d("ddms",
					"Good handshake from client, sending HELO to " + Poco::NumberFormatter::format(mClientData->getPid()));
			JdwpPacket::consumeHandshake(mReadBuffer);
			mConnState = ST_NEED_DDM_PKT;
			HandleHello::sendHelloCommands(shared_from_this(), SERVER_PROTOCOL_VERSION);
			// see if we have another packet in the buffer
			return getJdwpPacket();
		} else if (result == JdwpPacket::HANDSHAKE_BAD) {
			Log::d("ddms", "Bad handshake from client");
			if (true /* TODO: make configurable */) {
				// we should drop the client, but also attempt to reopen it.
				// This is done by the DeviceMonitor.
				mDevice.lock()->getMonitor()->addClientToDropAndReopen(shared_from_this(),
						IDebugPortProvider::NO_STATIC_PORT);
			} else {
				// mark it as bad, close the socket, and don't retry
				mConnState = ST_NOT_JDWP;
				close(true /* notify */);
			}
		} else if (result == JdwpPacket::HANDSHAKE_NOTYET) {
			Log::d("ddms", "No handshake from client yet.");
		} else {
			Log::e("ddms", "Unknown packet while waiting for client handshake");
		}
		return std::tr1::shared_ptr<JdwpPacket>();
	} else if (mConnState == ST_NEED_DDM_PKT || mConnState == ST_NOT_DDM || mConnState == ST_READY) {
		/*
		 * Normal packet traffic.
		 */
		if (mReadBuffer->getPosition() != 0) {
			if (Log::Config::LOGV)
				Log::v("ddms", "Checking " + Poco::NumberFormatter::format(mReadBuffer->getPosition()) + " bytes");
		}
		return JdwpPacket::findPacket(mReadBuffer);
	} else {
		/*
		 * Not expecting data when in this state.
		 */
		Log::e("ddms", "Receiving data in state = " + Poco::NumberFormatter::format(mConnState));
	}
	return std::tr1::shared_ptr<JdwpPacket>();
}

void Client::addRequestId(int id, std::tr1::shared_ptr<ChunkHandler> handler) {
	Poco::ScopedLock<Poco::Mutex> lock(mOutstandingReqsLock);
	if (Log::Config::LOGV)
		Log::v("ddms", "Adding req " + Poco::NumberFormatter::formatHex(id) + " to set");

	mOutstandingReqs[id] = handler;
}
void Client::removeRequestId(int id) {
	Poco::ScopedLock<Poco::Mutex> lock(mOutstandingReqsLock);
	if (Log::Config::LOGV)
		Log::v("ddms", "Removing req " + Poco::NumberFormatter::formatHex(id) + " from set");

	mOutstandingReqs.erase(id);
}
std::tr1::shared_ptr<ChunkHandler> Client::isResponseToUs(int id) {
	Poco::ScopedLock<Poco::Mutex> lock(mOutstandingReqsLock);
	std::tr1::shared_ptr<ChunkHandler> handler = mOutstandingReqs[id];
	if (handler != nullptr) {
		if (Log::Config::LOGV)
			Log::v("ddms", "Found 0x" + Poco::NumberFormatter::formatHex(id) + " in request set");
		return handler;
	}
	return std::tr1::shared_ptr<ChunkHandler>();
}

void Client::packetFailed(JdwpPacket & reply) {
	if (mConnState == ST_NEED_DDM_PKT) {
		Log::d("ddms", "Marking " + toString() + " as non-DDM client");
		mConnState = ST_NOT_DDM;
	} else if (mConnState != ST_NOT_DDM) {
		Log::w("ddms", "WEIRD: got JDWP failure packet on DDM req");
	}

}

bool Client::ddmSeen() {
	if (mConnState == ST_NEED_DDM_PKT) {
		mConnState = ST_READY;
		return false;
	} else if (mConnState != ST_READY) {
		Log::w("ddms", "WEIRD: in ddmSeen with state=" + Poco::NumberFormatter::format(mConnState));
	}

	return true;
}

void Client::close(bool notify) {
	Log::d("ddms", "Closing " + toString());
	mOutstandingReqs.clear();
	try {
		if (mChan != nullptr) {
			mChan->close();
			mChan.reset();
		}

		if (mDebugger != nullptr) {
			mDebugger->close();
			mDebugger.reset();
		}
	} catch (Poco::IOException &ioe) {
		Log::w("ddms", "failed to close " + toString());
		// swallow it -- not much else to do
	}
	mDevice.lock()->removeClient(shared_from_this(), notify);
}

bool Client::isValid() const {
	return mChan != nullptr;
}

void Client::update(int changeMask) {
	mDevice.lock()->update(shared_from_this(),changeMask);
}

void Client::processClientReadActivity(const Poco::AutoPtr<Poco::Net::ReadableNotification> & notification) {
	try {
		if (mChan == nullptr) {
			Log::d("ddms", toString() + ": Socket is not open! How did that even happen?");
			dropClient(true);
		}

		read();

		/*
		 * See if we have a full packet in the buffer. It's possible we have
		 * more than one packet, so we have to loop.
		 */
		std::tr1::shared_ptr<JdwpPacket> packet = getJdwpPacket();
		while (packet != nullptr) {
			if (packet->isDdmPacket()) {
				// unsolicited DDM request - hand it off
				assert(!packet->isReply());
				ChunkHandler::callHandler(shared_from_this(), packet, std::tr1::shared_ptr<ChunkHandler>());
				packet->consume();
			} else if (packet->isReply() && isResponseToUs(packet->getId()) != 0) {
				// reply to earlier DDM request
				std::tr1::shared_ptr<ChunkHandler> handler = isResponseToUs(packet->getId());
				if (packet->isError())
					packetFailed(*packet.get());
				else if (packet->isEmpty())
					Log::d("ddms",
							"Got empty reply for 0x" + Poco::NumberFormatter::formatHex(packet->getId()) + " from "
									+ toString());
				else
					ChunkHandler::callHandler(shared_from_this(), packet, handler);
				packet->consume();
				removeRequestId(packet->getId());
			} else {
				Log::v("ddms",
						"Forwarding client " + (packet->isReply() ? std::string("reply") : std::string("event")) + " 0x"
								+ Poco::NumberFormatter::formatHex(packet->getId()) + " to " + getDebugger()->toString());
				forwardPacketToDebugger(packet);
			}

			// find next
			packet = getJdwpPacket();
		}
	} catch (Poco::IOException &ex) {
		// something closed down, no need to print anything. The client is simply dropped.
		dropClient(true /* notify */);
	} catch (std::exception &ex) {
		Log::e("ddms", ex.what());

		/* close the client; automatically un-registers from selector */
		dropClient(true /* notify */);

//        if (ex instanceof BufferOverflowException) {
//            Log.w("ddms",
//                    "Client data packet exceeded maximum buffer size "
//                            + client);
//        } else {
//            // don't know what this is, display it
//            Log.e("ddms", ex);
//        }
	}

}

void Client::processClientShutdownActivity(const Poco::AutoPtr<Poco::Net::ShutdownNotification> & notification) {
	Log::d("ddms", toString() + ": Received socket shutdown notification");
}

void Client::processClientError(const Poco::AutoPtr<Poco::Net::ErrorNotification> & notification) {
	Log::d("ddms", toString() + ": Received socket error notification");
	dropClient(true /* notify */);
}

void Client::dropClient(bool notify) {
	close(notify);
	ChunkHandler::broadcast(CLIENT_DISCONNECTED, shared_from_this());
}

} /* namespace ddmlib */
