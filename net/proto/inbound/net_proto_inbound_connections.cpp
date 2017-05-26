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
		IPaddress *ip_tmp =
			SDLNet_TCP_GetPeerAddress(
				tcp_socket);
		if(ip_tmp == nullptr){
			print("couldn't pull ip info from new socket: " + (std::string)SDL_GetError(), P_WARN);
		}
		new_socket->set_tcp_socket(
			tcp_socket);
		new_proto_socket->set_socket_id(
			new_socket->id.get_id());
		new_proto_socket->send_id(
			net_proto::peer::get_self_as_peer());
		/*
		  I can't pull any valid IP or port info from SDL, so we are
		  going to live without it until data comes in, then we can
		  set it directly from the metadata.
		 */
		print("accepted a peer connection", P_NOTE);
	}
}

// returns true on success, false on failure (retval is should I destroy it?)

// this function is both inbound and outbound connections, since TCP holepunches
// are the same in practice

// the function isn't written yet, but sanity checking the pointer to con_req
// should happen regardless of the implementation
static bool net_proto_facilitate_tcp_holepunch(net_proto_con_req_t *con_req){
	net_socket_t *incoming_socket_ptr =
		PTR_DATA(incoming_id,
			 net_socket_t);
	if(incoming_socket_ptr == nullptr){
		// shouldn't happen ever
		print("incoming_socket_ptr is a nullptr", P_ERR);
	}
	id_t_ first_peer_id = ID_BLANK_ID;
	id_t_ second_peer_id = ID_BLANK_ID;
	id_t_ third_peer_id = ID_BLANK_ID;
	const id_t_ self_as_peer_id =
		net_proto::peer::get_self_as_peer();
	con_req->get_peer_ids(
		&first_peer_id,
		&second_peer_id,
		&third_peer_id);
	if(third_peer_id != ID_BLANK_ID){
		print("doing a simple TCP holepunch, but a third peer ID is give,, TCP doesn't need this", P_WARN);
		if(third_peer_id == self_as_peer_id){
			print("i'm the third peer ID, weird...", P_WARN);
		}
	}
	id_t_ peer_id = ID_BLANK_ID;
	if(first_peer_id == self_as_peer_id &&
	   second_peer_id != self_as_peer_id){
		peer_id = second_peer_id;
	}else if(first_peer_id != self_as_peer_id &&
		 second_peer_id == self_as_peer_id){
		peer_id = first_peer_id;
	}else{
		print("invalid peer ID configuration, I need to be only one", P_ERR);
	}
	/*
	  I'm hoping I can just do all of the fancy connecting work over the
	  incoming socket and let direct_connect whatever take care of importing
	  the socket and doing that stuff. Worst case scenario is I just write
	  some extra code here to do that (probably to signify connection type
	  used in the socket)
	 */
	TCPsocket sdl_tcp_socket =
		incoming_socket_ptr->get_tcp_socket();
	if(sdl_tcp_socket == nullptr){
		print("sdl_tcp_socket is a nullptr", P_ERR);
	}
 	print("TODO: implement a TCP hole punch", P_CRIT);
	return false;
}

static bool net_proto_facilitate_reverse_forward(
	net_proto_con_req_t *con_req){
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

static void net_proto_accept_unorthodox_connections(){
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
			"net_port",
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
	net_proto_accept_unorthodox_connections();
}
