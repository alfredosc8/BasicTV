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
#include "net_proto_connections.h"
#include "net_proto_meta.h"
#include "net_proto_api.h"

void net_proto_loop(){
	net_proto_handle_inbound_requests();
	net_proto_handle_outbound_requests();
	net_proto_connection_manager();
}

static void net_proto_init_self_peer(){
	net_proto::peer::set_self_peer_id(
		id_api::array::fetch_one_from_hash(
			convert::array::type::to("net_proto_peer_t"),
			get_id_hash(production_priv_key_id)));
	const uint16_t tmp_port =
		settings::get_setting_unsigned_def(
			"net_port",
			58486);
	std::string ip_addr =
		settings::get_setting(
			"net_hostname");
	if(ip_addr == ""){
		ip_addr = net_get_ip();
	}else{
		print("assuming the hostname of " + ip_addr, P_NOTE);
	}
	if(net_proto::peer::get_self_as_peer() == ID_BLANK_ID){
		print("can't find old net_proto_peer_t information, generating new", P_NOTE);
		net_proto::peer::set_self_peer_id(
			(new net_proto_peer_t)->id.get_id());
		net_proto::peer::set_self_as_peer(
			ip_addr,
			tmp_port);
	}
}

/*
  Creates a bootstrap node if it doesn't already exist

  Forces no encryption (generated locally)
 */

static std::vector<std::pair<std::string, uint16_t> > bootstrap_nodes;

static void net_proto_verify_bootstrap_nodes(){
	std::vector<id_t_> peer_vector =
		id_api::cache::get(
			"net_proto_peer_t");
	try{
		const std::string custom_bootstrap_ip =
			settings::get_setting(
				"net_proto_custom_bootstrap_ip");
		if(custom_bootstrap_ip != ""){
			print("adding custom bootstrap ip from settings", P_NOTE);
			bootstrap_nodes.push_back(
				std::make_pair(
					custom_bootstrap_ip,
					std::stoi(
						settings::get_setting(
							"net_proto_custom_bootstrap_port"))));
		}
	}catch(...){
		print("no custom bootstrap node specified", P_NOTE);
	}
	std::vector<std::pair<std::string, uint16_t> > nodes_to_connect;
	if(bootstrap_nodes.size() > 0){
		nodes_to_connect = 
		std::vector<std::pair<std::string, uint16_t> >(
			bootstrap_nodes.begin(),
			bootstrap_nodes.end());
	}
	print("attempting to read in " + std::to_string(nodes_to_connect.size()) + " bootstrap nodes", P_DEBUG);
	for(uint64_t i = 0;i < peer_vector.size();i++){
		net_proto_peer_t *proto_peer =
			PTR_DATA(peer_vector[i],
				 net_proto_peer_t);
		if(proto_peer == nullptr){
			continue;
		}
		const std::string ip_addr =
			proto_peer->get_net_ip_str();
		auto bootstrap_iter =
			std::find_if(
				nodes_to_connect.begin(),
				nodes_to_connect.end(),
				[&ip_addr](std::pair<std::string, uint16_t> const& elem){
					return ip_addr == elem.first;
				});
		if(bootstrap_iter != nodes_to_connect.end()){
			// remove duplicates, prefer encrypted version
			nodes_to_connect.erase(
				bootstrap_iter);
		}
	}
	for(uint64_t i = 0;i < nodes_to_connect.size();i++){
		net_proto_peer_t *proto_peer_ptr =
			new net_proto_peer_t;
		// no harm in assuming port is open
		// WRONG_KEY forces no encryption
		proto_peer_ptr->set_net_flags(
			NET_PEER_WRONG_KEY | NET_PEER_PORT_OPEN);
		proto_peer_ptr->set_net_ip(
			nodes_to_connect[i].first,
			nodes_to_connect[i].second);
		print("created peer with IP " + nodes_to_connect[i].first +
		      " and port " + std::to_string(nodes_to_connect[i].second),
		      P_NOTE);
	}
	const uint64_t start_node_count =
		id_api::cache::get(
			"net_proto_peer_t").size()-1; // don't count ourselves
	print("starting with " + std::to_string(start_node_count) + " unique nodes", P_NOTE);
}

void net_proto_init(){
	id_api::import::load_all_of_type(
		"net_proto_peer_t",
		ID_API_IMPORT_FROM_DISK);
	net_proto_init_self_peer();
	net_proto_verify_bootstrap_nodes();
	/*
	  TODO: connect net_proxy_t to net_socket_t somehow (default setting)
	 */
	if(settings::get_setting("socks_enable") == "true"){
		try{
			std::string socks_proxy_ip = settings::get_setting("socks_proxy_ip");
			uint16_t socks_proxy_port =
				std::stoi(settings::get_setting("socks_proxy_port"));
			if(socks_proxy_ip == ""){
				throw std::runtime_error("");
			}
			print("need to implement SOCKS with sockets", P_ERR);
			net_proxy_t *proxy_ptr =
				new net_proxy_t;
			proxy_ptr->set_net_ip(
				socks_proxy_ip,
				socks_proxy_port);
		}catch(std::exception e){
			const bool strict =
				settings::get_setting("socks_strict") == "true";
	 		print("unable to configure SOCKS",
			      (strict) ? P_ERR : P_NOTE);
	 	}
	}else{
	 	print("SOCKS has been disabled", P_NOTE);
	}
}

void net_proto_close(){
	// doesn't do anything, GC takes care of all data types
	// All data types should destroy any internal data
}
