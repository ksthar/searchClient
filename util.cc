#include <iostream>
#include <iomanip>
#include "util.h"

void print_uuid128s(const std::vector<std::vector< uint8_t > > &uuids) {
	std::cout << std::hex << std::setfill('0');
	for (std::vector< std::vector< uint8_t > >::const_iterator uuid_iter = uuids.begin(); uuid_iter != uuids.end(); ++uuid_iter) {
		const std::vector< uint8_t > &uuid = *uuid_iter;
		std::cout << "    ";
		for(int i = 0; i < 4; ++i)
			std::cout << std::setw(2) << (int)uuid[i];
		std::cout << "-";
		for(int a = 0; a < 3; ++a) {
			for(int i = 4 + a * 2; i < 6 + a * 2; ++i) 
				std::cout << std::setw(2) << (int)uuid[i];
			std::cout << "-";
		}
		for(int i = 10; i < 16; ++i) 
			std::cout << std::setw(2) <<(int)uuid[i];
		std::cout << std::endl;
	}
	std::cout << std::dec << std::setfill(' ');
}

void print_data(const std::vector< uint8_t > &data) {
	std::cout << std::hex << std::setfill('0');
	for (std::vector< uint8_t >::const_iterator data_iter = data.begin(); data_iter != data.end(); ++data_iter)
		std::cout << std::setw(2) << (int)*data_iter << " ";
	std::cout << std::endl; 
	std::cout << std::dec << std::setfill(' ');
}

