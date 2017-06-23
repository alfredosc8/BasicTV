#include "net_interface_packet.h"

void sanity_check_modulation_and_encapsulation(
	uint8_t modulation,
	uint8_t encapsulation){
	ASSERT((modulation == NET_INTERFACE_MEDIUM_PACKET_MODULATION_TCP &&
		encapsulation == NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_TCP) ||
	       (modulation == NET_INTERFACE_MEDIUM_PACKET_MODULATION_UDP &&
		encapsulation == NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_UDP_ORDERED), P_ERR);
	// i'm not bothering with radio yet
	
}
