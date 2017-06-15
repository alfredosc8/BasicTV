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

tv_transcode_state_encode_codec_t encodes[CODEC_ENCODE_COUNT] =
{
	tv_transcode_state_encode_codec_t(
		TV_AUDIO_FORMAT_OPUS,
		opus_encode_init_state,
		opus_encode_samples_to_snippets,
		opus_encode_close_state),
	tv_transcode_state_encode_codec_t(
		TV_AUDIO_FORMAT_WAV,
		wave_encode_init_state,
		wave_encode_samples_to_snippets,
		wave_encode_close_state),
};

tv_transcode_state_decode_codec_t decodes[CODEC_DECODE_COUNT] =
{
	tv_transcode_state_decode_codec_t(
		TV_AUDIO_FORMAT_OPUS,
		opus_decode_init_state,
		opus_decode_snippets_to_samples,
		opus_decode_close_state),
	tv_transcode_state_decode_codec_t(
		TV_AUDIO_FORMAT_WAV,
		wave_decode_init_state,
		wave_decode_snippets_to_samples,
		wave_decode_close_state),
};

// Addition to, and deletion from, the state vector is handled inside of
// the codec implementation itself, because a non-state codec can take
// a state interface (like WAV) and use previous frame data for some magic

std::vector<tv_transcode_encode_state_t> encode_states;
std::vector<tv_transcode_decode_state_t> decode_states;

tv_transcode_state_encode_codec_t encode_codec_lookup(uint8_t format){
	for(uint64_t i = 0;i < CODEC_ENCODE_COUNT;i++){
		if(encodes[i].get_format() == format){
			return encodes[i];
		}
	}
	P_V(format, P_WARN);
	print("invalid encode codec", P_ERR);
	return tv_transcode_state_encode_codec_t(
		0, nullptr, nullptr, nullptr);
}

tv_transcode_state_decode_codec_t decode_codec_lookup(uint8_t format){
	for(uint64_t i = 0;i < CODEC_DECODE_COUNT;i++){
		if(decodes[i].get_format() == format){
			return decodes[i];
		}
	}
	P_V(format, P_WARN);
	print("invalid decode codec", P_ERR);
	return tv_transcode_state_decode_codec_t(
		0, nullptr, nullptr, nullptr);
}

void audio_prop_sanity_check(tv_audio_prop_t audio_prop){
	if(audio_prop.get_format() == 0){
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
	std::vector<std::vector<uint8_t> > codec_set =
		transcode::audio::frames::to_codec(
			frame_set,
			output_audio_prop);
	PRINT_IF_EMPTY(codec_set, P_ERR);
	return transcode::audio::codec::to_frames(
		&codec_set,
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
		std::vector<std::vector<uint8_t> > frame_packet_set =
			frame_audio_ptr->get_packet_set();
		std::vector<uint8_t> samples =
			decode_codec.decode_snippets_to_samples(
				decode_state,
				&frame_packet_set,
				&sampling_freq,
				&bit_depth,
				&channel_count);
		PRINT_IF_EMPTY(samples, P_ERR);
		std::vector<std::vector<uint8_t> > tmp =
			encode_codec.encode_samples_to_snippets(
				encode_state,
				&samples,
				sampling_freq,
				bit_depth,
				channel_count);
		PRINT_IF_EMPTY(tmp, P_ERR);
		print("TODO: sanity check outputs for uncomputed samples", P_WARN);
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

std::vector<id_t_> transcode::audio::codec::to_frames(std::vector<std::vector<uint8_t> > *codec_set,
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
		P_V(snippets_to_frame, P_VAR);
		while(codec_set->size() > 0){
			tv_frame_audio_t *frame_audio_ptr =
				new tv_frame_audio_t;
			uint64_t snippet_set_end =
				(codec_set->size() > snippets_to_frame) ?
				snippets_to_frame :
				codec_set->size();
			std::vector<std::vector<uint8_t> > codec_snippet_subset(
				codec_set->begin(),
				codec_set->begin()+snippet_set_end);
			codec_set->erase(
				codec_set->begin(),
				codec_set->begin()+snippet_set_end);
			frame_audio_ptr->set_packet_set(
				codec_snippet_subset);
			frame_audio_ptr->set_audio_prop(
				*input_audio_prop); // just repacking it
			frame_audio_ptr->set_ttl_micro_s(
				input_audio_prop->get_snippet_duration_micro_s()*snippets_to_frame);
			frame_audio_ptr->set_frame_entry(
				retval.size());
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

		for(uint64_t i = 0;i < codec_set->size();i++){
		
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

std::vector<uint8_t> transcode::audio::codec::to_raw(std::vector<std::vector<uint8_t> > *codec_set,
						     tv_audio_prop_t *input_audio_prop,
						     uint32_t *sampling_freq,
						     uint8_t *bit_depth,
						     uint8_t *channel_count){
	PRINT_IF_NULL(sampling_freq, P_ERR);
	PRINT_IF_NULL(bit_depth, P_ERR);
	PRINT_IF_NULL(channel_count, P_ERR);
	PRINT_IF_NULL(input_audio_prop, P_ERR);
	PRINT_IF_EMPTY(*codec_set, P_ERR);
	audio_prop_sanity_check(*input_audio_prop);
	tv_transcode_state_decode_codec_t decode_codec =
		decode_codec_lookup(
			input_audio_prop->get_format());
	// IDEALLY some garbage collection would destroy un-needed states
	// but that isn't implemented yet
	tv_transcode_decode_state_t *decode_state =
		decode_codec.decode_init_state(
			input_audio_prop);
	return decode_codec.decode_snippets_to_samples(
			decode_state,
			codec_set,
			sampling_freq,
			bit_depth,
			channel_count);
}

std::vector<std::vector<uint8_t> > transcode::audio::raw::to_codec(std::vector<uint8_t> *raw_set,
								   uint32_t sampling_freq,
								   uint8_t bit_depth,
								   uint8_t channel_count,
								   tv_audio_prop_t *output_audio_prop){
	PRINT_IF_NULL(output_audio_prop, P_ERR);
	PRINT_IF_EMPTY(*raw_set, P_ERR);
	audio_prop_sanity_check(*output_audio_prop);
	tv_transcode_state_encode_codec_t encode_codec =
		encode_codec_lookup(
			output_audio_prop->get_format());
	tv_transcode_encode_state_t *encode_state =
		encode_codec.encode_init_state(
			output_audio_prop);
	return encode_codec.encode_samples_to_snippets(
			encode_state,
			raw_set,
			sampling_freq,
			bit_depth,
			channel_count);
}

std::vector<uint8_t> transcode::audio::frames::to_raw(
	std::vector<id_t_> frame_set,
	uint32_t *sampling_freq,
	uint8_t *bit_depth,
	uint8_t *channel_count){
	tv_audio_prop_t output_audio_prop;
	output_audio_prop.set_flags(
		TV_AUDIO_PROP_FORMAT_ONLY);
	output_audio_prop.set_format(
		TV_AUDIO_FORMAT_WAV); // anything lossless would work fine
	std::vector<std::vector<uint8_t> > codec_set =
		transcode::audio::frames::to_codec(
			frame_set,
			&output_audio_prop);
	PRINT_IF_EMPTY(codec_set, P_ERR);
	std::vector<uint8_t> raw_set =
		transcode::audio::codec::to_raw(
			&codec_set,
			&output_audio_prop,
			sampling_freq,
			bit_depth,
			channel_count);
	PRINT_IF_EMPTY(raw_set, P_ERR);
	P_V(codec_set.size(), P_VAR);
	P_V(raw_set.size(), P_VAR);
	P_V(*sampling_freq, P_VAR);
	P_V(*bit_depth, P_VAR);
	P_V(*channel_count, P_VAR);
	return raw_set;
			
}

/*
  Doesn't handle endian/signed conversions
 */

std::vector<std::vector<uint8_t> > samples_to_chunks_of_length(
	std::vector<uint8_t> *samples,
	uint32_t chunk_size){
	std::vector<std::vector<uint8_t> > retval;
	const uint64_t whole_chunks =
		samples->size()/chunk_size;
	for(uint64_t i = 0;i < whole_chunks;i++){
		retval.push_back(
			std::vector<uint8_t>(samples->begin()+(i*chunk_size),
					     samples->begin()+((i+1)*chunk_size)));
	}
	samples->erase(
		samples->begin(),
		samples->begin()+(chunk_size*whole_chunks));
	return retval;
}

void assert_sane_audio_metadata(
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count){
	if(bit_depth % 8){
		/*
		  Well, this can TECHNICALLY happen with insane codecs like
		  Codec2, but I would hope that they would have an at least
		  8-bit (preferably 16-bit) interface for sanity
		 */
		print("bit depth is not an even divisor of eight, this should never happen", P_ERR);
	}
	if(channel_count != 1){
		print("only mono is currently supported", P_ERR);
	}
	if(sampling_freq != 48000){
		print("only 48khz is current supported", P_ERR);
	}
}

/*
  duration_metadata_to_chunk_size function doesn't care about how
  multiple channels are encoded, as long as they are both in the
  same chunk (i.e. LRLR and LLRR, not two seperate tracks, which
  should be corrected by the caller)
 */

uint32_t duration_metadata_to_chunk_size(
	uint32_t duration_micro_s,
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count){
	const long double duration_s_ld =
		(long double)(duration_micro_s)/(long double)(1000000);
	const uint8_t byte_depth =
		bit_depth/8;
	ASSERT(byte_depth == 2, P_ERR);
	return (duration_s_ld*sampling_freq*byte_depth*channel_count);
}

#define CHECK_META_CODEC(x) if(x != state_encode_audio_prop.get_##x()){print((std::string)#x + " is not compatiable with the given audio prop", P_ERR);}

void assert_compatiable_audio_metadata(
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count,
	tv_audio_prop_t state_encode_audio_prop){
	CHECK_META_CODEC(sampling_freq);
	CHECK_META_CODEC(bit_depth);
	CHECK_META_CODEC(channel_count);
}

#undef CHECK_META_CODEC
