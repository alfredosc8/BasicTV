#ifndef NET_INTERFACE_IP_ADDRESS_H
#define NET_INTERFACE_IP_ADDRESS_H

#include "net_interface.h"
#include "net_interface_address.h"

#define NET_INTERFACE_IP_ADDRESS_TYPE_UNDEFINED 0
#define NET_INTERFACE_IP_ADDRESS_TYPE_IPV4 1
#define NET_INTERFACE_IP_ADDRESS_TYPE_IPV6 2
#define NET_INTERFACE_IP_ADDRESS_TYPE_DOMAIN 3

struct net_interface_ip_address_t : public net_interface_address_t{
private:
	// IPv4 is stored as four bytes in NBO
	// IPv6 is stored as sixteen bytes in NBO
	// Domain names are stored in system byte order
	std::vector<uint8_t> address;
	uint8_t address_type = 0;
	uint8_t required_intermediary = 0;
public:
	data_id_t id;
	net_interface_ip_address_t();
	~net_interface_ip_address_t();
	GET(address_type, uint8_t);
	GET(address, std::vector<uint8_t>);
	GET(required_intermediary, uint8_t);
};

#endif
