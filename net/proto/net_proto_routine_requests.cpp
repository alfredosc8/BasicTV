#include "net_proto_routine_requests.h"
#include "net_proto_socket.h"
#include "net_proto_request.h"
#include "net_proto_peer.h"
#include "../../id/id_api.h"
#include "../../settings.h"

static uint64_t last_request_fast_time_micro_s = 0;
static uint64_t last_request_slow_time_micro_s = 0;

// Interdependent upon other nodes, has a higher priority
std::vector<std::string> routine_request_fast_vector = {
	"net_con_req_t"
};

// Self-serving (most of the time), lower priority
std::vector<std::string> routine_request_slow_vector = {
	"tv_channel_t",
	"net_peer_t",
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
			net_proto_request_t *type_request =
				new net_proto_request_t;
			type_request->set_type(
				convert::array::type::to(
					type_vector[i]));
			type_request->set_flags(
				NET_REQUEST_BLACKLIST);
			type_request->set_ids(
			        id_vector);
			// mod_inc is handled inn set_ids
			type_request->set_proto_socket_id(
				ID_BLANK_ID);
			// net_request_ts with no bound sockets are assumed to
			// be outbound, which will always be true
		}
	}
	*last_request_time_micro_s = time_micro_s;
}

void net_proto_routine_requests_loop(){
	uint64_t request_interval_fast_micro_s,
		request_interval_slow_micro_s;
	try{
		request_interval_fast_micro_s =
			std::stoull(
				settings::get_setting(
					"net_proto_routine_request_fast_interval_micro_s"));
	}catch(...){
		request_interval_fast_micro_s = NET_PROTO_ROUTINE_REQUEST_DEFAULT_FAST_INTERVAL;
	}
	try{
		request_interval_slow_micro_s =
			std::stoull(
				settings::get_setting(
					"net_proto_routine_request_slow_interval_micro_s"));
	}catch(...){
		request_interval_slow_micro_s = NET_PROTO_ROUTINE_REQUEST_DEFAULT_SLOW_INTERVAL;
	}
	net_proto_routine_request_fill(
		routine_request_fast_vector,
		request_interval_fast_micro_s,
		&last_request_fast_time_micro_s);
	net_proto_routine_request_fill(
		routine_request_slow_vector,
		request_interval_slow_micro_s,
		&last_request_slow_time_micro_s);
	
}
