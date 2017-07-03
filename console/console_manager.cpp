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
	new_item->add_frame_id(
		console_tv_test_load_opus(file_path));
	new_item->set_tv_channel_id(
		convert::array::id::from_hex(channel_id));
	print_socket("added data properly");
	output_table =
		console_generate_generic_id_table(
			std::vector<id_t_>(
			{new_item->id.get_id(),
			 channel_ptr->id.get_id()}));
}

void console_t::tv_manager_play_loaded_item(
	net_socket_t *console_inbound_socket){
	print_socket("Item ID:");
	const std::string item_id =
		tv_manager_read_string(
			console_inbound_socket);
	print_socket("Offset from Live TV (0 for Live, - for Rewind, + for Forwards):");
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
	const uint64_t timestamp_offset =
		std::stoi(time_offset_micro_s_str);
	print_socket("interpreted timestamp offset as " + std::to_string(timestamp_offset) + "\n");
	window_ptr->set_timestamp_offset(
		timestamp_offset);
	window_ptr->set_item_id(
		item_ptr->id.get_id());
	window_ptr->add_active_stream_id(
		item_ptr->get_frame_id_vector()[0][0]);
	print_socket("everything should be loaded nicely now, right?\n");
}

void console_t::tv_manager_change_item_in_window(
	net_socket_t *console_inbound_socket){
	print_socket("Item ID:");
	const id_t_ item_id =
		convert::array::id::from_hex(
			tv_manager_read_string(
				console_inbound_socket));
	std::vector<id_t_> window_vector =
		id_api::cache::get(
			TYPE_TV_WINDOW_T);
	id_t_ window_id = ID_BLANK_ID;
	if(window_vector.size() == 1){
		print_socket("automatically using only created window\n");
		window_id = window_vector[0];
	}else{
		print_socket("Window ID:");
		window_id =
			convert::array::id::from_hex(
				tv_manager_read_string(
					console_inbound_socket));
	}
	tv_window_t *window_ptr =
		PTR_DATA(window_id,
			 tv_window_t);
	if(window_ptr == nullptr){
		print_socket("window_ptr is a nullptr\n");
		print("window_ptr is a nullptr", P_ERR);
	}
	if(PTR_DATA(item_id, tv_item_t) == nullptr){
		print_socket("item_ptr is a nullptr\n");
		print("item_ptr is a nullptr", P_WARN);
	}
	window_ptr->set_item_id(
		item_id);
}

void console_t::tv_manager_list_channels_and_items(){
	std::vector<id_t_> channel_vector =
		id_api::cache::get(
			TYPE_TV_CHANNEL_T);
	// uses output_table
	output_table.clear();
	for(uint64_t i = 0;i < channel_vector.size();i++){
		tv_channel_t *channel =
			PTR_DATA(channel_vector[i],
				 tv_channel_t);
		if(channel == nullptr){
			print_socket("NULL CHANNEL\n");
			continue;
		}
		std::vector<std::string> datum(
		{
			"ID: " + convert::array::id::to_hex(channel_vector[i]),
			"Desc: " + convert::string::from_bytes(channel->get_description()),
			"Name: " + convert::string::from_bytes(channel->get_name()),
			"Wallet Set ID: " + convert::array::id::to_hex(channel->get_wallet_set_id())
		});
		output_table.push_back(
			datum);
	}
}

void console_t::tv_manager_create_tv_channel(
	net_socket_t *console_inbound_socket){
	print_socket("Channel Name: ");
	const std::string name_str =
		tv_manager_read_string(
			console_inbound_socket);
	print_socket("Channel Description: ");
	const std::string desc_str =
		tv_manager_read_string(
			console_inbound_socket);
	print_socket("Channel Wallet Set ID (NULL for nothing): ");
	const std::string wallet_set_id_str =
		tv_manager_read_string(
			console_inbound_socket);
	tv_channel_t *channel_ptr = nullptr;
	try{
		channel_ptr = new tv_channel_t;
		channel_ptr->set_name(
			convert::string::to_bytes(
				name_str));
		channel_ptr->set_description(
			convert::string::to_bytes(
				desc_str));
		if(wallet_set_id_str != "NULL"){
			channel_ptr->set_wallet_set_id(
				convert::array::id::from_hex(
					wallet_set_id_str));
		}
	}catch(...){
		print_socket("unable to interpret input\n");
		if(channel_ptr != nullptr){
			id_api::destroy(channel_ptr->id.get_id());
			channel_ptr = nullptr;
		}
		return;
	}
	output_table =
		console_generate_generic_id_table(
			std::vector<id_t_>(
			{channel_ptr->id.get_id()}));
}

void console_t::tv_manager_print_options(){
	const std::string tmp =
		"(1) Load TV Item to Channel\n"
		"(2) Play Loaded TV Item\n"
		"(3) Change Item in Window\n"
		"(4) List TV Channels and Items\n"
		"(5) Create TV Channel\n"
		"(6) Exit TV Manager\n"
		"] ";
	print_socket(tmp);
}

DEC_CMD(tv_manager){
	net_socket_t* console_inbound_socket =
		PTR_DATA(get_socket_id(),
			 net_socket_t);
	if(console_inbound_socket == nullptr){
		print_socket("console_inbound_socket is a nullptr\n");
		print("console_inbound_socket is a nullptr", P_ERR);
	}
	bool tv_manager_loop = true;
	while(tv_manager_loop){
		print_socket("BasicTV TV Manager\n");
		tv_manager_print_options();
		std::string read_string =
			tv_manager_read_string(
				console_inbound_socket);
		const uint64_t option =
			std::stoi(read_string);
		print_socket("interpreted input as " + std::to_string(option) + "\n");
		output_table.clear();
		switch(option){
		case 1:
			tv_manager_load_item_to_channel(console_inbound_socket);
			break;
		case 2:
			tv_manager_play_loaded_item(console_inbound_socket);
			break;
		case 3:
			tv_manager_change_item_in_window(console_inbound_socket);
			break;
		case 4:
			tv_manager_list_channels_and_items();
			break;
		case 5:
			tv_manager_create_tv_channel(console_inbound_socket);
			break;
		case 6:
			print_socket("closing TV manager\n");
			tv_manager_loop = false;
		default:
			print_socket("invalid option\n");
			return;
		}
		print_output_table();
	}
}
