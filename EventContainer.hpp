/*
 * EventContainer.hpp
 *
 *  Created on: 28.02.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef EVENTCONTAINER_HPP_
#define EVENTCONTAINER_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class LogReceiver;
struct LogEntry;
class InvalidTypeException;

enum EventValueType {
	UNKNOWN,
	TYPE_INT,
	TYPE_LONG,
	TYPE_STRING,
	TYPE_LIST,
	TYPE_TREE
};

struct DDMLIB_API EventData {
	EventData() {
		init();
	}
	EventData(const std::string& val) {
		init();
		sValue = val;
		type = TYPE_STRING;
	}
	EventData(int value) {
		init();
		iValue = value;
		type = TYPE_INT;
	}
	EventData(long long val) {
		init();
		lValue = val;
		type = TYPE_LONG;
	}

	EventData(const std::vector< std::tr1::shared_ptr<EventData> > &val) {
		init();
		listValue = val;
		type = TYPE_LIST;
	}
	void init() {
		iValue = 0;
		lValue = 0;
		sValue = "";
		listValue.clear();
		type = -1;
	}
	std::string sValue;
	int iValue;
	long long lValue;
	std::vector< std::tr1::shared_ptr<EventData> > listValue;

	static Poco::RegularExpression STORAGE_PATTERN;

	int type;

public:

	/**
	 * Returns a storage string for an {@link Object} of type supported by
	 * {@link EventValueType}.
	 * <p/>
	 * Strings created by this method can be reloaded with
	 * {@link #getObjectFromStorageString(String)}.
	 * <p/>
	 * NOTE: for now, only {@link #STRING}, {@link #INT}, and {@link #LONG} are supported.
	 * @param object the object to "convert" into a storage string.
	 * @return a string storing the object and its type or null if the type was not recognized.
	 */
	std::string getStorageString() const {
		if (type == TYPE_STRING)
			return Poco::NumberFormatter::format(type) + "@" + sValue;
		if (type == TYPE_INT)
			return Poco::NumberFormatter::format(type) + "@" + Poco::NumberFormatter::format(iValue);
		if (type == TYPE_STRING)
			return Poco::NumberFormatter::format(type) + "@" + Poco::NumberFormatter::format(lValue);
		if (type == TYPE_LIST) {
			std::string result;
			for (std::vector< std::tr1::shared_ptr<EventData> >::const_iterator event = listValue.begin(); event != listValue.end(); ++event)
				if (!(*event)->getStorageString().empty())
					result += (*event)->getStorageString() + '\n';
			return result;
		}

		return "";
	}

	/**
	 * Creates an {@link Object} from a storage string created with
	 * {@link #getStorageString(Object)}.
	 * @param value the storage string
	 * @return an {@link Object} or null if the string or type were not recognized.
	 */
public:
//	static EventData* getObjectFromStorageString(const std::string& value) {
//		std::vector<std::string> mgroup;
//		if (STORAGE_PATTERN.split(value, mgroup)) {
//			try {
//				EventValueType* type = getEventValueType(Poco::NumberParser::parse(mgroup.at(1)));
//
//				if (type == NULL) {
//					return EventData(NULL, "");
//				}
//
//				return EventData(type, mgroup.at(2));
//			} catch (Poco::SyntaxException& nfe) {
//				return EventData(NULL, "");
//			}
//		}
//
//		return EventData(NULL, "");
//	}

	/**
	 * Returns the integer value of the enum.
	 */
	int get() const {
		return type;
	}

	int compareTo(std::tr1::shared_ptr<EventData> data) {
		if (data->type == TYPE_INT) {
			if (iValue < data->iValue)
				return -1;
			if (iValue == data->iValue)
				return 0;
			if (iValue == data->iValue)
				return 1;
		}
		if (data->type == TYPE_LONG) {
			if (lValue < data->lValue)
				return -1;
			if (lValue == data->lValue)
				return 0;
			if (lValue == data->lValue)
				return 1;
		}
		if (data->type == TYPE_STRING)
			return sValue.compare(data->sValue);
		return 0;
	}
//	std::string toString() {
//		return super.toString().toLowerCase();
//	}

};

class DDMLIB_API EventContainer{
public:
	class CompareMethod {

		std::string mName;
		std::string mTestString;

	public:
		CompareMethod(const std::string& name,
				const std::string& testString){
			mName = name;
			mTestString = testString;
		}

		/**
		 * Returns the display string.
		 */

		std::string toString() {
			return mName;
		}

		/**
		 * Returns a short string representing the comparison.
		 */
		std::string testString() {
			return mTestString;
		}
	};

    /**
     * Creates an {@link EventContainer} from a {@link LogEntry}.
     * @param entry  the LogEntry from which pid, tid, and time info is copied.
     * @param tag the event tag value
     * @param data the data of the EventContainer.
     */
	EventContainer(const LogEntry &entry, int tag, std::tr1::shared_ptr<EventData> data);
    /**
     * Creates an {@link EventContainer} with raw data
     */
	EventContainer(int tag, int pid, int tid, int sec, int nsec, std::tr1::shared_ptr<EventData> data);
    /**
     * Returns the data as an int.
     * @throws InvalidTypeException if the data type is not {@link EventValueType#INT}.
     * @see #getType()
     */
	int getInt();
    /**
     * Returns the data as a long.
     * @throws InvalidTypeException if the data type is not {@link EventValueType#LONG}.
     * @see #getType()
     */
	long long getLong();
    /**
     * Returns the data as a String.
     * @throws InvalidTypeException if the data type is not {@link EventValueType#STRING}.
     * @see #getType()
     */
	std::string getString();
    /**
     * Returns a value by index as a double.
     * @param valueIndex the index of the value. If the data is not a list, this is ignored.
     * @throws InvalidTypeException if the data type is not {@link EventValueType#INT},
     * {@link EventValueType#LONG}, {@link EventValueType#LIST}, or if the item in the
     * list at index <code>valueIndex</code> is not of type {@link EventValueType#INT} or
     * {@link EventValueType#LONG}.
     * @see #getType()
     */
	virtual double getValueAsDouble(size_t valueIndex);

	 /**
	     * Returns a value by index as a String.
	     * @param valueIndex the index of the value. If the data is not a list, this is ignored.
	     * @throws InvalidTypeException if the data type is not {@link EventValueType#INT},
	     * {@link EventValueType#LONG}, {@link EventValueType#STRING}, {@link EventValueType#LIST},
	     * or if the item in the list at index <code>valueIndex</code> is not of type
	     * {@link EventValueType#INT}, {@link EventValueType#LONG}, or {@link EventValueType#STRING}
	     * @see #getType()
	     */
	virtual std::string getValueAsString(size_t valueIndex);
    /**
     * Returns a value by index. The return type is defined by its type.
     * @param valueIndex the index of the value. If the data is not a list, this is ignored.
     */
	virtual std::tr1::shared_ptr<EventData> getValue(size_t valueIndex, bool recursive);
    /**
     * Returns the type of the data.
     */
	virtual EventValueType getType() {
	        return getType(mData);
	    }

	EventValueType getType(std::tr1::shared_ptr<EventData> data) {
		return EventValueType(data->type);
	}

	virtual bool testValue(int index, std::tr1::shared_ptr<EventData> value,
	            CompareMethod compareMethod);

	virtual ~EventContainer(){};

	virtual std::string toString();

public:
	int mTag;
    int pid;    /* generating process's pid */
    int tid;    /* generating process's tid */
    int sec;    /* seconds since Epoch */
    int nsec;   /* nanoseconds */

    static CompareMethod EQUAL_TO;
    static CompareMethod LESSER_THAN;
    static CompareMethod LESSER_THAN_STRICT;
    static CompareMethod GREATER_THAN;
    static CompareMethod GREATER_THAN_STRICT;
    static CompareMethod BIT_CHECK;



private:
    std::tr1::shared_ptr<EventData> getValue(std::tr1::shared_ptr<EventData> data, size_t valueIndex, bool recursive);
    double getValueAsDouble(std::tr1::shared_ptr<EventData> data, size_t valueIndex, bool recursive);
    std::string getValueAsString(std::tr1::shared_ptr<EventData> data, size_t valueIndex, bool recursive);

    std::tr1::shared_ptr<EventData> mData;
};



} /* namespace ddmlib */
#endif /* EVENTCONTAINER_HPP_ */
