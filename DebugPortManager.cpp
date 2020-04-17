/*
 * DebugPortManager.cpp
 *
 *  Created on: Feb 6, 2012
 *      Author: Ilya Polenov
 */

#include "ddmlib.hpp"
#include "DebugPortManager.hpp"
#include "Device.hpp"

namespace ddmlib {

std::tr1::shared_ptr<IDebugPortProvider> DebugPortManager::sProvider;
long IDebugPortProvider::NO_STATIC_PORT = -1;

DebugPortManager::DebugPortManager() {

}

DebugPortManager::~DebugPortManager() {
}

void DebugPortManager::setProvider(std::tr1::shared_ptr<IDebugPortProvider> provider) {
	sProvider.swap(provider);
}

std::tr1::shared_ptr<IDebugPortProvider> DebugPortManager::getProvider() {
	return sProvider;
}

} /* namespace ddmlib */
