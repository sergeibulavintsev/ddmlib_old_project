/*
 * EmulatorConsole.hpp
 *
 *  Created on: 23.03.2012
 *      Author: Sergey Bulavintsev
 */

#ifndef EMULATORCONSOLE_HPP_
#define EMULATORCONSOLE_HPP_

#include "ddmlib.hpp"

namespace ddmlib {

class Device;

/** Gsm Mode enum. */
class DDMLIB_API GsmMode {
	std::vector<std::string> tags;
	static std::vector<GsmMode*> vect;

public:
	GsmMode(const std::string& tag) {
		if (!tag.empty()) {
			tags.push_back(tag);
			vect.push_back(this);
		};
	}

	GsmMode(const std::string& tag, const std::string& tag2) {
		this->tags.push_back(tag);
		this->tags.push_back(tag2);
		vect.push_back(this);
	}

	static GsmMode* getEnum(const std::string& tag) {
		for (std::vector<GsmMode*>::iterator gsm = vect.begin(); gsm != vect.end(); ++gsm) {
			for (std::vector<std::string>::iterator t = (*gsm)->tags.begin(); t != (*gsm)->tags.end(); ++t) {
				if (*t == tag) {
					return *gsm;
				}
			}
		}
		return NULL;
	}

	const GsmMode* getGsm() const { return this; };

	/**
	 * Returns the first tag of the enum.
	 */
	std::string getTag() const {
		if (tags.size() > 0) {
			return tags[0];
		}
		return std::string("");
	}
};

/** Network Status class */
struct DDMLIB_API NetworkStatus {
	NetworkStatus() : speed(-1), latency(-1) {};
    /** network speed status. This is an index in the {@link #DOWNLOAD_SPEEDS} array. */
	int speed;
    /** network latency status.  This is an index in the {@link #MIN_LATENCIES} array. */
    int latency;
};

struct GsmStatus;

/**
 * Provides control over emulated hardware of the Android emulator.
 * <p/>This is basically a wrapper around the command line console normally used with telnet.
 *<p/>
 * Regarding line termination handling:<br>
 * One of the issues is that the telnet protocol <b>requires</b> usage of <code>\r\n</code>. Most
 * implementations don't enforce it (the dos one does). In this particular case, this is mostly
 * irrelevant since we don't use telnet in Java, but that means we want to make
 * sure we use the same line termination than what the console expects. The console
 * code removes <code>\r</code> and waits for <code>\n</code>.
 * <p/>However this means you <i>may</i> receive <code>\r\n</code> when reading from the console.
 * <p/>
 * <b>This API will change in the near future.</b>
 */
class DDMLIB_API EmulatorConsole {

	static const int WAIT_TIME = 5; // spin-wait sleep, in ms

	static const int STD_TIMEOUT = 5000; // standard delay, in ms

	static std::string HOST;

	static std::string COMMAND_PING;
	static std::string COMMAND_AVD_NAME;
	static std::string COMMAND_KILL;
	static std::string COMMAND_GSM_STATUS;
	static std::string COMMAND_GSM_CALL;
	static std::string COMMAND_GSM_CANCEL_CALL;
	static std::string COMMAND_GSM_DATA;
	static std::string COMMAND_GSM_VOICE;
	static std::string COMMAND_SMS_SEND;
	static std::string COMMAND_NETWORK_STATUS;
	static std::string COMMAND_NETWORK_SPEED;
	static std::string COMMAND_NETWORK_LATENCY;
	static std::string COMMAND_GPS;

	static Poco::RegularExpression RE_KO;

	static Poco::RegularExpression sEmulatorRegexp;
	static Poco::RegularExpression sVoiceStatusRegexp;
	static Poco::RegularExpression sDataStatusRegexp;
	static Poco::RegularExpression sDownloadSpeedRegexp;
	static Poco::RegularExpression sMinLatencyRegexp;

	static std::map<int, std::tr1::shared_ptr< EmulatorConsole> > sEmulators;


public:
	/**
	 * Array of delay values: no delay, gprs, edge/egprs, umts/3d
	 */
	static int MIN_LATENCIES[];
	/**
	 * Array of download speeds: full speed, gsm, hscsd, gprs, edge/egprs, umts/3g, hsdpa.
	 */
	static int DOWNLOAD_SPEEDS[];
	/** Arrays of valid network speeds */
	static std::string NETWORK_SPEEDS[];

	/** Arrays of valid network latencies */
	static std::string NETWORK_LATENCIES[];
	static GsmMode UNKNOWN;
	static GsmMode UNREGISTERED;
	static GsmMode HOME;
	static GsmMode ROAMING;
	static GsmMode SEARCHING;
	static GsmMode DENIED;

	static std::string RESULT_OK;


public:
    /**
     * Returns an {@link EmulatorConsole} object for the given {@link Device}. This can
     * be an already existing console, or a new one if it hadn't been created yet.
     * @param d The device that the console links to.
     * @return an <code>EmulatorConsole</code> object or <code>null</code> if the connection failed.
     */
    static std::tr1::shared_ptr< EmulatorConsole > getConsole(std::tr1::shared_ptr< Device > dev);
	/**
	 * Return port of emulator given its serial number.
	 *
	 * @param serialNumber the emulator's serial number
	 * @return the integer port or <code>null</code> if it could not be determined
	 */
	static int getEmulatorPort(const std::string& serialNumber);
	 /**
	     * Sends a KILL command to the emulator.
	     */
	void kill();
	std::string getAvdName();
	/**
	 * Get the network status of the emulator.
	 * @return a {@link NetworkStatus} object containing the {@link GsmStatus}, or
	 * <code>null</code> if the query failed.
	 */
	NetworkStatus getNetworkStatus();
	/**
	 * Returns the current gsm status of the emulator
	 * @return a {@link GsmStatus} object containing the gms status, or <code>null</code>
	 * if the query failed.
	 */
	GsmStatus getGsmStatus();

	/**
	 * Sets the GSM voice mode.
	 * @param mode the {@link GsmMode} value.
	 * @return RESULT_OK if success, an error String otherwise.
	 * @throws InvalidParameterException if mode is an invalid value.
	 */
	std::string setGsmVoiceMode(const GsmMode& mode);
	 /**
	     * Sets the GSM data mode.
	     * @param mode the {@link GsmMode} value
	     * @return {@link #RESULT_OK} if success, an error String otherwise.
	     * @throws InvalidParameterException if mode is an invalid value.
	     */
	std::string setGsmDataMode(const GsmMode& mode);
	/**
	 * Initiate an incoming call on the emulator.
	 * @param number a string representing the calling number.
	 * @return {@link #RESULT_OK} if success, an error String otherwise.
	 */
	std::string call(const std::string& number);
	/**
	 * Cancels a current call.
	 * @param number the number of the call to cancel
	 * @return {@link #RESULT_OK} if success, an error String otherwise.
	 */
	std::string cancelCall(const std::string& number);
	/**
	 * Sends an SMS to the emulator
	 * @param number The sender phone number
	 * @param message The SMS message. \ characters must be escaped. The carriage return is
	 * the 2 character sequence  {'\', 'n' }
	 *
	 * @return {@link #RESULT_OK} if success, an error String otherwise.
	 */
	std::string sendSms(const std::string& number, const std::string& message);
	/**
	 * Sets the network speed.
	 * @param selectionIndex The index in the {@link #NETWORK_SPEEDS} table.
	 * @return {@link #RESULT_OK} if success, an error String otherwise.
	 */
	std::string setNetworkSpeed(int selectionIndex);
	/**
	 * Sets the network latency.
	 * @param selectionIndex The index in the {@link #NETWORK_LATENCIES} table.
	 * @return {@link #RESULT_OK} if success, an error String otherwise.
	 */
	std::string setNetworkLatency(int selectionIndex);
	std::string sendLocation(double longitude, double latitude, double elevation);
	EmulatorConsole();
	EmulatorConsole(int port) : mPort(port) { mBuffer.resize(1024);};
	virtual ~EmulatorConsole();
private:
	/**
	 * Removes the console object associated with a port from the map.
	 * @param port The port of the console to remove.
	 */
	static void RemoveConsole(int port) {
		sEmulators.erase(port);
	}


	/**
	 * Starts the connection of the console.
	 * @return true if success.
	 */
	bool start();
	/**
	 * Ping the emulator to check if the connection is still alive.
	 * @return true if the connection is alive.
	 */
	bool ping();
	/**
	 * Sends a command to the emulator console.
	 * @param command The command string. <b>MUST BE TERMINATED BY \n</b>.
	 * @return true if success
	 */
	bool sendCommand(const std::string& command);
	/**
	 * Sends a command to the emulator and parses its answer.
	 * @param command the command to send.
	 * @return {@link #RESULT_OK} if success, an error message otherwise.
	 */
	std::string processCommand(const std::string& command);
	/**
	 * Reads line from the console socket. This call is blocking until we read the lines:
	 * <ul>
	 * <li>OK\r\n</li>
	 * <li>KO<msg>\r\n</li>
	 * </ul>
	 * @return the array of strings read from the emulator.
	 */
	 std::vector< std::string > readLines();
	/**
	 * Returns true if the 4 characters *before* the current position are "OK\r\n"
	 * @param currentPosition The current position
	 */
	 bool endsWithOK(int currentPosition);
	 /**
	 * Returns true if the last line starts with KO and is also terminated by \r\n
	 * @param currentPosition the current position
	 */
	bool lastLineIsKO(int currentPosition);
	 /**
	     * Returns true if the last line of the result does not start with KO
	     */
	bool isValid(const std::vector< std::string >& result);
	int getLatencyIndex(const std::string& value);
	int getSpeedIndex(const std::string& value);
private:
	int mPort;

	std::tr1::shared_ptr< Poco::Net::StreamSocket > mSocketChannel;

	std::vector <unsigned char> mBuffer;
};

/** Gsm Status class */
struct DDMLIB_API GsmStatus {
	GsmStatus() : voice(&EmulatorConsole::UNKNOWN), data(&EmulatorConsole::UNKNOWN) {};
    /** Voice status. */
    GsmMode* voice;
    /** Data status. */
    GsmMode* data;
};

} /* namespace ddmlib */
#endif /* EMULATORCONSOLE_HPP_ */
