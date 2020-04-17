/*
 * DebugPortManager.hpp
 *
 *  Created on: Feb 6, 2012
 *      Author: Ilya Polenov
 */

#ifndef DEBUGPORTMANAGER_HPP_
#define DEBUGPORTMANAGER_HPP_

#include "ddmlib.hpp"

namespace ddmlib {

class Device;

class DDMLIB_API IDebugPortProvider {
public:
	static long NO_STATIC_PORT;

	IDebugPortProvider() {
	}

	virtual ~IDebugPortProvider() {
	}

	/**
	 * Returns a non-random debugger port for the specified application running on the
	 * specified {@link Device}.
	 * @param device The device the application is running on.
	 * @param appName The application name, as defined in the <code>AndroidManifest.xml</code>
	 * <var>package</var> attribute of the <var>manifest</var> node.
	 * @return The non-random debugger port or {@link #NO_STATIC_PORT} if the {@link Client}
	 * should use the automatic debugger port provider.
	 */
	virtual int getPort(std::tr1::shared_ptr<Device> device, const std::wstring &appName) = 0;
};

class DDMLIB_API DebugPortManager {
	static std::tr1::shared_ptr<IDebugPortProvider> sProvider;
public:
	DebugPortManager();
	virtual ~DebugPortManager();

	/**
	 * Sets the {@link IDebugPortProvider} that will be used when a new {@link Client} requests
	 * a debugger port.
	 * @param provider the <code>IDebugPortProvider</code> to use.
	 */
	static void setProvider(std::tr1::shared_ptr<IDebugPortProvider> provider);

	/**
	 * Returns the
	 * @return
	 */
	static std::tr1::shared_ptr<IDebugPortProvider> getProvider();
};

} /* namespace ddmlib */
#endif /* DEBUGPORTMANAGER_HPP_ */
