#include "tv_transcode.h"
#include "tv_transcode_state.h"
#include "tv_transcode_audio.h"

std::vector<tv_transcode_encode_state_t> encode_state_vector;
std::vector<tv_transcode_decode_state_t> decode_state_vector;

// the start id in a tv_transcode_decode/encode_state_t doesn't
// need to have the start flag, remember, those might not exist

SEARCH_FOR_STATE(encode);

SEARCH_FOR_STATE(decode);

// Update instance of frame to be up to, but not including, the inclusion of
// frame_id (not including allows for some optimizations later on).

void tv_transcode_state_encode_fast_forward(id_t_ frame_id){
	tv_frame_audio_t *frame_audio_ptr =
		PTR_DATA(frame_id,
			 tv_frame_audio_t);
	if(frame_audio_ptr == nullptr){
		print("frame_audio_ptr is a nullptr", P_ERR);
	}
	tv_transcode_state_encode_codec_t encode_codec =
		encode_codec_lookup(
			frame_audio_ptr->get_audio_prop().get_format());
	tv_transcode_encode_state_t *encode_state_ptr =
		encode_search_for_state(
			frame_audio_ptr->get_audio_prop().get_format(),
			frame_audio_ptr->get_codec_state_ref());
	if(encode_state_ptr == nullptr){
		tv_audio_prop_t tmp_audio_prop =
			frame_audio_ptr->get_audio_prop();
		encode_state_ptr = 
			encode_codec.encode_init_state(
				&tmp_audio_prop);
		// encoder should interpret decoding output without modification
	}else if(encode_state_ptr->get_start_frame_id() == frame_id && encode_state_ptr->get_frame_depth() == 0){
		print("already given an optimal state, sweet", P_NOTE);
	}else{
		const int64_t frame_state_diff =
			id_api::linked_list::pos_in_linked_list(
				encode_state_ptr->get_start_frame_id(),
				frame_id,
				32); // 32 is completely arbitrary FYI
		if(frame_state_diff < 0){
			print("frame_id is supposedly before start id, not right", P_ERR);
		}else if(frame_state_diff == 0){
			print("something is seriously wrong here", P_CRIT);
		}else if(frame_state_diff > 0){
			if((uint64_t)frame_state_diff == encode_state_ptr->get_frame_depth()){
				print("after lookup, we are in an optimal state, sweet", P_NOTE);
			}else if((uint64_t)frame_state_diff > encode_state_ptr->get_frame_depth()){
				
			}else{
				
			}
		}
	}
}
