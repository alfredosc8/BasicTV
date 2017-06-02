#include "tv_frame_standard.h"
#include "tv_frame_audio.h"

tv_frame_audio_t::tv_frame_audio_t() : id(this, TYPE_TV_FRAME_AUDIO_T){
	list_virtual_data(&id);
	audio_prop.list_virtual_data(&id);
	id.add_data_one_byte_vector(&data, ~((uint32_t)0));
}

tv_frame_audio_t::~tv_frame_audio_t(){
}
