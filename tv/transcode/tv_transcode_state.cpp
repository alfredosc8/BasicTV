#include "tv_transcode.h"
#include "tv_transcode_state.h"
#include "tv_transcode_audio.h"

std::vector<tv_transcode_encode_state_t> encode_state_vector;
std::vector<tv_transcode_decode_state_t> decode_state_vector;

// the start id in a tv_transcode_decode/encode_state_t doesn't
// need to have the start flag, remember, those might not exist

// Update instance of frame to be up to, but not including, the inclusion of
// frame_id (not including allows for some optimizations later on).

SEARCH_FOR_STATE(encode);

SEARCH_FOR_STATE(decode);

// TODO: should optimize this for searching for the previous
// For example, possibly pass prev_frame_id to encode_search_for_state,
// since that would be the last_frame_id (saving linked list lookups if
// we check for that first, even with an unlikely macro)

void tv_transcode_state_decode_fast_forward(id_t_ frame_id){
	tv_frame_audio_t *frame_audio_ptr =
		PTR_DATA(frame_id,
			 tv_frame_audio_t);
	if(frame_audio_ptr == nullptr){
		print("frame_audio_ptr is a nullptr", P_ERR);
	}
	std::pair<std::vector<id_t_>, std::vector<id_t_> > linked_list =
		frame_audio_ptr->id.get_linked_list();
	id_t_ prev_frame_id = ID_BLANK_ID;
	if(linked_list.first.size() != 0){
		prev_frame_id = linked_list.first[0];
	}else{
		print("encoding first frame, we don't have a previous state", P_SPAM);
	}
	tv_transcode_state_decode_codec_t decode_codec =
		decode_codec_lookup(
			frame_audio_ptr->get_audio_prop().get_format());
	tv_transcode_decode_state_t *decode_state_ptr =
		decode_search_for_state(frame_id);
	if(decode_state_ptr == nullptr){
		tv_audio_prop_t tmp_audio_prop =
			frame_audio_ptr->get_audio_prop();
		decode_state_ptr = 
			decode_codec.decode_init_state(
				&tmp_audio_prop);
		// decoder should interpret decoding output without modification
	}else if(decode_state_ptr->get_last_frame_id() == prev_frame_id){
		print("already given an optimal state, sweet", P_NOTE);
	}else{
		const std::vector<id_t_> frame_buffer =
			id_api::linked_list::list::by_distance_until_match(
				frame_id,
				32, // should really be a setting
				decode_state_ptr->get_last_frame_id());
		if(frame_buffer.size() == 0){
			print("yeah, get a standardized search size setting pretty soon", P_CRIT);
			// stop being lazy...
		}else{
			uint32_t sampling_freq = 0;
			uint8_t bit_depth = 0;
			uint8_t channel_count = 0;
			std::vector<std::vector<uint8_t> > packet_set;
			for(uint64_t i = 0;i < frame_buffer.size();i++){
				tv_frame_audio_t *tmp_frame_audio_ptr =
					PTR_DATA(frame_buffer[i],
						 tv_frame_audio_t);
				CONTINUE_IF_NULL(tmp_frame_audio_ptr, P_WARN);
				// TODO: instead of continuing, allow passing
				// a blank vector into the encoding function,
				// and let the codec interpret that as a lost
				// packet so Opus can flex it's inband FEC
				// muscles
				std::vector<std::vector<uint8_t> > tmp_snippet_vector =
					tmp_frame_audio_ptr->get_packet_set();
				packet_set.insert(
					packet_set.end(),
					tmp_snippet_vector.begin(),
					tmp_snippet_vector.end());
				decode_codec.decode_snippets_to_samples(
					decode_state_ptr,
					&packet_set,
					&sampling_freq,
					&bit_depth,
					&channel_count);
			}
		}
	}
}
