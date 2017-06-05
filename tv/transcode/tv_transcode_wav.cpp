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

std::vector<uint8_t> wave_encode_raw(std::vector<uint8_t> raw_data,
				     uint32_t sampling_freq,
				     uint8_t bit_depth){
	std::vector<uint8_t> retval;
	wave_push_back(&retval, "RIFF");
	wave_push_back(&retval, (uint32_t)(raw_data.size()+36));
	wave_push_back(&retval, "WAVE");
	wave_push_back(&retval, "fmt ");
	wave_push_back(&retval, (uint32_t)16); // length of data section
	wave_push_back(&retval, (uint16_t)1); // uncompressed PCM
	wave_push_back(&retval, (uint16_t)1); // channel count
	wave_push_back(&retval, (uint32_t)sampling_freq);
	wave_push_back(&retval, (uint32_t)(sampling_freq*1*bit_depth/8));
	wave_push_back(&retval, (uint16_t)(1*bit_depth/8)); // block align
	wave_push_back(&retval, (uint16_t)bit_depth);
	wave_push_back(&retval, "data");
	wave_push_back(&retval, (uint32_t)raw_data.size());
	// WAV forces this data to be in signed 16-bit form, which this isn't
	retval.insert(
		retval.end(),
		raw_data.begin(),
		raw_data.end());
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

std::vector<uint8_t> wave_decode_raw(std::vector<uint8_t> wav_data,
				     uint32_t *sampling_freq,
				     uint8_t *bit_depth,
				     uint8_t *channel_count){
	wave_pull_back(&wav_data, "RIFF");
	wave_pull_back(&wav_data, 4);
	wave_pull_back(&wav_data, "WAVE");
	wave_pull_back(&wav_data, "fmt ");
	wave_pull_back(&wav_data, 4); // length of data section
	wave_pull_back(&wav_data, 2); // uncompressed PCM
	*channel_count =
		wave_pull_back_eight_byte(&wav_data, 2); // channel count
	*sampling_freq =
		wave_pull_back_eight_byte(&wav_data, 4);
	wave_pull_back(&wav_data, 4);
	wave_pull_back(&wav_data, 2); // block align
	*bit_depth = 
		wave_pull_back_eight_byte(&wav_data, 2);
	wave_pull_back(&wav_data, "data");
	wave_pull_back(&wav_data, 4);
	if(*channel_count == 1){
		return wav_data;
	}else{
		print("there is no formal multi-channel format for raw, should copy WAV", P_ERR);
	}
	P_V(*sampling_freq, P_VAR);
	P_V(*channel_count, P_VAR);
	P_V(*bit_depth, P_VAR);
	return wav_data;
}

// std::vector<id_t_> tv_audio_load_wav(id_t_ tv_item_id, uint64_t start_time_micro_s, std::string file){
// 	// five seconds ought to be enough
// 	const uint64_t frame_duration_micro_s =
// 		1000*1000;
// 	P_V(output_sampling_rate, P_VAR);
// 	P_V(output_bit_depth, P_VAR);
// 	const uint64_t sample_length_per_frame =
// 		((output_sampling_rate*output_bit_depth)/8.0)*(frame_duration_micro_s/(1000.0*1000.0));
// 	uint64_t current_start = 0;
// 	std::vector<id_t_> audio_frame_vector;
// 	while(current_start < chunk->alen){
// 		tv_frame_audio_t *audio =
// 			new tv_frame_audio_t;
// 		uint64_t length = 0;
// 		if(current_start + sample_length_per_frame < chunk->alen){
// 			print("adding full value of sample_length_per_frame", P_SPAM);
// 			length = sample_length_per_frame;
// 		}else{
// 			print("can't add full frame, ran out of room", P_SPAM);
// 			length = chunk->alen-current_start;
// 		}
// 		audio->set_data(
// 			std::vector<uint8_t>(
// 				&(chunk->abuf[current_start]),
// 				&(chunk->abuf[current_start+length])
// 				)
// 			);
// 		tv_audio_prop_t audio_prop;
// 		audio_prop.set_format(
// 			TV_AUDIO_FORMAT_RAW);
// 		audio_prop.set_sampling_freq(
// 			output_sampling_rate);
// 		audio_prop.set_bit_depth(
// 			output_bit_depth);
// 		audio->set_audio_prop(
// 			audio_prop);
// 		audio->set_standard(start_time_micro_s+(frame_duration_micro_s*audio_frame_vector.size()),
// 				    frame_duration_micro_s,
// 				    audio_frame_vector.size());
// 		P_V(current_start, P_VAR);
// 		P_V(length, P_VAR);
// 		current_start += length;
// 		audio_frame_vector.push_back(
// 			audio->id.get_id());
// 	}
// 	id_api::linked_list::link_vector(audio_frame_vector);
// 	tv_item_t *item_ptr =
// 		PTR_DATA(tv_item_id,
// 			 tv_item_t);
// 	if(item_ptr == nullptr){
// 		print("item_ptr is a nullptr", P_ERR);
// 	}
// 	item_ptr->add_frame_id(audio_frame_vector);
// 	Mix_FreeChunk(chunk);
// 	chunk = nullptr;
// 	return audio_frame_vector;
// }

/*
  Since WAV 'packets' have no state whatsoever, this is a nice template for
  how everything should work.
 */

// std::vector<id_t_> frame_set
// tv_audio_prop_t *output_audio_prop

// static void assert_fix_wav_prop(tv_audio_prop_t
