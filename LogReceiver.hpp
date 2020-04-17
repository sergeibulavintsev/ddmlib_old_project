/*
 * LogReceiver.hpp
 *
 *  Created on: 03.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef LOGRECEIVER_HPP_
#define LOGRECEIVER_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

struct LogEntry;

/**
 * Represents a log entry and its raw data.
 */
struct DDMLIB_API LogEntry {
	/*
	 * See //device/include/utils/logger.h
	 */
	/** 16bit unsigned: length of the payload. */
	int len; /* This is normally followed by a 16 bit padding */
	/** pid of the process that generated this {@link LogEntry} */
	int pid;
	/** tid of the process that generated this {@link LogEntry} */
	int tid;
	/** Seconds since epoch. */
	int sec;
	/** nanoseconds. */
	int nsec;
	/** The entry's raw data. */
	std::vector<unsigned char> data;
};

/**
 * Classes which implement this interface provide a method that deals
 * with {@link LogEntry} objects coming from log service through a {@link LogReceiver}.
 * <p/>This interface provides two methods.
 * <ul>
 * <li>{@link #newEntry(com.android.ddmlib.log.LogReceiver.LogEntry)} provides a
 * first level of parsing, extracting {@link LogEntry} objects out of the log service output.</li>
 * <li>{@link #newData(byte[], int, int)} provides a way to receive the raw information
 * coming directly from the log service.</li>
 * </ul>
 */
class DDMLIB_API ILogListener {
public:
	/**
	 * Sent when a new {@link LogEntry} has been parsed by the {@link LogReceiver}.
	 * @param entry the new log entry.
	 */
	virtual void newEntry(std::tr1::shared_ptr<LogEntry> entry) = 0;

	/**
	 * Sent when new raw data is coming from the log service.
	 * @param data the raw data buffer.
	 * @param offset the offset into the buffer signaling the beginning of the new data.
	 * @param length the length of the new data.
	 */
	virtual void newData(unsigned char* data, int offset, int length) = 0;
};

class DDMLIB_API LogReceiver {
public:

	~LogReceiver();


	LogReceiver(ILogListener *listener);
	/**
	 * Parses new data coming from the log service.
	 * @param data the data buffer
	 * @param offset the offset into the buffer signaling the beginning of the new data.
	 * @param length the length of the new data.
	 */
	void parseNewData(unsigned char* data, int offset, int length);

	/**
	 * Returns whether this receiver is canceling the remote service.
	 */
	bool isCancelled() {
		return mIsCancelled;
	}

	/**
	 * Cancels the current remote service.
	 */
	void cancel() {
		mIsCancelled = true;
	}
	void Init();

private:
	/**
	 * Creates a {@link LogEntry} from the array of bytes. This expects the data buffer size
	 * to be at least <code>offset + {@link #ENTRY_HEADER_SIZE}</code>.
	 * @param data the data buffer the entry is read from.
	 * @param offset the offset of the first byte from the buffer representing the entry.
	 * @return a new {@link LogEntry} or <code>null</code> if some error happened.
	 */
	std::tr1::shared_ptr<LogEntry> createEntry(unsigned char* data, int offset, int length);
	const static int ENTRY_HEADER_SIZE = 20; // 2*2 + 4*4; see LogEntry.
	std::tr1::shared_ptr<LogEntry> m_pCurrentEntry;

	/** Temp buffer to store partial entry headers. */
	unsigned char mEntryHeaderBuffer[ENTRY_HEADER_SIZE];
	/** Offset in the partial header buffer */
	int mEntryHeaderOffset;
	/** Offset in the partial entry data */
	int mEntryDataOffset;
	bool mIsCancelled;

	/** Listener waiting for receive fully read {@link LogEntry} objects */
	ILogListener *m_pListener;

};

} /* namespace ddmlib */
#endif /* LOGRECEIVER_HPP_ */
