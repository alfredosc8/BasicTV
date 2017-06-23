#ifndef NET_INTERFACE_SOFTWARE_H
#define NET_INTERFACE_SOFTWARE_H

#include "net_interface_medium.h"
#include "net_interface_packet.h"

struct net_interface_software_dev_t{
private:
	std::vector<std::vector<uint8_t> > inbound_data;
	uint64_t last_good_inbound_micro_s = 0;
	std::vector<std::vector<uint8_t> > outbound_data;
	uint64_t last_good_outbound_micro_s = 0;

	uint64_t max_packet_size_by_software = 0;

	uint8_t medium = 0;
	uint8_t packet_modulation = 0;
	uint8_t packet_encapsulation = 0;

	void *state_ptr = nullptr;
	
	id_t_ hardware_dev_id = ID_BLANK_ID;	
	id_t_ reliability_number_set_id = ID_BLANK_ID;
	id_t_ address_id = ID_BLANK_ID;

	// intermediary uint8_t is set on changes to intermediary_id
	id_t_ intermediary_id = ID_BLANK_ID;
	uint8_t intermediary = 0;
public:
	data_id_t id;
	net_interface_software_dev_t();
	~net_interface_software_dev_t();
	GET(last_good_inbound_micro_s, uint64_t);
	GET(last_good_outbound_micro_s, uint64_t);

	GET(state_ptr, void*);
	
	void set_hardware_dev_id(id_t_ hardware_dev_id_);
	GET_ID(hardware_dev_id);

	void set_address_id(id_t_ address_id_);
	GET_ID(address_id);
	GET(packet_modulation, uint8_t);
	GET(packet_encapsulation, uint8_t);

	void set_intermediary_id(id_t_ intermediary_id_);
	GET_ID(intermediary_id);
	GET(intermediary, uint8_t);

	GET(medium, uint8_t);
	
	GET_ID(reliability_number_set_id);

	GET_SET(inbound_data, std::vector<std::vector<uint8_t> >);
	ADD_DEL_VECTOR(inbound_data, std::vector<uint8_t>);
	GET_SET(outbound_data, std::vector<std::vector<uint8_t> >);
	ADD_DEL_VECTOR(outbound_data, std::vector<uint8_t>);
};

#endif
