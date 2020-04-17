/*
 * FileListingService.cpp
 *
 *  Created on: 09.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "FileListingService.hpp"
#include "Device.hpp"

namespace ddmlib {
std::string FileListingService::DIRECTORY_DATA = "data";
std::string FileListingService::DIRECTORY_SDCARD = "sdcard";
std::string FileListingService::DIRECTORY_MNT = "mnt";
std::string FileListingService::DIRECTORY_SYSTEM = "system";
std::string FileListingService::DIRECTORY_TEMP = "tmp";
std::string FileListingService::DIRECTORY_APP = "app";
long FileListingService::REFRESH_RATE = 5000L;
long FileListingService::REFRESH_TEST = (FileListingService::REFRESH_RATE * 4) / 5;
int FileListingService::TYPE_FILE = 0;
/** Entry type: Directory */
int FileListingService::TYPE_DIRECTORY = 1;
/** Entry type: Directory Link */
int FileListingService::TYPE_DIRECTORY_LINK = 2;
/** Entry type: Block */
int FileListingService::TYPE_BLOCK = 3;
/** Entry type: Character */
int FileListingService::TYPE_CHARACTER = 4;
/** Entry type: Link */
int FileListingService::TYPE_LINK = 5;
/** Entry type: Socket */
int FileListingService::TYPE_SOCKET = 6;
/** Entry type: FIFO */
int FileListingService::TYPE_FIFO = 7;
/** Entry type: Other */
int FileListingService::TYPE_OTHER = 8;
std::string FileListingService::FILE_SEPARATOR = "/";
Poco::RegularExpression FileListingService::sApkPattern(".*\\.apk", Poco::RegularExpression::RE_CASELESS);
std::string FileListingService::PM_FULL_LISTING = "pm list packages -f";
Poco::RegularExpression FileListingService::sPmPattern("^package:(.+?)=(.+)$");
const std::size_t FileListingService::NUM_DIRECTORY = 5;
std::string FileListingService::sRootLevelApprovedItems[NUM_DIRECTORY] = { DIRECTORY_DATA, DIRECTORY_SDCARD, DIRECTORY_SYSTEM,
		DIRECTORY_TEMP, DIRECTORY_MNT };
std::string FileListingService::FILE_ROOT = "/";
Poco::RegularExpression FileListingService::sLsPattern(
		"^([bcdlsp-][-r][-w][-xsS][-r][-w][-xsS][-r][-w][-xstST])\\s+(\\S+)\\s+(\\S+)\\s+([\\d\\s,]*)\\s+(\\d{4}-\\d\\d-\\d\\d)\\s+(\\d\\d:\\d\\d)\\s+(.*)$",
		Poco::RegularExpression::RE_NEWLINE_ANYCRLF | Poco::RegularExpression::RE_MULTILINE);
Poco::RegularExpression FileListingService::FileEntry::sEscapePattern("([\\\\()*+?\"'#/\\s])");

Poco::FastMutex FileListingService::mLock;
std::map<std::tr1::shared_ptr<Poco::Thread>, std::tr1::shared_ptr<FileListingService::getChildrenThread> > FileListingService::mThreadMap;

FileListingService::FileListingService(std::tr1::shared_ptr<Device> Device) :
		mDevice(Device) {
}

FileListingService::~FileListingService() {
}

FileListingService::FileEntry::FileEntry(std::tr1::shared_ptr<FileEntry> parent, const std::string& name, int type, bool isRoot) {
	this->parent = parent;
	this->name = name;
	this->type = type;
	this->misRoot = isRoot;
	fetchTime = 0;

	checkAppPackageStatus();
}

std::string FileListingService::FileEntry::getFullPath() {
	if (misRoot) {
		return FILE_ROOT;
	}
	std::string pathBuilder;
	fillPathBuilder(pathBuilder, false);

	return pathBuilder;
}

std::string FileListingService::FileEntry::getFullEscapedPath() {
	std::string pathBuilder;
	fillPathBuilder(pathBuilder, true);

	return pathBuilder;
}

std::vector<std::string> FileListingService::FileEntry::getPathSegments() {
	std::vector<std::string> list;
	fillPathSegments(list);

	return list;
}

std::tr1::shared_ptr<FileListingService::FileEntry> FileListingService::FileEntry::findChild(const std::string& name) {
	for (std::vector<std::tr1::shared_ptr<FileEntry> >::iterator entry = mChildren.begin(); entry != mChildren.end(); ++entry) {
		if ((*entry)->name == name) {
			return *entry;
		}
	}
	return std::tr1::shared_ptr<FileListingService::FileEntry>();
}

bool FileListingService::FileEntry::needFetch() {
	if (fetchTime == 0) {
		return true;
	}
	long long current = Poco::Timestamp().epochMicroseconds() / 1000;
	if (current - fetchTime > REFRESH_TEST) {
		return true;
	}

	return false;
}

std::string FileListingService::FileEntry::escape(const std::string& entryName) {
	Poco::RegularExpression::match(entryName, std::string("\\\\$1"), Poco::RegularExpression::RE_GLOBAL);
	return entryName;
}

void FileListingService::FileEntry::fillPathBuilder(std::string& pathBuilder, bool escapePath) {
	if (misRoot) {
		return;
	}

	if (parent != nullptr) {
		parent->fillPathBuilder(pathBuilder, escapePath);
	}
	pathBuilder.append(FILE_SEPARATOR);
	pathBuilder.append(escapePath ? escape(name) : name);
}

void FileListingService::FileEntry::fillPathSegments(std::vector<std::string>& list) {
	if (misRoot) {
		return;
	}

	if (parent != nullptr) {
		parent->fillPathSegments(list);
	}

	list.push_back(name);
}

void FileListingService::FileEntry::checkAppPackageStatus() {
	misAppPackage = false;

	std::vector<std::string> segments = getPathSegments();
	if (type == TYPE_FILE && segments.size() == 3 && isAppFileName()) {
		misAppPackage = (DIRECTORY_APP == segments[1] && (DIRECTORY_SYSTEM == segments[0] || DIRECTORY_DATA == segments[0]));
	}
}

std::tr1::shared_ptr<FileListingService::FileEntry> FileListingService::LsReceiver::getExistingEntry(const std::string& name) {
	for (unsigned int i = 0; i < mCurrentChildren.size(); ++i) {
		std::tr1::shared_ptr<FileEntry> e = mCurrentChildren[i];

		// since we're going to "erase" the one we use, we need to
		// check that the item is not null.
		if (e != nullptr) {
			// compare per name, case-sensitive.
			if (name == e->name) {
				// erase from the list
				mCurrentChildren[i].reset();

				// and return the object
				return e;
			}
		}
	}

	// couldn't find any matching object, return null
	return std::tr1::shared_ptr<FileListingService::FileEntry>();
}

void FileListingService::LsReceiver::processNewLines(const std::vector<std::string>& lines) {
	for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
		// no need to handle empty lines.
		if ((*line).length() == 0) {
			continue;
		}

		// run the line through the regexp
		std::vector<std::string> mgroup;
		if (sLsPattern.split(*line, mgroup) == 0) {
			continue;
		}

		// get the name
		std::string name = mgroup[7];

		// if the parent is root, we only accept selected items
		if (mParentEntry->isRoot()) {
			bool found = false;
			for (std::size_t i = 0; i != NUM_DIRECTORY; ++i) {
				if (sRootLevelApprovedItems[i] == name) {
					found = true;
					break;
				}
			}

			// if it's not in the approved list we skip this entry.
			if (found == false) {
				continue;
			}
		}

		// get the rest of the groups
		std::string permissions = mgroup[1];
		std::string owner = mgroup[2];
		std::string group = mgroup[3];
		std::string size = mgroup[4];
		std::string date = mgroup[5];
		std::string time = mgroup[6];
		std::string info = "";

		// and the type
		int objectType = TYPE_OTHER;
		switch (permissions.at(0)) {
		case '-':
			objectType = TYPE_FILE;
			break;
		case 'b':
			objectType = TYPE_BLOCK;
			break;
		case 'c':
			objectType = TYPE_CHARACTER;
			break;
		case 'd':
			objectType = TYPE_DIRECTORY;
			break;
		case 'l':
			objectType = TYPE_LINK;
			break;
		case 's':
			objectType = TYPE_SOCKET;
			break;
		case 'p':
			objectType = TYPE_FIFO;
			break;
		}

		// now check what we may be linking to
		if (objectType == TYPE_LINK) {
			Poco::RegularExpression regexName("\\s->\\s");
			std::vector<std::string> segments;
			regexName.split(name, segments); 

			// we should have 2 segments
			if (segments.size() == 2) {
				// update the entry name to not contain the link
				name = segments[0];

				// and the link name
				info = segments[1];

				// now get the path to the link
				Poco::RegularExpression regexInfo(FILE_SEPARATOR);
				std::vector<std::string> pathSegments;
				regexInfo.split(info, pathSegments);
				if (pathSegments.size() == 1) {
					// the link is to something in the same directory,
					// unless the link is ..
					if (pathSegments[0] == "..") { 
						// set the type and we're done.
						objectType = TYPE_DIRECTORY_LINK;
					} else {
						// either we found the object already
						// or we'll find it later.
					}
				}
			}

			// add an arrow in front to specify it's a link.
			info = "-> " + info; ;
		}

		// get the entry, either from an existing one, or a new one
		std::tr1::shared_ptr<FileEntry> entry = getExistingEntry(name);
		if (entry == nullptr) {
			entry = std::tr1::shared_ptr<FileEntry>(new FileEntry(mParentEntry, name, objectType, false /* isRoot */));
		}

		// add some misc info
		entry->permissions = permissions;
		entry->size = size;
		entry->date = date;
		entry->time = time;
		entry->owner = owner;
		entry->group = group;
		if (objectType == TYPE_LINK) {
			entry->info = info;
		}

		mEntryList->push_back(entry);
	}
}

std::tr1::shared_ptr<FileListingService::FileEntry> FileListingService::getRoot() {
	if (mDevice != nullptr) {
		if (mRoot == nullptr) {
			mRoot = std::tr1::shared_ptr<FileEntry>(new FileEntry(std::tr1::shared_ptr<FileEntry>() /* parent */, "" /* name */, TYPE_DIRECTORY, true /* isRoot */));
		}

		return mRoot;
	}

	return std::tr1::shared_ptr<FileListingService::FileEntry>();
}

std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> > FileListingService::getChildren(std::tr1::shared_ptr<FileEntry> entry,
		bool useCache, std::tr1::shared_ptr<IListingReceiver> receiver) {

	// first thing we do is check the cache, and if we already have a recent
	// enough children list, we just return that.
	if (useCache && entry->needFetch() == false) {
		return entry->getCachedChildren();
	}

	// if there's no receiver, then this is a synchronous call, and we
	// return the result of ls
	if (receiver == nullptr) {
		doLs(entry);
		return entry->getCachedChildren();
	}

	// this is a asynchronous call.
	// we launch a thread that will do ls and give the listing
	// to the receiver
	std::tr1::shared_ptr<Poco::Thread> t(new Poco::Thread("ls " + entry->getFullPath()));

	std::tr1::shared_ptr<getChildrenThread> rThread(new getChildrenThread(entry, receiver, mDevice, this));
	// we don't want to run multiple ls on the device at the same time, so we
	// store the thread in a list and launch it only if there's no other thread running.
	// the thread will launch the next one once it's done.
	Poco::ScopedLock<Poco::FastMutex> lock(mLock);
	// add to the list
	mThreadMap[t] = rThread;

	// if it's the only one, launch it.
	if (mThreadMap.size() == 1) {
		t->start(*rThread.get());
	}

	// and we return null.
	return std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> >();
}

std::vector<std::tr1::shared_ptr<FileListingService::FileEntry> > FileListingService::getChildrenSync(
		std::tr1::shared_ptr<FileEntry> entry) {
	doLsAndThrow(entry);
	return entry->getCachedChildren();
}

void FileListingService::doLs(std::tr1::shared_ptr<FileEntry> entry) {
	try {
		doLsAndThrow(entry);
	} catch (Poco::Exception& e) {
		// do nothing
	}
}

void FileListingService::doLsAndThrow(std::tr1::shared_ptr<FileEntry> entry) {
	// create a list that will receive the list of the entries
	std::vector<std::tr1::shared_ptr<FileEntry> > entryList;

	// create a list that will receive the link to compute post ls;
	std::vector<std::string> linkList;

	// create the command
	std::string command = "ls -l " + entry->getFullEscapedPath(); 

			// create the receiver object that will parse the result from ls
	std::tr1::shared_ptr<LsReceiver> receiver(new LsReceiver(entry, &entryList, linkList));

	// call ls.
	mDevice->executeShellCommand(command, receiver.get());

	// finish the process of the receiver to handle links
	receiver->finishLinks();

	// at this point we need to refresh the viewer
	entry->fetchTime = Poco::Timestamp().epochMicroseconds() / 1000;

	// sort the children and set them as the new children
	// Collections.sort(entryList, FileEntry.sEntryComparator);
//	std::sort(
//			entryList.begin(),
//			entryList.end(),
//			[] (std::tr1::shared_ptr<FileEntry> a,std::tr1::shared_ptr<FileEntry> b) {return FileEntry::compare(a,b);});

	entry->setChildren(entryList);
}

void FileListingService::getChildrenThread::run() {
	mFileListingService->doLs(mEntry);

	mReceiver->setChildren(mEntry, mEntry->getCachedChildren());

	std::vector<std::tr1::shared_ptr<FileEntry> > children = mEntry->getCachedChildren();
	if (children.size() > 0 && children[0]->isApplicationPackage()) {
		std::map<std::string, std::tr1::shared_ptr<FileEntry> > mapChild;

		for (std::vector<std::tr1::shared_ptr<FileEntry> >::iterator child = children.begin(); child != children.end(); ++child) {
			std::string path = (*child)->getFullPath();
			mapChild[path] = *child;
		}

		// call pm.
		std::string command = PM_FULL_LISTING;
		try {
			class ListingReceiver: public MultiLineReceiver {

			public:
				ListingReceiver(std::tr1::shared_ptr<IListingReceiver> receiver,
						std::map<std::string, std::tr1::shared_ptr<FileEntry> >& mapChild) :
						mReceiver(receiver), mapReceiver(mapChild) {
				}
				;
				void processNewLines(const std::vector<std::string>& lines) {
					for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
						if ((*line).length() > 0) {
							// get the filepath and package from the line
							std::vector<std::string> mgroup;
							if (sPmPattern.split(*line, mgroup)) {
								// get the children with that path
								std::tr1::shared_ptr<FileEntry> entry = mapReceiver[mgroup[1]];
								if (entry != nullptr) {
									entry->info = mgroup[2];
									mReceiver->refreshEntry(entry);
								}
							}
						}
					}
				}
				bool isCancelled() {
					return false;
				}
			private:
				std::tr1::shared_ptr<IListingReceiver> mReceiver;
				std::map<std::string, std::tr1::shared_ptr<FileEntry> > mapReceiver;
			};
			std::tr1::shared_ptr<ListingReceiver> listingReceiver(new ListingReceiver(mReceiver, mapChild));
			mDevice->executeShellCommand(command, listingReceiver.get());
		} catch (Poco::Exception& e) {
			// adb failed somehow, we do nothing.
		}
	}

	// if another thread is pending, launch it

	Poco::ScopedLock<Poco::FastMutex> lock(mLock);
	// first remove ourselves from the list
	mThreadMap.erase(std::tr1::shared_ptr<Poco::Thread>(Poco::Thread::current()));

	// then launch the next one if applicable.
	if (mThreadMap.size() > 0) {
		std::map<std::tr1::shared_ptr<Poco::Thread>, std::tr1::shared_ptr<getChildrenThread> >::iterator it = mThreadMap.begin();
		std::tr1::shared_ptr<Poco::Thread> t = it->first;
		t->start(*(it->second.get())); // TODO need to check does it work or not?
	}

}
} /* namespace ddmlib */
