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
	ADD_DATA(heartbeat_timestamp);
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

uint64_t net_proto_con_req_t::get_heartbeat_timestamp(){
	return heartbeat_timestamp;
}

void net_proto_con_req_t::set(uint8_t flags_,
			      id_t_ first_peer_id_,
			      id_t_ second_peer_id_,
			      id_t_ third_peer_id_,
			      uint64_t heartbeat_timestamp_){
	flags = flags_;
	first_peer_id = first_peer_id_;
	second_peer_id = second_peer_id_;
	third_peer_id = third_peer_id_;
	heartbeat_timestamp = heartbeat_timestamp_;
}

id_t_ net_proto_generate_con_req(id_t_ peer_id){
	net_proto_peer_t *peer_ptr =
		PTR_DATA_FAST(peer_id,
			      net_proto_peer_t);
	if(peer_ptr == nullptr){
		return ID_BLANK_ID;
	}
	print("set networking flags properly", P_CRIT);
	uint8_t net_flags = NET_CON_REQ_HOLEPUNCH;
	uint64_t heartbeat_timestamp = 0;
	if(net_flags & NET_CON_REQ_HOLEPUNCH){
		heartbeat_timestamp =
			get_time_microseconds()+
			settings::get_setting_unsigned_def(
				"net_holepunch_time_offset", 10000000);
		// ten seconds default
	}
	net_proto_con_req_t *proto_con_req =
		new net_proto_con_req_t;
	proto_con_req->set(
		net_flags,
		net_proto::peer::get_self_as_peer(),
		peer_id,
		ID_BLANK_ID,
		heartbeat_timestamp); // reserved for third party for UDP (not implemented)
	return proto_con_req->id.get_id();
}
