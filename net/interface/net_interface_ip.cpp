#include "net_interface.h"
#include "net_interface_ip.h"
#include "net_interface_tcp.h"
#include "net_interface_hardware.h"
#include "net_interface_software.h"
#include "net_interface_intermediary.h"
#include "net_interface_medium.h"
#include "net_interface_ip_address.h"

#include "net_interface_helper.h"

/*
  Moving away from SDL_net and towards POSIX sockets

  TODO: completely rewrite net_interface_ip.cpp for
  Windows
 */

struct net_interface_medium_ip_ptr_t{
public:
	int socket_fd = 0;
};


static void hardware_software_address_sanity_check(
	net_interface_hardware_dev_t *hardware_dev_ptr,
	net_interface_software_dev_t *software_dev_ptr,
	net_interface_ip_address_t *ip_address_ptr){
	if(software_dev_ptr == nullptr){
		ASSERT(ip_address_ptr->get_medium() == hardware_dev_ptr->get_medium(), P_ERR);
	}else{
		ASSERT(ip_address_ptr->get_required_intermediary() == 0 ||
		       ip_address_ptr->get_required_intermediary() == software_dev_ptr->get_intermediary(), P_ERR);
		ASSERT(hardware_dev_ptr->get_medium() == software_dev_ptr->get_medium() &&
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

#pragma message("no attempt at UDP support whatever right now")
#pragma message("packetizer isn't actually called for TCP (doesn't NEED to be)")

INTERFACE_SEND(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_SW_PTR(software_dev_id);
	ASSERT(payload->size() != 0, P_ERR);

	// sanity checks for medium, modulation, and encapsulation are done
	// on assignment to address, so they should be fine herex

	int64_t sent_bytes = 0;

	net_interface_medium_ip_ptr_t *working_state = 
		static_cast<net_interface_medium_ip_ptr_t*>(software_dev_ptr->get_state_ptr());
		
	net_interface_medium_packet_t medium_packet =
		medium_packet_lookup(
			software_dev_ptr->get_medium(),
			software_dev_ptr->get_packet_modulation(),
			software_dev_ptr->get_packet_encapsulation());
	
	std::vector<std::vector<uint8_t> > packetized =
		medium_packet.packetize(
			hardware_dev_id,
			software_dev_id,
			payload);
	for(uint64_t i = 0;i < packetized.size();i++){
		switch(software_dev_ptr->get_packet_modulation()){
		case NET_INTERFACE_MEDIUM_PACKET_MODULATION_TCP:
			// sent_bytes =
			// 	SDLNet_TCP_Send(
			// 		working_state->tcp_socket,
			// 		packetized[i].data(),
			// 		packetized[i].size());
			sent_bytes =
				send(working_state->socket_fd,
				     packetized[i].data(),
				     packetized[i].size(),
				     MSG_NOSIGNAL);
			if(sent_bytes == -1){
				print("TCP send didn't work", P_WARN);
				sent_bytes = 0;
			}else if(sent_bytes < (int64_t)packetized[i].size() || (sent_bytes == -EPIPE)){
				print("not all TCP data was sent (socket was probably broken)", P_WARN);
			}
			break;
		case NET_INTERFACE_MEDIUM_PACKET_MODULATION_UDP:
			print("udp isn't implemented yet", P_CRIT);
			break;
		default:
			print("unsupported encapsulation scheme for ip", P_ERR);
		}
		packetized[i].erase(
			packetized[i].begin(),
			packetized[i].begin()+sent_bytes);
		if(packetized[i].size() == 0){
			packetized.erase(
				packetized.begin()+i);
			i--;
		}
	}
}

/*
  Instead of offloading the data, maybe use MSG_PEEK for the block, get the
  appropriate size, somehow get the size of the received buffer through POSIX,
  and do a copy-on-write
 */

INTERFACE_RECV_ALL(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_SW_PTR(software_dev_id);
	std::vector<uint8_t> retval;
	char buffer[65536];
	int32_t recv_bytes = -1;

	net_interface_medium_ip_ptr_t *working_state = 
		static_cast<net_interface_medium_ip_ptr_t*>(software_dev_ptr->get_state_ptr());
	
	while(recv_bytes != 0){
		recv_bytes = 0;
		switch(software_dev_ptr->get_packet_modulation()){
		case NET_INTERFACE_MEDIUM_PACKET_MODULATION_TCP:
			recv_bytes =
				recv(working_state->socket_fd,
				     &(buffer[0]),
				     65536,
				     MSG_DONTWAIT);
			if(recv_bytes <= 0){
				print("TCP recv didn't work", P_WARN);
				recv_bytes = 0;
			}
			break;
		case NET_INTERFACE_MEDIUM_PACKET_MODULATION_UDP:
			print("udp isn't implemented yet", P_CRIT);
			break;
		default:
			print("unsupported encapsulation scheme for ip", P_ERR);
		}
		if(recv_bytes != 0){
			software_dev_ptr->add_inbound_data(
				std::vector<uint8_t>(
					buffer,
					buffer+recv_bytes));
		}
	}
	net_interface_medium_packet_t medium_packet =
		medium_packet_lookup(
			software_dev_ptr->get_medium(),
			software_dev_ptr->get_packet_modulation(),
			software_dev_ptr->get_packet_encapsulation());
	std::vector<std::vector<uint8_t> > inbound_data_ =
		software_dev_ptr->get_inbound_data();
	std::vector<uint8_t> unpacketized =
		medium_packet.depacketize(
			hardware_dev_id,
			software_dev_id,
			&inbound_data_);
	software_dev_ptr->set_inbound_data(
		inbound_data_);
	software_dev_ptr->add_inbound_data(
		unpacketized);
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

INTERFACE_ADD_ADDRESS(ip){
	INTERFACE_SET_HW_PTR(hardware_dev_id);
	INTERFACE_SET_ADDR_PTR(address_id);
	ASSERT(address_id != ID_BLANK_ID, P_ERR);
	net_interface_software_dev_t *software_dev_ptr =
		new net_interface_software_dev_t;
	software_dev_ptr->set_address_id(
		address_id);
	software_dev_ptr->set_hardware_dev_id(
		hardware_dev_id);
	hardware_dev_ptr->add_soft_dev_list(
		software_dev_ptr->id.get_id());
	net_interface_medium_ip_ptr_t *working_state =
		new net_interface_medium_ip_ptr_t;
	software_dev_ptr->set_state_ptr(
		(void*)working_state);
	return software_dev_ptr->id.get_id();
}
