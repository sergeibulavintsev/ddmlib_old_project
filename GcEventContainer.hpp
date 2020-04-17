/*
 * GcEventContainer.hpp
 *
 *  Created on: 02.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef GCEVENTCONTAINER_HPP_
#define GCEVENTCONTAINER_HPP_
#include "ddmlib.hpp"

#include "EventContainer.hpp"

namespace ddmlib {

class EventValueDescription;
class LogReceiver;

class DDMLIB_API GcEventContainer : public EventContainer {
public:
	static int GC_EVENT_TAG;
	static const std::size_t NUM_EVENT_CONTAIN;
	static std::tr1::shared_ptr<EventValueDescription> sValueDescriptionVect[];
private:

	std::string processId;
    long long gcTime;
    long long bytesFreed;
    long long objectsFreed;
    long long actualSize;
    long long allowedSize;
    long long softLimit;
    long long objectsAllocated;
    long long bytesAllocated;
    long long zActualSize;
    long long zAllowedSize;
    long long zObjectsAllocated;
    long long zBytesAllocated;
    long long dlmallocFootprint;
    long long mallinfoTotalAllocatedSpace;
    long long externalLimit;
    long long externalBytesAllocated;
public:

    GcEventContainer(const LogEntry &entry, int tag, std::tr1::shared_ptr<EventData> data) :
			EventContainer(entry, tag, data) {
		init(data);
	}

	GcEventContainer(int tag, int pid, int tid, int sec, int nsec, std::tr1::shared_ptr<EventData> data) :
			EventContainer(tag, pid, tid, sec, nsec, data) {
		init(data);
	}

	EventValueType getType() {
		return TYPE_LIST;
	}
	bool testValue(int index, std::tr1::shared_ptr<EventData> value, CompareMethod compareMethod);
	std::tr1::shared_ptr<EventData> getValue(int valueIndex);
	double getValueAsDouble(int valueIndex);
	std::string getValueAsString(int valueIndex);
    /**
     * Returns a custom array of {@link EventValueDescription} since the actual content of this
     * event (list of (long, long) does not match the values encoded into those longs.
     */
	static std::vector< std::tr1::shared_ptr<EventValueDescription> > getValueDescriptions();
    /**
     * Converts a 12 bit float representation into an unsigned int (returned as a long)
     * @param f12
     */
	static long long float12ToInt(int f12);
    /**
     * puts an unsigned value in an array.
     * @param value The value to put.
     * @param dest the destination array
     * @param offset the offset in the array where to put the value.
     *      Array length must be at least offset + 8
     */
	static void put64bitsToArray(long long value, std::vector<unsigned char> &dest, int offset);
	virtual ~GcEventContainer();

	virtual std::string toString();

private:
	void init(std::tr1::shared_ptr<EventData> data);
	void parseDvmHeapInfo(long long data, int index);
    /**
     * Returns the long value of the <code>valueIndex</code>-th value.
     * @param valueIndex the index of the value.
     * @throws InvalidTypeException if index is 0 as it is a string value.
     */
	long long getValueAsLong(int valueIndex);

};




} /* namespace ddmlib */
#endif /* GCEVENTCONTAINER_HPP_ */
