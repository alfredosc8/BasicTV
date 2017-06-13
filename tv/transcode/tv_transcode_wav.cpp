#include "tv_transcode.h"

static void wave_push_back(std::vector<uint8_t> *retval, const char *data){
	retval->push_back(((uint8_t*)data)[0]);
	retval->push_back(((uint8_t*)data)[1]);
	retval->push_back(((uint8_t*)data)[2]);
	retval->push_back(((uint8_t*)data)[3]);
}

static void wave_push_back(std::vector<uint8_t> *retval, uint32_t data){
#ifdef IS_BIG_ENDIAN
	data = NBO_32(data);
#endif
	retval->push_back(((uint8_t*)&data)[0]);
	retval->push_back(((uint8_t*)&data)[1]);
	retval->push_back(((uint8_t*)&data)[2]);
	retval->push_back(((uint8_t*)&data)[3]);
}

static void wave_push_back(std::vector<uint8_t> *retval, uint16_t data){
#ifdef IS_BIG_ENDIAN
	data = NBO_16(data);
#endif
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
	// WAV allows for pretty much any format, so no need to sanity check
	retval->set_audio_prop(*audio_prop);
	return retval;
}

void wave_encode_close_state(tv_transcode_encode_state_t *encode_state){
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

std::vector<std::vector<uint8_t> > wave_encode_sample_vector_to_snippet_vector(tv_transcode_encode_state_t *state,
									       std::vector<std::vector<uint8_t> > raw_data_,
									       uint32_t sampling_freq,
									       uint8_t bit_depth,
									       uint8_t channel_count){
	PRINT_IF_NULL(state, P_ERR);
	if(channel_count != 1){
		print("multichannel WAV isn't supported right now", P_ERR);
	}
	std::vector<std::vector<uint8_t> > retval;
	std::vector<uint8_t> raw_data =
		convert::vector::collapse_2d_vector(
			raw_data_);
	P_V(sampling_freq, P_VAR);
	P_V(bit_depth, P_VAR);
	uint64_t snippet_size =
		sampling_freq*(bit_depth/8)*60; // not in microseconds
	if(snippet_size > raw_data.size()){
		snippet_size = raw_data.size();
	}
	P_V(snippet_size, P_VAR);
	// TODO: might be better to define this as a snprintf
	for(uint64_t i = 0;i <= raw_data.size()-snippet_size;i+=snippet_size){
		std::vector<uint8_t> tmp;
		wave_push_back(&tmp, "RIFF");
		wave_push_back(&tmp, (uint32_t)(snippet_size+36));
		wave_push_back(&tmp, "WAVE");
		wave_push_back(&tmp, "fmt ");
		wave_push_back(&tmp, (uint32_t)16); // length of data section
		wave_push_back(&tmp, (uint16_t)1); // uncompressed PCM
		wave_push_back(&tmp, (uint16_t)1); // channel count
		wave_push_back(&tmp, (uint32_t)sampling_freq);
		wave_push_back(&tmp, (uint32_t)(sampling_freq*1*bit_depth/8));
		wave_push_back(&tmp, (uint16_t)(1*bit_depth/8)); // block align
		wave_push_back(&tmp, (uint16_t)bit_depth);
		wave_push_back(&tmp, "data");
		wave_push_back(&tmp, (uint32_t)snippet_size);
		// WAV forces this data to be in signed 16-bit form, which this isn't
		retval.push_back(
			tmp);
		retval[retval.size()-1].insert(
			retval[retval.size()-1].end(),
			raw_data.begin()+i,
			raw_data.begin()+i+snippet_size);
	}
	return retval;
}

static std::vector<uint8_t> wave_pull_back(std::vector<uint8_t> *data, uint32_t len){
	std::vector<uint8_t> retval(
		data->begin(),
		data->begin()+len);
	retval = convert::nbo::to(retval);
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
	std::vector<uint8_t> tmp_data(8, 0);
	if(data->size() > len){
		print("wave file doesn't have enough room for len", P_ERR);
	}
	memcpy(tmp_data.data(),
	       data->data(),
	       len);
	return *((uint64_t*)tmp_data.data());
}

#pragma message("haven't tested wav_decode_raw, seems pretty sketch")

std::vector<std::vector<uint8_t> > wave_decode_snippet_vector_to_sample_vector(tv_transcode_decode_state_t *state,
									       std::vector<std::vector<uint8_t> > wav_data,
									       uint32_t *sampling_freq,
									       uint8_t *bit_depth,
									       uint8_t *channel_count){
	PRINT_IF_NULL(state, P_ERR);
	std::vector<std::vector<uint8_t> > retval;
	for(uint64_t i = 0;i < wav_data.size();i++){
		wave_pull_back(&wav_data[i], "RIFF");
		wave_pull_back(&wav_data[i], 4);
		wave_pull_back(&wav_data[i], "WAVE");
		wave_pull_back(&wav_data[i], "fmt ");
		wave_pull_back(&wav_data[i], 4); // length of data section
		wave_pull_back(&wav_data[i], 2); // uncompressed PCM
		*channel_count =
			wave_pull_back_eight_byte(&wav_data[i], 2); // channel count
		*sampling_freq =
			wave_pull_back_eight_byte(&wav_data[i], 4);
		wave_pull_back(&wav_data[i], 4);
		wave_pull_back(&wav_data[i], 2); // block align
		*bit_depth = 
			wave_pull_back_eight_byte(&wav_data[i], 2);
		wave_pull_back(&wav_data[i], "data");
		wave_pull_back(&wav_data[i], 4);
		if(*channel_count != 1){
			print("there is no formal multi-channel format for raw, should copy WAV", P_ERR);
		}
		retval.push_back(wav_data[i]);
	}
	return retval;
}
