#include "net_proto_outbound_data.h"
#include "../net_proto_socket.h"
#include "../net_proto_dev_ctrl.h" // apply dev_ctrl function
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
		const id_t_ peer_id = proto_type_request->get_peer_id();
	 	if(peer_id != net_proto::peer::get_self_as_peer()){
			const std::vector<id_t_> raw_id_vector =
				proto_type_request->get_ids();
			const std::vector<id_t_> type_vector =
				id_api::cache::get(
					proto_type_request->get_type());
			try{
				net_proto_send_logic(
					remove_ids_from_vector(
						type_vector,
						raw_id_vector),
					peer_id);
				delete proto_type_request;
				proto_type_request = nullptr;
			}catch(...){}
	 	}else{
			// created locally, distribute out randomly
			// TODO: assign a peer to send out to in type itself
			net_proto_socket_t *proto_socket_ptr =
				PTR_DATA(
					net_proto::socket::optimal_proto_socket_of_peer(
						net_proto::peer::random_peer_id()),
					net_proto_socket_t);
			if(proto_socket_ptr == nullptr){
				print("socket is a nullptr", P_WARN);
				continue;
			}
			proto_socket_ptr->send_id(
				net_proto_type_requests[i]);
			delete proto_type_request;
			proto_type_request = nullptr;
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
		const id_t_ peer_id = proto_id_request->get_peer_id();
	 	if(peer_id != net_proto::peer::get_self_as_peer()){
			const std::vector<id_t_> id_vector =
				proto_id_request->get_ids();
			
			try{
				net_proto_send_logic(
					id_vector, peer_id);
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
		const id_t_ peer_id = proto_linked_list_request->get_peer_id();
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
 */

static void net_proto_send_type_requests(){
}

static void net_proto_send_id_requests(){
}

static void net_proto_send_linked_list_requests(){
}

void net_proto_handle_outbound_requests(){
	net_proto_fill_type_requests();
	net_proto_fill_id_requests();
	net_proto_fill_linked_list_requests();
	net_proto_send_type_requests();
	net_proto_send_id_requests();
	net_proto_send_linked_list_requests();
}
