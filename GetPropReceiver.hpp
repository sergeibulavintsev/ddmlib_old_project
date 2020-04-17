/*
 * GetPropReceiver.h
 *
 *  Created on: 09.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef GETPROPRECEIVER_HPP_
#define GETPROPRECEIVER_HPP_
#include "Device.hpp"
#include "ddmlib.hpp"
#include "MultiLineReceiver.hpp"

namespace ddmlib {

class DDMLIB_API GetPropReceiver: public MultiLineReceiver {
public:
	static std::string GETPROP_COMMAND;
	GetPropReceiver(std::tr1::shared_ptr<Device> device) {
		mDevice = device;
	}
	void processNewLines(const std::vector<std::string>& lines);
	bool isCancelled() {
		return false;
	}
	void done() {
		mDevice->update(Device::CHANGE_BUILD_INFO);
	}

private:
	static Poco::RegularExpression GETPROP_PATTERN; 

	/** indicates if we need to read the first */
	std::tr1::shared_ptr<Device> mDevice;
};

} /* namespace ddmlib */
#endif /* GETPROPRECEIVER_H_ */
