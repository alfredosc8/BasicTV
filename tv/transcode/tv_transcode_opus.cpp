#include "tv_transcode.h"
#include "tv_transcode_audio.h"
#include "tv_transcode_state.h"
#include "opus/opus.h"

/*
  For simplicity, raw samples don't have a tv_audio_prop_t
 */

#pragma message("assert_opus_input doesn't check beyond format flag, worth fixing?")

/*
  TODO: should make these settings, but that's for another time
 */

static uint64_t get_frame_size(tv_audio_prop_t audio_prop){
	const uint64_t retval = (audio_prop.get_snippet_duration_micro_s()/1000)*(audio_prop.get_sampling_freq()/1000)*audio_prop.get_channel_count();
	P_V(retval, P_NOTE);
	return retval;
}

static tv_audio_prop_t gen_standard_opus_format(){
	tv_audio_prop_t retval;
	retval.set_sampling_freq(
		48000); // OPUS_BANDWIDTH_FULLBAND
	retval.set_bit_depth(
		16); // good enough
	retval.set_bit_rate(
		65536); // actual bits
	retval.set_channel_count(
		1);
	retval.set_snippet_duration_micro_s(
		10000);
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
			print("only going with ms frame sizes right now, can expand when proven to work", P_WARN);
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

// currently doesn't write anything to audio_prop
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

std::vector<std::vector<uint8_t> > opus_decode_snippet_vector_to_sample_vector(
	tv_transcode_decode_state_t *state,
	std::vector<std::vector<uint8_t> > snippet_vector,
	uint32_t *sampling_freq,
	uint8_t *bit_depth,
	uint8_t *channel_count){

	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_NULL(state->get_state_ptr(), P_ERR);
	PRINT_IF_EMPTY(snippet_vector, P_ERR);
	PRINT_IF_EMPTY(snippet_vector[0], P_ERR);
	ASSERT(sampling_freq != nullptr && bit_depth != nullptr && channel_count != nullptr, P_ERR);


	std::vector<std::vector<uint8_t> > retval;
	for(uint64_t i = 0;i < snippet_vector.size();i++){
		std::vector<uint8_t> tmp(OPUS_MAX_PACKET_SIZE, 0);
		const int32_t opus_retval =
			opus_decode(
				(OpusDecoder*)state->get_state_ptr(),
				snippet_vector[i].data(),
				snippet_vector[i].size(),
				(opus_int16*)tmp.data(),
				OPUS_MAX_PACKET_SIZE/state->get_audio_prop().get_channel_count(),
				0); // FEC
		if(opus_retval > 0){
			print("successfuly decode opus data", P_SPAM);
			tmp.erase(
				tmp.begin()+opus_retval,
				tmp.end());
			retval.push_back(tmp);
		}else if(opus_retval == 0){
			print("no opus data to decode", P_WARN);
		}else{
			print("opus decode failed with error code " + std::to_string(opus_retval), P_WARN);
		}
	}
	*sampling_freq = state->get_audio_prop().get_sampling_freq();
	*bit_depth = state->get_audio_prop().get_bit_depth();
	*channel_count = state->get_audio_prop().get_channel_count();
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

std::vector<std::vector<uint8_t> >  opus_encode_sample_vector_to_snippet_vector(
	tv_transcode_encode_state_t* state,
	std::vector<std::vector<uint8_t> > sample_vector,
	uint32_t sampling_freq,
	uint8_t bit_depth,
	uint8_t channel_count){ // SPSPC is a state property

	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_NULL(state->get_state_ptr(), P_ERR);
	PRINT_IF_EMPTY(sample_vector, P_ERR);
	std::vector<std::vector<uint8_t> > retval;
	if(bit_depth != state->get_audio_prop().get_bit_depth() ||
	   sampling_freq != state->get_audio_prop().get_sampling_freq() ||
	   channel_count != state->get_audio_prop().get_channel_count()){
		print("there is no raw transcoding yet, so I can't do anything", P_ERR);
	}
	const uint32_t iterator_value =
		get_frame_size(state->get_audio_prop());
	std::vector<uint8_t> sample_vector_flat =
		convert::vector::collapse_2d_vector(
			sample_vector);
	P_V(iterator_value, P_VAR);
	for(uint64_t i = 0;sample_vector_flat.size() > iterator_value+i;i += iterator_value){
		uint8_t encoded_data_tmp[OPUS_MAX_PACKET_SIZE];
		const opus_int32 encode_retval =
			opus_encode(
				(OpusEncoder*)state->get_state_ptr(),
				(const opus_int16*)&(sample_vector_flat[i]),
				iterator_value,
				&(encoded_data_tmp[0]),
				OPUS_MAX_PACKET_SIZE);
		if(encode_retval > 0){
			retval.push_back(
				std::vector<uint8_t>(
					&(encoded_data_tmp[0]),
					&(encoded_data_tmp[0])+encode_retval));
		}else if(encode_retval == 0){
			print("no data encoded", P_WARN);
		}else{
			print("opus encode failed with error " + std::to_string(encode_retval), P_ERR);
			/*
			  Opus' state is a shallow copy, so we could back up
			  the state information and re-create it later, but i'm 
			  not sure how common that would be across codecs.
			  It's useful enough for me to look into it at least.

			  I'm fine with cloning tv_transcode_*_state_t with
			  different depths for this sort of use case as
			  a generic state codec function.
			*/
		}
	}
	P_V((long double)sample_vector_flat.size()/(long double)iterator_value, P_NOTE);
	return retval;
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
