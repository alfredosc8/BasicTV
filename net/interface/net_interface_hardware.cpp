#include "net_interface.h"
#include "net_interface_ip.h"
#include "net_interface_tcp.h"
#include "net_interface_hardware.h"
#include "net_interface_software.h"
#include "net_interface_intermediary.h"
#include "net_interface_medium.h"
#include "net_interface_ip_address.h"

#include "../net.h"

net_interface_hardware_dev_t::net_interface_hardware_dev_t() : id(this, TYPE_NET_INTERFACE_HARDWARE_DEV_T){
	SIMPLE_ADD(max_soft_dev);
	SIMPLE_ADD(outbound_transport_type);
	SIMPLE_ADD(inbound_transport_type);
	SIMPLE_ADD(outbound_transport_flags);
	SIMPLE_ADD(inbound_transport_flags);
	SIMPLE_ADD(medium);

	id.add_data_one_byte_vector(&soft_dev_list, ~0);
	id.add_data_id(&inbound_throughput_number_set_id, 1);
	id.add_data_id(&outbound_throughput_number_set_id, 1);
}

net_interface_hardware_dev_t::~net_interface_hardware_dev_t(){
}


