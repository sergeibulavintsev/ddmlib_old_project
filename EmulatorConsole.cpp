/*
 * EmulatorConsole.cpp
 *
 *  Created on: 23.03.2012
 *      Author: Sergey Bulavintsev
 */

#include "ddmlib.hpp"
#include "EmulatorConsole.hpp"
#include "Device.hpp"
#include "AdbHelper.hpp"
#include "DdmPreferences.hpp"
#include "Log.hpp"


namespace ddmlib {

std::string DEFAULT_ENCODING = "ISO-8859-1";

std::string EmulatorConsole::HOST = "127.0.0.1";

std::string EmulatorConsole::COMMAND_PING = "help\r\n";
std::string EmulatorConsole::COMMAND_AVD_NAME = "avd name\r\n";
std::string EmulatorConsole::COMMAND_KILL = "kill\r\n";
std::string EmulatorConsole::COMMAND_GSM_STATUS = "gsm status\r\n";
std::string EmulatorConsole::COMMAND_GSM_CALL = "gsm call %1$s\r\n";
std::string EmulatorConsole::COMMAND_GSM_CANCEL_CALL = "gsm cancel %1$s\r\n";
std::string EmulatorConsole::COMMAND_GSM_DATA = "gsm data %1$s\r\n";
std::string EmulatorConsole::COMMAND_GSM_VOICE = "gsm voice %1$s\r\n";
std::string EmulatorConsole::COMMAND_SMS_SEND = "sms send %1$s %2$s\r\n";
std::string EmulatorConsole::COMMAND_NETWORK_STATUS = "network status\r\n";
std::string EmulatorConsole::COMMAND_NETWORK_SPEED = "network speed %1$s\r\n";
std::string EmulatorConsole::COMMAND_NETWORK_LATENCY = "network delay %1$s\r\n";
std::string EmulatorConsole::COMMAND_GPS = "geo fix %1$f %2$f %3$f\r\n";

Poco::RegularExpression EmulatorConsole::RE_KO("KO:\\s+(.*)");

int EmulatorConsole::MIN_LATENCIES[] = { 0, // No delay
		150, // gprs
		80, // edge/egprs
		35 // umts/3g
		};

int EmulatorConsole::DOWNLOAD_SPEEDS[] = { 0, // full speed
		14400, // gsm
		43200, // hscsd
		80000, // gprs
		236800, // edge/egprs
		1920000, // umts/3g
		14400000 // hsdpa
		};

std::string EmulatorConsole::NETWORK_SPEEDS[] = { "full",
		"gsm",
		"hscsd",
		"gprs",
		"edge",
		"umts",
		"hsdpa",
		};

std::string EmulatorConsole::NETWORK_LATENCIES[] = { "none",
		"gprs",
		"edge",
		"umts",
		};

Poco::RegularExpression EmulatorConsole::sEmulatorRegexp(Device::RE_EMULATOR_SN);
Poco::RegularExpression EmulatorConsole::sVoiceStatusRegexp("gsm\\s+voice\\s+state:\\s*([a-z]+)",
		Poco::RegularExpression::RE_CASELESS);
Poco::RegularExpression EmulatorConsole::sDataStatusRegexp("gsm\\s+data\\s+state:\\s*([a-z]+)",
		Poco::RegularExpression::RE_CASELESS);
Poco::RegularExpression EmulatorConsole::sDownloadSpeedRegexp(
		"\\s+download\\s+speed:\\s+(\\d+)\\s+bits.*", Poco::RegularExpression::RE_CASELESS);
Poco::RegularExpression EmulatorConsole::sMinLatencyRegexp("\\s+minimum\\s+latency:\\s+(\\d+)\\s+ms",
		Poco::RegularExpression::RE_CASELESS);

std::map<int, std::tr1::shared_ptr<EmulatorConsole> > EmulatorConsole::sEmulators;
std::vector<GsmMode*> GsmMode::vect;

GsmMode EmulatorConsole::UNKNOWN(""), EmulatorConsole::UNREGISTERED("unregistered", "off"), EmulatorConsole::HOME(
		"home", "on"), EmulatorConsole::ROAMING("roaming"), EmulatorConsole::SEARCHING("searching"),
		EmulatorConsole::DENIED("denied");

std::string EmulatorConsole::RESULT_OK;

EmulatorConsole::EmulatorConsole() {
	// TODO Auto-generated constructor stub
	mBuffer.resize(1024);
}

EmulatorConsole::~EmulatorConsole() {
	// TODO Auto-generated destructor stub
}

std::tr1::shared_ptr<EmulatorConsole> EmulatorConsole::getConsole(std::tr1::shared_ptr<Device> dev) {
	// we need to make sure that the device is an emulator
	// get the port number. This is the console port.
	int port = getEmulatorPort(dev->getSerialNumber());
	if (port == 0) {
		return std::tr1::shared_ptr<EmulatorConsole>();
	}
	std::tr1::shared_ptr<EmulatorConsole> console = sEmulators[port];

	if (console != nullptr) {
		// if the console exist, we ping the emulator to check the connection.
		if (console->ping() == false) {
			RemoveConsole(console->mPort);
			console.reset();
		}
	}

	if (console == nullptr) {
		// no console object exists for this port so we create one, and start
		// the connection.
		console = std::tr1::shared_ptr<EmulatorConsole>(new EmulatorConsole(port));
		if (console->start()) {
			sEmulators[port] = console;
		} else {
			console.reset();
		}
	}
	return console;
}

int EmulatorConsole::getEmulatorPort(const std::string& serialNumber) {
	std::vector<std::string> mgroup;
	if (sEmulatorRegexp.split(serialNumber, mgroup)) {
		// get the port number. This is the console port.
		int port = 0;
		try {
			port = Poco::NumberParser::parse(mgroup.at(1));
			if (port > 0) {
				return port;
			}
		} catch (Poco::DataFormatException& e) {
			// looks like we failed to get the port number. This is a bit strange since
			// it's coming from a regexp that only accept digit, but we handle the case
			// and return null.
		}
	}

	return 0;
}

bool EmulatorConsole::start() {

	Poco::Net::SocketAddress socketAddr(HOST, mPort);
	try {
		mSocketChannel = std::tr1::shared_ptr<Poco::Net::StreamSocket>(new Poco::Net::StreamSocket(socketAddr));
	} catch (Poco::IOException& e1) {
		return false;
	}

	// read some stuff from it
	readLines();

	return true;
}

bool EmulatorConsole::ping() {
	// it looks like we can send stuff, even when the emulator quit, but we can't read
	// from the socket. So we check the return of readLines()
	if (sendCommand(COMMAND_PING)) {
		return !readLines().empty();
	}

	return false;
}

void EmulatorConsole::kill() {
	if (sendCommand(COMMAND_KILL)) {
		RemoveConsole(mPort);
	}
}

std::string EmulatorConsole::getAvdName() {
	if (sendCommand(COMMAND_AVD_NAME)) {
		std::vector<std::string> result = readLines();
		if (!result.empty() && result.size() == 2) { // this should be the name on first line,
													 // and ok on 2nd line
			return result[0];
		} else {
			// try to see if there's a message after KO
			std::vector<std::string> mgroup;
			if (RE_KO.split(result[result.size() - 1], mgroup)) {
				return mgroup.at(1);
			}
		}
	}

	return std::string("");
}

NetworkStatus EmulatorConsole::getNetworkStatus() {
	if (sendCommand(COMMAND_NETWORK_STATUS)) {
		/* Result is in the format
		 Current network status:
		 download speed:      14400 bits/s (1.8 KB/s)
		 upload speed:        14400 bits/s (1.8 KB/s)
		 minimum latency:  0 ms
		 maximum latency:  0 ms
		 */
		std::vector<std::string> result = readLines();

		if (isValid(result)) {
			// we only compare agains the min latency and the download speed
			// let's not rely on the order of the output, and simply loop through
			// the line testing the regexp.
			NetworkStatus status;
			for (std::vector<std::string>::iterator line = result.begin(); line != result.end(); ++line) {
				std::vector<std::string> mgroup;

				if (sDownloadSpeedRegexp.split(*line, mgroup)) {
					// get the string value
					std::string value = mgroup.at(1);

					// get the index from the list
					status.speed = getSpeedIndex(value);

					// move on to next line.
					continue;
				}

				mgroup.clear();
				if (sMinLatencyRegexp.split(*line, mgroup)) {
					// get the string value
					std::string value = mgroup.at(1);

					// get the index from the list
					status.latency = getLatencyIndex(value);

					// move on to next line.
					continue;
				}
			}

			return status;
		}
	}

	return NetworkStatus();
}

GsmStatus EmulatorConsole::getGsmStatus() {
	if (sendCommand(COMMAND_GSM_STATUS)) {
		/*
		 * result is in the format:
		 * gsm status
		 * gsm voice state: home
		 * gsm data state:  home
		 */

		std::vector<std::string> result = readLines();
		if (isValid(result)) {

			GsmStatus status;

			// let's not rely on the order of the output, and simply loop through
			// the line testing the regexp.
			for (std::vector<std::string>::iterator line = result.begin(); line != result.end(); ++line) {
				std::vector<std::string> mgroup;
				if (sVoiceStatusRegexp.split(*line, mgroup)) {
					// get the string value
					std::string value = mgroup.at(1);

					// get the index from the list
					status.voice = GsmMode::getEnum(Poco::toLower(value));

					// move on to next line.
					continue;
				}

				mgroup.clear();
				if (sDataStatusRegexp.split(*line, mgroup)) {
					// get the string value
					std::string value = mgroup.at(1);

					// get the index from the list
					status.data = GsmMode::getEnum(Poco::toLower(value));

					// move on to next line.
					continue;
				}
			}

			return status;
		}
	}

	return GsmStatus();
}

std::string EmulatorConsole::setGsmVoiceMode(const GsmMode& mode) {
	if (mode.getGsm() == &UNKNOWN) {
		throw Poco::InvalidArgumentException();
	}

	std::string command = "gsm voice " + mode.getTag() + "\r\n";
	return processCommand(command);
}

std::string EmulatorConsole::setGsmDataMode(const GsmMode& mode) {
	if (mode.getGsm() == &UNKNOWN) {
		throw Poco::InvalidArgumentException();
	}

	std::string command = "gsm data " + mode.getTag() + "\r\n";
	return processCommand(command);
}

std::string EmulatorConsole::call(const std::string& number) {
	std::string command = "gsm call " + number + "\r\n";
	return processCommand(command);
}

std::string EmulatorConsole::cancelCall(const std::string& number) {
	std::string command = "gsm cancel " + number + "\r\n";
	return processCommand(command);
}

std::string EmulatorConsole::sendSms(const std::string& number, const std::string& message){
	std::string command = "sms send " + number +" " + message + "\r\n";
	return processCommand(command);
}

std::string EmulatorConsole::setNetworkSpeed(int selectionIndex) {
	std::string command = "network speed " + NETWORK_SPEEDS[selectionIndex] + "\r\n";
	return processCommand(command);
}

std::string EmulatorConsole::setNetworkLatency(int selectionIndex){
	std::string command = "network delay " + NETWORK_LATENCIES[selectionIndex] + "\r\n";
	return processCommand(command);
}

std::string EmulatorConsole::sendLocation(double longitude, double latitude, double elevation) {

	// need to make sure the string format uses dot and not comma
	std::string formatter(
			std::string("geo fix ") + " " + Poco::NumberFormatter::format(longitude) + " " + Poco::NumberFormatter::format(latitude) + " "
					+ Poco::NumberFormatter::format(elevation) + "\r\n");

	return processCommand(formatter);
}

bool EmulatorConsole::sendCommand(const std::string& command) {
	bool result = false;
	try {
		std::vector<unsigned char> bCommand(command.size());
		std::copy(command.begin(), command.end(), bCommand.begin());

		// write the command
		AdbHelper::write(mSocketChannel, bCommand, bCommand.size(), DdmPreferences::getTimeOut());

		result = true;
	} catch (Poco::Exception& e) {
		return false;
	}

	if (result == false) {
		// FIXME connection failed somehow, we need to disconnect the console.
		RemoveConsole(mPort);
	}

	return result;
}

std::string EmulatorConsole::processCommand(const std::string& command) {
	if (sendCommand(command)) {
		std::vector<std::string> result = readLines();

		if (!result.empty() && result.size() > 0) {
			std::vector<std::string> mgroup;
			if (RE_KO.split(result[result.size() - 1], mgroup)) {
				return mgroup.at(1);
			}
			return RESULT_OK;
		}

		return "Unable to communicate with the emulator";
	}

	return "Unable to send command to the emulator";
}

std::vector<std::string> EmulatorConsole::readLines() {
	try {
		char buf[1024] = {0};
		int numWaits = 0;
		int pos = 0;
		bool stop = false;
		mSocketChannel->setReceiveTimeout(WAIT_TIME * 1000);
		while (pos < 1024 && stop == false) {
			int count = 0;
			try {
				if (numWaits > STD_TIMEOUT) {
					return std::vector<std::string>();
				}
				count = mSocketChannel->receiveBytes(buf + pos, 1024);
				if (count < 0) {
					return std::vector<std::string>();
				}
			} catch (Poco::TimeoutException& e) {
				// non-blocking spin
				numWaits += WAIT_TIME;
				Poco::Thread::sleep(WAIT_TIME);
			}
			pos += count;
			// check the last few char aren't OK. For a valid message to test
			// we need at least 4 bytes (OK/KO + \r\n)
			if (pos >= 4) {
				std::copy(buf, buf + pos, mBuffer.begin());
				if (endsWithOK(pos) || lastLineIsKO(pos)) {
					stop = true;
				}
			}
		}
		std::string msg(buf, pos);
		Poco::StringTokenizer token(msg, "\r\n");
		return std::vector<std::string>(token.begin(), token.end());
	} catch (Poco::IOException& e) {
		return std::vector<std::string>();
	}
	return std::vector<std::string>();
}

bool EmulatorConsole::endsWithOK(int currentPosition) {
	if (mBuffer[currentPosition - 1] == '\n' && mBuffer[currentPosition - 2] == '\r' && mBuffer[currentPosition - 3] == 'K'
			&& mBuffer[currentPosition - 4] == 'O') {
		return true;
	}

	return false;
}

bool EmulatorConsole::lastLineIsKO(int currentPosition) {
	// first check that the last 2 characters are CRLF
	if (mBuffer[currentPosition - 1] != '\n' || mBuffer[currentPosition - 2] != '\r') {
		return false;
	}

	// now loop backward looking for the previous CRLF, or the beginning of the buffer
	int i = 0;
	for (i = currentPosition - 3; i >= 0; i--) {
		if (mBuffer[i] == '\n') {
			// found \n!
			if (i > 0 && mBuffer[i - 1] == '\r') {
				// found \r!
				break;
			}
		}
	}

	// here it is either -1 if we reached the start of the buffer without finding
	// a CRLF, or the position of \n. So in both case we look at the characters at i+1 and i+2
	if (mBuffer[i + 1] == 'K' && mBuffer[i + 2] == 'O') {
		// found error!
		return true;
	}

	return false;
}

bool EmulatorConsole::isValid(const std::vector<std::string>& result) {
	if (!result.empty() && result.size() > 0) {
		return !(RE_KO.match(result[result.size() - 1]));
	}
	return false;
}

int EmulatorConsole::getLatencyIndex(const std::string& value) {
	try {
		// get the int value
		int latency = Poco::NumberParser::parse(value);

		// check for the speed from the index
		for (int i = 0; i < 4; i++) {
			if (MIN_LATENCIES[i] == latency) {
				return i;
			}
		}
	} catch (Poco::SyntaxException& e) {
		// Do nothing, we'll just return -1.
	}

	return -1;
}

int EmulatorConsole::getSpeedIndex(const std::string& value) {
	try {
		// get the int value
		int speed = Poco::NumberParser::parse(value);

		// check for the speed from the index
		for (int i = 0; i < 7; i++) {
			if (DOWNLOAD_SPEEDS[i] == speed) {
				return i;
			}
		}
	} catch (Poco::DataFormatException& e) {
		// Do nothing, we'll just return -1.
	}

	return -1;
}

} /* namespace ddmlib */
