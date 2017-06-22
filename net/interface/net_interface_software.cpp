#include "net_interface.h"
#include "../net.h"

net_interface_software_dev_t::net_interface_software_dev_t() : id(this, TYPE_NET_INTERFACE_SOFTWARE_DEV_T){
	id.add_data_one_byte_vector(&inbound_data, ~0);
	id.add_data_one_byte_vector(&outbound_data, ~0);
	id.add_data_id(&hardware_dev_id, 1);
	id.add_data_id(&reliability_number_set_id, ~0);
	id.add_data_id(&address_id, ~0);
}

net_interface_software_dev_t::~net_interface_software_dev_t(){
	
}
