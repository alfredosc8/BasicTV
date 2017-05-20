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
std::vector<std::string> routine_request_fast_vector = {
	"net_proto_con_req_t"
};

// Self-serving (most of the time), lower priority
std::vector<std::string> routine_request_slow_vector = {
	"tv_channel_t",
	"net_proto_peer_t",
	"encrypt_pub_key_t"
};

static void net_proto_routine_request_fill(std::vector<std::string> type_vector,
					   uint64_t request_interval_micro_s,
					   uint64_t *last_request_time_micro_s){
	const uint64_t time_micro_s =
		get_time_microseconds();
	if(time_micro_s-(*last_request_time_micro_s) > request_interval_micro_s){
		print("sending routine type request to network", P_SPAM);
		for(uint64_t i = 0;i < type_vector.size();i++){
			std::vector<id_t_> id_vector =
				id_api::cache::get(
					type_vector[i]);
			std::vector<uint64_t> mod_vector =
				id_api::bulk_fetch::mod(
 					id_vector);
 			net_proto_type_request_t *type_request =
 				new net_proto_type_request_t;
 			type_request->update_type(
 				convert::type::to(
 					type_vector[i]));
 			type_request->set_receiver_peer_id(
 				net_proto::peer::get_self_as_peer());
			type_request->set_sender_peer_id(
				net_proto::peer::random_peer_id());
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
