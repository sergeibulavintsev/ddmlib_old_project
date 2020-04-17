/*
 * HandleNativeHeap.cpp
 *
 *  Created on: Feb 13, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleNativeHeap.hpp"
#include "Log.hpp"
#include "Client.hpp"
#include "ClientData.hpp"
#include "JdwpPacket.hpp"
#include "ClientData.hpp"
#include "NativeAllocationInfo.hpp"
#include "HandleHeap.hpp"
#include "JdwpPacket.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

int HandleNativeHeap::CHUNK_NHGT = type("NHGT");
int HandleNativeHeap::CHUNK_NHSG = type("NHSG");
int HandleNativeHeap::CHUNK_NHST = type("NHST");
int HandleNativeHeap::CHUNK_NHEN = type("NHEN");

std::tr1::shared_ptr<HandleNativeHeap> HandleNativeHeap::mInst(new HandleNativeHeap());

HandleNativeHeap::HandleNativeHeap() {
}

HandleNativeHeap::~HandleNativeHeap() {
}

void HandleNativeHeap::registerInReactor() {
	registerChunkHandler(CHUNK_NHGT, mInst);
	registerChunkHandler(CHUNK_NHSG, mInst);
	registerChunkHandler(CHUNK_NHST, mInst);
	registerChunkHandler(CHUNK_NHEN, mInst);
}

void HandleNativeHeap::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {

	Log::d("ddm-nativeheap", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_NHGT) {
		handleNHGT(client, data);
	} else if (type == CHUNK_NHST) {
		// start chunk before any NHSG chunk(s)
		client->getClientData()->getNativeHeapData()->clearHeapData();
	} else if (type == CHUNK_NHEN) {
		// end chunk after NHSG chunk(s)
		client->getClientData()->getNativeHeapData()->sealHeapData();
	} else if (type == CHUNK_NHSG) {
		handleNHSG(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}

	client->update(Client::CHANGE_NATIVE_HEAP_DATA);
}

void HandleNativeHeap::sendNHGT(std::tr1::shared_ptr<Client> client) {

	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data in request message

	finishChunkPacket(packet, CHUNK_NHGT, buf->getPosition());
	Log::d("ddm-nativeheap", "Sending " + name(CHUNK_NHGT));
	client->sendAndConsume(packet, mInst);

	rawBuf = allocBuffer(2);
	packet.reset(new JdwpPacket(rawBuf));
	buf = getChunkDataBuf(rawBuf);

	buf->put((byte) HandleHeap::WHEN_DISABLE);
	buf->put((byte) HandleHeap::WHAT_OBJ);

	finishChunkPacket(packet, CHUNK_NHSG, buf->getPosition());
	Log::d("ddm-nativeheap", "Sending " + name(CHUNK_NHSG));
	client->sendAndConsume(packet, mInst);
}

void HandleNativeHeap::handleNHGT(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	std::tr1::shared_ptr<ClientData> cd = client->getClientData();

	Log::d("ddm-nativeheap", "NHGT: " + Poco::NumberFormatter::format(data->getLimit()) + " bytes");

	// TODO - process incoming data and save in "cd"
	// clear the previous run
	cd->clearNativeAllocationInfo();
	std::tr1::shared_ptr<ByteBuffer> buffer(data->clone());

	buffer->setSwapEndianness(false);

//        read the header
//        typedef struct Header {
//            uint32_t mapSize;
//            uint32_t allocSize;
//            uint32_t allocInfoSize;
//            uint32_t totalMemory;
//              uint32_t backtraceSize;
//        };

	int mapSize = buffer->getInt();
	int allocSize = buffer->getInt();
	int allocInfoSize = buffer->getInt();
	int totalMemory = buffer->getInt();
	int backtraceSize = buffer->getInt();

	Log::d("ddms", "mapSize: " + Poco::NumberFormatter::format(mapSize));
	Log::d("ddms", "allocSize: " + Poco::NumberFormatter::format(allocSize));
	Log::d("ddms", "allocInfoSize: " + Poco::NumberFormatter::format(allocInfoSize));
	Log::d("ddms", "totalMemory: " + Poco::NumberFormatter::format(totalMemory));

	cd->setTotalNativeMemory(totalMemory);

	// this means that updates aren't turned on.
	if (allocInfoSize == 0)
		return;

	if (mapSize > 0) {
		std::vector< unsigned char > maps(mapSize);
		std::copy(buffer->getArray(), buffer->getArray() + mapSize, maps.begin());
		parseMaps(cd, maps);
	}

	int iterations = allocSize / allocInfoSize;

	for (int i = 0; i < iterations; i++) {
		NativeAllocationInfo info(buffer->getInt() /* size */, buffer->getInt() /* allocations */);

		for (int j = 0; j < backtraceSize; j++) {
			long long addr = (static_cast<long long>(buffer->getInt())) & 0x00000000ffffffffL;

			if (addr == 0x0) {
				// skip past null addresses
				continue;
			}

			info.addStackCallAddress(addr);

		}

		cd->addNativeAllocation(info);
	}
}

void HandleNativeHeap::handleNHSG(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	data->rewind();
	client->getClientData()->getNativeHeapData()->addHeapData(data);

	if (true) {
		return;
	}

	// WORK IN PROGRESS

//        Log::e("ddm-nativeheap", "NHSG: ----------------------------------");
//        Log::e("ddm-nativeheap", "NHSG: " + data.limit() + " bytes");


	std::tr1::shared_ptr<ByteBuffer> buffer(data->clone());
	buffer->setSwapEndianness(true);

	int id = buffer->getInt();
	int unitsize = buffer->get();
	long startAddress = buffer->getInt() & 0x00000000ffffffffL;
	int offset = buffer->getInt();
	int allocationUnitCount = buffer->getInt();

//	Log::e("ddm-nativeheap", "id: " + Poco::NumberFormatter::format(id));
//	Log::e("ddm-nativeheap", "unitsize: " + Poco::NumberFormatter::format(unitsize));
//	Log::e("ddm-nativeheap", "startAddress: 0x" + Poco::NumberFormatter::formatHex(startAddress));
//	Log::e("ddm-nativeheap", "offset: " + Poco::NumberFormatter::format(offset));
//	Log::e("ddm-nativeheap", "allocationUnitCount: " + Poco::NumberFormatter::format(allocationUnitCount));
//	Log::e("ddm-nativeheap", "end: 0x" + Poco::NumberFormatter::formatHex(startAddress + unitsize * allocationUnitCount));

	// read the usage
	while (buffer->getPosition() < buffer->getLimit()) {
		int eState = buffer->get() & 0x000000ff;
		int eLen = (buffer->get() & 0x000000ff) + 1;
//		Log::e("ddm-nativeheap", "solidity: " + Poco::NumberFormatter::format(eState & 0x7) + " - kind: "
//		        + Poco::NumberFormatter::format((eState >> 3) & 0x7) + " - len: " + Poco::NumberFormatter::format(eLen));
	}

//        count += unitsize * allocationUnitCount;
//        Log::e("ddm-nativeheap", "count = " + count);

}

void HandleNativeHeap::parseMaps(std::tr1::shared_ptr<ClientData> cd, std::vector< unsigned char > maps) {
	std::stringstream input;
	std::string line;
	for (unsigned int ix = 0; ix < maps.size(); ++ix)
		input << maps[ix];
	try {

		// most libraries are defined on several lines, so we need to make sure we parse
		// all the library lines and only add the library at the end
		unsigned long long startAddr = 0;
		unsigned long long endAddr = 0;
		std::string library;

		while (std::getline(input, line)) {
			Log::d("ddms", "line: " + line);
			if (line.length() < 16) {
				continue;
			}

			try {
				unsigned long long tmpStart = Poco::NumberParser::parseHex64(line.substr(0, 8));
				unsigned long long tmpEnd = Poco::NumberParser::parseHex64(line.substr(9, 17));

				size_t index = line.find_first_of('/');

				if (index == std::string::npos)
					continue;

				std::string tmpLib = line.substr(index);

				if (library.empty() || (!library.empty() && library != tmpLib)) {

					if (!library.empty()) {
						cd->addNativeLibraryMapInfo(startAddr, endAddr, library);
						Log::d("ddms",
								library + "(" + Poco::NumberFormatter::formatHex(startAddr) + " - "
										+ Poco::NumberFormatter::formatHex(endAddr) + ")");
					}

					// now init the new library
					library = tmpLib;
					startAddr = tmpStart;
					endAddr = tmpEnd;
				} else {
					// add the new end
					endAddr = tmpEnd;
				}
			} catch (Poco::SyntaxException& e) {
				//std::cerr << e.what() << std::endl;
			}
		}

		if (!library.empty()) {
			cd->addNativeLibraryMapInfo(startAddr, endAddr, library);
			Log::d("ddms",
					library + "(" + Poco::NumberFormatter::formatHex(startAddr) + " - "
							+ Poco::NumberFormatter::formatHex(endAddr) + ")");
		}
	} catch (Poco::IOException& e) {
		//std::cerr << e.what() << std::endl;
	}
}

} /* namespace ddmlib */
