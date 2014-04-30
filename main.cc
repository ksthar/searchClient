#include "glue/sd_proxy-glue.h"
#include "glue/gatt_proxy-glue.h"

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
class GattClient;

// Globabls to keep code clean and simple
DBus::BusDispatcher dispatcher;
SdClient *sdClient = NULL;
GattClient *gattClient = NULL;
bool interrupted = false;
bool shortoutput = true;
// create an array to load our discovered addresses in
std::string addr[ 25 ];
int addrIdx = 0;
// create a gattId
uint32_t currGattId = 0;

// -----------------------------------------------------------------------------------------------------
// SdClient 
//
// The search discovery class, finds BLE advertisements
// -----------------------------------------------------------------------------------------------------
class SdClient: public sd_proxy, public DBus::IntrospectableProxy, public DBus::ObjectProxy 
{
  public:
	SdClient(DBus::Connection &connection, const char *path, const char *name):
		DBus::ObjectProxy(connection, path, name)
	{}

	// This catches the response from our SearchReq issued in main and prints crap out...
	virtual void SearchResultInd(const std::string& deviceAddr,
				     const uint32_t& deviceClass,
				     const int16_t& rssi,
				     const std::map< uint16_t, ::DBus::Variant >& info,
				     const uint32_t& deviceStatus) 
	{

		// Print Bluetooth address, device class and RSSI
		if (!shortoutput) {
			std::cout << "-----------------------------" << std::endl;
			std::cout << "  \033[32m" << deviceAddr << "\033[37m" << " 0x" << std::setw(6) <<
				     std::setfill('0') << std::hex << deviceClass <<
				     " " << std::dec << "\033[34m" << rssi << "\033[37m dBm" << std::endl;
			// Add the address to our array so we can connect to it later...
			addr[ addrIdx ] = deviceAddr;
			std::cout << "  \033[32mADDRESS LOADED IN ARRAY:\033[37m" << addr[ addrIdx ] << std::endl;

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
		} /* if !shortoutput */

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
}; /* class SdClient */

// -----------------------------------------------------------------------------------------------------
// GattClient 
//
// The Gatt class: actually connects to BLE devices
// -----------------------------------------------------------------------------------------------------
class GattClient: public gatt_proxy, public DBus::IntrospectableProxy, public DBus::ObjectProxy 
{
  public:
	GattClient(DBus::Connection &connection, const char *path, const char *name):
		DBus::ObjectProxy(connection, path, name)
	{}

	public:
	// Below are the DBus signal handlers...
	
	// I think CentralReq creates a connection...we'll see
	virtual void CentralCfm( 
			const uint32_t& gattId, 
			const uint32_t& address, 
			const uint16_t& flags, 
			const uint16_t& preferredMtu )
	{
		// do something here like print out some stuff...
		std::cout << "  >> Connected to " << address << std::endl;
		dispatcher.leave(0);
		std::cout << "  DEBUG>> Left dispatcher " << std::endl;
	}

	// This is the reply from our RegisterReq, giving us a gattId
	virtual void RegisterCfm( 
			const uint32_t& gattId, 
			const uint16_t& resultCode, 
			const uint16_t& resultSupplier, 
			const uint16_t& context )
	{
		// do something else here...  Like print the gattId to confirm we have it.
		std::cout << "  >> App has been registered as " << gattId << std::endl;
		currGattId = gattId;
		dispatcher.leave(0);
		std::cout << "  DEBUG>> Left dispatcher " << std::endl;
	}

	// This is the reply from our UnregisterReq, which should release whatever insane
	// resources we reserved with the RegisterReq...
	virtual void UnregisterCfm( 
			const uint32_t& gattId, 
			const uint16_t& resultCode, 
			const uint16_t& resultSupplier )
	{
		// do something else here...  Like print the resultCode or something.
		std::cout << "  >> App with gattId=" << gattId <<" has been unregistered" << std::endl;
		dispatcher.leave(0);
		std::cout << "  DEBUG>> Left dispatcher " << std::endl;
	}

}; /* class GattClient */
// -----------------------------------------------------------------------------------------------------

// Capture ctrl-c
void interrupt_signal_handler(int sig){
	if (!interrupted) {
		std::cout << ">> Canceling service discovery" << std::endl;
		sdClient->CancelSearchReq(APPHANDLE);
		interrupted = true;
	} else {
		std::cerr << ">> Caught second interrupt. Exiting. " << std::endl;
		exit(EXIT_FAILURE);
	}
}

// help
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

// -----------------------------------------------------------------------------------------------------
// main
//
// -----------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	int totaltime = 10000, rssitime = totaltime / 3, i = 0;
	unsigned int flags = BT_SD_SEARCH_USE_STANDARD |
		    BT_SD_SEARCH_DISABLE_NAME_READING |
		    BT_SD_SEARCH_SHOW_UNKNOWN_DEVICE_NAMES |
		    BT_SD_SEARCH_DISABLE_BT_LE |
		    BT_SD_SEARCH_ALLOW_UNSORTED_SEARCH_RESULTS;
	bool loop = false;

	// HAR NOTE: we'll need to preselect these...
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

	std::cout << "\033[36mHar's New and Improved Hacked searchclient\033[37m" << std::endl;

	// handle the command line options...
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
		std::cout << ">> Please check parameters, dummy!" << std::endl << std::endl;
		return usage(argv[0]);
	}

	// Setup the DBus dispatcher
	DBus::default_dispatcher = &dispatcher;

	// Create DBus connections for our class instances...
	DBus::Connection *sd_conn = new DBus::Connection(DBus::Connection::SystemBus());
	sdClient = new SdClient(*sd_conn, "/bt/sd", "com.bluegiga.v2.bt0");
	DBus::Connection *gatt_conn = new DBus::Connection(DBus::Connection::SystemBus());
	gattClient = new GattClient(*gatt_conn, "/bt/gatt", "com.bluegiga.v2.bt0");

	// Register our app...
	uint16_t context = 0;
	gattClient->RegisterReq( context );
	std::cout << "  DEBUG>> entering dbus dispatcher for RegisterReq" << std::endl;
	dispatcher.enter();

	signal(SIGINT, interrupt_signal_handler);
	addrIdx = 0;

	do {
		if (!shortoutput)
			std::cout << ">> Starting service discovery" << std::endl;

		sdClient->SearchReq(APPHANDLE,
			flags,
			rssitime, /* buffer, default 3sec */
			totaltime, /* total time, default 10sec */
			BT_SD_RSSI_THRESHOLD_DONT_CARE,
			0, /* cod filter */
			0, /* cod filter mask, 0 = no mask */
			BT_SD_ACCESS_CODE_GIAC,
			std::vector< uint8_t >());

		std::cout << "  DEBUG>> entering dbus dispatcher for SearchReq" << std::endl;
		dispatcher.enter();

	} while (loop && !interrupted);

	gattClient->CentralReq( currGattId, addr[ 0 ], 0, 0 );
	std::cout << "  DEBUG>> entering dbus dispatcher for CentralReq" << std::endl;
	dispatcher.enter();

	gattClient->UnregisterReq( currGattId );
	std::cout << "  DEBUG>> entering dbus dispatcher for UnregisterReq" << std::endl;
	dispatcher.enter();

	return EXIT_SUCCESS;
} /* main */
