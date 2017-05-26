#include <SDL2/SDL_net.h>
#include <algorithm>
#include "net_ip.h"
#include "../convert.h"
#include "../util.h"

net_ip_t::net_ip_t(){
}

net_ip_t::~net_ip_t(){
}

void net_ip_t::list_virtual_data(data_id_t *id){
	id->add_data_raw(&(address[0]), 64);
	id->add_data_raw(&port, sizeof(port));
	id->add_data_raw(&type, sizeof(type));
}

void net_ip_t::set_net_ip(std::string ip_, uint16_t port_){
	port = port_;
	if(ip_.size() >= address.size()){
		print("attempted ip length is greater than 63 characters", P_ERR);
	}
	memset(&(address[0]), 0, 64);
	const bool ver_six =
		ip_.find_first_of(":") != std::string::npos;
	const bool ver_four =
		(std::count(ip_.begin(), ip_.end(), '.') == 3) &&
		(ip_.size() > 15);
	if(ver_six){
		print("SDLnet doesn't (currently) support IPv6, use another library", P_ERR);
		type = NET_IP_FMT_IPV6;
		memcpy(&(address[0]), ip_.data(), ip_.size());
	}else if(ver_four){
		type = NET_IP_FMT_IPV4;
		IPaddress ip_addr;
		SDLNet_ResolveHost(&ip_addr, ip_.data(), port_);
		const uint32_t ip_addr_fixed =
			NBO_32(ip_addr.host);
		memcpy(&(address[0]), &ip_addr_fixed, 4);
	}else{
		type = NET_IP_FMT_DOMAIN;
		memcpy(&(address[0]), ip_.data(), ip_.size());
	}
	P_V_S((char*)&(address[0]), P_VAR);
	P_V(port, P_VAR);
}

std::string net_ip_t::get_net_ip_str(){
	std::string retval;
	if(type == NET_IP_FMT_IPV6){
		print("again, can't handle IPv6 yet", P_ERR);
	}else if(type == NET_IP_FMT_IPV4){
		IPaddress ip_addr_tmp;
		ip_addr_tmp.host = NBO_32(*((uint32_t*)&address[0]));
		ip_addr_tmp.port = NBO_16(get_net_port());
		retval = SDLNet_ResolveIP(&ip_addr_tmp);
		if(retval == "0.0.0.0"){
			retval = ""; // legacy
		}else{
			print("registering " + retval + " as a legit IP", P_SPAM);
		}
	}else if(type == NET_IP_FMT_DOMAIN){
		retval = (char*)(&(address[0]));
	}
	return retval;
}

uint16_t net_ip_t::get_net_port(){
	return port;
}
