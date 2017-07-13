#include "net_proto_outbound_data.h"
#include "../net_proto_socket.h"
#include "../net_proto_meta.h" // apply meta function
#include "../net_proto_request.h"
#include "../net_proto_peer.h"
#include "../net_proto.h"
#include "../../../settings.h"
#include "../../../id/id_api.h"

/*
  TODO: requests use a lot of constants, so try and move away from that
 */

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
			print("cannot establish connection to peer", P_UNABLE);
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
			if(first.at(i) == second.at(c)){
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

  All checks go into these two functions: ownership, ID mismatch, etc.

  A request that is filled is destroyed afterwards
 */

template<typename T>
static bool net_proto_valid_request_to_fill(T request){
	const id_t_ origin_peer_id =
		request->get_origin_peer_id();
	const id_t_ destination_peer_id =
		request->get_destination_peer_id();
	const bool origin_id_ok =
		origin_peer_id != net_proto::peer::get_self_as_peer();
	const bool destination_id_ok =
		destination_peer_id == net_proto::peer::get_self_as_peer();
	const bool not_owner =
		get_id_hash(request->id.get_id()) !=
		get_id_hash(production_priv_key_id);
	ASSERT(origin_peer_id != ID_BLANK_ID, P_ERR);
	ASSERT(destination_peer_id != ID_BLANK_ID, P_ERR);
	ASSERT(request->get_request_time() != 0, P_ERR);
	// P_V(not_owner, P_DEBUG);
	// P_V(origin_id_ok, P_DEBUG);
	// P_V(destination_id_ok, P_DEBUG);
	return not_owner && origin_id_ok && destination_id_ok;
}

template<typename T>
static bool net_proto_valid_request_to_send(
	T request,
	uint64_t frequency_micro_s){
	ASSERT(request != nullptr, P_ERR);
	const id_t_ origin_peer_id =
		request->get_origin_peer_id();
	const id_t_ destination_peer_id =
		request->get_destination_peer_id();
	const bool origin_id_ok =
		origin_peer_id == net_proto::peer::get_self_as_peer();
	const bool destination_id_ok =
		destination_peer_id != net_proto::peer::get_self_as_peer();
	const bool is_owner =
		get_id_hash(request->id.get_id()) ==
		get_id_hash(production_priv_key_id);
	const bool time_ok =
		get_time_microseconds()-request->get_broadcast_time_micro_s() > frequency_micro_s;
	ASSERT(origin_peer_id != ID_BLANK_ID, P_ERR);
	ASSERT(destination_peer_id != ID_BLANK_ID, P_ERR);
	ASSERT(request->get_request_time() != 0, P_ERR);
	return time_ok && is_owner && origin_id_ok && destination_id_ok;
}

static void net_proto_fill_type_requests(){
	std::vector<id_t_> net_proto_type_requests =
	 	id_api::cache::get(
			TYPE_NET_PROTO_TYPE_REQUEST_T);
	for(uint64_t i = 0;i < net_proto_type_requests.size();i++){
	 	net_proto_type_request_t *proto_type_request =
	 		PTR_DATA(net_proto_type_requests[i],
	 			 net_proto_type_request_t);
		CONTINUE_IF_NULL(proto_type_request, P_DEBUG);
		if(net_proto_valid_request_to_fill(proto_type_request)){
			print("filling a valid type request " + id_breakdown(net_proto_type_requests[i]) + "for type " + convert::type::from(proto_type_request->get_type()), P_DEBUG);
			try{
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
					}catch(...){
						print("couldn't send type request", P_WARN);
					}
				}
			}catch(...){}
			id_api::destroy(net_proto_type_requests[i]);
			proto_type_request = nullptr;
		}
	}
}


static void net_proto_fill_id_requests(){
	std::vector<id_t_> net_proto_id_requests =
	 	id_api::cache::get(TYPE_NET_PROTO_ID_REQUEST_T);
	for(uint64_t i = 0;i < net_proto_id_requests.size();i++){
	 	net_proto_id_request_t *proto_id_request =
	 		PTR_DATA(net_proto_id_requests[i],
	 			 net_proto_id_request_t);
	 	if(proto_id_request == nullptr){
	 		continue;
	 	}
		if(net_proto_valid_request_to_fill(proto_id_request)){
			print("filling ID request" + id_breakdown(net_proto_id_requests[i]), P_SPAM);
			try{
				const std::vector<id_t_> id_vector =
					proto_id_request->get_ids();
				const id_t_ origin_peer_id =
					proto_id_request->get_origin_peer_id();
				for(uint64_t c = 0;c < id_vector.size();c++){
					print("ID request contains " + id_breakdown(id_vector[i]), P_SPAM);
				}
				try{
					net_proto_send_logic(
						id_vector,
						origin_peer_id);
				}catch(...){}
			}catch(...){}
			id_api::destroy(net_proto_id_requests[i]);
			proto_id_request = nullptr;
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
			}catch(...){}
			if(proto_linked_list_request->get_curr_length() == 0){
				id_api::destroy(net_proto_linked_list_requests[i]);
			}
		}
	}
}

template <typename T>
void net_proto_handle_request_send(T request_ptr){
	if(request_ptr == nullptr){
		print("request_ptr is a nullptr", P_NOTE);
		return;
	}
	const id_t_ destination_peer_id =
		request_ptr->get_destination_peer_id();
	try{
		net_proto_send_logic(
			std::vector<id_t_>({request_ptr->id.get_id()}),
			destination_peer_id);
		request_ptr->update_broadcast_time_micro_s();
		print("sent request to peer" + net_proto::peer::get_breakdown(
			      destination_peer_id), P_DEBUG);
	}catch(...){
		print("couldn't send request to peer, probably no available socket", P_DEBUG);
		net_proto::socket::connect(
			destination_peer_id,
			1);
	}
}

/*
  TODO: Find a better way to bind individual type timeouts with the function
  calls (local constant variable that's referenced in the function should work
  fine)
 */

#define NET_PROTO_HANDLE_REQUEST_HANDLER(type)			\
	std::vector<id_t_> request_vector =			\
		id_api::cache::get(				\
			#type);					\
	for(uint64_t i = 0;i < request_vector.size();i++){	\
		type *ptr = PTR_DATA(request_vector[i], type);	\
		if(net_proto_valid_request_to_send(		\
			   ptr,					\
			   timeout_micro_s)){			\
			net_proto_handle_request_send(		\
				ptr);				\
			ptr->update_broadcast_time_micro_s();	\
		}						\
	}							

static void net_proto_send_type_requests(){
	const uint64_t timeout_micro_s =
		settings::get_setting_unsigned_def(
			"net_proto_type_request_timeout_micro_s",
			15*1000*1000);
	NET_PROTO_HANDLE_REQUEST_HANDLER(net_proto_type_request_t);
}

static void net_proto_send_id_requests(){
	const uint64_t timeout_micro_s =
		settings::get_setting_unsigned_def(
			"net_proto_id_request_timeout_micro_s",
			15*1000*1000);
	NET_PROTO_HANDLE_REQUEST_HANDLER(net_proto_id_request_t);
}

static void net_proto_send_linked_list_requests(){
	const uint64_t timeout_micro_s =
		settings::get_setting_unsigned_def(
			"net_proto_linked_list_request_timeout_micro_s",
			15*1000*1000);
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
