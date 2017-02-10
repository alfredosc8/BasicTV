#include "net_proto_outbound_data.h"
#include "../net_proto_socket.h"
#include "../net_proto_dev_ctrl.h" // apply dev_ctrl function
#include "../net_proto_meta.h" // apply meta function
#include "../net_proto_request.h"
#include "../net_proto_peer.h"
#include "../net_proto.h"
#include "../../../settings.h"
#include "../../../id/id_api.h"

/*
  It's about time I get something down about how to write this

  For every ID requested, I need a vector of net_proto_socket_t IDs and their
  probabilities (already defined in net_proto_request_t::update_probs)

  For every ID, I also need a list of sockets that had, in some form, a request
  sent down them, as well as any possible responses
 */

/*
  Get list of requested IDs with all sockets and probs
  Get list of socket IDs and IDs sent to them
  Re-send information to those sockets if within interval
  Send to one new node every retry interval
*/

// static void net_proto_send_requests(net_proto_request_t *request, uint64_t retry_interval_micro_s){
// 	request->update_probs();
// 	const uint64_t socket_count =
// 		settings::get_setting_unsigned_def(
// 			"net_proto_request_whitelist_socket_count",
// 			4);
// 	std::vector<id_t_> all_request_ids =
// 		request->get_ids();
// 	for(uint64_t i = 0;i < all_request_ids.size();i++){
// 		const id_t_ req_id =
// 			all_request_ids[i];
// 		std::vector<id_t_> socket_ids_ordered =
// 			request->get_prob_socket_ids_ordered(
// 				req_id);
// 		uint64_t new_sockets = 0;
// 		for(uint64_t c = 0;c < socket_ids_ordered.size() && new_sockets < socket_count;c++){
// 			const id_t_ socket_id =
// 				socket_ids_ordered[c];
// 			if(request->should_send_to_socket(
// 				   req_id,
// 				   socket_id,
// 				   retry_interval_micro_s)){
// 				request->send_to_socket(
// 					req_id,
// 					socket_id);
// 				new_sockets++;
// 			}
// 		}
// 	}
// }

// static void net_proto_request_logic(net_proto_request_t *request,
// 				    uint64_t retry_interval_micro_s){
// 	if(request->get_flags() & NET_REQUEST_BLACKLIST){
// 		if(convert::array::type::from(
// 			   request->get_type()) == ""){
// 			print("can't request a blacklist without a type", P_NOTE);
// 			return;
// 			// checking isn't needed, the vector would be empty
// 		}
// 		std::vector<id_t_> all_net_proto_sockets =
// 			id_api::cache::get(
// 				"net_proto_socket_t");
// 		std::random_shuffle(
// 			all_net_proto_sockets.begin(),
// 			all_net_proto_sockets.end());
// 		// not defined whatsoever
// 		uint64_t socket_count =
// 			settings::get_setting_unsigned_def(
// 				"net_proto_request_blacklist_socket_count",
// 				3);
// 		try{
// 			// TODO: define this as a setting
// 			for(uint64_t i = 0;i < socket_count;i++){
// 				net_proto_socket_t *tmp_proto_socket =
// 					PTR_DATA(all_net_proto_sockets[i],
// 						 net_proto_socket_t);
// 				if(tmp_proto_socket != nullptr){
// 					request->send_to_socket(
// 						ID_BLANK_ID,
// 						all_net_proto_sockets[i]);
// 				}
// 			}
// 		}catch(...){}
// 	}else{
// 		net_proto_send_requests(
// 			request,
// 			retry_interval_micro_s);
// 	}
// }

static void net_proto_send_logic(std::vector<id_t_> id_vector,
				 id_t_ net_proto_peer_id){
	net_proto_peer_t *proto_peer_ptr =
		PTR_DATA(net_proto_peer_id,
			 net_proto_peer_t);
	if(proto_peer_ptr == nullptr){
		print("can't send request to an invalid network peer", P_ERR);
		return;
	}
	// TODO: effectively send across multiple?
	id_t_ optimal_proto_socket_id =
		net_proto::socket::optimal_proto_socket_of_peer(
			net_proto_peer_id);
	net_proto_socket_t *proto_socket =
		PTR_DATA(optimal_proto_socket_id,
			 net_proto_socket_t);
	if(proto_socket == nullptr){
		print("connect this to connection call", P_CRIT);
	}
	for(uint64_t i = 0;i < id_vector.size();i++){
		data_id_t *id_ptr =
			PTR_ID(id_vector[i], );
		if(id_ptr == nullptr){
			continue;
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

static void net_proto_loop_handle_outbound_type_requests(){
	std::vector<id_t_> net_proto_type_requests =
	 	id_api::cache::get("net_proto_type_request_t");
	for(uint64_t i = 0;i < net_proto_type_requests.size();i++){
	 	net_proto_type_request_t *proto_type_request =
	 		PTR_DATA(net_proto_type_requests[i],
	 			 net_proto_type_request_t);
	 	if(proto_type_request == nullptr){
	 		continue;
	 	}
		const id_t_ peer_id = proto_type_request->get_peer_id();
	 	if(peer_id != net_proto::peer::get_self_as_peer()){
	 		net_proto_send_logic(
	 			remove_ids_from_vector(
					id_api::cache::get(
						proto_type_request->get_type()),
					proto_type_request->get_ids()),
	 			proto_type_request->get_peer_id());
	 	}
	}
}

static void net_proto_loop_handle_outbound_id_requests(){
}

static void net_proto_loop_handle_outbound_linked_list_requests(){
}


void net_proto_loop_handle_outbound_requests(){
	const uint64_t retry_interval_micro_s =
		settings::get_setting_unsigned_def(
			"net_proto_request_retry_interval_micro_s",
			1000*10);
}
