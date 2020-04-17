/*
 * SyncException.cpp
 *
 *  Created on: 02.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "SyncException.hpp"

namespace ddmlib {

std::vector<SyncException::SyncError*> SyncException::SyncError::vect_ptr;

SyncException::SyncError SyncException::CANCELED("Operation was canceled by the user."),
		SyncException::TRANSFER_PROTOCOL_ERROR("Adb Transfer Protocol Error."), SyncException::NO_REMOTE_OBJECT(
				"Remote object doesn't exist!"), SyncException::TARGET_IS_FILE("Target object is a file."),
		SyncException::NO_DIR_TARGET("Target directory doesn't exist."), SyncException::REMOTE_PATH_ENCODING(
				"Remote Path encoding is not supported."), SyncException::REMOTE_PATH_LENGTH("Remote path is too long."),
		SyncException::FILE_READ_ERROR("Reading local file failed!"), SyncException::FILE_WRITE_ERROR(
				"Writing local file failed!"), SyncException::LOCAL_IS_DIRECTORY("Local path is a directory."),
		SyncException::NO_LOCAL_FILE("Local path doesn't exist."), SyncException::REMOTE_IS_FILE("Remote path is a file."),
		SyncException::BUFFER_OVERRUN("Receiving too much data.");

SyncException::SyncException(SyncError &error) :
		CanceledException(error.what()) {
	pError = &error;
}

SyncException::SyncException(SyncError &error, std::string &msg) :
		CanceledException(msg) {
	pError = &error;
}

SyncException::SyncException(SyncError& error, std::runtime_error& e) :
		CanceledException(error.what()) {
	pError = &error;
}

} /* namespace ddmlib */
