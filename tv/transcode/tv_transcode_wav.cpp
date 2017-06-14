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
	// WAV allows for pretty much any format, so no need to sanity check
	audio_prop->set_snippet_duration_micro_s(
		1000*10);
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


std::vector<std::vector<uint8_t> > wave_encode_samples_to_snippets(tv_transcode_encode_state_t *state,
								   std::vector<uint8_t> *raw_data,
								   uint32_t sampling_freq,
								   uint8_t bit_depth,
								   uint8_t channel_count){
	PRINT_IF_NULL(state, P_ERR);
	PRINT_IF_EMPTY(*raw_data, P_ERR);
	if(channel_count != 1){
		print("multichannel WAV isn't supported right now", P_ERR);
	}
	std::vector<std::vector<uint8_t> > retval;
	uint64_t snippet_size =
		(state->get_audio_prop().get_snippet_duration_micro_s()/1000)*(sampling_freq)*channel_count;
	if(snippet_size > raw_data->size()){
		/*
		  WAVE is complying as hard as it can right now to the state
		  system that's in place for Opus
		 */
	}
	uint64_t old_vector_size = 0;
	while(raw_data->size() != old_vector_size &&
	      raw_data->size() >= snippet_size){
		std::vector<uint8_t> tmp;
		wave_push_back(&tmp, "RIFF");
		wave_push_back(&tmp, (uint32_t)(snippet_size+36));
		wave_push_back(&tmp, "WAVE");
		wave_push_back(&tmp, "fmt ");
		wave_push_back(&tmp, (uint32_t)16); // length of data section
		wave_push_back(&tmp, (uint16_t)1); // uncompressed PCM
		wave_push_back(&tmp, (uint16_t)channel_count); // channel count
		wave_push_back(&tmp, (uint32_t)sampling_freq);
		wave_push_back(&tmp, (uint32_t)(sampling_freq*channel_count*bit_depth/8));
		wave_push_back(&tmp, (uint16_t)(channel_count*bit_depth/8)); // block align
		wave_push_back(&tmp, (uint16_t)bit_depth);
		wave_push_back(&tmp, "data");
		wave_push_back(&tmp, (uint32_t)snippet_size);
		// WAV forces this data to be in signed 16-bit form, which this isn't
		P_V(tmp.size(), P_NOTE);
		retval.push_back(
			tmp);
		retval[retval.size()-1].insert(
			retval[retval.size()-1].end(),
			raw_data->begin(),
			raw_data->begin()+snippet_size);
		raw_data->erase(
			raw_data->begin(),
			raw_data->begin()+snippet_size);
	}
	return retval;
}

static std::vector<uint8_t> wave_pull_back(std::vector<uint8_t> *data, uint32_t len){
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

#define DISCREP_DETECT(x) if(*x != state->get_audio_prop().get_##x()){print((std::string)#x + " discrepency detected in wave", P_ERR);}

#pragma message("wave_decode_snippets_to_samples doesn't search for start of next packet in subvectors, should be safe to do with precautions")

std::vector<uint8_t> wave_decode_snippets_to_samples(tv_transcode_decode_state_t *state,
						     std::vector<std::vector<uint8_t> > *wav_data,
						     uint32_t *sampling_freq,
						     uint8_t *bit_depth,
						     uint8_t *channel_count){
	// used internally in tv_audio without a state, pretty hacky, but
	// it works I guess (should go with RAW samples)
	PRINT_IF_NULL(state, P_ERR);
	// state->get_state_ptr() is useless right now
	PRINT_IF_EMPTY(*wav_data, P_ERR);
	PRINT_IF_EMPTY((*wav_data)[0], P_ERR);
	std::vector<uint8_t> retval;
	const uint64_t snippet_size =
		(state->get_audio_prop().get_snippet_duration_micro_s()/1000)*(state->get_audio_prop().get_sampling_freq())*state->get_audio_prop().get_channel_count();
	for(uint64_t c = 0;c < wav_data->size();c++){
		uint64_t old_vector_size = 0;
		while((*wav_data)[c].size() != old_vector_size){
			old_vector_size = (*wav_data)[c].size();
			wave_pull_back(&(*wav_data)[c], "RIFF");
			wave_pull_back(&(*wav_data)[c], 4);
			wave_pull_back(&(*wav_data)[c], "WAVE");
			wave_pull_back(&(*wav_data)[c], "fmt ");
			wave_pull_back(&(*wav_data)[c], 4); // length of data section
			wave_pull_back(&(*wav_data)[c], 2); // uncompressed PCM
			*channel_count =
				wave_pull_back_eight_byte(&(*wav_data)[c], 2); // channel count
			*sampling_freq =
				wave_pull_back_eight_byte(&(*wav_data)[c], 4);
			wave_pull_back(&(*wav_data)[c], 4);
			wave_pull_back(&(*wav_data)[c], 2); // block align
			*bit_depth = 
				wave_pull_back_eight_byte(&(*wav_data)[c], 2);
			wave_pull_back(&(*wav_data)[c], "data");
			DISCREP_DETECT(channel_count);
			DISCREP_DETECT(sampling_freq);
			DISCREP_DETECT(bit_depth);
			const uint32_t chunk_size =
				(uint32_t)wave_pull_back_eight_byte(&(*wav_data)[c], 4);
			if((uint32_t)chunk_size != (uint32_t)snippet_size){
				// We can pretty much definitively end this safely by
				// just searching for the next header format, but
				// WAVE is foolproof enough that only malicious clients
				// would trip it right now (and I have other things
				// to work on right now)
				print("discrepency between wave chunk size and state chunk size, can't definitively end", P_ERR);
			}
			P_V(*sampling_freq, P_VAR);
			P_V(*bit_depth, P_VAR);
			P_V(*channel_count, P_VAR);
			if(*channel_count != 1){
				print("there is no formal multi-channel format for raw, should copy WAV", P_ERR);
			}
			retval.insert(
				retval.end(),
				(*wav_data)[c].begin(),
				(*wav_data)[c].begin()+chunk_size);
			(*wav_data)[c].erase(
				(*wav_data)[c].begin(),
				(*wav_data)[c].begin()+chunk_size);
		}
	}
	return retval;
}
