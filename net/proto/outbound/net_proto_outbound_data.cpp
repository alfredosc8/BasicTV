#include "net_proto_outbound_data.h"
#include "../net_proto_socket.h"
#include "../net_proto_meta.h" // apply meta function
#include "../net_proto_request.h"
#include "../net_proto_peer.h"
#include "../net_proto.h"
#include "../../../settings.h"
#include "../../../id/id_api.h"

static void net_proto_send_logic(std::vector<id_t_> id_vector,
				 id_t_ net_proto_peer_id){
	if(net_proto_peer_id == net_proto::peer::get_self_as_peer()){
		print("refusing to fill a request to send data to myself", P_ERR);
	}
	net_proto_peer_t *proto_peer_ptr =
		PTR_DATA(net_proto_peer_id,
			 net_proto_peer_t);
	if(proto_peer_ptr == nullptr){
		if(net_proto_peer_id == ID_BLANK_ID){
			print("network peer ID is intentionally blank, this is probably my fault, sorry...", P_WARN);
		}
		print("can't send request to an invalid network peer", P_UNABLE);
	}
	// TODO: effectively send across multiple?
	id_t_ optimal_proto_socket_id =
		net_proto::socket::optimal_proto_socket_of_peer(
			net_proto_peer_id);
	net_proto_socket_t *proto_socket =
		PTR_DATA(optimal_proto_socket_id,
			 net_proto_socket_t);
	if(proto_socket == nullptr){
		net_proto::socket::connect(
			net_proto_peer_id, 1);
		optimal_proto_socket_id =
			net_proto::socket::optimal_proto_socket_of_peer(
				net_proto_peer_id);
		proto_socket =
			PTR_DATA(optimal_proto_socket_id,
				 net_proto_socket_t);
		if(proto_socket == nullptr){
			print("proto_socket is a nullptr", P_ERR);
		}
	}
	for(uint64_t i = 0;i < id_vector.size();i++){
		data_id_t *id_ptr =
			PTR_ID(id_vector[i], );
		if(id_ptr == nullptr){
			continue;
		}
		try{
			proto_socket->send_id(id_vector[i]);
		}catch(...){
			print("proto_socket is broken, deleting net_socket_t and net_proto_socket_t", P_WARN);
			id_api::destroy(proto_socket->get_socket_id());
			id_api::destroy(proto_socket->id.get_id());
			proto_socket = nullptr;
			break;
		}
	}
}

static std::vector<id_t_> remove_ids_from_vector(std::vector<id_t_> first,
						 std::vector<id_t_> second){
	for(uint64_t i = 0;i < first.size();i++){
		for(uint64_t c = 0;c < second.size();c++){
			if(first[i] == second[c]){
				first.erase(
					first.begin()+i);
				i--;
			}
		}
	}
	return first;
}

/*
  Fill request functions are different enough to make generics and abstractions
  too complicated.
 */

template<typename T>
static bool net_proto_valid_request_to_fill(T request){
	const id_t_ origin_peer_id =
		request->get_origin_peer_id();
	const id_t_ destination_peer_id =
		request->get_destination_peer_id();
	if(origin_peer_id == net_proto::peer::get_self_as_peer()){
		return false;
	}
	if(destination_peer_id != net_proto::peer::get_self_as_peer()){
		return false;
	}
	return true;
}

template<typename T>
static bool net_proto_valid_request_to_send(T request){
	const id_t_ origin_peer_id =
		request->get_origin_peer_id();
	const id_t_ destination_peer_id =
		request->get_destination_peer_id();
	if(origin_peer_id != net_proto::peer::get_self_as_peer()){
		return false;
	}
	if(destination_peer_id == net_proto::peer::get_self_as_peer()){
		return false;
	}
	return true;
}

static void net_proto_fill_type_requests(){
	std::vector<id_t_> net_proto_type_requests =
	 	id_api::cache::get("net_proto_type_request_t");
	for(uint64_t i = 0;i < net_proto_type_requests.size();i++){
	 	net_proto_type_request_t *proto_type_request =
	 		PTR_DATA(net_proto_type_requests[i],
	 			 net_proto_type_request_t);
	 	if(proto_type_request == nullptr){
	 		continue;
	 	}
		if(net_proto_valid_request_to_fill(proto_type_request)){
			print("filling a valid type request " + id_breakdown(net_proto_type_requests[i]) + "for type " + convert::type::from(proto_type_request->get_type()), P_DEBUG);
			const std::vector<id_t_> raw_id_vector =
				proto_type_request->get_ids();
			const std::vector<id_t_> type_vector =
				id_api::cache::get(
					proto_type_request->get_type());
			const std::vector<id_t_> real_payload =
				remove_ids_from_vector(
					type_vector,
					raw_id_vector);
			P_V_S(convert::type::from(proto_type_request->get_type()), P_VAR);
			P_V(type_vector.size(), P_VAR);
			P_V(raw_id_vector.size(), P_VAR);
			P_V(real_payload.size(), P_VAR);
			if(real_payload.size() == 0){
				print("we don't have any new data to send out, "
				      "not sending anything (should probably have "
				      "some sort of response for not finding it)", P_DEBUG);
			}else{
				try{
					net_proto_send_logic(
						real_payload,
						proto_type_request->get_origin_peer_id());
					id_api::destroy(net_proto_type_requests[i]);
					proto_type_request = nullptr;
				}catch(...){
					print("couldn't send type request", P_WARN);
				}
			}
		}
	}
}


static void net_proto_fill_id_requests(){
	std::vector<id_t_> net_proto_id_requests =
	 	id_api::cache::get("net_proto_id_request_t");
	for(uint64_t i = 0;i < net_proto_id_requests.size();i++){
	 	net_proto_id_request_t *proto_id_request =
	 		PTR_DATA(net_proto_id_requests[i],
	 			 net_proto_id_request_t);
	 	if(proto_id_request == nullptr){
	 		continue;
	 	}
		if(net_proto_valid_request_to_fill(proto_id_request)){
			const std::vector<id_t_> id_vector =
				proto_id_request->get_ids();
			try{
				net_proto_send_logic(
					id_vector,
					proto_id_request->get_origin_peer_id());
				id_api::destroy(net_proto_id_requests[i]);
			}catch(...){}
		}
	}
}

static void net_proto_fill_linked_list_requests(){
	std::vector<id_t_> net_proto_linked_list_requests =
	 	id_api::cache::get("net_proto_linked_list_request_t");
	for(uint64_t i = 0;i < net_proto_linked_list_requests.size();i++){
	 	net_proto_linked_list_request_t *proto_linked_list_request =
	 		PTR_DATA(net_proto_linked_list_requests[i],
	 			 net_proto_linked_list_request_t);
	 	if(proto_linked_list_request == nullptr){
	 		continue;
	 	}
		if(net_proto_valid_request_to_fill(proto_linked_list_request)){
			const id_t_ origin_id =
				proto_linked_list_request->get_origin_peer_id();
			const id_t_ curr_id =
				proto_linked_list_request->get_curr_id();
			try{
				if(PTR_ID(curr_id, ) != nullptr){
					proto_linked_list_request->increase_id();
				}
				net_proto_send_logic(
					std::vector<id_t_>({curr_id}),
					origin_id);
				if(proto_linked_list_request->get_curr_length() == 0){
					id_api::destroy(net_proto_linked_list_requests[i]);
				}
			}catch(...){}
		}
	}
}

/*
  General rule of thumb is the code can be as tacky as it can be SO LONG AS
  all of the tacky code can be represented on the screen, in one file, at one
  time.
 */

template <typename T>
void net_proto_handle_request_send(T request_ptr){
	if(request_ptr == nullptr){
		print("request_ptr is a nullptr", P_NOTE);
		return;
	}
	/*
	  Until I can hammer out formal responses and the sort, let's assume
	  that 100 percent of the requests are processed and no reply means
	  they don't have it
	 */
	const uint64_t request_micro_s = 
		settings::get_setting_unsigned_def(
			"net_proto_type_request_timeout_micro_s",
			60*1000*1000); // don't set too high
	if(get_time_microseconds()-request_ptr->get_request_time() > request_micro_s){
		print("type request has expired, deleting", P_NOTE);
		id_api::destroy(request_ptr->id.get_id());
		return;
	}
	const id_t_ destination_peer_id =
		request_ptr->get_destination_peer_id();
	if(net_proto_valid_request_to_send(request_ptr)){
		try{
			net_proto_send_logic(
				std::vector<id_t_>({request_ptr->id.get_id()}),
				destination_peer_id);
			request_ptr->update_request_time();
			print("sent request to peer" + net_proto::peer::get_breakdown(
				      destination_peer_id), P_DEBUG);
		}catch(...){
			print("couldn't send request to peer, probably no available socket", P_DEBUG);
			net_proto::socket::connect(
				destination_peer_id,
				1);
		}
	}
}

#define NET_PROTO_HANDLE_REQUEST_HANDLER(type)			\
	std::vector<id_t_> request_vector =			\
		id_api::cache::get(				\
			#type);					\
	P_V(request_vector.size(), P_NOTE);			\
	for(uint64_t i = 0;i < request_vector.size();i++){	\
		net_proto_handle_request_send(			\
			PTR_DATA(request_vector[i],		\
				 type));			\
	}							\
	
static void net_proto_send_type_requests(){
	NET_PROTO_HANDLE_REQUEST_HANDLER(net_proto_type_request_t);
}

static void net_proto_send_id_requests(){
	NET_PROTO_HANDLE_REQUEST_HANDLER(net_proto_id_request_t);
}

static void net_proto_send_linked_list_requests(){
	NET_PROTO_HANDLE_REQUEST_HANDLER(net_proto_linked_list_request_t);
}

void net_proto_handle_outbound_requests(){
	net_proto_fill_type_requests();
	net_proto_fill_id_requests();
	net_proto_fill_linked_list_requests();
	net_proto_send_type_requests();
	net_proto_send_id_requests();
	net_proto_send_linked_list_requests();
}
