#ifndef NET_IP_H
#define NET_IP_H

#include <array>
#include <cstdint>
#include "../id/id.h"

/*
  net_ip_t: virtual overlay for any type needing an IP

  Polymorphism is used as a general interface among programs, where
  it is critical to the usage, while not needing
 */

// TODO: make this more legit

#define NET_IP_FMT_DOMAIN 1
#define NET_IP_FMT_IPV4 2
#define NET_IP_FMT_IPV6 3

struct net_ip_t{
protected:
	/*
	  Converts to the binary version (unless domain name)
	 */
	std::array<uint8_t, 64> address;
	uint16_t port = 0;
	uint8_t type = 0;
public:
	net_ip_t();
	~net_ip_t();
	void list_virtual_data(data_id_t *id);
	void set_net_ip(std::string ip_,
			uint16_t port_);
	std::string get_net_ip_str();
	std::string get_address();
	uint16_t get_net_port();
};

#endif
