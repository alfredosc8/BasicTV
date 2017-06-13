#include "tv_transcode.h"

#include "../tv_frame_audio.h"
#include "../tv_frame_video.h"

#include "tv_transcode_opus.h"
#include "tv_transcode_wav.h"

#include "tv_transcode_state.h"
#include "tv_transcode_audio.h"

// caller needs to recognize what can be written to and check/act accordingly

#define CODEC_ENCODE_COUNT 2
#define CODEC_DECODE_COUNT 2

tv_transcode_state_encode_codec_t encode_vector[CODEC_ENCODE_COUNT] =
{
	tv_transcode_state_encode_codec_t(TV_AUDIO_FORMAT_OPUS,
					  opus_encode_init_state,
					  opus_encode_sample_vector_to_snippet_vector,
					  opus_encode_close_state),
	tv_transcode_state_encode_codec_t(TV_AUDIO_FORMAT_WAV,
					  wave_encode_init_state,
					  wave_encode_sample_vector_to_snippet_vector,
					  wave_encode_close_state),
};

tv_transcode_state_decode_codec_t decode_vector[CODEC_DECODE_COUNT] =
{
	tv_transcode_state_decode_codec_t(TV_AUDIO_FORMAT_OPUS,
					  opus_decode_init_state,
					  opus_decode_snippet_vector_to_sample_vector,
					  opus_decode_close_state),
	tv_transcode_state_decode_codec_t(TV_AUDIO_FORMAT_OPUS,
					  wave_decode_init_state,
					  wave_decode_snippet_vector_to_sample_vector,
					  wave_decode_close_state),
};

// Addition to, and deletion from, the state vector is handled inside of
// the codec implementation itself, because a non-state codec can take
// a state interface (like WAV) and use previous frame data for some magic

std::vector<tv_transcode_encode_state_t> encode_state_vector;
std::vector<tv_transcode_decode_state_t> decode_state_vector;

tv_transcode_state_encode_codec_t encode_codec_lookup(uint8_t format){
	for(uint64_t i = 0;i < CODEC_ENCODE_COUNT;i++){
		if(encode_vector[i].get_format() == format){
			return encode_vector[i];
		}
	}
	P_V(format, P_WARN);
	print("invalid encode codec", P_ERR);
	return tv_transcode_state_encode_codec_t(
		0, nullptr, nullptr, nullptr);
}

tv_transcode_state_decode_codec_t decode_codec_lookup(uint8_t format){
	for(uint64_t i = 0;i < CODEC_DECODE_COUNT;i++){
		if(decode_vector[i].get_format() == format){
			return decode_vector[i];
		}
	}
	P_V(format, P_WARN);
	print("invalid decode codec", P_ERR);
	return tv_transcode_state_decode_codec_t(
		0, nullptr, nullptr, nullptr);
}

void audio_prop_sanity_check(tv_audio_prop_t audio_prop){
	if(audio_prop.get_format() == 0){
		std::raise(SIGINT);
		print("invalid format", P_ERR);
	}
}

static tv_audio_prop_t pull_audio_prop_from_id(id_t_ id){
	tv_frame_audio_t *frame_audio_ptr =
		PTR_DATA(id, tv_frame_audio_t);
	PRINT_IF_NULL(frame_audio_ptr, P_ERR);
	audio_prop_sanity_check(frame_audio_ptr->get_audio_prop());
	return frame_audio_ptr->get_audio_prop();
}

std::vector<id_t_> transcode::audio::frames::to_frames(std::vector<id_t_> frame_set,
						       tv_audio_prop_t *output_audio_prop,
						       uint64_t frame_duration_micro_s){
	PRINT_IF_NULL(output_audio_prop, P_ERR);
	audio_prop_sanity_check(*output_audio_prop);
	return transcode::audio::codec::to_frames(
		transcode::audio::frames::to_codec(
			frame_set,
			output_audio_prop),
		output_audio_prop,
		output_audio_prop,
		frame_duration_micro_s);
}

std::vector<std::vector<uint8_t> > transcode::audio::frames::to_codec(std::vector<id_t_> frame_set,
								     tv_audio_prop_t *output_audio_prop){
	PRINT_IF_NULL(output_audio_prop, P_ERR);
	PRINT_IF_EMPTY(frame_set, P_ERR);
	audio_prop_sanity_check(*output_audio_prop);
        tv_audio_prop_t input_audio_prop =
		pull_audio_prop_from_id(
			frame_set[0]);
	std::vector<std::vector<uint8_t> > retval;
	tv_transcode_state_decode_codec_t decode_codec =
		decode_codec_lookup(
			input_audio_prop.get_format());
	tv_transcode_state_encode_codec_t encode_codec =
		encode_codec_lookup(
			output_audio_prop->get_format());
	tv_transcode_decode_state_t *decode_state =
		decode_codec.decode_init_state(
			&input_audio_prop);
	tv_transcode_encode_state_t *encode_state =
		encode_codec.encode_init_state(
			output_audio_prop);
	for(uint64_t i = 0;i < frame_set.size();i++){
		tv_frame_audio_t *frame_audio_ptr =
			PTR_DATA(frame_set[i],
				 tv_frame_audio_t);
		CONTINUE_IF_NULL(frame_audio_ptr, P_WARN);
		audio_prop_sanity_check(
			frame_audio_ptr->get_audio_prop());
		uint32_t sampling_freq =
			frame_audio_ptr->get_audio_prop().get_sampling_freq();
		uint8_t bit_depth =
			frame_audio_ptr->get_audio_prop().get_bit_depth();
		uint8_t channel_count =
			frame_audio_ptr->get_audio_prop().get_channel_count();
		P_V(sampling_freq, P_VAR);
		P_V(bit_depth, P_VAR);
		P_V(channel_count, P_VAR);
		std::vector<std::vector<uint8_t> > tmp =
			encode_codec.encode_sample_vector_to_snippet_vector(
				encode_state,
				decode_codec.decode_snippet_vector_to_sample_vector(
					decode_state,
					frame_audio_ptr->get_packet_set(),
					&sampling_freq,
					&bit_depth,
					&channel_count),
				sampling_freq,
				bit_depth,
				channel_count);
		retval.insert(
			retval.end(),
			tmp.begin(),
			tmp.end());
		output_audio_prop->set_sampling_freq(
			sampling_freq);
		output_audio_prop->set_bit_depth(
			bit_depth);
		output_audio_prop->set_channel_count(
			channel_count);
	}
	encode_codec.encode_close_state(encode_state);
	encode_state = nullptr;
	decode_codec.decode_close_state(decode_state);
	decode_state = nullptr;
	return retval;
}

std::vector<id_t_> transcode::audio::codec::to_frames(std::vector<std::vector<uint8_t> > codec_set,
						      tv_audio_prop_t *input_audio_prop,
						      tv_audio_prop_t *output_audio_prop,
						      uint64_t frame_duration_micro_s){
	PRINT_IF_NULL(input_audio_prop, P_ERR);
	PRINT_IF_NULL(output_audio_prop, P_ERR);
	audio_prop_sanity_check(*input_audio_prop);
	audio_prop_sanity_check(*output_audio_prop);
	std::vector<id_t_> retval;
	const bool repackage =
		input_audio_prop->get_format() == output_audio_prop->get_format() &&
		output_audio_prop->get_flags() & TV_AUDIO_PROP_FORMAT_ONLY;	
	if(repackage){
		// really nice honestly
		print("don't care about the output specifics, and the formats match, copying over directly", P_NOTE);
		uint64_t snippets_to_frame = 0;
		if(input_audio_prop->get_snippet_duration_micro_s() != 0){
			snippets_to_frame =
				frame_duration_micro_s/input_audio_prop->get_snippet_duration_micro_s();
		}else{
			print("frame_duration of codec data is zero, setting one per frame", P_WARN);
		}
		if(snippets_to_frame == 0){
			print("can't assign zero snippets to a frame, updating to a minimum of one", P_NOTE);
			snippets_to_frame++;
		}
		while(codec_set.size() > 0){
			tv_frame_audio_t *frame_audio_ptr =
				new tv_frame_audio_t;
			uint64_t snippet_set_end =
				(codec_set.size() > snippets_to_frame) ?
				snippets_to_frame :
				codec_set.size();
			std::vector<std::vector<uint8_t> > codec_snippet_subset(
				codec_set.begin(),
				codec_set.begin()+snippet_set_end);
			codec_set.erase(
				codec_set.begin(),
				codec_set.begin()+snippet_set_end);
			frame_audio_ptr->set_packet_set(
				codec_snippet_subset);
			frame_audio_ptr->set_audio_prop(
				*input_audio_prop); // just repacking it
			frame_audio_ptr->set_ttl_micro_s(
				input_audio_prop->get_snippet_duration_micro_s()*snippets_to_frame);
			retval.push_back(
				frame_audio_ptr->id.get_id());
		}
		*output_audio_prop = *input_audio_prop;
	}else{
		print("can't simply repackage, decoding and encoding to proper frame size", P_NOTE);
		tv_transcode_state_decode_codec_t decode_codec =
			decode_codec_lookup(
				input_audio_prop->get_format());
		tv_transcode_state_encode_codec_t encode_codec =
			encode_codec_lookup(
				input_audio_prop->get_format());
		tv_transcode_decode_state_t *decode_state =
			decode_codec.decode_init_state(
				input_audio_prop);
		tv_transcode_encode_state_t *encode_state =
			encode_codec.encode_init_state(
				output_audio_prop);
	
		std::raise(SIGINT);
		print("work on me later", P_CRIT);

		for(uint64_t i = 0;i < codec_set.size();i++){
		
		}
	
		encode_codec.encode_close_state(encode_state);
		encode_state = nullptr;
		decode_codec.decode_close_state(decode_state);
		decode_state = nullptr;
	}
	P_V(retval.size(), P_NOTE);
	id_api::linked_list::link_vector(retval);
	return retval;
}

std::vector<std::vector<uint8_t> > transcode::audio::codec::to_raw(std::vector<std::vector<uint8_t> > codec_set,
								   tv_audio_prop_t *input_audio_prop,
								   uint32_t *sampling_freq,
								   uint8_t *bit_depth,
								   uint8_t *channel_count){
	PRINT_IF_NULL(sampling_freq, P_ERR);
	PRINT_IF_NULL(bit_depth, P_ERR);
	PRINT_IF_NULL(channel_count, P_ERR);
	PRINT_IF_NULL(input_audio_prop, P_ERR);
	PRINT_IF_EMPTY(codec_set, P_ERR);
	audio_prop_sanity_check(*input_audio_prop);
	tv_transcode_state_decode_codec_t decode_codec =
		decode_codec_lookup(
			input_audio_prop->get_format());
	// IDEALLY some garbage collection would destroy un-needed states
	// but that isn't implemented yet
	tv_transcode_decode_state_t *decode_state =
		decode_codec.decode_init_state(
			input_audio_prop);
	return decode_codec.decode_snippet_vector_to_sample_vector(
			decode_state,
			codec_set,
			sampling_freq,
			bit_depth,
			channel_count);
}

std::vector<std::vector<uint8_t> > transcode::audio::raw::to_codec(std::vector<std::vector<uint8_t> > codec_set,
								   uint32_t sampling_freq,
								   uint8_t bit_depth,
								   uint8_t channel_count,
								   tv_audio_prop_t *output_audio_prop){
	PRINT_IF_NULL(output_audio_prop, P_ERR);
	PRINT_IF_EMPTY(codec_set, P_ERR);
	audio_prop_sanity_check(*output_audio_prop);
	tv_transcode_state_encode_codec_t encode_codec =
		encode_codec_lookup(
			output_audio_prop->get_format());
	tv_transcode_encode_state_t *encode_state =
		encode_codec.encode_init_state(
			output_audio_prop);
	return encode_codec.encode_sample_vector_to_snippet_vector(
			encode_state,
			codec_set,
			sampling_freq,
			bit_depth,
			channel_count);
}
