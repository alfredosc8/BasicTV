#include "net_proto_routine_requests.h"
#include "net_proto_socket.h"
#include "net_proto_request.h"
#include "net_proto_peer.h"
#include "net_proto.h"
#include "../../id/id_api.h"
#include "../../settings.h"

static uint64_t last_request_fast_time_micro_s = 0;
static uint64_t last_request_slow_time_micro_s = 0;

// Interdependent upon other nodes, has a higher priority
std::vector<type_t_> routine_request_fast_vector = {
	TYPE_NET_PROTO_CON_REQ_T
};

std::vector<type_t_> routine_request_slow_vector = {
	TYPE_TV_CHANNEL_T,
	TYPE_NET_PROTO_PEER_T,
	TYPE_ENCRYPT_PUB_KEY_T
};

static void net_proto_routine_request_fill(std::vector<type_t_> type_vector,
					   uint64_t request_interval_micro_s,
					   uint64_t *last_request_time_micro_s){
	const uint64_t time_micro_s =
		get_time_microseconds();
	if(time_micro_s-(*last_request_time_micro_s) > request_interval_micro_s){
		for(uint64_t i = 0;i < type_vector.size();i++){
			// all request names are in the perspective of the
			// sender, not the receiver. This makes the receiver
			// the person we send it to, and the sender ourselves.
			id_t_ recv_peer_id =
				net_proto::peer::random_peer_id();
			if(recv_peer_id == ID_BLANK_ID){
				print("we have no other peer information whatsoever, not creating any network requests", P_DEBUG);
			}else{
				std::vector<id_t_> id_vector =
					id_api::cache::get(
						type_vector[i]);
				std::vector<uint64_t> mod_vector =
					id_api::bulk_fetch::mod(
						id_vector);
				net_proto_type_request_t *type_request =
					new net_proto_type_request_t;
				type_request->update_type(
						type_vector[i]);
				type_request->set_receiver_peer_id(
					net_proto::peer::random_peer_id());
				type_request->set_sender_peer_id(
					net_proto::peer::get_self_as_peer());
			}
 		}
		*last_request_time_micro_s = time_micro_s;
 	}
}

void net_proto_routine_requests_loop(){
 	net_proto_routine_request_fill(
 		routine_request_fast_vector,
		settings::get_setting_unsigned_def(
			"net_proto_routine_request_fast_interval_micro_s",
			NET_PROTO_ROUTINE_REQUEST_DEFAULT_FAST_INTERVAL),
 		&last_request_fast_time_micro_s);
 	net_proto_routine_request_fill(
 		routine_request_slow_vector,
		settings::get_setting_unsigned_def(
			"net_proto_routine_request_slow_interval_micro_s",
			NET_PROTO_ROUTINE_REQUEST_DEFAULT_SLOW_INTERVAL),
 		&last_request_slow_time_micro_s);
}
