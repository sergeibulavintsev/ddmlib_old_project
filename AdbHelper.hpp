/*
 * AdbHelper.hpp
 *
 *  Created on: 06.02.2012
 *      Author: sergey bulavintsev
 */

#ifndef ADBHELPER_HPP_
#define ADBHELPER_HPP_
#include "ddmlib.hpp"

namespace ddmlib {

class Device;
class LogReceiver;
class IShellOutputReceiver;
class RawImage;

class DDMLIB_API AdbHelper {
public:
	// public static final long kOkay = 0x59414b4fL;
	// public static final long kFail = 0x4c494146L;
	const static int WAIT_TIME = 5; // spin-wait sleep, in ms
	const static std::string DEFAULT_ENCODING;

	struct AdbResponse {
		AdbResponse() {
			okay = false;
			message = "";
		}

		bool okay; // first 4 bytes in response were "OKAY"?
		std::string message; // diagnostic string if #okay is false
	};

	/**
	 * Create and connect a new pass-through socket, from the host to a port on
	 * the device.
	 *
	 * @param adbSockAddr
	 * @param device the device to connect to. Can be null in which case the connection will be
	 * to the first available device.
	 * @param devicePort the port we're opening
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws IOException in case of I/O error on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 */
	static std::tr1::shared_ptr<Poco::Net::StreamSocket> open(const Poco::Net::SocketAddress& adbSockAddr,
			std::tr1::shared_ptr<Device> device, int devicePort);

	/**
	 * Creates and connects a new pass-through socket, from the host to a port on
	 * the device.
	 *
	 * @param adbSockAddr
	 * @param device the device to connect to. Can be null in which case the connection will be
	 * to the first available device.
	 * @param pid the process pid to connect to.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static std::tr1::shared_ptr<Poco::Net::StreamSocket> createPassThroughConnection(const Poco::Net::SocketAddress& adbSockAddr,
			std::tr1::shared_ptr<Device> device, int pid);

	/**
	 * Creates a port forwarding request for adb. This returns an array
	 * containing "####tcp:{port}:{addStr}".
	 * @param addrStr the host. Can be null.
	 * @param port the port on the device. This does not need to be numeric.
	 */
	static std::vector<unsigned char> createAdbForwardRequest(const std::string &addrStr, int port);

	/**
	 * Creates a port forwarding request to a jdwp process. This returns an array
	 * containing "####jwdp:{pid}".
	 * @param pid the jdwp process pid on the device.
	 */
	static std::vector<unsigned char> createJdwpForwardRequest(int pid);

	/**
	 * Create an ASCII string preceeded by four hex digits. The opening "####"
	 * is the length of the rest of the string, encoded as ASCII hex (case
	 * doesn't matter). "port" and "host" are what we want to forward to. If
	 * we're on the host side connecting into the device, "addrStr" should be
	 * null.
	 */
	static std::vector<unsigned char> formAdbRequest(const std::string &req);

	/**
	 * Reads the response from ADB after a command.
	 * @param chan The socket channel that is connected to adb.
	 * @param readDiagString If true, we're expecting an OKAY response to be
	 *      followed by a diagnostic string. Otherwise, we only expect the
	 *      diagnostic string to follow a FAIL.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws IOException in case of I/O error on the connection.
	 */
	static AdbResponse readAdbResponse(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, bool readDiagString);

	/**
	 * Retrieve the frame buffer from the device.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static std::tr1::shared_ptr<RawImage> getFrameBuffer(const Poco::Net::SocketAddress& adbSockAddr,
			std::tr1::shared_ptr<Device> device);

	/**
	 * Executes a shell command on the device and retrieve the output. The output is
	 * handed to <var>rcvr</var> as it arrives.
	 *
	 * @param adbSockAddr the {@link InetSocketAddress} to adb.
	 * @param command the shell command to execute
	 * @param device the {@link Device} on which to execute the command.
	 * @param rcvr the {@link IShellOutputReceiver} that will receives the output of the shell
	 *            command
	 * @param maxTimeToOutputResponse max time between command output. If more time passes
	 *            between command output, the method will throw
	 *            {@link ShellCommandUnresponsiveException}. A value of 0 means the method will
	 *            wait forever for command output and never throw.
	 * @throws TimeoutException in case of timeout on the connection when sending the command.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws ShellCommandUnresponsiveException in case the shell command doesn't send any output
	 *            for a period longer than <var>maxTimeToOutputResponse</var>.
	 * @throws IOException in case of I/O error on the connection.
	 *
	 * @see DdmPreferences#getTimeOut()
	 */
	static void executeRemoteCommand(const Poco::Net::SocketAddress& adbSockAddr, const std::string& command,
			std::tr1::shared_ptr<Device> device, std::tr1::shared_ptr<IShellOutputReceiver> rcvr, int maxTimeToOutputResponse);
	static void executeRemoteCommand(const Poco::Net::SocketAddress& adbSockAddr, const std::string& command,
			Device *device, IShellOutputReceiver *rcvr, int maxTimeToOutputResponse);

	/**
	 * Runs the Event log service on the {@link Device}, and provides its output to the
	 * {@link LogReceiver}.
	 * <p/>This call is blocking until {@link LogReceiver#isCancelled()} returns true.
	 * @param adbSockAddr the socket address to connect to adb
	 * @param device the Device on which to run the service
	 * @param rcvr the {@link LogReceiver} to receive the log output
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void runEventLogService(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device,
			std::tr1::shared_ptr<LogReceiver> rcvr);
	static void runEventLogService(const Poco::Net::SocketAddress& adbSockAddr, Device *device,
			LogReceiver *rcvr);

	/**
	 * Runs a log service on the {@link Device}, and provides its output to the {@link LogReceiver}.
	 * <p/>This call is blocking until {@link LogReceiver#isCancelled()} returns true.
	 * @param adbSockAddr the socket address to connect to adb
	 * @param device the Device on which to run the service
	 * @param logName the name of the log file to output
	 * @param rcvr the {@link LogReceiver} to receive the log output
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void runLogService(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device,
			const std::string& logName, std::tr1::shared_ptr<LogReceiver> rcvr);
	static void runLogService(const Poco::Net::SocketAddress& adbSockAddr, Device *device,
					const std::string& logName, LogReceiver *rcvr);

	/**
	 * Creates a port forwarding between a local and a remote port.
	 * @param adbSockAddr the socket address to connect to adb
	 * @param device the device on which to do the port fowarding
	 * @param localPort the local port to forward
	 * @param remotePort the remote port.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void createForward(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device, int localPort,
			int remotePort);

	/**
	 * Remove a port forwarding between a local and a remote port.
	 * @param adbSockAddr the socket address to connect to adb
	 * @param device the device on which to remove the port fowarding
	 * @param localPort the local port of the forward
	 * @param remotePort the remote port.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void removeForward(const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device, int localPort,
			int remotePort);

	/**
	 * Checks to see if the first four bytes in "reply" are OKAY.
	 */
	static bool isOkay(std::vector<unsigned char>& reply);

	/**
	 * Converts an ADB reply to a string.
	 */
	static std::string replyToString(std::vector<unsigned char>& reply);

	/**
	 * Reads from the socket until the array is filled, or no more data is coming (because
	 * the socket closed or the timeout expired).
	 * <p/>This uses the default time out value.
	 *
	 * @param chan the opened socket to read from. It must be in non-blocking
	 *      mode for timeouts to work
	 * @param data the buffer to store the read data into.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void read(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, std::vector<unsigned char>& data);

	/**
	 * Reads from the socket until the array is filled, the optional length
	 * is reached, or no more data is coming (because the socket closed or the
	 * timeout expired). After "timeout" milliseconds since the
	 * previous successful read, this will return whether or not new data has
	 * been found.
	 *
	 * @param chan the opened socket to read from. It must be in non-blocking
	 *      mode for timeouts to work
	 * @param data the buffer to store the read data into.
	 * @param length the length to read or -1 to fill the data buffer completely
	 * @param timeout The timeout value. A timeout of zero means "wait forever".
	 */
	static void read(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, std::vector<unsigned char>& data, int length,
			int timeout);

	/**
	 * Write until all data in "data" is written or the connection fails or times out.
	 * <p/>This uses the default time out value.
	 * @param chan the opened socket to write to.
	 * @param data the buffer to send.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void write(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, const std::vector<unsigned char>& data);

	/**
	 * Write until all data in "data" is written, the optional length is reached,
	 * the timeout expires, or the connection fails. Returns "true" if all
	 * data was written.
	 * @param chan the opened socket to write to.
	 * @param data the buffer to send.
	 * @param length the length to write or -1 to send the whole buffer.
	 * @param timeout The timeout value. A timeout of zero means "wait forever".
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void write(std::tr1::shared_ptr<Poco::Net::StreamSocket> chan, const std::vector<unsigned char>& data, int length,
			int timeout);

	/**
	 * tells adb to talk to a specific device
	 *
	 * @param adbChan the socket connection to adb
	 * @param device The device to talk to.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void setDevice(std::tr1::shared_ptr<Poco::Net::StreamSocket> adbChan, Device *device);

	/**
	 * Reboot the device.
	 *
	 * @param into what to reboot into (recovery, bootloader).  Or null to just reboot.
	 * @throws TimeoutException in case of timeout on the connection.
	 * @throws AdbCommandRejectedException if adb rejects the command
	 * @throws IOException in case of I/O error on the connection.
	 */
	static void reboot(const std::string &into, const Poco::Net::SocketAddress& adbSockAddr, std::tr1::shared_ptr<Device> device);


	/**
	* Restart adbd on device in tcpip mode
	*/
	static void restartInTcpip(std::tr1::shared_ptr<Device> device, int port);

	/**
	* Restart adbd on device in usb mode
	*/
	static void restartInUSB(std::tr1::shared_ptr<Device> device);

	AdbHelper();
	virtual ~AdbHelper();
	
	static std::string connectToNetworkDevice(const std::string &address);

	static std::string disconnectFromNetworkDevice(const std::string &address);
};

} /* namespace ddmlib */
#endif /* ADBHELPER_HPP_ */
