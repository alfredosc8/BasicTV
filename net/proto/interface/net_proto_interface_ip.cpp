#include "net_proto_interface_ip.h"

#include <SDL2/SDL_net.h>


/*
  TODO: use even more abstractions to define the transport layer
  (TCP/UDP). Especially for radio transmission
 */

net_proto_interface_peer_ip_t::net_proto_interface_peer_ip_t() :
	id(this, TYPE_NET_PROTO_INTERFACE_PEER_IP_T){
	id.add_data_raw(&(port), sizeof(port));
	id.add_data_raw(&(addr_type), sizeof(addr_type));
	id.add_data_one_byte_vector(&addr,
				    NET_PROTO_INTERFACE_PEER_IP_DOMAIN_MAX_LEN);
}

/*
  net_proto_interface_state_t *state
  id_t_ interface_peer
 */

NET_PROTO_INTERFACE_CONNECT_STATE(ip){
	state_sanity_check(state);
	if(get_id_type(interface_peer) != TYPE_NET_PROTO_INTERFACE_PEER_IP_T){
		print("wrong type for given interface peer", P_ERR);
	}
	net_proto_interface_peer_ip_t *peer_ptr =
		PTR_DATA(interface_peer,
			 net_proto_interface_peer_ip_t);
	if(peer_ptr == nullptr){
		print("peer_ptr is a nullptr", P_ERR);
	}
	IPaddress *tmp_ip_addr =
		nullptr;
	std::vector<uint8_t> addr =
		peer_ptr->get_addr();
	uint16_t port =
		peer_ptr->get_port();
	int64_t retval;
	switch(peer_ptr->get_addr_type()){
	case NET_PROTO_INTERFACE_PEER_IP_ADDR_TYPE_IPV4:
	case NET_PROTO_INTERFACE_PEER_IP_ADDR_TYPE_DOMAIN:
		retval = SDLNet_ResolveHost(
			tmp_ip_addr,
			(const char*)addr.data(),
			port);
	case NET_PROTO_INTERFACE_PEER_IP_ADDR_TYPE_IPV6:
		print("i'm pretty sure SDL2_net doesn't support IPv6", P_ERR);
		break;
	default:
		print("invalid address type", P_ERR);
	};
	if(retval == -1){
		print("couldn't resolve host", P_ERR);
	}
	TCPsocket socket =
		SDLNet_TCP_Open(tmp_ip_addr);
	if(socket == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	state->set_state_ptr(
		socket);
	state->set_state_format(
		NET_PROTO_INTERFACE_STATE_FORMAT_IP);
}

/*
  TODO: generic implementation of escape characters with all
  networking systems
 */

NET_PROTO_INTERFACE_ITERATE_STATE(ip){
	state_sanity_check(state);
	// inbound
	char tmp[512];
	int32_t recv_bytes = 0;
	std::vector<uint8_t> inbound_buffer =
		state->get_inbound_buffer();
	while((recv_bytes =
	       SDLNet_TCP_Recv((TCPsocket)state->get_state_ptr(),
			      &(tmp[0]),
			       512)) > 0){
		inbound_buffer.insert(
			inbound_buffer.end(),
			&(tmp[0]),
			&(tmp[0])+recv_bytes);
	}
	state->set_inbound_buffer(
		inbound_buffer);
	//outbound
	std::vector<uint8_t> outbound_buffer =
		state->get_outbound_buffer();
	int64_t data_sent =
		SDLNet_TCP_Send(
			(TCPsocket)state->get_state_ptr(),
			outbound_buffer.data(),
			outbound_buffer.size());
	if((uint64_t)data_sent == outbound_buffer.size()){
		print("all data was sent", P_ERR);
		outbound_buffer.clear();
	}else if(data_sent < 0){
		print("an error occured, no data was sent", P_ERR); // ?
	}else if(data_sent > 0){
		print("an error occured, some data was sent", P_ERR);
		outbound_buffer.erase(
			outbound_buffer.begin(),
			outbound_buffer.begin()+data_sent);
	}
	state->set_outbound_buffer(
		outbound_buffer);
}

NET_PROTO_INTERFACE_SEND_STATE(ip){
	// SDL has a buffer already in place, so we just pass it on
	state_sanity_check(state);
	std::vector<uint8_t> inbound_buffer =
		state->get_inbound_buffer();
	inbound_buffer.insert(
		inbound_buffer.begin(),
		data.begin(),
		data.end());
	state->set_inbound_buffer(
		inbound_buffer);
}

/*
  Again, since SDL handles a buffer internally, we just pull from that
 */

NET_PROTO_INTERFACE_RECV_STATE(ip){
	state_sanity_check(state);
	while(state->get_inbound_buffer().size() < bytes_to_recv && hang){
		ip_iterate_state(state);

	}
	std::vector<uint8_t> inbound_buffer =
		state->get_inbound_buffer();
	if(inbound_buffer.size() < bytes_to_recv){
		print("don't have enough information with no hang, returning blank", P_NOTE);
		return {};
	}
	std::vector<uint8_t> retval(
		inbound_buffer.begin(),
		inbound_buffer.begin()+bytes_to_recv);
	inbound_buffer.erase(
		inbound_buffer.begin(),
		inbound_buffer.begin()+bytes_to_recv);
	return retval;
}

NET_PROTO_INTERFACE_CLOSE_STATE(ip){
	state_sanity_check(state);
	SDLNet_TCP_Close((TCPsocket)state->get_state_ptr());
	state->set_state_ptr(
		nullptr);
}
