/*
 * SyncService.cpp
 *
 *  Created on: Feb 22, 2012
 *      Author: sergey bulavintsev
 */

#ifndef SYNCSERVICE_HPP_
#define SYNCSERVICE_HPP_
#include "ddmlib.hpp"
#include "FileListingService.hpp"

namespace ddmlib {

class Device;

/**
 * Classes which implement this interface provide methods that deal
 * with displaying transfer progress.
 */
class DDMLIB_API ISyncProgressMonitor {
public:
	virtual ~ISyncProgressMonitor() {
	}

	/**
	 * Sent when the transfer starts
	 * @param totalWork the total amount of work.
	 */
	virtual void start(int totalWork) =0;
	/**
	 * Sent when the transfer is finished or interrupted.
	 */
	virtual void stop() = 0;
	/**
	 * Sent to query for possible cancellation.
	 * @return true if the transfer should be stopped.
	 */
	virtual bool isCanceled() const = 0;
	/**
	 * Sent when a sub task is started.
	 * @param name the name of the sub task.
	 */
	virtual void startSubTask(const std::string &name) = 0;
	/**
	 * Sent when some progress have been made.
	 * @param work the amount of work done.
	 */
	virtual void advance(int work) = 0;
};

/**
 * A Sync progress monitor that does nothing
 */
class DDMLIB_API NullSyncProgressMonitor: public ISyncProgressMonitor {
public:
	void advance(int work) {
	}
	bool isCanceled() const {
		return false;
	}

	void start(int totalWork) {
	}
	void startSubTask(const std::string &name) {
	}
	void stop() {
	}
};

class DDMLIB_API SyncService {
	static unsigned char ID_OKAY[];
	static unsigned char ID_FAIL[];
	static unsigned char ID_STAT[];
	static unsigned char ID_RECV[];
	static unsigned char ID_DATA[];
	static unsigned char ID_DONE[];
	static unsigned char ID_SEND[];
	static unsigned char ID_LIST[];
	static unsigned char ID_DENT[];

	static std::tr1::shared_ptr<NullSyncProgressMonitor> sNullSyncProgressMonitor;

	static const unsigned int S_ISOCK; // type: symbolic link
	static const unsigned int S_IFLNK; // type: symbolic link
	static const unsigned int S_IFREG; // type: regular file
	static const unsigned int S_IFBLK; // type: block device
	static const unsigned int S_IFDIR; // type: directory
	static const unsigned int S_IFCHR; // type: character device
	static const unsigned int S_IFIFO; // type: fifo
	/*
	 static const unsigned int S_ISUID = 0x0800; // set-uid bit
	 static const unsigned int S_ISGID = 0x0400; // set-gid bit
	 static const unsigned int S_ISVTX = 0x0200; // sticky bit
	 static const unsigned int S_IRWXU = 0x01C0; // user permissions
	 static const unsigned int S_IRUSR = 0x0100; // user: read
	 static const unsigned int S_IWUSR = 0x0080; // user: write
	 static const unsigned int S_IXUSR = 0x0040; // user: execute
	 static const unsigned int S_IRWXG = 0x0038; // group permissions
	 static const unsigned int S_IRGRP = 0x0020; // group: read
	 static const unsigned int S_IWGRP = 0x0010; // group: write
	 static const unsigned int S_IXGRP = 0x0008; // group: execute
	 static const unsigned int S_IRWXO = 0x0007; // other permissions
	 static const unsigned int S_IROTH = 0x0004; // other: read
	 static const unsigned int S_IWOTH = 0x0002; // other: write
	 static const unsigned int S_IXOTH = 0x0001; // other: execute
	 */
	static const unsigned int SYNC_DATA_MAX = 64 * 1024;
	static const unsigned int REMOTE_PATH_MAX_LENGTH = 1024;

	Poco::Net::SocketAddress mAddress;
	std::tr1::shared_ptr<Device> mDevice;
	std::tr1::shared_ptr<Poco::Net::StreamSocket> mChannel;

	/**
	 * Buffer used to send data. Allocated when needed and reused afterward.
	 */
	std::vector<unsigned char> mBuffer;

public:

	/**
	 * Creates a Sync service object.
	 * @param address The address to connect to
	 * @param device the {@link Device} that the service connects to.
	 */
	SyncService(const Poco::Net::SocketAddress &address, std::tr1::shared_ptr<Device> device);

	/**
	 * Opens the sync connection. This must be called before any calls to push[File] / pull[File].
	 * @return true if the connection opened, false if adb refuse the connection. This can happen
	 * if the {@link Device} is invalid.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException If the connection to adb failed.
	 */
	bool openSync();

	/**
	 * Closes the connection.
	 */
	void close() {
	}

	/**
	 * Returns a sync progress monitor that does nothing. This allows background tasks that don't
	 * want/need to display ui, to pass a valid {@link ISyncProgressMonitor}.
	 * <p/>This object can be reused multiple times and can be used by concurrent threads.
	 */
	static std::tr1::shared_ptr<ISyncProgressMonitor> getNullProgressMonitor();

	/**
	 * Pulls file(s) or folder(s).
	 * @param entries the remote item(s) to pull
	 * @param localPath The local destination. If the entries count is > 1 or
	 *      if the unique entry is a folder, this should be a folder.
	 * @param monitor The progress monitor. Cannot be null.
	 * @throws SyncException
	 * @throws IOException
	 * @throws TimeoutException
	 *
	 * @see FileListingService.FileEntry
	 * @see #getNullProgressMonitor()
	 */
	void pull(const std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >& entries, const std::string& localPath,
			std::tr1::shared_ptr<ISyncProgressMonitor> monitor);

	void pull(const std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >& entries, const std::string& localPath,
			ISyncProgressMonitor* monitor);
	/**
	 * Pulls a single file.
	 * @param remote the remote file
	 * @param localFilename The local destination.
	 * @param monitor The progress monitor. Cannot be null.
	 *
	 * @throws IOException in case of an IO exception.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 * @throws SyncException in case of a sync exception.
	 *
	 * @see FileListingService.FileEntry
	 * @see #getNullProgressMonitor()
	 */
	void pullFile(std::tr1::shared_ptr<FileListingService::FileEntry> remote, const std::string& localFilename,
			std::tr1::shared_ptr<ISyncProgressMonitor> monitor);

	void pullFile(std::tr1::shared_ptr<FileListingService::FileEntry> remote, const std::string& localFilename,
				ISyncProgressMonitor* monitor);

	/**
	 * Pulls a single file.
	 * <p/>Because this method just deals with a String for the remote file instead of a
	 * {@link FileEntry}, the size of the file being pulled is unknown and the
	 * {@link ISyncProgressMonitor} will not properly show the progress
	 * @param remoteFilepath the full path to the remote file
	 * @param localFilename The local destination.
	 * @param monitor The progress monitor. Cannot be null.
	 *
	 * @throws IOException in case of an IO exception.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 * @throws SyncException in case of a sync exception.
	 *
	 * @see #getNullProgressMonitor()
	 */
	void pullFile(const std::string& remoteFilepath, const std::string& localFilename,
			std::tr1::shared_ptr<ISyncProgressMonitor> monitor);

	void pullFile(const std::string& remoteFilepath, const std::string& localFilename,
			ISyncProgressMonitor* monitor);

	/**
	 * Push several files.
	 * @param local An array of loca files to push
	 * @param remote the remote {@link FileEntry} representing a directory.
	 * @param monitor The progress monitor. Cannot be null.
	 * @throws SyncException if file could not be pushed
	 * @throws IOException in case of I/O error on the connection.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 */
	void push(const std::vector<std::string>& local, std::tr1::shared_ptr<FileListingService::FileEntry> remote,
			std::tr1::shared_ptr<ISyncProgressMonitor> monitor);

	void push(const std::vector<std::string>& local, std::tr1::shared_ptr<FileListingService::FileEntry> remote,
			ISyncProgressMonitor* monitor);

	/**
	 * Push a single file.
	 * @param local the local filepath.
	 * @param remote The remote filepath.
	 * @param monitor The progress monitor. Cannot be null.
	 *
	 * @throws SyncException if file could not be pushed
	 * @throws IOException in case of I/O error on the connection.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 */
	void pushFile(const std::string& local, const std::string& remote, std::tr1::shared_ptr<ISyncProgressMonitor> monitor);

	void pushFile(const std::string& local, const std::string& remote, ISyncProgressMonitor* monitor);

public:
	SyncService();
	virtual ~SyncService();

private:

	/**
	 * compute the recursive file size of all the files in the list. Folder
	 * have a weight of 1.
	 * @param entries
	 * @param fls
	 * @return
	 */
	int getTotalRemoteFileSize(const std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >& entries,
			std::tr1::shared_ptr<FileListingService> fls);

	/**
	 * compute the recursive file size of all the files in the list. Folder
	 * have a weight of 1.
	 * This does not check for circular links.
	 * @param files
	 * @return
	 */
	unsigned long long getTotalLocalFileSize(const std::vector<Poco::File>& files);
	/**
	 * Pulls multiple files/folders recursively.
	 * @param entries The list of entry to pull
	 * @param localPath the localpath to a directory
	 * @param fileListingService a FileListingService object to browse through remote directories.
	 * @param monitor the progress monitor. Must be started already.
	 *
	 * @throws SyncException if file could not be pushed
	 * @throws IOException in case of I/O error on the connection.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 */
	void doPull(std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> > entries, const std::string& localPath,
			std::tr1::shared_ptr<FileListingService> fileListingService, ISyncProgressMonitor* monitor);
	/**
	 * Pulls a remote file
	 * @param remotePath the remote file (length max is 1024)
	 * @param localPath the local destination
	 * @param monitor the monitor. The monitor must be started already.
	 * @throws SyncException if file could not be pushed
	 * @throws IOException in case of I/O error on the connection.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 */
	void doPullFile(const std::string& remotePath, const std::string& localPath,
			ISyncProgressMonitor* monitor);
	/**
	 * Push multiple files
	 * @param fileArray
	 * @param remotePath
	 * @param monitor
	 *
	 * @throws SyncException if file could not be pushed
	 * @throws IOException in case of I/O error on the connection.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 */
	void doPush(const std::vector<Poco::File>& fileArray, const std::string& remotePath,
			ISyncProgressMonitor* monitor);
	/**
	 * Push a single file
	 * @param localPath the local file to push
	 * @param remotePath the remote file (length max is 1024)
	 * @param monitor the monitor. The monitor must be started already.
	 *
	 * @throws SyncException if file could not be pushed
	 * @throws IOException in case of I/O error on the connection.
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 */
	void doPushFile(const std::string& localPath, const std::string& remotePath,
			ISyncProgressMonitor* monitor);
	/**
	 * Reads an error message from the opened {@link #mChannel}.
	 * @param result the current adb result. Must contain both FAIL and the length of the message.
	 * @param timeOut
	 * @return
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 * @throws IOException
	 */
	std::string readErrorMessage(const std::vector<unsigned char>& result, int timeOut);
	/**
	 * Returns the mode of the remote file.
	 * @param path the remote file
	 * @return an Integer containing the mode if all went well or null
	 *      otherwise
	 * @throws IOException
	 * @throws TimeoutException in case of a timeout reading responses from the device.
	 */
	int readMode(const std::string& path);
	/**
	 * Create a command with a code and an int values
	 * @param command
	 * @param value
	 * @return
	 */
	static std::vector<unsigned char> createReq(unsigned char* command, int value);
	/**
	 * Creates the data array for a stat request.
	 * @param command the 4 byte command (ID_STAT, ID_RECV, ...)
	 * @param path The path of the remote file on which to execute the command
	 * @return the byte[] to send to the device through adb
	 */
	static std::vector<unsigned char> createFileReq(unsigned char* command, const std::string& path);
	/**
	 * Creates the data array for a file request. This creates an array with a 4 byte command + the
	 * remote file name.
	 * @param command the 4 byte command (ID_STAT, ID_RECV, ...).
	 * @param path The path, as a byte array, of the remote file on which to
	 *      execute the command.
	 * @return the byte[] to send to the device through adb
	 */
	static std::vector<unsigned char> createFileReq(unsigned char* command,
			const std::vector<unsigned char>& path);

	static std::vector<unsigned char> createSendFileReq(unsigned char* command,
			const std::vector<unsigned char>& path, int mode);

	/**
	 * Checks the result array starts with the provided code
	 * @param result The result array to check
	 * @param code The 4 byte code.
	 * @return true if the code matches.
	 */
	static bool checkResult(std::vector<unsigned char> result, unsigned char* code);
	static int getFileType(int mode);

};

} /* namespace ddmlib */
#endif /* SYNCSERVICE_HPP_ */
