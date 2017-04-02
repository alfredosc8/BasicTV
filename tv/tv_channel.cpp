#include "../main.h"
#include "../util.h"
#include "tv_channel.h"
#include "tv_frame_standard.h"
#include "tv.h"

tv_channel_t::tv_channel_t() : id(this, TYPE_TV_CHANNEL_T){
	id.add_data(&stream_list, TV_CHAN_FRAME_LIST_SIZE);
	ADD_DATA(status);
}

tv_channel_t::~tv_channel_t(){
}

bool tv_channel_t::is_streaming(){
	return status & TV_CHAN_STREAMING;
}

bool tv_channel_t::is_audio(){
	return status & TV_CHAN_AUDIO;
}

bool tv_channel_t::is_video(){
	return status & TV_CHAN_VIDEO;
}

std::vector<id_t_> tv_channel_t::get_stream_list(){
	return std::vector<id_t_>(
		stream_list.begin(),
		stream_list.end());
}

void tv_channel_t::add_stream_id(id_t_ id_){
	del_stream_id(id_);
	stream_list.push_back(id_);
}

void tv_channel_t::del_stream_id(id_t_ id_){
	for(uint64_t i = 0;i < stream_list.size();i++){
		if(stream_list[i] == id_){
			stream_list.erase(
				stream_list.begin()+i);
			// don't break until I know everything works fine
		}
	}
}
