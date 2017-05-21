#include "../../../id/id.h"
#include "../../../id/id_api.h"
#include "../../../settings.h"
#include "../net_proto.h"
#include "../net_proto_con_req.h"
#include "../net_proto_socket.h"
#include "../net_proto_peer.h"
#include "../../net_socket.h"
#include "net_proto_inbound_connections.h"

// socket ID
static id_t_ incoming_id = ID_BLANK_ID;

static void net_proto_accept_direct_connections(net_socket_t *incoming_conn){
	TCPsocket incoming_socket =
		incoming_conn->get_tcp_socket();
	if(incoming_socket == nullptr){
		return;
	}
	TCPsocket tcp_socket = nullptr;
	while((tcp_socket = SDLNet_TCP_Accept(incoming_socket)) != nullptr){
		net_socket_t *new_socket =
			new net_socket_t;
		net_proto_socket_t *new_proto_socket =
			new net_proto_socket_t;
		net_proto_peer_t *fake_proto_peer_ptr =
			new net_proto_peer_t;
		IPaddress *ip_tmp =
			SDLNet_TCP_GetPeerAddress(
				tcp_socket);
		fake_proto_peer_ptr->set_net_ip(
			SDLNet_ResolveIP(ip_tmp),
			NBO_16(ip_tmp->port)); // IPaddress is in NBO
		// proto_peer_ptr should be upgraded
		new_socket->set_tcp_socket(
			tcp_socket);
		new_proto_socket->set_socket_id(
			new_socket->id.get_id());
		// peer information is sent in metadata
		// no information needs to be sent over without a request, so
		// everything should be fine from here on out
	}
}

// returns true on success, false on failure (retval is should I destroy it?)

static bool net_proto_facilitate_tcp_holepunch(net_proto_con_req_t *con_req){
 	print("TODO: implement a TCP hole punch", P_CRIT);
	return false;
}

static bool net_proto_facilitate_reverse_forward(net_proto_con_req_t *con_req){
	try{
		id_t_ first_peer_id = ID_BLANK_ID;
		con_req->get_peer_ids(&first_peer_id,
				      nullptr,
				      nullptr);
		net_proto_peer_t *first_peer_ptr =
			PTR_DATA_FAST(first_peer_id,
				      net_proto_peer_t);
		if(first_peer_ptr == nullptr){
			print("can't establish a reverse forward connection without net_proto_peer_t data", P_ERR);
		}
		net_socket_t *tmp_socket =
			new net_socket_t;
		net_proto_socket_t *proto_socket =
			new net_proto_socket_t;
		// Should I do this or create a net_con_req_t going out?
		tmp_socket->set_net_ip(
			first_peer_ptr->get_net_ip_str(),
			first_peer_ptr->get_net_port());
		tmp_socket->connect();
		proto_socket->set_socket_id(
			tmp_socket->id.get_id());
	}catch(...){
		return false;
	}
	return true;
}

static void net_proto_accept_unorthodox_connections(net_socket_t *incoming_conn){
	std::vector<id_t_> con_req_vector =
		id_api::cache::get(
			"net_proto_con_req_t");
	for(uint64_t i = 0;i < con_req_vector.size();i++){
		net_proto_con_req_t *con_req =
			PTR_DATA_FAST(con_req_vector[i],
				      net_proto_con_req_t);
		if(con_req == nullptr){
			continue;
		}
		id_t_ second_peer_id = ID_BLANK_ID;
		con_req->get_peer_ids(
			nullptr, &second_peer_id, nullptr);
		if(second_peer_id != net_proto::peer::get_self_as_peer()){
			continue;
		}
		switch(con_req->get_flags()){
		case (NET_CON_REQ_TCP | NET_CON_REQ_HOLEPUNCH):
			net_proto_facilitate_tcp_holepunch(con_req);
			break;
		case (NET_CON_REQ_TCP | NET_CON_REQ_REVERSE_FORWARD):
			net_proto_facilitate_reverse_forward(con_req);
			break;
		default:
			print("unrecognized and unimplemented connection method", P_WARN);
		}
		
	}
}

static void net_proto_create_incoming_socket(){
	net_socket_t *incoming_conn =
		new net_socket_t;
	incoming_id =
		incoming_conn->id.get_id();
	incoming_conn->set_net_ip(
		"",
 		settings::get_setting_unsigned_def(
			"network_port",
			58486));
	incoming_conn->connect();
}

void net_proto_accept_all_connections(){
	net_socket_t *incoming_conn =
		PTR_DATA_FAST(incoming_id,
			      net_socket_t);
	if(incoming_conn == nullptr){
		net_proto_create_incoming_socket();
		incoming_conn =
			PTR_DATA_FAST(incoming_id,
				      net_socket_t);
		if(incoming_conn == nullptr){
			print("can't open incoming socket", P_ERR);
		}
        }
	net_proto_accept_direct_connections(incoming_conn);
	net_proto_accept_unorthodox_connections(incoming_conn);
}
