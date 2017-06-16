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

  tv_audio_raw_queue_t stores the raw samples that need to be played,
  as well as the actual time to play them (incorporating the window's
  timestamp offset)

  The SDL audio callback function just goes through that list, 
  finds all of the chunks of audio to play next, asserts there is
  only one, and appends that to the data to play.

  Multiple streams can be a problem, since with only one stream, we know
  that the information we have that is next in line (via the timestamp) is
  what should be queried up next, but creating another type via SDL's userdata
  field for a seperate datatype to help with this would be needed and useful.
 */

struct tv_audio_raw_queue_t{
public:
	std::vector<uint8_t> raw_samples;
	uint32_t sampling_freq = 0;
	uint8_t bit_depth = 0;
	uint8_t channel_count = 0;

	uint64_t start_time_micro_s = 0;
	uint64_t end_time_micro_s = 0;

	uint32_t window_active_stream_entry = 0;
	uint32_t item_active_stream_entry = 0;

	id_t_ frame_id = ID_BLANK_ID;
};

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

static std::vector<tv_audio_raw_queue_t> audio_queue;

static std::vector<uint8_t> get_standard_sine_wave_form(){
	std::vector<uint8_t> retval;
	for(uint64_t i = 0;i < 48000*10;i++){
		uint16_t tmp =
			(uint16_t)((long double)(sin(1000 * (2 * 3.1415) * i / 48000))*65535);
		retval.push_back((uint8_t)(tmp&0xFF));
		retval.push_back((uint8_t)((tmp>>8)&0xFF));
	}
	return retval;
}

static void tv_audio_load_samples_queue(){
	uint64_t audio_to_queue = 0;
	for(uint64_t i = 0;i < audio_queue.size();i++){
		if(audio_queue[i].start_time_micro_s > get_time_microseconds() &&
		   audio_queue[i].start_time_micro_s < audio_queue[audio_to_queue].start_time_micro_s){
			audio_to_queue = i;
		}
	}
	if(audio_queue.size() != 0){
		SDL_QueueAudio(
			audio_device_id,
			audio_queue[audio_to_queue].raw_samples.data(),
			audio_queue[audio_to_queue].raw_samples.size());
		audio_queue.erase(
			audio_queue.begin()+audio_to_queue);
	}
}

/*
  We can assume that frame_audio_id is from the output of 
  tv_frame_scroll_to_time
 */

static void tv_audio_add_id_to_audio_queue(id_t_ window_id,
					   id_t_ frame_audio_id,
					   uint64_t forward_buffer_micro_s){
	std::vector<id_t_> id_vector;
	tv_window_t *window_ptr =
		PTR_DATA(window_id,
			 tv_window_t);
	tv_frame_audio_t *frame_audio_ptr =
		PTR_DATA(frame_audio_id,
			 tv_frame_audio_t);
	const uint64_t time_micro_s =
		get_time_microseconds();
	while(frame_audio_ptr != nullptr &&
	      frame_audio_ptr->get_end_time_micro_s()-time_micro_s <= forward_buffer_micro_s){
		std::pair<std::vector<id_t_>, std::vector<id_t_> > linked_list =
			frame_audio_ptr->id.get_linked_list();
		id_vector.push_back(
			frame_audio_ptr->id.get_id());
		if(linked_list.second.size() > 0){
			frame_audio_ptr =
				PTR_DATA(linked_list.second[0],
					 tv_frame_audio_t);
		}else{
			frame_audio_ptr = nullptr;
		}
	}
	for(uint64_t i = 0;i < audio_queue.size();i++){
		for(uint64_t c = 0;c < id_vector.size();c++){
			if(audio_queue[i].frame_id == id_vector[c]){
				id_vector.erase(
					id_vector.begin()+c);
			}
		}
	}
	for(uint64_t i = 0;i < id_vector.size();i++){
		tv_audio_raw_queue_t queue;
		queue.raw_samples =
			transcode::audio::frames::to_raw(
				std::vector<id_t_>({id_vector[i]}),
				&queue.sampling_freq,
				&queue.bit_depth,
				&queue.channel_count);
		switch(have.format){
		case AUDIO_U16:
			print("audio is fine as it is, no conversion needed", P_NOTE);
			break;
		case AUDIO_S16:
			print("performing unsigned to signed conversion", P_NOTE);
			queue.raw_samples =
				transcode::audio::raw::signed_to_unsigned(
					queue.raw_samples,
					queue.bit_depth);
			break;
		default:
			print("unrecognized audio format, this is bad", P_CRIT);
		}
		tv_frame_audio_t *frame_audio_ptr_ =
			PTR_DATA(id_vector[i],
				 tv_frame_audio_t);
		CONTINUE_ON_NULL(frame_audio_ptr, P_WARN);
		queue.start_time_micro_s =
			frame_audio_ptr_->get_start_time_micro_s()+window_ptr->get_timestamp_offset();
		queue.end_time_micro_s =
			frame_audio_ptr_->get_end_time_micro_s()+window_ptr->get_timestamp_offset();
		queue.frame_id =
			id_vector[i];
		audio_queue.push_back(
			queue);
	}
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
			nullptr;
		desired.userdata =
			nullptr;
		audio_device_id =
			SDL_OpenAudioDevice(
				nullptr,
				0,
				&desired,
				&have,
				SDL_AUDIO_ALLOW_ANY_CHANGE);
		P_V(desired.freq, P_WARN);
		P_V(desired.format, P_WARN);
		P_V(desired.channels, P_WARN);
		P_V(desired.samples, P_WARN);
		P_V(have.freq, P_WARN);
		P_V(have.format, P_WARN);
		P_V(have.channels, P_WARN);
		P_V(have.samples, P_WARN);
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
		tv_audio_load_samples_queue();
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
