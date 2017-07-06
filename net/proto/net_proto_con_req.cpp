#include "../../id/id.h"
#include "../../id/id_api.h"
#include "../../settings.h"
#include "net_proto.h"
#include "net_proto_peer.h"
#include "net_proto_con_req.h"

net_proto_con_req_t::net_proto_con_req_t() : id(this, TYPE_NET_PROTO_CON_REQ_T){
	ADD_DATA(first_peer_id);
	ADD_DATA(second_peer_id);
	ADD_DATA(third_peer_id);
	ADD_DATA(timestamp);
	id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_PUBLIC,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
}

net_proto_con_req_t::~net_proto_con_req_t(){
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

void net_proto_con_req_t::set(id_t_ first_peer_id_,
			      id_t_ second_peer_id_,
			      id_t_ third_peer_id_,
			      uint64_t timestamp_){
	first_peer_id = first_peer_id_;
	second_peer_id = second_peer_id_;
	third_peer_id = third_peer_id_;
	timestamp = timestamp_;
}
