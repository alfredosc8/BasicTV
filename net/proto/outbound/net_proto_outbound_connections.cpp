#include "net_proto_outbound_connections.h"
#include "../net_proto_socket.h"
#include "../net_proto_connections.h"
#include "../net_proto_api.h"
#include "../../../id/id.h"
#include "../../../id/id_api.h"

#include <vector>

static void net_proto_initiate_direct_tcp(net_proto_con_req_t *con_req){
}

static void net_proto_first_id_logic(net_proto_con_req_t *con_req){
	switch(con_req->get_flags()){
	case (NET_CON_REQ_TCP | NET_CON_REQ_DIRECT):
		net_proto_initiate_direct_tcp(con_req);
		break;
	case (NET_CON_REQ_TCP | NET_CON_REQ_HOLEPUNCH):
		net_proto_handle_tcp_holepunch(con_req);
		break;
	default:
		break;
	}
}

void net_proto_initiate_all_connections(){
	std::vector<id_t_> proto_con_req_vector =
		id_api::cache::get(
			"net_proto_con_req_t");
	for(uint64_t i = 0;i < proto_con_req_vector.size();i++){
		net_proto_con_req_t *con_req =
			PTR_DATA_FAST(proto_con_req_vector[i],
				      net_proto_con_req_t);
		id_t_ first_id = ID_BLANK_ID;
		id_t_ second_id = ID_BLANK_ID;
		const id_t_ self_peer_id =
			net_proto::peer::get_self_as_peer();
		// third is unused (would be third party for UDP)
		con_req->get_peer_ids(
			&first_id,
			&second_id,
			nullptr);
		if(first_id == self_peer_id){
			net_proto_first_id_logic(con_req);
		}
		// second is always inbound, don't bother with that here
	}
}
