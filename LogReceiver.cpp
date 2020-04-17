/*
 * LogReceiver.cpp
 *
 *  Created on: 03.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "LogReceiver.hpp"
#include "ArrayHelper.hpp"

namespace ddmlib {

LogReceiver::LogReceiver(ILogListener *listener) {
	Init();
	m_pListener = listener;
}

LogReceiver::~LogReceiver() {
}

void LogReceiver::Init() {

	mEntryHeaderOffset = 0;
	mEntryDataOffset = 0;
	mIsCancelled = false;
	m_pCurrentEntry.reset();

}

void LogReceiver::parseNewData(unsigned char* data, int offset, int length) {
	// notify the listener of new raw data
	if (m_pListener != nullptr) {
		m_pListener->newData(data, offset, length);
	}
	int init_length = length;
	// loop while there is still data to be read and the receiver has not be cancelled.
	while (length > 0 && mIsCancelled == false) {
		// first check if we have no current entry.
		if (m_pCurrentEntry == nullptr) {
			m_pCurrentEntry = std::tr1::shared_ptr<LogEntry>(new LogEntry);
			if (mEntryHeaderOffset + length < ENTRY_HEADER_SIZE) {
				// if we don't have enough data to finish the header, save
				// the data we have and return
				//System.arraycopy(data, offset, mEntryHeaderBuffer, mEntryHeaderOffset, length);
				std::copy(data + offset, data + length + offset, mEntryHeaderBuffer + mEntryHeaderOffset);
				mEntryHeaderOffset += length;
				return;
			} else {
				// we have enough to fill the header, let's do it.
				// did we store some part at the beginning of the header?
				if (mEntryHeaderOffset != 0) {
					// copy the rest of the entry header into the header buffer
					int size = ENTRY_HEADER_SIZE - mEntryHeaderOffset;

					std::copy(data + offset, data + size + offset, mEntryHeaderBuffer + mEntryHeaderOffset);
					// create the entry from the header buffer
					m_pCurrentEntry = createEntry(mEntryHeaderBuffer, 0, ENTRY_HEADER_SIZE);

					// since we used the whole entry header buffer, we reset  the offset
					mEntryHeaderOffset = 0;

					// adjust current offset and remaining length to the beginning
					// of the entry data
					offset += size;
					length -= size;
				} else {
					// create the entry directly from the data array
					//Sergey: init_length because here is needed to pass as argument initial length of the incoming data
					m_pCurrentEntry = createEntry(data, offset, init_length);

					// adjust current offset and remaining length to the beginning
					// of the entry data
					offset += ENTRY_HEADER_SIZE;
					length -= ENTRY_HEADER_SIZE;
				}
			}
		}

		// at this point, we have an entry, and offset/length have been updated to skip
		// the entry header.

		// if we have enough data for this entry or more, we'll need to end this entry
		if (length >= m_pCurrentEntry->len - mEntryDataOffset) {
			// compute and save the size of the data that we have to read for this entry,
			// based on how much we may already have read.
			int dataSize = m_pCurrentEntry->len - mEntryDataOffset;

			// we only read what we need, and put it in the entry buffer.
			std::copy(data + offset, data + dataSize + offset, m_pCurrentEntry->data.begin() + mEntryDataOffset);
			// notify the listener of a new entry
			if (m_pListener != nullptr) {
				m_pListener->newEntry(m_pCurrentEntry);
			}

			// reset some flags: we have read 0 data of the current entry.
			// and we have no current entry being read.
			mEntryDataOffset = 0;
			m_pCurrentEntry.reset();

			// and update the data buffer info to the end of the current entry / start
			// of the next one.
			offset += dataSize;
			length -= dataSize;
		} else {
			// we don't have enough data to fill this entry, so we store what we have
			// in the entry itself.
			std::copy(data + offset, data + length + offset, m_pCurrentEntry->data.begin() + mEntryDataOffset);
			// save the amount read for the data.
			mEntryDataOffset += length;
			return;
		}
	}
}
/**
 * Creates a {@link LogEntry} from the array of bytes. This expects the data buffer size
 * to be at least <code>offset + {@link #ENTRY_HEADER_SIZE}</code>.
 * @param data the data buffer the entry is read from.
 * @param offset the offset of the first byte from the buffer representing the entry.
 * @return a new {@link LogEntry} or <code>null</code> if some error happened.
 */
std::tr1::shared_ptr<LogEntry> LogReceiver::createEntry(unsigned char* data, int offset, int length) {
	if (length < offset + ENTRY_HEADER_SIZE) {
		throw Poco::InvalidArgumentException("Buffer not big enough to hold full LoggerEntry header");
	}

	// create the new entry and fill it.
	std::tr1::shared_ptr<LogEntry> entry(new LogEntry);

	entry->len = ArrayHelper::swapU16bitFromArray(data, offset);

	// we've read only 16 bits, but since there's also a 16 bit padding,
	// we can skip right over both.
	offset += 4;

	entry->pid = ArrayHelper::swap32bitFromArray(data, offset);
	offset += 4;
	entry->tid = ArrayHelper::swap32bitFromArray(data, offset);
	offset += 4;
	entry->sec = ArrayHelper::swap32bitFromArray(data, offset);
	offset += 4;
	entry->nsec = ArrayHelper::swap32bitFromArray(data, offset);
	offset += 4;

	// allocate the data
	entry->data.resize(entry->len);

	return entry;
}

} /* namespace ddmlib */
