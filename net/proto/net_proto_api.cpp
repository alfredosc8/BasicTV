#include "../../id/id_api.h"
#include "../../util.h"
#include "../../settings.h"
#include "net_proto.h"
#include "net_proto_api.h"
#include "net_proto_socket.h"
#include "net_proto_peer.h"
#include "net_proto_request.h"
#include "net_proto_con_req.h"

static id_t_ self_peer_id = ID_BLANK_ID;

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



void net_proto::peer::set_self_peer_id(id_t_ self_peer_id_){
	self_peer_id = self_peer_id_;
}

/*
  Should be done in net_proto_init, actually
 */

static uint8_t net_proto_generate_ip_address_nat_type(){
	print("assuming there is no NAT (not safe, but it'll work for now?)", P_WARN);
	return NET_INTERFACE_IP_ADDRESS_NAT_TYPE_NONE;
}

void net_proto::peer::set_self_as_peer(std::string ip, uint16_t port){
	net_proto_peer_t *proto_peer =
		PTR_DATA(self_peer_id,
			 net_proto_peer_t);
	PRINT_IF_NULL(proto_peer, P_ERR);
	// ASSERT(net_interface::medium::from_address(proto_peer->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_ERR);
	net_interface_ip_address_t *ip_address_ptr =
		PTR_DATA(proto_peer->get_address_id(),
			 net_interface_ip_address_t);
	PRINT_IF_NULL(ip_address_ptr, P_ERR);
	ip_address_ptr->set_address_data(
		ip,
		port,
		net_proto_generate_ip_address_nat_type());
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
	//print("no other peers detected, cannot return a valid peer id", P_WARN);
	return ID_BLANK_ID;
}

/*
  The holepunching stuff here should also be abstracted out as a requirement
  for connection initiation, but it works fine for here for now
 */

static id_t_ net_proto_generate_con_req(id_t_ peer_id){
	net_proto_peer_t *proto_peer_ptr =
		PTR_DATA(peer_id,
			 net_proto_peer_t);
	PRINT_IF_NULL(proto_peer_ptr, P_ERR);
	// ASSERT(net_interface::medium::from_address(proto_peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_UNABLE);
	net_proto_con_req_t *con_req_ptr =
		new net_proto_con_req_t;
	con_req_ptr->set(
		net_proto::peer::get_self_as_peer(),
		peer_id,
		ID_BLANK_ID,
		get_time_microseconds()+10000000);  // TODO: make 10s a settings
	return con_req_ptr->id.get_id();
}

/*
  TODO: should define hardware devices, software devices, and all that jazz
  when that part of the program is developed enough
 */

void net_proto::socket::connect(id_t_ peer_id_, uint32_t min){
	std::vector<id_t_> retval;
	net_proto_peer_t *proto_peer_ptr =
		PTR_DATA(peer_id_,
			 net_proto_peer_t);
	if(proto_peer_ptr == nullptr){
		print("cannot connect to a null peer" + id_breakdown(peer_id_), P_WARN);
	}	
	net_interface_ip_address_t *ip_address_ptr =
		PTR_DATA(proto_peer_ptr->get_address_id(),
			 net_interface_ip_address_t);
	PRINT_IF_NULL(ip_address_ptr);
	const int64_t sockets_to_open =
		min-all_proto_socket_of_peer(peer_id_).size();
	for(int64_t i = 0;i < sockets_to_open;i){
		net_proto_generate_con_req(peer_id_);
	}
	print("created " + std::to_string(sockets_to_open) + " connections to peer" + convert::array::id::to_hex(peer_id_) + " as " + net_interface::ip::ra::to_readable(ip_address_ptr->get_address()) + " : " + std::to_string(ip_address_ptr->get_port()), P_NOTE);
}


#pragma message("optimal_peer_for_id only searches for matching hash, this isn't sustainable for large-scale deployment")

id_t_ net_proto::peer::optimal_peer_for_id(id_t_ id){
	const std::vector<id_t_> proto_peer_vector =
		id_api::cache::get(TYPE_NET_PROTO_PEER_T);
	const hash_t_ id_hash =
		get_id_hash(id);
	for(uint64_t i = 0;i < proto_peer_vector.size();i++){
		if(id_hash == get_id_hash(proto_peer_vector[i])){
			// wrong, but fail-safe, assumption that there can
			// be only one peer per hash
			return proto_peer_vector[i];
		}
	}
	return ID_BLANK_ID;
}
