#include "console.h"
#include "../tv/tv.h"
#include "../tv/tv_channel.h"
#include "../tv/tv_item.h"
#include "../tv/tv_audio.h"
#include "../tv/tv_frame_audio.h"
#include "../tv/tv_window.h"
#include "../util.h"

/*
  Somewhat hacky, but far better, interface for actual BasicTV functionality.
 */

std::string console_t::tv_manager_read_string(
	net_socket_t *console_inbound_socket){
	std::string retval;
	bool in_loop = true;
	while(in_loop){
		std::vector<uint8_t> tmp_vector =
			console_inbound_socket->recv(1, NET_SOCKET_RECV_NO_HANG);
		if(tmp_vector.size() != 0){
			if(tmp_vector[0] != '\r' && tmp_vector[0] != '\n'){
				retval += std::string(1, tmp_vector[0]);
			}else{
				if(retval.size() != 0){
					in_loop = false;
				}
			}
		}
	}
	console_inbound_socket->send(retval + "\n");
	return retval;
}

void console_t::tv_manager_load_item_to_channel(
	net_socket_t *console_inbound_socket){
	print_socket("Channel ID:");
	const std::string channel_id =
		tv_manager_read_string(
			console_inbound_socket);
	print_socket("Start Time Offset (microseconds):");
	const std::string start_time_micro_s_offset =
		tv_manager_read_string(
			console_inbound_socket);
	print_socket("File Path (WAV only):");
	const std::string file_path =
		tv_manager_read_string(
			console_inbound_socket);
	print_socket("interpreted ID as " +
		     convert::array::id::to_hex(
			     convert::array::id::from_hex(
				     channel_id)) + "\n");
	tv_channel_t *channel_ptr =
		PTR_DATA(convert::array::id::from_hex(channel_id),
			 tv_channel_t);
	if(channel_ptr == nullptr){
		print_socket("channel_ptr is a nullptr");
	}
	tv_item_t *new_item =
		new tv_item_t;
	try{
		::tv_audio_load_wav(
			new_item->id.get_id(),
			std::stoi(start_time_micro_s_offset)+get_time_microseconds(),
			file_path);
	}catch(...){
		print_socket("failed to load WAV file\n");
		print("failed to load WAV file", P_ERR);
	}
	new_item->set_tv_channel_id(
		convert::array::id::from_hex(channel_id));
	print_socket("added data properly");
}

void console_t::tv_manager_play_loaded_item(
	net_socket_t *console_inbound_socket){
	print_socket("Item ID:");
	const std::string item_id =
		tv_manager_read_string(
			console_inbound_socket);
	print_socket("Time Offset to Start (microseconds):");
	const std::string time_offset_micro_s_str =
		tv_manager_read_string(
			console_inbound_socket);
	tv_item_t *item_ptr =
		PTR_DATA(convert::array::id::from_hex(item_id),
			 tv_item_t);
	if(item_ptr == nullptr){
		print_socket("item_ptr is a nullptr");	
		print("item_ptr is a nullptr", P_ERR);
	}
	tv_channel_t *channel_ptr =
		PTR_DATA(item_ptr->get_tv_channel_id(),
			 tv_channel_t);
	// technically we don't need channel_ptr, but printing out metadata
	// is something i'd like to do real soon
	if(channel_ptr == nullptr){
		print_socket("channel_ptr is a nullptr\n");
		print("channel_ptr is a nullptr", P_ERR);
	}
	std::vector<id_t_> window_vector =
		id_api::cache::get(
			TYPE_TV_WINDOW_T);
	tv_window_t *window_ptr = nullptr;
	if(window_vector.size() == 0){
		window_ptr =
			new tv_window_t;
	}else{
		if(window_vector.size() > 1){
			print_socket("more than one window created, this is good, but we don't have full support yet\n");
		}
		window_ptr =
			PTR_DATA(window_vector[0],
				 tv_window_t);
		if(window_ptr == nullptr){
			window_ptr =
				new tv_window_t;
		}
	}
	// const uint64_t timestamp_offset =
	// 	item_ptr->get_start_time_micro_s()-get_time_microseconds()+std::stoi(time_offset_micro_s_str);
	// print_socket("interpreted timestamp offset as " + std::to_string(timestamp_offset) + "\n");
	window_ptr->set_timestamp_offset(0);
		// timestamp_offset);
	window_ptr->set_item_id(
		item_ptr->id.get_id());
	// TODO: should implement this a different way, perhaps have
	// dedicated identifiers for the different streams
	window_ptr->add_active_stream_id(
		item_ptr->get_frame_id_vector()[0][0]);
	print_socket("everything should be loaded nicely now, right?\n");
}

void console_t::tv_manager_print_options(){
	const std::string tmp =
		"(1) Load TV Item to Channel\n"
		"(2) Play Loaded TV Item\n"
		"(3) Change Channel in Window\n"
		"(4) List TV Channels and Items\n"
		"]";
	print_socket(tmp);
}

DEC_CMD(tv_manager){
	net_socket_t* console_inbound_socket =
		PTR_DATA(get_socket_id(),
			 net_socket_t);
	if(console_inbound_socket == nullptr){
		print_socket("console_inbound_socket is a nullptr");
		print("console_inbound_socket is a nullptr", P_ERR);
	}
	while(true){
		print_socket("BasicTV TV Manager\n");
		tv_manager_print_options();
		std::string read_string =
			tv_manager_read_string(
				console_inbound_socket);
		const uint64_t option =
			std::stoi(read_string);
		print_socket("interpreted input as " + std::to_string(option) + "\n");
		switch(option){
		case 1:
			tv_manager_load_item_to_channel(console_inbound_socket);
			break;
		case 2:
			tv_manager_play_loaded_item(console_inbound_socket);
			break;
		case 3:
		case 4:
		default:
			print_socket("invalid option\n");
			return;
		}
	}
}
