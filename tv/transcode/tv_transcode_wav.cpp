#include "tv_transcode.h"

static void wave_push_back(std::vector<uint8_t> *retval, const char *data){
	retval->push_back(((uint8_t*)data)[0]);
	retval->push_back(((uint8_t*)data)[1]);
	retval->push_back(((uint8_t*)data)[2]);
	retval->push_back(((uint8_t*)data)[3]);
}

static void wave_push_back(std::vector<uint8_t> *retval, uint32_t data){
	retval->push_back(((uint8_t*)&data)[0]);
	retval->push_back(((uint8_t*)&data)[1]);
	retval->push_back(((uint8_t*)&data)[2]);
	retval->push_back(((uint8_t*)&data)[3]);
}

static void wave_push_back(std::vector<uint8_t> *retval, uint16_t data){
	retval->push_back(((uint8_t*)&data)[0]);
	retval->push_back(((uint8_t*)&data)[1]);
}

/*
  Even though WAV isn't a state codec, we can still write some mock code that
  uses the pointer it gives us to store some information to help with lost
  packets and latencies (I don't think we need it for anything, but it can't
  be a nullptr for rogue non-compliant code taking a peek at it)
*/

tv_transcode_encode_state_t *wave_encode_init_state(tv_audio_prop_t *audio_prop){
	tv_transcode_encode_state_t *retval =
		new tv_transcode_encode_state_t;
	audio_prop->set_format(
		TV_AUDIO_FORMAT_WAV);
	if(audio_prop->get_snippet_duration_micro_s() == 0){
		print("no snippet duration provided for WAVE, assuming 10ms", P_WARN);
		audio_prop->set_snippet_duration_micro_s(
			1000*10);
	}
	audio_prop->set_sampling_freq(
		48000);
	audio_prop->set_bit_depth(
		16);
	audio_prop->set_channel_count(
		1);
	retval->set_audio_prop(*audio_prop);
	// state_ptr is a universal wave prepend, saves a lot of time
	const uint32_t snippet_size =
		duration_metadata_to_chunk_size(
			audio_prop->get_snippet_duration_micro_s(),
			audio_prop->get_sampling_freq(),
			audio_prop->get_bit_depth(),
			audio_prop->get_channel_count());
	std::vector<uint8_t> *universal_wave_prepend =
		new std::vector<uint8_t>;
	wave_push_back(universal_wave_prepend, "RIFF");
	wave_push_back(universal_wave_prepend, (uint32_t)(snippet_size+36));
	wave_push_back(universal_wave_prepend, "WAVE");
	wave_push_back(universal_wave_prepend, "fmt ");
	wave_push_back(universal_wave_prepend, (uint32_t)16); // length of data section
	wave_push_back(universal_wave_prepend, (uint16_t)1); // uncompressed PCM
	wave_push_back(universal_wave_prepend, (uint16_t)audio_prop->get_channel_count()); // channel count
	wave_push_back(universal_wave_prepend, (uint32_t)audio_prop->get_sampling_freq());
	wave_push_back(universal_wave_prepend, (uint32_t)(audio_prop->get_sampling_freq()*audio_prop->get_channel_count()*audio_prop->get_bit_rate()/8));
	wave_push_back(universal_wave_prepend, (uint16_t)(audio_prop->get_channel_count()*audio_prop->get_bit_depth()/8)); // block align
	wave_push_back(universal_wave_prepend, (uint16_t)audio_prop->get_bit_depth());
	wave_push_back(universal_wave_prepend, "data");
	wave_push_back(universal_wave_prepend, (uint32_t)snippet_size);
	retval->set_state_ptr(
		(void*)universal_wave_prepend);

	return retval;
}

void wave_encode_close_state(tv_transcode_encode_state_t *encode_state){
	delete (std::vector<uint8_t>*)encode_state->get_state_ptr();
	delete encode_state;
}

tv_transcode_decode_state_t *wave_decode_init_state(tv_audio_prop_t *audio_prop){
	tv_transcode_decode_state_t *retval =
		new tv_transcode_decode_state_t;
	retval->set_audio_prop(*audio_prop);
	return retval;
}

void wave_decode_close_state(tv_transcode_decode_state_t *decode_state){
	delete decode_state;
}



/*
  Since the rest of the software assumes that this is a packetized system, and
  there is no repacketizer for this yet, we can segment this into 60ms segments
  to fall inline with Opus (but it could be any number we want, since the
  segmenting is only important to encapsulating it in tv_frame_audio_t)
 */

std::vector<std::vector<uint8_t> > wave_encode_samples_to_snippets(tv_transcode_encode_state_t *state,
								   std::vector<uint8_t> *raw_data,
								   uint32_t sampling_freq,
								   uint8_t bit_depth,
								   uint8_t channel_count){
	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_EMPTY(*raw_data, P_ERR);
	assert_sane_audio_metadata(
		sampling_freq,
		bit_depth,
		channel_count);
	assert_compatiable_audio_metadata(
		sampling_freq,
		bit_depth,
		channel_count,
		state->get_audio_prop());
	if(state->get_audio_prop().get_bit_depth() > 8){
		print("wave bit depth larger than eight, converting to signed samples", P_DEBUG);
		*raw_data =
			transcode::audio::raw::unsigned_to_signed(
				*raw_data,
				state->get_audio_prop().get_bit_depth());
	}else{
		print("wave bit depth less than or equal to eight, no conversion", P_NOTE);
	}
	std::vector<std::vector<uint8_t> > encoded_data =
		samples_to_chunks_of_length(
		        raw_data,
			duration_metadata_to_chunk_size(
				state->get_audio_prop().get_snippet_duration_micro_s(),
				state->get_audio_prop().get_sampling_freq(),
				state->get_audio_prop().get_bit_depth(),
				state->get_audio_prop().get_channel_count()));
	for(uint64_t i = 0;i < encoded_data.size();i++){
		encoded_data[i].insert(
			encoded_data[i].begin(),
			((std::vector<uint8_t>*)state->get_state_ptr())->begin(),
			((std::vector<uint8_t>*)state->get_state_ptr())->end());
	}
	return encoded_data;
}

static std::vector<uint8_t> wave_pull_back(std::vector<uint8_t> *data, uint32_t len){
	if(data->size() < len){
		print("WAVE data is corrutped or not finished", P_ERR);
	}
	std::vector<uint8_t> retval(
		data->begin(),
		data->begin()+len);
	data->erase(
		data->begin(),
		data->begin()+len);
	return retval;
}

// easier to read, that's it
static std::vector<uint8_t> wave_pull_back(std::vector<uint8_t> *data, std::string str){
	return wave_pull_back(data, str.size());
}

static uint64_t wave_pull_back_eight_byte(std::vector<uint8_t> *data, uint32_t len){
	uint64_t tmp_data = 0;
	if(data->size() < len){
		print("wave file doesn't have enough room for len", P_ERR);
	}
	memcpy(&tmp_data,
	       data->data(),
	       len);
	data->erase(
		data->begin(),
		data->begin()+len);
	return tmp_data;
}

#pragma message("haven't tested wav_decode_raw, seems pretty sketch")

#pragma message("wave_decode_snippets_to_samples doesn't search for start of next packet in subvectors, should be safe to do with precautions")

static void sanity_check_prepend(std::vector<uint8_t> *universal_wave_prepend,
				 uint32_t target_sampling_freq,
				 uint8_t target_bit_depth,
				 uint8_t target_channel_count){
	wave_pull_back(universal_wave_prepend, "RIFF");
	wave_pull_back(universal_wave_prepend, 4);
	wave_pull_back(universal_wave_prepend, "WAVE");
	wave_pull_back(universal_wave_prepend, "fmt ");
	wave_pull_back(universal_wave_prepend, 4); // length of data section
	wave_pull_back(universal_wave_prepend, 2); // uncompressed PCM
	const uint64_t channel_count =
		wave_pull_back_eight_byte(universal_wave_prepend, 2); // channel count
	const uint64_t sampling_freq =
		wave_pull_back_eight_byte(universal_wave_prepend, 4);
	wave_pull_back(universal_wave_prepend, 4);
	wave_pull_back(universal_wave_prepend, 2); // block align
	const uint64_t bit_depth = 
		wave_pull_back_eight_byte(universal_wave_prepend, 2);
	wave_pull_back(universal_wave_prepend, "data");
	const bool good_samp_freq =
		sampling_freq == target_sampling_freq;
	const bool good_channel_count =
		channel_count == target_channel_count;
	const bool good_bit_depth =
		bit_depth == target_bit_depth;
	if(!(good_samp_freq && good_channel_count && good_bit_depth)){
		print("invalid WAVE data", P_ERR);
	}
}

/*
  This function should also RESIZE the 2D vector so any snippets that
  are successfully decoded don't become blank
 */

std::vector<uint8_t> wave_decode_snippets_to_samples(tv_transcode_decode_state_t *state,
						     std::vector<std::vector<uint8_t> > *wav_data,
						     uint32_t *sampling_freq,
						     uint8_t *bit_depth,
						     uint8_t *channel_count){
	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_EMPTY(*wav_data, P_ERR);
	PRINT_IF_EMPTY((*wav_data)[0], P_ERR);
	std::vector<uint8_t> retval;
	uint64_t old_wav_data_size = 0;
	while(old_wav_data_size != wav_data->size()){
		sanity_check_prepend(
			&((*wav_data)[0]),
			state->get_audio_prop().get_sampling_freq(),
			state->get_audio_prop().get_bit_depth(),
			state->get_audio_prop().get_channel_count());
		P_V(wav_data->begin()->size(), P_VAR);
		retval.insert(
			retval.end(),
			wav_data->begin()->begin(),
			wav_data->begin()->end());
		wav_data->erase(
			wav_data->begin());
	}
	*sampling_freq =
		state->get_audio_prop().get_sampling_freq();
	*bit_depth =
		state->get_audio_prop().get_bit_depth();
	*channel_count =
		state->get_audio_prop().get_channel_count();
	if(*bit_depth > 8){
		print("wave bit depth larger than eight, converting to signed samples", P_DEBUG);
		retval =
			transcode::audio::raw::signed_to_unsigned(
				retval,
				*bit_depth);
	}else{
		// far less common
		print("wave bit depth less than or equal to eight, no conversion", P_NOTE);
	}
	P_V(retval.size(), P_VAR);
	return retval;
}
