#include "net_interface.h"
#include "net_interface_ip.h"
#include "net_interface_tcp.h"

net_interface_medium_t medium_array[NET_INTERFACE_MEDIUM_COUNT] =
{
	net_interface_medium_t(
	        net_interface_ip_add_address_cost,
		net_interface_ip_calculate_most_efficient_drop,
		net_interface_ip_calculate_most_efficient_transfer,
		net_interface_ip_send,
		net_interface_ip_recv_all)
};

net_interface_medium_packet_t medium_packet_array[NET_INTERFACE_MEDIUM_PACKET_COUNT] =
{
	net_interface_medium_packet_t(
		net_interface_ip_tcp_packetize,
		net_interface_ip_tcp_depacketize,
		~0) // MTU here means we don't need to set a max size per vector
};

net_interface_medium_t interface_medium_lookup(uint8_t medium){
	ASSERT(medium == NET_INTERFACE_MEDIUM_UNDEFINED, P_ERR);
	return medium_array[medium-1];
}

