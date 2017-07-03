#include "tv_frame_audio.h"
#include "tv_audio.h"
#include "tv_window.h"
#include "tv_channel.h"
#include "tv_item.h"
#include "tv.h"
#include "../convert.h"
#include "../settings.h"
#include "transcode/tv_transcode.h"
#include "../util.h"

/*
  We have more flexibility to make things right now that the codebase
  has switched to SDL_Audio
 */

static SDL_AudioDeviceID audio_device_id = 0;
static SDL_AudioSpec desired, have;

// static uint32_t output_sampling_rate = 0;
// static uint8_t output_bit_depth = 0;
// static uint8_t output_channel_count = 0;
// static uint32_t output_chunk_size = 0;

/*
  TODO: implement a universal time state that all streams can sync up with
  (tv_time_t?)
 */

// tv_audio_channel_t is simple enough to stay in this file

tv_audio_prop_t::tv_audio_prop_t(){
}

tv_audio_prop_t::~tv_audio_prop_t(){
}

void tv_audio_prop_t::list_virtual_data(data_id_t *id){
	id->add_data_raw(&flags, sizeof(flags));
	id->add_data_raw(&format, sizeof(format));
	id->add_data_raw(&bit_depth, sizeof(bit_depth));
	id->add_data_raw(&sampling_freq, sizeof(sampling_freq));
}

static std::vector<std::pair<id_t_, std::vector<uint8_t> > > samples_to_play;

static void tv_audio_sdl_callback(void *userdata, uint8_t *stream, int32_t len){
	// only plays the first for now
	ASSERT(userdata == nullptr, P_ERR);
	if(len == 0){
		return;
	}
	SDL_memset(stream, 0, len);
	if(samples_to_play.size() != 0){
		int32_t true_len = len;
		if(samples_to_play.at(0).second.size() < (uint32_t)true_len){
			print("we need to queue more audio data", P_WARN);
			true_len = samples_to_play.at(0).second.size();
		}
		SDL_MixAudio(
			stream,
			samples_to_play.at(0).second.data(),
			true_len,
			SDL_MIX_MAXVOLUME);
		samples_to_play.at(0).second.erase(
			samples_to_play.at(0).second.begin()+true_len);
	}
}

static void tv_audio_add_id_to_audio_queue(
	id_t_ window_id,
	id_t_ frame_id,
	uint64_t forward_buffer_micro_s){
	P_V(forward_buffer_micro_s, P_NOTE); // not yet re-implemented
	uint32_t sampling_freq = 0;
	uint8_t bit_depth = 0;
	uint8_t channel_count = 0;
	std::vector<uint8_t> raw =
		transcode::audio::frames::to_raw(
			{frame_id},
			&sampling_freq,
			&bit_depth,
			&channel_count);
	ASSERT(have.freq == (int32_t)sampling_freq, P_ERR);
	ASSERT(bit_depth == 16, P_ERR);
	ASSERT(channel_count == 1, P_ERR); //temporary
	int64_t entry = -1;
	SDL_LockAudioDevice(audio_device_id);
	for(uint64_t i = 0;i < samples_to_play.size();i++){
		if(samples_to_play[i].first == window_id){
			entry = (int64_t)i;
		}
	}
	if(entry == -1){
		samples_to_play.push_back(
			std::make_pair(window_id, raw));
		entry = samples_to_play.size()-1;
	}else{
		samples_to_play[entry].second.insert(
			samples_to_play[entry].second.begin(),
			raw.begin(),
			raw.end());
	}
	SDL_UnlockAudioDevice(audio_device_id);
}

void tv_audio_init(){
	if(settings::get_setting("audio") == "true"){
		SDL_Init(SDL_INIT_AUDIO);
		// try{
		// 	output_sampling_rate =
		// 		std::stoi(
		// 			settings::get_setting(
		// 				"snd_output_sampling_rate"));
		// 	output_bit_depth =
		// 		std::stoi(
		// 			settings::get_setting(
		// 				"snd_output_bit_depth"));
		// 	output_channel_count =
		// 		std::stoi(
		// 			settings::get_setting(
		// 				"snd_output_channel_count"));
		// 	output_chunk_size =
		// 		std::stoi(
		// 			settings::get_setting(
		// 				"snd_output_chunk_size"));
		// }catch(...){
		// 	// no big problem, these values are sane (maybe not chunk size)
		// 	print("cannot read sound settings from file, setting default", P_WARN);
		// 	output_sampling_rate = TV_AUDIO_DEFAULT_SAMPLING_RATE;
		// 	output_bit_depth = TV_AUDIO_DEFAULT_BIT_DEPTH;
		// 	output_channel_count = TV_AUDIO_DEFAULT_CHANNEL_COUNT;
		// 	output_chunk_size = TV_AUDIO_DEFAULT_CHUNK_SIZE;
		// }
		CLEAR(desired);
		CLEAR(have);
		desired.freq =
			48000;
		desired.format =
			AUDIO_U16;
		desired.samples =
			4096; // Decoding latencies can push this pretty far
		desired.channels =
			1; // temporary
		desired.callback =
			tv_audio_sdl_callback;
		desired.userdata =
			nullptr;
		audio_device_id =
			SDL_OpenAudioDevice(
				nullptr,
				0,
				&desired,
				&have,
				0);
		P_V(desired.freq, P_WARN);
		P_V(desired.format, P_WARN);
		P_V(desired.channels, P_WARN);
		P_V(desired.samples, P_WARN);
		P_V(have.freq, P_WARN);
		P_V(have.format, P_WARN);
		P_V(have.channels, P_WARN);
		P_V(have.samples, P_WARN);

		P_V_B(have.format, P_WARN); // comments in SDL_audio.h make for better debuggin
		if(audio_device_id == 0){
			print("cannot open audio:" + (std::string)(SDL_GetError()), P_ERR);
		}
	}else{
		print("audio has been disabled in the settings", P_NOTE);
	}
}

static std::vector<std::pair<id_t_, id_t_> > tv_audio_get_current_frame_audios(){
	std::vector<id_t_> windows =
		id_api::cache::get(
			"tv_window_t");
	const uint64_t cur_timestamp_micro_s =
		get_time_microseconds();
	std::vector<std::pair<id_t_, id_t_> > frame_audios;
	for(uint64_t i = 0;i < windows.size();i++){
		tv_window_t *window =
			PTR_DATA(windows[i],
				 tv_window_t);
		if(window == nullptr){
			print("window is a nullptr", P_WARN);
			continue;
		}
		const std::vector<id_t_> active_streams =
			window->get_active_streams();
		if(active_streams.size() == 0){
			print("no designated active streams", P_WARN);
		}
		const uint64_t play_time =
			cur_timestamp_micro_s+window->get_timestamp_offset();
		for(uint64_t c = 0;c < active_streams.size();c++){
			tv_frame_audio_t *audio_frame_tmp =
				PTR_DATA(active_streams[c],
					 tv_frame_audio_t);
			if(audio_frame_tmp == nullptr){
				print("supposed active stream is a nullptr", P_ERR);
				continue;
			}
			id_t_ curr_id =
				tv_frame_scroll_to_time(
					audio_frame_tmp,
					play_time);
			// stream is no longer active
			if(curr_id != ID_BLANK_ID){
				frame_audios.push_back(
					std::make_pair(
						windows[i],
						curr_id));
			}else{
				print("curr_id is a nullptr, probably an invalid timestamp", P_WARN);
			}
		}
	}
	return frame_audios;
}

void tv_audio_loop(){
	if(settings::get_setting("audio") == "true"){
		SDL_PauseAudioDevice(audio_device_id, 0);
		std::vector<std::pair<id_t_, id_t_> > current_frame_audios =
			tv_audio_get_current_frame_audios();
		for(uint64_t i = 0;i < current_frame_audios.size();i++){
			tv_audio_add_id_to_audio_queue(
				current_frame_audios[i].first,
				current_frame_audios[i].second,
				1000*1000*5);
		}
	}
}

void tv_audio_close(){
}

/*
  No defined or known outputs, let's the encoder and decoder fill in the gaps,
  useful for making tv_audio_prop_t for output functions.
 */

tv_audio_prop_t gen_format_only_audio_prop(uint8_t fmt){
	tv_audio_prop_t retval;
	retval.set_format(fmt);
	retval.set_flags(TV_AUDIO_PROP_FORMAT_ONLY);
	return retval;
}
