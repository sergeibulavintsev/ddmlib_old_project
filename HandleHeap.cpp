/*
 * HandleHeap.cpp
 *
 *  Created on: Feb 10, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "HandleHeap.hpp"
#include "Client.hpp"
#include "ClientData.hpp"
#include "Log.hpp"
#include "StackTraceElement.hpp"
#include "JdwpPacket.hpp"
#include "AllocationInfo.hpp"
#include "ByteBuffer.hpp"

namespace ddmlib {

int HandleHeap::CHUNK_HPIF = type("HPIF");
int HandleHeap::CHUNK_HPST = type("HPST");
int HandleHeap::CHUNK_HPEN = type("HPEN");
int HandleHeap::CHUNK_HPSG = type("HPSG");
int HandleHeap::CHUNK_HPGC = type("HPGC");
int HandleHeap::CHUNK_HPDU = type("HPDU");
int HandleHeap::CHUNK_HPDS = type("HPDS");
int HandleHeap::CHUNK_REAE = type("REAE");
int HandleHeap::CHUNK_REAQ = type("REAQ");
int HandleHeap::CHUNK_REAL = type("REAL");

std::tr1::shared_ptr<HandleHeap> HandleHeap::mInst(new HandleHeap);

// args to sendHPSG
const int HandleHeap::WHEN_DISABLE = 0;
const int HandleHeap::WHEN_GC = 1;
const int HandleHeap::WHAT_MERGE = 0; // merge adjacent objects
const int HandleHeap::WHAT_OBJ = 1; // keep objects distinct

// args to sendHPIF
const int HandleHeap::HPIF_WHEN_NEVER = 0;
const int HandleHeap::HPIF_WHEN_NOW = 1;
const int HandleHeap::HPIF_WHEN_NEXT_GC = 2;
const int HandleHeap::HPIF_WHEN_EVERY_GC = 3;

HandleHeap::HandleHeap() {
}

HandleHeap::~HandleHeap() {
}

void HandleHeap::registerInReactor() {
	registerChunkHandler(CHUNK_HPIF, mInst);
	registerChunkHandler(CHUNK_HPST, mInst);
	registerChunkHandler(CHUNK_HPEN, mInst);
	registerChunkHandler(CHUNK_HPSG, mInst);
	registerChunkHandler(CHUNK_HPDS, mInst);
	registerChunkHandler(CHUNK_REAQ, mInst);
	registerChunkHandler(CHUNK_REAL, mInst);
}

void HandleHeap::clientReady(std::tr1::shared_ptr<Client> client) {
	if (client->isHeapUpdateEnabled()) {
		//sendHPSG(client, WHEN_GC, WHAT_MERGE);
		sendHPIF(client, HPIF_WHEN_EVERY_GC);
	}
}

void HandleHeap::handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply,
		int msgId) {
	Log::d("ddm-heap", "handling " + ChunkHandler::name(type));

	if (type == CHUNK_HPIF) {
		handleHPIF(client, data);
	} else if (type == CHUNK_HPST) {
		handleHPST(client, data);
	} else if (type == CHUNK_HPEN) {
		handleHPEN(client, data);
	} else if (type == CHUNK_HPSG) {
		handleHPSG(client, data);
	} else if (type == CHUNK_HPDU) {
		handleHPDU(client, data);
	} else if (type == CHUNK_HPDS) {
		handleHPDS(client, data);
	} else if (type == CHUNK_REAQ) {
		handleREAQ(client, data);
	} else if (type == CHUNK_REAL) {
		handleREAL(client, data);
	} else {
		handleUnknownChunk(client, type, data, isReply, msgId);
	}

}

void HandleHeap::handleHPIF(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	Log::d("ddm-heap", "HPIF!");
	try {
		int numHeaps = data->getInt();

		for (int i = 0; i < numHeaps; i++) {
			int heapId = data->getInt();
			long long timeStamp = data->getLong();
			byte reason = data->get();
			long long maxHeapSize = (long long) data->getInt() & 0x00ffffffff;
			long long heapSize = (long long) data->getInt() & 0x00ffffffff;
			long long bytesAllocated = (long long) data->getInt() & 0x00ffffffff;
			long long objectsAllocated = (long long) data->getInt() & 0x00ffffffff;

			client->getClientData()->setHeapInfo(heapId, maxHeapSize, heapSize, bytesAllocated, objectsAllocated);
			client->update(Client::CHANGE_HEAP_DATA);
		}
	} catch (std::underflow_error& ex) {
		Log::w("ddm-heap", "malformed HPIF chunk from client");
	}
}

void HandleHeap::sendHPIF(std::tr1::shared_ptr<Client> client, int when) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(1);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	buf->put((byte) when);

	finishChunkPacket(packet, CHUNK_HPIF, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_HPIF) + ": when=" + Poco::NumberFormatter::format(when));
	client->sendAndConsume(packet, mInst);
}

void HandleHeap::handleHPST(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	/* Clear out any data that's sitting around to
	 * get ready for the chunks that are about to come.
	 */
//xxx todo: only clear data that belongs to the heap mentioned in <data>.
	client->getClientData()->getVmHeapData()->clearHeapData();
}

void HandleHeap::handleHPEN(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	/* Let the UI know that we've received all of the
	 * data for this heap.
	 */
//xxx todo: only seal data that belongs to the heap mentioned in <data>.
	client->getClientData()->getVmHeapData()->sealHeapData();
	client->update(Client::CHANGE_HEAP_DATA);
}

void HandleHeap::handleHPSG(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	data->rewind();
	std::tr1::shared_ptr<ByteBuffer> dataCopy(data->clone());
	client->getClientData()->getVmHeapData()->addHeapData(dataCopy);
//xxx todo: add to the heap mentioned in <data>
}

void HandleHeap::sendHPSG(std::tr1::shared_ptr<Client> client, int when, int what) {

	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(2);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	buf->put((byte) when);
	buf->put((byte) what);

	finishChunkPacket(packet, CHUNK_HPSG, buf->getPosition());
	Log::d("ddm-heap",
			"Sending " + name(CHUNK_HPSG) + ": when=" + Poco::NumberFormatter::format(when) + ", what="
					+ Poco::NumberFormatter::format(what));
	client->sendAndConsume(packet, mInst);
}

void HandleHeap::sendHPGC(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data

	finishChunkPacket(packet, CHUNK_HPGC, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_HPGC));
	client->sendAndConsume(packet, mInst);
}

void HandleHeap::sendHPDU(std::tr1::shared_ptr<Client> client, const std::wstring& fileName) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(4 + fileName.length() * 2);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	buf->setSwapEndianness(true);
	buf->putInt(fileName.length());
	putString(buf, fileName);

	finishChunkPacket(packet, CHUNK_HPDU, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_HPDU) + " '" + Log::convertUtf16ToUtf8(fileName) + "'");
	client->sendAndConsume(packet, mInst);
	client->getClientData()->setPendingHprofDump(fileName);
}

void HandleHeap::sendHPDS(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	finishChunkPacket(packet, CHUNK_HPDS, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_HPDS));
	client->sendAndConsume(packet, mInst);
}

void HandleHeap::handleHPDU(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	unsigned char result;

	// get the filename and make the client not have pending HPROF dump anymore.
	std::wstring filename = client->getClientData()->getPendingHprofDump();
	client->getClientData()->setPendingHprofDump(std::wstring(L""));

	// get the dump result
	result = data->get();

	// get the app-level handler for HPROF dump
	std::tr1::shared_ptr<IHprofDumpHandler> handler = ClientData::getHprofDumpHandler();
	if (handler != nullptr) {
		if (result == 0) {
			handler->onSuccess(filename, client);

			Log::d("ddm-heap", "Heap dump request has finished");
		} else {
			handler->onEndFailure(client, std::string(""));
			Log::w("ddm-heap", "Heap dump request failed (check device log)");
		}
	}
}

void HandleHeap::handleHPDS(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	std::tr1::shared_ptr<IHprofDumpHandler> handler = ClientData::getHprofDumpHandler();
	if (handler != nullptr) {
		unsigned char *stuff = new unsigned char[data->getCapacity()];
		try {
			std::copy(data->getArray(), data->getArray() + data->getCapacity(), stuff);

			Log::d("ddm-hprof", "got hprof file, size: " + Poco::NumberFormatter::format(data->getCapacity()) + " bytes");
			handler->onSuccess(stuff, data->getCapacity(), client);
		} catch (...) {
		}
		delete []stuff;
	}
}

void HandleHeap::sendREAE(std::tr1::shared_ptr<Client> client, bool enable) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(1);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	buf->put((byte) (enable ? 1 : 0));

	finishChunkPacket(packet, CHUNK_REAE, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_REAE) + ": " + Poco::NumberFormatter::format(enable));
	client->sendAndConsume(packet, mInst);
}

void HandleHeap::sendREAQ(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data

	finishChunkPacket(packet, CHUNK_REAQ, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_REAQ));
	client->sendAndConsume(packet, mInst);
}

void HandleHeap::sendREAL(std::tr1::shared_ptr<Client> client) {
	std::tr1::shared_ptr<ByteBuffer> rawBuf = allocBuffer(0);
	std::tr1::shared_ptr<JdwpPacket> packet(new JdwpPacket(rawBuf));
	std::tr1::shared_ptr<ByteBuffer> buf = getChunkDataBuf(rawBuf);

	// no data

	finishChunkPacket(packet, CHUNK_REAL, buf->getPosition());
	Log::d("ddm-heap", "Sending " + name(CHUNK_REAL));
	client->sendAndConsume(packet, mInst);
}

void HandleHeap::handleREAQ(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	bool enabled;
	enabled = (data->get() != 0);
	std::stringstream ss;
	ss << std::boolalpha << enabled;
	Log::d("ddm-heap", "REAQ says: enabled=" + ss.str());

	client->getClientData()->setAllocationStatus(enabled ? AllocationTrackingStatusON : AllocationTrackingStatusOFF);
	client->update(Client::CHANGE_HEAP_ALLOCATION_STATUS);
}

std::wstring HandleHeap::descriptorToDot(std::wstring& str) {
	// count the number of arrays.
	int array = 0;
	while (str.find(L"[") == 0) {
		str = str.substr(1);
		array++;
	}

	int len = str.length();

	/* strip off leading 'L' and trailing ';' if appropriate */
	if (len >= 2 && str.at(0) == L'L' && str.at(len - 1) == L';') {
		str = str.substr(1, len - 1);

		std::replace(str.begin(), str.end(), L'/', L'.');
	} else {
		// convert the basic types
		if (str == L"C") {
			str = L"char";
		} else if (str == L"B") {
			str = L"byte";
		} else if (str == L"Z") {
			str = L"boolean";
		} else if (str == L"S") {
			str = L"short";
		} else if (str == L"I") {
			str = L"int";
		} else if (str == L"J") {
			str = L"long";
		} else if (str == L"F") {
			str = L"float";
		} else if (str == L"D") {
			str = L"double";
		}
	}

	// now add the array part
	for (int a = 0; a < array; a++) {
		str = str + L"[]";
	}

	return str;
}

void HandleHeap::readStringTable(std::tr1::shared_ptr<ByteBuffer> data, std::vector<std::wstring>& strings) {
	int count = strings.size();
	int i;

	for (i = 0; i < count; i++) {
		int nameLen = data->getInt();
		std::wstring descriptor = getString(data, nameLen);
		strings[i] = descriptorToDot(descriptor);
	}
}

/*
 * Handle a REcent ALlocation response.
 *
 * Message header (all values big-endian):
 *   (1b) message header len (to allow future expansion); includes itself
 *   (1b) entry header len
 *   (1b) stack frame len
 *   (2b) number of entries
 *   (4b) offset to string table from start of message
 *   (2b) number of class name strings
 *   (2b) number of method name strings
 *   (2b) number of source file name strings
 *   For each entry:
 *     (4b) total allocation size
 *     (2b) threadId
 *     (2b) allocated object's class name index
 *     (1b) stack depth
 *     For each stack frame:
 *       (2b) method's class name
 *       (2b) method name
 *       (2b) method source file
 *       (2b) line number, clipped to 32767; -2 if native; -1 if no source
 *   (xb) class name strings
 *   (xb) method name strings
 *   (xb) source file strings
 *
 *   As with other DDM traffic, strings are sent as a 4-byte length
 *   followed by UTF-16 data.
 */
void HandleHeap::handleREAL(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data) {
	Log::e("ddm-heap", "*** Received " + name(CHUNK_REAL));
	int messageHdrLen, entryHdrLen, stackFrameLen;
	short int numEntries;
	int offsetToStrings;
	short int numClassNames, numMethodNames, numFileNames;

	/*
	 * Read the header.
	 */
	messageHdrLen = (data->get() & 0xff);
	entryHdrLen = (data->get() & 0xff);
	stackFrameLen = (data->get() & 0xff);
	numEntries = (data->getShort() & 0xffff);
	offsetToStrings = data->getInt();
	numClassNames = (data->getShort() & 0xffff);
	numMethodNames = (data->getShort() & 0xffff);
	numFileNames = (data->getShort() & 0xffff);

	/*
	 * Skip forward to the strings and read them.
	 */
	data->setPosition(offsetToStrings);

	std::vector<std::wstring> classNames(numClassNames);
	std::vector<std::wstring> methodNames(numMethodNames);
	std::vector<std::wstring> fileNames(numFileNames);

	readStringTable(data, classNames);
	readStringTable(data, methodNames);
	//System.out.println("METHODS: "
	//    + java.util.Arrays.deepToString(methodNames));
	readStringTable(data, fileNames);

	/*
	 * Skip back to a point just past the header and start reading
	 * entries.
	 */
	data->setPosition(messageHdrLen);

	std::vector<std::tr1::shared_ptr<AllocationInfo> > list(numEntries);
	int allocNumber = numEntries; // order value for the entry. This is sent in reverse order.
	for (int i = 0; i < numEntries; i++) {
		int totalSize;
		short int threadId, classNameIndex;
		unsigned char stackDepth;

		totalSize = data->getInt();
		threadId = (data->getShort() & 0xffff);
		classNameIndex = (data->getShort() & 0xffff);
		stackDepth = (data->get() & 0xff);
		/* we've consumed 9 bytes; gobble up any extra */
		for (int skip = 9; skip < entryHdrLen; skip++)
			data->get();

		std::vector<std::tr1::shared_ptr<StackTraceElement> > steArray(stackDepth);

		/*
		 * Pull out the stack trace.
		 */
		for (int sti = 0; sti < stackDepth; sti++) {
			int methodClassNameIndex, methodNameIndex;
			int methodSourceFileIndex;
			short int lineNumber;
			std::wstring methodClassName, methodName, methodSourceFile;

			methodClassNameIndex = (data->getShort() & 0xffff);
			methodNameIndex = (data->getShort() & 0xffff);
			methodSourceFileIndex = (data->getShort() & 0xffff);
			lineNumber = data->getShort();

			methodClassName = classNames[methodClassNameIndex];
			methodName = methodNames[methodNameIndex];
			methodSourceFile = fileNames[methodSourceFileIndex];

			steArray[sti] = std::tr1::shared_ptr<StackTraceElement>(new StackTraceElement(methodClassName, methodName, methodSourceFile, lineNumber));

			/* we've consumed 8 bytes; gobble up any extra */
			for (int skip = 9; skip < stackFrameLen; skip++)
				data->get();
		}

		list.push_back(
				std::tr1::shared_ptr<AllocationInfo>(new AllocationInfo(allocNumber--, classNames[classNameIndex], totalSize, (short) threadId,
						steArray)));
	}

	client->getClientData()->setAllocations(list);
	client->update(Client::CHANGE_HEAP_ALLOCATIONS);
}

} /* namespace ddmlib */
