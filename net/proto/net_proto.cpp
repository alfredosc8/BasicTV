#include "../../main.h"
#include "../../util.h"
#include "../../lock.h"
#include "../../settings.h"
#include "../net.h"
#include "../net_socket.h"
#include "../../id/id_api.h"

#include "net_proto.h"
#include "net_proto_socket.h"
#include "inbound/net_proto_inbound_connections.h"
#include "inbound/net_proto_inbound_data.h"
#include "outbound/net_proto_outbound_connections.h"
#include "outbound/net_proto_outbound_data.h"
#include "net_proto_connections.h"
#include "net_proto_meta.h"
#include "net_proto_api.h"
#include "net_proto_request.h"


/*
  Goes through and cleans out network requests that are irrelevant.
 */

void net_proto_loop(){
	net_proto_handle_inbound_data();
	net_proto_handle_outbound_requests();
	net_proto_connection_manager(); // in and out
	net_proto_requests_loop();
}

/*
  NEW IDEA:
  We can mark all of our own net_proto_peer_ts as non-exportable
  but networkable. Other people can save ours, as they are not
  configured to be non-exportable (non-exporability and
  non-networkability only apply to local version, once exported
  they are reset).
 */

static void net_proto_init_self_peer(){
	if(net_proto::peer::get_self_as_peer() != ID_BLANK_ID){
		print("We already have local peer data (at init), where did it come from?", P_ERR);
	}
	if(settings::get_setting("net_interface_ip_tcp_enabled") == "true"){
		const uint16_t tmp_port =
			std::stoi(
				settings::get_setting(
					"net_interface_ip_tcp_port"));
		std::string ip_addr = "";
		try{
			ip_addr =
				settings::get_setting(
					"net_interface_ip_hostname");
		}catch(...){
			print("finding IP address automatically", P_NOTE);
			ip_addr = net_get_ip();
		}
		print("Peer information as seen by the network is " + ip_addr + ":" + std::to_string(tmp_port), P_NOTE);
		P_V(tmp_port, P_NOTE);
		P_V_S(ip_addr, P_NOTE);
		net_proto_peer_t *proto_peer_ptr =
			new net_proto_peer_t;
		proto_peer_ptr->id.set_lowest_global_flag_level(
			ID_DATA_NETWORK_RULE_PUBLIC,
			ID_DATA_EXPORT_RULE_NEVER,
			ID_DATA_PEER_RULE_ALWAYS);
		net_interface_ip_address_t *ip_address_ptr =
			new net_interface_ip_address_t;
		ip_address_ptr->set_medium_modulation_encapsulation(
			NET_INTERFACE_MEDIUM_IP,
			NET_INTERFACE_MEDIUM_PACKET_MODULATION_TCP,
			NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_TCP);
		ip_address_ptr->set_address_data(
			ip_addr,
			tmp_port,
			NET_INTERFACE_IP_ADDRESS_NAT_TYPE_NONE);
		net_proto::peer::set_self_peer_id(
			proto_peer_ptr->id.get_id());
		P_V_S(net_interface::ip::raw::to_readable(
			      ip_address_ptr->get_address()), P_VAR);
		P_V(ip_address_ptr->get_port(), P_VAR);
	}
	if(settings::get_setting("net_interface_ip_udp_enabled") == "true"){
		print("we have no UDP support, disable it in the settings", P_ERR);
	}
}

/*
  Creates a bootstrap node if it doesn't already exist

  Forces no encryption (generated locally)
 */

static std::vector<std::pair<std::string, uint16_t> > bootstrap_nodes = {};

static void net_proto_verify_bootstrap_nodes(){
	std::vector<id_t_> peer_vector =
		id_api::cache::get(
			TYPE_NET_PROTO_PEER_T);
	try{
		const std::string custom_bootstrap_ip =
			settings::get_setting(
				"net_proto_ip_tcp_bootstrap_ip");
		if(custom_bootstrap_ip != ""){
			print("adding custom bootstrap ip from settings", P_NOTE);
			bootstrap_nodes.push_back(
				std::make_pair(
					custom_bootstrap_ip,
					std::stoi(
						settings::get_setting(
							"net_proto_ip_tcp_bootstrap_port"))));
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
		net_proto_peer_t *tmp_proto_peer =
			PTR_DATA(peer_vector[i],
				 net_proto_peer_t);
		CONTINUE_IF_NULL(tmp_proto_peer, P_WARN);
		net_interface_ip_address_t *tmp_ip_address_ptr =
			PTR_DATA(tmp_proto_peer->get_address_id(),
				 net_interface_ip_address_t);
		CONTINUE_IF_NULL(tmp_ip_address_ptr, P_WARN);
		const std::vector<uint8_t> ip_addr =
			tmp_ip_address_ptr->get_address().first;
		const uint16_t port =
			tmp_ip_address_ptr->get_port();
		try{
			auto bootstrap_iter =
				std::find_if(
					nodes_to_connect.begin(),
					nodes_to_connect.end(),
					[&ip_addr, &port](std::pair<std::string, uint16_t> const& elem){
						return ip_addr == net_interface::ip::readable::to_raw(elem.first).first &&
						port == elem.second; // always not in NBO
					});
			if(bootstrap_iter != nodes_to_connect.end()){
				// remove duplicates, prefer encrypted version
				P_V_S(net_interface::ip::raw::to_readable(
					      std::make_pair(
						      ip_addr, NET_INTERFACE_IP_ADDRESS_TYPE_IPV4)), P_SPAM);
				P_V(port, P_SPAM);
				print("erasing obsolete bootstrap information", P_SPAM);
				nodes_to_connect.erase(
					bootstrap_iter);
				continue;
			}
		}catch(...){}
	}
	for(uint64_t i = 0;i < nodes_to_connect.size();i++){
		net_proto_peer_t *proto_peer_ptr =
			new net_proto_peer_t;
		net_interface_ip_address_t *ip_address_ptr =
			new net_interface_ip_address_t;
		ip_address_ptr->set_medium_modulation_encapsulation(
			NET_INTERFACE_MEDIUM_IP,
			NET_INTERFACE_MEDIUM_PACKET_MODULATION_TCP,
			NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_TCP);
		ip_address_ptr->set_address_data(
			nodes_to_connect[i].first,
			nodes_to_connect[i].second,
			NET_INTERFACE_IP_ADDRESS_NAT_TYPE_NONE);
		proto_peer_ptr->set_address_id(
			ip_address_ptr->id.get_id());
		proto_peer_ptr->id.set_lowest_global_flag_level(
			ID_DATA_RULE_UNDEF,
			ID_DATA_EXPORT_RULE_NEVER,
			ID_DATA_RULE_UNDEF);
		ip_address_ptr->id.set_lowest_global_flag_level(
			ID_DATA_RULE_UNDEF,
			ID_DATA_EXPORT_RULE_NEVER,
			ID_DATA_RULE_UNDEF);
		// no harm in assuming port is open
		// WRONG_KEY forces no encryption
		print("created peer with IP " + nodes_to_connect[i].first +
		      " and port " + std::to_string(nodes_to_connect[i].second),
		      P_NOTE);
	}
	const uint64_t start_node_count =
		id_api::cache::get(
			"net_proto_peer_t").size()-1; // don't count ourselves
	if(start_node_count == 0){
		// only OK if i'm connecting with another peer first
		print("no peers exist whatsoever, I better know what i'm doing", P_WARN);
	}
	print("starting with " + std::to_string(start_node_count) + " unique nodes", P_NOTE);
}

static void net_proto_init_proxy(){
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
			print("added a proxy of " + socks_proxy_ip + ":" + std::to_string(socks_proxy_port), P_NOTE);
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

void net_proto_init(){
	id_api::import::load_all_of_type(
		"net_proto_peer_t",
		ID_API_IMPORT_FROM_DISK);
	net_proto_init_self_peer();	
	net_proto_init_proxy();
	net_proto_verify_bootstrap_nodes();
}

void net_proto_close(){
	// doesn't do anything, GC takes care of all data types
	// All data types should destroy any internal data
}
