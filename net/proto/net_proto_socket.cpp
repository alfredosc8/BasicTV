#include "../../main.h"
#include "../../id/id_api.h"
#include "../../util.h"
#include "../net.h"
#include "../net_socket.h"
#include "net_proto_meta.h"
#include "net_proto_socket.h"
#include "../../id/id_api.h"

/*
  TODO: make a thread smart linked list creator for each
  of the IDs non-redundantly (using PTR_ID_FAST)
 */

net_proto_socket_t::net_proto_socket_t() : id(this, __FUNCTION__){
}

net_proto_socket_t::~net_proto_socket_t(){}

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
	  
	  TODO: possibly add a class where exporting doesn't make sense and
	  where exporting is malicious
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
		socket->send(exported_data);
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

void net_proto_socket_t::update(){
	net_socket_t *socket_ptr =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("socket_ptr is a nullptr", P_ERR);
	}
	std::vector<uint8_t> net_data =
		socket_ptr->recv_all_buffer();
	working_buffer.insert(
		working_buffer.end(),
		net_data.begin(),
		net_data.end()); // should always add one byte
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
	if(net_final.second.size() > 0){
		last_update_micro_s = get_time_microseconds();
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
	// TODO: find IDs for all new data and put it in the id_log
}

void net_proto_socket_t::add_id_to_log(id_t_ id_log_){
	id_log.push_back(
		std::make_pair(
			get_time_microseconds(),
			id_log_));
}

std::vector<std::pair<uint64_t, id_t_> > net_proto_socket_t::get_id_log(){
	return id_log;
}

uint64_t net_proto_socket_t::get_last_update_micro_s(){
	return last_update_micro_s;
}

void net_proto_socket_t::update_connection(){
	net_proto_peer_t *peer_ptr =
		PTR_DATA(peer_id,
			 net_proto_peer_t);
	if(peer_ptr == nullptr){
		print("protocol socket is not bound to a peer, cannot connect", P_ERR);
	}
	net_socket_t *socket_ptr =
		PTR_DATA(socket_id,
			 net_socket_t);
	if(socket_ptr == nullptr){
		print("no bound socket, creating new one", P_NOTE);
		// not actually that bad
		socket_ptr = new net_socket_t;
		socket_id = socket_ptr->id.get_id();
	}
	const bool current =
		socket_ptr->get_net_ip_str() == peer_ptr->get_net_ip_str() &&
		socket_ptr->get_net_port() == peer_ptr->get_net_port();
	if(!current || socket_ptr->is_alive()){
		socket_ptr->disconnect();
		socket_ptr->set_net_ip(
			peer_ptr->get_net_ip_str(),
			peer_ptr->get_net_port(),
			NET_IP_VER_4);
		socket_ptr->connect();
	}
}
