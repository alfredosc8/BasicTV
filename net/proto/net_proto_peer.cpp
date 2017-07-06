#include "net_proto.h"
#include "net_proto_peer.h"

net_proto_peer_t::net_proto_peer_t() : id(this, TYPE_NET_PROTO_PEER_T){
	id.add_data_raw(&(flags), 1);
	id.add_data_id(&crypto_wallet_id, 1);
	id.add_data_id(&address_id, 1);
	id.set_lowest_global_flag_level(
		ID_DATA_NETWORK_RULE_PUBLIC,
		ID_DATA_EXPORT_RULE_ALWAYS,
		ID_DATA_PEER_RULE_ALWAYS);
}

net_proto_peer_t::~net_proto_peer_t(){
}
