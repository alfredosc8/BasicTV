#include "tv_frame_audio.h"
#include "tv_audio.h"
#include "tv_window.h"
#include "tv_channel.h"
#include "tv_item.h"
#include "tv.h"
#include "../convert.h"
#include "../settings.h"

#include "transcode/tv_transcode.h"

static uint32_t output_sampling_rate = 0;
static uint8_t output_bit_depth = 0;
static uint32_t output_chunk_size = 0;

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

static uint32_t tv_audio_sdl_format_from_depth(uint8_t bit_depth){
	switch(bit_depth){
	case 24:
		print("24-bit sound output isn't supported yet, falling back to 16-bit", P_WARN);
	case 16:
		print("using unsigned 16-bit system byte order", P_NOTE);
		return AUDIO_U16LSB;
	case 8:
		print("using unsigned 8-bit system byte order (are you sure you want to do this?)", P_WARN);
		return AUDIO_U8; // no SYS in 8-bit, is it assumed?
	default:
		print("unknown bit depth for SDL conversion, using 16-bit", P_WARN);
		return AUDIO_U16LSB;
	}
}

void tv_audio_init(){
	if(settings::get_setting("audio") == "true"){
		SDL_Init(SDL_INIT_AUDIO);
		try{
			output_sampling_rate =
				std::stoi(
					settings::get_setting(
						"snd_output_sampling_rate"));
			output_bit_depth =
				std::stoi(
					settings::get_setting(
						"snd_output_bit_depth"));
			output_chunk_size =
				std::stoi(
					settings::get_setting(
						"snd_output_chunk_size"));
		}catch(...){
			// no big problem, these values are sane (maybe not chunk size)
			print("cannot read sound settings from file, setting default", P_WARN);
			output_sampling_rate = TV_AUDIO_DEFAULT_SAMPLING_RATE;
			output_bit_depth = TV_AUDIO_DEFAULT_BIT_DEPTH;
			output_chunk_size = TV_AUDIO_DEFAULT_CHUNK_SIZE;
		}
		if(Mix_OpenAudio(output_sampling_rate,
				 tv_audio_sdl_format_from_depth(
					 output_bit_depth),
				 1,
				 output_chunk_size) < 0){
			P_V(output_sampling_rate, P_WARN);
			P_V(output_bit_depth, P_WARN);
			P_V(output_chunk_size, P_WARN);
			print("cannot open audio:" + (std::string)(Mix_GetError()), P_ERR);
		}
	}else{
		print("audio has been disabled in the settings", P_NOTE);
	}
}

/*
  First is the ID for the tv_frame_audio_t type
  Second is the timestamp (in microseconds) that the playback ends (this is
  computed from the end_time_micro_s() function in the tv_frame_audio_t type
  with the timestamp_offset of tv_window_t
  Third is the Mix_Chunk that SDL_mixer uses

  To prevent blips on older machines, I should either move towards SDL_Audio and
  feeding PCM information directly, or just add a start time with the end time,
  so there isn't a need for OTF conversions between sampling frequencies. I'm
  not worried about the little blips for now, and I haven't seen any problems
  as long as I don't use Valgrind
 */

std::vector<std::tuple<id_t_, uint64_t, Mix_Chunk*> > audio_data;

static void tv_audio_clean_audio_data(){
	const uint64_t cur_time_micro_s =
		get_time_microseconds();
	for(uint64_t i = 0;i < audio_data.size();i++){
		const uint64_t end_time_micro_s =
			std::get<1>(audio_data[i]);
		// Make sure that SDL is done with it (should use locks)
		if(end_time_micro_s < cur_time_micro_s){
			print("destroying old audio data", P_SPAM);
			Mix_Chunk **ptr =
				&std::get<2>(audio_data[i]);
			// should always not be null
			if(*ptr != nullptr){
				Mix_FreeChunk(*ptr);
				*ptr = nullptr;
				ptr = nullptr;
			}
			audio_data.erase(audio_data.begin()+i);
		}
	}
}

/*
  All frame_audios entries need to be started NOW, so don't offset anything
 */

static std::vector<id_t_> tv_audio_remove_redundant_ids(std::vector<id_t_> frame_audios){
	for(uint64_t i = 0;i < frame_audios.size();i++){
		for(uint64_t c = 0;c < audio_data.size();c++){
			if(frame_audios[i] == std::get<0>(audio_data[c])){
				frame_audios.erase(
					frame_audios.begin()+i);
				i--;
				c = 0;
			}
		}
	}
	return frame_audios;
}

static void tv_audio_add_frame_audios(std::vector<id_t_> frame_audios){
	const uint64_t cur_timestamp_microseconds =
		get_time_microseconds();
	for(uint64_t i = 0;i < frame_audios.size();i++){
		tv_frame_audio_t *audio =
			PTR_DATA(frame_audios[i],
				 tv_frame_audio_t);
		P_V_S(convert::type::from(get_id_type(frame_audios[i])), P_WARN);
		if(audio == nullptr){
			print("audio is a nullptr", P_WARN);
			continue;
		}
		const uint64_t ttl_micro_s =
			audio->get_ttl_micro_s();
		tv_audio_prop_t wav_audio_prop;
		wav_audio_prop.set_flags(
			TV_AUDIO_PROP_FORMAT_ONLY);
		wav_audio_prop.set_format(
			TV_AUDIO_FORMAT_WAV);
		std::vector<uint8_t> wav_data =
			convert::vector::collapse_2d_vector(
				transcode::audio::frames::to_codec(
					{frame_audios[i]},
					&wav_audio_prop));
		P_V(wav_data.size(), P_VAR);
		P_V(wav_audio_prop.get_sampling_freq(), P_VAR);
		P_V(wav_audio_prop.get_bit_rate(), P_VAR);
		P_V(wav_audio_prop.get_bit_depth(), P_VAR);
		P_V(wav_audio_prop.get_channel_count(), P_VAR);
		SDL_RWops *rw =
			SDL_RWFromMem(
				wav_data.data(),
				wav_data.size());
		if(rw == nullptr){
			print("can't allocate SDL_RWops:"+(std::string)SDL_GetError(), P_ERR);
		}
		Mix_Chunk *chunk =
			Mix_LoadWAV_RW(rw, 1);
		if(chunk == nullptr){
			print("can't load Mix_Chunk:"+(std::string)Mix_GetError(), P_ERR);
		}
		std::tuple<id_t_,
			   uint64_t,
			   Mix_Chunk*> new_data =
			std::make_tuple(
				frame_audios[i],
				cur_timestamp_microseconds+ttl_micro_s,
				chunk);
		print("appending tv_frame_audio_t tuple to audio_data", P_SPAM);
		audio_data.push_back(new_data);
		print("playing audio", P_SPAM);
		Mix_PlayChannel(-1, chunk, 0);
	}
}

static std::vector<id_t_> tv_audio_get_current_frame_audios(){
	std::vector<id_t_> windows =
		id_api::cache::get(
			"tv_window_t");
	const uint64_t cur_timestamp_micro_s =
		get_time_microseconds();
	std::vector<id_t_> frame_audios;
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
					curr_id);
			}
		}
	}
	return frame_audios;
}

static void print_id_vector(std::vector<id_t_> id_vector){
	for(uint64_t i = 0;i < id_vector.size();i++){
		P_V_S(convert::array::id::to_hex(id_vector[i]), P_NOTE);
	}
}

void tv_audio_loop(){
	if(settings::get_setting("audio") == "true"){
		Mix_Volume(-1, MIX_MAX_VOLUME); // -1 sets all channels
		tv_audio_clean_audio_data();
		std::vector<id_t_> current_id_set =
			tv_audio_get_current_frame_audios();
		current_id_set =
			tv_audio_remove_redundant_ids(
				current_id_set);
		if(current_id_set.size() != 0){
			print("adding a new audio stream chunk", P_NOTE);
		}
		tv_audio_add_frame_audios(
			current_id_set);
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
