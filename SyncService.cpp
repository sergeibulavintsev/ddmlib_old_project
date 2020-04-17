/*
 * SyncService.cpp
 *
 *  Created on: Feb 22, 2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "SyncService.hpp"
#include "AdbHelper.hpp"
#include "Log.hpp"
#include "ArrayHelper.hpp"
#include "FileListingService.hpp"
#include "SyncException.hpp"

namespace ddmlib {

unsigned char SyncService::ID_OKAY[] = { 'O', 'K', 'A', 'Y' };
unsigned char SyncService::ID_FAIL[] = { 'F', 'A', 'I', 'L' };
unsigned char SyncService::ID_STAT[] = { 'S', 'T', 'A', 'T' };
unsigned char SyncService::ID_RECV[] = { 'R', 'E', 'C', 'V' };
unsigned char SyncService::ID_DATA[] = { 'D', 'A', 'T', 'A' };
unsigned char SyncService::ID_DONE[] = { 'D', 'O', 'N', 'E' };
unsigned char SyncService::ID_SEND[] = { 'S', 'E', 'N', 'D' };
unsigned char SyncService::ID_LIST[] = { 'L', 'I', 'S', 'T' };
unsigned char SyncService::ID_DENT[] = { 'D', 'E', 'N', 'T' };

const unsigned int SyncService::S_ISOCK = 0xC000; // type: symbolic link
const unsigned int SyncService::S_IFLNK = 0xA000; // type: symbolic link
const unsigned int SyncService::S_IFREG = 0x8000; // type: regular file
const unsigned int SyncService::S_IFBLK = 0x6000; // type: block device
const unsigned int SyncService::S_IFDIR = 0x4000; // type: directory
const unsigned int SyncService::S_IFCHR = 0x2000; // type: character device
const unsigned int SyncService::S_IFIFO = 0x1000; // type: fifo

/*
 const unsigned int S_ISUID = 0x0800; // set-uid bit
 const unsigned int S_ISGID = 0x0400; // set-gid bit
 const unsigned int S_ISVTX = 0x0200; // sticky bit
 const unsigned int S_IRWXU = 0x01C0; // user permissions
 const unsigned int S_IRUSR = 0x0100; // user: read
 const unsigned int S_IWUSR = 0x0080; // user: write
 const unsigned int S_IXUSR = 0x0040; // user: execute
 const unsigned int S_IRWXG = 0x0038; // group permissions
 const unsigned int S_IRGRP = 0x0020; // group: read
 const unsigned int S_IWGRP = 0x0010; // group: write
 const unsigned int S_IXGRP = 0x0008; // group: execute
 const unsigned int S_IRWXO = 0x0007; // other permissions
 const unsigned int S_IROTH = 0x0004; // other: read
 const unsigned int S_IWOTH = 0x0002; // other: write
 const unsigned int S_IXOTH = 0x0001; // other: execute
 */

std::tr1::shared_ptr<NullSyncProgressMonitor> SyncService::sNullSyncProgressMonitor(new NullSyncProgressMonitor);

SyncService::SyncService(const Poco::Net::SocketAddress& address, std::tr1::shared_ptr<Device> device) {
	mAddress = address;
	mDevice = device;
}

SyncService::~SyncService() {
}

bool SyncService::openSync() {
	try {
		mChannel = std::tr1::shared_ptr<Poco::Net::StreamSocket>(new Poco::Net::StreamSocket(mAddress));

		// target a specific device
		AdbHelper::setDevice(mChannel, mDevice.get());

		std::vector<unsigned char> request = AdbHelper::formAdbRequest("sync:");
		AdbHelper::write(mChannel, request, -1, DdmPreferences::getTimeOut());

		AdbHelper::AdbResponse resp = AdbHelper::readAdbResponse(mChannel, false /* readDiagString */);

		if (resp.okay == false) {
			Log::w("ddms", "Got unhappy response from ADB sync req: " + resp.message);
			mChannel->close();
			mChannel.reset();
			return false;
		}
	} catch (Poco::TimeoutException& e) {
		if (mChannel != nullptr) {
			try {
				Log::w("ddms", "Timed out on open sync. Closing socket.");
				mChannel->close();
			} catch (Poco::IOException& e2) {
				// we want to throw the original exception, so we ignore this one.
			}
			mChannel.reset();
		}

		throw;
	} catch (Poco::IOException& e) {
		if (mChannel != nullptr) {
			try {
				Log::w("ddms", "IO exception on open sync. Closing socket.");
				mChannel->close();
			} catch (Poco::IOException& e2) {
				// we want to throw the original exception, so we ignore this one.
			}
			mChannel.reset();
		}

		throw;
	}

	return true;
}

std::tr1::shared_ptr<ISyncProgressMonitor> SyncService::getNullProgressMonitor() {
	return sNullSyncProgressMonitor;
}

void SyncService::pull(const std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >& entries,
		const std::string& localPath, std::tr1::shared_ptr<ISyncProgressMonitor> monitor) {
	// first we check the destination is a directory and exists
	pull(entries, localPath, monitor.get());
}

void SyncService::pull(const std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >& entries, const std::string& localPath,
			ISyncProgressMonitor* monitor){
	// first we check the destination is a directory and exists
	Poco::File f(localPath);
	if (f.exists() == false) {
		throw SyncException(SyncException::NO_DIR_TARGET);
	}
	if (f.isDirectory() == false) {
		throw SyncException(SyncException::TARGET_IS_FILE);
	}

	// get a FileListingService object
	std::tr1::shared_ptr<FileListingService> fls(new FileListingService(mDevice));

	// compute the number of file to move
	int total = getTotalRemoteFileSize(entries, fls);

	// start the monitor
	monitor->start(total);

	doPull(entries, localPath, fls, monitor);

	monitor->stop();
}

void SyncService::pullFile(std::tr1::shared_ptr<FileListingService::FileEntry> remote, const std::string& localFilename,
		std::tr1::shared_ptr<ISyncProgressMonitor> monitor) {
	pullFile(remote, localFilename, monitor.get());
}


void SyncService::pullFile(std::tr1::shared_ptr<FileListingService::FileEntry> remote, const std::string& localFilename,
		ISyncProgressMonitor* monitor) {
	int total = remote->getSizeValue();
	monitor->start(total);

	doPullFile(remote->getFullPath(), localFilename, monitor);

	monitor->stop();
}

void SyncService::pullFile(const std::string& remoteFilepath, const std::string& localFilename,
		std::tr1::shared_ptr< ISyncProgressMonitor > monitor) {
	pullFile(remoteFilepath, localFilename, monitor.get());
}

void SyncService::pullFile(const std::string& remoteFilepath, const std::string& localFilename,
		ISyncProgressMonitor* monitor) {
	int mode = readMode(remoteFilepath);
	if (mode == -1) {
		// attempts to download anyway
	} else if (mode == 0) {
		throw SyncException(SyncException::NO_REMOTE_OBJECT);
	}

	monitor->start(0);
	//TODO: use the {@link FileListingService} to get the file size.

	doPullFile(remoteFilepath, localFilename, monitor);

	monitor->stop();
}

void SyncService::push(const std::vector<std::string>& local, std::tr1::shared_ptr<FileListingService::FileEntry> remote,
		std::tr1::shared_ptr<ISyncProgressMonitor> monitor){
	push(local, remote, monitor.get());
}

void SyncService::push(const std::vector<std::string>& local, std::tr1::shared_ptr<FileListingService::FileEntry> remote,
		ISyncProgressMonitor* monitor) {
	if (remote->isDirectory() == false) {
		throw SyncException(SyncException::REMOTE_IS_FILE);
	}

	// make a list of File from the list of String
	std::vector<Poco::File> files;
	for (std::vector<std::string>::const_iterator path = local.begin(); path != local.end(); ++path) {
		files.push_back(Poco::File(*path));
	}

	// get the total count of the bytes to transfer
	unsigned long long total = getTotalLocalFileSize(files);

	monitor->start(total);

	doPush(files, remote->getFullPath(), monitor);

	monitor->stop();
}

void SyncService::pushFile(const std::string& local, const std::string& remote,
		std::tr1::shared_ptr<ISyncProgressMonitor> monitor){
	pushFile(local, remote, monitor.get());
}

void SyncService::pushFile(const std::string& local, const std::string& remote,
		ISyncProgressMonitor* monitor) {
	std::tr1::shared_ptr<Poco::File> f(new Poco::File(local));
	if (f->exists() == false) {
		throw SyncException(SyncException::NO_LOCAL_FILE);
	}

	if (f->isDirectory()) {
		throw SyncException(SyncException::LOCAL_IS_DIRECTORY);
	}

	monitor->start((int) f->getSize());

	doPushFile(local, remote, monitor);

	monitor->stop();
}

int SyncService::getTotalRemoteFileSize(const std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >& entries,
		std::tr1::shared_ptr<FileListingService> fls) {
	int count = 0;
	for (std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >::const_iterator e = entries.begin(); e != entries.end(); ++e) {
		int type = (*e)->getType();
		if (type == FileListingService::TYPE_DIRECTORY) {
			// get the children
			std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> > children = 
				fls->getChildren(*e, false, std::tr1::shared_ptr<FileListingService::IListingReceiver>());
			count += getTotalRemoteFileSize(children, fls) + 1;
		} else if (type == FileListingService::TYPE_FILE) {
			count += (*e)->getSizeValue();
		}
	}

	return count;
}

unsigned long long SyncService::getTotalLocalFileSize(const std::vector<Poco::File>& files) {
	unsigned long long count = 0;

	for (std::vector<Poco::File>::const_iterator f = files.begin(); f != files.end(); ++f) {
		if ((*f).exists()) {
			if ((*f).isDirectory()) {
				std::vector<Poco::File> fl;
				(*f).list(fl);
				return getTotalLocalFileSize(fl) + 1;
			} else if ((*f).isFile()) {
				count += (*f).getSize();
			}
		}
	}

	return count;
}

void SyncService::doPull(std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> > entries, const std::string& localPath,
		std::tr1::shared_ptr<FileListingService> fileListingService, ISyncProgressMonitor* monitor) {

	for (std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >::iterator e = entries.begin(); e != entries.end(); ++e) {
		// check if we're cancelled
		if (monitor->isCanceled() == true) {
			throw SyncException(SyncException::CANCELED);
		}

		// get type (we only pull directory and files for now)
		int type = (*e)->getType();
		if (type == FileListingService::TYPE_DIRECTORY) {
			monitor->startSubTask((*e)->getFullPath());

			std::string dest = localPath + Poco::Path::separator() + (*e)->getName();

			// make the directory
			Poco::File d(dest);
			d.createDirectory();

			// then recursively call the content. Since we did a ls command
			// to get the number of files, we can use the cache
			std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> > children = fileListingService->getChildren(*e, true,
				std::tr1::shared_ptr<FileListingService::IListingReceiver>());
			doPull(children, dest, fileListingService, monitor);
			monitor->advance(1);
		} else if (type == FileListingService::TYPE_FILE) {
			monitor->startSubTask((*e)->getFullPath());
			std::string dest = localPath + Poco::Path::separator() + (*e)->getName();
			doPullFile((*e)->getFullPath(), dest, monitor);
		}
	}
}


void SyncService::doPullFile(const std::string& remotePath, const std::string& localPath,
		ISyncProgressMonitor* monitor) {
	std::vector<unsigned char> msg;
	std::vector<unsigned char> pullResult(8);

	int timeOut = DdmPreferences::getTimeOut();

	std::vector<unsigned char> remotePathContent(remotePath.size());
	std::copy(remotePath.begin(), remotePath.end(), remotePathContent.begin());

	if (remotePathContent.size() > REMOTE_PATH_MAX_LENGTH) {
		throw SyncException(SyncException::REMOTE_PATH_LENGTH);
	}

	// create the full request message
	msg = createFileReq(ID_RECV, remotePathContent);

	// and send it.
	AdbHelper::write(mChannel, msg, -1, timeOut);

	// read the result, in a byte array containing 2 ints
	// (id, size)
	AdbHelper::read(mChannel, pullResult, -1, timeOut);

	// check we have the proper data back
	if (checkResult(pullResult, ID_DATA) == false && checkResult(pullResult, ID_DONE) == false) {
		std::string str = readErrorMessage(pullResult, timeOut);
		throw SyncException(SyncException::TRANSFER_PROTOCOL_ERROR, str);
	}

	// access the destination file
	Poco::File f(localPath);

	// create the stream to write in the file. We use a new try/catch block to differentiate
	// between file and network io exceptions.
	Poco::FileOutputStream fos;
	try {
		fos.open(localPath, std::ios::out | std::ios::binary);
	} catch (Poco::IOException& e) {
		Log::e("ddms", std::string("Failed to open local file " + f.path() + " for writing, Reason: " + e.what()));
		throw SyncException(SyncException::FILE_WRITE_ERROR);
	}

	// the buffer to read the data
	std::vector<unsigned char> data(SYNC_DATA_MAX);

	// loop to get data until we're done.
	while (true) {
		// check if we're cancelled
		if (monitor->isCanceled() == true) {
			throw SyncException(SyncException::CANCELED);
		}

		// if we're done, we stop the loop
		if (checkResult(pullResult, ID_DONE)) {
			break;
		}
		if (checkResult(pullResult, ID_DATA) == false) {
			// hmm there's an error
			std::string str = readErrorMessage(pullResult, timeOut);
			throw SyncException(SyncException::TRANSFER_PROTOCOL_ERROR, str);
		}

		unsigned int length = ArrayHelper::swap32bitFromArray(pullResult, 4);
		if (length > SYNC_DATA_MAX) {
			// buffer overrun!
			// error and exit
			throw SyncException(SyncException::BUFFER_OVERRUN);
		}

		// now read the length we received
		AdbHelper::read(mChannel, data, length, timeOut);

		// get the header for the next packet.
		AdbHelper::read(mChannel, pullResult, -1, timeOut);

		// write the content in the file
		for (unsigned int ix = 0; ix != length; ++ix)
			fos.put(data[ix]);
		fos.flush();

		monitor->advance(length);
	}

	fos.close();
}

void SyncService::doPush(const std::vector<Poco::File>& fileArray, const std::string& remotePath,
		ISyncProgressMonitor* monitor) {
	for (std::vector<Poco::File>::const_iterator f = fileArray.begin(); f != fileArray.end(); ++f) {
		// check if we're canceled
		if (monitor->isCanceled() == true) {
			throw SyncException(SyncException::CANCELED);
		}
		if ((*f).exists()) {
			if ((*f).isDirectory()) {
				// append the name of the directory to the remote path
				std::string dest = remotePath + "/" + Poco::Path((*f).path()).getFileName(); // $NON-NLS-1S
				monitor->startSubTask(dest);
				std::vector<Poco::File> fArray;
				(*f).list(fArray);
				doPush(fArray, dest, monitor);

				monitor->advance(1);
			} else if ((*f).isFile()) {
				// append the name of the file to the remote path
				std::string remoteFile = remotePath + "/" + Poco::Path((*f).path()).getFileName();
				monitor->startSubTask(remoteFile);
				doPushFile((*f).path(), remoteFile, monitor);
			}
		}
	}
}

void SyncService::doPushFile(const std::string& localPath, const std::string& remotePath,
		ISyncProgressMonitor* monitor) {
	Poco::FileInputStream fis;
	std::vector<unsigned char> msg;

	int timeOut = DdmPreferences::getTimeOut();

	std::vector<unsigned char> remotePathContent(remotePath.size());
	std::copy(remotePath.begin(), remotePath.end(), remotePathContent.begin());

	if (remotePathContent.size() > REMOTE_PATH_MAX_LENGTH) {
		throw SyncException(SyncException::REMOTE_PATH_LENGTH);
	}

	Poco::File f(localPath);

	// create the stream to read the file
	fis.open(f.path(), std::ios::in | std::ios::binary);

	// create the header for the action
	msg = createSendFileReq(ID_SEND, remotePathContent, 0644);

	// and send it. We use a custom try/catch block to make the difference between
	// file and network IO exceptions.
	AdbHelper::write(mChannel, msg, -1, timeOut);

	// create the buffer used to read.
	// we read max SYNC_DATA_MAX, but we need 2 4 bytes at the beginning.
	if (mBuffer.empty()) {
		mBuffer.resize(SYNC_DATA_MAX + 8);
	}
	std::copy(ID_DATA, ID_DATA + 4, mBuffer.begin());

	// look while there is something to read
	while (!fis.eof()) {
		// check if we're canceled
		if (monitor->isCanceled() == true) {
			throw SyncException(SyncException::CANCELED);
		}

		// read up to SYNC_DATA_MAX
		unsigned int readCount = 0;
		while (readCount + 8 < SYNC_DATA_MAX) {
			mBuffer[8 + readCount] = fis.get();
			if (!fis.eof())
				++readCount;
			else
				break;
		}

		// now send the data to the device
		// first write the amount read
		ArrayHelper::swap32bitsToArray(readCount, mBuffer, 4);

		// now write it
		AdbHelper::write(mChannel, mBuffer, readCount + 8, timeOut);

		// and advance the monitor
		monitor->advance(readCount);
	}
	// close the local file
	fis.close();

	// create the DONE message
	long long time = Poco::Timestamp().epochMicroseconds() / 1000000;
	msg = createReq(ID_DONE, (int) time);

	// and send it.
	AdbHelper::write(mChannel, msg, -1, timeOut);

	// read the result, in a byte array containing 2 ints
	// (id, size)
	std::vector<unsigned char> result(8);
	AdbHelper::read(mChannel, result, -1 /* full length */, timeOut);

	if (checkResult(result, ID_OKAY) == false) {
		std::string str = readErrorMessage(result, timeOut);
		throw SyncException(SyncException::TRANSFER_PROTOCOL_ERROR, str);
	}
}

std::string SyncService::readErrorMessage(const std::vector<unsigned char>& result, int timeOut) {
	if (checkResult(result, ID_FAIL)) {

		int len = ArrayHelper::swap32bitFromArray(result, 4);

		if (len > 0) {
			AdbHelper::read(mChannel, mBuffer, len, timeOut);

			std::string message(mBuffer.begin(), mBuffer.begin() + len);
			Log::e("ddms", "transfer error: " + message);

			return message;
		}
	}

	return "";
}

int SyncService::readMode(const std::string& path) {
	// create the stat request message.
	std::vector<unsigned char> msg = createFileReq(ID_STAT, path);

	AdbHelper::write(mChannel, msg, -1 /* full length */, DdmPreferences::getTimeOut());

	// read the result, in a byte array containing 4 ints
	// (id, mode, size, time)
	std::vector<unsigned char> statResult(16);
	AdbHelper::read(mChannel, statResult, -1 /* full length */, DdmPreferences::getTimeOut());

	// check we have the proper data back
	if (checkResult(statResult, ID_STAT) == false) {
		return -1;
	}

	// we return the mode (2nd int in the array)
	return ArrayHelper::swap32bitFromArray(statResult, 4);
}

std::vector<unsigned char> SyncService::createReq(unsigned char* command, int value) {
	std::vector<unsigned char> array(8);

	std::copy(command, command + 4, array.begin());
	ArrayHelper::swap32bitsToArray(value, array, 4);

	return array;
}

std::vector<unsigned char> SyncService::createFileReq(unsigned char* command, const std::string& path) {
	std::vector<unsigned char> pathContent(path.size());
	std::copy(path.begin(), path.end(), pathContent.begin());

	return createFileReq(command, pathContent);
}

std::vector<unsigned char> SyncService::createFileReq(unsigned char* command,
		const std::vector<unsigned char>& path) {
	std::vector<unsigned char> array(8 + path.size());

	std::copy(command, command + 4, array.begin());
	ArrayHelper::swap32bitsToArray(path.size(), array, 4);
	std::copy(path.begin(), path.begin() + path.size(), array.begin() + 8);

	return array;
}

std::vector<unsigned char> SyncService::createSendFileReq(unsigned char* command,
		const std::vector<unsigned char>& path, int mode) {
	// make the mode into a string
	std::string modeStr = "," + Poco::NumberFormatter::format(mode & 0777); // $NON-NLS-1S
	std::vector<unsigned char> modeContent(modeStr.length());

	std::copy(modeStr.begin(), modeStr.end(), modeContent.begin());

	std::vector<unsigned char> array(8 + path.size() + modeContent.size());

	std::copy(command, command + 4, array.begin());
	ArrayHelper::swap32bitsToArray(path.size() + modeContent.size(), array, 4);
	std::copy(path.begin(), path.end(), array.begin() + 8);
	std::copy(modeContent.begin(), modeContent.end(), array.begin() + 8 + path.size());

	return array;

}

bool SyncService::checkResult(std::vector<unsigned char> result, unsigned char* code) {
	if (result[0] != code[0] || result[1] != code[1] || result[2] != code[2] || result[3] != code[3]) {
		return false;
	}

	return true;

}

int SyncService::getFileType(int mode) {
	if ((mode & S_ISOCK) == S_ISOCK) {
		return FileListingService::TYPE_SOCKET;
	}

	if ((mode & S_IFLNK) == S_IFLNK) {
		return FileListingService::TYPE_LINK;
	}

	if ((mode & S_IFREG) == S_IFREG) {
		return FileListingService::TYPE_FILE;
	}

	if ((mode & S_IFBLK) == S_IFBLK) {
		return FileListingService::TYPE_BLOCK;
	}

	if ((mode & S_IFDIR) == S_IFDIR) {
		return FileListingService::TYPE_DIRECTORY;
	}

	if ((mode & S_IFCHR) == S_IFCHR) {
		return FileListingService::TYPE_CHARACTER;
	}

	if ((mode & S_IFIFO) == S_IFIFO) {
		return FileListingService::TYPE_FIFO;
	}

	return FileListingService::TYPE_OTHER;
}
} /* namespace ddmlib */
