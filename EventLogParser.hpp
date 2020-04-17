/*
 * EventLogParser.hpp
 *
 *  Created on: 01.03.2012
 *      Author: Ilya Polenov
 */

#ifndef EVENTLOGPARSER_HPP_
#define EVENTLOGPARSER_HPP_
#include "ddmlib.hpp"
#include "EventContainer.hpp"

namespace ddmlib {

class Device;
class EventContainer;
class EventValueDescription;
class LogReceiver;
struct LogEntry;
struct EventData;

/**
 * Parser for the "event" log.
 */
class DDMLIB_API EventLogParser {
    /** Location of the tag map file on the device */
    static std::string EVENT_TAG_MAP_FILE;

    /**
     * Event log entry types.  These must match up with the declarations in
     * java/android/android/util/EventLog.java.
     */
    static const int EVENT_TYPE_INT;
    static const int EVENT_TYPE_LONG;
    static const int EVENT_TYPE_STRING;
    static const int EVENT_TYPE_LIST;

    static Poco::RegularExpression PATTERN_SIMPLE_TAG;
    static Poco::RegularExpression PATTERN_TAG_WITH_DESC;
    static Poco::RegularExpression PATTERN_DESCRIPTION;

    static Poco::RegularExpression TEXT_LOG_LINE;

    std::map<int, std::string> mTagMap;
    Poco::Mutex sync;

    std::map<int, std::vector< std::tr1::shared_ptr<EventValueDescription> > > mValueDescriptionMap;

    std::vector< std::tr1::shared_ptr<EventValueDescription> > processDescription(const std::string &description);
public:
	EventLogParser();
	virtual ~EventLogParser();

    /**
     * Inits the parser for a specific Device.
     * <p/>
     * This methods reads the event-log-tags located on the device to find out
     * what tags are being written to the event log and what their format is.
     * @param device The device.
     * @return <code>true</code> if success, <code>false</code> if failure or cancellation.
     */
	bool init(std::tr1::shared_ptr<Device> device);

    /**
     * Inits the parser with the content of a tag file.
     * @param tagFileContent the lines of a tag file.
     * @return <code>true</code> if success, <code>false</code> if failure.
     */
	bool init(const std::vector<std::string> &tagFileContent);

    /**
     * Inits the parser with a specified event-log-tags file.
     * @param filePath
     * @return <code>true</code> if success, <code>false</code> if failure.
     */
    bool init(const std::string &filePath);

    /**
     * Processes a line from the event-log-tags file.
     * @param line the line to process
     */
    void processTagLine(const std::string &line);

    std::tr1::shared_ptr<EventContainer> parse(const LogEntry &entry);

    std::tr1::shared_ptr<EventContainer> parse(const std::string &textLogLine);

    std::map<int, std::string> getTagMap();

    std::string getTag(int tagValue);

    std::map<int, std::vector< std::tr1::shared_ptr<EventValueDescription> > > getEventInfoMap();

    /**
     * Recursively convert binary log data to printable form.
     *
     * This needs to be recursive because you can have lists of lists.
     *
     * If we run out of room, we stop processing immediately.  It's important
     * for us to check for space on every output element to avoid producing
     * garbled output.
     *
     * Returns the amount read on success, -1 on failure.
     */
    static int parseBinaryEvent(const std::vector<unsigned char> &eventData,
    		int dataOffset, std::tr1::shared_ptr<EventData> &dest);

    std::tr1::shared_ptr<EventData> parseTextData(const std::string &data, int tagValue);

    std::tr1::shared_ptr<EventData> getObjectFromString(const std::string &value, EventValueType type);


    /**
     * Recreates the event-log-tags at the specified file path.
     * @param filePath the file path to write the file.
     * @throws IOException
     */
    void saveTags(const std::string &filePath);
};

} /* namespace ddmlib */
#endif /* EVENTLOGPARSER_HPP_ */
