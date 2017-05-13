#include "tv_item.h"
#include "tv_channel.h"
#include "tv_window.h"
#include "tv_frame_audio.h"
#include "tv_frame_video.h"
#include "tv_frame_caption.h"
#include "tv_frame_numbers.h"

#include "../id/id_set.h"

std::vector<std::vector<id_t_> > tv_item_t::get_frame_id_vector(){
	std::vector<std::vector<id_t_> > retval;
	for(uint64_t i = 0;i < frame_sets.size();i++){
		retval.push_back(
			expand_id_set(frame_sets[i]));
	}
	return retval;
}

void tv_item_t::add_frame_id(std::vector<id_t_> stream_id_vector_){
	// TODO: check for redundancies if that isn't too slow
	// (it probably will be)
	frame_sets.push_back(
		compact_id_set(stream_id_vector_));
}

// it is pretty hard to keep track of what is what, espeically
// with variable lengths, so just clear everything and only re-add what
// we need (I'm pretty sure there isn't a current use for del_frame_id
// in tv_channel_t (the way it was when I wrote this), so I don't think
// it will be missed).

void tv_item_t::clear_frame_sets(){
	frame_sets.clear();
}

tv_item_t::tv_item_t() : id(this, TYPE_TV_ITEM_T){
}

tv_item_t::~tv_item_t(){}
