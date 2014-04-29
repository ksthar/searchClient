#include "glue/sd_proxy-glue.h"

#include <com_bluegiga_v2_bt.h>
#include <signal.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <stdlib.h>
#include <getopt.h>

#include <unistd.h>
#include "util.h"

#define APPHANDLE 0

using namespace com::bluegiga::v2::bt;

class SdClient;

// Globabls to keep code clean and simple
DBus::BusDispatcher dispatcher;
SdClient *client = NULL;
bool interrupted = false;
bool shortoutput = true;

class SdClient: public sd_proxy, public DBus::IntrospectableProxy, public DBus::ObjectProxy {
  public:
	SdClient(DBus::Connection &connection, const char *path, const char *name):
		DBus::ObjectProxy(connection, path, name)
	{}

	virtual void SearchResultInd(const std::string& deviceAddr,
				     const uint32_t& deviceClass,
				     const int16_t& rssi,
				     const std::map< uint16_t, ::DBus::Variant >& info,
				     const uint32_t& deviceStatus) {

		// Print Bluetooth address, device class and RSSI
		if (!shortoutput) {
			std::cout << "-----------------------------" << std::endl;
			std::cout << "  \033[32m" << deviceAddr << "\033[37m" << " 0x" << std::setw(6) <<
				     std::setfill('0') << std::hex << deviceClass <<
				     " " << std::dec << "\033[34m" << rssi << "\033[37m dBm" << std::endl;
		} else {
			std::string name;
			std::map< uint16_t, ::DBus::Variant >::const_iterator i;
			i = info.find(BT_EIR_DATA_TYPE_COMPLETE_LOCAL_NAME);
			if (i != info.end())
				name = i->second.reader().get_string();
			std::time_t t = std::time(0);
			std::cout << "| " << t << " | " << deviceAddr <<
				     " | " << "\033[32m" << name << "\033[37m" << " | " <<
				     rssi << " | " << std::endl;
			return;
		}

		// Display extra information with '--long'
		for (std::map< uint16_t, ::DBus::Variant >::const_iterator i = info.begin(); i != info.end(); i++) {
			switch(i->first) {
				case BT_EIR_DATA_TYPE_MORE_128_BIT_UUID:
				{
					std::cout << "  PARTIAL LIST OF SERVICES:" << std::endl;
					std::vector< std::vector< uint8_t > > uuids;
					::DBus::MessageIter ri = i->second.reader();
					ri >> uuids;
					print_uuid128s(uuids);
					break;
				}
				case BT_EIR_DATA_TYPE_COMPLETE_128_BIT_UUID:
				{
					std::cout << "  \033[32mLIST OF SERVICES:\033[37m" << std::endl;
					std::vector< std::vector< uint8_t > > uuids;
					::DBus::MessageIter ri = i->second.reader();
					ri >> uuids;
					print_uuid128s(uuids);
					break;
				}
				case BT_EIR_DATA_TYPE_SHORT_LOCAL_NAME: // Incomplete/truncated name
					std::cout << "  PARTIAL NAME: \"" << i->second.reader().get_string() << "\"..." << std::endl;
					break;
				case BT_EIR_DATA_TYPE_COMPLETE_LOCAL_NAME:
					std::cout << "  \033[32mNAME:\033[37m \"" << i->second.reader().get_string() << "\"" << std::endl;
					break;
				case BT_EIR_DATA_TYPE_TX_POWER:
					std::cout << "  \033[31mTX POWER LEVEL:\033[37m" << i->second.reader().get_int16() << std::endl;
					break;
				default:
				{
					if (i->first == BT_EIR_DATA_TYPE_DEVICE_ID)
						//std::cout << "  DEVICE ID DATA: " << std::endl;
						std::cout << "  \033[32mDEVICE ID DATA:\033[37m ";
					else if (i->first == BT_EIR_DATA_TYPE_MANUFACTURER_SPECIFIC)
						//std::cout << "  \033[32mMANUFACTURER DATA:\033[37m " << std::endl;
						std::cout << "  \033[32mMANUFACTURER DATA:\033[37m " ;
					else
						std::cout << "  \033[32mOTHER\033[37m (0x" << std::hex << i->first << std::dec << ") " ;

					std::vector< uint8_t > data;
					::DBus::MessageIter ri = i->second.reader();
					ri >> data;
					print_data(data);
					break;
				}
			} /* switch */
		} /* for */
	} /* SearchResultInd */

	virtual void CloseSearchInd(const uint16_t& resultCode, const uint16_t& resultSupplier) {
		if (!shortoutput)
		{
			std::cout << "-----------------------------" << std::endl;
			std::cout << ">> Service discovery ended" << std::endl;
		}
		dispatcher.leave(0);
		std::cout << "  DEBUG>> returned from dispatcher" << std::endl;
	}
};

void interrupt_signal_handler(int sig){
	if (!interrupted) {
		std::cout << ">> Canceling service discovery" << std::endl;
		client->CancelSearchReq(APPHANDLE);
		interrupted = true;
	} else {
		std::cerr << ">> Caught second interrupt. Exiting. " << std::endl;
		exit(EXIT_FAILURE);
	}
}

int usage(const char *name) {
	std::cout << "Usage: " << name << " [parameter(s)]" << std::endl;
	std::cout << std::endl;
	std::cout << "  --help          this help" << std::endl;
	std::cout << "  --le            scan also LE devices" << std::endl;
	std::cout << "  --le-only       scan only LE devices" << std::endl;
	std::cout << "  --totaltime s   total search time in seconds (default 10s)" << std::endl;
	std::cout << "  --rssitime s    rssi buffer time in seconds (default 3s)" << std::endl;
	std::cout << "  --loop          run forever" << std::endl;
	std::cout << "  --long          use long output format" << std::endl;
	std::cout << "  --name          do name queries" << std::endl;

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	int totaltime = 10000, rssitime = totaltime / 3, i = 0;
	unsigned int flags = BT_SD_SEARCH_USE_STANDARD |
		    BT_SD_SEARCH_DISABLE_NAME_READING |
		    BT_SD_SEARCH_SHOW_UNKNOWN_DEVICE_NAMES |
		    BT_SD_SEARCH_DISABLE_BT_LE |
		    BT_SD_SEARCH_ALLOW_UNSORTED_SEARCH_RESULTS;
	bool loop = false;

	static struct option long_options[] = {
		{ "help",      0, 0, 'h' },
		{ "le",        0, 0, 'l' },
		{ "le-only",   0, 0, 'o' },
		{ "totaltime", 1, 0, 't' },
		{ "rssitime",  1, 0, 'r' },
		{ "loop",      0, 0, 'x' },
		{ "long",      0, 0, 's' },
		{ "name",      0, 0, 'n' },
		{ 0,           0, 0, 0   },
	};

	std::cout << "\033[36mHar's Hacked searchclient\033[37m" << std::endl;

	while (i != -1) {
		i = getopt_long(argc, argv, "hlot:r:xsn", long_options, NULL);
		switch (i) {
		case 'h':
			return usage(argv[0]);
			break;
		case 'l':
			flags &= ~BT_SD_SEARCH_DISABLE_BT_LE;
			break;
		case 'o':
			flags |= BT_SD_SEARCH_DISABLE_BT_CLASSIC;
			flags &= ~BT_SD_SEARCH_DISABLE_BT_LE;
			break;
		case 't':
			totaltime = atoi(optarg) * 1000;
			break;
		case 'r':
			rssitime = atoi(optarg) * 1000;
			break;
		case 'x':
			loop = true;
			break;
		case 's':
			shortoutput = false;
			break;
		case 'n':
			flags &= ~BT_SD_SEARCH_DISABLE_NAME_READING;
			flags |= BT_SD_SEARCH_FORCE_NAME_UPDATE;
			break;
		case '?':
			return usage(argv[0]);
			break;
		}
	}

	// Simple sanity check
	if (totaltime < 0 || rssitime < 0) {
		std::cout << ">> Please check parameters!" << std::endl << std::endl;
		return usage(argv[0]);
	}

	DBus::default_dispatcher = &dispatcher;
	DBus::Connection *conn = new DBus::Connection(DBus::Connection::SystemBus());
	client = new SdClient(*conn, "/bt/sd", "com.bluegiga.v2.bt0");

	signal(SIGINT, interrupt_signal_handler);

	do {
		if (!shortoutput)
			std::cout << ">> Starting service discovery" << std::endl;

		client->SearchReq(APPHANDLE,
			flags,
			rssitime, /* buffer, default 3sec */
			totaltime, /* total time, default 10sec */
			BT_SD_RSSI_THRESHOLD_DONT_CARE,
			0, /* cod filter */
			0, /* cod filter mask, 0 = no mask */
			BT_SD_ACCESS_CODE_GIAC,
			std::vector< uint8_t >());

		std::cout << "  DEBUG>> entering dbus dispatcher" << std::endl;
		dispatcher.enter();
	} while (loop && !interrupted);

	return EXIT_SUCCESS;
} /* main */
