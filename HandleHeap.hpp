/*
 * HandleHeap.hpp
 *
 *  Created on: Feb 10, 2012
 *      Author: sergey bulavintsev
 */

#ifndef HANDLEHEAP_HPP_
#define HANDLEHEAP_HPP_
#include "ddmlib.hpp"

#include "ChunkHandler.hpp"

class ByteBuffer;

namespace ddmlib {

class Client;

class DDMLIB_LOCAL HandleHeap: public ChunkHandler {

public:
	static int CHUNK_HPIF; // = type("HPIF");
	static int CHUNK_HPST; // = type("HPST");
	static int CHUNK_HPEN; // = type("HPEN");
	static int CHUNK_HPSG; // = type("HPSG");
	static int CHUNK_HPGC; // = type("HPGC");
	static int CHUNK_HPDU; // = type("HPDU");
	static int CHUNK_HPDS; // = type("HPDS");
	static int CHUNK_REAE; // = type("REAE");
	static int CHUNK_REAQ; // = type("REAQ");
	static int CHUNK_REAL; // = type("REAL");

	// args to sendHPSG
	static const int WHEN_DISABLE;
	static const int WHEN_GC;
	static const int WHAT_MERGE; // merge adjacent objects
	static const int WHAT_OBJ; // keep objects distinct

	// args to sendHPIF
	static const int HPIF_WHEN_NEVER;
	static const int HPIF_WHEN_NOW;
	static const int HPIF_WHEN_NEXT_GC;
	static const int HPIF_WHEN_EVERY_GC;

	HandleHeap();

	static void registerInReactor();
	void clientReady(std::tr1::shared_ptr<Client> client);
	void clientDisconnected(std::tr1::shared_ptr<Client> client) {
	}
	;
	void handleChunk(std::tr1::shared_ptr<Client> client, int type, std::tr1::shared_ptr<ByteBuffer> data, bool isReply, int msgId);
    /**
     * Send an HPIF (HeaP InFo) request to the client.
     */
	static void sendHPIF(std::tr1::shared_ptr<Client> client, int when);
    /**
     * Sends an HPSG (HeaP SeGment) request to the client.
     */
	static void sendHPSG(std::tr1::shared_ptr<Client> client, int when, int what);
    /**
     * Sends an HPGC request to the client.
     */
	static void sendHPGC(std::tr1::shared_ptr<Client> client);

	/**
	 * Sends an HPDU request to the client.
	 *
	 * We will get an HPDU response when the heap dump has completed.  On
	 * failure we get a generic failure response.
	 *
	 * @param fileName name of output file (on device)
	 */
	static void sendHPDU(std::tr1::shared_ptr<Client> client, const std::wstring& fileName);

	/**
	 * Sends an HPDS request to the client.
	 *
	 * We will get an HPDS response when the heap dump has completed.  On
	 * failure we get a generic failure response.
	 *
	 * This is more expensive for the device than HPDU, because the entire
	 * heap dump is held in RAM instead of spooled out to a temp file.  On
	 * the other hand, permission to write to /sdcard is not required.
	 *
	 * @param fileName name of output file (on device)
	 */
	static void sendHPDS(std::tr1::shared_ptr<Client> client);
    /**
     * Sends a REAE (REcent Allocation Enable) request to the client.
     */
	static void sendREAE(std::tr1::shared_ptr<Client> client, bool enable);
    /**
     * Sends a REAQ (REcent Allocation Query) request to the client.
     */
	static void sendREAQ(std::tr1::shared_ptr<Client> client);
    /**
     * Sends a REAL (REcent ALlocation) request to the client.
     */
	static void sendREAL(std::tr1::shared_ptr<Client> client);

	virtual ~HandleHeap();

private:
    /*
     * Handle a heap segment series start message.
     */
	void handleHPST(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
    /*
     * Handle a heap segment series end message.
     */
	void handleHPEN(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
    /*
     * Handle a heap info message.
     */
	void handleHPIF(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
    /*
     * Handle a heap segment message.
     */
	void handleHPSG(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
    /*
     * Handle notification of completion of a HeaP DUmp.
     */
	void handleHPDU(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
    /*
     * Handle HeaP Dump Streaming response.  "data" contains the full
     * hprof dump.
     */
	void handleHPDS(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);
    /*
     * Handle the response from our REcent Allocation Query message.
     */
	void handleREAQ(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);

    /**
     * Converts a VM class descriptor string ("Landroid/os/Debug;") to
     * a dot-notation class name ("android.os.Debug").
     */
	std::wstring descriptorToDot(std::wstring& str);

    /**
     * Reads a string table out of "data".
     *
     * This is just a serial collection of strings, each of which is a
     * four-byte length followed by UTF-16 data.
     */
	void readStringTable(std::tr1::shared_ptr<ByteBuffer> data, std::vector<std::wstring>& strings);

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
	void handleREAL(std::tr1::shared_ptr<Client> client, std::tr1::shared_ptr<ByteBuffer> data);

	static std::tr1::shared_ptr<HandleHeap> mInst;
};

} /* namespace ddmlib */
#endif /* HANDLEHEAP_HPP_ */
