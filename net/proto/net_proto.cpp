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

// socket ID
static id_t_ incoming_id = ID_BLANK_ID;

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
	net_proto::peer::set_self_peer_id(
		id_api::array::fetch_one_from_hash(
			convert::array::type::to("net_proto_peer_t"),
			get_id_hash(production_priv_key_id)));
	if(net_proto::peer::get_self_as_peer() == ID_BLANK_ID){
		print("can't find old net_proto_peer_t information, generating new", P_NOTE);
		net_proto::peer::set_self_peer_id(
			(new net_proto_peer_t)->id.get_id());
		net_proto::peer::set_self_as_peer(
			net_get_ip(),
			settings::get_setting_unsigned_def(
				"network_port",
				58486));
	}
	const uint16_t tmp_port =
		settings::get_setting_unsigned_def(
			"network_port",
			58486);
	// TODO: reimplement this when the information is done
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
				socks_proxy_port,
				NET_IP_VER_4);
		}catch(std::exception e){
			const bool strict =
				settings::get_setting("socks_strict") == "true";
	 		print("unable to configure SOCKS",
			      (strict) ? P_ERR : P_NOTE);
	 	}
	}else{
	 	print("SOCKS has been disabled", P_NOTE);
		incoming->set_net_ip("", tmp_port, NET_IP_VER_4);
	 	incoming->connect();
	}
}

void net_proto_close(){
	// doesn't do anything, GC takes care of all data types
	// All data types should destroy any internal data
}
