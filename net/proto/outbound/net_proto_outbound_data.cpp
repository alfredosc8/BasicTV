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
	net_proto_peer_t *proto_peer_ptr =
		PTR_DATA(net_proto_peer_id,
			 net_proto_peer_t);
	if(proto_peer_ptr == nullptr){
		if(net_proto_peer_id == ID_BLANK_ID){
			print("network peer ID is intentionally blank, this is probably my fault, sorry...", P_WARN);
		}
		print("can't send request to an invalid network peer", P_ERR);
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
		proto_socket->send_id(id_vector[i]);
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
		const id_t_ target_peer_id =
			proto_type_request->get_sender_peer_id();
	 	if(target_peer_id == net_proto::peer::get_self_as_peer()){
			print("request is not meant for me", P_SPAM);
			continue;
		} // check if we are sending it to ourselves
		const id_t_ origin_peer_id =
			proto_type_request->get_receiver_peer_id();
		if(origin_peer_id != net_proto::peer::get_self_as_peer()){
			print("request originates from me", P_SPAM);
			continue;
		}
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
			      "some sort of response for not finding it)", P_SPAM);
		}else{
			try{
				net_proto_send_logic(
					real_payload,
					target_peer_id);
				id_api::destroy(net_proto_type_requests[i]);
				proto_type_request = nullptr;
			}catch(...){
				print("couldn't send type request", P_ERR);
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
		const id_t_ peer_id = proto_id_request->get_sender_peer_id();
	 	if(peer_id == net_proto::peer::get_self_as_peer()){
			print("id request has me sending a request to myself, weird", P_WARN);
		}
		const std::vector<id_t_> id_vector =
			proto_id_request->get_ids();
		try{
			net_proto_send_logic(
				id_vector, peer_id);
			id_api::destroy(net_proto_id_requests[i]);
		}catch(...){}
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
		const id_t_ peer_id = proto_linked_list_request->get_sender_peer_id();
	 	if(peer_id != net_proto::peer::get_self_as_peer()){
			const id_t_ curr_id =
				proto_linked_list_request->get_curr_id();
			try{
				if(PTR_ID(curr_id, ) != nullptr){
					proto_linked_list_request->increase_id();
				}
				net_proto_send_logic({curr_id}, peer_id);
				id_api::destroy(net_proto_linked_list_requests[i]);
			}catch(...){}
	 	}
	}
}

/*
  TODO: better implement networking stats before this is written

  TODO: actually write this stuff

  TODO: check for best route for data
 */

static bool net_proto_send_id_to_peer(
	id_t_ payload_id,
	id_t_ peer_id){
	bool sent = false;
	std::vector<id_t_> all_sockets =
		id_api::cache::get(
			TYPE_NET_PROTO_SOCKET_T);
	P_V(all_sockets.size(), P_VAR);
	for(uint64_t i = 0;i < all_sockets.size();i++){
		try{
			net_proto_socket_t *proto_socket_ptr =
				PTR_DATA(all_sockets[i],
					 net_proto_socket_t);
			if(proto_socket_ptr == nullptr){
				print("proto_socket_ptr is a nullptr", P_NOTE);
				continue;
			}	
			if(proto_socket_ptr->get_peer_id() == peer_id){
				print("peer socket already exists, sending over first found", P_NOTE);
				proto_socket_ptr->send_id(
					payload_id);
				sent = true;
				break;
			}
		}catch(...){} // search for another socket I guess...
	}
	if(sent == false){
		print("no valid proto socket found, requesting new socket for peer", P_NOTE);
		net_proto::socket::connect(
			peer_id, 1); // one socket currently
		/*
		  Connections aren't made until later on in the code, so just
		  forget about sending any data this iteration, not to mention
		  latencies and holepunching jargon.
		 */
	}
	return sent;
}

/*
  General rule of thumb is the code can be as tacky as it can be SO LONG AS
  all of the tacky code can be represented on the screen, in one file, at one
  time.
 */

template <typename T>
void net_proto_handle_request(T* request_ptr){
	if(request_ptr == nullptr){
		print("request_ptr is a nullptr", P_NOTE);
		return;
	}
	/*
	  Until I can hammer out formal responses and the sort, let's assume
	  that 100 percent of the requests are processed and no reply means
	  they don't have it
	 */
	if(//request_ptr->get_request_time() == 0 &&
	   net_proto_send_id_to_peer(
		   request_ptr->id.get_id(),
		   request_ptr->get_sender_peer_id()) == false){
		request_ptr->update_request_time();
	}else{
		print("couldn't send request to peer, probably no available socket", P_SPAM);
	}

}

#define NET_PROTO_HANDLE_REQUEST_HANDLER(type)	\
	std::vector<id_t_> request_vector =	\
		id_api::cache::get(		\
			#type);					\
	for(uint64_t i = 0;i < request_vector.size();i++){	\
		net_proto_handle_request(			\
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
