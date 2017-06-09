#include "console.h"
#include "../tv/tv.h"
#include "../tv/tv_channel.h"
#include "../tv/tv_item.h"
#include "../tv/tv_audio.h"
#include "../tv/tv_frame_audio.h"
#include "../tv/tv_window.h"
#include "../tv/transcode/tv_transcode.h"
#include "../util.h"

#include "../file.h"

/*
  tv_window_t handlers

  TODO: I changed tv_channel_t internally. Instead of just commenting out
  the offending blocks and confusing me later, I should redefine these
  commands so they more accurately reflect the internals (create commands
  directly for tv_item_t and binding commands for tv_channel_t)
 */

DEC_CMD(tv_window_set_channel_id){
	id_t_ tv_window_id =
		convert::array::id::from_hex(
			registers[0]);
	id_t_ tv_item_id =
		convert::array::id::from_hex(
			registers[1]);
	tv_window_t *window =
		PTR_DATA(tv_window_id,
			 tv_window_t);
	if(window == nullptr){
		print("window is a nullptr", P_ERR);
	}
	window->set_item_id(tv_item_id);
}

DEC_CMD(tv_window_list_active_streams){
	id_t_ tv_window_id =
		convert::array::id::from_hex(
			registers[0]);
	tv_window_t *window =
		PTR_DATA(tv_window_id,
			 tv_window_t);
	if(window == nullptr){
		print("window is a nullptr", P_ERR);
	}
	output_table = 
		console_generate_generic_id_table(
			window->get_active_streams());
	ADD_COLUMN_TO_TABLE(
		output_table, 0, get_item_id, convert::array::id::to_hex, tv_window_t);
}

DEC_CMD(tv_window_clear_active_streams){
}

DEC_CMD(tv_window_add_active_stream){
}

DEC_CMD(tv_window_del_active_stream){
}

DEC_CMD(tv_window_create){
	tv_window_t *window =
		new tv_window_t;
	output_table =
		console_generate_generic_id_table(
			std::vector<id_t_>({window->id.get_id()}));
}

DEC_CMD(tv_window_set_to_time){
	tv_window_t *window =
		PTR_DATA(
			convert::array::id::from_hex(registers[0]),
			tv_window_t);
	if(window == nullptr){
		print("window is a nullptr", P_ERR);
	}
	uint64_t time = std::stoull(registers[1]);
	window->set_timestamp_offset(time-get_time_microseconds());
}

DEC_CMD(tv_window_set_timestamp_offset){
	tv_window_t *window =
		PTR_DATA(
			convert::array::id::from_hex(registers[0]),
			tv_window_t);
	if(window == nullptr){
		print("window is a nullptr", P_ERR);
	}
	uint64_t time = std::stoull(registers[1]);
	window->set_timestamp_offset(time);
}

/*
  tv_channel_t handlers
 */

DEC_CMD(tv_channel_create){
	tv_channel_t *channel =
		new tv_channel_t;
	output_table =
		console_generate_generic_id_table(
			std::vector<id_t_>({channel->id.get_id()}));
}

/*
  tv_item_t handlers
 */

DEC_CMD(tv_item_create){
	output_table =
		console_generate_generic_id_table(
	{(new tv_item_t)->id.get_id()});
}

DEC_CMD(tv_item_get_stream_list){
	tv_item_t *item =
	 	PTR_DATA(convert::array::id::from_hex(registers[0]),
	 		 tv_item_t);
	if(item == nullptr){
		print("item is a nullptr", P_ERR);
	}
	print("this just fetches the first ID in each frame list for simplicity", P_NOTE);
	// I wouldn't mind changing it over when I get helper functions
	
	std::vector<std::vector<id_t_> > frame_set =
		item->get_frame_id_vector();
	std::vector<id_t_> fixed_id_vector;
	for(uint64_t i = 0;i < frame_set.size();i++){
		if(frame_set[i].size() == 0){
			print("empty frame set, skipping", P_NOTE);
			continue;
		}
		if(frame_set[i].size() > 1){
			print("truncating possibly useful data", P_NOTE);
		}
		fixed_id_vector.push_back(frame_set[i][0]);
	}
	// console_generate_generic_id_table only works on one-dimensional
	// data, and since tv_item_t is the only known major exception to
	// this (as it pertains to getting output to the table), i'm not too
	// worried about getting a helper function. Maybe for something more
	// mission critical though...
	output_table =
		console_generate_generic_id_table(
			fixed_id_vector);
}

DEC_CMD(tv_audio_load_wav){
	print("again, no more tv_audio_load_wav, do it right this time", P_CRIT);
	// id_t_ channel_id =
	// 	convert::array::id::from_hex(registers.at(0));
	// uint64_t start_time_micro_s =
	// 	std::stoull(registers.at(1));
	// std::string file = registers.at(2);
	// ::tv_audio_load_wav(
	// 	channel_id,
	// 	start_time_micro_s,
	// 	file);			
}

/*
  These test commands are self contained systems, so they create
  and use their own variables. The program should be smart enough
  to correctly handle multiple channels and windows by this point
  (and if it doesn't that should be fixed).

  The two parameters this takes is the offset (in microseconds) that the
  broadcast will start at, and the path to the file (absolute or from
  the directory of BasicTV, not the directory of where the telnet client
  is being ran, obviously)
 */

static std::vector<id_t_> console_tv_test_load_opus(std::string file){
	std::vector<uint8_t> raw_samples;
	int32_t ogg_opus_error;
	OggOpusFile *opus_file =
		op_open_file(
			file.c_str(),
			&ogg_opus_error);
	if(opus_file == nullptr){
		print("couldn't open the OGG Opus file, error code " + std::to_string(ogg_opus_error), P_ERR);
	}
	opus_int16 pcm[5760*2];
	int samples_read = 0;
	while((samples_read =
	       op_read(opus_file,
		       &(pcm[0]),
		       5760*2,
		       nullptr)) > 0){
		raw_samples.insert(
			raw_samples.end(),
			(uint8_t*)(&(pcm[0])),
			(uint8_t*)(&(pcm[0])+samples_read));
	}

	op_free(opus_file);
	opus_file = nullptr;

	// Intermediate (raw to codec)
	tv_audio_prop_t opus_audio_prop;
	opus_audio_prop.set_flags(
		TV_AUDIO_PROP_FORMAT_ONLY);
	opus_audio_prop.set_format(
		TV_AUDIO_FORMAT_OPUS);

	// Final frame output
	tv_audio_prop_t frame_audio_prop;
	frame_audio_prop.set_flags(
		TV_AUDIO_PROP_FORMAT_ONLY);
	frame_audio_prop.set_format(
		TV_AUDIO_FORMAT_OPUS);

	// standard output properties for Opus to raw samples
	const uint32_t sampling_freq =
		48000;
	const uint8_t bit_depth =
		16;
	const uint8_t channel_count =
		1;
	
	/*
	  to_frames with the same format SHOULD repacketize each individual
	  frame, one at a time, until everything is finished, and do no
	  conversions whatsoever if the outputs can be valid as the inputs
	  (i.e. no specified output sampling freq, bit depth, or special encoder
	  jargon)

	  Right now it just decodes and encodes everything, which is OK for
	  small loads, but becomes unreasonable very quickly
	 */

	std::vector<std::vector<uint8_t> > packetized_codec_data =
		transcode::audio::raw::to_codec(
			raw_samples,
			sampling_freq,
			bit_depth,
			channel_count,
			&opus_audio_prop);
	if(packetized_codec_data.size() == 0){
		HANG();
		print("packetized_codec_data is empty", P_ERR);
	}
	std::vector<id_t_> retval =
		transcode::audio::codec::to_frames(
			packetized_codec_data,
			&opus_audio_prop,
			&frame_audio_prop);
	const uint64_t packet_duration =
		(raw_samples.size()*(uint64_t)sampling_freq*((uint64_t)bit_depth/(uint64_t)8)/(uint64_t)channel_count)/(uint64_t)packetized_codec_data.size();
	for(uint64_t i = 0;i < retval.size();i++){
		tv_frame_audio_t *frame_audio =
			PTR_DATA(retval[i],
				 tv_frame_audio_t);
		if(frame_audio == nullptr){
			print("frame_audio is a nullptr", P_ERR);
		}
		frame_audio->set_standard(
			get_time_microseconds()+(i*packet_duration),
			packet_duration,
			i);
	}
	if(retval.size() == 0){
		HANG();
		print("frame vector is empty", P_ERR);
	}
	return retval;
}

DEC_CMD(tv_test_audio){
	const uint64_t start_time_micro_s =
		get_time_microseconds()+std::stoull(registers.at(0));
	const std::string file =
		registers.at(1);
	tv_window_t *window = nullptr;
	std::vector<id_t_> all_windows =
		id_api::cache::get(
			"tv_window_t");
	if(all_windows.size() > 0){
		window =
			PTR_DATA(all_windows.at(0),
				 tv_window_t);
		if(window == nullptr){
			print_socket("false flag raised by cache get, creating new window\n");
			window = new tv_window_t;
		}
		print_socket("using pre-existing window with the ID " + convert::array::id::to_hex(all_windows.at(0)) + "\n");
	}else{
		window = new tv_window_t;
	}
	tv_channel_t *channel =
		new tv_channel_t;
	channel->id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_NEVER,
		ID_DATA_EXPORT_RULE_NEVER,
		ID_DATA_PEER_RULE_NEVER);
	channel->set_description(
		convert::string::to_bytes("BasicTV Audio Test"));
	tv_item_t *item =
		new tv_item_t;
	std::vector<id_t_> all_frame_audios =
		console_tv_test_load_opus(
			file);
	item->add_frame_id(
		all_frame_audios);
	P_V(start_time_micro_s, P_VAR);
	for(uint64_t i = 0;i < all_frame_audios.size();i++){
		ID_MAKE_TMP(all_frame_audios[i]);
	}
	// add the item
	window->set_item_id(item->id.get_id());
	const std::vector<std::vector<id_t_> > frame_set =
	 	item->get_frame_id_vector();
	if(frame_set.size() == 0){
	 	print_socket("couldn't load WAV information");
		HANG();
	}else if(frame_set.size() > 1){
	 	print_socket("more streams than anticipated\n");
		HANG();
	}else{
	 	window->add_active_stream_id(
	 		all_frame_audios.at(0));
	}
}

DEC_CMD(tv_test_menu){
	print_socket("not implemented yet\n");
}

DEC_CMD(tv_test_card){
	print_socket("not implemented yet\n");
}
