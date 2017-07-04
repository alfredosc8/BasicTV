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

/*
  This specifically handles creating net_proto_con_req_t. Actual connecting
  code is handled in inbound and outbound connection files directly.
 */

void net_proto_handle_tcp_holepunch(net_proto_con_req_t *con_req){
	const uint64_t timestamp =
		con_req->get_timestamp();
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
			PTR_DATA(second_id,
				 net_proto_peer_t);
		PRINT_IF_NULL(peer_ptr, P_ERR);
		ASSERT(net_interface::medium::from_address(peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_ERR);
		net_interface_ip_address_t *ip_address_ptr =
			PTR_DATA(peer_ptr->get_address_id(),
				 net_interface_ip_address_t);
		PRINT_IF_NULL(ip_address_ptr, P_ERR);
		net_socket_t *holepunch_socket =
			new net_socket_t;
		holepunch_socket->set_net_ip(
			net_interface::ip::raw::to_readable(
				ip_address_ptr->get_address()),
			ip_address_ptr->get_port());
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

// TODO: make this not clearnet-y

static bool pending_clearnet_con_req_for_peer(id_t_ peer_id){
	std::vector<id_t_> con_req_vector =
		id_api::cache::get(
			"net_proto_con_req_t");
	for(uint64_t i = 0;i < con_req_vector.size();i++){
		net_proto_con_req_t *con_req_ptr =
			PTR_DATA_FAST(
				con_req_vector[i],
				net_proto_con_req_t);
		if(con_req_ptr == nullptr){
			continue;
		}
		id_t_ peer_id_con_req = ID_BLANK_ID;
		con_req_ptr->get_peer_ids(
			nullptr, &peer_id_con_req, nullptr);
		if(peer_id == peer_id_con_req){
			return true;
		}
	}
	return false;
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
		(uint64_t)std::stoi(
			settings::get_setting(
				"net_interface_ip_tcp_max_con"));
	std::vector<id_t_> peer_id_vector =
		id_api::cache::get(
			"net_proto_peer_t");
	std::random_shuffle(
		peer_id_vector.begin(),
		peer_id_vector.end());
	uint64_t connections_to_start =
		max_connection_count-connection_number;
	if(connections_to_start > peer_id_vector.size()){
		connections_to_start = peer_id_vector.size();
	}
	for(uint64_t i = 0;i < connections_to_start;i++){
		if(peer_id_vector[i] == net_proto::peer::get_self_as_peer()){
			continue;
		}
		if(peer_id_vector[i] == ID_BLANK_ID){
			continue;
		}
		if(pending_clearnet_con_req_for_peer(peer_id_vector[i]) == false){
			print("creating connection with peer " + convert::array::id::to_hex(peer_id_vector[i]), P_DEBUG);
			net_proto::socket::connect(
				peer_id_vector[i], 1);
		}
	}
}

static void net_proto_remove_stale_requests(){
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
		if(get_time_microseconds() > con_req_ptr->get_timestamp()+(30*1000*1000)){
			print("deleting stale con_req", P_NOTE);
			print("TODO: turn con_req timeout into a setting", P_NOTE);
			// no harm in deleting the type if it is known that
			// it cannot be exported or networked
			delete con_req_ptr;
			con_req_ptr = nullptr;
		}
	}
}

void net_proto_connection_manager(){
	net_proto_accept_all_connections();
	net_proto_initiate_all_connections();
	net_proto_create_random_connections();
	net_proto_remove_stale_requests();
}
