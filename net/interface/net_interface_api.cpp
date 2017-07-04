#include "net_interface_helper.h"
#include "net_interface_api.h"
#include "net_interface_ip.h"
#include "net_interface_ip_address.h"
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
	
uint8_t net_interface::medium::from_address(id_t_ address_id){
	// more efficient for sure
	if(address_id == ID_BLANK_ID){
		print("address to derive medium from is blank", P_UNABLE);
		return NET_INTERFACE_MEDIUM_UNDEFINED; // a.k.a. 0
	}
	switch(get_id_type(address_id)){
	case TYPE_NET_INTERFACE_IP_ADDRESS_T:
		return NET_INTERFACE_MEDIUM_IP;
	case TYPE_NET_INTERFACE_RADIO_ADDRESS_T:
		return NET_INTERFACE_MEDIUM_RADIO;
	default:
		P_V_S(convert::type::from(get_id_type(address_id)), P_WARN);
		print("type passed is not a valid address type", P_ERR);
	}
	return 0;
}

#pragma message("this IP type detector isn't too good")

uint8_t net_interface::ip::get_address_type(std::string ip){
	P_V_S(ip, P_SPAM);
	const uint64_t dot_count =
		std::count(ip.begin(), ip.end(), '.');
	if(dot_count == 3 && ip.size() <= 15){
		print("this is (probably) an IPv4 address", P_NOTE);
		return NET_INTERFACE_IP_ADDRESS_TYPE_IPV4;
	}
	const uint64_t colon_count =
		std::count(ip.begin(), ip.end(), ':');
	if(BETWEEN(2, colon_count, 8)){
		print("this is (probably) an IPv6 address", P_NOTE);
		return NET_INTERFACE_IP_ADDRESS_TYPE_IPV6;
	}
	print("no checks on domain name, assuming this is a domain name", P_NOTE);
	return NET_INTERFACE_IP_ADDRESS_TYPE_DOMAIN;
}

// Raw is not in NBO format

std::string net_interface::ip::raw::to_readable(std::pair<std::vector<uint8_t>, uint8_t> raw){
	std::string retval;
	switch(raw.second){
	case NET_INTERFACE_IP_ADDRESS_TYPE_IPV4:	
		if(true){
			struct in_addr addr;
			CLEAR(addr);
			memcpy(&addr.s_addr, raw.first.data(), 4);
			addr.s_addr =
				NBO_32(addr.s_addr);
			char buf[INET_ADDRSTRLEN];
			inet_ntop(
				AF_INET,
				(struct in_addr*)&addr,
				&(buf[0]),
				INET_ADDRSTRLEN);
			retval = buf;
		}
		break;
	default:
		print("UNSUPPORTED ADDRESS TYPE", P_ERR);
		break;
	}
	P_V_S(retval, P_DEBUG);
	return retval;
}
	
std::pair<std::vector<uint8_t>, uint8_t> net_interface::ip::readable::to_raw(std::string readable){
	std::pair<std::vector<uint8_t>, uint8_t> retval;
	uint8_t address_type = 0;
	P_V_S(readable, P_NOTE);
	switch((address_type = get_address_type(readable))){
	case NET_INTERFACE_IP_ADDRESS_TYPE_IPV4:
		if(true){
			struct in_addr addr;
			CLEAR(addr);
			if(inet_pton(AF_INET, readable.c_str(), &addr) < 0){
				print("couldn't convert to network representation", P_ERR);
			}
			retval.first =
				convert::nbo::from(
					std::vector<uint8_t>(
						((uint8_t*)&addr.s_addr),
						((uint8_t*)&addr.s_addr)+sizeof(addr.s_addr)));
		}
		break;
	case NET_INTERFACE_IP_ADDRESS_TYPE_IPV6:
		print("SDL2 doesn't support IPv6", P_ERR);
		break;
	case NET_INTERFACE_IP_ADDRESS_TYPE_DOMAIN:
		break;
	default:
		print("unknwon address type", P_ERR);
	}
	retval.second = address_type;
	P_V(retval.first.size(), P_DEBUG);
	P_V(retval.second, P_DEBUG);
	return retval;
}
