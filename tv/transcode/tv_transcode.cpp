#include "tv_transcode.h"

id_t_ tv_transcode_encode_state_t::get_start_frame_id(){
	if(start_frame_id == ID_BLANK_ID){
		print("invalid start_frame_id", P_ERR);
	}
	return start_frame_id;
}

void tv_transcode_encode_state_t::set_start_frame_id(id_t_ id){
	start_frame_id = id;
	tv_frame_audio_t *audio_frame_ptr =
		PTR_DATA(id,
			 tv_frame_audio_t);
	if(audio_frame_ptr == nullptr){
		print("audio_frame_ptr is a nullptr, can't add derived data", P_ERR);
	}
	state_format = audio_frame_ptr->get_audio_prop().get_format();
}

id_t_ tv_transcode_decode_state_t::get_start_frame_id(){
	if(start_frame_id == ID_BLANK_ID){
		print("invalid start_frame_id", P_ERR);
	}
	return start_frame_id;
}

void tv_transcode_decode_state_t::set_start_frame_id(id_t_ id){
	start_frame_id = id;
	tv_frame_audio_t *audio_frame_ptr =
		PTR_DATA(id,
			 tv_frame_audio_t);
	if(audio_frame_ptr == nullptr){
		print("audio_frame_ptr is a nullptr, can't add derived data", P_ERR);
	}
	state_format = audio_frame_ptr->get_audio_prop().get_format();
}
