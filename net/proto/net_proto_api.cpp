#include "../../id/id_api.h"
#include "../../util.h"
#include "../../settings.h"
#include "net_proto.h"
#include "net_proto_api.h"
#include "net_proto_socket.h"
#include "net_proto_peer.h"
#include "net_proto_routine_requests.h"
#include "net_proto_con_req.h"

static id_t_ self_peer_id = ID_BLANK_ID;
static std::vector<id_t_> id_request_buffer;
static std::vector<std::pair<id_t_, int64_t> > linked_list_request_buffer;
static std::vector<type_t_ > type_request_buffer; // ?

static id_t_ net_proto_preferable_id_from_hash(
	std::array<uint8_t, 32> hash){
	id_t_ matching_hash_peer_id =
		ID_BLANK_ID;
	std::vector<id_t_> peer_vector =
		id_api::cache::get(
			TYPE_NET_PROTO_PEER_T);
	for(uint64_t i = 0;i < peer_vector.size();i++){
		if(unlikely(hash ==
			    get_id_hash(peer_vector[i]))){
			matching_hash_peer_id =
				peer_vector[i];
		}
	}
	if(matching_hash_peer_id == ID_BLANK_ID){
		matching_hash_peer_id =
			net_proto::peer::random_peer_id();
	}
	if(matching_hash_peer_id == ID_BLANK_ID){
		print("no connected peers to facilitate request", P_SPAM);
	}
	return matching_hash_peer_id;
}


void net_proto::request::add_id(id_t_ id){
	// could probably speed this up
	for(uint64_t i = 0;i < id_request_buffer.size();i++){
		if(get_id_hash(id) == get_id_hash(id_request_buffer[i])){
			id_request_buffer.insert(
				id_request_buffer.begin()+i,
				id);
			break;
		}
	}
	id_request_buffer.push_back(id);
}

void net_proto::request::add_id(std::vector<id_t_> id){
	// could probably speed this up
	for(uint64_t c = 0;c < id.size();c++){
		for(uint64_t i = 0;i < id_request_buffer.size();i++){
			while(get_id_hash(id[c]) == get_id_hash(id_request_buffer[i])){
				id_request_buffer.insert(
					id_request_buffer.begin()+i,
					id[c]);
				if(c < id.size()){
					c++;
				}
			}
		}
		id_request_buffer.push_back(id[c]);
	}
}

/*
  Signed-ness is important for dictating direction
 */

void net_proto::request::add_id_linked_list(id_t_ id, int64_t length){
	for(uint64_t i = 0;i < linked_list_request_buffer.size();i++){
		if(unlikely(get_id_hash(linked_list_request_buffer[i].first) ==
			    get_id_hash(id))){
			std::pair<id_t_, int64_t> request_entry =
				std::make_pair(
					id, length);
			linked_list_request_buffer.insert(
				linked_list_request_buffer.begin()+i,
				request_entry);
			return;
		}
	}
	linked_list_request_buffer.push_back(
		std::make_pair(
			id, length));
}

void net_proto::request::del_id(id_t_ id){
	std::vector<id_t_> id_request_vector =
		id_api::cache::get(
			"net_proto_id_request_t");
	for(uint64_t i = 0;i < id_request_vector.size();i++){
		net_proto_id_request_t *id_request =
			PTR_DATA(id_request_vector[i],
				 net_proto_id_request_t);
		if(id_request == nullptr){
			continue;
		}
		std::vector<id_t_> id_vector =
			id_request->get_ids();
		auto id_ptr =
			std::find(
				id_vector.begin(),
				id_vector.end(),
				id);
		if(id_ptr != id_vector.end()){
			id_vector.erase(
				id_ptr);
			id_request->set_ids(
				id_vector);
			return;
		}
	}
	print("cannot delete request for ID I didn't request", P_ERR);
}

void net_proto::peer::set_self_peer_id(id_t_ self_peer_id_){
	self_peer_id = self_peer_id_;
}

/*
  Should be done in net_proto_init, actually
 */

void net_proto::peer::set_self_as_peer(std::string ip, uint16_t port){
	net_proto_peer_t *proto_peer =
		PTR_DATA(self_peer_id,
			 net_proto_peer_t);
	PRINT_IF_NULL(proto_peer, P_ERR);
	print("TODO: check for open TCP ports, checking settings file", P_WARN);
	uint8_t net_flags_tmp = NET_PEER_PUNCHABLE;
	print("assuming we are TCP hole punchable", P_NOTE);
	if(settings::get_setting("net_open_tcp_port") == "true"){
		print("we have an open TCP port defined in settings, adding to my net_proto_peer_t", P_NOTE);
		net_flags_tmp |= NET_PEER_PORT_OPEN;
	}else{
		print("we don't have an open TCP port defined in settings, consider making one for maximum connectivity", P_NOTE);
	}
	proto_peer->set_net_flags(net_flags_tmp);
	proto_peer->set_net_ip(ip, port);
}

id_t_ net_proto::peer::get_self_as_peer(){
	return self_peer_id;
}

std::vector<id_t_> net_proto::socket::all_proto_socket_of_peer(id_t_ peer_id){
	std::vector<id_t_> retval;
	std::vector<id_t_> proto_socket_vector =
		id_api::cache::get(
			"net_proto_socket_t");
	for(uint64_t i = 0;i < proto_socket_vector.size();i++){
		net_proto_socket_t *proto_socket =
			PTR_DATA(proto_socket_vector[i],
				 net_proto_socket_t);
		if(proto_socket == nullptr){
			continue;
		}
		if(unlikely(proto_socket->get_peer_id() == peer_id)){
			retval.push_back(
				proto_socket_vector[i]);
		}
	}
	return retval;
}

id_t_ net_proto::socket::optimal_proto_socket_of_peer(id_t_ peer_id){
	std::vector<id_t_> proto_socket_vector =
		all_proto_socket_of_peer(
			peer_id);
	std::pair<id_t_, uint64_t> optimal_socket = {ID_BLANK_ID, 0};
	for(uint64_t i = 0;i < proto_socket_vector.size();i++){
		net_proto_socket_t *proto_socket =
			PTR_DATA(proto_socket_vector[i],
				 net_proto_socket_t);
		if(proto_socket == nullptr){
			continue;
		}
		if(optimal_socket.first == ID_BLANK_ID ||
		   proto_socket->get_last_recv_micro_s() > optimal_socket.second){
			optimal_socket =
				std::make_pair(
					proto_socket_vector[i],
					proto_socket->get_last_recv_micro_s());
		}
	}
	P_V_S(convert::array::id::to_hex(optimal_socket.first), P_VAR);
	return optimal_socket.first;
}

id_t_ net_proto::peer::random_peer_id(){
	std::vector<id_t_> proto_peer_vector =
		id_api::cache::get(
			TYPE_NET_PROTO_PEER_T);
	if(proto_peer_vector.size() >= 2){
		std::random_shuffle(
			proto_peer_vector.begin(),
			proto_peer_vector.begin()+2);
		for(uint64_t i = 0;i < 2;i++){
			if(proto_peer_vector[i] !=
			   net_proto::peer::get_self_as_peer()){
				return proto_peer_vector[i];
			}
		}
	}else{
		P_V(proto_peer_vector.size(), P_VAR);
	}
	print("no other peers detected, cannot return a valid peer id", P_WARN);
	return ID_BLANK_ID;
}

void net_proto::socket::connect(id_t_ peer_id_, uint32_t min){
	std::vector<id_t_> retval;
	net_proto_peer_t *proto_peer_ptr =
		PTR_DATA(peer_id_,
			 net_proto_peer_t);
	P_V_S(convert::array::id::to_hex(peer_id_), P_VAR);
	if(proto_peer_ptr == nullptr){
		print("cannot connect to a null peer", P_ERR);
	}
	P_V_S(proto_peer_ptr->get_net_ip_str(), P_VAR);
	P_V(proto_peer_ptr->get_net_port(), P_VAR);
	int64_t sockets_to_open =
		min-all_proto_socket_of_peer(peer_id_).size();
	P_V(sockets_to_open, P_VAR);
	for(;sockets_to_open > 0;sockets_to_open--){
		net_proto_generate_con_req(peer_id_);
	}
}

void net_proto::request::add_fast_routine_type(std::string type){
	routine_request_fast_vector.push_back(
		convert::type::to(
			type));
}

void net_proto::request::add_slow_routine_type(std::string type){
	routine_request_slow_vector.push_back(
		convert::type::to(
			type));
}

void net_proto::request::del_fast_routine_type(std::string type){
	auto iterator =
		std::find(
			routine_request_fast_vector.begin(),
			routine_request_fast_vector.end(),
			convert::type::to(type));
	if(iterator != routine_request_fast_vector.end()){
		routine_request_fast_vector.erase(
			iterator);
	}
}

void net_proto::request::del_slow_routine_type(std::string type){
	auto iterator =
		std::find(
			routine_request_slow_vector.begin(),
			routine_request_slow_vector.end(),
			convert::type::to(type));
	if(iterator != routine_request_slow_vector.end()){
		routine_request_slow_vector.erase(
			iterator);
	}
}
