#include "net_proto_outbound_connections.h"
#include "../net_proto_socket.h"
#include "../net_proto_connections.h"
#include "../net_proto_api.h"
#include "../../../id/id.h"
#include "../../../id/id_api.h"

#include "../../../encrypt/encrypt.h"

#include <vector>

#define SEND_IF_VALID(x) if(PTR_ID(x, )){proto_socket_ptr->send_id(x);}else{print("can't send id " + id_breakdown(x), P_ERR);}

static void net_proto_initiate_direct_tcp(
	net_proto_con_req_t *con_req,
	net_proto_peer_t *proto_peer_ptr,
	net_interface_ip_address_t *ip_address_ptr){

	ASSERT(con_req != nullptr, P_ERR);
	ASSERT(proto_peer_ptr != nullptr, P_ERR);
	ASSERT(ip_address_ptr != nullptr, P_ERR);
	// ASSERT(net_interface::medium::from_address(proto_peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_ERR);

	net_socket_t *socket_ptr = nullptr;
	net_proto_socket_t *proto_socket_ptr = nullptr;
	try{
		socket_ptr =
			new net_socket_t;
		socket_ptr->set_net_ip(
			net_interface::ip::raw::to_readable(
				ip_address_ptr->get_address()),
			ip_address_ptr->get_port());
		socket_ptr->connect();
		if(socket_ptr->is_alive() == false){
			print("couldn't connect to peer", P_NOTE);
		}else{
			print("opened connection with peer (IP: " +
			      net_interface::ip::raw::to_readable(
				      ip_address_ptr->get_address()) + " port:" +
			      std::to_string(ip_address_ptr->get_port()) + ")",
			      P_NOTE);
			delete con_req;
			con_req = nullptr;
		}

		ip_address_ptr->set_last_attempted_connect_time(
			get_time_microseconds());
		proto_socket_ptr =
			new net_proto_socket_t;
		proto_socket_ptr->set_peer_id(
			proto_peer_ptr->id.get_id());
		proto_socket_ptr->set_socket_id(
			socket_ptr->id.get_id());
		const id_t_ my_peer_id =
			net_proto::peer::get_self_as_peer();
		net_proto_peer_t *my_proto_peer_ptr =
			PTR_DATA(my_peer_id,
				 net_proto_peer_t);
		ASSERT(my_proto_peer_ptr != nullptr, P_ERR);

		SEND_IF_VALID(encrypt_api::search::pub_key_from_hash(get_id_hash(my_peer_id)));
		SEND_IF_VALID(my_peer_id);
		SEND_IF_VALID(my_proto_peer_ptr->get_address_id());
	}catch(...){
		print("socket is a nullptr (client disconnect), destroying net_proto_socket_t", P_NOTE);
		if(proto_socket_ptr != nullptr){
			delete proto_socket_ptr;
			proto_socket_ptr = nullptr;
		}
		if(socket_ptr != nullptr){
			delete socket_ptr;
			socket_ptr = nullptr;
		}
	}
}

#pragma message("net_proto_first_id_logic only does direct TCP without any checks")

static void net_proto_first_id_logic(net_proto_con_req_t *con_req_ptr,
				     net_proto_peer_t *proto_peer_ptr,
				     net_interface_ip_address_t *ip_address_ptr){
	net_proto_initiate_direct_tcp(
		con_req_ptr,
		proto_peer_ptr,
		ip_address_ptr);
}

void net_proto_initiate_all_connections(){
	std::vector<id_t_> proto_con_req_vector =
		id_api::cache::get(
			"net_proto_con_req_t");
	const uint64_t cur_time_micro_s =
		get_time_microseconds();
	const id_t_ self_peer_id =
		net_proto::peer::get_self_as_peer();
	for(uint64_t i = 0;i < proto_con_req_vector.size();i++){
		net_proto_con_req_t *con_req =
			PTR_DATA(proto_con_req_vector[i],
				 net_proto_con_req_t);
		CONTINUE_IF_NULL(con_req, P_WARN);
		id_t_ first_id = ID_BLANK_ID;
		id_t_ second_id = ID_BLANK_ID;
		con_req->get_peer_ids(
			&first_id,
			&second_id,
			nullptr);
		if(first_id != self_peer_id){
			continue;
		}
		if(second_id == self_peer_id){
			continue;
		}
		net_proto_peer_t *second_peer_ptr =
			PTR_DATA(second_id,
				 net_proto_peer_t);
		CONTINUE_IF_NULL(second_peer_ptr, P_WARN);
		// ASSERT(net_interface::medium::from_address(second_peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_ERR);
		net_interface_ip_address_t *ip_address_ptr =
			PTR_DATA(second_peer_ptr->get_address_id(),
				 net_interface_ip_address_t);
		if(ip_address_ptr == nullptr){
			print("ip_address_ptr is a nullptr " + id_breakdown(second_peer_ptr->get_address_id()) +
			      " in association with " + id_breakdown(second_peer_ptr->id.get_id()), P_WARN);
			id_api::destroy(second_peer_ptr->id.get_id());
			second_peer_ptr = nullptr;
			continue;
		}
		// CONTINUE_IF_NULL(ip_address_ptr, P_WARN);
		if(cur_time_micro_s >
		   ip_address_ptr->get_last_attempted_connect_time()+1000000){
			net_proto_first_id_logic(
				con_req,
				second_peer_ptr,
				ip_address_ptr); // only write to IP address is last connect attempt time
		}
	}
	// second is always inbound, don't bother with that here
}
