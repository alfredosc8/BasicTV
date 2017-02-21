#include "../../util.h"
#include "../../id/id.h"
#include "../../id/id_api.h"
#include "net_proto_connections.h"
#include "net_proto_con_req.h"
#include "net_proto_socket.h"
#include "../net_socket.h"
#include "../../settings.h"
#include "net_proto.h"
#include "inbound/net_proto_inbound_connections.h"
#include "outbound/net_proto_outbound_connections.h"

#include <algorithm>

void net_proto_handle_tcp_holepunch(net_proto_con_req_t *con_req){
	const uint64_t timestamp =
		con_req->get_heartbeat_timestamp();
	const uint64_t cur_timestamp =
		get_time_microseconds();
	const uint64_t offset = 100000;
	if(BETWEEN(cur_timestamp-offset,
		   timestamp,
		   cur_timestamp+offset)){
		id_t_ second_id = ID_BLANK_ID;
		con_req->get_peer_ids(
			nullptr,
			&second_id,
			nullptr);
		net_proto_peer_t *peer_ptr =
			PTR_DATA_FAST(second_id,
				      net_proto_peer_t);
		if(peer_ptr == nullptr){
			print("cannot holepunch to a null peer", P_ERR);
		}
		const std::string peer_ip =
			peer_ptr->get_net_ip_str();
		const uint16_t peer_port =
			peer_ptr->get_net_port();
		net_socket_t *holepunch_socket =
			new net_socket_t;
		holepunch_socket->set_net_ip(
			peer_ip,
			peer_port);
		holepunch_socket->connect();
		print("add logic to finish connection", P_CRIT);
	}
}

static void list_to_socket_count(id_t_ peer_id, std::vector<std::pair<id_t_, uint64_t> > *peer_socket_count){
	auto id_iter =
		std::find_if(
			peer_socket_count->begin(),
			peer_socket_count->end(),
			[&peer_id](std::pair<id_t_, uint64_t> const& elem){
				return elem.first == peer_id;
			});
	if(id_iter != peer_socket_count->end()){
		id_iter->second++;
	}else{
		peer_socket_count->push_back(
			std::make_pair(
				peer_id,
				1));
	}

}

static void add_sockets_to_socket_count(std::vector<std::pair<id_t_, uint64_t> > *peer_socket_count){
	std::vector<id_t_> proto_socket_count =
		id_api::cache::get(
			"net_proto_socket_t");
	for(uint64_t i = 0;i < proto_socket_count.size();i++){
		net_proto_socket_t *proto_socket_ptr =
			PTR_DATA(proto_socket_count[i],
				 net_proto_socket_t);
		if(proto_socket_ptr == nullptr){
			continue;
		}
		const id_t_ peer_id =
			proto_socket_ptr->get_peer_id();
		list_to_socket_count(
			peer_id, peer_socket_count);
	}
}

static void add_con_req_to_socket_count(std::vector<std::pair<id_t_, uint64_t> > *peer_socket_count){
	std::vector<id_t_> con_req_vector =
		id_api::cache::get(
			"net_proto_con_req_t");
	for(uint64_t i = 0;i < con_req_vector.size();i++){
		net_proto_con_req_t *con_req_ptr =
			PTR_DATA(con_req_vector[i],
				 net_proto_con_req_t);
		if(con_req_ptr == nullptr){
			continue;
		}
		id_t_ peer_id_vector[2] =
			{ID_BLANK_ID, ID_BLANK_ID};
		con_req_ptr->get_peer_ids(
			&(peer_id_vector[0]),
			&(peer_id_vector[1]),
			nullptr);
		list_to_socket_count(peer_id_vector[0], peer_socket_count);
		list_to_socket_count(peer_id_vector[1], peer_socket_count);
	}
}

static uint64_t get_all_peer_socket_count(std::vector<std::pair<id_t_, uint64_t> > peer_socket_count){
	uint64_t retval = 0;
	for(uint64_t i = 0;i < peer_socket_count.size();i++){
		retval += peer_socket_count[i].second;
	}
	return retval;
}

/*
  Creates random connections to peers
 */

void net_proto_create_random_connections(){
	std::vector<std::pair<id_t_, uint64_t> > peer_socket_count;
	add_sockets_to_socket_count(&peer_socket_count);
	add_con_req_to_socket_count(&peer_socket_count);
	const uint64_t max_connection_count =
		get_all_peer_socket_count(
			peer_socket_count);
	const uint64_t connection_number =
		settings::get_setting_unsigned_def(
			"tcp_max_con",
			64); // semi-reasonable max
	std::vector<id_t_> peer_id_vector =
		id_api::cache::get(
			"net_proto_peer_t");
	std::random_shuffle(
		peer_id_vector.begin(),
		peer_id_vector.end());
	for(uint64_t i = 0;i < max_connection_count-connection_number;i++){
		net_proto::socket::connect(
			peer_id_vector[i], 1);
	}
}

void net_proto_connection_manager(){
	net_proto_accept_all_connections();
	net_proto_initiate_all_connections();
	net_proto_create_random_connections();
}
