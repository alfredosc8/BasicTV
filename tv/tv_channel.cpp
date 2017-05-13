#include "../main.h"
#include "../util.h"
#include "tv_channel.h"
#include "tv_frame_standard.h"
#include "tv.h"

tv_channel_t::tv_channel_t() : id(this, TYPE_TV_CHANNEL_T){
}

tv_channel_t::~tv_channel_t(){
}

void tv_channel_t::set_desc(std::string desc){
	description.clear();
	description = std::vector<uint8_t>(0, desc.size());
	memcpy(description.data(),
	       desc.c_str(),
	       desc.size());
}

std::string tv_channel_t::get_desc(){
	return (char*)(&description[0]);
}
