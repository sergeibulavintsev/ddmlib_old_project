/*
 * AdbHelper.cpp
 *
 *  Created on: 06.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "AdbHelper.hpp"
#include "AdbCommandRejectedException.hpp"
#include "ShellCommandUnresponsiveException.hpp"
#include "LogReceiver.hpp"
#include "ByteBuffer.hpp"
#include "Device.hpp"
#include "DdmPreferences.hpp"
#include "IShellOutputReceiver.hpp"
#include "Log.hpp"
#include "RawImage.hpp"
#include "AndroidDebugBridge.hpp"

namespace ddmlib {

const std::string AdbHelper::DEFAULT_ENCODING = "ISO-8859-1";

AdbHelper::AdbHelper() {
}

AdbHelper::~AdbHelper() {
}

std::tr1::shared_ptr<Poco::Net::StreamSocket> AdbHelper::open(const Poco::Net::SocketAddress& adbSockAddr,
		std::tr1::shared_ptr<Device> device, int devicePort) {

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));

	try {
		adbChan->setNoDelay(true);

		// if the device is not -1, then we first tell adb we're looking to
		// talk to a specific device
		setDevice(adbChan, device.get());

		std::vector<unsigned char> req = createAdbForwardRequest("", devicePort);
		// Log::hexDump(req);

		write(adbChan, req);

		AdbResponse resp = readAdbResponse(adbChan, false);
		if (resp.okay == false) {
			throw AdbCommandRejectedException(resp.message);
		}

	} catch (Poco::TimeoutException& e) {
		Log::d("ddms", "Socket timeout in Adbhelper open.");
		adbChan->close();
		throw;
	} catch (Poco::IOException& e) {
		Log::d("ddms", "IO Exception in Adbhelper open.");
		adbChan->close();
		throw;
	}

	return adbChan;
}

std::tr1::shared_ptr<Poco::Net::StreamSocket> AdbHelper::createPassThroughConnection(const Poco::Net::SocketAddress& adbSockAddr,
		std::tr1::shared_ptr<Device> device, int pid) {

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));
	try {
		adbChan->setNoDelay(true);
		// if the device is not -1, then we first tell adb we're looking to
		// talk to a specific device
		setDevice(adbChan, device.get());

		std::vector<unsigned char> req = createJdwpForwardRequest(pid);
		// Log.hexDump(req);

		write(adbChan, req);

		AdbResponse resp = readAdbResponse(adbChan, false /* readDiagString */);
		if (resp.okay == false) {
			throw AdbCommandRejectedException(resp.message);
		}

	} catch (Poco::TimeoutException& e) {
		adbChan->close();
		throw;
	} catch (Poco::IOException& e) {
		adbChan->close();
		throw;
	}

	return adbChan;
}

std::vector<unsigned char> AdbHelper::createAdbForwardRequest(const std::string& addrStr, int port) {

	std::string reqStr;

	if (addrStr.empty())
		reqStr = "tcp:" + Poco::NumberFormatter::format(port);
	else
		reqStr = "tcp:" + Poco::NumberFormatter::format(port) + ":" + addrStr;

	return formAdbRequest(reqStr);
}

std::vector<unsigned char> AdbHelper::createJdwpForwardRequest(int pid) {

	std::string reqStr = "jdwp:" + Poco::NumberFormatter::format(pid); 
	return formAdbRequest(reqStr);
}

std::vector<unsigned char> AdbHelper::formAdbRequest(const std::string &req) {

	//Java version: String.format("%04X%s", req.length(), req); 
	std::string resultStr = Poco::NumberFormatter::formatHex(req.length(), 4) + req;
	std::vector<unsigned char> result(resultStr.length());

	std::copy(resultStr.begin(), resultStr.end(), result.begin());

	assert(result.size() == req.length() + 4);

	return result;
}

AdbHelper::AdbResponse AdbHelper::readAdbResponse(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, bool readDiagString) {

	AdbResponse resp;

	std::vector<unsigned char> reply(4);
	read(chan, reply);

	if (isOkay(reply)) {
		resp.okay = true;
	} else {
		readDiagString = true; // look for a reason after the FAIL
		resp.okay = false;
	}

	// not a loop -- use "while" so we can use "break"
	try {
		while (readDiagString) {
			// length string is in next 4 bytes
			std::vector<unsigned char> lenBuf(4);
			read(chan, lenBuf);

			std::string lenStr = replyToString(lenBuf);

			int len = 0;
			try {
				len = Poco::NumberParser::parseHex(lenStr);
			} catch (Poco::SyntaxException& nfe) {
				Log::w("ddms",
						"Expected digits, got '" + lenStr + "': " + Poco::NumberFormatter::format(lenBuf[0]) + " "
								+ Poco::NumberFormatter::format(lenBuf[1]) + " " + Poco::NumberFormatter::format(lenBuf[2])
								+ " " + Poco::NumberFormatter::format(lenBuf[3]));
				Log::w("ddms", "reply was " + replyToString(reply));
				break;
			}

			std::vector<unsigned char> msg(len);
			read(chan, msg);

			resp.message = replyToString(msg);
			Log::v("ddms", "Got reply '" + replyToString(reply) + "', diag='" + resp.message + "'");

			break;
		}
	} catch (std::exception& e) {
		// ignore those, since it's just reading the diagnose string, the response will
		// contain okay==false anyway.
	}

	return resp;
}

std::tr1::shared_ptr<RawImage> AdbHelper::getFrameBuffer(const Poco::Net::SocketAddress& adbSockAddr,
		std::tr1::shared_ptr<Device> device) {

	std::tr1::shared_ptr<RawImage> imageParams(new RawImage());
	std::vector<unsigned char> request = formAdbRequest("framebuffer:"); 
	std::vector<unsigned char> nudge(1,0);
	std::vector<unsigned char> reply;

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));

	// if the device is not -1, then we first tell adb we're looking to talk
	// to a specific device
	setDevice(adbChan, device.get());

	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, false/*readDiagString*/);
	if (resp.okay == false) {
		if (adbChan != nullptr) {
			adbChan->close();
		}
		throw AdbCommandRejectedException(resp.message);
	}

	// first the protocol version.
	reply.resize(4);

	read(adbChan, reply);

	unsigned char temp[4];
	std::copy(reply.begin(), reply.end(), temp);

	std::tr1::shared_ptr<ByteBuffer> buf(ByteBuffer::wrap(temp, 4));
	buf->setSwapEndianness(false);

	int version = buf->getInt();

	// get the header size (this is a count of int)
	int headerSize = RawImage::getHeaderSize(version);

	// read the header
	reply.resize(headerSize * 4);

	read(adbChan, reply);

	unsigned char *temp2 = new unsigned char[headerSize * 4];
	std::copy(reply.begin(), reply.end(), temp2);

	buf = std::tr1::shared_ptr<ByteBuffer>(ByteBuffer::wrap(temp2, headerSize * 4));
	buf->setSwapEndianness(false);

	// fill the RawImage with the header
	if (imageParams->readHeader(version, buf) == false) {

		if (adbChan != nullptr) {
			adbChan->close();
		}
		Log::e("Screenshot", "Unsupported protocol: " + Poco::NumberFormatter::format(version));
		return std::tr1::shared_ptr<RawImage>();
	}
	delete []temp2;

	Log::d("ddms",
			"image params: bpp=" + Poco::NumberFormatter::format(imageParams->bpp) + ", size="
					+ Poco::NumberFormatter::format(imageParams->size) + ", width="
					+ Poco::NumberFormatter::format(imageParams->width) + ", height="
					+ Poco::NumberFormatter::format(imageParams->height));

	write(adbChan, nudge);

	imageParams->data.resize(imageParams->size);
	read(adbChan, imageParams->data);

	if (adbChan != nullptr) {
		adbChan->close();
	}
	return imageParams;
}

void AdbHelper::executeRemoteCommand(const Poco::Net::SocketAddress& adbSockAddr, const std::string& command,
		std::tr1::shared_ptr<Device> device, std::tr1::shared_ptr<IShellOutputReceiver> rcvr, int maxTimeToOutputResponse) {
	executeRemoteCommand(adbSockAddr, command, device.get(), rcvr.get(), maxTimeToOutputResponse);
}

void AdbHelper::executeRemoteCommand(const Poco::Net::SocketAddress& adbSockAddr, const std::string& command,
		Device *device, IShellOutputReceiver *rcvr, int maxTimeToOutputResponse) {
	Log::v("ddms", "execute: running " + command);

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));

	Poco::Timespan::TimeDiff timeout(maxTimeToOutputResponse * 1000);
	adbChan->setReceiveTimeout(timeout);
	//adbChan->setReceiveBufferSize(16*1024*1024); // 16MB buffer to avoid overflows
	// if the device is not -1, then we first tell adb we're looking to
	// talk
	// to a specific device
	setDevice(adbChan, device);

	std::vector<unsigned char> request = formAdbRequest("shell:" + command); 
	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, false /* readDiagString */);
	if (resp.okay == false) {
		if (adbChan != nullptr) {
			adbChan->close();
		}
		Log::v("ddms", "execute: returning");
		Log::e("ddms", "ADB rejected shell command (" + command + "): " + resp.message);
		throw AdbCommandRejectedException(resp.message);
	}

	int length = 16384;
	unsigned char buf[16384];
	int waitCount = 0;
	adbChan->setReceiveTimeout(Poco::Timespan(WAIT_TIME));
	while (true) {
		int count = 0;

		if (rcvr != nullptr && rcvr->isCancelled()) {
			break;
		}
		try {
			waitCount += WAIT_TIME;
			if (waitCount > maxTimeToOutputResponse) {
				Log::v("ddms", "execute: returning due to timeout exceed");
				break;
			}
			count = adbChan->receiveBytes(buf, length);

			if (count > 0) {
							// send data to receiver if present
							if (rcvr != nullptr) {
								rcvr->addOutput(buf, 0, count);
							}
							waitCount = 0;
			} else if (count < 0) {
				if (rcvr != nullptr) {
					rcvr->flush();
				}
                Log::v("ddms", "execute '" + command + "' on '" + device->toString() + "' : EOF hit. Read: "
                        + Poco::NumberFormatter::format(count));
                break;
			} else {
				// graceful shutdown
				if (rcvr != nullptr) {
					rcvr->flush();
				}
				break;
			}
		} catch (Poco::TimeoutException& e) {
			//Log::v("ddms", "execute: returning due to timeout exceed");
			//break;
			Poco::Thread::sleep(WAIT_TIME);
		} catch (Poco::Net::NetException& e) {
			Log::v("ddms", "execute: returning due to socket error");
			break;
		} catch (Poco::Exception& e) {
			Log::e("ddms" , std::string("execute: caught exception: ") + e.what());
			break;
		};
	}
	if (adbChan != nullptr) {
		//Log::v("ddms","executeRemoteCommand finished, closing channel");
		adbChan->close();
	}
	if (rcvr != nullptr && rcvr->isCancelled()) {
		Log::v("ddms", "execute: cancelled");
	}
}

void AdbHelper::runEventLogService(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device,
		std::tr1::shared_ptr<LogReceiver> rcvr) {
	runEventLogService(adbSockAddr, device.get(), rcvr.get());
}

void AdbHelper::runEventLogService(const Poco::Net::SocketAddress& adbSockAddr, Device *device, LogReceiver *rcvr) {
	runLogService(adbSockAddr, device, "events", rcvr);
}

void AdbHelper::runLogService(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device,
		const std::string& logName, std::tr1::shared_ptr<LogReceiver> rcvr) {
	runLogService(adbSockAddr, device.get(), logName, rcvr.get());
}

void AdbHelper::runLogService(const Poco::Net::SocketAddress& adbSockAddr, Device *device,
		const std::string& logName, LogReceiver *rcvr) {

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));

	// if the device is not -1, then we first tell adb we're looking to talk
	// to a specific device
	setDevice(adbChan, device);

	std::vector<unsigned char> request = formAdbRequest("log:" + logName);
	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, false /* readDiagString */);
	if (resp.okay == false) {
		if (adbChan != nullptr) {
			adbChan->close();
		}
		throw AdbCommandRejectedException(resp.message);
	}

	const int length = 16384;
	unsigned char buf[length];
	while (true) {
		int count = 0;

		if (rcvr == nullptr || rcvr->isCancelled()) {
			Log::v("ddms", "Log service cancelled");
			break;
		}
		try {
			count = adbChan->receiveBytes(buf, length);
			if (count > 0) {
							// send data to receiver if present
							if (rcvr != nullptr) {
								rcvr->parseNewData(buf, 0, count);
							}
			} else if (count < 0) {
                Log::v("ddms", "Log service '" + logName + "' on '" + device->toString() + "' : EOF hit. Read: "
                        + Poco::NumberFormatter::format(count));
                break;
			} else {
				// graceful shutdown
				break;
			}
		} catch (Poco::Net::NetException& e) {
			break;
		} catch (Poco::TimeoutException& e) {
			Poco::Thread::sleep(WAIT_TIME);
			//continue;
		} catch (std::exception& e) {
			Log::e("ddms", e.what());
		}
	}
	if (adbChan != nullptr) {
		adbChan->close();
	}
}

void AdbHelper::createForward(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device, int localPort,
		int remotePort) {

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));

	std::vector<unsigned char> request = formAdbRequest(
			"host-serial:" + device->getSerialNumber() + ":forward:tcp:" + Poco::NumberFormatter::format(localPort)
					+ ";tcp:" + Poco::NumberFormatter::format(remotePort));

	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, false /* readDiagString */);
	if (resp.okay == false) {
		if (adbChan != nullptr) {
			adbChan->close();
		}
		Log::w("create-forward", "Error creating forward: " + resp.message);
		throw AdbCommandRejectedException(resp.message);
	}
}

void AdbHelper::removeForward(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device, int localPort,
		int remotePort) {

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));

	std::vector<unsigned char> request = formAdbRequest(
			"host-serial:" + device->getSerialNumber() + ":killforward:tcp:" + Poco::NumberFormatter::format(localPort)
					+ ";tcp:" + Poco::NumberFormatter::format(remotePort));

	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, false /* readDiagString */);
	if (resp.okay == false) {
		if (adbChan != nullptr) {
			adbChan->close();
		}
		Log::w("remove-forward", "Error creating forward: " + resp.message);
		throw AdbCommandRejectedException(resp.message);
	}
}

bool AdbHelper::isOkay(std::vector<unsigned char>& reply) {
	return (reply[0] == 'O' && reply[1] == 'K' && reply[2] == 'A' && reply[3] == 'Y');
}

std::string AdbHelper::replyToString(std::vector<unsigned char>& reply) {
	std::string result;
	result.resize(reply.size());
	std::copy(reply.begin(), reply.end(), result.begin());
	return result;
}

void AdbHelper::read(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, std::vector<unsigned char>& data) {
	read(chan, data, -1, DdmPreferences::getTimeOut());
}

void AdbHelper::read(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, std::vector<unsigned char>& data, int length,
		int timeout) {

	if (length == -1)
		length = data.size();
	unsigned char *temp = new unsigned char[length];

	chan->setReceiveTimeout(Poco::Timespan(timeout * 1000));
	int count = 0;
	int received = 0;

	try {
		do {
			count = chan->receiveBytes(temp + received, length - received);

//			if (count == 0) {
//				Log::d("ddms", "read: channel EOF");
//				throw Poco::IOException("EOF");
//			}

			received += count;
			Poco::Thread::sleep(1);
		} while (received < length);
	} catch (Poco::TimeoutException& e) {
		Log::d("ddms", "read: timeout");
	} catch (...) {
		delete []temp;
		throw;
	}
	if (count > 0)
		std::copy(temp, temp + length, data.begin());
	delete []temp;
}

void AdbHelper::write(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, const std::vector<unsigned char>& data) {
	write(chan, data, -1, DdmPreferences::getTimeOut());
}

void AdbHelper::write(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, const std::vector<unsigned char>& data, int length,
		int timeout) {

	if (length == -1)
		length = data.size();

	unsigned char *temp = new unsigned char[length];
	std::copy(data.begin(), data.begin() + length, temp);
	chan->setSendTimeout(Poco::Timespan(timeout * 1000));
	int count = 0;

	try {
		count = chan->sendBytes(temp, length);
		if (count < 0) {
			Log::d("ddms", "write: channel EOF");
			throw Poco::IOException("channel EOF");
		}
	} catch (Poco::TimeoutException& e) {
		Log::d("ddms", "write: timeout");
	} catch (...) {
		delete []temp;
		throw;
	}
	delete []temp;

}

void AdbHelper::setDevice(std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan, Device *device) {
	// if the device is not -1, then we first tell adb we're looking to talk
	// to a specific device
	if (device != nullptr) {
		std::string msg = "host:transport:" + device->getSerialNumber(); 
		std::vector<unsigned char> device_query = formAdbRequest(msg);

		write(adbChan, device_query);

		AdbResponse resp = readAdbResponse(adbChan, false /* readDiagString */);
		if (resp.okay == false) {
			throw AdbCommandRejectedException(resp.message, true/*errorDuringDeviceSelection*/);
		}
	}
}

void AdbHelper::reboot(const std::string &into, const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device) {
	std::vector<unsigned char> request;
	if (into.empty()) {
		request = formAdbRequest("reboot:"); 
	} else {
		request = formAdbRequest("reboot:" + into); 
	}

	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(adbSockAddr));

	// if the device is not -1, then we first tell adb we're looking to talk
	// to a specific device
	setDevice(adbChan, device.get());

	write(adbChan, request);

	if (adbChan != nullptr) {
		adbChan->close();
	}
}

void AdbHelper::restartInTcpip(std::tr1::shared_ptr<Device> device, int port) {
	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(AndroidDebugBridge::getSocketAddress()));

	std::vector<unsigned char> request = formAdbRequest("tcpip:" + Poco::NumberFormatter::format(port));
	setDevice(adbChan, device.get());
	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, true /* readDiagString */);
	if (resp.okay == false) {
		throw AdbCommandRejectedException(resp.message, true);
	}
	if (adbChan != nullptr) {
		adbChan->close();
	}
}

void AdbHelper::restartInUSB(std::tr1::shared_ptr<Device> device) {
	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(AndroidDebugBridge::getSocketAddress()));

	std::vector<unsigned char> request = formAdbRequest("usb:");
	setDevice(adbChan, device.get());
	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, true /* readDiagString */);
	if (resp.okay == false) {
		throw AdbCommandRejectedException(resp.message, true);
	}
	if (adbChan != nullptr) {
		adbChan->close();
	}
}

std::string AdbHelper::connectToNetworkDevice(const std::string &address) {
	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(AndroidDebugBridge::getSocketAddress()));

	std::vector<unsigned char> request = formAdbRequest("host:connect:" + address);
	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, true /* readDiagString */);
	if (resp.okay == false) {
		throw AdbCommandRejectedException(resp.message, true);
	}
	if (adbChan != nullptr) {
		adbChan->close();
	}
	return resp.message;
}

std::string AdbHelper::disconnectFromNetworkDevice(const std::string &address) {
	std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan(new Poco::Net::StreamSocket(AndroidDebugBridge::getSocketAddress()));

	std::vector<unsigned char> request = formAdbRequest("host:disconnect:" + address);
	write(adbChan, request);

	AdbResponse resp = readAdbResponse(adbChan, true /* readDiagString */);
	if (resp.okay == false) {
		throw AdbCommandRejectedException(resp.message, true);
	}
	if (adbChan != nullptr) {
		adbChan->close();
	}
	return resp.message;
}

} /* namespace ddmlib */
