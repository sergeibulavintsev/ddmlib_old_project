/*
 * EventContainer.cpp
 *
 *  Created on: 28.02.2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "EventContainer.hpp"
#include "LogReceiver.hpp"
#include "InvalidTypeException.hpp"

namespace ddmlib {
Poco::RegularExpression EventData::STORAGE_PATTERN("^(\\d+)@(.*)$");
EventContainer::CompareMethod EventContainer::EQUAL_TO("equals", "=="),
		EventContainer::LESSER_THAN("less than or equals to", "<="),
		EventContainer::LESSER_THAN_STRICT("less than", "<"),
		EventContainer::GREATER_THAN("greater than or equals to", ">="),
		EventContainer::GREATER_THAN_STRICT("greater than", ">"),
		EventContainer::BIT_CHECK("bit check", "&");
EventContainer::EventContainer(const LogEntry &entry, int tag, std::tr1::shared_ptr<EventData> data){
	mTag = tag;
	mData = data;
	pid = entry.pid;
	tid = entry.tid;
	sec = entry.sec;
	nsec = entry.nsec;
}

EventContainer::EventContainer(int tag, int pid, int tid, int sec, int nsec, std::tr1::shared_ptr<EventData> data) {
    mTag = tag;
    mData = data;

    this->pid = pid;
    this->tid = tid;
    this->sec = sec;
    this->nsec = nsec;
}

int EventContainer::getInt() {
	if (getType(mData) == TYPE_INT)
		return mData->iValue;
	else return -1;
}

long long EventContainer::getLong() {
	if (getType(mData) == TYPE_LONG) {
		return mData->lValue;
	}

	return -1;
}

std::string EventContainer::getString(){
        if (getType(mData) == TYPE_STRING) {
            return mData->sValue;
        }
        return "";
}

std::tr1::shared_ptr<EventData> EventContainer::getValue(size_t valueIndex, bool recursive = true) {
	return getValue(mData, valueIndex, recursive);
}

double EventContainer::getValueAsDouble(size_t valueIndex){
	return getValueAsDouble(mData, valueIndex, true);
}

std::string EventContainer::getValueAsString(size_t valueIndex){
        return getValueAsString(mData, valueIndex, true);
}


bool EventContainer::testValue(int index, std::tr1::shared_ptr<EventData> value, CompareMethod compareMethod) {
	EventValueType type = getType(mData);
	if (index > 0 && type != TYPE_LIST) {
		throw InvalidTypeException();
	}

	std::tr1::shared_ptr<EventData> data = mData;
	if (type == TYPE_LIST) {
		data->listValue = mData->listValue;
	}

	if (compareMethod.testString() == "=="){
		if (data->type == TYPE_INT) {
			return (data->iValue == value->iValue);
		} else if (data->type == TYPE_LONG) {
			return (data->lValue == value->lValue);
		} throw InvalidTypeException();
	}


	if (compareMethod.testString() == "<=") {
		if (data->type == TYPE_INT) {
			return (data->compareTo(value) <= 0);
		} else if (data->type == TYPE_LONG) {
			return (data->compareTo(value) <= 0);
		} throw InvalidTypeException();
	}

	// other types can't use this compare method.

	if (compareMethod.testString() == "<") {
		if (data->type == TYPE_INT) {
			return (data->compareTo(value) < 0);
		} else if (data->type == TYPE_LONG) {
			return (data->compareTo(value) < 0);
		} throw InvalidTypeException();
	}


	if (compareMethod.testString() == ">=") {
		if (data->type == TYPE_INT) {
			return (data->compareTo(value) >= 0);
		} else if (data->type == TYPE_LONG) {
			return (data->compareTo(value) >= 0);
		} throw InvalidTypeException();
	}

	if (compareMethod.testString() == ">") {
		if (data->type == TYPE_INT) {
			return (data->compareTo(value) > 0);
		} else if (data->type == TYPE_LONG) {
			return (data->compareTo(value) > 0);
		} throw InvalidTypeException();
	}

	if (compareMethod.testString() == "&") {
		if (data->type == TYPE_INT) {
			return (data->iValue & value->iValue) != 0;
		} else if (data->type == TYPE_LONG) {
			return (data->lValue & value->lValue) != 0;
		}
		throw InvalidTypeException();
	}

	return false;
}

std::tr1::shared_ptr<EventData> EventContainer::getValue(std::tr1::shared_ptr<EventData> data, size_t valueIndex, bool recursive) {
	EventValueType type = getType(data);

	switch (type) {
	case TYPE_INT:
	case TYPE_LONG:
	case TYPE_STRING:
		return data;
	case TYPE_LIST:
		if (recursive) {
			std::vector<std::tr1::shared_ptr<EventData> > list = data->listValue;
			if (valueIndex < list.size()) {
				return getValue(list[valueIndex], valueIndex, false);
			}
			break;
		} else
			return data;
		break;
	case TYPE_TREE:
		break;
	case UNKNOWN:
		break;
	}

	return std::tr1::shared_ptr<EventData>();
}

double EventContainer::getValueAsDouble(std::tr1::shared_ptr<EventData> data, size_t valueIndex, bool recursive) {
	EventValueType type = getType(data);

	switch (type) {
	case TYPE_INT:
		return ((double) data->iValue);
	case TYPE_LONG:
		return ((double) data->lValue);
	case TYPE_STRING:
		throw InvalidTypeException();
	case TYPE_LIST:
		if (recursive) {
			std::vector<std::tr1::shared_ptr<EventData> > list = data->listValue;
			if (valueIndex < list.size()) {
				return getValueAsDouble(list[valueIndex], valueIndex, false);
			}
			break;
		}
		break;
	case TYPE_TREE:
		break;
	case UNKNOWN:
		break;
	}

	throw InvalidTypeException();
}

std::string EventContainer::getValueAsString(std::tr1::shared_ptr<EventData> data, size_t valueIndex, bool recursive = true) {
	EventValueType type = getType(data);

	switch (type) {
	case TYPE_INT:
		return Poco::NumberFormatter::format(data->iValue);
	case TYPE_LONG:
		return Poco::NumberFormatter::format(data->lValue);
	case TYPE_STRING:
		return data->sValue;
	case TYPE_LIST:
		if (recursive) {
			std::vector<std::tr1::shared_ptr<EventData> > list = data->listValue;
			if (valueIndex < list.size()) {
				return getValueAsString(list[valueIndex], valueIndex, false);
			}
		} else {
			throw InvalidTypeException("getValueAsString() doesn't support EventValueType.TREE");
		}
		break;
	case TYPE_TREE:
		break;
	case UNKNOWN:
		break;
	}

	throw InvalidTypeException("getValueAsString() unsupported type:" + Poco::NumberFormatter::format(type));
}

std::string EventContainer::toString() {
	std::string result;
	result += "Pid: " + Poco::NumberFormatter::format(pid) + '\n';
	result += "Tid: " + Poco::NumberFormatter::format(tid) + '\n';
	result += "Timestamp:" + Poco::NumberFormatter::format(sec) + '.' + Poco::NumberFormatter::format0(nsec, 9) + '\n';
	result += mData->getStorageString();
	return result;
}

} /* namespace ddmlib */
