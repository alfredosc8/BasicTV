#include "../../util.h"
#include "../../id/id.h"
#include "../../id/id_api.h"
#include "net_proto_stat.h"
#include "net_proto_socket.h"
#include "net_proto_request.h"
#include "net_proto.h"

/*
  TODO: reimplement this in a way that makes sense
 */

// uint16_t net_proto_socket_t::get_prob_of_id(id_t_ id_){
// 	uint16_t highest_stat = 0;
// 	for(uint64_t i = 0;i < id_log.size();i++){
// 		const uint64_t distance =
// 			id_api::linked_list::distance_fast(
// 				id_log[i].second,
// 				id_);
// 		const uint16_t curr_stat =
// 			(1.0/(distance+1))*65535;
// 		if(unlikely(curr_stat > highest_stat)){
// 			highest_stat = curr_stat;
// 		}
// 	}
// 	// actually interpreted as x/65535
// 	P_V(highest_stat, P_SPAM);
// 	return highest_stat;
// }

std::vector<id_t_> net_proto::socket::stats::sort(std::vector<id_t_> socket_ids,
					    id_t_ request_id){
	// std::vector<std::pair<id_t_, uint16_t> > prob_of_id;
	// for(uint64_t i = 0;i < socket_ids.size();i++){
	// 	net_proto_socket_t *proto_socket =
	// 		PTR_ID(socket_ids[i],
	// 		       net_proto_socket_t);
	// 	if(proto_socket != nullptr){
	// 		prob_of_id.push_back(
	// 			std::make_pair(
	// 				socket_ids[i],
	// 				proto_socket->get_prob_of_id(
	// 					request_id)));
	// 	}
	// }
	// print("not implemented yet", P_CRIT);
	// return {};
}
