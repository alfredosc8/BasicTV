#ifndef NET_INTERFACE_IP_ADDRESS_H
#define NET_INTERFACE_IP_ADDRESS_H

#include "net_interface.h"
#include "net_interface_address.h"

#define NET_INTERFACE_IP_ADDRESS_TYPE_UNDEFINED 0
#define NET_INTERFACE_IP_ADDRESS_TYPE_IPV4 1
#define NET_INTERFACE_IP_ADDRESS_TYPE_IPV6 2
#define NET_INTERFACE_IP_ADDRESS_TYPE_DOMAIN 3

#define NET_INTERFACE_IP_ADDRESS_NAT_TYPE_UNDEFINED 0
#define NET_INTERFACE_IP_ADDRESS_NAT_TYPE_NONE 1
#define NET_INTERFACE_IP_ADDRESS_NAT_TYPE_FULL_CONE 2
#define NET_INTERFACE_IP_ADDRESS_NAT_TYPE_RESTRICTED_CONE 3
#define NET_INTERFACE_IP_ADDRESS_NAT_TYPE_PORT_RESTRICTED_CONE 4
#define NET_INTERFACE_IP_ADDRESS_NAT_TYPE_SYMMETRIC 5


/*
  Remember that net_interface_address_t covers the mediums being used,
  modulation, and encapsulation scheme identifiers
 */

struct net_interface_ip_address_t : public net_interface_address_t{
private:
	// IPv4 is stored as four bytes in NBO
	// IPv6 is stored as sixteen bytes in NBO
	// Domain names are stored in system byte order
	std::vector<uint8_t> address;
	uint8_t address_type = 0;
	uint8_t nat_type = 0;
	uint16_t port = 0;
public:
	data_id_t id;
	net_interface_ip_address_t();
	~net_interface_ip_address_t();
	void set_address_data(
		std::string address_,
		uint16_t port_,
		uint8_t nat_type_);
	std::pair<std::vector<uint8_t>, uint8_t> get_address();
	GET(nat_type, uint8_t);
	GET(address_type, uint8_t);
	GET(port, uint16_t);
};

#endif
