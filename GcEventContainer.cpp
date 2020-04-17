/*
 * GcEventContainer.cpp
 *
 *  Created on: 02.03.2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "GcEventContainer.hpp"
#include "InvalidTypeException.hpp"
#include "InvalidValueTypeException.hpp"
#include "EventValueDescription.hpp"

namespace ddmlib {
int GcEventContainer::GC_EVENT_TAG = 20001;

const std::size_t GcEventContainer::NUM_EVENT_CONTAIN = 21;
 std::tr1::shared_ptr<EventValueDescription> GcEventContainer::sValueDescriptionVect[NUM_EVENT_CONTAIN] = {
	std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Process Name", TYPE_STRING)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("GC Time", TYPE_LONG,
					EventValueDescription::MILLISECONDS)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Freed Objects", TYPE_LONG,
					EventValueDescription::OBJECTS)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Freed Bytes", TYPE_LONG, EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Soft Limit", TYPE_LONG, EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Actual Size (aggregate)", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allowed Size (aggregate)", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allocated Objects (aggregate)",
			TYPE_LONG, EventValueDescription::OBJECTS)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allocated Bytes (aggregate)", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Actual Size", TYPE_LONG, EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allowed Size", TYPE_LONG, EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allocated Objects", TYPE_LONG,
					EventValueDescription::OBJECTS)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allocated Bytes", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Actual Size (zygote)", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allowed Size (zygote)", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allocated Objects (zygote)", TYPE_LONG,
					EventValueDescription::OBJECTS)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Allocated Bytes (zygote)", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("External Allocation Limit", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("External Bytes Allocated", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("dlmalloc Footprint", TYPE_LONG,
					EventValueDescription::BYTES)),
			std::tr1::shared_ptr<EventValueDescription>(new EventValueDescription("Malloc Info: Total Allocated Space",
			TYPE_LONG, EventValueDescription::BYTES))
};

void GcEventContainer::init(std::tr1::shared_ptr<EventData> data) {
	if (data->type == TYPE_LIST) {
		std::vector<std::tr1::shared_ptr<EventData> > values = data->listValue;
		for (unsigned int i = 0; i < values.size(); ++i) {
			if (values[i]->type == TYPE_LONG) {
				parseDvmHeapInfo(values[i]->lValue, i);
			}
		}
	}
}

bool GcEventContainer::testValue(int index, std::tr1::shared_ptr<EventData> value, CompareMethod compareMethod) {
	// do a quick easy check on the type.
	if (index == 0) {
		if (value->type != TYPE_STRING) {
			throw InvalidTypeException();
		}
	} else if (value->type != TYPE_LONG) {
		throw InvalidTypeException();
	}

	if (compareMethod.testString() == "==") {
		if (index == 0) {
			return processId == value->sValue;
		} else {
			return getValueAsLong(index) == value->lValue;
		}
	}

	if (compareMethod.testString() == "<=") {
		return getValueAsLong(index) <= value->lValue;
	}

	if (compareMethod.testString() == "<") {
		return getValueAsLong(index) < value->lValue;
	}

	if (compareMethod.testString() == ">=") {
		return getValueAsLong(index) >= value->lValue;
	}

	if (compareMethod.testString() == ">") {
		return getValueAsLong(index) > value->lValue;
	}
	if (compareMethod.testString() == "&")
		return (getValueAsLong(index) & value->lValue) != 0;
	throw Poco::InvalidArgumentException("index out of bounds");
	return false;


}

std::tr1::shared_ptr<EventData> GcEventContainer::getValue(int valueIndex) {
	if (valueIndex == 0) {
		return std::tr1::shared_ptr<EventData>(new EventData(processId));
	}

	try {
		return std::tr1::shared_ptr<EventData>(new EventData(getValueAsLong(valueIndex)));
	} catch (InvalidTypeException& e) {
		// this would only happened if valueIndex was 0, which we test above.
	}

	return std::tr1::shared_ptr<EventData>();
}

double GcEventContainer::getValueAsDouble(int valueIndex) {
	return (double)getValueAsLong(valueIndex);
}

std::string GcEventContainer::getValueAsString(int valueIndex) {
	switch (valueIndex) {
	case 0:
		return processId;
	default:
		try {
			return Poco::NumberFormatter::format(getValueAsLong(valueIndex));
		} catch (Poco::InvalidArgumentException& e) {
			// we shouldn't stop there since we test, in this method first.
		} break;
	}
	throw Poco::InvalidArgumentException("index out of bounds");
}


std::vector< std::tr1::shared_ptr<EventValueDescription> > GcEventContainer::getValueDescriptions() {
	try {
		return std::vector< std::tr1::shared_ptr<EventValueDescription> >(sValueDescriptionVect, sValueDescriptionVect + NUM_EVENT_CONTAIN);
	} catch (InvalidValueTypeException& e) {
		// this shouldn't happen since we control manual the EventValueType and the ValueType
		// values. For development purpose, we assert if this happens.
		assert(false);
	}

	// this shouldn't happen, but the compiler complains otherwise.
	return std::vector<std::tr1::shared_ptr<EventValueDescription> >(1, std::tr1::shared_ptr<EventValueDescription>());
}

void GcEventContainer::parseDvmHeapInfo(long long data, int index) {
	std::vector<unsigned char> dataArray(8);
	switch (index) {
	case 0:
		//    [63   ] Must be zero
		//    [62-24] ASCII process identifier
		//    [23-12] GC time in ms
		//    [11- 0] Bytes freed

		gcTime = float12ToInt((int) ((data >> 12) & 0xFFFL));
		bytesFreed = float12ToInt((int) (data & 0xFFFL));

		// convert the long into an array, in the proper order so that we can convert the
		// first 5 char into a string.

		put64bitsToArray(data, dataArray, 0);

		// get the name from the string
		processId.resize(5);
		std::copy(dataArray.begin(), dataArray.begin() + 5, processId.begin());
		break;
	case 1:
		//    [63-62] 10
		//    [61-60] Reserved; must be zero
		//    [59-48] Objects freed
		//    [47-36] Actual size (current footprint)
		//    [35-24] Allowed size (current hard max)
		//    [23-12] Objects allocated
		//    [11- 0] Bytes allocated
		objectsFreed = float12ToInt((int) ((data >> 48) & 0xFFFL));
		actualSize = float12ToInt((int) ((data >> 36) & 0xFFFL));
		allowedSize = float12ToInt((int) ((data >> 24) & 0xFFFL));
		objectsAllocated = float12ToInt((int) ((data >> 12) & 0xFFFL));
		bytesAllocated = float12ToInt((int) (data & 0xFFFL));
		break;
	case 2:
		//    [63-62] 11
		//    [61-60] Reserved; must be zero
		//    [59-48] Soft limit (current soft max)
		//    [47-36] Actual size (current footprint)
		//    [35-24] Allowed size (current hard max)
		//    [23-12] Objects allocated
		//    [11- 0] Bytes allocated
		softLimit = float12ToInt((int) ((data >> 48) & 0xFFFL));
		zActualSize = float12ToInt((int) ((data >> 36) & 0xFFFL));
		zAllowedSize = float12ToInt((int) ((data >> 24) & 0xFFFL));
		zObjectsAllocated = float12ToInt((int) ((data >> 12) & 0xFFFL));
		zBytesAllocated = float12ToInt((int) (data & 0xFFFL));
		break;
	case 3:
		//    [63-48] Reserved; must be zero
		//    [47-36] dlmallocFootprint
		//    [35-24] mallinfo: total allocated space
		//    [23-12] External byte limit
		//    [11- 0] External bytes allocated
		dlmallocFootprint = float12ToInt((int) ((data >> 36) & 0xFFFL));
		mallinfoTotalAllocatedSpace = float12ToInt((int) ((data >> 24) & 0xFFFL));
		externalLimit = float12ToInt((int) ((data >> 12) & 0xFFFL));
		externalBytesAllocated = float12ToInt((int) (data & 0xFFFL));
		break;
	default:
		break;
	}
}

long long GcEventContainer::float12ToInt(int f12) {
	return (f12 & 0x1FF) << (((unsigned char) (f12 >> 9)) * 4);
}

void GcEventContainer::put64bitsToArray(long long value, std::vector<unsigned char> &dest, int offset) {
	dest[offset + 7] = (unsigned char) (value & 0x00000000000000FFL);
	dest[offset + 6] = (unsigned char) ((value & 0x000000000000FF00L) >> 8);
	dest[offset + 5] = (unsigned char) ((value & 0x0000000000FF0000L) >> 16);
	dest[offset + 4] = (unsigned char) ((value & 0x00000000FF000000L) >> 24);
	dest[offset + 3] = (unsigned char) ((value & 0x000000FF00000000L) >> 32);
	dest[offset + 2] = (unsigned char) ((value & 0x0000FF0000000000L) >> 40);
	dest[offset + 1] = (unsigned char) ((value & 0x00FF000000000000L) >> 48);
	dest[offset + 0] = (unsigned char) ((value & 0xFF00000000000000L) >> 56);
}

long long GcEventContainer::getValueAsLong(int valueIndex){
	switch (valueIndex) {
		case 0:
		throw InvalidTypeException();
		case 1:
		return gcTime;
		case 2:
		return objectsFreed;
		case 3:
		return bytesFreed;
		case 4:
		return softLimit;
		case 5:
		return actualSize;
		case 6:
		return allowedSize;
		case 7:
		return objectsAllocated;
		case 8:
		return bytesAllocated;
		case 9:
		return actualSize - zActualSize;
		case 10:
		return allowedSize - zAllowedSize;
		case 11:
		return objectsAllocated - zObjectsAllocated;
		case 12:
		return bytesAllocated - zBytesAllocated;
		case 13:
		return zActualSize;
		case 14:
		return zAllowedSize;
		case 15:
		return zObjectsAllocated;
		case 16:
		return zBytesAllocated;
		case 17:
		return externalLimit;
		case 18:
		return externalBytesAllocated;
		case 19:
		return dlmallocFootprint;
		case 20:
		return mallinfoTotalAllocatedSpace;
	}

	throw Poco::InvalidArgumentException("index out of bounds");
}

GcEventContainer::~GcEventContainer() {
	// TODO Auto-generated destructor stub
}

std::string GcEventContainer::toString() {
	std::string result;
	result += "Pid: " + Poco::NumberFormatter::format(pid) + '\n';
	result += "Tid: " + Poco::NumberFormatter::format(tid) + '\n';
	result += "Timestamp:" + Poco::NumberFormatter::format(sec) + '.' + Poco::NumberFormatter::format0(nsec, 9) + '\n';
	for (unsigned int i = 0; i < NUM_EVENT_CONTAIN; ++i) {
		result += sValueDescriptionVect[i]->getName() + ": " + getValueAsString(i) + "\n";
	}
	return result;
}

} /* namespace ddmlib */
