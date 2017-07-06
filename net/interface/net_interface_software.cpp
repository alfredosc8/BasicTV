#include "net_interface.h"
#include "net_interface_ip.h"
#include "net_interface_tcp.h"
#include "net_interface_hardware.h"
#include "net_interface_software.h"
#include "net_interface_intermediary.h"
#include "net_interface_medium.h"
#include "net_interface_ip_address.h"
#include "../net.h"

#include "net_interface_helper.h"

#include "../../util.h"
#include "../../id/id_api.h"

net_interface_software_dev_t::net_interface_software_dev_t() : id(this, TYPE_NET_INTERFACE_SOFTWARE_DEV_T){
	id.add_data_one_byte_vector_vector(&inbound_data, ~0, ~0);
	id.add_data_one_byte_vector_vector(&outbound_data, ~0, ~0);
	id.add_data_id(&hardware_dev_id, 1);
	id.add_data_id(&reliability_number_set_id, 1);
	id.add_data_id(&address_id, 1);
}

net_interface_software_dev_t::~net_interface_software_dev_t(){
	
}

void net_interface_software_dev_t::set_hardware_dev_id(id_t_ hardware_dev_id_){
	INTERFACE_SET_HW_PTR(hardware_dev_id_);
	hardware_dev_id = hardware_dev_id_;
}

void net_interface_software_dev_t::set_intermediary_id(id_t_ intermediary_id_){
	INTERFACE_SET_INTERMEDIARY_PTR(intermediary_id_);
	intermediary = intermediary_ptr->get_intermediary();
	intermediary_id = intermediary_id_;
}

void net_interface_software_dev_t::set_address_id(id_t_ address_id_){
	INTERFACE_SET_ADDR_PTR(address_id_);
	ASSERT(ip_address_ptr->get_required_intermediary() != NET_INTERFACE_INTERMEDIARY_UNDEFINED, P_ERR);
	intermediary = NET_INTERFACE_INTERMEDIARY_UNDEFINED;
	if(ip_address_ptr->get_required_intermediary() != NET_INTERFACE_INTERMEDIARY_NONE){
		std::vector<id_t_> intermediary_vector =
			id_api::cache::get(
				TYPE_NET_INTERFACE_INTERMEDIARY_T);
		for(uint64_t i = 0;i < intermediary_vector.size();i++){
			net_interface_intermediary_t *intermediary_ptr =
				PTR_DATA(intermediary_vector[i],
					 net_interface_intermediary_t);
			CONTINUE_IF_NULL(intermediary_ptr, P_WARN);
			/*
			  As it stands right now, just search for the first one
			  and just add it. We should be able to pull some
			  traffic information from it as well.
			 */
			if(intermediary_ptr->get_intermediary() ==
			   ip_address_ptr->get_required_intermediary()){
				set_intermediary_id(
					intermediary_vector[i]);
				break;
			}
		}
	}
	if(intermediary == NET_INTERFACE_INTERMEDIARY_UNDEFINED){
		P_V(ip_address_ptr->get_required_intermediary(), P_WARN);
		print("attempted to connect to a client whose address requires an intermediary I don't have", P_ERR);
	}
	address_id = address_id_;
}
