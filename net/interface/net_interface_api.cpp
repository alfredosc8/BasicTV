#include "net_interface_helper.h"
#include "net_interface_api.h"
#include "net_interface.h"

#include "../../util.h"
#include "../../id/id_api.h"

/*
  An address technically isn't bound to the hardware, but a newly created
  software socket is (effectively a socket)
 */

id_t_ net_interface::bind::address_to_hardware(
	id_t_ address_id,
	id_t_ hardware_dev_id){

	net_interface_hardware_dev_t *hardware_dev_ptr =
		PTR_DATA(hardware_dev_id,
			 net_interface_hardware_dev_t);
	PRINT_IF_NULL(hardware_dev_ptr, P_ERR);
	ASSERT(address_id != ID_BLANK_ID, P_ERR);
	net_interface_medium_t medium =
		interface_medium_lookup(
			NET_INTERFACE_MEDIUM_IP);
	const uint8_t cost_of_addition =
		medium.add_address_cost(
			hardware_dev_id,
			address_id);
	if(cost_of_addition != NET_INTERFACE_HARDWARE_ADD_ADDRESS_FREE){
		P_V(cost_of_addition, P_NOTE);
		print("address addition comes at a cost to the chosen hardware device, remove old connections first", P_ERR);
	}
	
	net_interface_software_dev_t *software_dev_ptr =
		new net_interface_software_dev_t;
	software_dev_ptr->set_address_id(
		address_id);
	software_dev_ptr->set_hardware_dev_id(
		hardware_dev_ptr->id.get_id());
	hardware_dev_ptr->add_soft_dev_list(
		software_dev_ptr->id.get_id());
	return software_dev_ptr->id.get_id();
}

void net_interface::unbind::software_to_hardware(
	id_t_ software_dev_id,
	id_t_ hardware_dev_id){
	net_interface_software_dev_t *software_dev_ptr =
		PTR_DATA(software_dev_id,
			 net_interface_software_dev_t);
	net_interface_hardware_dev_t *hardware_dev_ptr =
		PTR_DATA(hardware_dev_id,
			 net_interface_hardware_dev_t);
	PRINT_IF_NULL(software_dev_ptr, P_ERR);
	PRINT_IF_NULL(hardware_dev_ptr, P_ERR);
}
	
