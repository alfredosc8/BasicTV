#include "tv_transcode.h"

/*
  state_format is a sanity check to prevent audio/video crossover (pointer
  danger).
*/

std::vector<tv_transcode_encode_state_t> transcode_encode_state_vector;
std::vector<tv_transcode_decode_state_t> transcode_decode_state_vector;

id_t_ tv_transcode_encode_state_t::get_start_frame_id(){
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
	codec_state_ref = audio_frame_ptr->get_codec_state_ref();
}

id_t_ tv_transcode_decode_state_t::get_start_frame_id(){
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
	codec_state_ref = audio_frame_ptr->get_codec_state_ref();
}
