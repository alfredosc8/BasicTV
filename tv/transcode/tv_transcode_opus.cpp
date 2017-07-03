#include "tv_transcode.h"
#include "tv_transcode_audio.h"
#include "tv_transcode_state.h"
#include "opus/opus.h"

#pragma message("assert_opus_input doesn't check beyond format flag, worth fixing?")

/*
  TODO: should make these settings, but that's for another time

  Convert this over to some of the tv_transcode_audio.cpp abstractions
 */

static tv_audio_prop_t gen_standard_opus_format(){
	tv_audio_prop_t retval;
	retval.set_sampling_freq(
		48000); // OPUS_BANDWIDTH_FULLBAND4
	retval.set_bit_depth(
		16); // good enough
	retval.set_bit_rate(
		65536); // actual bits
	retval.set_channel_count(
		1);
	retval.set_snippet_duration_micro_s(
		40*1000);
	retval.set_format(
		TV_AUDIO_FORMAT_OPUS);
	return retval;
}

static bool assert_fix_opus_prop(tv_audio_prop_t *audio_prop){
	PRINT_IF_NULL(audio_prop, P_ERR);
	ASSERT(audio_prop->get_format() == TV_AUDIO_FORMAT_OPUS, P_ERR);
	bool tainted = false;
	if((audio_prop->get_flags() & TV_AUDIO_PROP_FORMAT_ONLY) == false){
		print("TODO: actually check the other variables, don't trust it's correct Opus by format flag", P_WARN);
		if(audio_prop->get_sampling_freq() != 48000){
			print("fullband is recommended, forcing", P_WARN);
			audio_prop->set_sampling_freq(
				48000);
			// could do some rounding, but that's not a big deal
			// right now
			tainted = true;
		}
		if(BETWEEN(6000, audio_prop->get_bit_rate(), 510000) == false){
			print("output bit rate isn't compatiable with Opus, selecting 64", P_WARN);
			audio_prop->set_bit_rate(
				65536);
			tainted = true;
		}
		if(audio_prop->get_channel_count() != 1){
			print("only going with mono right now, can expand when proven to work", P_WARN);
			audio_prop->set_channel_count(
				1);
			tainted = true;
		}
		if(audio_prop->get_snippet_duration_micro_s() != 10000){
			print("only going with 10ms frame sizes right now, can expand when proven to work", P_WARN);
			audio_prop->set_snippet_duration_micro_s(
				10000);
		}
	}else{
		print("only format is valid, inserting sane defaults", P_NOTE);
		*audio_prop = gen_standard_opus_format();
		tainted = true;
	}
	return tainted;
}

static void assert_opus_prop(tv_audio_prop_t audio_prop){
	if(assert_fix_opus_prop(&audio_prop)){
		print("assertion on sane opus values failed", P_ERR);
	}
}

#pragma message("opus_encode_init_state doesn't set CTLs for bitrate or specific encoder flags")

tv_transcode_encode_state_t *opus_encode_init_state(tv_audio_prop_t *audio_prop){
	tv_transcode_encode_state_t state;
	if(assert_fix_opus_prop(audio_prop)){
		print("given a bad audio_prop value, using standard Opus for ease of debugging", P_NOTE);
		*audio_prop = gen_standard_opus_format();
	}
	int32_t error = 0;
	OpusEncoder *encode_state =
		opus_encoder_create(
			audio_prop->get_sampling_freq(),
			audio_prop->get_channel_count(),
			OPUS_APPLICATION_AUDIO,
			&error);
	if(error != OPUS_OK){
		print("opus failed to create the tv_transcode_encode_t", P_ERR);
	}
	state.set_state_ptr((void*)encode_state);
	state.set_audio_prop(*audio_prop);
	encode_state_vector.push_back(
		state);
	return &encode_state_vector[encode_state_vector.size()-1];
}

tv_transcode_decode_state_t *opus_decode_init_state(tv_audio_prop_t *audio_prop){
	tv_transcode_decode_state_t state;
	if(assert_fix_opus_prop(audio_prop)){
		print("given a bad audio_prop value, using standard Opus for ease of debugging", P_NOTE);
		*audio_prop = gen_standard_opus_format();
	}
	int32_t error = 0;
	OpusDecoder *decode_state =
		opus_decoder_create(
			audio_prop->get_sampling_freq(),
			audio_prop->get_channel_count(),
			&error);
	if(error != OPUS_OK){
		print("opus failed to create the tv_transcode_decode_t", P_ERR);
	}
	state.set_state_ptr((void*)decode_state);
	state.set_audio_prop(*audio_prop);
	decode_state_vector.push_back(
		state);
	return &decode_state_vector[decode_state_vector.size()-1];
}

std::vector<uint8_t> opus_decode_snippets_to_samples(
	tv_transcode_decode_state_t *state,
	std::vector<std::vector<uint8_t> > *snippet_vector,
	uint32_t *sampling_freq,
	uint8_t *bit_depth,
	uint8_t *channel_count){

	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_NULL(state->get_state_ptr(), P_ERR);
	PRINT_IF_EMPTY(*snippet_vector, P_ERR);
	PRINT_IF_EMPTY((*snippet_vector)[0], P_ERR);

	ASSERT(sampling_freq != nullptr && bit_depth != nullptr && channel_count != nullptr, P_ERR);

	std::vector<uint8_t> retval;
	uint64_t old_vector_size = 0;
	while(snippet_vector->size() != old_vector_size){
		std::vector<int16_t> tmp(5760, 0);
		const int32_t opus_retval =
			opus_decode(
				(OpusDecoder*)state->get_state_ptr(),
				(*snippet_vector)[0].data(),
				(*snippet_vector)[0].size(),
				tmp.data(),
				5760,
				0); // FEC
		if(opus_retval > 0){
			retval.insert(
				retval.end(),
				((uint8_t*)&tmp[0]),
				((uint8_t*)&tmp[0])+(sizeof(opus_int16)*opus_retval));
			snippet_vector->erase(
				snippet_vector->begin());
 		}else if(opus_retval == 0){
			print("no opus data to decode", P_WARN);
			break;
		}else{
			print("opus decode failed with error code " + std::to_string(opus_retval), P_WARN);
			break;
		}
	}
	*sampling_freq = state->get_audio_prop().get_sampling_freq();
	*bit_depth = state->get_audio_prop().get_bit_depth();
	*channel_count = state->get_audio_prop().get_channel_count();
	retval = 
		transcode::audio::raw::signed_to_unsigned(
			retval,
			*bit_depth);
	return retval;
}

void opus_decode_close_state(tv_transcode_decode_state_t *state){
	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_NULL(state->get_state_ptr(), P_ERR);
	if(state == nullptr){
		print("state is a nullptr", P_ERR);
	}
	if(state->get_state_ptr() == nullptr){
		print("state_ptr is a nullptr", P_ERR);
	}
	opus_decoder_destroy(
		(OpusDecoder*)state->get_state_ptr());
	state->set_state_ptr(
		nullptr);
}

std::vector<std::vector<uint8_t> > opus_encode_samples_to_snippets(
	tv_transcode_encode_state_t* state,
	std::vector<uint8_t> *sample_vector,
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count){
	
	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_NULL(state->get_state_ptr(), P_ERR);
	PRINT_IF_EMPTY(*sample_vector, P_ERR);
	assert_sane_audio_metadata(
		sampling_freq,
		bit_depth,
		channel_count);
	assert_compatiable_audio_metadata(
		sampling_freq,
		bit_depth,
		channel_count,
		state->get_audio_prop());

	const uint64_t chunk_size =
		duration_metadata_to_chunk_size(
			state->get_audio_prop().get_snippet_duration_micro_s(),
			state->get_audio_prop().get_sampling_freq(),
			state->get_audio_prop().get_bit_depth(),
			state->get_audio_prop().get_channel_count());
	std::vector<std::vector<uint8_t> > encoded_data =
		samples_to_chunks_of_length(
			sample_vector,
			chunk_size);
	uint8_t encoded_data_tmp[OPUS_MAX_PACKET_SIZE];
	P_V(chunk_size, P_NOTE);
	for(uint64_t i = 0;i < encoded_data.size();i++){
		encoded_data[i] =
			transcode::audio::raw::unsigned_to_signed(
				encoded_data[i],
				bit_depth);
		if(chunk_size != encoded_data[i].size()){
			P_V(chunk_size, P_WARN);
			P_V(encoded_data[i].size(), P_WARN);
			print("chunk size and vector size discrepency", P_ERR);
		}
		const opus_int32 encode_retval =
			opus_encode(
				(OpusEncoder*)state->get_state_ptr(),
				(const opus_int16*)encoded_data[i].data(),
				encoded_data[i].size()/(sizeof(opus_int16)*state->get_audio_prop().get_channel_count()),
				&(encoded_data_tmp[0]),
				OPUS_MAX_PACKET_SIZE);
		if(encode_retval > 0){
			encoded_data[i] =
				std::vector<uint8_t>(
					&(encoded_data_tmp[0]),
					&(encoded_data_tmp[0])+encode_retval);
		}else if(encode_retval == 0){
			print("no data encoded", P_WARN);
		}else{
			print("opus encode failed with error " + std::to_string(encode_retval), P_ERR);
		}
	}
	return encoded_data;
}

void opus_encode_close_state(
	tv_transcode_encode_state_t *state){
	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_NULL(state->get_state_ptr(), P_ERR);
	opus_encoder_destroy(
		(OpusEncoder*)state->get_state_ptr());
	state->set_state_ptr(
		nullptr);
}
