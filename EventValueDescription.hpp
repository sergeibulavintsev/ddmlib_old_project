/*
 * EventValueDescription.hpp
 *
 *  Created on: 02.03.2012
 *      Author: sergey bulavintsev
 */

#ifndef EVENTVALUEDESCRIPTION_HPP_
#define EVENTVALUEDESCRIPTION_HPP_
#include "ddmlib.hpp"
#include "EventContainer.hpp"
#include "InvalidValueTypeException.hpp"

namespace ddmlib {

class EventContainer;

enum ValueType {
    VTNOT_APPLICABLE,
    OBJECTS,
    BYTES,
    MILLISECONDS,
    ALLOCATIONS,
    ID,
    VTPERCENT
};

class DDMLIB_API EventValueDescription {
public:

	class DDMLIB_API ValueData {
	public:
		int mValue;

		/**
		 * Checks that the {@link EventValueType} is compatible with the {@link ValueType}.
		 * @param type the {@link EventValueType} to check.
		 * @throws InvalidValueTypeException if the types are not compatible.
		 */
		void checkType(EventValueType type) {
			if ((type != TYPE_INT && type != TYPE_LONG)
					&& this->mValue != VTNOT_APPLICABLE) {
				throw InvalidValueTypeException(
						Poco::NumberFormatter::format(type) + " doesn't support type " + Poco::NumberFormatter::format(this));
			}
		}

		/**
		 * Returns a {@link ValueType} from an integer value, or <code>null</code> if no match
		 * were found.
		 * @param value the integer value.
		 */
		static ValueType getValueType(int value) {
			return (ValueType)value;
		}

		/**
		 * Returns the integer value of the enum.
		 */
		ValueType getValue() {
			return (ValueType)mValue;
		}

		ValueData(int value) {
			mValue = value;	}
		ValueData() {
			mValue = 0;
		}

	};
	std::string getName() { return mName; }
	EventValueType getEventValueType() { return mEventValueType; }
	ValueType getValueType() { return mValueType.getValue(); }
	std::string toString();
	bool checkForType(std::tr1::shared_ptr<EventData> value);
	std::tr1::shared_ptr<EventData> getObjectFromString(const std::string& value);

	EventValueDescription(){};
	EventValueDescription(std::string name, EventValueType type);
	EventValueDescription(std::string name, EventValueType type, ValueType valueType);
	virtual ~EventValueDescription();

	static ValueType NOT_APPLICABLE;
	static ValueType OBJECTS;
	static ValueType BYTES;
	static ValueType MILLISECONDS;
	static ValueType ALLOCATIONS;
	static ValueType ID;
	static ValueType PERCENT;
private:
	std::string mName;
	EventValueType mEventValueType;
	ValueData mValueType;
};

} /* namespace ddmlib */
#endif /* EVENTVALUEDESCRIPTION_HPP_ */
