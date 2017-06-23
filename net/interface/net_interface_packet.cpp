#include "net_interface.h"
#include "net_interface_packet.h"
#include "net_interface_tcp.h"
#include "net_interface_medium.h"

net_interface_medium_packet_t medium_packet_array[NET_INTERFACE_MEDIUM_PACKET_COUNT] =
{
	net_interface_medium_packet_t(
		net_interface_ip_tcp_packetize,
		net_interface_ip_tcp_depacketize,
		~0) // MTU here means we don't need to set a max size per vector
};

void sanity_check_medium_modulation_encapsulation(
	uint8_t medium, 
	uint8_t modulation,
	uint8_t encapsulation){
	ASSERT((medium == NET_INTERFACE_MEDIUM_IP &&
		modulation == NET_INTERFACE_MEDIUM_PACKET_MODULATION_TCP &&
		encapsulation == NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_TCP) ||
	       (medium == NET_INTERFACE_MEDIUM_IP &&
		modulation == NET_INTERFACE_MEDIUM_PACKET_MODULATION_UDP &&
		encapsulation == NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_UDP_ORDERED), P_ERR);
	// i'm not bothering with radio yet
	
}

net_interface_medium_packet_t medium_packet_lookup(
	uint8_t medium,
	uint8_t modulation,
	uint8_t encapsulation){
	for(uint8_t i = 0;i < NET_INTERFACE_MEDIUM_PACKET_COUNT;i++){
		if(medium_packet_array[i].get_medium() == medium &&
		   medium_packet_array[i].get_packet_modulation() == modulation &&
		   medium_packet_array[i].get_packet_encapsulation() == encapsulation){
			return medium_packet_array[i];
		}
	}
	print("no valid packet function set found", P_ERR);
	return net_interface_medium_packet_t(
		nullptr, nullptr, 0);
}
