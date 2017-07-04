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
	P_V_S(ip_, P_VAR);
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
		struct sockaddr_in tmp;
		if(inet_pton(AF_INET, ip_.c_str(), (struct in_addr*)&tmp) < 0){
			print("couldn't convert IPv4 to human readable", P_ERR);
		}
		memcpy(&(address[0]), &tmp.sin_addr.s_addr, 4);
		convert::nbo::to(
			address.begin(),
			4); // should work fine?

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
		struct sockaddr_in addr;
		CLEAR(addr);
		addr.sin_addr.s_addr =
			NBO_32(*(reinterpret_cast<uint32_t*>(&(address[0]))));
		char buf[INET_ADDRSTRLEN];
		retval =
			inet_ntop(
				AF_INET,
				(struct in_addr*)&addr.sin_addr,
				&(buf[0]),
				INET_ADDRSTRLEN);
		retval = buf;
	}else if(type == NET_IP_FMT_DOMAIN){
		retval = (char*)(&(address[0]));
	}
	return retval;
}

uint16_t net_ip_t::get_net_port(){
	return port;
}

std::vector<uint8_t> net_ip_t::get_net_ip_raw(){
	switch(type){
	case NET_IP_FMT_IPV4:
		ASSERT(address.size() == 4, P_ERR);
		return std::vector<uint8_t>(
			address.begin(),
			address.begin()+4);
	case NET_IP_FMT_IPV6:
		ASSERT(address.size() == 16, P_ERR);
		return std::vector<uint8_t>(
			address.begin(),
			address.begin()+16);
	case NET_IP_FMT_DOMAIN:
		ASSERT(std::find(address.begin(), address.end(), 0) != address.end(), P_ERR);
		return std::vector<uint8_t>(
			address.begin(),
			std::find(
				address.begin(),
				address.end(),
				0));
	default:
		print("unknown domain (did you confuse NET_INTERFACE and NET_IP?)", P_ERR);
	}
	return {};
}
