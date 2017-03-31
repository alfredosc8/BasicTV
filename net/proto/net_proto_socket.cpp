#include "../../main.h"
#include "../../id/id_api.h"
#include "../../util.h"
#include "../net.h"
#include "../net_socket.h"
#include "net_proto_meta.h"
#include "net_proto_socket.h"
#include "../../id/id_api.h"
#include "../../encrypt/encrypt.h"
#include "../../escape.h"

/*
  New transport model:
  TCP and UDP will both use encapsulation of data provided by escape.cpp
  to handle the payload and the metadata. Each chunk of data should look
  something like this:
  [STANDARD DATA #1] [PAYLOAD #1] [STANDARD DATA #2] [PAYLOAD #2]

  No spaces nor numbers are used, they are purely visual.

  If a payload is received without a corresponding standard data block, it
  is disregarded (TODO: is this good behavior?). 
 */

net_proto_socket_t::net_proto_socket_t() : id(this, __FUNCTION__){
	net_proto_standard_data_t std_data_;
	std_data_.ver_major = VERSION_MAJOR;
	std_data_.ver_minor = VERSION_MINOR;
	std_data_.ver_patch = VERSION_REVISION;
	std_data_.macros = 0;
	std_data_.unused = 0;
	std_data_.peer_id = net_proto::peer::get_self_as_peer();
	std_data =
		net_proto_write_packet_metadata(
			std_data_);
}

net_proto_socket_t::~net_proto_socket_t(){
}

void net_proto_socket_t::update_working_buffer(){
	net_socket_t *socket_ptr =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	std::vector<uint8_t> buffer =
		socket_ptr->recv_all_buffer();
	if(buffer.size() != 0){
		P_V(buffer.size(), P_SPAM);
		last_recv_micro_s = get_time_microseconds();
		working_buffer.insert(
			working_buffer.end(),
			buffer.begin(),
			buffer.end());
	}
}

void net_proto_socket_t::update_block_buffer(){
	std::pair<std::vector<std::vector<uint8_t> >, std::vector<uint8_t> > block_data =
		unescape_all_vectors(
			working_buffer,
			NET_PROTO_ESCAPE);
	/*
	  This has no out of sync protection. 

	  TODO: Possibly do sanity checks on length of first segment and slide
	  all buffers left by one to fix?
	 */
	if(block_buffer.size() == 0){
		block_buffer.push_back(
			std::make_pair(
				std::vector<uint8_t>(),
				std::vector<uint8_t>()));
	}
	for(uint64_t i = 0;i < block_data.first.size();i++){
		if(block_buffer[block_buffer.size()-1].first.size() == 0){
			block_buffer[block_buffer.size()-1].first =
				block_data.first[i];
		}else if(block_buffer[block_buffer.size()-1].second.size() == 0){
			block_buffer[block_buffer.size()-1].second =
				block_data.first[i];
		}else{
			block_buffer.push_back(
				std::make_pair(
					block_data.first[i],
					std::vector<uint8_t>({})));
		}
	}
	working_buffer = block_data.second;
}

void net_proto_socket_t::load_blocks_as_data(){
	/*
	  Check for encryption
	  If it doesn't need encryption, remove the request and load it
	  	TODO: allow accepting data without a request from routine requests
	  If it does need encryption, fetch the net_peer_t, fetch the pub key,
	  and work on it from there (not implemented yet)
	  
	 */
}

void net_proto_socket_t::send_id(id_t_ id_){
	data_id_t *id_tmp =
		PTR_ID(id_, );
	if(id_tmp == nullptr){
		print("id to send is a nullptr", P_ERR);
	}
	std::vector<uint8_t> payload =
		id_tmp->export_data(ID_DATA_NOEXP);
	if(payload.size() == 0){
		print("exported size of ID is zero, not sending", P_NOTE);
		return;
	}
	net_socket_t *socket_ptr =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	socket_ptr->send(escape_vector(std_data, NET_PROTO_ESCAPE));
	socket_ptr->send(escape_vector(payload, NET_PROTO_ESCAPE));
	P_V(std_data.size(), P_SPAM);
	P_V(payload.size(), P_SPAM);
}

void net_proto_socket_t::send_id_vector(std::vector<id_t_> id_vector){
	for(uint64_t i = 0;i < id_vector.size();i++){
		send_id(id_vector[i]);
		// TODO: should optimize this somehow...
	}
}

void net_proto_socket_t::update(){
	update_working_buffer();
	update_block_buffer();
}
