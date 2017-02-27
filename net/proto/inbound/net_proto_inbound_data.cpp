#include "net_proto_inbound_data.h"
// meta has apply and unapply functions for DEV_CTRL_1
#include "../net_proto_meta.h"
#include "../net_proto_socket.h"
#include "../net_proto_request.h"
#include "../../../util.h"
#include "../../../id/id_api.h"
#include "../net_proto_api.h"

// static std::vector<uint8_t> net_proto_read_struct_segment(uint8_t *data,
// 							  uint64_t data_size){
// 	// size is the rest of data, not the size of the read data
// 	net_proto_standard_size_t size = 0;
// 	net_proto_standard_ver_t ver_major = 0;
// 	net_proto_standard_ver_t ver_minor = 0;
// 	net_proto_standard_ver_t ver_patch = 0;
// 	net_proto_standard_macros_t macros = 0;
// 	net_proto_standard_unused_t unused = 0;
// 	net_proto_read_packet_metadata(data,
// 				       data_size,
// 				       &size,
// 				       &ver_major,
// 				       &ver_minor,
// 				       &ver_patch,
// 				       &macros,
// 				       &unused);
// 	if(unused != 0){
// 		print("received a packet that utilized currently unused space, you might be running an old version", P_WARN);
// 	}
// 	if(data_size < size){
// 		// not enough data has been read yet
// 		return {};
// 	}
// 	return std::vector<uint8_t>(data, data+size);
// }

// /*
//   Reads all readable data from packet buffer, and returns a vector of the
//   RAW strings from the socket (needs to be iterated over with the unapply
//   function)

//   This code hasn't been tested at all, and should be cleaned up a lot before
//   actual use takes place

//   All of the struct segments should be ran through a decoder to remove the
//   extra DEV_CTRL_1 entries. The extra characters that are written should be
//   taken into account when computing the payload length.

//   TODO: offset the work of importing to the net_socket_t. Assuming all
//   sockets created follow this rule of only sending this sort of information,
//   that would make everything nicer (I can't think of any other case right now)
//  */

// #define EFFECTIVE_LENGTH() (buffer.size()-(i+1)-NET_PROTO_META_LENGTH)

// static std::vector<std::vector<uint8_t> > net_proto_get_struct_segments(net_socket_t *socket){
// 	std::vector<std::vector<uint8_t> > retval;
// 	std::vector<uint8_t> buffer =
// 		socket->recv(
// 			-socket->get_backwards_buffer_size(),
// 			NET_SOCKET_RECV_NO_HANG);
// 	uint64_t i = 0;
// 	// we don't know how nested it is
// 	for(;i < buffer.size();i++){
// 		if(buffer[i] != NET_PROTO_DEV_CTRL_1){
// 			break;
// 		}
// 	}
// 	for(;i < buffer.size();i++){
// 		try{
// 			if(buffer.at(i-1) != NET_PROTO_DEV_CTRL_1 &&
// 			   buffer.at(i+0) == NET_PROTO_DEV_CTRL_1 &&
// 			   buffer.at(i+1) != NET_PROTO_DEV_CTRL_1){
// 				net_proto_standard_size_t payload_size = 0;
// 				net_proto_read_packet_metadata(&(buffer[i]),
// 							       buffer.size()-i,
// 							       &payload_size,
// 							       nullptr,
// 							       nullptr,
// 							       nullptr,
// 							       nullptr,
// 							       nullptr);
// 				if(EFFECTIVE_LENGTH() < payload_size){
// 					/*
// 					  Unless the read buffer becomes large
// 					  enough, I need to precisely measure
// 					  how much data should be read and only
// 					  read that much to prevent chopping off
// 					  additional metadata
// 					 */
// 					std::vector<uint8_t> new_data =
// 						socket->recv(
// 							payload_size-EFFECTIVE_LENGTH(),
// 							NET_SOCKET_RECV_NO_HANG);
// 					buffer.insert(
// 						buffer.end(),
// 						new_data.begin(),
// 						new_data.end());
// 				}
// 				if(payload_size <= EFFECTIVE_LENGTH()){
// 					retval.push_back(
// 						net_proto_read_struct_segment(
// 							&(buffer[i+1]),
// 							payload_size));
// 				}
// 				i += payload_size;
// 			}
// 		}catch(...){}
// 	}
// 	return retval;
// }

// /*
//   Fetches all incoming data and handle it
//  */

// void net_proto_loop_handle_inbound_data(){
// 	net_socket_t *tmp_socket = nullptr;
// 	std::vector<uint64_t> peer_id_list =
// 		id_api::cache::get("net_peer_t");
// 	for(uint64_t i = 0;i < peer_id_list.size();i++){
// 		net_peer_t *peer =
// 			PTR_DATA(peer_id_list[i], net_peer_t);
// 		for(uint64_t s = 0;s < NET_PROTO_MAX_SOCKET_PER_PEER;s++){
// 			tmp_socket =
// 				PTR_DATA(peer->get_socket_id(s),
// 					 net_socket_t);
// 			if(likely(tmp_socket == nullptr)){
// 				continue;
// 			}
// 			/*
// 			  Ideally, store all of this on a global std::vector
// 			  and create worker threads to parse through it with
// 			  locks.
// 			 */
// 			std::vector<std::vector<uint8_t> > segments =
// 				net_proto_get_struct_segments(tmp_socket);
// 			for(uint64_t seg = 0;seg < segments.size();seg++){
// 				id_api::array::add_data(
// 					net_proto_unapply_dev_ctrl(
// 						std::vector<uint8_t>(
// 							segments[seg].begin()+NET_PROTO_META_LENGTH,
// 							segments[seg].end()
// 							)
// 						)
// 					);
// 			}
// 		}
// 	}
// }


// // reads from all net_socket_t, for testing only
// void net_proto_loop_dummy_read(){
// 	std::vector<uint64_t> all_sockets =
// 		id_api::cache::get("net_socket_t");
// 	for(uint64_t i = 0;i < all_sockets.size();i++){
// 		net_socket_t *socket_ =
// 			PTR_DATA(all_sockets[i], net_socket_t);
// 		if(socket_ == nullptr){
// 			print("socket is nullptr", P_ERR);
// 			continue;
// 		}
// 		if(socket_->get_client_conn().first == ""){
// 			// inbound
// 			continue;
// 		}
// 		if(socket_->activity()){
// 			print("detected activity on a socket", P_SPAM);
// 			std::vector<uint8_t> incoming_data;
// 			while((incoming_data = socket_->recv(1, NET_SOCKET_RECV_NO_HANG)).size() != 0){
// 				P_V_E((uint64_t)incoming_data[0], P_NOTE);
// 			}
// 		}
// 	}
// }

// // reads what we have, returns proper status depending on it

// void net_proto_loop_handle_inbound_requests(){
	
// }

static void net_proto_dummy_read(){
}

void net_proto_handle_inbound_data(){
	std::vector<id_t_> proto_sockets =
		id_api::cache::get("net_proto_socket_t");
	for(uint64_t i = 0;i < proto_sockets.size();i++){
		net_proto_socket_t *proto_socket =
			PTR_DATA(proto_sockets[i],
				 net_proto_socket_t);
		if(proto_socket == nullptr){
			print("proto_socket is a nullptr", P_ERR);
		}
		proto_socket->update();
	}
}

/*
  malicious_to_send is technically redundant for security purposes, since
  any information with any security needs would have ID_DATA_NONET.
  However, this speeds things up, allows for a blacklist on peers who attempt
  this, and might prevent exporting one useless variable to eat up bandwidth.
  Also, security redundancy is pretty important.
 */

// for security reasons
static std::array<std::string, 4> malicious_to_send =
{"encrypt_priv_key_t",
 "net_proxy_t",
 "net_socket_t",
 "net_proto_socket_t"};

// for DoS/DDoS reasons
static std::array<std::string, 3> malicious_to_bulk_send =
{"tv_frame_video_t",
 "tv_frame_audio_t",
 "tv_frame_caption_t"};

// net_con_req_t are in a different file
// receives net_proto_request_t, sends data out

/*
  Move bulk exporting of IDs down to net_proto_socket_t for security reasons
 */

static std::vector<uint8_t> net_proto_export_id_vector(std::vector<id_t_> ids){
	std::vector<uint8_t> retval;
	for(uint64_t i = 0;i < ids.size();i++){
		data_id_t *tmp_id =
			PTR_ID(ids[i], );
		CONTINUE_IF_NULL(tmp_id);
		const std::vector<uint8_t> export_data =
			tmp_id->export_data(ID_DATA_NONET);
		if(export_data.size() != 0){
			retval.insert(
				retval.end(),
				export_data.begin(),
				export_data.end());
		}
	}
	return retval;
}

static void net_proto_handle_inbound_type_request(id_t_ request_id){
	net_proto_type_request_t *request =
		PTR_DATA(request_id,
			 net_proto_type_request_t);
	if(request == nullptr){
		print("request is a nullptr", P_NOTE);
		return;
	}
	/*
	  TODO: Pull net_proto_peer_t ID from request, search all sockets
	  to find a valid socket the data can be sent on, and send 
	  everything down the one.
	  
	  After I know that works, then start optimizing away by registering
	  sockets with net_proto_peer_t and using that instead and other smalls
	 */
	const id_t_ peer_id = request->get_peer_id();
	net_proto_socket_t *proto_socket_ptr = nullptr;
	std::vector<id_t_> proto_socket_vector =
		id_api::cache::get("net_proto_socket_t");
	for(uint64_t i = 0;i < proto_socket_vector.size();i++){
		net_proto_socket_t *proto_socket =
			PTR_DATA(proto_socket_vector[i],
				 net_proto_socket_t);
		CONTINUE_IF_NULL(proto_socket);
		if(proto_socket->get_peer_id() != peer_id &&
		   proto_socket->is_alive()){
			proto_socket_ptr =
				PTR_DATA(proto_socket_vector[i],
					 net_proto_socket_t);
			if(proto_socket_ptr != nullptr){
				break;
			}
		}
	}
	if(proto_socket_ptr != nullptr){
		proto_socket_ptr->send_id_vector(
			request->get_ids());
	}else{
		print("couldn't find a valid socket for peer request filling", P_NOTE);
	}
}

static void net_proto_handle_all_inbound_type_requests(){
	std::vector<id_t_> type_request_vector =
		id_api::cache::get("net_proto_type_request_t");
	for(uint64_t i = 0;i < type_request_vector.size();i++){
		net_proto_type_request_t *type_request =
			PTR_DATA(type_request_vector[i],
				 net_proto_type_request_t);
		if(type_request == nullptr){
			print("type request is a nullptr", P_DEBUG);
			continue;
		}
		if(type_request->get_peer_id() ==
		   net_proto::peer::get_self_as_peer()){
			continue;
		}
		net_proto_handle_inbound_type_request(
			type_request_vector[i]);
	}
}

static void net_proto_handle_all_inbound_standard_requests(){
}

/*
  Remember the information about the request state is handled inside
  of the data type, so don't delete it if it still has information
  I can fill (but can't right now).
 */

static void net_proto_handle_all_inbound_linked_list_requests(){
}

void net_proto_handle_inbound_requests(){
	net_proto_handle_all_inbound_type_requests();
	net_proto_handle_all_inbound_standard_requests();
	net_proto_handle_all_inbound_linked_list_requests();
}
