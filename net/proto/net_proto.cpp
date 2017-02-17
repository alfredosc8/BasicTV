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
#include "net_proto_api.h"

// TODO: remove "loop" from all of these to make it more uniform

void net_proto_loop(){
	// all things inbound
	net_proto_handle_inbound_requests();
	net_proto_accept_all_connections();
	// all things outbound
	net_proto_handle_outbound_requests();
	net_proto_initiate_all_connections();
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

void net_proto_init(){
	net_proto_init_self_peer();
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
