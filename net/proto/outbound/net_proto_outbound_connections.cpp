#include "net_proto_outbound_connections.h"
#include "../net_proto_socket.h"
#include "../net_proto_connections.h"
#include "../net_proto_api.h"
#include "../../../id/id.h"
#include "../../../id/id_api.h"

#include "../../../encrypt/encrypt.h"

#include <vector>

static void net_proto_initiate_direct_tcp(net_proto_con_req_t *con_req){
	if(con_req == nullptr){
		print("con_req is a nullptr", P_ERR);
	}
	id_t_ peer_id = ID_BLANK_ID;
	con_req->get_peer_ids(
		nullptr, &peer_id, nullptr);
	net_proto_peer_t *proto_peer =
		PTR_DATA(peer_id,
			 net_proto_peer_t);
	if(proto_peer == nullptr){
		print("proto_peer is a nullptr", P_ERR);
	}
	net_socket_t *socket_ptr = nullptr;
	net_proto_socket_t *proto_socket_ptr = nullptr;
	try{
		socket_ptr =
			new net_socket_t;
		socket_ptr->set_net_ip(
			proto_peer->get_net_ip_str(),
			proto_peer->get_net_port());
		socket_ptr->connect();
		if(socket_ptr->is_alive() == false){
			print("couldn't connect to peer", P_NOTE);
		}else{
			print("opened connection with peer (IP: " +
			      proto_peer->get_net_ip_str() + " port:" +
			      std::to_string(proto_peer->get_net_port()) + ")",
			      P_NOTE);
			delete con_req;
			con_req = nullptr;
		}
		proto_peer->set_last_attempted_connect_time(
			get_time_microseconds());
		proto_socket_ptr =
			new net_proto_socket_t;
		proto_socket_ptr->set_peer_id(peer_id);
		proto_socket_ptr->set_socket_id(socket_ptr->id.get_id());
		const id_t_ my_peer_id =
			net_proto::peer::get_self_as_peer();
		proto_socket_ptr->send_id(
			my_peer_id);
		proto_socket_ptr->send_id(
			encrypt_api::search::pub_key_from_hash(
				get_id_hash(
					my_peer_id)));
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

static void net_proto_first_id_logic(net_proto_con_req_t *con_req){
	switch(con_req->get_flags()){
	case (NET_CON_REQ_TCP | NET_CON_REQ_DIRECT):
		print("attempting a direct TCP connection", P_DEBUG);
		net_proto_initiate_direct_tcp(con_req);
		break;
	case (NET_CON_REQ_TCP | NET_CON_REQ_HOLEPUNCH):
		print("TODO: properly implement a TCP holepunch", P_WARN);
		//print("attempting a TCP holepunch", P_DEBUG);
		//net_proto_handle_tcp_holepunch(con_req);
		//break;
	default:
		print("invalid flags for con_req, not establishing", P_WARN);
		break;
	}
}

void net_proto_initiate_all_connections(){
	std::vector<id_t_> proto_con_req_vector =
		id_api::cache::get(
			"net_proto_con_req_t");
	for(uint64_t i = 0;i < proto_con_req_vector.size();i++){
		net_proto_con_req_t *con_req =
			PTR_DATA_FAST(proto_con_req_vector[i],
				      net_proto_con_req_t);
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
			net_proto_peer_t *second_peer_ptr =
				PTR_DATA(second_id,
					 net_proto_peer_t);
			if(second_peer_ptr == nullptr){
				print("second peer is a nullptr", P_NOTE);
				continue;
			}
			if(get_time_microseconds() >
			   second_peer_ptr->get_last_attempted_connect_time()+1000000){
				net_proto_first_id_logic(con_req);
			}else{
				//print("skipping connection to peer, too fast", P_SPAM);
			}
		}else{
			P_V_S(convert::array::id::to_hex(first_id), P_VAR);
			P_V_S(convert::array::id::to_hex(second_id), P_VAR);
			P_V_S(convert::array::id::to_hex(
				      net_proto::peer::get_self_as_peer()), P_VAR);
		}
		// second is always inbound, don't bother with that here
	}
}
