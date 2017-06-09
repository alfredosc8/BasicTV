#include "net_proto_interface.h"

void peer_sanity_check(id_t_ id, type_t_ type){
	if(get_id_type(id) != type){
		const type_t_ id_type =
			get_id_type(id);
		P_V(id_type, P_WARN);
		P_V(type, P_WARN);
		print("id does not match the requested type", P_ERR);
	}
}

net_proto_interface_state_t::net_proto_interface_state_t() : id(this, TYPE_NET_PROTO_INTERFACE_STATE_T){
	
}

net_proto_interface_state_t::~net_proto_interface_state_t(){
}
