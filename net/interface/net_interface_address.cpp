#include "net_interface.h"
#include "net_interface_medium.h"
#include "net_interface_packet.h"
#include "net_interface_address.h"

#pragma message("times for availability should be set with a conversion function from HH:MM:SS to offsets from midnight UTC")

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

void net_interface_address_t::list_virtual_data(data_id_t *id){
	id->add_data_raw(&medium, sizeof(medium));
	id->add_data_raw(&packet_modulation, sizeof(packet_modulation));
	id->add_data_raw(&packet_encapsulation, sizeof(packet_encapsulation));
}
