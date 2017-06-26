#ifndef NET_PROTO_PEER_H
#define NET_PROTO_PEER_H
#include "../net_ip.h"

#define NET_PEER_WRONG_KEY (1 << 0)

/*
  Only one address_id is here, because the larger a net_proto_peer_t becomes,
  the harder it becomes to properly connect to thatp eer when some of the
  address IDs go offline (either completely delete the net_proto_peer_t,
  make a cache and keep a log (which can't be transferred between peers),
  or just have some bloat and sluggishness with stuff)
 */

struct net_proto_peer_t{
private:
	uint8_t flags = 0;
	id_t_ crypto_wallet_id = ID_BLANK_ID;
	id_t_ address_id = ID_BLANK_ID;

	// cache
	uint8_t local_flags = 0;
public:
	data_id_t id;
	net_proto_peer_t();
	~net_proto_peer_t();

	GET_SET_ID(address_id);
	GET_SET_ID(crypto_wallet_id);
	GET_SET(flags, uint8_t);
	GET_SET(local_flags, uint8_t);
};
#endif
