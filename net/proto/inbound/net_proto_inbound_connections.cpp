#include "../../../id/id.h"
#include "../../../id/id_api.h"
#include "../../../settings.h"
#include "../net_proto.h"
#include "../net_proto_con_req.h"
#include "../net_proto_socket.h"
#include "../net_proto_peer.h"
#include "../../net_socket.h"
#include "net_proto_inbound_connections.h"

#include "../../../encrypt/encrypt.h"

// socket ID
static id_t_ incoming_id = ID_BLANK_ID;

static void net_proto_accept_direct_connections(net_socket_t *incoming_conn){
	id_t_ new_socket_id = ID_BLANK_ID;
	while((new_socket_id = incoming_conn->accept()) != ID_BLANK_ID){
		net_proto_socket_t *new_proto_socket =
			new net_proto_socket_t;
		new_proto_socket->set_socket_id(
			new_socket_id);
		new_proto_socket->send_id(
			net_proto::peer::get_self_as_peer());
		new_proto_socket->send_id(
			encrypt_api::search::pub_key_from_hash(
				get_id_hash(
					net_proto::peer::get_self_as_peer())));
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
		ASSERT(net_interface::medium::from_address(first_peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_ERR);
		net_interface_ip_address_t *ip_address_ptr =
			PTR_DATA(first_peer_ptr->get_address_id(),
				 net_interface_ip_address_t);
		PRINT_IF_NULL(ip_address_ptr, P_ERR);
		net_socket_t *tmp_socket =
			new net_socket_t;
		net_proto_socket_t *proto_socket =
			new net_proto_socket_t;
		// Should I do this or create a net_con_req_t going out?
		tmp_socket->set_net_ip(
			net_interface::ip::raw::to_readable(
				ip_address_ptr->get_address()),
			ip_address_ptr->get_port());
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
		id_t_ peer_ids[2] = {ID_BLANK_ID, ID_BLANK_ID};
		con_req->get_peer_ids(
			&peer_ids[0], &peer_ids[1], nullptr);
		if(peer_ids[1] != net_proto::peer::get_self_as_peer()){
			continue;
		}
		net_proto_peer_t *proto_peer_ptr =
			PTR_DATA(peer_ids[0],
				 net_proto_peer_t);
		CONTINUE_IF_NULL(proto_peer_ptr, P_WARN);
		ASSERT(net_interface::medium::from_address(proto_peer_ptr->get_address_id()) == NET_INTERFACE_MEDIUM_IP, P_WARN);
		net_interface_ip_address_t *ip_address_ptr =
			PTR_DATA(proto_peer_ptr->get_address_id(),
				 net_interface_ip_address_t);
		CONTINUE_IF_NULL(ip_address_ptr, P_WARN);
		switch(ip_address_ptr->get_packet_encapsulation()){
		case NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_TCP:
			switch(ip_address_ptr->get_nat_type()){
			case NET_INTERFACE_IP_ADDRESS_NAT_TYPE_NONE:
				net_proto_facilitate_tcp_holepunch(con_req);
				break;
			default:
				print("attempting a reverse forward since this section of the code needs updating", P_WARN);
				net_proto_facilitate_reverse_forward(con_req);
				break;
			}
			break;
		case NET_INTERFACE_MEDIUM_PACKET_ENCAPSULATION_UDP_ORDERED:
			print("we have no udp support right now", P_ERR);
			break;
		default:
			print("unknown encapsulation scheme", P_ERR);
			break;
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
 		std::stoi(
			settings::get_setting(
				"net_interface_ip_tcp_port")));
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
