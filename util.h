#ifndef __util_h__
#define __util_h__

#include <vector>

#include <stdint.h>
#include <stddef.h>

void print_uuid128s(const std::vector<std::vector< uint8_t > > &uuids);
void print_data(const std::vector< uint8_t > &data);
#endif
