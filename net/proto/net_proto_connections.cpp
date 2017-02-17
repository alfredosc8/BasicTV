#include "../../util.h"
#include "../../id/id.h"
#include "../../id/id_api.h"
#include "net_proto_connections.h"
#include "net_proto_con_req.h"
#include "net_proto_socket.h"
#include "../net_socket.h"

void net_proto_handle_tcp_holepunch(net_proto_con_req_t *con_req){
	const uint64_t timestamp =
		con_req->get_heartbeat_timestamp();
	const uint64_t cur_timestamp =
		get_time_microseconds();
	const uint64_t offset = 100000;
	if(BETWEEN(cur_timestamp-offset,
		   timestamp,
		   cur_timestamp+offset)){
		id_t_ second_id = ID_BLANK_ID;
		con_req->get_peer_ids(
			nullptr,
			&second_id,
			nullptr);
		net_proto_peer_t *peer_ptr =
			PTR_DATA_FAST(second_id,
				      net_proto_peer_t);
		if(peer_ptr == nullptr){
			print("cannot holepunch to a null peer", P_ERR);
		}
		const std::string peer_ip =
			peer_ptr->get_net_ip_str();
		const uint16_t peer_port =
			peer_ptr->get_net_port();
		net_socket_t *holepunch_socket =
			new net_socket_t;
		holepunch_socket->set_net_ip(
			peer_ip,
			peer_port);
		holepunch_socket->connect();
		print("add logic to finish connection", P_CRIT);
	}
}
