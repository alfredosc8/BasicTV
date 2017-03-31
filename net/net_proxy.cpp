#include "net_proxy.h"
#include "../stats/stats.h"

net_proxy_t::net_proxy_t() : id(this, __FUNCTION__){
}

net_proxy_t::~net_proxy_t(){
}

void net_proxy_t::set_flags(uint8_t flags_){
	flags = flags_;
}

uint8_t net_proxy_t::get_flags(){
	return flags;
}
