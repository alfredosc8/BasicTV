#include "../../id/id.h"
#include "../../id/id_api.h"
#include "../../settings.h"
#include "net_proto.h"
#include "net_proto_peer.h"
#include "net_proto_con_req.h"

net_proto_con_req_t::net_proto_con_req_t() : id(this, __FUNCTION__){
	ADD_DATA(flags);
	ADD_DATA(first_peer_id);
	ADD_DATA(second_peer_id);
	ADD_DATA(third_peer_id);
	ADD_DATA(timestamp);
}

net_proto_con_req_t::~net_proto_con_req_t(){
}

uint8_t net_proto_con_req_t::get_flags(){
	return flags;
}

void net_proto_con_req_t::get_peer_ids(id_t_ *first_peer_id_,
				       id_t_ *second_peer_id_,
				       id_t_ *third_peer_id_){
	if(first_peer_id_ != nullptr){
		*first_peer_id_ = first_peer_id;
	}
	if(second_peer_id_ != nullptr){
		*second_peer_id_ = second_peer_id;
	}
	if(third_peer_id_ != nullptr){
		*third_peer_id_ = third_peer_id;
	}
}

uint64_t net_proto_con_req_t::get_timestamp(){
	return timestamp;
}

void net_proto_con_req_t::set(uint8_t flags_,
			      id_t_ first_peer_id_,
			      id_t_ second_peer_id_,
			      id_t_ third_peer_id_,
			      uint64_t timestamp_){
	flags = flags_;
	first_peer_id = first_peer_id_;
	second_peer_id = second_peer_id_;
	third_peer_id = third_peer_id_;
	timestamp = timestamp_;
}

static uint8_t peer_connection_flags_to_con_req(net_proto_peer_t *peer_ptr){
	uint8_t retval = 0;
	const uint8_t peer_flags = peer_ptr->get_net_flags();
	if((peer_flags & 0b11) == NET_PEER_TCP){
		retval = NET_CON_REQ_TCP;
	}else if((peer_flags & 0b11) == NET_PEER_UDP){
		retval = NET_CON_REQ_UDP;
	}
	if(peer_flags & NET_PEER_PORT_OPEN){
		return retval | NET_CON_REQ_DIRECT;
	}else if(peer_flags & NET_PEER_PUNCHABLE){
		return retval | NET_CON_REQ_HOLEPUNCH;
	}
	return retval;
}

id_t_ net_proto_generate_con_req(id_t_ peer_id){
	if(peer_id == net_proto::peer::get_self_as_peer()){
		//print("attempted to connect to myself, not connecting", P_WARN);
		return ID_BLANK_ID;
	}
	net_proto_peer_t *peer_ptr =
		PTR_DATA_FAST(peer_id,
			      net_proto_peer_t);
	if(peer_ptr == nullptr){
		return ID_BLANK_ID;
	}
	const uint8_t net_flags =
		peer_connection_flags_to_con_req(peer_ptr);
	uint64_t timestamp = 0;
	net_proto_con_req_t *proto_con_req =
		new net_proto_con_req_t;
	proto_con_req->set(
		net_flags,
		net_proto::peer::get_self_as_peer(),
		peer_id,
		ID_BLANK_ID,
		get_time_microseconds()); // reserved for third party for UDP (not implemented)
	print("started connection negotiation with peer " +
	      convert::array::id::to_hex(peer_id) + " (IP: " +
	      peer_ptr->get_net_ip_str() + " port: " +
	      std::to_string(peer_ptr->get_net_port()) + ")", P_NOTE);
	return proto_con_req->id.get_id();
}
