#include "tv_transcode.h"

#include "../tv_frame_audio.h"
#include "../tv_frame_video.h"

#include "tv_transcode_opus.h"
#include "tv_transcode_wav.h"

// caller needs to recognize what can be written to and check/act accordingly

#define CHECK_FORMAT_AND_ENCODE_INIT_STATE(prop, fmt, codec, encoder_prop, code_state) if(prop.get_format() == fmt){code_state = codec##_encode_init_state(encoder_prop);}
#define CHECK_FORMAT_AND_ENCODE_SAMPLES(prop, fmt, codec, state, coded_data, sample_set, sampling_freq, bit_depth, channel_count) if(prop.get_format() == fmt){coded_data = codec##_encode_samples(state, sample_set, sampling_freq, bit_depth, channel_count);}
#define CHECK_FORMAT_AND_ENCODE_CLOSE_STATE(prop, fmt, codec, code_state) if(prop.get_format() == fmt){codec##_encode_close_state(code_state);}

#define CHECK_FORMAT_AND_DECODE_INIT_STATE(prop, fmt, codec, decoder_prop, code_state) if(prop.get_format() == fmt){code_state = codec##_decode_init_state(decoder_prop);}
#define CHECK_FORMAT_AND_DECODE_PACKET(prop, fmt, codec, state, packet, sample_set, sampling_freq, bit_depth, channel_count) if(prop.get_format() == fmt){sample_set = codec##_decode_packet(state, packet, sampling_freq, bit_depth, channel_count);}
#define CHECK_FORMAT_AND_DECODE_CLOSE_STATE(prop, fmt, codec, code_state) if(prop.get_format() == fmt){codec##_decode_close_state(code_state);}

#define CHECK_ALL_FORMAT_AND_ENCODE_INIT_STATE(a, d, e)			\
	CHECK_FORMAT_AND_ENCODE_INIT_STATE(a, TV_AUDIO_FORMAT_OPUS, opus, d, e)	\
//	CHECK_FORMAT_AND_ENCODE_INIT_STATE(a, TV_AUDIO_FORMAT_WAV, wav, d, e) 

#define CHECK_ALL_FORMAT_AND_ENCODE_SAMPLES(a, d, e, f, g, h, i)		\
	CHECK_FORMAT_AND_ENCODE_SAMPLES(a, TV_AUDIO_FORMAT_OPUS, opus, d, e, f, g, h, i) \
//	CHECK_FORMAT_AND_ENCODE_SAMPLES(a, TV_AUDIO_FORMAT_WAV, wav, d, e, f, g, h, i) 

#define CHECK_ALL_FORMAT_AND_ENCODE_CLOSE_STATE(a, d)		\
	CHECK_FORMAT_AND_ENCODE_CLOSE_STATE(a, TV_AUDIO_FORMAT_OPUS, opus, d) \
//	CHECK_FORMAT_AND_ENCODE_CLOSE_STATE(a, TV_AUDIO_FORMAT_WAV, wav, d) 

#define CHECK_ALL_FORMAT_AND_DECODE_INIT_STATE(a, d, e)			\
	CHECK_FORMAT_AND_DECODE_INIT_STATE(a, TV_AUDIO_FORMAT_OPUS, opus, d, e)	\
//	CHECK_FORMAT_AND_DECODE_INIT_STATE(a, TV_AUDIO_FORMAT_WAV, wav, d, e) 

#define CHECK_ALL_FORMAT_AND_DECODE_PACKET(a, d, e, f, g, h, i)		\
	CHECK_FORMAT_AND_DECODE_PACKET(a, TV_AUDIO_FORMAT_OPUS, opus, d, e, f, g, h, i) \
//	CHECK_FORMAT_AND_DECODE_PACKET(a, TV_AUDIO_FORMAT_WAV, wav, d, e, f, g, h, i) 

#define CHECK_ALL_FORMAT_AND_DECODE_CLOSE_STATE(a, d)		\
	CHECK_FORMAT_AND_DECODE_CLOSE_STATE(a, TV_AUDIO_FORMAT_OPUS, opus, d) \
//	CHECK_FORMAT_AND_DECODE_CLOSE_STATEa, TV_AUDIO_FORMAT_WAV, wav, d) 


/*
  Since i'm only doing audio right now, I can make this inline and adjust it
  as I go on (assuming making it generic enough for video would work as well).
 */

static std::vector<std::vector<tv_frame_audio_t*> > transcode_create_sets(std::vector<id_t_> frame_set){
	uint64_t i = 0;
	tv_frame_audio_t *bootstrap_ptr =
		nullptr;
	while(bootstrap_ptr == nullptr && i < frame_set.size()){
		bootstrap_ptr =
			PTR_DATA(frame_set[i],
				 tv_frame_audio_t);
	}
	if(i == frame_set.size()){
		print("no valid frames to decode any data from", P_WARN);
		return std::vector<std::vector<tv_frame_audio_t*> >();
	}
	std::vector<std::vector<tv_frame_audio_t* > > all_id_sets_ptr =
		std::vector<std::vector<tv_frame_audio_t*> >({std::vector<tv_frame_audio_t *>({PTR_DATA(frame_set[0], tv_frame_audio_t)})});
	// Get largest contiguous sets of an encoding scheme
	for(;i < frame_set.size();i++){
		tv_frame_audio_t *frame_audio =
			PTR_DATA(frame_set[i],
				 tv_frame_audio_t);
		if(frame_audio == nullptr){
			print("frame_audio is a nullptr", P_WARN);
		}
		tv_audio_prop_t frame_audio_prop =
			frame_audio->get_audio_prop();
		tv_frame_audio_t *frame_set_ptr =
			all_id_sets_ptr[all_id_sets_ptr.size()-1][all_id_sets_ptr[all_id_sets_ptr.size()-1].size()-1];
		const bool correct_format_in_set =
			frame_set_ptr->get_audio_prop().get_format() == frame_audio->get_audio_prop().get_format();
		const bool correct_codec_state_ref =
			frame_set_ptr->get_codec_state_ref() == frame_audio->get_codec_state_ref();
		if(correct_format_in_set && correct_codec_state_ref){
			// No need to formally read in END_PACKET flags yet
			all_id_sets_ptr[all_id_sets_ptr.size()-1].push_back(
				PTR_DATA(frame_set[i],
					 tv_frame_audio_t));
		}else{
			const bool designated_start_packet =
				frame_audio->get_flags() & TV_FRAME_STANDARD_START_PACKET;
			if(designated_start_packet){
				// should be the majority of cases if done right
				print("peaceful and natural transition between encoding states", P_SPAM);
			}else{
				print("abrupt change between encoding states, it'll work, but it won't be pretty", P_SPAM);
			}
			all_id_sets_ptr.push_back(
				std::vector<tv_frame_audio_t*>({
					PTR_DATA(frame_set[i],
						 tv_frame_audio_t)}));
		}
	}
	return all_id_sets_ptr;
}

/*
  handles state decoding lookups and can hot-create a decoding state if
  the vector's start and end frames have START_PACKET and END_PACKET

  all tv_frame_audio_t's have the same codec_state_ref, which means they
  all have the same sampling frequency, bit depth, and channel count 
  (enough information for true raw samples).
 */

#pragma message("non-self-contained encoded audio blocks aren't supported whatsoever, needs to be tested and more code needs to be written")

static void transcode_raw_samples_sanity_check(
	uint32_t master_sampling_freq,
	uint32_t sampling_freq,
	uint8_t master_bit_depth,
	uint8_t bit_depth,
	uint8_t master_channel_count,
	uint8_t channel_count){
	if(master_sampling_freq != sampling_freq){
		P_V(master_sampling_freq, P_WARN);
		P_V(sampling_freq, P_WARN);
		print("mismatch between new and old sampling freq", P_ERR);
	}
	if(master_bit_depth != bit_depth){
		P_V(master_bit_depth, P_WARN);
		P_V(bit_depth, P_WARN);
		print("mismatch between new and old bit depth", P_ERR);
	}
	if(master_channel_count != channel_count){
		P_V(master_channel_count, P_WARN);
		P_V(channel_count, P_WARN);
		print("mismatch between new and old channel count", P_ERR);
	}
}

std::vector<std::vector<uint8_t> > transcode_create_samples(std::vector<tv_frame_audio_t *> audio_data,
					      uint32_t *sampling_freq,
					      uint8_t *bit_depth,
					      uint8_t *channel_count){
	std::vector<std::vector<uint8_t> > retval;
	*sampling_freq =
		audio_data[0]->get_audio_prop().get_sampling_freq();
	*bit_depth =
		audio_data[0]->get_audio_prop().get_bit_depth();
	*channel_count =
		audio_data[0]->get_audio_prop().get_channel_count();
	const bool self_contained =
		audio_data[0]->get_audio_prop().get_flags() & TV_FRAME_STANDARD_START_PACKET &&
		audio_data[audio_data.size()-1]->get_audio_prop().get_flags() & TV_FRAME_STANDARD_END_PACKET;
	if(self_contained){
		print("data to decode is self-contained, creating temporary tv_transcode_decode_state_t", P_SPAM);
		try{
			tv_transcode_decode_state_t decode_state;
			CHECK_FORMAT_AND_DECODE_INIT_STATE(
				audio_data[0]->get_audio_prop(), // checks against format
				TV_AUDIO_FORMAT_OPUS,
				opus,
				audio_data[0]->get_audio_prop(), // prop for Opus compression
				decode_state);
			for(uint64_t i = 0;i < audio_data.size();i++){
				std::vector<std::vector<uint8_t> > tmp_sample_set;
				std::vector<std::vector<uint8_t> > frame_packet_set =
					audio_data[i]->get_packet_set();
				uint32_t tmp_sampling_freq = 0;
				uint8_t tmp_bit_depth = 0;
				uint8_t tmp_channel_count = 0;
				CHECK_FORMAT_AND_DECODE_PACKET(
					audio_data[i]->get_audio_prop(),
					TV_AUDIO_FORMAT_OPUS,
					opus,
					&decode_state,
					frame_packet_set, // input
					tmp_sample_set, // output
					&tmp_sampling_freq,
					&tmp_bit_depth,
					&tmp_channel_count);
				try{
					transcode_raw_samples_sanity_check(
						*sampling_freq,
						tmp_sampling_freq,
						*bit_depth,
						tmp_bit_depth,
						*channel_count,
						tmp_channel_count);
				}catch(...){
					print("TODO: fill in sample set with an appropriate blank space until a converter is set up", P_NOTE);
				}
				retval.insert(
					retval.begin(),
					tmp_sample_set.begin(),
					tmp_sample_set.end());
			}
			CHECK_FORMAT_AND_DECODE_CLOSE_STATE(
				audio_data[0]->get_audio_prop(),
				TV_AUDIO_FORMAT_OPUS,
				opus,
				&decode_state);
		}catch(...){
			print("couldn't decode self-contained data", P_ERR);
		}
	}else{
		print("there's a lot of work to do before I dive into this (statefulness across multiple calls)", P_CRIT);
	}
	return retval;
}

std::vector<id_t_> transcode::audio::frames::to_frames(std::vector<id_t_> frame_set,
						       tv_audio_prop_t *output_audio_prop){
	std::vector<id_t_> retval;
	if(frame_set.size() == 0){
		print("can't transcode an empty frame vector", P_WARN);
		return retval;
	}
	if(output_audio_prop == nullptr){
		print("no specified destination for audio conversion", P_ERR);
	}
	std::vector<std::vector<tv_frame_audio_t *> > frame_states =
		transcode_create_sets(frame_set);
	/*
	  We have a few options here:
	  - Entire chunks of encoded data coming in, since they it is a self
	  contained unit, doesn't need to register a tv_transcode_decode_t
	  state
	  - If we have a decoding state already, we can pull the start ID,
	  fast forward to the current state (frame_depth), and see what new
	  information we can add and then pull by skipping X number of Y
	  ms (X being packet count and Y is the packet size).
	 */
	return retval;
}

std::vector<std::vector<uint8_t> > transcode::audio::frames::to_codec(std::vector<id_t_> frame_set,
								      tv_audio_prop_t *output_audio_prop){
	/*
	  create_sets organizes everything in a 2D array, each array being
	  as much data as we have on one encoder state at one time
	  (majority of the time it would be straight across, but also
	  operates as a sanity check).

	  create_samples actually calls the codec and creates one contiguous
	  sample set. It makes sense for it to return everything at one time,
	  since (most of) the calls aren't the entire chunk, and it has
	  the states of the encoder and decoders saved into vectors
	  defined in tv_transcode.cpp automatically (support is far away for
	  that though...)
	 */
	if(output_audio_prop == nullptr){
		// should never be a valid use case
		print("we have no place to write raw sample metadata", P_ERR);
	}
	uint32_t sampling_freq = 0;
	uint8_t bit_depth = 0;
	uint8_t channel_count = 0;
	std::vector<std::vector<tv_frame_audio_t*> > frame_audio_full =
		transcode_create_sets(
			frame_set);
	if(frame_audio_full.size() > 1){
		P_V(frame_audio_full.size(), P_VAR);
		print("this is a hack, but the frame_audio_full vector should be one or zero", P_ERR);
	}
	std::vector<std::vector<uint8_t> > retval =
		transcode_create_samples(
			frame_audio_full[0],
			&sampling_freq,
			&bit_depth,
			&channel_count);
	return retval;
}

/*
  This function handles linking the vector together, since there is no use case
  where multiple consecutive IDs aren't linked
 */

std::vector<id_t_> transcode::audio::codec::to_frames(std::vector<std::vector<uint8_t> > codec_set,
						      tv_audio_prop_t *input_audio_prop,
						      tv_audio_prop_t *output_audio_prop){
	std::vector<id_t_> retval;
	if(input_audio_prop == nullptr){
		print("input_audio_prop is a nullptr", P_ERR);
	}
	if(output_audio_prop == nullptr){
		print("output_audio_prop is a nullptr", P_ERR);
	}
	tv_transcode_decode_state_t decode_state;
	CHECK_ALL_FORMAT_AND_DECODE_INIT_STATE(
		(*input_audio_prop),
		(*input_audio_prop),
		decode_state);
	std::vector<uint8_t> raw_samples;
	uint32_t sampling_freq = 0;
	uint8_t bit_depth = 0;
	uint8_t channel_count = 0; 
	std::vector<std::vector<uint8_t> > raw_sample_dv; // double vectors
	CHECK_ALL_FORMAT_AND_DECODE_PACKET(
		(*input_audio_prop),
		&decode_state,
		codec_set,
		raw_sample_dv,
		&sampling_freq,
		&bit_depth,
		&channel_count);
	PRINT_IF_EMPTY(raw_sample_dv, P_ERR);
	for(uint64_t i = 0;i < raw_sample_dv.size();i++){
		raw_samples.insert(
			raw_samples.end(),
			raw_sample_dv[i].begin(),
			raw_sample_dv[i].end());
	}
	CHECK_ALL_FORMAT_AND_DECODE_CLOSE_STATE(
		(*input_audio_prop),
		&decode_state);
	PRINT_IF_EMPTY(raw_samples, P_ERR);

	std::vector<std::vector<uint8_t> > packetized_encode;
	tv_transcode_encode_state_t encode_state;
	CHECK_ALL_FORMAT_AND_ENCODE_INIT_STATE(
		(*output_audio_prop),
		(*output_audio_prop),
		encode_state);
	CHECK_ALL_FORMAT_AND_ENCODE_SAMPLES(
		(*output_audio_prop),
		&encode_state,
		packetized_encode,
		raw_samples,
		sampling_freq,
		bit_depth,
		channel_count);
	PRINT_IF_EMPTY(packetized_encode, P_ERR);
	for(uint64_t i = 0;i < packetized_encode.size();i++){
		if(packetized_encode[i].size() == 0){
			print("invalid packetized_encode size", P_ERR);
		}
		tv_frame_audio_t *audio_frame =
			new tv_frame_audio_t;
		audio_frame->set_audio_prop(
			*output_audio_prop);
		/*
		  TODO: do some size sanity checks and see if it makes sense
		  to put each audio packet into its own tv_frame_audio_t.
		  For example, we don't want to send so few out to the network
		  that the broadcast delay is shorter than the length of the
		  audio frame (little to no permeation time), but we don't want
		  to saturate the network with tv_frame_audio_t metadata.

		  The playback doesn't work at all right now, so i'm opting for
		  the one packet approach for simplicity on that end (right now)
		 */
		audio_frame->set_packet_set(
			std::vector<std::vector<uint8_t> >(
				{packetized_encode[i]}));
		retval.push_back(
			audio_frame->id.get_id());
	}
	id_api::linked_list::link_vector(
		retval);
	return retval;
}

std::vector<std::vector<uint8_t> > transcode::audio::raw::to_codec(
	std::vector<uint8_t> raw_set,
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count,
	tv_audio_prop_t *output_audio_prop){
	std::vector<std::vector<uint8_t> > retval;
	tv_transcode_encode_state_t state;
	CHECK_ALL_FORMAT_AND_ENCODE_INIT_STATE(
		(*output_audio_prop),
		(*output_audio_prop),
		state);
	CHECK_ALL_FORMAT_AND_ENCODE_SAMPLES(
		(*output_audio_prop),
		&state,
		retval,
		raw_set,
		sampling_freq,
		bit_depth,
		channel_count);
	CHECK_ALL_FORMAT_AND_ENCODE_CLOSE_STATE(
		(*output_audio_prop),
		&state);
	return retval;
}
