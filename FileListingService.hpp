/*
 * FileListingService.hpp
 *
 *  Created on: 09.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef FILELISTINGSERVICE_HPP_
#define FILELISTINGSERVICE_HPP_
#include "ddmlib.hpp"

#include "MultiLineReceiver.hpp"


namespace ddmlib {

class Device;

class DDMLIB_API FileListingService {
public:
	/** Top level data folder. */
	static std::string DIRECTORY_DATA;
	/** Top level sdcard folder. */
	static std::string DIRECTORY_SDCARD;
	/** Top level mount folder. */
	static std::string DIRECTORY_MNT;
	/** Top level system folder. */
	static std::string DIRECTORY_SYSTEM;
	/** Top level temp folder. */
	static std::string DIRECTORY_TEMP;
	/** Application folder. */
	static std::string DIRECTORY_APP;
	static const std::size_t NUM_DIRECTORY;
	static long REFRESH_RATE;
	/**
	 * Refresh test has to be slightly lower for precision issue.
	 */
	static long REFRESH_TEST;
	/** Entry type: File */
	static int TYPE_FILE;
	/** Entry type: Directory */
	static int TYPE_DIRECTORY;
	/** Entry type: Directory Link */
	static int TYPE_DIRECTORY_LINK;
	/** Entry type: Block */
	static int TYPE_BLOCK;
	/** Entry type: Character */
	static int TYPE_CHARACTER;
	/** Entry type: Link */
	static int TYPE_LINK;
	/** Entry type: Socket */
	static int TYPE_SOCKET;
	/** Entry type: FIFO */
	static int TYPE_FIFO;
	/** Entry type: Other */
	static int TYPE_OTHER;

	/** Device side file separator. */
	static std::string FILE_SEPARATOR;

	/**
	 * Represents an entry in a directory. This can be a file or a directory.
	 */
	class DDMLIB_API FileEntry {
	public:
		FileEntry() {
			fetchTime = 0;
			misRoot = false;
			misAppPackage = false;
			type = TYPE_OTHER;
		}

		~FileEntry() {
		}

		/** Pattern to escape filenames for shell command consumption. */
		/**
		 * Creates a new file entry.
		 * @param parent parent entry or null if entry is root
		 * @param name name of the entry.
		 * @param type entry type. Can be one of the following: {@link FileListingService#TYPE_FILE},
		 * {@link FileListingService#TYPE_DIRECTORY}, {@link FileListingService#TYPE_OTHER}.
		 */
		FileEntry(std::tr1::shared_ptr<FileEntry> parent, const std::string& name, int type, bool isRoot);

		static int compare(std::tr1::shared_ptr<FileEntry> o1, std::tr1::shared_ptr<FileEntry> o2) {
			return (o1->name.compare(o2->name));
		}
		/**
		 * Returns the name of the entry
		 */
		std::string getName() {
			return name;
		}

		/**
		 * Returns the size string of the entry, as returned by <code>ls</code>.
		 */
		std::string getSize() {
			return size;
		}

		/**
		 * Returns the size of the entry.
		 */
		int getSizeValue() {
			return Poco::NumberParser::parse(size);
		}

		/**
		 * Returns the date string of the entry, as returned by <code>ls</code>.
		 */
		std::string getDate() {
			return date;
		}

		/**
		 * Returns the time string of the entry, as returned by <code>ls</code>.
		 */
		std::string getTime() {
			return time;
		}

		/**
		 * Returns the permission string of the entry, as returned by <code>ls</code>.
		 */
		std::string getPermissions() {
			return permissions;
		}

		/**
		 * Returns the extra info for the entry.
		 * <p/>For a link, it will be a description of the link.
		 * <p/>For an application apk file it will be the application package as returned
		 * by the Package Manager.
		 */
		std::string getInfo() {
			return info;
		}

		/**
		 * Return the full path of the entry.
		 * @return a path string using {@link FileListingService#FILE_SEPARATOR} as separator.
		 */
		std::string getFullPath();

		/**
		 * Return the fully escaped path of the entry. This path is safe to use in a
		 * shell command line.
		 * @return a path string using {@link FileListingService#FILE_SEPARATOR} as separator
		 */
		std::string getFullEscapedPath();

		/**
		 * Returns the path as a list of segments.
		 */
		std::vector<std::string> getPathSegments();

		/**
		 * Returns true if the entry is a directory, false otherwise;
		 */
		int getType() {
			return type;
		}

		/**
		 * Returns if the entry is a folder or a link to a folder.
		 */
		bool isDirectory() {
			return type == TYPE_DIRECTORY || type == TYPE_DIRECTORY_LINK;
		}

		/**
		 * Returns the parent entry.
		 */
		std::tr1::shared_ptr<FileEntry> getParent() {
			return parent;
		}

		/**
		 * Returns the cached children of the entry. This returns the cache created from calling
		 * <code>FileListingService.getChildren()</code>.
		 */
		std::vector<std::tr1::shared_ptr<FileEntry> > getCachedChildren() {
			return mChildren;
		}

		/**
		 * Returns the child {@link FileEntry} matching the name.
		 * This uses the cached children list.
		 * @param name the name of the child to return.
		 * @return the FileEntry matching the name or null.
		 */
		std::tr1::shared_ptr<FileEntry> findChild(const std::string& name);

		/**
		 * Returns whether the entry is the root.
		 */
		bool isRoot() {
			return misRoot;
		}

		void addChild(std::tr1::shared_ptr<FileEntry> child) {
			mChildren.push_back(child);
		}

		void setChildren(std::vector<std::tr1::shared_ptr<FileEntry> >& newChildren) {
			mChildren.clear();
			mChildren = newChildren;
		}

		bool needFetch();

		/**
		 * Returns if the entry is a valid application package.
		 */
		bool isApplicationPackage() {
			return misAppPackage;
		}

		/**
		 * Returns if the file name is an application package name.
		 */
		bool isAppFileName() {
			return sApkPattern.match(name);
		}

		/**
		 * Returns an escaped version of the entry name.
		 * @param entryName
		 */
		static std::string escape(const std::string& entryName);

		/**
		 * Recursively fills the pathBuilder with the full path
		 * @param pathBuilder a StringBuilder used to create the path.
		 * @param escapePath Whether the path need to be escaped for consumption by
		 * a shell command line.
		 */
		std::string name;
		std::string info;
		std::string permissions;
		std::string size;
		std::string date;
		std::string time;
		std::string owner;
		std::string group;
		long fetchTime;
	protected:
		void fillPathBuilder(std::string& pathBuilder, bool escapePath);

		/**
		 * Recursively fills the segment list with the full path.
		 * @param list The list of segments to fill.
		 */
		void fillPathSegments(std::vector<std::string>& list);

	private:
		static Poco::RegularExpression sEscapePattern;

		std::tr1::shared_ptr<FileEntry> parent;

		int type;
		bool misAppPackage;

		bool misRoot;

		/**
		 * Indicates whether the entry content has been fetched yet, or not.
		 */

		std::vector<std::tr1::shared_ptr<FileEntry> > mChildren;

		/**
		 * Sets the internal app package status flag. This checks whether the entry is in an app
		 * directory like /data/app or /system/app
		 */
		void checkAppPackageStatus();

	}; ////End of FileEntry Class
	class IListingReceiver {
	public:
		virtual void setChildren(std::tr1::shared_ptr<FileEntry> entry, std::vector<std::tr1::shared_ptr<FileEntry> > children) = 0;

		virtual void refreshEntry(std::tr1::shared_ptr<FileEntry> entry) = 0;
	};
	/**
	 * Returns the root element.
	 * @return the {@link FileEntry} object representing the root element or
	 * <code>null</code> if the device is invalid.
	 */
	std::tr1::shared_ptr<FileEntry> getRoot();
	/**
	 * Returns the children of a {@link FileEntry}.
	 * <p/>
	 * This method supports a cache mechanism and synchronous and asynchronous modes.
	 * <p/>
	 * If <var>receiver</var> is <code>null</code>, the device side <code>ls</code>
	 * command is done synchronously, and the method will return upon completion of the command.<br>
	 * If <var>receiver</var> is non <code>null</code>, the command is launched is a separate
	 * thread and upon completion, the receiver will be notified of the result.
	 * <p/>
	 * The result for each <code>ls</code> command is cached in the parent
	 * <code>FileEntry</code>. <var>useCache</var> allows usage of this cache, but only if the
	 * cache is valid. The cache is valid only for {@link FileListingService#REFRESH_RATE} ms.
	 * After that a new <code>ls</code> command is always executed.
	 * <p/>
	 * If the cache is valid and <code>useCache == true</code>, the method will always simply
	 * return the value of the cache, whether a {@link IListingReceiver} has been provided or not.
	 *
	 * @param entry The parent entry.
	 * @param useCache A flag to use the cache or to force a new ls command.
	 * @param receiver A receiver for asynchronous calls.
	 * @return The list of children or <code>null</code> for asynchronous calls.
	 *
	 * @see FileEntry#getCachedChildren()
	 */
	std::vector<std::tr1::shared_ptr<FileEntry> > getChildren(std::tr1::shared_ptr<FileEntry> entry, bool useCache,
			std::tr1::shared_ptr<IListingReceiver> receiver);
	/**
	 * Returns the children of a {@link FileEntry}.
	 * <p/>
	 * This method is the explicit synchronous version of
	 * {@link #getChildren(FileEntry, boolean, IListingReceiver)}. It is roughly equivalent to
	 * calling
	 * getChildren(FileEntry, false, null)
	 *
	 * @param entry The parent entry.
	 * @return The list of children
	 * @throws TimeoutException in case of timeout on the connection when sending the command.
	 * @throws AdbCommandRejectedException if adb rejects the command.
	 * @throws ShellCommandUnresponsiveException in case the shell command doesn't send any output
	 *            for a period longer than <var>maxTimeToOutputResponse</var>.
	 * @throws IOException in case of I/O error on the connection.
	 */
	std::vector<std::tr1::shared_ptr<FileEntry> > getChildrenSync(std::tr1::shared_ptr<FileEntry> entry);
	FileListingService(std::tr1::shared_ptr<Device> Device);
	virtual ~FileListingService();

private:

	class LsReceiver: public MultiLineReceiver {

		std::vector<std::tr1::shared_ptr<FileEntry> > *mEntryList;
		std::vector<std::string> mLinkList;
		std::vector<std::tr1::shared_ptr<FileEntry> > mCurrentChildren;
		std::tr1::shared_ptr<FileEntry> mParentEntry;
		/**
		 * Queries for an already existing Entry per name
		 * @param name the name of the entry
		 * @return the existing FileEntry or null if no entry with a matching
		 * name exists.
		 */
		std::tr1::shared_ptr<FileEntry> getExistingEntry(const std::string& name);
	public:
		/**
		 * Create an ls receiver/parser.
		 * @param currentChildren The list of current children. To prevent
		 *      collapse during update, reusing the same FileEntry objects for
		 *      files that were already there is paramount.
		 * @param entryList the list of new children to be filled by the
		 *      receiver.
		 * @param linkList the list of link path to compute post ls, to figure
		 *      out if the link pointed to a file or to a directory.
		 */
		LsReceiver(std::tr1::shared_ptr<FileEntry> parentEntry, std::vector<std::tr1::shared_ptr<FileEntry> > *entryList,
				std::vector<std::string>& linkList) {
			mParentEntry = parentEntry;
			mCurrentChildren = parentEntry->getCachedChildren();
			mEntryList = entryList;
			mLinkList = linkList;
		}

		virtual void processNewLines(const std::vector<std::string>& lines);

		bool isCancelled() {
			return false;
		}

		void finishLinks() {
			// TODO Handle links in the listing service
		}
	};

	/**
	 * Classes which implement this interface provide a method that deals with asynchronous
	 * result from <code>ls</code> command on the device.
	 *
	 * @see FileListingService#getChildren(com.android.ddmlib.FileListingService.FileEntry, boolean, com.android.ddmlib.FileListingService.IListingReceiver)
	 */

	class getChildrenThread: public Poco::Runnable { 

	public:
		getChildrenThread(std::tr1::shared_ptr<FileEntry> entry, std::tr1::shared_ptr<IListingReceiver> Receiver,
				std::tr1::shared_ptr<Device> threadDevice, FileListingService* pObject) :
				mEntry(entry), mReceiver(Receiver), mDevice(threadDevice), mFileListingService(pObject) {
		}
		;
		void run();
	private:
		std::tr1::shared_ptr<FileEntry> mEntry;
		std::tr1::shared_ptr<IListingReceiver> mReceiver;
		std::tr1::shared_ptr<Device> mDevice;
		FileListingService* mFileListingService;
	};

	void doLs(std::tr1::shared_ptr<FileEntry> entry);
	void doLsAndThrow(std::tr1::shared_ptr<FileEntry> entry);
	/** Pattern to find filenames that match "*.apk" */
	static Poco::RegularExpression sApkPattern;
	static std::string PM_FULL_LISTING;
	/** Pattern to parse the output of the 'pm -lf' command.<br>
	 * The output format looks like:<br>
	 * /data/app/myapp.apk=com.mypackage.myapp */
	static Poco::RegularExpression sPmPattern;
	static std::string sRootLevelApprovedItems[];
	static std::string FILE_ROOT;
	static Poco::RegularExpression sLsPattern;
//	Poco::Thread mThread;
	static Poco::FastMutex mLock;

	std::tr1::shared_ptr<Device> mDevice;
	std::tr1::shared_ptr<FileEntry> mRoot;
	static std::map<std::tr1::shared_ptr<Poco::Thread>, std::tr1::shared_ptr<getChildrenThread> > mThreadMap;

};

} /* namespace ddmlib */
#endif /* FILELISTINGSERVICE_HPP_ */
