#include "net_proto_inbound_data.h"
// meta has apply and unapply functions for DEV_CTRL_1
#include "../net_proto_meta.h"
#include "../net_proto_socket.h"
#include "../net_proto_request.h"
#include "../../../util.h"
#include "../../../id/id_api.h"
#include "../net_proto_api.h"

void net_proto_handle_inbound_data(){
	std::vector<id_t_> proto_sockets =
		id_api::cache::get("net_proto_socket_t");
	for(uint64_t i = 0;i < proto_sockets.size();i++){
		net_proto_socket_t *proto_socket =
			PTR_DATA(proto_sockets[i],
				 net_proto_socket_t);
		if(proto_socket == nullptr){
			print("proto_socket is a nullptr", P_WARN);
		}
		try{
			proto_socket->update();
		}catch(...){
			print("peer has disconnected, deleting proto_socket", P_DEBUG);
			id_api::destroy(proto_socket->id.get_id());
			proto_socket = nullptr;
		}
	}
}
