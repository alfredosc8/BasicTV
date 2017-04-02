#include "net_proto.h"
#include "net_proto_peer.h"

net_proto_peer_t::net_proto_peer_t() : id(this, TYPE_NET_PROTO_PEER_T){
	list_virtual_data(&id); // net_ip_t
	id.add_data(&(flags), 1);
	id.add_data(&(net_flags), 1);
}

net_proto_peer_t::~net_proto_peer_t(){
}

uint8_t net_proto_peer_t::get_net_flags(){
	return net_flags;
}

void net_proto_peer_t::set_net_flags(uint8_t net_flags_){
	net_flags = net_flags_;
}

void net_proto_peer_t::set_last_attempted_connect_time(uint64_t time_micro_s){
	last_attempted_connect = time_micro_s;
}

uint64_t net_proto_peer_t::get_last_attempted_connect_time(){
	return last_attempted_connect;
}
