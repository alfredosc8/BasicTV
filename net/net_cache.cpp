#include "net_socket.h"
#include "net.h"
#include "net_cache.h"

net_cache_t::net_cache_t(std::string url_, std::string data_, uint64_t timestamp_){
	url = url_;
	data = data_;
	timestamp = timestamp_;
	complete = false;
}

net_cache_t::~net_cache_t(){
	url = "";
	data = "";
	timestamp = 0;
	complete = false;
}

std::string net_cache_t::get_url(){
	return url;
}

void net_cache_t::set_data(std::string data_){
	data += data_;
}

std::string net_cache_t::get_data(){
	return data;
}

uint64_t net_cache_t::get_timestamp(){
	return timestamp;
}

void net_cache_t::set_complete(bool complete_){
	complete = complete_;
}

bool net_cache_t::get_complete(){
	return complete;
}
