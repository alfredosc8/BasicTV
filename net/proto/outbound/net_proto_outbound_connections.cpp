#include "net_proto_outbound_connections.h"
#include "../net_proto_socket.h"
#include "../net_proto_connections.h"
#include "../net_proto_api.h"
#include "../../../id/id.h"
#include "../../../id/id_api.h"

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
	net_socket_t *socket_ptr =
		new net_socket_t;
	socket_ptr->set_net_ip(
		proto_peer->get_net_ip_str(),
		proto_peer->get_net_port());
	socket_ptr->connect();
	if(socket_ptr->is_alive() == false){
		print("couldn't connect to peer", P_NOTE);
	}
	proto_peer->set_last_attempted_connect_time(
		get_time_microseconds());
	net_proto_socket_t *proto_socket_ptr =
		new net_proto_socket_t;
	proto_socket_ptr->set_peer_id(peer_id);
	proto_socket_ptr->set_socket_id(socket_ptr->id.get_id());
}

static void net_proto_first_id_logic(net_proto_con_req_t *con_req){
	switch(con_req->get_flags()){
	case (NET_CON_REQ_TCP | NET_CON_REQ_DIRECT):
		net_proto_initiate_direct_tcp(con_req);
		break;
	case (NET_CON_REQ_TCP | NET_CON_REQ_HOLEPUNCH):
		net_proto_handle_tcp_holepunch(con_req);
		break;
	default:
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
			if(second_peer_ptr->get_last_attempted_connect_time()-1000000 <
			   get_time_microseconds()){
				net_proto_first_id_logic(con_req);
			}else{
				print("skipping connection to peer, too fast", P_NOTE);
			}
		}
		// second is always inbound, don't bother with that here
	}
}
