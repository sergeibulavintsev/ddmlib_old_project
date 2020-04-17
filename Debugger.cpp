/*
 * Debugger.cpp
 *
 *  Created on: Jan 26, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "Debugger.hpp"
#include "ByteBuffer.hpp"
#include "Log.hpp"
#include "Device.hpp"
#include "DeviceMonitor.hpp"
#include "DebugPortManager.hpp"
#include "ByteBuffer.hpp"
#include "JdwpPacket.hpp"
#include "ClientData.hpp"
#include "Client.hpp"
#include "AndroidDebugBridge.hpp"

namespace ddmlib {
const int Debugger::INITIAL_BUF_SIZE = 1 * 1024;
const int Debugger::MAX_BUF_SIZE = 32 * 1024;

const int Debugger::PRE_DATA_BUF_SIZE = 256;

const int Debugger::ST_NOT_CONNECTED = 1;
const int Debugger::ST_AWAIT_SHAKE = 2;
const int Debugger::ST_READY = 3;

Debugger::Debugger(const std::tr1::shared_ptr<Client> & client, int listenPort) {
	mClient = client;
	mListenPort = listenPort;

	Poco::Net::SocketAddress addr("localhost", listenPort);
	mListenChannel = std::tr1::shared_ptr<Poco::Net::ServerSocket>(new Poco::Net::ServerSocket(addr));
//	mListenChannel->setBlocking(false); // required for Selector

//	mListenChannel->setReuseAddress(true); // enable SO_REUSEADDR
//	mListenChannel->bind(addr, true);

	mReadBuffer = std::tr1::shared_ptr<ByteBuffer>(new ByteBuffer(INITIAL_BUF_SIZE));
	mPreDataBuffer = std::tr1::shared_ptr<ByteBuffer>(new ByteBuffer(PRE_DATA_BUF_SIZE));
	mConnState = ST_NOT_CONNECTED;

	Log::d("ddms", "Debugger created: " + toString());
}

bool Debugger::isDebuggerAttached() {
	return mChannel != nullptr;
}

std::string Debugger::toString() {
	// mChannel != null means we have connection, ST_READY means it's going
	if (!mClient.expired())
		return std::string("[Debugger ") + Poco::NumberFormatter::format(mListenPort) + "-->"
			+ Poco::NumberFormatter::format(mClient.lock()->getClientData()->getPid())
			+ ((mConnState != ST_READY) ? " inactive]" : " active]");
	else
		return "";
}

void Debugger::registerInReactor() {
	if (mListenChannel != nullptr) {
		AndroidDebugBridge::getReactor().addEventHandler(*mListenChannel.get(),
				Poco::NObserver<Debugger, Poco::Net::ReadableNotification>(*this, &Debugger::processDebuggerActivity));
	}
}

std::tr1::shared_ptr<Client> Debugger::getClient() {
	return mClient.lock();
}

std::tr1::shared_ptr<Poco::Net::StreamSocket> Debugger::accept() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	return accept(mListenChannel);

}

std::tr1::shared_ptr<Poco::Net::StreamSocket> Debugger::accept(std::tr1::shared_ptr<Poco::Net::ServerSocket> listenChan) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	if (listenChan != nullptr) {
		Poco::Net::ServerSocket newChan;

		newChan = listenChan->acceptConnection();
		if (mChannel != nullptr) {
			Log::w("ddms",
					"debugger already talking to " + mClient.lock()->toString() + " on "
							+ Poco::NumberFormatter::format(mListenPort));
			newChan.close();
			return std::tr1::shared_ptr<Poco::Net::StreamSocket>();
		}
		mChannel = std::tr1::shared_ptr<Poco::Net::StreamSocket>(new Poco::Net::StreamSocket(newChan));
		mChannel->setBlocking(false); // required for Selector
		mConnState = ST_AWAIT_SHAKE;
		return mChannel;
	}

	return std::tr1::shared_ptr<Poco::Net::StreamSocket>();
}

void Debugger::closeData() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	try {
		if (mChannel != nullptr) {
			mChannel->close();
			mChannel.reset();
			mConnState = ST_NOT_CONNECTED;

			std::tr1::shared_ptr<ClientData> cd = mClient.lock()->getClientData();
			cd->setDebuggerInterest(DebuggerStatusDEFAULT);
			mClient.lock()->update(Client::CHANGE_DEBUGGER_STATUS);
		}
	} catch (Poco::IOException &ioe) {
		Log::w("ddms", "Failed to close data " + toString());
	}
}

void Debugger::close() {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	try {
		if (mListenChannel != nullptr) {
			mListenChannel->close();
		}
		mListenChannel.reset();
		closeData();
	} catch (Poco::IOException &ioe) {
		Log::w("ddms", "Failed to close listener " + toString());
	}
}

void Debugger::read() {
	int count;
	/*
	 if (mReadBuffer->getPosition() == mReadBuffer->getCapacity()) {
	 if (mReadBuffer->getCapacity() * 2 > MAX_BUF_SIZE) {
	 throw BufferOverflowException();
	 }
	 Log::d("ddms", "Expanding read buffer to "
	 + mReadBuffer->getCapacity() * 2);

	 ByteBuffer newBuffer =
	 ByteBuffer.allocate(mReadBuffer.capacity() * 2);
	 mReadBuffer.position(0);
	 newBuffer.put(mReadBuffer);     // leaves "position" at end

	 mReadBuffer = newBuffer;
	 }
	 */
	unsigned char buf[MAX_BUF_SIZE];
	count = mChannel->receiveBytes(buf, MAX_BUF_SIZE);
	for (int i = 0; i < count; ++i)
		mReadBuffer->put(buf[i]);
	Log::v("ddms", "Read " + Poco::NumberFormatter::format(count) + " bytes from " + toString());
	if (count < 0)
		throw Poco::IOException("read failed");
}

std::tr1::shared_ptr<JdwpPacket> Debugger::getJdwpPacket() {
	/*
	 * On entry, the data starts at offset 0 and ends at "position".
	 * "limit" is set to the buffer capacity.
	 */
	if (mConnState == ST_AWAIT_SHAKE) {
		unsigned int result;

		result = JdwpPacket::findHandshake(mReadBuffer);
		//Log.v("ddms", "findHand: " + result);
		if (result == JdwpPacket::HANDSHAKE_GOOD) {
			Log::d("ddms", "Good handshake from debugger");
			JdwpPacket::consumeHandshake(mReadBuffer);
			sendHandshake();
			mConnState = ST_READY;

			std::tr1::shared_ptr<ClientData> cd = mClient.lock()->getClientData();
			cd->setDebuggerInterest(DebuggerStatusATTACHED);
			mClient.lock()->update(Client::CHANGE_DEBUGGER_STATUS);

			// see if we have another packet in the buffer
			return getJdwpPacket();
		} else if (result == JdwpPacket::HANDSHAKE_BAD) {
			// not a debugger, throw an exception so we drop the line
			Log::d("ddms", "Bad handshake from debugger");
			throw Poco::IOException("bad handshake");
		} else if (result == JdwpPacket::HANDSHAKE_NOTYET) {

		} else {
			Log::e("ddms", "Unknown packet while waiting for client handshake");
		}
		return std::tr1::shared_ptr<JdwpPacket>();
	} else if (mConnState == ST_READY) {
		if (mReadBuffer->getPosition() != 0) {
			Log::v("ddms", "Checking " + Poco::NumberFormatter::format(mReadBuffer->getPosition()) + " bytes");
		}
		return JdwpPacket::findPacket(mReadBuffer);
	} else {
		Log::e("ddms", "Receiving data in state = " + Poco::NumberFormatter::format(mConnState));
	}

	return std::tr1::shared_ptr<JdwpPacket>();
}

void Debugger::forwardPacketToClient(std::tr1::shared_ptr<JdwpPacket> packet) {
	mClient.lock()->sendAndConsume(packet);
}

void Debugger::sendHandshake() {
	std::tr1::shared_ptr<ByteBuffer> tempBuffer(new ByteBuffer(JdwpPacket::HANDSHAKE_LEN));
	JdwpPacket::putHandshake(tempBuffer);
	int expectedLength = tempBuffer->getPosition();
	tempBuffer->flip();
	if (mChannel->sendBytes(tempBuffer->getArray(), expectedLength) != expectedLength) {
		throw Poco::IOException("partial handshake write");
	}
	tempBuffer->setPosition(expectedLength);

	expectedLength = mPreDataBuffer->getPosition();
	if (expectedLength > 0) {
		Log::d("ddms", "Sending " + Poco::NumberFormatter::format(mPreDataBuffer->getPosition()) + " bytes of saved data");
		mPreDataBuffer->flip();
		if (mChannel->sendBytes(mPreDataBuffer->getArray(), expectedLength) != expectedLength) {
			throw Poco::IOException("partial pre-data write");
		}
		mPreDataBuffer->clear();
	}
}

void Debugger::sendAndConsume(std::tr1::shared_ptr<JdwpPacket> packet) {
	Poco::ScopedLock<Poco::Mutex> lock(mLock);
	if (mChannel == nullptr) {
		/*
		 * Buffer this up so we can send it to the debugger when it
		 * finally does connect.  This is essential because the VM_START
		 * message might be telling the debugger that the VM is
		 * suspended.  The alternative approach would be for us to
		 * capture and interpret VM_START and send it later if we
		 * didn't choose to un-suspend the VM for our own purposes.
		 */
		Log::d("ddms", "Saving packet 0x" + Poco::NumberFormatter::formatHex(packet->getId()));
		packet->movePacket(mPreDataBuffer);
	} else {
		packet->writeAndConsume(mChannel);
	}
}

void Debugger::processDebuggerActivity(const Poco::AutoPtr<Poco::Net::ReadableNotification> & event) {
	accept();
	Log::v("ddms", "processDebuggerActivity in" + toString());
	if (mChannel != nullptr) {
//		AndroidDebugBridge::getReactor().addEventHandler(*mChannel.get(),
//				Poco::NObserver<Debugger, Poco::Net::ReadableNotification>(*this, &Debugger::processDebuggerData));
	} else {
		Log::w("ddms", "ignoring duplicate debugger");
		// new connection already closed
	}
}

Debugger::~Debugger() {
	if (mListenChannel != nullptr) {
		AndroidDebugBridge::getReactor().removeEventHandler(*mListenChannel.get(),
				Poco::NObserver<Debugger, Poco::Net::ReadableNotification>(*this, &Debugger::processDebuggerActivity));
	}
	if (mChannel != nullptr) {
		AndroidDebugBridge::getReactor().removeEventHandler(*mChannel.get(),
				Poco::NObserver<Debugger, Poco::Net::ReadableNotification>(*this, &Debugger::processDebuggerData));
	}
	Log::v("ddms", "Debugger " + toString() + " is destroyed");
}

void Debugger::processDebuggerData(const Poco::AutoPtr<Poco::Net::ReadableNotification> & event) {
	try {
		/*
		 * Read pending data.
		 */
		read();

		/*
		 * See if we have a full packet in the buffer. It's possible we have
		 * more than one packet, so we have to loop.
		 */
		std::tr1::shared_ptr<JdwpPacket> packet = getJdwpPacket();
		while (packet != nullptr) {
			Log::v("ddms",
					"Forwarding dbg req 0x" + Poco::NumberFormatter::formatHex(packet->getId()) + " to "
							+ getClient()->toString());

			forwardPacketToClient(packet);

			packet = getJdwpPacket();
		}
	} catch (Poco::IOException &ioe) {
		/*
		 * Close data connection; automatically un-registers dbg from
		 * selector. The failure could be caused by the debugger going away,
		 * or by the client going away and failing to accept our data.
		 * Either way, the debugger connection does not need to exist any
		 * longer. We also need to recycle the connection to the client, so
		 * that the VM sees the debugger disconnect. For a DDM-aware client
		 * this won't be necessary, and we can just send a "debugger
		 * disconnected" message.
		 */
		Log::d("ddms", "Closing connection to debugger " + toString());
		closeData();
		std::tr1::shared_ptr<Client> client = getClient();
		if (client->isDdmAware()) {
			// TODO: soft-disconnect DDM-aware clients
			Log::d("ddms", " (recycling client connection as well)");

			// we should drop the client, but also attempt to reopen it.
			// This is done by the DeviceMonitor.
			client->getDevice()->getMonitor()->addClientToDropAndReopen(client, IDebugPortProvider::NO_STATIC_PORT);
		} else {
			Log::d("ddms", " (recycling client connection as well)");
			// we should drop the client, but also attempt to reopen it.
			// This is done by the DeviceMonitor.
			client->getDevice()->getMonitor()->addClientToDropAndReopen(client, IDebugPortProvider::NO_STATIC_PORT);
		}
	}
}

} /* namespace ddmlib */

