#include "../../main.h"
#include "../../id/id_api.h"
#include "../../util.h"
#include "../net.h"
#include "../net_socket.h"
#include "net_proto_meta.h"
#include "net_proto_socket.h"
#include "../../id/id_api.h"
#include "../../encrypt/encrypt.h"

net_proto_socket_t::net_proto_socket_t() : id(this, __FUNCTION__){
}

net_proto_socket_t::~net_proto_socket_t(){}

/*
  bare_* takes care of encryption, decryption, and verification of information

  Should put all information into a second local buffer for reading structs
 */

void net_proto_socket_t::bare_send(std::vector<uint8_t> data){
	net_proto_peer_t *proto_peer =
		PTR_DATA(peer_id,
			 net_proto_peer_t);
	if(proto_peer == nullptr){
		print("proto_peer is a nullptr", P_ERR);
	}
	net_socket_t *socket_ptr =
		PTR_DATA(get_socket_id(),
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("socket is a nullptr", P_ERR);
	}
	try{
		net_proto_standard_data_t std_data;
		std_data.peer_id = net_proto::peer::get_self_as_peer();
		std_data.ver_major = VERSION_MAJOR;
		std_data.ver_minor = VERSION_MINOR;
		std_data.ver_patch = VERSION_REVISION;
		const id_t_ pub_key_id =
			encrypt_api::search::pub_key_from_hash(
				get_id_hash(
					proto_peer->id.get_id()));
		std::vector<uint8_t> payload;
		if(get_flags() & NET_PROTO_SOCKET_NO_ENCRYPT){
			payload = data;
			print("sending data unencrypted", P_WARN);
		}else{
			payload =
				encrypt_api::encrypt(
					data,
					pub_key_id);
			std_data.macros |= NET_STANDARD_ENCRYPT_PACKET;
		}
		std_data.size = payload.size();
		std::vector<uint8_t> data_to_send =
			net_proto_write_packet_metadata(std_data);
		data_to_send.insert(
			data_to_send.end(),
			payload.begin(),
			payload.end());
		socket_ptr->send(data_to_send);
	}catch(...){
		print("couldn't send data", P_ERR);
	}
			
}

uint8_t net_proto_socket_t::get_flags(){
	return flags;
}

void net_proto_socket_t::set_flags(uint8_t flags_){
	flags = flags_;
}

void net_proto_socket_t::bare_recv(){
	last_update_time_micro_s = get_time_microseconds();
	net_socket_t *socket_ptr =
		PTR_DATA_FAST(
			socket_id,
			net_socket_t);
	if(socket_ptr == nullptr){
		print("socket for net_proto_socket_t is a nullptr", P_WARN);
		return;
	}
	std::vector<uint8_t> socket_buf =
		socket_ptr->recv_all_buffer();
	working_buffer.insert(
		working_buffer.end(),
		socket_buf.begin(),
		socket_buf.end());
	// is only responsible for updating the bare recv vector
	/*
	  TODO: don't copy the information over, but just return a pointer
	  or something (of course make it thread safe tho).
	 */
}

void net_proto_socket_t::set_socket_id(id_t_ socket_id_){
	socket_id = socket_id_;
	working_buffer.clear();
	if(buffer.size() != 0){
		print("valid buffers still exist, should have fetched the structs before setting new socket_id", P_WARN);
	}
	buffer.clear();
}

id_t_ net_proto_socket_t::get_socket_id(){
	return socket_id;
}

void net_proto_socket_t::set_peer_id(id_t_ peer_id_){
	peer_id = peer_id_;
}

id_t_ net_proto_socket_t::get_peer_id(){
	return peer_id;
}

std::vector<std::vector<uint8_t> > net_proto_socket_t::get_buffer(){
	std::vector<std::vector<uint8_t> > retval;
	retval = buffer;
	buffer.clear();
	return retval;
}

void net_proto_socket_t::send_id(id_t_ id_){
	data_id_t *ptr_id =
		PTR_ID(id_, );
	if(ptr_id == nullptr){
		print("can't send non-existent ID", P_NOTE);
		return;
	}
	std::string type = ptr_id->get_type();
	/*
	  The "malicious" flag isn't the only protection. NONET flags in the
	  data should be tripped when anything serious is being handled (RSA
	  private keys). That doesn't inherently mean nothing is sent though,
	  and this makes it not fill the request at all.

	  There are currently three checks for malicious information:
	  1. Constructor has NONET
	  2. Fulfilling net_proto_request_t checks it with DDoS vectors
	  3. Right here (only for security)
	 */
	if(type == "encrypt_priv_key_t"){
		print("malicious request, not filling", P_WARN);
	}else{
		net_socket_t *socket =
			PTR_DATA(socket_id,
				 net_socket_t);
		if(socket == nullptr){
			print("socket is invalid, can't send", P_WARN);
			return;
		}
		std::vector<uint8_t> exported_data =
			ptr_id->export_data(ID_DATA_NONET);
		// encryption status of encrypt_pub_key_t and similar types
		// should be handled inside of the export function
		try{
			socket->send(exported_data);
		}catch(...){
			print("cannot send ID across socket", P_WARN);
		}
	}
}

void net_proto_socket_t::send_id_vector(std::vector<id_t_> id_vector){
	for(uint64_t i = 0;i < id_vector.size();i++){
		send_id(id_vector[i]);
	}
}

// isn't used outside of this file (should be in meta though).

static std::pair<net_proto_standard_data_t, std::vector<uint8_t> >
net_proto_socket_read_struct_segment(std::vector<uint8_t> working_buffer){
	// size is the rest of data, not the size of the read data
	std::pair<net_proto_standard_data_t, std::vector<uint8_t> > retval;
	net_proto_read_packet_metadata(working_buffer.data(),
				       working_buffer.size(),
				       &retval.first);
	/*
	  I don't see a need to have a sanity check with this
	 */
	if(working_buffer.size() >= retval.first.size){
		uint8_t *start = working_buffer.data()+NET_PROTO_STANDARD_DATA_LENGTH;
		uint8_t *end = working_buffer.data()+NET_PROTO_STANDARD_DATA_LENGTH+retval.first.size;
		retval.second =
			std::vector<uint8_t>(start, end);
	}
	return retval;
}

void net_proto_socket_t::read_and_parse(){
	net_socket_t *socket_ptr =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("socket_ptr is a nullptr", P_ERR);
	}
	bare_recv();
	if(working_buffer.size() != 0){
		std::pair<net_proto_standard_data_t, std::vector<uint8_t> > net_final;
		try{
			/*
			  TODO: If the first character is a DEV_CTRL_1, then work from
			  that. If it isn't (for some strange reason), then keep
			  deleting information until the first standalone DEV_CTRL_1 is
			  found.

			  It might be beneficial to add a "junk" byte before the
			  DEV_CTRL_1 to prove that it is a standalone, versus assuming
			  that we are at the start. The junk byte can be anything.
			  However, since TCP is very predictible, I think only malicious
			  clients will make this fall out of sync (safely)
			*/
			net_final =
				net_proto_socket_read_struct_segment(
					working_buffer);
		}catch(...){
			print("unrecognized metadata for packet", P_WARN);
			return;
		}
		if(net_final.first.peer_id != ID_BLANK_ID){
			peer_id = net_final.first.peer_id;
		}
		if(net_final.first.macros & NET_STANDARD_ENCRYPT_PACKET){
			net_final.second =
				encrypt_api::decrypt(
				        net_final.second,
					encrypt_api::search::pub_key_from_hash(
						get_id_hash(peer_id)));
		}
		if(net_final.second.size() > 0){
			// net_final.second size is the raw size from the socket, so it
			// includes all extra DEV_CTRL_1s, so it can be used directly to
			// clean up working_buffer
			const uint64_t actual_size =
				net_final.second.size()+NET_PROTO_STANDARD_DATA_LENGTH+1;
			// final 1 is for the first DEV_CTRL_1
			buffer.push_back(net_final.second);
			working_buffer.erase(
				working_buffer.begin(),
				working_buffer.begin()+actual_size);
			// fixes the double DEV_CTRL_1
			for(uint64_t i = 0;i < net_final.second.size()-1;i++){
				if(net_final.second[i] == NET_PROTO_DEV_CTRL_1){
					if(net_final.second[i+1] == NET_PROTO_DEV_CTRL_1){
						net_final.second.erase(net_final.second.begin()+i);
						for(;i < net_final.second.size();i++){
							if(net_final.second[i] != NET_PROTO_DEV_CTRL_1){
								break;
							}
						}
					}else{
						print("it seems like the size and the next packet metadata overlap, this shouldn't happen", P_ERR);
					}
				}
			}
		} // doesn't trip when the data isn't finished receiving
	}
}

/*
  TODO: 
  CHECK ENCRYPT_PUB_KEY_T FOR CORRECTNESS HERE
  NOW
 */

void net_proto_socket_t::process_buffer(){
	for(uint64_t i = 0;i < buffer.size();i++){
		id_t_ tmp_id = ID_BLANK_ID;
		if(buffer[i].size() < sizeof(id_t_)){
			print("supposedly complete buffer entry is too short for ID", P_WARN);
			continue;
		}
		memcpy(&(tmp_id[0]), buffer[i].data(), sizeof(id_t_));
		try{
			net_proto::request::del_id(tmp_id);
		}catch(...){
			print("received information not requested from a socket", P_WARN);
		}
	}
	buffer.clear();
}

void net_proto_socket_t::update(){
	read_and_parse();
	process_buffer();
}

uint64_t net_proto_socket_t::get_last_update_micro_s(){
	return last_update_time_micro_s;
}

bool net_proto_socket_t::is_alive(){
	net_socket_t *socket =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket == nullptr){
		return false;
	}
	return socket->is_alive();
}
