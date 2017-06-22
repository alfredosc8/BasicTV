#include "net_interface.h"

static void hardware_software_address_sanity_check(
	net_interface_hardware_dev_t *hardware_dev_ptr,
	net_interface_software_dev_t *software_dev_ptr,
	net_interface_ip_address_t *ip_address_ptr){
	ASSERT(ip_address_ptr->get_required_intermediary() == 0 ||
	       ip_address_ptr->get_required_intermediary() == software_dev_ptr->get_intermediary(), P_ERR);
	ASSERT(hardware_dev_ptr->get_medium() == software_dev_ptr->get_state_format() &&
	       hardware_dev_ptr->get_medium() == ip_address_ptr->get_medium(), P_ERR);
}

// getters and setters for the local-global variables

#define IP_SET_HW_PTR(x)					\
	net_interface_hardware_dev_t *hardware_dev_ptr =	\
		PTR_DATA(x,					\
			 net_interface_hardware_dev_t);		\
	PRINT_IF_NULL(hardware_dev_ptr, P_ERR);			\
	
#define IP_SET_SW_PTR(x)					\
	net_interface_software_dev_t *software_dev_ptr =	\
		PTR_DATA(x,					\
			 net_interface_software_dev_t);		\
	PRINT_IF_NULL(software_dev_ptr, P_ERR);			\
	
#define IP_SET_ADDR_PTR(x)					\
	net_interface_ip_address_t *ip_address_ptr =		\
		PTR_DATA(x,					\
			 net_interface_ip_address_t);		\
	PRINT_IF_NULL(ip_address_ptr, P_ERR);			\


std::vector<id_t_> ip_calculate_most_efficient_drop(
	id_t_ hardware_dev_id,
	id_t_ software_dev_id){
	
	IP_SET_HW_PTR(hardware_dev_id);
	IP_SET_SW_PTR(software_dev_id);
	IP_SET_ADDR_PTR(software_dev_ptr->get_address_id());
	hardware_software_address_sanity_check(
		hardware_dev_ptr,
		software_dev_ptr,
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

void ip_perform_drop(
	std::vector<id_t_> drop_vector,
	id_t_ hardware_dev_id){
	net_interface_hardware_dev_t *hardware_dev_ptr =
		PTR_DATA(hardware_dev_id,
			 net_interface_hardware_dev_t);
	PRINT_IF_NULL(hardware_dev_ptr, P_ERR);
	for(uint64_t i = 0;i < drop_vector.size();i++){
		
	}
}
