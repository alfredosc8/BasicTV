#include "console.h"
#include "../tv/tv.h"
#include "../tv/tv_channel.h"
#include "../tv/tv_audio.h"
#include "../tv/tv_window.h"
#include "../util.h"

/*
  tv_window_t handlers
 */

DEC_CMD(tv_window_set_channel_id){
	id_t_ tv_window_id =
		convert::array::id::from_hex(
			registers[0]);
	id_t_ tv_channel_id =
		convert::array::id::from_hex(
			registers[1]);
	tv_window_t *window =
		PTR_DATA(tv_window_id,
			 tv_window_t);
	if(window == nullptr){
		print("window is a nullptr", P_ERR);
	}
	window->set_channel_id(tv_channel_id);
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
		output_table, 0, get_channel_id, convert::array::id::to_hex, tv_window_t);
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

DEC_CMD(tv_channel_get_stream_list){
	tv_channel_t *channel =
		PTR_DATA(convert::array::id::from_hex(registers[0]),
			 tv_channel_t);
	if(channel == nullptr){
		print("channel is a nullptr", P_ERR);
	}
	output_table =
		console_generate_generic_id_table(
			channel->get_stream_list());
}

DEC_CMD(tv_audio_load_wav){
	id_t_ channel_id =
		convert::array::id::from_hex(registers.at(0));
	uint64_t start_time_micro_s =
		std::stoull(registers.at(1));
	std::string file = registers.at(2);
	::tv_audio_load_wav(
		channel_id,
		start_time_micro_s,
		file);			
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

DEC_CMD(tv_test_audio){
	const uint64_t start_time_micro_s =
		get_time_microseconds()+std::stoull(registers.at(0));
	const std::string file =
		cmd_vector.at(1);
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
	window->set_channel_id(channel->id.get_id());
	::tv_audio_load_wav(
		channel->id.get_id(),
		start_time_micro_s,
		file);
	const std::vector<id_t_> stream_list =
		channel->get_stream_list();
	if(stream_list.size() == 0){
		print_socket("couldn't load WAV information into channel");
	}else if(stream_list.size() > 1){
		print_socket("more streams than anticipated\n");
	}else{
		window->add_active_stream_id(
			stream_list.at(0));
	}
}

DEC_CMD(tv_test_menu){
	print_socket("not implemented yet\n");
}

DEC_CMD(tv_test_card){
	print_socket("not implemented yet\n");
}
