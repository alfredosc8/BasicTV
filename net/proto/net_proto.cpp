#include "../../main.h"
#include "../../util.h"
#include "../../lock.h"
#include "../../settings.h"
#include "../net.h"
#include "../net_socket.h"
#include "../../id/id_api.h"

#include "net_proto.h"
#include "net_proto_socket.h"
#include "net_proto_dev_ctrl.h"
#include "inbound/net_proto_inbound_connections.h"
#include "inbound/net_proto_inbound_data.h"
#include "outbound/net_proto_outbound_connections.h"
#include "outbound/net_proto_outbound_data.h"
#include "net_proto_meta.h"

// socket ID
static id_t_ incoming_id = ID_BLANK_ID;
// peer_id

/*
  TODO: possibly allow for multiple different identities on the network
  at any given time, but that isn't important
 */

static id_t_ self_peer_id = ID_BLANK_ID;

void net_proto_loop(){
	net_socket_t *incoming_socket =
		PTR_DATA(incoming_id, net_socket_t);
	if(incoming_socket == nullptr){
		print("incoming_socket == nullptr", P_ERR);
	}
	// all things inbound
	net_proto_loop_handle_inbound_requests();
	net_proto_loop_accept_all_connections();
	// all things outbound
	net_proto_loop_handle_outbound_requests();
	net_proto_loop_initiate_all_connections();
}

void net_proto_init(){
	net_socket_t *incoming = new net_socket_t;
	incoming_id = incoming->id.get_id();
	/*
	  Assume all IDs that should be imported are from earlier code
	 */
	self_peer_id = id_api::array::fetch_one_from_hash(
		convert::array::type::to("net_proto_peer_t"),
		get_id_hash(production_priv_key_id));
	if(self_peer_id == ID_BLANK_ID){
		print("can't find old net_proto_peer_t information, generating new", P_NOTE);
		self_peer_id = (new net_proto_peer_t)->id.get_id();
		net_proto::peer::set_self_as_peer(
			net_get_ip(),
			settings::get_setting_unsigned_def(
				"network_port",
				58486));
	}
	// uint16_t tmp_port = 0;
	// try{
	// 	tmp_port = (uint16_t)std::stoi(settings::get_setting("network_port"));
	// }catch(std::exception e){
	// 	print("cannot pull port from settings", P_ERR);
	// }
	// TODO: reimplement this when the information is done
	// if(settings::get_setting("socks_enable") == "true"){
	// 	try{
	// 		std::string socks_proxy_ip = settings::get_setting("socks_proxy_ip");
	// 		uint16_t socks_proxy_port =
	// 			std::stoi(settings::get_setting("socks_proxy_port"));
	// 		if(socks_proxy_ip == ""){
	// 			throw std::runtime_error("");
	// 		}
	// 		incoming->enable_socks(
	// 			std::make_pair(socks_proxy_ip,
	// 				       socks_proxy_port),
	// 			std::make_pair("",
	// 				       tmp_port)
	// 			);
	// 	}catch(std::exception e){
	// 		uint32_t level = P_WARN;
	// 		if(settings::get_setting("socks_strict") == "true"){
	// 			level = P_ERR;
	// 		}
	// 		print("unable to configure SOCKS", level);
	// 	}
	// }else{
	// 	print("SOCKS has been disabled", P_NOTE);
	// 	incoming->connect({"", tmp_port});
	// }
}

void net_proto_close(){
	// doesn't do anything, GC takes care of all data types
	// All data types should destroy any internal data
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
	print("actually program this in", P_CRIT);
}
