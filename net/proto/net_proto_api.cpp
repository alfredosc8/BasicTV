#include "../../id/id_api.h"
#include "../../util.h"
#include "net_proto.h"
#include "net_proto_api.h"
#include "net_proto_socket.h"
#include "net_proto_peer.h"

static id_t_ self_peer_id = ID_BLANK_ID;

void net_proto::peer::set_self_peer_id(id_t_ self_peer_id_){
	self_peer_id = self_peer_id_;
}

void net_proto::peer::set_self_as_peer(std::string ip, uint16_t port){
	net_proto_peer_t *proto_peer =
		PTR_DATA(self_peer_id,
			 net_proto_peer_t);
	PRINT_IF_NULL(proto_peer, P_ERR);
	proto_peer->set_net_ip(ip, port, NET_IP_VER_4); // ?
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
		   proto_socket->get_last_update_micro_s() > optimal_socket.second){
			optimal_socket =
				std::make_pair(
					proto_socket_vector[i],
					proto_socket->get_last_update_micro_s());
		}
	}
	return optimal_socket.first;
}

std::vector<id_t_> net_proto::socket::connect(id_t_ peer_id_, uint32_t min){
	std::vector<id_t_> retval;
	net_proto_peer_t *proto_peer_ptr =
		PTR_DATA(peer_id_,
			 net_proto_peer_t);
	if(proto_peer_ptr == nullptr){
		return retval;
	}
	int64_t sockets_to_open =
		min-all_proto_socket_of_peer(peer_id_).size();
	for(;sockets_to_open > 0;sockets_to_open--){
		net_proto_socket_t *proto_socket =
			new net_proto_socket_t;
		proto_socket->set_peer_id(peer_id_);
		proto_socket->update_connection();
		retval.push_back(
			proto_socket->id.get_id());
	}
	return retval;
}

