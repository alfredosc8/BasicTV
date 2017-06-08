#ifndef NET_PROTO_INTERFACE_IP_H
#define NET_PROTO_INTERFACE_IP_H

#include "net_proto_interface.h"

/*
  The IP Interface

  This is the main interface everybody is going to use. Currently it
  uses SDL2_net, but i'd like to only use SDL2_net as a fallback, 
  and allow using other, more optimized interfaces for Linux, just
  because that would be the ideal server platform.
 */

#define NET_PROTO_INTERFACE_STATE_FORMAT_IP 1

#define NET_PROTO_INTERFACE_PEER_IP_ADDR_TYPE_IPV4 1
#define NET_PROTO_INTERFACE_PEER_IP_ADDR_TYPE_IPV6 2
#define NET_PROTO_INTERFACE_PEER_IP_ADDR_TYPE_DOMAIN 3

// DNS only goes up to 63, but redirection and whatnot could be
// useful sometime in the future

#define NET_PROTO_INTERFACE_PEER_IP_DOMAIN_MAX_LEN 128


struct net_proto_interface_peer_ip_t{
private:
	uint16_t port = 0;
	uint8_t addr_type = 0;
	std::vector<uint8_t> addr;
public:
	data_id_t id;
	GET_SET(port, uint16_t);
	GET_SET(addr_type, uint8_t);
	GET_SET(addr, std::vector<uint8_t>);
	net_proto_interface_peer_ip_t();
	~net_proto_interface_peer_ip_t();
};

extern NET_PROTO_INTERFACE_CONNECT_STATE(ip);
extern NET_PROTO_INTERFACE_ITERATE_STATE(ip);
extern NET_PROTO_INTERFACE_SEND_STATE(ip);
extern NET_PROTO_INTERFACE_RECV_STATE(ip);
extern NET_PROTO_INTERFACE_CLOSE_STATE(ip);

#endif
