#include "console.h"

console_t::console_t() : id(this, __FUNCTION__){
	id.nonet_all_data();
	id.noexp_all_data();
}

console_t::~console_t(){
}

void console_t::set_socket_id(id_t_ socket_id_){
	socket_id = socket_id_;
}

id_t_ console_t::get_socket_id(){
	return socket_id;
}

void console_t::print_socket(std::string str){
	net_socket_t *socket =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	try{
		socket->send(str);
	}catch(...){
		print("couldn't send to console socket", P_WARN);
	}
}

DEC_CMD(output_table_clear){
	output_table.clear();
}

void console_t::execute(std::vector<std::string> cmd_vector){
	bool ran = false;
	try{
		LIST_CMD(output_table_clear);
		LIST_CMD(reg_set_const);
		LIST_CMD(reg_set_table);
		LIST_CMD(reg_copy);
		LIST_CMD(reg_clear);
		LIST_CMD(reg_left_shift);
		LIST_CMD(reg_right_shift);
		LIST_CMD(print_output_table);
		LIST_CMD(print_reg);
		LIST_CMD(exit);
		LIST_CMD(id_api_get_type_cache);
		// TV
		LIST_CMD(tv_window_list_active_streams);
		LIST_CMD(tv_window_clear_active_streams);
		LIST_CMD(tv_window_add_active_stream);
		LIST_CMD(tv_window_del_active_stream);
		LIST_CMD(tv_window_create);
		LIST_CMD(tv_window_set_to_time);
		LIST_CMD(tv_window_set_timestamp_offset);
		LIST_CMD(tv_window_set_channel_id);
		LIST_CMD(tv_channel_create);
		LIST_CMD(tv_channel_get_stream_list);
		LIST_CMD(tv_audio_load_wav);
	}catch(std::exception &e){
		print_socket("command failed:" + (std::string)e.what() + "\n");
		return;
	}
	if(ran == false){
		print_socket("invalid command\n");
	}
	print_socket("command succeeded\n");
}

DEC_CMD(exit){
	net_socket_t *socket =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket == nullptr){
		print("socket is a nullptr", P_WARN);
	}
	socket->disconnect();
}

void console_t::run(){
	net_socket_t *socket =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket == nullptr){
		print("no valid socket", P_ERR);
	}
	std::vector<uint8_t> inbound_data =
		socket->recv_all_buffer();
	for(uint64_t i = 0;i < inbound_data.size();i++){
		P_V((int16_t)inbound_data[i], P_DEBUG);
	}
	working_input.insert(
		working_input.end(),
		inbound_data.begin(),
		inbound_data.end());
	auto pos = working_input.end();
	while((pos = std::find(working_input.begin(),
			       working_input.end(),
			       (uint8_t)'\n')) != working_input.end()){
		std::string input_full(working_input.begin(),
				       pos);
		std::vector<std::string> cmd_vector;
		for(uint64_t i = 0;i < input_full.size();i++){
			if(input_full[i] == ' ' || input_full[i] == '\r'){
				cmd_vector.push_back(
					std::string(
						&(input_full[0]),
						&(input_full[i])));
				input_full.erase(
					input_full.begin(),
					input_full.begin()+i+1);
				i = 0;
				if(cmd_vector.size() > 0){
					P_V_S(cmd_vector[cmd_vector.size()-1], P_SPAM);
				}
			}
		}
		working_input.erase(
			working_input.begin(),
			pos);
		P_V(working_input.size(), P_DEBUG);
		if(working_input.size() <= 2){
			working_input.clear();
		}
		execute(cmd_vector);
	}
}

/*
  Manages sockets
 */

static id_t_ console_socket_id = ID_BLANK_ID;

void console_init(){
	net_socket_t *inbound_socket =
		new net_socket_t;
	inbound_socket->set_net_ip("", 59000, NET_IP_VER_4);
	inbound_socket->connect();
	console_socket_id =
		inbound_socket->id.get_id();
}

static void console_accept_connections(){
	net_socket_t *socket =
		PTR_DATA(console_socket_id,
			 net_socket_t);
	if(socket == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	TCPsocket tmp_socket =
		socket->get_tcp_socket();
	TCPsocket new_socket =
		nullptr;
	if((new_socket = SDLNet_TCP_Accept(tmp_socket)) != nullptr){
		print("accepting a new console connection", P_NOTE);
		console_t *console_new =
			new console_t;
		net_socket_t *console_socket =
			new net_socket_t;
		console_socket->set_tcp_socket(new_socket);
		console_socket->send("BasicTV console\n");
		console_new->set_socket_id(console_socket->id.get_id());
	}
}

static bool console_is_alive(console_t *console){
	net_socket_t *socket =
		PTR_DATA(console->get_socket_id(),
			 net_socket_t);
	if(socket == nullptr){
		return false;
	}
	return socket->is_alive();
}

void console_loop(){
	console_accept_connections();
	std::vector<id_t_> console_entries =
		id_api::cache::get(
			"console_t");
	for(uint64_t i = 0;i < console_entries.size();i++){
		console_t *console_tmp =
			PTR_DATA(console_entries[i],
				 console_t);
		if(console_tmp == nullptr){
			continue;
		}
		if(console_is_alive(console_tmp)){
			console_tmp->run();
		}else{
			id_api::destroy(console_tmp->id.get_id());
		}
	}
}

void console_close(){
}

std::vector<std::vector<std::string> > console_generate_generic_id_table(std::vector<id_t_> id_vector){
	std::vector<std::vector<std::string> > retval;
	retval.push_back({"ID", "Type", "Prev Linked List", "Next Linked List", "Mod. Inc."});
	for(uint64_t i = 0;i < id_vector.size();i++){
		std::vector<std::string> tmp_vector;
		tmp_vector.push_back(convert::array::id::to_hex(id_vector[i]));
		data_id_t *id_ptr =
			PTR_ID(id_vector[i], );
		if(id_ptr != nullptr){
			tmp_vector.push_back(
				id_ptr->get_type());
			tmp_vector.push_back(
				convert::array::id::to_hex(
					id_ptr->get_prev_linked_list()));
			tmp_vector.push_back(
				convert::array::id::to_hex(
					id_ptr->get_next_linked_list()));
			tmp_vector.push_back(
				std::to_string(id_ptr->get_mod_inc()));
		}else{
			tmp_vector.insert(
				tmp_vector.end(), 4, "NULL");
		}
		retval.push_back(tmp_vector);
	}
	return retval;
}