#include "net_interface.h"
#include "net_interface_medium.h"
#include "net_interface_packet.h"
#include "net_interface_address.h"

void net_interface_address_t::set_medium_modulation_encapsulation(
	uint8_t medium_,
	uint8_t modulation_,
	uint8_t encapsulation_){
	sanity_check_medium_modulation_encapsulation(
		medium_,
		modulation_,
		encapsulation_);
	medium = medium_;
	packet_modulation = modulation_;
	packet_encapsulation = encapsulation_;
}
