#include "net_interface.h"
#include "net_interface_ip.h"

#include "net_interface_helper.h"

static void hardware_software_address_sanity_check(
	net_interface_hardware_dev_t *hardware_dev_ptr,
	net_interface_software_dev_t *software_dev_ptr,
	net_interface_ip_address_t *ip_address_ptr){
	if(software_dev_ptr == nullptr){
		ASSERT(ip_address_ptr->get_medium() == hardware_dev_ptr->get_medium(), P_ERR);
	}else{
		ASSERT(ip_address_ptr->get_required_intermediary() == 0 ||
		       ip_address_ptr->get_required_intermediary() == software_dev_ptr->get_intermediary(), P_ERR);
		ASSERT(hardware_dev_ptr->get_medium() == software_dev_ptr->get_state_format() &&
		       hardware_dev_ptr->get_medium() == ip_address_ptr->get_medium(), P_ERR);
	}
}

// getters and setters for the local-global variables


/*
  The likelihoop that we need to formally call the drop function in IP is
  pretty low, so we just go through and drop the software devices with the
  oldest latest activit
 */

INTERFACE_CALCULATE_MOST_EFFICIENT_DROP(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_ADDR_PTR(address_id);
	hardware_software_address_sanity_check(
		hardware_dev_ptr,
		nullptr,
		ip_address_ptr);
	uint64_t soft_dev_to_remove = 0;
	if(hardware_dev_ptr->get_max_soft_dev() ==
	   hardware_dev_ptr->get_size_soft_dev_list()){
		print("sitting at max_soft_dev limit", P_NOTE);
		soft_dev_to_remove = 1;
	}
	std::vector<id_t_> soft_dev_list =
		hardware_dev_ptr->get_soft_dev_list();
	std::vector<id_t_> retval;
	const uint64_t cur_time_micro_s =
		get_time_microseconds();
	while(retval.size() != soft_dev_to_remove){
		id_t_ preferable_id =
			ID_BLANK_ID;
		uint64_t preferable_diff_micro_s =
			0;
		for(uint64_t i = 0;i < soft_dev_list.size();i++){
			net_interface_software_dev_t *tmp_software_dev_ptr =
				PTR_DATA(soft_dev_list[i],
					 net_interface_software_dev_t);
			CONTINUE_IF_NULL(tmp_software_dev_ptr, P_WARN);
			const uint64_t tmp_diff_micro_s =
				cur_time_micro_s-tmp_software_dev_ptr->get_last_good_inbound_micro_s();
			if(tmp_diff_micro_s > preferable_diff_micro_s &&
			   std::find(retval.begin(), retval.end(), soft_dev_list[i]) == retval.end()){
				preferable_id =
					soft_dev_list[i];
				preferable_diff_micro_s =
					tmp_diff_micro_s;
			}
		}
		retval.push_back(preferable_id);
	}
	return retval;
}

INTERFACE_CALCULATE_MOST_EFFICIENT_TRANSFER(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_ADDR_PTR(address_id);
	/*
	  Until I implement modulation/packet schemes, we are keeping with TCP,
	  and sockets can't be traded between interfaces easily without
	  disconnecting (same for UDP)
	 */
	return {};
}

INTERFACE_SEND(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_SW_PTR(software_dev_id);
	ASSERT(payload.size() != 0, P_ERR);

	
}

INTERFACE_RECV_ALL(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_SW_PTR(software_dev_id);
	return {};
}

INTERFACE_ADD_ADDRESS_COST(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_ADDR_PTR(address_id);
	if(hardware_dev_ptr->get_max_soft_dev() < hardware_dev_ptr->get_size_soft_dev_list()){
		return NET_INTERFACE_HARDWARE_ADD_ADDRESS_FREE;
	}else{
		return NET_INTERFACE_HARDWARE_ADD_ADDRESS_DROP;
	}
}
