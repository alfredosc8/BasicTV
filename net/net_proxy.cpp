#include "net_proxy.h"
#include "../math/math.h"

net_proxy_t::net_proxy_t() : id(this, TYPE_NET_PROXY_T){
}

net_proxy_t::~net_proxy_t(){
}

void net_proxy_t::set_flags(uint8_t flags_){
	flags = flags_;
}

uint8_t net_proxy_t::get_flags(){
	return flags;
}

void net_proxy_t::set_outbound_stat_sample_set_id(id_t_ outbound_stat_sample_set_id_){
	outbound_stat_sample_set_id = outbound_stat_sample_set_id_;
}

id_t_ net_proxy_t::get_outbound_stat_sample_set_id(){
	return outbound_stat_sample_set_id;
}

void net_proxy_t::set_inbound_stat_sample_set_id(id_t_ inbound_stat_sample_set_id_){
	inbound_stat_sample_set_id = inbound_stat_sample_set_id_;
}

id_t_ net_proxy_t::get_inbound_stat_sample_set_id(){
	return inbound_stat_sample_set_id;
}
