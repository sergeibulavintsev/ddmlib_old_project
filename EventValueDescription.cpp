/*
 * EventValueDescription.cpp
 *
 *  Created on: 02.03.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "EventValueDescription.hpp"

namespace ddmlib {

ValueType EventValueDescription::NOT_APPLICABLE,
		EventValueDescription::OBJECTS,
		EventValueDescription::BYTES,
		EventValueDescription::MILLISECONDS,
		EventValueDescription::ALLOCATIONS,
		EventValueDescription::ID,
		EventValueDescription::PERCENT;

EventValueDescription::EventValueDescription(std::string name, EventValueType type) {
	// TODO Auto-generated constructor stub
	mName = name;
	mEventValueType = type;
	if (mEventValueType == TYPE_INT || mEventValueType == TYPE_LONG) {
		mValueType.mValue = BYTES;
	} else {
		mValueType.mValue = VTNOT_APPLICABLE;
	}
}

EventValueDescription::EventValueDescription(std::string name, EventValueType type, ValueType valueType) {
	mName = name;
	mEventValueType = type;
	mValueType.mValue = (int)valueType;
	mValueType.checkType(mEventValueType);
}

std::string EventValueDescription::toString() {
	if (mValueType.mValue != VTNOT_APPLICABLE) {
		return mName + " (" + Poco::NumberFormatter::format(mEventValueType) + ", "
				+ Poco::NumberFormatter::format(mValueType.getValue()) + ")";
	}

	return mName + " (" + Poco::NumberFormatter::format(mEventValueType) + ")";
}

bool EventValueDescription::checkForType(std::tr1::shared_ptr<EventData> value) {
	return mEventValueType == value->type;
}

std::tr1::shared_ptr<EventData> EventValueDescription::getObjectFromString(const std::string& value) {
	switch (mEventValueType) {
	case TYPE_INT:
		try {

			return std::tr1::shared_ptr<EventData>(new EventData(Poco::NumberParser::parse(value)));
		} catch (Poco::DataFormatException& e) {
			return std::tr1::shared_ptr<EventData>();
		}
		break;
	case TYPE_LONG:
		try {
			return std::tr1::shared_ptr<EventData>(new EventData(Poco::NumberParser::parse64(value)));
		} catch (Poco::DataFormatException& e) {
			return std::tr1::shared_ptr<EventData>();
		}
		break;
	case TYPE_STRING:
		return std::tr1::shared_ptr<EventData>(new EventData(value));
	case TYPE_LIST:
	case TYPE_TREE:
	default:
		return std::tr1::shared_ptr<EventData>();
	}

	return std::tr1::shared_ptr<EventData>();
}

EventValueDescription::~EventValueDescription() {
	// TODO Auto-generated destructor stub
}

} /* namespace ddmlib */
