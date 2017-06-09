#include "../main.h"
#include "../util.h"
#include "tv.h"
#include "tv_frame_standard.h"
#include "tv_frame_video.h"
#include "tv_frame_audio.h"
#include "tv_window.h"
#include "tv_channel.h"
#include "tv_item.h"

tv_window_t::tv_window_t() : id(this, TYPE_TV_WINDOW_T){
	id.add_data_raw(&pos, sizeof(pos));
	id.add_data_id(&item_id, 1);
	id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_NEVER,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_RULE_UNDEF);
}

tv_window_t::~tv_window_t(){
}

id_t_ tv_window_t::get_item_id(){
	return item_id;
}

void tv_window_t::set_item_id(id_t_ item_id_){
	item_id = item_id_;
}

void tv_window_t::set_pos(uint8_t pos_){
	pos = pos_;
}

uint8_t tv_window_t::get_pos(){
	return pos;
}

void tv_window_t::set_timestamp_offset(int64_t timestamp_offset_){
	timestamp_offset = timestamp_offset_;
}

void tv_window_t::add_active_stream_id(id_t_ id_){
	del_active_stream_id(id_);
	active_streams.push_back(id_);
}

void tv_window_t::del_active_stream_id(id_t_ id_){
	for(uint64_t i = 0;i < active_streams.size();i++){
		if(active_streams[i] == id_){
			active_streams.erase(
				active_streams.begin()+i);
			i--;
		}
	}
}
