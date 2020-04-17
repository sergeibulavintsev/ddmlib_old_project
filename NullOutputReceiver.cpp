/*
 * NullOutputReceiver.cpp
 *
 *  Created on: 09.02.2012
 *      Author: sergey bulavintsev
 */

#include "ddmlib.hpp"
#include "NullOutputReceiver.hpp"

namespace ddmlib {

std::tr1::shared_ptr<NullOutputReceiver> NullOutputReceiver::sReceiver(new NullOutputReceiver);

} /* namespace ddmlib */
