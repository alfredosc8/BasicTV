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

