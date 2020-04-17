/*
 * SyncException.h
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef SYNCEXCEPTION_HPP_
#define SYNCEXCEPTION_HPP_
#include "ddmlib.hpp"

#include "CanceledException.hpp"
namespace ddmlib {

class DDMLIB_API SyncException: public CanceledException {
public:

	class SyncError {
	public:
		SyncError(const std::string &msg) :
				message(msg) {
			vect_ptr.push_back(this);
		}

		SyncError(const std::runtime_error &e) {
			message = e.what();
			vect_ptr.push_back(this);
		}
		std::string what() const {
			return message;
		}
	private:
		std::string message;
		static std::vector<SyncError*> vect_ptr;
	};

	SyncError* getErrorCode() {
		return pError;
	}

	static SyncError CANCELED;
	static SyncError TRANSFER_PROTOCOL_ERROR;
	static SyncError NO_REMOTE_OBJECT;
	static SyncError TARGET_IS_FILE;
	static SyncError NO_DIR_TARGET;
	static SyncError REMOTE_PATH_ENCODING;
	static SyncError REMOTE_PATH_LENGTH;
	static SyncError FILE_READ_ERROR;
	static SyncError FILE_WRITE_ERROR;
	static SyncError LOCAL_IS_DIRECTORY;
	static SyncError NO_LOCAL_FILE;
	static SyncError REMOTE_IS_FILE;
	static SyncError BUFFER_OVERRUN;

	bool wasCanceled() const {
		return (pError == &CANCELED);
	}
	SyncException(SyncError& error);
	SyncException(SyncError& error, std::string& msg);
	SyncException(SyncError& error, std::runtime_error& e);
private:
	SyncError* pError;

};

} /* namespace ddmlib */
#endif /* SYNCEXCEPTION_H_ */
