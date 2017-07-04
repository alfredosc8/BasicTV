#include "net_proto_outbound_connections.h"
#include "../net_proto_socket.h"
#include "../net_proto_connections.h"
#include "../net_proto_api.h"
#include "../../../id/id.h"
#include "../../../id/id_api.h"

#include "../../../settings.h"

#include "../../../encrypt/encrypt.h"

#include <vector>

/*
  TODO: remove all checks against IP address connection frequency here
  and move that over to net_proto_api (and cleanup with net_proto loop)
 */

static void net_proto_initiate_direct_tcp(
	net_proto_con_req_t *con_req,
	net_proto_peer_t *proto_peer_ptr,
	net_interface_ip_address_t *ip_address_ptr){

	ASSERT(con_req != nullptr, P_ERR);
	ASSERT(proto_peer_ptr != nullptr, P_ERR);
	ASSERT(ip_address_ptr != nullptr, P_ERR);
	ASSERT(net_interface::medium::from_address(proto_peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_ERR);

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
		print("forwarding to caller", P_UNABLE);
	}
	print("opened connection with peer (IP: " +
	      net_interface::ip::raw::to_readable(
		      ip_address_ptr->get_address()) + " port:" +
	      std::to_string(ip_address_ptr->get_port()) + ")",
	      P_NOTE);
	delete con_req;
	con_req = nullptr;
	
	ip_address_ptr->set_last_attempted_connect_time_micro_s(
		get_time_microseconds());
	proto_socket_ptr =
		new net_proto_socket_t;
	proto_socket_ptr->set_peer_id(
		proto_peer_ptr->id.get_id());
	proto_socket_ptr->set_socket_id(
		socket_ptr->id.get_id());
	const id_t_ my_peer_id =
		net_proto::peer::get_self_as_peer();
	proto_socket_ptr->send_id(
		my_peer_id);
	proto_socket_ptr->send_id(
		encrypt_api::search::pub_key_from_hash(
			get_id_hash(
				my_peer_id)));
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

static std::vector<std::pair<std::vector<uint8_t>, uint16_t> > net_proto_get_all_connections(){
	std::vector<std::pair<std::vector<uint8_t>, uint16_t> > retval;
	std::vector<id_t_> socket_vector =
		id_api::cache::get(
			TYPE_NET_SOCKET_T);
	for(uint64_t i = 0;i < socket_vector.size();i++){
		net_socket_t *socket_ptr =
			PTR_DATA(socket_vector[i],
				 net_socket_t);
		CONTINUE_IF_NULL(socket_ptr, P_ERR);
		std::vector<uint8_t> raw_ip_addr =
			socket_ptr->get_net_ip_raw();
		uint16_t port =
			socket_ptr->get_net_port();
		auto ip_iter =
			std::find_if(
				retval.begin(),
				retval.end(),
				[&raw_ip_addr, &port](std::pair<std::vector<uint8_t>, uint16_t> const& elem){
					return elem.first == raw_ip_addr &&
					elem.second == port;
				});
		if(ip_iter == retval.end()){
			retval.push_back(
				std::make_pair(
					raw_ip_addr,
					port));
		}
	}
	return retval;
}

void net_proto_initiate_all_connections(){
	std::vector<id_t_> proto_con_req_vector =
		id_api::cache::get(
			"net_proto_con_req_t");
	// const uint64_t cur_time_micro_s =
	// 	get_time_microseconds();
	for(uint64_t i = 0;i < proto_con_req_vector.size();i++){
		if(id_api::cache::get(TYPE_NET_SOCKET_T).size() >=
		   (uint32_t)std::stoi(settings::get_setting("net_interface_ip_tcp_max_con"))){
			print("can't create any more sockets", P_ERR);
		}
		net_proto_con_req_t *con_req =
			PTR_DATA(proto_con_req_vector[i],
				 net_proto_con_req_t);
		CONTINUE_IF_NULL(con_req, P_WARN);
		id_t_ first_id = ID_BLANK_ID;
		id_t_ second_id = ID_BLANK_ID;
		const id_t_ self_peer_id =
			net_proto::peer::get_self_as_peer();
		// third is unused (would be third party for UDP)
		con_req->get_peer_ids(
			&first_id,
			&second_id,
			nullptr);
		if(first_id == self_peer_id){
			ASSERT(second_id != self_peer_id, P_ERR);
			net_proto_peer_t *second_peer_ptr =
				PTR_DATA(second_id,
					 net_proto_peer_t);
			CONTINUE_IF_NULL(second_peer_ptr, P_NOTE);
			ASSERT(net_interface::medium::from_address(second_peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_ERR);
			net_interface_ip_address_t *ip_address_ptr =
				PTR_DATA(second_peer_ptr->get_address_id(),
					 net_interface_ip_address_t);
			CONTINUE_IF_NULL(ip_address_ptr, P_ERR);
			std::pair<std::vector<uint8_t>, uint16_t> raw_ip_data =
				std::make_pair(
					ip_address_ptr->get_address().first, 
					ip_address_ptr->get_port());
			// if(cur_time_micro_s >
			//    ip_address_ptr->get_last_attempted_connect_time_micro_s()+
			//    settings::get_setting_unsigned_def("net_interface_ip_address_connection_delay_micro_s", 10000000)){
				try{
					net_proto_first_id_logic(
						con_req,
						second_peer_ptr,
						ip_address_ptr); // only write to IP address is last connect attempt time
				}catch(...){
					print("couldn't open con_req, destroying", P_WARN);
					delete con_req;
					con_req = nullptr;
					continue;
				}
			// }
		}else{
			P_V_S(convert::array::id::to_hex(first_id), P_VAR);
			P_V_S(convert::array::id::to_hex(second_id), P_VAR);
			P_V_S(convert::array::id::to_hex(
				      net_proto::peer::get_self_as_peer()), P_VAR);
		}
		// second is always inbound, don't bother with that here
	}
}
