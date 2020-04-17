/*
 * EventLogParser.cpp
 *
 *  Created on: 01.03.2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "EventLogParser.hpp"
#include "Device.hpp"
#include "ArrayHelper.hpp"
#include "Log.hpp"
#include "LogReceiver.hpp"
#include "MultiLineReceiver.hpp"
#include "EventContainer.hpp"
#include "EventValueDescription.hpp"
#include "GcEventContainer.hpp"

namespace ddmlib {

std::string EventLogParser::EVENT_TAG_MAP_FILE = "/system/etc/event-log-tags";

const int EventLogParser::EVENT_TYPE_INT = 0;
const int EventLogParser::EVENT_TYPE_LONG = 1;
const int EventLogParser::EVENT_TYPE_STRING = 2;
const int EventLogParser::EVENT_TYPE_LIST = 3;

Poco::RegularExpression EventLogParser::PATTERN_SIMPLE_TAG("^(\\d+)\\s+([A-Za-z0-9_]+)\\s*$",
		Poco::RegularExpression::RE_NEWLINE_ANYCRLF);
Poco::RegularExpression EventLogParser::PATTERN_TAG_WITH_DESC("^(\\d+)\\s+([A-Za-z0-9_]+)\\s*(.*)\\s*$",
		Poco::RegularExpression::RE_NEWLINE_ANYCRLF);
Poco::RegularExpression EventLogParser::PATTERN_DESCRIPTION("\\(([A-Za-z0-9_\\s]+)\\|(\\d+)(\\|\\d+){0,1}\\)",
		Poco::RegularExpression::RE_NEWLINE_ANYCRLF);

Poco::RegularExpression EventLogParser::TEXT_LOG_LINE(
		"(\\d\\d)-(\\d\\d)\\s(\\d\\d):(\\d\\d):(\\d\\d).(\\d{3})\\s+I/([a-zA-Z0-9_]+)\\s*\\(\\s*(\\d+)\\):\\s+(.*)",
		Poco::RegularExpression::RE_NEWLINE_ANYCRLF | Poco::RegularExpression::RE_MULTILINE);

EventLogParser::EventLogParser() {
}

std::vector<std::tr1::shared_ptr<EventValueDescription> > EventLogParser::processDescription(const std::string& description) {
    Poco::StringTokenizer descriptions(description, ",");
    std::vector< std::tr1::shared_ptr<EventValueDescription> > list;

    for (Poco::StringTokenizer::Iterator desc = descriptions.begin(); desc != descriptions.end(); ++desc) {
    	std::vector<std::string> matches;
        int n = PATTERN_DESCRIPTION.split(*desc, matches);
        if (matches.size() > 2) {
            try {
                std::string name = matches[1];

                std::string typeString = matches[2];
                int typeValue = Poco::NumberParser::parse(typeString);
                EventValueType eventValueType = EventValueType(typeValue);
//                if (eventValueType == null) {
//                    // just ignore this description if the value is not recognized.
//                    // TODO: log the error.
//                }

                if (matches.size() > 3) {
                    typeString = matches[3];
                    //skip the |
                    typeString = typeString.substr(1);

                    typeValue = Poco::NumberParser::parse(typeString);
                    ValueType valueType = ValueType(typeValue);

                    list.push_back(std::tr1::shared_ptr< EventValueDescription >(new EventValueDescription(name, eventValueType, valueType)));
                } else {
                    list.push_back(std::tr1::shared_ptr< EventValueDescription >(new EventValueDescription(name, eventValueType)));
                }
            } catch (Poco::SyntaxException &nfe) {
                // just ignore this description if one number is malformed.
                Log::v("EventLogParser", "Number is malformed in " + description);
            } catch (Poco::InvalidArgumentException &e) {
                // just ignore this description if data type and data unit don't match
                Log::v("EventLogParser", "Data type and data unit don't match in " + description);
            }
        } else {
            Log::e("EventLogParser", "Can't parse " + description);
        }
    }

    if (list.size() == 0) {
    	Log::v("EventLogParser", "Description \"" + description + "\" produced nothing");
        //return list;
    }

    return list;

}

EventLogParser::~EventLogParser() {
}


bool EventLogParser::init(std::tr1::shared_ptr<Device> device) {
	class taglineRcvr: public MultiLineReceiver {
		EventLogParser *elp;
		Poco::Mutex *sync;
	public:
		taglineRcvr(EventLogParser *elp, Poco::Mutex *sync): elp(elp), sync(sync) {
			sync->lock();
		}

		void processNewLines(const std::vector<std::string>& lines) {
			for (std::vector<std::string>::const_iterator line = lines.begin(); line != lines.end(); ++line) {
				elp->processTagLine(*line);
			}
		}

		bool isCancelled() {
			return false;
		}

		~taglineRcvr() {
			sync->unlock();
		}
	};

    // read the event tag map file on the device.
    try {
    	std::tr1::shared_ptr<taglineRcvr> rcvr(new taglineRcvr(this, &sync));
        device->executeShellCommand("cat " + EVENT_TAG_MAP_FILE, rcvr.get());
    } catch (std::exception &e) {
        // catch all possible exceptions and return false.
        return false;
    }

    return true;
}

bool EventLogParser::init(const std::vector<std::string>& tagFileContent) {
    for (std::vector<std::string>::const_iterator line = tagFileContent.begin(); line != tagFileContent.end(); ++line) {
        processTagLine(*line);
    }
    return true;
}

bool EventLogParser::init(const std::string& filePath) {
	try {
		Poco::FileInputStream reader(filePath, std::ios::in);

		std::string line;
		while (reader.good()) {
			getline(reader, line);
			if (!line.empty()) {
				processTagLine(line);
			}
		}
	} catch (std::exception &e) {
		Log::e("EventLogParser", "Error reading event log tags file");
		return false;
	}
	return true;
}

void EventLogParser::processTagLine(const std::string& line) {
    // ignore empty lines and comment lines
    if (line.size() > 0 && line[0] != '#') {
    	std::vector< std::string > matches;
        int n = PATTERN_TAG_WITH_DESC.split(line, matches);
        if (n > 0) {
            try {
                int value = Poco::NumberParser::parse(matches[1]);
                std::string name = matches[2];
                if (!name.empty() && mTagMap.count(value) == 0) {
                    mTagMap[value] = name;
                }

                // special case for the GC tag. We ignore what is in the file,
                // and take what the custom GcEventContainer class tells us.
                // This is due to the event encoding several values on 2 longs.
                // @see GcEventContainer
                if (value == GcEventContainer::GC_EVENT_TAG) {
                    mValueDescriptionMap[value] = GcEventContainer::getValueDescriptions();
                } else {
                    std::string description = matches[3];
                    if (description.length() > 0) {
                    	std::vector<std::tr1::shared_ptr<EventValueDescription> > desc =
                            processDescription(description);

                        if (!desc.empty()) {
                            mValueDescriptionMap[value] = desc;
                        }
                    }
                }
            } catch (Poco::SyntaxException &e) {
                // failed to convert the number into a string. just ignore it.
            }
        } else {
            int m = PATTERN_SIMPLE_TAG.split(line, matches);
            if (m > 0) {
                int value = Poco::NumberParser::parse(matches[1]);
                std::string name = matches[2];
                if (!name.empty() && mTagMap.count(value) == 0) {
                    mTagMap[value] = name;
                }
            }
        }
    }
}

std::tr1::shared_ptr<EventContainer> EventLogParser::parse(const LogEntry& entry) {
    if (entry.len < 4) {
        return std::tr1::shared_ptr<EventContainer>();
    }

    int inOffset = 0;

    int tagValue = ArrayHelper::swap32bitFromArray(entry.data, inOffset);
    inOffset += 4;

    std::string tag("");
    sync.lock();
    sync.unlock();
    if (mTagMap.count(tagValue) == 0) {
        Log::e("EventLogParser", "unknown tag number: " + Poco::NumberFormatter::format(tagValue));
    } else {
    	tag = mTagMap[tagValue];
    }

    std::tr1::shared_ptr<EventData> evtData;
    if (parseBinaryEvent(entry.data, inOffset, evtData) == -1) {
        return std::tr1::shared_ptr<EventContainer>();
    }

    std::tr1::shared_ptr<EventContainer> event;
    if (tagValue == GcEventContainer::GC_EVENT_TAG) {
        event = std::tr1::shared_ptr<GcEventContainer>(new GcEventContainer(entry, tagValue, evtData));
    } else {
        event = std::tr1::shared_ptr<EventContainer>(new EventContainer(entry, tagValue, evtData));
    }

    return event;
}

std::tr1::shared_ptr<EventContainer> EventLogParser::parse(const std::string& textLogLine) {
    // line will look like
    // 04-29 23:16:16.691 I/dvm_gc_info(  427): <data>
    // where <data> is either
    // [value1,value2...]
    // or
    // value
    if (textLogLine.length() == 0) {
        return std::tr1::shared_ptr<EventContainer>();
    }

    // parse the header first
    std::vector< std::string > matches;
    int n = TEXT_LOG_LINE.split(textLogLine, matches);
    if (n > 0) {
        try {
            int month = Poco::NumberParser::parse(matches[1]);
            int day = Poco::NumberParser::parse(matches[2]);
            int hours = Poco::NumberParser::parse(matches[3]);
            int minutes = Poco::NumberParser::parse(matches[4]);
            int seconds = Poco::NumberParser::parse(matches[5]);
            int milliseconds = Poco::NumberParser::parse(matches[6]);

            // convert into seconds since epoch and nano-seconds.
            Poco::DateTime dateTime(Poco::DateTime().year(), month, day, hours, minutes, seconds, milliseconds);

            int sec = dateTime.timestamp().epochMicroseconds() / 1000000;
            int nsec = dateTime.timestamp().epochMicroseconds()  * 1000;

            std::string tag = matches[7];

            // get the numerical tag value
            int tagValue = -1;
            sync.lock();
            sync.unlock();
            for (std::map<int, std::string>::iterator entry = mTagMap.begin(); entry != mTagMap.end(); ++entry) {
                if (tag == (*entry).second) {
                    tagValue = (*entry).first;
                    break;
                }
            }

            if (tagValue == -1) {
                return std::tr1::shared_ptr<EventContainer>();
            }

            int pid = Poco::NumberParser::parse(matches[8]);

            std::tr1::shared_ptr<EventData> data = parseTextData(matches[9], tagValue);
            if (data == nullptr) {
                return std::tr1::shared_ptr<EventContainer>();
            }

            // now we can allocate and return the EventContainer
            std::tr1::shared_ptr<EventContainer> event;
            if (tagValue == GcEventContainer::GC_EVENT_TAG) {
                event = std::tr1::shared_ptr<GcEventContainer>(new GcEventContainer(tagValue, pid, -1 , sec, nsec, data));
            } else {
                event = std::tr1::shared_ptr<EventContainer>(new EventContainer(tagValue, pid, -1 , sec, nsec, data));
            }

            return event;
        } catch (Poco::SyntaxException &e) {
            return std::tr1::shared_ptr<EventContainer>();
        }
    }

    return std::tr1::shared_ptr<EventContainer>();
}

std::map<int, std::string> EventLogParser::getTagMap() {
	return mTagMap;
}

std::string EventLogParser::getTag(int tagValue) {
	if (mTagMap.count(tagValue) == 1)
		return mTagMap[tagValue];
	else
		return "";
}

std::map<int, std::vector<std::tr1::shared_ptr<EventValueDescription> > > EventLogParser::getEventInfoMap() {
	return mValueDescriptionMap;
}

int EventLogParser::parseBinaryEvent(const std::vector<unsigned char>& eventData, int dataOffset,
		std::tr1::shared_ptr<EventData> &dest) {
	if (eventData.size() - dataOffset < 1)
		return -1;

	int offset = dataOffset;

	int type = eventData[offset++];

	switch (type) {
	case EVENT_TYPE_INT: { /* 32-bit signed int */
        int ival;

        if (eventData.size() - offset < 4)
            return -1;
        ival = ArrayHelper::swap32bitFromArray(eventData, offset);
        offset += 4;

        dest = std::tr1::shared_ptr<EventData>(new EventData(ival));
	}
		break;
	case EVENT_TYPE_LONG: { /* 64-bit signed long */
        long long lval;

        if (eventData.size() - offset < 8)
            return -1;
		unsigned char *tmp = new unsigned char[eventData.size()];
		std::copy(eventData.begin(), eventData.end(), tmp);
        lval = ArrayHelper::swap64bitFromArray(tmp, offset);
		delete []tmp;
        offset += 8;

        dest = std::tr1::shared_ptr<EventData>(new EventData(lval));
	}
		break;
	case EVENT_TYPE_STRING: { /* UTF-8 chars, not NULL-terminated */
        unsigned int strLen;

        if (eventData.size() - offset < 4)
            return -1;
        strLen = ArrayHelper::swap32bitFromArray(eventData, offset);
        offset += 4;

        if (eventData.size() - offset < strLen)
            return -1;

        // get the string
		std::string str(eventData.begin() + offset, eventData.begin() + offset + strLen); 
		dest = std::tr1::shared_ptr<EventData>(new EventData(str));
        offset += strLen;
	}
		break;
    case EVENT_TYPE_LIST: { /* N items, all different types */

            if (eventData.size() - offset < 1)
                return -1;

            int count = eventData[offset++];

            // make a new temp list
            std::vector<std::tr1::shared_ptr<EventData> > subList;
            for (int i = 0; i < count; i++) {
            	std::tr1::shared_ptr<EventData> evt;
                int result = parseBinaryEvent(eventData, offset, evt);
                if (result == -1) {
                    return result;
                }

                subList.push_back(evt);
                offset += result;
            }

            dest = std::tr1::shared_ptr<EventData>(new EventData(subList));
        }
        break;
    default:
        Log::e("EventLogParser", "Unknown binary event type " + Poco::NumberFormatter::format(type));
        return -1;
	}
    return offset - dataOffset;
}

std::tr1::shared_ptr<EventData> EventLogParser::parseTextData(const std::string& data, int tagValue) {
    // first, get the description of what we're supposed to parse
	std::vector< std::tr1::shared_ptr<EventValueDescription> > desc = mValueDescriptionMap[tagValue];

    if (desc.empty()) {
        // TODO parse and create string values.
        return std::tr1::shared_ptr<EventData>();
    }

    if (desc.size() == 1) {
        return getObjectFromString(data, desc[0]->getEventValueType());
    } else if (data[0] == '[' && *(data.end() - 1) == ']') {
        std::string theData = data.substr(1, data.length() - 2);

        // get each individual values as String
        Poco::StringTokenizer values(theData, ",");

        if (tagValue == GcEventContainer::GC_EVENT_TAG) {
            // special case for the GC event!
            std::vector< std::tr1::shared_ptr<EventData> > objects(2);

            objects[0] = getObjectFromString(values[0], TYPE_LONG);
            objects[1] = getObjectFromString(values[1], TYPE_LONG);

            return std::tr1::shared_ptr<EventData>(new EventData(objects));
        } else {
            // must be the same number as the number of descriptors.
            if (values.count() != desc.size()) {
                return std::tr1::shared_ptr<EventData>();
            }

            std::vector< std::tr1::shared_ptr<EventData> > objects(values.count());

            for (size_t i = 0 ; i < desc.size() ; ++i) {
            	std::tr1::shared_ptr<EventData> obj = getObjectFromString(values[i], desc[i]->getEventValueType());
                if (obj == nullptr) {
                    return std::tr1::shared_ptr<EventData>();
                }
                objects[i] = obj;
            }

            return std::tr1::shared_ptr<EventData>(new EventData(objects));
        }
    }

    return std::tr1::shared_ptr<EventData>();
}

std::tr1::shared_ptr<EventData> EventLogParser::getObjectFromString(const std::string &value, EventValueType type) {
    try {
    	switch (type) {
			case TYPE_INT:
				return std::tr1::shared_ptr<EventData>(new EventData(Poco::NumberParser::parse(value)));
			case TYPE_LONG:
				return std::tr1::shared_ptr<EventData>(new EventData(Poco::NumberParser::parse64(value)));
			case TYPE_STRING:
				return std::tr1::shared_ptr<EventData>(new EventData(value));
			case TYPE_LIST:
			case TYPE_TREE:
			case UNKNOWN:
			default:
				return std::tr1::shared_ptr<EventData>();
    	}
    } catch (Poco::SyntaxException &e) {
        // do nothing, we'll return null.
    }

    return std::tr1::shared_ptr<EventData>();
}

void EventLogParser::saveTags(const std::string& filePath) {
}

} /* namespace ddmlib */
